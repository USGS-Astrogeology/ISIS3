/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ImageExporter.h"

#include "Buffer.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "ExportDescription.h"
#include "FileName.h"
#include "JP2Exporter.h"
#include "PixelType.h"
#include "ProcessExport.h"
#include "QtExporter.h"
#include "TiffExporter.h"
#include "UserInterface.h"

using namespace Isis;
using namespace std;


namespace Isis {
  /**
   * Construct the exporter.
   */
  ImageExporter::ImageExporter() {
    m_process = NULL;
    m_process = new ProcessExport;

    m_writeMethod = NULL;

    m_exportDescription = NULL;
    m_exportDescription = new ExportDescription();

    m_extension = "";
    m_worldExtension = "";

    m_samples = 0;
    m_lines = 0;
    m_bands = 0;

    m_outputPixelMinimum = 0.0;
    m_outputPixelMaximum = 0.0;
  }


  /**
   * Generic initialization with the export description.  Set the export
   * description given the pixel type of the passed in description. Use
   * the number of channels in the export description to determine the
   * write method (i.e. grayscale, rgb, or rgba). This will also set the
   * member variables for the number of samples, lines, and bands.
   *
   * @param desc Export description containing necessary channel information
   */
  void ImageExporter::initialize(ExportDescription &desc) {
    setExportDescription(desc);
    initializeProcess();
  }

  /**
   * Destruct the exporter.  Also deletes the process object.
   */
  ImageExporter::~ImageExporter() {
    delete m_process;
    m_process = NULL;
    delete m_exportDescription;
    m_exportDescription = NULL;
  }


  /**
   * The method for writing a line of input image data (with potentially several
   * bands representing color channels) to the output image.  It is called for
   * each line of data in the input images (provides a vector containing the
   * same line from each single-band input image).  Enables the exporter to be
   * used as a functor in a custom ProcessExport routine, bypassing the
   * black-box write() method entirely.
   *
   * @param in Vector containing a line of input data from each input channel
   */
  void ImageExporter::operator()(vector<Buffer *> &in) const {
    (this->*m_writeMethod)(in);
  }


  /**
   * Export the Isis cube channels to the given standard image.  If supported by
   * the specific exporter child, will also produce an image with the given
   * scaled quality. Will also use the selected compression algorithm if
   * supported by the image format. This will do a black-box export using a
   * ProcessExport routine. After the export is completed, a world file will
   * also be created if a map projection exists.
   *
   * @param outputName The filename of the output cube
   * @param quality The quality of the output from 0 to 100, defaults to 100
   * @param compression The compression algorithm used. Image format specific.
   * @param ui The optional user interface to set the input image pixel range.
   */
  void ImageExporter::write(FileName outputName, int quality,
                            QString compression, UserInterface *ui) {
    ProcessExport &p = process();
    if (!p.HasInputRange()) {
      if (ui) {
        p.SetInputRange(*ui);
      }
      else {
        p.SetInputRange();
      }
    }
    p.ProcessCubes(*this);

    outputName = outputName.addExtension(m_extension.toStdString());

    createWorldFile(outputName);
  }


  /**
   * Number of samples (columns) in the output image.
   *
   * @return The width of the output
   */
  int ImageExporter::samples() const {
    return m_samples;
  }


  /**
   * Number of lines (rows) in the output image.
   *
   * @return The height of the output
   */
  int ImageExporter::lines() const {
    return m_lines;
  }


  /**
   * Number of bands (channels) in the output image.
   *
   * @return The depth of the output
   */
  int ImageExporter::bands() const {
    return m_bands;
  }


  /**
   * Returns the input minimum for the given channel.  Any value of min in the
   * input cube will be mapped to the output min of the output channel.
   *
   * @param channel Index of the channel within the process
   *
   * @return The minimum DN in the input mapped to the minimum of the output
   */
  double ImageExporter::inputMinimum(int channel) const {
    return m_process->GetInputMinimum(channel);
  }


