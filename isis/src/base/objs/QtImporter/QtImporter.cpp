/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "QtImporter.h"

#include <QImage>

#include "FileName.h"
#include "IException.h"
#include "IString.h"

using namespace Isis;


namespace Isis {
  /**
   * Construct the importer.
   *
   * @param inputName The name of the input image
   */
  QtImporter::QtImporter(FileName inputName) : ImageImporter(inputName) {
    m_qimage = NULL;

    m_qimage = new QImage(QString::fromStdString(inputName.expanded()));
    if (m_qimage->isNull()) {
      throw IException(IException::User,
          "The file [" + inputName.expanded() +
          "] does not contain a recognized image format",
          _FILEINFO_);
    }

    setSamples(m_qimage->width());
    setLines(m_qimage->height());
    setDefaultBands();
  }


  /**
   * Destruct the importer.
   */
  QtImporter::~QtImporter() {
    delete m_qimage;
    m_qimage = NULL;
  }


  /**
   * Tests to see if the input image is grayscale (no RGB/A).
   *
   * @return True if the image is grayscale, false otherwise
   */
  bool QtImporter::isGrayscale() const {
    return m_qimage->isGrayscale();
  }


  /**
   * Tests to see if the input image is neither grayscale nor has an alpha
   * channel, implying RGB (no alpha).
   *
   * @return True if the image is RGB, false otherwise
   */
  bool QtImporter::isRgb() const {
    return !isGrayscale() && !isArgb();
  }


  /**
   * Tests to see if the input image is has an alpha channel, implying RGBA.
   *
   * @return True if the image is RGBA, false otherwise
   */
  bool QtImporter::isArgb() const {
    return m_qimage->hasAlphaChannel();
  }


  /**
   * Does nothing as Qt reads the entire input image into memory, and therefore
   * does not need to be updated throughout the import process.
   *
   * @param line Current line of the output buffer
   * @param band Current band of the output buffer
   */
  void QtImporter::updateRawBuffer(int line, int band) const {
  }


  /**
   * Returns a representation of a pixel for the input format that can then be
   * broken down into specific gray or RGB/A components.
   *
   * @param s The sample of the desired pixel
   * @param l The line of the desired pixel
   *
   * @return The pixel at the given sample and line of the input with all
   *         channel info
   */
  int QtImporter::getPixel(int s, int l) const {
    return m_qimage->pixel(s, l);
  }


  /**
   * Retrieves the gray component of the given pixel.
   *
   * @param pixel Representation of a Qt pixel value
   *
   * @return The gray component
   */
  int QtImporter::getGray(int pixel) const {
    return qGray(pixel);
  }


  /**
   * Retrieves the red component of the given pixel.
   *
   * @param pixel Representation of a Qt pixel value
   *
   * @return The red component
   */
  int QtImporter::getRed(int pixel) const {
    return qRed(pixel);
  }


  /**
   * Retrieves the green component of the given pixel.
   *
   * @param pixel Representation of a Qt pixel value
   *
   * @return The green component
   */
  int QtImporter::getGreen(int pixel) const {
    return qGreen(pixel);
  }


  /**
   * Retrieves the blue component of the given pixel.
   *
   * @param pixel Representation of a Qt pixel value
   *
   * @return The blue component
   */
  int QtImporter::getBlue(int pixel) const {
    return qBlue(pixel);
  }


  /**
   * Retrieves the alpha component of the given pixel.
   *
   * @param pixel Representation of a Qt pixel value
   *
   * @return The alpha component
   */
  int QtImporter::getAlpha(int pixel) const {
    return qAlpha(pixel);
  }
};

