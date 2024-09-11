#include "FindTool.h"

#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QValidator>

#include <float.h>

#include "Camera.h"
#include "Distance.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "Projection.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "ToolPad.h"
#include "UniversalGroundMap.h"
#include "Workspace.h"


namespace Isis {
  /**
   * Constructs a FindTool object
   *
   * @param parent
   */
  FindTool::FindTool(QWidget *parent) : Tool(parent) {
    p_dialog = NULL;
    p_findPoint = NULL;
    p_showDialogButton = NULL;
    p_linkViewportsButton = NULL;
    p_syncScale = NULL;
    p_statusEdit = NULL;
    p_tabWidget = NULL;
    p_groundTab = NULL;
    p_imageTab = NULL;

    p_lat = DBL_MAX;
    p_lon = DBL_MAX;
    p_samp = DBL_MAX;
    p_line = DBL_MAX;
    p_pointVisible = true;

    //Set up dialog box
    createDialog(parent);

    // Set up find point action
    p_findPoint = new QAction(parent);
    p_findPoint->setShortcut(Qt::CTRL + Qt::Key_F);
    p_findPoint->setText("&Find Point");
    p_findPoint->setIcon( QPixmap(toolIconDir() + "/find.png") );
    QString text =
      "<b>Function:</b> Centers all linked viewports to the selected lat/lon. \
      The user can click anywhere on the image to have that point centered, or \
      they can use the shortcut or button to bring up a window that they can \
      enter a specific lat/lon position into. \
      <p><b>Shortcut: </b> Ctrl+F </p> \
      <p><b>Hint: </b> This option will only work if the image has a camera \
            model or is projected, and will only center the point on images  \
            where the selected lat/lon position exists.</p>";
    p_findPoint->setWhatsThis(text);
    p_findPoint->setEnabled(false); 
    connect( p_findPoint, SIGNAL( triggered() ),
             p_dialog, SLOT( show() ) ); 
  }

  FindTool::~FindTool() {
    delete p_groundTab->p_latLineEdit->validator();
    delete p_groundTab->p_lonLineEdit->validator();
  }

  
  /**
   * Creates the dialog used by this tool
   *
   * @param parent
   */
  void FindTool::createDialog(QWidget *parent) {
    p_dialog = new QDialog(parent);
    p_tabWidget = new QTabWidget(p_dialog);
    p_dialog->setWindowTitle("Find Latitude/Longitude Coordinate");

    p_groundTab = new GroundTab();
    p_imageTab = new ImageTab();
    p_tabWidget->addTab(p_imageTab, "Image");
    p_tabWidget->addTab(p_groundTab, "Ground");

    // Create the action buttons
    QPushButton *okButton = new QPushButton("Ok");
    connect( okButton, SIGNAL( clicked() ), 
             this, SLOT( handleOkClicked() ) );

    QPushButton *recordButton = new QPushButton("Record Point");
    connect( recordButton, SIGNAL( clicked() ), 
             this, SLOT( handleRecordClicked() ) );

    QPushButton *closeButton = new QPushButton("Close");
    connect( closeButton, SIGNAL( clicked() ), 
             p_dialog, SLOT( hide() ) );

    // Put the buttons in a horizontal orientation
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(okButton);
    actionLayout->addWidget(recordButton);
    actionLayout->addWidget(closeButton);

    QVBoxLayout *dialogLayout = new QVBoxLayout;
    dialogLayout->addWidget(p_tabWidget);
    dialogLayout->addLayout(actionLayout);
    p_dialog->setLayout(dialogLayout);
  }


