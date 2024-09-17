/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"
#include "IException.h"

#include "CSVReader.h"
#include "IString.h"
#include "UserInterface.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "Pipeline.h"
#include "SpecialPixel.h"

// Debugging
//#define _DEBUG_

using namespace std;
using namespace Isis;

enum eCoefficients {CCD_CH, R0, R1, R2, MAX_LINE};
CSVReader::CSVAxis csvArr;

void GetCCD_Channel_Coefficients(Pvl & pCubeLabel);
void ReadCoefficientFile(QString psCoeffile, QString psCcd, int piChannel);
void AnalyzeCubenormStats(QString psStatsFile, int piSumming, double & pdMinDN, double & pdMaxDN);
void CleanUp(vector<QString> & psTempFiles, QString psInfile);

void IsisMain() {
  vector<QString> sTempFiles;
  QString inFile, outFile;

  try {
    // Get user interface
    UserInterface &ui = Application::GetUserInterface();

    bool bRemoveTempFiles = ui.GetBoolean("REMOVE");

    bool bMapping       = ui.GetBoolean("MAPPING");
    bool bIngestion     = ui.GetBoolean("INGESTION");
    bool bHideStripe    = ui.GetBoolean("DESTRIPE");
    bool bNoiseFilter   = ui.GetBoolean("NOISE_FILTER");
    bool bRemoveFurrows = ui.GetBoolean("FURROWS");

    // Get the cube info
    inFile  = ui.GetCubeName("FROM");
    outFile = ui.GetCubeName("TO");
    Pvl cubeLabel;
    if (bIngestion) {
      int sDotPos = outFile.indexOf('.');
      QString sIngestedFile = outFile.mid(0, sDotPos);
      cubeLabel = Pvl(sIngestedFile.toStdString() + ".lev2.cub");
    }
    else {
      cubeLabel = Pvl(inFile.toStdString());
    }
    // Get the Summing from the label
    int iSumming = std::stoi(cubeLabel.findObject("IsisCube").findGroup("Instrument").findKeyword("Summing")[0]);

    Pipeline p1("hicalproc1");
    p1.SetInputFile("FROM");
    p1.SetOutputFile(FileName("$TEMPORARY/p1_out.cub"));
    sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/p1_out.cub").expanded()));
    p1.KeepTemporaryFiles(!bRemoveTempFiles);

    // If Raw image convert to Isis format
    p1.AddToPipeline("hi2isis");
    p1.Application("hi2isis").SetInputParameter ("FROM", false);
    p1.Application("hi2isis").SetOutputParameter("TO", "lev2");
    if (!bIngestion) {
      p1.Application("hi2isis").Disable();
    }

    // Run spiceinit
    p1.AddToPipeline("spiceinit");
    p1.Application("spiceinit").SetInputParameter("FROM", false);
    p1.Application("spiceinit").AddConstParameter("ATTACH", "NO");
    p1.Application("spiceinit").AddParameter     ("PCK", "PCK");
    p1.Application("spiceinit").AddParameter     ("CK", "CK");
    p1.Application("spiceinit").AddParameter     ("SPK", "SPK");
    p1.Application("spiceinit").AddParameter     ("SHAPE", "SHAPE");
    p1.Application("spiceinit").AddParameter     ("MODEL", "MODEL");
    p1.Application("spiceinit").AddParameter     ("CKNADIR", "CKNADIR");
    if (!bMapping) {
      p1.Application("spiceinit").Disable();
    }

    p1.AddToPipeline("hifurrows");
    p1.Application("hifurrows").SetInputParameter ("FROM",        false);
    p1.Application("hifurrows").SetOutputParameter("TO",          "rmfrw");
    p1.Application("hifurrows").AddConstParameter ("NEW_VERSION", "true");
    p1.Application("hifurrows").AddConstParameter ("LOWPASS",     "true");
    if (!bRemoveFurrows) {
      p1.Application("hifurrows").Disable();
    }

    // MASK if NoiseFilter==true
    p1.AddToPipeline("mask");
    // if nothing to mask, continue even if app generates an exception
    p1.Application("mask").SetContinue(true);
    p1.Application("mask").SetInputParameter ("FROM",     false);
    p1.Application("mask").SetOutputParameter("TO",       "mask1");
    p1.Application("mask").AddConstParameter ("MINIMUM",  "1200");
    p1.Application("mask").AddConstParameter ("MAXIMUM",  "16383");
    p1.Application("mask").AddConstParameter ("PRESERVE", "INSIDE");
    p1.Application("mask").AddConstParameter ("SPIXELS",  "NONE");
    if (!bNoiseFilter) {
      p1.Application("mask").Disable();
    }

    // There is no ouput created as the Furrows and Noise Filter apps are disabled
    // Hence just copy the input to the output
    if (!bIngestion && !bMapping && !bRemoveFurrows && !bNoiseFilter) {
      p1.AddToPipeline("crop");
      p1.Application("crop").SetInputParameter ("FROM", false);
      p1.Application("crop").SetOutputParameter("TO",   "copy");
    }
#ifdef _DEBUG_
    cout << p1 << endl;
    cout << "*****************************************************************\n";
#endif
    p1.Run();


    Pipeline pStats;
    pStats.SetInputFile(FileName("$TEMPORARY/p1_out.cub"));
    pStats.SetOutputFile(FileName("$TEMPORARY/statsMask"));
    sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/statsMask").expanded()));
    pStats.KeepTemporaryFiles(!bRemoveTempFiles);

    pStats.AddToPipeline("cubenorm");
    pStats.Application("cubenorm").SetInputParameter("FROM",   false);
    pStats.Application("cubenorm").SetOutputParameter("STATS", "statsMask");
    if (!bNoiseFilter) {
      pStats.Application("cubenorm").Disable();
    }
    else {
#ifdef _DEBUG_
      cout << pStats << endl;
      cout << "*****************************************************************\n";
#endif
      pStats.Run();
    }

    double dMinDN=0, dMaxDN=0;
    if (bNoiseFilter) {
      AnalyzeCubenormStats(QString::fromStdString(FileName("$TEMPORARY/statsMask").expanded()), iSumming, dMinDN, dMaxDN);
    }

    Pipeline p2("hicalproc2");
    p2.SetInputFile(FileName("$TEMPORARY/p1_out.cub"));
    if (bNoiseFilter || bHideStripe || bMapping ) {
      p2.SetOutputFile(FileName("$TEMPORARY/p2_out.cub"));
      sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/p2_out.cub").expanded()));
    }
    else {
      p2.SetOutputFile("TO");
    }
    p2.KeepTemporaryFiles(!bRemoveTempFiles);

    p2.AddToPipeline("mask");
    p2.Application("mask").SetContinue(true);
    p2.Application("mask").SetInputParameter("FROM",     false);
    p2.Application("mask").SetOutputParameter("TO",      "mask2");
    p2.Application("mask").AddConstParameter("MINIMUM",  QString::number(dMinDN));
    p2.Application("mask").AddConstParameter("MAXIMUM",  QString::number(dMaxDN));
    p2.Application("mask").AddConstParameter("PRESERVE", "INSIDE");
    p2.Application("mask").AddConstParameter("SPIXELS",  "NONE");
    if (!bNoiseFilter) {
      p2.Application("mask").Disable();
    }

    // Caliberation
    p2.AddToPipeline("hical");
    p2.Application("hical").SetInputParameter("FROM", false);
    p2.Application("hical").SetOutputParameter("TO", "hical");

    // Gaindrift Corrections
    GetCCD_Channel_Coefficients(cubeLabel);
    p2.AddToPipeline("fx");
    p2.Application("fx").SetInputParameter("F1", false);
    p2.Application("fx").SetOutputParameter("TO", "gnfx");
    p2.Application("fx").AddConstParameter("MODE", "CUBES");
    QString sEquation = "\\((F1/(" + csvArr[R0]+ "+( " + csvArr[R1] + "*line)+(" + csvArr[R2];
    sEquation += "*line*line))) *(line<" + csvArr[MAX_LINE]+ ") + (F1*(line>=" + csvArr[MAX_LINE] + ")))";
    sEquation = sEquation.simplified();
    p2.Application("fx").AddConstParameter("EQUATION", sEquation);
#ifdef _DEBUG_
    cerr << "FX Equation=" << sEquation << endl;
    cout << p2<<endl;
    cout << "*****************************************************************\n";
#endif
    p2.Run();

    // ********************************************************************************************
    // CubeNorm Corrections
    // ********************************************************************************************
    if (bNoiseFilter || bHideStripe) {
      Pipeline p3("hicalproc3");
      p3.SetInputFile(FileName("$TEMPORARY/p2_out.cub"));
      p3.SetOutputFile(FileName("$TEMPORARY/StatsCubeNorm1"));
      sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/StatsCubeNorm1").expanded()));
      p3.KeepTemporaryFiles(!bRemoveTempFiles);

      // Crop if skip top and bottom lines are defined in the Configuration file
      /*p3.AddToPipeline("crop");
      p3.Application("crop").SetInputParameter ("FROM",   false);
      p3.Application("crop").SetOutputParameter("TO",     "crop");
      p3.Application("crop").AddConstParameter ("line",   "1");
      //p3.Application("crop").AddConstParameter ("nlines", "156");
      p3.Application("crop").AddConstParameter ("nlines", "209");*/

      p3.AddToPipeline("cubenorm");
      p3.Application("cubenorm").SetInputParameter ("FROM",   false);
      p3.Application("cubenorm").SetOutputParameter("stats",  "stats");
      p3.Application("cubenorm").AddConstParameter ("format", "table");
      p3.Run();

  #ifdef _DEBUG_
      cout << p3 << endl;
      cout << "*****************************************************************\n";
  #endif

      Pipeline p4("hicalproc4");
      p4.SetInputFile(FileName("$TEMPORARY/p2_out.cub"));
      p4.SetOutputFile(FileName("$TEMPORARY/StatsCubeNorm2"));
      sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/StatsCubeNorm2").expanded()));
      p4.KeepTemporaryFiles(!bRemoveTempFiles);

      p4.AddToPipeline("hicubenorm");
      p4.Application("hicubenorm").SetInputParameter ("FROM",         false);
      p4.Application("hicubenorm").SetOutputParameter("STATS",        "hicbnrm");
      p4.Application("hicubenorm").AddConstParameter ("FORMAT",       "TABLE");
      p4.Application("hicubenorm").AddConstParameter ("FILTER",       "5");
      p4.Application("hicubenorm").AddConstParameter ("STATSOURCE",   "TABLE");
      p4.Application("hicubenorm").AddConstParameter ("FROMSTATS",
                                       QString::fromStdString(FileName("$TEMPORARY/StatsCubeNorm1").expanded()));
      p4.Application("hicubenorm").AddConstParameter ("NEW_VERSION",   "TRUE");
      p4.Application("hicubenorm").AddConstParameter ("HIGHPASS_MODE", "HIGHPASS_DIVIDE");
      p4.Application("hicubenorm").AddConstParameter ("PAUSECROP",     "TRUE");

      p4.Run();

  #ifdef _DEBUG_
      cout << p4 << endl;
      cout << "*****************************************************************\n";
  #endif

      Pipeline p5("hicalproc5");
      p5.SetInputFile(FileName("$TEMPORARY/p2_out.cub"));
      if (bHideStripe || bMapping) {
        p5.SetOutputFile(FileName("$TEMPORARY/p5_out.cub"));
        sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/p5_out.cub").expanded()));
      }
      else {
        p5.SetOutputFile("TO");
      }
      p5.KeepTemporaryFiles(!bRemoveTempFiles);
      p5.SetContinue(true);

      p5.AddToPipeline("cubenorm");
      p5.Application("cubenorm").SetInputParameter ("FROM",       false);
      p5.Application("cubenorm").SetOutputParameter("TO",         "cbnorm");
      p5.Application("cubenorm").AddConstParameter ("format",     "TABLE");
      p5.Application("cubenorm").AddConstParameter ("STATSOURCE", "TABLE");
      p5.Application("cubenorm").AddConstParameter ("FROMSTATS",  QString::fromStdString(FileName("$TEMPORARY/StatsCubeNorm2").expanded()));
      p5.Application("cubenorm").AddConstParameter ("DIRECTION",  "COLUMN");
      p5.Application("cubenorm").AddConstParameter ("NORMALIZER", "AVERAGE");
      p5.Application("cubenorm").AddConstParameter ("PRESERVE",   "FALSE");
      p5.Application("cubenorm").AddConstParameter ("MODE",       "DIVIDE");

      // **********************************************************************************
      // Noise Filter
      if (bNoiseFilter) {
        p5.AddToPipeline("hinoise");
        p5.Application("hinoise").SetInputParameter ("FROM", false);
        p5.Application("hinoise").SetOutputParameter("TO",   "hinoise");
        p5.Application("hinoise").AddConstParameter ("REMOVE", QString::number(bRemoveTempFiles));

        // Values got from HiCal configuration file
        // Lowpass options
        p5.Application("hinoise").AddConstParameter ("LPF_LINES",   "251");
        p5.Application("hinoise").AddConstParameter ("LPF_SAMPLES", "3");
        p5.Application("hinoise").AddConstParameter ("LPF_MINPER",  "5");

        // Highpass Options
        p5.Application("hinoise").AddConstParameter ("HPF_LINES",   "251");
        p5.Application("hinoise").AddConstParameter ("HPF_SAMPLES", "1");
        p5.Application("hinoise").AddConstParameter ("HPF_MINPER",  "5");

        // Noise Filter options
        p5.Application("hinoise").AddConstParameter ("NULL_COLUMNS",         "FALSE");
        p5.Application("hinoise").AddConstParameter ("TOLMIN",               "3.5");
        p5.Application("hinoise").AddConstParameter ("TOLMAX",               "3.5");
        p5.Application("hinoise").AddConstParameter ("FLATTOL",              "1.0");
        p5.Application("hinoise").AddConstParameter ("MIN_VALUE",            "0.0");
        p5.Application("hinoise").AddConstParameter ("HARD_TOLMIN",          "3.5");
        p5.Application("hinoise").AddConstParameter ("HARD_TOLMAX",          "3.5");
        p5.Application("hinoise").AddConstParameter ("LPFZ_LINES",           "5");
        p5.Application("hinoise").AddConstParameter ("LPFZ_SAMPLES",         "5");
        p5.Application("hinoise").AddConstParameter ("NOISE_LINES",          "7");
        p5.Application("hinoise").AddConstParameter ("NOISE_SAMPLES",        "7");
        p5.Application("hinoise").AddConstParameter ("CLEAR_FRACTION",       "0.8");
        p5.Application("hinoise").AddConstParameter ("NONVALID_FRACTION",    "0.9");
        p5.Application("hinoise").AddConstParameter ("HARD_FILTERING",       "0.1");
        p5.Application("hinoise").AddConstParameter ("HIGHEND_PERCENT",      "99.999");
        p5.Application("hinoise").AddConstParameter ("HARD_HIGHEND_PERCENT", "99.99");


        p5.Application("hinoise").Disable();
      }
      p5.Run();
#ifdef _DEBUG_
      cout << p5 << endl;
      cout << "*****************************************************************\n";
#endif
    }

    // **********************************************************************************
    // HideStripe Filter
    if (bHideStripe) {
      Pipeline p6("hicalproc6");
      p6.SetInputFile(FileName("$TEMPORARY/p5_out.cub"));
      if (!bMapping) {
        p6.SetOutputFile("TO");
      }
      else {
        p6.SetOutputFile(FileName("$TEMPORARY/p6_out.cub"));
        sTempFiles.push_back(QString::fromStdString(FileName("$TEMPORARY/p6_out.cub").expanded()));
      }
      p6.KeepTemporaryFiles(!bRemoveTempFiles);

      if (iSumming == 1 || iSumming == 2) {
        p6.AddToPipeline("hidestripe", "hidestripe1");
        p6.Application("hidestripe1").SetInputParameter ("FROM",       false);
        p6.Application("hidestripe1").SetOutputParameter("TO",         "hdstrp1");
        p6.Application("hidestripe1").AddConstParameter ("PARITY",     "EVEN");
        p6.Application("hidestripe1").AddConstParameter ("CORRECTION", "ADD");

        p6.AddToPipeline("hidestripe", "hidestripe2");
        p6.Application("hidestripe2").SetInputParameter ("FROM",       false);
        p6.Application("hidestripe2").SetOutputParameter("TO",         "hdstrp2");
        p6.Application("hidestripe2").AddConstParameter ("PARITY",     "ODD");
        p6.Application("hidestripe2").AddConstParameter ("CORRECTION", "ADD");

      }
      else {
        p6.AddToPipeline("hidestripe");
        p6.Application("hidestripe").SetInputParameter ("FROM",       false);
        p6.Application("hidestripe").SetOutputParameter("TO",         "hdstrp");
        p6.Application("hidestripe").AddConstParameter ("PARITY",     "AUTO");
        p6.Application("hidestripe").AddConstParameter ("CORRECTION", "ADD");
      }
      p6.Run();
#ifdef _DEBUG_
      cout << p6 << endl;
      cout << "*****************************************************************\n";
#endif
    }

    // **********************************************************************************
    // Projection
    if (bMapping) {
      Pipeline p7("hicalproc7");
      if (bHideStripe) {
        p7.SetInputFile(FileName("$TEMPORARY/p6_out.cub"));
      }
      else {
        p7.SetInputFile(FileName("$TEMPORARY/p2_out.cub"));
      }
      p7.SetOutputFile("TO");
      p7.KeepTemporaryFiles(!bRemoveTempFiles);

      p7.AddToPipeline("cam2map");
      p7.Application("cam2map").SetInputParameter ("FROM", false);
      p7.Application("cam2map").SetOutputParameter("TO", "map");
      p7.Application("cam2map").AddParameter      ("MAP", "MAP");
      p7.Application("cam2map").AddParameter      ("PIXRES", "RESOLUTION");
      if(ui.WasEntered("PIXRES")) {
        p7.Application("cam2map").AddConstParameter("PIXRES", "MPP");
      }
#ifdef _DEBUG_
      cout << p7 << endl;
      cout << "*****************************************************************\n";
#endif

      p7.Run();
    }
    CleanUp(sTempFiles, inFile);
  }
  catch(IException &) {
    CleanUp(sTempFiles, inFile);
    throw;
  }
  catch(std::exception const &se) {
    CleanUp(sTempFiles, inFile);
    std::string message = "std::exception: " + (std::string)se.what();
    throw IException(IException::User, message, _FILEINFO_);
  }
  catch(...) {
    CleanUp(sTempFiles, inFile);
    std::string message = "Other Error";
    throw IException(IException::User, message, _FILEINFO_);
  }
}

