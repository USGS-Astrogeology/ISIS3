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
#include "Image.h"
#include "ImageList.h"
#include "IException.h"

#include <QAction>
#include <QColorDialog>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFuture>
#include <QInputDialog>
#include <QLabel>
#include <QProgressDialog>
#include <QtConcurrentMap>
#include <QXmlStreamWriter>

#include "Color.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Project.h"

namespace Isis {
  /**
   * Creates an image list from an image list name and path (does not read Images).
   *
   * @param name The ImageList's name (i.e. import1, import2, ...).
   * @param path The ImageList's folder name (i.e. import1, import2, ...).
   * @param parent The Qt-relationship parent.
   */
  ImageList::ImageList(QString name, QString path, QObject *parent) : QObject(parent) {
    m_name = name;
    m_path = path;
  }


  /**
   * Creates a blank image list.
   *
   * @param parent The Qt-relationship parent.
   */
  ImageList::ImageList(QObject *parent) : QObject(parent) {
  }


  /**
   * Creates an image list from a list of images.
   *
   * @param images The list of images.
   * @param parent The Qt-relationship parent.
   */
  ImageList::ImageList(QList<Image *> images, QObject *parent) : QObject(parent) {
    append(images);
  }


  /**
   * Copy constructor.
   *
   * @param other The image list to copy.
   */
  ImageList::ImageList(const ImageList &other) :
      QList<Image *>(other) {
    m_name = other.m_name;
    m_path = other.m_path;
  }


  /**
   * Creates an image list from a list of cube file names. This is slow (serial) and not recommended.
   *
   * @param fileNames The list of cube fileNames.
   */
  ImageList::ImageList(QStringList &fileNames) {
    foreach (QString fileName, fileNames) {
      try {
        Image *image = new Image(fileName);
        append(image);
        image->closeCube();
      }
      catch (IException &) {
      }
    }
  }


  /**
   * Destructor. This does not free the Images from memory.
   */
  ImageList::~ImageList() {
  }


  /**
   * Creates a SerialNumberList from the image list.
   *
   * @return @b SerialNumberList The list of serial numbers for the cubes in the ImageList.
   */
  SerialNumberList *ImageList::serialNumberList() {

    SerialNumberList *result = new SerialNumberList();

    for (int i = 0; i < count(); i++) {
      result->add((*this)[i]->fileName());
    }
    return result;
  }


  /**
   * Appends an image to the image list.
   *
   * @param value The image to be appended.
   *
   * @see QList<Image *>::append().
   */
  void ImageList::append(Image * const &value) {
    QList<Image *>::append(value);
    emit countChanged(count());
  }


  /**
   * Appends a list of images to the image list.
   *
   * @param value the list of images to be appened.
   *
   * @see QList<Image *>::append().
   */
  void ImageList::append(const QList<Image *> &value) {
    QList<Image *>::append(value);
    emit countChanged(count());
  }


  /**
   * Clears the image list.
   *
   * @see QList<Image *>::clear().
   */
  void ImageList::clear() {
    bool countChanging = count();
    QList<Image *>::clear();
    if (countChanging) {
      emit countChanged(count());
    }
  }


  /**
   * Erases a single image from the image list.
   *
   * @param pos An iterator pointing to the image to be erased.
   *
   * @return @b QList::iterator An iterator pointing to the image after the image that was removed.
   *
   * @see QList<Image *>::erase()
   */
  QList<Image *>::iterator ImageList::erase(iterator pos) {
    iterator result = QList<Image *>::erase(pos);
    emit countChanged(count());
    return result;
  }


  /**
   * Erases a range of images from the image list.
   * Erases all images from begin up to (but not including) end.
   *
   * @param begin An iterator pointing to the first image to be erased.
   * @param end An iterator pointing to the image just after the last image to be erased.
   *                Will be invalid after the call.
   *
   * @return @b QList::iterator An iterator pointing to the image end refered to before the call.
   *
   * @see QList<Image *>::erase()
   */
  QList<Image *>::iterator ImageList::erase(iterator begin, iterator end) {
    iterator result = QList<Image *>::erase(begin, end);
    emit countChanged(count());
    return result;
  }


  /**
   * Inserts an image into the image list at an index
   *
   * @param i The index at which to insert the image.
   * @param value the image to be inserted.
   *
   * @see QList<Image *>::insert()
   */
  void ImageList::insert(int i, Image * const &value) {
    QList<Image *>::insert(i, value);

    emit countChanged(count());
  }


