/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CubeIoHandler.h"

#include <algorithm>
#include <cmath>
#include <iomanip>

#include <QDebug>
#include <QFile>
#include <QList>
#include <QListIterator>
#include <QMapIterator>
#include <QMutex>
#include <QPair>
#include <QRect>
#include <QElapsedTimer>

#include "Area3D.h"
#include "Brick.h"
#include "CubeCachingAlgorithm.h"
#include "Displacement.h"
#include "Distance.h"
#include "IEndian.h"
#include "EndianSwapper.h"
#include "IException.h"
#include "IString.h"
#include "PixelType.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "RawCubeChunk.h"
#include "RegionalCachingAlgorithm.h"
#include "SpecialPixel.h"
#include "Statistics.h"

using namespace std;

namespace Isis {
  /**
   * Creates a new CubeIoHandler using a RegionalCachingAlgorithm. The chunk
   *   sizes must be set by a child in its constructor.
   *
   * @param dataFile The file that contains cube data. This should be a valid,
   *          opened QFile and may not be NULL. The file should have at least
   *          read permissions.
   * @param virtualBandList A list where the indices are the vbands and the
   *          values are the physical bands. The values are 1-based. This can
   *          be specified as NULL, in which case the vbands are the physical
   *          bands. The virtual band list is copied (the pointer provided isn't
   *          remembered).
   * @param label The label which contains the "Pixels" and "Core" groups.
   * @param alreadyOnDisk True if the cube exists; false ensures all NULLs are
   *          initialized into the file before this object is destructed.
   */
  CubeIoHandler::CubeIoHandler(QFile * dataFile,
      const QList<int> *virtualBandList, const Pvl &label, bool alreadyOnDisk) {
    m_byteSwapper = NULL;
    m_cachingAlgorithms = NULL;
    m_dataIsOnDiskMap = NULL;
    m_rawData = NULL;
    m_virtualBands = NULL;
    m_nullChunkData = NULL;
    m_lastProcessByLineChunks = NULL;
    m_writeCache = NULL;
    m_ioThreadPool = NULL;
    m_writeThreadMutex = NULL;

    try {
      if (!dataFile) {
        IString msg = "Cannot create a CubeIoHandler with a NULL data file";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      m_cachingAlgorithms = new QList<CubeCachingAlgorithm *>;

      PvlGroup &performancePrefs =
          Preference::Preferences().findGroup("Performance");
      IString cubeWritePerfOpt = performancePrefs["CubeWriteThread"][0];
      m_useOptimizedCubeWrite = (cubeWritePerfOpt.DownCase() == "optimized");
      if ((m_useOptimizedCubeWrite && !alreadyOnDisk) ||
           cubeWritePerfOpt.DownCase() == "always") {
        m_ioThreadPool = new QThreadPool;
        m_ioThreadPool->setMaxThreadCount(1);
      }

      m_consecutiveOverflowCount = 0;
      m_lastOperationWasWrite = false;
      m_rawData = new QMap<int, RawCubeChunk *>;
      m_writeCache = new QPair< QMutex *, QList<Buffer *> >;
      m_writeCache->first = new QMutex;
      m_writeThreadMutex = new QMutex;

      m_idealFlushSize = 32;

      m_cachingAlgorithms->append(new RegionalCachingAlgorithm);

      m_dataFile = dataFile;

      const PvlObject &core = label.findObject("IsisCube").findObject("Core");
      const PvlGroup &pixelGroup = core.findGroup("Pixels");

      QString byteOrderStr = pixelGroup.findKeyword("ByteOrder")[0];
      m_byteSwapper = new EndianSwapper(
          byteOrderStr.toUpper());
      m_base = pixelGroup.findKeyword("Base");
      m_multiplier = pixelGroup.findKeyword("Multiplier");
      m_pixelType = PixelTypeEnumeration(pixelGroup.findKeyword("Type"));

      // If the byte swapper isn't going to do anything, then get rid of it
      //   because it's quicker to check for a NULL byte swapper member than to
      //   call a swap that won't do anything.
      if(!m_byteSwapper->willSwap()) {
        delete m_byteSwapper;
        m_byteSwapper = NULL;
      }

      const PvlGroup &dimensions = core.findGroup("Dimensions");
      m_numSamples = dimensions.findKeyword("Samples");
      m_numLines = dimensions.findKeyword("Lines");
      m_numBands = dimensions.findKeyword("Bands");

      m_startByte = (int)core.findKeyword("StartByte") - 1;

      m_samplesInChunk = -1;
      m_linesInChunk = -1;
      m_bandsInChunk = -1;

      if(!alreadyOnDisk) {
        m_dataIsOnDiskMap = new QMap<int, bool>;
      }

      setVirtualBands(virtualBandList);
    }
    catch(IException &e) {
      IString msg = "Constructing CubeIoHandler failed";
      throw IException(e, IException::Programmer, msg, _FILEINFO_);
    }
    catch(...) {
      IString msg = "Constructing CubeIoHandler failed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Cleans up all allocated memory. Child destructors must call clearCache()
   *   because we can no longer do IO by the time this destructor is called.
   */
  CubeIoHandler::~CubeIoHandler() {

    if (m_ioThreadPool)
      m_ioThreadPool->waitForDone();

    delete m_ioThreadPool;
    m_ioThreadPool = NULL;

    delete m_dataIsOnDiskMap;
    m_dataIsOnDiskMap = NULL;

    if (m_cachingAlgorithms) {
      QListIterator<CubeCachingAlgorithm *> it(*m_cachingAlgorithms);
      while (it.hasNext()) {
        delete it.next();
      }
      delete m_cachingAlgorithms;
      m_cachingAlgorithms = NULL;
    }

    if (m_rawData) {
      QMapIterator<int, RawCubeChunk *> it(*m_rawData);
      while (it.hasNext()) {
        // Unwritten data here means it cannot be written :(
        it.next();

        if(it.value())
          delete it.value();
      }
      delete m_rawData;
      m_rawData = NULL;
    }

    if (m_writeCache) {
      delete m_writeCache->first;
      m_writeCache->first = NULL;

      for (int i = 0; i < m_writeCache->second.size(); i++) {
        delete m_writeCache->second[i];
      }
      m_writeCache->second.clear();

      delete m_writeCache;
      m_writeCache = NULL;
    }

    delete m_byteSwapper;
    m_byteSwapper = NULL;

    delete m_virtualBands;
    m_virtualBands = NULL;

    delete m_nullChunkData;
    m_nullChunkData = NULL;

    delete m_lastProcessByLineChunks;
    m_lastProcessByLineChunks = NULL;

    delete m_writeThreadMutex;
    m_writeThreadMutex = NULL;
  }


  /**
   * Read cube data from disk into the buffer.
   *
   * This is not const because it caches the read cube chunks from the
   *   disk.
   *
   * @param bufferToFill The buffer to populate with cube data.
   */
  void CubeIoHandler::read(Buffer &bufferToFill) const {
    // We need to record the current chunk count size so we can use
    // it to evaluate if the cache should be minimized
    int lastChunkCount = m_rawData->size();

    if (m_lastOperationWasWrite) {
      // Do the remaining writes
      flushWriteCache(true);

      m_lastOperationWasWrite = false;

      // Stop backgrounding writes now, we don't want to keep incurring this
      //   penalty.
      if (m_useOptimizedCubeWrite) {
        delete m_ioThreadPool;
        m_ioThreadPool = NULL;
      }
    }

    QMutexLocker lock(m_writeThreadMutex);

    // NON-THREADED CUBE READ
    QList<RawCubeChunk *> cubeChunks;
    QList<int > chunkBands;

    int bufferSampleCount = bufferToFill.SampleDimension();
    int bufferLineCount = bufferToFill.LineDimension();
    int bufferBandCount = bufferToFill.BandDimension();

    // our chunk dimensions are same as buffer shape dimensions
    if (bufferSampleCount == m_samplesInChunk &&
        bufferLineCount == m_linesInChunk &&
        bufferBandCount == m_bandsInChunk) {
      int bufferStartSample = bufferToFill.Sample();
      int bufferStartLine = bufferToFill.Line();
      int bufferStartBand = bufferToFill.Band();

      int bufferEndSample = bufferStartSample + bufferSampleCount - 1;
      int bufferEndLine = bufferStartLine + bufferLineCount - 1;
      int bufferEndBand = bufferStartBand + bufferBandCount - 1;

      // make sure we access the correct band
      int startBand = bufferStartBand - 1;
      if (m_virtualBands)
        startBand = m_virtualBands->at(bufferStartBand - 1);

      int expectedChunkIndex =
          ((bufferStartSample - 1) / getSampleCountInChunk()) +
          ((bufferStartLine - 1) / getLineCountInChunk()) *
            getChunkCountInSampleDimension() +
          ((startBand - 1) / getBandCountInChunk()) *
            getChunkCountInSampleDimension() *
            getChunkCountInLineDimension();

      int chunkStartSample, chunkStartLine, chunkStartBand,
          chunkEndSample, chunkEndLine, chunkEndBand;

      getChunkPlacement(expectedChunkIndex,
          chunkStartSample, chunkStartLine, chunkStartBand,
          chunkEndSample, chunkEndLine, chunkEndBand);

      if (chunkStartSample == bufferStartSample &&
          chunkStartLine == bufferStartLine &&
          chunkStartBand == bufferStartBand &&
          chunkEndSample == bufferEndSample &&
          chunkEndLine == bufferEndLine &&
          chunkEndBand == bufferEndBand) {
        cubeChunks.append(getChunk(expectedChunkIndex, true));
      chunkBands.append(cubeChunks.last()->getStartBand());
      }
    }

    if (cubeChunks.empty()) {
      // We can't guarantee our cube chunks will encompass the buffer
      //   if the buffer goes beyond the cube bounds.
      for(int i = 0; i < bufferToFill.size(); i++) {
        bufferToFill[i] = Null;
      }

    QPair< QList<RawCubeChunk *>, QList<int> > chunkInfo;
      chunkInfo = findCubeChunks(
          bufferToFill.Sample(), bufferToFill.SampleDimension(),
          bufferToFill.Line(), bufferToFill.LineDimension(),
          bufferToFill.Band(), bufferToFill.BandDimension());
      cubeChunks = chunkInfo.first;
      chunkBands = chunkInfo.second;
    }

    for (int i = 0; i < cubeChunks.size(); i++) {
      writeIntoDouble(*cubeChunks[i], bufferToFill, chunkBands[i]);
    }

    // Minimize the cache if it changed in size
    if (lastChunkCount != m_rawData->size()) {
      minimizeCache(cubeChunks, bufferToFill);
    }
  }


  /**
   * Write buffer data into the cube data on disk.
   *
   * This could do no IO if the cube chunks required are already in memory
   *   and our caching algorithms say to not free any of the cube chunks
   *   afterwards.
   *
   * @param bufferToWrite The buffer to get cube data from.
   */
  void CubeIoHandler::write(const Buffer &bufferToWrite) {
    m_lastOperationWasWrite = true;

    if (m_ioThreadPool) {
      // THREADED CUBE WRITE
      Buffer * copy = new Buffer(bufferToWrite);
      {
        QMutexLocker locker(m_writeCache->first);
        m_writeCache->second.append(copy);
      }

      flushWriteCache();
    }
    else {
      QMutexLocker lock(m_writeThreadMutex);
      // NON-THREADED CUBE WRITE
      synchronousWrite(bufferToWrite);
    }
  }


  /**
   * This will add the given caching algorithm to the list of attempted caching
   *   algorithms. The algorithms are tried in the opposite order that they
   *   were added - the first algorithm added is the last algorithm tried.
   *
   * This method takes ownership of algorithm.
   *
   * @param algorithm The caching algorithm to add to the Cube for I/O
   */
  void CubeIoHandler::addCachingAlgorithm(CubeCachingAlgorithm *algorithm) {
    m_cachingAlgorithms->prepend(algorithm);
  }


  /**
   * Free all cube chunks (cached cube data) from memory and write them to
   *   disk. Child destructors need to call this method.
   *
   * This method should only be called otherwise when lots of cubes are in
   *   memory and the many caches cause problems with system RAM limitations.
   *
   * @param blockForWriteCache This should be true unless this method is called
   *                           from the write thread.
   */
  void CubeIoHandler::clearCache(bool blockForWriteCache) const {
    if (blockForWriteCache) {
      // Start the rest of the writes
      flushWriteCache(true);
    }

    // If this map is allocated, then this is a brand new cube and we need to
    //   make sure it's filled with data or NULLs.
    if(m_dataIsOnDiskMap) {
      writeNullDataToDisk();
    }

    // This should be allocated. This is a list of the cached cube data.
    //   Write it all to disk.
    if (m_rawData) {
      QMapIterator<int, RawCubeChunk *> it(*m_rawData);
      while (it.hasNext()) {
        it.next();

        if(it.value()) {
          if(it.value()->isDirty()) {
            (const_cast<CubeIoHandler *>(this))->writeRaw(*it.value());
          }

          delete it.value();
        }
      }

      m_rawData->clear();
    }

    if(m_lastProcessByLineChunks) {
      delete m_lastProcessByLineChunks;
      m_lastProcessByLineChunks = NULL;
    }
  }


  /**
   * @return the number of bytes that the cube DNs will take up. This includes
   *   padding caused by the cube chunks not aligning with the cube dimensions.
   */
  BigInt CubeIoHandler::getDataSize() const {
    return (BigInt)getChunkCountInSampleDimension() *
           (BigInt)getChunkCountInLineDimension() *
           (BigInt)getChunkCountInBandDimension() *
           (BigInt)getBytesPerChunk();
  }


  /**
   * This changes the virtual band list.
   *
   * @param virtualBandList A list where the indices are the vbands and the
   *          values are the physical bands. The values are 1-based. This can
   *          be specified as NULL, in which case the vbands are the physical
   *          bands. The virtual band list is copied
   *          (the pointer provided isn't remembered).
   */
  void CubeIoHandler::setVirtualBands(const QList<int> *virtualBandList) {
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
  QMutex *CubeIoHandler::dataFileMutex() {
    return m_writeThreadMutex;
  }

  /**
   * @return the number of physical bands in the cube.
   */
  int CubeIoHandler::bandCount() const {
    return m_numBands;
  }


  /**
   * @return the number of bands per chunk for this cube.
   */
  int CubeIoHandler::getBandCountInChunk() const {
    return m_bandsInChunk;
  }


  /**
   * @return the byte size of each chunk in the cube. Currently they must be
   *   constant size, but this is planned to be changed at some point in
   *   time.
   *
   * @returns Number of bytes in a cube chunk
   */
  BigInt CubeIoHandler::getBytesPerChunk() const {
    return m_samplesInChunk * m_linesInChunk * m_bandsInChunk *
        SizeOf(m_pixelType);
  }


  /**
   * @return the total number of chunks in the band (Z) dimension. This is
   *   always enough to contain every band in the cube.
   */
  int CubeIoHandler::getChunkCountInBandDimension() const {
    return (int)ceil((double)m_numBands / (double)m_bandsInChunk);
  }


  /**
   * @return the total number of chunks in the line (Y) dimension. This is
   *   always enough to contain every line in the cube.
   */
  int CubeIoHandler::getChunkCountInLineDimension() const {
    return (int)ceil((double)m_numLines / (double)m_linesInChunk);
  }


  /**
   * @return the total number of chunks in the sample (X) dimension. This is
   *   always enough to contain every sample in the cube.
   */
  int CubeIoHandler::getChunkCountInSampleDimension() const {
    return (int)ceil((double)m_numSamples / (double)m_samplesInChunk);
  }


  /**
   * Given a chunk, what's its index in the file. Chunks are ordered from
   *   left to right, then top to bottom, then front to back (BSQ). The
   *   first chunk is at the top left of band 1 and is index 0, for example. In
   *   other words, this is going from the value of m_rawData to the key.
   *
   * Chunks which sit outside of the cube entirely must not be passed into this
   *   method; the results will be wrong.
   *
   * @return The chunk's index into the file
   */
  int CubeIoHandler::getChunkIndex(const RawCubeChunk &chunk)  const {

    int sampleIndex = (chunk.getStartSample() - 1) / getSampleCountInChunk();
    int lineIndex = (chunk.getStartLine() - 1) / getLineCountInChunk();
    int bandIndex = (chunk.getStartBand() - 1) / getBandCountInChunk();

    int indexInBand =
        sampleIndex + lineIndex * getChunkCountInSampleDimension();
    int indexOffsetToBand = bandIndex * getChunkCountInSampleDimension() *
        getChunkCountInLineDimension();

    return indexOffsetToBand + indexInBand;
  }


  /**
   * @return the byte offset to the beginning of the cube data.
   */
  BigInt CubeIoHandler::getDataStartByte() const {
    return m_startByte;
  }


  /**
   * @return the QFile containing cube data. This is what should be read from and
   *   written to.
   *
   * @return The data file for I/O
   */
  QFile * CubeIoHandler::getDataFile() {
    return m_dataFile;
  }


  /**
   * @return the number of lines in the cube. This does not include lines created
   *   by the chunk overflowing the line dimension.
   */
  int CubeIoHandler::lineCount() const {
    return m_numLines;
  }


  /**
   * @return the number of lines in each chunk of the cube.
   */
  int CubeIoHandler::getLineCountInChunk() const {
    return m_linesInChunk;
  }


  /**
   * @return the physical cube DN format.
   */
  PixelType CubeIoHandler::pixelType() const {
    return m_pixelType;
  }


  /**
   * @return the number of samples in the cube. This does not include samples
   *   created by the chunk overflowing the sample dimension.
   */
  int CubeIoHandler::sampleCount() const {
    return m_numSamples;
  }


  /**
   * @return the number of samples in each chunk of the cube.
   */
  int CubeIoHandler::getSampleCountInChunk() const {
    return m_samplesInChunk;
  }


  /**
   * This should be called once from the child constructor. This determines the
   *   chunk sizes used for the cube and often should remain constant for a
   *   particular cube (BSQ, for example, doesn't need it to be constant but
   *   Tile does). These being large can cause excessive use of disk space that
   *   stores no cube data. These being large can also drastically increase the
   *   amount of RAM consumed.
   *
   * @param numSamples The chunk size in the sample dimension
   * @param numLines The chunk size in the line dimension
   * @param numBands The chunk size in the band dimension
   */
  void CubeIoHandler::setChunkSizes(
      int numSamples, int numLines, int numBands) {
    bool success = false;
    IString msg;

    if(m_samplesInChunk != -1 || m_linesInChunk != -1 || m_bandsInChunk != -1) {
      IString msg = "You cannot change the chunk sizes once set";
    }
    else if(numSamples < 1) {
      msg = "Negative and zero chunk sizes are not supported, samples per chunk"
          " cannot be [" + IString(numSamples) + "]";
    }
    else if(numLines < 1) {
      msg = "Negative and zero chunk sizes are not supported, lines per chunk "
            "cannot be [" + IString(numLines) + "]";
    }
    else if(numBands < 1) {
      msg = "Negative and zero chunk sizes are not supported, lines per chunk "
            "cannot be [" + IString(numBands) + "]";
    }
    else {
      success = true;
    }

    if(success) {
      m_samplesInChunk = numSamples;
      m_linesInChunk = numLines;
      m_bandsInChunk = numBands;

      if(m_dataIsOnDiskMap) {
        m_dataFile->resize(getDataStartByte() + getDataSize());
      }
      else if(m_dataFile->size() < getDataStartByte() + getDataSize()) {
        success = false;
        msg = "File size [" + IString((BigInt)m_dataFile->size()) +
            " bytes] not big enough to hold data [" +
            IString(getDataStartByte() + getDataSize()) + " bytes] where the "
            "offset to the cube data is [" + IString(getDataStartByte()) +
            " bytes]";
      }
    }
    else {
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This blocks (doesn't return) until the number of active runnables in the
   *   thread pool goes to 0. This uses the m_writeThreadMutex, because the
   *   thread pool doesn't delete threads immediately, so runnables might
   *   actually still exist but are inconsequential/being deleted already.
   */
  void CubeIoHandler::blockUntilThreadPoolEmpty() const {
    if (m_ioThreadPool) {
      QMutexLocker lock(m_writeThreadMutex);
    }
  }


  /**
   * This is used for sorting buffers into the most efficient write order.
   *
   * @param lhs The left hand side of the '<' operator
   * @param rhs The right hand side of the '<' operator
   * @return True if lhs is obviously earlier in a cube than rhs.
   */
  bool CubeIoHandler::bufferLessThan(Buffer * const &lhs, Buffer * const &rhs) {
    bool lessThan = false;

    // If there is any overlap then we need to return false due to it being
    //   ambiguous.
    Area3D lhsArea(
        Displacement(lhs->Sample(), Displacement::Pixels),
        Displacement(lhs->Line(), Displacement::Pixels),
        Displacement(lhs->Band(), Displacement::Pixels),
        Distance(lhs->SampleDimension() - 1, Distance::Pixels),
        Distance(lhs->LineDimension() - 1, Distance::Pixels),
        Distance(lhs->BandDimension() - 1, Distance::Pixels));
    Area3D rhsArea(
        Displacement(rhs->Sample(), Displacement::Pixels),
        Displacement(rhs->Line(), Displacement::Pixels),
        Displacement(rhs->Band(), Displacement::Pixels),
        Distance(rhs->SampleDimension() - 1, Distance::Pixels),
        Distance(rhs->LineDimension() - 1, Distance::Pixels),
        Distance(rhs->BandDimension() - 1, Distance::Pixels));

    if (!lhsArea.intersect(rhsArea).isValid()) {
      if (lhs->Band() != rhs->Band()) {
        lessThan = lhs->Band() < rhs->Band();
      }
      else if (lhs->Line() != rhs->Line()) {
        lessThan = lhs->Line() < rhs->Line();
      }
      else if (lhs->Sample() != rhs->Sample()) {
        lessThan = lhs->Sample() < rhs->Sample();
      }
    }

    return lessThan;
  }


  /**
   * Get the cube chunks that correspond to the given cube area.
   *   This will create and initialize the chunks if they are not already in
   *   memory.
   *
   * @param startSample The starting sample of the cube data
   * @param numSamples The number of samples of cube data
   * @param startLine The starting line of the cube data
   * @param numLines The number of lines of cube data
   * @param startBand The starting band of the cube data
   * @param numBands The number of bands of cube data
   * @return The cube chunks that correspond to the given cube area
   */
  QPair< QList<RawCubeChunk *>, QList<int> > CubeIoHandler::findCubeChunks(int startSample,
      int numSamples, int startLine, int numLines, int startBand,
      int numBands) const {
    QList<RawCubeChunk *> results;
    QList<int> resultBands;
/************************************************************************CHANGED THIS!!!!!!!!******/
    int lastBand = startBand + numBands - 1;
//     int lastBand = min(startBand + numBands - 1,
//                        bandCount());
/************************************************************************CHANGED THIS!!!!!!!!******/

    QRect areaInBand(
        QPoint(max(startSample, 1),
                max(startLine, 1)),
        QPoint(min(startSample + numSamples - 1,
                   sampleCount()),
                min(startLine + numLines - 1,
                    lineCount())));

    // We are considering only 1 band at a time.. we can't use m_bandsInChunk
    //   because of virtual bands, but every extra loop should not need extra
    //   IO.
    for(int band = startBand; band <= lastBand; band ++) {
      // This is the user-requested area in this band
      QRect areaLeftInBand(areaInBand);

      int actualBand = band;

      if(m_virtualBands) {
        if (band < 1 || band > m_virtualBands->size())
          actualBand = 0;
        else
          actualBand = (m_virtualBands->at(band - 1) - 1) / m_bandsInChunk + 1;
      }
      // We will be consuming areaLeftInBand until we've got all of the area
      //   requested.
      while(!areaLeftInBand.isEmpty()) {
        /**
        *
        * The user has requested a slice of a cube (a rectangle):
        *    -----------------------
        *    |         Cube        |
        *    |                     |
        *    |   --------          |
        *    |   |      | <------------ Requested Area
        *    |   |      |          |
        *    |   --------          |
        *    |                     |
        *    -----------------------
        *
        * But our cube is split into "chunks" which may not line up to what
        *   was requested:
        *
        *    ------------------------
        *    |       |       |     ||
        *    |---------------------||
        *    |   --------    |     ||
        *    |---|------|----------|| (User requested area intersected with
        *    |   |   |  |    |     ||  cube chunks)
        *    |---------------------||
        *    |       |       |     ||
        *    ------------------------
        *
        *    ------------------------
        *    |   1   |   2   |  3  ||
        *    |---------------------||
        *    |   4   |   5   |  6  ||
        *    |---------------------|| (User requested area intersected with
        *    |   7   |   8   |  9  ||  cube chunks)
        *    |---------------------||
        *    |  10   |  11   | 12  ||
        *    ------------------------
        *
        * (In this case we want chunks 4, 5, 7, and 8)
        *
        * So we want to find which cube chunks line up to the requested area.
        *   To do this, we find which chunk intersects the top left of the
        *   requested cube area. That chunk is a given that we need it. Then
        *   we search in the positive X direction for more chunks that
        *   intersect the user-requested area until we no longer do. Then we
        *   know all of the chunks that intersect a whole y-range of the
        *   requested area. We shrink the requested area (remove the found
        *   y-range) and repeat.
        */
        int areaStartLine = areaLeftInBand.top();
        int areaStartSample = areaLeftInBand.left();

        int initialChunkXPos = (areaStartSample - 1) / m_samplesInChunk;
        int initialChunkYPos = (areaStartLine - 1)   / m_linesInChunk;
        int initialChunkZPos = (actualBand - 1) / m_bandsInChunk;
        int initialChunkBand = initialChunkZPos * m_bandsInChunk + 1;


        QRect chunkRect(initialChunkXPos * m_samplesInChunk + 1,
                      initialChunkYPos * m_linesInChunk + 1,
                      m_samplesInChunk, m_linesInChunk);

        // The chunk rectangle must intersect the remaining area that is in the
        // current band, and the chunk's initial band must be between 1 and the
        // number of physical bands in the cube (inclusive).
        while(chunkRect.intersects(areaLeftInBand) &&
              (initialChunkBand >= 1 && initialChunkBand <= bandCount())) {
          int chunkXPos = (chunkRect.left() - 1) / m_samplesInChunk;
          int chunkYPos = (chunkRect.top() - 1) / m_linesInChunk;
          int chunkZPos = initialChunkZPos;

          // We now have an X,Y,Z position for the chunk. What's its index?
          int chunkIndex = chunkXPos +
              (chunkYPos * getChunkCountInSampleDimension()) +
              (chunkZPos * getChunkCountInSampleDimension() *
                          getChunkCountInLineDimension());

          RawCubeChunk * newChunk = getChunk(chunkIndex, true);

          results.append(newChunk);
          resultBands.append(band);

          chunkRect.moveLeft(chunkRect.right() + 1);
        }

        areaLeftInBand.setTop(chunkRect.bottom() + 1);
      }
    }

    return QPair< QList<RawCubeChunk *>, QList<int> >(results, resultBands);
  }


  /**
   * Find the intersection between the buffer area and the cube chunk. This
   *   accounts for virtual bands when considering the buffer and may return
   *   an area where all of the bands don't actually intersect due to virtual
   *   bands. This is implemented the way it is for performance reasons -
   *   so much as returning an Area3D or a QList<int> caused undesirable
   *   slowdowns.
   *
   * @param cube1 The cube chunk to intersect
   * @param cube2 The buffer (in virtual band space) to intersect
   * @param startX (output) The leftmost sample position (inclusive)
   * @param startY (output) The topmost line position (inclusive)
   * @param startZ (output) The frontmost band position (inclusive)
   * @param endX (output) The rightmost sample position (inclusive)
   * @param endY (output) The bottommost line position (inclusive)
   * @param endZ (output) The backmost band position (inclusive)
   */
  void CubeIoHandler::findIntersection(
      const RawCubeChunk &cube1, const Buffer &cube2,
      int &startX, int &startY, int &startZ,
      int &endX, int &endY, int &endZ) const {
    // So we have 2 3D "cubes" (not Cube cubes but 3d areas) we need to
    //   intersect in order to figure out what chunk data goes into the output
    //   buffer.

    // To find the band range we actually have to map all of the bands from
    //   virtual bands to physical bands
    int startVBand = cube2.Band();
    int endVBand = startVBand + cube2.BandDimension() - 1;

    int startPhysicalBand = 0;
    int endPhysicalBand = 0;

    bool startVBandFound = false;
    for(int virtualBand = startVBand; virtualBand <= endVBand; virtualBand ++) {
      int physicalBand = virtualBand;

      bool bandExists = true;
      if(m_virtualBands) {
        if (virtualBand < 1 || virtualBand > m_virtualBands->size())
          bandExists = false;
        else {
          physicalBand = m_virtualBands->at(virtualBand - 1);
        }
      }

      if (bandExists) {
        if(!startVBandFound) {
          startPhysicalBand = physicalBand;
          endPhysicalBand = physicalBand;
          startVBandFound = true;
        }
        else {
          if(physicalBand < startPhysicalBand)
            startPhysicalBand = physicalBand;

          if(physicalBand > endPhysicalBand)
            endPhysicalBand = physicalBand;
        }
      }
    }

    startX = max(cube1.getStartSample(), cube2.Sample());
    startY = max(cube1.getStartLine(), cube2.Line());
    startZ = max(cube1.getStartBand(), startPhysicalBand);
    endX = min(cube1.getStartSample() + cube1.sampleCount() - 1,
               cube2.Sample() + cube2.SampleDimension() - 1);
    endY = min(cube1.getStartLine() + cube1.lineCount() - 1,
               cube2.Line() + cube2.LineDimension() - 1);
    endZ = min(cube1.getStartBand() + cube1.bandCount() - 1,
               endPhysicalBand);
  }


  /**
   * This attempts to write the so-far-unwritten buffers from m_writeCache
   *   into the cube's RawCubeChunk cache. This will not do anything if
   *   there is a runnable and will block if the write cache grows to
   *   be too large.
   *
   * @param force Set to true to force start a flush
   */
  void CubeIoHandler::flushWriteCache(bool force) const {
    if (m_ioThreadPool) {
      bool shouldFlush = m_writeCache->second.size() >= m_idealFlushSize ||
                         force;
      bool cacheOverflowing =
          (m_writeCache->second.size() > m_idealFlushSize * 10);
      bool shouldAndCanFlush = false;
      bool forceStart = force;

      if (shouldFlush) {
        shouldAndCanFlush = m_writeThreadMutex->tryLock();
        if (shouldAndCanFlush) {
          m_writeThreadMutex->unlock();
        }
      }

      if (cacheOverflowing && !shouldAndCanFlush) {
        forceStart = true;
        m_consecutiveOverflowCount++;
      }

      if (forceStart) {
        blockUntilThreadPoolEmpty();

        if (m_writeCache->second.size() != 0) {
          m_idealFlushSize = m_writeCache->second.size();
          shouldFlush = true;
          shouldAndCanFlush = true;
        }
      }
      else if (!cacheOverflowing && shouldAndCanFlush) {
        m_consecutiveOverflowCount = 0;
      }

      if (cacheOverflowing && m_useOptimizedCubeWrite) {
        blockUntilThreadPoolEmpty();

        // If the process is very I/O bound, then write caching isn't helping
        //   anything. In fact, it hurts, so turn it off.
        if (m_consecutiveOverflowCount > 10) {
          delete m_ioThreadPool;
          m_ioThreadPool = NULL;
        }

        // Write it all synchronously.
        foreach (Buffer *bufferToWrite, m_writeCache->second) {
          const_cast<CubeIoHandler *>(this)->synchronousWrite(*bufferToWrite);
          delete bufferToWrite;
        }

        m_writeCache->second.clear();
      }

      if (shouldAndCanFlush && m_ioThreadPool) {
        QMutexLocker locker(m_writeCache->first);
        QRunnable *writer = new BufferToChunkWriter(
            const_cast<CubeIoHandler *>(this), m_writeCache->second);

        m_ioThreadPool->start(writer);

        m_writeCache->second.clear();
        m_lastOperationWasWrite = true;
      }

      if (force) {
        blockUntilThreadPoolEmpty();
      }
    }
  }


  /**
   * If the chunk is dirty, then we write it to disk. Regardless, we then
   *   free it from memory.
   *
   * @param chunkToFree The chunk we're removing from memory
   */
  void CubeIoHandler::freeChunk(RawCubeChunk *chunkToFree) const {
    if(chunkToFree && m_rawData) {
      int chunkIndex = getChunkIndex(*chunkToFree);

      m_rawData->erase(m_rawData->find(chunkIndex));

      if(chunkToFree->isDirty())
        (const_cast<CubeIoHandler *>(this))->writeRaw(*chunkToFree);

      delete chunkToFree;

      if(m_lastProcessByLineChunks) {
        delete m_lastProcessByLineChunks;
        m_lastProcessByLineChunks = NULL;
      }
    }
  }


  /**
   * Retrieve the cached chunk at the given chunk index, if there is one.
   *
   * @param chunkIndex The position of the chunk in the cube
   * @param allocateIfNecessary If true, the chunk will be read into cache
   *                            when necessary and this method will not return
   *                            NULL.
   * @return NULL if data is not cached, otherwise the cube file data
   */
  RawCubeChunk *CubeIoHandler::getChunk(int chunkIndex,
                                        bool allocateIfNecessary) const {
    RawCubeChunk *chunk = NULL;

    if(m_rawData) {
      chunk = m_rawData->value(chunkIndex);
    }

    if(allocateIfNecessary && !chunk) {
      if(m_dataIsOnDiskMap && !(*m_dataIsOnDiskMap)[chunkIndex]) {
        chunk = getNullChunk(chunkIndex);
        (*m_dataIsOnDiskMap)[chunkIndex] = true;
      }
      else {
        int startSample;
        int startLine;
        int startBand;
        int endSample;
        int endLine;
        int endBand;
        getChunkPlacement(chunkIndex, startSample, startLine, startBand,
                          endSample, endLine, endBand);
        chunk = new RawCubeChunk(startSample, startLine, startBand,
                                    endSample, endLine, endBand,
                                    getBytesPerChunk());

        (const_cast<CubeIoHandler *>(this))->readRaw(*chunk);
        chunk->setDirty(false);
      }

      (*m_rawData)[chunkIndex] = chunk;
    }

    return chunk;
  }


  /**
   * @return The number of chunks that are required to encapsulate all of the
   *   cube data
   */
  int CubeIoHandler::getChunkCount() const {
    return getChunkCountInSampleDimension() *
           getChunkCountInLineDimension() *
           getChunkCountInBandDimension();
  }


  /**
   * Get the X/Y/Z (Sample,Line,Band) range of the chunk at the given index.
   *
   * @param chunkIndex The chunk number in the cube file
   * @param startSample (output) The leftmost sample position (inclusive)
   * @param startLine (output) The topmost line position (inclusive)
   * @param startBand (output) The frontmost band position (inclusive)
   * @param endSample (output) The rightmost sample position (inclusive)
   * @param endLine (output) The bottommost line position (inclusive)
   * @param endBand (output) The backmost band position (inclusive)
   */
  void CubeIoHandler::getChunkPlacement(int chunkIndex,
      int &startSample, int &startLine, int &startBand,
      int &endSample, int &endLine, int &endBand) const {
    int chunkSampleIndex = chunkIndex % getChunkCountInSampleDimension();

    chunkIndex =
        (chunkIndex - chunkSampleIndex) / getChunkCountInSampleDimension();

    int chunkLineIndex = chunkIndex % getChunkCountInLineDimension();
    chunkIndex = (chunkIndex - chunkLineIndex) / getChunkCountInLineDimension();

    int chunkBandIndex = chunkIndex;

    startSample = chunkSampleIndex * getSampleCountInChunk() + 1;
    endSample = startSample + getSampleCountInChunk() - 1;
    startLine = chunkLineIndex * getLineCountInChunk() + 1;
    endLine = startLine + getLineCountInChunk() - 1;
    startBand = chunkBandIndex * getBandCountInChunk() + 1;
    endBand = startBand + getBandCountInChunk() - 1;
  }


  /**
   * This creates a chunk filled with NULLs whose placement is at chunkIndex's
   *   position. This is used for getting NULL-filled chunks the first time
   *   the chunk is requested.
   *
   * Ownership of the return value is given to the caller.
   *
   * @param chunkIndex The chunk's index which provides it's positioning.
   * @return A chunk filled with nulls at the chunkIndex position
   */
  RawCubeChunk *CubeIoHandler::getNullChunk(int chunkIndex) const {
    // Shouldn't ask for null chunks when the area has already been allocated

    int startSample = 0;
    int startLine = 0;
    int startBand = 0;

    int endSample = 0;
    int endLine = 0;
    int endBand = 0;

    getChunkPlacement(chunkIndex, startSample, startLine, startBand,
                      endSample, endLine, endBand);

    RawCubeChunk *result = new RawCubeChunk(startSample, startLine, startBand,
        endSample, endLine, endBand, getBytesPerChunk());

    if(!m_nullChunkData) {
      // the pixel type doesn't really matter, so pick something small
      Brick nullBuffer(result->sampleCount(),
                      result->lineCount(),
                      result->bandCount(),
                      UnsignedByte);

      nullBuffer.SetBasePosition(result->getStartSample(),
                            result->getStartLine(),
                            result->getStartBand());
      for(int i = 0; i < nullBuffer.size(); i++) {
        nullBuffer[i] = Null;
      }

      writeIntoRaw(nullBuffer, *result, result->getStartBand());
      m_nullChunkData = new QByteArray(result->getRawData());
    }
    else {
      result->setRawData(*m_nullChunkData);
    }

    result->setDirty(true);
    return result;
  }


  /**
   * Apply the caching algorithms and get rid of excess cube data in memory.
   *   This is intended to be called after every IO operation.
   *
   * @param justUsed The cube chunks that were used in the IO operation that
   *     is calling this method.
   * @param justRequested The buffer that was used in the IO operation that
   *     is calling this method.
   */
  void CubeIoHandler::minimizeCache(const QList<RawCubeChunk *> &justUsed,
                                    const Buffer &justRequested) const {
    // Since we have a lock on the cache, no newly created threads can utilize
    //   or access any cache data until we're done.
    if (m_rawData->size() * getBytesPerChunk() > 1 * 1024 * 1024 ||
       m_cachingAlgorithms->size() > 1) {
      bool algorithmAccepted = false;

      int algorithmIndex = 0;
      while(!algorithmAccepted &&
            algorithmIndex < m_cachingAlgorithms->size()) {
        CubeCachingAlgorithm *algorithm = (*m_cachingAlgorithms)[algorithmIndex];

        CubeCachingAlgorithm::CacheResult result =
            algorithm->recommendChunksToFree(m_rawData->values(), justUsed,
                                             justRequested);

        algorithmAccepted = result.algorithmUnderstoodData();

        if(algorithmAccepted) {
          QList<RawCubeChunk *> chunksToFree = result.getChunksToFree();

          RawCubeChunk *chunkToFree;
          foreach(chunkToFree, chunksToFree) {
            freeChunk(chunkToFree);
          }
        }

        algorithmIndex ++;
      }

      // Fall back - no algorithms liked us :(
      if(!algorithmAccepted && m_rawData->size() > 100) {
        // This (minimizeCache()) is typically executed in the Runnable thread.
        // We don't want to wait for ourselves.
        clearCache(false);
      }
    }
  }


  /**
   * This method takes the given buffer and synchronously puts it into the
   *   Cube's cache. This includes reading missing cache areas and freeing
   *   extra cache areas. This is what the non-threaded CubeIoHandler::write
   *   used to do.
   *
   * @param bufferToWrite The buffer we're writing into this cube, synchronously
   */
  void CubeIoHandler::synchronousWrite(const Buffer &bufferToWrite) {
    QList<RawCubeChunk *> cubeChunks;
    QList<int> cubeChunkBands;

    int bufferSampleCount = bufferToWrite.SampleDimension();
    int bufferLineCount = bufferToWrite.LineDimension();
    int bufferBandCount = bufferToWrite.BandDimension();

    // process by line optimization
    if(m_lastProcessByLineChunks &&
       m_lastProcessByLineChunks->size()) {
      // Not optimized yet, let's see if we can optimize
      if(bufferToWrite.Sample() == 1 &&
         bufferSampleCount == sampleCount() &&
         bufferLineCount == 1 &&
         bufferBandCount == 1) {
        // We look like a process by line, are we using the same chunks as
        //   before?
        int bufferLine = bufferToWrite.Line();
        int chunkStartLine =
            (*m_lastProcessByLineChunks)[0]->getStartLine();
        int chunkLines =
            (*m_lastProcessByLineChunks)[0]->lineCount();
        int bufferBand = bufferToWrite.Band();
        int chunkStartBand =
            (*m_lastProcessByLineChunks)[0]->getStartBand();
        int chunkBands =
            (*m_lastProcessByLineChunks)[0]->bandCount();

        if(bufferLine >= chunkStartLine &&
           bufferLine <= chunkStartLine + chunkLines - 1 &&
           bufferBand >= chunkStartBand &&
           bufferBand <= chunkStartBand + chunkBands - 1) {
          cubeChunks = *m_lastProcessByLineChunks;
          for (int i = 0; i < cubeChunks.size(); i++) {
            cubeChunkBands.append( cubeChunks[i]->getStartBand() );
          }
        }
      }
    }
    // Processing by chunk size
    else if (bufferSampleCount == m_samplesInChunk &&
             bufferLineCount == m_linesInChunk &&
             bufferBandCount == m_bandsInChunk) {
      int bufferStartSample = bufferToWrite.Sample();
      int bufferStartLine = bufferToWrite.Line();
      int bufferStartBand = bufferToWrite.Band();

      int bufferEndSample = bufferStartSample + bufferSampleCount - 1;
      int bufferEndLine = bufferStartLine + bufferLineCount - 1;
      int bufferEndBand = bufferStartBand + bufferBandCount - 1;

      int expectedChunkIndex =
          ((bufferStartSample - 1) / getSampleCountInChunk()) +
          ((bufferStartLine - 1) / getLineCountInChunk()) *
            getChunkCountInSampleDimension() +
          ((bufferStartBand - 1) / getBandCountInChunk()) *
            getChunkCountInSampleDimension() *
            getChunkCountInLineDimension();

      int chunkStartSample, chunkStartLine, chunkStartBand,
          chunkEndSample, chunkEndLine, chunkEndBand;

      getChunkPlacement(expectedChunkIndex,
          chunkStartSample, chunkStartLine, chunkStartBand,
          chunkEndSample, chunkEndLine, chunkEndBand);

      if (chunkStartSample == bufferStartSample &&
          chunkStartLine == bufferStartLine &&
          chunkStartBand == bufferStartBand &&
          chunkEndSample == bufferEndSample &&
          chunkEndLine == bufferEndLine &&
          chunkEndBand == bufferEndBand) {
        cubeChunks.append(getChunk(expectedChunkIndex, true));
        cubeChunkBands.append(cubeChunks.last()->getStartBand());
      }
    }

    QPair< QList<RawCubeChunk *>, QList<int> > chunkInfo;
    if(cubeChunks.empty()) {
      chunkInfo = findCubeChunks(
         bufferToWrite.Sample(), bufferSampleCount,
         bufferToWrite.Line(), bufferLineCount,
         bufferToWrite.Band(), bufferBandCount);
      cubeChunks = chunkInfo.first;
      cubeChunkBands = chunkInfo.second;
    }

    // process by line optimization
    if(bufferToWrite.Sample() == 1 &&
        bufferSampleCount == sampleCount() &&
        bufferLineCount == 1 &&
        bufferBandCount == 1) {
      if(!m_lastProcessByLineChunks)
        m_lastProcessByLineChunks =
            new QList<RawCubeChunk *>(cubeChunks);
      else
        *m_lastProcessByLineChunks = cubeChunks;
    }

    for(int i = 0; i < cubeChunks.size(); i++) {
      writeIntoRaw(bufferToWrite, *cubeChunks[i], cubeChunkBands[i]);
    }

    minimizeCache(cubeChunks, bufferToWrite);
  }


  /**
   * Write the intersecting area of the chunk into the buffer.
   *
   * @param chunk The data source
   * @param output The data destination
   * @param index int
   */
  void CubeIoHandler::writeIntoDouble(const RawCubeChunk &chunk,
                                      Buffer &output, int index) const {
    // The code in this method is highly optimized. Even the order of the if
    //   statements will have a significant impact on performance if changed.
    //   Also, there is a lot of duplicate code in both writeIntoDouble(...) and
    //   writeIntoRaw(...). This is needed for performance gain. Any function
    //   or method calls from within the x loop cause significant performance
    //   decreases.
    int startX = 0;
    int startY = 0;
    int startZ = 0;

    int endX = 0;
    int endY = 0;
    int endZ = 0;

    findIntersection(chunk, output, startX, startY, startZ, endX, endY, endZ);

    int bufferBand = output.Band();
    int bufferBands = output.BandDimension();
    int chunkStartSample = chunk.getStartSample();
    int chunkStartLine = chunk.getStartLine();
    int chunkStartBand = chunk.getStartBand();
    int chunkLineSize = chunk.sampleCount();
    int chunkBandSize = chunkLineSize * chunk.lineCount();
    //double *buffersDoubleBuf = output.p_buf;
    double *buffersDoubleBuf = output.DoubleBuffer();
    const char *chunkBuf = chunk.getRawData().data();
    char *buffersRawBuf = (char *)output.RawBuffer();

    for(int z = startZ; z <= endZ; z++) {
      const int &bandIntoChunk = z - chunkStartBand;
      int virtualBand = z;

      virtualBand = index;

      if(virtualBand != 0 && virtualBand >= bufferBand &&
         virtualBand <= bufferBand + bufferBands - 1) {

        for(int y = startY; y <= endY; y++) {
          const int &lineIntoChunk = y - chunkStartLine;
          int bufferIndex = output.Index(startX, y, virtualBand);

          for(int x = startX; x <= endX; x++) {
            const int &sampleIntoChunk = x - chunkStartSample;

            const int &chunkIndex = sampleIntoChunk +
                (chunkLineSize * lineIntoChunk) +
                (chunkBandSize * bandIntoChunk);

            double &bufferVal = buffersDoubleBuf[bufferIndex];

            if(m_pixelType == Real) {
              float raw = ((float *)chunkBuf)[chunkIndex];
              if(m_byteSwapper)
                raw = m_byteSwapper->Float(&raw);

              if(raw >= VALID_MIN4) {
                bufferVal = (double) raw;
              }
              else {
                if(raw == NULL4)
                  bufferVal = NULL8;
                else if(raw == LOW_INSTR_SAT4)
                  bufferVal = LOW_INSTR_SAT8;
                else if(raw == LOW_REPR_SAT4)
                  bufferVal = LOW_REPR_SAT8;
                else if(raw == HIGH_INSTR_SAT4)
                  bufferVal = HIGH_INSTR_SAT8;
                else if(raw == HIGH_REPR_SAT4)
                  bufferVal = HIGH_REPR_SAT8;
                else
                  bufferVal = LOW_REPR_SAT8;
              }

              ((float *)buffersRawBuf)[bufferIndex] = raw;
            }

            else if(m_pixelType == SignedWord) {
              short raw = ((short *)chunkBuf)[chunkIndex];
              if(m_byteSwapper)
                raw = m_byteSwapper->ShortInt(&raw);

              if(raw >= VALID_MIN2) {
                bufferVal = (double) raw * m_multiplier + m_base;
              }
              else {
                if(raw == NULL2)
                  bufferVal = NULL8;
                else if(raw == LOW_INSTR_SAT2)
                  bufferVal = LOW_INSTR_SAT8;
                else if(raw == LOW_REPR_SAT2)
                  bufferVal = LOW_REPR_SAT8;
                else if(raw == HIGH_INSTR_SAT2)
                  bufferVal = HIGH_INSTR_SAT8;
                else if(raw == HIGH_REPR_SAT2)
                  bufferVal = HIGH_REPR_SAT8;
                else
                  bufferVal = LOW_REPR_SAT8;
              }

              ((short *)buffersRawBuf)[bufferIndex] = raw;
            }


            else if(m_pixelType == UnsignedWord) {
              unsigned short raw = ((unsigned short *)chunkBuf)[chunkIndex];
              if(m_byteSwapper)
                raw = m_byteSwapper->UnsignedShortInt(&raw);

              if(raw >= VALID_MINU2) {
                bufferVal = (double) raw * m_multiplier + m_base;
              }
              else if (raw > VALID_MAXU2) {
                if(raw == HIGH_INSTR_SATU2)
                  bufferVal = HIGH_INSTR_SAT8;
                else if(raw == HIGH_REPR_SATU2)
                  bufferVal = HIGH_REPR_SAT8;
                else
                  bufferVal = LOW_REPR_SAT8;
              }
              else {
                if(raw == NULLU2)
                  bufferVal = NULL8;
                else if(raw == LOW_INSTR_SATU2)
                  bufferVal = LOW_INSTR_SAT8;
                else if(raw == LOW_REPR_SATU2)
                  bufferVal = LOW_REPR_SAT8;
                else
                  bufferVal = LOW_REPR_SAT8;
              }

              ((unsigned short *)buffersRawBuf)[bufferIndex] = raw;
            }

            else if(m_pixelType == UnsignedInteger) {

              unsigned int raw = ((unsigned int *)chunkBuf)[chunkIndex];
              if(m_byteSwapper)
                raw = m_byteSwapper->Uint32_t(&raw);

              if(raw >= VALID_MINUI4) {
                bufferVal = (double) raw * m_multiplier + m_base;
              }
              else if (raw > VALID_MAXUI4) {
                if(raw == HIGH_INSTR_SATUI4)
                  bufferVal = HIGH_INSTR_SAT8;
                else if(raw == HIGH_REPR_SATUI4)
                  bufferVal = HIGH_REPR_SAT8;
                else
                  bufferVal = LOW_REPR_SAT8;
              }
              else {
                if(raw == NULLUI4)
                  bufferVal = NULL8;
                else if(raw == LOW_INSTR_SATUI4)
                  bufferVal = LOW_INSTR_SAT8;
                else if(raw == LOW_REPR_SATUI4)
                  bufferVal = LOW_REPR_SAT8;
                else
                  bufferVal = LOW_REPR_SAT8;
              }

              ((unsigned int *)buffersRawBuf)[bufferIndex] = raw;



            }

            else if(m_pixelType == UnsignedByte) {
              unsigned char raw = ((unsigned char *)chunkBuf)[chunkIndex];

              if(raw == NULL1) {
                bufferVal = NULL8;
              }
              else if(raw == HIGH_REPR_SAT1) {
                bufferVal = HIGH_REPR_SAT8;
              }
              else {
                bufferVal = (double) raw * m_multiplier + m_base;
              }

              ((unsigned char *)buffersRawBuf)[bufferIndex] = raw;
            }

            bufferIndex ++;
          }
        }
      }
    }
  }


  /**
   * Write the intersecting area of the buffer into the chunk.
   *
   * @param buffer The data source
   * @param output The data destination
   * @param index int
   */
  void CubeIoHandler::writeIntoRaw(const Buffer &buffer, RawCubeChunk &output, int index)
      const {
    // The code in this method is highly optimized. Even the order of the if
    //   statements will have a significant impact on performance if changed.
    //   Also, there is a lot of duplicate code in both writeIntoDouble(...) and
    //   writeIntoRaw(...). This is needed for performance gain. Any function
    //   or method calls from within the x loop cause significant performance
    //   decreases.
    int startX = 0;
    int startY = 0;
    int startZ = 0;

    int endX = 0;
    int endY = 0;
    int endZ = 0;

    output.setDirty(true);
    findIntersection(output, buffer, startX, startY, startZ, endX, endY, endZ);

    int bufferBand = buffer.Band();
    int bufferBands = buffer.BandDimension();
    int outputStartSample = output.getStartSample();
    int outputStartLine = output.getStartLine();
    int outputStartBand = output.getStartBand();
    int lineSize = output.sampleCount();
    int bandSize = lineSize * output.lineCount();
    double *buffersDoubleBuf = buffer.DoubleBuffer();
    char *chunkBuf = output.getRawData().data();

    for(int z = startZ; z <= endZ; z++) {
      const int &bandIntoChunk = z - outputStartBand;
      int virtualBand = index;


      if(m_virtualBands) {
        virtualBand = m_virtualBands->indexOf(virtualBand) + 1;
      }

      if(virtualBand != 0 && virtualBand >= bufferBand &&
         virtualBand <= bufferBand + bufferBands - 1) {

        for(int y = startY; y <= endY; y++) {
          const int &lineIntoChunk = y - outputStartLine;
          int bufferIndex = buffer.Index(startX, y, virtualBand);

          for(int x = startX; x <= endX; x++) {
            const int &sampleIntoChunk = x - outputStartSample;

            const int &chunkIndex = sampleIntoChunk +
                (lineSize * lineIntoChunk) + (bandSize * bandIntoChunk);

            double bufferVal = buffersDoubleBuf[bufferIndex];

            if(m_pixelType == Real) {
              float raw = 0;

              if(bufferVal >= VALID_MIN8) {
                double filePixelValueDbl = (bufferVal - m_base) /
                    m_multiplier;

                if(filePixelValueDbl < (double) VALID_MIN4) {
                  raw = LOW_REPR_SAT4;
                }
                else if(filePixelValueDbl > (double) VALID_MAX4) {
                  raw = HIGH_REPR_SAT4;
                }
                else {
                  raw = (float) filePixelValueDbl;
                }
              }
              else {
                if(bufferVal == NULL8)
                  raw = NULL4;
                else if(bufferVal == LOW_INSTR_SAT8)
                  raw = LOW_INSTR_SAT4;
                else if(bufferVal == LOW_REPR_SAT8)
                  raw = LOW_REPR_SAT4;
                else if(bufferVal == HIGH_INSTR_SAT8)
                  raw = HIGH_INSTR_SAT4;
                else if(bufferVal == HIGH_REPR_SAT8)
                  raw = HIGH_REPR_SAT4;
                else
                  raw = LOW_REPR_SAT4;
              }
              ((float *)chunkBuf)[chunkIndex] =
                  m_byteSwapper ? m_byteSwapper->Float(&raw) : raw;
            }

            else if(m_pixelType == SignedWord) {
              short raw;

              if(bufferVal >= VALID_MIN8) {
                double filePixelValueDbl = (bufferVal - m_base) /
                    m_multiplier;
                if(filePixelValueDbl < VALID_MIN2 - 0.5) {
                  raw = LOW_REPR_SAT2;
                }
                if(filePixelValueDbl > VALID_MAX2 + 0.5) {
                  raw = HIGH_REPR_SAT2;
                }
                else {
                  int filePixelValue = (int)round(filePixelValueDbl);

                  if(filePixelValue < VALID_MIN2) {
                    raw = LOW_REPR_SAT2;
                  }
                  else if(filePixelValue > VALID_MAX2) {
                    raw = HIGH_REPR_SAT2;
                  }
                  else {
                    raw = filePixelValue;
                  }
                }
              }
              else {
                if(bufferVal == NULL8)
                  raw = NULL2;
                else if(bufferVal == LOW_INSTR_SAT8)
                  raw = LOW_INSTR_SAT2;
                else if(bufferVal == LOW_REPR_SAT8)
                  raw = LOW_REPR_SAT2;
                else if(bufferVal == HIGH_INSTR_SAT8)
                  raw = HIGH_INSTR_SAT2;
                else if(bufferVal == HIGH_REPR_SAT8)
                  raw = HIGH_REPR_SAT2;
                else
                  raw = LOW_REPR_SAT2;
              }

              ((short *)chunkBuf)[chunkIndex] =
                  m_byteSwapper ? m_byteSwapper->ShortInt(&raw) : raw;
            }

            else if(m_pixelType == UnsignedInteger) {

              unsigned int raw;

              if(bufferVal >= VALID_MINUI4) {
                double filePixelValueDbl = (bufferVal - m_base) /
                    m_multiplier;
                if(filePixelValueDbl < VALID_MINUI4 - 0.5) {
                  raw = LOW_REPR_SATUI4;
                }
                if(filePixelValueDbl > VALID_MAXUI4) {
                  raw = HIGH_REPR_SATUI4;
                }
                else {
                  unsigned int filePixelValue = (unsigned int)round(filePixelValueDbl);

                  if(filePixelValue < VALID_MINUI4) {
                    raw = LOW_REPR_SATUI4;
                  }
                  else if(filePixelValue > VALID_MAXUI4) {
                    raw = HIGH_REPR_SATUI4;
                  }
                  else {
                    raw = filePixelValue;
                  }
                }
              }
              else {
                if(bufferVal == NULL8)
                  raw = NULLUI4;
                else if(bufferVal == LOW_INSTR_SAT8)
                  raw = LOW_INSTR_SATUI4;
                else if(bufferVal == LOW_REPR_SAT8)
                  raw = LOW_REPR_SATUI4;
                else if(bufferVal == HIGH_INSTR_SAT8)
                  raw = HIGH_INSTR_SATUI4;
                else if(bufferVal == HIGH_REPR_SAT8)
                  raw = HIGH_REPR_SATUI4;
                else
                  raw = LOW_REPR_SATUI4;
              }

              ((unsigned int *)chunkBuf)[chunkIndex] =
                  m_byteSwapper ? m_byteSwapper->Uint32_t(&raw) : raw;

            }


            else if(m_pixelType == UnsignedWord) {
              unsigned short raw;

              if(bufferVal >= VALID_MIN8) {
                double filePixelValueDbl = (bufferVal - m_base) /
                    m_multiplier;
                if(filePixelValueDbl < VALID_MINU2 - 0.5) {
                  raw = LOW_REPR_SATU2;
                }
                if(filePixelValueDbl > VALID_MAXU2 + 0.5) {
                  raw = HIGH_REPR_SATU2;
                }
                else {
                  int filePixelValue = (int)round(filePixelValueDbl);

                  if(filePixelValue < VALID_MINU2) {
                    raw = LOW_REPR_SATU2;
                  }
                  else if(filePixelValue > VALID_MAXU2) {
                    raw = HIGH_REPR_SATU2;
                  }
                  else {
                    raw = filePixelValue;
                  }
                }
              }
              else {
                if(bufferVal == NULL8)
                  raw = NULLU2;
                else if(bufferVal == LOW_INSTR_SAT8)
                  raw = LOW_INSTR_SATU2;
                else if(bufferVal == LOW_REPR_SAT8)
                  raw = LOW_REPR_SATU2;
                else if(bufferVal == HIGH_INSTR_SAT8)
                  raw = HIGH_INSTR_SATU2;
                else if(bufferVal == HIGH_REPR_SAT8)
                  raw = HIGH_REPR_SATU2;
                else
                  raw = LOW_REPR_SATU2;
              }

              ((unsigned short *)chunkBuf)[chunkIndex] =
                  m_byteSwapper ? m_byteSwapper->UnsignedShortInt(&raw) : raw;
            }
            else if(m_pixelType == UnsignedByte) {
              unsigned char raw;

              if(bufferVal >= VALID_MIN8) {
                double filePixelValueDbl = (bufferVal - m_base) /
                    m_multiplier;
                if(filePixelValueDbl < VALID_MIN1 - 0.5) {
                  raw = LOW_REPR_SAT1;
                }
                else if(filePixelValueDbl > VALID_MAX1 + 0.5) {
                  raw = HIGH_REPR_SAT1;
                }
                else {
                  int filePixelValue = (int)(filePixelValueDbl + 0.5);
                  if(filePixelValue < VALID_MIN1) {
                    raw = LOW_REPR_SAT1;
                  }
                  else if(filePixelValue > VALID_MAX1) {
                    raw = HIGH_REPR_SAT1;
                  }
                  else {
                    raw = (unsigned char)(filePixelValue);
                  }
                }
              }
              else {
                if(bufferVal == NULL8)
                  raw = NULL1;
                else if(bufferVal == LOW_INSTR_SAT8)
                  raw = LOW_INSTR_SAT1;
                else if(bufferVal == LOW_REPR_SAT8)
                  raw = LOW_REPR_SAT1;
                else if(bufferVal == HIGH_INSTR_SAT8)
                  raw = HIGH_INSTR_SAT1;
                else if(bufferVal == HIGH_REPR_SAT8)
                  raw = HIGH_REPR_SAT1;
                else
                  raw = LOW_REPR_SAT1;
              }

              ((unsigned char *)chunkBuf)[chunkIndex] = raw;
            }

            bufferIndex ++;
          }
        }
      }
    }
  }


  /**
   * Write all NULL cube chunks that have not yet been accessed to disk.
   */
  void CubeIoHandler::writeNullDataToDisk() const {
    if(!m_dataIsOnDiskMap) {
      IString msg = "Cannot call CubeIoHandler::writeNullDataToDisk unless "
          "data is not already on disk (Cube::Create was called)";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    int numChunks = getChunkCount();
    for(int i = 0; i < numChunks; i++) {
      if(!(*m_dataIsOnDiskMap)[i]) {
        RawCubeChunk *nullChunk = getNullChunk(i);
        (const_cast<CubeIoHandler *>(this))->writeRaw(*nullChunk);
        (*m_dataIsOnDiskMap)[i] = true;

        delete nullChunk;
        nullChunk = NULL;
      }
    }
  }


  /**
   * Create a BufferToChunkWriter which is designed to asynchronously move
   *   the given buffers into the cube cache. This will lock the
   *   ioHandler's m_writeThreadMutex.
   *
   * @param ioHandler This is the cube IO handler. It's not const because you
   *                  should have non-constness on a cube in order to write to
   *                  it. This does, sometimes, need overridden externally if
   *                  you are simply flushing the cache and not doing a new
   *                  write.
   * @param buffersToWrite The buffers to put into the cube. This takes
   *                  ownership of the given buffers.
   */
  CubeIoHandler::BufferToChunkWriter::BufferToChunkWriter(
      CubeIoHandler * ioHandler, QList<Buffer *> buffersToWrite) {
    m_ioHandler = ioHandler;
    m_buffersToWrite = new QList<Buffer *>(buffersToWrite);
    m_timer = new QElapsedTimer;
    m_timer->start();

    m_ioHandler->m_writeThreadMutex->lock();
  }


  /**
   * We're done writing our buffers into the cube, clean up. It turns out you
   *   can't start the next cache flush from here, so this does not initiate
   *   another write cache flush.
   *
   * Before releasing the lock on the QDataFile, this destructor attempts to
   *   correct the ideal cache size to reflect the current run. This tries to
   *   bring the cache size to a point where this class lives for a total of
   *   100ms.
   */
  CubeIoHandler::BufferToChunkWriter::~BufferToChunkWriter() {
    int elapsedMs = m_timer->elapsed();
    int idealFlushElapsedTime = 100; // ms

    // We want to aim our flush size at 100ms, so adjust accordingly to aim at
    //   our target. This method seems to be extremely effective because
    //   we maximize our I/O calls when caching is interfering and normalize
    //   them otherwise.
    int msOffIdeal = elapsedMs - idealFlushElapsedTime;
    double percentOffIdeal = msOffIdeal / (double)idealFlushElapsedTime;

    // flush size is bounded to [32, 5000]
    int currentCacheSize = m_ioHandler->m_idealFlushSize;
    int desiredAdjustment = -1 * currentCacheSize * percentOffIdeal;
    int desiredCacheSize = (int)(currentCacheSize + desiredAdjustment);
    m_ioHandler->m_idealFlushSize = (int)(qMin(5000,
                                               qMax(32, desiredCacheSize)));

    delete m_timer;
    m_timer = NULL;

    m_ioHandler->m_writeThreadMutex->unlock();
    m_ioHandler = NULL;

    delete m_buffersToWrite;
    m_buffersToWrite = NULL;
  }


  /**
   * This is the asynchronous computation. Write the given buffers into the
   *   cube synchronously in this thread and delete the buffers when we're
   *   done.
   */
  void CubeIoHandler::BufferToChunkWriter::run() {
    // Sorting the buffers didn't seem to have a large positive impact on speed,
    //   but does increase complexity so it's disabled.
//     QList<Buffer * > buffersToWrite(*m_buffersToWrite);
//     std::sort(buffersToWrite.begin(), buffersToWrite.end(), bufferLessThan);

    // If the buffers have any overlap at all then we can't sort them and still
    //   guarantee the last write() call makes it into the correct place. The
    //   bufferLessThan is guaranteed to return false if there is overlap.
//     bool sortable = true;
//     for (int i = 1; sortable && i < buffersToWrite.size(); i++) {
//       if (!bufferLessThan(buffersToWrite[i - 1], buffersToWrite[i])) {
//         sortable = false;
//       }
//     }

//     if (!sortable) {
//       buffersToWrite = *m_buffersToWrite;
//     }

    foreach (Buffer * bufferToWrite, *m_buffersToWrite) {
      m_ioHandler->synchronousWrite(*bufferToWrite);
      delete bufferToWrite;
    }

    m_buffersToWrite->clear();
    m_ioHandler->m_dataFile->flush();
  }
}
