/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ChipViewportsWidget.h"

#include <sstream>
#include <vector>
#include <iomanip>

#include <QtWidgets>
#include <QMessageBox>
#include <QMouseEvent>

#include "Application.h"
#include "Camera.h"
#include "ChipViewport.h"
#include "ControlMeasureEditWidget.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "Pvl.h"
#include "PvlEditDialog.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "SpecialPixel.h"
#include "ToolPad.h"
#include "ViewportMainWindow.h"
#include "Workspace.h"

using namespace std;

namespace Isis {
  const int VIEWSIZE = 301;

  /**
   * Consructs the ChipViewportsWidget widget
   *
   * @param parent Pointer to the parent widget for the ChipViewportsWidget
   *
   */
  ChipViewportsWidget::ChipViewportsWidget (QWidget *parent) : QWidget(parent) {

    m_netChanged = false;

    m_parent = parent;

    // TODO 05/10/2016 TLS   What is the right way to handle serial number lists in IPCE?
    m_serialNumberList = NULL;

//  m_cubeMeasureEditMap = QMap<Cube *, ControlMeasureEditWidget *>();

    createChipViewports(parent);

    installEventFilter(this);

//  connect(this, SIGNAL(newControlNetwork(ControlNet *)),
//          m_measureEditor, SIGNAL(newControlNetwork(ControlNet *)));
  }


  ChipViewportsWidget::~ChipViewportsWidget () {
    // TODO: Don't write settings in destructor, must do this earlier in close event
//  writeSettings();

  }



  /**
   * create the widget for display all ControlMeasures for ControlPoint as ChipViewports
   *
   * @param parent Pointer to parent QWidget
   * @internal
   */
  void ChipViewportsWidget::createChipViewports(QWidget *parent) {

    setWindowTitle("ChipViewports");
    setObjectName("ChipViewportsWidget");
    connect(this, SIGNAL(destroyed(QObject *)), this, SLOT(clearPoint()));

//  createActions();

    m_chipViewportsLayout = new QGridLayout;

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(m_chipViewportsLayout);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setObjectName("ChipViewportsWidgetScroll");
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(centralWidget);
    centralWidget->adjustSize();

    QVBoxLayout *mainLayout = new QVBoxLayout;

    m_ptIdValue = new QLabel();

    QCheckBox *showPoints = new QCheckBox("Show control points");
    showPoints->setToolTip("Draw control point crosshairs");
    showPoints->setWhatsThis("This will toggle whether crosshairs are drawn"
                     " for the control points located within the measure''s"
                     " view.  For areas of dense measurements, turning this"
                     " off will allow easier viewing of features.");
    connect(showPoints, SIGNAL(toggled(bool)), this, SLOT(showPoints(bool)));
    showPoints->setChecked(true);

    QCheckBox *geomChips = new QCheckBox("Geom Chips to Reference");
    geomChips->setToolTip("Geom Chips to Reference Control Measure");
    geomChips->setWhatsThis("This will toggle whether chips are geomed to"
                     " the reference control measure.");
    connect(geomChips, SIGNAL(toggled(bool)), this, SLOT(geomChips(bool)));
    geomChips->setChecked(false);

    QHBoxLayout *pointsGeomLayout = new QHBoxLayout;
    pointsGeomLayout->addWidget(showPoints);
    pointsGeomLayout->addWidget(geomChips);
    pointsGeomLayout->addStretch();

    mainLayout->addWidget(m_ptIdValue);
    mainLayout->addLayout(pointsGeomLayout);
    mainLayout->addWidget(scrollArea);
    setLayout(mainLayout);

//  readSettings();
  }



  void ChipViewportsWidget::createActions() {

  }



  void ChipViewportsWidget::setSerialNumberList(SerialNumberList *snList) {

//    qDebug()<<"ChipViewportsWidget::setSerialNumberList  snList = "<<snList;
    // TODO   If network & snList already exists do some error checking
    m_serialNumberList = snList;
  }



