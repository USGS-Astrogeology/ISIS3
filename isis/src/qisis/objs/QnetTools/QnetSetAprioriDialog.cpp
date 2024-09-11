#include "QnetSetAprioriDialog.h"

#include <QDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QtWidgets>
#include <QVBoxLayout>

#include "Camera.h"
#include "CameraDistortionMap.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Distance.h"
#include "Latitude.h"
#include "Longitude.h"
#include "QnetTool.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"

using namespace std;

namespace Isis {
  QnetSetAprioriDialog::QnetSetAprioriDialog(QnetTool *qnetTool, QWidget *parent) : QDialog(parent) {
    m_qnetTool = qnetTool;

    m_aprioriDialog = NULL;
    m_aprioriGridLayout = NULL;
    m_okButton = NULL;
    m_cancelButton = NULL;
    m_applyButton = NULL;
    m_pointInfoStack = NULL;

    m_singlePointInfoGroup = NULL;
    m_pointIDLabel = NULL;
    m_pointTypeLabel = NULL;
    m_pointMeasureNumber = NULL;
    m_editLockedBoolLabel = NULL;
    m_ignoredBoolLabel = NULL;

    m_multiplePointsInfoGroup = NULL;
    m_pointsCount = NULL;
    m_pointsMeasuresCount = NULL;
    m_constrainedPointsCount = NULL;
    m_fixedPointsCount = NULL;
    m_freePointsCount = NULL;
    m_pointsEditLockedCount = NULL;
    m_pointsIgnoredCount = NULL;

    m_pointGroup = NULL;
    m_aprioriLatLabel = NULL;
    m_aprioriLonLabel = NULL;
    m_aprioriRadiusLabel = NULL;
    m_latLineEdit = NULL;
    m_lonLineEdit = NULL;
    m_radiusLineEdit = NULL;
    m_currentAprioriButton = NULL;
    m_referenceAprioriButton = NULL;
    m_averageAprioriButton = NULL;

    m_sigmaGroup = NULL;
    m_sigmaWarningLabel = NULL;
    m_currentSigmaButton = NULL;
    m_latSigmaLabel = NULL;
    m_lonSigmaLabel = NULL;
    m_radiusSigmaLabel = NULL;
    m_latSigmaLineEdit = NULL;
    m_lonSigmaLineEdit = NULL;
    m_radiusSigmaLineEdit = NULL;

    m_multiPointsMeasureCount = 0;
    m_multiPointsConstraintedCount = 0;
    m_multiPointsFixedCount = 0;
    m_multiPointsFreeCount = 0;
    m_multiPointsEditLockedCount = 0;
    m_multiPointsIgnoredCount = 0;

    createSetAprioriDialog(parent);

    connect(m_currentAprioriButton, SIGNAL(clicked()), this, SLOT(fillCurrentAprioriLineEdits()));
    connect(m_referenceAprioriButton, SIGNAL(clicked()), this,
            SLOT(fillReferenceAprioriLineEdits()));
    connect(m_averageAprioriButton, SIGNAL(clicked()), this, SLOT(fillAverageAprioriLineEdits()));

    //TODO create a ground button that retrieves the lat/lon/radius of the ground source if there
    //is one. The basic connections are already done, just need to actually do it
//     connect(m_groundAprioriButton, SIGNAL(clicked()), this,
//     SLOT(fillGroundSourceAprioriLineEdits()));

    connect(m_currentSigmaButton, SIGNAL(clicked()), this, SLOT(fillSigmaLineEdits()));
    connect(m_currentSigmaButton, SIGNAL(clicked()), this, SLOT(fillSigmaLineEdits()));

    connect(m_aprioriDialog, SIGNAL(rejected()), this, SLOT(reject()));

    connect(m_okButton, SIGNAL(clicked()), this, SLOT(setApriori()));
    connect(m_okButton, SIGNAL(clicked()), this, SLOT(closeEvent()));
    connect(m_okButton, SIGNAL(clicked()), m_aprioriDialog, SLOT(close()));

    connect(m_applyButton, SIGNAL(clicked()), this, SLOT(setApriori()));
    connect(m_cancelButton, SIGNAL(clicked()), this, SLOT(closeEvent()));
    connect(m_cancelButton, SIGNAL(clicked()), m_aprioriDialog, SLOT(close()));
  }


