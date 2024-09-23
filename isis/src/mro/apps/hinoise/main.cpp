/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include <iostream>
#include <QString>
#include <stdio.h>
#include "CSVReader.h"
#include "FileName.h"
#include "IException.h"
#include "Pipeline.h"
#include <QList>

using namespace std;
using namespace Isis;

// For Debugging
//#define _DEBUG_

vector <QString> sTempFiles;

// Global values
int giLpfLines, giLpfSamples, giLpfMinPer;

// Get Highpass Filter values
int giHpfLines, giHpfSamples, giHpfMinPer;

// Noise Filter values
bool gbNullColumns;
int  giNoiseLines, giNoiseSamples;
double gdTolMin, gdTolMax;
double gdFlatTol, gdMinValue;
double gdHard_TolMin, gdHard_TolMax;
double gdLpfzLines, gdLpfzSamples;
double gdClearFrac, gdNonValidFrac;
double gdHardFilter, gdHighEndPercent, gdHardHighEndPercent;

// Create temporary file names
QString gsCubeStats1, gsCubeStats2;
QString gsTempFile;

void GetValuesFromHistogram(QString psHistFile, double & pdLisPer, double & pdMaxDN, double & pdStdDev);
void ProcessCubeNormStats(QString psStatsFile, int piChannel, int piSumming);

