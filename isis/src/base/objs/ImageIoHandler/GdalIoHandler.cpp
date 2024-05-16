#include "GdalIoHandler.h"

#include "Brick.h"
#include "Buffer.h"
#include "CubeCachingAlgorithm.h"
#include "IException.h"
#include "SpecialPixel.h"

#include <QList>
#include <QMutex>
#include <QString>

#include <iostream>

using namespace std;

namespace Isis {
  GdalIoHandler::GdalIoHandler(QString &dataFilePath, const QList<int> *virtualBandList, GDALDataType pixelType) : 
                 ImageIoHandler(virtualBandList) {
    GDALAllRegister();
    const GDALAccess eAccess = GA_Update;
    std::string dataFilePathStr = dataFilePath.toUtf8().constData();
    const char *charDataFilePath = dataFilePathStr.c_str();
    m_geodataSet = GDALDatasetUniquePtr(GDALDataset::FromHandle(GDALOpen(charDataFilePath, eAccess)));
    if (!m_geodataSet) {
      QString msg = "Constructing GdalIoHandler failed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    m_geodataSet->CreateMaskBand(0);
    m_geodataSet->GetRasterBand(1)->GetMaskBand()->Fill(255);
    m_pixelType = pixelType;
    m_samples = m_geodataSet->GetRasterXSize();
    m_lines = m_geodataSet->GetRasterYSize();
    m_bands = m_geodataSet->GetRasterCount();
    GDALRasterBand *band = m_geodataSet->GetRasterBand(1);
    m_offset = band->GetOffset();
    m_scale = band->GetScale();
  }

  GdalIoHandler::~GdalIoHandler() {
  }

  void GdalIoHandler::read(Buffer &bufferToFill) const {
    GDALRasterBand  *poBand;
    int band = bufferToFill.Band();

    if (m_virtualBands)
      band = m_virtualBands->at(band - 1);
    poBand = m_geodataSet->GetRasterBand(band);
    
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
    if (lineStart + bufferToFill.LineDimension() > m_lines) {
      lineStart = m_lines - bufferToFill.LineDimension();
      outOfBounds = true;
    }
    if (sampleStart < 0) {
      sampleStart = 0;
      outOfBounds = true;
    }
    if (sampleStart + bufferToFill.SampleDimension() > m_samples) {
      sampleStart = m_samples - bufferToFill.SampleDimension();
      outOfBounds = true;
    }
    if (outOfBounds) {
      Brick boundedBrick(bufferToFill.SampleDimension(), bufferToFill.LineDimension(), bufferToFill.BandDimension(), GdalPixelToIsis(m_pixelType));
      boundedBrick.SetBasePosition(lineStart + 1, sampleStart + 1, bufferToFill.Band());
      CPLErr err = poBand->RasterIO(GF_Read, sampleStart, lineStart,
                                    sampleSize, lineSize,
                                    boundedBrick.RawBuffer(),
                                    boundedBrick.SampleDimension(), boundedBrick.LineDimension(),
                                    m_pixelType,
                                    0, 0);

      // Handle pixel type conversion
      char *buffersRawBuf = (char *)boundedBrick.RawBuffer();
      double *buffersDoubleBuf = boundedBrick.DoubleBuffer();
      for (int bufferIdx = 0; bufferIdx < boundedBrick.size(); bufferIdx++) {
        readPixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx);
      }
      bufferToFill.CopyOverlapFrom(boundedBrick);
    }
    else {
      // silence warnings
      CPLErr err = poBand->RasterIO(GF_Read, sampleStart, lineStart,
                                    sampleSize, lineSize,
                                    bufferToFill.RawBuffer(),
                                    bufferToFill.SampleDimension(), bufferToFill.LineDimension(),
                                    m_pixelType,
                                    0, 0);

      // Handle pixel type conversion
      char *buffersRawBuf = (char *)bufferToFill.RawBuffer();
      double *buffersDoubleBuf = bufferToFill.DoubleBuffer();
      for (int bufferIdx = 0; bufferIdx < bufferToFill.size(); bufferIdx++) {
        readPixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx);
      }
    }
  }

  void GdalIoHandler::write(const Buffer &bufferToWrite) {
    GDALRasterBand  *poBand;
    m_maskBuff = (char *) CPLMalloc(sizeof(char) * bufferToWrite.size());
    for (int i = 0; i < bufferToWrite.size(); i++) {
      m_maskBuff[i] = 255;
    }
    
    int band = bufferToWrite.Band();
    if(m_virtualBands) 
      band = m_virtualBands->indexOf(band) + 1;

    poBand = m_geodataSet->GetRasterBand(band);

    int lineStart = bufferToWrite.Line() - 1;
    int sampleStart = bufferToWrite.Sample() - 1;
    int lineEnd = bufferToWrite.LineDimension();
    int sampleEnd = bufferToWrite.SampleDimension();
    // Handle buffers that exit valid DN dimensions
    // if (lineStart <= 0) {
    //   lineStart = 0;
    // }
    // if (lineStart + bufferToWrite.LineDimension() > m_lines) {
    //   lineEnd = m_lines - lineStart;
    // }
    // if (sampleStart <= 0) {
    //   sampleStart = 0;
    // }
    // if (sampleStart + bufferToWrite.SampleDimension() > m_samples) {
    //   sampleEnd = m_samples - sampleStart;
    // }

    // Handle pixel type conversion
    char *buffersRawBuf = (char *)bufferToWrite.RawBuffer();
    double *buffersDoubleBuf = bufferToWrite.DoubleBuffer();
    for (int bufferIdx = 0; bufferIdx < bufferToWrite.size(); bufferIdx++) {
      if (writePixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx)) {
        m_maskBuff[bufferIdx] = 0;
      };
    }

    // silence warning
    CPLErr err = poBand->RasterIO(GF_Write, sampleStart, lineStart,
                                  sampleEnd, lineEnd,
                                  bufferToWrite.RawBuffer(),
                                  bufferToWrite.SampleDimension(), bufferToWrite.LineDimension(),
                                  m_pixelType,
                                  0, 0);
    poBand = m_geodataSet->GetRasterBand(band)->GetMaskBand();
    err = poBand->RasterIO(GF_Write, sampleStart, lineStart,
                           sampleEnd, lineEnd,
                           m_maskBuff,
                           bufferToWrite.SampleDimension(), bufferToWrite.LineDimension(),
                           GDT_Byte,
                           0, 0);
    free(m_maskBuff);
  }

  BigInt GdalIoHandler::getDataSize() const {
    return 0;
  }
  /**
   * Function to update the labels with a Pvl object
   *
   * @param labels Pvl object to update with
   */
  void GdalIoHandler::updateLabels(Pvl &labels) {}

  void GdalIoHandler::readPixelType(double *doubleBuff, void *rawBuff, int idx) const {
    double &bufferVal = doubleBuff[idx];
    if (m_pixelType == GDT_Float64) {
      bufferVal = ((double *)rawBuff)[idx];
    }
    
    else if(m_pixelType == GDT_Float32) {
      float raw = ((float *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->Float(&raw);

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
      ((unsigned int *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_UInt32) {
      unsigned int raw = ((unsigned int *)rawBuff)[idx];
      // if(m_byteSwapper)
      //   raw = m_byteSwapper->Uint32_t(&raw);

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