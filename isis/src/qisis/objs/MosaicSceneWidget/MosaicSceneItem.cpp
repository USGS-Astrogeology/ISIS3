#include "MosaicSceneItem.h"

#include <iostream>
#include <cfloat>

#include <QApplication>
#include <QBrush>
#include <QEvent>
#include <QGraphicsItem>
#include <QGraphicsSceneContextMenuEvent>
#include <QList>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QStyleOptionGraphicsItem>
#include <QTreeWidgetItem>

#include "Directory.h"
#include "DisplayProperties.h"
#include "FileDialog.h"
#include "Histogram.h"
#include "Image.h"
#include "ImageList.h"
#include "ImagePolygon.h"
#include "IString.h"
#include "LineManager.h"
#include "MosaicGraphicsView.h"
#include "MosaicSceneWidget.h"
#include "PolygonTools.h"
#include "Project.h"
#include "SerialNumber.h"
#include "Statistics.h"
#include "Stretch.h"
#include "Table.h"
#include "TProjection.h"

using namespace geos::geom;

namespace Isis {
  /**
   * MosaicSceneItem constructor
   *
   *
   * @param cubeFileName
   * @param parent
   */
  MosaicSceneItem::MosaicSceneItem(Image *image, MosaicSceneWidget *parent) : QGraphicsObject() {
    if (parent->getProjection() == NULL) {
      std::string msg = "Parent does not have projection in MosaicWidget";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    m_image = image;

    connect(m_image, SIGNAL(destroyed(QObject *)),
            this, SLOT(lostCubeDisplay()));
    connect(m_image, SIGNAL(destroyed(QObject *)),
            this, SLOT(deleteLater()));

    m_mp = NULL;
    m_polygons = NULL;
    m_cubeDnStretch = NULL;
    groundMap = NULL;
    m_showingLabel = false;
    m_ignoreCubeDisplayChanged = false;

    m_scene = parent;

    m_polygons = new QList< QGraphicsPolygonItem *>();

    setupFootprint();

    setToolTip(m_image->displayProperties()->displayName());

    setAcceptHoverEvents(true);

    ImageDisplayProperties *displayProp = m_image->displayProperties();
    ImageDisplayProperties::Property supportToAdd = (ImageDisplayProperties::Property)
        (ImageDisplayProperties::Color       |
         ImageDisplayProperties::Selected    |
         ImageDisplayProperties::ShowDNs     |
         ImageDisplayProperties::ShowFill    |
         ImageDisplayProperties::ShowLabel   |
         ImageDisplayProperties::ShowOutline |
         ImageDisplayProperties::ZOrdering);

    if(parent->userHasTools()) {
      supportToAdd = (ImageDisplayProperties::Property)
          (supportToAdd | ImageDisplayProperties::Zooming);
    }

    displayProp->addSupport(supportToAdd);

    connect(displayProp, SIGNAL(propertyChanged(DisplayProperties *)),
            this, SLOT(cubeDisplayChanged()));
  }


  /**
   * Mosaic Item destructor
   *
   */
  MosaicSceneItem::~MosaicSceneItem() {
    if(scene())
      scene()->removeItem(this);

    while(m_polygons->size()) {
      delete m_polygons->takeAt(0);
    }
  }


  QRectF MosaicSceneItem::boundingRect() const {
    QRectF boundingRect;

    QGraphicsPolygonItem *polygon;
    foreach(polygon, *m_polygons) {
      boundingRect = boundingRect.united(polygon->boundingRect());

      QGraphicsItem *polyChild;
      foreach(polyChild, polygon->childItems()) {
        if(polyChild->isVisible()) {
          boundingRect = boundingRect.united(
            mapFromItem(polyChild, polyChild->boundingRect()).boundingRect());
        }
      }
    }

    return boundingRect;
  }


  /**
   * Re-paints the item
   *
   * @param painter
   * @param option
   * @param widget
   */
  void MosaicSceneItem::paint(QPainter *painter,
      const QStyleOptionGraphicsItem *option, QWidget *widget) {
    if(m_image &&
       m_image->displayProperties()->getValue(ImageDisplayProperties::ShowDNs).toBool()) {
      drawImage(painter, option);
    }

    // We don't add the polygon items as children because manually painting them is a huge speed
    //   improvement. It cannot be undone due to the amount of speed it gives.
    if (!childItems().count()) {
      foreach (QGraphicsPolygonItem *polyItem, *m_polygons) {
        polyItem->paint(painter, option, widget);
      }
    }
  }


  /**
   *
   */
  void MosaicSceneItem::setupFootprint() {
    if(m_image) {
      m_mp = m_image->footprint();

      if (!m_mp) {
        throw IException(IException::Unknown,
            "Cannot display footprints of images which have no footprints. "
            "Tried to display [" + m_image->displayProperties()->displayName().toStdString() + "]",
            _FILEINFO_);
      }

      try {
        reproject();
      }
      catch(IException &e) {
        m_image->deleteLater();

        IString msg = "Could not project the footprint from cube [" +
            m_image->displayProperties()->displayName().toStdString() + "]";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }
  }


  /**
   * Called anytime the user reprojects the cube. (Selects a new
   * map file.) And everytime a mosaic item is created.
   */
  void MosaicSceneItem::reproject() {
    prepareGeometryChange();

    MultiPolygon *mp;
    TProjection *proj = (TProjection *)m_scene->getProjection();

    // Remove current polygons from the scene
    while(m_polygons->size()) {
      QGraphicsPolygonItem *polyItem = m_polygons->at(0);

      if (polyItem->scene()) {
        polyItem->scene()->removeItem(polyItem);
      }
      m_polygons->removeAll(polyItem);

      delete polyItem;
      polyItem = NULL;
    }

    if (proj->Has180Domain()) {
      m_180mp = PolygonTools::To180(m_mp);
      mp = m_180mp;
    }
    else {
      mp = m_mp;
    }

    m_showingLabel =
        m_image->displayProperties()->getValue(ImageDisplayProperties::ShowLabel).toBool();

    //----------------------------------------------------------
    // We need to loop thru the num. geom. because some of the
    // cubes will have more than one geom. if it crosses lat/lon
    // boundries.
    //----------------------------------------------------------
    bool useFullChildrenHierarchy = (mp->getNumGeometries() > 1) || m_showingLabel;

    for (unsigned int i = 0; i < mp->getNumGeometries(); i++) {
      const Geometry *geom = mp->getGeometryN(i);
      CoordinateSequence *pts;

      pts = geom->getCoordinates().release();
      double lat, lon;
      QVector<QPointF> polyPoints;

      //--------------------------------------------------------------
      // We need to convert the footprint polygons from lat/lon to x/y
      // in order to display them in the QGraphicsScene
      //--------------------------------------------------------------
      for (unsigned int j = 0; j < pts->getSize(); j++) {
        lat = pts->getY(j);
        lon = pts->getX(j);
        if (proj->SetGround(lat, lon)) {
          double x = proj->XCoord();
          double y = -1 * (proj->YCoord());

          polyPoints.push_back(QPointF(x, y));
        }
      }

      setFlag(QGraphicsItem::ItemIsSelectable, true);
      setFlag(QGraphicsItem::ItemIsFocusable, true);

      QGraphicsPolygonItem *polyItem = NULL;

      if (useFullChildrenHierarchy) {
        polyItem = new QGraphicsPolygonItem(this);
      }
      else {
        polyItem = new QGraphicsPolygonItem;
      }

      polyItem->setPolygon(QPolygonF(polyPoints));

      if (m_showingLabel) {
        QGraphicsSimpleTextItem *label = NULL;

        label = new QGraphicsSimpleTextItem(polyItem);

        if(m_image)
          label->setText(m_image->displayProperties()->displayName());
        label->setFlag(QGraphicsItem::ItemIsMovable);
        label->setFont(QFont("Helvetica", 10));
        label->setPos(polyItem->polygon().boundingRect().center());
        label->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        QRectF boundingRect = polyItem->boundingRect();
        if(boundingRect.width() < boundingRect.height())
          label->setRotation(90.0);
      }

      m_polygons->append(polyItem);

      delete pts;
    }

    updateChildren();
  }


  /**
   * Returns the pixel value at the given sample/line.
   *
   *
   * @param sample
   * @param line
   *
   * @return int
   */
  double MosaicSceneItem::getPixelValue(int sample, int line) {
    double pixelValue = 0;

    if(m_image) {
      Brick gryBrick(1, 1, 1, m_image->cube()->pixelType());
      gryBrick.SetBasePosition((int)(sample + 0.5), (int)(line + 0.5), 1);
      m_image->cube()->read(gryBrick);

      pixelValue = gryBrick[0];
      if (pixelValue == Null) {
        return Null;
      }
      if (pixelValue < 0) pixelValue = 0;
      if (pixelValue > 255) pixelValue = 255;
    }

    return pixelValue;
  }


  /**
   * This method reads in and draws the image associated with this
   * item.
   * @param painter
   */
  void MosaicSceneItem::drawImage(QPainter *painter,
      const QStyleOptionGraphicsItem *option) {
    Stretch *stretch = getStretch();
    QApplication::setOverrideCursor(Qt::WaitCursor);

    try {
      QGraphicsPolygonItem *polygon;
      foreach(polygon, *m_polygons) {
        QPolygonF polyBounding = polygon->polygon();
        QRectF sceneRect = polyBounding.boundingRect();
        QPolygon screenPoly = m_scene->getView()->mapFromScene(sceneRect);
        QRect visibleBox = screenPoly.boundingRect();

        int bbWidth  = (int)visibleBox.width();
        int bbHeight = (int)visibleBox.height();

        int bbLeft = visibleBox.left();
        int bbTop = visibleBox.top();
        int bbRight = visibleBox.right();
        int bbBottom = visibleBox.bottom();

        QImage image(bbWidth, bbHeight, QImage::Format_ARGB32);

        for (int y = bbTop; y <= bbBottom; y++) {
          QRgb *lineData = (QRgb *)image.scanLine(y - bbTop);

          for (int x = bbLeft; x <= bbRight; x++) {
            lineData[x - bbLeft] = qRgba(0, 0, 0, 0);

            // We have an x,y in screen space. Let's translate it to
            //   projected space, ask the polygon if it's in the area,
            QPointF scenePos = m_scene->getView()->mapToScene(
                QPoint(x, y));

            if(polygon->polygon().containsPoint(scenePos, Qt::OddEvenFill)) {
              // This is likely in the cube... use the projection to go to
              //   lat/lon and use that lat/lon to go to cube sample,line
              m_scene->getProjection()->SetCoordinate(scenePos.x(),
                                                      -1 * scenePos.y());

              double lat = ((TProjection *)(m_scene->getProjection()))->UniversalLatitude();
              double lon = ((TProjection *)(m_scene->getProjection()))->UniversalLongitude();

              if(m_image) {
                if(!groundMap) {
                  groundMap = new UniversalGroundMap(*m_image->cube());
                }

                if(groundMap->SetUniversalGround(lat, lon)) {
                  double dn = Null;

                  if(groundMap->Camera() && groundMap->Camera()->InCube()) {
                    double samp = groundMap->Camera()->Sample();
                    double line = groundMap->Camera()->Line();

                    dn = getPixelValue((int)(samp + 0.5),
                                       (int)(line + 0.5));
                  }
                  else {
                    double samp = groundMap->Projection()->WorldX();
                    double line = groundMap->Projection()->WorldY();

                    dn = getPixelValue((int)(samp + 0.5),
                                       (int)(line + 0.5));
                  }

                  if(!IsSpecial(dn)) {
                    int stretched = (int)stretch->Map(dn);

                    lineData[x - bbLeft] = qRgba(stretched, stretched,
                                                stretched, 255);
                  }
                }
              }
            }
          }
        }

//         m_lastImages.append(image);
        painter->drawImage(polygon->boundingRect(), image);
      }
    }
    catch(IException &e) {
      e.print();
    }

    QApplication::restoreOverrideCursor();
  }


  QColor MosaicSceneItem::color() const {
    return
        m_image->displayProperties()->getValue(ImageDisplayProperties::Color).value<QColor>();
  }


  /**
   * Someone changed something in the cube display properties, re-read the
   *   whole thing.
   */
  void MosaicSceneItem::cubeDisplayChanged() {
    if (!m_ignoreCubeDisplayChanged) {
      bool wasBlocking = m_scene->blockSelectionChange(true);
      updateSelection(false);
      m_scene->blockSelectionChange(wasBlocking);

      if (m_showingLabel !=
          m_image->displayProperties()->getValue(ImageDisplayProperties::ShowLabel).toBool()) {
        // Reproject will create or not create a label item correctly. This is an important speed
        //   improvement - invisible items still cost us time.
        reproject();
      }
      else {
        updateChildren();
      }
    }
  }


  /**
   * This filters out events that happen within our polygons. This is necessary
   *   because usually events are filtered based on the bounding box alone.
   *
   * @param watched
   * @param event
   *
   * @return bool
   */
  bool MosaicSceneItem::sceneEvent(QEvent *event) {
    // We need to verify this event is really ours
    QPointF scenePos;

    switch (event->type()) {
      case QEvent::GraphicsSceneContextMenu:
        scenePos = ((QGraphicsSceneContextMenuEvent *)event)->scenePos();
        break;
      case QEvent::GraphicsSceneHoverEnter:
      case QEvent::GraphicsSceneHoverMove:
      case QEvent::GraphicsSceneHoverLeave:
        scenePos = ((QGraphicsSceneHoverEvent *)event)->scenePos();
        break;
      case QEvent::GraphicsSceneMouseMove:
      case QEvent::GraphicsSceneMousePress:
      case QEvent::GraphicsSceneMouseRelease:
      case QEvent::GraphicsSceneMouseDoubleClick:
        scenePos = ((QGraphicsSceneMouseEvent *)event)->scenePos();
        break;
      default:
        break;
    }

    bool ourEvent = true;
    if(!scenePos.isNull()) {
      ourEvent = contains(scenePos);
    }

    if(ourEvent) {
      return QGraphicsObject::sceneEvent(event);
    }
    else {
//    //qDebug()<<"MosaicSceneItem::sceneEvent  Ignore event";
      event->ignore();
      return true;
    }
  }


  /**
   * Test if we contain the point. Even though our rect is empty, return true
   *   if a child polygon contains it for toolTips and other events.
   */
  bool MosaicSceneItem::contains(const QPointF &p) const {
    if(p.isNull())
      return false;

    QGraphicsPolygonItem * polygon;
    foreach(polygon, *m_polygons) {
      if(polygon->contains(p)) {
        return true;
      }
    }

    return false;
  }


  /**
   * Update the selected state.
   *
   * @param save True if we need to write to the
   *             DisplayProperties, false if we need to read
   *             from them.
   */
  void MosaicSceneItem::updateSelection(bool save) {
    QGraphicsPolygonItem * polygon;

    m_ignoreCubeDisplayChanged = true;
    if (m_image) {
      bool selected =
          m_image->displayProperties()->getValue(ImageDisplayProperties::Selected).toBool();

      if(save) {
        selected = isSelected();

        // This code only works if the polygons are in the scene.
        foreach(polygon, *m_polygons) {
          selected = selected || (polygon->scene() && polygon->isSelected());
        }

        m_image->displayProperties()->setSelected(selected);
      }

      if(selected != isSelected()) {
        bool wasBlocking = m_scene->blockSelectionChange(true);
        setSelected(selected);
        m_scene->blockSelectionChange(wasBlocking);
      }

      foreach(polygon, *m_polygons) {
        if(polygon->isSelected() != selected) {
          polygon->setSelected(selected);
        }
      }
    }
    m_ignoreCubeDisplayChanged = false;
  }


  /**
   * The user right clicked on us (or otherwise requested a context menu).
   *
   * Give it to them.
   */
  void MosaicSceneItem::contextMenuEvent(
      QGraphicsSceneContextMenuEvent *event) {

    if(m_image) {
      QMenu menu;

      QAction *title = menu.addAction(m_image->displayProperties()->displayName());
      title->setEnabled(false);
      menu.addSeparator();

      ImageList images;
      images.append(m_image);

      Directory *directory = m_scene->directory();
      Project *project = directory ? directory->project() : NULL;

      QList<QAction *> displayActs = images.supportedActions(project);

      if (directory) {
        displayActs.append(NULL);
        displayActs.append(directory->supportedActions(new ImageList(images)));
      }

      QAction *displayAct;
      foreach(displayAct, displayActs) {
        if (displayAct == NULL) {
          menu.addSeparator();
        }
        else {
          menu.addAction(displayAct);
        }
      }

      menu.addSeparator();
      QAction *removeAction = menu.addAction("Close Cube");
      
      if (QApplication::applicationName() == "qmos") {
        connect(removeAction, SIGNAL(triggered()),
                m_image, SLOT(deleteLater()));
      }
      else {
        connect(removeAction, SIGNAL(triggered()), SLOT(onCloseCube()));
      }

      menu.exec(event->screenPos());
    }

  }


  void MosaicSceneItem::lostCubeDisplay() {
    m_image = NULL;
  }


  /**
   * Emits a signal when Close Cube is selected from the context menu
   */
  void MosaicSceneItem::onCloseCube() {
    emit mosaicCubeClosed(m_image);
  }
  
  
  /**
   * This applies the displayProperties and selectability. It's
   *   called updateChildren because the child items are the
   *   visually displayed items on the scene.
   */
  void MosaicSceneItem::updateChildren() {
    if (childItems().count()) {
      setFlag(QGraphicsItem::ItemIsSelectable, false);
    }
    else {
      setFlag(QGraphicsItem::ItemIsSelectable, m_scene->cubesSelectable());
    }

    QList<QRectF> regionsChanged;

    if(m_image) {
      foreach(QAbstractGraphicsShapeItem *polygon, *m_polygons) {
        // Fill
        if (m_image->displayProperties()->getValue(ImageDisplayProperties::ShowFill).toBool()) {
          polygon->setBrush(color());
        }
        else {
          polygon->setBrush(Qt::NoBrush);
        }

        // Outline
        QColor opaqueColor(color());
        opaqueColor.setAlpha(255);
        if (m_image->displayProperties()->getValue(ImageDisplayProperties::ShowOutline).toBool()) {
          // Make sure the outline is cosmetic (i.e. is always 1 pixel width on screen)
          QPen pen(opaqueColor);
          pen.setCosmetic(true);
          polygon->setPen(pen);
        }
        else {
          polygon->setPen(Qt::NoPen);
        }

        polygon->setFlag(QGraphicsItem::ItemIsSelectable,
                        m_scene->cubesSelectable());

        // Children (labels are the only children, and there should only be one)
        foreach(QGraphicsItem *polyChild, polygon->childItems()) {
          polyChild->setVisible(
              m_image->displayProperties()->getValue(ImageDisplayProperties::ShowLabel).toBool());

          polyChild->setFlag(QGraphicsItem::ItemIsSelectable,
                          m_scene->cubesSelectable());

          // Qt documentation was lacking the enum that this matches to, so this
          //   is the best I could do
          if(polyChild->type() == 9) {
            QGraphicsSimpleTextItem * text =
                (QGraphicsSimpleTextItem *)polyChild;
            text->setBrush(opaqueColor);
          }
        }
      }

      update();
      emit changed(regionsChanged);
    }
  }


  /**
   * This gets a Stretch object that will work for the
   *   cubeDisplay converting from DN to screen pixel.
   *
   * The first time this is called the stretch is calculated, later calls
   *   re-use the original object. Ownership remains at the class scope.
   */
  Stretch *MosaicSceneItem::getStretch() {
    if (m_cubeDnStretch != NULL || !m_image) return m_cubeDnStretch;

    LineManager mgr(*m_image->cube());

    mgr.begin();
    Statistics stats;

    const int skip = 0;

    while(mgr ++) {
      m_image->cube()->read(mgr);
      stats.AddData(mgr.DoubleBuffer(), mgr.size());

      for(int i = 0; i < skip; i++)
        mgr ++;
    }

    m_cubeDnStretch = new Stretch();
    m_cubeDnStretch->AddPair(stats.BestMinimum(), 0.0);
    m_cubeDnStretch->AddPair(stats.BestMaximum(), 255.0);

    m_cubeDnStretch->SetNull(0.0);
    m_cubeDnStretch->SetLis(0.0);
    m_cubeDnStretch->SetLrs(0.0);
    m_cubeDnStretch->SetHis(255.0);
    m_cubeDnStretch->SetHrs(255.0);
    m_cubeDnStretch->SetMinimum(0.0);
    m_cubeDnStretch->SetMaximum(255.0);

    return m_cubeDnStretch;
  }
}
