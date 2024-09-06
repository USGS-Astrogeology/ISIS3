/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ImageImporter.h"

#include <QImageReader>

#include "Buffer.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "History.h"
#include "JP2Decoder.h"
#include "JP2Importer.h"
#include "ProcessByLine.h"
#include "PvlGroup.h"
#include "QtImporter.h"
#include "SpecialPixel.h"
#include "TiffImporter.h"

using namespace Isis;


namespace Isis {
  /**
   * Construct the importer.
   *
   * @param inputName The name of the input image
   */
  ImageImporter::ImageImporter(FileName inputName) {
    m_inputName = NULL;
    m_outCube = NULL;

    m_inputName = new FileName(inputName);
    m_outCube = new Cube;

    m_nullMin = DBL_MAX;
    m_nullMax = DBL_MIN;
    m_lrsMin = DBL_MAX;
    m_lrsMax = DBL_MIN;
    m_hrsMin = DBL_MAX;
    m_hrsMax = DBL_MIN;
  }


  /**
   * Destruct the importer.  Also deletes the output cube handle.
   */
  ImageImporter::~ImageImporter() {
    delete m_inputName;
    m_inputName = NULL;

    delete m_outCube;
    m_outCube = NULL;
  }


  /**
   * Pure virtual method for converting projection information in the file being imported to an 
   * ISIS Mapping group. 
   *  
   * @return An ISIS Mapping group
   */
  PvlGroup ImageImporter::convertProjection() const {
    PvlGroup map("Mapping");
    return map;
  }


  /**
   * The method for processing the output cube in place, called for each line of
   * the output image.  Enables the importer to be used as a functor in a custom
   * ProcessByLine routine, bypassing the black-box import() method entirely.
   * Care should be taken, however, to observe the requirements placed on the
   * structure of such a processing routine by this method.  For example, the
   * JP2Importer child requires that the processing proceed in the direction of
   * bands before lines, because its input data is structured in a BIL (band
   * interlaced by line) format.
   *
   * @param out A reference to a line of output data to be written to
   */
  void ImageImporter::operator()(Buffer &out) const {
    // Get the method responsible for finding the color component for the
    // current output pixel
    GetChannelMethod getChannel = getBandChannel(out.Band());

    // Updates the raw buffer of input data when only part of the image is
    // stored in memory at a time
    updateRawBuffer(out.Line(), out.Band());

    // Processing by line, so loop over every sample in the buffer and get its
    // color component for the current output band, filter based on our special
    // pixel ranges, then output the resulting DN
    int l = out.Line() - 1;
    for (int s = 0; s < out.SampleDimension(); s++) {
      out[s] = testSpecial((this->*getChannel)(getPixel(s, l)));
    }
  }


  /**
   * Import the image with default output attributes.
   *
   * @param outputName The filename of the output cube
   */
  Cube * ImageImporter::import(FileName outputName) {
    CubeAttributeOutput att;
    return import(outputName, att);
  }


  /**
   * Import the input image this instance was constructed with into the given
   * output Isis cube with the given output attributes.  This will do a
   * black-box import using a ProcessByLine routine.  The BandBin group will be
   * updated in the output cube with the names of the color channels included.
   * The output cube will be returned on completion of the import process so the
   * caller can continue to modify the labels.  The importer instance will
   * retain ownership of this cube, such that the cube handle will be
   * deallocated upon destruction of the importer.
   *
   * @param outputName The filename of the output cube
   * @param att The attributes for writing the output cube
   *
   * @return A handle on the newly imported Isis cube owned by the importer
   */
  Cube * ImageImporter::import(FileName outputName, CubeAttributeOutput &att) {
    ProcessByLine p;
    Cube *cube = createOutput(outputName, att);

    Pvl *label = cube->label();
    PvlGroup bandBin("BandBin");

    PvlKeyword name("Name");
    if (bands() == 1) {
      name += "Gray";
    }
    else if (bands() == 3 || bands() == 4) {
      name += "Red";
      name += "Green";
      name += "Blue";
      if (bands() == 4) name += "Alpha";
    }
    else {
      throw IException(IException::Programmer,
          "Cannot interpret BandBin for [" + IString(bands()) + "] band image",
          _FILEINFO_);
    }
    bandBin += name;
    PvlObject &cubeObj = label->findObject("IsisCube");
    cubeObj.addGroup(bandBin);

    PvlGroup mapping = convertProjection();
    if (mapping.keywords() > 0) {
      cubeObj.addGroup(mapping);
    }

    p.SetInputCube(cube);
    p.WriteHistory(*cube);
    p.SetProcessingDirection(ProcessByBrick::BandsFirst);
    p.ProcessCubeInPlace(*this, false);
    p.EndProcess();

    return cube;
  }


