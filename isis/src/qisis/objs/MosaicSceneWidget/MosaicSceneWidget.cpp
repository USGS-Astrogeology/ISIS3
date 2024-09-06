#include "MosaicSceneWidget.h"

#include <sstream>

#include <QFileDialog>
#include <QGraphicsSceneContextMenuEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMenu>
#include <QRubberBand>
#include <QScrollBar>
#include <QStatusBar>
#include <QString>
#include <QToolButton>
#include <QToolTip>
#include <QtCore>
#include <QtWidgets>
#include <QtXml>

#include "Camera.h"
#include "Cube.h"
#include "Directory.h"
#include "Distance.h"
#include "FileName.h"
#include "GraphicsView.h"
#include "Image.h"
#include "ImageList.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MosaicAreaTool.h"
#include "MosaicControlNetTool.h"
#include "MosaicFindTool.h"
#include "MosaicGraphicsScene.h"
#include "MosaicGraphicsView.h"
#include "MosaicGridTool.h"
#include "MosaicPanTool.h"
#include "MosaicSceneItem.h"
#include "MosaicSelectTool.h"
#include "MosaicTrackTool.h"
#include "MosaicZoomTool.h"
#include "MoveDownOneSceneWorkOrder.h"
#include "MoveToBottomSceneWorkOrder.h"
#include "MoveToTopSceneWorkOrder.h"
#include "MoveUpOneSceneWorkOrder.h"
#include "ProgressBar.h"
#include "Project.h"
#include "Projection.h"
#include "ProjectionConfigDialog.h"
#include "ProjectionFactory.h"
#include "PvlObject.h"
#include "Pvl.h"
#include "Shape.h"
#include "TextFile.h"
#include "Target.h"
#include "ToolPad.h"

namespace Isis {
  /**
   * Create a scene widget.
   */
  MosaicSceneWidget::MosaicSceneWidget(QStatusBar *status, bool showTools,
                                       bool internalizeToolBarsAndProgress, Directory *directory,
                                       QWidget *parent) : QWidget(parent) {
    m_projectImageZOrders = NULL;
    m_projectViewTransform = NULL;
    m_directory = directory;

    m_mosaicSceneItems = new QList<MosaicSceneItem *>;

    m_graphicsScene = new MosaicGraphicsScene(this);
    m_graphicsScene->installEventFilter(this);

    m_graphicsView = new MosaicGraphicsView(m_graphicsScene, this);
    m_graphicsView->setScene(m_graphicsScene);
    m_graphicsView->setInteractive(true);
//     m_graphicsView->setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    m_graphicsView->setResizeAnchor(QGraphicsView::AnchorViewCenter);
    // This enables OpenGL acceleration
//     m_graphicsView->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
//     QPixmapCache::setCacheLimit(1024 * 1024);

    m_projection = NULL;
    m_mapButton = NULL;
    m_quickMapAction = NULL;

    m_cubesSelectable = true;
    m_customRubberBandEnabled = false;
    m_customRubberBand = NULL;
    m_rubberBandOrigin = NULL;
    m_outlineRect = NULL;
    m_blockingSelectionChanged = false;
    m_queuedSelectionChanged = false;
    m_shouldRequeueSelectionChanged = false;

    m_userToolControl = false;
    m_ownProjection = false;

    m_progress = new ProgressBar;
    m_progress->setVisible(false);

    QGridLayout * sceneLayout = new QGridLayout;
    sceneLayout->setContentsMargins(0, 0, 0, 0);
    setLayout(sceneLayout);

    // If we are making our own layout, we can create our own status area.
    if (!status && internalizeToolBarsAndProgress)
      status = new QStatusBar;

    // Create the tools we want
    m_tools = new QList<MosaicTool *>;
    m_tools->append(new MosaicSelectTool(this));
    m_tools->append(new MosaicZoomTool(this));
    m_tools->append(new MosaicPanTool(this));
    MosaicControlNetTool *cnetTool = new MosaicControlNetTool(this);
    m_tools->append(cnetTool);

    //  Pass on Signals emitted from MosaicControlNetTool
    //  TODO 2016-09-09 TLS Design:  Use a proxy model instead of signals?
    connect(cnetTool, SIGNAL(modifyControlPoint(ControlPoint *)),
            this, SIGNAL(modifyControlPoint(ControlPoint *)));

    connect(cnetTool, SIGNAL(deleteControlPoint(ControlPoint *)),
            this, SIGNAL(deleteControlPoint(ControlPoint *)));

    connect(cnetTool, SIGNAL(createControlPoint(double, double)),
            this, SIGNAL(createControlPoint(double, double)));

    // Pass on signals to the MosaicControlNetTool
    connect(this, SIGNAL(cnetModified()), cnetTool, SLOT(rebuildPointGraphics()));

    m_tools->append(new MosaicAreaTool(this));
    m_tools->append(new MosaicFindTool(this));
    m_tools->append(new MosaicGridTool(this));
    if (status)
      m_tools->append(new MosaicTrackTool(this, status));

    m_tools->at(0)->activate(true);

    if (showTools) {

      if (internalizeToolBarsAndProgress) {
        // Internalized Toolbar Layout:
        /*
         *  -------TOOLBARS------ Colspan=2, Rowspan=1
         *  |   SCENE        | T |
         *  |   CS=1, RS=1   | O |
         *  |                | O |
         *  |                | L |
         *  |                | B |
         *  |                | A |
         *  |                | R |*Vertical tool bar CS=1, RS=1
         *  ----PROGRESS---STATUS- Colspan=2, Rowspan=1
         *
         *
         */
        QHBoxLayout *horizontalToolBarsLayout = new QHBoxLayout;

        m_permToolbar = new QToolBar("Standard Tools");
        m_permToolbar->setWhatsThis("This area contains options that are always present in the "
            "footprint view");
        horizontalToolBarsLayout->addWidget(m_permToolbar);

        m_activeToolbar = new QToolBar("Active Tool", this);
        m_activeToolbar->setObjectName("Active Tool");
        m_activeToolbar->setWhatsThis("The currently selected tool's options will "
            "show up here. Not all tools have options.");
        horizontalToolBarsLayout->addWidget(m_activeToolbar);

        sceneLayout->addLayout(horizontalToolBarsLayout, 0, 0, 1, 2);

        sceneLayout->addWidget(m_graphicsView, 1, 0, 1, 1);

        m_toolpad = new ToolPad("Tool Pad", this);
        m_toolpad->setObjectName("Tool Pad");
        m_toolpad->setOrientation(Qt::Vertical);
        m_toolpad->setFloatable(true);
        sceneLayout->addWidget(m_toolpad, 1, 1, 1, 1);

        QHBoxLayout *horizontalStatusLayout = new QHBoxLayout;
        horizontalStatusLayout->addWidget(m_progress);
        horizontalStatusLayout->addStretch();
        horizontalStatusLayout->addWidget(status);

        sceneLayout->addLayout(horizontalStatusLayout, 2, 0, 1, 2);

        addToPermanent(m_permToolbar);
        m_permToolbar->addSeparator();

        addTo(m_activeToolbar);
        addTo(m_toolpad);
      }
      else {
        sceneLayout->addWidget(m_graphicsView, 0, 0, 1, 1);
      }

      m_userToolControl = true;

      setWhatsThis("This is the mosaic scene. The opened cubes will be "
          "shown here. You can fully interact with the files shown here.");

      getView()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
      getView()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

      getView()->enableResizeZooming(false);

      connect(getView()->horizontalScrollBar(), SIGNAL(valueChanged(int)),
              this, SLOT(sendVisibleRectChanged()));
      connect(getView()->verticalScrollBar() , SIGNAL(valueChanged(int)),
              this, SLOT(sendVisibleRectChanged()));
      connect(getView()->horizontalScrollBar(), SIGNAL(rangeChanged(int, int)),
              this, SLOT(sendVisibleRectChanged()));
      connect(getView()->verticalScrollBar() , SIGNAL(rangeChanged(int, int)),
              this, SLOT(sendVisibleRectChanged()));

    }
    else {
      sceneLayout->addWidget(m_graphicsView, 0, 0, 1, 1);

      getView()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
      getView()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

      setWhatsThis("This is the mosaic world view. The opened cubes will be "
          "shown here, but you cannot zoom in. You can select cubes by dragging "
          "a box over them, zoom to a particular cube by right clicking on it "
          "and selecting 'Zoom Fit', and many other actions are available.");
    }

    // These values are OK as long as they are less than the min/greater than the max. No footprints
    //   are available yet, so 0 qualifies for both (makes a good starting point).
    m_currentMinimumFootprintZ = 0;
    m_currentMaximumFootprintZ = 0;

    connect(getScene(), SIGNAL(selectionChanged()),
            this, SLOT(onSelectionChanged()));
    // This is set up to do a single selection changed after all selections have changed, instead
    //   of 1 selection changed per 1 changed item.
    connect(this, SIGNAL(queueSelectionChanged()),
            this, SLOT(onQueuedSelectionChanged()), Qt::QueuedConnection);
  }

