#include "MosaicController.h"

#include <QAction>
#include <QApplication>
#include <QBuffer>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QSettings>
#include <QUuid>
#include <QtConcurrentMap>

#include "ControlNet.h"
#include "Cube.h"
#include "DisplayProperties.h"
#include "FileName.h"
#include "IException.h"
#include "ImageFileListWidget.h"
#include "ImageReader.h"
#include "MosaicSceneWidget.h"
#include "ProgressBar.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "TextFile.h"


namespace Isis {

  /**
   * MosaicWidget constructor.
   * MosaicWidget is derived from QSplitter, the left side of the
   * splitter is a QTreeWidget and the right side of the splitter
   * is a QGraphicsView.
   *
   *
   * @param parent
   */
  MosaicController::MosaicController(QStatusBar *status, QSettings &settings) {
    m_fileList = NULL;
    m_scene = NULL;
    m_worldScene = NULL;
    m_imageReader = NULL;

    m_mutex = new QMutex;

    m_imageReader = new ImageReader(m_mutex);

    connect(m_imageReader, SIGNAL(imagesReady(ImageList)), this, SLOT(imagesReady(ImageList)));

    m_fileList = new ImageFileListWidget;
    m_scene = new MosaicSceneWidget(status, true, false, NULL);
    m_worldScene = new MosaicSceneWidget(status, false, false, NULL);

    connect(this, SIGNAL(imagesAdded(ImageList)), m_scene, SLOT(addImages(ImageList)));
    connect(this, SIGNAL(imagesAdded(ImageList)), m_worldScene, SLOT(addImages(ImageList)));
    connect(this, SIGNAL(imagesAdded(ImageList *)), m_fileList, SLOT(addImages(ImageList *)));

    connect(m_scene, SIGNAL(projectionChanged(Projection *)),
            m_worldScene, SLOT(setProjection(Projection *)));
    connect(m_scene, SIGNAL(visibleRectChanged(QRectF)),
            m_worldScene, SLOT(setOutlineRect(QRectF)));

    settings.beginGroup("MosaicController");
    m_maxThreads = settings.value("maxThreads", 0).toInt();
    settings.endGroup();

    applyMaxThreadCount();
  }


  /**
   * Free the allocated memory by this object
   */
  MosaicController::~MosaicController() {
    delete m_fileList;
    delete m_scene;
    delete m_worldScene;
    delete m_imageReader;
  }


  /**
   * Add actions that are export-related to the menu
   */
  void MosaicController::addExportActions(QMenu &fileMenu) {
    QList<QAction *> exportActions = m_scene->getExportActions();

    foreach (QAction * exportAct, exportActions) {
      fileMenu.addAction(exportAct);
    }

    exportActions = m_fileList->getExportActions();

    foreach (QAction * exportAct, exportActions) {
      fileMenu.addAction(exportAct);
    }
  }


  /**
   * 
   */
  QProgressBar *MosaicController::getProgress() {
    return m_imageReader->progress();
  }


  void MosaicController::saveProject(QString projFileName) {
    Pvl projFile;

    PvlObject imageProps("Images");

    foreach (Image *image, m_images) {
      imageProps += image->toPvl();
    }

    projFile += imageProps;
    projFile += m_fileList->toPvl();
    projFile += m_scene->toPvl();

    projFile.write(projFileName.toStdString());
  }


  QList<QAction *> MosaicController::getSettingsActions() {
    QList<QAction *> settingsActs;

    settingsActs.append(m_imageReader->actions(ImageDisplayProperties::FootprintViewProperties));
    settingsActs.append(m_fileList->actions());

    QAction *setThreads = new QAction("Set &Thread Limit", this);
    connect(setThreads, SIGNAL(triggered(bool)),
            this, SLOT(changeMaxThreads()));
    settingsActs.append(setThreads);

    return settingsActs;
  }


  void MosaicController::saveSettings(QSettings &settings) {
    settings.beginGroup("MosaicController");
    settings.setValue("maxThreads", m_maxThreads);
    settings.endGroup();
  }