  /**
   * New control network being edited
   *
   * @param cnet ControlNet *
   * @param filename Qstring,  Need filename to write to widget label.  ControlNet doesn't
   *                       contain a filename.
   * @internal
  */
  void ChipViewportsWidget::setControlNet(ControlNet *cnet, QString cnetFilename) {
//    qDebug()<<"ChipViewportsWidget::setControlNet cnet = "<<cnet<<"   filename = "<<cnetFilename;
    //  TODO  more error checking
    m_controlNet = cnet;
    m_cnetFileName = cnetFilename;
//  setWindowTitle("Control Point Editor- Control Network File: " + cnetFilename);

    //qDebug()<<"ChipViewportsWidget::setControlNet  cnetFilename = "<<cnetFilename<<"  cnet = "<<cnet;
    emit newControlNetwork(cnet);
  }



  void ChipViewportsWidget::setPoint(ControlPoint *controlPoint) {

    //  TODO  TLS 5/2/2016  Error checks - Make sure there is control net, serial list
    //            and valid edit point (make sure editpoint exists in control net).

    //qDebug()<<"ChipViewportsWidget::setEditPoint incoming ptId = "<<controlPoint->GetId();
    //  Create the measure editor widget for the reference measure first if it exists.
    //  TODO, is there always a reference measure returned, or could there be an exception thrown?

    // Create a control measure editor for each measure first since we need to get its templateFileName
    // later
    // TODO  5-23-2016 TLS   Delete measure widgets before clearing QList
//  qDebug()<<"ChipViewportsWidget::setEditPoint   #measureEditors = "<<m_measureEditors.size();
    if (m_chipViewports.size() > 0) clearPoint();
    m_controlPoint = controlPoint;

    //  Write pointId
    QString CPId = m_controlPoint->GetId();
    QString ptId("Point ID:  ");
    ptId += (QString) CPId;
    m_ptIdValue->setText(ptId);

//  m_cubeMeasureEditMap.clear();
//    qDebug()<<"ChipViewportsWidget::setEditPoint  pt = "<<controlPoint->GetId();
//  qDebug()<<"  Control Net = "<<m_controlNet;
    //  Find reference measure first,  the measure editor needs the reference measure to load the
    //  chip viewport properly (needs to geom to Reference measure).
    if (m_controlPoint->IsReferenceExplicit()) {


      Cube *measureCube = new Cube(m_serialNumberList->fileName(
                                   m_controlPoint->GetRefMeasure()->GetCubeSerialNumber()).toStdString());
      Chip *chip = new Chip(VIEWSIZE, VIEWSIZE);
      ControlMeasure *measure = m_controlPoint->GetRefMeasure();
      chip->TackCube(measure->GetSample(), measure->GetLine());
      chip->Load(*measureCube);
      ChipViewport *measureChipViewport = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
      measureChipViewport->setChip(chip, measureCube);
      measureChipViewport->setControlNet(m_controlNet);
      measureChipViewport->setPoints(true);
      m_chipViewports.append(measureChipViewport);
    }

//  qDebug()<<"ChipViewportsWidget::setPoint  Before measure loop  #measureEditors = "<<m_measureEditors.size();
//  bool referenceMeasure = false;
    for (int i = 0; i < m_controlPoint->GetNumMeasures(); i++) {
      ControlMeasure *measure = (*m_controlPoint)[i];
      if (!m_controlPoint->IsReferenceExplicit() && i == 0) {
        //  Use first as reference measure
//      referenceMeasure = true;
//      qDebug()<<"ChipViewportsWidget::setPoint  NO Skip Reference Measure i= "<<i;
      }
      else if (m_controlPoint->IsReferenceExplicit() &&
          (QString) measure->GetCubeSerialNumber() == m_controlPoint->GetReferenceSN()) {
        //  We've already added explicit reference measure, so skip
//      qDebug()<<"ChipViewportsWidget::setPoint  Skip Reference Measure i= "<<i;
        continue;
      }
//    qDebug()<<"                                                                after Test for Skip Reference Measure i= "<<i;
      Cube *measureCube = new Cube(m_serialNumberList->fileName(measure->GetCubeSerialNumber()).toStdString());
      Chip *chip = new Chip(VIEWSIZE, VIEWSIZE);
//      measure = m_controlPoint->GetRefMeasure();
      chip->TackCube(measure->GetSample(), measure->GetLine());
      chip->Load(*measureCube);
      ChipViewport *measureChipViewport = new ChipViewport(VIEWSIZE, VIEWSIZE, this);
      measureChipViewport->setChip(chip, measureCube);
      measureChipViewport->setControlNet(m_controlNet);
      measureChipViewport->setPoints(true);
      m_chipViewports.append(measureChipViewport);
    }

//  qDebug()<<"ChipViewportsWidget::setPoint  before adding editors to layout #editors = "<<m_measureEditors.size();
// TODO 5/19/2016 TLS  Clear old measure widgets before re-filling
    //  Add all measure editors to layout
    int gridDimension = sqrt(qreal(m_chipViewports.size())) -1;
    int x = 0;
    int y = 0;
    foreach(ChipViewport *measurechipViewport, m_chipViewports) {
      if (x > gridDimension) {
        x = 0;
        y++;
      }
      m_chipViewportsLayout->addWidget(measurechipViewport, y, x);
      x++;
    }
  }