  /**
   * Creates the dialog box for the set apriori tool
   *
   * @param parent The parent widget for the set apriori dialog
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::createSetAprioriDialog(QWidget *parent) {

    //Create all of the individual elements
    m_pointIDLabel = new QLabel("Point ID: ");
    m_pointTypeLabel = new QLabel("Point Type: ");
    m_pointMeasureNumber = new QLabel("Number of Measures: ");
    m_editLockedBoolLabel = new QLabel("EditLocked: ");
    m_ignoredBoolLabel= new QLabel("Ignored: ");

    m_pointsCount = new QLabel("Number of Points: ");
    m_pointsMeasuresCount = new QLabel("Total Number of Measures: ");
    m_constrainedPointsCount = new QLabel("Number of Constrained Points: ");
    m_fixedPointsCount = new QLabel("Number of Fixed Points: ");
    m_freePointsCount = new QLabel("Number of Free Points: ");
    m_pointsEditLockedCount = new QLabel("Number of Edit Locked Points: ");
    m_pointsIgnoredCount = new QLabel("Number of Ignored Points: ");

    m_currentAprioriButton = new QPushButton("Current");
    m_currentAprioriButton->setDefault(false);
    m_currentAprioriButton->setToolTip("Populate with the current Apriori Position");

    m_referenceAprioriButton = new QPushButton("Reference");
    m_referenceAprioriButton->setToolTip("Populate with Apriori Position of the reference measure");

    m_averageAprioriButton = new QPushButton("Average");
    m_averageAprioriButton->setToolTip("Calculate and populate with the average Apriori Position");

//     m_groundAprioriButton = new QPushButton("Ground");
//     m_groundAprioriButton->setToolTip("Populate with the Ground Source position, if a Ground Source is loaded");

    m_aprioriLatLabel = new QLabel(tr("Apriori Latitude"));
    m_aprioriLonLabel = new QLabel(tr("Apriori Longitude"));
    m_aprioriRadiusLabel = new QLabel(tr("Apriori Radius"));
    m_latLineEdit = new QLineEdit();
    m_lonLineEdit = new QLineEdit();
    m_radiusLineEdit = new QLineEdit();

    m_sigmaWarningLabel = new QLabel("");

    m_currentSigmaButton = new QPushButton("Current");
    m_currentSigmaButton->setToolTip("Populate the current sigma values");

    m_latSigmaLabel = new QLabel(tr("Latitude Sigma"));
    m_lonSigmaLabel = new QLabel(tr("Longitude Sigma"));
    m_radiusSigmaLabel = new QLabel(tr("Radius Sigma"));
    m_latSigmaLineEdit = new QLineEdit();
    m_lonSigmaLineEdit = new QLineEdit();
    m_radiusSigmaLineEdit = new QLineEdit();

    m_okButton = new QPushButton("&OK");
    m_okButton->setToolTip("Apply changes and close this dialog");

    m_cancelButton = new QPushButton("&Cancel");
    m_cancelButton->setToolTip("Discard changes and close this dialog");

    m_applyButton = new QPushButton("&Apply");
    m_applyButton->setAutoDefault(true);
    m_applyButton->setDefault(true);
    m_applyButton->setToolTip("Apply changes");

    //Create the point group box and layout
    m_pointGroup = new QGroupBox(tr("Apriori Point"));
    m_pointGroup->setToolTip("Apriori Point Position");

    QGridLayout *pointGridLayout = new QGridLayout(m_pointGroup);
    pointGridLayout->addWidget(m_currentAprioriButton, 1, 1);
    pointGridLayout->addWidget(m_referenceAprioriButton, 1, 2);
    pointGridLayout->addWidget(m_averageAprioriButton, 1, 3);
//     pointGridLayout->addWidget(m_groundAprioriButton, 1, 4);
    pointGridLayout->addWidget(m_aprioriLatLabel, 2, 1);
    pointGridLayout->addWidget(m_aprioriLonLabel, 3, 1);
    pointGridLayout->addWidget(m_aprioriRadiusLabel, 4, 1);
    pointGridLayout->addWidget(m_latLineEdit, 2, 2, 1, -1);
    pointGridLayout->addWidget(m_lonLineEdit, 3, 2, 1, -1);
    pointGridLayout->addWidget(m_radiusLineEdit, 4, 2, 1, -1);
    m_pointGroup->setLayout(pointGridLayout);

    //Create the sigma group box and layout
    m_sigmaGroup = new QGroupBox(tr("Apriori Constraints"));
    QGridLayout *sigmaGridLayout = new QGridLayout(m_sigmaGroup);
    sigmaGridLayout->addWidget(m_currentSigmaButton, 1, 1);
    sigmaGridLayout->addWidget(m_latSigmaLabel, 2, 1);
    sigmaGridLayout->addWidget(m_lonSigmaLabel, 3, 1);
    sigmaGridLayout->addWidget(m_radiusSigmaLabel, 4, 1);
    sigmaGridLayout->addWidget(m_latSigmaLineEdit, 2, 2, 1, 3);
    sigmaGridLayout->addWidget(m_lonSigmaLineEdit, 3, 2, 1, 3);
    sigmaGridLayout->addWidget(m_radiusSigmaLineEdit, 4, 2, 1, 3);
    m_sigmaGroup->setLayout(sigmaGridLayout);

    //Create group box for the single point information labels
    m_singlePointInfoGroup = new QGroupBox(tr("Point Information"));
    m_singlePointInfoGroup->setToolTip("Information on Point selected");

    QVBoxLayout *m_singlePointInfoLayout = new QVBoxLayout(m_singlePointInfoGroup);
    m_singlePointInfoLayout->addWidget(m_pointIDLabel);
    m_singlePointInfoLayout->addWidget(m_pointTypeLabel);
    m_singlePointInfoLayout->addWidget(m_pointMeasureNumber);
    m_singlePointInfoLayout->addWidget(m_editLockedBoolLabel);
    m_singlePointInfoLayout->addWidget(m_ignoredBoolLabel);
    m_singlePointInfoGroup->setLayout(m_singlePointInfoLayout);

    //Create group box for the multiple point information labels
    m_multiplePointsInfoGroup = new QGroupBox(tr("Multiple Point Information"));
    m_multiplePointsInfoGroup->setToolTip("Information on Points selected");
    QVBoxLayout *m_multiplePointsInfoLayout = new QVBoxLayout(m_multiplePointsInfoGroup);
    m_multiplePointsInfoLayout->addWidget(m_pointsCount);
    m_multiplePointsInfoLayout->addWidget(m_pointsMeasuresCount);
    m_multiplePointsInfoLayout->addWidget(m_constrainedPointsCount);
    m_multiplePointsInfoLayout->addWidget(m_fixedPointsCount);
    m_multiplePointsInfoLayout->addWidget(m_freePointsCount);
    m_multiplePointsInfoLayout->addWidget(m_pointsEditLockedCount);
    m_multiplePointsInfoLayout->addWidget(m_pointsIgnoredCount);
    m_multiplePointsInfoGroup->setLayout(m_multiplePointsInfoLayout);

    //Create a stacked widget to switch between the single point and multiple point labels
    m_pointInfoStack = new QStackedWidget;
    m_pointInfoStack->addWidget(m_singlePointInfoGroup);
    m_pointInfoStack->addWidget(m_multiplePointsInfoGroup);

    //Add all of the major widgets to the main dialog layout
    m_aprioriGridLayout = new QGridLayout(m_aprioriDialog);
    m_aprioriGridLayout->addWidget(m_pointInfoStack, 1, 1, 1, -1);
    m_aprioriGridLayout->addWidget(m_pointGroup, 5, 1, 1, -1);
    m_aprioriGridLayout->addWidget(m_sigmaGroup, 6, 1, 1, -1);
    m_aprioriGridLayout->addWidget(m_sigmaWarningLabel, 7, 1, 1, -1);
    m_aprioriGridLayout->addWidget(m_okButton, 8, 2);
    m_aprioriGridLayout->addWidget(m_cancelButton, 8, 3);
    m_aprioriGridLayout->addWidget(m_applyButton, 8, 4);


    m_aprioriDialog = new QDialog(parent);
    m_aprioriDialog->setWindowTitle("Set Apriori Point and Constraints");
    m_aprioriDialog->setLayout(m_aprioriGridLayout);

    setVisiblity();
  }


  /**
   * This is called when the user selects the X button on the top right or they hit ESC.
   * Disconnect all slots on close event.
   *
   * @author 2016-11-18 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::reject() {
    closeEvent();
    QDialog::reject();
  }


  /**
   * Disconnect all of the slots on a close event
   *
   * @author 2016-11-18 Makayla Shepherd
   */
  void QnetSetAprioriDialog::closeEvent() {

      disconnect(m_currentAprioriButton, SIGNAL(clicked()), this,
                SLOT(fillCurrentAprioriLineEdits()));
      disconnect(m_referenceAprioriButton, SIGNAL(clicked()), this,
                SLOT(fillReferenceAprioriLineEdits()));
      disconnect(m_averageAprioriButton, SIGNAL(clicked()), this,
                SLOT(fillAverageAprioriLineEdits()));

      disconnect(m_currentSigmaButton, SIGNAL(clicked()), this, SLOT(fillSigmaLineEdits()));
      disconnect(m_currentSigmaButton, SIGNAL(clicked()), this, SLOT(fillSigmaLineEdits()));

      disconnect(m_okButton, SIGNAL(clicked()), this, SLOT(setApriori()));
      disconnect(m_applyButton, SIGNAL(clicked()), this, SLOT(setApriori()));

      emit aprioriDialogClosed();

  }


