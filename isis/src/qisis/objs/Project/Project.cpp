/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include "Project.h"

#include <unistd.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QFuture>
#include <QFutureWatcher>
#include <QMessageBox>
#include <QMutex>
#include <QMutexLocker>
#include <QProgressBar>
#include <QRegExp>
#include <QSettings>
#include <QStringList>
#include <QtDebug>
#include <QTextStream>
#include <QWidget>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>

#include "BundleSettings.h"
#include "BundleSolutionInfo.h"
#include "Camera.h"
#include "Control.h"
#include "ControlList.h"
#include "ControlNet.h"
#include "CorrelationMatrix.h"
#include "Cube.h"
#include "Directory.h"
#include "Environment.h"
#include "FileName.h"
#include "GuiCamera.h"
#include "GuiCameraList.h"
#include "ImageList.h"
#include "ImageReader.h"
#include "IException.h"
#include "ProgressBar.h"
#include "ProjectItem.h"
#include "ProjectItemModel.h"
#include "SerialNumberList.h"
#include "SetActiveControlWorkOrder.h"
#include "SetActiveImageListWorkOrder.h"
#include "Shape.h"
#include "ShapeList.h"
#include "ShapeReader.h"
#include "Target.h"
#include "TargetBodyList.h"
#include "Template.h"
#include "TemplateList.h"
#include "WorkOrder.h"
#include "WorkOrderFactory.h"

namespace Isis {

  /**
   * Create a new Project. This creates a project on disk at /tmp/username_appname_pid.
   */
  Project::Project(Directory &directory, QObject *parent) :
      QObject(parent) {
    m_bundleSettings = NULL;
    m_clearing = false;
    m_directory = &directory;
    m_projectRoot = NULL;
    m_cnetRoot = NULL;
    m_idToControlMap = NULL;
    m_idToImageMap = NULL;
    m_idToShapeMap = NULL;
    m_idToTargetBodyMap = NULL;
    m_idToGuiCameraMap = NULL;
    m_images = NULL;
    m_imageReader = NULL;
    m_shapeReader = NULL;
    m_shapes = NULL;
    m_mapTemplates = NULL;
    m_regTemplates = NULL;
    m_warnings = NULL;
    m_workOrderHistory = NULL;
    m_isTemporaryProject = true;
    m_isOpen = false;
    m_isClean = true;
    m_activeControl = NULL;
    m_activeImageList = NULL;

    m_numImagesCurrentlyReading = 0;

    m_mutex = NULL;
    m_workOrderMutex = NULL;
    m_imageReadingMutex = NULL;

    m_numShapesCurrentlyReading = 0;
    m_shapeMutex = NULL;
    m_shapeReadingMutex = NULL;

    m_idToControlMap = new QMap<QString, Control *>;
    m_idToImageMap = new QMap<QString, Image *>;
    m_idToShapeMap = new QMap<QString, Shape *>;
    m_idToTargetBodyMap = new QMap<QString, TargetBody *>;
    m_idToGuiCameraMap = new QMap<QString, GuiCamera *>;
    m_idToBundleSolutionInfoMap = new QMap<QString, BundleSolutionInfo *>;

    m_name = "Project";

    // Look for old projects
    QDir tempDir = QDir::temp();
    QStringList nameFilters;
    nameFilters.append(Environment::userName() + "_" +
                       QApplication::applicationName() + "_*");
    tempDir.setNameFilters(nameFilters);

    QStringList existingProjects = tempDir.entryList();
    bool crashedPreviously = false;

    foreach (QString existingProject, existingProjects) {
      FileName existingProjectFileName(tempDir.absolutePath() + "/" + existingProject);
      QString pidString = QString(existingProject).replace(QRegExp(".*_"), "");
      int otherPid = pidString.toInt();

      if (otherPid != 0) {
        if ( !QFile::exists("/proc/" + pidString) ) {
          crashedPreviously = true;
          int status = system( ("rm -rf '" +
                                existingProjectFileName.expanded() + "' &").toLatin1().data() );
          if (status != 0) {
            std::string msg = "Executing command [rm -rf" + existingProjectFileName.expanded() +
                          "' &] failed with return status [" + toString(status) + "]";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        }
      }
    }

    if (crashedPreviously && false) {
      QMessageBox::information( NULL,
                                QObject::tr("Crashed"),
                                QObject::tr("It appears %1 crashed. We're sorry.").
                                         arg( QApplication::applicationName() ) );
    }

    QCoreApplication* ipce_app = static_cast<QCoreApplication *>(directory.parent());

    try {
      QString tmpFolder = QDir::temp().absolutePath() + "/"
            + Environment::userName() + "_"
            + QApplication::applicationName() + "_" + QString::number( getpid() );
      QDir temp(tmpFolder + "/tmpProject");
      m_projectRoot = new QDir(temp);

      if (ipce_app->arguments().count() == 1) {
        createFolders();
      }
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer, "Error creating project folders.", _FILEINFO_);
    }
    catch (std::exception &e) {
      //  e.what()
      throw IException(IException::Programmer,
          tr("Error creating project folders [%1]").arg( e.what() ), _FILEINFO_);
    }
    //  TODO TLS 2016-07-13  This seems to only be used by ControlNet when SetTarget is called.
    //     This needs to be better documented, possibly renamed or redesigned??
    m_mutex = new QMutex;
    m_workOrderMutex = new QMutex;
    // image reader
    m_imageReader = new ImageReader(m_mutex, true);

    connect( m_imageReader, SIGNAL( imagesReady(ImageList) ),
             this, SLOT( imagesReady(ImageList) ) );

    // Project will be listening for when both cnets and images have been added.
    // It will emit a signal, controlsAndImagesAvailable, when this occurs.
    // Directory sets up a listener on the JigsawWorkOrder clone to enable itself
    // when it hears this signal.
    connect(this, SIGNAL(imagesAdded(ImageList *)),
            this, SLOT(checkControlsAndImagesAvailable()));
    connect(this, SIGNAL(controlListAdded(ControlList *)),
            this, SLOT(checkControlsAndImagesAvailable()));
    connect(m_directory, SIGNAL(cleanProject(bool)),
            this, SLOT(setClean(bool)));

    m_images = new QList<ImageList *>;

    // Shape reader
    m_shapeMutex = new QMutex;

    m_shapeReader = new ShapeReader(m_shapeMutex, false);

    connect( m_shapeReader, SIGNAL( shapesReady(ShapeList) ),
             this, SLOT( shapesReady(ShapeList) ) );

    m_shapes = new QList<ShapeList *>;

    m_controls = new QList<ControlList *>;

    m_targets = new TargetBodyList;

    m_mapTemplates = new QList<TemplateList *>;

    m_regTemplates = new QList<TemplateList *>;

    m_guiCameras = new GuiCameraList;

    m_bundleSolutionInfo = new QList<BundleSolutionInfo *>;

    m_warnings = new QStringList;
    m_workOrderHistory = new QList< QPointer<WorkOrder> >;

    m_imageReadingMutex = new QMutex;

    m_shapeReadingMutex = new QMutex;

    // Listen for when an active control is set and when an active image list is set.
    // This is used for enabling the JigsawWorkOrder (the Bundle Adjustment menu action).
//  connect(this, &Project::activeControlSet,
//          this, &Project::checkActiveControlAndImageList);
//  connect(this, &Project::activeImageListSet,
//          this, &Project::checkActiveControlAndImageList);
    // TODO: ken testing
//    m_bundleSettings = NULL;
//    m_bundleSettings = new BundleSettings();
  }


  /**
   * Clean up the project. This will bring the Project back to a safe on-disk state.
   */
  Project::~Project() {


    if (m_images) {
      foreach (ImageList *imageList, *m_images) {
        foreach (Image *image, *imageList) {
          delete image;
        }

        delete imageList;
      }

      delete m_images;
      m_images = NULL;
    }


    if (m_shapes) {
      foreach (ShapeList *shapeList, *m_shapes) {
        foreach (Shape *shape, *shapeList) {
          delete shape;
        }

        delete shapeList;
      }

      delete m_shapes;
      m_shapes = NULL;
    }


    if (m_controls) {
      foreach (ControlList *controlList, *m_controls) {
        foreach (Control *control, *controlList) {
          delete control;
        }

        delete controlList;
      }
      delete m_controls;
      m_controls = NULL;
    }


    if (m_mapTemplates) {
      foreach (TemplateList *templateList, *m_mapTemplates) {
        foreach (Template *templateFile, *templateList) {
          delete templateFile;
        }

        delete templateList;
      }

      delete m_mapTemplates;
      m_mapTemplates = NULL;
    }


    if (m_regTemplates) {
      foreach (TemplateList *templateList, *m_regTemplates) {
        foreach (Template *templateFile, *templateList) {
          delete templateFile;
        }

        delete templateList;
      }

      delete m_regTemplates;
      m_regTemplates = NULL;
    }


    m_activeControl = NULL;
    m_activeImageList = NULL;

    if (m_bundleSolutionInfo) {
      foreach (BundleSolutionInfo *bundleSolutionInfo, *m_bundleSolutionInfo) {
        delete bundleSolutionInfo;
      }

      delete m_bundleSolutionInfo;
      m_bundleSolutionInfo = NULL;
    }

    delete m_idToControlMap;
    m_idToControlMap = NULL;

    delete m_idToImageMap;
    m_idToImageMap = NULL;

    delete m_idToShapeMap;
    m_idToShapeMap = NULL;

    delete m_idToTargetBodyMap;
    m_idToTargetBodyMap = NULL;

    delete m_idToGuiCameraMap;
    m_idToGuiCameraMap = NULL;

    delete m_idToBundleSolutionInfoMap;
    m_idToBundleSolutionInfoMap = NULL;

    m_directory = NULL;

    delete m_projectRoot;
    m_projectRoot = NULL;

    delete m_cnetRoot;
    m_cnetRoot = NULL;

    delete m_imageReader;
    delete m_shapeReader;

    delete m_warnings;
    m_warnings = NULL;

    m_workOrderHistory->removeAll(NULL);
    m_workOrderHistory = NULL;

    delete m_bundleSettings;
    m_bundleSettings = NULL;
  }