void IsisMain() {
  try {
    UserInterface &ui = Application::GetUserInterface();

    QString sInputFile  = ui.GetAsString("FROM");
    QString sOutputFile = ui.GetAsString("TO");
    QString sInBaseName = QString::fromStdString(FileName(sInputFile.toStdString()).baseName());

    bool bRemove = ui.GetBoolean("REMOVE");

    // Get Lowpass Filter Values
    giLpfLines   = ui.GetInteger("LPF_LINES");
    giLpfSamples = ui.GetInteger("LPF_SAMPLES");
    giLpfMinPer  = ui.GetInteger("LPF_MINPER");

    // Get Highpass Filter values
    giHpfLines   = ui.GetInteger("HPF_LINES");
    giHpfSamples = ui.GetInteger("HPF_SAMPLES");
    giHpfMinPer  = ui.GetInteger("HPF_MINPER");

    // Noise Filter values
    gbNullColumns        = ui.GetBoolean("NULL_COLUMNS");
    gdTolMin             = ui.GetDouble("TOLMIN");
    gdTolMax             = ui.GetDouble("TOLMAX");
    gdFlatTol            = ui.GetDouble("FLATTOL");
    gdMinValue           = ui.GetDouble("MIN_VALUE");
    gdHard_TolMin        = ui.GetDouble("HARD_TOLMIN");
    gdHard_TolMax        = ui.GetDouble("HARD_TOLMAX");
    gdLpfzLines          = ui.GetInteger("LPFZ_LINES");
    gdLpfzSamples        = ui.GetInteger("LPFZ_SAMPLES");
    giNoiseLines         = ui.GetInteger("NOISE_LINES");
    giNoiseSamples       = ui.GetInteger("NOISE_SAMPLES");
    gdClearFrac          = ui.GetDouble("CLEAR_FRACTION");
    gdNonValidFrac       = ui.GetDouble("NONVALID_FRACTION");
    gdHardFilter         = ui.GetDouble("HARD_FILTERING");
    gdHighEndPercent     = ui.GetDouble("HIGHEND_PERCENT");
    gdHardHighEndPercent = ui.GetDouble("HARD_HIGHEND_PERCENT");

    // Get Summing, CcdId and Channel Number from the cube label
    Pvl cubeLabel = Pvl(sInputFile.toStdString());
    int iSumming  = Isis::toInt(cubeLabel.findObject("IsisCube").findGroup("Instrument").findKeyword("Summing")[0]);
    int iChannel  = Isis::toInt(cubeLabel.findObject("IsisCube").findGroup("Instrument").findKeyword("ChannelNumber")[0]);
    QString sCcdId = QString::fromStdString(cubeLabel.findObject("IsisCube").findGroup("Instrument").findKeyword("CcdId"));

    // Get the image histogram
    Pipeline p1("hinoise1");
    p1.SetInputFile("FROM");
    QString sTempHistFile = "$TEMPORARY/" + sInBaseName + "_hist.txt";
    p1.SetOutputFile(FileName(sTempHistFile.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempHistFile.toStdString()).expanded()));
    p1.KeepTemporaryFiles(!bRemove);

    p1.AddToPipeline("hist");
    p1.Application("hist").SetInputParameter("FROM",  false);
    p1.Application("hist").SetOutputParameter("TO",   "hist");
  #ifdef _DEBUG_
    cout << p1 << endl;
    cout << "****************************************************************************\n";
  #endif
    p1.Run();

    double dLisPer, dMaxDN, dStdDev;
    GetValuesFromHistogram(QString::fromStdString(FileName(sTempHistFile.toStdString()).expanded()), dLisPer, dMaxDN, dStdDev);
  #ifdef _DEBUG_
    cerr << "Lis=" << dLisPer << "  MaxDN=" << dMaxDN << " StdDev=" << dStdDev << endl;
  #endif

    Pipeline p2("hinoise2");
    p2.SetInputFile("FROM");
    QString sTempFile2 = "$TEMPORARY/" + sInBaseName + "_cubenorm.txt";
    p2.SetOutputFile(FileName(sTempFile2.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile2.toStdString()).expanded()));
    p2.KeepTemporaryFiles(!bRemove);

    p2.AddToPipeline("cubenorm");
    p2.Application("cubenorm").SetInputParameter("FROM",      false);
    p2.Application("cubenorm").SetOutputParameter("STATS",    "cubenorm");
    p2.Application("cubenorm").AddConstParameter("FORMAT",    "TABLE");
    p2.Application("cubenorm").AddConstParameter("DIRECTION", "COLUMN");
  #ifdef _DEBUG_
    cout << p2 << endl;
    cout << "****************************************************************************\n";
  #endif
    p2.Run();

    gsCubeStats1 = QString::fromStdString(FileName("$TEMPORARY/" + sInBaseName.toStdString() + "_cubenorm1.txt").expanded());
    gsCubeStats2 = QString::fromStdString(FileName("$TEMPORARY/" + sInBaseName.toStdString() + "_cubenorm2.txt").expanded());
  #ifdef _DEBUG_
    cerr << gsCubeStats1 << "  " << gsCubeStats2 << endl;
  #endif
    sTempFiles.push_back(gsCubeStats1);
    sTempFiles.push_back(gsCubeStats2);

    ProcessCubeNormStats(QString::fromStdString(FileName(sTempFile2.toStdString()).expanded()), iChannel, iSumming);

    // Clear the bad colmns for the highpass filter
    Pipeline p3("hinoise3");
    p3.SetInputFile("FROM");
    QString sTempFile3 = "$TEMPORARY/" + sInBaseName + "_Temp_p3_out.cub";
    p3.SetOutputFile(FileName(sTempFile3.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile3.toStdString()).expanded()));
    p3.KeepTemporaryFiles(!bRemove);
  #ifdef _DEBUG_
    cerr << "stats1=" << gsCubeStats1 << endl;
  #endif
    p3.AddToPipeline("cubenorm");
    p3.Application("cubenorm").SetInputParameter ("FROM",      false);
    p3.Application("cubenorm").SetOutputParameter("TO",        "cubenorm.p3");
    p3.Application("cubenorm").AddConstParameter ("FROMSTATS", gsCubeStats1);
    p3.Application("cubenorm").AddConstParameter ("STATSOURCE","TABLE");
    p3.Application("cubenorm").AddConstParameter ("MODE",      "DIVIDE");
    p3.Application("cubenorm").AddConstParameter ("NORMALIZER","AVERAGE");
    p3.Application("cubenorm").AddConstParameter ("PRESERVE",  "FALSE");
  #ifdef _DEBUG_
    cout << p3 << endl;
    cout << "****************************************************************************\n";
  #endif
    p3.Run();

    // Clear the bad colmns for the lowpass filter
    Pipeline p4("hinoise4");
    p4.SetInputFile("FROM");
    QString sTempFile4 = "$TEMPORARY/" + sInBaseName + "_Temp_p4_out.cub";
    p4.SetOutputFile(FileName(sTempFile4.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile4.toStdString()).expanded()));
    p4.KeepTemporaryFiles(!bRemove);

    p4.AddToPipeline("cubenorm");
    p4.Application("cubenorm").SetInputParameter ("FROM",      false);
    p4.Application("cubenorm").SetOutputParameter("TO",        "cubenorm.p4");
    p4.Application("cubenorm").AddConstParameter ("FROMSTATS", gsCubeStats2);
    p4.Application("cubenorm").AddConstParameter ("STATSOURCE","TABLE");
    p4.Application("cubenorm").AddConstParameter ("MODE",      "DIVIDE");
    p4.Application("cubenorm").AddConstParameter ("NORMALIZER","AVERAGE");
    p4.Application("cubenorm").AddConstParameter ("PRESERVE",  "FALSE");
  #ifdef _DEBUG_
    cout << p4 << endl;
    cout << "****************************************************************************\n";
  #endif
    p4.Run();

    // ****************************************************************************
    // Perform highpass/lowpass filter vertical destripping
    // ****************************************************************************
    // a. Lowpass
    Pipeline p5("hinoise5");
    QString sTempFile5 = "$TEMPORARY/" + sInBaseName + "_Temp_p5_out.cub";
    p5.SetInputFile(FileName(sTempFile4.toStdString()));
    p5.SetOutputFile(FileName(sTempFile5.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile5.toStdString()).expanded()));
    p5.KeepTemporaryFiles(!bRemove);

    p5.AddToPipeline("lowpass");
    p5.Application("lowpass").SetInputParameter ("FROM",    false);
    p5.Application("lowpass").SetOutputParameter("TO",      "lowpass.p5");
    p5.Application("lowpass").AddConstParameter ("SAMPLES", QString::number(giLpfSamples));
    p5.Application("lowpass").AddConstParameter ("LINES",   QString::number(giLpfLines));
    p5.Application("lowpass").AddConstParameter ("MINOPT",  "PERCENT");
    p5.Application("lowpass").AddConstParameter ("LIS",     "FALSE");
    p5.Application("lowpass").AddConstParameter ("MINIMUM", QString::number(giLpfMinPer));
    p5.Application("lowpass").AddConstParameter ("REPLACE", "NULL");
  #ifdef _DEBUG_
    cout << p5 << endl;
    cout << "****************************************************************************\n";
  #endif
    p5.Run();

    // b. Highpass
    Pipeline p6("hinoise6");
    p6.SetInputFile (FileName(sTempFile3.toStdString()));
    QString sTempFile6 = "$TEMPORARY/" + sInBaseName + "_Temp_p6_out.cub";
    p6.SetOutputFile(FileName(sTempFile6.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile6.toStdString()).expanded()));
    p6.KeepTemporaryFiles(!bRemove);

    p6.AddToPipeline("highpass");
    p6.Application("highpass").SetInputParameter ("FROM",    false);
    p6.Application("highpass").SetOutputParameter("TO",      "highpass.p6");
    p6.Application("highpass").AddConstParameter ("SAMPLES", QString::number(giHpfSamples));
    p6.Application("highpass").AddConstParameter ("LINES",   QString::number(giHpfLines));
    p6.Application("highpass").AddConstParameter ("MINIMUM", QString::number(giHpfMinPer));
    p6.Application("highpass").AddConstParameter ("MINOPT", "PERCENT");
  #ifdef _DEBUG_
    cout << p6 << endl;
    cout << "****************************************************************************\n";
  #endif
    p6.Run();

    // Enter the outputs of lowpass and highpass filenames to a list file
    QString sTempListFile = "$TEMPORARY/" + sInBaseName + "_TempList.lis";
    gsTempFile = QString::fromStdString(FileName(sTempListFile.toStdString()).expanded());
    sTempFiles.push_back(gsTempFile);
    fstream ostm;
    ostm.open(gsTempFile.toLatin1().data(), std::ios::out);
    ostm << FileName(sTempFile5.toStdString()).expanded() << endl;
    ostm << FileName(sTempFile6.toStdString()).expanded() << endl;
    ostm.close();

    // c. algebra (lowpass + highpass)
    Pipeline p7("hinoise7");
    p7.SetInputFile (FileName(sTempListFile.toStdString()));
    QString sTempFile7 = "$TEMPORARY/" + sInBaseName + "_Temp_p7_out.cub";
    p7.SetOutputFile(FileName(sTempFile7.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile7.toStdString()).expanded()));
    p7.KeepTemporaryFiles(!bRemove);

    p7.AddToPipeline("fx");
    p7.Application("fx").SetInputParameter ("FROMLIST", false);
    p7.Application("fx").SetOutputParameter("TO",       "add.p7");
    p7.Application("fx").AddConstParameter ("MODE",     "LIST");
    p7.Application("fx").AddConstParameter ("EQUATION", "f1+f2");
  #ifdef _DEBUG_
    cout << p7 << endl;
    cout << "****************************************************************************\n";
  #endif
    p7.Run();

    remove(gsTempFile.toLatin1().data());

    // ****************************************************************************
    // Perform noise filter 3 times
    // ****************************************************************************
    Pipeline p8("hinoise8");
    p8.SetInputFile (FileName(sTempFile7.toStdString()));
    QString sTempFile8 = "$TEMPORARY/" + sInBaseName + "_Temp_p8_out.cub";
    p8.SetOutputFile(FileName(sTempFile8.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile8.toStdString()).expanded()));
    p8.KeepTemporaryFiles(!bRemove);

    if (dLisPer >= gdHardFilter) {
      gdTolMin = gdHard_TolMin;
      gdTolMax = gdHard_TolMax;
    }
    gdFlatTol = dStdDev * gdFlatTol;
    if (gdFlatTol < 0.00001) {
      gdFlatTol = 0.00001;
    }

    // Perform the 1st noise filter
    p8.AddToPipeline("noisefilter", "noisefilter_pass1");
    p8.Application("noisefilter_pass1").SetInputParameter ("FROM",    false);
    p8.Application("noisefilter_pass1").SetOutputParameter("TO",      "noisefilter.1");
    p8.Application("noisefilter_pass1").AddConstParameter ("FLATTOL", QString::number(gdFlatTol));
    p8.Application("noisefilter_pass1").AddConstParameter ("TOLDEF",  "STDDEV");
    p8.Application("noisefilter_pass1").AddConstParameter ("LOW",     QString::number(gdMinValue));
    p8.Application("noisefilter_pass1").AddConstParameter ("HIGH",    QString::number(dMaxDN));
    p8.Application("noisefilter_pass1").AddConstParameter ("TOLMIN",  QString::number(gdTolMin));
    p8.Application("noisefilter_pass1").AddConstParameter ("TOLMAX",  QString::number(gdTolMax));
    p8.Application("noisefilter_pass1").AddConstParameter ("REPLACE", "NULL");
    p8.Application("noisefilter_pass1").AddConstParameter ("SAMPLE",  QString::number(giNoiseSamples));
    p8.Application("noisefilter_pass1").AddConstParameter ("LINE",    QString::number(giNoiseLines));
    p8.Application("noisefilter_pass1").AddConstParameter ("LISISNOISE", "TRUE");
    p8.Application("noisefilter_pass1").AddConstParameter ("LRSISNOISE", "TRUE");

    // Perform the 2nd noise filter
    p8.AddToPipeline("noisefilter", "noisefilter_pass2");
    p8.Application("noisefilter_pass2").SetInputParameter ("FROM",    false);
    p8.Application("noisefilter_pass2").SetOutputParameter("TO",      "noisefilter.2");
    p8.Application("noisefilter_pass2").AddConstParameter ("FLATTOL", QString::number(gdFlatTol));
    p8.Application("noisefilter_pass2").AddConstParameter ("TOLDEF",  "STDDEV");
    p8.Application("noisefilter_pass2").AddConstParameter ("LOW",     QString::number(gdMinValue));
    p8.Application("noisefilter_pass2").AddConstParameter ("HIGH",    QString::number(dMaxDN));
    p8.Application("noisefilter_pass2").AddConstParameter ("TOLMIN",  QString::number(gdTolMin));
    p8.Application("noisefilter_pass2").AddConstParameter ("TOLMAX",  QString::number(gdTolMax));
    p8.Application("noisefilter_pass2").AddConstParameter ("REPLACE", "NULL");
    p8.Application("noisefilter_pass2").AddConstParameter ("SAMPLE",  QString::number(giNoiseSamples));
    p8.Application("noisefilter_pass2").AddConstParameter ("LINE",    QString::number(giNoiseLines));
    p8.Application("noisefilter_pass2").AddConstParameter ("LISISNOISE", "TRUE");
    p8.Application("noisefilter_pass2").AddConstParameter ("LRSISNOISE", "TRUE");

    // Perform the 3rd noise filter
    p8.AddToPipeline("noisefilter", "noisefilter_pass3");
    p8.Application("noisefilter_pass3").SetInputParameter ("FROM",    false);
    p8.Application("noisefilter_pass3").SetOutputParameter("TO",      "noisefilter.3");
    p8.Application("noisefilter_pass3").AddConstParameter ("FLATTOL", QString::number(gdFlatTol));
    p8.Application("noisefilter_pass3").AddConstParameter ("TOLDEF",  "STDDEV");
    p8.Application("noisefilter_pass3").AddConstParameter ("LOW",     QString::number(gdMinValue));
    p8.Application("noisefilter_pass3").AddConstParameter ("HIGH",    QString::number(dMaxDN));
    p8.Application("noisefilter_pass3").AddConstParameter ("TOLMIN",  QString::number(gdTolMin));
    p8.Application("noisefilter_pass3").AddConstParameter ("TOLMAX",  QString::number(gdTolMax));
    p8.Application("noisefilter_pass3").AddConstParameter ("REPLACE", "NULL");
    p8.Application("noisefilter_pass3").AddConstParameter ("SAMPLE",  QString::number(giNoiseSamples));
    p8.Application("noisefilter_pass3").AddConstParameter ("LINE",    QString::number(giNoiseLines));
    p8.Application("noisefilter_pass3").AddConstParameter ("LISISNOISE", "TRUE");
    p8.Application("noisefilter_pass3").AddConstParameter ("LRSISNOISE", "TRUE");
  #ifdef _DEBUG_
    cout << p8 << endl;
    cout << "****************************************************************************\n";
  #endif
    p8.Run();

    // ****************************************************************************
    // Perform another highpass /lowpass filter now that the
    // data are much cleaner
    // ****************************************************************************
    // a. Lowpass
    Pipeline p9("hinoise9");
    p9.SetInputFile (FileName(sTempFile8.toStdString()));
    QString sTempFile9 = "$TEMPORARY/" + sInBaseName + "_Temp_p9_out.cub";
    p9.SetOutputFile(FileName(sTempFile9.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile9.toStdString()).expanded()));
    p9.KeepTemporaryFiles(!bRemove);

    p9.AddToPipeline("lowpass");
    p9.Application("lowpass").SetInputParameter ("FROM",    false);
    p9.Application("lowpass").SetOutputParameter("TO",      "lowpass.p9");
    p9.Application("lowpass").AddConstParameter ("SAMPLES", QString::number(giLpfSamples));
    p9.Application("lowpass").AddConstParameter ("LINES",   QString::number(giLpfLines));
    p9.Application("lowpass").AddConstParameter ("MINOPT",  "PERCENT");
    p9.Application("lowpass").AddConstParameter ("MINIMUM", QString::number(giLpfMinPer));
    p9.Application("lowpass").AddConstParameter ("REPLACE", "NULL");
    p9.Application("lowpass").AddConstParameter ("NULL",    "FALSE");
    p9.Application("lowpass").AddConstParameter ("HRS",     "FALSE");
    p9.Application("lowpass").AddConstParameter ("HIS",     "FALSE");
    p9.Application("lowpass").AddConstParameter ("LRS",     "FALSE");
    p9.Application("lowpass").AddConstParameter ("LIS",     "FALSE");
  #ifdef _DEBUG_
    cout << p9 << endl;
    cout << "****************************************************************************\n";
  #endif
    p9.Run();

    // b. Highpass
    Pipeline p10("hinoise10");
    p10.SetInputFile (FileName(sTempFile8.toStdString()));
    QString sTempFile10 = "$TEMPORARY/" + sInBaseName + "_Temp_p10_out.cub";
    p10.SetOutputFile(FileName(sTempFile10.toStdString()));
    sTempFiles.push_back(QString::fromStdString(FileName(sTempFile10.toStdString()).expanded()));
    p10.KeepTemporaryFiles(!bRemove);

    p10.AddToPipeline("highpass");
    p10.Application("highpass").SetInputParameter ("FROM",    false);
    p10.Application("highpass").SetOutputParameter("TO",      "highpass.p10");
    p10.Application("highpass").AddConstParameter ("SAMPLES", QString::number(giHpfSamples));
    p10.Application("highpass").AddConstParameter ("LINES",   QString::number(giHpfLines));
    p10.Application("highpass").AddConstParameter ("MINIMUM", QString::number(giHpfMinPer));
    p10.Application("highpass").AddConstParameter ("MINOPT", "PERCENT");
  #ifdef _DEBUG_
    cout << p10 << endl;
    cout << "****************************************************************************\n";
  #endif
    p10.Run();

    // Enter the outputs of lowpass and highpass filenames to a list file
    gsTempFile = QString::fromStdString(FileName(sTempListFile.toStdString()).expanded());
    sTempFiles.push_back(gsTempFile);
    ostm.open(gsTempFile.toLatin1().data(), std::ios::out);
    ostm << FileName(sTempFile9.toStdString()).expanded() << endl;
    ostm << FileName(sTempFile10.toStdString()).expanded() << endl;
    ostm.close();

    // c. algebra (lowpass + highpass)
    Pipeline p11("hinoise11");
    p11.SetInputFile (FileName(sTempListFile.toStdString()));
    QString sTempFile11;
    if (sCcdId == "RED") {
      sTempFile11 = "$TEMPORARY/" + sInBaseName + "_Temp_p11_out.cub";
      p11.SetOutputFile(FileName(sTempFile11.toStdString()));
      sTempFiles.push_back(QString::fromStdString(FileName(sTempFile11.toStdString()).expanded()));
    }
    else {
      p11.SetOutputFile("TO");
    }
    p11.KeepTemporaryFiles(!bRemove);

    p11.AddToPipeline("fx");
    p11.Application("fx").SetInputParameter ("FROMLIST", false);
    p11.Application("fx").SetOutputParameter("TO",       "add.p11");
    p11.Application("fx").AddConstParameter ("MODE",     "LIST");
    p11.Application("fx").AddConstParameter ("EQUATION", "f1+f2");
  #ifdef _DEBUG_
    cout << p11 << endl;
    cout << "****************************************************************************\n";
  #endif
    p11.Run();

    // ****************************************************************************
    // Perform LPFZ  filters if we have a RED filter image.
    // For IR and BG filter data, assume that the HiColorNorm pipeline
    // step will interpolate using the BG/RED and IR/RED ratio data.
    // ****************************************************************************
    if (sCcdId == "RED") {
      int iMin = int( (gdLpfzLines * gdLpfzSamples)/3 );
      Pipeline p12("hinoise12");
      p12.SetInputFile(FileName(sTempFile11.toStdString()));
      p12.SetOutputFile("TO");
      p12.KeepTemporaryFiles(!bRemove);

      p12.AddToPipeline("lowpass", "lowpass_pass1");
      p12.Application("lowpass_pass1").SetInputParameter ("FROM",    false);
      p12.Application("lowpass_pass1").SetOutputParameter("TO",      "lowpass.p12.1");
      p12.Application("lowpass_pass1").AddConstParameter ("SAMPLES", "3");
      p12.Application("lowpass_pass1").AddConstParameter ("LINES",   "3");

      p12.Application("lowpass_pass1").AddConstParameter ("MINOPT",  "COUNT");
      p12.Application("lowpass_pass1").AddConstParameter ("MINIMUM", "1");
      p12.Application("lowpass_pass1").AddConstParameter ("FILTER",  "OUTSIDE");
      p12.Application("lowpass_pass1").AddConstParameter ("NULL",    "TRUE");
      p12.Application("lowpass_pass1").AddConstParameter ("HRS",     "FALSE");
      p12.Application("lowpass_pass1").AddConstParameter ("HIS",     "TRUE");
      p12.Application("lowpass_pass1").AddConstParameter ("LRS",     "TRUE");
      p12.Application("lowpass_pass1").AddConstParameter ("LIS",     "TRUE");

      p12.AddToPipeline("lowpass", "lowpass_pass2");
      p12.Application("lowpass_pass2").SetInputParameter ("FROM",    false);
      p12.Application("lowpass_pass2").SetOutputParameter("TO",      "lowpass.p12.2");
      p12.Application("lowpass_pass2").AddConstParameter ("SAMPLES", QString::number(gdLpfzSamples));
      p12.Application("lowpass_pass2").AddConstParameter ("LINES",   QString::number(gdLpfzLines));
      p12.Application("lowpass_pass2").AddConstParameter ("MINOPT",  "COUNT");
      p12.Application("lowpass_pass2").AddConstParameter ("MINIMUM", QString::number(iMin));
      p12.Application("lowpass_pass2").AddConstParameter ("FILTER",  "OUTSIDE");
      p12.Application("lowpass_pass2").AddConstParameter ("NULL",    "TRUE");
      p12.Application("lowpass_pass2").AddConstParameter ("HRS",     "FALSE");
      p12.Application("lowpass_pass2").AddConstParameter ("HIS",     "TRUE");
      p12.Application("lowpass_pass2").AddConstParameter ("LRS",     "TRUE");
      p12.Application("lowpass_pass2").AddConstParameter ("LIS",     "TRUE");
  #ifdef _DEBUG_
      cout << p12 << endl;
      cout << "****************************************************************************\n";
  #endif
      p12.Run();
    }

    // more clean up
    for (int i=0; i<(int)sTempFiles.size(); i++) {
      remove(sTempFiles[i].toLatin1().data());
    }
    sTempFiles.clear();
  }
  catch(IException &) {
    throw;
  }
  catch(std::exception const &se) {
    std::string sErrMsg = "std::exception: " + (std::string)se.what();
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }
  catch(...) {
    std::string sErrMsg = "Other Error";
    throw IException(IException::User, sErrMsg, _FILEINFO_);
  }
}