  MosaicSceneWidget::~MosaicSceneWidget() {
    m_outlineRect = NULL; // The scene will clean this up

    if (m_tools) {
      foreach(MosaicTool *tool, *m_tools) {
        delete tool;
        tool = NULL;
      }

      delete m_tools;
      m_tools = NULL;
    }

    if (m_ownProjection && m_projection) {
      delete m_projection;
    }
    m_projection = NULL;

    delete m_projectImageZOrders;
    m_projectImageZOrders = NULL;

    delete m_projectViewTransform;
    m_projectViewTransform = NULL;
  }


  void MosaicSceneWidget::setProjection(const PvlGroup &mapping, Pvl label) {
    Pvl tmp;
    tmp += mapping;

    if (!mapping.hasKeyword("EquatorialRadius")) {
      PvlGroup radii = Target::radiiGroup(label, mapping);
      tmp.findGroup("Mapping") += radii["EquatorialRadius"];
      tmp.findGroup("Mapping") += radii["PolarRadius"];
    }

    setProjection(ProjectionFactory::Create(tmp));
    m_ownProjection = true;
  }


  /**
   * This method takes ownership of proj
   */
  void MosaicSceneWidget::setProjection(Projection *proj) {
    PvlGroup mapping(proj->Mapping());

    if (m_mapButton) {
      PvlKeyword projectionKeyword = mapping.findKeyword("ProjectionName");
      QString projName = QString::fromStdString(projectionKeyword[0]);
      m_mapButton->setText(tr("View/Edit %1 Projection").arg(projName));
    }

    Projection *old = m_projection;
    m_projection = proj;

    reprojectItems();
    emit projectionChanged(m_projection);

    if (old && m_ownProjection) {
      delete old;
      old = NULL;
    }

    m_ownProjection = false;
  }



  void MosaicSceneWidget::setOutlineRect(QRectF outline) {
    if (outline.united(getView()->sceneRect()) != getView()->sceneRect())
      outline = QRectF();

    if (!m_outlineRect) {
      m_outlineRect = getScene()->addRect(outline,
                                          QPen(Qt::black),
                                          Qt::NoBrush);
      m_outlineRect->setZValue(DBL_MAX);
    }
    else {
      m_outlineRect->setRect(outline);
    }

    if (!m_userToolControl)
      refit();
  }


  PvlGroup MosaicSceneWidget::createInitialProjection(
      Image *image) {
    Projection *proj = NULL;
    Cube *cube = image->cube();
    Pvl *label = cube->label();

    try {
      proj = ProjectionFactory::CreateFromCube(*label);
      return proj->Mapping();
    }
    catch (IException &) {
      Pvl mappingPvl("$ISISROOT/appdata/templates/maps/equirectangular.map");
      PvlGroup &mappingGrp = mappingPvl.findGroup("Mapping");
      mappingGrp += PvlKeyword("LatitudeType", "Planetocentric");
      mappingGrp += PvlKeyword("LongitudeDirection", "PositiveEast");
      mappingGrp += PvlKeyword("LongitudeDomain", "360");
      mappingGrp += PvlKeyword("CenterLatitude", "0");
      mappingGrp += PvlKeyword("CenterLongitude", "180");
      mappingGrp += PvlKeyword("MinimumLatitude", "-90");
      mappingGrp += PvlKeyword("MaximumLatitude", "90");
      mappingGrp += PvlKeyword("MinimumLongitude", "0");
      mappingGrp += PvlKeyword("MaximumLongitude", "360");

      try {
        Camera * cam = cube->camera();
        Distance radii[3];
        cam->radii(radii);

        mappingGrp += PvlKeyword("TargetName", cam->target()->name().toStdString());
        mappingGrp += PvlKeyword("EquatorialRadius", std::to_string(radii[0].meters()),
                                 "meters");
        mappingGrp += PvlKeyword("PolarRadius", std::to_string(radii[2].meters()),
                                 "meters");

      }
      catch (IException &) {
        mappingGrp +=
            label->findGroup("Instrument", Pvl::Traverse)["TargetName"];
      }

      return mappingGrp;
    }
  }


  void MosaicSceneWidget::addToPermanent(QToolBar *perm) {
    m_mapButton = new QToolButton(this);
    connect(this, SIGNAL(destroyed()), m_mapButton, SLOT(deleteLater()));
    m_mapButton->setText(tr("View/Edit/Load Map File"));
    m_mapButton->setToolTip(tr("View/Edit/Load Map File"));
    m_mapButton->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/ographic.png").expanded())));
    m_mapButton->setWhatsThis(tr("This is the projection used by the mosaic "
        "scene. Cubes can not be shown in the scene without a projection, so "
        "if one is not selected, a default of Equirectangular will be used. "
        "The selected file should be a map file, examples are available in "
        "$ISISROOT/appdata/templates/maps."));
    m_mapButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(m_mapButton, SIGNAL(clicked()), this, SLOT(configProjectionParameters()));

    if (m_projection) {
      PvlKeyword projectionKeyword =
          m_projection->Mapping().findKeyword("ProjectionName");
      QString projName = QString::fromStdString(projectionKeyword[0]);
      m_mapButton->setText(projName);
    }

    m_quickMapAction = new QAction(tr("Quick Load Map"), this);
    m_quickMapAction->setToolTip(tr("Quick Load Map"));
    m_quickMapAction->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/quickopen.png").expanded())));
    m_quickMapAction->setWhatsThis(tr("This is the projection used by the mosaic "
        "scene. Cubes can not be shown in the scene without a projection, so "
        "if one is not selected, a default of Equirectangular will be used."));
    connect(m_quickMapAction, SIGNAL(triggered()), this, SLOT(quickConfigProjectionParameters()));

    perm->addWidget(m_mapButton);
    perm->addAction(m_quickMapAction);
  }


  void MosaicSceneWidget::addTo(QToolBar *toolbar) {
    MosaicTool *tool;
    foreach(tool, *m_tools) {
      tool->addTo(toolbar);
    }
  }


  void MosaicSceneWidget::addTo(QMenu *menu) {
    MosaicTool *tool;
    foreach(tool, *m_tools) {
      tool->addTo(menu);
    }
  }


  void MosaicSceneWidget::addTo(ToolPad *toolPad) {
    MosaicTool *tool;
    foreach(tool, *m_tools) {
      tool->addTo(toolPad);
    }
  }


