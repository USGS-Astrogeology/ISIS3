/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QtGlobal>
#include <QCoreApplication>

#include "FileName.h"
#include "IException.h"

#include "EmbreeTargetManager.h"

using namespace std;

namespace Isis {

  /**
   * Initialize the singleton factory pointer
   */
  EmbreeTargetManager *EmbreeTargetManager::m_maker = 0;


  /**
   * This constructor will initialize the EmbreeTargetManager object to default
   * values. The default maximum number of shape models held in memory is 10
   */
  EmbreeTargetManager::EmbreeTargetManager()
    : m_maxCacheSize(10) {
    // This ensures this singleton is shut down when the application exists
    qAddPostRoutine(DieAtExit);
    return;
  }


  /**
   * Destructor that frees all of the EmbreeTargetShapes managed by this object
   */
  EmbreeTargetManager::~EmbreeTargetManager() {
    QList<QString> targetFiles = m_targeCache.keys();
    for (int i = 0; i < targetFiles.size(); i++) {
      removeTargetShape(targetFiles[i]);
    }
  }


  /**
   * @brief Exit termination routine
   *
   * This (static) method ensure this object is destroyed when Qt exits.
   *
   * Note that it is error to add this to the system _atexit() routine because
   * this object utilizes Qt classes.  At the time the atexit call stack is
   * executed, Qt is long gone resulting in Very Bad Things.  Fortunately, Qt has
   * an exit stack function as well.  This method is added to the Qt exit call
   * stack.
   */
  void EmbreeTargetManager::DieAtExit() {
    delete  m_maker;
    m_maker = 0;
    return;
  }


  /**
   * @brief Retrieve reference to Singleton instance of this object
   *
   * The only access provided for Singleton instance of this object. All method
   * access is made through the pointer returned by this method. The object is
   * created upon the first call to this method. The object is deleted when Qt
   * shuts down.
   *
   * @return @b EmbreeTargetManager* Pointer to Singleton instance of this object.
   */
  EmbreeTargetManager *EmbreeTargetManager::getInstance() {
    if (!m_maker) {
      m_maker = new EmbreeTargetManager();
    }
    return (m_maker);
  }


  /**
   * Helper function that takes a file path and returns the full file path.
   *
   * @param filePath The file path to expand.
   *
   * @return @b QString The full file path.
   *
   * @see FileName::Expanded()
   */
  QString EmbreeTargetManager::fullFilePath(const QString &filePath) const {
    FileName fileName(filePath.toStdString());
    return QString::fromStdString(fileName.expanded());
  }