/**
 * Process/Filter cubenorm stats
 *
 * @author Sharmila Prasad (2/7/2011)
 *
 * @param psStatsFile - input cubenorm stats
 * @param piChannel   - Channel Number 0/1
 * @param piSumming   - Summing mode
 */
void ProcessCubeNormStats(QString psStatsFile, int piChannel, int piSumming)
{
  CSVReader::CSVAxis csvArr;
  CSVReader statsFile(psStatsFile, false, 0, ' ', false, true);
  int iRows = statsFile.rows();
  int iMaxValidPoints = 1;
  vector<double> dNorm1, dNorm2;
  vector<int> iValidPoints, iBand, iRowCol;
  QString sHeader="      ";

#ifdef _DEBUG_
  cerr << "Rows="<< iRows << endl;
#endif
  for (int i=0; i<iRows; i++) {
    csvArr = statsFile.getRow(i);
    if (i) {
      iBand.push_back(csvArr[0].toInt());
      iRowCol.push_back(csvArr[1].toInt());
      int iPoints = csvArr[2].toInt();
      iValidPoints.push_back(iPoints);
      if (iPoints > iMaxValidPoints) {
        iMaxValidPoints = iPoints;
      }
    }
    else {
      for (int j=0; j<csvArr.dim(); j++) {
        sHeader +=  csvArr[j] + "   " ;
      }
      sHeader += "\n";
    }
  }

  // Disregard the header
  iRows--;
  for (int i=0; i<iRows; i++) {
    dNorm1.push_back(1.0);
    dNorm2.push_back(1.0);
    double dFrac = iValidPoints[i]/iMaxValidPoints;
    if (dFrac < gdClearFrac && gbNullColumns ) {
      dNorm1[i] = 0.0;
      dNorm2[i] = 0.0;
    }
  }

  // Determine if the pause point pixels need to be zapped
  if (piSumming == 1) {
    QList<int> iChannelPause, iChannelWidth;
    QString sChDirection;

    if (piChannel == 0 ) {
      iChannelPause << 1 << 252 << 515 << 778; // Channel 0 pause point sample locations (1st pixel = index 1)
      iChannelWidth << 3 << 6 << 6 << 6;       // Number of pixels to cut from pause point
      sChDirection = "RIGHT";                  // Direction of cut
    }
    else {
      iChannelPause << 247 << 510 << 773 << 1024;  // Channel 1 pause point sample locations (1st pixel = index 1)
      iChannelWidth << 8 << 7 << 6 << 3;           // Number of pixels to cut from pause point
      sChDirection = "LEFT";
    }

    bool bNoiseTrigger = false;
    for (int ip=0; ip<iChannelPause.size(); ip++) {
      int i1=0, i2=0;
      if (sChDirection == "LEFT") {
        i1 = iChannelPause[ip] - iChannelWidth[ip];
        i2 = iChannelPause[ip] - 1;
      }
      else {
        i1 = iChannelPause[ip] - 1;
        i2 = iChannelPause[ip] + iChannelWidth[ip] - 2;
      }
      if (i1 < 0){
        i1 = 0;
      }
      if (i2 >= iRows){
        i2 = iRows-1;
      }

      for (int i=i1; i<=i2; i++){
        if (iValidPoints[i]/iMaxValidPoints < gdNonValidFrac) {
          bNoiseTrigger = true;
        }
      }
    }

    for (int ip=0; ip<iChannelPause.size(); ip++) {
      int i1, i2;
      if (sChDirection == "LEFT") {
        i1 = iChannelPause[ip] - iChannelWidth[ip];
        i2 = iChannelPause[ip] - 1;
      }
      else {
        i1 = iChannelPause[ip] - 1;
        i2 = iChannelPause[ip] + iChannelWidth[ip] - 2;
      }
      if (i1 < 0){
        i1 = 0;
      }
      if (i2 >= iRows){
        i2 = iRows-1;
      }
      for (int i=i1; i<=i2; i++) {
        if (bNoiseTrigger) {
          dNorm1[i] = 0;
        }
        dNorm2[i] = 0.0;
      }
    }
  }

  // Write the results of the filtered cubenorm data into 2 output files
  FILE * file1, *file2;
  file1 = fopen(gsCubeStats1.toLatin1().data(), "w");
  file2 = fopen(gsCubeStats2.toLatin1().data(), "w");

  fprintf (file1, "%8s%8s%15s%15s%15s%15s%15s%15s\n","Band", "RowCol", "ValidPoints",
           "Average", "Median", "StdDev", "Minimum", "Maximim");
  fprintf (file2, "%8s%8s%15s%15s%15s%15s%15s%15s\n","Band", "RowCol", "ValidPoints",
           "Average", "Median", "StdDev", "Minimum", "Maximim");
  for (int i=0; i<iRows; i++) {
    fprintf (file1, "%8d%8d%15d%15.3f%15.3f%15.3f%15.3f%15.3f\n",
             iBand[i], iRowCol[i], iValidPoints[i], dNorm1[i], dNorm1[i], dNorm1[i], dNorm1[i], dNorm1[i]);
    fprintf (file2, "%8d%8d%15d%15.3f%15.3f%15.3f%15.3f%15.3f\n",
             iBand[i], iRowCol[i], iValidPoints[i], dNorm2[i], dNorm2[i], dNorm2[i], dNorm2[i], dNorm2[i]);
  }
  fclose (file1);
  fclose (file2);
}