  /**
   * Create the output cube from the given filename and attributes.  Set its
   * dimensions based on those encapsulated by the importer.
   *
   * @param outputName The filename of the output cube
   * @param att The attributes for writing the output cube
   *
   * @return The newly created cube handle devoid of any data
   */
  Cube * ImageImporter::createOutput(
      FileName outputName, CubeAttributeOutput &att) {

    m_outCube->setDimensions(samples(), lines(), bands());
    m_outCube->create(QString::fromStdString(outputName.expanded()), att);
    return m_outCube;
  }


  /**
   * Set the number of bands to be created for the output cube based on the
   * number of color channels in the input image.
   */
  void ImageImporter::setDefaultBands() {
    setBands((isGrayscale()) ? 1 : (isArgb()) ? 4 : 3);
  }


  /**
   * Set the range of DN values within which a pixel from the input image will
   * be set to Null in the output.
   *
   * @param min Any DN less than this value will not be set to Null.
   * @param max Any DN greater than this value will not be set to Null.
   */
  void ImageImporter::setNullRange(double min, double max) {
    m_nullMin = min;
    m_nullMax = max;
  }


  /**
   * Set the range of DN values within which a pixel from the input image will
   * be set to LRS in the output.
   *
   * @param min Any DN less than this value will not be set to LRS.
   * @param max Any DN greater than this value will not be set to LRS.
   */
  void ImageImporter::setLrsRange(double min, double max) {
    m_lrsMin = min;
    m_lrsMax = max;
  }


  /**
   * Set the range of DN values within which a pixel from the input image will
   * be set to HRS in the output.
   *
   * @param min Any DN less than this value will not be set to HRS.
   * @param max Any DN greater than this value will not be set to HRS.
   */
  void ImageImporter::setHrsRange(double min, double max) {
    m_hrsMin = min;
    m_hrsMax = max;
  }


  /**
   * Set the sample dimension (width) of the output image.
   *
   * @param s The new sample dimension
   */
  void ImageImporter::setSamples(int s) {
    m_samples = s;
  }


  /**
   * Set the line dimension (height) of the output image.
   *
   * @param l The new line dimension
   */
  void ImageImporter::setLines(int l) {
    m_lines = l;
  }


  /**
   * Set the band dimension (depth) of the output image.  Because this importer
   * only works on Grayscale, RGB, and RGBA images, possible values are 1, 3,
   * and 4 for the respective color modes.
   *
   * @param b The new band dimension
   */
  void ImageImporter::setBands(int b) {
    if (b == 2 || b > 4)
      throw IException(IException::Programmer,
          "Cannot create an image with [" + IString(b) + "] bands",
          _FILEINFO_);

    m_bands = b;
  }


  /**
   * The sample dimension (width) of the output image.
   *
   * @return The sample dimension
   */
  int ImageImporter::samples() const {
    return m_samples;
  }


  /**
   * The line dimension (height) of the output image.
   *
   * @return The line dimension
   */
  int ImageImporter::lines() const {
    return m_lines;
  }


  /**
   * The band dimension (depth) of the output image.
   *
   * @return The band dimension
   */
  int ImageImporter::bands() const {
    return m_bands;
  }


  /**
   * The filename of the input image this instance was constructed with.
   *
   * @return A copy of the input filename
   */
  FileName ImageImporter::filename() const {
    return *m_inputName;
  }