  /**
   * Returns the input maximum for the given channel.  Any value of max in the
   * input cube will be mapped to the output max of the output channel.
   *
   * @param channel Index of the channel within the process
   *
   * @return The maximum DN in the input mapped to the maximum of the output
   */
  double ImageExporter::inputMaximum(int channel) const {
    return m_process->GetInputMaximum(channel);
  }


  /**
   * Set the DN floor and ceiling for the exported image.  All DNs less than
   * the min will be set to the min in the exported image.  Similarly, all DNs
   * greater than the max will be set to the max in the exported image.
   *
   * Note: These values may be "special".  For example, if Null pixels are
   * exported to 0.0 and the minimum valid pixels are exported to 2.0, then 0.0
   * should be passed in for the value of the @b min parameter.
   *
   * @param min The absolute minimum output DN value.
   * @param max The absolute maximum output DN value.
   */
  void ImageExporter::setOutputPixelRange(double outputPixelMinimum, double outputPixelMaximum) {
    m_outputPixelMinimum = outputPixelMinimum;
    m_outputPixelMaximum = outputPixelMaximum;
  }


  /**
   * Sets the extension for the output image and generates the extension for the
   * world file from it.
   *
   * @param extension The extension for the output image
   */
  void ImageExporter::setExtension(QString extension) {
    m_extension = extension;

    // World file extension is the first and last characters of the extension
    // with an added 'w' at the end
    int last = extension.length() - 1;
    m_worldExtension = extension.mid(0, 1) + extension.mid(last) + "w";
  }


  /**
   * Gets the extension for the output image.
   *
   * @return The extension for the output image
   */
  QString ImageExporter::extension() const {
    return m_extension;
  }

  /**
   * Sets the description for the output image.
   *
   * @param desc The export description
   */
  void ImageExporter::setExportDescription(ExportDescription &desc) {
    *m_exportDescription = desc;
  }

  /**
   * Gets the description for the output image.
   *
   * @return The export description
   */
  ExportDescription &ImageExporter::exportDescription() const {
    return *m_exportDescription;
  }


  /**
   * Sets up the export process with the parameters described within the given
   * description.
   *
   * This method determines determines whether to write the data as grayscale,
   * RGB, or RGBA. It then opens a cube for retrieving the input data and
   * establishing the dimensions of the output image. Next, the ProcessExport
   * format is set to BIL and the ProcessExport output pixel type, output valid
   * data range, and output null are set based on the given export description.
   * Last, the absolute output pixel range is set based on the given description
   * (this is the smallest and largest allowed pixel values in the output,
   * including "special" pixel values).
   *
   * @return A cube pointer to the first channel created, owned by the process
   */
  Cube *ImageExporter::initializeProcess() {
    switch (m_exportDescription->channelCount()) {
      case 1:
        m_writeMethod = &ImageExporter::writeGrayscale;
        break;
      case 3:
        m_writeMethod = &ImageExporter::writeRgb;
        break;
      case 4:
        m_writeMethod = &ImageExporter::writeRgba;
        break;
      default:
        throw IException(IException::Programmer,
            "Cannot export an image with [" + Isis::toString(m_exportDescription->channelCount()) +
            "] channels",
            _FILEINFO_);
    }

    ProcessExport &p = process();
    Cube *cube = addChannel(0);
    m_samples = cube->sampleCount();
    m_lines = cube->lineCount();
    m_bands = m_exportDescription->channelCount();

    for (int i = 1; i < m_exportDescription->channelCount(); i++) addChannel(i);

    p.setFormat(ProcessExport::BIL);// why BIL and not default to BSQ??? Doesn't appear to make a
                                    // difference in output images

    // set up the output pixel type, special pixels and valid output range for
    // the stretch that will be performed by ProcessExport
    p.SetOutputType(exportDescription().pixelType());
    p.SetOutputRange(m_exportDescription->outputPixelValidMin(),
                     m_exportDescription->outputPixelValidMax());

    // the dafault value for the null
    p.SetOutputNull(m_exportDescription->outputPixelNull());

    // set the absolute min/max values for all pixels (including specials) in the output image
    setOutputPixelRange(m_exportDescription->outputPixelAbsoluteMin(),
                        m_exportDescription->outputPixelAbsoluteMax());
    return cube;
  }


