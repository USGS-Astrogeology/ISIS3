/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AdvancedTrackTool.h"

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QListIterator>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSize>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolBar>
#include <QVBoxLayout>

#include "Angle.h"
#include "Camera.h"
#include "CameraDistortionMap.h"
#include "CameraFocalPlaneMap.h"
#include "Distance.h"
#include "iTime.h"
#include "Longitude.h"
#include "MdiCubeViewport.h"
#include "Projection.h"
#include "RingPlaneProjection.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "TableMainWindow.h"
#include "Target.h"
#include "TProjection.h"
#include "TrackingTable.h"

namespace Isis {

  // For mosaic tracking
#define FLOAT_MIN         -16777215

  /**
   * Constructs an AdvancedTrackTool object
   *
   * @param parent
   */
  AdvancedTrackTool::AdvancedTrackTool(QWidget *parent) : Tool(parent) {
    p_tableWin = new TableMainWindow("Advanced Tracking", parent);
    p_tableWin->setTrackListItems(true);
    connect(p_tableWin, SIGNAL(fileLoaded()), this, SLOT(updateID()));

    p_action = new QAction(parent);
    p_action->setText("Tracking ...");
    p_action->setIcon(QPixmap(toolIconDir() + "/goto.png"));
    p_action->setShortcut(Qt::CTRL + Qt::Key_T);
    p_action->setWhatsThis("<b>Function: </b> Opens the Advanced Tracking Tool \
                           window. This window will track sample/line positions,\
                           lat/lon positions, and many other pieces of \
                           information.  All of the data in the window can be \
                           saved to a text file. <p><b>Shortcut: </b> Ctrl+T</p>");
    connect(p_action, SIGNAL(triggered()), p_tableWin, SLOT(showTable()));
    activate(true);
    connect(p_action, SIGNAL(triggered()), p_tableWin, SLOT(raise()));
    connect(p_action, SIGNAL(triggered()), p_tableWin, SLOT(syncColumns()));
    p_tableWin->installEventFilter(this);

    // Adds each item of checkBoxItems to the table.
    // If a tool tip is specified, we cannot skip parameters, so -1 and
    // Qt::Horizontal are specified.
    QList< QList<QString> >::iterator iter;
    for (iter = checkBoxItems.begin(); iter != checkBoxItems.end(); ++iter) {
      QList<QString> currentList = *iter;
      QString header = currentList[0];
      QString menuText = currentList[2];
      QString toolTip = currentList[3];
      bool onByDefault;
      if (currentList[1] == QString("true")) {
        onByDefault = true;
      }
      else {
        onByDefault = false;
      }

      if (toolTip != QString("")) {
        p_tableWin->addToTable(onByDefault, header, menuText,
                          -1, Qt::Horizontal, toolTip);
      }
      else {
        p_tableWin->addToTable(onByDefault, header, menuText);
      }
    }

    //This variable will keep track of how many times
    // the user has issued the 'record' command.
    p_id = 0;

    // Setup 10 blank rows in the table
    for(int r = 0; r < 10; r++) {
      p_tableWin->table()->insertRow(r);
      for(int c = 0; c < p_tableWin->table()->columnCount(); c++) {
        QTableWidgetItem *item = new QTableWidgetItem("");
        p_tableWin->table()->setItem(r, c, item);
      }
    }

    // Create the action for recording points
    QAction *recordAction = new QAction(parent);
    recordAction->setShortcut(Qt::Key_R);
    parent->addAction(recordAction);
    connect(recordAction, SIGNAL(triggered()), this, SLOT(record()));
    p_tableWin->setStatusMessage("To record press the R key"
                                 "  ---  Double click on a cell to enable crtl+c (copy) and"
                                 " ctrl+v (paste).");

    // Add a help menu to the menu bar
    QMenuBar *menuBar = p_tableWin->menuBar();
    QMenu *helpMenu = menuBar->addMenu("&Help");
    QAction *help = new QAction(p_tableWin);
    help->setText("&Tool Help");
    help->setShortcut(Qt::CTRL + Qt::Key_H);
    connect(help, SIGNAL(triggered()), this, SLOT(helpDialog()));
    helpMenu->addAction(help);
    p_tableWin->setMenuBar(menuBar);
    installEventFilter(p_tableWin);

    m_showHelpOnStart = true;
    readSettings();
  }

  /**
   * An event filter that calls methods on certain events.
   *
   * @param o
   * @param e
   *
   * @return bool
   */
  bool AdvancedTrackTool::eventFilter(QObject *o, QEvent *e) {
    if(e->type() == QEvent::Show) {
      activate(true);
      if (m_showHelpOnStart) {
        helpDialog();
        m_showHelpOnStart = false;
        writeSettings();
      }
    }
    else if(e->type() == QEvent::Hide) {
      activate(false);
    }
    return Tool::eventFilter(o, e);
  }