  /**
   * Shows the dialog box
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::setVisiblity() {
    if (m_aprioriDialog != NULL) {
      m_aprioriDialog->setVisible(true);
    }
  }


  /**
   * Set control points in the dialog
   *
   * @param selectedPoints QList<QListWidgetItem *> ControlPoints listed
   *
   * @internal
   * @history 2011-10-03 Tracie Sucharski - Do not enable user Entered Button,
   *                        this will only be enabled if the group box is enabled.
   * @history 2016-02-05 Makayla Shepherd - Redesigned to work with new UI
   */
  void QnetSetAprioriDialog::setPoints(QList<QListWidgetItem *> selectedPoints) {
    m_points = selectedPoints;

    checkPointInfoDisable(m_points);
    resetInfoLabels();
    clearLineEdits();

    setInfoStack(m_points);

    if (m_points.size() == 1) {
      fillCurrentAprioriLineEdits();
      fillSigmaLineEdits();
    }
  }


  /**
   * Populates the apriori lat/lon/radius line edits with the current values. If
   * there are no current values, the line edits are empty. This only works on single points
   * and is disabled for multiple points.
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::fillCurrentAprioriLineEdits() {
    if (m_points.size() == 0) {
      QString msg = "There are no Points selected. Please select a Point.";
      QMessageBox::warning((QWidget *)parent(), "Warning", msg);
      return;
    }

    //we can only populate the line edits if there is one point selected
    if (m_points.size() == 1) {
      QString id = m_points.at(0)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
      SurfacePoint sPt = pt->GetAprioriSurfacePoint();

      if (sPt.GetLatitude().degrees() != Null) {
        m_latLineEdit->setText(
          QString::number(sPt.GetLatitude().degrees()));
      }
      else {
        m_latLineEdit->clear();
      }
      if (sPt.GetLongitude().degrees() != Null) {
        m_lonLineEdit->setText(
          QString::number(sPt.GetLongitude().degrees()));
      }
      else {
        m_lonLineEdit->clear();
      }
      //The 'f', 2 allows the radius to be displayed in meters rather than scientific notation
      if (sPt.GetLocalRadius().meters() != Null) {
        m_radiusLineEdit->setText(
          QString::number(sPt.GetLocalRadius().meters(), 'f', 2));
      }
      else {
        m_radiusLineEdit->clear();
      }
      m_aprioriSource = (Source) USER;
    }
  }


  /**
   * Populates the apriori lat/lon/radius line edits with the reference measure values.
   * This only works on single points and is disabled for multiple points. The calculations
   * were moved from setApriori.
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::fillReferenceAprioriLineEdits() {
    if (m_points.size() == 0) {
      QString msg = "There are no Points selected. Please select a Point.";
      QMessageBox::warning((QWidget *)parent(), "Warning", msg);
      return;
    }

    //we can only populate the line edits if there is one point selected
    if (m_points.size() == 1) {

      QString id = m_points.at(0)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
      ControlMeasure *m = pt->GetRefMeasure();

      // Find camera from network camera list
      int camIndex = m_qnetTool->serialNumberList()->serialNumberIndex(
          m->GetCubeSerialNumber());
      Camera *cam = m_qnetTool->controlNet()->Camera(camIndex);
      cam->SetImage(m->GetSample(),m->GetLine());
      SurfacePoint refSPt = cam->GetSurfacePoint();
      if (refSPt.GetLatitude().degrees() != Null) {
         m_latLineEdit->setText(
          QString::number(refSPt.GetLatitude().degrees()));
      }
      else {
        m_latLineEdit->clear();
      }
      if (refSPt.GetLongitude().degrees() != Null) {
        m_lonLineEdit->setText(
          QString::number(refSPt.GetLongitude().degrees()));
      }
      else {
        m_lonLineEdit->clear();
      }
      //The 'f', 2 allows the radius to be displayed in meters rather than scientific notation
      if (refSPt.GetLocalRadius().meters() != Null) {
        m_radiusLineEdit->setText(
          QString::number(refSPt.GetLocalRadius().meters(), 'f', 2));
      }
      else {
        m_radiusLineEdit->clear();
      }

      //If all of the line edits are empty something went wrong, tell the user, and return
      if ((m_latLineEdit->text() == "") && (m_lonLineEdit->text() == "")
          && (m_radiusLineEdit->text() == "")) {
        QString msg = "Cannot retrieve the latitude, longitude, and radius from the reference";
        msg = msg + "measure; this is the result of a known problem in our system. Please select ";
        msg = msg + "Current, Average, or enter your own values.";
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }
      m_aprioriSource = (Source) REFERENCE;
    }
  }


  /**
   * Populates the apriori lat/lon/radius line edits with the average measure values.
   * This only works on single points and is disabled for multiple points. The calculations
   * were moved from setApriori.
   *
   * the code used to compute the average of the measures is copied from
   * ControlPoint::ComputeApriori
   *
   * There is a known issue with this code that if something goes wrong and the average cannot be
   * computed then the lat, lon, radius is set to null. In that case, clear the line edits
   * and display an error message.
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::fillAverageAprioriLineEdits() {
    if (m_points.size() == 0) {
      QString msg = "There are no Points selected. Please select a Point.";
      QMessageBox::warning((QWidget *)parent(), "Warning", msg);
      return;
    }

    //we can only populate the line edits if there is one point selected
    if (m_points.size() == 1) {
      double xB = 0.0;
      double yB = 0.0;
      double zB = 0.0;
      double r2B = 0.0;
      int goodMeasures = 0;
      SurfacePoint aprioriSurfacePoint;

      QString id = m_points.at(0)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);



      //this code is copied from ControlPoint::ComputeApriori

      for (int i = 0; i < pt->GetNumMeasures(); i++) {
        ControlMeasure *m = pt->GetMeasure(i);
        if (m->IsIgnored()) {
          // TODO: How do we deal with ignored measures
        }
        else {
          Camera *cam = m->Camera();
          if (cam == NULL) {
            std::string msg = "The Camera must be set prior to calculating apriori";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
          if (cam->SetImage(m->GetSample(), m->GetLine())) {
            goodMeasures++;
            double pB[3];
            cam->Coordinate(pB);
            xB += pB[0];
            yB += pB[1];
            zB += pB[2];
            r2B += pB[0]*pB[0] + pB[1]*pB[1] + pB[2]*pB[2];

            //double x = cam->DistortionMap()->UndistortedFocalPlaneX();
            //double y = cam->DistortionMap()->UndistortedFocalPlaneY();
            //m->SetFocalPlaneMeasured(x, y);
          }
          else {
            // JAA: Don't stop if we know the lat/lon.  The SetImage may fail
            // but the FocalPlane measures have been set
            if (pt->GetPointTypeString() == "Fixed")
              continue;
          }
        }
      }

      // Did we have any measures?
      if (goodMeasures == 0) {
        std::string msg = "ControlPoint [" + id.toStdString() + "] has no measures which "
           "project to lat/lon/radius (x/y/z)";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Compute the averages
      //if (NumberOfConstrainedCoordinates() == 0) {
      if (pt->GetPointTypeString() == "Free" || pt->NumberOfConstrainedCoordinates() == 0) {
        double avgX = xB / goodMeasures;
        double avgY = yB / goodMeasures;
        double avgZ = zB / goodMeasures;
        double avgR2 = r2B / goodMeasures;
        double scale = sqrt(avgR2/(avgX*avgX+avgY*avgY+avgZ*avgZ));

        aprioriSurfacePoint.SetRectangular(
          Displacement((avgX*scale), Displacement::Kilometers),
          Displacement((avgY*scale), Displacement::Kilometers),
          Displacement((avgZ*scale), Displacement::Kilometers));
      }
      // Since we are not solving yet for x,y,and z in the bundle directly,
      // longitude must be constrained.  This constrains x and y as well.
      else if (!(pt->GetPointTypeString() == "Fixed") &&
               !(pt->NumberOfConstrainedCoordinates() == 3) &&
               !pt->IsCoord1Constrained() &&
               !pt->IsCoord2Constrained() &&
               !pt->IsCoord3Constrained()){
          aprioriSurfacePoint.SetRectangular(
          Displacement(aprioriSurfacePoint.GetX().meters(), Displacement::Meters),
          Displacement(aprioriSurfacePoint.GetY().meters(), Displacement::Meters),
          Displacement((zB / goodMeasures), Displacement::Kilometers));
      }

      //End of copied code



      SurfacePoint sPt = aprioriSurfacePoint;

      if (sPt.GetLatitude().degrees() != Null) {
        m_latLineEdit->setText(
          QString::number(sPt.GetLatitude().degrees()));
      }
      else {
        m_latLineEdit->clear();
      }
      if (sPt.GetLongitude().degrees() != Null) {
        m_lonLineEdit->setText(
          QString::number(sPt.GetLongitude().degrees()));
      }
      else {
        m_lonLineEdit->clear();
      }
      //The 'f', 2 allows the radius to be displayed in meters rather than scientific notation
      if (sPt.GetLocalRadius().meters() != Null) {
        m_radiusLineEdit->setText(
          QString::number(sPt.GetLocalRadius().meters(), 'f', 2));
      }
      else {
        m_radiusLineEdit->clear();
      }

      //If all of the line edits are empty something went wrong
      //tell the user that the average canot be computed and leave the apriori source alone
      if ((m_latLineEdit->text() == "") && (m_lonLineEdit->text() == "")
          && (m_radiusLineEdit->text() == "")) {
        QString msg = "Average cannot be computed for this point [" + m_points.at(0)->text();
        msg = msg + "]; this is the result of a known problem in our system. Please select ";
        msg = msg + "Current, Reference, or enter your own values.";
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }
      m_aprioriSource = (Source) AVERAGE;
    }
  }

//   /**
//    This is the base for a new ticket that allows for a ground source button
//
//    * Populates the apriori lat/lon/radius line edits with the ground source values, if there is a
//    * ground source. This only works on single points and is disabled for multiple points.
//    *
//    * @author 2016-08-05 Makayla Shepherd
//    *
//    */
//   void QnetSetAprioriDialog::fillGroundSourceAprioriLineEdits() {
//     if (m_points.size() == 0) {
//       std::string msg = "There are no Points selected. Please select a Point.";
//       QMessageBox::warning((QWidget *)parent(), "Warning", msg);
//       return;
//     }
//     if (m_points.size() == 1) {
//
//     }
//   }


