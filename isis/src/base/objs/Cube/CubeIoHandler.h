/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef CubeIoHandler_h
#define CubeIoHandler_h

#include <QRunnable>
#include <QThreadPool>

#include "Constants.h"
#include "IEndian.h"
#include "PixelType.h"

class QFile;
class QMutex;
class QElapsedTimer;
template <typename A> class QList;
template <typename A, typename B> class QMap;
template <typename A, typename B> struct QPair;

namespace Isis {
  class Buffer;
  class CubeCachingAlgorithm;
  class EndianSwapper;
  class Pvl;
  class RawCubeChunk;

  /**
   * @ingroup Low Level Cube IO
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
  class CubeIoHandler {
    public:
      CubeIoHandler(QFile * dataFile, const QList<int> *virtualBandList,
          const Pvl &label, bool alreadyOnDisk);
      virtual ~CubeIoHandler();

      void read(Buffer &bufferToFill) const;
      void write(const Buffer &bufferToWrite);

      void addCachingAlgorithm(CubeCachingAlgorithm *algorithm);
      void clearCache(bool blockForWriteCache = true) const;
      BigInt getDataSize() const;
      void setVirtualBands(const QList<int> *virtualBandList);
      /**
       * Function to update the labels with a Pvl object
       *
       * @param labels Pvl object to update with
       */
      virtual void updateLabels(Pvl &labels) = 0;

      QMutex *dataFileMutex();
      int bandCount() const;
      int getBandCountInChunk() const;
      BigInt getBytesPerChunk() const;
      int getChunkCountInBandDimension() const;
      int getChunkCountInLineDimension() const;
      int getChunkCountInSampleDimension() const;
      int getChunkIndex(const RawCubeChunk &)  const;
      BigInt getDataStartByte() const;
      int lineCount() const;
      int getLineCountInChunk() const;
      PixelType pixelType() const;
      int sampleCount() const;
      int getSampleCountInChunk() const;

    protected:
      QFile * getDataFile();

      void setChunkSizes(int numSamples, int numLines, int numBands);

      /**
       * This needs to populate the chunkToFill with unswapped raw bytes from
       *   the disk.
       *
       * @see CubeTileHandler::readRaw()
       * @param chunkToFill The container that needs to be filled with cube
       *                    data.
       */
      virtual void readRaw(RawCubeChunk &chunkToFill) = 0;

      /**
       * This needs to write the chunkToWrite directly to disk with no
       *   modifications to the data itself.
       *
       * @see CubeTileHandler::writeRaw()
       * @param chunkToWrite The container that needs to be put on disk.
       */
      virtual void writeRaw(const RawCubeChunk &chunkToWrite) = 0;

    private:
      /**
       * This class is designed to handle write() asynchronously.
       *
       * Only one of these should be running at a time, otherwise
       *   race conditions may exist, so this class locks the
       *   ioHandler->m_writeThreadMutex appropriately.
       *
       * This works by doing what write() used to do, but inside
       *   of run(), so that it can happen simultaneously. If any
       *   read operations occur, they must block until all writes
       *   are done.
       *
       * @author 2012-02-22 Steven Lambright
       *
       * @internal
       */
      class BufferToChunkWriter : public QRunnable {
        public:
          BufferToChunkWriter(CubeIoHandler * ioHandler,
                              QList<Buffer *> buffersToWrite);
          ~BufferToChunkWriter();

          void run();

        private:
          /**
           * This is disabled.
           * @param other Nothing.
           */
          BufferToChunkWriter(const BufferToChunkWriter & other);
          /**
           * This is disabled.
           * @param rhs Nothing.
           * @return Nothing.
           */
          BufferToChunkWriter & operator=(const BufferToChunkWriter & rhs);

        private:
          //! The IO Handler instance to put the buffers into
          CubeIoHandler * m_ioHandler;
          //! The buffers that need pushed into the IO handler; we own these
          QList<Buffer * > * m_buffersToWrite;
          //! Used to calculate the lifetime of an instance of this class
          QElapsedTimer *m_timer;
      };


      /**
       * Disallow copying of this object.
       *
       * @param other The object to copy.
       */
      CubeIoHandler(const CubeIoHandler &other);

      /**
       * Disallow assignments of this object
       *
       * @param other The CubeIoHandler on the right-hand side of the assignment
       *              that we are copying into *this.
       * @return A reference to *this.
       */
      CubeIoHandler &operator=(const CubeIoHandler &other);