/**
 * Clean up intermediate and log files
 *
 * @author Sharmila Prasad (2/14/2011)
 */
void CleanUp(vector<QString> & psTempFiles, QString psInfile)
{
  // more clean up
  for (int i=0; i<(int)psTempFiles.size(); i++) {
    remove(psTempFiles[i].toLatin1().data());
  }
  psTempFiles.clear();

  // Cleanup log files
  int ipos = psInfile.indexOf(".cub");
  if (ipos != -1) {
    QString sLogFile = psInfile.replace(ipos, 4, ".hical.log");
    ipos = sLogFile.lastIndexOf("/");
    sLogFile.replace(0, ipos+1, "./");
    //cerr << "Log=" << sLogFile << endl;
    remove(sLogFile.toLatin1().data());
  }
}

/**
 * With the Channel, CCD in the isis label, find the coefficient values
 * for this image
 *
 * @author Sharmila Prasad (11/24/2010)
 *
 * @param pCubeLabel
 */
void GetCCD_Channel_Coefficients(Pvl & pCubeLabel)
{
  int iChannel=-1, iSumming=-1;
  QString sCcd="";

  PvlGroup instrGrp = pCubeLabel.findObject("IsisCube").findGroup("Instrument");

  // Summing keyword
  if (!instrGrp.hasKeyword("Summing")) {
    std::string sMsg = "Summing keyword not found";
    throw IException(IException::User, sMsg, _FILEINFO_);
  }
  else {
    PvlKeyword binKey = instrGrp.findKeyword("Summing");
    iSumming = std::stoi(binKey[0]);
    if (iSumming != 1 && iSumming != 2 && iSumming != 4) {
      std::string sMsg = "Invalid Summing value in input file, must be 1,2,or 4";
      throw IException(IException::User, sMsg, _FILEINFO_);
    }
  }

  // CCD Keyword
  if (!instrGrp.hasKeyword("CcdId")) {
    std::string sMsg = "CcdId keyword not found";
    throw IException(IException::User, sMsg, _FILEINFO_);
  }
  else {
    PvlKeyword ccdKey = instrGrp.findKeyword("CcdId");
    sCcd = QString::fromStdString(ccdKey[0]);
  }

  // Channel Keyword
  if (!instrGrp.hasKeyword("ChannelNumber")) {
    std::string sMsg = "ChannelNumber keyword not found";
    throw IException(IException::User, sMsg, _FILEINFO_);
  }
  else {
    PvlKeyword channelKey = instrGrp.findKeyword("ChannelNumber");
    iChannel = std::stoi(channelKey[0]);
  }

  // Get the coefficient file name
  QString dCoeffFile = "$mro/calibration/HiRISE_Gain_Drift_Correction_Bin" + QString::number(iSumming) + ".0001.csv";
  //QString dCoeffFile = "/home/sprasad/isis3/isis/src/mro/apps/hicalproc/HiRISE_Gain_Drift_Correction_Bin" + toString(iSumming) + ".0001.csv";
#ifdef _DEBUG_
  cout << dCoeffFile << endl;
#endif

  // Get the coefficients
  ReadCoefficientFile(QString::fromStdString(FileName(dCoeffFile.toStdString()).expanded()), sCcd, iChannel);
}