  /**
   * Inserts an image into the image list after an iterator.
   *
   * @param before An iterator pointing to the image that value will be inserted after
   * @param value The image to be inserted.
   *
   * @return @b QList::iterator An iterator pointing to the inserted image.
   *
   * @see QList<Image *>::insert()
   */
  QList<Image *>::iterator ImageList::insert(iterator before, Image * const &value) {
    iterator result = QList<Image *>::insert(before, value);
    emit countChanged(count());
    return result;
  }


  /**
   * Inserts an image at the beginning of the image list.
   *
   * @param value The image to be inserted.
   *
   * @see QList<Image *>::prepend()
   */
  void ImageList::prepend(Image * const &value) {
    QList<Image *>::prepend(value);
    emit countChanged(count());
  }


  /**
   * Appends an image to the end of the image list.
   * Equivalent to append().
   *
   * @param value The image to be appended.
   *
   * @see QList<Image *>::push_back()
   */
  void ImageList::push_back(Image * const &value) {
    QList<Image *>::push_back(value);
    emit countChanged(count());
  }


  /**
   * Prepends an image to the beginning of the image list.
   * Equivalent to prepend().
   *
   * @param value The image to be appended.
   *
   * @see QList<Image *>::push_front()
   */
  void ImageList::push_front(Image * const &value) {
    QList<Image *>::push_front(value);
    emit countChanged(count());
  }


  /**
   * Removes all occurances of an image.
   *
   * @param value The image to be removed.
   *
   * @return @b int The number of occurances of the image.
   *
   * @see QList<Image *>::removeAll()
   */
  int ImageList::removeAll(Image * const &value) {
    int result = QList<Image *>::removeAll(value);

    if (result != 0) {
      emit countChanged(count());
    }

    return result;
  }


  /**
   * Removes the image at an index.
   *
   * @param i The index of the image to be removed.
   *
   * @see QList<Image *>::removeAt()
   */
  void ImageList::removeAt(int i) {
    QList<Image *>::removeAt(i);
    emit countChanged(count());
  }


  /**
   * Removes the image at the front of the image list.
   *
   * @see QList<Image *>::removeFirst()
   */
  void ImageList::removeFirst() {
    QList<Image *>::removeFirst();
    emit countChanged(count());
  }


  /**
   * Removes the image at the end of the image list.
   *
   * @see QList<Image *>::removeLast()
   */
  void ImageList::removeLast() {
    QList<Image *>::removeLast();
    emit countChanged(count());
  }


  /**
   * Removes the first occurance of an image.
   *
   * @param value The image to be removed.
   *
   * @return @b bool True if successful, otherwise false.
   *
   * @see QList<Image *>::removeOne()
   */
  bool ImageList::removeOne(Image * const &value) {
    bool result = QList<Image *>::removeOne(value);

    if (result) {
      emit countChanged(count());
    }

    return result;
  }


  /**
   * Swaps the image list with another list of images.
   *
   * @param other The list of images to swapped with.
   *
   * @see QList<Image *>::swap()
   */
  void ImageList::swap(QList<Image *> &other) {
    QList<Image *>::swap(other);

    if (count() != other.count()) {
      emit countChanged(count());
    }
  }


  /**
   * Removes the image at an index and returns it.
   *
   * @param i The index of the image to be removed and returned.
   *
   * @return @b Image * The removed image.
   *
   * @see QList<Image *>::takeAt()
   */
  Image *ImageList::takeAt(int i) {
    Image * result = QList<Image *>::takeAt(i);
    emit countChanged(count());
    return result;
  }


  /**
   * Removes and returns the first image.
   *
   * @return @b Image * The first image.
   *
   * @see QList<Image *>::takeFirst()
   */
  Image *ImageList::takeFirst() {
    Image *result = QList<Image *>::takeFirst();
    emit countChanged(count());
    return result;
  }


  /**
   * Removes and returns the last image.
   *
   * @return @b Image * The last image.
   *
   * @see QList<Image *>::takeLast()
   */
  Image *ImageList::takeLast() {
    Image *result = QList<Image *>::takeLast();
    emit countChanged(count());
    return result;
  }


  /**
   * Appends a list of images to the end of the image list.
   *
   * @param other The list of images to be appended.
   *
   * @return @b ImageList & A reference to the imageList.
   *
   * @see append()
   * @see QList<Image *>::operator+=()
   */
  ImageList &ImageList::operator+=(const QList<Image *> &other) {
    QList<Image *>::operator+=(other);

    if (other.count()) {
      emit countChanged(count());
    }

    return *this;
  }


  /**
   * Appends a single image to the end of the image list.
   *
   * @param other The image to be appended.
   *
   * @return @b ImageList & A reference to the imageList.
   *
   * @see append()
   * @see QList<Image *>::operator+=()
   */
  ImageList &ImageList::operator+=(Image * const &other) {
    QList<Image *>::operator+=(other);
    emit countChanged(count());
    return *this;
  }


