/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdio>
#include <QString>
#include <vector>
#include <algorithm>
#include <sstream>
#include <iostream>

#include "Application.h"
#include "FileName.h"
#include "ProcessByLine.h"
#include "UserInterface.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "IString.h"
#include "HiCalConf.h"
#include "CollectorMap.h"
#include "HiCalTypes.h"
#include "HiCalUtil.h"
#include "HiCalData.h"
#include "Statistics.h"
#include "SplineFill.h"              // SpineFillComp.h
#include "LowPassFilter.h"           // LowPassFilterComp.h
#include "ZeroBufferSmooth.h"        // DriftBuffer.h  (Zf)
#include "ZeroBufferFit.h"           // DriftCorrect.h (Zb)
#include "ZeroReverse.h"             // OffsetCorrect.h (Zz)
#include "ZeroDark.h"                // DarkSubtractComp.h (Zd)
#include "ZeroDarkRate.h"            // DarkSubtractComp.h (Zdr)
#include "GainLineDrift.h"           // GainVLineComp.h (Zg)
#include "GainNonLinearity.h"        // Non-linear gain (new)
#include "GainChannelNormalize.h"    // ZggModule.h (Zgg)
#include "GainFlatField.h"           // FlatFieldComp.h (Za)
#include "GainTemperature.h"         // TempGainCorrect.h (Zt)
#include "GainUnitConversion.h"      // ZiofModule.h (Ziof)

#include "hical.h"

using namespace std;

namespace Isis {

  //!< Define the matrix container for systematic processing
  typedef CollectorMap<IString, HiVector, NoCaseStringCompare> MatrixList;

