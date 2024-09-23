/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubeBsqHandler.h"

#include <iostream>

#include <QFile>

#include "IException.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "RawCubeChunk.h"

using namespace std;

namespace Isis {
  /**
   * Construct a BSQ IO handler. This will determine a good chunk size to use
   *   that does not result in the cube being enlarged or misordered.
   *
   * @param dataFile The file with cube DN data in it
   * @param virtualBandList The mapping from virtual band to physical band, see
   *        CubeIoHandler's description.
   * @param labels The Pvl labels for the cube
   * @param alreadyOnDisk True if the cube is allocated on the disk, false
   *     otherwise
   */
  CubeBsqHandler::CubeBsqHandler(QFile * dataFile,
      const QList<int> *virtualBandList, const Pvl &labels, bool alreadyOnDisk)
    : CubeIoHandler(dataFile, virtualBandList, labels, alreadyOnDisk) {
    int numSamplesInChunk = sampleCount();
    int numLinesInChunk = 1;
    QList<int> primeFactors;

    // we want our chunk sizes to be less than 1GB
    int sizeLimit = 1024 * 1024 * 1024;
    int maxNumLines = (sizeLimit) / (SizeOf(pixelType()) * numSamplesInChunk);

    // we've exceed our sizeLimit; increase our limit so we can process an entire line
    if (maxNumLines == 0)
      maxNumLines = 1;

    numLinesInChunk = findGoodSize(maxNumLines, lineCount());

    setChunkSizes(numSamplesInChunk, numLinesInChunk, 1);
  }


  /**
   * The destructor writes all cached data to disk.
   */
  CubeBsqHandler::~CubeBsqHandler() {
    clearCache();
  }


  /**
   * Function to update the labels with a Pvl object
   *
   * @param label Pvl object to update with
   */
  void CubeBsqHandler::updateLabels(Pvl &label) {
    PvlObject &core = label.findObject("IsisCube").findObject("Core");
    core.addKeyword(PvlKeyword("Format", "BandSequential"),
                    PvlContainer::Replace);
  }


  void CubeBsqHandler::readRaw(RawCubeChunk &chunkToFill) {
    BigInt startByte = getChunkStartByte(chunkToFill);

    bool success = false;

    QFile * dataFile = getDataFile();
    if(dataFile->seek(startByte)) {
      QByteArray binaryData = dataFile->read(chunkToFill.getByteCount());

      if(binaryData.size() == chunkToFill.getByteCount()) {
        chunkToFill.setRawData(binaryData);
        success = true;
      }
    }

    if(!success) {
      IString msg = "Reading from the file [" + dataFile->fileName().toStdString() + "] "
          "failed with reading [" +
          toString(chunkToFill.getByteCount()) +
          "] bytes at position [" + toString(startByte) + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }


  void CubeBsqHandler::writeRaw(const RawCubeChunk &chunkToWrite) {
    BigInt startByte = getChunkStartByte(chunkToWrite);

    bool success = false;

    QFile * dataFile = getDataFile();
    if(dataFile->seek(startByte)) {
      BigInt dataWritten = dataFile->write(chunkToWrite.getRawData());

      if(dataWritten == chunkToWrite.getByteCount()) {
        success = true;
      }
    }

    if(!success) {
      IString msg = "Writing to the file [" + dataFile->fileName().toStdString() + "] "
          "failed with writing [" +
          toString(chunkToWrite.getByteCount()) +
          "] bytes at position [" + toString(startByte) + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }


  /**
   * This method attempts to compute a good chunk line size. Chunk band size is
   * always 1 and chunk sample size is always number of samples in the cube for this format.
   *
   * @param maxSize The largest allowed size of a chunk dimension
   * @param dimensionSize The cube's size for the chunk size we are trying to calculate
   *     (number of lines)
   * @return The calculated chunk size for the dimension given
   */
  int CubeBsqHandler::findGoodSize(int maxSize, int dimensionSize) const {
    int chunkDimensionSize;

    if (dimensionSize <= maxSize) {
      chunkDimensionSize = dimensionSize;
    }
    else {
      // find largest divisor of dimension size so chunks fit into cube uniformly
      int greatestDivisor = maxSize;
      while (dimensionSize % greatestDivisor > 0) {
        greatestDivisor--;
      }
      chunkDimensionSize = greatestDivisor;
    }
    return chunkDimensionSize;
  }


  /**
   * This is a helper method that goes from chunk to file position.
   *
   * @param chunk The chunk to locate in the file.
   * @return The file position to start reading or writing at
   */
  BigInt CubeBsqHandler::getChunkStartByte(const RawCubeChunk &chunk) const {
    return getDataStartByte() + getChunkIndex(chunk) * getBytesPerChunk();
  }
}