  /**
   * Appends a list of images to the end of the image list.
   *
   * @param other The list of images to be appended.
   *
   * @return @b ImageList & A reference to the imageList.
   *
   * @see append()
   * @see QList<Image *>::operator<<()
   */
  ImageList &ImageList::operator<<(const QList<Image *> &other) {
    QList<Image *>::operator<<(other);

    if (other.count()) {
      emit countChanged(count());
    }

    return *this;
  }


  /**
   * Appends a single image to the end of the image list.
   *
   * @param other The image to be appended.
   *
   * @return @b ImageList & A reference to the imageList.
   *
   * @see append()
   * @see QList<Image *>::operator<<()
   */
  ImageList &ImageList::operator<<(Image * const &other) {
    QList<Image *>::operator<<(other);
    emit countChanged(count());
    return *this;
  }


  /**
   * Assigns another list of images to the image list.
   *
   * @param rhs The list of images that imageList will become a copy of.
   *
   * @return @b ImageList & A reference to the imageList.
   *
   * @see QList<Image *>::operator=()
   */
  ImageList &ImageList::operator=(const QList<Image *> &rhs) {
    bool countChanging = (rhs.count() != count());
    QList<Image *>::operator=(rhs);

    if (countChanging) {
      emit countChanged(count());
    }

    return *this;
  }


  /**
   * Assignment operator
   *
   * @param rhs The right hand side of the '=' operator
   *
   * @return @b ImageList & This image list.
   */
  ImageList &ImageList::operator=(const ImageList &rhs) {
    bool countChanging = (rhs.count() != count());
    QList<Image *>::operator=(rhs);

    m_name = rhs.m_name;
    m_path = rhs.m_path;

    if (countChanging) {
      emit countChanged(count());
    }

    return *this;
  }


  /**
   * Gets a list of pre-connected actions that have to do with display.  If any image
   * does not support a given set of actions, then those will actions will be skipped for
   * all images.
   *
   * @param project The project that owns the images in the imageList.
   *
   * @return @b QList<QAction *> A list of connected actions.  The actions are as follows:
   *                                 ChangeTransparency, ChangeColor, RandomColor,
   *                                 ToggleShowLabel, ToggleShowFilled, ToggleShowCubeData,
   *                                 ToggleShowOutline.
   * @internal
   *   @history 2017-07-21 Marjorie Hahn - Removed unnecessary null project check around
   *                           the QActions for moveToTopAct, moveToTop, moveToBottomAct,
   *                           moveToBottom, and zoomFit. This allows these actions to be
   *                           available in IPCE as well as qmos. Fixes #5027.
   */
  QList<QAction *> ImageList::supportedActions(Project *project) {

    QList<QAction *> actions;

    // It turns out connect() statements cannot be templated, hence they aren't inside of
    //   createWorkOrder().
    if (allSupport(ImageDisplayProperties::Color)) {

      QAction *alphaAction = createWorkOrder(project, ImageListActionWorkOrder::ChangeTransparency);
      if (!project) {
        connect(alphaAction, SIGNAL(triggered()),
                this, SLOT(askAndUpdateAlpha()));
      }
      actions.append(alphaAction);

      QAction *colorAction = createWorkOrder(project, ImageListActionWorkOrder::ChangeColor);
      if (!project) {
        connect(colorAction, SIGNAL(triggered()),
                this, SLOT(askAndUpdateColor()));
      }
      actions.append(colorAction);

      QAction *ranColorAction = createWorkOrder(project, ImageListActionWorkOrder::RandomizeColor);
      if (!project) {
        connect(ranColorAction, SIGNAL(triggered()),
                this, SLOT(showRandomColor()));
      }
      actions.append(ranColorAction);
    }

    if (allSupport(ImageDisplayProperties::ShowLabel)) {
      QAction *labelVisibleAction = createWorkOrder(project,
                                                    ImageListActionWorkOrder::ToggleShowLabel);
      if (!project) {
        connect(labelVisibleAction, SIGNAL(triggered()),
                this, SLOT(saveAndToggleShowLabel()));
      }
      actions.append(labelVisibleAction);
    }

    if (allSupport(ImageDisplayProperties::ShowFill)) {
      QAction *fillAction = createWorkOrder(project, ImageListActionWorkOrder::ToggleShowFilled);
      if (!project) {
        connect(fillAction, SIGNAL(triggered()),
                this, SLOT(saveAndToggleShowFill()));
      }
      actions.append(fillAction);
    }

    if (allSupport(ImageDisplayProperties::ShowDNs)) {
      QAction *cubeDataAction = createWorkOrder(project,
                                                ImageListActionWorkOrder::ToggleShowCubeData);
      if (!project) {
        connect(cubeDataAction, SIGNAL(triggered()),
                this, SLOT(saveAndToggleShowDNs()));
      }
      actions.append(cubeDataAction);
    }

    if (allSupport(ImageDisplayProperties::ShowOutline)) {
      QAction *outlineAction = createWorkOrder(project,
                                               ImageListActionWorkOrder::ToggleShowOutline);
      if (!project) {
        connect(outlineAction, SIGNAL(triggered()),
                this, SLOT(saveAndToggleShowOutline()));
      }
      actions.append(outlineAction);
    }

    actions.append(NULL);

    if (allSupport(ImageDisplayProperties::ZOrdering)) {

      QAction *moveToTopAct = createWorkOrder(project, ImageListActionWorkOrder::MoveToTop);
      QAction *moveUpAct = createWorkOrder(project, ImageListActionWorkOrder::MoveUpOne);
      QAction *moveToBottomAct = createWorkOrder(project, ImageListActionWorkOrder::MoveToBottom);
      QAction *moveDownAct = createWorkOrder(project, ImageListActionWorkOrder::MoveDownOne);

      foreach (Image *image, *this) {
        connect(moveToTopAct, SIGNAL(triggered()),
                image->displayProperties(), SIGNAL(moveToTop()));

        connect(moveUpAct, SIGNAL(triggered()),
                image->displayProperties(), SIGNAL(moveUpOne()));

        connect(moveToBottomAct, SIGNAL(triggered()),
                image->displayProperties(), SIGNAL(moveToBottom()));

        connect(moveDownAct, SIGNAL(triggered()),
                image->displayProperties(), SIGNAL(moveDownOne()));
      }

      actions.append(moveToTopAct);
      actions.append(moveUpAct);
      actions.append(moveToBottomAct);
      actions.append(moveDownAct);
    }

    actions.append(NULL);

    if (size() == 1 && allSupport(ImageDisplayProperties::Zooming)) {
      QAction *zoomFit = createWorkOrder(project, ImageListActionWorkOrder::ZoomFit);
      connect(zoomFit, SIGNAL(triggered()),
              first()->displayProperties(), SIGNAL(zoomFit()));
      actions.append(zoomFit);
    }

    return actions;
  }