  /**
   * Populates the sigma lat/lon/radius line edits with the current values. If
   * there are no current sigma values, the line edits are empty. This only works on
   * single points and is disabled for multiple points.
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::fillSigmaLineEdits() {
    if (m_points.size() == 0) {
      QString msg = "There are no Points selected. Please select a Point.";
      QMessageBox::warning((QWidget *)parent(), "Warning", msg);
      return;
    }

    //we can only populate the sigma line edits if there is one point selected
    if (m_points.size() == 1) {
      QString id = m_points.at(0)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
      SurfacePoint sPt = pt->GetAprioriSurfacePoint();
      if (sPt.GetLatSigmaDistance().meters() != Null) {
        m_latSigmaLineEdit->setText(
          QString::number(sPt.GetLatSigmaDistance().meters()));
      }
      if (sPt.GetLonSigmaDistance().meters() != Null) {
        m_lonSigmaLineEdit->setText(
          QString::number(sPt.GetLonSigmaDistance().meters()));
      }
      //The 'f', 2 allows the radius to be displayed in meters rather than scientific notation
      if (sPt.GetLocalRadiusSigma().meters() != Null) {
        m_radiusSigmaLineEdit->setText(
          QString::number(sPt.GetLocalRadiusSigma().meters(), 'f', 2));
      }
      pt = NULL;
    }
  }


  /**
   * Switches what information is visible based on how many points are selected. Defaults to single point
   * information.
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::setInfoStack(QList<QListWidgetItem *> selectedPoints) {
    if (selectedPoints.size() > 1) {
      m_pointInfoStack->setCurrentWidget(m_multiplePointsInfoGroup);
    }
    else {
      m_pointInfoStack->setCurrentWidget(m_singlePointInfoGroup);
    }
  }


  /**
   * Enables/Disables features based on if there are multiple points selected or not. If multiple points are
   * selected it also counts how many are EditLocked, Ignored, the number of each type of point, and the total
   * number of measures.
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::checkPointInfoDisable(QList<QListWidgetItem *> selectedPoints) {
    m_points = selectedPoints;
    m_aprioriDialog->setEnabled(true);
    m_sigmaGroup->setEnabled(true);
    m_pointGroup->setEnabled(true);
    m_currentSigmaButton->setEnabled(true);
    m_sigmaWarningLabel->clear();

    m_multiPointsMeasureCount = 0;
    m_multiPointsConstraintedCount = 0;
    m_multiPointsFixedCount = 0;
    m_multiPointsFreeCount = 0;
    m_multiPointsEditLockedCount = 0;
    m_multiPointsIgnoredCount = 0;

    //handle multiple points
    if (m_points.size() > 1) {
      for (int i = 0; i < m_points.size(); i++) {
        QString id = m_points.at(i)->text();
        ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
        m_multiPointsMeasureCount += pt->GetNumMeasures();
        if (pt->IsEditLocked()) {
          m_multiPointsEditLockedCount++;
        }
        if (pt->IsIgnored()) {
          m_multiPointsIgnoredCount++;
        }
        if (pt->GetPointTypeString() == "Constrained") {
          m_multiPointsConstraintedCount++;
        }
        if (pt->GetPointTypeString() == "Fixed") {
          m_multiPointsFixedCount++;
        }
        if (pt->GetPointTypeString() == "Free") {
          m_multiPointsFreeCount++;
        }
      }
      if (m_multiPointsEditLockedCount > 0) {
        m_aprioriDialog->setDisabled(true);
        QString msg = "There is an EditLocked point selected. To continue, unselect the";
        msg = msg + " Editlocked point.";
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }
      if (m_multiPointsFixedCount > 0 || m_multiPointsFreeCount > 0) {
        m_aprioriDialog->setDisabled(true);
        QString msg = "Sigmas can only be set on Constrained points. Use Filters to filter by";
        msg = msg + " Constrained points.";
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }
      m_pointGroup->setDisabled(true);
      m_currentSigmaButton->setDisabled(true);
    }
    //handle a single point
    else if (m_points.size() == 1) {
      QString id = m_points.at(0)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
      if (pt->IsEditLocked()) {
        m_aprioriDialog->setDisabled(true);
        QString msg = "This control point is edit locked.  The Apriori latitude, longitude and ";
        msg += "radius cannot be updated.  You must first unlock the point by clicking the ";
        msg += "check box above labeled \"Edit Lock Point\".";
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }
      else if (pt->GetPointTypeString() == "Fixed" || pt->GetPointTypeString() == "Free") {
        m_sigmaWarningLabel->setText("Change point type to Constrained to enter constraints (Apriori Sigmas).");
        m_sigmaGroup->setDisabled(true);
      }
      else {
        m_aprioriDialog->setEnabled(true);
        m_sigmaGroup->setEnabled(true);
        m_pointGroup->setEnabled(true);
        m_currentSigmaButton->setEnabled(true);
      }
    }
  }


  /**
   * Clears the line edits
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   * @history 2016-02-05 Makayla Shepherd - Modified to work with new UI
   */
  void QnetSetAprioriDialog::clearLineEdits() {
    m_latLineEdit->clear();
    m_lonLineEdit->clear();
    m_radiusLineEdit->clear();
    m_latSigmaLineEdit->clear();
    m_lonSigmaLineEdit->clear();
    m_radiusSigmaLineEdit->clear();
  }