  void hical(UserInterface &ui, Pvl *log) {
    const QString hical_program = "hical";
    const QString hical_version = "5.0";
    const QString hical_revision = "$Revision: 6715 $";
    const QString hical_runtime = Application::DateTime();

    QString procStep("prepping phase");
    MatrixList *calVars = nullptr;
    try {
      // The output from the last processing is the input into subsequent processing
      ProcessByLine p;

      Cube *hifrom = p.SetInputCube(ui.GetCubeName("FROM"), ui.GetInputAttribute("FROM"));
      int nsamps = hifrom->sampleCount();
      int nlines = hifrom->lineCount();

      // Initialize the configuration file
      QString conf(ui.GetAsString("CONF"));
      HiCalConf hiconf(*(hifrom->label()), conf);
      DbProfile hiprof = hiconf.getMatrixProfile();

      // Check for label propagation and set the output cube
      Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"));
      if ( !IsTrueValue(hiprof,"PropagateTables", "TRUE") ) {
        RemoveHiBlobs(*(ocube->label()));
      }

      // Set specified profile if entered by user
      if (ui.WasEntered("PROFILE")) {
        hiconf.selectProfile(ui.GetAsString("PROFILE"));
      }


      // Add OPATH parameter to profiles
      if (ui.WasEntered("OPATH")) {
        hiconf.add("OPATH",ui.GetAsString("OPATH"));
      }
      else {
        // Set default to output directory
        hiconf.add("OPATH", FileName(ocube->fileName()).path());
      }

      // Do I/F output DN conversions
      QString units = ui.GetString("UNITS");

      // Allocate the calibration list
      calVars = new MatrixList;

      // Set up access to HiRISE ancillary data (tables, blobs) here.  Note it they
      // are gone, this will error out. See PropagateTables in conf file.
      HiCalData caldata(*hifrom);

      ////////////////////////////////////////////////////////////////////////////
      //  Drift Correction (Zf) using buffer pixels
      //    Extracts specified regions of the calibration buffer pixels and runs
      //    series of lowpass filters.  Apply spline fit if any missing data
      //    remains.  Config file contains parameters for this operation.
      procStep = "ZeroBufferSmooth module";
      hiconf.selectProfile("ZeroBufferSmooth");
      hiprof = hiconf.getMatrixProfile();
      HiHistory ZbsHist;
      ZbsHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        ZeroBufferSmooth zbs(caldata, hiconf);
        calVars->add("ZeroBufferSmooth", zbs.ref());
        ZbsHist = zbs.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          zbs.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        //  NOT RECOMMENDED!  This is required for the next step!
        //  SURELY must be skipped with ZeroBufferSmooth step as well!
        calVars->add("ZeroBufferSmooth", HiVector(nlines, 0.0));
        ZbsHist.add("Debug::SkipModule invoked!");
      }

      /////////////////////////////////////////////////////////////////////
      //  ZeroBufferFit
      //    Compute second level of drift correction.  The high level noise
      //    is removed from a modeled non-linear fit.
      //
      procStep = "ZeroBufferFit module";
      HiHistory ZbfHist;
      hiconf.selectProfile("ZeroBufferFit");
      hiprof = hiconf.getMatrixProfile();
      ZbfHist.add("Profile["+ hiprof.Name()+"]");
      if (!SkipModule(hiprof) ) {
        ZeroBufferFit zbf(hiconf);

        calVars->add(hiconf.getProfileName(),
                     zbf.Normalize(zbf.Solve(calVars->get("ZeroBufferSmooth"))));
        ZbfHist = zbf.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          zbf.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nlines, 0.0));
        ZbfHist.add("Debug::SkipModule invoked!");
      }


      ////////////////////////////////////////////////////////////////////
      //  ZeroReverse
      procStep = "ZeroReverse module";
      hiconf.selectProfile("ZeroReverse");
      hiprof = hiconf.getMatrixProfile();
      HiHistory ZrHist;
      ZrHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        ZeroReverse zr(caldata, hiconf);
        calVars->add(hiconf.getProfileName(), zr.ref());
        ZrHist = zr.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          zr.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nsamps, 0.0));
        ZrHist.add("Debug::SkipModule invoked!");
      }


      /////////////////////////////////////////////////////////////////
      //  ZeroDarkRate removes dark current with a new approach compared
      //    with ZeroDark.
      bool zdrFallback = false;
      procStep = "ZeroDarkRate module";
      HiHistory ZdrHist;
      ZdrHist.add("Profile[ZeroDarkRate]");
      // Check for backwards compatibility with old config files
      if (!hiconf.profileExists("ZeroDarkRate")) {
        calVars->add("ZeroDarkRate", HiVector(nsamps, 0.0));
        ZdrHist.add("Skipped, module not in config file");
      }
      else {
        hiconf.selectProfile("ZeroDarkRate");
        hiprof =  hiconf.getMatrixProfile();

        if ( !SkipModule(hiprof) ) {
          // Make sure we aren't applying ZeroDark and ZeroDarkRate
          if ( !SkipModule(hiconf.getMatrixProfile("ZeroDark")) ){
            std::string mess = "You have enabled both the ZeroDark and the ZeroDarkRate modules."
                           "This means you are attempting to remove the dark current twice with "
                           "two different algorithms. This is not approved use of hical. "
                           "Please disable one or the other module using the Debug::SkipModule "
                           "in your configuration file.";
            throw IException(IException::User, mess, _FILEINFO_);
          }
          try{
            ZeroDarkRate zdr(hiconf);
            calVars->add(hiconf.getProfileName(), zdr.ref());
            ZdrHist = zdr.History();
            if ( hiprof.exists("DumpModuleFile") ) {
              zdr.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
            }
          }
          catch(IException &e){
            if (hiprof.exists("Fallback") && IsTrueValue(hiprof, "Fallback")){
                zdrFallback = true;
                calVars->add(hiconf.getProfileName(), HiVector(nsamps, 0.0));
                std::cerr << "Falling back to ZeroDark implementation. Unable to initialize ZeroDarkRate "
                          << "module with the following error:" << std::endl << e.what() << "\nContinuing..."<< std::endl;
                ZdrHist.add("Debug::Unable to initialize ZeroDarkRate module. "
                            "Falling back to ZeroDark implementation");
            } else {
              // If fallback is not found or fallback is false, throw the exception
              std::cerr<< "\nNot all combinations of CCD, channel, TDI rate, binning value, and ADC setting "
                            "have a DarkRate*.csv available, and you may need to run hical with ZeroDark instead "
                            "of ZeroDarkRate specified in the configuration file. Alternatively, you may specify "
                            "Fallback = True in the ZeroDarkRate configuration profile to automatically use the "
                            "ZeroDark module on ZeroDarkRate failure.\n" << std::endl;
              throw(e);
            }
          }
        }
        else {
          calVars->add(hiconf.getProfileName(), HiVector(nsamps, 0.0));
          ZdrHist.add("Debug::SkipModule invoked!");
        }
      }

      /////////////////////////////////////////////////////////////////
      //  ZeroDark removes dark current
      //
      procStep = "ZeroDark module";
      hiconf.selectProfile("ZeroDark");
      hiprof =  hiconf.getMatrixProfile();
      HiHistory ZdHist;
      ZdHist.add("Profile["+ hiprof.Name()+"]");
      // If ZeroDark module is enabled or we need to fall back to zero dark from zero dark rate
      if ( !SkipModule(hiprof) || zdrFallback) {
        ZeroDark zd(hiconf);
        calVars->add(hiconf.getProfileName(), zd.ref());
        ZdHist = zd.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          zd.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nsamps, 0.0));
        ZdHist.add("Debug::SkipModule invoked!");
      }


      ////////////////////////////////////////////////////////////////////
      //  GainLineDrift correct for gain-based drift
      //
      procStep = "GainLineDrift module";
      hiconf.selectProfile("GainLineDrift");
      hiprof = hiconf.getMatrixProfile();
      HiHistory GldHist;
      GldHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        GainLineDrift gld(hiconf);
        calVars->add(hiconf.getProfileName(), gld.ref());
        GldHist = gld.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          gld.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nlines, 1.0));
        GldHist.add("Debug::SkipModule invoked!");
      }

      ////////////////////////////////////////////////////////////////////
      //  GainNonLinearity  Correct for non-linear gain
      procStep = "GainNonLinearity module";
      hiconf.selectProfile("GainNonLinearity");
      hiprof =  hiconf.getMatrixProfile();
      HiHistory GnlHist;
      GnlHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        GainNonLinearity gnl(hiconf);
        calVars->add(hiconf.getProfileName(), gnl.ref());
        GnlHist = gnl.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          gnl.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(1, 0.0));
        GnlHist.add("Debug::SkipModule invoked!");
      }

      ////////////////////////////////////////////////////////////////////
      //  GainChannelNormalize  Correct for sample gain with the G matrix
      procStep = "GainChannelNormalize module";
      hiconf.selectProfile("GainChannelNormalize");
      hiprof =  hiconf.getMatrixProfile();
      HiHistory GcnHist;
      GcnHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        GainChannelNormalize gcn(hiconf);
        calVars->add(hiconf.getProfileName(), gcn.ref());
        GcnHist = gcn.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          gcn.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nsamps, 1.0));
        GcnHist.add("Debug::SkipModule invoked!");
      }

      ////////////////////////////////////////////////////////////////////
      //  GainFlatField  Flat field correction with A matrix
      procStep = "GainFlatField module";
      hiconf.selectProfile("GainFlatField");
      hiprof =  hiconf.getMatrixProfile();
      HiHistory GffHist;
      GffHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        GainFlatField gff(hiconf);
        calVars->add(hiconf.getProfileName(), gff.ref());
        GffHist = gff.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          gff.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nsamps, 1.0));
        GffHist.add("Debug::SkipModule invoked!");
      }

      ////////////////////////////////////////////////////////////////////
      //  GainTemperature -  Temperature-dependant gain correction
      procStep = "GainTemperature module";
      hiconf.selectProfile("GainTemperature");
      hiprof =  hiconf.getMatrixProfile();
      HiHistory GtHist;
      GtHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        GainTemperature gt(hiconf);
        calVars->add(hiconf.getProfileName(), gt.ref());
        GtHist = gt.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          gt.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(nsamps, 1.0));
        GtHist.add("Debug::SkipModule invoked!");
      }

      ////////////////////////////////////////////////////////////////////
      //  GainUnitConversion converts to requested units
      //
      procStep = "GainUnitConversion module";
      hiconf.selectProfile("GainUnitConversion");
      hiprof = hiconf.getMatrixProfile();
      HiHistory GucHist;
      GucHist.add("Profile["+ hiprof.Name()+"]");
      if ( !SkipModule(hiprof) ) {
        GainUnitConversion guc(hiconf, units, hifrom);
        calVars->add(hiconf.getProfileName(), guc.ref());
        GucHist = guc.History();
        if ( hiprof.exists("DumpModuleFile") ) {
          guc.Dump(hiconf.getMatrixSource("DumpModuleFile",hiprof));
        }
      }
      else {
        calVars->add(hiconf.getProfileName(), HiVector(1,1.0));
        GucHist.add("Debug::SkipModule invoked!");
        GucHist.add("Units[Unknown]");
      }

      // Reset the profile selection to default
      hiconf.selectProfile();

      //----------------------------------------------------------------------
      //
      /////////////////////////////////////////////////////////////////////////
      //  Main processing function
      /**
       * @brief Apply calibration to each HiRISE image line
       *
       * This function applies the calbration equation to each input image line.  It
       * gets matrices and constants from the \b calVars container that is established
       * in the main with some user input via the configuration (CONF) parameter.
       *
       * @param in  Input raw image line buffer
       * @param out Output calibrated image line buffer
       */
      auto calibrationFunc = [calVars](Buffer &in, Buffer &out)->void {
        const HiVector &ZBF    = calVars->get("ZeroBufferFit");
        const HiVector &ZRev   = calVars->get("ZeroReverse");
        const HiVector &ZD     = calVars->get("ZeroDark");
        const HiVector &ZDR    = calVars->get("ZeroDarkRate");
        const HiVector &GLD    = calVars->get("GainLineDrift");
        const HiVector &GCN    = calVars->get("GainChannelNormalize");
        const double GNL       = calVars->get("GainNonLinearity")[0];
        const HiVector &GFF    = calVars->get("GainFlatField");
        const HiVector &GT     = calVars->get("GainTemperature");
        const double GUC       = calVars->get("GainUnitConversion")[0];

        // Set current line (index)
        int line(in.Line()-1);
        if (calVars->exists("LastGoodLine")) {
          int lastline = ((int) (calVars->get("LastGoodLine"))[0]) - 1;
          if ( line > lastline ) { line = lastline; }
        }


        // Apply correction to point of non-linearity accumulating average
        vector<double> data;
        for (int i = 0 ; i < in.size() ; i++) {
          if (IsSpecial(in[i])) {
            out[i] = in[i];
          }
          else {
            double hdn;
            hdn = (in[i] - ZBF[line] - ZRev[i] - ZD[i] - ZDR[i]); // Drift, Reverse, Dark, DarkRate
            hdn = hdn / GLD[line];  // GainLineDrift
            data.push_back(hdn);   // Accumulate average for non-linearity
            out[i] = hdn;
          }
        }

        // Second loop require to apply non-linearity gain correction from average
        if ( data.size() > 0 ) {  // No valid pixels means just that - done
          // See HiCalUtil.h for the function that returns this stat.
          double NLGain = 1.0 - (GNL * GainLineStat(data));
          for (int i = 0 ; i < out.size() ; i++) {
            if (!IsSpecial(out[i])) {
              double hdn = out[i];
              hdn = hdn * GCN[i] * NLGain * GFF[i] * GT[i];  // Gain, Non-linearity
                                                          // gain,  FlatField,
                                                          // TempGain
              out[i] = hdn / GUC;                         // I/F or DN or DN/US
            }
          }
        }
        return;
      };

      procStep = "calibration phase";
      p.StartProcess(calibrationFunc);

      // Get the default profile for logging purposes
      hiprof = hiconf.getMatrixProfile();
      const QString conf_file = hiconf.filepath(conf);

      // Quitely dumps parameter history to alternative format file.  This
      // is completely controlled by the configuration file
      if ( hiprof.exists("DumpHistoryFile") ) {
        procStep = "logging/reporting phase";
        FileName hdump(hiconf.getMatrixSource("DumpHistoryFile",hiprof));
        QString hdumpFile = hdump.expanded();
        ofstream ofile(hdumpFile.toLatin1().data(), ios::out);
        if (!ofile) {
          std::string mess = "Unable to open/create history dump file " +
                        hdump.expanded();
          IException(IException::User, mess, _FILEINFO_).print();
        }
        else {
          ofile << "Program:  " << hical_program << endl;
          ofile << "RunTime:  " << hical_runtime << endl;
          ofile << "Version:  " << hical_version << endl;
          ofile << "Revision: " << hical_revision << endl << endl;

          ofile << "FROM:     " << hifrom->fileName() << endl;
          ofile << "TO:       " << ocube->fileName()  << endl;
          ofile << "CONF:     " << conf_file  << endl << endl;

          ofile << "/* " << hical_program << " application equation */\n"
                << "/* hdn = (idn - ZeroBufferFit(ZeroBufferSmooth) - ZeroReverse -"
                << "(ZeroDark OR ZeroDarkRate) */\n"
                << "/* odn = hdn / GainLineDrift * GainNonLinearity * GainChannelNormalize */\n"
                << "/*           * GainFlatField  * GainTemperature / GainUnitConversion */\n\n";

          ofile << "****** PARAMETER GENERATION HISTORY *******" << endl;
          ofile << "\nZeroBufferSmooth   = " << ZbsHist << endl;
          ofile << "\nZeroBufferFit   = " << ZbfHist << endl;
          ofile << "\nZeroReverse   = " << ZrHist << endl;
          ofile << "\nZeroDark   = " << ZdHist << endl;
          ofile << "\nZeroDarkRate   = " << ZdrHist << endl;
          ofile << "\nGainLineDrift   = " << GldHist << endl;
          ofile << "\nGainNonLinearity   = " << GnlHist << endl;
          ofile << "\nGainChannelNormalize = " << GcnHist << endl;
          ofile << "\nGainFlatField   = " << GffHist << endl;
          ofile << "\nGainTemperature   = " << GtHist << endl;
          ofile << "\nGainUnitConversion = " << GucHist << endl;

          ofile.close();
        }
      }

      // Ensure the RadiometricCalibration group is out there
      const QString rcalGroup("RadiometricCalibration");
      if (!ocube->hasGroup(rcalGroup)) {
        PvlGroup temp(rcalGroup.toStdString());
        ocube->putGroup(temp);
      }

      PvlGroup &rcal = ocube->group(rcalGroup);
      rcal += PvlKeyword("Program", hical_program.toStdString());
      rcal += PvlKeyword("RunTime", hical_runtime.toStdString());
      rcal += PvlKeyword("Version",hical_version.toStdString());
      rcal += PvlKeyword("Revision",hical_revision.toStdString());

      PvlKeyword key("Conf", conf_file.toStdString());
      key.addCommentWrapped("/* " + hical_program.toStdString() + " application equation */");
      key.addComment("/* hdn = idn - ZeroBufferFit(ZeroBufferSmooth) */");
      key.addComment("/*           - ZeroReverse - (ZeroDark OR ZeroDarkRate) */");
      key.addComment("/* odn = hdn / GainLineDrift * GainNonLinearity */");
      key.addComment("/*           * GainChannelNormalize * GainFlatField */");
      key.addComment("/*           * GainTemperature / GainUnitConversion */");
      rcal += key;

      // Record parameter generation history.  Controllable in configuration
      // file.  Note this is optional because of a BUG!! in the ISIS label
      // writer as this application was initially developed
      if ( IsEqual(ConfKey(hiprof,"LogParameterHistory",QString("TRUE")),"TRUE")) {
        rcal += ZbsHist.makekey("ZeroBufferSmooth");
        rcal += ZbfHist.makekey("ZeroBufferFit");
        rcal += ZrHist.makekey("ZeroReverse");
        rcal += ZdHist.makekey("ZeroDark");
        rcal += ZdrHist.makekey("ZeroDarkRate");
        rcal += GldHist.makekey("GainLineDrift");
        rcal += GnlHist.makekey("GainNonLinearity");
        rcal += GcnHist.makekey("GainChannelNormalize");
        rcal += GffHist.makekey("GainFlatField");
        rcal += GtHist.makekey("GainTemperature");
        rcal += GucHist.makekey("GainUnitConversion");
      }

      p.EndProcess();
    }
    catch (IException &ie) {
      delete calVars;
      calVars = 0;
      std::string mess = "Failed in " + procStep;
      throw IException(ie, IException::User, mess.toLatin1().data(), _FILEINFO_);
    }

    // Clean up parameters
    delete calVars;
    calVars = 0;
  }
}