  /**
   * Check if all images in the image list support a display property.
   *
   * @param prop The property we're testing for support for
   *
   * @return @b bool True if all images in the Image List support the property.
   *                     Otherwise, false.
   */
  bool ImageList::allSupport(ImageDisplayProperties::Property prop) {
    if (isEmpty())
      return false;

    foreach (Image *image, *this) {
      if (!image->displayProperties()->supports(prop))
        return false;
    }

    return true;
  }


  /**
   * Set the human-readable name of this image list. This is really only useful for project
   * image lists (not anonymous temporary ones).
   *
   * @param newName The name to give this image list
   */
  void ImageList::setName(QString newName) {
    m_name = newName;
  }


  /**
   * Set the relative path (from the project root) to this image list's folder. This is really only
   * useful for project image lists (not anonymous temporary ones).
   *
   * @param newPath The path to the images in this image list
   */
  void ImageList::setPath(QString newPath) {
    m_path = newPath;
  }


  /**
   * Get the human-readable name of this image list
   *
   * @return @b Qstring The name of the image list (or an empty string if anonymous).
   */
  QString ImageList::name() const {
    return m_name;
  }


  /**
   * Get the path to the images in the image list (relative to project root). This only applies to
   * an image list from the project.
   *
   * @return @b QString The path to the images in the image list (or an empty string if unknown).
   */
  QString ImageList::path() const {
    return m_path;
  }


  /**
   * Delete all of the contained Images from disk.
   *
   * @param project The project the images in the image list belong to.
   *
   * @see Image::deleteFromDisk()
   */
  void ImageList::deleteFromDisk(Project *project) {
    foreach (Image *image, *this) {
      image->deleteFromDisk();
    }

    if (!m_path.isEmpty()) {
      QFile::remove(project->imageDataRoot() + "/" + m_path + "/images.xml");

      QDir dir;
      dir.rmdir(project->imageDataRoot() + "/" + m_path);
    }
  }