  /**
   * Resets and populates the information stack labels
   *
   * @author 2016-02-05 Makayla Shepherd
   *
   */
  void QnetSetAprioriDialog::resetInfoLabels() {

    if (m_points.size() < 0) {
      m_pointIDLabel->setText("Point ID: ");
      m_pointTypeLabel->setText("Point Type: ");
      m_pointMeasureNumber->setText("Number of Measures: ");
      m_editLockedBoolLabel->setText("EditLocked: ");
      m_ignoredBoolLabel->setText("Ignored: ");
    }
    //reset all the information needed for a single point
    else if (m_points.size() == 1) {
      QString id = m_points.at(0)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
      m_pointIDLabel->setText("Point ID: " + QString(id));
      m_pointTypeLabel->setText("Point Type: " + QString(pt->GetPointTypeString()));
      m_pointMeasureNumber->setText("Number of Measures: " + QString::number(pt->GetNumMeasures()));

      if (pt->IsEditLocked()) {
        m_editLockedBoolLabel->setText("EditLocked: True");
      }
      else {
        m_editLockedBoolLabel->setText("EditLocked: False");
      }
      if (pt->IsIgnored()) {
        m_ignoredBoolLabel->setText("Ignored: True");
      }
      else {
        m_ignoredBoolLabel->setText("Ignored: False");
      }
    }
    //reset all of the information needed for multiple points
    else if (m_points.size() > 1) {
      m_pointsCount->setText("Number of Points: " + QString::number(m_points.size()));
      m_pointsMeasuresCount->setText("Total Number of Measures: " +
                                     QString::number(m_multiPointsMeasureCount));
      m_constrainedPointsCount->setText("Number of Constrained Points: " +
                                        QString::number(m_multiPointsConstraintedCount));
      m_fixedPointsCount->setText("Number of Fixed Points: " +
                                  QString::number(m_multiPointsFixedCount));
      m_freePointsCount->setText("Number of Free Points: " +
                                 QString::number(m_multiPointsFreeCount));
      m_pointsEditLockedCount->setText("Number of Edit Locked Points: " +
                                       QString::number(m_multiPointsEditLockedCount));
      m_pointsIgnoredCount->setText("Number of Ignored Points: " +
                                    QString::number(m_multiPointsIgnoredCount));
    }
  }