      void blockUntilThreadPoolEmpty() const;

      static bool bufferLessThan(Buffer * const &lhs, Buffer * const &rhs);

      QPair< QList<RawCubeChunk *>, QList<int> > findCubeChunks(int startSample, int numSamples,
                                                                int startLine, int numLines,
                                                                int startBand, int numBands) const;

      void findIntersection(const RawCubeChunk &cube1,
          const Buffer &cube2, int &startX, int &startY, int &startZ,
          int &endX, int &endY, int &endZ) const;

      void flushWriteCache(bool force = false) const;

      void freeChunk(RawCubeChunk *chunkToFree) const;

      RawCubeChunk *getChunk(int chunkIndex, bool allocateIfNecessary) const;

      int getChunkCount() const;

      void getChunkPlacement(int chunkIndex,
        int &startSample, int &startLine, int &startBand,
        int &endSample, int &endLine, int &endBand) const;

      RawCubeChunk *getNullChunk(int chunkIndex) const;

      void minimizeCache(const QList<RawCubeChunk *> &justUsed,
                         const Buffer &justRequested) const;

      void synchronousWrite(const Buffer &bufferToWrite);

      void writeIntoDouble(const RawCubeChunk &chunk, Buffer &output, int startIndex) const;

      void writeIntoRaw(const Buffer &buffer, RawCubeChunk &output, int index) const;

      void writeNullDataToDisk() const;

    private:
      //! The file containing cube data.
      QFile * m_dataFile;

      /**
       * The start byte of the cube data. This is 0-based (i.e. a value of 0
       *   means write data into the first byte of the file). Usually the label
       *   is between 0 and m_startByte.
       */
      BigInt m_startByte;

      //! The format of each DN in the cube.
      PixelType m_pixelType;

      //! The additive offset of the data on disk.
      double m_base;

      //! The multiplicative factor of the data on disk.
      double m_multiplier;

      //! The byte order (endianness) of the data on disk.
      ByteOrder m_byteOrder;

      //! A helper that swaps byte order to and from file order.
      EndianSwapper * m_byteSwapper;

      //! The number of samples in the cube.
      int m_numSamples;

      //! The number of lines in the cube.
      int m_numLines;

      //! The number of physical bands in the cube.
      int m_numBands;

      //! The caching algorithms to use, in order of priority.
      QList<CubeCachingAlgorithm *> * m_cachingAlgorithms;

      //! The map from chunk index to chunk for cached data.
      mutable QMap<int, RawCubeChunk *> * m_rawData;

      //! The map from chunk index to on-disk status, all true if not allocated.
      mutable QMap<int, bool> * m_dataIsOnDiskMap;

      //! Converts from virtual band to physical band.
      QList<int> * m_virtualBands;

      //! The number of samples in a cube chunk.
      int m_samplesInChunk;

      //! The number of lines in a cube chunk.
      int m_linesInChunk;

      //! The number of physical bands in a cube chunk.
      int m_bandsInChunk;

      /**
       * This is an optimization for process by line. It relies on chunks found
       *   often being the exact same between multiple reads or multiple writes.
       */
      mutable QList<RawCubeChunk *> *m_lastProcessByLineChunks;

      //! A raw cube chunk's data when it was all NULL's. Used for speed.
      mutable QByteArray *m_nullChunkData;

      //! These are the buffers we need to write to raw cube chunks
      QPair< QMutex *, QList<Buffer *> > *m_writeCache;

      /**
       * This contains threads for doing cube writes (and later maybe cube
       *   reads). We're using a thread pool so that the same QThread is reused
       *   for performance reasons.
       */
      mutable QThreadPool *m_ioThreadPool;

      /**
       * If the last operation was a write then we need to flush the cache when
       *   reading. Keep track of this.
       */
      mutable bool m_lastOperationWasWrite;
      /**
       * This is true if the Isis preference for the cube write thread is
       *   optimized.
       */
      bool m_useOptimizedCubeWrite;

      //! This enables us to block while the write thread is working
      QMutex *m_writeThreadMutex;

      //! Ideal write cache flush size
      mutable volatile int m_idealFlushSize;

      //! How many times the write cache has overflown in a row
      mutable int m_consecutiveOverflowCount;
  };
}

#endif
