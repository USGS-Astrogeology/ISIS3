#include "GdalIoHandler.h"

#include "Brick.h"
#include "Buffer.h"
#include "CubeCachingAlgorithm.h"
#include "IException.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "SpecialPixel.h"

#include <QList>
#include <QMutex>
#include <QString>

#include <iostream>

using namespace std;

namespace Isis {
  GdalIoHandler::GdalIoHandler(QString &dataFilePath, const QList<int> *virtualBandList, GDALDataType pixelType, GDALAccess eAccess) : 
                 ImageIoHandler(virtualBandList) {
    GDALAllRegister();
    m_geodataSetPath = dataFilePath.toUtf8().constData();
    m_geodataSet = GDALDataset::FromHandle(GDALOpen(m_geodataSetPath.c_str(), eAccess));
    m_datasetOwner = true;
    m_pixelType = pixelType;
    init();
  }

  GdalIoHandler::GdalIoHandler(GDALDataset *geodataSet, const QList<int> *virtualBandList, GDALDataType pixelType) : 
                 ImageIoHandler(virtualBandList) {
    m_geodataSet = geodataSet;
    if (!m_geodataSet) {
      QString msg = "Constructing GdalIoHandler failed. Null geodataSet pointer passed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_datasetOwner = false;
    m_pixelType = pixelType;
    init();
  }

  void GdalIoHandler::init() {
    m_samples = m_geodataSet->GetRasterXSize();
    m_lines = m_geodataSet->GetRasterYSize();
    m_bands = m_geodataSet->GetRasterCount();

    m_driverName = std::string(m_geodataSet->GetDriverName());

    // Check if we need to create the mask band
    if ((m_geodataSet->GetAccess() == GA_Update) && (m_driverName != "ISIS3")) {
      m_geodataSet->CreateMaskBand(8);
      m_geodataSet->GetRasterBand(1)->GetMaskBand()->Fill(255);
    }

    GDALRasterBand *band = m_geodataSet->GetRasterBand(1);
    m_offset = band->GetOffset();
    m_scale = band->GetScale();
    int *pbSuccess = new int;
    m_gdalNoDataValue = band->GetNoDataValue(pbSuccess);
    if (!pbSuccess) {
      m_gdalNoDataValue = NULL8;
    }
    delete pbSuccess;
  }

  GdalIoHandler::~GdalIoHandler() {
    clearCache();
    if (m_maskBuff) {
      delete m_maskBuff;
    }
    if (m_geodataSet && m_datasetOwner) {
      m_geodataSet->Close();
    }
  }

  void GdalIoHandler::read(Buffer &bufferToFill) const {
    GDALRasterBand  *poBand;

    bufferToFill = NULL8;
    int maxBand = bufferToFill.BandDimension() + bufferToFill.Band() - 1;
    for (int band = bufferToFill.Band(); band <= maxBand; band++) {
      int vband = band;
      if (m_virtualBands)
        vband = m_virtualBands->at(band - 1);

      if (vband > m_bands) {
        return;
      }

      poBand = m_geodataSet->GetRasterBand(vband);
      
      // Account for 1 based line and sample reading from ISIS process classes
      // as gdal reads 0 based lines and samples
      int lineStart = bufferToFill.Line() - 1;
      int sampleStart = bufferToFill.Sample() - 1;
      int lineSize = bufferToFill.LineDimension();
      int sampleSize = bufferToFill.SampleDimension();

      // Handle reading out of bound pixels as cube IO handler would
      bool outOfBounds = false;
      if (lineStart < 0) {
        lineStart = 0;
        outOfBounds = true;
      }
      if (lineStart + lineSize > m_lines) {
        lineSize = m_lines - lineStart;
        outOfBounds = true;
      }
      if (sampleStart < 0) {
        sampleStart = 0;
        outOfBounds = true;
      }
      if (sampleStart + sampleSize > m_samples) {
        sampleSize = m_samples - sampleStart;
        outOfBounds = true;
      }

      if (sampleSize > 0 && lineSize > 0) {
        if (outOfBounds) {
          Brick boundedBrick(sampleSize, lineSize, bufferToFill.BandDimension(), GdalPixelToIsis(m_pixelType), false, bufferToFill.scale());
          boundedBrick.SetBasePosition(sampleStart + 1, lineStart + 1, bufferToFill.Band());
          int bufferSize = boundedBrick.SampleDimensionScaled() * boundedBrick.LineDimensionScaled();
          int currentBandIdx = bufferSize * (vband - boundedBrick.Band());
          char *buffersRawBuf = &(((char *)boundedBrick.RawBuffer())[(int)(currentBandIdx * SizeOf(boundedBrick.PixelType()))]);
          double *buffersDoubleBuf = &(boundedBrick.DoubleBuffer()[currentBandIdx]);
          CPLErr err = poBand->RasterIO(GF_Read, sampleStart, lineStart,
                                        sampleSize, lineSize,
                                        buffersRawBuf,
                                        boundedBrick.SampleDimensionScaled(), boundedBrick.LineDimensionScaled(),
                                        m_pixelType,
                                        0, 0);

          if (err >= CE_Failure) {
            QString msg = "Failure when trying to read image";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }

          // Handle pixel type conversion
          for (int bufferIdx = 0; bufferIdx < bufferSize; bufferIdx++) {
            readPixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx);
          }
          bufferToFill.CopyOverlapFrom(boundedBrick);
        }
        else {
          int bufferSize = bufferToFill.SampleDimensionScaled() * bufferToFill.LineDimensionScaled();;
          int currentBandIdx = bufferSize * (vband - bufferToFill.Band());
          // silence warnings
          char *buffersRawBuf = &(((char *)bufferToFill.RawBuffer())[(int)(currentBandIdx * SizeOf(bufferToFill.PixelType()))]);
          double *buffersDoubleBuf = &(bufferToFill.DoubleBuffer()[currentBandIdx]);
          CPLErr err = poBand->RasterIO(GF_Read, sampleStart, lineStart,
                                        sampleSize, lineSize,
                                        buffersRawBuf,
                                        bufferToFill.SampleDimensionScaled(), bufferToFill.LineDimensionScaled(),
                                        m_pixelType,
                                        0, 0);
          if (err >= CE_Failure) {
            QString msg = "Failure when trying to read image";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }

          // Handle pixel type conversion
          for (int bufferIdx = 0; bufferIdx < bufferSize; bufferIdx++) {
            readPixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx);
          }
        }
      }
    }
  }

  void GdalIoHandler::write(const Buffer &bufferToWrite) {
    CPLErr gdalerr;
    GDALRasterBand  *poBand;

    int maxBand = bufferToWrite.BandDimension() + bufferToWrite.Band() - 1;
    for (int band = bufferToWrite.Band(); band <= maxBand; band++) {
      int vband = band;
      if(m_virtualBands) 
        vband = m_virtualBands->indexOf(band) + 1;
      
      if (band > m_bands) {
        return;
      }

      poBand = m_geodataSet->GetRasterBand(vband);

      int lineStart = bufferToWrite.Line() - 1;
      int sampleStart = bufferToWrite.Sample() - 1;
      int lineSize = bufferToWrite.LineDimension();
      int sampleSize = bufferToWrite.SampleDimension();

      bool outOfBounds = false;
      if (lineStart < 0) {
        lineStart = 0;
        outOfBounds = true;
      }
      if (lineStart + lineSize > m_lines) {
        lineSize = m_lines - lineStart;
        outOfBounds = true;
      }
      if (sampleStart < 0) {
        sampleStart = 0;
        outOfBounds = true;
      }
      if (sampleStart + sampleSize > m_samples) {
        sampleSize = m_samples - sampleStart;
        outOfBounds = true;
      }

      int bufferSize = sampleSize * lineSize;
      int currentBandIdx = bufferSize * (vband - bufferToWrite.Band());
      m_maskBuff = (unsigned char *) CPLMalloc(sizeof(unsigned char) * bufferSize);
      for (int i = 0; i < bufferSize; i++) {
        m_maskBuff[i] = 255;
      }


      // Handle pixel type conversion
      char *buffersRawBuf = &(((char *)bufferToWrite.RawBuffer())[(int)(currentBandIdx * SizeOf(bufferToWrite.PixelType()))]);
      double *buffersDoubleBuf = &(bufferToWrite.DoubleBuffer()[currentBandIdx]);
      // std::cout << buffersDoubleBuf[0] << std::endl;
      for (int bufferIdx = 0; bufferIdx < bufferSize; bufferIdx++) {
        if (writePixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx)) {
          m_maskBuff[bufferIdx] = 0;
        };
      }

      // silence warning
      gdalerr = poBand->RasterIO(GF_Write, sampleStart, lineStart,
                                sampleSize, lineSize,
                                (void *)buffersRawBuf,
                                sampleSize, lineSize,
                                m_pixelType,
                                0, 0);
      if (gdalerr >= CE_Failure) {
        QString msg = "Failure when trying to write image";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      if (m_driverName != "ISIS3") { 
        poBand = m_geodataSet->GetRasterBand(band)->GetMaskBand();
        gdalerr = poBand->RasterIO(GF_Write, sampleStart, lineStart,
                                  sampleSize, lineSize,
                                  m_maskBuff,
                                  sampleSize, lineSize,
                                  GDT_Byte,
                                  0, 0);
        if (gdalerr >= CE_Failure) {
          QString msg = "Failure when trying to write msk file";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }
    }

    delete m_maskBuff;
    m_maskBuff = NULL;
  }

  BigInt GdalIoHandler::getDataSize() const {
    return 0;
  }
  /**
   * Function to update the labels with a Pvl object
   *
   * @param labels Pvl object to update with
   */
  void GdalIoHandler::updateLabels(Pvl &labels) {
    PvlObject &core = labels.findObject("IsisCube").findObject("Core");
    core.addKeyword(PvlKeyword("Format", "GTiff"), PvlContainer::Replace);
  }

  void GdalIoHandler::readPixelType(double *doubleBuff, void *rawBuff, int idx) const {
    double &bufferVal = doubleBuff[idx];
    
    if (m_pixelType == GDT_Float64) {
      bufferVal = ((double *)rawBuff)[idx];
      if (bufferVal == m_gdalNoDataValue) {
        bufferVal = NULL8;
      }
    }
    
    else if(m_pixelType == GDT_Float32) {
      float raw = ((float *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->Float(&raw);
      if (raw == (float) m_gdalNoDataValue) {
        raw = NULL4;
      }

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

      ((float *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int32) {
      int raw = ((int *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->Uint32_t(&raw);
      if (raw == (int) m_gdalNoDataValue) {
        raw = NULLI4;
      }

      if(raw >= VALID_MINI4) {
        bufferVal = (double) raw * m_scale + m_offset;
      }
      else {
        if (raw == NULLI4)
          bufferVal = NULL8;
        else if (raw == LOW_INSTR_SATI4)
          bufferVal = LOW_INSTR_SAT8;
        else if (raw == LOW_REPR_SATI4)
          bufferVal = LOW_REPR_SAT8;
        else if (raw == HIGH_INSTR_SATI4)
          bufferVal = HIGH_INSTR_SAT8;
        else if (raw == HIGH_REPR_SATI4)
          bufferVal = HIGH_REPR_SAT8;
        else
          bufferVal = LOW_REPR_SAT8;
      }
      ((int *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_UInt32) {
      unsigned int raw = ((unsigned int *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->Uint32_t(&raw);
      if (raw == (unsigned int) m_gdalNoDataValue) {
        raw = NULLUI4;
      }

      if(raw >= VALID_MINUI4) {
        bufferVal = (double) raw * m_scale + m_offset;
        if (raw >= VALID_MAXUI4) {
          if(raw == HIGH_INSTR_SATUI4)
            bufferVal = HIGH_INSTR_SAT8;
          else if(raw == HIGH_REPR_SATUI4)
            bufferVal = HIGH_REPR_SAT8;
          else
            bufferVal = HIGH_REPR_SAT8;
        }
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
      ((unsigned int *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int16) {
      short raw = ((short *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->ShortInt(&raw);
      if (raw == (short) m_gdalNoDataValue) {
        raw = NULL2;
      }

      if(raw >= VALID_MIN2) {
        bufferVal = (double) raw * m_scale + m_offset;
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

      ((short *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_UInt16) {
      unsigned short raw = ((unsigned short *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->UnsignedShortInt(&raw);
      if (raw == (unsigned short) m_gdalNoDataValue) {
        raw = NULLU2;
      }

      if(raw >= VALID_MINU2) {
        bufferVal = (double) raw * m_scale + m_offset;
        if (raw > VALID_MAXU2) {
          if(raw == HIGH_INSTR_SATU2)
            bufferVal = HIGH_INSTR_SAT8;
          else if(raw == HIGH_REPR_SATU2)
            bufferVal = HIGH_REPR_SAT8;
          else
            bufferVal = HIGH_REPR_SAT8;
        }
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
      ((unsigned short *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int8) {
      char raw = ((char *)rawBuff)[idx];
      if (raw == (char) m_gdalNoDataValue) {
        raw = NULLS1;
      }

      if(raw == NULLS1) {
        bufferVal = NULL8;
      }
      else if(raw == HIGH_REPR_SATS1) {
        bufferVal = HIGH_REPR_SAT8;
      }
      else {
        bufferVal = (double) raw * m_scale + m_offset;
      }

      ((char *)rawBuff)[idx] = raw;
    }
    
    else if(m_pixelType == GDT_Byte) {
      unsigned char raw = ((unsigned char *)rawBuff)[idx];
      if (raw == (char) m_gdalNoDataValue) {
        raw = NULL1;
      }

      if(raw == NULL1) {
        bufferVal = NULL8;
      }
      else if(raw == HIGH_REPR_SAT1) {
        bufferVal = HIGH_REPR_SAT8;
      }
      else {
        bufferVal = (double) raw * m_scale + m_offset;
      }

      ((unsigned char *)rawBuff)[idx] = raw;
    }
  }

  bool GdalIoHandler::writePixelType(double *doubleBuff, void *rawBuff, int idx) const {
    double bufferVal = doubleBuff[idx];
    bool isSpecial = false;
    if (m_pixelType == GDT_Float64) {
      ((double *)rawBuff)[idx] = bufferVal;
    }
    
    else if(m_pixelType == GDT_Float32) {
      float raw = 0;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;

        if(filePixelValueDbl < (double) VALID_MIN4) {
          raw = LOW_REPR_SAT4;
          isSpecial = true;
        }
        else if(filePixelValueDbl > (double) VALID_MAX4) {
          raw = HIGH_REPR_SAT4;
          isSpecial = true;
        }
        else {
          raw = (float) filePixelValueDbl;
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULL4;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SAT4;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SAT4;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SAT4;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SAT4;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SAT4;
          isSpecial = true;
        }
      }
      ((float *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int32) {
      int raw;

      if(bufferVal >= VALID_MINI4) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MINI4 - 0.5) {
          raw = LOW_REPR_SATI4;
          isSpecial = true;
        }
        if(filePixelValueDbl > VALID_MAXI4) {
          raw = HIGH_REPR_SATI4;
          isSpecial = true;
        }
        else {
          int filePixelValue = (int)round(filePixelValueDbl);

          if(filePixelValue < VALID_MINI4) {
            raw = LOW_REPR_SATI4;
            isSpecial = true;
          }
          else if(filePixelValue > VALID_MAXI4) {
            raw = HIGH_REPR_SATI4;
            isSpecial = true;
          }
          else {
            raw = filePixelValue;
          }
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULLI4;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SATI4;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SATI4;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SATI4;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SATI4;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SATI4;
          isSpecial = true;
        }
      }
      ((int *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_UInt32) {
      unsigned int raw;

      if(bufferVal >= VALID_MINUI4) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MINUI4 - 0.5) {
          raw = LOW_REPR_SATUI4;
          isSpecial = true;
        }
        if(filePixelValueDbl > VALID_MAXUI4) {
          raw = HIGH_REPR_SATUI4;
          isSpecial = true;
        }
        else {
          unsigned int filePixelValue = (unsigned int)round(filePixelValueDbl);

          if(filePixelValue < VALID_MINUI4) {
            raw = LOW_REPR_SATUI4;
            isSpecial = true;
          }
          else if(filePixelValue > VALID_MAXUI4) {
            raw = HIGH_REPR_SATUI4;
            isSpecial = true;
          }
          else {
            raw = filePixelValue;
          }
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULLUI4;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SATUI4;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SATUI4;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SATUI4;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SATUI4;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SATUI4;
          isSpecial = true;
        }
      }
      ((unsigned int *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int16) {
      short raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MIN2 - 0.5) {
          raw = LOW_REPR_SAT2;
          isSpecial = true;
        }
        if(filePixelValueDbl > VALID_MAX2 + 0.5) {
          raw = HIGH_REPR_SAT2;
          isSpecial = true;
        }
        else {
          int filePixelValue = (int)round(filePixelValueDbl);

          if(filePixelValue < VALID_MIN2) {
            raw = LOW_REPR_SAT2;
            isSpecial = true;
          }
          else if(filePixelValue > VALID_MAX2) {
            raw = HIGH_REPR_SAT2;
            isSpecial = true;
          }
          else {
            raw = filePixelValue;
          }
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULL2;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SAT2;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SAT2;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SAT2;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SAT2;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SAT2;
          isSpecial = true;
        }
      }
      ((short *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_UInt16) {
      unsigned short raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MINU2 - 0.5) {
          raw = LOW_REPR_SATU2;
          isSpecial = true;
        }
        if(filePixelValueDbl > VALID_MAXU2 + 0.5) {
          raw = HIGH_REPR_SATU2;
          isSpecial = true;
        }
        else {
          int filePixelValue = (int)round(filePixelValueDbl);

          if(filePixelValue < VALID_MINU2) {
            raw = LOW_REPR_SATU2;
            isSpecial = true;
          }
          else if(filePixelValue > VALID_MAXU2) {
            raw = HIGH_REPR_SATU2;
            isSpecial = true;
          }
          else {
            raw = filePixelValue;
          }
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULLU2;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SATU2;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SATU2;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SATU2;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SATU2;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SATU2;
          isSpecial = true;
        }
      }
      ((unsigned short *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int8) {
     char raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MINS1 - 0.5) {
          raw = LOW_REPR_SATS1;
          isSpecial = true;
        }
        else if(filePixelValueDbl > VALID_MAXS1 + 0.5) {
          raw = HIGH_REPR_SATS1;
          isSpecial = true;
        }
        else {
          int filePixelValue = (int)(filePixelValueDbl + 0.5);
          if(filePixelValue < VALID_MINS1) {
            raw = LOW_REPR_SATS1;
            isSpecial = true;
          }
          else if(filePixelValue > VALID_MAXS1) {
            raw = HIGH_REPR_SATS1;
            isSpecial = true;
          }
          else {
            raw = (char)(filePixelValue);
          }
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULLS1;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SATS1;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SATS1;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SATS1;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SATS1;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SATS1;
          isSpecial = true;
        }
      }
      ((char *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Byte) {
      unsigned char raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MIN1 - 0.5) {
          raw = LOW_REPR_SAT1;
          isSpecial = true;
        }
        else if(filePixelValueDbl > VALID_MAX1 + 0.5) {
          raw = HIGH_REPR_SAT1;
          isSpecial = true;
        }
        else {
          int filePixelValue = (int)(filePixelValueDbl + 0.5);
          if(filePixelValue < VALID_MIN1) {
            raw = LOW_REPR_SAT1;
            isSpecial = true;
          }
          else if(filePixelValue > VALID_MAX1) {
            raw = HIGH_REPR_SAT1;
            isSpecial = true;
          }
          else {
            raw = (unsigned char)(filePixelValue);
          }
        }
      }
      else {
        if(bufferVal == NULL8) {
          raw = NULL1;
          isSpecial = true;
        }
        else if(bufferVal == LOW_INSTR_SAT8) {
          raw = LOW_INSTR_SAT1;
          isSpecial = true;
        }
        else if(bufferVal == LOW_REPR_SAT8) {
          raw = LOW_REPR_SAT1;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_INSTR_SAT8) {
          raw = HIGH_INSTR_SAT1;
          isSpecial = true;
        }
        else if(bufferVal == HIGH_REPR_SAT8) {
          raw = HIGH_REPR_SAT1;
          isSpecial = true;
        }
        else {
          raw = LOW_REPR_SAT1;
          isSpecial = true;
        }
      }
      ((unsigned char *)rawBuff)[idx] = raw;
    }
    return isSpecial;
  }
}