  /**
   * Get a pointer to an EmbreeTargetShape containing the information from a
   * shape file. If the shape file has already been stored in an
   * EmbreeTargetShape then a pointer to that is returned. If the shape file
   * has not been loaded yet, then it is loaded and a pointer to the
   * EmbreeTargetShape is returned.
   *
   * In both cases, ownership of the pointer
   * is not passed. Use EmbreeTargetManager::free to notify the
   * EmbreeTargetManager that the pointer is no longer in use.
   *
   * The EmbreeTargetShapes take a large amount of time to create and memory to
   * store, so the manager limits the number that can be opened at one time. If
   * the limit is reached and a new one is requested, an error is thrown. Use
   * EmbreeTargetManager::setMaxCacheSize to change the maximum number of
   * EmbreeTargetShapes.
   *
   * @param shapeFile The path to the file to create an EmbreeTargetShape from
   *
   * @return @b EmbreeTargetShape* A pointer to the loaded target shape. The
   *                               manager still owns the object so it should
   *                               not be deleted. Use EmbreeTargetManager::free
   *                               to notify the manager that it is no longer
   *                               in use.
   *
   * @see EmbreeTargetManager::free
   *
   * @throws IException::Programmer
   */
  EmbreeTargetShape *EmbreeTargetManager::create(const QString &shapeFile) {
    // Get the full path to the file
    QString fullPath = fullFilePath(shapeFile);

    // If the an EmbreeTargetShape already exists, increment its reference count
    // and return a pointer.
    if ( inCache(fullPath) ) {
      ++(m_targeCache[fullPath].m_referenceCount);
      return ( m_targeCache[fullPath].m_targetShape );
    }

    // Otherwise, make a new EmbreeTargetShape

    // First check how many already exist
    if ( m_targeCache.size() >= maxCacheSize() ) {
      std::string msg = "Failed creating EmbreeTargetShape for [" + shapeFile.toStdString()
                    + "] Too many EmbreeTargetShapes are already open.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // If there's still space make a new one
    EmbreeTargetShape *targetShape = new EmbreeTargetShape(fullPath);
    EmbreeTargetShapeContainer targetShapeContainer(fullPath, targetShape);
    ++(targetShapeContainer.m_referenceCount);
    m_targeCache.insert(fullPath, targetShapeContainer);
    return ( targetShapeContainer.m_targetShape );
  }


  /**
   * Notify the manager that an EmbreeTargetShape is no longer in use. This
   * method will decrease the internal reference count for the target shape.
   * Then if there are not more references. the EmbreeTargetShape is destroyed
   * to free up memory.
   *
   * @param shapeFile The path to the file used to create the EmbreeTargetShape.
   *                  This should be the same as the parameter used with
   *                  EmbreeTargetManager::create to get a pointer to the
   *                  EmbreeTargetShape.
   *
   * @see EmbreeTargetManager::free
   *
   * @throws IException::Programmer
   */
  void EmbreeTargetManager::free(const QString &shapeFile) {
    // Get the full path to the file
    QString fullPath = fullFilePath(shapeFile);

    // Sanity check
    if ( !inCache(fullPath) ) {
      std::string msg = "Cannot free EmbreeTargetShape for file ["
                    + fullPath.toStdString() + "] because it is not stored in the cache.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the container for the shapeFile
    EmbreeTargetShapeContainer targetContainer = m_targeCache[fullPath];

    // Decrement the reference count
    int newCount = --(m_targeCache[fullPath].m_referenceCount);

    // if the EmbreeTargetShape is no longer in use, delete it
    if ( newCount < 1 ) {
      removeTargetShape(fullPath);
    }
  }


  /**
   * Method for removing an EmbreeTargetShape from the internal cache. The
   * EmbreeTargetShapeContainer destructor does not delete the
   * EmbreeTargetShape, so this should be used to free delete anything from the
   * cache.
   *
   * @param shapeFile The path to the shape file used to create the
   *                  EmbreeTargetShape that will be removed.
   *
   * @see EmbreeTargetManager::free
   *
   * @throws IException::Programmer
   */
  void EmbreeTargetManager::removeTargetShape(const QString &shapeFile) {
    // Get the full path to the file
    QString fullPath = fullFilePath(shapeFile);

    // Sanity check
    if ( !inCache(fullPath) ) {
      std::string msg = "Cannot free EmbreeTargetShape for file ["
                    + fullPath.toStdString() + "] because it is not stored in the cache.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the container for the shapeFile
    EmbreeTargetShapeContainer targetContainer = m_targeCache[fullPath];
    delete targetContainer.m_targetShape;
    m_targeCache.remove(fullPath);
  }


  /**
   * Return the number of currently stored EmbreeTargetShapes.
   *
   * @return @b int The number of currently stored EmbreeTargetShapes.
   */
  int EmbreeTargetManager::currentCacheSize() const {
    return m_targeCache.size();
  }


  /**
   * Return the maximum number of stored EmbreeTargetShapes.
   *
   * @return @b int The maximum number of stored EmbreeTargetShapes.
   */
  int EmbreeTargetManager::maxCacheSize() const {
    return m_maxCacheSize;
  }


  /**
   * Set the maximum number of stored EmbreeTargetShapes. This does not apply
   * retroactively. If there are more stored EmbreeTargetShapes than the new
   * maximum, then they will remain and new EmbreeTargetShapes cannot be
   * created until the number currently open goes below the maximum.
   *
   * @param numShapes The new maximum number of stored EmbreeTargetShapes.
   */
  void EmbreeTargetManager::setMaxCacheSize(const int &numShapes) {
    m_maxCacheSize = numShapes;
  }


  /**
   * Check if there is an already created EmbreeTargetShape for a file.
   *
   * @param shapeFile The path to the file to check for
   */
  bool EmbreeTargetManager::inCache(const QString &shapeFile) const{
    return m_targeCache.contains( fullFilePath(shapeFile) );
  }
}
