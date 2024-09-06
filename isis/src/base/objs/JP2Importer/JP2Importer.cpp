/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "JP2Importer.h"

#include "FileName.h"
#include "IException.h"
#include "JP2Decoder.h"
#include "ProcessImport.h"

using namespace Isis;


namespace Isis {
  /**
   * Construct the importer.
   *
   * @param inputName The name of the input image
   */
  JP2Importer::JP2Importer(FileName inputName) : ImageImporter(inputName) {
    m_decoder = NULL;
    m_buffer = NULL;

    try {
      // Determine if input file is a JPEG2000 file
      m_decoder = new JP2Decoder(QString::fromStdString(inputName.expanded()));
      m_decoder->OpenFile();
      setSamples(m_decoder->GetSampleDimension());
      setLines(m_decoder->GetLineDimension());
      setBands(m_decoder->GetBandDimension());

      int pixelBytes = m_decoder->GetPixelBytes();
      if (pixelBytes == 1) {
        m_pixelType = Isis::UnsignedByte;
      }
      else if (pixelBytes == 2) {
        bool signedData = m_decoder->GetSignedData();
        m_pixelType = signedData ? Isis::SignedWord : Isis::UnsignedWord;
      }
      else {
        throw IException(IException::User,
            "The file [" + filename().expanded() +
            "] contains unsupported data type",
            _FILEINFO_);
      }

      int pixelSize = Isis::SizeOf(m_pixelType);
      int readBytes = pixelSize * samples() * bands();

      m_buffer = new char* [bands()];
      for (int i = 0; i < bands(); i++) m_buffer[i] = new char [readBytes];
    }
    catch (IException &e) {
      throw IException(IException::Programmer,
          "The file [" + inputName.expanded() +
          "] cannot be opened as a JPEG 2000 file",
          _FILEINFO_);
    }
  }


  /**
   * Destruct the importer.
   */
  JP2Importer::~JP2Importer() {
    delete m_decoder;
    m_decoder = NULL;

    delete [] m_buffer;
    m_buffer = NULL;
  }


  /**
   * Tests to see if the input image is single-banded, implying grayscale (no
   * RGB/A).
   *
   * @return True if the image is grayscale, false otherwise
   */
  bool JP2Importer::isGrayscale() const {
    return m_decoder->GetBandDimension() == 1;
  }


  /**
   * Tests to see if the input image is triple-banded, implying RGB (no alpha).
   *
   * @return True if the image is RGB, false otherwise
   */
  bool JP2Importer::isRgb() const {
    return m_decoder->GetBandDimension() == 3;
  }


  /**
   * Tests to see if the input image is quadruple-banded, implying RGBA.
   *
   * @return True if the image is RGBA, false otherwise
   */
  bool JP2Importer::isArgb() const {
    return m_decoder->GetBandDimension() == 4;
  }


  /**
   * Updates the buffer used to store chunks of the input data at a time.  Reads
   * a single line of data from the input with all its color channels.  Uses the
   * pixel type to determine the size of each pixel value to read in.
   *
   * @param line Current line of the output buffer
   * @param band Current band of the output buffer
   */
  void JP2Importer::updateRawBuffer(int line, int band) const {
    // Only read a new chunk of data when we move to a new line, since we read
    // all the input bands for the current line at once
    // NOTE m_buffer is changed in this method, making the const-ness a lie
    // TODO make the buffer local to the operator() method and read all bands of
    // a line at a time with a ProcessByBrick
    if (band == 1) {
      if (m_pixelType == Isis::UnsignedByte)
        m_decoder->Read((unsigned char **) m_buffer);
      else
        m_decoder->Read((short int **) m_buffer);
    }
  }


  /**
   * Returns a representation of a pixel for the input format that can then be
   * broken down into specific gray or RGB/A components.
   *
   * @param s The sample of the desired pixel
   * @param l The line of the desired pixel
   *
   * @return The current sample, used as an index into the data buffer
   *
   * @todo This design was created before it was determined that JPEG 2000 does
   *       not have a traditional pixel representation used to get color
   *       components, so this is somewhat of a hack.
   */
  int JP2Importer::getPixel(int s, int l) const {
    return s;
  }


  /**
   * Retrieves the gray component of the given pixel.  For grayscale images,
   * simply returns the value in the single band.  For RGB/A, converts the RGB
   * components into grayscale.
   *
   * @param pixel Index into the line buffer corresponding to a sample
   *
   * @return The gray component
   */
  int JP2Importer::getGray(int pixel) const {
    return isGrayscale() ? getFromBuffer(pixel, 0) : convertRgbToGray(pixel);
  }


  /**
   * Retrieves the red component of the given pixel from the first band of the
   * input buffer.
   *
   * @param pixel Index into the line buffer corresponding to a sample
   *
   * @return The red component
   */
  int JP2Importer::getRed(int pixel) const {
    return getFromBuffer(pixel, 0);
  }


  /**
   * Retrieves the green component of the given pixel from the second band of
   * the input buffer.
   *
   * @param pixel Index into the line buffer corresponding to a sample
   *
   * @return The green component
   */
  int JP2Importer::getGreen(int pixel) const {
    return getFromBuffer(pixel, 1);
  }


  /**
   * Retrieves the blue component of the given pixel from the third band of the
   * input buffer.
   *
   * @param pixel Index into the line buffer corresponding to a sample
   *
   * @return The blue component
   */
  int JP2Importer::getBlue(int pixel) const {
    return getFromBuffer(pixel, 2);
  }


  /**
   * Retrieves the alpha component of the given pixel from the fourth band of
   * the input buffer.
   *
   * @param pixel Index into the line buffer corresponding to a sample
   *
   * @return The alpha component
   */
  int JP2Importer::getAlpha(int pixel) const {
    return getFromBuffer(pixel, 3);
  }


  /**
   * Retrieves the pixel value from the input buffer corresponding to the given
   * sample and band (the buffer contains an entire line).  Dependent upon the
   * pixel type of the input data.
   *
   * @param s Index into the line buffer corresponding to a sample
   * @param b Index into the line buffer corresponding to a band
   *
   * @return The pixel value of the given component
   */
  int JP2Importer::getFromBuffer(int s, int b) const {
    int value;

    switch (m_pixelType) {
      case Isis::UnsignedByte:
        value = (int) ((unsigned char *) m_buffer[b])[s];
        break;
      case Isis::UnsignedWord:
        value = (int) ((unsigned short int *) m_buffer[b])[s];
        break;
      case Isis::SignedWord:
        value = (int) ((short int *) m_buffer[b])[s];
        break;
      default:
        throw IException(IException::Programmer,
            "Unknown pixel type [" + IString(m_pixelType) + "]",
            _FILEINFO_);
    }

    return value;
  }
};