/**
 * Get Max Dn Value, std deviation and percentage of LIS pixels from the histogram file
 *
 * @author Sharmila Prasad (2/4/2011)
 *
 * @param psHistFile - input histogram file
 * @param pdLisPer   - calculated percentage of LIS pixels
 * @param pdMaxDN    - caculated Max Dn Value
 * @param pdStdDev   - std deviation from the histogram
 */
void GetValuesFromHistogram(QString psHistFile, double & pdLisPer, double & pdMaxDN, double & pdStdDev)
{
  int iTotalPixels=0, iNullPixels=0, iLisPixels=0;

  if (gdHighEndPercent < 99.0) {
    gdHighEndPercent = 99.0;
  }
  if (gdHardHighEndPercent < 99.9) {
    gdHardHighEndPercent = 99.9;
  }

  CSVReader::CSVAxis csvArr;
  CSVReader histFile(psHistFile, true, 1, ':', false, true);
  int iRows = histFile.rows();
  int iStartIndex=0;
  for (int i=0; i<iRows; i++) {
    csvArr = histFile.getRow(i);

    if (csvArr.dim() < 2) {
      iStartIndex = i;
      break;
    }

    if (csvArr[0] == "Std Deviation") {
      pdStdDev = csvArr[1].toDouble();
    }
    else if (csvArr[0] == "Total Pixels") {
      iTotalPixels = csvArr[1].toInt();
    }
    else if (csvArr[0] == "Null Pixels") {
      iNullPixels = csvArr[1].toInt();
    }
    else if (csvArr[0] == "Lis Pixels") {
      iLisPixels = csvArr[1].toInt();
    }
  }

  pdLisPer=0;
  if (iTotalPixels - iNullPixels > 0) {
    pdLisPer = iLisPixels / (iTotalPixels - iNullPixels)*100.0;
  }

  double dCumPer = gdHighEndPercent;
  if (pdLisPer > gdHardFilter) {
    dCumPer = gdHardHighEndPercent;
  }

  pdMaxDN=0;
  histFile= CSVReader(psHistFile, true, 1, ',', false, true);
  iRows = histFile.rows();
  iStartIndex++;
  for (int i=iStartIndex; i<iRows; i++) {
    csvArr = histFile.getRow(i);
    double dCurrCumPer = csvArr[5].toDouble();
    if (dCurrCumPer > dCumPer) {
      pdMaxDN = ((csvArr[0]).toDouble() + csvArr[1].toDouble()) / 2.0;
    }
  }
}