/**
 * Reads the CSV coefficient file
 *
 * @author Sharmila Prasad (11/24/2010)
 *
 * @param psCoeffFile = Coefficient file name
 */
void ReadCoefficientFile(QString psCoeffile, QString psCcd, int piChannel)
{
  CSVReader coefFile(psCoeffile, true, 1, ',', false, true);
  double dCoeff[5];
  int iRows = coefFile.rows();
  int iRowIndex = -1;

  QString sColName = psCcd + "_" + QString::number(piChannel);
#ifdef _DEBUG_
  cout << endl << "Col name=" << sColName <<  "Rows=" << iRows << endl;
#endif

  for (int i=0; i<iRows; i++) {
    csvArr = coefFile.getRow(i);
    if (csvArr[0] == sColName) {
      iRowIndex = i;
      break;
    }
  }

  if (iRowIndex != -1) {
    int iArrSize = csvArr.dim();
    for (int i=1; i<iArrSize; i++) {
      csvArr[i] = csvArr[i].trimmed().remove(QRegExp("(^,*|,*$)"));
      dCoeff[i] = csvArr[i].toDouble();
      if (dCoeff[i] < 0) {
        dCoeff[i] *= -1;
        csvArr[i] = QString::number(dCoeff[i]) ;
      }
    }
  }
}