  /**
   * Convert this image list into XML format for saving/restoring capabilities.
   *
   * This writes:
   * <pre>
   *   <imageList name="..." path="..."/>
   * </pre>
   * to the given xml stream, and creates an 'images.xml' inside the folder with the images.
   * Inside the images.xml, this writes:
   *
   * <pre>
   *   <images>
   *     ...
   *   </images>
   * </pre>
   *
   * @param stream XmlStream to write out the document.
   * @param project The project the image list will be saved to.
   * @param newProjectRoot The path to the root directory for the new project.
   *
   * @throws iException::Io "Failed to create directory"
   * @throws iException::Io "Unable to save image information because new file could not
   *         be opened for writing"
   */
  void ImageList::save(QXmlStreamWriter &stream, const Project *project, FileName newProjectRoot)
      const {
    stream.writeStartElement("imageList");
    stream.writeAttribute("name", m_name);
    stream.writeAttribute("path", m_path);
    // The newProjectRoot contains the full path and we want the dataRoot to be relative to the
    // projectRoot so that projects can be moved. 
    QString dataRoot =
        Project::imageDataRoot(QString::fromStdString(newProjectRoot.toString())).remove(project->newProjectRoot());
    // Get rid of any preceding "/"
    if (dataRoot.startsWith("/")) {
      dataRoot.remove(0,1);
    }
    stream.writeAttribute("dataRoot", dataRoot);

    FileName settingsFileName(Project::imageDataRoot(QString::fromStdString(newProjectRoot.toString())).toStdString() +
                              "/" + m_path.toStdString() + "/images.xml");

    if (!std::filesystem::create_directories(settingsFileName.dir())) {
      throw IException(IException::Io, "Failed to create directory [" + settingsFileName.path() + "]",
                       _FILEINFO_);
    }
    QFile imageListContentsFile(QString::fromStdString(settingsFileName.toString()));

    if (!imageListContentsFile.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
      throw IException(IException::Io, "Unable to save image information for [" +  m_name.toStdString() + "] because [" + settingsFileName.original() + "] could not be opened for writing",
          _FILEINFO_);
    }

    QXmlStreamWriter imageDetailsWriter(&imageListContentsFile);
    imageDetailsWriter.setAutoFormatting(true);
    imageDetailsWriter.writeStartDocument();

    imageDetailsWriter.writeStartElement("images");

    // Only copy images if saving to new location
    if (project->newProjectRoot() != project->projectRoot()) {
      int countWidth = QString("%1L").arg(count()).size() - 1;
      QChar paddingChar('0');

      QLabel *progressLabel = new QLabel;

      QProgressDialog progressDialog;
      progressDialog.setLabel(progressLabel);
      progressDialog.setRange(-1, count());
      progressDialog.setValue(-1);

      // Mapped is way faster than hundreds/thousands of run() calls... so use mapped for performance
      QFuture<void *> future = QtConcurrent::mapped(*this,
                                                    CopyImageDataFunctor(project, newProjectRoot));

      for (int i = 0; i < count(); i++) {
        int newProgressValue = progressDialog.value() + 1;
        progressLabel->setText(
            tr("Saving Image Information for [%1] - %L2/%L3 done")
              .arg(m_name)
              .arg(newProgressValue, countWidth, 10, paddingChar)
              .arg(count()));
        progressDialog.setValue(newProgressValue);
        try {
          future.resultAt(i);
        }
        catch(std::exception &e) {
          std::string msg = "Could not save ImageList: "+ this->name().toStdString();
          throw IException(IException::Io,msg,_FILEINFO_);
        }
      }

      progressLabel->setText(tr("Finalizing..."));
      progressDialog.setRange(0, 0);
      progressDialog.setValue(0);
    }

    foreach (Image *image, *this) {
      image->save(imageDetailsWriter, project, newProjectRoot);
    }

    imageDetailsWriter.writeEndElement();

    imageDetailsWriter.writeEndDocument();

    stream.writeEndElement();
  }


  /**
   * Constructor for CopyImageDataFunctor.
   *
   * @param project The project that the image data will be saved to when the functor is used
   * @param newProjectRoot The path to the project root
   */
  ImageList::CopyImageDataFunctor::CopyImageDataFunctor(const Project *project,
                                                        FileName newProjectRoot) {
    m_project = project;
    m_newProjectRoot = newProjectRoot;
  }


  /**
   * Copy constructor for CopyImageDataFunctor.
   *
   * @param other The functor to copy from
   */
  ImageList::CopyImageDataFunctor::CopyImageDataFunctor(const CopyImageDataFunctor &other) {
    m_project = other.m_project;
    m_newProjectRoot = other.m_newProjectRoot;
  }


  /**
   * Destructor for CopyImageDataFunctor.
   */
  ImageList::CopyImageDataFunctor::~CopyImageDataFunctor() {
  }


