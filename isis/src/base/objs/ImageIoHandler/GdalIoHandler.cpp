#include "GdalIoHandler.h"

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
    // Check bands and apply virtual bands
    poBand = m_geodataSet->GetRasterBand(band);
    // Account for 1 based line and sample reading from ISIS process classes
    // as gdal reads 0 based lines and samples
    int lineStart = bufferToFill.Line() - 1;
    int sampleStart = bufferToFill.Sample() - 1;
    int lineEnd = bufferToFill.LineDimension();
    int sampleEnd = bufferToFill.SampleDimension();
    if (lineStart < 0) {
      lineStart = 0;
    }
    if (lineStart + bufferToFill.LineDimension() > m_lines) {
      lineEnd = m_lines - lineStart;
    }
    if (sampleStart < 0) {
      sampleStart = 0;
    }
    if (sampleStart + bufferToFill.SampleDimension() > m_samples) {
      sampleEnd = m_samples - sampleStart;
    }
    // silence warnings
    poBand->RasterIO(GF_Read, sampleStart, lineStart, 
                              sampleEnd, lineEnd,
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

  void GdalIoHandler::write(const Buffer &bufferToWrite) {
    GDALRasterBand  *poBand;
    int band = bufferToWrite.Band();
    poBand = m_geodataSet->GetRasterBand(band);

    int lineStart = bufferToWrite.Line() - 1;
    int sampleStart = bufferToWrite.Sample() - 1;
    int lineEnd = bufferToWrite.LineDimension();
    int sampleEnd = bufferToWrite.SampleDimension();
    // Handle buffers the exit valid DN dimensions
    if (lineStart <= 0) {
      lineStart = 0;
    }
    if (lineStart + bufferToWrite.LineDimension() > m_lines) {
      lineEnd = m_lines - lineStart;
    }
    if (sampleStart <= 0) {
      sampleStart = 0;
    }
    if (sampleStart + bufferToWrite.SampleDimension() > m_samples) {
      sampleEnd = m_samples - sampleStart;
    }

    // Handle pixel type conversion
    char *buffersRawBuf = (char *)bufferToWrite.RawBuffer();
    double *buffersDoubleBuf = bufferToWrite.DoubleBuffer();
    for (int bufferIdx = 0; bufferIdx < bufferToWrite.size(); bufferIdx++) {
      writePixelType(buffersDoubleBuf, buffersRawBuf, bufferIdx);
    }

    // silence warning
    CPLErr err = poBand->RasterIO(GF_Write, sampleStart, lineStart,
                                  sampleEnd, lineEnd,
                                  bufferToWrite.RawBuffer(),
                                  bufferToWrite.SampleDimension(), bufferToWrite.LineDimension(),
                                  m_pixelType,
                                  0, 0);
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

  void GdalIoHandler::writePixelType(double *doubleBuff, void *rawBuff, int idx) const {
    double bufferVal = doubleBuff[idx];
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
      ((float *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int32) {
      int raw;

      if(bufferVal >= VALID_MINI4) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MINI4 - 0.5) {
          raw = LOW_REPR_SATI4;
        }
        if(filePixelValueDbl > VALID_MAXI4) {
          raw = HIGH_REPR_SATI4;
        }
        else {
          int filePixelValue = (int)round(filePixelValueDbl);

          if(filePixelValue < VALID_MINI4) {
            raw = LOW_REPR_SATI4;
          }
          else if(filePixelValue > VALID_MAXI4) {
            raw = HIGH_REPR_SATI4;
          }
          else {
            raw = filePixelValue;
          }
        }
      }
      else {
        if(bufferVal == NULL8)
          raw = NULLI4;
        else if(bufferVal == LOW_INSTR_SAT8)
          raw = LOW_INSTR_SATI4;
        else if(bufferVal == LOW_REPR_SAT8)
          raw = LOW_REPR_SATI4;
        else if(bufferVal == HIGH_INSTR_SAT8)
          raw = HIGH_INSTR_SATI4;
        else if(bufferVal == HIGH_REPR_SAT8)
          raw = HIGH_REPR_SATI4;
        else
          raw = LOW_REPR_SATI4;
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
      ((unsigned int *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int16) {
      short raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
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
      ((short *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_UInt16) {
      unsigned short raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
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
      ((unsigned short *)rawBuff)[idx] = raw;
    }

    else if(m_pixelType == GDT_Int8) {
     char raw;

      if(bufferVal >= VALID_MIN8) {
        double filePixelValueDbl = (bufferVal - m_offset) /
            m_scale;
        if(filePixelValueDbl < VALID_MINS1 - 0.5) {
          raw = LOW_REPR_SATS1;
        }
        else if(filePixelValueDbl > VALID_MAXS1 + 0.5) {
          raw = HIGH_REPR_SATS1;
        }
        else {
          int filePixelValue = (int)(filePixelValueDbl + 0.5);
          if(filePixelValue < VALID_MINS1) {
            raw = LOW_REPR_SATS1;
          }
          else if(filePixelValue > VALID_MAXS1) {
            raw = HIGH_REPR_SATS1;
          }
          else {
            raw = (char)(filePixelValue);
          }
        }
      }
      else {
        if(bufferVal == NULL8)
          raw = NULLS1;
        else if(bufferVal == LOW_INSTR_SAT8)
          raw = LOW_INSTR_SATS1;
        else if(bufferVal == LOW_REPR_SAT8)
          raw = LOW_REPR_SATS1;
        else if(bufferVal == HIGH_INSTR_SAT8)
          raw = HIGH_INSTR_SATS1;
        else if(bufferVal == HIGH_REPR_SAT8)
          raw = HIGH_REPR_SATS1;
        else
          raw = LOW_REPR_SATS1;
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
      ((unsigned char *)rawBuff)[idx] = raw;
    }
  }
}