/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef ImageIoHandler_h
#define ImageIoHandler_h

#include "Constants.h"

class QFile;
class QMutex;
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
   * @author 2011-??-?? Jai Rideout and Steven Lambright
   *
   * @internal
   *   @history 2011-06-27 Steven Lambright - Added addCachingAlgorithm()
   *   @history 2011-07-18 Jai Rideout and Steven Lambright - Added
   *                           unimplemented copy constructor and assignment
   *                           operator.
   *   @history 2011-07-18 Jai Rideout and Steven Lambright - getDataFile() is
   *                           no longer const, along with readRaw() and
   *                           writeRaw().
   *   @history 2011-11-23 Jai Rideout - Added fix to findCubeChunks() and
   *                           findIntersection() so that requested areas that
   *                           are outside of the virtual bands of the cube will
   *                           no longer fail, but instead will fill the buffer
   *                           with nulls.
   *   @history 2012-02-17 Steven Lambright - The read() method is now const -
   *                           the caching should be transparent to the API. It
   *                           is still important to note that you cannot call
   *                           read() and write() simultaneously - Cube is
   *                           guaranteeing this. Backgrounded writes are now
   *                           implemented and fully supported. Timing tests
   *                           show results like this:
   *                           Before:
   *                           User: 358.71 Kernel: 38.29 Total Elapsed: 6:49.83
   *                           After:
   *                           User: 380.01 Kernel: 44.97 Total Elapsed: 6:19.96
   *                             User- user-space CPU time consumed.
   *                             Kernel- kernel-space CPU time consumed.
   *                           The CPU consumption and kernel time consumption
   *                           goes up, but the overall run time is lower. This
   *                           makes the option perfectly viable for typical
   *                           multi-core desktop solutions. Turning off the
   *                           cube write optimization results in equivalent run
   *                           times as before implementation. These time tests
   *                           were run before the adaptive cache flush size was
   *                           added, which improved performance further.
   *                           References #727.
   *   @history 2012-06-06 Jeff Anderson - The read() method was modified to
   *                           improve the speed for small buffer sizes as seen
   *                           in ProcessByBoxcar and ProcessRubbersheet.  The
   *                           internal cache was always checked to be minimized
   *                           which is slow for every read.  Now the cache is
   *                           only mimimized if it changed in size.
   *                           Reference #894.
   *   @history 2014-04-07 Kimberly Oyama and Stuart Sides - Modified the findCubeChunks,
   *                            writeIntoDouble, and writeIntoRaw methods to handle
   *                            repeating virtual bands. Fixes #1927.
   *   @history 2014-09-30 Ian Humphrey - Modified read method to correctly access virtual bands
   *                           when cube dimensions and buffer shape are same size. Fixes #1689.
   *   @history 2015-01-30 Ian Humphrey - Modified destructor to free m_writThreadMutex to
   *                           prevent memory leaks upon destruction. Fixes #2082.
   *   @history 2016-04-21 Makayla Shepherd - Added UnsignedWord pixel type handling.
   *   @history 2016-06-21 Kris Becker - Properly forward declare QPair as struct not class
   *   @history 2016-08-28 Kelvin Rodriguez - updateLabels now a pure virtual, it had no
   *                           implementation causing warnings in clang. Part of OS X 10.11 porting.
   *                           QPair forward declaration now properly claims it as a struct.
   *   @history 2017-09-22 Cole Neubauer - Fixed documentation. References #4807
   *   @history 2018-07-20 Tyler Wilson - Added support for unsigned integer special pixel values.
   *                            in functions writeIntoRaw(...) and writeIntoDouble(...)
   *                            References #971.
   *   @history 2018-08-13 Summer Stapleton - Fixed incoming buffer comparison values for 
   *                            unsigned int type in writeIntoRaw(...). 
   */
  class ImageIoHandler {
    public:
      ImageIoHandler(const QList<int> *virtualBandList);
      virtual ~ImageIoHandler();

      virtual void read(Buffer &bufferToFill) const = 0;
      virtual void write(const Buffer &bufferToWrite) = 0;

      virtual void addCachingAlgorithm(CubeCachingAlgorithm *algorithm);
      virtual void clearCache(bool blockForWriteCache = true) const;
      virtual BigInt getDataSize() const = 0;
      void setVirtualBands(const QList<int> *virtualBandList);
      /**
       * Function to update the labels with a Pvl object
       *
       * @param labels Pvl object to update with
       */
      virtual void updateLabels(Pvl &labels) = 0;
      QMutex *dataFileMutex();

    protected:
      //! This enables us to block while the write thread is working
      QMutex *m_writeThreadMutex;

      //! Converts from virtual band to physical band.
      QList<int> *m_virtualBands;
  };
}

#endif