  /**
   * Copies the cub/ecub files for an image into m_project.
   * Used by save to copy the imageList into a new project.
   *
   * @param imageToCopy The image to copy into m_project.
   *
   * @see save
   */
  void *ImageList::CopyImageDataFunctor::operator()(Image * const &imageToCopy) {
    try {
      imageToCopy->copyToNewProjectRoot(m_project, m_newProjectRoot); 
    }
    catch (IException &e) {
      IString msg = "Could not copy image [" + imageToCopy->displayProperties()->displayName().toStdString() +
                    "]";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }
    return NULL;
  }


  /**
   * Assignment operator for CopyImageDataFunctor.
   *
   * @param rhs The functor to assign from
   *
   * @return @b ImageList::CopyImageDataFunctor & A reference to a copy of the functor
   */
  ImageList::CopyImageDataFunctor &ImageList::CopyImageDataFunctor::operator=(
      const CopyImageDataFunctor &rhs) {
    m_project = rhs.m_project;
    m_newProjectRoot = rhs.m_newProjectRoot;
    return *this;
  }


  /**
   * Sets the alpha values of the images based on a list of values.
   * The alpha value of the first image in the image list will be set to the first value in alphaValues,
   * the alpha value of the second image will be set to the second value, etc.
   *
   * @param alphaValues The list of alpha values to be applied.
   *
   */
  void ImageList::applyAlphas(QStringList alphaValues) {
    if (count() == alphaValues.count()) {
      for (int i = 0; i < count(); i++) {
        ImageDisplayProperties *dispProps = (*this)[i]->displayProperties();
        QColor color = dispProps->getValue(ImageDisplayProperties::Color).value<QColor>();
        color.setAlpha(alphaValues[i].toInt());
        dispProps->setColor(color);
      }
    }
  }


  /**
   * Sets the colors values of the images based on a list of values.
   * The color values of the first image in the image list will be set to the values in the first element of
   * colorValues, the color values of the second image will be set to the values in the second element, etc.
   *
   * @param colorValues The list of color values to be applies. Color values should be formated as RGBA
   *                        with each value separated by " ".
   * @param column The number of entries in each color value.   Usually 4, R G B A.
   */
  void ImageList::applyColors(QStringList colorValues, int column) {
    if (count() == colorValues.count()) {
      for (int i = 0; i < count(); i++) {
        QString colorData = colorValues[i].split(" ")[column];
        (*this)[i]->displayProperties()->setColor(Color::fromRGBAString(colorData));
      }
    }
  }


  /**
   * Sets the visibility of the display names of the images in the image list based on a list of values.
   * The visibility of the display name of the first image in the image list will be set based on the
   * first value in showLabelValues, The visibility of the display name of the second image will be set
   * based on the second value, etc.
   *
   * @param showLabelValues The list of values to determine which image display names will be shown.
   *                            If a value in showLabelValues is "shown", then the display name of the
   *                            associated image will be shown.  Otherwise it will not be shown.
   */
  void ImageList::applyShowLabel(QStringList showLabelValues) {
    if (count() == showLabelValues.count()) {
      for (int i = 0; i < count(); i++) {
        ImageDisplayProperties *dispProps = (*this)[i]->displayProperties();
        dispProps->setShowLabel(showLabelValues[i] == "shown");
      }
    }
  }


  /**
   * Sets the visibility of the fill areas of the images in the image list based on a list of values.
   * The visibility of the fill area of the first image in the image list will be set based on the first
   * value in showFillValues, the visibility of the fill area of the second image will be set based on the
   * second value, etc.
   *
   * @param showFillValues The list of values to determine which image fill areas will be shown.
   *                           If a value in showFillValues is "shown", then the fill area of the
   *                           associated image will be shown.  Otherwise it will not be shown.
   */
  void ImageList::applyShowFill(QStringList showFillValues) {
    if (count() == showFillValues.count()) {
      for (int i = 0; i < count(); i++) {
        ImageDisplayProperties *dispProps = (*this)[i]->displayProperties();
        dispProps->setShowFill(showFillValues[i] == "shown");
      }
    }
  }


  /**
   * Sets the visibility of the DNs of the images in the image list based on a list of values.
   * The visibility of the DNs of the first image in the image list will be set based on the first value in
   * showDNsValues, the visibility of the DNs of the second image will be set based on the second value, etc.
   *
   * @param showDNsValues The list of values to determine which image DNs will be shown.
   *                          If a value in showDNsValues is "shown", then the fill area of the
   *                          associated image will be shown.  Otherwise it will not be shown.
   */
  void ImageList::applyShowDNs(QStringList showDNsValues) {
    if (count() == showDNsValues.count()) {
      for (int i = 0; i < count(); i++) {
        ImageDisplayProperties *dispProps = (*this)[i]->displayProperties();
        dispProps->setShowDNs(showDNsValues[i] == "shown");
      }
    }
  }