  /**
   * Handle opening cubes by filename. This class constructs and owns the
   *   actual Cube objects.
   */
  void MosaicController::openImages(QStringList cubeNames) {
    m_imageReader->read(cubeNames);
  }


  void MosaicController::openProjectImages(PvlObject projectImages) {
    m_imageReader->read(projectImages);
  }


  void MosaicController::imagesReady(ImageList images) {
    m_images.append(images);

    foreach (Image *image, images) {
      connect(image, SIGNAL(destroyed(QObject *)),
              this, SLOT(imageClosed(QObject *)));
    }

    // We really can't have all of the cubes in memory before
    //   the OS stops letting us open more files.
    // Assume cameras are being used in other parts of code since it's
    //   unknown
    QMutexLocker lock(m_mutex);
    emit imagesAdded(images);
    emit imagesAdded(&images);

    Image *openImage;
    foreach (openImage, images) {
      openImage->closeCube();
    }
  }


  void MosaicController::changeMaxThreads() {
    bool ok = false;

    QStringList options;

    int current = 0;
    options << tr("Use all available");

    for(int i = 1; i < 24; i++) {
      QString option = tr("Use %1 threads").arg(i + 1);

      options << option;
      if(m_maxThreads == i + 1)
        current = i;
    }

    QString res = QInputDialog::getItem(NULL, tr("Concurrency"),
        tr("Set the number of threads to use"),
        options, current, false, &ok);

    if (ok) {
      m_maxThreads = options.indexOf(res) + 1;

      applyMaxThreadCount();
    }
  }


  /**
   * An open image is being deleted
   */
  void MosaicController::imageClosed(QObject * imageObj) {
    Image *image = (Image *)imageObj;

    if (image) {
      ImageList::iterator foundElement;
      foundElement = std::find(m_images.begin(), m_images.end(), image);
      m_images.erase(foundElement);

      if(m_images.empty())
        emit allImagesClosed();
    }
  }


  void MosaicController::readProject(QString filename) {
    try {
      Pvl projectPvl(filename.toStdString());

      // Convert versions <= isis3.4.1 to newer
      if (projectPvl.hasObject("Cubes")) {
        convertV1ToV2(projectPvl);
      }

      PvlObject &projImages(projectPvl.findObject("Images"));

      if (projectPvl.hasObject("MosaicScene"))
        m_scene->fromPvl(projectPvl.findObject("MosaicScene"));

      if (projectPvl.hasObject("ImageFileList"))
        m_fileList->fromPvl(projectPvl.findObject("ImageFileList"));

      openProjectImages(projImages);
    }
    catch(IException &e) {
      throw IException(e, IException::Unknown,
                       "Input file is not a valid qmos project", _FILEINFO_);
    }
  }


  void MosaicController::saveList() {
    QString output =
        QFileDialog::getSaveFileName((QWidget *)parent(),
          "Choose output file",
          QDir::currentPath() + "/files.lis",
          QString("List File (*.lis);;Text File (*.txt);;All Files (*.*)"));
    if(output.isEmpty()) return;

    TextFile file(output, "output");

    Image *image;
    foreach (image, m_images) {
      file.PutLine( image->fileName() );
    }
  }


  void MosaicController::applyMaxThreadCount() {
    if (m_maxThreads <= 1) {
      QThreadPool::globalInstance()->setMaxThreadCount(QThread::idealThreadCount());
    }
    else {
      QThreadPool::globalInstance()->setMaxThreadCount(m_maxThreads - 1);
    }
  }


