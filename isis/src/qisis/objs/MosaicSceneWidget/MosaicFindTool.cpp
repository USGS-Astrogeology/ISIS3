#include "MosaicFindTool.h"

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
   * MosaicFindTool constructor
   *
   *
   * @param parent
   */
  MosaicFindTool::MosaicFindTool(MosaicSceneWidget *scene) :
      MosaicTool(scene) {
    p_findSpot = NULL;
  }


  /**
   *
   *
   */
  void MosaicFindTool::getUserGroundPoint() {

    //Validate latitude value
    QString latitude = p_latLineEdit->text();
    int cursorPos = 0;
    QValidator::State validLat =
      p_latLineEdit->validator()->validate(latitude, cursorPos);
    if(validLat != QValidator::Acceptable) {
      QMessageBox::warning(getWidget(), "Error",
                           "Latitude value must be in the range -90 to 90",
                           QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
      return;
    }

    //Validate longitude value
    QString longitude = p_lonLineEdit->text();
    QValidator::State validLon =
      p_lonLineEdit->validator()->validate(longitude, cursorPos);
    if(validLon != QValidator::Acceptable) {
      QMessageBox::warning(getWidget(), "Error",
                           "Longitude value must be a double",
                           QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
      return;
    }

    double lat = IString(latitude.toStdString()).ToDouble();
    double lon = IString(longitude.toStdString()).ToDouble();

    Projection *projection = getWidget()->getProjection();
    Projection::ProjectionType ptype = projection->projectionType();

    if (projection && ptype == Projection::Triaxial) {
      TProjection *tproj = (TProjection *) projection;
      if (tproj->SetGround(lat, lon)) {
        QPointF scenePos(projection->XCoord(), -1 * projection->YCoord());
        QRectF sceneRect(getWidget()->getView()->sceneRect());

        if (sceneRect.contains(scenePos)) {
          if(p_findSpot != NULL) {
            clearPoint();
          }

          p_findSpot = new FindSpotGraphicsItem(scenePos, getWidget());

          getWidget()->getScene()->addItem(p_findSpot);
          getWidget()->getView()->centerOn(scenePos);
        }
        else {
          QString message = "Lat/Lon not within this view.";
          QMessageBox::information(getWidget(), "Point Not Found",
                                   message, QMessageBox::Ok);
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
  QAction *MosaicFindTool::getPrimaryAction() {
    p_action = new QAction(this);
    p_action->setIcon(getIcon("find.png"));
    p_action->setToolTip("Find (f)");
    p_action->setShortcut(Qt::Key_F);
    QString text  =
      "<b>Function:</b>  Find a specific latitude/longitude on the mosaic "
      "scene.<br><br>"
      "This tool allows you to type in a latitude and longitude, in the "
      "projection's native units, and that point will be centered and given "
      "a red dot on the mosaic scene. Alternatively, you can <b>click</b> on "
      "the mosaic scene and it will give you the latitude and longitude values "
      "along with drawing the red dot."
      "<p><b>Shortcut:</b>  f</p> ";
    p_action->setWhatsThis(text);
    return p_action;
  }


  QWidget *MosaicFindTool::getToolBarWidget() {
    p_latLineEdit = new QLineEdit();
    p_latLineEdit->setValidator(new QDoubleValidator(-90.0, 90.0, 99, this));

    p_lonLineEdit = new QLineEdit();
    p_lonLineEdit->setValidator(new QDoubleValidator(this));

    QLabel *latLabel = new QLabel("Latitude");
    QLabel *lonLabel = new QLabel("Longitude");

    // Create the action buttons
    QPushButton *okButton = new QPushButton("Go to Point");
    connect(okButton, SIGNAL(clicked()), this, SLOT(getUserGroundPoint()));

    QPushButton *clearButton = new QPushButton("Clear Point");
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clearPoint()));

    // Put the buttons in a horizontal orientation
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(latLabel);
    actionLayout->addWidget(p_latLineEdit);
    actionLayout->addWidget(lonLabel);
    actionLayout->addWidget(p_lonLineEdit);
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
  void MosaicFindTool::addToMenu(QMenu *menu) {

  }


  PvlObject MosaicFindTool::toPvl() const {
    PvlObject obj(projectPvlObjectName().toStdString());

    obj += PvlKeyword("Latitude", p_latLineEdit->text().toStdString());
    obj += PvlKeyword("Longitude", p_lonLineEdit->text().toStdString());
    obj += PvlKeyword("Visible", Isis::toString((int)(p_findSpot != NULL)));

    return obj;
  }


  void MosaicFindTool::fromPvl(const PvlObject &obj) {
    p_latLineEdit->setText(QString::fromStdString(obj["Latitude"][0]));
    p_lonLineEdit->setText(QString::fromStdString(obj["Longitude"][0]));
    if(toBool(QString::fromStdString(obj["Visible"][0]).toStdString())) {
      getUserGroundPoint();
    }
  }


  QString MosaicFindTool::projectPvlObjectName() const {
    return "MosaicFindTool";
  }


  /**
   * Creates the widget to add to the tool bar.
   *
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *MosaicFindTool::createToolBarWidget() {
    QWidget *widget = new QWidget();
    return widget;
  }


  void MosaicFindTool::mouseButtonRelease(QPointF mouseLoc, Qt::MouseButton s) {
    if(!isActive())
      return;

    if(s == Qt::LeftButton) {
      Projection *proj = getWidget()->getProjection();
      Projection::ProjectionType ptype = proj->projectionType();

      if (ptype == Projection::Triaxial) {
        TProjection *tproj = (TProjection *) proj;
        if (tproj && getWidget()->getView()->sceneRect().contains(mouseLoc)) {
          if (tproj->SetCoordinate(mouseLoc.x(), -1 * mouseLoc.y())) {
            if (p_findSpot != NULL) {
              clearPoint();
            }

            p_findSpot = new FindSpotGraphicsItem(mouseLoc, getWidget());

            getWidget()->getScene()->addItem(p_findSpot);

            p_latLineEdit->setText(QString::number(tproj->Latitude(), 'g', 10));
            p_lonLineEdit->setText(QString::number(tproj->Longitude(), 'g', 10));
          }
        }
      }
    }
  }


  /**
   *
   *
   */
  void MosaicFindTool::clearPoint() {
    if(p_findSpot != NULL) {
      getWidget()->getScene()->removeItem(p_findSpot);

      delete p_findSpot;
      p_findSpot = NULL;
    }
  }


  /**
   * This method sets the QGraphicsView to allow the user to select
   * mosaic items by dragging a rubber band.
   *
   */
  void MosaicFindTool::updateTool() {
    if(isActive()) {
    }
  }

}

