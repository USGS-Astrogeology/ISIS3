/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "QtieFileTool.h"

#include <QApplication>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QGridLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QPainter>

#include "Application.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "FileName.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "SerialNumber.h"
#include "Target.h"
#include "UniversalGroundMap.h"
#include "Workspace.h"

using namespace std;


namespace Isis {
  // Constructor
  QtieFileTool::QtieFileTool(QWidget *parent) : FileTool(parent) {
    openAction()->setToolTip("Open images");
    QString whatsThis = "<b>Function:</b> Open a <i>images</i> \
                        <p><b>Shortcut:</b>  Ctrl+O\n</p>";
    openAction()->setWhatsThis(whatsThis);

    saveAction()->setEnabled(false);
  }


  /**
   *  Open base image and image to be adjusted
   *
   * @author 2007-08-13 Tracie Sucharski
   *
   * @internal
   * @history 2009-06-10  Tracie Sucharski - Added error checking and looping
   *                            when opening files.  Allow new files to be
   *                            opened.
   * @history 2010-05-11  Tracie Sucharski - Added ability to read in control
   *                            network.
   * @history 2010-05-13  Tracie Sucharski - Use match cube directory to determine
   *                            default directory for cnet.
   * @history 2012-05-10  Tracie Sucharski - The FileTool::closeAll method no longer
   *                            closes viewports, so re-implemented closing of old
   *                            cube viewports before opening new.
   *
   */
  void QtieFileTool::open() {

    //  If we've already opened files, clear before starting over
    if (cubeViewportList()->size() > 0) {
      //  Close viewport containing ground source
      MdiCubeViewport *vp;
      while ((int)cubeViewportList()->size() != 0) {
        vp = (*(cubeViewportList()))[0];
        vp->parentWidget()->parentWidget()->close();
        QApplication::processEvents();
      }

      emit newFiles();
    }

    QString baseFile, matchFile;
    QString dir;
    QString filter = "Isis Cubes (*.cub);;";
    filter += "Detached labels (*.lbl);;";
    filter += "All (*)";

    Cube *baseCube = new Cube;
    UniversalGroundMap *baseGM = NULL;

    while (!baseCube->isOpen()) {
      baseFile = QFileDialog::getOpenFileName((QWidget *)parent(),
                                              "Select basemap cube (projected)",
                                              ".", filter);
      //  Cancel button
      if (baseFile.isEmpty()) {
        delete baseCube;
        baseCube = NULL;
        return;
      }

      // Find directory and save for use in file dialog for match cube
      FileName fname(baseFile);
      dir = fname.path();

      //  Make sure base is projected
      try {
        baseCube->open(baseFile);
        try {
          baseCube->projection();
        }
        catch (IException &) {
          std::string message = "Base must be projected";
          QMessageBox::critical((QWidget *)parent(), "Error", message);
          baseCube->close();
          continue;
        }

        //  Make sure we can initialize universal ground maps
        try {
          baseGM = new UniversalGroundMap(*baseCube);
        }
        catch (IException &e) {
          std::string message = "Cannot initialize universal ground map for basemap.\n";
          std::string errors = e.toString();
          message += errors;
          QMessageBox::critical((QWidget *)parent(), "Error", message);
          baseCube->close();
          continue;
        }
      }
      catch (IException &) {
        std::string message = "Unable to open base cube";
        QMessageBox::critical((QWidget *)parent(), "Error", message);
      }
    }

    Cube *matchCube = new Cube;
    UniversalGroundMap *matchGM = NULL;

    while (!matchCube->isOpen()) {
      // Get match cube
      matchFile = QFileDialog::getOpenFileName((QWidget *)parent(),
                  "Select cube to tie to base",
                  dir, filter);
      //  Cancel button
      if (matchFile.isEmpty()) {
        delete baseCube;
        baseCube = NULL;
        delete baseGM;
        baseGM = NULL;
        delete matchCube;
        matchCube = NULL;
        return;
      }

      //Make sure match is not projected
      try {
        matchCube->open(matchFile);
        try {

          if (matchCube->hasGroup("Mapping")) {
            std::string message = "The match cube cannot be a projected cube.";
            QMessageBox::critical((QWidget *)parent(), "Error", message);
            matchCube->close();
            continue;
          }
        }
        catch (IException &) {
          std::string message = "Error reading match cube labels.";
          QMessageBox::critical((QWidget *)parent(), "Error", message);
          matchCube->close();
          continue;
        }

        try {
          matchGM = new UniversalGroundMap(*matchCube);
        }
        catch (IException &e) {
          std::string message = "Cannot initialize universal ground map for match cube.\n";
          std::string errors = e.toString();
          message += errors;
          QMessageBox::critical((QWidget *)parent(), "Error", message);
          matchCube->close();
          continue;
        }
      }
      catch (IException &) {
        std::string message = "Unable to open match cube";
        QMessageBox::critical((QWidget *)parent(), "Error", message);
      }
    }

    // Find target
    QString target = matchCube->camera()->target()->name();

    // Find directory and save for use in file dialog for control net
    FileName fname(matchFile);
    dir = fname.path();

    bool netOk = false;

    ControlNet *cnet = NULL;
    while (!netOk) {
      //Open control net
      filter = "Control net (*.net);;";
      filter += "Text file (*.txt);;";
      filter += "All (*)";
      QString cnetFile = QFileDialog::getOpenFileName((QWidget *)parent(),
                         "Select a control network (Cancel will create new control network.)",
                         dir, filter);
      if (cnetFile.isEmpty()) {
        cnet = new ControlNet();
        cnet->SetNetworkId("Qtie");
        cnet->SetUserName(Application::UserName());
        //  Set control net target
        cnet->SetTarget(*matchCube->label());
        netOk = true;
      }
      else {
        try {
          QApplication::setOverrideCursor(Qt::WaitCursor);
          cnet = new ControlNet(cnetFile);
          QApplication::restoreOverrideCursor();

          netOk = checkNet(baseCube, baseGM, matchCube, matchGM, cnet);
          if (!netOk) {
            delete cnet;
            cnet = NULL;
          }
        }
        catch (IException &e) {
          std::string message = "Invalid control network.  \n";
          std::string errors = e.toString();
          message += errors;
          QMessageBox::information((QWidget *)parent(), "Error", message);
        }
      }
    }

    //  Close cubes, since the base class, FileTool will re-open and create new cubes.
    baseCube->close();
    matchCube->close();

    QApplication::restoreOverrideCursor();

    //  Open the cube viewports
    QApplication::setOverrideCursor(Qt::WaitCursor);
    emit fileSelected(baseFile);
    baseCube = (*(cubeViewportList()))[0]->cube();
    QApplication::restoreOverrideCursor();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    emit fileSelected(matchFile);
    matchCube = (*(cubeViewportList()))[1]->cube();

    //  Let QtieTool know that cubes and net are opened
    emit cubesOpened(baseCube, matchCube, cnet);


    QApplication::restoreOverrideCursor();

    return;
  }



