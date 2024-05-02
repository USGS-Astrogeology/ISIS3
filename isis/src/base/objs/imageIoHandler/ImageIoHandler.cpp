#include "ImageIoHandler.h"

#include "Buffer.h"
#include "CubeCachingAlgorithm.h"

#include <QList>
#include <QMutex>

using namespace std;

namespace Isis {
  ImageIoHandler::ImageIoHandler(const QList<int> *virtualBandList) {
    m_writeThreadMutex = NULL;
    m_writeThreadMutex = new QMutex;
    m_virtualBands = NULL;

    setVirtualBands(virtualBandList);
  }

  ImageIoHandler::~ImageIoHandler() {
    delete m_virtualBands;
    m_virtualBands = NULL;

    delete m_writeThreadMutex;
    m_writeThreadMutex = NULL;
  }

  void ImageIoHandler::addCachingAlgorithm(CubeCachingAlgorithm *algorithm) {}

  void ImageIoHandler::clearCache(bool blockForWriteCache) const {}

  /**
   * This changes the virtual band list.
   *
   * @param virtualBandList A list where the indices are the vbands and the
   *          values are the physical bands. The values are 1-based. This can
   *          be specified as NULL, in which case the vbands are the physical
   *          bands. The virtual band list is copied
   *          (the pointer provided isn't remembered).
   */
  void ImageIoHandler::setVirtualBands(const QList<int> *virtualBandList) {
    if(m_virtualBands) {
      delete m_virtualBands;
      m_virtualBands = NULL;
    }

    if(virtualBandList && !virtualBandList->empty())
      m_virtualBands = new QList<int>(*virtualBandList);
  }

  /**
   * Get the mutex that this IO handler is using around I/Os on the given
   *   data file. A lock should be acquired before doing any reads/writes on
   *   the data file externally.
   *
   * @return A mutex that can guarantee exclusive access to the data file
   */
  QMutex *ImageIoHandler::dataFileMutex() {
    return m_writeThreadMutex;
  }
}