  /**
   * This converts qmos project files (no other project files existed) from their original version
   *   (file names everywhere, CubeDisplayProperties, non-OR'able display properties) to the
   *   second major version. Cubes are now Images, Display properties' indices have changed,
   *   Image ID's instead of file names in many places.
   *
   * This is only intended to get the project to the major V2 version... minor version adjustments
   *   ought to be handled internally by each object.
   */
  void MosaicController::convertV1ToV2(Pvl &project) {
    PvlObject &images = project.findObject("Cubes");

    images.setName("Images");

    QMap<QString, QString> imageFileToNewId;

    for (int imgObjIndex = 0; imgObjIndex < images.objects(); imgObjIndex++) {
      PvlObject &image = images.object(imgObjIndex);
      image.setName("Image");


      QBuffer idDataBuffer;
      idDataBuffer.open(QIODevice::ReadWrite);

      QDataStream idStream(&idDataBuffer);

      QUuid newId = QUuid::createUuid();
      QString fileName = QString::fromStdString(image["FileName"][0]);
      idStream << newId;

      idDataBuffer.seek(0);

      QString idHex;
      image += PvlKeyword("ID", std::string(idDataBuffer.data().toHex()));

      PvlKeyword oldDisplayPropsValues = image["Values"];
      image.deleteKeyword("Values");

      PvlObject displayProps("DisplayProperties");
      displayProps += PvlKeyword("DisplayName", FileName(fileName).name().toStdString());

      // Convert display properties over
      enum OldDispProps {
        OldDispPropColor,
        OldDispPropUntransferred1, // Selected
        OldDispPropShowDNs,
        OldDispPropShowFill,
        OldDispPropShowLabel,
        OldDispPropShowOutline,
        OldDispPropUntransferred2, // Zooming
        OldDispPropUntransferred3  // ZOrdering
      };

      QMap<int, QVariant> oldProps;
      QByteArray oldHexValues(QString::fromStdString(oldDisplayPropsValues[0]).toLatin1());
      QDataStream oldValuesStream(QByteArray::fromHex(oldHexValues));
      oldValuesStream >> oldProps;

      enum V2DispProps {
        V2DispPropColor = 1,
        V2DispPropShowDNs = 4,
        V2DispPropShowFill = 8,
        V2DispPropShowLabel = 16,
        V2DispPropShowOutline = 32
      };

      QMap<int, QVariant> newProps;
      newProps[V2DispPropColor] = oldProps[OldDispPropColor];
      newProps[V2DispPropShowDNs] = oldProps[OldDispPropShowDNs];
      newProps[V2DispPropShowFill] = oldProps[OldDispPropShowFill];
      newProps[V2DispPropShowLabel] = oldProps[OldDispPropShowLabel];
      newProps[V2DispPropShowOutline] = oldProps[OldDispPropShowOutline];

      QBuffer newPropsDataBuffer;
      newPropsDataBuffer.open(QIODevice::ReadWrite);

      QDataStream newPropsStream(&newPropsDataBuffer);
      newPropsStream << newProps;
      newPropsDataBuffer.seek(0);

      displayProps += PvlKeyword("Values", newPropsDataBuffer.data().toHex().data());
      // Finished converting display properties from V1->V2


      image += displayProps;

      imageFileToNewId[fileName] = newId.toString().replace(QRegExp("[{}]"), "");
    }

    PvlObject &fileListOpts = project.findObject("MosaicFileList");
    fileListOpts.setName("ImageFileList");

    for (int fileListIndex = 0; fileListIndex < fileListOpts.objects(); fileListIndex++) {
      PvlObject &fileListOrderObj = fileListOpts.object(fileListIndex);

      for (int i = 0; i < fileListOrderObj.keywords(); i++) {
        PvlKeyword &key = fileListOrderObj[i];

        if (key.isNamed("Cube")) {
          key.setName("Image");
          key[0] = imageFileToNewId[QString::fromStdString(key[0])].toStdString();
        }
      }
    }

    PvlObject &sceneOpts = project.findObject("MosaicScene");

    if (sceneOpts.hasObject("ZOrdering")) {
      PvlObject &zOrdering = sceneOpts.findObject("ZOrdering");


      for (int i = 0; i < zOrdering.keywords(); i++) {
        PvlKeyword &key = zOrdering[i];

        if (key.isNamed("ZValue")) {
          key[0] = imageFileToNewId[QString::fromStdString(key[0])].toStdString();
        }
      }
    }
  }
}