  /**
   * The ground tab used by the dialog in the FindTool
   *
   * @param parent
   */
  GroundTab::GroundTab(QWidget *parent) : QWidget(parent) {
    p_latLineEdit = new QLineEdit();
    p_latLineEdit->setText("");
    p_latLineEdit->setValidator( new QDoubleValidator(-90.0, 90.0, 99, parent) );
    p_lonLineEdit = new QLineEdit();
    p_lonLineEdit->setText("");
    p_lonLineEdit->setValidator( new QDoubleValidator(parent) );
    QLabel *latLabel = new QLabel("Latitude");
    QLabel *lonLabel = new QLabel("Longitude");

    // Put the buttons and text field in a gridlayout
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(latLabel, 0, 0);
    gridLayout->addWidget(p_latLineEdit, 0, 1);
    gridLayout->addWidget(lonLabel, 1, 0);
    gridLayout->addWidget(p_lonLineEdit, 1, 1);
    setLayout(gridLayout);
  }

  
  /**
   * The image tab used by the dialog in the FindTool
   *
   * @param parent
   */
  ImageTab::ImageTab(QWidget *parent) : QWidget(parent) {
    p_sampLineEdit = new QLineEdit();
    p_sampLineEdit->setText("");
    p_lineLineEdit = new QLineEdit();
    p_lineLineEdit->setText("");
    QLabel *sampleLabel = new QLabel("Sample");
    QLabel *lineLabel = new QLabel("Line");

    // Put the buttons and text field in a gridlayout
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(sampleLabel, 0, 0);
    gridLayout->addWidget(p_sampLineEdit, 0, 1);
    gridLayout->addWidget(lineLabel, 1, 0);
    gridLayout->addWidget(p_lineLineEdit, 1, 1);
    setLayout(gridLayout);
  }

 
  /**
   * Adds the find tool to the toolpad
   *
   * @param toolpad
   *
   * @return QAction*
   */
  QAction *FindTool::toolPadAction(ToolPad *toolpad) {
    QAction *action = new QAction(toolpad);
    action->setIcon( QPixmap(toolIconDir() + "/find.png") );
    action->setToolTip("Find (F)");
    action->setShortcut(Qt::Key_F);
    QString text =
      "<b>Function:</b>  Find a lat/lon or line/sample coordinate in this cube. \
      <p><b>Shortcut:</b>F</p> ";
    action->setWhatsThis(text);
    return action;
  }

  
  /**
   * Adds the find tool to the menu
   *
   * @param menu
   */
  void FindTool::addTo(QMenu *menu) {
    menu->addAction(p_findPoint);
  }

  
  /**
   * Creates the tool bar for the find tool
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *FindTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget;

    p_showDialogButton = new QToolButton(hbox);
    p_showDialogButton->setIcon( QPixmap(toolIconDir() + "/find.png") );
    p_showDialogButton->setToolTip("Find Point");
    QString text =
      "<b>Function:</b> Centers all linked viewports to the selected lat/lon. \
      The user can click anywhere on the image to have that point centered, or \
      they can use the shortcut or button to bring up a window that they can \
      enter a specific lat/lon position into. \
      <p><b>Shortcut: </b> Ctrl+F </p> \
      <p><b>Hint: </b> This option will only work if the image has a camera \
            model or is projected, and will only center the point on images  \
            where the selected lat/lon position exists.</p>";
    p_showDialogButton->setWhatsThis(text);
    connect( p_showDialogButton, SIGNAL( clicked() ), 
             p_dialog, SLOT( show() ) );
    p_showDialogButton->setAutoRaise(true);
    p_showDialogButton->setIconSize( QSize(22, 22) );

    p_syncScale = new QCheckBox("Sync Scale");
    p_syncScale->setChecked(true);
    p_syncScale->setToolTip("Synchronize Scale");
    text = "<b>Function:</b> Syncronizes the scale of all linked viewports.";
    p_syncScale->setWhatsThis(text);

    p_linkViewportsButton = new QToolButton(hbox);
    p_linkViewportsButton->setIcon( QPixmap(toolIconDir() + "/link_valid.png") );
    p_linkViewportsButton->setToolTip("Link Georeferenced Images");
    p_linkViewportsButton->setWhatsThis("<b>Function: </b> Links all open images that have\
                              a camera model or are map projections");
    connect( p_linkViewportsButton, SIGNAL( clicked() ), 
             this, SLOT( handleLinkClicked() ) );
    p_linkViewportsButton->setAutoRaise(true);
    p_linkViewportsButton->setIconSize( QSize(22, 22) );

    p_togglePointVisibleButton = new QToolButton(hbox);
    p_togglePointVisibleButton->setIcon( QPixmap(toolIconDir() + "/redDot.png") );
    p_togglePointVisibleButton->setToolTip("Hide red dot");
    p_togglePointVisibleButton->setCheckable(true);
    p_togglePointVisibleButton->setChecked(true);
    connect( p_togglePointVisibleButton, SIGNAL( clicked() ), 
             this, SLOT( togglePointVisible() ) );

    p_statusEdit = new QLineEdit();
    p_statusEdit->setReadOnly(true);
    p_statusEdit->setToolTip("Cube Type");
    p_statusEdit->setWhatsThis("<b>Function: </b> Displays whether the active cube \
                           is a camera model, projection, both, or none. <p> \
                           <b>Hint: </b> If the cube is 'None' the find tool \
                           will not be active</p>");

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(p_statusEdit);
    layout->addWidget(p_showDialogButton);
    layout->addWidget(p_linkViewportsButton);
    layout->addWidget(p_togglePointVisibleButton);
    layout->addWidget(p_syncScale);
    layout->addStretch(1);
    hbox->setLayout(layout);

    return hbox;
  }
  
  
  /**
   * Overriden method to update this tool - Checks if cube is open and
   * checks if the image has camera and/or projection or none and sets
   * the flags accordingly. Also checks whether the images are linked.
   *
   * @history 2010-03-24 Sharmila Prasad - Enable FindTool for no camera image
   *                                       & display status as "None"
   *
   * @history 2010-05-06 Eric Hyer - This method now also updates the line edits
   *                                 within the dialog.
   */
  void FindTool::updateTool() {
    MdiCubeViewport *activeViewport = cubeViewport();

    if (activeViewport == NULL) {
      p_linkViewportsButton->setEnabled(false);
      p_findPoint->setEnabled(false);
      p_showDialogButton->setEnabled(false);
      p_syncScale->setEnabled(false);
      p_statusEdit->setText("None");
      if ( p_dialog->isVisible() ) {
        p_dialog->close();
      }
    }
    else {
      p_findPoint->setEnabled(true);
      p_showDialogButton->setEnabled(true);
      p_statusEdit->setText("None");

      if (cubeViewportList()->size() > 1) {
        p_linkViewportsButton->setEnabled(true);
        p_syncScale->setEnabled(true);
      }
      else {
        p_linkViewportsButton->setEnabled(false);
        p_syncScale->setEnabled(false);
      }

      if (activeViewport->camera() != NULL) {
        p_statusEdit->setText("Camera");
        if ( cubeViewport()->camera()->HasProjection() ) {
          p_statusEdit->setText("Both");
        }
      }
      else if (activeViewport->projection() != NULL) {
        p_statusEdit->setText("Projection");
      }

      // from here until the rest of this method we are just updating the
      // line edits within the dialog

      UniversalGroundMap *groundMap = activeViewport->universalGroundMap();

      if (p_samp != DBL_MAX && p_line != DBL_MAX) {
        if ( groundMap && groundMap->SetImage(p_samp, p_line) ) {
          QString latStr = QString::number( groundMap->UniversalLatitude() );
          QString lonStr = QString::number( groundMap->UniversalLongitude() );
          p_groundTab->p_latLineEdit->setText(latStr);
          p_groundTab->p_lonLineEdit->setText(lonStr);
        }
        else {
          p_groundTab->p_latLineEdit->setText("");
          p_groundTab->p_lonLineEdit->setText("");
        }

        p_imageTab->p_sampLineEdit->setText( QString::number(p_samp) );
        p_imageTab->p_lineLineEdit->setText( QString::number(p_line) );
      }
      else if (p_lat != DBL_MAX && p_lon != DBL_MAX) {
        // this should also work for rings (radius, azimuth)
        if ( groundMap && groundMap->SetUniversalGround(p_lat, p_lon) ) {
          QString lineStr = QString::number( groundMap->Line() );
          QString sampStr = QString::number( groundMap->Sample() );
          p_imageTab->p_lineLineEdit->setText(lineStr);
          p_imageTab->p_sampLineEdit->setText(sampStr);
        }
        else {
          p_imageTab->p_lineLineEdit->setText("");
          p_imageTab->p_sampLineEdit->setText("");
        }

        p_groundTab->p_latLineEdit->setText( QString::number(p_lat) );
        p_groundTab->p_lonLineEdit->setText( QString::number(p_lon) );
      }
    } // of if activeViewport != NULL
  }