  /**
   * Slot to set apriori on selected Points from Navigator list box
   *
   * @author 2011-03-24 Tracie Sucharski
   *
   * @internal
   *
   * @history 2011-04-04 Tracie Sucharski - Grey out userEntered if more than
   *                        a single point is selected.  Grey out lat,lon,radius
   *                        edits if UserEntered is not selected.
   * @history 2011-04-13 Tracie Sucharski - If single point selected, fill in
   *                        LineEdit's with current controlPoint values.
   * @history 2011-04-19 Tracie Sucharski - Redesign using modeless dialog.
   * @history 2011-04-26 Tracie Sucharski - Move from QnetNavTool to
   *                        QnetSetAprioriDialog.
   * @history 2016-02-05 Makayla Shepherd - Redesigned to work with the new UI. This method
   *                        only takes the values in the line edits and uses them to set the
   *                        Apriori values. The calculations for reference and average values
   *                        are made and populated in the fill methods.
   * @history 2016-10-14 Makayla Shepherd - Fixed an issue that caused the apriori sigmas to be set
   *                        to NULL. You can now set the apriori sigmas.
   * @history 2016-11-16 Makayla Shepherd - Fixed the sigma setting for Fixed and Free points.
   *
   */
  void QnetSetAprioriDialog::setApriori() {

    if (m_points.size() == 0) {
      QString msg = "There are no Points selected. Please select a Point.";
      QMessageBox::warning((QWidget *)parent(), "Warning", msg);
      return;
    }

    double latSigma = Null;
    double lat = Null;
    double lonSigma = Null;
    double lon = Null;
    double radiusSigma = Null;
    double radius = Null;
    bool lineEditModified = false;

    //retrieve all of the line edit values and check if the line edits have been modified
    if (m_latLineEdit->text() != "") {
      lat = m_latLineEdit->text().toDouble();
      if (lat > 90 || lat < -90) {
        QString msg = "Invalid latitude value. Please enter a latitude value between -90 and 90.";
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }
      if (m_latLineEdit->isModified()) {
        lineEditModified = true;
      }
    }
    if (m_lonLineEdit->text() != "") {
      lon = m_lonLineEdit->text().toDouble();
      if (m_lonLineEdit->isModified()) {
        lineEditModified = true;
      }
    }
    if (m_radiusLineEdit->text() != "") {
      radius = m_radiusLineEdit->text().toDouble();
      if (m_radiusLineEdit->isModified()) {
        lineEditModified = true;
      }
    }
    if (m_latSigmaLineEdit->text() != "") {
      latSigma = m_latSigmaLineEdit->text().toDouble();
    }
    if (m_lonSigmaLineEdit->text() != "") {
      lonSigma = m_lonSigmaLineEdit->text().toDouble();
    }
    if (m_radiusSigmaLineEdit->text() != "") {
      radiusSigma = m_radiusSigmaLineEdit->text().toDouble();
    }


    //if any of the line edits have been modified then the AprioriSurfacePointSource and
    //RadiusSource are the user
    if (lineEditModified) {
      m_aprioriSource = (Source) USER;
    }

    for (int i = 0; i < m_points.size(); i++) {
      QString id = m_points.at(i)->text();
      ControlPoint *pt = m_qnetTool->controlNet()->GetPoint(id);
      if (m_points.size() == 1) {
        pt->SetAprioriSurfacePoint(SurfacePoint(
                                   Latitude(lat, Angle::Degrees),
                                   Longitude(lon, Angle::Degrees),
                                   Distance(radius,Distance::Meters)));
        if (m_aprioriSource == (Source) REFERENCE) {
          pt->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::Reference);
        }
        else if (m_aprioriSource == (Source) AVERAGE) {
          pt->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::AverageOfMeasures);
//          pt->SetAprioriRadiusSource(ControlPoint::RadiusSource::AverageOfMeasures);
        }
        else if (m_aprioriSource == (Source) USER) {
          pt->SetAprioriSurfacePointSource(ControlPoint::SurfacePointSource::User);
//          pt->SetAprioriRadiusSource(ControlPoint::RadiusSource::User);
        }
      }
      if (!pt->HasAprioriCoordinates()) {
        QString msg = "Point [" + id + "] does not have an Apriori coordinate.  "
          "Make sure to save the ground source measurement then the Point before "
          "setting the sigmas. ";
        if (m_points.size() > 1) {
          msg += "The sigmas for all of the selected points will not be set.";
        }
        else {
          msg += "The sigmas for this point will not be set.";
        }
        QMessageBox::warning((QWidget *)parent(), "Warning", msg);
        return;
      }

      try {
        //  Read Surface point from the control point and set the sigmas,
        //  first set the target radii
        SurfacePoint spt = pt->GetAprioriSurfacePoint();

        spt.SetSphericalSigmasDistance(Distance(latSigma,Distance::Meters),
                                        Distance(lonSigma,Distance::Meters),
                                        Distance(radiusSigma,Distance::Meters));

        //  Write the surface point back out to the controlPoint
        pt->SetAprioriSurfacePoint(spt);

        //  TODO:  Is the following line necessary, should error be thrown
        //  for free or fixed pts?
        //pt->SetType(ControlPoint::Constrained);
        emit pointChanged(id);
        emit netChanged();
      }
      catch (IException &e)  {
        QString message = "Error setting sigmas. \n";
        message += QString::fromStdString(e.toString());
        QMessageBox::critical((QWidget *)parent(),"Error",message);
        QApplication::restoreOverrideCursor();
        // Sigmas failed, but surface pt coordinate was set
        emit pointChanged(id);
        emit netChanged();
        return;
      }
    }
  }
}
