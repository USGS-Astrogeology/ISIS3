#include "MosaicAreaTool.h"

#include <cmath>
#include <float.h>

#include <QDialog>
#include <QDoubleValidator>
#include <QGraphicsEllipseItem>
#include <QGraphicsScene>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>

#include "Angle.h"
#include "Distance.h"
#include "IString.h"
#include "FindSpotGraphicsItem.h"
#include "MosaicGraphicsView.h"
#include "MosaicSceneWidget.h"
#include "Projection.h"
#include "TProjection.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

namespace Isis {
  /**
   * MosaicAreaTool constructor
   *
   *
   * @param parent
   */
  MosaicAreaTool::MosaicAreaTool(MosaicSceneWidget *scene) :
      MosaicTool(scene) {
    m_box = NULL;
    m_drawBox = NULL;
    m_latLineEdit = NULL;
    m_lonLineEdit = NULL;
    m_areaLineEdit = NULL;

    connect(scene, SIGNAL(projectionChanged(Projection *)),
            this, SLOT(userChangedBox()));
  }


  /**
   *
   *
   */
  void MosaicAreaTool::userChangedBox() {
    bool latValid = false;
    bool lonValid = false;
    bool areaValid = false;

    if(!m_latLineEdit || !m_lonLineEdit || !m_areaLineEdit) {
      clearBox();
      return;
    }

    QString latitude = m_latLineEdit->text();

    if(latitude != "Null" && latitude != "") {
      int cursorPos = 0;
      QValidator::State validLat =
        m_latLineEdit->validator()->validate(latitude, cursorPos);
      if(validLat != QValidator::Acceptable) {
        QMessageBox::warning(getWidget(), "Error",
                            "Latitude value must be in the range -90 to 90",
                            QMessageBox::Ok, QMessageBox::NoButton,
                            QMessageBox::NoButton);
      }
      else {
        latValid = true;
      }
    }

    //Validate longitude value
    QString longitude = m_lonLineEdit->text();
    if(longitude != "Null" && longitude != "" && latValid) {
      int cursorPos = 0;
      QValidator::State validLon =
        m_lonLineEdit->validator()->validate(longitude, cursorPos);
      if(validLon != QValidator::Acceptable) {
        QMessageBox::warning(getWidget(), "Error",
                            "Longitude value invalid",
                            QMessageBox::Ok, QMessageBox::NoButton,
                            QMessageBox::NoButton);
      }
      else {
        lonValid = true;
      }
    }

    QString areaString = m_areaLineEdit->text();
    if(areaString != "Null" && areaString != "" && latValid && lonValid) {
      int cursorPos = 0;
      QValidator::State validArea =
        m_areaLineEdit->validator()->validate(areaString, cursorPos);
      if(validArea != QValidator::Acceptable) {
        QMessageBox::warning(getWidget(), "Error",
                            "Area value invalid",
                            QMessageBox::Ok, QMessageBox::NoButton,
                            QMessageBox::NoButton);
      }
      else {
        areaValid = true;
      }
    }


    if(latValid && lonValid && areaValid) {
      double lat = IString(latitude.toStdString()).ToDouble();
      double lon = IString(longitude.toStdString()).ToDouble();
      double area = IString(areaString.toStdString()).ToDouble();

      Projection *projection = getWidget()->getProjection();
      Projection::ProjectionType ptype = projection->projectionType();

      if (projection && ptype == Projection::Triaxial) {
        TProjection * tproj = (TProjection *) projection;
        if (tproj->SetGround(lat, lon)) {
          QPointF scenePos(projection->XCoord(), -1 * projection->YCoord());
          QRectF sceneRect(getWidget()->getView()->sceneRect());

          if(sceneRect.contains(scenePos)) {
            if(m_box != NULL) {
              clearBox();
            }

            Distance distance(area, Distance::Meters);

            QPolygonF boxPoly;
            QRectF latLonRange = calcLatLonRange(QPointF(lon, lat), distance);

            double xStep = latLonRange.width() / 100.0;
            double yStep = latLonRange.height() / 100.0;

            bool hasPole = (latLonRange.top() == -90 ||
                            latLonRange.bottom() == 90);

            double yPos = latLonRange.top();
            if (yPos != -90) {
              for(double xPos = latLonRange.left();
                xPos <= latLonRange.right();
                xPos += xStep) {
                if (tproj->SetGround(yPos, xPos)) {
                  QPointF pos(tproj->XCoord(), -1 * tproj->YCoord());
                  boxPoly << pos;
                }
              }
            }

            double xPos = latLonRange.right();
            for (double yPos = latLonRange.top();
                !hasPole && yPos <= latLonRange.bottom();
                yPos += yStep) {
              if (tproj->SetGround(yPos, xPos)) {
                QPointF pos(tproj->XCoord(), -1 * tproj->YCoord());
                boxPoly << pos;
              }
            }

            yPos = latLonRange.bottom();
            if (yPos != 90) {
              for (double xPos = latLonRange.right();
                xPos >= latLonRange.left();
                xPos -= xStep) {
                if (tproj->SetGround(yPos, xPos)) {
                  QPointF pos(tproj->XCoord(), -1 * tproj->YCoord());
                  boxPoly << pos;
                }
              }
            }

            xPos = latLonRange.left();
            for (double yPos = latLonRange.bottom();
              !hasPole && yPos >= latLonRange.top();
              yPos -= yStep) {
              if (tproj->SetGround(yPos, xPos)) {
                QPointF pos(tproj->XCoord(), -1 * tproj->YCoord());
                boxPoly << pos;
              }
            }

            if (boxPoly.size() > 0) {
              boxPoly << boxPoly[0];

              m_box = new QGraphicsPolygonItem(boxPoly);
              m_box->setZValue(DBL_MAX);
              // Ensure lines are cosmetic (i.e. always 1 pixel on screen)
              QPen pen;
              pen.setCosmetic(true);
              m_box->setPen(pen);

              getWidget()->getScene()->addItem(m_box);
              getWidget()->getView()->centerOn(scenePos);
            }
          }
          else {
          std::string message = "Lat/Lon not within this view.";
          QMessageBox::information(getWidget(), "Cannot Calculate Box",
                                  message, QMessageBox::Ok);
          }
        }
      }
    }
  }