  /**
   * Actions to take when the dialog's ok button is clicked.
   *
   *  - gets numerical info from the line edits in the dialog
   *  - centers and repaints the viewports (see refresh for the painting)
   *  - updateTool (see updateTool)
   */
  void FindTool::handleOkClicked() {
    p_samp = DBL_MAX;
    p_line = DBL_MAX;
    p_lat = DBL_MAX;
    p_lon = DBL_MAX;

    if (p_tabWidget->tabText( p_tabWidget->currentIndex() ) == "Ground") {
      p_lat = p_groundTab->p_latLineEdit->text().toDouble();
      p_lon = p_groundTab->p_lonLineEdit->text().toDouble();
      if (p_lat > 90 || p_lat < -90) {
        QString mess = QString::number(p_lat) + " is an invalid latitude value. "
                         + "Please enter a latitude between -90 and 90.";
        QMessageBox::warning((QWidget *)parent(), "Warning", mess);
        p_lat = DBL_MAX;
        p_lon = DBL_MAX;
      }
    }
    else if (p_tabWidget->tabText( p_tabWidget->currentIndex() ) == "Image") {
      p_line = p_imageTab->p_lineLineEdit->text().toDouble();
      p_samp = p_imageTab->p_sampLineEdit->text().toDouble();
    }

    centerLinkedViewports();
    refresh();
    updateTool();
  }

  
  /**
   * Slot called when the record button is clicked.  It creates a
   * QPoint from the current line/sample in the active cube
   * viewport and emits the recordPoint() signal.
   *
   * @return void
   * @author Jeannie Walldren
   *
   * @internal
   *  @history 2010-03-08 - Jeannie Walldren - This slot was
   *           created to connect the recordPoint() signal to the
   *           AdvancedTrackTool record() slot in qview.
   *  @history 2010-05-06 - Eric Hyer - Modified to work with new design of this
   *                                    class
   */
  void FindTool::handleRecordClicked() {
    double line = p_line;
    double samp = p_samp;

    if (p_lat != DBL_MAX && p_lon != DBL_MAX) {
      MdiCubeViewport *cvp = cubeViewport();
      UniversalGroundMap *groundMap = cvp->universalGroundMap();

      if (groundMap) {
        if ( groundMap->SetUniversalGround(p_lat, p_lon) ) {
          line = groundMap->Line();
          samp = groundMap->Sample();
        }
      }
    }

    if (line != DBL_MAX && samp != DBL_MAX) {
      int x, y;
      MdiCubeViewport *cvp = cubeViewport();
      cvp->cubeToViewport(samp, line, x, y);
      QPoint p(x, y);
      emit recordPoint(p);
    }
  }


