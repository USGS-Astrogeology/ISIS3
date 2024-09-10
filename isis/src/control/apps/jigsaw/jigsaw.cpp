/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <highfive/H5File.hpp>
#include <highfive/H5DataType.hpp>
#include <highfive/H5DataSet.hpp>
#include <highfive/H5Group.hpp>
#include <vector>

#include <QDir>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QString>

#include "Application.h"
#include "Blob.h"
#include "BundleAdjust.h"
#include "BundleObservationSolveSettings.h"
#include "BundleResults.h"
#include "BundleSettings.h"
#include "BundleSolutionInfo.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "iTime.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Process.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "Table.h"

#include "jigsaw.h"

using namespace std;
using namespace HighFive;


namespace Isis {

  BundleSettingsQsp bundleSettings(UserInterface &ui);
  void checkImageList(SerialNumberList &heldSerialList, SerialNumberList &cubeSerialList);
  QList<BundleObservationSolveSettings> observationSolveSettings(UserInterface &ui);
  ControlNetQsp fixHeldImages(const QString &cnetFile,
                              const QString &heldList,
                              const QString &imageList);

  void jigsaw(UserInterface &ui, Pvl *log) {

    QString cubeList = ui.GetFileName("FROMLIST");

    // Check for ADJUSTMENT_INPUT file and apply
    try {
      if (ui.WasEntered("ADJUSTMENT_INPUT")) {
        
        // Set default values
        ui.Clear("TWIST");
        ui.PutBoolean("TWIST", false);
        ui.PutBoolean("BUNDLEOUT_TXT", false);
        ui.PutBoolean("IMAGESCSV", false);
        ui.PutBoolean("OUTPUT_CSV", false);
        ui.PutBoolean("RESIDUALS_CSV", false);
        ui.PutBoolean("LIDAR_CSV", false);
        ui.PutBoolean("OUTADJUSTMENTH5", false);
        
        File fileRead(ui.GetFileName("ADJUSTMENT_INPUT").toStdString(), File::ReadOnly);
        SerialNumberList *snList = new SerialNumberList(cubeList);

        QString jigApplied = "JigApplied = " + Isis::iTime::CurrentLocalTime();

        for (int i = 0; i < snList->size(); i++) {
          Process p;
          CubeAttributeInput inAtt;
          Cube *c = p.SetInputCube(snList->fileName(i), inAtt, ReadWrite);

          if (c->hasBlob("CSMState", "String")) {
            QString msg = "Unable to apply bundle adjustment values to cubes with CSMState blobs.";
            throw IException(IException::User, msg, _FILEINFO_);
          }

          QString serialNumber = snList->serialNumber(i);
          QString cmatrixName = "InstrumentPointing";
          QString spvectorName = "InstrumentPosition";

          std::string cmatrixKey = serialNumber.toStdString() + "/" + cmatrixName.toStdString();
          std::string spvectorKey = serialNumber.toStdString() + "/" + spvectorName.toStdString();

          // Read h5 into table
          DataSet datasetRead = fileRead.getDataSet(cmatrixKey);
          auto cmatrixData = datasetRead.read<std::string>();
          Table cmatrixTable(cmatrixName, cmatrixData, ',');

          datasetRead = fileRead.getDataSet(spvectorKey);
          auto spvectorData = datasetRead.read<std::string>();
          Table spvectorTable(spvectorName, spvectorData, ',');

          // Write bundle adjustment values out
          cmatrixTable.Label().addComment(jigApplied);
          c->write(cmatrixTable);
          spvectorTable.Label().addComment(jigApplied);
          c->write(spvectorTable);

          p.WriteHistory(*c);
        }

        if (log) {
          PvlGroup gp("JigsawResults");
          
          gp += PvlKeyword("Status", "Bundle adjustment values from [" + ui.GetFileName("ADJUSTMENT_INPUT") 
                            + "] were applied to the cubes in [" + cubeList+ "]");
          Application::AppendAndLog(gp, log);
        }

        return;
      }
    } catch (IException &e) {
      QString msg = "Unable to apply bundle adjustment values from [" + ui.GetFileName("ADJUSTMENT_INPUT") + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    // Check to make sure user entered something to adjust... Or can just points be in solution?
    // YES - we should be able to just TRIANGULATE the points in the control net
    // right now to do this we have to fake out jigsaw by
    // 1) solving for both position and pointing but giving them high weights or
    // 2) solving for either position OR pointing but giving them high weights (the one not solved for
    //    is effectively "fixed" also)
    if (ui.GetString("CAMSOLVE") == "NONE"  &&  ui.GetString("SPSOLVE") == "NONE") {
      string msg = "Must either solve for camera pointing or spacecraft position";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    QString cnetFile = ui.GetFileName("CNET");

    // retrieve settings from jigsaw gui
    BundleSettingsQsp settings = bundleSettings(ui);
    if(settings->bundleTargetBody()) {
      if(settings->bundleTargetBody()->solveTriaxialRadii() ||
         settings->bundleTargetBody()->solveMeanRadius()) {
        PvlGroup radiusSolveWarning("RadiusSolveWarning");
        radiusSolveWarning.addKeyword(PvlKeyword("Warning", "The radii solve is currently \
                                                   under review and is likely resulting \
                                                   in addition error in the bundle adjust. \
                                                   We recommend that you do not solve for radii at this moment."));
        Application::Log(radiusSolveWarning);
      }
    }
    settings->setCubeList(cubeList);

    BundleAdjust *bundleAdjustment = NULL;
    try {
      // Get the held list if entered and prep for bundle adjustment
      if (ui.WasEntered("HELDLIST")) {
        QString heldList = ui.GetFileName("HELDLIST");
        // Update the control network so that any control points intersecting a held image are fixed
        ControlNetQsp cnet = fixHeldImages(cnetFile, heldList, cubeList);
        bundleAdjustment = new BundleAdjust(settings, cnet, cubeList);
      }
    else if (ui.WasEntered("LIDARDATA")) {
      QString lidarFile = ui.GetFileName("LIDARDATA");

      // validate lidar point file exists
      if (!QFile::exists(lidarFile)) {
        string msg = "Input lidar point file does not exist";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      bundleAdjustment = new BundleAdjust(settings, cnetFile, cubeList, lidarFile);
    }
      else {
        bundleAdjustment = new BundleAdjust(settings, cnetFile, cubeList);
      }
    }
    catch (IException &e) {
      throw;
    }

    // Bundle adjust the network
    try {

      QObject::connect( bundleAdjustment, SIGNAL( statusUpdate(QString) ),
                        bundleAdjustment, SLOT( outputBundleStatus(QString) ) );
      BundleSolutionInfo *bundleSolution = bundleAdjustment->solveCholeskyBR();
      bundleSolution->setOutputControlName( QString::fromStdString(FileName(ui.GetFileName("ONET").toStdString()).expanded()) );
      cout << "\nGenerating report files\n" << endl;

      // write output files
      if (ui.GetBoolean("BUNDLEOUT_TXT")) {
        bundleSolution->outputText();
      }

      if (ui.GetBoolean("IMAGESCSV")) {
        bundleSolution->outputImagesCSV();
      }

      if (ui.GetBoolean("OUTPUT_CSV")) {
        bundleSolution->outputPointsCSV();
      }
      if (ui.GetBoolean("RESIDUALS_CSV")) {
        bundleSolution->outputResiduals();
      }

      // write lidar csv output file
      if (ui.GetBoolean("LIDAR_CSV")) {
        bundleSolution->outputLidarCSV();
      }

      // write updated control net
      bundleAdjustment->controlNet()->Write(ui.GetFileName("ONET"));

    // write updated lidar data file
    if (ui.WasEntered("LIDARDATA")) {
      if (ui.GetString("OLIDARFORMAT") == "JSON") {
        bundleAdjustment->lidarData()->write(ui.GetFileName("OLIDARDATA").toStdString(),LidarData::Format::Json);
      }
      else {
        bundleAdjustment->lidarData()->write(ui.GetFileName("OLIDARDATA").toStdString(),LidarData::Format::Binary);
      }
    }
      PvlGroup gp("JigsawResults");
      QString jigComment = "Jigged = " + Isis::iTime::CurrentLocalTime();

      std::string outputFilePrefix = settings->outputFilePrefix().toStdString();

      // ALWAYS* WRITE OUT ADJUSTMENT VALUES
      // Do NOT write out for cubes w/ CSMState (TODO)
      if (ui.GetBoolean("OUTADJUSTMENTH5")) {
        std::string adjustmentFilename = outputFilePrefix + "adjustment_out.h5";
        
        File file(adjustmentFilename, File::Truncate);

        for (int i = 0; i < bundleAdjustment->numberOfImages(); i++) {
          Process p;
          CubeAttributeInput inAtt;
          Cube *c = p.SetInputCube(bundleAdjustment->fileName(i), inAtt, 0); // 0 for read only

          // Only for ISIS adjustment values
          if (!c->hasBlob("CSMState", "String")) {
            Table cmatrix = bundleAdjustment->cMatrix(i);
            Table spvector = bundleAdjustment->spVector(i);

            QString serialNumber = bundleAdjustment->serialNumberList()->serialNumber(i);
            QString cmatrixName = cmatrix.Name();
            QString spvectorName = spvector.Name();

            std::string cmatrixKey = serialNumber.toStdString() + "/" + cmatrixName.toStdString();
            std::string spvectorKey = serialNumber.toStdString() + "/" + spvectorName.toStdString();

            // Save bundle adjustment values to HDF5 file
            std::string cmatrixTableStr = Table::toString(cmatrix).toStdString();
            DataSet dataset = file.createDataSet<std::string>(cmatrixKey, cmatrixTableStr);
            std::string spvectorTableStr = Table::toString(spvector).toStdString();
            dataset = file.createDataSet<std::string>(spvectorKey, spvectorTableStr);
          }
        }
        file.flush();
      }

      // Update the cube pointing if requested but ONLY if bundle has converged
      if (ui.GetBoolean("UPDATE") ) {
        if ( !bundleAdjustment->isConverged() ) {
          gp += PvlKeyword("Status","Bundle did not converge, camera pointing NOT updated");
          std::string msg = "Bundle did not converge within MAXITS [" + toString(ui.GetInteger("MAXITS")) + "] iterations [" + cnetFile.toStdString() +  "]";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        else {

          // Loop through images
          for (int i = 0; i < bundleAdjustment->numberOfImages(); i++) {
            Process p;
            CubeAttributeInput inAtt;
            Cube *c = p.SetInputCube(bundleAdjustment->fileName(i), inAtt, ReadWrite);
            //check for existing polygon, if exists delete it
            if (c->label()->hasObject("Polygon")) {
              c->label()->deleteObject("Polygon");
            }

            // check for CameraStatistics Table, if exists, delete
            for (int iobj = 0; iobj < c->label()->objects(); iobj++) {
              PvlObject obj = c->label()->object(iobj);
              if (obj.name() != "Table") continue;
              if (obj["Name"][0] != "CameraStatistics") continue;
              c->label()->deleteObject(iobj);
              break;
            }

            // Only apply adjustment_input values for non-CSM for now
            if (c->hasBlob("CSMState", "String")) {
              Blob csmStateBlob("CSMState", "String");
              // Read the BLOB from the cube to propagate things like the model
              // and plugin name
              c->read(csmStateBlob);
              std::string modelState = bundleAdjustment->modelState(i).toStdString();
              csmStateBlob.setData(modelState.c_str(), modelState.size());
              csmStateBlob.Label().addComment(jigComment.toStdString());
              c->write(csmStateBlob);
            } else {
              // Write bundle adjustment values to cube
              Table cmatrix = bundleAdjustment->cMatrix(i);
              cmatrix.Label().addComment(jigComment.toStdString());
              Table spvector = bundleAdjustment->spVector(i);
              spvector.Label().addComment(jigComment.toStdString());
              c->write(cmatrix);
              c->write(spvector);
            }

            p.WriteHistory(*c);
          }
          gp += PvlKeyword("Status", "Camera pointing updated");
        }
      }
      else {
        gp += PvlKeyword("Status", "Camera pointing NOT updated");
      }

      Pvl summary;
      std::istringstream iss (bundleAdjustment->iterationSummaryGroup().toStdString());
      iss >> summary;

      for (auto grpIt = summary.beginGroup(); grpIt!= summary.endGroup(); grpIt++) {
        Application::Log(*grpIt);
      }

      Application::Log(gp);

      delete bundleSolution;
    }
    catch(IException &e) {
      bundleAdjustment->controlNet()->Write(ui.GetFileName("ONET"));
      std::string msg = "Unable to bundle adjust network [" + cnetFile.toStdString() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

  //TODO - WHY DOES THIS MAKE VALGRIND ANGRY???
    delete bundleAdjustment;
  }

  BundleSettingsQsp bundleSettings(UserInterface &ui) {
  //  BundleSettings settings;
    BundleSettingsQsp settings = BundleSettingsQsp(new BundleSettings);

    settings->setValidateNetwork(true);

    // solve options
    QString coordTypeBundleStr = ui.GetString("CONTROL_POINT_COORDINATE_TYPE_BUNDLE");
    QString coordTypeReportsStr = ui.GetString("CONTROL_POINT_COORDINATE_TYPE_REPORTS");
    SurfacePoint::CoordinateType ctypeBundle = SurfacePoint::Latitudinal;
    SurfacePoint::CoordinateType ctypeReports = SurfacePoint::Latitudinal;

    if (coordTypeBundleStr == "RECTANGULAR") {
      ctypeBundle = SurfacePoint::Rectangular;
    }

    if (coordTypeReportsStr == "RECTANGULAR") {
      ctypeReports = SurfacePoint::Rectangular;
    }

    double coord1Sigma  = Isis::Null;
    double coord2Sigma = Isis::Null;
    double coord3Sigma    = Isis::Null;
    if (ui.WasEntered("POINT_LATITUDE_SIGMA")) {
      coord1Sigma = ui.GetDouble("POINT_LATITUDE_SIGMA");
    }
    if (ui.WasEntered("POINT_LONGITUDE_SIGMA")) {
      coord2Sigma = ui.GetDouble("POINT_LONGITUDE_SIGMA");
    }
    if (ui.WasEntered("POINT_RADIUS_SIGMA")) {
      coord3Sigma = ui.GetDouble("POINT_RADIUS_SIGMA");
    }
    if (ui.WasEntered("POINT_X_SIGMA")) {
      coord1Sigma = ui.GetDouble("POINT_X_SIGMA");
    }
    if (ui.WasEntered("POINT_Y_SIGMA")) {
      coord2Sigma = ui.GetDouble("POINT_Y_SIGMA");
    }
    if (ui.WasEntered("POINT_Z_SIGMA")) {
      coord3Sigma = ui.GetDouble("POINT_Z_SIGMA");
    }

    settings->setSolveOptions(ui.GetBoolean("OBSERVATIONS"),
                              ui.GetBoolean("UPDATE"),
                              ui.GetBoolean("ERRORPROPAGATION"),
                              ui.GetBoolean("RADIUS"),
                              ctypeBundle, ctypeReports,
                              coord1Sigma,
                              coord2Sigma,
                              coord3Sigma);

    // Don't create the inverse correlation matrix file
    settings->setCreateInverseMatrix(false);

    settings->setOutlierRejection(ui.GetBoolean("OUTLIER_REJECTION"),
                                 ui.GetDouble("REJECTION_MULTIPLIER"));

    QList<BundleObservationSolveSettings> solveSettingsList = observationSolveSettings(ui);
    settings->setObservationSolveOptions(solveSettingsList);
    // convergence criteria
    settings->setConvergenceCriteria(BundleSettings::Sigma0,
                                    ui.GetDouble("SIGMA0"),
                                    ui.GetInteger("MAXITS"));

    // max likelihood estimation
    if (ui.GetString("MODEL1").compare("NONE") != 0) {
      // if model1 is not "NONE", add to the models list with its quantile
      settings->addMaximumLikelihoodEstimatorModel(
          MaximumLikelihoodWFunctions::stringToModel(ui.GetString("MODEL1")),
              ui.GetDouble("MAX_MODEL1_C_QUANTILE"));

      if (ui.GetString("MODEL2").compare("NONE") != 0) {
        // if model2 is not "NONE", add to the models list with its quantile
        settings->addMaximumLikelihoodEstimatorModel(
            MaximumLikelihoodWFunctions::stringToModel(ui.GetString("MODEL2")),
                ui.GetDouble("MAX_MODEL2_C_QUANTILE"));

        if (ui.GetString("MODEL3").compare("NONE") != 0) {
          // if model3 is not "NONE", add to the models list with its quantile
          settings->addMaximumLikelihoodEstimatorModel(
              MaximumLikelihoodWFunctions::stringToModel(ui.GetString("MODEL3")),
                  ui.GetDouble("MAX_MODEL3_C_QUANTILE"));
        }
      }
    }

    // target body options
    if (ui.GetBoolean("SOLVETARGETBODY") == true) {
      PvlObject obj;
      ui.GetFileName("TBPARAMETERS");
      Pvl tbParPvl(FileName(ui.GetFileName("TBPARAMETERS").toStdString()).expanded());
      if (!tbParPvl.hasObject("Target")) {
        std::string msg = "Input Target parameters file missing main Target object";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // read target body pvl file into BundleTargetBody object
      BundleTargetBodyQsp bundleTargetBody = BundleTargetBodyQsp(new BundleTargetBody);

      obj = tbParPvl.findObject("Target");
      bundleTargetBody->readFromPvl(obj);

      // ensure user entered something to adjust
      if (bundleTargetBody->numberParameters() == 0) {
        string msg = "Must solve for at least one target body option";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      settings->setBundleTargetBody(bundleTargetBody);
    }

    // output options
    QString outputfileprefix = "";
    if (ui.WasEntered("FILE_PREFIX"))  {
      outputfileprefix = ui.GetString("FILE_PREFIX");
      int length = (outputfileprefix.length()) - 1;
      QChar endvalue = outputfileprefix[length];
      QString check = "/";
      if (endvalue != check) {
        outputfileprefix += "_";
      }
    }
    settings->setOutputFilePrefix(outputfileprefix);

    return settings;
  }


  /**
   * Checks that all the first serial numbers are in the second serial numbers list (FROMLIST).
   *
   * Note: This function is used for verifying HELDLIST is in the FROMLIST.
   *
   * @param imageList List of image serial numbers to check to make sure they're in the second list.
   * @param fromList List of input image serial numbers to check against.
   *
   * @throws IException::User "The following images are not in the FROMLIST:"
   */
  void checkImageList(SerialNumberList &imageList, SerialNumberList &fromList) {
    // Keep track of which held images are not in the FROMLIST
    QString imagesNotFound;

    for (int img = 0; img < imageList.size(); img++) {
      // When the FROMLIST does not have the current image, record the image's filename
      if ( !fromList.hasSerialNumber(imageList.serialNumber(img)) ) {
        imagesNotFound += " [" + imageList.fileName(img) + "]";
      }
    }

    // Inform the user which images are not in the second list
    if (!imagesNotFound.isEmpty()) {
      std::string msg = "The following images are not in the FROMLIST:";
      msg += imagesNotFound.toStdString() + ".";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  QList<BundleObservationSolveSettings> observationSolveSettings(UserInterface &ui) {
    //************************************************************************************************
    QList<BundleObservationSolveSettings> observationSolveSettingsList;
    QString fromList = ui.GetFileName("FROMLIST");
    SerialNumberList cubeSNs(fromList);
    // QVector<BundleObservationSolveSettings*> cubeSolveSettings(cubeSNs.size(), nullptr);

    if (ui.WasEntered("SCCONFIG")) {
      PvlObject obj;
      Pvl scConfig(FileName(ui.GetFileName("SCCONFIG").toStdString()).expanded());
      // QMap<QString, BundleObservationSolveSettings*> instIDtoBOSS;
      if (!scConfig.hasObject("SensorParameters")) {
        std::string msg = "Input SCCONFIG file missing SensorParameters object";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // loop over parameter groups, read settings for each sensor into a
      // BundleObservationSolveSettings object, and append to observationSolveSettingsList
      obj = scConfig.findObject("SensorParameters");
      PvlObject::PvlGroupIterator g;
      for(g = obj.beginGroup(); g != obj.endGroup(); ++g) {
        BundleObservationSolveSettings solveSettings(*g);
        observationSolveSettingsList.append(solveSettings);
      }

      // loop through serial number list, check if current instrumentID matches
      // any of the BOSS's instrumentID. If so, add observationNumber to BOSS.
      for (int snIndex = 0; snIndex < cubeSNs.size(); snIndex++) {
        QString snInstId = cubeSNs.spacecraftInstrumentId(snIndex);
        bool found = false;
        for (auto bossIt = observationSolveSettingsList.begin(); bossIt != observationSolveSettingsList.end(); bossIt++) {
          if (bossIt->instrumentId() == snInstId) {
            bossIt->addObservationNumber(cubeSNs.observationNumber(snIndex));
            found = true;
          }
        }
        if (!found){
          std::string msg = "No BundleObservationSolveSettings found for " + snInstId.toStdString();
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
    }

    else {
      // We are not using the PVL, so get what will be solve settings for all images from gui
      BundleObservationSolveSettings observationSolveSettings;

      BundleObservationSolveSettings::InstrumentPointingSolveOption pointingSolveOption =
          BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
              ui.GetString("CAMSOLVE"));

      double anglesAprioriSigma, angularVelocityAprioriSigma, angularAccelerationAprioriSigma;
      anglesAprioriSigma = angularVelocityAprioriSigma = angularAccelerationAprioriSigma = Isis::Null;
      if (ui.WasEntered("CAMERA_ANGLES_SIGMA")) {
        anglesAprioriSigma = ui.GetDouble("CAMERA_ANGLES_SIGMA");
      }
      if (ui.WasEntered("CAMERA_ANGULAR_VELOCITY_SIGMA")) {
        angularVelocityAprioriSigma = ui.GetDouble("CAMERA_ANGULAR_VELOCITY_SIGMA");
      }
      if (ui.WasEntered("CAMERA_ANGULAR_ACCELERATION_SIGMA")) {
        angularAccelerationAprioriSigma = ui.GetDouble("CAMERA_ANGULAR_ACCELERATION_SIGMA");
      }

      observationSolveSettings.setInstrumentPointingSettings(pointingSolveOption,
                                                              ui.GetBoolean("TWIST"),
                                                              ui.GetInteger("CKDEGREE"),
                                                              ui.GetInteger("CKSOLVEDEGREE"),
                                                              ui.GetBoolean("OVEREXISTING"),
                                                              anglesAprioriSigma,
                                                              angularVelocityAprioriSigma,
                                                              angularAccelerationAprioriSigma);

      BundleObservationSolveSettings::InstrumentPositionSolveOption positionSolveOption =
          BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
              ui.GetString("SPSOLVE"));

      double positionAprioriSigma, positionVelocityAprioriSigma, positionAccelerationAprioriSigma;
      positionAprioriSigma = positionVelocityAprioriSigma = positionAccelerationAprioriSigma
                            = Isis::Null;
      if ( ui.WasEntered("SPACECRAFT_POSITION_SIGMA") ) {
        positionAprioriSigma = ui.GetDouble("SPACECRAFT_POSITION_SIGMA");
      }
      if ( ui.WasEntered("SPACECRAFT_VELOCITY_SIGMA") ) {
        positionVelocityAprioriSigma = ui.GetDouble("SPACECRAFT_VELOCITY_SIGMA");
      }
      if ( ui.WasEntered("SPACECRAFT_ACCELERATION_SIGMA") ) {
        positionAccelerationAprioriSigma = ui.GetDouble("SPACECRAFT_ACCELERATION_SIGMA");
      }

      observationSolveSettings.setInstrumentPositionSettings(positionSolveOption,
                                                              ui.GetInteger("SPKDEGREE"),
                                                              ui.GetInteger("SPKSOLVEDEGREE"),
                                                              ui.GetBoolean("OVERHERMITE"),
                                                              positionAprioriSigma,
                                                              positionVelocityAprioriSigma,
                                                              positionAccelerationAprioriSigma);

      if ((ui.WasEntered("CSMSOLVESET")  && ui.WasEntered("CSMSOLVETYPE")) ||
          (ui.WasEntered("CSMSOLVESET")  && ui.WasEntered("CSMSOLVELIST")) ||
          (ui.WasEntered("CSMSOLVETYPE") && ui.WasEntered("CSMSOLVELIST")) ) {
        std::string msg = "Only one of CSMSOLVESET, CSMSOLVETYPE, and CSMSOLVELIST "
                      "can be specified at a time.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      if (ui.WasEntered("CSMSOLVESET")) {
        observationSolveSettings.setCSMSolveSet(
            BundleObservationSolveSettings::stringToCSMSolveSet(ui.GetString("CSMSOLVESET")));
      }
      else if (ui.WasEntered("CSMSOLVETYPE")) {
        observationSolveSettings.setCSMSolveType(
            BundleObservationSolveSettings::stringToCSMSolveType(ui.GetString("CSMSOLVETYPE")));
      }
      else if (ui.WasEntered("CSMSOLVELIST")) {
        std::vector<QString> csmParamVector;
        ui.GetString("CSMSOLVELIST", csmParamVector);
        QStringList csmParamList = QStringList::fromVector(QVector<QString>(csmParamVector.begin(), csmParamVector.end()));
        observationSolveSettings.setCSMSolveParameterList(csmParamList);
      }

      // add all image observation numbers to this BOSS.
      for (int sn = 0; sn < cubeSNs.size(); sn++) {
        observationSolveSettings.addObservationNumber(cubeSNs.observationNumber(sn));
      }

      // append the GUI acquired solve parameters to BOSS list.
      observationSolveSettingsList.append(observationSolveSettings);

    }


    // If we are holding any images, then we need a BundleObservationSolveSettings for the held
    // images, and another one for the non-held images
    if (ui.WasEntered("HELDLIST")) {
      // Check that the held images are present in the input image list
      QString heldList = ui.GetFileName("HELDLIST");
      SerialNumberList heldSNs(heldList);
      checkImageList(heldSNs, cubeSNs);

      double anglesAprioriSigma, angularVelocityAprioriSigma, angularAccelerationAprioriSigma;
      anglesAprioriSigma = angularVelocityAprioriSigma = angularAccelerationAprioriSigma = Isis::Null;

      double positionAprioriSigma, positionVelocityAprioriSigma, positionAccelerationAprioriSigma;
      positionAprioriSigma = positionVelocityAprioriSigma = positionAccelerationAprioriSigma
                            = Isis::Null;

      // The settings for the held images will have no pointing or position factors considered
      BundleObservationSolveSettings heldSettings;
      BundleObservationSolveSettings::InstrumentPointingSolveOption noPointing =
          BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
              "NoPointingFactors");
      heldSettings.setInstrumentPointingSettings(noPointing,
                                                  ui.GetBoolean("TWIST"),
                                                  ui.GetInteger("CKDEGREE"),
                                                  ui.GetInteger("CKSOLVEDEGREE"),
                                                  ui.GetBoolean("OVEREXISTING"),
                                                  anglesAprioriSigma,
                                                  angularVelocityAprioriSigma,
                                                  angularAccelerationAprioriSigma);
      BundleObservationSolveSettings::InstrumentPositionSolveOption noPosition =
          BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
              "NoPositionFactors");
      heldSettings.setInstrumentPositionSettings(noPosition,
                                                  ui.GetInteger("SPKDEGREE"),
                                                  ui.GetInteger("SPKSOLVEDEGREE"),
                                                  ui.GetBoolean("OVERHERMITE"),
                                                  positionAprioriSigma,
                                                  positionVelocityAprioriSigma,
                                                  positionAccelerationAprioriSigma);

      // Add the held images' observationNumbers to the held observation solve settings
      for (int sn = 0; sn < cubeSNs.size(); sn++) {
        if ( heldSNs.hasSerialNumber(cubeSNs.serialNumber(sn)) ) {
          // For held images, we want to set pointing and position settings to NONE, effectively
          // ensuring that the number of pointing and position parameters for the holds are 0
          heldSettings.addObservationNumber(cubeSNs.observationNumber(sn));
          QString snInstId = cubeSNs.spacecraftInstrumentId(sn);
          //for each held serial number, locate corresponding BOSS in BOSSlist, remove.
          for (auto bossIt = observationSolveSettingsList.begin(); bossIt != observationSolveSettingsList.end(); bossIt++) {
            if (bossIt->instrumentId() == snInstId) {
              bossIt->removeObservationNumber(cubeSNs.observationNumber(sn));
            }
          }
        }
      }
      // Add the held observation solve settings to the list of solve settings
      // for the BundleAdjust
      observationSolveSettingsList.append(heldSettings);
    }

    //************************************************************************************************
    return observationSolveSettingsList;
  }


  /**
   * Control points that intersect the held images are set to fixed. The points' a priori values
   * are each set to the corresponding surface points of the associated held image's measures.
   *
   * Note that this returns a ControlNetQsp to the modified input control network.
   *
   * @param cnetFile QString name of the control network file.
   * @param heldList QString name of the held list file.
   * @param imageLise QString name of the input image list file.
   *
   * @throws IException::User "Cannot compute surface point for control point [], measure [].
   *
   * @return @b ControlNetQsp Returns a shared pointer to the modified control network.
   *
   * @internal
   *   @todo Currently only works for NON-overlapping held images. Any control points that intersect
   *         the held images are set to FIXED and have their apriori surface points set to
   *         corresponding surface points for the held image's measures.
   */
  ControlNetQsp fixHeldImages(const QString &cnetFile,
                              const QString &heldList,
                              const QString &snList) {
    ControlNetQsp cnet(new ControlNet(cnetFile));
    // Set up the cameras for all the input images in the control net
    cnet->SetImages(snList);

    // For all held images' measures, set their parent control points' a priori values,
    // and set their types to Fixed
    SerialNumberList heldSNs(heldList);
    for (int sn = 0; sn < heldSNs.size(); sn++) {
      // Get the measures in the held image
      QList<ControlMeasure *> measures = cnet->GetMeasuresInCube(heldSNs.serialNumber(sn));

      foreach (ControlMeasure *cm, measures) {
        Camera *cam = cm->Camera();
        ControlPoint *pt = cm->Parent();
        pt->SetType(ControlPoint::Fixed);
        // If possible, set the apriori surface point for the current measure's control point
        if ( cam->SetImage(cm->GetSample(), cm->GetLine()) ) {
          pt->SetAprioriSurfacePoint(cam->GetSurfacePoint());
        }
        else {
          std::string msg = "Cannot compute surface point for control point [" + pt->GetId().toStdString() +
              "], measure [" + cm->GetCubeSerialNumber().toStdString() + "].";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

    }
    return cnet;
  }

}