/**
 * Develop min and max thresholds for elminating bad pixels. This part of
 * the Mask() functionality in the hical pipeline
 *
 * @author Sharmila Prasad (2/1/2011)
 *
 * @param psStatsFile - Cubenorm stats output file
 * @param pdMinDN     - Minimum DN Value
 * @param pdMaxDN     - Maximum DN value
 */
void AnalyzeCubenormStats(QString psStatsFile, int piSumming, double & pdMinDN, double & pdMaxDN)
{
  CSVReader coefFile(psStatsFile, true, 1, ' ', false, true);
  int iRows = coefFile.rows();
  vector <int> iValidPoints;
  vector <double> dStdDev, dMinimum, dMaximum;

  // Skip the header row
  int iMaxValidPoints=0;
  for (int i=1; i<iRows; i++) {
    csvArr = coefFile.getRow(i);

    int iArrSize = csvArr.dim();
    for (int j=0; j<iArrSize; j++) {
      csvArr[i] = csvArr[i].trimmed().remove(QRegExp("(^,*|,*$)"));
      // Store Valid Points and get the Max Valid Points for a
      // given RowCol
      //cerr << "  " << j << "." << csvArr[j];
      if (j==2) {
        iValidPoints.push_back(csvArr[j].toInt());
        int iCurrValidPoints = csvArr[j].toInt();
        if (iCurrValidPoints > iMaxValidPoints) {
          iMaxValidPoints = iCurrValidPoints;
        }
        //cerr << iCurrValidPoints << endl;
      }
      // Get the Standard deviation
      if (j==5) {
        dStdDev.push_back(csvArr[j].toDouble());
      }

      // Get the Maximum and Minimum values
      if (j==6) {
        dMinimum.push_back(csvArr[j].toDouble());
      }
      if (j==7) {
        dMaximum.push_back(csvArr[j].toDouble());
      }
    }
  }

  // Get the median standard deviation value for all columns that have
  // the maximum valid pixel count
  vector <double> dStdDevValidPnts;
  for (int i=0; i<iRows; i++) {
    if (iValidPoints[i] == iMaxValidPoints) {
      dStdDevValidPnts.push_back(dStdDev[i]);
    }
  }

  sort (dStdDevValidPnts.begin(), dStdDevValidPnts.end());
  double dStdFraction  = 0.95;
  double dMedianStdDev = dStdDevValidPnts[ int( (dStdDevValidPnts.size()-1) * dStdFraction) ];

  // Find the minimum of minimums and the maximum of maximums for any
  // column whose std is less than or equal to $medstd + $tol;
  double dValidPointFrac = (iMaxValidPoints * 0.90);
  vector<double>dValidMin, dValidMax;
  for (int i=0; i<iRows; i++) {
    if (iValidPoints[i] >= dValidPointFrac && dStdDev[i] < dMedianStdDev) {
      dValidMin.push_back(dMinimum[i]);
      dValidMax.push_back(dMaximum[i]);
    }
  }
  sort (dValidMin.begin(), dValidMin.end());
  sort (dValidMax.begin(), dValidMax.end());

  pdMinDN = int( dValidMin[int( (dValidMin.size()-1) * 0.05)] );
  pdMaxDN = int( dValidMax[int( (dValidMax.size()-1) * 0.95)] );

  double dMinFraction=0, dMaxFraction=0;
  if (piSumming == 1) {
    dMinFraction = 0.70;
    dMaxFraction = 1.30;
  }
  else if (piSumming == 2) {
    dMinFraction = 0.60;
    dMaxFraction = 1.40;
  }
  else {
    dMinFraction = 0.50;
    dMaxFraction = 1.50;
  }
  pdMinDN = dMinFraction * pdMinDN;
  pdMaxDN = dMaxFraction * pdMaxDN;
#ifdef _DEBUG_
  cerr << "MinDN=" << pdMinDN << "   MaxDN=" << pdMaxDN << endl;
#endif
}