  /**
   * Adds the action to the toolpad.
   *
   *
   * @param toolpad
   *
   * @return QAction*
   */
  QAction *MosaicAreaTool::getPrimaryAction() {
    m_action = new QAction(this);
    m_action->setIcon(getIcon("qmos_area.png"));
    m_action->setToolTip("Show Area (a)");
    m_action->setShortcut(Qt::Key_A);
    QString text  =
      "<b>Function:</b>  Draw a box given a distance centered on a "
      "latitude/longitude.<br><br>"
      "This tool draws a black square, given an edge length in meters, "
      "centered on a latitude/longitude point. This box would be a square on "
      "the surface of the target, and is designed to be modified and warped by "
      "the current projection."
      "<p><b>Shortcut:</b>  a</p> ";
    m_action->setWhatsThis(text);
    return m_action;
  }


  QWidget *MosaicAreaTool::getToolBarWidget() {
    m_latLineEdit = new QLineEdit();
    m_latLineEdit->setValidator(new QDoubleValidator(-90.0, 90.0, 99, this));

    m_lonLineEdit = new QLineEdit();
    m_lonLineEdit->setValidator(new QDoubleValidator(this));

    m_areaLineEdit = new QLineEdit();
    m_areaLineEdit->setValidator(new QDoubleValidator(this));
    m_areaLineEdit->setText("10000");

    QLabel *latLabel = new QLabel("Latitude");
    QLabel *lonLabel = new QLabel("Longitude");
    QLabel *areaLabel = new QLabel("Size (meters)");
    areaLabel->setToolTip("This is the width and the height of the box");

    // Create the action buttons
    QPushButton *okButton = new QPushButton("Update Box");
    connect(okButton, SIGNAL(clicked()), this, SLOT(userChangedBox()));

    QPushButton *clearButton = new QPushButton("Clear Box");
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clearBox()));

    // Put the buttons in a horizontal orientation
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(latLabel);
    actionLayout->addWidget(m_latLineEdit);
    actionLayout->addWidget(lonLabel);
    actionLayout->addWidget(m_lonLineEdit);
    actionLayout->addWidget(areaLabel);
    actionLayout->addWidget(m_areaLineEdit);
    actionLayout->addWidget(okButton);
    actionLayout->addWidget(clearButton);
    actionLayout->addStretch(1);
    actionLayout->setMargin(0);

    QWidget *toolBarWidget = new QWidget;
    toolBarWidget->setLayout(actionLayout);

    return toolBarWidget;
  }


  /**
   * Adds the pan action to the given menu.
   *
   *
   * @param menu
   */
  void MosaicAreaTool::addToMenu(QMenu *menu) {

  }


  PvlObject MosaicAreaTool::toPvl() const {
    PvlObject obj(projectPvlObjectName().toStdString());

    if(m_box) {
      obj += PvlKeyword("Latitude", m_latLineEdit->text().toStdString());
      obj += PvlKeyword("Longitude", m_lonLineEdit->text().toStdString());
      obj += PvlKeyword("Area", m_areaLineEdit->text().toStdString());
      obj += PvlKeyword("Visible", std::to_string((int)(m_box != NULL)));
    }

    return obj;
  }


  void MosaicAreaTool::fromPvl(const PvlObject &obj) {
    if(obj.hasKeyword("Visible")) {
      if(obj.hasKeyword("Latitude") && obj["Latitude"][0] != "Null")
        m_latLineEdit->setText(QString::fromStdString(obj["Latitude"][0]));

      if(obj.hasKeyword("Longitude") && obj["Longitude"][0] != "Null")
        m_lonLineEdit->setText(QString::fromStdString(obj["Longitude"][0]));

      if(obj.hasKeyword("Area") && obj["Area"][0] != "Null")
        m_areaLineEdit->setText(QString::fromStdString(obj["Area"][0]));

      if(toBool(QString::fromStdString(obj["Visible"][0])) != false) {
        userChangedBox();
      }
    }
  }


  QString MosaicAreaTool::projectPvlObjectName() const {
    return "MosaicAreaTool";
  }


  /**
   * Creates the widget to add to the tool bar.
   *
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *MosaicAreaTool::createToolBarWidget() {
    QWidget *widget = new QWidget();
    return widget;
  }


  void MosaicAreaTool::mouseButtonRelease(QPointF mouseLoc, Qt::MouseButton s) {
    if(!isActive())
      return;

    if(s == Qt::LeftButton) {
      TProjection *tproj = (TProjection *) getWidget()->getProjection();

      if(tproj && getWidget()->getView()->sceneRect().contains(mouseLoc)) {
        if(tproj->SetCoordinate(mouseLoc.x(), -1 * mouseLoc.y())) {
          if(m_drawBox != NULL) {
            clearBox();
          }

          m_latLineEdit->setText(QString::number(tproj->Latitude(), 'g', 10));
          m_lonLineEdit->setText(QString::number(tproj->Longitude(), 'g', 10));

          userChangedBox();
        }
      }
    }
  }


  /**
   *
   *
   */
  void MosaicAreaTool::clearBox() {
    if(m_box != NULL) {
      getWidget()->getScene()->removeItem(m_box);

      delete m_box;
      m_box = NULL;
    }
  }


  /**
   * Given a distance and a center lat,lon this will return the bounding lat,lon
   *   rect.
   *
   * The distance is the distance across the entire rectangle (i.e. width and
   *   height).
   */
  QRectF MosaicAreaTool::calcLatLonRange(QPointF centerLatLon,
      Distance size) {
    Distance distanceFromCenter = size / 2.0;
    QRectF latLonBoundingBox;

    Angle centerLat(centerLatLon.y(), Angle::Degrees);
    Angle centerLon(centerLatLon.x(), Angle::Degrees);

    TProjection *tproj = (TProjection *) getWidget()->getProjection();

    if (tproj) {
      bool longitudeWraps = false;
      Distance radius(tproj->LocalRadius(centerLat.degrees()),
          Distance::Meters);

      // First we can get the angle between the latitudes...
      // d = arcsin ( movementDistance / radiusDistance )
      Angle deltaLat(asin( distanceFromCenter / radius ), Angle::Radians);

      latLonBoundingBox.setTop( (centerLat - deltaLat).degrees() );

      if (latLonBoundingBox.top() < -90 && centerLatLon.y() != -90) {

        // Block infinite recursion
        if (centerLatLon.y() != 90) {
          qWarning("The pole is included in the area but not centered");
          centerLatLon.setY(-90);
          return calcLatLonRange(centerLatLon, size);
        }
        else
          return QRectF();
      }
      else if (centerLatLon.y() == -90) {
        longitudeWraps = true;
      }

      latLonBoundingBox.setBottom( (centerLat + deltaLat).degrees() );

      if (latLonBoundingBox.bottom() > 90 && centerLatLon.y() != 90) {

        // Block infinite recursion
        if (centerLatLon.y() != -90) {
          qWarning("The pole is included in the area but not centered");
          centerLatLon.setY(90);
          return calcLatLonRange(centerLatLon, size);
        }
        else
          return QRectF();
      }
      else if (centerLatLon.y() == 90) {
        longitudeWraps = true;
      }

      // Now let's do lons...
      Angle widestLat(
          asin( sin(centerLat.radians()) /
                cos( distanceFromCenter / radius ) ),
          Angle::Radians);

      double valueToASin = sin(distanceFromCenter / radius) /
          cos(widestLat.radians());

      if(valueToASin < -1 || valueToASin > 1)
        longitudeWraps = true;

      // Longitude wraps
      if (longitudeWraps) {
        if (tproj->Has360Domain()) {
          latLonBoundingBox.setLeft( 0 );
          latLonBoundingBox.setRight( 360 );
        }
        else {
          latLonBoundingBox.setLeft( -180 );
          latLonBoundingBox.setRight( 180 );
        }
      }
      else {
        Angle deltaLon(
            asin( sin(distanceFromCenter / radius) /
                  cos(widestLat.radians())),
            Angle::Radians);
        latLonBoundingBox.setLeft( (centerLon - deltaLon).degrees() );
        latLonBoundingBox.setRight( (centerLon + deltaLon).degrees() );
      }
    }

    return latLonBoundingBox;
  }
}

