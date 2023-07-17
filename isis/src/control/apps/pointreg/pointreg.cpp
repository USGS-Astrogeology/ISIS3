/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <sys/resource.h>

#include "pointreg.h"

#include "Application.h"
#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "Camera.h"
#include "Chip.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "CubeManager.h"
#include "Pixel.h"
#include "Progress.h"
#include "SerialNumberList.h"
#include "UserInterface.h"
#include "Application.h"
#include "IException.h"
#include "iTime.h"

using namespace std;
namespace Isis {

  AutoReg *ar;
  AutoReg *validator;
  CubeManager *cubeMgr;
  SerialNumberList *files;
  QList<QString> *falsePositives;

  int ignored;
  int locked;
  int registered;
  int notintersected;
  int unregistered;

  bool logFalsePositives;
  bool revertFalsePositives;
  int expansion;
  double resTolerance;

  /**
   * @author 2012-04-06 Travis Addair
   *
   * @internal
   */
  class Validation {
    public:
      enum Result {
        Untested,
        Success,
        Failure,
        Skipped
      };

      Validation(QString test, ControlMeasure *held, ControlMeasure *registered,
          double tolerance) {

        m_test = test;
        m_pointId = registered->Parent()->GetId();
        m_heldId = FileName(
            files->fileName(held->GetCubeSerialNumber())).baseName();
        m_registeredId = FileName(
            files->fileName(registered->GetCubeSerialNumber())).baseName();

        m_aprioriSample = registered->GetSample();
        m_aprioriLine = registered->GetLine();
        m_shiftSample = 0.0;
        m_shiftLine = 0.0;

        m_resDiff = 0.0;
        m_resTolerance = 0.0;

        m_shift = 0.0;
        m_shiftTolerance = tolerance;

        m_result = Untested;
      }

      ~Validation() {}

      bool untested() const {
        return m_result == Untested;
      }

      bool succeeded() const {
        return m_result == Success;
      }

      bool failed() const {
        return m_result == Failure;
      }

      bool skipped() const {
        return m_result == Skipped;
      }

      QString resultString() const {
        switch (m_result) {
          case Untested: return "Untested";
          case Success: return "Success";
          case Failure: return "Failure";
          case Skipped: return "Skipped";
          default: throw IException(IException::Programmer,
                       "Unknown value for m_result", _FILEINFO_);
        }
      }

      void setValidity(bool valid) {
        m_result = valid ? Success : Failure;
      }

      void skip(QString testFailure) {
        m_test = testFailure;
        m_result = Skipped;
      }

      void compareResolutions(double heldResolution, double registeredResolution,
          double tolerance) {

        m_resDiff = fabs(heldResolution - registeredResolution);
        m_resTolerance = tolerance;

        if (m_resDiff > m_resTolerance) skip("Resolution Tolerance");
      }

      void compare(double shiftSample, double shiftLine) {
        m_shiftSample = shiftSample;
        m_shiftLine = shiftLine;

        double sampleShift = shiftSample - m_aprioriSample;
        double lineShift = shiftLine - m_aprioriLine;

        m_shift = sqrt(pow(sampleShift, 2) + pow(lineShift, 2));
        setValidity(m_shift <= m_shiftTolerance);
      }

      static QString getHeader() {
        return "Result,Test,PointID,HeldID,RegisteredID,"
            "Sample,Line,ShiftedSample,ShiftedLine,"
            "ResolutionDifference,ResolutionTolerance,"
            "Shift,ShiftTolerance";
      }

      QString toString() {
        stringstream stream;
        stream <<
          resultString() << "," << m_test << "," << m_pointId << "," <<
          m_heldId << "," << m_registeredId << "," <<
          m_aprioriSample << "," << m_aprioriLine << "," <<
          m_shiftSample << "," << m_shiftLine << "," <<
          m_resDiff << "," << m_resTolerance << "," <<
          m_shift << "," << m_shiftTolerance;
        return stream.str().c_str();
      }

    private:
      QString m_test;
      QString m_pointId;
      QString m_heldId;
      QString m_registeredId;

      double m_aprioriSample;
      double m_aprioriLine;
      double m_shiftSample;
      double m_shiftLine;