  /**
   * Sets the visibility of the outlines of the images in the image list based on a list of values.
   * The visibility of the outline of the first image in the image list will be set based on the first value in
   * showOutlineValues, the visibility of the outline of the second image will be set based on the second value, etc.
   *
   * @param showOutlineValues The list of values to determine which image outlines will be shown.
   *                              If a value in showOutlineValues is "shown", then the outline of the associated
   *                              image will be shown.  Otherwise it will not be shown.
   */
  void ImageList::applyShowOutline(QStringList showOutlineValues) {
    if (count() == showOutlineValues.count()) {
      for (int i = 0; i < count(); i++) {
        ImageDisplayProperties *dispProps = (*this)[i]->displayProperties();
        dispProps->setShowOutline(showOutlineValues[i] == "shown");
      }
    }
  }


  /**
   * Prompts the user for an alpha value. If the user selects
   * an alpha then this sets alphaResult and returns true.  Does
   * not modify the image list.
   *
   * @param alphaResult The alpha value input by the user
   *
   * @return @b bool True if the user input an alpha value.
   *                     Otherwise false.
   *
   * @see ImageList::askAndUpdateAlpha
   * @see ImageListActionWorkOrder::execute
   */
  bool ImageList::askAlpha(int *alphaResult) const {
    bool result = false;

    if (!isEmpty()) {
      ImageDisplayProperties *dispProps = first()->displayProperties();
      *alphaResult = QInputDialog::getInt(NULL, "Transparency Value",
          "Set the cube's transparency\nValues are 0 (invisible) to 255 (solid)",
          dispProps->getValue(ImageDisplayProperties::Color).value<QColor>().alpha(),
          0, 255, 1, &result);
    }

    return result;
  }


  /**
   * Prompts the user for color values.  If the user selects color
   * values then this sets colorResult and returns true.  Does no
   * modify the image list.
   *
   * @param colorResult The color values input by the user
   *
   * @return @b bool True if the user input a color value.
   *                     Otherwise false.
   *
   * @see ImageList::askAndUpdateColor
   * @see ImageListActionWorkOrder::execute
   */
  bool ImageList::askNewColor(QColor *colorResult) const {
    *colorResult = QColor();

    if (!isEmpty()) {
      ImageDisplayProperties *dispProps = first()->displayProperties();
      *colorResult = QColorDialog::getColor(
          dispProps->getValue(ImageDisplayProperties::Color).value<QColor>(), NULL,
          "Cube Display Color",
          QColorDialog::ShowAlphaChannel);
    }

    return colorResult->isValid();
  }


  /**
   * Sets the alpha value of every image in the image list to a specificed value.
   * Saves and returns the old alpha values.
   *
   * @param newAlpha The alpha value which every image's alpha values will be set to.
   *
   * @return @b QStringList A list of all the old alpha values.
   */
  QStringList ImageList::saveAndApplyAlpha(int newAlpha) {
    QStringList results;
    foreach (Image *image, *this) {
      ImageDisplayProperties *displayProperties = image->displayProperties();

      QColor displayColor = displayProperties->getValue(
          ImageDisplayProperties::Color).value<QColor>();

      results.append(QString::number(displayColor.alpha()));

      displayColor.setAlpha(newAlpha);
      displayProperties->setColor(displayColor);
    }

    return results;
  }


  /**
   * Sets the color values of every image to a specificed set of values.
   * Saves and returns the old color values for each image.
   *
   * @param newColor The colro values which every image's color values will be set to.
   *
   * @return @b QStringList A list of all the old color values.  Every image's old color values are
   *                            listed as R G B A, separated with " ".
   */
  QStringList ImageList::saveAndApplyColor(QColor newColor) {
    QStringList results;

    if (newColor.isValid()) {
      foreach (Image *image, *this) {
        ImageDisplayProperties *displayProperties = image->displayProperties();

        QColor displayColor = displayProperties->getValue(
            ImageDisplayProperties::Color).value<QColor>();

        results.append(Color::toRGBAString(displayColor));

        displayProperties->setColor(newColor);
      }
    }

    return results;
  }


  /**
   * Sets the color values of every image to a random color.
   * Preserves the alpha values of each individual image.
   *
   * @return @b QStringList A list of all the old color values and all the new color values
   *                            in rgba format.
   */
  QStringList ImageList::saveAndApplyRandomColor() {
    QStringList results;

    foreach (Image *image, *this) {
      QColor ranColor = ImageDisplayProperties::randomColor();

      ImageDisplayProperties *displayProperties = image->displayProperties();

      QColor displayColor = displayProperties->getValue(
          ImageDisplayProperties::Color).value<QColor>();

      // Preserve alpha
      ranColor.setAlpha(displayColor.alpha());

      // QColor::name() doesn't preserve alpha.
      results.append(
          QString("%1 %2").arg(Color::toRGBAString(displayColor))
                          .arg(Color::toRGBAString(ranColor)));

      displayProperties->setColor(ranColor);
    }

    return results;
  }