  /**
   * Tests a pixel against the Null, HRS, and LRS ranges defined by the
   * importer's handler.  Any pixel value falling within one of these ranges
   * will be converted into the given type of special pixel.  In case of
   * overlapping ranges, these tests will be performed in the order mentioned at
   * the start of this description.  By default, these ranges are set such that
   * all incoming pixels will retain their original values.
   *
   * @param pixel The DN value to be tested
   *
   * @return The valid DN or special pixel if it fell within the special ranges
   */
  double ImageImporter::testSpecial(double pixel) const {
    if (pixel <= m_nullMax && pixel >= m_nullMin) {
      return Isis::NULL8;
    }
    else if (pixel <= m_hrsMax && pixel >= m_hrsMin) {
      return Isis::HIGH_REPR_SAT8;
    }
    else if (pixel <= m_lrsMax && pixel >= m_lrsMin) {
      return Isis::LOW_REPR_SAT8;
    }
    else {
      return pixel;
    }
  }


  /**
   * Retrieve the method responsible for fetching the color channel from the
   * input image corresponding to the current band out of output being filled.
   * This will always be the getGray() method for single band output images.
   * For RGB/A images, band 1 will be red, band 2 green, band 3 blue, and band 4
   * alpha.
   *
   * @param band The current band of the output image
   *
   * @return The method that converts input pixels into the current band's color
   *         component
   */
  ImageImporter::GetChannelMethod
    ImageImporter::getBandChannel(int band) const {

    GetChannelMethod getChannel;
    if (bands() == 1) {
      getChannel = &ImageImporter::getGray;
    }
    else {
      switch (band) {
        case 1:
          getChannel = &ImageImporter::getRed;
          break;
        case 2:
          getChannel = &ImageImporter::getGreen;
          break;
        case 3:
          getChannel = &ImageImporter::getBlue;
          break;
        case 4:
          getChannel = &ImageImporter::getAlpha;
          break;
        default:
          throw IException(IException::Programmer,
              "Cannot determine channel for band [" + IString(band) + "]",
              _FILEINFO_);
      }
    }
    return getChannel;
  }


  /**
   * Convert the current pixel, taken from an RGB/A image, and blend its
   * RGB components into a single grayscale DN.  The three color components are
   * weighted by the following formula:
   *   
   *   gray = (red * 11 + green * 16 + blue * 5) / 32
   *
   * This formula was taken from the Qt documentation on converting an RGB value
   * to grayscale: http://qt-project.org/doc/qt-4.8/qcolor.html#qGray-2
   *
   * @param pixel The pixel value to be broken up into RGB components and
   *        converted to grayscale
   *
   * @return The grayscale DN value
   */
  int ImageImporter::convertRgbToGray(int pixel) const {
    int red = getRed(pixel);
    int green = getBlue(pixel);
    int blue = getGreen(pixel);
    return (red * 11 + green * 16 + blue * 5) / 32;
  }


  /**
   * A static (factory) method for constructing an ImageImporter instance from
   * an input filename.  The specific subclass of the returned instance is
   * determined from the interpreted image format of the input image.  Such
   * tests are done by reading a minimal amount of the input data necessary to
   * determine the format.  It is the caller's responsibility to delete the
   * importer instance when they are finished with it.  Note that deleting the
   * importer will also delete the cube handle returned by the import() method.
   *
   * @param inputName The filename of the input image to be imported
   *
   * @return A pointer to the instantiated importer owned by the caller
   */
  ImageImporter * ImageImporter::fromFileName(FileName inputName) {
    ImageImporter *importer = NULL;

    QString format = QImageReader::imageFormat(QString::fromStdString(inputName.expanded()));
    if (format == "tiff") {
      importer = new TiffImporter(inputName);
    }
    else if (format != "" && format != "jp2") {
      importer = new QtImporter(inputName);
    }
    else if (JP2Decoder::IsJP2(inputName.expanded().c_str())) {
      importer = new JP2Importer(inputName);
    }
    else {
      throw IException(IException::Programmer,
          "Cannot determine image format for [" + inputName.expanded() + "]",
          _FILEINFO_);
    }

    return importer;
  }
};