  /**
   *  Handles mouse clickes in the CubeViewport.  Uses the point where click
   *  occurred to calculate line/samp or lat/lon (if there is a camera).
   *
   * @param p
   * @param s
   */
  void FindTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    MdiCubeViewport *activeViewport = cubeViewport();
    UniversalGroundMap *groundMap = activeViewport->universalGroundMap();

    double samp, line;
    activeViewport->viewportToCube(p.x(), p.y(), samp, line);

    p_samp = DBL_MAX;
    p_line = DBL_MAX;
    p_lat = DBL_MAX;
    p_lon = DBL_MAX;

    if (groundMap) {
      if ( groundMap->SetImage(samp, line) ) {
        if (activeViewport->camera() != NULL) {
          if (activeViewport->camera()->target()->isSky()) {
            p_lat = activeViewport->camera()->Declination();
            p_lon = activeViewport->camera()->RightAscension();
          }
          else {
            p_lat = groundMap->UniversalLatitude();
            p_lon = groundMap->UniversalLongitude();
          }
        }
        else {
          p_lat = groundMap->UniversalLatitude();
          p_lon = groundMap->UniversalLongitude();
        }
      }
    }
    else {
      p_samp = samp;
      p_line = line;
    }

    centerLinkedViewports();
    refresh();
    updateTool();
  }


  /**
   * This method paints the viewport
   *
   * @param vp
   * @param painter
   *
   * @history 2010-03-24 Sharmila Prasad - Enable FindTool for no camera image
   *                     only for Image Point (lines and Samples)
   * @history 2010-05-06 Eric Hyer - points now calculated here for every
   *                                 repaint.  This method is now used for all
   *                                 images, whether they have a cam or not
   */
  void FindTool::paintViewport(MdiCubeViewport *vp, QPainter *painter) {
    if ( (vp == cubeViewport() || ( cubeViewport()->isLinked() && vp->isLinked() ) )
        && p_pointVisible) {
      double samp = p_samp;
      double line = p_line;

      UniversalGroundMap *groundMap = vp->universalGroundMap();

      if (p_lat != DBL_MAX && p_lon != DBL_MAX && groundMap) {
        if ( groundMap->SetUniversalGround(p_lat, p_lon) ) {
          samp = groundMap->Sample();
          line = groundMap->Line();
        }
      }

      if (samp != DBL_MAX && line != DBL_MAX) {
        // first find the point
        int x, y;
        vp->cubeToViewport(samp, line, x, y);

        // now that we have the point draw it!
        QPen pen(Qt::red);
        pen.setWidth(3);
        pen.setStyle(Qt::SolidLine);
        painter->setPen(pen);
        painter->drawRoundedRect(x - 2, y - 2, 4, 4, 1, 1, Qt::RelativeSize);
      }
    }
  }

  
  //! toggles visibility of the red circle
  void FindTool::togglePointVisible() {
    // toggle the member boolean that specifies visibility
    p_pointVisible = !p_pointVisible;

    // update the buttons text
    if (p_pointVisible) {
      p_togglePointVisibleButton->setChecked(true);
      p_togglePointVisibleButton->setToolTip("Hide red dot");
    }
    else {
      p_togglePointVisibleButton->setChecked(false);
      p_togglePointVisibleButton->setToolTip("Show red dot");
    }

    refresh();
  }


  //! Links all cubes that have camera models or are map projections
  void FindTool::handleLinkClicked() {
    MdiCubeViewport *d;
    for (int i = 0; i < (int)cubeViewportList()->size(); i++) {
      d = ( *( cubeViewportList() ) )[i];
      if (d->universalGroundMap() != NULL) {
        d->setLinked(true);
      }
      else {
        d->setLinked(false);
      }
    }
  }


  //! centers all linked viewports
  void FindTool::centerLinkedViewports() {
    MdiCubeViewport *activeViewport = cubeViewport();
    bool syncScale = p_syncScale->isChecked();
    // This will be the ground map resolution of the active viewport in meters
    //   per pixel.
    Distance viewportResolutionToMatch;
    UniversalGroundMap *groundMap = activeViewport->universalGroundMap();

    // Equation to match viewports:
    //   otherViewportZoomFactor = activeViewportZoomFactor *
    //       (otherViewportResolution / activeViewportResolution)
    if (syncScale) {
      viewportResolutionToMatch = distancePerPixel(activeViewport, p_lat, p_lon);
    }

    for (int i = 0; i < cubeViewportList()->size(); i++) {
      MdiCubeViewport *viewport = ( *( cubeViewportList() ) )[i];

      if ( viewport == activeViewport ||
           ( activeViewport->isLinked() && viewport->isLinked() ) ) {
        groundMap = viewport->universalGroundMap();
        double otherViewportZoomFactor = viewport->scale();

        if ( groundMap && !IsSpecial(p_lat) && p_lat != DBL_MAX &&
             !IsSpecial(p_lon) && p_lon != DBL_MAX &&
             groundMap->SetUniversalGround(p_lat, p_lon) ) {
          double samp = groundMap->Sample();
          double line = groundMap->Line();

          if ( viewportResolutionToMatch.isValid() ) {
            Distance otherViewportResolution = distancePerPixel(viewport,
                                                                p_lat, p_lon);
            otherViewportZoomFactor = activeViewport->scale() *
                (otherViewportResolution / viewportResolutionToMatch);
          }

          if (p_lat != DBL_MAX && p_lon != DBL_MAX && groundMap) {
            viewport->setScale(otherViewportZoomFactor, samp, line);
          }
        }

        if (p_line != DBL_MAX && p_samp != DBL_MAX) {
          viewport->setScale(otherViewportZoomFactor, p_samp, p_line);
        }
      }
    }
  }


  /**
   * This computes the distance covered by a pixel at the given lat/lon in the
   *   given viewport. This computation is used for synchronizing the scales of
   *   the viewports so that features appear approximately the right size. The
   *   resolution is the universal ground map's resolution unless we're dealing
   *   with a projection. If the cube in the viewport is projected then we try
   *   to calculate a resolution centered at the clicked on pixel... if that
   *   fails then we give the projection's resolution.
   *
   * @param viewport The viewport for which we want the resolution
   * @param lat The latitude to calculate the resolution at
   * @param lon The longitude to calculate the resolution at
   *
   * @return The resolution, in distance/pixel, of the data in the viewport. The
   *         distance will be in a unit other than pixels.
   */
  Distance FindTool::distancePerPixel(MdiCubeViewport *viewport,
                                      double lat, double lon) {
    // UniversalGroundMaps default to camera priority, so create a new one so that we can use projection if it exists.
    std::unique_ptr<UniversalGroundMap> groundMap(new UniversalGroundMap(*(viewport->cube()), UniversalGroundMap::ProjectionFirst));
    Distance viewportResolution;
    if (groundMap->Camera() != NULL){
      if (groundMap->Camera()->target()->isSky()) {
        return Distance(groundMap->Camera()->RaDecResolution(), Distance::Units::Meters);
      }
    }

    try {
      if ( groundMap && !IsSpecial(lat) && !IsSpecial(lon) &&
           lat != DBL_MAX && lon != DBL_MAX &&
           groundMap->SetUniversalGround(lat, lon) ) {
        // Distance/pixel
        viewportResolution = Distance(groundMap->Resolution(), Distance::Meters);
        double samp = groundMap->Sample();
        double line = groundMap->Line();

        if ( groundMap->SetImage(samp - 0.5, line - 0.5) ) {
          double lat1 = groundMap->UniversalLatitude();
          double lon1 = groundMap->UniversalLongitude();

          if ( groundMap->SetImage(samp + 0.5, line + 0.5) ) {
            double lat2 = groundMap->UniversalLatitude();
            double lon2 = groundMap->UniversalLongitude();

            double radius = groundMap->HasProjection()?
                groundMap->Projection()->LocalRadius() :
                groundMap->Camera()->LocalRadius().meters();

            SurfacePoint point1( Latitude(lat1, Angle::Degrees),
                                 Longitude(lon1, Angle::Degrees),
                                 Distance(radius, Distance::Meters) );

            SurfacePoint point2( Latitude(lat2, Angle::Degrees),
                                 Longitude(lon2, Angle::Degrees),
                                 Distance(radius, Distance::Meters) );

            viewportResolution = point1.GetDistanceToPoint(point2);
          }
        }
      }
    }
    catch (IException &e) {
      p_samp = DBL_MAX;
      p_line = DBL_MAX;
      p_lat = DBL_MAX;
      p_lon = DBL_MAX;
      QMessageBox::warning((QWidget *)parent(), "Warning", QString::fromStdString(e.toString()));
    }

    return viewportResolution;
  }


  //! does a repaint for active viewport and also for any linked viewports
  void FindTool::refresh() {
    MdiCubeViewport *activeVp = cubeViewport();
    for (int i = 0; i < cubeViewportList()->size(); i++) {
      MdiCubeViewport *vp = ( *( cubeViewportList() ) )[i];

      if ( vp == activeVp || (activeVp->isLinked() && vp->isLinked() ) )
        vp->viewport()->repaint();
    }
  }
}