  /**
   * Get a reference to the process object, useful for subclasses to access and
   * manipulate the process.
   *
   * @return A reference to the process object
   */
  ProcessExport &ImageExporter::process() const {
    return *m_process;
  }


  /**
   * Returns the pixel type.  Defaults to None if not set by the user.
   *
   * @return The pixel type: {None, UnsignedByte, SignedWord, UnsignedWord}
   */
  PixelType ImageExporter::pixelType() const {
    if (m_exportDescription) {
      return m_exportDescription->pixelType();
    }
    else {
      return Isis::None;
    }
  }


  /**
   * Return the output clamped integer pixel value from the input
   * double-precision DN.
   *
   * @param dn The input DN
   *
   * @return The pixel value for the output
   */
  int ImageExporter::outputPixelValue(double dn) const {
    if (dn < m_outputPixelMinimum) {
      return m_outputPixelMinimum;
    }
    else if (dn > m_outputPixelMaximum) {
      return m_outputPixelMaximum;
    }
    else {
      return dn;
    }
  }


  /**
   * Add a channel of input data to the process from the export description at
   * the given index.
   *
   * @param i Index of the channel to add within the export description
   *
   * @return A cube pointer to the channel added, owned by the process
   */
  Cube * ImageExporter::addChannel(int i) {
    ProcessExport &p = process();

    const ExportDescription::ChannelDescription &channel = m_exportDescription->channel(i);
    Cube *cube = p.SetInputCube(QString::fromStdString(channel.filename().expanded()), channel.attributes(), Isis::OneBand);

    if (channel.hasCustomRange())
      p.SetInputRange(channel.inputMinimum(), channel.inputMaximum(), i);

    return cube;
  }


  /**
   * Creates a world file is the input has a map projection.
   *
   * @param outputName Name of the world file
   */
  void ImageExporter::createWorldFile(FileName outputName) {
    outputName = outputName.removeExtension();
    outputName = outputName.addExtension(m_worldExtension.toStdString());

    ProcessExport &p = process();
    p.CreateWorldFile(QString::fromStdString(outputName.expanded()));
    p.EndProcess();
  }


  /**
   * A static (factory) method for constructing an ImageExporter instance from
   * an output format.  The specific subclass of the returned instance is
   * determined from the desired output image format provided.  Each subclass
   * has knowledge of whether or not it can write a particular format.  Because
   * the ability to export an image format is not mutually exclusive amongst
   * exporters, the order of condieration here matters.  For example, using a
   * TIFF exporter takes precedence over a Qt exporter for TIFF images, because
   * the former can process cubes greater than 2GB while the latter cannot.  It
   * is the caller's responsibility to delete the exporter instance when they
   * are finished with it.
   *
   * @param format The format for the output image to be created
   *
   * @return A pointer to the instantiated exporter owned by the caller
   */
  ImageExporter * ImageExporter::fromFormat(QString format) {
    ImageExporter *exporter = NULL;

    format = format.toLower();
    if (TiffExporter::canWriteFormat(format)) {
      exporter = new TiffExporter();
    }
    else if (JP2Exporter::canWriteFormat(format)) {
      exporter = new JP2Exporter();
    }
    else if (QtExporter::canWriteFormat(format)) {
      exporter = new QtExporter(format);
    }
    else {
      throw IException(IException::Programmer,
          "Cannot export image as format [" + format.toStdString() + "]",
          _FILEINFO_);
    }

    return exporter;
  }
};

