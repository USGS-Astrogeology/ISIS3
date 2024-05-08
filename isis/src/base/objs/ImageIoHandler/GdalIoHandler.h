/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef GdalIoHandler_h
#define GdalIoHandler_h

#include "Constants.h"
#include "ImageIoHandler.h"

#include "gdal_priv.h"

class QFile;
class QMutex;
class QString;
template <typename A> class QList;

namespace Isis {
  class Buffer;
  class CubeCachingAlgorithm;
  class Pvl;

  /**
   * @ingroup Low Level Image IO
   *
   * @brief Handles converting buffers to and from disk.
   *
   * This class handles converting buffers to and from disk. This class holds
   *   the cube chunks in memory and is capable of reading and writing them. It
   *   asks the caching algorithms to recommend cube chunks to not keep in
   *   memory. Children need to call setChunkSizes() in their constructor.
   *
   * This class handles all of the virtual band conversions. This class also
   *   guarantees that unwritten cube data ends up read and written as NULLs.
   *   The default caching algorithm is a RegionalCachingAlgorithm.
   *
   * @author 2024-04-30 Adam Paquette
   *
   */
  class GdalIoHandler : public ImageIoHandler {
    public:
      GdalIoHandler(QString &dataFilePath, const QList<int> *virtualBandList, GDALDataType pixelType = GDT_Float64);
      virtual ~GdalIoHandler();

      virtual void read(Buffer &bufferToFill) const;
      virtual void write(const Buffer &bufferToWrite);

      virtual BigInt getDataSize() const;
      /**
       * Function to update the labels with a Pvl object
       *
       * @param labels Pvl object to update with
       */
      virtual void updateLabels(Pvl &labels);

    private:
      void readPixelType(double *doubleBuff, void *rawBuff, int idx) const;
      void writePixelType(double *doubleBuff, void *rawBuff, int idx) const;

      GDALDatasetUniquePtr m_geodataSet = NULL;
      GDALDataType m_pixelType;
      int m_lines;
      int m_samples;
      int m_bands;
      double m_offset;
      double m_scale;
  };
}

#endif
