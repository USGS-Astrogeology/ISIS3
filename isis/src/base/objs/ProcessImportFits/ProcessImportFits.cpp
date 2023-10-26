/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessImportFits.h"

#include <QDebug>
#include <QString>

#include <iostream>
#include <math.h>
#include <sstream>

#include "IException.h"
#include "IString.h"
#include "LineManager.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PixelType.h"
#include "SpecialPixel.h"
#include "UserInterface.h"

namespace Isis {

  /**
   * Constructor for ProcessImportFits
   */
  ProcessImportFits::ProcessImportFits() {
    m_fitsImageLabels = NULL;
    m_extraFitsLabels = NULL;
    m_headerSizes = NULL;
    m_dataStarts = NULL;
  }


  /**
   * Destructor for ProcessImportFits
   */
  ProcessImportFits::~ProcessImportFits() {
    delete m_fitsImageLabels;
    delete m_extraFitsLabels;
    delete m_headerSizes;
    delete m_dataStarts;
    m_file.close();
  }


  /**
   * Extract all the FITS labels from the file. This includes the image labels for the main 
   * and each extensions as well as any extras. 
   *
   */
  void ProcessImportFits::extractFitsLabels() {

    m_fitsImageLabels = new QList< PvlGroup * >;
    m_extraFitsLabels = new QList< PvlGroup * >;
    m_headerSizes = new QList < int >;
    m_dataStarts = new QList < int >;

    // Process each FITS label area, storing each in its own PvlGroup.
    char readBuf[81];
    IString line = "";
    unsigned int place;

    // The main FITS label starts at the beginning of the file.
    // FITS extension labels start after the previous data and on a 2880 byte boundry
    // Each FITS keyword in all lables is stored in 80 bytes (space padded to 80 if
    // necessary).

    // Start at the beginning of the file for the main FITS label
    m_file.seekg(0, std::ios_base::beg);

    // Read the first label line (80 chars)
    // We are assuming the file pointer is set to the beginning of the first/next label
    while (m_file.read(readBuf, 80) && m_file.gcount() == 80) {

      PvlGroup *fitsLabel = new PvlGroup("FitsLabels");

      readBuf[80] = '\0';
      line = readBuf;
      place = 80;

      // Process each fits label record (80 bytes) and place keyword, value pairs into PvlKeywords
      // with any associated comments.
      while (line.substr(0, 3) != "END") {

        // Check for blank lines
        if (line.substr(0, 1) != " " && line.substr(0, 1) != "/") {
          // Name of keyword
          PvlKeyword label(line.Token(" =")); // Stop on spaces OR equal sign
          if (QString::compare(QString::fromStdString(label.name()), "OBJECT", Qt::CaseInsensitive) == 0) {
            label.setName("TARGET");
            label.addComment("NOTE: This keyword name was changed from 'OBJECT' in the original "
                             "fit header file.");
          }
          // Remove up to beginning of data
          line.TrimHead(" =");
          line.TrimTail(" ");
          if (label.name() == "COMMENT" || label.name() == "HISTORY") {
            label += line;
          }
          else {
            // Check for a quoted value
            if (line.substr(0,1) == "'") {
              line.TrimHead("'");
              label += line.Token("'").TrimHead(" ").TrimTail(" ");
              line.TrimHead(" '");
            }
            else {
              // Access any remaining data without the trailing comment if there is one
              IString value = line.Token("/");
              // Clear to end of data
              value.TrimTail(" ");
              label += value;
              line.TrimHead(" ");
            }
            // If the line still has anything in it, treat it is as a comment.
            if (line.size() > 2) {
              line.TrimHead(" /");
              label.addComment(line);
              // A possible format for units, other possiblites exist.
              if (line != line.Token("[")) {
                label.setUnits(line.Token("[").Token("]"));
              }
            }
          }
          fitsLabel->addKeyword(label);
        }

        // Read the next label line
        m_file.read(readBuf, 80);
        readBuf[80] = '\0';
        line = readBuf;
        place += 80;
      }

      // Save off the PvlGroup and the number of records read from this label
      m_fitsImageLabels->append(fitsLabel);
      m_headerSizes->append((int)ceil(place / 2880.0));

      // The file pointer should be pointing at the end of the record that contained "END"
      // Move the file pointer past the padding after the "END" (i.e., points to start of data)
      std::streamoff jump;
      jump = m_headerSizes->last() * 2880 - place;
      m_file.seekg(jump, std::ios_base::cur);

      m_dataStarts->append(m_file.tellg());

      // Does this look like a label for a FITS image? Stop after the first label that does not
      // because we don't know how to move the file pointer past a non-image data extension.
      if (fitsLabel->hasKeyword("BITPIX") && fitsLabel->hasKeyword("NAXIS")) {
        
        // This section can only handle image data 
        // (i.e., keywords BITPIX & NAXIS & NAXISx must exist).
        if((int)fitsLabel->findKeyword("NAXIS") > 0) {
          int bytesPerPixel = 0;
          bytesPerPixel = (int)((*fitsLabel)["BITPIX"]);
          bytesPerPixel = std::abs(bytesPerPixel);
          bytesPerPixel /= 8;
          
          unsigned int axis1 = 1;
          axis1 = std::stoi((*fitsLabel)["NAXIS1"]);
          
          unsigned int axis2 = 1;
          if (fitsLabel->hasKeyword("NAXIS2")) {
            axis2 = std::stoi((*fitsLabel)["NAXIS2"]);
          }
          
          unsigned int axis3 = 1;
          if (fitsLabel->hasKeyword("NAXIS3")) {
            axis3 = std::stoi((*fitsLabel)["NAXIS3"]);
          }
          
          jump = (int)(ceil(bytesPerPixel * axis1 * axis2 * axis3 / 2880.0) * 2880.0);
          m_file.seekg(jump, std::ios_base::cur);
        }
        else {
          // Note: this will allow us to read extra label sections that have 0 axes,
          // but has image-related info (so BITPIX and NAXIS keywords exist). This
          // includes informational labels, as seen at the beginning of hayabusa2
          // images in this case, there is NO DATA, so no jump should be needed to
          // get to the next section.
          PvlGroup *extraLabelGroup = m_fitsImageLabels->last();
          extraLabelGroup->setName("FitsExtras");
          m_extraFitsLabels->append(extraLabelGroup);

          m_fitsImageLabels->removeLast();
          m_headerSizes->removeLast();
          m_dataStarts->removeLast();
        }
      }
      // Do we have at least on header that looks like it has image data? If so, we can continue,
      // but ignore the rest of the file because we don't know how to skip over a non-image data.
      else if (m_fitsImageLabels->size() > 1) {
        m_fitsImageLabels->removeLast();
        m_headerSizes->removeLast();
        m_dataStarts->removeLast();
        break;
      }

      else {
        QString msg = QObject::tr("The FITS file does not contain a section header that appears "
                                  "to describe an image [%1].").arg(m_name.toString());
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }


  /**
   * Supplies the extra FITS label corresponding to the given index. Zero
   * returns the first label in the FITS header that has no data. To access 
   * image labels, call fitsImageLabel().
   *  
   * @see fitsImageLabel() 
   *
   * @param labelNumber The index to the extra FITS header label. 
   *  
   * @return PvlGroup version of a FITS label corresponding to requested label number.
   */
  PvlGroup ProcessImportFits::extraFitsLabel(int labelNumber) const {
    if (labelNumber >= m_extraFitsLabels->size()) {
      QString msg = QObject::tr("The requested label number [%1], from file [%2] is "
                                "past the last extra group found in this FITS file. "
                                "Extra label count is [%3]").arg(labelNumber).
                                arg(m_name.expanded()).arg(m_extraFitsLabels->size()-1);
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (!m_extraFitsLabels) {
      QString msg = QObject::tr("The FITS label has not been initialized, "
                                "call setFitsFile() first.");
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if (m_extraFitsLabels->size() < labelNumber) {
      QString msg = QObject::tr("The requested FITS label number "
                                "was not found in file [%1].").arg(m_name.toString());
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *(*m_extraFitsLabels)[labelNumber];
  }


  /**
   * Supplies the FITS image label corresponding to the given index. Zero 
   * returns the first label that precedes actual image data. Extra labels that 
   * have no data can be accessed by calling extraFitsLabel(). 
   *  
   * @see extraFitsLabel() 
   *
   * @param labelNumber The index to the FITS header image label. 
   *  
   * @return PvlGroup version of a FITS label corresponding to requested label number.
   */
  PvlGroup ProcessImportFits::fitsImageLabel(int labelNumber) const {

    if (labelNumber >= m_fitsImageLabels->size()) {
      QString msg = QObject::tr("The requested label number [%1], from file [%2] is "
                                "past the last image group found in this FITS file. "
                                "Image label count is [%3]").arg(labelNumber).
                                arg(m_name.expanded()).arg(m_fitsImageLabels->size()-1);
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (!m_fitsImageLabels) {
      QString msg = QObject::tr("The FITS label has not been initialized, "
                                "call setFitsFile first.");
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else if (m_fitsImageLabels->size() < labelNumber) {
      QString msg = QObject::tr("The requested FITS label number "
                                "was not found in file [%1].").arg(m_name.toString());
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *(*m_fitsImageLabels)[labelNumber];
  }


  /**
   * Return a PVL instrument group populated with expected default values. 
   *  
   * NOTE: These values are not appropriate for all missions. See specific 
   * mission documentation to verify that these are the keywords needed.
   *
   * @param fitsLabel A PVL group containing keywords from the FITS label.
   *  
   * @return PvlGroup An ISIS cube instrument group filled with keywords from 
   *                  the given FITS label.
   */
  PvlGroup ProcessImportFits::standardInstrumentGroup(PvlGroup fitsLabel) const {

    // NOTE: This needs to be changed over to use translation files

    // Attempt to extract the standard instrument group keywords
    PvlGroup inst("Instrument");
    if (fitsLabel.hasKeyword("DATE-OBS")) {
      inst += PvlKeyword("StartTime", fitsLabel["DATE-OBS"][0]);
    }
    if (fitsLabel.hasKeyword("TARGET")) {
      inst += PvlKeyword("Target", fitsLabel["TARGET"][0]);
    }
    if (fitsLabel.hasKeyword("INSTRUME")) {
      inst += PvlKeyword("InstrumentId", fitsLabel["INSTRUME"][0]);
    }
    if (fitsLabel.hasKeyword("OBSERVER")) {
      inst += PvlKeyword("SpacecraftName", fitsLabel["OBSERVER"][0]);
    }

    return inst;
  }


  /**
   * Opens a FITS image file with header and reads the FITS labels. Note: 
   * This method does not handle detached label files. 
   *
   * @param fitsFile Name of the FITS file to open.
   */
  void ProcessImportFits::setFitsFile(FileName fitsFile) {
    m_name = fitsFile;

    SetInputFile(fitsFile.toString()); // Make sure the file exists

    m_file.open(fitsFile.expanded().toLocal8Bit().constData(), std::ios::in  | std::ios::binary);

    if (!m_file.is_open()) {
      QString msg = QObject::tr("Unable to open FITS formatted file [%1].")
                               .arg(fitsFile.toString());
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Get the FITS labels internalized
    extractFitsLabels();

    // Check to make sure it is a FITS file we can handle
    PvlGroup label = fitsImageLabel(0);
    if (label.hasKeyword("SIMPLE") && label["SIMPLE"][0] == "F") {
      QString msg = QObject::tr("The file [%1] cannot be processed. "
                                "It is an unsupported format.").
          arg(fitsFile.toString());
      throw IException(IException::User, msg, _FILEINFO_);
    }

    m_file.close();
  }


  /**
   * Sets the Process file structure parameters based on the given image label index. 
   *  
   * NOTE: The (DataPrefixBytes + DataSuffixByte) / PixelSize is subtracted from the number of 
   * samples before the output file is created. 
   *
   * @param labelNumber FITS image index. Zero indicates the first image label, one indicates
   *                    the first image extension, etc...
   *
   */
  void ProcessImportFits::setProcessFileStructure(int labelNumber) {

    if (labelNumber >= m_fitsImageLabels->size()) {
      QString msg = QObject::tr("The requested label number [%1], from file [%2] is "
                                "past the last image in this FITS file [%3].").arg(labelNumber).
                                arg(InputFile()).arg(m_fitsImageLabels->size()-1);
      throw IException(IException::User, msg, _FILEINFO_);
    }

    PvlGroup label = *(*m_fitsImageLabels)[labelNumber];

    // Set the ProcessImport to skip over all the previous images and their labels and the label for
    // this image. Don't save this info (think memory)
    SetFileHeaderBytes((*m_dataStarts)[labelNumber]);

    // Find pixel type. NOTE: There are several unsupported possiblites
    Isis::PixelType type;
    std::string msg = "";
    switch (std::stoi(label["BITPIX"][0])) {
      case 8:
        type = Isis::UnsignedByte;
        break;
      case 16:
        type = Isis::SignedWord;
        break;
      case 32:
        type = Isis::SignedInteger;
        break;
      case -32:
        type = Isis::Real;
        break;
      case 64:
        msg = "Signed 64-bit integer (long) pixel type is not supported for FITS imports.";
        throw IException(IException::User, msg, _FILEINFO_);
        break;
      case -64:
        type = Isis::Double;
        break;
      default:
        msg = "Unknown pixel type [" + label["BITPIX"][0] + "] is not supported for FITS imports.";
        throw IException(IException::User, msg, _FILEINFO_);
        break;
    }

    SetPixelType(type);

    // It is possible to have a NAXIS value of 0 meaning no data, the file could include
    // xtensions with data, however, those aren't supported because we need the code to know
    // how to skip over them.
    // NOTE: FITS files, at least the ones seen till now, do not specify a line prefix or suffix
    // data byte count. Some FITS files do have them and ISIS needs to remove them so it is not
    // considered part of the DNs. So, use the parent class' prefix/suffix byte count to reduce
    // the number of samples.
    if (Organization() == BSQ) {
      if (std::stoi(label["NAXIS"][0]) == 2) {
        SetDimensions(std::stoi(label["NAXIS1"][0])
                      - (DataPrefixBytes()+DataSuffixBytes())/SizeOf(type),
                      std::stoi(label["NAXIS2"][0]), 1);
      }
      else if (std::stoi(label["NAXIS"][0]) == 3) {
        SetDimensions(std::stoi(label["NAXIS1"][0]) 
                      - (DataPrefixBytes()+DataSuffixBytes())/SizeOf(type),
                      std::stoi(label["NAXIS2"][0]), std::stoi(label["NAXIS3"][0]));
      }
      else {
        std::string msg = "NAXIS count of [" 
                      + label["NAXIS"][0]
                      + "] is not supported for FITS imports.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else if (Organization() == BIL) {
      if (std::stoi(label["NAXIS"][0]) == 2) {
        SetDimensions(std::stoi(label["NAXIS1"][0]) 
                      - (DataPrefixBytes()+DataSuffixBytes())/SizeOf(type),
                      1, std::stoi(label["NAXIS2"][0]));
      }
      else if (std::stoi(label["NAXIS"][0]) == 3) {
        SetDimensions(std::stoi(label["NAXIS1"][0]) 
                      - (DataPrefixBytes()+DataSuffixBytes())/SizeOf(type),
                      std::stoi(label["NAXIS3"][0]), std::stoi(label["NAXIS2"][0]));
      }
      else {
        std::string msg = "NAXIS count of [" 
                      + label["NAXIS"][0] 
                      + "] is not supported for FITS imports.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else if (Organization() == BIP) {
      std::string msg = "BIP (Band Interleaved by Pixel) "
                    "organization is not supported for FITS imports.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    else {
      std::string msg = "Unknown organization is not supported for FITS imports.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Base and multiplier
    if (label.hasKeyword("BZERO")) {
      SetBase(std::stod(label["BZERO"][0]));
    }
    else {
      SetBase(0.0);
    }
    if (label.hasKeyword("BSCALE")) {
      SetMultiplier(std::stod(label["BSCALE"][0]));
    }
    else {
      SetMultiplier(1.0);
    }

    // Byte order
    SetByteOrder(Isis::Msb);

  }

} // end namespace Isis