  /**
   * This method adds the action to bring up the track tool to the menu.
   *
   * @param menu
   */
  void AdvancedTrackTool::addTo(QMenu *menu) {
    menu->addAction(p_action);
  }

  /**
   * This method adds the action to bring up the track tool to the permanent tool
   * bar.
   *
   * @param perm
   */
  void AdvancedTrackTool::addToPermanent(QToolBar *perm) {
    perm->addAction(p_action);
  }

  /**
   * This method is called when the mouse has moved across the viewport and
   * updates the row accordingly.
   *
   * @param p
   */
  void AdvancedTrackTool::mouseMove(QPoint p) {
    updateRow(p);
  }

  /**
   * This method is called when the mouse leaves the viewport and clears any rows
   * accordingly.
   *
   */
  void AdvancedTrackTool::mouseLeave() {

    if(cubeViewport()->isLinked()) {
      for(int i = 0; i < p_numRows; i++) {
        p_tableWin->clearRow(i + p_tableWin->currentRow());
      }
    }
    else {
      p_tableWin->clearRow(p_tableWin->currentRow());
    }

  }

  /**
   * This method updates the row with data from the point given.
   *
   * @param p
   */
  void AdvancedTrackTool::updateRow(QPoint p) {
    MdiCubeViewport *cvp = cubeViewport();
    if(cvp == NULL) {
      p_tableWin->clearRow(p_tableWin->currentRow());
      return;
    }

    if(!cubeViewport()->isLinked()) {
      updateRow(cvp, p, p_tableWin->currentRow());
      p_numRows = 1;
    }
    else {
      p_numRows = 0;
      for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
        MdiCubeViewport *d = (*(cubeViewportList()))[i];
        if(d->isLinked()) {
          updateRow(d, p, p_tableWin->currentRow() + p_numRows);
          p_numRows++;
        }
      }
    }
  }


    /**
     * This method finds the index of the header in checkBoxItems by looping
     * through checkBoxItems, grabbing the header from each QList, and parsing
     * the header at ":" to account for check boxes selecting multiple columns.
     *
     * @param keyword Header to be found
     * @return int The index of the item to be added
     */
    int AdvancedTrackTool::getIndex(QString keyword) {
      int index = 0;
      QList< QList<QString> >::iterator iter;
      for (iter = checkBoxItems.begin(); iter != checkBoxItems.end(); ++iter) {
        QList<QString> currentList = *iter;
        QList<QString> splitHeader = currentList[0].split(":");
        QList<QString>::iterator headerIter;
        for (headerIter = splitHeader.begin(); headerIter != splitHeader.end(); ++headerIter) {
          QString header = *headerIter;
          if (header.toLower() == keyword.toLower()) {
            return index;
          }
          index++;
      }
    }
    IString msg = "Header [" + keyword + "] not found; make sure spelling is correct";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  /**
   * This method updates the row given with data from the viewport cvp at point p.
   *
   * @param cvp CubeViewPort that contains p
   * @param p   QPoint from which the row will be updated
   * @param row Row to be updated
   */
  void AdvancedTrackTool::updateRow(MdiCubeViewport *cvp, QPoint p, int row) {
    // Get the sample line position to report
    double sample, line;
    cvp->viewportToCube(p.x(), p.y(), sample, line);
    int isample = int (sample + 0.5);
    int iline = int (line + 0.5);

    /*if there are linked cvp's then we want to highlight (select)
    the row of the active cvp.*/
    if(cvp->isLinked()) {

      if(cvp == cubeViewport()) {
        p_tableWin->table()->selectRow(row);
      }

    }


    // Do we need more rows?
    if(row + 1 > p_tableWin->table()->rowCount()) {
      p_tableWin->table()->insertRow(row);
      for(int c = 0; c < p_tableWin->table()->columnCount(); c++) {
        QTableWidgetItem *item = new QTableWidgetItem("");
        p_tableWin->table()->setItem(row, c, item);
        if(c == 0) p_tableWin->table()->scrollToItem(item);
      }
    }

    // Blank out the row to remove stuff left over from previous cvps
    for(int c = 0; c < p_tableWin->table()->columnCount(); c++) {
      p_tableWin->table()->item(row, c)->setText("");
    }

    // Don't write anything if we are outside the cube
    if(sample < 0.5) return;
    if(line < 0.5) return;
    if(sample > cvp->cubeSamples() + 0.5) return;
    if(line > cvp->cubeLines() + 0.5) return;

    // Write cols 0-2 (id, sample, line)
    p_tableWin->table()->item(row, getIndex("ID"))->setText(QString::number(p_id));
    p_tableWin->table()->item(row, getIndex("Sample"))->setText(QString::number(sample));
    p_tableWin->table()->item(row, getIndex("Line"))->setText(QString::number(line));

    // Write col 3 (band)
    if (cvp->isGray()) {
      p_tableWin->table()->item(row, getIndex("Band"))->setText(QString::number(cvp->grayBand()));
    }
    else {
      p_tableWin->table()->item(row, getIndex("Band"))->setText(QString::number(cvp->redBand()));
    }

    // Write out the path, filename, and serial number
    FileName fname = FileName(cvp->cube()->fileName().toStdString()).expanded();
    QString fnamePath = QString::fromStdString(fname.path());
    QString fnameName = QString::fromStdString(fname.name());
    p_tableWin->table()->item(row, getIndex("Path"))->setText(fnamePath);
    p_tableWin->table()->item(row, getIndex("FileName"))->setText(fnameName);
    if (!cvp->cube()->hasGroup("Tracking") && !cvp->cube()->hasTable("InputImages")){
      p_tableWin->table()->item(row, getIndex("Serial Number"))->setText(SerialNumber::Compose(*cvp->cube()));
    }

    // If we are outside of the image then we are done
    if((sample < 0.5) || (line < 0.5) ||
        (sample > cvp->cubeSamples() + 0.5) ||
        (line > cvp->cubeLines() + 0.5)) {
      return;
    }

    // Otherwise write out col 4 (Pixel value)
    QString pixel;
    if(cvp->isGray()) {
      pixel = PixelToString(cvp->grayPixel(isample, iline), 12);
    }
    else {
      pixel = PixelToString(cvp->redPixel(isample, iline), 12);
    }
    p_tableWin->table()->item(row, getIndex("Pixel"))->setText(pixel);

    // Do we have a camera model?
    if(cvp->camera() != NULL) {
      if(cvp->camera()->SetImage(sample, line)) {
        if (cvp->camera()->target()->isSky()) {
          double dec = cvp->camera()->Declination();
          double ra = cvp->camera()->RightAscension();
          p_tableWin->table()->item(row, getIndex("Right Ascension"))->
                                setText(QString::number(ra, 'f', 15));
          p_tableWin->table()->item(row, getIndex("Declination"))->
                                setText(QString::number(dec, 'f', 15));
        }
        else {
          // Write columns ocentric lat/lon, and radius, only if set image succeeds
          double lat = cvp->camera()->UniversalLatitude();
          double lon = cvp->camera()->UniversalLongitude();

          double radius = cvp->camera()->LocalRadius().meters();
          p_tableWin->table()->item(row, getIndex("Planetocentric Latitude"))->
                               setText(QString::number(lat, 'f', 15));
          p_tableWin->table()->item(row, getIndex("360 Positive East Longitude"))->
                               setText(QString::number(lon, 'f', 15));
          p_tableWin->table()->item(row, getIndex("Local Radius"))->
                               setText(QString::number(radius, 'f', 15));

          /* 180 Positive East Lon. */
          p_tableWin->table()->item(row, getIndex("180 Positive East Longitude"))->
                               setText(QString::number(TProjection::To180Domain(lon), 'f', 15));

          // Write out the planetographic and positive west values, only if set image succeeds
          lon = -lon;
          while(lon < 0.0) lon += 360.0;
          Distance radii[3];
          cvp->camera()->radii(radii);
          lat = TProjection::ToPlanetographic(lat, radii[0].meters(), radii[2].meters());
          p_tableWin->table()->item(row, getIndex("Planetographic Latitude"))->
                               setText(QString::number(lat, 'f', 15));
          p_tableWin->table()->item(row, getIndex("360 Positive West Longitude"))->
                               setText(QString::number(lon, 'f', 15));

          /*180 Positive West Lon.  */
          p_tableWin->table()->item(row, getIndex("180 Positive West Longitude"))->setText(
                               QString::number(TProjection::To180Domain(lon), 'f', 15));

          // Next write out columns, the x/y/z position of the lat/lon, only if set image succeeds
          double pos[3];
          cvp->camera()->Coordinate(pos);
          p_tableWin->table()->item(row, getIndex("Point X"))->setText(QString::number(pos[0]));
          p_tableWin->table()->item(row, getIndex("Point Y"))->setText(QString::number(pos[1]));
          p_tableWin->table()->item(row, getIndex("Point Z"))->setText(QString::number(pos[2]));

          // Write out columns resolution, only if set image succeeds
          double res = cvp->camera()->PixelResolution();
          if (res != -1.0) {
            p_tableWin->table()->item(row, getIndex("Resolution"))->setText(QString::number(res));
          }
          else {
            p_tableWin->table()->item(row, getIndex("Resolution"))->setText("");
          }

          // Write out columns, oblique pixel resolution, only if set image succeeds
          double obliquePRes = cvp->camera()->ObliquePixelResolution();
          if (obliquePRes != Isis::Null) {
            p_tableWin->table()->item(row, getIndex("Oblique Pixel Resolution"))->
                                 setText(QString::number(obliquePRes));
          }
          else {
            p_tableWin->table()->item(row, getIndex("Oblique Pixel Resolution"))->setText("");
          }

          // Write out columns photometric angle values, only if set image succeeds
          double phase = cvp->camera()->PhaseAngle();
          p_tableWin->table()->item(row, getIndex("Phase"))->setText(QString::number(phase));
          double incidence = cvp->camera()->IncidenceAngle();
          p_tableWin->table()->item(row, getIndex("Incidence"))->setText(QString::number(incidence));
          double emission = cvp->camera()->EmissionAngle();
          p_tableWin->table()->item(row, getIndex("Emission"))->setText(QString::number(emission));

          // Write out columns local incidence and emission, only if set image
          // succeeds.  This might fail if there are holes in the DEM.
          // Calculates the angles local to the slope for the DEMs, compare against
          // the incidence and emission angles calculated for the sphere
          Angle phaseAngle, incidenceAngle, emissionAngle;
          bool bSuccess = false;
          cvp->camera()->LocalPhotometricAngles(phaseAngle, incidenceAngle, emissionAngle, bSuccess);
          if(bSuccess) {
            p_tableWin->table()->item(row, getIndex("LocalIncidence"))->
                                 setText(QString::number(incidenceAngle.degrees()));
            p_tableWin->table()->item(row, getIndex("LocalEmission"))->
                                 setText(QString::number(emissionAngle.degrees()));
          }
          else {
            p_tableWin->table()->item(row, getIndex("LocalIncidence"))->setText("");
            p_tableWin->table()->item(row, getIndex("LocalEmission"))->setText("");
          }

          // If set image succeeds, write out columns north azimuth, sun azimuth, solar longitude
          // north azimuth is meaningless for ring plane projections
          double northAzi = cvp->camera()->NorthAzimuth();
          if (cvp->camera()->target()->shape()->name() != "Plane"
              && Isis::IsValidPixel(northAzi)) {
            p_tableWin->table()->item(row, getIndex("North Azimuth"))->
                                 setText(QString::number(northAzi));
          }
          else { // north azimuth is meaningless for ring plane projections
            p_tableWin->table()->item(row, getIndex("North Azimuth"))->setText("");
          }


          try {
            double sunAzi = cvp->camera()->SunAzimuth();
            if (Isis::IsValidPixel(sunAzi)) {
              p_tableWin->table()->item(row, getIndex("Sun Azimuth"))->
                                   setText(QString::number(sunAzi));
            }
            else { // sun azimuth is null
              p_tableWin->table()->item(row, getIndex("Sun Azimuth"))->setText("");
            }
          }
          catch(IException &e) {
            p_tableWin->table()->item(row, getIndex("Sun Azimuth"))->setText("");
          }


          double spacecraftAzi = cvp->camera()->SpacecraftAzimuth();
          if (Isis::IsValidPixel(spacecraftAzi)) {
            p_tableWin->table()->item(row, getIndex("Spacecraft Azimuth"))->
                                 setText(QString::number(spacecraftAzi));
          }
          else { // spacecraft azimuth is null
            p_tableWin->table()->item(row, getIndex("Spacecraft Azimuth"))->setText("");
          }

          // Write out columns solar lon, slant distance, local solar time
          try {
            double solarLon = cvp->camera()->solarLongitude().degrees();
            p_tableWin->table()->item(row, getIndex("Solar Longitude"))->
                                 setText(QString::number(solarLon));
          }
          catch (IException &e) {
            p_tableWin->table()->item(row, getIndex("Solar Longitude"))->
                                 setText("");
          }

          double slantDistance = cvp->camera()->SlantDistance();
          p_tableWin->table()->item(row, getIndex("Slant Distance"))->
                               setText(QString::number(slantDistance));
          try {
            double lst = cvp->camera()->LocalSolarTime();
            p_tableWin->table()->item(row, getIndex("Local Solar Time"))->
                                 setText(QString::number(lst));
          }
          catch (IException &e) {
            p_tableWin->table()->item(row, getIndex("Local Solar Time"))->
                                 setText("");
          }
        }

      } // end if set image succeeds

      // Always write out the x/y/z of the undistorted focal plane
      if (cvp->camera()->DistortionMap() != NULL) {
        CameraDistortionMap *distortedMap = cvp->camera()->DistortionMap();
        double undistortedFocalPlaneX = distortedMap->UndistortedFocalPlaneX();
        p_tableWin->table()->item(row, getIndex("Undistorted Focal X"))->
                             setText(QString::number(undistortedFocalPlaneX));
        double undistortedFocalPlaneY = distortedMap->UndistortedFocalPlaneY();
        p_tableWin->table()->item(row, getIndex("Undistorted Focal Y"))->
                             setText(QString::number(undistortedFocalPlaneY));
        double undistortedFocalPlaneZ = distortedMap->UndistortedFocalPlaneZ();
        p_tableWin->table()->item(row, getIndex("Undistorted Focal Z"))->
                             setText(QString::number(undistortedFocalPlaneZ));
      }
      else {
        p_tableWin->table()->item(row, getIndex("Undistorted Focal X"))->
                             setText("");
        p_tableWin->table()->item(row, getIndex("Undistorted Focal Y"))->
                             setText("");
        p_tableWin->table()->item(row, getIndex("Undistorted Focal Z"))->
                             setText("");
      }

      // Always write out the x/y of the distorted focal plane
      if (cvp->camera()->FocalPlaneMap() != NULL) {
        CameraFocalPlaneMap *focalPlaneMap = cvp->camera()->FocalPlaneMap();
        double distortedFocalPlaneX = focalPlaneMap->FocalPlaneX();
        p_tableWin->table()->item(row, getIndex("Focal Plane X"))->
                             setText(QString::number(distortedFocalPlaneX));
        double distortedFocalPlaneY = focalPlaneMap->FocalPlaneY();
        p_tableWin->table()->item(row, getIndex("Focal Plane Y"))->
                             setText(QString::number(distortedFocalPlaneY));
      }
      else {
        p_tableWin->table()->item(row, getIndex("Focal Plane X"))->
                             setText("");
        p_tableWin->table()->item(row, getIndex("Focal Plane Y"))->
                             setText("");  
      }


      // Always write out columns ra/dec, regardless of whether set image succeeds
      double ra = cvp->camera()->RightAscension();
      p_tableWin->table()->item(row, getIndex("Right Ascension"))->setText(QString::number(ra));
      double dec = cvp->camera()->Declination();
      p_tableWin->table()->item(row, getIndex("Declination"))->setText(QString::number(dec));

      // Always write out columns et and utc, regardless of whether set image succeeds
      iTime time(cvp->camera()->time());
      p_tableWin->table()->item(row, getIndex("Ephemeris Time"))->
                           setText(QString::number(time.Et(), 'f', 15));
      QString time_utc = time.UTC();
      p_tableWin->table()->item(row, getIndex("UTC"))->setText(time_utc);

      // Always out columns spacecraft position, regardless of whether set image succeeds
      double pos[3];
      cvp->camera()->instrumentPosition(pos);
      p_tableWin->table()->item(row, getIndex("Spacecraft X"))->setText(QString::number(pos[0]));
      p_tableWin->table()->item(row, getIndex("Spacecraft Y"))->setText(QString::number(pos[1]));
      p_tableWin->table()->item(row, getIndex("Spacecraft Z"))->setText(QString::number(pos[2]));
    }

    else if (cvp->projection() != NULL) {
      // Determine the projection type
      Projection::ProjectionType projType = cvp->projection()->projectionType();

      if (cvp->projection()->SetWorld(sample, line)) {
        if (projType == Projection::Triaxial) {
          TProjection *tproj = (TProjection *) cvp->projection();
          double lat = tproj->UniversalLatitude();
          double lon = tproj->UniversalLongitude();

          double glat = tproj->ToPlanetographic(lat);
          double wlon = -lon;
          while(wlon < 0.0) wlon += 360.0;
          if (tproj->IsSky()) {
            lon = tproj->Longitude();
            p_tableWin->table()->item(row, getIndex("Right Ascension"))->
                                 setText(QString::number(lon, 'f', 15));
            p_tableWin->table()->item(row, getIndex("Declination"))->
                                 setText(QString::number(lat, 'f', 15));
          }
          else {
            double radius = tproj->LocalRadius();
            p_tableWin->table()->item(row, getIndex("Planetocentric Latitude"))->
                                 setText(QString::number(lat, 'f', 15));
            p_tableWin->table()->item(row, getIndex("Planetographic Latitude"))->
                                 setText(QString::number(glat, 'f', 15));
            p_tableWin->table()->item(row, getIndex("360 Positive East Longitude"))->
                                 setText(QString::number(lon, 'f', 15));
            p_tableWin->table()->item(row, getIndex("180 Positive East Longitude"))->
                                 setText(QString::number(TProjection::To180Domain(lon), 'f', 15));
            p_tableWin->table()->item(row, getIndex("360 Positive West Longitude"))->
                                 setText(QString::number(wlon, 'f', 15));
            p_tableWin->table()->item(row, getIndex("180 Positive East Longitude"))->
                                 setText(QString::number(TProjection::To180Domain(wlon), 'f', 15));
            p_tableWin->table()->item(row, getIndex("Local Radius"))->setText(QString::number(radius, 'f', 15));
          }
        }
        else { // RingPlane
          RingPlaneProjection *rproj = (RingPlaneProjection *) cvp->projection();
          double lat = rproj->UniversalRingRadius();
          double lon = rproj->UniversalRingLongitude();

          double wlon = -lon;
          while(wlon < 0.0) wlon += 360.0;
          double radius = lat;
          p_tableWin->table()->item(row, getIndex("Planetocentric Latitude"))->setText("0.0");
          p_tableWin->table()->item(row, getIndex("Planetographic Latitude"))->setText("0.0");
          p_tableWin->table()->item(row, getIndex("360 Positive East Longitude"))->
                               setText(QString::number(lon, 'f', 15));
          p_tableWin->table()->item(row, getIndex("180 Positive East Longitude"))->
                               setText(QString::number(RingPlaneProjection::To180Domain(lon), 'f', 15));
          p_tableWin->table()->item(row, getIndex("360 Positive West Longitude"))->
                               setText(QString::number(wlon, 'f', 15));
          p_tableWin->table()->item(row, getIndex("180 Positive West Longitude"))->
                               setText(QString::number(RingPlaneProjection::To180Domain(wlon), 'f', 15));
          p_tableWin->table()->item(row, getIndex("Local Radius"))->
                               setText(QString::number(radius, 'f', 15));
        }
      }
    }

    //If there is a projection add the Projected X and Y coords to the table
    if(cvp->projection() != NULL) {
      if(cvp->projection()->SetWorld(sample, line)) {
        double projX = cvp->projection()->XCoord();
        double projY = cvp->projection()->YCoord();
        p_tableWin->table()->item(row, getIndex("Projected X"))->
                             setText(QString::number(projX, 'f', 15));
        p_tableWin->table()->item(row, getIndex("Projected Y"))->
                             setText(QString::number(projY, 'f', 15));
      }
    }

    // Track the Mosaic Origin -  Index (Zero based) and FileName
    if (cvp->cube()->hasTable("InputImages") || cvp->cube()->hasGroup("Tracking")) {
      int iMosaicOrigin = -1;
      QString sSrcFileName = "";
      QString sSrcSerialNum = "";
      TrackMosaicOrigin(cvp, iline, isample, iMosaicOrigin, sSrcFileName, sSrcSerialNum);
      p_tableWin->table()->item(row, getIndex("Track Mosaic Index"))->
                           setText(QString::number(iMosaicOrigin));
      p_tableWin->table()->item(row, getIndex("Track Mosaic FileName"))->
                           setText(QString(sSrcFileName));
      p_tableWin->table()->item(row, getIndex("Track Mosaic Serial Number"))->
                           setText(QString(sSrcSerialNum));
    }
  }


  /**
   * TrackMosaicOrigin - Given the pointer to Cube and line and
   *   sample index, finds the origin of the mosaic if the TRACKING
   *   band and Mosaic Origin Table  exists.
   *
   * @author sprasad (11/16/2009)
   *
   * @param cvp           - Points to the CubeViewPort
   * @param piLine        - Line Index
   * @param piSample      - Sample Index
   * @param piOrigin      - Contains the Src Index (zero based)
   * @param psSrcFileName - Contains the Src FileName
   * @param psSrcSerialNum- Contains the Src Serial Number
   *
   * @return void
   */
  void AdvancedTrackTool::TrackMosaicOrigin(MdiCubeViewport *cvp, int piLine,
      int piSample, int &piOrigin, QString &psSrcFileName,
      QString &psSrcSerialNum) {
      try {
        Cube *cCube = cvp->cube();
        int iTrackBand = -1;

        // This is a mosaic in the new format or the external tracking cube itself
        if(cCube->hasGroup("Tracking") ||
                (cCube->hasTable(trackingTableName) && cCube->bandCount() == 1)) {
          Cube *trackingCube;
          if(cCube->hasGroup("Tracking")) {
            trackingCube = cvp->trackingCube();
          }
          else {
            trackingCube = cCube;
          }

          // Read the cube DN value from TRACKING cube at location (piLine, piSample)
          Portal trackingPortal(trackingCube->sampleCount(), 1, trackingCube->pixelType());
          trackingPortal.SetPosition(piSample, piLine, 1);
          trackingCube->read(trackingPortal);

          unsigned int currentPixel = trackingPortal[0];
          if (currentPixel != NULLUI4) {  // If from an image
            Table table = trackingCube->readTable(trackingTableName); // trackingTableName from TrackingTable
            TrackingTable trackingTable(table);

            FileName trackingFileName = trackingTable.pixelToFileName(currentPixel);
            psSrcFileName = QString::fromStdString(trackingFileName.name());
            psSrcSerialNum = trackingTable.pixelToSN(currentPixel);
            piOrigin = trackingTable.fileNameToIndex(trackingFileName, psSrcSerialNum);
          }
        }
        // Backwards compatability. Have this tool work with attached TRACKING bands
        else if(cCube->hasTable(trackingTableName)) {
          Pvl *cPvl = cCube->label();
          PvlObject cObjIsisCube = cPvl->findObject("IsisCube");
          PvlGroup cGrpBandBin = cObjIsisCube.findGroup("BandBin");
          for(int i = 0; i < cGrpBandBin.keywords(); i++) {
            PvlKeyword &cKeyTrackBand = cGrpBandBin[i];
            for(int j = 0; j < cKeyTrackBand.size(); j++) {
              if(cKeyTrackBand[j] == "TRACKING") {
                iTrackBand = j;
                break;
              }
            }
          }

          if(iTrackBand > 0 && iTrackBand <= cCube->bandCount()) {
            Portal cOrgPortal(cCube->sampleCount(), 1,
                              cCube->pixelType());
            cOrgPortal.SetPosition(piSample, piLine, iTrackBand + 1); // 1 based
            cCube->read(cOrgPortal);
            piOrigin = (int)cOrgPortal[0];
            switch(SizeOf(cCube->pixelType())) {
              case 1:
                piOrigin -= VALID_MIN1;
                break;

              case 2:
                piOrigin -= VALID_MIN2;
                break;

              case 4:
                piOrigin -= FLOAT_MIN;
                break;
            }

            // Get the input file name and serial number
            Table cFileTable = cCube->readTable(trackingTableName);
            int iRecs =   cFileTable.Records();
            if(piOrigin >= 0 && piOrigin < iRecs) {
              psSrcFileName = QString::fromStdString(cFileTable[piOrigin][0]);
              psSrcSerialNum = QString::fromStdString(cFileTable[piOrigin][1]);
            }
          }
        }

      }
      catch (IException &e) {
          // This gets called too frequently to raise a warning; so, suppress the error
          // and return invalid.
          piOrigin = -1;
      }

      if (piOrigin == -1) { // If not from an image, display N/A
        psSrcFileName = "N/A";
        psSrcSerialNum = "N/A";
      }
  }


  /**
   * This method creates a dialog box that shows help tips. It is displayed when the tool is
   *   opened the first time (unless the user says otherwise) and when the user opens it through
   *   the help menu.
   */
  void AdvancedTrackTool::helpDialog() {

      QDialog  *helpDialog = new QDialog(p_tableWin);

      QVBoxLayout *dialogLayout = new QVBoxLayout;
      helpDialog->setLayout(dialogLayout);
      QLabel *dialogTitle = new QLabel("<h3>Advanced Tracking Tool</h3>");
      dialogLayout->addWidget(dialogTitle);

      QTabWidget *tabArea = new QTabWidget;
      dialogLayout->addWidget(tabArea);

      QScrollArea *recordTab = new QScrollArea;
      QWidget *recordContainer = new QWidget;
      QVBoxLayout *recordLayout = new QVBoxLayout;
      recordContainer->setLayout(recordLayout);
      QLabel *recordText = new QLabel("To record, click on the viewport of interest and"
                                      " press (r) while the mouse is hovering over the image.");
      recordText->setWordWrap(true);
      recordLayout->addWidget(recordText);
      recordTab->setWidget(recordContainer);

      QScrollArea *helpTab = new QScrollArea;
      QWidget *helpContainer = new QWidget;
      QVBoxLayout *helpLayout = new QVBoxLayout;
      helpContainer->setLayout(helpLayout);
      QLabel *helpText = new QLabel("In order to use <i>ctrl+c</i> to copy and <i>ctrl+v</i> to"
                                    " paste, you need to double click on the cell you are copying"
                                    " from (the text should be highlighted). Then double click on"
                                    " the cell you are pasting to (you should see a cursor if the"
                                    " cell is blank). When a cell is in this editing mode, most"
                                    " keyboard shortcuts work.");
      helpText->setWordWrap(true);
      recordText->setWordWrap(true);
      helpLayout->addWidget(helpText);
      helpTab->setWidget(helpContainer);

      tabArea->addTab(recordTab, "Record");
      tabArea->addTab(helpTab, "Table Help");

      QPushButton *okButton = new QPushButton("OK");
      dialogLayout->addStretch();
      dialogLayout->addWidget(okButton);
      helpDialog->show();
      connect(okButton, SIGNAL(clicked()), helpDialog, SLOT(accept()));
  }


  /**
   * This method records data to the current row.
   *
   */
  void AdvancedTrackTool::record() {
    if(p_tableWin->table()->isHidden()) return;
    if(p_tableWin->table()->item(p_tableWin->currentRow(), 0)->text() == "") return;

    int row = 0;
    p_tableWin->setCurrentRow(p_tableWin->currentRow() + p_numRows);
    p_tableWin->setCurrentIndex(p_tableWin->currentIndex() + p_numRows);
    while(p_tableWin->currentRow() >= p_tableWin->table()->rowCount()) {

      row = p_tableWin->table()->rowCount();

      p_tableWin->table()->insertRow(row);
      for(int c = 0; c < p_tableWin->table()->columnCount(); c++) {
        QTableWidgetItem *item = new QTableWidgetItem("");
        p_tableWin->table()->setItem(row, c, item);
      }
    }

    QApplication::sendPostedEvents(p_tableWin->table(), 0);
    p_tableWin->table()->scrollToItem(p_tableWin->table()->item(p_tableWin->currentRow(), 0),
                                      QAbstractItemView::PositionAtBottom);

    //Keep track of number times user presses 'R' (record command)
    p_id = p_tableWin->table()->item(p_tableWin->currentRow() - 1, 0)->text().toInt() + 1;
  }


  /**
   * This slot updates the row with data from the point given and
   *   records data to the current row.
   *
   * @param p   QPoint from which the row(s) will be updated and
   *            recorded.
   * @return void
   * @author Jeannie Walldren
   *
   * @internal
   *  @history 2010-03-08 - Jeannie Walldren - This slot was
   *           added to be connected to the FindTool recordPoint()
   *           signal in qview.
   *  @history 2010-05-07 - Eric Hyer - Now shows the table as well
   *  @history 2017-11-13 - Adam Goins - Made the call to showTable() first
   *                            So that the table draws the recorded point if
   *                            the point is the first point record. Fixes #5143.
   */
  void AdvancedTrackTool::record(QPoint p) {
    p_tableWin->showTable();
    updateRow(p);
    record();
  }


  /**
   * This method updates the record ID.
   *
   */
  void AdvancedTrackTool::updateID() {
    //Check if the current row is 0
    if(p_tableWin->currentRow() == 0)
      p_id = 0;
    else
      p_id = p_tableWin->table()->item(p_tableWin->currentRow() - 1, getIndex("ID"))->text().toInt() + 1;
  }


  /**
   * Read this tool's preserved state. This uses the current state as defaults,
   *   so please make sure your variables are initialized before calling this
   *   method.
   */
  void AdvancedTrackTool::readSettings() {

    QSettings settings(settingsFilePath() , QSettings::NativeFormat);

    m_showHelpOnStart = settings.value("showHelpOnStart", m_showHelpOnStart).toBool();
  }


  /**
   * Write out this tool's preserved state between runs. This is NOT called on
   *   close, so you should call this any time you change the preserved state.
   */
  void AdvancedTrackTool::writeSettings() {

    QSettings settings(settingsFilePath(), QSettings::NativeFormat);

    settings.setValue("showHelpOnStart", m_showHelpOnStart);
  }


  /**
   * Generate the correct path for the config file.
   *
   * @return the config file path
   */
  QString AdvancedTrackTool::settingsFilePath() const {

    if (QApplication::applicationName() == "") {
      throw IException(IException::Programmer, "You must set QApplication's "
          "application name before using the Isis::MainWindow class. Window "
          "state and geometry can not be saved and restored", _FILEINFO_);
    }

    FileName config(FileName("$HOME/.Isis/" + QApplication::applicationName().toStdString() + "/").path() + "/" +
                    "advancedTrackTool.config");

    return QString::fromStdString(config.expanded());
  }
}