  /**
   * This is called by MosaicGraphicsScene::contextMenuEvent.
   *
   * Return false if not handled, true if handled.
   */
  bool MosaicSceneWidget::contextMenuEvent(QGraphicsSceneContextMenuEvent *event) {

    bool handled = false;
    QList<QGraphicsItem *> selectedGraphicsItems = getScene()->selectedItems();
    QList<MosaicSceneItem *> selectedImageItems;
    ImageList selectedImages;
    foreach (QGraphicsItem *graphicsItem, selectedGraphicsItems) {
      MosaicSceneItem *sceneImageItem = dynamic_cast<MosaicSceneItem *>(graphicsItem);

      if (!sceneImageItem) {
        sceneImageItem = dynamic_cast<MosaicSceneItem *>(graphicsItem->parentItem());
      }

      if (sceneImageItem && sceneImageItem->image()) {
        selectedImageItems.append(sceneImageItem);
        selectedImages.append(sceneImageItem->image());
      }
    }

    if (selectedImageItems.count()) {
      QMenu menu;

      QAction *title = menu.addAction(tr("%L1 Selected Images").arg(selectedImages.count()));
      title->setEnabled(false);
      menu.addSeparator();

      Project *project = m_directory ? m_directory->project() : NULL;

      QList<QAction *> displayActs = selectedImages.supportedActions(project);

      if (m_directory) {
        displayActs.append(NULL);
        displayActs.append(m_directory->supportedActions(new ImageList(selectedImages)));
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

      handled = true;
      menu.exec(event->screenPos());
    }

    return handled;

  }


  void MosaicSceneWidget::enableRubberBand(bool enable) {
    m_customRubberBandEnabled = enable;
  }


  MosaicSceneItem *MosaicSceneWidget::cubeToMosaic(Image *image) {
    if (image == NULL) {
      std::string msg = tr("Can not find a NULL image in the mosaic");
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return cubeToMosaic(image->displayProperties());
  }


  bool MosaicSceneWidget::blockSelectionChange(bool block) {
    bool wasBlocking = m_blockingSelectionChanged;

    m_blockingSelectionChanged = block;

    return wasBlocking;
  }


  QProgressBar *MosaicSceneWidget::getProgress() {
    return m_progress;
  }


  PvlObject MosaicSceneWidget::toPvl() const {
    PvlObject output("MosaicScene");

    if (m_projection) {
      output += m_projection->Mapping();

      QBuffer dataBuffer;
      dataBuffer.open(QIODevice::ReadWrite);
      QDataStream transformStream(&dataBuffer);
      transformStream << getView()->transform();
      dataBuffer.seek(0);

      PvlObject mosaicScenePosition("SceneVisiblePosition");
      mosaicScenePosition += PvlKeyword("ViewTransform",
                                        std::string(dataBuffer.data().toHex()));
      PvlKeyword scrollPos("ScrollPosition");
      scrollPos += std::to_string(getView()->horizontalScrollBar()->value());
      scrollPos += std::to_string(getView()->verticalScrollBar()->value());
      mosaicScenePosition += scrollPos;

      output += mosaicScenePosition;

      MosaicTool *tool;
      foreach(tool, *m_tools) {
        if (tool->projectPvlObjectName() != "") {
          PvlObject toolObj = tool->toPvl();
          toolObj.setName(tool->projectPvlObjectName().toStdString());
          output += toolObj;
        }
      }

      PvlObject zOrders("ZOrdering");
      foreach(MosaicSceneItem * mosaicSceneItem, *m_mosaicSceneItems) {
        PvlKeyword zValue("ZValue");
        zValue += mosaicSceneItem->image()->id().toStdString();
        zValue += std::to_string(mosaicSceneItem->zValue());
        zOrders += zValue;
      }

      output += zOrders;
    }
    else {
      throw IException(IException::User,
          "Cannot save a scene without a projection to a project file",
          _FILEINFO_);
    }

    return output;
  }


  /**
   * Call this method after loading any cubes when loading a project.
   *
   * @param project The project Pvl
   */
  void MosaicSceneWidget::fromPvl(const PvlObject &project) {
    setProjection(project.findGroup("Mapping"));
    recalcSceneRect();

    MosaicTool *tool;
    foreach(tool, *m_tools) {
      if (tool->projectPvlObjectName() != "") {
        if (project.hasObject(tool->projectPvlObjectName().toStdString())) {
          const PvlObject &toolSettings(
              project.findObject(tool->projectPvlObjectName().toStdString()));
          tool->fromPvl(toolSettings);
        }
      }

      if (project.hasObject("ZOrdering")) {
        const PvlObject &zOrders = project.findObject("ZOrdering");

        delete m_projectImageZOrders;
        m_projectImageZOrders = NULL;
        m_projectImageZOrders = new QHash<QString, double>;

        for (int zOrderIndex = 0;
             zOrderIndex < zOrders.keywords();
             zOrderIndex ++) {
          const PvlKeyword &zOrder = zOrders[zOrderIndex];

          (*m_projectImageZOrders)[QString::fromStdString(zOrder[0])] = std::stod(zOrder[1]);
        }
      }

      if (project.hasObject("SceneVisiblePosition")) {
        const PvlObject &positionInfo =
            project.findObject("SceneVisiblePosition");

        delete m_projectViewTransform;
        m_projectViewTransform = new PvlObject(positionInfo);
      }
    }
  }


  void MosaicSceneWidget::save(QXmlStreamWriter &stream, Project *, FileName ) const {
    if (m_projection) {
      stream.writeStartElement("mosaicScene");

      stream.writeStartElement("projection");
      PvlGroup mapping = m_projection->Mapping();
      std::stringstream strStream;
      strStream << mapping;
      stream.writeCharacters(strStream.str().c_str());
      stream.writeEndElement();

      stream.writeStartElement("images");
      foreach(MosaicSceneItem * mosaicSceneItem, *m_mosaicSceneItems) {
        stream.writeStartElement("image");
        stream.writeAttribute("id", mosaicSceneItem->image()->id());
        stream.writeAttribute("zValue", toString(mosaicSceneItem->zValue()));
        stream.writeEndElement();
      }
      stream.writeEndElement();

      stream.writeStartElement("viewTransform");
      stream.writeAttribute("scrollBarXValue", toString(getView()->horizontalScrollBar()->value()));
      stream.writeAttribute("scrollBarYValue", toString(getView()->verticalScrollBar()->value()));
      QBuffer dataBuffer;
      dataBuffer.open(QIODevice::ReadWrite);
      QDataStream transformStream(&dataBuffer);
      transformStream << getView()->transform();
      dataBuffer.seek(0);
      stream.writeCharacters(dataBuffer.data().toHex());
      stream.writeEndElement();

      foreach(MosaicTool *tool, *m_tools) {
        QString projectPvlObjectName = tool->projectPvlObjectName();
        if (projectPvlObjectName != "") {
          PvlObject toolObj = tool->toPvl();
          toolObj.setName(projectPvlObjectName.toStdString());

          stream.writeStartElement("toolData");
          stream.writeAttribute("objectName", projectPvlObjectName);
          std::stringstream strStream;
          strStream << toolObj;
          stream.writeCharacters(strStream.str().c_str());
          stream.writeEndElement();
        }
      }

      stream.writeEndElement();
    }
  }


  QRectF MosaicSceneWidget::cubesBoundingRect() const {
    QRectF boundingRect;

    MosaicSceneItem * mosaicItem;
    foreach(mosaicItem, *m_mosaicSceneItems) {
      if (boundingRect.isEmpty())
        boundingRect = mosaicItem->boundingRect();
      else
        boundingRect = boundingRect.united(mosaicItem->boundingRect());
    }

    if (m_outlineRect)
      boundingRect = boundingRect.united(m_outlineRect->boundingRect());

    return boundingRect;
  }


  MosaicSceneItem *MosaicSceneWidget::cubeToMosaic(DisplayProperties *props) {
    if (props == NULL) {
      std::string msg = tr("Can not find a NULL Display Properties in the mosaic");
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return m_displayPropsToMosaicSceneItemMap[props];
  }


  QStringList MosaicSceneWidget::cubeFileNames() {
    QStringList cubes;

    MosaicSceneItem *mosaicSceneItem;
    foreach(mosaicSceneItem, *m_mosaicSceneItems) {
      if (mosaicSceneItem->image())
        cubes.append(mosaicSceneItem->image()->fileName());
    }

    return cubes;
  }


  Directory *MosaicSceneWidget::directory() const {
    return m_directory;
  }


  ImageList MosaicSceneWidget::images() {
    ImageList images;

    MosaicSceneItem *mosaicSceneItem;
    foreach(mosaicSceneItem, *m_mosaicSceneItems) {
      if (mosaicSceneItem->image())
        images.append(mosaicSceneItem->image());
    }

    return images;
  }


  /**
   * Returns a list of all the cubes selected in the scene
   *
   * @return QList<Cube *>
   */
  ImageList MosaicSceneWidget::selectedImages()  {

    QList<QGraphicsItem *> selectedGraphicsItems = getScene()->selectedItems();
    QList<MosaicSceneItem *> selectedImageItems;
    ImageList selectedImages;

    foreach (QGraphicsItem *graphicsItem, selectedGraphicsItems) {
      MosaicSceneItem *sceneImageItem = dynamic_cast<MosaicSceneItem *>(graphicsItem);

      if (!sceneImageItem) {
        sceneImageItem = dynamic_cast<MosaicSceneItem *>(graphicsItem->parentItem());
      }

      if (sceneImageItem && sceneImageItem->image()) {
        selectedImageItems.append(sceneImageItem);
        selectedImages.append(sceneImageItem->image());
      }
    }
    return selectedImages;





//  ImageList images;
//  //qDebug()<<"MosaicSceneWidget::selectedImages  TotalItems = "<<m_mosaicSceneItems->size();
//  MosaicSceneItem *mosaicItem;
//  foreach(mosaicItem, *m_mosaicSceneItems) {
//    if (mosaicItem->isSelected()) {
//      images.append(mosaicItem->image());
//    }
//  }
//
//  return images;
  }


  QList<QAction *> MosaicSceneWidget::getExportActions() {
    QList<QAction *> exportActs;

    QAction *exportView = new QAction(this);
    exportView->setText("&Export View...");
    connect(exportView, SIGNAL(triggered()), this, SLOT(exportView()));

    QAction *saveList = new QAction(this);
    saveList->setText("Save Entire Cube List (ordered by &view)...");
    connect(saveList, SIGNAL(triggered()), this, SLOT(saveList()));

    exportActs.append(exportView);
    exportActs.append(saveList);

    return exportActs;
  }


  QList<QAction *> MosaicSceneWidget::getViewActions() {
    QList<QAction *> viewActs;

    foreach(MosaicTool *tool, *m_tools) {
      QList<QAction *> toolViewActs = tool->getViewActions();
      viewActs.append(toolViewActs);
    }

    return viewActs;
  }


  /**
   * Get a list of actions this scene can perform given "images"
   */
  QList<QAction *> MosaicSceneWidget::supportedActions(ImageList *images) {
    QList<QAction *> results;
    bool allImagesInView = !images->isEmpty();

    foreach (Image *image, *images) {
      allImagesInView = allImagesInView && (cubeToMosaic(image) != NULL);
    }

    if (allImagesInView) {
      MoveToTopSceneWorkOrder *moveToTopAct =
          new MoveToTopSceneWorkOrder(this, m_directory->project());
      moveToTopAct->setData(images);
      results.append(moveToTopAct);

      MoveUpOneSceneWorkOrder *moveUpOneAct =
          new MoveUpOneSceneWorkOrder(this, m_directory->project());
      moveUpOneAct->setData(images);
      results.append(moveUpOneAct);

      MoveToBottomSceneWorkOrder *moveToBottomAct =
          new MoveToBottomSceneWorkOrder(this, m_directory->project());
      moveToBottomAct->setData(images);
      results.append(moveToBottomAct);

      MoveDownOneSceneWorkOrder *moveDownOneAct =
          new MoveDownOneSceneWorkOrder(this, m_directory->project());
      moveDownOneAct->setData(images);
      results.append(moveDownOneAct);

      results.append(NULL);

      QAction *zoomFitAct = new QAction(tr("Zoom Fit"), this);
      zoomFitAct->setData(QVariant::fromValue(images));
      connect(zoomFitAct, SIGNAL(triggered()), this, SLOT(fitInView()));
      results.append(zoomFitAct);
    }
    return results;
  }


  bool MosaicSceneWidget::isControlNetToolActive() {

    foreach(MosaicTool *tool, *m_tools) {
      MosaicControlNetTool *cnTool = dynamic_cast<MosaicControlNetTool *>(tool);
      if (cnTool) {
        if (cnTool->isActive()) return true;
      }
    }
    return false;
  }


  QWidget * MosaicSceneWidget::getControlNetHelp(QWidget *cnetToolContainer) {
    QScrollArea *cnetHelpWidgetScrollArea = new QScrollArea;

    QWidget *cnetHelpWidget = new QWidget;

    QVBoxLayout *cnetHelpLayout = new QVBoxLayout;
    cnetHelpWidget->setLayout(cnetHelpLayout);

    QLabel *title = new QLabel("<h2>Control Networks</h2>");
    cnetHelpLayout->addWidget(title);

    QPixmap previewPixmap;

    if (cnetToolContainer) {
      previewPixmap = cnetToolContainer->grab().scaled(
          QSize(500, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else {
      ToolPad tmpToolPad("Example Tool Pad", NULL);
      MosaicControlNetTool tmpTool(NULL);
      tmpTool.addTo(&tmpToolPad);

      tmpToolPad.resize(QSize(32, 32));

      previewPixmap = tmpToolPad.grab();
    }

    QLabel *previewWrapper = new QLabel;
    previewWrapper->setPixmap(previewPixmap);
    cnetHelpLayout->addWidget(previewWrapper);

    QLabel *overview = new QLabel("The mosaic scene can display control points "
        "in addition to the usual cube footprints. This feature is currently "
        "offered as one of the Mosaic Scene's tools. To open a network, click "
        "on the control network tool. It will immediately prompt you for a "
        "control network file if one is not open. Only control points for "
        "which the latitude and longitude can be established will be "
        "displayed. Other control points will be ignored by qmos.<br><br>"
        "<b>Warning: Opening large control networks is slow.</b>"
        "<h3>Control Network Tool Options</h3>"
        "<ul>"
          "<li>The control network tool opens control networks in two ways. "
          "First, if you select the control network tool and no network is "
          "open, then it will prompt you for one. Second, if there is an open "
          "network, the buttons for the available options are displayed in the "
          "active tool area.</li>"
          "<li>The control network tool can toggle whether or not control "
          "points are displayed on the screen using the 'Display' button. "
          "Control points are always on top and colored based on their "
          "ignored, locked and type values.</li>"
          "<li>This tool can also change the color of your footprints based on "
          "connectivity through control points. This is available through the "
          "'Color Islands' button. When you press color islands, all of the "
          "current cube coloring information is lost and re-done based on "
          "how the control network connects the files. Each set of connected "
          "cubes are colored differently; generally speaking, islands are not "
          "a good thing to have in your control network.</li>"
          "<li>This tool will color your footprints on a per-image basis if you "
          "click color images, effectively reversing color islands.</li>"
          "<li>The show movement option under 'Configure Movement Display' "
          "only displays data when the control "
          "network has adjusted values. This means that show movement only "
          "works after you have done a jigsaw solution on the control network. "
          "This displays arrows emanating from the apriori latitude/longitude "
          "and pointing toward the adjusted latitude/longitude.</li>");
    overview->setWordWrap(true);
    cnetHelpLayout->addWidget(overview);

    cnetHelpWidgetScrollArea->setWidget(cnetHelpWidget);

    return cnetHelpWidgetScrollArea;
  }


  QWidget * MosaicSceneWidget::getGridHelp(QWidget *gridToolContainer) {
    QScrollArea *gridHelpWidgetScrollArea = new QScrollArea;

    QWidget *gridHelpWidget = new QWidget;

    QVBoxLayout *gridHelpLayout = new QVBoxLayout;
    gridHelpWidget->setLayout(gridHelpLayout);

    QLabel *title = new QLabel("<h2>Map Grid Tool</h2>");
    gridHelpLayout->addWidget(title);

    QPixmap previewPixmap;

    if (gridToolContainer) {
      previewPixmap = gridToolContainer->grab().scaled(
          QSize(500, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    else {
      ToolPad tmpToolPad("Example Tool Pad", NULL);
      MosaicGridTool tmpTool(NULL);
      tmpTool.addTo(&tmpToolPad);

      tmpToolPad.resize(QSize(32, 32));

      previewPixmap = tmpToolPad.grab();
    }

    QLabel *previewWrapper = new QLabel;
    previewWrapper->setPixmap(previewPixmap);
    gridHelpLayout->addWidget(previewWrapper);

    QLabel *overview = new QLabel("Superimpose a map grid over the area of "
        "displayed footprints in the 'mosaic scene.'"
        "<h2>Overview</h2>"
        "<ul>"
          "<li>The Map Grid Tool is activated by selecting the 'cross-hatch' "
              "icon or typing 'g' at the keyboard."
          "</li>"
          "<li>The parameter options are displayed below the menubar. "
              "Clicking the 'Grid Options' button will open the dialog. Checking "
              "'Auto Grid' will draw a grid based on the open cubes. Hitting "
              "'Show Grid' will display or hide the grid."
          "</li>"
          "<li>The map grid is defined by the loaded Map File (just as the "
              "footprints and image data are), the opened cubes, or the grid "
              "tool parameters."
          "</li>"
          "<li>If a Map File has not been selected, the default "
              "Equirectangular projection will be used. The resulting grid "
              "lines in the default 'Equi' map file will be drawn for the "
              "full global range (latitude range = -90,90; longitude range = "
              "0,360) at the default latitude and longitude increment values."
          "</li>"
          "<li>"
              "If the grid lines are not immediately visible, try to "
              "'zoom out' in the 'mosaic scene' window and modify the "
              "Latitude and Longitude Increment parameters."
          "</li>"
        "</ul>"
        "<strong>Options:</strong>"
        "<ul>"
        "<li>The 'Show Grid' option draws (checked) or clears (unchecked) the grid."
        "</li>"
        "<li>The 'Auto Grid' option draws a grid with extents and increments "
            "computed from the set of images that are opened. The values displayed in the dialog "
            "will reflect those used to draw the grid."
        "</li>"
        "<li>The expected units for each entry are displayed to the right of the "
            "dialog box."
        "</li>"
        "<li>The 'Extent Type' combo boxes allow you to pick the source of the "
            "grid extents from the map file, from the open cubes <default>, or manually "
            "entered."
        "</li>"
        "<li>The 'Auto Apply' checkbox, inside the 'Grid Options' dialog box, allows you to see "
            "real time updates in the grid when you change the parameters."
        "</li>"
        "<li> Depending on the projection, the grid may not behave as expected. For instance, "
            "with a polarstereographic projection, the pole will not be included in the 'Auto "
            "Grid' if it is not in the cube region. In this case the 'Manual' option for latitude "
            "extents allows you to force the grid to the pole."
        "</li>"
        "</ul>");
    overview->setWordWrap(true);
    gridHelpLayout->addWidget(overview);

    gridHelpWidgetScrollArea->setWidget(gridHelpWidget);

    return gridHelpWidgetScrollArea;
  }


  QWidget * MosaicSceneWidget::getLongHelp(QWidget *sceneContainer) {
    QScrollArea *longHelpWidgetScrollArea = new QScrollArea;

    QWidget *longHelpWidget = new QWidget;

    QVBoxLayout *longHelpLayout = new QVBoxLayout;
    longHelpWidget->setLayout(longHelpLayout);

    QLabel *title = new QLabel("<h2>Mosaic Scene</h2>");
    longHelpLayout->addWidget(title);

    if (sceneContainer) {
      QPixmap previewPixmap = sceneContainer->grab().scaled(
          QSize(500, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      QLabel *previewWrapper = new QLabel;
      previewWrapper->setPixmap(previewPixmap);
      longHelpLayout->addWidget(previewWrapper);
    }

    QLabel *overview = new QLabel("The mosaic scene displays cube footprints "
        "to show where files are on a target and how they overlap. "
        "The scene always represents projected image space and cannot show raw "
        "or unprojected images; images will be projected on the fly."
        "<h3>Tools</h3>"
            "<p>Interact with the mosaic scene in different ways using "
            "the tools. The tools are usually in a toolbar next to the scene. "
            "The tools define what is displayed and what happens when you "
            "click in the mosaic scene. The tools include</p>"
              "<ul><li>Select Tool</li>"
              "<li>Zoom Tool</li>"
              "<li>Pan Tool</li>"
              "<li>Control Network Tool</li>"
              "<li>Show Area Tool</li>"
              "<li>Find Tool</li>"
              "<li>Grid Tool</li></ul>"
        "<h3>Context Menus</h3>"
            "Right click on anything in the mosaic scene and a context menu will pop up showing "
            "a list of actions or information relevant to the item you clicked on. "
            "<p>Note:  The context menu is not associated with any selection, only the item "
            "clicked on.</p>");
    overview->setWordWrap(true);
    longHelpLayout->addWidget(overview);

    longHelpWidgetScrollArea->setWidget(longHelpWidget);

    return longHelpWidgetScrollArea;
  }


  QWidget * MosaicSceneWidget::getMapHelp(QWidget *mapContainer) {
    QScrollArea *mapHelpWidgetScrollArea = new QScrollArea;

    QWidget *mapHelpWidget = new QWidget;

    QVBoxLayout *mapHelpLayout = new QVBoxLayout;
    mapHelpWidget->setLayout(mapHelpLayout);

    QLabel *title = new QLabel(tr("<h2>Map File</h2>"));
    mapHelpLayout->addWidget(title);

    if (mapContainer) {
      QPixmap previewPixmap = mapContainer->grab().scaled(
          QSize(500, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      QLabel *previewWrapper = new QLabel;
      previewWrapper->setPixmap(previewPixmap);
      mapHelpLayout->addWidget(previewWrapper);
    }

    QLabel *overviewMapIcon = new QLabel;

    overviewMapIcon->setPixmap(
        QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/ographic.png").expanded())).pixmap(32, 32));
    mapHelpLayout->addWidget(overviewMapIcon);

    QLabel *defaultMapFile = new QLabel(tr(
        "<h3>Default Map File</h3>"
        "The mosaic scene's projection is defined by a \"Map File\" that consists of keywords "
        "that describe the map layout to be used. If a cube or a list of cubes are "
        "loaded before a map file is selected, the default map file defines the "
        "equirectangular projection, planetocentric latitude, positive longitude east, 360 "
        "longitude domain, latitude range=90S-90N, longitude range=0-360E. The radius will "
        "default to the IAU standards (ellipsoid or sphere) for the specific planetary body "
        "defined for the \"TargetName\" in the labels of the image cube(s)."));

    defaultMapFile->setWordWrap(true);
    mapHelpLayout->addWidget(defaultMapFile);

    QLabel *userDefinedMapFileOverview = new QLabel(tr(
        "<h3>User Defined Map File</h3>"
        "You can load an existing \"Map File\" before loading images into %1 by selecting the "
        "\"View/Edit/Load Map File\" option. You will be greeted with a dialog box that will "
        "enable you to select an existing map file by clicking on \"Load Map File.\" Once "
        "the map file is selected, the contents is displayed in the dialog box where "
        "modifications can be made as well. If the modified map file is to be used later, "
        "save the map file by clicking on \"Save Map File\" button.")
        .arg(QCoreApplication::applicationName()));

    userDefinedMapFileOverview->setWordWrap(true);
    mapHelpLayout->addWidget(userDefinedMapFileOverview);

    QLabel *userDefinedMapFileQuickLoad = new QLabel(tr(
        "The \"Quick Load Map\" option (lightning icon) allows you to efficiently select a "
        "prepared \"Map File\" without an immediate need to view or edit the contents."));

    userDefinedMapFileQuickLoad->setWordWrap(true);
    mapHelpLayout->addWidget(userDefinedMapFileQuickLoad);

    QLabel *userDefinedMapFileAnyTime = new QLabel(tr(
        "At any point, you have access to the \"View/Edit\" functionality to modify or load a "
        "different map file."));

    userDefinedMapFileAnyTime->setWordWrap(true);
    mapHelpLayout->addWidget(userDefinedMapFileAnyTime);

    QString mapProjWorkshopUrl("https://github.com/USGS-Astrogeology/ISIS3/wiki/Learning_About_Map_Projections");
    QLabel *preparingMapFile = new QLabel(tr(
        "<h3>Preparing a Map File</h3>"
        "Please refer to Isis applications such as 'maptemplate' or 'mosrange' for more details "
        "on creating a custom map file that defines the desired projection, latitude "
        "system, and longitude direction and domain. This program will use the latitude range "
        "and longitude range if they exist in the loaded file. A choice of map templates that can be used as "
        "a starting point for supported map projections can be found in $ISISROOT/appdata/templates/maps (refer "
        "to maptemplate or mosrange for more details and information on the required parameters "
        "for a projection). The website: "
        "<a href='%1'>%1</a> also provides useful information about map projections.")
        .arg(mapProjWorkshopUrl));

    preparingMapFile->setOpenExternalLinks(true);
    preparingMapFile->setWordWrap(true);
    mapHelpLayout->addWidget(preparingMapFile);

    QLabel *mapFileDisplayResults = new QLabel(tr(
        "<h3>Display Results with the Map File</h3>"
        "The footprints and image data that are displayed in the mosaic scene are defined by the "
        "loaded \"Map File\" regardless of whether the opened cubes are Level1 (raw "
        "camera space) or Level2 (map projected). The associated footprint polygons for "
        "Level2 cubes will be re-mapped as needed based on the loaded map file."));

    mapFileDisplayResults->setWordWrap(true);
    mapHelpLayout->addWidget(mapFileDisplayResults);

    QLabel *editingMapFileOverview = new QLabel(tr(
        "<h3>Editing a Map File</h3>"
        "Editing a map file is possible through the dialog box displayed by clicking on the "
        "'View/Edit/Load Map File' icon/button. The edits are "
        "applied to the current session and will be included with a 'Saved Project' (refer to "
        "the help under File-Save Project or Save Project as)."));

    editingMapFileOverview->setWordWrap(true);
    mapHelpLayout->addWidget(editingMapFileOverview);

    QLabel *saveMapFileToDiskBullet = new QLabel(tr(
        "<ul>"
          "<li>"
            "To save or write the changes to a map file on disk, choose 'Save Map File' button. "
            "Map files can be saved to an existing map file (overwrites) or to a new file. This "
            "program always saves <strong>exactly</strong> what you see, the text, in the dialog "
            "box."
          "</li>"
        "</ul>"));

    saveMapFileToDiskBullet->setWordWrap(true);
    mapHelpLayout->addWidget(saveMapFileToDiskBullet);

    QLabel *mapFileValidityBullet = new QLabel(tr(
        "<ul>"
          "<li>"
            "As you modify the contents of a loaded map file in the dialog box, the entry is "
            "verified as you type with a bold black indicator message displaying whether the "
            "text is valid or is not valid. If you want to see the actual error messages, "
            "select the 'Show Errors' box and the errors will be displayed in red font "
            "along with the black bolded message. The errors will update "
            "as you type."
          "</li>"
        "</ul>"));

    mapFileValidityBullet->setWordWrap(true);
    mapHelpLayout->addWidget(mapFileValidityBullet);

    QLabel *mapFileCommentsBullet = new QLabel(tr(
        "<ul>"
          "<li>"
            "Map files may contain 'commented-out' lines (text that starts with \"#\" at "
            "the beginning of the line). These are referred to as \"unnecessary\""
            "or \"unknown\" keywords, they are simply ignored. If these lines are to be saved to "
            "the output map file on disk, click 'Save Map File' BEFORE clicking 'Ok' or 'Apply.' "
            "The comments are removed from the dialog box when you hit 'Ok' or 'Apply,' if they "
            "are just above \"End_Group\" or follow \"End_Group\" or \"End\".<br/><br/>"

            "If you want these comments retained, make sure they are immediately above a valid "
            "keyword inside of \"Group = Mapping.\" Note that any lines (commented or not) will "
            "not be saved if they are placed outside of \"Group = Mapping\" and \"End_Group\"."
          "</li>"
        "</ul>"));

    mapFileCommentsBullet->setWordWrap(true);
    mapHelpLayout->addWidget(mapFileCommentsBullet);

    mapHelpWidgetScrollArea->setWidget(mapHelpWidget);

    return mapHelpWidgetScrollArea;
  }


  QWidget * MosaicSceneWidget::getPreviewHelp(QWidget *worldViewContainer) {
    QScrollArea *previewHelpWidgetScrollArea = new QScrollArea;

    QWidget *previewHelpWidget = new QWidget;

    QVBoxLayout *previewHelpLayout = new QVBoxLayout;
    previewHelpWidget->setLayout(previewHelpLayout);

    QLabel *title = new QLabel("<h2>Mosaic World View</h2>");
    previewHelpLayout->addWidget(title);

    if (worldViewContainer) {
      QPixmap previewPixmap = worldViewContainer->grab().scaled(
          QSize(500, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      QLabel *previewWrapper = new QLabel;
      previewWrapper->setPixmap(previewPixmap);
      previewHelpLayout->addWidget(previewWrapper);
    }

    QLabel *overview = new QLabel("The mosaic world view displays cube "
        "footprints to show you where your files are on a target and their "
        "general arrangement. The world view does not have tools like "
        "mosaic scenes, but otherwise they are very similar.");
    overview->setWordWrap(true);
    previewHelpLayout->addWidget(overview);

    previewHelpWidgetScrollArea->setWidget(previewHelpWidget);

    return previewHelpWidgetScrollArea;
  }


  MosaicSceneItem *MosaicSceneWidget::addImage(Image *image) {
    if (m_projection == NULL) {
      setProjection(createInitialProjection(image), *image->cube()->label());
    }

    MosaicSceneItem *mosItem = NULL;

    // Verify we don't have this cube already... if we do, ignore the add request
    if (!cubeToMosaic(image)) {
      mosItem = new MosaicSceneItem(image, this);

      connect(mosItem, SIGNAL(changed(const QList<QRectF> &)),
              m_graphicsView, SLOT(updateScene(const QList<QRectF> &)));
      connect(mosItem, SIGNAL(mosaicCubeClosed(Image *)),
              this, SIGNAL(mosCubeClosed(Image *)));

      // We want everything to have a unique Z value so we can manage the z order
      //   well.
      if (m_projectImageZOrders && m_projectImageZOrders->contains(image->id())) {
        double zOrder = m_projectImageZOrders->value(image->id());
        m_projectImageZOrders->remove(image->id());

        foreach (MosaicSceneItem *mosaicItem, *m_mosaicSceneItems) {
          if (mosaicItem->zValue() == zOrder) {
            mosaicItem->setZValue(maximumZ() + 1);
            m_currentMaximumFootprintZ = maximumZ() + 1;
          }
        }

        m_currentMaximumFootprintZ = qMax(zOrder, maximumZ());
        mosItem->setZValue(zOrder);
      }
      else {
        mosItem->setZValue(maximumZ() + 1);
        m_currentMaximumFootprintZ = maximumZ() + 1;
      }

      getScene()->addItem(mosItem);
      m_mosaicSceneItems->append(mosItem);
      m_displayPropsToMosaicSceneItemMap[image->displayProperties()] = mosItem;

      connect(mosItem, SIGNAL(destroyed(QObject *)),
              this, SLOT(removeMosItem(QObject *)));

      ImageDisplayProperties *prop = image->displayProperties();
      connect(prop, SIGNAL(moveDownOne()),
              this, SLOT(moveDownOne()));
      connect(prop, SIGNAL(moveToBottom()),
              this, SLOT(moveToBottom()));
      connect(prop, SIGNAL(moveUpOne()),
              this, SLOT(moveUpOne()));
      connect(prop, SIGNAL(moveToTop()),
              this, SLOT(moveToTop()));
      connect(prop, SIGNAL(zoomFit()),
              this, SLOT(fitInView()));
    }

    return mosItem;
  }


  double MosaicSceneWidget::maximumZ() {
    return m_currentMaximumFootprintZ;
  }


  double MosaicSceneWidget::minimumZ() {
    return m_currentMinimumFootprintZ;
  }

  void MosaicSceneWidget::recalcSceneRect() {
    if (m_projection) {
      double minX, minY, maxX, maxY;
      m_projection->XYRange(minX, maxX, minY, maxY);

      QRectF projRect(minX, -maxY, maxX - minX, maxY - minY);
      QRectF cubesBounding = cubesBoundingRect();

      QRectF bounding = projRect.united(cubesBounding);

      if (m_outlineRect && m_outlineRect->isVisible())
        bounding = bounding.united(m_outlineRect->boundingRect());

      getView()->setSceneRect(bounding);
    }
  }

  void MosaicSceneWidget::addImages(ImageList images) {
    if (m_userToolControl)
      m_progress->setText("Loading primary scene");
    else
      m_progress->setText("Loading secondary scene");

    m_progress->setRange(0, images.size() - 1);
    m_progress->setValue(0);
    m_progress->setVisible(true);

    foreach(Image *image, images) {
      try {
        addImage(image);
      }
      catch (IException &e) {
        e.print();
      }

      m_progress->setValue(m_progress->value() + 1);
    }

    recalcSceneRect();

    if (m_projectViewTransform) {
      PvlObject &positionInfo = *m_projectViewTransform;
      QByteArray hexValues(QString::fromStdString(positionInfo["ViewTransform"][0]).toLatin1());
      QDataStream transformStream(QByteArray::fromHex(hexValues));

      QTransform viewTransform;
      transformStream >> viewTransform;
      getView()->setTransform(viewTransform);

      QPoint projectScrollPos(std::stoi(positionInfo["ScrollPosition"][0]),
                              std::stoi(positionInfo["ScrollPosition"][1]));

      getView()->horizontalScrollBar()->setValue(projectScrollPos.x());
      getView()->verticalScrollBar()->setValue(projectScrollPos.y());
    }
    else {
      refit();
    }

    if (!m_projectImageZOrders || m_projectImageZOrders->isEmpty()) {
      delete m_projectViewTransform;
      m_projectViewTransform = NULL;
    }

    m_progress->setVisible(false);
    emit cubesChanged();
  }


  void MosaicSceneWidget::removeImages(ImageList images) {
    // TODO:  2016-08-02 TLS  Should this be done similarly to addImages. Re-do Image list
    //  then redo scene?
    foreach(Image *image, images) {
      try {
        MosaicSceneItem *item = cubeToMosaic(image);
        item->deleteLater();
        removeMosItem(item);
      }
      catch (IException &e) {
        e.print();
      }
    }
  }


  /**
   * Saves the scene as a png, jpg, or tif file.
   */
  void MosaicSceneWidget::exportView() {
    QString output =
      QFileDialog::getSaveFileName((QWidget *)parent(),
                                   "Choose output file",
                                   QDir::currentPath() + "/untitled.png",
                                   QString("Images (*.png *.jpg *.tif)"));
    if (output.isEmpty()) return;

    // Use png format is the user did not add a suffix to their output filename.
    if (QFileInfo(output).suffix().isEmpty()) {
      output = output + ".png";
    }

    QString format = QFileInfo(output).suffix();
    QPixmap pm = getScene()->views().last()->grab();

    std::string formatString = format.toStdString();
    if (!pm.save(output, formatString.c_str())) {
      QMessageBox::information(this, "Error",
                               "Unable to save [" + output + "]");
    }
  }


  void MosaicSceneWidget::saveList() {
    QString output =
        QFileDialog::getSaveFileName((QWidget *)parent(),
          "Choose output file",
          QDir::currentPath() + "/files.lis",
          QString("List File (*.lis);;Text File (*.txt);;All Files (*.*)"));
    if (output.isEmpty()) return;

    TextFile file(output, "overwrite");

    QList<MosaicSceneItem *> sorted = *m_mosaicSceneItems;
    std::sort(sorted.begin(), sorted.end(), zOrderGreaterThan);

    MosaicSceneItem *sceneItem;
    foreach(sceneItem, sorted) {
      file.PutLine( sceneItem->image()->fileName() );
    }
  }


  void MosaicSceneWidget::removeMosItem(QObject *mosItem) {
    MosaicSceneItem *castedMosItem = (MosaicSceneItem *) mosItem;
    m_mosaicSceneItems->removeAll(castedMosItem);
    m_displayPropsToMosaicSceneItemMap.remove(
        m_displayPropsToMosaicSceneItemMap.key(castedMosItem));
    recalcSceneRect();
    emit cubesChanged();
  }


  /**
   * This method refits t:he items in the graphics view.
   *
   */
  void MosaicSceneWidget::refit() {
    QRectF sceneRect = cubesBoundingRect();

    if (sceneRect.isEmpty())
      return;

    double xPadding = sceneRect.width() * 0.10;
    double yPadding = sceneRect.height() * 0.10;

    sceneRect.adjust(-xPadding, -yPadding, xPadding, yPadding);
    getView()->fitInView(sceneRect, Qt::KeepAspectRatio);
  }


  void MosaicSceneWidget::setCubesSelectable(bool selectable) {
    if (m_cubesSelectable != selectable) {
      m_cubesSelectable = selectable;

      MosaicSceneItem *mosaicSceneItem;
      foreach(mosaicSceneItem, *m_mosaicSceneItems) {
        mosaicSceneItem->scenePropertiesChanged();
      }
    }
  }


  /**
   * This happens when the user clicks on the map action (the button that is named after the
   *   current projection). This method pops up a modal configuration dialog for the map file.
   */
  void MosaicSceneWidget::configProjectionParameters() {
    ProjectionConfigDialog configDialog(this);
    configDialog.exec();
  }


  void MosaicSceneWidget::quickConfigProjectionParameters() {
    ProjectionConfigDialog configDialog(this);
    configDialog.setQuickConfig(true);
    configDialog.exec();
  }


  void MosaicSceneWidget::sendVisibleRectChanged() {
    QPointF topLeft = getView()->mapToScene(0, 0);
    QPointF bottomRight = getView()->mapToScene(
        (int)getView()->width(),
        (int)getView()->height());

    QRectF visibleRect(topLeft, bottomRight);
    emit visibleRectChanged(visibleRect);
  }


  bool MosaicSceneWidget::eventFilter(QObject *obj, QEvent *event) {
    bool stopProcessingEvent = true;

    switch(event->type()) {
      case QMouseEvent::GraphicsSceneMousePress: {
        if (m_customRubberBandEnabled) {
          // Intiate the rubber banding!
          if (!m_customRubberBand) {
            m_customRubberBand = new QRubberBand(QRubberBand::Rectangle,
                                                 getView());
          }

          if (!m_rubberBandOrigin) {
            m_rubberBandOrigin = new QPoint;
          }

          *m_rubberBandOrigin = getView()->mapFromScene(
              ((QGraphicsSceneMouseEvent *)event)->scenePos());
          m_customRubberBand->setGeometry(QRect(*m_rubberBandOrigin, QSize()));
          m_customRubberBand->show();
        }

        emit mouseButtonPress(
              ((QGraphicsSceneMouseEvent *)event)->scenePos(),
              ((QGraphicsSceneMouseEvent *)event)->button());
        stopProcessingEvent = false;
        break;
      }

      case QMouseEvent::GraphicsSceneMouseRelease: {
        bool signalEmitted = false;
        if (m_customRubberBandEnabled && m_rubberBandOrigin &&
           m_customRubberBand) {
          if (m_customRubberBand->geometry().width() +
             m_customRubberBand->geometry().height() > 10) {
            emit rubberBandComplete(
                getView()->mapToScene(
                    m_customRubberBand->geometry()).boundingRect(),
                ((QGraphicsSceneMouseEvent *)event)->button());
            signalEmitted = true;
          }

          delete m_rubberBandOrigin;
          m_rubberBandOrigin = NULL;

          delete m_customRubberBand;
          m_customRubberBand = NULL;
        }

        if (!signalEmitted) {
          stopProcessingEvent = false;
          emit mouseButtonRelease(
                ((QGraphicsSceneMouseEvent *)event)->scenePos(),
                ((QGraphicsSceneMouseEvent *)event)->button());
        }
        break;
      }

      case QMouseEvent::GraphicsSceneMouseDoubleClick:
        emit mouseDoubleClick(
              ((QGraphicsSceneMouseEvent *)event)->scenePos());
        stopProcessingEvent = false;
        break;

      case QMouseEvent::GraphicsSceneMouseMove:
        if (m_customRubberBandEnabled && m_rubberBandOrigin &&
           m_customRubberBand) {
          QPointF scenePos = ((QGraphicsSceneMouseEvent *)event)->scenePos();
          QPoint screenPos = getView()->mapFromScene(scenePos);

          QRect rubberBandRect =
              QRect(*m_rubberBandOrigin, screenPos).normalized();

          m_customRubberBand->setGeometry(rubberBandRect);
        }
        else {
          stopProcessingEvent = false;
        }

        emit mouseMove(
              ((QGraphicsSceneMouseEvent *)event)->scenePos());
        break;

      case QEvent::GraphicsSceneWheel:
        emit mouseWheel(
              ((QGraphicsSceneWheelEvent *)event)->scenePos(),
              ((QGraphicsSceneWheelEvent *)event)->delta());
        event->accept();
        stopProcessingEvent = true;
        break;

      case QMouseEvent::Enter:
        emit mouseEnter();
        stopProcessingEvent = false;
        break;

      case QMouseEvent::Leave:
        emit mouseLeave();
        stopProcessingEvent = false;
        break;

      case QEvent::GraphicsSceneHelp: {
        setToolTip("");
        bool toolTipFound = false;

        QGraphicsItem *sceneItem;
        foreach(sceneItem, getScene()->items()) {
          if (!toolTipFound) {
            if (sceneItem->contains(
              ((QGraphicsSceneHelpEvent*)event)->scenePos()) &&
              sceneItem->toolTip().size() > 0) {
              setToolTip(sceneItem->toolTip());
              toolTipFound = true;
            }
          }
        }

        if (toolTipFound) {
          stopProcessingEvent = true;
          QToolTip::showText(
              ((QGraphicsSceneHelpEvent*)event)->screenPos(),
              toolTip());
        }
        break;
      }

      default:
        stopProcessingEvent = false;
        break;
    }

    return stopProcessingEvent;
  }


  /**
   * Reprojects all the items in the view.
   * Also makes sure to resize the view rectangle to fit the newly
   * projected footprints.
   *
   */
  void MosaicSceneWidget::reprojectItems() {
    if (m_mosaicSceneItems->size() == 0)
      return;

    if (m_userToolControl)
      m_progress->setText("Reprojecting primary scene");
    else
      m_progress->setText("Reprojecting secondary scene");

    // This gives some pretty graphics as thing work
    int reprojectsPerUpdate = qMax(1, m_mosaicSceneItems->size() / 20);

    m_progress->setRange(0,
        (m_mosaicSceneItems->size() - 1) / reprojectsPerUpdate + 1);
    m_progress->setValue(0);
    m_progress->setVisible(true);

    MosaicSceneItem *mosaicSceneItem;

    int progressCountdown = reprojectsPerUpdate;
    foreach(mosaicSceneItem, *m_mosaicSceneItems) {
      try {
        mosaicSceneItem->reproject();
      }
      catch (IException &e) {
        std::string msg = "The file [";

        if (mosaicSceneItem->image())
          msg += mosaicSceneItem->image()->displayProperties()->displayName();

        msg += "] is being removed due to not being able to project onto the scene";

        IException tmp(e, IException::Programmer, msg, _FILEINFO_);
        tmp.print();
        mosaicSceneItem->image()->deleteLater();
      }

      progressCountdown --;
      if (progressCountdown == 0) {
        m_progress->setValue(m_progress->value() + 1);
        progressCountdown = reprojectsPerUpdate;
        refit();
      }
    }

    m_progress->setValue(m_progress->maximum());

    recalcSceneRect();
    refit();
    m_progress->setVisible(false);
  }


  void MosaicSceneWidget::moveDownOne() {
    DisplayProperties *props = qobject_cast<DisplayProperties *>(sender());

    if (props) {
      moveDownOne(cubeToMosaic(props));
    }
  }


  /**
   */
  double MosaicSceneWidget::moveDownOne(MosaicSceneItem *item) {
    MosaicSceneItem *nextDown = getNextItem(item, false);
    double originalZ = item->zValue();

    if (nextDown) {
      double newZValue = nextDown->zValue() - 1;
      moveZ(item, newZValue, true);
    }

    return originalZ;
  }


  /**
   */
  double MosaicSceneWidget::moveDownOne(Image *image) {
    return moveDownOne(cubeToMosaic(image));
  }


  /**
   */
  QList<double> MosaicSceneWidget::moveDownOne(ImageList *images) {
    QList<double> results;

    foreach (Image *image, *images) {
      results.append(moveDownOne(image));
    }

    return results;
  }


  void MosaicSceneWidget::moveToBottom() {
    DisplayProperties *props = qobject_cast<DisplayProperties *>(sender());
    if (props) {
      moveToBottom(cubeToMosaic(props));
    }
  }


  /**
   * This doesn't compress the Z values - the original Z values of this scene item is guaranteed to
   *   be empty after this operation.
   */
  double MosaicSceneWidget::moveToBottom(MosaicSceneItem *item) {
    double originalZ = item->zValue();
    double minZ = minimumZ();

    if (originalZ != minZ) {
      // We know min-1 isn't already used
      int newZValue = qRound(minZ - 1);
      item->setZValue(newZValue);

      // Remove this if we enable the compress
      m_currentMinimumFootprintZ--;
    }

    return originalZ;
  }


  /**
   * This doesn't compress the Z values - the original Z value of this image is guaranteed to be
   *   empty after this operation.
   */
  double MosaicSceneWidget::moveToBottom(Image *image) {
    return moveToBottom(cubeToMosaic(image));
  }


  /**
   * This doesn't compress the Z values - the original Z values of these images are guaranteed to be
   *   empty after this operation.
   */
  QList<double> MosaicSceneWidget::moveToBottom(ImageList *images) {
    QList<double> results;

    foreach (Image *image, *images) {
      results.append(moveToBottom(image));
    }

    return results;
  }


  void MosaicSceneWidget::moveUpOne() {
    DisplayProperties *props = qobject_cast<DisplayProperties *>(sender());

    if (props) {
      moveUpOne(cubeToMosaic(props));
    }
  }


  /**
   */
  double MosaicSceneWidget::moveUpOne(MosaicSceneItem *item) {
    MosaicSceneItem *nextUp = getNextItem(item, true);
    double originalZ = item->zValue();

    if (nextUp) {
      double newZValue = nextUp->zValue() + 1;
      moveZ(item, newZValue, true);
    }

    return originalZ;
  }


  /**
   */
  double MosaicSceneWidget::moveUpOne(Image *image) {
    return moveUpOne(cubeToMosaic(image));
  }


  /**
   */
  QList<double> MosaicSceneWidget::moveUpOne(ImageList *images) {
    QList<double> results;

    foreach (Image *image, *images) {
      results.append(moveUpOne(image));
    }

    return results;
  }


  void MosaicSceneWidget::moveToTop() {
    DisplayProperties *props = qobject_cast<DisplayProperties *>(sender());

    if (props) {
      moveToTop(cubeToMosaic(props));
    }
  }


  /**
   * This doesn't compress the Z values - the original Z values of this scene item is guaranteed to
   *   be empty after this operation.
   */
  double MosaicSceneWidget::moveToTop(MosaicSceneItem *item) {
    double originalZ = item->zValue();
    double maxZ = maximumZ();

    if (originalZ != maxZ) {
      // We know max+1 isn't already used
      int newZValue = qRound(maxZ + 1);
      item->setZValue(newZValue);

      // Remove this if we enable the compress
      m_currentMaximumFootprintZ++;
    }

    // Compress... this makes this method have a time complexity of N instead of constant; there
    //   isn't really a good justification for the slow down. I'm leaving this (working) code here
    //   for reference and in case it's needed later.
    // foreach (MosaicSceneItem *otherItem, *m_mosaicSceneItems) {
    //   double otherItemZ = otherItem->zValue();
    //   if (otherItemZ > originalZ) {
    //     otherItem->setZValue(otherItemZ - 1.0);
    //   }
    // }

    return originalZ;
  }


  /**
   * This doesn't compress the Z values - the original Z value of this image is guaranteed to be
   *   empty after this operation.
   */
  double MosaicSceneWidget::moveToTop(Image *image) {
    return moveToTop(cubeToMosaic(image));
  }


  /**
   * This doesn't compress the Z values - the original Z values of these images are guaranteed to be
   *   empty after this operation.
   */
  QList<double> MosaicSceneWidget::moveToTop(ImageList *images) {
    QList<double> results;

    foreach (Image *image, *images) {
      results.append(moveToTop(image));
    }


// printZ(m_mosaicSceneItems);
    return results;
  }


  /**
   * This method moves the given scene item to the given Z value. By default, this does not create
   *   gaps in the Z-values and makes the necessary room/adjustments for the new item to go to it's
   *   spot. If the last operation was moveToTop or moveToBottom, because those methods don't
   *   compress/adjust the surrounding Z values, we can avoid any and all adjustments here
   *   (significant performance boost) by using newZValueMightExist=false.
   *
   * N = # items in the scene
   * The time complexity of this method is N if newZValueMightExist.
   * The time complexity of this method is constant if !newZValueMightExist.
   *
   * @param sceneItem The item in this scene to change the Z value
   * @param newZ The new Z value for the item
   * @param newZValueMightExist True if an item in the scene might occupy the new Z value
   */
  double MosaicSceneWidget::moveZ(MosaicSceneItem *sceneItem, double newZ,
                                  bool newZValueMightExist) {
    double originalZ = sceneItem->zValue();

    if (newZValueMightExist) {
      // Adjust items between original and new position, recalculate min/max - time complexity=N
      m_currentMinimumFootprintZ = 0.0;
      m_currentMaximumFootprintZ = 0.0;

      foreach (MosaicSceneItem *otherItem, *m_mosaicSceneItems) {
        double otherItemOrigZ = otherItem->zValue();
        double otherItemNewZ = otherItemOrigZ;

        // Moving downwards (new Z is lower than current Z) and item is in the middle
        if (originalZ > newZ && otherItemOrigZ >= newZ && otherItemOrigZ < originalZ) {
          otherItemNewZ = otherItemOrigZ + 1;
        }
        // Moving upwards (new Z is higher than current Z) and item is in the middle
        else if (originalZ < newZ && otherItemOrigZ <= newZ && otherItemOrigZ > originalZ) {
          otherItemNewZ = otherItemOrigZ - 1;
        }

        m_currentMinimumFootprintZ = qMin(m_currentMinimumFootprintZ, otherItemNewZ);
        m_currentMaximumFootprintZ = qMax(m_currentMaximumFootprintZ, otherItemNewZ);
        otherItem->setZValue(otherItemNewZ);
      }
    }

    sceneItem->setZValue(newZ);

    if (!newZValueMightExist) {
      // If we moved the max or min item, adjust the max down or min up respectively
      if (originalZ == maximumZ() && newZ < originalZ) {
        m_currentMaximumFootprintZ--;
      }
      else if (originalZ == minimumZ() && newZ > originalZ) {
        m_currentMinimumFootprintZ++;
      }
    }

    return originalZ;
  }


  double MosaicSceneWidget::moveZ(Image *image, double newZ,
                                  bool newZValueMightExist) {
    return moveZ(cubeToMosaic(image), newZ, newZValueMightExist);
  }


  void MosaicSceneWidget::fitInView() {
    if (m_userToolControl) {
      QRectF boundingBox;

      DisplayProperties *props = qobject_cast<DisplayProperties *>(sender());

      if (props) {
        MosaicSceneItem *item = cubeToMosaic(props);
        boundingBox = item->boundingRect();
      }
      else {
        QAction *action = qobject_cast<QAction *>(sender());

        if (action) {
          ImageList *images = action->data().value<ImageList *>();

          foreach (Image *image, *images) {
            boundingBox = boundingBox.united(cubeToMosaic(image)->boundingRect());
          }
        }
      }

      if (!boundingBox.isNull()) {
        double xPadding = boundingBox.width() * 0.10;
        double yPadding = boundingBox.height() * 0.10;

        boundingBox.setLeft(boundingBox.left() - xPadding);
        boundingBox.setRight(boundingBox.right() + xPadding);

        boundingBox.setTop(boundingBox.top() - yPadding);
        boundingBox.setBottom(boundingBox.bottom() + yPadding);

        getView()->fitInView(boundingBox, Qt::KeepAspectRatio);
        getView()->centerOn(boundingBox.center());
      }
    }
  }


  void MosaicSceneWidget::onSelectionChanged() {
    if (!m_blockingSelectionChanged) {
      if (!m_queuedSelectionChanged) {
        emit queueSelectionChanged();
        m_queuedSelectionChanged = true;
      }
      else {
        m_shouldRequeueSelectionChanged = true;
      }
    }
  }


  void MosaicSceneWidget::onQueuedSelectionChanged() {
    m_queuedSelectionChanged = false;

    if (m_shouldRequeueSelectionChanged) {
      m_shouldRequeueSelectionChanged = false;
      onSelectionChanged();
    }
    else {
      foreach(MosaicSceneItem *mosaicSceneItem, *m_mosaicSceneItems) {
        mosaicSceneItem->updateSelection(true);
      }
    }
  }


  //! Implemented because we want invisible items too
  MosaicSceneItem *MosaicSceneWidget::getNextItem(MosaicSceneItem *item, bool up) {
    MosaicSceneItem *nextZValueItem = NULL;
    MosaicSceneItem *mosaicSceneItem;

    foreach(mosaicSceneItem, *m_mosaicSceneItems) {
      if (mosaicSceneItem != item &&
          mosaicSceneItem->boundingRect().intersects(item->boundingRect())) {
        // Does this item qualify as above or below at all?
        if ( (up  && mosaicSceneItem->zValue() > item->zValue()) ||
            (!up && mosaicSceneItem->zValue() < item->zValue())) {
          // It is in the correct direction, set the initial guess if we don't
          //   have one or test if it's better
          if (!nextZValueItem) {
            nextZValueItem = mosaicSceneItem;
          }
          else {
            // We know it qualifies, we want to know if it's closer than
            //   nextZValueItem
            if ((up && mosaicSceneItem->zValue() < nextZValueItem->zValue()) ||
                (!up && mosaicSceneItem->zValue() > nextZValueItem->zValue())) {
              nextZValueItem = mosaicSceneItem;
            }
          }
        }
      }
    }

    return nextZValueItem;
  }


  bool MosaicSceneWidget::zOrderGreaterThan(MosaicSceneItem *first,
                                            MosaicSceneItem *second) {
    return first->zValue() > second->zValue();
  }
}