      double m_resDiff;
      double m_resTolerance;

      double m_shift;
      double m_shiftTolerance;

      Result m_result;
  };


  void registerPoint(ControlPoint *outPoint, ControlMeasure *patternCM,
      QString registerMeasures, bool outputFailed);
  void validatePoint(ControlPoint *point, ControlMeasure *reference,
      double shiftTolerance);
  Validation backRegister(ControlMeasure *measure, ControlMeasure *reference,
      double shiftTolerance);

  double getResolution(Cube &cube, ControlMeasure &measure);
  void verifyCube(Cube & cube);
  bool outputValue(ofstream &os, double value);
  int calcGoodMeasureCount(const ControlPoint *point);
  void printTemp();

  map<QString, void *> GuiHelpers() {
    map<QString, void *> helper;
    helper["PrintTemp"] = (void *) printTemp;
    return helper;
  }


  void pointreg(UserInterface &ui, Pvl *appLog) {
    // Initialize variables
    ar = NULL;
    validator = NULL;
    cubeMgr = NULL;
    falsePositives = NULL;

    ignored = 0;
    locked = 0;
    registered = 0;
    notintersected = 0;
    unregistered = 0;

    logFalsePositives = false;
    revertFalsePositives = false;

    if (ui.WasEntered("FALSEPOSITIVES")) {
      logFalsePositives = true;
      falsePositives = new QList<QString>;
    }

    // Determine which points/measures to register
    QString registerPoints = ui.GetString("POINTS");
    QString registerMeasures = ui.GetString("MEASURES");

    bool outputFailed = ui.GetBoolean("OUTPUTFAILED");
    bool outputIgnored = ui.GetBoolean("OUTPUTIGNORED");

    // Open the files list in a SerialNumberList for
    // reference by SerialNumber
    files = new SerialNumberList(ui.GetFileName("FROMLIST"));

    // Create the output ControlNet from the input file
    ControlNet outNet(ui.GetFileName("CNET"));

    if (outNet.GetNumPoints() <= 0) {
      QString msg = "Control network [" + ui.GetFileName("CNET") + "] ";
      msg += "contains no points";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    outNet.SetUserName(Application::UserName());

    // Create an AutoReg from the template file
    Pvl pvl(ui.GetFileName("DEFFILE"));
    ar = AutoRegFactory::Create(pvl);

    Progress progress;
    progress.SetText("Registering Points");
    progress.SetMaximumSteps(outNet.GetNumPoints());
    progress.CheckStatus();

    //  Get the maximum allowable number of open files
    struct rlimit limit;
    if (getrlimit(RLIMIT_NOFILE, &limit) != 0) {
      QString msg = "Cannot read the maximum allowable open files from system resources.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    //  Allow for library files, etc
    unsigned int maxOpenFiles = limit.rlim_cur * .60;


    cubeMgr = new CubeManager;
    cubeMgr->SetNumOpenCubes(maxOpenFiles);

    QString validate = ui.GetString("VALIDATE");
    if (validate != "SKIP") {
      validator = AutoRegFactory::Create(pvl);

      validator->SetTolerance(validator->MostLenientTolerance());
      validator->SetPatternZScoreMinimum(DBL_MIN);
      validator->SetPatternValidPercent(DBL_MIN);
      validator->SetSubsearchValidPercent(DBL_MIN);

      validator->SetSurfaceModelDistanceTolerance(validator->WindowSize());

      expansion = ui.WasEntered("SEARCH") ?
        ui.GetInteger("SEARCH") : validator->WindowSize();
      expansion *= 2;

      int patternSamples = validator->PatternChip()->Samples();
      int patternLines = validator->PatternChip()->Lines();
      validator->SearchChip()->SetSize(
          patternSamples + expansion, patternLines + expansion);

      revertFalsePositives = ui.GetBoolean("REVERT");
      resTolerance = ui.GetDouble("RESTOLERANCE");
    }

    // Register the points and create a new
    // ControlNet containing the refined measurements
    int i = 0;
    while (i < outNet.GetNumPoints()) {

      progress.CheckStatus();

      ControlPoint * outPoint = outNet.GetPoint(i);

      // Establish whether or not we want to attempt to register this point.
      bool wantToRegister = true;
      if (outPoint->IsIgnored()) {
        if (registerPoints == "NONIGNORED") wantToRegister = false;
      }
      else {
        if (registerPoints == "IGNORED") wantToRegister = false;
      }

      // Check if this is a point we wish to disregard.
      if (!wantToRegister) {
        // Keep track of how many ignored points we didn't register.
        if (outPoint->IsIgnored()) {
          ignored++;

          // If the point is ignored and the user doesn't want them, delete it
          if (!outputIgnored) {
            outNet.DeletePoint(i);
            continue;
          }
        }
      }
      else {  // "Ignore" or "valid" point to be registered
        if (outPoint->IsIgnored()) {
          outPoint->SetIgnored(false);
        }

        ControlMeasure * patternCM = outPoint->GetRefMeasure();

        // In case this is an implicit reference, make it explicit since we'll be
        // registering measures to it
        outPoint->SetRefMeasure(patternCM);

        if (validate != "ONLY") {
          registerPoint(outPoint, patternCM, registerMeasures, outputFailed);
        }
        if (validate != "SKIP") {
          validatePoint(outPoint, patternCM, ui.GetDouble("SHIFT"));
        }

        // Check to see if the control point has now been assigned
        // to "ignore".  If not, add it to the network. If so, only
        // add it to the output if the OUTPUTIGNORED parameter is selected
        // 2008-11-14 Jeannie Walldren
        if (outPoint->IsIgnored()) {
          ignored++;
          if (!outputIgnored) {
            outNet.DeletePoint(i);
            continue;
          }
        }
      }

      // The point wasn't deleted, so the network size is the same and we should
      // increment our index.
      i++;
    }

    // If flatfile was entered, create the flatfile
    // The flatfile is comma seperated and can be imported into an excel
    // spreadsheet
    if (ui.WasEntered("FLATFILE")) {
      QString fFile = FileName(ui.GetFileName("FLATFILE")).expanded();

      ofstream os;
      os.open(fFile.toLatin1().data(), ios::out);
      os <<
        "PointId,Filename,MeasureType,Reference,EditLock,Ignore,Registered," <<
        "OriginalMeasurementSample,OriginalMeasurementLine," <<
        "RegisteredMeasurementSample,RegisteredMeasurementLine,SampleShift," <<
        "LineShift,PixelShift,ZScoreMin,ZScoreMax,GoodnessOfFit" << endl;
      os << Null << endl;

      // Create a ControlNet from the original input file
      ControlNet inNet(ui.GetFileName("CNET"));

      for (int i = 0; i < outNet.GetNumPoints(); i++) {

        // get point from output control net and its
        // corresponding point from input control net
        const ControlPoint * outPoint = outNet.GetPoint(i);
        QString outPointId = outPoint->GetId();
        const ControlPoint * inPoint = inNet.GetPoint(outPointId);

        if (!outPoint->IsIgnored()) {
          for (int i = 0; i < outPoint->GetNumMeasures(); i++) {

            // Get measure and find its corresponding measure from input net
            const ControlMeasure * cmTrans = outPoint->GetMeasure(i);
            const ControlMeasure * cmOrig =
              inPoint->GetMeasure((QString) cmTrans->GetCubeSerialNumber());

            QString pointId = outPoint->GetId();

            QString fullName = files->fileName(cmTrans->GetCubeSerialNumber());
            QString filename = FileName(fullName).baseName();

            QString measureType = cmTrans->MeasureTypeToString(
                cmTrans->GetType());
            QString reference = outPoint->GetRefMeasure() == cmTrans ?
              "true" : "false";
            QString editLock = cmTrans->IsEditLocked() ? "true" : "false";
            QString ignore = cmTrans->IsIgnored() ? "true" : "false";
            QString registered =
              !cmOrig->IsRegistered() && cmTrans->IsRegistered() ?
              "true" : "false";

            double inSamp = cmOrig->GetSample();
            double inLine = cmOrig->GetLine();

            double outSamp = cmTrans->GetSample();
            double outLine = cmTrans->GetLine();

            os <<
              pointId << "," <<
              filename << "," <<
              measureType << "," <<
              reference << "," <<
              editLock << "," <<
              ignore << "," <<
              registered << "," <<
              inSamp << "," <<
              inLine << "," <<
              outSamp << "," <<
              outLine;

            double sampleShift = cmTrans->GetSampleShift();
            double lineShift = cmTrans->GetLineShift();
            double pixelShift = cmTrans->GetPixelShift();

            outputValue(os, sampleShift);
            outputValue(os, lineShift);
            outputValue(os, pixelShift);

            double zScoreMin = cmTrans->GetLogData(
                ControlMeasureLogData::MinimumPixelZScore).GetNumericalValue();
            double zScoreMax = cmTrans->GetLogData(
                ControlMeasureLogData::MaximumPixelZScore).GetNumericalValue();
            double goodnessOfFit = cmTrans->GetLogData(
                ControlMeasureLogData::GoodnessOfFit).GetNumericalValue();

            outputValue(os, zScoreMin);
            outputValue(os, zScoreMax);
            outputValue(os, goodnessOfFit);

            os << endl;
          }
        }
      }
    }

    if (logFalsePositives) {
      QString filename = FileName(ui.GetFileName("FALSEPOSITIVES")).expanded();
      ofstream os;
      os.open(filename.toLatin1().data(), ios::out);

      os << Validation::getHeader() << endl;
      for (int i = 0; i < falsePositives->size(); i++) {
        os << (*falsePositives)[i].toStdString() << endl;
      }

      os.close();
    }

    PvlGroup pLog("Points");
    pLog += PvlKeyword("Total", toString(outNet.GetNumPoints()));
    pLog += PvlKeyword("Ignored", toString(ignored));
    Application::Log(pLog);

    PvlGroup mLog("Measures");
    mLog += PvlKeyword("Locked", toString(locked));
    mLog += PvlKeyword("Registered", toString(registered));
    mLog += PvlKeyword("NotIntersected", toString(notintersected));
    mLog += PvlKeyword("Unregistered", toString(unregistered));
    Application::Log(mLog);

    // Log Registration Statistics
    Pvl arPvl = ar->RegistrationStatistics();

    for (int i = 0; i < arPvl.groups(); i++) {
      Application::Log(arPvl.group(i));
    }

    // add the auto registration information to print.prt
    PvlGroup autoRegTemplate = ar->RegTemplate();
    Application::Log(autoRegTemplate);

    if (validator) {
      PvlGroup validationGroup("ValidationStatistics");

      Pvl validationPvl = validator->RegistrationStatistics();
      for (int g = 0; g < validationPvl.groups(); g++) {
        PvlGroup &group = validationPvl.group(g);
        if (group.keywords() > 0) {
          group[0].addComment(group.name());
          for (int k = 0; k < group.keywords(); k++)
            validationGroup.addKeyword(group[k]);
        }
      }

      Application::Log(validationGroup);

      PvlGroup validationTemplate = validator->UpdatedTemplate();
      validationTemplate.setName("ValidationTemplate");
      Application::Log(validationTemplate);
    }

    outNet.Write(ui.GetFileName("ONET"));

    delete ar;
    ar = NULL;

    delete validator;
    validator = NULL;

    delete cubeMgr;
    cubeMgr = NULL;

    delete files;
    files = NULL;

    delete falsePositives;
    falsePositives = NULL;
  }


  void registerPoint(ControlPoint *outPoint, ControlMeasure *patternCM,
      QString registerMeasures, bool outputFailed) {

    Cube &patternCube = *cubeMgr->OpenCube(
        files->fileName(patternCM->GetCubeSerialNumber()));

    ar->PatternChip()->TackCube(patternCM->GetSample(), patternCM->GetLine());
    ar->PatternChip()->Load(patternCube);

    if (patternCM->IsEditLocked()) {
      locked++;
    }

    if (outPoint->GetRefMeasure() != patternCM) {
      outPoint->SetRefMeasure(patternCM);
    }

    // Register all the unlocked measurements
    int j = 0;
    while (j < outPoint->GetNumMeasures()) {
      if (j != outPoint->IndexOfRefMeasure()) {

        ControlMeasure * measure = outPoint->GetMeasure(j);
        if (measure->IsEditLocked()) {
          // If the measurement is locked, keep it as is and go to next measure
          locked++;
        }
        else if (!measure->IsMeasured() || registerMeasures != "CANDIDATES") {

          // refresh pattern cube pointer to ensure it stays valid
          Cube &patternCube = *cubeMgr->OpenCube(files->fileName(
                patternCM->GetCubeSerialNumber()));
          Cube &searchCube = *cubeMgr->OpenCube(files->fileName(
                measure->GetCubeSerialNumber()));

          ar->SearchChip()->TackCube(measure->GetSample(), measure->GetLine());

          verifyCube(patternCube);
          verifyCube(searchCube);

          try {
            ar->SearchChip()->Load(searchCube, *(ar->PatternChip()), patternCube);

            // If the measurements were correctly registered
            // Write them to the new ControlNet
            AutoReg::RegisterStatus res = ar->Register();
            searchCube.clearIoCache();
            patternCube.clearIoCache();

            double score1, score2;
            ar->ZScores(score1, score2);

            // Set the minimum and maximum z-score values for the measure
            measure->SetLogData(ControlMeasureLogData(
                  ControlMeasureLogData::MinimumPixelZScore, score1));
            measure->SetLogData(ControlMeasureLogData(
                  ControlMeasureLogData::MaximumPixelZScore, score2));

            if (ar->Success()) {
              // Check to make sure the newly calculated measure position is on
              // the surface of the planet
              Camera *cam = searchCube.camera();
              bool foundLatLon = cam->SetImage(ar->CubeSample(), ar->CubeLine());

              if (foundLatLon) {
                registered++;

                if (res == AutoReg::SuccessSubPixel) {
                  measure->SetType(ControlMeasure::RegisteredSubPixel);
                }
                else {
                  measure->SetType(ControlMeasure::RegisteredPixel);
                }

                measure->SetLogData(ControlMeasureLogData(
                      ControlMeasureLogData::GoodnessOfFit,
                      ar->GoodnessOfFit()));

                measure->SetAprioriSample(measure->GetSample());
                measure->SetAprioriLine(measure->GetLine());
                measure->SetCoordinate(ar->CubeSample(), ar->CubeLine());
                measure->SetIgnored(false);

                // We successfully registered the current measure to the
                // reference, and since we set the current measure to be
                // unignored, it follows that its reference should also be made
                // unignored.
                patternCM->SetIgnored(false);
              }
              else {
                notintersected++;

                if (outputFailed) {
                  measure->SetType(ControlMeasure::Candidate);
                  measure->SetIgnored(true);
                }
                else {
                  outPoint->Delete(j);
                  continue;
                }
              }
            }
            // Else use the original marked as "Candidate"
            else {
              unregistered++;

              if (outputFailed) {
                measure->SetType(ControlMeasure::Candidate);

                if (res == AutoReg::FitChipToleranceNotMet) {
                  measure->SetLogData(ControlMeasureLogData(
                        ControlMeasureLogData::GoodnessOfFit,
                        ar->GoodnessOfFit()));
                }
                measure->SetIgnored(true);
              }
              else {
                outPoint->Delete(j);
                continue;
              }
            }
          }
          catch (IException &e) {
            unregistered++;

            if (outputFailed) {
              measure->SetType(ControlMeasure::Candidate);
              measure->SetIgnored(true);
            }
            else {
              outPoint->Delete(j);
              continue;
            }
          }
        }
      }

      // If we made it here (without continuing on to the next measure),
      // then the measure wasn't deleted and we should therefore increment
      // the index of the measure we're looking at.
      j++;
    }

    // Jeff Anderson put in this test (Dec 2, 2008) to allow for control
    // points to be good so long as at least two measure could be
    // registered. When a measure can't be registered to the reference then
    // that measure is set to be ignored where in the past the whole point
    // was ignored
    if (calcGoodMeasureCount(outPoint) < 2 &&
        outPoint->GetType() != ControlPoint::Fixed) {
      outPoint->SetIgnored(true);
    }

    // Otherwise, ignore=false. This is already set at the beginning of the
    // registration process
  }


  void validatePoint(ControlPoint *point, ControlMeasure *reference,
      double shiftTolerance) {

    for (int i = 0; i < point->GetNumMeasures(); i++) {
      if (i != point->IndexOfRefMeasure()) {
        ControlMeasure *measure = point->GetMeasure(i);
        if (measure->IsMeasured() && !measure->IsEditLocked()) {
          Validation validation = backRegister(
              reference, measure, shiftTolerance);

          // If the validation failed, or we were unable to perform the validation
          // due to registration errors, we consider this registration to be a
          // false positive
          if (validation.failed() || validation.untested()) {
            if (revertFalsePositives) {
              measure->SetType(ControlMeasure::Candidate);
              measure->SetCoordinate(
                  measure->GetAprioriSample(), measure->GetAprioriLine());
              measure->SetIgnored(true);
              // TODO remove log data here
            }
          }

          // If the registration did not succeed for whatever reason (untested,
          // failed, or skipped due to incompatible data), log the result
          if (logFalsePositives) {
            if (!validation.succeeded()) {
              falsePositives->append(
                  validation.toString());
            }
          }
        }
      }
    }
  }


  Validation backRegister(ControlMeasure *reference, ControlMeasure *measure,
      double shiftTolerance) {

    Validation validation(
        "Back-Registration", measure, reference, shiftTolerance);

    Cube &patternCube = *cubeMgr->OpenCube(files->fileName(
          measure->GetCubeSerialNumber()));
    Cube &searchCube = *cubeMgr->OpenCube(files->fileName(
          reference->GetCubeSerialNumber()));

    double patternRes = getResolution(patternCube, *measure);
    double searchRes = getResolution(searchCube, *reference);
    validation.compareResolutions(patternRes, searchRes, resTolerance);

    if (validation.skipped())
      return validation;

    validator->SearchChip()->TackCube(
        reference->GetSample(), reference->GetLine());
    validator->PatternChip()->TackCube(measure->GetSample(), measure->GetLine());
    validator->PatternChip()->Load(patternCube);

    verifyCube(patternCube);
    verifyCube(searchCube);

    try {
      validator->SearchChip()->Load(
          searchCube, *(validator->PatternChip()), patternCube);

      // If the measurements were correctly registered
      // Write them to the new ControlNet
      validator->Register();
      searchCube.clearIoCache();
      patternCube.clearIoCache();

      if (validator->Success()) {
        // Check to make sure the newly calculated measure position is on
        // the surface of the planet
        Camera *cam = searchCube.camera();
        bool foundLatLon = cam->SetImage(
            validator->CubeSample(), validator->CubeLine());

        if (foundLatLon) {
          validation.compare(
              validator->CubeSample(), validator->CubeLine());
        }
      }
    }
    catch (IException &e) {
    }

    return validation;
  }


  double getResolution(Cube &cube, ControlMeasure &measure) {
    // TODO retrieve for projection
    Camera *camera = cube.camera();
    camera->SetImage(measure.GetSample(), measure.GetLine());
    return camera->PixelResolution();
  }


  // Verify a cube has either a Camera or a Projection, throw an exception if not
  void verifyCube(Cube & cube) {
    try {
      cube.camera();
    }
    catch (IException &camError) {
      try {
        cube.projection();
      }
      catch (IException &projError) {
        projError.append(camError);
        throw projError;
      }
    }
  }


  bool outputValue(ofstream &os, double value) {
    os << ",";

    if (fabs(value) > DBL_EPSILON && value != Null) {
      os << value;
      return true;
    }
    else {
      os << "NA";
      return false;
    }
  }


  int calcGoodMeasureCount(const ControlPoint *point) {
    int goodMeasureCount = 0;
    for (int i = 0; i < point->GetNumMeasures(); i++) {
      const ControlMeasure *measure = point->GetMeasure(i);
      if (!measure->IsIgnored()) goodMeasureCount++;
    }

    return goodMeasureCount;
  }


  // Helper function to print out template to session log
  void printTemp() {
    UserInterface &ui = Application::GetUserInterface();

    // Get template pvl
    Pvl userTemp;
    userTemp.read(ui.GetFileName("DEFFILE"));

    //Write template file out to the log
    Isis::Application::GuiLog(userTemp);
  }
}