  bool QtieFileTool::checkNet(Cube *baseCube, UniversalGroundMap *baseGM,
                              Cube *matchCube, UniversalGroundMap *matchGM, ControlNet *cnet) {

    if (cnet->GetNumPoints() == 0) {
      return true;
    }

    // Make sure targets match
    if (cnet->GetTarget() != matchCube->camera()->target()->name()) {
      std::string message = tr("Control Net target, [%1], is not the same as the cube target, [%2].")
                        .arg(QString(cnet->GetTarget()))
        .arg(QString(matchCube->camera()->target()->name()));
      QMessageBox::critical((QWidget *)parent(), "Invalid Control Network", message);
      return false;
    }

    for (int i = 0; i < cnet->GetNumPoints(); i++) {
      ControlPoint &p = *(*cnet)[i];
      double baseSamp, baseLine;

      if (p.GetNumMeasures() > 1) {
        std::string message = "Control Networks coming into Qtie can only have a single control "
                          "measure for the match cube in each control point.";
        QMessageBox::critical((QWidget *)parent(), "Invalid Control Network", message);
        return false;
      }

      //  Make sure there is a measure for the match cube
      if (!p.HasSerialNumber(SerialNumber::Compose(*matchCube))) {
        std::string message = "Cannot find a measure for the match cube";
        QMessageBox::critical((QWidget *)parent(), "Invalid Control Network", message);
        return false;
      }

      //  Make sure point on base cube
      try {
        if (baseGM->SetGround(p.GetBestSurfacePoint())) {
          baseSamp = baseGM->Sample();
          baseLine = baseGM->Line();
          if (baseSamp < 1 || baseSamp > baseCube->sampleCount() ||
              baseLine < 1 || baseLine > baseCube->lineCount()) {
            // throw error? point not on base
            std::string message = "Error parsing input control net.  Lat/Lon for Point Id: " +
                              (p.GetId()) + " computes to a sample/line off " +
                              "the edge of the basemap cube.  This point will be skipped.";
            QMessageBox::critical((QWidget *)parent(), "Invalid Control Net", message);
            return false;
          }
        }
        else {
          // throw error?  point not on base cube
          qDebug()<<"SetGround else";
          std::string message = "Error parsing input control net.  Point Id: " +
                            (p.GetId()) + " does not exist on basemap.  "
                            "This point will be skipped.";
          QMessageBox::critical((QWidget *)parent(), "Invalid Control Network", message);
          return false;
        }
      }
      catch (IException &e) {
        qDebug()<<"catch";
        std::string message = "Error in SetGround.  Error parsing input control net.  Point Id: " +
                          (p.GetId()) + " does not exist on basemap.  "
                          "This point will be skipped.";
        QMessageBox::critical((QWidget *)parent(), "Invalid Control Network", message);
        return false;
      }

      ControlMeasure *mB = new ControlMeasure;
      mB->SetCubeSerialNumber(SerialNumber::Compose(*baseCube, true));
      mB->SetCoordinate(baseSamp, baseLine);
      mB->SetDateTime();
      mB->SetChooserName(Application::UserName());
      mB->SetIgnored(true);

      p.Add(mB);
    }
    return true;
  }
}