  /**
   * This method is called from the constructor so that when the
   * Main window is created, it know's it's size and location.
   *
   */
#if 0
  void ChipViewportsWidget::readSettings() {
    FileName config("$HOME/.Isis/qview/ChipViewportsWidget.config");
    QSettings settings(config.expanded(),
                       QSettings::NativeFormat);
    QPoint pos = settings.value("pos", QPoint(300, 100)).toPoint();
    QSize size = settings.value("size", QSize(900, 500)).toSize();
    this->resize(size);
    this->move(pos);
  }


  /**
   * This method is called when the Main window is closed or
   * hidden to write the size and location settings to a config
   * file in the user's home directory.
   *
   */
  void ChipViewportsWidget::writeSettings() const {
    /*We do not want to write the settings unless the window is
      visible at the time of closing the application*/
    if (!this->isVisible()) return;
    FileName config("$HOME/.Isis/qview/ChipViewportsWidget.config");
    QSettings settings(config.expanded(),
                       QSettings::NativeFormat);
    settings.setValue("pos", this->pos());
    settings.setValue("size", this->size());
  }
#endif



  void ChipViewportsWidget::showPoints(bool showPoints) {

    foreach (ChipViewport *chipViewport, m_chipViewports) {
      chipViewport->setPoints(showPoints);
    }
  }



  void ChipViewportsWidget::geomChips(bool geomChips) {

    if (!geomChips) {
      for (int i=1; i<m_chipViewports.size(); i++) {
        m_chipViewports.at(i)->nogeomChip();
      }
    }

    if (geomChips) {
      for (int i=1; i<m_chipViewports.size(); i++) {
        m_chipViewports.at(i)->geomChip(m_chipViewports.at(0)->chip(),
                                        m_chipViewports.at(0)->chipCube());
      }
    }
  }



  void ChipViewportsWidget::clearPoint() {
//  qDebug()<<"ChipViewportsWidget::clearPoint #measureEditors = "<<m_measureEditors.size();
    if (m_chipViewports.size() > 0) {
      foreach (ChipViewport *measureChipViewport, m_chipViewports) {
//      qDebug()<<"                    measureEditor = "<<measureEditor;
        delete measureChipViewport;
      }
      m_chipViewports.clear();
    }
//  m_controlPoint = NULL;
  }


  bool ChipViewportsWidget::eventFilter(QObject *object, QEvent *event) {

    bool blockEvent = false;

    switch (event->type()) {
      case QEvent::MouseButtonPress: {
        mousePressEvent(object, (QMouseEvent *)event);
        blockEvent = true;
        break;
      }

      default:
        break;
    }
    return blockEvent;
  }


  void ChipViewportsWidget::mousePressEvent(QObject *object, QMouseEvent *event) {

    if (event->button() == Qt::RightButton) {
      qDebug()<<"ChipViewportsWidget::mousePressEvent  right mouse";
      //  Find child widget (ChipViewport) under the mouse
      ChipViewport *chipViewport = qobject_cast<ChipViewport *>(focusWidget());

      QMenu contextMenu;

      QAction *setReferenceMeasureAction = new QAction(tr("Set as Reference Measure"), this);
      contextMenu.addAction(setReferenceMeasureAction);

      QAction *chosenAction =
          contextMenu.exec(qobject_cast<QWidget *>(object)->mapToGlobal(event->pos()));

      if (chosenAction == setReferenceMeasureAction) {
        qDebug()<<"ChipViewportsWidget::mousePressEvent setRefMeasureAction chipViewport = "<<chipViewport;
        //setNewReferenceMeasure(chipViewport);
      }
    }
  }
}