  /**
   * This creates the project root, image root, and control net root directories.
   */
  void Project::createFolders() {
    QDir dir;
    if ( !dir.mkpath( m_projectRoot->path() ) ) {
      warn("Cannot create project directory.");
      throw IException(IException::Io,
                       tr("Unable to create folder [%1] when trying to initialize project")
                         .arg(m_projectRoot->path() ),
                       _FILEINFO_);
    }

    try {
      if ( !dir.mkdir( cnetRoot() ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( cnetRoot() );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      if ( !dir.mkdir( imageDataRoot() ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( imageDataRoot() );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      if ( !dir.mkdir( shapeDataRoot() ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( shapeDataRoot() );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      if ( !dir.mkdir( resultsRoot() ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( resultsRoot() );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      if ( !dir.mkdir( bundleSolutionInfoRoot() ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( bundleSolutionInfoRoot() );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      if ( !dir.mkdir( templateRoot() ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( templateRoot() );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      if ( !dir.mkdir( templateRoot() + "/maps" ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( templateRoot() + "/maps" );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      if ( !dir.mkdir( templateRoot() + "/registrations" ) ) {
        std::string msg = QString("Unable to create folder [%1] when trying to initialize project")
                        .arg( templateRoot() + "/registrations" );
        warn(msg);
        throw IException(IException::Io, msg, _FILEINFO_);
      }
    }
    catch (...) {
      warn("Failed to create project directory structure");
      throw;
    }
  }


  /**
   * Function to clear out all values in a project essentially making it a new project object.
   * This function is also respoinsible for cleaning any directories created when importing but
   * are no longer a part of the project.
   */
  void Project::clear() {
    m_clearing = true;

    // We need to look through the project.xml and remove every directory not in the project
    QStringList shapeDirList;
    bool shapes = false;
    QStringList imageDirList;
    bool images = false;
    QStringList cnetDirList;
    bool controls = false;
    QStringList mapTemplateDirList;
    bool mapTemplates = false;
    QStringList regTemplateDirList;
    bool regTemplates = false;
    QStringList bundleDirList;
    bool bundles = false;
    QFile projectXml(projectRoot() + "/project.xml");

    if (projectXml.open(QIODevice::ReadOnly)) {
      QTextStream projectXmlInput(&projectXml);

      while (!projectXmlInput.atEnd() ) {

        QString line = projectXmlInput.readLine();

        if (controls || line.contains("<controlNets>") ) {
          controls = true;

          if (line.contains("</controlNets>") ) {
            controls = false;
          }

          else if (!line.contains("<controlNets>") ) {
            cnetDirList.append(line.split('"').at(3));
          }
        }

        else if (images || line.contains("<imageLists>") ) {
          images = true;

          if (line.contains("</imageLists>")) {
            images = false;
          }

          else if (!line.contains("<imageLists>") ) {
            imageDirList.append(line.split('"').at(3).simplified());
          }
        }

        else if (shapes || line.contains("<shapeLists>")) {
          shapes = true;

          if (line.contains("</shapeLists>") ) {
            shapes = false;
          }

          else if (!line.contains("<shapeLists>") ) {
            shapeDirList.append(line.split('"').at(3));
          }
        }

        else if (mapTemplates || line.contains("<mapTemplateLists>") ) {
          mapTemplates = true;

          if (line.contains("</mapTemplateLists>") ) {
            mapTemplates = false;
          }

          else if (!line.contains("<mapTemplateLists>") ) {
            QList<QString> components = line.split('"');
            mapTemplateDirList.append(components.at(5));
          }
        }

        else if (regTemplates || line.contains("<regTemplateLists>") ) {
          regTemplates = true;

          if (line.contains("</regTemplateLists>") ) {
            regTemplates = false;
          }

          else if (!line.contains("<regTemplateLists>") ) {
            QList<QString> components = line.split('"');
            regTemplateDirList.append(components.at(5));
          }
        }

        else if (bundles || line.contains("<bundleSolutionInfo>") ) {
          bundles = true;

          if (line.contains("</bundleSolutionInfo>") ) {
            bundles = false;
          }

          else if (line.contains("<runTime>") ) {
            bundleDirList.append(line.split('>').at(1).split('<').at(0));
          }
        }
      }

      QDir cnetsDir(m_projectRoot->path() + "/cnets/");
      cnetsDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
      QStringList cnetsList = cnetsDir.entryList();
      foreach (QString dir, cnetsList) {
        dir = dir.simplified();

        if ( !cnetDirList.contains(dir) ) {
          QDir tempDir(cnetsDir.path() + "/" + dir);
          tempDir.removeRecursively();
        }
      }

      QDir imagesDir(m_projectRoot->path() + "/images/");
      imagesDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
      QStringList imagesList = imagesDir.entryList();
      foreach (QString dir, imagesList) {
        dir = dir.simplified();

        if ( !imageDirList.contains(dir) ) {
          QDir tempDir(imagesDir.path() + "/" + dir);
          tempDir.removeRecursively();
        }
      }

      QDir shapesDir(m_projectRoot->path() + "/shapes/");
      shapesDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
      QStringList shapesList = shapesDir.entryList();
      foreach (QString dir, shapesList) {
        dir = dir.simplified();

        if ( !shapeDirList.contains(dir) ) {
          QDir tempDir(shapesDir.path() + "/" + dir);
          tempDir.removeRecursively();
        }
      }

      QDir mapTemplatesDir(m_projectRoot->path() + "/templates/maps");
      mapTemplatesDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
      QStringList mapTemplatesList = mapTemplatesDir.entryList();
      foreach (QString dir, mapTemplatesList) {
        dir = dir.simplified();

        if ( !mapTemplateDirList.contains("maps/" + dir) ) {
          QDir tempDir(mapTemplatesDir.path() + "/" + dir);
          tempDir.removeRecursively();
        }
      }

      QDir regTemplatesDir(m_projectRoot->path() + "/templates/registrations");
      regTemplatesDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
      QStringList regTemplatesList = regTemplatesDir.entryList();
      foreach (QString dir, regTemplatesList) {
        dir = dir.simplified();

        if ( !regTemplateDirList.contains("registrations/" + dir)) {
          QDir tempDir(regTemplatesDir.path() + "/" + dir);
          tempDir.removeRecursively();
        }
      }

      QDir bundlesDir(m_projectRoot->path() + "/results/bundle/");
      bundlesDir.setFilter(QDir::NoDotAndDotDot | QDir::Dirs);
      QStringList bundleList = bundlesDir.entryList();
      foreach (QString dir, bundleList) {
        dir = dir.simplified();

        if ( !bundleDirList.contains(dir) ) {
          QDir tempDir(bundlesDir.path() + "/" + dir);
          tempDir.removeRecursively();
        }
      }

      projectXml.close();
    }

    try {
      QString tmpFolder = QDir::temp().absolutePath() + "/"
            + Environment::userName() + "_"
            + QApplication::applicationName() + "_" + QString::number( getpid() );
      QDir temp(tmpFolder + "/tmpProject");
      m_projectRoot = new QDir(temp);
    }

    catch (IException &e) {
      throw IException(e, IException::Programmer, "Error creating project folders.", _FILEINFO_);
    }

    catch (std::exception &e) {
      throw IException(IException::Programmer,
          tr("Error creating project folders [%1]").arg( e.what() ), _FILEINFO_);
    }

    m_images->clear();
    m_shapes->clear();
    m_controls->clear();
    m_mapTemplates->clear();
    m_regTemplates->clear();
    m_targets->clear();
    m_guiCameras->clear();
    m_bundleSolutionInfo->clear();
    m_workOrderHistory->clear();

    directory()->clean();
    setClean(true);
  }


  bool Project::clearing() {
    return m_clearing;
  }


  ImageList *Project::createOrRetrieveImageList(QString name, QString path) {
    ImageList *result = imageList(name);
    if (!result) {
      result = new ImageList;

      result->setName(name);
      if (path == "") {
        result->setPath(name);
      }
      else {
        result->setPath(path);
      }

      connect( result, SIGNAL( destroyed(QObject *) ),
               this, SLOT( imageListDeleted(QObject *) ) );
      m_images->append(result);
    }
    return result;
  }


  ShapeList *Project::createOrRetrieveShapeList(QString name, QString path) {
    ShapeList *result = shapeList(name);
    if (!result) {
      result = new ShapeList;

      result->setName(name);
      if (path == "") {
        result->setPath(name);
      }
      else {
        result->setPath(path);
      }

      connect( result, SIGNAL( destroyed(QObject *) ),
               this, SLOT( shapeListDeleted(QObject *) ) );
      m_shapes->append(result);
    }
    return result;
  }


  /**
   * Converts the project settings into XML.
   *
   * The format of the project settings is:
   *
   *  <pre>
   *   <project>
   *     <controlNets>
   *       <controlNet name="..." />
   *     </controlNets>
   *   </project>
   *  </pre>
   */
  void Project::save(QXmlStreamWriter &stream, FileName newProjectRoot) const {
    stream.writeStartElement("project");

    stream.writeAttribute("name", m_name);

    if ( !m_controls->isEmpty() ) {
      stream.writeStartElement("controlNets");

      for (int i = 0; i < m_controls->count(); i++) {
        m_controls->at(i)->save(stream, this, newProjectRoot);
      }

      stream.writeEndElement();
    }

    if ( !m_images->isEmpty() ) {
      stream.writeStartElement("imageLists");
      for (int i = 0; i < m_images->count(); i++) {
        m_images->at(i)->save(stream, this, newProjectRoot);
      }

      stream.writeEndElement();
    }

    if ( !m_shapes->isEmpty() ) {
      stream.writeStartElement("shapeLists");

      for (int i = 0; i < m_shapes->count(); i++) {
        m_shapes->at(i)->save(stream, this, newProjectRoot);
      }

      stream.writeEndElement();
    }

    if ( !m_mapTemplates->isEmpty() ) {
      stream.writeStartElement("mapTemplateLists");

      for (int i = 0; i < m_mapTemplates->count(); i++) {
        m_mapTemplates->at(i)->save(stream, this, newProjectRoot);
      }

      stream.writeEndElement();
    }

    if ( !m_regTemplates->isEmpty() ) {
      stream.writeStartElement("regTemplateLists");

      for (int i = 0; i < m_regTemplates->count(); i++) {
        m_regTemplates->at(i)->save(stream, this, newProjectRoot);
      }

      stream.writeEndElement();
    }

    // TODO:  Finish implementing serialization of TargetBody & GuiCameras
//  if (!m_targets->isEmpty()) {
//    stream.writeStartElement("targets");
//
//    for (int i = 0; i < m_targets->count(); i++) {
//      m_targets->at(i)->save(stream, this, newProjectRoot);
//    }
//
//    stream.writeEndElement();
//  }
//
//  if (!m_guiCameras->isEmpty()) {
//    stream.writeStartElement("cameras");
//
//    for (int i = 0; i < m_guiCameras->count(); i++) {
//      m_guiCameras->at(i)->save(stream, this, newProjectRoot);
//    }
//
//    stream.writeEndElement();
//  }

//  Write general look of gui, including docked widges
//  QVariant geo_data = saveGeometry();
//  QVariant layout_data = saveState();
//
//  stream.writeStartElement("dockRestore");
//  stream.writeAttribute("geometry", geo_data.toString());
//  stream.writeAttribute("state", layout_data.toString());


    if ( !m_bundleSolutionInfo->isEmpty() ) {
      stream.writeStartElement("results");

      for (int i = 0; i < m_bundleSolutionInfo->count(); i++) {
        m_bundleSolutionInfo->at(i)->save(stream, this, newProjectRoot);
      }

      stream.writeEndElement();
    }

    if (m_activeImageList) {
      stream.writeStartElement("activeImageList");
      stream.writeAttribute("displayName", m_activeImageList->name());
      stream.writeEndElement();
    }

    if (m_activeControl) {
      stream.writeStartElement("activeControl");
      stream.writeAttribute("displayName", m_activeControl->displayProperties()->displayName());
      stream.writeEndElement();
    }

    stream.writeEndElement();
  }


  /**
   * Serialize the work orders into the given XML
   *
   * The format of the history xml is:
   * <pre>
   *   <history>
   *     <workOrder>
   *        ...
   *     </workOrder>
   *     <workOrder>
   *        ...
   *     </workOrder>
   *   </history>
   * </pre>
   */
  void Project::saveHistory(QXmlStreamWriter &stream) const {
    stream.writeStartElement("history");

    foreach (WorkOrder *workOrder, *m_workOrderHistory) {
      if (workOrder) {
        workOrder->save(stream);
      }
    }

    stream.writeEndElement();
  }

  /**
   * Serialize the warnings into the given XML
   *
   * The format of the warnings xml is:
   * <pre>
   *   <warnings>
   *     <warning text="..." />
   *     <warning text="..." />
   *   </warnings>
   * </pre>
   */
  void Project::saveWarnings(QXmlStreamWriter &stream) const {
    stream.writeStartElement("warnings");

    foreach (QString warning, *m_warnings) {
      stream.writeStartElement("warning");
      stream.writeAttribute("text", warning);
      stream.writeEndElement();
    }

    stream.writeEndElement();
  }


  /**
   * Verify that the input fileNames are image files.
   *
   * @param fileNames names of files on disk
   * @returns the files that are images.
   */
  // TODO: thread via ImageReader
  QStringList Project::images(QStringList fileNames) {

    QStringList result;

    foreach (QString fileName, fileNames) {
      try {
        Cube tmp(fileName);
        result.append(fileName);
      }
      catch (IException &) {
      }
    }

    return result;
  }


  /**
   * Get a list of configuration/settings actions related to reading images into this Project.
   *
   * These are things like default opacity, default filled, etc.
   */
  QList<QAction *> Project::userPreferenceActions() {
    return m_imageReader->actions(ImageDisplayProperties::FootprintViewProperties);
  }


  /**
   * Create and return the name of a folder for placing control networks.
   *
   * This can be called from multiple threads, but should only be called by one thread at a time.
   */
  QDir Project::addCnetFolder(QString prefix) {
    QDir cnetFolder = cnetRoot();
    prefix += "%1";
    int prefixCounter = 0;

    QString numberedPrefix;
    do {
      prefixCounter++;
      numberedPrefix = prefix.arg( QString::number(prefixCounter) );
    }
    while ( cnetFolder.exists(numberedPrefix) );

    if ( !cnetFolder.mkpath(numberedPrefix) ) {
      throw IException(IException::Io,
          tr("Could not create control network directory [%1] in [%2].")
            .arg(numberedPrefix).arg( cnetFolder.absolutePath() ),
          _FILEINFO_);
    }

    cnetFolder.cd(numberedPrefix);

    m_currentCnetFolder = cnetFolder;

    return cnetFolder;
  }


  /**
   * Add the given Control's to the current project. This will cause the controls to be
   *   saved/restored from disk, Project-related GUIs to display the control, and enable access to
   *   the controls given access to the project.
   */
  void Project::addControl(Control *control) {

    connect( control, SIGNAL( destroyed(QObject *) ),
             this, SLOT( controlClosed(QObject *) ) );
    connect( this, SIGNAL( projectRelocated(Project *) ),
             control, SLOT( updateFileName(Project *) ) );

    createOrRetrieveControlList( FileName( control->fileName() ).dir().dirName(), "" )->append(control);

    (*m_idToControlMap)[control->id()] = control;

    emit controlAdded(control);
  }


  ControlList *Project::createOrRetrieveControlList(QString name, QString path) {
    ControlList *result = controlList(name);

    if (!result) {
      result = new ControlList;

      result->setName(name);
      if (path == "") {
        result->setPath(name);
      }
      else {
        result->setPath(path);
      }

      connect( result, SIGNAL( destroyed(QObject *) ),
               this, SLOT( controlListDeleted(QObject *) ) );

      m_controls->append(result);
      emit controlListAdded(result);
    }

    return result;
  }


  /**
   * Create and return the name of a folder for placing images.
   *
   * This can be called from multiple threads, but should only be called by one thread at a time.
   */
  QDir Project::addImageFolder(QString prefix) {
    QDir imageFolder = imageDataRoot();
    prefix += "%1";
    int prefixCounter = 0;

    QString numberedPrefix;
    do {
      prefixCounter++;
      numberedPrefix = prefix.arg( QString::number(prefixCounter) );
    }
    while ( imageFolder.exists(numberedPrefix) );

    if ( !imageFolder.mkpath(numberedPrefix) ) {
      throw IException(IException::Io,
          tr("Could not create image directory [%1] in [%2].")
            .arg(numberedPrefix).arg( imageFolder.absolutePath() ),
          _FILEINFO_);
    }

    imageFolder.cd(numberedPrefix);

    return imageFolder;
  }


  /**
   * Read the given cube file names as Images and add them to the project.
   * @param QStringList names of imageFiles
   */
  void Project::addImages(QStringList imageFiles) {
    if (m_numImagesCurrentlyReading == 0) {
      m_imageReadingMutex->lock();
    }

    m_numImagesCurrentlyReading += imageFiles.count();
    m_imageReader->read(imageFiles);
  }


  /**
   * Read the given cube file names as Images and add them to the project.
   * @param ImageList
   */
  void Project::addImages(ImageList newImages) {
    imagesReady(newImages);

    //  The each
    emit guiCamerasAdded(m_guiCameras);
    emit targetsAdded(m_targets);
  }


  /**
   * Create and return the name of a folder for placing shape models.
   *
   * This can be called from multiple threads, but should only be called by one thread at a time.
   */
  QDir Project::addShapeFolder(QString prefix) {
    QDir shapeFolder = shapeDataRoot();
    prefix += "%1";
    int prefixCounter = 0;

    QString numberedPrefix;
    do {
      prefixCounter++;
      numberedPrefix = prefix.arg( QString::number(prefixCounter) );
    }
    while ( shapeFolder.exists(numberedPrefix) );

    if ( !shapeFolder.mkpath(numberedPrefix) ) {
      throw IException(IException::Io,
          tr("Could not create shape directory [%1] in [%2].")
            .arg(numberedPrefix).arg( shapeFolder.absolutePath() ),
          _FILEINFO_);
    }

    shapeFolder.cd(numberedPrefix);

    return shapeFolder;
  }


  /**
   * Read the given shape model cube file names as Images and add them to the project.
   * @param QStringList of shape Files names
   */
  void Project::addShapes(QStringList shapeFiles) {
    if (m_numShapesCurrentlyReading == 0) {
      m_shapeReadingMutex->lock();
    }

    m_numShapesCurrentlyReading += shapeFiles.count();
    m_shapeReader->read(shapeFiles);
  }


  /**
   * Read the given shape model cube file names as Images and add them to the project.
   * @param ShapeList
   */
  void Project::addShapes(ShapeList newShapes) {
    shapesReady(newShapes);
  }


  /**
   * Add new templates to m_mapTemplates or m_regTemplates and update project item model
   *
   * @param newFileList QList of FileNames for each new imported template
   */
  void Project::addTemplates(TemplateList *templateList) {
    foreach (Template *templateFile, *templateList) {
      connect( this, SIGNAL( projectRelocated(Project *) ),
               templateFile, SLOT( updateFileName(Project *) ) );
    }
    if (templateList->type() == "maps") {
      m_mapTemplates->append(templateList);
    }
    else if (templateList->type() == "registrations") {
      m_regTemplates->append(templateList);
    }

    emit templatesAdded(templateList);
  }


  /**
   * Create and navigate to the appropriate template type folder in the project directory.
   *
   * @param prefix The name of the director under templates/ to store the template file.
   */
  QDir Project::addTemplateFolder(QString prefix) {
    QDir templateFolder = templateRoot();
    prefix += "%1";
    int prefixCounter = 0;
    QString numberedPrefix;

    do {
      prefixCounter++;
      numberedPrefix = prefix.arg( QString::number(prefixCounter) );
    }
    while ( templateFolder.exists(numberedPrefix) );

    if ( !templateFolder.mkpath(numberedPrefix) ) {
      throw IException(IException::Io,
          tr("Could not create template directory [%1] in [%2].")
            .arg(numberedPrefix).arg( templateFolder.absolutePath() ),
          _FILEINFO_);
    }

    templateFolder.cd(numberedPrefix);

    return templateFolder;
  }


  /**
   * Given the id return the corrsponding control net
   * @param QString id of control net
   */
  Control *Project::control(QString id) {
    return (*m_idToControlMap)[id];
  }


  /**
   * Create and return the name of a folder for placing BundleSolutionInfo.
   *
   * TODO: don't know if sentence below is accurate.
   * This can be called from multiple threads, but should only be called by one thread at a time.
   */
  QDir Project::addBundleSolutionInfoFolder(QString folder) {
    QDir bundleSolutionInfoFolder(bundleSolutionInfoRoot());

    if (!bundleSolutionInfoFolder.mkpath(folder)) {
      throw IException(IException::Io,
                       tr("Could not create bundle results directory [%1] in [%2].")
                       .arg(folder).arg(bundleSolutionInfoFolder.absolutePath()),
                       _FILEINFO_);
    }

    bundleSolutionInfoFolder.cd(folder);
    return bundleSolutionInfoFolder;
  }


  /**
   * Add the given BundleSolutionInfo to the current project. This will cause the
   * BundleSolutionInfo to be saved/restored from disk, Project-related GUIs to display the
   * BundleSolutionInfo, and enable access to the BundleSolutionInfo given access to the project.
   */
  void Project::addBundleSolutionInfo(BundleSolutionInfo *bundleSolutionInfo) {
    connect(bundleSolutionInfo, SIGNAL(destroyed(QObject *)),
            this, SLOT(bundleSolutionInfoClosed(QObject *)));//???
    connect(this, SIGNAL(projectRelocated(Project *)),
            bundleSolutionInfo, SLOT(updateFileName(Project *)));//DNE???


    loadBundleSolutionInfo(bundleSolutionInfo);
  }


  /**
   * Loads bundle solution info into project
   *
   * @param BundleSolutionInfo
   */
  void Project::loadBundleSolutionInfo(BundleSolutionInfo *bundleSolutionInfo) {
    m_bundleSolutionInfo->append(bundleSolutionInfo);

    // add BundleSolutionInfo to project's m_idToBundleSolutionInfoMap
    (*m_idToBundleSolutionInfoMap)[bundleSolutionInfo->id()] = bundleSolutionInfo;

    // add BundleSolutionInfo's control to project's m_idToControlMap
    (*m_idToControlMap)[bundleSolutionInfo->control()->id()] = bundleSolutionInfo->control();

    emit bundleSolutionInfoAdded(bundleSolutionInfo);
  }


  /**
   * Returns the directory associated with this Project. The directory is not part of the project
   *   so a non-const pointer is returned and this is okay.
   *
   * @return The Directory that was used to create this Project.
   */
  Directory *Project::directory() const {
    return m_directory;
  }


  void Project::writeSettings() {

    QString appName = QApplication::applicationName();


    QSettings globalSettings(
        FileName("$HOME/.Isis/" + appName + "/" + appName + "_" + "Project.config")
          .expanded(),
        QSettings::NativeFormat);

    globalSettings.beginGroup("recent_projects");
    QStringList keys = globalSettings.allKeys();
    QMap<QString,QString> recentProjects;

    foreach (QString key,keys) {

      recentProjects[key]=globalSettings.value(key).toString();

    }

    QList<QString> projectPaths = recentProjects.values();

    if (keys.count() >= m_maxRecentProjects) {

      //Clear out the recent projects before repopulating this group
      globalSettings.remove("");



      //If the currently open project is a project that has been saved and is not within the current
      //list of recently open projects, then remove the oldest project from the list.
      if (!this->projectRoot().contains("tmpProject") && !projectPaths.contains(this->projectRoot()) ) {
        QString s=keys.first();
        recentProjects.remove( s );
      }

      //If the currently open project is already contained within the list,
      //then remove the earlier reference.

      if (projectPaths.contains(this->projectRoot())) {
        QString key = recentProjects.key(this->projectRoot());
        recentProjects.remove(key);
      }

      QMap<QString,QString>::iterator i;

      //Iterate through the recentProjects QMap and set the <key,val> pairs.
      for (i=recentProjects.begin();i!=recentProjects.end();i++) {

          globalSettings.setValue(i.key(),i.value());

      }

      //Get a unique time value for generating a key
      long t0 = QDateTime::currentMSecsSinceEpoch();
      QString projName = this->name();

      QString t0String=QString::number(t0);

      //Save the project location
      if (!this->projectRoot().contains("tmpProject") ) {
              globalSettings.setValue(t0String+"%%%%%"+projName,this->projectRoot());

      }

    }

    //The numer of recent open projects is less than m_maxRecentProjects
    else {

      //Clear out the recent projects before repopulating this group
      globalSettings.remove("");
      if (projectPaths.contains(this->projectRoot())) {
        QString key = recentProjects.key(this->projectRoot());
        recentProjects.remove(key);
      }
      QMap<QString,QString>::iterator i;

      //Iterate through the recentProjects QMap and set the <key,val> pairs.
      for ( i=recentProjects.begin(); i!=recentProjects.end(); i++ ) {
          globalSettings.setValue(i.key(),i.value());
      }

      long t0 = QDateTime::currentMSecsSinceEpoch();
      QString projName = this->name();
      QString t0String=QString::number(t0);

      //if (!this->projectRoot().contains("tmpProject") && !projectPaths.contains( this->projectRoot() ) ) {
      if (!this->projectRoot().contains("tmpProject") ) {
        globalSettings.setValue(t0String+"%%%%%"+projName,this->projectRoot());
      }

    }


    globalSettings.endGroup();
  }


  /**
   * Open the project at the given path.
   * @param The path to the project folder
   * @internal
   *   @history Tyler Wilson - Added try-catch blocks around all reader.parse
   *                  calls.  The exception information is not piped to the Warnings tab
   *                  in the GUI instead of the command line, and the application starts
   *                  instead of executing prematurely.  Fixes #4488.
   *   @history 2017-07-24 Cole Neubauer -  Moved all exception checking in Open function to
   *                  beginning of function to avoid clearing a project when an invalid
   *                  directory is chosen Fixes #4969
   * */
  void Project::open(QString projectPathStr) {
    // Expand projectPathStr to contain absolute path
    QString projectAbsolutePathStr = QDir(projectPathStr).absolutePath();

    QString projectXmlPath = projectAbsolutePathStr + "/project.xml";
    QFile file(projectXmlPath);

    if ( !file.open(QFile::ReadOnly) ) {
      throw IException(IException::Io,
                       QString("Unable to open [%1] with read access")
                       .arg(projectXmlPath),
                       _FILEINFO_);
    }

    QString projectXmlHistoryPath = projectAbsolutePathStr + "/history.xml";
    QFile historyFile(projectXmlHistoryPath);

    if ( !historyFile.open(QFile::ReadOnly) ) {
      throw IException(IException::Io,
                       QString("Unable to open [%1] with read access")
                               .arg(projectXmlHistoryPath),
                       _FILEINFO_);
    }

    QString projectXmlWarningsPath = projectAbsolutePathStr + "/warnings.xml";
    QFile warningsFile(projectXmlWarningsPath);

    if (!warningsFile.open(QFile::ReadOnly)) {
      throw IException(IException::Io,
                       QString("Unable to open [%1] with read access")
                       .arg(projectXmlWarningsPath),
                       _FILEINFO_);
    }

    QString directoryXmlPath = projectAbsolutePathStr + "/directory.xml";
    QFile directoryFile(directoryXmlPath);

    if (!directoryFile.open(QFile::ReadOnly)) {
      throw IException(IException::Io,
                       QString("Unable to open [%1] with read access")
                       .arg(directoryXmlPath),
                       _FILEINFO_);
    }

    if (isOpen() || !isClean()) {
      clear();
    }
    m_clearing = false;
    m_isTemporaryProject = false;

    QDir oldProjectRoot(*m_projectRoot);
    *m_projectRoot =  QDir(projectAbsolutePathStr);

    QXmlStreamReader projectXmlReader(&file);

    //This prevents the project from not loading if everything
    //can't be loaded, and outputs the warnings/errors to the
    //Warnings Tab
    try {
      readProjectXml(&projectXmlReader);
    }
    catch (IException &e) {
      directory()->showWarning(QString("Failed to open project completely [%1]")
                               .arg(projectAbsolutePathStr));
      directory()->showWarning(e.toString());
    }
    catch (std::exception &e) {
      directory()->showWarning(QString("Failed to open project completely[%1]")
                               .arg(projectAbsolutePathStr));
      directory()->showWarning(e.what());
    }

    QXmlStreamReader reader2(&historyFile);

    try {
      while (!reader2.atEnd()) {
        reader2.readNext();
      }
    }

    catch (IException &e) {
      directory()->showWarning(QString("Failed to read history from project[%1]")
                               .arg(projectAbsolutePathStr));
      directory()->showWarning(e.toString());
      }
    catch (std::exception &e) {
      directory()->showWarning(QString("Failed to read history from project[%1]")
                                .arg(projectAbsolutePathStr));
      directory()->showWarning(e.what());
    }
    
    QXmlStreamReader reader3(&warningsFile);

    while (!reader3.atEnd()) {
      reader3.readNext();
    }
    if (reader3.hasError()) {
      warn(tr("Failed to read warnings from project [%1]").arg(projectAbsolutePathStr));
    }

    QXmlStreamReader reader4(&directoryFile);

    try {
      while (!reader4.atEnd()) {
        reader4.readNext();
      }
    }
    catch (IException &e) {
      directory()->showWarning(QString("Failed to read GUI state from project[%1]")
                               .arg(projectAbsolutePathStr));
      directory()->showWarning(e.toString());

    }
    catch (std::exception &e) {
      directory()->showWarning(QString("Failed to read GUI state from project[%1]")
                               .arg(projectAbsolutePathStr));
      directory()->showWarning(e.what());
    }

    QDir bundleRoot(bundleSolutionInfoRoot());
    if (bundleRoot.exists()) {
      // get QFileInfo for each directory in the bundle root
      bundleRoot.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks); // sym links ok???

      QFileInfoList bundleDirs = bundleRoot.entryInfoList();

      for (int dirListIndex = 0; dirListIndex < bundleDirs.size(); dirListIndex++) {

        // get QFileInfo for each file in this directory
        QDir bundleSolutionDir(bundleDirs[dirListIndex].absoluteFilePath());
        bundleSolutionDir.setFilter(QDir::Files | QDir::NoSymLinks); // sym links ok???

//         QFileInfoList bundleSolutionFiles = bundleSolutionDir.entryInfoList();
//         for (int fileListIndex = 0; fileListIndex < bundleSolutionFiles.size(); fileListIndex++) {
//           // if the file is an hdf file with BundleSolutionInfo object, add it to the project tree
//           if (bundleSolutionFiles[fileListIndex].fileName().contains("_BundleSolutionInfo.hdf")) {
//             QString  absoluteFileName = bundleSolutionFiles[fileListIndex].absoluteFilePath();
//             FileName solutionFile(bundleSolutionFiles[fileListIndex].absoluteFilePath());
//             loadBundleSolutionInfo(new BundleSolutionInfo(solutionFile));
//           }
//         }
      }
    }
    m_isOpen = true;

    setClean(true);
    emit projectLoaded(this);
  }


  void Project::readProjectXml(QXmlStreamReader *xmlReader) {
    if (xmlReader->readNextStartElement()) {
      if (xmlReader->name() == "project") {
        QStringRef name = xmlReader->attributes().value("name");
        if (!name.isEmpty()) {
          m_project->setName(*(name.string()));
        }
      }
      else if (xmlReader->name() == "controlNets") {
        // m_controls.append(new ControlList(m_project, xmlReader));
      }
      else if (xmlReader->name() == "imageList") {
        // m_imageLists.append(new ImageList(m_project, xmlReader));
      }
      else if (xmlReader->name() == "shapeList") {
        // m_shapeLists.append(new ShapeList(m_project, xmlReader));
      }
      else if (xmlReader->name() == "mapTemplateList") {
        // m_mapTemplateLists.append(new TemplateList(m_project, xmlReader));
      }
      else if (xmlReader->name() == "regTemplateList") {
        // m_regTemplateLists.append(new TemplateList(m_project, xmlReader));
      }
      //  workOrders are stored in history.xml, using same reader as project.xml
      else if (xmlReader->name() == "workOrder") {
        QString type = *(xmlReader->attributes().value("type").string());

        m_workOrder = WorkOrderFactory::create(m_project, type);

        // m_workOrder->read(xmlReader);
      }
      //  warnings stored in warning.xml, using same reader as project.xml
      else if (xmlReader->name() == "warning") {
        QString warningText = *(xmlReader->attributes().value("text").string());

        if (!warningText.isEmpty())
        {
          m_project->warn(warningText);
        }
      }
      else if (xmlReader->name() == "directory") {
        // m_project->directory()->load(xmlReader);
      }
      else if (xmlReader->name() == "dockRestore") {
        //    QVariant geo_data = QVariant(atts.value("geometry"));
        //    restoreGeometry(geo_data);
        //    QVariant layout_data = QVariant(atts.value("state"));
        //    restoreState(layout_data);
      }
      else if (xmlReader->name() == "bundleSolutionInfo") {
        m_bundleSolutionInfos.append(new BundleSolutionInfo(m_project, xmlReader));
      }
      else if (xmlReader->name() == "activeImageList") {
        QString displayName = *(xmlReader->attributes().value("displayName").string());
        m_project->setActiveImageList(displayName);
      }
      else if (xmlReader->name() == "activeControl") {
        // Find Control
        QString displayName = *(xmlReader->attributes().value("displayName").string());
        m_project->setActiveControl(displayName);
      }
      else
      {
        xmlReader->raiseError(QObject::tr("Incorrect file"));
      }
    }
  }

  QProgressBar *Project::progress() {
    return m_imageReader->progress();
  }


  /**
   * Return an image given its id
   * @param QString id
   * @return Image matching id
   */
  Image *Project::image(QString id) {

    return (*m_idToImageMap)[id];
  }


  /**
   * Return an imagelist given its name
   * @param QString name
   * @return Imagelist matching name
   */
  ImageList *Project::imageList(QString name) {
    QListIterator<ImageList *> it(*m_images);

    ImageList *result = NULL;
    while (it.hasNext() && !result) {
      ImageList *list = it.next();

      if (list->name() == name) result = list;
    }

    return result;
  }


  /**
   * Return a shape given its id
   * @param QString id
   * @return Shape matching id
   */
  Shape *Project::shape(QString id) {
    return (*m_idToShapeMap)[id];
  }


  /**
   * Return a shapelist given its name
   * @param QString name
   * @return Shapelist matching name
   */
  ShapeList *Project::shapeList(QString name) {
    QListIterator<ShapeList *> it(*m_shapes);

    ShapeList *result = NULL;
    while (it.hasNext() && !result) {
      ShapeList *list = it.next();

      if (list->name() == name) result = list;
    }

    return result;
  }


  /**
   * Returns if the project is a temp project or not
   * @return bool true if the project is temporary false otherwise
   */
  bool Project::isTemporaryProject() const {
    return m_isTemporaryProject;
  }


 /**
  * Accessor to determine whether a current project is Open
  */
  bool Project::isOpen() {
    return m_isOpen;
  }


 /**
  * Accessor to determine whether the current project is Unsaved. This is used to determine how the
  * program should react with things like opening a new project or closing he window.
  */
  bool Project::isClean() {
    return m_isClean;
  }


 /**
  * Function to change the clean state of the project. This is needed because not every action that
  * changes the project is a workorder. This needs to be called everytime a user would consider an
  * action changing the project. This also explicitely sets the undoStack to the same value passed.
  * @param the boolean value to set the clean state to
  */
  void Project::setClean(bool value) {
    m_isClean = value;
    m_undoStack.cleanChanged(value);
  }


  /**
   * Return the last not undone workorder
   * @return WorkOrder
   */
  WorkOrder *Project::lastNotUndoneWorkOrder() {
    WorkOrder *result = NULL;
    QListIterator< QPointer<WorkOrder> > it(*m_workOrderHistory);
    it.toBack();

    while ( !result && it.hasPrevious() ) {
      WorkOrder *workOrder = it.previous();

      if ( !workOrder->isUndone() && !workOrder->isUndoing() ) {
        result = workOrder;
      }
    }

    return result;
  }


  /**
   * Get the project's GUI name
   */
  QString Project::name() const {
    return m_name;
  }


  /**
   * Return the last not undone workorder
   * @return WorkOrder
   */
  const WorkOrder *Project::lastNotUndoneWorkOrder() const {
    const WorkOrder *result = NULL;
    QListIterator< QPointer<WorkOrder> > it(*m_workOrderHistory);
    it.toBack();

    while ( !result && it.hasPrevious() ) {
      WorkOrder *workOrder = it.previous();

      if ( !workOrder->isUndone() && !workOrder->isUndoing() ) {
        result = workOrder;
      }
    }

    return result;
  }


  /**
   * Return mutex used for Naif calls.  This method is thread-safe.
   *
   * @author 2012-09-11 Tracie Sucharski
   *
   * @return QMutex*
   */
  QMutex *Project::mutex() {
    return m_mutex;
  }


  /**
   * Get the top-level folder of the project. This is where the project is opened from/saved to.
   */
  QString Project::projectRoot() const {
    return m_projectRoot->path();
  }


  /**
   * Get the top-level folder of the new project. This is where the project is opened from/saved to.
   * This is set when a Save As operation is in progress.
   */
  QString Project::newProjectRoot() const {
    return m_newProjectRoot;
  }


  /**
   * Change the project's name (GUI only, doesn't affect location on disk).
   */

  void Project::setName(QString newName) {
    m_name = newName;
    emit nameChanged(m_name);
  }


  /**
   * Returns the Projects stack of QUndoCommands
   * @return QUndoStack
   */
  QUndoStack *Project::undoStack() {
    return &m_undoStack;
  }


  QString Project::nextImageListGroupName() {
    int numLists = m_images->size();
    QString maxName = "";
    QString newGroupName = "Group";

    foreach (ImageList *imageList, *m_images) {
      QString name = imageList->name();
      if ( !name.contains("Group") ) continue;
      if ( maxName.isEmpty() ) {
        maxName = name;
      }
      else if (name > maxName) {
        maxName = name;
      }
    }

    if ( maxName.isEmpty() ) {
      newGroupName += QString::number(numLists+1);
    }
    else {
      int maxNum = maxName.remove("Group").toInt();
      maxNum++;

      newGroupName += QString::number(maxNum);
    }
    return newGroupName;

  }


  /**
   * Locks program if another spot in code is still running and called this function
   */
  void Project::waitForImageReaderFinished() {
    QMutexLocker locker(m_imageReadingMutex);
  }


  /**
   * Locks program if another spot in code is still running and called this function
   */
  void Project::waitForShapeReaderFinished() {
    QMutexLocker locker(m_shapeReadingMutex);
  }


  /**
   * Get the entire list of work orders that have executed.
   */
  QList<WorkOrder *> Project::workOrderHistory() {
    QList<WorkOrder *> result;
    foreach (WorkOrder *workOrder, *m_workOrderHistory) {
      result.append(workOrder);
    }

    return result;
  }


  /**
   * @brief Checks if both an active control and active image list have been set.
   *
   * This can be used to check when both an active control and active image list have been set.
   * This is used for enabling the jigsaw work order on the Project menu when there is an active
   * control and image list set.
   *
   * @see Directory::initializeActions()
   */
  void Project::checkActiveControlAndImageList() {
    if (m_activeControl && m_activeImageList) {
      emit activeControlAndImageListSet();
    }
  }


  /**
   * @brief Checks if at least one control and image have been added to the project.
   *
   * This can be used to check whenever there are control nets and images available
   * in the project. This is used for enabling the jigsaw work order on the Project menu when
   * a control net and image are available / loaded in the project.
   *
   * @see Project::Project(Directory &directory, QObject *parent)
   * @see Directory::initializeActions()
   */
  void Project::checkControlsAndImagesAvailable() {
    if (controls().count() > 0 && images().count() > 0) {
      emit controlsAndImagesAvailable();
    }
  }


  /**
   * @brief Set the Active Control (control network)
   *
   * Set the active control (control network) for views which need to operate on the
   * same control, ie. Footprint2dView, CubeDnView, ControlPointEditView.
   *
   * @internal
   *   @history 2016-06-23 Tracie Sucharski - Original version.
   *   @history 2016-12-22 Tracie Sucharski - Changed to take a displayName, so that it can be used
   *                           when loading a saved project which has an active control saved with
   *                           the displayName.
   *   @history 2017-01-09 Tracie Sucharski - Moved SetImages step from
   *                           SetActiveControlWorkOrder::execute so that SetImages is always done
   *                           whether from the workorder or calling this method directly from the
   *                           project loading.  TODO:  should project loading call the WorkOrder
   *                           rather than this method directly?
   *  @history 2017-07-25 Cole Neubauer - Removed code that stops a new control from
   *                           being chosen Fixes #4969
   *  @history 2017-08-02 Cole Neubauer - Added functionality to switch between active controls
   *                           Fixes #4567
   *  @history 2018-03-30 Tracie Sucharski - If current activeControl has been modified, prompt for
   *                           saving. Emit signal to discardActiveControlEdits.
   *  @history 2018-07-12 Tracie Sucharski - Moved the close/open control net from
   *                           Directory::reloadActiveControlInCnetEditorView to this method to
   *                           prevent seg fault when there are multiple cnetEditorViews with same
   *                           cnet.
   *
   */
  void Project::setActiveControl(QString displayName) {
    Control *previousControl = m_activeControl;
    if (m_activeControl) {

      // If the current active control has been modified, ask user if they want to save or discard
      // changes.
      if (m_activeControl->isModified()) {
        QMessageBox msgBox;
        msgBox.setText("Save current active control");
        msgBox.setInformativeText("The current active control has been modified.  Do you want "
                                  "to save before setting a new active control?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Save);
        int ret = msgBox.exec();
        switch (ret) {
          // Save current active control
          case QMessageBox::Save:
            m_activeControl->write();
            break;
          // Discard any changes made to cnet
          case QMessageBox::Discard:
            // Close, then re-open effectively discarding edits
            m_activeControl->closeControlNet();
            m_activeControl->openControlNet();
            emit discardActiveControlEdits();
            break;
          // Cancel operation
          case QMessageBox::Cancel:
            return;
        }
      }
      emit activeControlSet(false);
      ProjectItem *item = directory()->model()->findItemData(m_activeControl->
                          displayProperties()->displayName(), Qt::DisplayRole);
      item->setTextColor(Qt::black);
      // Make sure active not used in a CnetEditorWidget before closing
      if (!directory()->controlUsedInCnetEditorWidget(m_activeControl)) {
        m_activeControl->closeControlNet();
      }
    }

    ProjectItem *item = directory()->model()->findItemData(displayName, Qt::DisplayRole);
    if (item && item->isControl()) {
      m_activeControl = item->control();

      try {
          m_activeControl->controlNet()->SetImages(*(activeImageList()->serialNumberList()));
          item->setTextColor(Qt::darkGreen);
      }
      catch(IException &e){
          if (previousControl) {
            m_activeControl = previousControl;
            item = directory()->model()->findItemData(m_activeControl->
                                displayProperties()->displayName(), Qt::DisplayRole);
            item->setTextColor(Qt::darkGreen);
            m_activeControl->controlNet()->SetImages(*(activeImageList()->serialNumberList()));
          }
          else {
            m_activeControl = NULL;
          }
          throw IException(e);
        }
      }
    emit activeControlSet(true);
  }


  /**
   * @brief Return the Active Control (control network)
   *
   * @description Returns the active control (control network) for views which need to operate on
   * the same control, ie. Footprint2dView, CubeDnView, ControlPointEditView.
   * IMPORTANT:  Returns NULL if no active Control.
   *
   * @return @b Control * Returns the active Control if set, otherwise returns NULL
   *
   * @internal
   *   @history 2016-06-23 Tracie Sucharski - Original version.
   *   @history 2017-05-17 Tracie Sucharski - If no active control set & there is only one control
   *                          in the project, default to that control.
   *   @history 2017-10-16 Ian Humphrey - Check to make sure we have imported images before trying
   *                           to set an active control when there is only one control in the
   *                           project. Fixes #5160.
   */
  Control *Project::activeControl() {

    if (!m_activeControl && (m_controls->count() == 1 && m_controls->at(0)->count() ==1)) {
      //  Can only set a default control if an active imageList exists or if a default can be set
      if (activeImageList()) {
        QString controlName = m_controls->at(0)->at(0)->displayProperties()->displayName();
        setActiveControl(controlName);
      }
    }

    return m_activeControl;
  }


  /**
   * When a cnet is modified, set the project state to not clean.
   * If the active control was modified, send a signal back to Directory
   * so that other views know that the active was modified. This allows
   * for CubeDnView and Footprint2DView to be redrawn.
   * Currently, this was the easiest place to emit this signal.
   */
  void Project::cnetModified() {
    if (m_activeControl && m_activeControl->isModified()) {
      emit activeControlModified();
    }
    setClean(false);
  }


  /**
   * @brief Set the Active ImageList from the displayName which is saved in project.xml
   *
   * Set the active ImageList for views which need to operate on the
   * same list of images, ie. Footprint2dView, CubeDnView, ControlPointEditView. This version of
   * the setActiveImageList method is used when loading a project which has an activeImageList
   * saved.
   *
   * @internal
   *   @history 2016-12-02 Tracie Sucharski - Original version.
   *   @history 2016-12-29 Tracie Sucharski - Combined the functionality of
   *                           setActiveImageList(ImageList *) in this method.  This will allow
   *                           projects saved with an active ImageList to be restored properly.
   *                           Only the displayName is saved in a project since the ImageList is
   *                           created when the project is loaded.  As long as the Images and
   *                           Controls are loaded before the setActiveImageList is loaded, there
   *                           will be a correct correspondence between the displayName and
   *                           ImageList.
   *  @history 2017-07-25 Cole Neubauer - Removed code that stops a new imageList from
   *                           being choosen Fixes #4969
   *  @history 2017-08-02 Cole Neubauer - Added functionality to switch between active imagelist
   *                           Fixes #4567
   */
  void Project::setActiveImageList(QString displayName) {
    ImageList *previousImageList = m_activeImageList;
    if (m_activeImageList) {
      ProjectItem *item = directory()->model()->findItemData(m_activeImageList->
                    name(), Qt::DisplayRole);
      item->setTextColor(Qt::black);
    }
    ProjectItem *item = directory()->model()->findItemData(displayName, Qt::DisplayRole);
    if (item && item->isImageList()) {
      m_activeImageList = item->imageList();

      if (m_activeControl) {
        try {
          activeControl()->controlNet()->SetImages(*(m_activeImageList->serialNumberList()));
        }
        catch(IException &e){
          if (previousImageList) {
            m_activeImageList = previousImageList;
            item = directory()->model()->findItemData(m_activeImageList->
                          name(), Qt::DisplayRole);
            item->setTextColor(Qt::darkGreen);
            activeControl()->controlNet()->SetImages(*(m_activeImageList->serialNumberList()));
          }
          else {
            m_activeImageList = NULL;
          }
          throw IException(e);
        }
      }
      item->setTextColor(Qt::darkGreen);
      emit activeImageListSet();
    }
  }


  /**
   * @brief  Returns the active ImageList
   *
   * Returns the active ImageList for views which need to operate on the
   * same list of images, ie. Footprint2dView, CubeDnView, ControlPointEditView.
   * IMPORTANT:  Returns NULL if active ImageList is not set and a default cannot be set if there
   *             are multiple image lists in the project.
   *
   * @internal
   *   @history 2016-06-23 Tracie Sucharski - Original version.
   *   @history 2017-05-17 Tracie Sucharski - If no active ImageList set & there is only one
   *                          ImageList in the project, default to that ImageList.
   */
  ImageList *Project::activeImageList() {

    if (!m_activeImageList && m_images->count() == 1) {
      QString imageList = m_images->at(0)->name();

      setActiveImageList(imageList);
    }
    return m_activeImageList;
  }


  /**
   * Appends the root directory name 'cnets' to the project.
   *
   * @return The path to the root directory of the cnet data.
   */
  QString Project::cnetRoot(QString projectRoot) {
    return projectRoot + "/cnets";
  }


  /**
   * Get where control networks ought to be stored inside the project. This is a full path.
   *
   * @return The path to the root directory of the cnet data.
   */
  QString Project::cnetRoot() const {
    return cnetRoot( m_projectRoot->path() );
  }


  /**
   * Return controls in project
   * @return QList of ControlList
   */
  QList<ControlList *> Project::controls() {
    return *m_controls;
  }


  /**
   * Return controlslist matching name in Project
   * @param QString name of controllist to be returned
   * @return ControlList matching name
   */
  ControlList *Project::controlList(QString name) {
    QListIterator< ControlList * > it(*m_controls);

    ControlList *result  = NULL;
    while (it.hasNext() && !result) {
      ControlList *list = it.next();

      if (list->name() == name) result = list;
    }

    return result;
  }


  /**
   * Appends the root directory name 'images' to the project .
   *
   * @return The path to the root directory of the image data.
   */
  QString Project::imageDataRoot(QString projectRoot) {
    return projectRoot + "/images";
  }


  /**
   * Accessor for the root directory of the image data.
   *
   * @return The path to the root directory of the image data.
   */
  QString Project::imageDataRoot() const {
    return imageDataRoot( m_projectRoot->path() );
  }


  /**
   * Appends the root directory name 'shapes' to the project .
   *
   * @return The path to the root directory of the shape models data.
   */
  QString Project::shapeDataRoot(QString projectRoot) {
    return projectRoot + "/shapes";
  }


  /**
   * Accessor for the root directory of the shape model data.
   *
   * @return The path to the root directory of the shape model data.
   */
  QString Project::shapeDataRoot() const {
    return shapeDataRoot( m_projectRoot->path() );
  }


  /**
   * Return the projects shapelist
   * @return Qlist of Shapelist
   */
  QList<ShapeList *> Project::shapes() {
    return *m_shapes;
  }


  /**
   * Return projects imagelist
   * @return Imagelist
   */
  QList<ImageList *> Project::images() {
    return *m_images;
  }


  /**
   * Appends the root directory name 'templates' to the project .
   *
   * @return The path to the root directory of the templates data.
   */
  QString Project::templateRoot(QString projectRoot) {
    return projectRoot + "/templates";
  }


  /**
   * Accessor for the root directory of the template data.
   *
   * @return The path to the root directory of the template data.
   */
  QString Project::templateRoot() const {
    return templateRoot( m_projectRoot->path() );
  }


  /**
   * Return all template FileNames
   *
   * @return QList of FileName
   */
  QList<TemplateList *> Project::templates() {
    QList<TemplateList *> allTemplates = *m_mapTemplates + *m_regTemplates;
    return allTemplates;
  }


  /**
   * Return map template FileNames
   *
   * @return QList of FileName
   */
  QList<TemplateList *> Project::mapTemplates() {
    return *m_mapTemplates;
  }


  /**
   * Return registration template FileNames
   *
   * @return QList of FileName
   */
  QList<TemplateList *> Project::regTemplates() {
    return *m_regTemplates;
  }


  /**
   * Appends the root directory name 'targets' to the project .
   *
   * @return The path to the root directory of the target body data.
   */
  QString Project::targetBodyRoot(QString projectRoot) {
    return projectRoot + "/targets";
  }


  /**
   * Accessor for the root directory of the target body data.
   *
   * @return The path to the root directory of the target body data.
   */
  QString Project::targetBodyRoot() const {
    return targetBodyRoot( m_projectRoot->path() );
  }


  /**
   * Return TargetBodyList in Project
   */
  TargetBodyList Project::targetBodies() {
    return *m_targets;
  }


  /**
   * Appends the root directory name 'results' to the project.
   *
   * @return The path to the root directory of bundleresults data.
   */
  QString Project::resultsRoot(QString projectRoot) {
    return projectRoot + "/results";
  }


  /**
   * Accessor for the root directory of the results data.
   *
   * @return The path to the root directory of the results data.
   */
  QString Project::resultsRoot() const {
    return resultsRoot( m_projectRoot->path() );
  }


  /**
   * Return BundleSolutionInfo objects in Project
   * @return QList of BundleSolutionInfo
   */
  QList<BundleSolutionInfo *> Project::bundleSolutionInfo() {
    return *m_bundleSolutionInfo;
  }


  /**
   * Appends the root directory name 'bundle' to the project results directory.
   *
   * @return The path to the root directory of bundle results data.
   */
  QString Project::bundleSolutionInfoRoot(QString projectRoot) {
    return projectRoot + "/results/bundle";
  }


  /**
   * Accessor for the root directory of the results data.
   *
   * @return The path to the root directory of the results data.
   */
  QString Project::bundleSolutionInfoRoot() const {
    return bundleSolutionInfoRoot( m_projectRoot->path() );
  }


  /**
   * Delete all of the files, that this project stores, from disk.
   */
  void Project::deleteAllProjectFiles() {

    // Currently the deleteFromDisk methods for Image and Shape delete the Cube if it exists, the
    //  other objects deleteFromDisk methods simply remove files.  This could be achieved easier
    //  in this method by simply calling QDir::removeRecursively(), but for future functionality
    //  call each objects deleteFromDisk.  Currently there are no cleanup methods for Bundle results
    //  or templates, so simply remove directory recursively.
    foreach (ImageList *imagesInAFolder, *m_images) {
      imagesInAFolder->deleteFromDisk(this);
    }

    if ( !m_projectRoot->rmdir( imageDataRoot() ) ) {
      warn( tr("Did not properly clean up images folder [%1] in project").arg( imageDataRoot() ) );
    }

    foreach (ShapeList *shapesInAFolder, *m_shapes) {
      shapesInAFolder->deleteFromDisk(this);
    }

    if ( !m_projectRoot->rmdir( shapeDataRoot() ) ) {
      warn( tr("Did not properly clean up shapes folder [%1] in project").
            arg( shapeDataRoot() ) );
    }

    foreach (ControlList *controlsInAFolder, *m_controls) {
      controlsInAFolder->deleteFromDisk(this);
    }

    if ( !m_projectRoot->rmdir( cnetRoot() ) ) {
      warn( tr("Did not properly clean up control network folder [%1] in project")
             .arg( cnetRoot() ) );
    }

    if ( !(QDir(resultsRoot()).removeRecursively()) ) {
      warn( tr("Did not properly clean up results folder [%1] in project")
             .arg( resultsRoot() ) );
    }

    if ( !(QDir(templateRoot()).removeRecursively()) ) {
      warn( tr("Did not properly clean up templates folder [%1] in project")
             .arg( templateRoot() ) );
    }

    if ( !m_projectRoot->rmpath( m_projectRoot->path() ) ) {
      warn( tr("Did not properly clean up project in [%1]").arg( m_projectRoot->path() ) );
    }
  }


  /**
   * This is called when the project is moved.
   *
   * @param newProjectRoot The new root directory for the project.
   */
  void Project::relocateProjectRoot(QString newProjectRoot) {
    m_projectRoot->setPath(newProjectRoot);
    emit projectRelocated(this);
  }


  /**
   * Generic save method to save the state of the project.
   *
   * This method is used to save the state of the project. If the project is currently a temporary
   * project, this method will create a file dialog to prompt the user for a place/name to save
   * the project as. Otherwise, the existing project state will be saved. This method also informs
   * the caller whether or not the save occurred. It is possible for a save to NOT occur if the
   * project is a temporary project and the user cancels/closes the dialog prompt.
   *
   * @return @b bool Returns true if the save completed. The save is considered incomplete if the
   * project is a temporary project and the user either cancels or closes the file dialog prompt
   * that is created.
   */
  bool Project::save() {
    // Let caller know if the save dialog was cancelled
    bool saveDialogCompleted = true;

    if (m_isTemporaryProject) {
      QString newDestination = QFileDialog::getSaveFileName(NULL,
                                                            QString("Project Location"),
                                                            QString("."));

      if ( !newDestination.isEmpty() ) {
        m_isTemporaryProject = false;
        save( QFileInfo(newDestination + "/").absolutePath() );

        // delete the temporary project
        deleteAllProjectFiles();
        relocateProjectRoot(newDestination);

        // 2014-03-14 kle This is a lame kludge because we think that relocateProjectRoot is not
        // working properly. For example, when we save a new project and try to view a control net
        // the it thinks it's still in the /tmp area
        // see ticket #5292
        open(newDestination);
      }
      // Dialog was cancelled
      else {
        saveDialogCompleted = false;
      }
    }
    else {
      // Save all modified controls. If "Save As" is being processed,
      // the controls are written in the Control::copyToNewProjectRoot, so the controls in
      // current project are not modified.
      foreach (ControlList *controlList, *m_controls) {
        foreach (Control *control, *controlList) {
          if (control->isModified()) {
            control->write();
          }
        }
      }
      save(m_projectRoot->absolutePath(), false);
      emit cnetSaved(true);
    }

    return saveDialogCompleted;
  }




  /**
   * @brief Project::save  Saves the project state out to an XML file
   * @param projectPath  The path to the project directory.
   * @param verifyPathDoesntExist A boolean variable which is set to true
   * if we wish to check that we are not overwriting a pre-existing save.
   *
   * XML Serialization.  Below is a tree listing the XML tag hiearchy.
   *
   * @startsalt{projectXMLTagHierarchy.png}"Project::Save XML Tag Hierarchy"
   *
   * {
   * {T
   * +project (project.xml)
   * ++controlNets
   * +++controlList 1 (controls.xml)
   * ++++controls
   * +++++controlNet
   * +++controlList 2
   * ++++controls
   * +++++controlNet
   * ++imageLists
   * +++imageList 1 (images.xml)
   * ++++images
   * +++++image
   * +++imageList 2 (images.xml)
   * ++++images
   * +++++image
   * ++shapeLists
   * +++shapeList 1 (shapes.xml)
   * ++++shapes
   * +++++shape
   * +++shapeList 2 (shapes.xml)
   * ++bundleRuns (currently the tag is output, but not run times)
   * ++activeImageList
   * ++activeControl
   * }
   * }
   * @endsalt
   *
   *
   * The figure below represents a flow chart for XML code generation which starts
   * with a call to this function:
   *
   *   @startuml {projectSaveWorkFlow.png} "Projec Save"
   *   |XML Processing|
   *   start
   *   - Project::save(FileName &, bool &)
   *    -Project::save(QxmlStreamWriter &stream,FileName &)[save project.xml]
   *     |XML Code|
   *     -project.xml
   *     |XML Processing|
   *    repeat
   *     - m_controls[i]->save(stream)
   *    repeat while (i < m_controls.size() )
   *   |XML Code|
   *   -controls.xml for each control list in separate folders.
   *   |XML Processing|
   *   repeat
   *     - m_imagesLists[i]->save(stream)
   *   repeat while (i < m_imageLists.size() )
   *   |XML Code|
   *   -images.xml
   *   |XML Processing|
   *   repeat
   *     - m_shapeLists[i]->save(stream)
   *   repeat while (i < m_shapeLists.size() )
   *
   *   |XML Code|
   *   -shapes.xml for each control list in separate folders.
   *   |XML Processing|
   *   -stream.write("bundleruns") [if it is non-empty]
   *   |XML Code|
   *     -project.xml
   *   |XML Processing|
   *   -stream.write("activeImageList") [if it exists]
   *   |XML Code|
   *     -project.xml
   *   |XML Processing|
   *   -stream.write("activeControl") [if it exists]
   *   |XML Code|
   *   -project.xml
   *   |XML Processing|
   *   -Project::saveHistory(stream)[save history.xml]
   *   |XML Code|
   *   -history.xml in project root folder
   *   |XML Processing|
   *   -Project::saveWarnings(stream)[save warnings.xml]
   *   |XML Code|
   *   -warnings.xml in project root folder
   *   |XML Processing|
   *   -Directory::save(stream)[save directory.xml]
   *   |XML Code|
   *   -directory.xml in project root folder
   *   |XML Processing|
   *   stop
   *   @enduml
   *
   *
   */
  void Project::save(FileName newPath, bool verifyPathDoesntExist) {
    if ( verifyPathDoesntExist && QFile::exists( newPath.toString() ) ) {
      throw IException(IException::Io,
                       QString("Projects may not be saved to an existing path [%1]; "
                               "please select a new path or delete the current folder")
                       .arg(newPath.original()),
                       _FILEINFO_);
    }

    QDir dir;
    if (!dir.mkpath(newPath.toString())) {
      throw IException(IException::Io,
                       QString("Unable to save project at [%1] "
                               "because we could not create the folder")
                       .arg(newPath.original()),
                       _FILEINFO_);
    }

    //  TODO Set newpath member variable.  This is used for some of the data copy methods and is not
    //  the ideal way to handle this.  Maybe change the data copy methods to either take the new
    //  project root in addition to the data root or put the data root in the dataList (ImageList,
    //  etc.). If performing a "Save", m_newProjectRoot == m_projectRoot
    m_newProjectRoot = newPath.toString();

    //  For now set the member variable rather than calling setName which emits signal and updates
    //  ProjectItemModel & the project name on the tree.  This will be updated when the new project
    //  is opened.
    m_name = newPath.name();

    QFile projectSettingsFile(newPath.toString() + "/project.xml");
    if (!projectSettingsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
      throw IException(IException::Io,
                       QString("Unable to save project at [%1] because the file [%2] "
                               "could not be opened for writing")
                       .arg(newPath.original()).arg(projectSettingsFile.fileName()),
                       _FILEINFO_);
    }

    QXmlStreamWriter writer(&projectSettingsFile);
    writer.setAutoFormatting(true);

    writer.writeStartDocument();

    // Do amazing, fantastical stuff here!!!
    save(writer, newPath);

    writer.writeEndDocument();

    QFile projectHistoryFile(newPath.toString() + "/history.xml");
    if (!projectHistoryFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
      throw IException(IException::Io,
                       QString("Unable to save project at [%1] because the file [%2] "
                               "could not be opened for writing")
                       .arg(newPath.original()).arg(projectHistoryFile.fileName()),
                       _FILEINFO_);
    }

    QXmlStreamWriter historyWriter(&projectHistoryFile);
    historyWriter.setAutoFormatting(true);

    historyWriter.writeStartDocument();
    saveHistory(historyWriter);
    historyWriter.writeEndDocument();

    QFile projectWarningsFile(newPath.toString() + "/warnings.xml");
    if (!projectWarningsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
      throw IException(IException::Io,
                       QString("Unable to save project at [%1] because the file [%2] could not be "
                               "opened for writing")
                       .arg(newPath.original()).arg(projectWarningsFile.fileName()),
                       _FILEINFO_);
    }

    QXmlStreamWriter warningsWriter(&projectWarningsFile);
    warningsWriter.setAutoFormatting(true);

    warningsWriter.writeStartDocument();
    saveWarnings(warningsWriter);
    warningsWriter.writeEndDocument();

    //  Save the Directory structure
    QFile directoryStateFile(newPath.toString() + "/directory.xml");
    if (!directoryStateFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
      throw IException(IException::Io,
                       QString("Unable to save project at [%1] because the file [%2] could not be "
                               "opened for writing")
                       .arg(newPath.original()).arg(directoryStateFile.fileName()),
                       _FILEINFO_);
    }

    QXmlStreamWriter directoryStateWriter(&directoryStateFile);
    directoryStateWriter.setAutoFormatting(true);

    directoryStateWriter.writeStartDocument();

    /*
     * TODO: Does Project need to know about Directory?
     * This is the only place that project uses m_directory. This makes me wonder if it is
     * necessary for project to have a Directory member variable.
     */
    m_directory->save(directoryStateWriter, newPath);

    directoryStateWriter.writeEndDocument();
    m_isOpen = true;

    emit projectSaved(this);

  }


  /**
   * @brief This executes the WorkOrder and stores it in the project.
   *
   * @decription Run the WorkOrder and stores it in the project. If WorkOrder::setupExecution()
   * returns true then the WorkOrder's redo is called. This takes ownership of WorkOrder.
   *
   * The order of events is:
   *   1) WorkOrder::setupExecution()
   *   2) emit workOrderStarting()
   *   3) WorkOrder::redo()
   *
   * @see WorkOrder::redo()
   *
   * @param workOrder The work order to be executed. This work order must not already be in the
   *                    project.
   */
  void Project::addToProject(WorkOrder *workOrder) {
    if (workOrder) {
      connect(workOrder, SIGNAL(finished(WorkOrder *)),
              this, SIGNAL(workOrderFinished(WorkOrder *)));

      workOrder->setPrevious(lastNotUndoneWorkOrder());

      if (workOrder->setupExecution()) {
        if (workOrder->previous()) workOrder->previous()->setNext(workOrder);

        m_workOrderHistory->append(workOrder);

        if (workOrder->isSavedToHistory()) {
          emit workOrderStarting(workOrder);
        }

        // Work orders that create clean states (save, save as) don't belong on the undo stack.
        //   Instead, we tell the undo stack that we're now clean.
        if (workOrder->createsCleanState()) {
          m_undoStack.setClean();
          workOrder->execute();
        }
        // All other work orders go onto the undo stack, unless specifically told not to
        else if (workOrder->isUndoable()) {
          // This calls WorkOrder::redo for us through Qt's QUndoStack::push method, redo is only
          // implemented in the base class.  Child work orders do not implement redo.
          m_undoStack.push(workOrder);
        }
        else {
          // If we get this far the WorkOrder is not-undoable therefore we have to call redo by
          // hand.

          workOrder->redo();
        }
        // Clean up deleted work orders (the m_undoStack.push() can delete work orders)
        m_workOrderHistory->removeAll(NULL);
      }
      else {
        delete workOrder;
        workOrder = NULL;
      }
    }
  }


  template<typename Data> void Project::warn(QString text, Data relevantData) {
    storeWarning(text, relevantData);
    directory()->showWarning(text, relevantData);
  }


  void Project::warn(QString text) {
    foreach (QString line, text.split("\n")) {
      storeWarning(line);
      directory()->showWarning(line);
    }
  }


  void Project::storeWarning(QString text) {
    m_warnings->append(text);
  }


  /**
   * Prepare new images for opening
   * @param Imagelist of images
   */
  void Project::imagesReady(ImageList images) {

    m_numImagesCurrentlyReading -= images.count();

    foreach (Image *image, images) {
      connect(image, SIGNAL(destroyed(QObject *)),
              this, SLOT(imageClosed(QObject *)));
      connect(this, SIGNAL(projectRelocated(Project *)),
              image, SLOT(updateFileName(Project *)));

      (*m_idToImageMap)[image->id()] = image;
      if (images.name() != "") {
        createOrRetrieveImageList(images.name(), images.path())->append(image);
      }
      else {
        createOrRetrieveImageList(FileName(images[0]->fileName()).dir().dirName(), "")->append(image);
      }
    }

    // We really can't have all of the cubes in memory before
    //   the OS stops letting us open more files.
    // Assume cameras are being used in other parts of code since it's
    //   unknown
    QMutexLocker lock(m_mutex);
    emit imagesAdded(m_images->last());

    Image *openImage;
    foreach (openImage, images) {
      openImage->closeCube();
    }

//     if(m_projectPvl && m_projectPvl->HasObject("MosaicFileList") )
//       m_fileList->fromPvl(m_projectPvl->FindObject("MosaicFileList") );

//     if(m_projectPvl && m_projectPvl->HasObject("MosaicScene") )
//       m_scene->fromPvl(m_projectPvl->FindObject("MosaicScene") );

    if (m_numImagesCurrentlyReading == 0) {
      m_imageReadingMutex->unlock();
    }
  }


  /**
  * @brief This method checks for the existence of a target based on TargetName
  *
  * @param id The target string to be compared.
  * @return bool Returns true if targetBody already exists in project
  */
  bool Project::hasTarget(QString id) {
    foreach (TargetBodyQsp targetBody, *m_targets) {
      if (QString::compare(targetBody->targetName(), id, Qt::CaseInsensitive) == 0) {
        return true;
      }
    }
    return false;
  }


  /**
  * Adds a new target to the project.
  *
  * @param target The target to be added.
  */
  void Project::addTarget(Target *target) {

    TargetBodyQsp targetBody = TargetBodyQsp(new TargetBody(target));

    m_targets->append(targetBody);

  }


  /**
  * @brief This method checks for the existence of a camera based on InstrumentId
  *
  * @param id The instrument string to be compared.
  * @return bool Returns true if GuiCamera already exists in project
  */
  bool Project::hasCamera(QString id) {
    foreach (GuiCameraQsp camera, *m_guiCameras) {

      if (QString::compare(camera->instrumentId(), id, Qt::CaseInsensitive) == 0) {
        return true;
      }
    }
    return false;
  }


  /**
  * Adds a new camera to the project.
  *
  * @param camera The camera to be added.
  */
  void Project::addCamera(Camera *camera) {

    GuiCameraQsp guiCamera = GuiCameraQsp(new GuiCamera(camera));

    m_guiCameras->append(guiCamera);

  }


  /**
   * Add images to the id map which are not under the projects main data area, the Images node on
   * the project tree, such as the images under bundle results.  This is an interim solution since
   * the Project and model/view does not seem to be properly handling data which is not on the main
   * data part of the project tree.
   *
   * @param ImagesList of images
   */
  void Project::addImagesToIdMap(ImageList images) {

    foreach (Image *image, images) {
      (*m_idToImageMap)[image->id()] = image;
    }
  }


  void Project::removeImages(ImageList &imageList) {
    foreach (Image *image, imageList) {
      delete image;
    }
    foreach (ImageList *list, *m_images) {
      if (list->name() == imageList.name()) {
        m_images->removeOne(list);
      }
    }
  }


  /**
   * An image is being deleted from the project
   * @param QObject image object to be closed
   */
  void Project::imageClosed(QObject *imageObj) {
    QMutableListIterator<ImageList *> it(*m_images);
    while (it.hasNext()) {
      ImageList *list = it.next();

      int foundElement = list->indexOf((Image *)imageObj);

      if (foundElement != -1) {
        list->removeAt(foundElement);
      }
    }

    m_idToImageMap->remove(m_idToImageMap->key((Image *)imageObj));
  }


  /**
   * An image list is being deleted from the project.
   * @param QObject list of images to be deleted
   */
  void Project::imageListDeleted(QObject *imageListObj) {
    int indexToRemove = m_images->indexOf(static_cast<ImageList *>(imageListObj));
    if (indexToRemove != -1) {
      m_images->removeAt(indexToRemove);
    }
  }


  /**
   * A control is being deleted from the project
   */
  void Project::controlClosed(QObject *controlObj) {
    m_idToControlMap->remove(m_idToControlMap->key((Control *)controlObj));
  }


  /**
   * An control list is being deleted from the project.
   */
  void Project::controlListDeleted(QObject *controlListObj) {
    int indexToRemove = m_controls->indexOf(static_cast<ControlList *>(controlListObj));
    if (indexToRemove != -1) {
      m_controls->removeAt(indexToRemove);
    }

    if (controls().count() == 0) {
      emit allControlsRemoved();
    }
  }


  /**
   * A shape model list is being deleted from the project.
   */
  void Project::shapeListDeleted(QObject *shapeListObj) {
    int indexToRemove = m_shapes->indexOf(static_cast<ShapeList *>(shapeListObj));
    if (indexToRemove != -1) {
      m_shapes->removeAt(indexToRemove);
    }
  }


  /**
   * A BundleSolutionInfo object is being deleted from the project
   */
  void Project::bundleSolutionInfoClosed(QObject *bundleSolutionInfoObj) {
    QMutableListIterator<BundleSolutionInfo *> it(*m_bundleSolutionInfo);
    while (it.hasNext()) {
      BundleSolutionInfo *bundleSolutionInfo = it.next();
      if (!bundleSolutionInfo) {
        // throw error???
      }

      int foundElement = m_bundleSolutionInfo->indexOf(
          (BundleSolutionInfo *)bundleSolutionInfoObj);

      if (foundElement != -1) {
        m_bundleSolutionInfo->removeAt(foundElement);
      }
    }

    m_idToBundleSolutionInfoMap->remove(
        m_idToBundleSolutionInfoMap->key((BundleSolutionInfo * )bundleSolutionInfoObj));
  }


  /**
   * A target body is being deleted from the project.
   * TODO: should prevent deleting a target body if there are currently images in the project with
   *       this target?
   */
  void Project::targetBodyClosed(QObject *targetBodyObj) {
//    QMutableListIterator<TargetBody *> it(*m_targets);
//    while ( it.hasNext() ) {
//      TargetBody *targetBody = it.next();
//      if (!targetBody) {
//        // throw error???
//      }

//      int foundElement = m_targets->indexOf( (TargetBody *)targetBodyObj );

//      if (foundElement != -1) {
//        m_targets->removeAt(foundElement);
//      }
//    }

//    m_idToTargetBodyMap->remove(m_idToTargetBodyMap->key((TargetBody *)targetBodyObj));
  }



  void Project::shapesReady(ShapeList shapes) {

    m_numShapesCurrentlyReading -= shapes.count();

    foreach (Shape *shape, shapes) {
      connect(shape, SIGNAL(destroyed(QObject *)),
              this, SLOT(shapeClosed(QObject *)));
      connect(this, SIGNAL(projectRelocated(Project *)),
              shape, SLOT(updateFileName(Project *)));

      (*m_idToShapeMap)[shape->id()] = shape;
      if (shapes.name() != "") {
        createOrRetrieveShapeList(shapes.name(), shapes.path())->append(shape);
      }
      else {
        createOrRetrieveShapeList(FileName(shapes[0]->fileName()).dir().dirName(), "")->append(shape);
      }

    }

    // We really can't have all of the cubes in memory before
    //   the OS stops letting us open more files.
    // Assume cameras are being used in other parts of code since it's
    //   unknown
    QMutexLocker lock(m_shapeMutex);
    emit shapesAdded(m_shapes->last());

    Shape *openShape;
    foreach (openShape, shapes) {
      openShape->closeCube();
    }

    if (m_numShapesCurrentlyReading == 0) {
      m_shapeReadingMutex->unlock();
    }
  }


  /**
   * A shape model is being deleted from the project
   */
  void Project::shapeClosed(QObject *imageObj) {
    QMutableListIterator<ShapeList *> it(*m_shapes);
    while (it.hasNext()) {
      ShapeList *list = it.next();

      int foundElement = list->indexOf((Shape *)imageObj);

      if (foundElement != -1) {
        list->removeAt(foundElement);
      }
    }

    m_idToShapeMap->remove(m_idToShapeMap->key((Shape *)imageObj));
  }

 /**
  * This function returns a QMutex. This was needed to be able to deal with a threading issue with
  * Work Order functions returning a member variable. This is used by creating a QMutexLocker
  * inside of a function accessing a member variable and using the returned QMutex from this
  * function as a parameter. Because the QMutexLocker was created in the function accessing the
  * member variable when the function exits the QMutexLocker is destroyed and the QMutex is
  * unlocked.
  *
  * @return QMutex
  */
  QMutex *Project::workOrderMutex() {
    return m_workOrderMutex;
  }
}