  /**
   * Prompt the user for a new alpha value. If the user selects a new
   * alpha then every image's display properties is updated.
   */
  void ImageList::askAndUpdateAlpha() {
    int newAlpha = 255;

    if (askAlpha(&newAlpha)) {
      saveAndApplyAlpha(newAlpha);
    }
  }


  /**
   * Prompt the user for a new color. If the user selects a new color
   * then every image's display properties is updated.
   */
  void ImageList::askAndUpdateColor() {
    QColor newColor;
    askNewColor(&newColor);
    saveAndApplyColor(newColor);
  }


  /**
   * This applies a new semi-random color to every image's display
   * property for every image in this image list.
   */
  void ImageList::showRandomColor() {
    foreach (Image *image, *this) {
      QColor ranColor = ImageDisplayProperties::randomColor();
      image->displayProperties()->setColor(ranColor);
    }
  }


  /**
   * Changes the visibility of the DNs of the first image in the image list and synchronizes the
   * visibility of the DNs of every other image with the visibility of the DNs of the first image.
   *
   * @return @b QStringList A list containing the original visibility of every image's DNs.
   */
  QStringList ImageList::saveAndToggleShowDNs() {
    QStringList results;

    if (!isEmpty()) {
      ImageDisplayProperties *firstDisplayProperties = first()->displayProperties();
      bool newValue = !firstDisplayProperties->getValue(ImageDisplayProperties::ShowDNs).toBool();

      foreach (Image *image, *this) {
        ImageDisplayProperties *displayProps = image->displayProperties();

        bool value = displayProps->getValue(ImageDisplayProperties::ShowDNs).toBool();
        results.append(value? "shown" : "hidden");

        image->displayProperties()->setShowDNs(newValue);
      }
    }

    return results;
  }


  /**
   * Changes the visibility of the fill area of the first image in the image list and synchronizes
   * the visibility of the fill areas of every other image with the visibility of fill area of the
   * first image.
   *
   * @return @b QStringList A list containing the original visibility of every image's fill area.
   */
  QStringList ImageList::saveAndToggleShowFill() {
    QStringList results;

    if (!isEmpty()) {
      ImageDisplayProperties *firstDisplayProps = first()->displayProperties();
      bool newValue = !firstDisplayProps->getValue(ImageDisplayProperties::ShowFill).toBool();

      foreach (Image *image, *this) {
        ImageDisplayProperties *displayProps = image->displayProperties();

        bool value = displayProps->getValue(ImageDisplayProperties::ShowFill).toBool();
        results.append(value? "shown" : "hidden");

        image->displayProperties()->setShowFill(newValue);
      }
    }

    return results;
  }


  /**
   * Changes the visibility of the display name of the first image in the image list and synchronizes
   * the visibility of the display names of every other image with the visibility of the display name
   * of the first image.
   *
   * @return @b QStringList A list containing the original visibility of every image's display name.
   */
  QStringList ImageList::saveAndToggleShowLabel() {
    QStringList results;

    if (!isEmpty()) {
      ImageDisplayProperties *firstDisplayProps = first()->displayProperties();
      bool newValue = !firstDisplayProps->getValue(ImageDisplayProperties::ShowLabel).toBool();

      foreach (Image *image, *this) {
        ImageDisplayProperties *displayProps = image->displayProperties();

        bool value = displayProps->getValue(ImageDisplayProperties::ShowLabel).toBool();
        results.append(value? "shown" : "hidden");

        image->displayProperties()->setShowLabel(newValue);
      }
    }

    return results;
  }


  /**
   * Changes the visibility of the outline of the first image in the image list and synchronizes the
   * visibility of the outlines of every other image with the visibility of the outline of the
   * first image.
   *
   * @return @b QStringList A list returning the original visibility of every image's outline.
   */
  QStringList ImageList::saveAndToggleShowOutline() {
    QStringList results;

    if (!isEmpty()) {
      ImageDisplayProperties *firstDisplayProps = first()->displayProperties();
      bool newValue = !firstDisplayProps->getValue(ImageDisplayProperties::ShowOutline).toBool();

      foreach (Image *image, *this) {
        ImageDisplayProperties *displayProps = image->displayProperties();

        bool value = displayProps->getValue(ImageDisplayProperties::ShowOutline).toBool();
        results.append(value? "shown" : "hidden");

        image->displayProperties()->setShowOutline(newValue);
      }
    }

    return results;
  }
}
