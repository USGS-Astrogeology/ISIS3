/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "TiffExporter.h"

#include <QDebug>

#include "Buffer.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "UserInterface.h"

using namespace Isis;


namespace Isis {
  /**
   * Construct the TIFF exporter.
   */
  TiffExporter::TiffExporter() : StreamExporter() {
    m_image = NULL;
    m_raster = NULL;

    setExtension("tif");
  }


  /**
   * Destruct the exporter.
   */
  TiffExporter::~TiffExporter() {
    if (m_image) {
      TIFFClose(m_image);
      m_image = NULL;
    }

    delete [] m_raster;
    m_raster = NULL;
  }


  /**
   * Creates the buffer to store a chunk of streamed line data with one or more
   * bands.
   */
  void TiffExporter::createBuffer() {
    PixelType type = pixelType();
    int mult = (type == Isis::UnsignedByte) ? 1 : 2;
    int size = samples() * bands() * mult;

    try {
      m_raster = new unsigned char[size];
    }
    catch (...) {
      throw IException(IException::Unknown,
          "Could not allocate enough memory", _FILEINFO_);
    }
  }


  /**
   * Open the output file for writing, initialize its fields, then let the base
   * ImageExporter handle the generic black-box writing routine.
   *
   * @param outputName The filename of the output cube
   * @param quality The quality of the output, not used for TIFF
   * @param compression The compression algorithm used. Currenly supports
   *                         "packbits", "lzw", "deflate", and "none".
   */
  void TiffExporter::write(FileName outputName, int quality,
                           QString compression, UserInterface *ui) {

    outputName = outputName.addExtension(extension().toStdString());

    // Open the output image
    m_image = TIFFOpen(outputName.expanded().c_str(), "w");

    if (m_image == NULL) {
      throw IException(IException::Programmer,
          "Could not open output image", _FILEINFO_);
    }

    TIFFSetField(m_image, TIFFTAG_IMAGEWIDTH, samples());
    TIFFSetField(m_image, TIFFTAG_IMAGELENGTH, lines());
    TIFFSetField(m_image, TIFFTAG_ROWSPERSTRIP, 1);
    if (compression == "packbits") {
      TIFFSetField(m_image, TIFFTAG_COMPRESSION, COMPRESSION_PACKBITS);
    }
    else if (compression == "lzw") {
      TIFFSetField(m_image, TIFFTAG_COMPRESSION, COMPRESSION_LZW);
    }
    else if (compression == "deflate") {
      TIFFSetField(m_image, TIFFTAG_COMPRESSION, COMPRESSION_ADOBE_DEFLATE);
    }
    else if (compression == "none") {
      TIFFSetField(m_image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    }
    else {
      std::string msg = "Invalid TIFF compression algorithm: " + compression.toStdString();
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    TIFFSetField(m_image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    TIFFSetField(m_image, TIFFTAG_PHOTOMETRIC,
        bands() == 1 ? PHOTOMETRIC_MINISBLACK : PHOTOMETRIC_RGB);

    PixelType type = pixelType();
    int bps = (type == Isis::UnsignedByte) ? 8 : 16;
    TIFFSetField(m_image, TIFFTAG_BITSPERSAMPLE, bps);
    int sampleFormat = (type == Isis::SignedWord) ? 2 : 1 ;
    TIFFSetField(m_image, TIFFTAG_SAMPLEFORMAT, sampleFormat);

    TIFFSetField(m_image, TIFFTAG_SAMPLESPERPIXEL, bands());

    ImageExporter::write(outputName, quality, compression, ui);
  }


  /**
   * Set the DN value at the given sample and band, resolved to a single index,
   * of the line buffer.
   *
   * @param s The sample component of the index into the buffer
   * @param b The band component of the index into the buffer
   * @param dn The value to set at the given index
   */
  void TiffExporter::setBuffer(int s, int b, int dn) const {
    PixelType type = pixelType();
    int index = s * bands() + b;

    switch (type) {
      case UnsignedByte:
        m_raster[index] = (unsigned char) dn;
        break;
      case SignedWord:
        ((short int *) m_raster)[index] = (short int) dn;
        break;
      case UnsignedWord:
        ((short unsigned int *) m_raster)[index] = (short unsigned int) dn;
        break;
      default:
        throw IException(IException::Programmer,
            "Invalid pixel type for data [" + toString(type) + "]",
            _FILEINFO_);
    }
  }


  /**
   * Writes a line of buffered data to the output image on disk.
   *
   * @param l The line of the output image
   */
  void TiffExporter::writeLine(int l) const {
    if (!TIFFWriteScanline(m_image, m_raster, l)) {
      throw IException(IException::Programmer,
          "Could not write image", _FILEINFO_);
    }
  }


  /**
   * Returns true if the format is "tiff".
   *
   * @param format Lowercase format abbreviation
   *
   * @return True if "tiff", false otherwise
   */
  bool TiffExporter::canWriteFormat(QString format) {
    return format == "tiff";
  }
};

