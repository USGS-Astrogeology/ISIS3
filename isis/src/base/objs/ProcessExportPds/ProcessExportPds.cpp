/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessExportPds.h"

#include <cmath>
#include <iostream>
#include <sstream>

#include "Endian.h"
#include "ExportPdsTable.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "PixelType.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlFormat.h"
#include "PvlToPvlTranslationManager.h"
#include "PvlFormatPds.h"
#include "SpecialPixel.h"
#include "Table.h"


using namespace std;

namespace Isis {
  /**
   * Default Constructor - Set to default the data members
   *
   */
  ProcessExportPds::ProcessExportPds() {
    m_label       = NULL;
    m_formatter   = NULL;
    m_exportType  = Stream;
    m_exportResolution  = Meter;

    m_forceBands = true;
    m_forceBandName = true;
    m_forceCenterFilterWavelength = true;
    m_forceBandwidth = true;
    m_forceBandStorageType = true;
    m_forceOffset = true;
    m_forceScalingFactor = true;
    m_forceSampleBits = true;
    m_forceSampleBitMask = true;
    m_forceSampleType = true;
    m_forceCoreNull = true;
    m_forceCoreLrs = true;
    m_forceCoreLis = true;
    m_forceCoreHrs = true;
    m_forceCoreHis = true;
    m_detachedLabel = false;

    m_pdsFileType = Image;
    m_tableRecords.clear();
    m_tableBuffers.clear();
  }

  /**
   * Destructor
   *
   */
  ProcessExportPds::~ProcessExportPds() {
    delete m_label;
    delete m_formatter;
    for (unsigned int i = 0; i < m_tableBuffers.size(); i++) {
      delete [] m_tableBuffers[i];
      m_tableBuffers[i] = NULL;
    }
    m_tableBuffers.clear();
  }

  /**
   * Create a standard PDS label of the type specified
   *
   * Create a standard PDS label of the type specified. The type must be Image.
   * Qube and SpectralQube are not supported.
   *
   * NOTE: This method should not be called until all setting have been
   * finished. For example ProcessExport::SetOutputEndian
   *
   * @param type The type of PDS label to be created.
   * @return Pvl containing label information
   *
   * @throws Isis::iException::Message
   */
  Pvl &ProcessExportPds::StandardPdsLabel(ProcessExportPds::PdsFileType type) {
    m_label = new Pvl;

    m_pdsFileType = type;
    if(m_pdsFileType == ProcessExportPds::JP2Image) {
      setFormat(JP2);
    }

    m_formatter = new PvlFormatPds("$ISISROOT/appdata/translations/pdsExportRootGen.typ");
    m_label->setFormat(m_formatter);
    m_label->setTerminator("END");

    if(type == ProcessExportPds::Image || type == ProcessExportPds::JP2Image) {
      CreateImageLabel();
    }
    else {
      std::string msg = "Unsupported PDS output type";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    return *m_label;
  }


  /**
   * Create a standard PDS label for type IMAGE
   */
  void ProcessExportPds::CreateImageLabel() {

    Pvl &mainPvl = *m_label;

    if(m_exportType == Stream) {
      if(m_pdsFileType == ProcessExportPds::Image) {
        StreamImageRoot(mainPvl);
      }
      else if(m_pdsFileType == ProcessExportPds::JP2Image) {
        StreamJP2ImageRoot(mainPvl);
      }
    }
    else if(m_exportType == Fixed) {
      if(m_pdsFileType == ProcessExportPds::Image) {
        FixedImageRoot(mainPvl);
      }
      else if(m_pdsFileType == ProcessExportPds::JP2Image) {
        FixedJP2ImageRoot(mainPvl);
      }
    }
    else {
      std::string msg = "Invalid PDS export type.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(m_pdsFileType == ProcessExportPds::JP2Image) {
      StandardJP2Image(mainPvl);
    }
    else {
      StandardImageImage(mainPvl);
    }

    // The IMAGE_MAP_PROJECTION group is located in the ROOT for PDS IMAGEs. The
    // standard routines will add the IMAGE_MAP_PROJECTION correctly
    StandardAllMapping(mainPvl);
    mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportAllMapping.typ");
  }


  /**
   * Create a standard PDS label for type QUBE
   */
  void ProcessExportPds::CreateQubeLabel() {
    Pvl &mainPvl = *m_label;

//    StandardQubeRoot (mainPvl);
//    StandardQubeQube (mainPvl);

    // The IMAGE_MAP_PROJECTION group is located inside the QUBE object for PDS
    // QUBEs. Create a temporary PVL so the StandardAllMapping member can add an
    // IMAGE_MAP_PROJECTION group then later it can be extracted and added to
    // the output PDS label.
    Pvl mapTmp;
    StandardAllMapping(mapTmp);
    if(mapTmp.hasObject("IMAGE_MAP_PROJECTION")) {
      mainPvl.findObject("QUBE").addObject(mapTmp.findObject("IMAGE_MAP_PROJECTION"));
    }
  }


  /**
   * Create a standard PDS label for type SPECTRAL_QUBE
   */
  void ProcessExportPds::CreateSpectralQubeLabel() {
    Pvl &mainPvl = *m_label;

//    StandardSpectralQubeRoot (mainPvl);
//    StandardSpectralQubeSpectralQube (mainPvl);

    // The IMAGE_MAP_PROJECTION group is located inside the SPECTRAL_QUBE object
    // for PDS SPECTRAL_QUBEs. Create a temporary PVL so the StandardAllMapping
    // member can add an IMAGE_MAP_PROJECTION group then later it can be
    // extracted and added to the output PDS label.
    Pvl mapTmp;
    StandardAllMapping(mapTmp);
    if(mapTmp.hasObject("IMAGE_MAP_PROJECTION")) {
      mainPvl.findObject("QUBE").addObject(mapTmp.findObject("IMAGE_MAP_PROJECTION"));
    }
  }


  /**
   * Create the standard keywords for the ROOT object in a PDS IMAGE file
   *
   * @param mainPvl
   */
  void ProcessExportPds::StreamImageRoot(Pvl &mainPvl) {
    // Create standard ROOT object keywords
    mainPvl += PvlKeyword("PDS_VERSION_ID", "PDS3");
    mainPvl += PvlKeyword("RECORD_TYPE", "UNDEFINED");
    // NOTE: WARNING: If the number of "?"s in the next few lines changes, you
    // must also changes the corresponding lines in the OutputLabel member
    mainPvl += PvlKeyword("LABEL_RECORDS", "???????", "BYTES");
    if(m_detachedLabel) {
      QString sImageFile = m_detachedPdsLabelFile;
      int iFound = sImageFile.indexOf(".lbl");
      if(iFound != -1) {
        sImageFile.replace(iFound, 4, ".img");
      }
      else {
        sImageFile += ".img";
      }
      FileName outFile(sImageFile.toStdString());
      mainPvl += PvlKeyword("^IMAGE", outFile.name());
    }
    else {
      mainPvl += PvlKeyword("^IMAGE", "???????", "BYTES");
    }
    // MD5 checksums are 128-bit -> 32 characters when stringified from hex.
    if (canGenerateChecksum()) {
      mainPvl += PvlKeyword("CHECKSUM", "????????????????????????????????");
    }
  }


  /**
   * Create the standard keywords for the ROOT object in a PDS JP2 IMAGE file
   *
   * @param mainPvl
   */
  void ProcessExportPds::StreamJP2ImageRoot(Pvl &mainPvl) {
    mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageJP2.typ");
    // Create standard ROOT object keywords
    mainPvl += PvlKeyword("PDS_VERSION_ID", "PDS3");
    QString sImageFile = m_detachedPdsLabelFile;
    if(m_detachedLabel) {
      int iFound = sImageFile.indexOf(".lbl");
      if(iFound != -1) {
        sImageFile.replace(iFound, 4, ".jp2");
      }
      else {
        sImageFile += ".jp2";
      }
    }
    else {
      std::string msg = "Labels must be detached for JP2 files.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    FileName outFile(sImageFile.toStdString());
    PvlObject cmpObj("COMPRESSED_FILE");
    cmpObj += PvlKeyword("FILE_NAME", outFile.name());
    cmpObj += PvlKeyword("RECORD_TYPE", "UNDEFINED");
    cmpObj += PvlKeyword("ENCODING_TYPE", "JP2");
    cmpObj += PvlKeyword("ENCODING_TYPE_VERSION_NAME", "ISO/IEC15444-1:2004");
    cmpObj += PvlKeyword("INTERCHANGE_FORMAT", "BINARY");
    FileName infilename(InputCubes[0]->fileName().toStdString());
    cmpObj += PvlKeyword("UNCOMPRESSED_FILE_NAME", infilename.name());
    int storagebytes = InputCubes[0]->sampleCount() * InputCubes[0]->lineCount();
    if(p_pixelType == Isis::Real) {
      std::string msg = "JPEG2000 does not support floating point data.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if(p_pixelType == Isis::UnsignedWord || p_pixelType == Isis::SignedWord) {
      storagebytes = storagebytes * 2;
    }
    cmpObj += PvlKeyword("REQUIRED_STORAGE_BYTES", toString(storagebytes));
    mainPvl.addObject(cmpObj);
    PvlObject ucmpObj("UNCOMPRESSED_FILE");
    ucmpObj += PvlKeyword("FILE_NAME", infilename.name());
    ucmpObj += PvlKeyword("RECORD_TYPE", "FIXED_LENGTH");
    int recordbytes = InputCubes[0]->sampleCount();
    if(p_pixelType == Isis::UnsignedWord || p_pixelType == Isis::SignedWord) {
      recordbytes = recordbytes * 2;
    }
    ucmpObj += PvlKeyword("RECORD_BYTES", toString(recordbytes));
    ucmpObj += PvlKeyword("FILE_RECORDS", toString(InputCubes[0]->lineCount()));
    ucmpObj += PvlKeyword("^IMAGE", infilename.name());
    mainPvl.addObject(ucmpObj);
  }


  /**
   * Create the fixed keywords for the ROOT object in a PDS IMAGE file
   *
   * @param mainPvl
   */
  void ProcessExportPds::FixedImageRoot(Pvl &mainPvl) {
    //Create fixed ROOT object keywords
    mainPvl += PvlKeyword("PDS_VERSION_ID", "PDS3");
    mainPvl += PvlKeyword("RECORD_TYPE", "FIXED_LENGTH");
    // NOTE: WARNING: If the number of "?"s in the next few lines changes, you
    // must also changes the corresponding lines in the OutputLabel member
    mainPvl += PvlKeyword("RECORD_BYTES", "???????");
    mainPvl += PvlKeyword("FILE_RECORDS", "???????");
    mainPvl += PvlKeyword("LABEL_RECORDS", "????");
    if(m_detachedLabel) {
      QString sImageFile = m_detachedPdsLabelFile;
      int iFound = sImageFile.indexOf(".lbl");
      if(iFound != -1) {
        sImageFile.replace(iFound, 4, ".img");
      }
      else {
        sImageFile += ".img";
      }
      FileName outFile(sImageFile.toStdString());
      mainPvl += PvlKeyword("^IMAGE", outFile.name());
    }
    else {
      mainPvl += PvlKeyword("^IMAGE", "???");
    }
    if (canGenerateChecksum()) {
      mainPvl += PvlKeyword("CHECKSUM", "????????????????????????????????");
    }
  }


  /**
   * Create the fixed keywords for the ROOT object in a PDS JP2 IMAGE file
   *
   * @param  mainPvl
   */
  void ProcessExportPds::FixedJP2ImageRoot(Pvl &mainPvl) {
    mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageJP2.typ");
    //Create fixed ROOT object keywords
    mainPvl += PvlKeyword("PDS_VERSION_ID", "PDS3");
    QString sImageFile = m_detachedPdsLabelFile;
    if(m_detachedLabel) {
      int iFound = sImageFile.indexOf(".lbl");
      if(iFound != -1) {
        sImageFile.replace(iFound, 4, ".jp2");
      }
      else {
        sImageFile += ".jp2";
      }
    }
    else {
      std::string msg = "Labels must be detached for JP2 files.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    FileName outFile(sImageFile.toStdString());
    PvlObject cmpObj("COMPRESSED_FILE");
    cmpObj += PvlKeyword("FILE_NAME", outFile.name());
    cmpObj += PvlKeyword("RECORD_TYPE", "UNDEFINED");
    cmpObj += PvlKeyword("ENCODING_TYPE", "JP2");
    cmpObj += PvlKeyword("ENCODING_TYPE_VERSION_NAME", "ISO/IEC15444-1:2004");
    cmpObj += PvlKeyword("INTERCHANGE_FORMAT", "BINARY");
    FileName infilename(InputCubes[0]->fileName().toStdString());
    cmpObj += PvlKeyword("UNCOMPRESSED_FILE_NAME", infilename.name());
    int storagebytes = InputCubes[0]->sampleCount() * InputCubes[0]->lineCount();
    if(p_pixelType == Isis::Real) {
      std::string msg = "JPEG2000 does not support floating point data.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    if(p_pixelType == Isis::UnsignedWord || p_pixelType == Isis::SignedWord) {
      storagebytes = storagebytes * 2;
    }
    cmpObj += PvlKeyword("REQUIRED_STORAGE_BYTES", toString(storagebytes));
    mainPvl.addObject(cmpObj);
    PvlObject ucmpObj("UNCOMPRESSED_FILE");
    ucmpObj += PvlKeyword("FILE_NAME", infilename.name());
    ucmpObj += PvlKeyword("RECORD_TYPE", "FIXED_LENGTH");
    int recordbytes = InputCubes[0]->sampleCount();
    if(p_pixelType == Isis::UnsignedWord || p_pixelType == Isis::SignedWord) {
      recordbytes = recordbytes * 2;
    }
    ucmpObj += PvlKeyword("RECORD_BYTES", toString(recordbytes));
    ucmpObj += PvlKeyword("FILE_RECORDS", toString(InputCubes[0]->lineCount()));
    ucmpObj += PvlKeyword("^IMAGE", infilename.name());
    mainPvl.addObject(ucmpObj);
  }


  /**
   * Create the standard keywords for an IMAGE object in a PDS IMAGE file.
   * This should not be called until after all settings have been made. The
   * labels may contain the wrong data if it is.
   *
   * @param  mainPvl
   *
   * @throws Isis::IException::Message
   */
  void ProcessExportPds::StandardImageImage(Pvl &mainPvl) {
    mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImage.typ");
    // Build up an IMAGE object:
    // Auto translate standard keywords for the IMAGE object
    Pvl *inputLabel = InputCubes[0]->label();
    FileName transfile;
    transfile = "$ISISROOT/appdata/translations/pdsExportImageImage.trn";
    PvlToPvlTranslationManager Xlator(*inputLabel, QString::fromStdString(transfile.expanded()));
    Xlator.Auto(mainPvl);

    // Calculate the core base/mult for this cube
    double base = 0.0;
    double multiplier = 1.0;
    double x1, x2;

    double minimum = (p_inputMinimum.size()) ? p_inputMinimum[0] : 0.0;
    double maximum = (p_inputMaximum.size()) ? p_inputMaximum[0] : 0.0;

    for(unsigned int i = 0; i < p_inputMinimum.size(); i ++) {
      minimum = std::min(minimum, p_inputMinimum[i]);
      maximum = std::max(maximum, p_inputMaximum[i]);
    }

    x1 = p_outputMinimum;
    x2 = p_outputMaximum;

    if(p_inputMinimum.size() && p_pixelType == Isis::UnsignedByte) {
      multiplier = (maximum - minimum) / (x2 - x1);
      base = minimum - multiplier * x1;
    }
    else if(p_inputMinimum.size() && p_pixelType == Isis::SignedWord) {
      multiplier = (maximum - minimum) / (x2 - x1);
      base = minimum - multiplier * x1;
    }
    else if(p_inputMinimum.size() && p_pixelType == Isis::UnsignedWord) {
      multiplier = (maximum - minimum) / (x2 - x1);
      base = minimum - multiplier * x1;
    }

    // Manually set the keyword for the number of bits in a pixel
    // NOTE: this is dependent on settings in ProcessExport and not the cube
    PvlObject &imgObj = mainPvl.findObject("IMAGE");

    if(!m_forceBands) imgObj.deleteKeyword("BANDS");
    if(!m_forceBandName && imgObj.hasKeyword("BAND_NAME")) imgObj.deleteKeyword("BAND_NAME");
    if(!m_forceCenterFilterWavelength && imgObj.hasKeyword("CENTER_FILTER_WAVELENGTH")) imgObj.deleteKeyword("CENTER_FILTER_WAVELENGTH");
    if(!m_forceBandwidth && imgObj.hasKeyword("BANDWIDTH")) imgObj.deleteKeyword("BANDWIDTH");

    if(m_forceBandStorageType) imgObj += PvlKeyword("BAND_STORAGE_TYPE", "BAND_SEQUENTIAL");
    if(m_forceOffset) imgObj += PvlKeyword("OFFSET", toString(base));
    if(m_forceScalingFactor) imgObj += PvlKeyword("SCALING_FACTOR", toString(multiplier));

    // Manually set the keyword for pixel type and special pixels
    if(p_pixelType == Isis::UnsignedByte) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "8");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "MSB_UNSIGNED_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel8.typ");
    }
    else if((p_pixelType == Isis::UnsignedWord) && (p_endianType == Isis::Msb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "MSB_UNSIGNED_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if((p_pixelType == Isis::UnsignedWord) && (p_endianType == Isis::Lsb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "LSB_UNSIGNED_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if((p_pixelType == Isis::SignedWord) && (p_endianType == Isis::Msb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "MSB_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if((p_pixelType == Isis::SignedWord) && (p_endianType == Isis::Lsb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString((BigInt)0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "LSB_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if(p_pixelType == Isis::Real) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "32");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString((BigInt)0xffffffff));

      if(p_endianType == Isis::Msb) {
        if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "IEEE_REAL");
      }
      else {
        if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "PC_REAL");
      }
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString(Isis::INULL4));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString(Isis::ILOW_REPR_SAT4));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString(Isis::ILOW_INSTR_SAT4));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString(Isis::IHIGH_REPR_SAT4));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString(Isis::IHIGH_INSTR_SAT4));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel32.typ");
    }
    else {
      std::string msg = "Unsupported PDS pixel type or sample size";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Create the standard keywords for an IMAGE object in a PDS JP2 IMAGE file.
   * This should not be called until after all settings have been made. The
   * labels may contain the wrong data if it is.
   *
   * @param mainPvl
   *
   * @throws Isis::IException::Message
   */
  void ProcessExportPds::StandardJP2Image(Pvl &mainPvl) {
    mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImage.typ");
    if(m_pdsFileType == ProcessExportPds::JP2Image) {
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageJP2.typ");
    }
    // Build up a JP2 IMAGE object:
    // Auto translate standard keywords for the IMAGE object
    Pvl *inputLabel = InputCubes[0]->label();
    FileName transfile;
    transfile = "$ISISROOT/appdata/translations/pdsExportImageJP2.trn";
    PvlToPvlTranslationManager Xlator(*inputLabel, QString::fromStdString(transfile.expanded()));
    Xlator.Auto(mainPvl);

    // Calculate the core base/mult for this cube
    double base = 0.0;
    double multiplier = 1.0;
    double x1, x2;

    double minimum = (p_inputMinimum.size()) ? p_inputMinimum[0] : 0.0;
    double maximum = (p_inputMaximum.size()) ? p_inputMaximum[0] : 0.0;

    for(unsigned int i = 0; i < p_inputMinimum.size(); i ++) {
      minimum = std::min(minimum, p_inputMinimum[i]);
      maximum = std::max(maximum, p_inputMaximum[i]);
    }

    x1 = p_outputMinimum;
    x2 = p_outputMaximum;

    if(p_inputMinimum.size() && p_pixelType == Isis::UnsignedByte) {
      multiplier = (maximum - minimum) / (x2 - x1);
      base = minimum - multiplier * x1;
    }
    else if(p_inputMinimum.size() && p_pixelType == Isis::SignedWord) {
      multiplier = (maximum - minimum) / (x2 - x1);
      base = minimum - multiplier * x1;
    }
    else if(p_inputMinimum.size() && p_pixelType == Isis::UnsignedWord) {
      multiplier = (maximum - minimum) / (x2 - x1);
      base = minimum - multiplier * x1;
    }

    // Manually set the keyword for the number of bits in a pixel
    // NOTE: this is dependent on settings in ProcessExport and not the cube
    PvlObject &imgObj = mainPvl.findObject("UNCOMPRESSED_FILE").findObject("IMAGE");

    if(!m_forceBands) imgObj.deleteKeyword("BANDS");
    if(!m_forceBandName && imgObj.hasKeyword("BAND_NAME")) imgObj.deleteKeyword("BAND_NAME");
    if(!m_forceCenterFilterWavelength && imgObj.hasKeyword("CENTER_FILTER_WAVELENGTH")) imgObj.deleteKeyword("CENTER_FILTER_WAVELENGTH");
    if(!m_forceBandwidth && imgObj.hasKeyword("BANDWIDTH")) imgObj.deleteKeyword("BANDWIDTH");

    if(m_forceBandStorageType) imgObj += PvlKeyword("BAND_STORAGE_TYPE", "BAND_SEQUENTIAL");
    if(m_forceOffset) imgObj += PvlKeyword("OFFSET", toString(base));
    if(m_forceScalingFactor) imgObj += PvlKeyword("SCALING_FACTOR", toString(multiplier));

    // Manually set the keyword for pixel type and special pixels
    if(p_pixelType == Isis::UnsignedByte) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "8");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "MSB_UNSIGNED_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel8.typ");
    }
    else if((p_pixelType == Isis::UnsignedWord) && (p_endianType == Isis::Msb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "MSB_UNSIGNED_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if((p_pixelType == Isis::UnsignedWord) && (p_endianType == Isis::Lsb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "LSB_UNSIGNED_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if((p_pixelType == Isis::SignedWord) && (p_endianType == Isis::Msb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString(0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "MSB_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if((p_pixelType == Isis::SignedWord) && (p_endianType == Isis::Lsb)) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "16");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString((BigInt)0xffff));
      if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "LSB_INTEGER");
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString((int)OutputNull()));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString((int)OutputLrs()));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString((int)OutputLis()));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString((int)OutputHrs()));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString((int)OutputHis()));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel16.typ");
    }
    else if(p_pixelType == Isis::Real) {
      if(m_forceSampleBits) imgObj += PvlKeyword("SAMPLE_BITS", "32");
      if(m_forceSampleBitMask) imgObj += PvlKeyword("SAMPLE_BIT_MASK", toString((BigInt)0xffffffff));

      if(p_endianType == Isis::Msb) {
        if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "IEEE_REAL");
      }
      else {
        if(m_forceSampleType) imgObj += PvlKeyword("SAMPLE_TYPE", "PC_REAL");
      }
      if(m_forceCoreNull) imgObj += PvlKeyword("CORE_NULL", toString(Isis::INULL4));
      if(m_forceCoreLrs) imgObj += PvlKeyword("CORE_LOW_REPR_SATURATION", toString(Isis::ILOW_REPR_SAT4));
      if(m_forceCoreLis) imgObj += PvlKeyword("CORE_LOW_INSTR_SATURATION", toString(Isis::ILOW_INSTR_SAT4));
      if(m_forceCoreHrs) imgObj += PvlKeyword("CORE_HIGH_REPR_SATURATION", toString(Isis::IHIGH_REPR_SAT4));
      if(m_forceCoreHis) imgObj += PvlKeyword("CORE_HIGH_INSTR_SATURATION", toString(Isis::IHIGH_INSTR_SAT4));
      mainPvl.format()->add("$ISISROOT/appdata/translations/pdsExportImageImagePixel32.typ");
    }
    else {
      std::string msg = "Unsupported PDS pixel type or sample size";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Create the standard keywords for the IMAGE_MAP_PROJECTION group in a PDS
   * label
   *
   * @param mainPvl
   *
   * @throws Isis::IException::Message
   */
  void ProcessExportPds::StandardAllMapping(Pvl &outputPvl) {

    // Get the input Isis cube label and find the Mapping group if it has one
    Pvl *inputLabel = InputCubes[0]->label();
    if(inputLabel->hasObject("IsisCube") &&
        !(inputLabel->findObject("IsisCube").hasGroup("Mapping"))) return;
    PvlGroup &inputMapping = inputLabel->findGroup("Mapping", Pvl::Traverse);

    // Translate the projection specific keywords for a PDS IMAGE_MAP_PROJECTION
    QString projName = ProjectionName(*inputLabel);
    PvlToPvlTranslationManager xlatSpecProj(*inputLabel,
                                       "$ISISROOT/appdata/translations/pdsExport" + projName + ".trn");
    xlatSpecProj.Auto(outputPvl);

    // Translate the target name
    PvlToPvlTranslationManager xlatTarget(*inputLabel,
                                     "$ISISROOT/appdata/translations/pdsExportTarget.trn");
    xlatTarget.Auto(outputPvl);

    // Add keywords to the PDS labels that could not be handled automatically
    PvlObject &pdsMapObj = outputPvl.findObject("IMAGE_MAP_PROJECTION");

    // Add the projection name
//    pdsMapObj += PvlKeyword ("MAP_PROJECTION_TYPE", projName.toUpper());

    // Modify the radii to be km.
    PvlKeyword &aRadius = pdsMapObj["A_AXIS_RADIUS"];
    QString unit = QString::fromStdString(aRadius.unit());
    if( (unit.toUpper() == "METERS") || (unit == "") ) { //if no units, assume in meters
      double dValue = (double)aRadius;
      dValue /= 1000.0;
      aRadius.setValue(toString(dValue), "KM");
    }
    PvlKeyword &bRadius = pdsMapObj["B_AXIS_RADIUS"];
    unit = QString::fromStdString(bRadius.unit());
    if( (unit.toUpper() == "METERS") || (unit == "") ) {
      double dValue = (double)bRadius;
      dValue /= 1000.0;
      bRadius.setValue(toString(dValue), "KM");
    }
    PvlKeyword &cRadius = pdsMapObj["C_AXIS_RADIUS"];
    unit = QString::fromStdString(cRadius.unit());
    if( (unit.toUpper() == "METERS") || (unit == "") ) {
      double dValue = (double)cRadius;
      dValue /= 1000.0;
      cRadius.setValue(toString(dValue), "KM");
    }

    // Modify the units on MAP_SCALE and MAP_RESOLUTION
    PvlKeyword &mapScale = pdsMapObj["MAP_SCALE"];
    unit = QString::fromStdString(mapScale.unit());
    //if no units, assume in meters/pixel
    if( (unit.toUpper() == "METERS/PIX") || (unit.toUpper() == "METERS/PIXEL") || (unit == "") ) {
      if(m_exportResolution == Kilometer) {
        double dValue = (double)mapScale;
        dValue /= 1000.0;
        mapScale.setValue(toString(dValue), "KM/PIXEL");
      }
      else {
        mapScale.setValue(toString((double)mapScale), "METERS/PIXEL");
      }
    }
    PvlKeyword &mapRes = pdsMapObj["MAP_RESOLUTION"];
    unit = QString::fromStdString(mapRes.unit());
    //if no units, asume in pixels/degree
    if( (unit.toUpper() == "PIXELS/DEGREE") || (unit == "") ) {
      mapRes.setValue(mapRes, "PIX/DEG");
    }

    // Add the EASTERNMOST AND WESTERNMOST LONGITUDE keywords
    PvlKeyword &isisLonDir = inputMapping.findKeyword("LongitudeDirection");
    QString lonDir = QString::fromStdString(isisLonDir[0]);
    lonDir = lonDir.toUpper();
    if (inputMapping.hasKeyword("MaximumLongitude") && inputMapping.hasKeyword("MinimumLongitude")) {
      double maxLon = inputMapping.findKeyword("MaximumLongitude");
      double minLon = inputMapping.findKeyword("MinimumLongitude");
      if(lonDir == "POSITIVEEAST") {
        pdsMapObj += PvlKeyword("EASTERNMOST_LONGITUDE", toString(maxLon));
        pdsMapObj += PvlKeyword("WESTERNMOST_LONGITUDE", toString(minLon));
      }
      else {
        pdsMapObj += PvlKeyword("EASTERNMOST_LONGITUDE", toString(minLon));
        pdsMapObj += PvlKeyword("WESTERNMOST_LONGITUDE", toString(maxLon));
      }
    }

    // Add the LINE_PROJECTION_OFFSET and SAMPLE_PROJECTION_OFFSET keywords
    // These keywords are the distance from the origin of the image to the
    // origin of the projection. The units are line or samples. The image origin
    // is the middle of pixel (1,1)
    double lineOffset = inputMapping.findKeyword("UpperLeftCornerY");
    lineOffset /= (double)inputMapping.findKeyword("PixelResolution");
    lineOffset *= 1.0;
    lineOffset -= 0.5; // Add half a line to get to the center of (1,1)
    pdsMapObj += PvlKeyword("LINE_PROJECTION_OFFSET", toString(lineOffset), "PIXEL");
    double sampleOffset = inputMapping.findKeyword("UpperLeftCornerX");
    sampleOffset /= (double)inputMapping.findKeyword("PixelResolution");
    sampleOffset *= -1.0;
    sampleOffset -= 0.5; // Add half a sample to get to the center of (1,1)
    pdsMapObj += PvlKeyword("SAMPLE_PROJECTION_OFFSET", toString(sampleOffset), "PIXEL");

    // Add units to keywords already in the IMAGE_MAP_PROJECTION object as necessary
    if(pdsMapObj.hasKeyword("CENTER_LATITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("CENTER_LATITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
    if(pdsMapObj.hasKeyword("CENTER_LONGITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("CENTER_LONGITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
//    if (pdsMapObj.hasKeyword("REFERENCE_LATITUDE")) {
//      PvlKeyword &tempKey = pdsMapObj.findKeyword("REFERENCE_LATITUDE");
//      tempKey.setValue(tempKey[0], "DEG");
//    }
    if(pdsMapObj.hasKeyword("REFERENCE_LONGITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("REFERENCE_LONGITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
    if(pdsMapObj.hasKeyword("MAXIMUM_LATITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("MAXIMUM_LATITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
    if(pdsMapObj.hasKeyword("MINIMUM_LATITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("MINIMUM_LATITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
    if(pdsMapObj.hasKeyword("EASTERNMOST_LONGITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("EASTERNMOST_LONGITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
    if(pdsMapObj.hasKeyword("WESTERNMOST_LONGITUDE")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("WESTERNMOST_LONGITUDE");
      tempKey.setValue(tempKey[0], "DEG");
    }
    if(pdsMapObj.hasKeyword("MAP_PROJECTION_ROTATION")) {
      PvlKeyword &tempKey = pdsMapObj.findKeyword("MAP_PROJECTION_ROTATION");
      tempKey.setValue(tempKey[0], "DEG");
    }

  }


  /**
   * Return a projection name
   *
   * @param inputLabel
   *
   * @return String containing the name of the projection
   */
  QString ProcessExportPds::ProjectionName(Pvl &inputLabel) {
    Projection *proj = ProjectionFactory::Create(inputLabel);
    QString name = proj->Name();
    delete proj;
    return name;
  }


  /**
   * Return the line bytes (record size) for the input cube, at present this is
   * based on the number of samples and the bytes per pixel.
   *
   * @return Total number of bytes per line.
   */
  int ProcessExportPds::LineBytes() {
    Cube *cube = InputCubes[0];
    int a = SizeOf(p_pixelType);
    int b = cube->sampleCount();
    return b * a ;
  }


  /**
   * Return the size of the output PDS label.
   *
   * @return Number of bytes in the label.
   */
  int ProcessExportPds::LabelSize() {
    ostringstream temp;
    if(m_label->format() != NULL) {
      temp << *m_label << m_label->format()->formatEOL();
    }
    else {
      temp << *m_label << endl;
    }
    return temp.tellp();
  }

  /**
   * Write the PDS label to the a detached file. The PDS keywords
   * that have place holder "?" for their values (such as LABEL_RECORDS and
   * ^IMAGE) will be updated to their correct values before they are written.
   */
  void ProcessExportPds::OutputDetachedLabel() {
    if(!m_detachedLabel) {
      std::string msg = "Unable to output detached label. Use "
                    "ProcessExportPds::SetDetached() to set the "
                    "output PDS label file name.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    std::ofstream sOutLabelStream(m_detachedPdsLabelFile.toLatin1().data());
    OutputLabel(sOutLabelStream);
    sOutLabelStream.close();
  }

  /**
   * Write the PDS label to the supplied stream. The PDS keywords that have place
   * holder "?" for their values (such as LABEL_RECORDS and ^IMAGE) will
   * be updated to their correct values before they are written.
   *
   * @param Output file stream to which the pds label will be written.
   */
  void ProcessExportPds::OutputLabel(std::ofstream &os) {
    int labSize = LabelSize(); // labSize will be the old label size with "?"
    // NOTE: WARNING: If anything changes in the next two lines, you must also changes the
    // corresponding lines in the StandardImageRoot member

    if(m_exportType == Stream) {
      if(m_pdsFileType != ProcessExportPds::JP2Image) {
        (*m_label)["LABEL_RECORDS"].setValue(toString(labSize), "BYTES");
        if(!m_detachedLabel) {
          (*m_label)["^IMAGE"].setValue(toString(labSize + 1), "BYTES");
        }
      }
      if(m_label->format() != NULL) {
        os << *m_label << m_label->format()->formatEOL();
      }
      else {
        os << *m_label << endl;
      }
      // Fill the difference between the old and new label size with spaces.
      if(m_pdsFileType != ProcessExportPds::JP2Image) {
        for(int i = LabelSize(); i < labSize; ++i) os << ' ';
      }
    }
    else if(m_exportType == Fixed) {
      int lineBytes;
      int labelRecords;
      if(m_pdsFileType != ProcessExportPds::JP2Image) {
        lineBytes = LineBytes();
        (*m_label)["RECORD_BYTES"].setValue(toString(lineBytes));

        // The number of label records is dependent on the number of label bytes
        // and the lint bytes
        labelRecords = (int)ceil((double)labSize / (double)lineBytes);
        if(m_label->hasKeyword("LABEL_RECORDS")) { //LRO MRF doesn't have this keyword
          (*m_label)["LABEL_RECORDS"].setValue(toString(labelRecords));
        }
        int totalTableRecords = 0;
        for (unsigned int i = 0; i < m_tableRecords.size(); i++) {
          totalTableRecords += m_tableRecords[i];
        }
        int imageRecords = InputCubes[0]->lineCount()
                           * InputCubes[0]->bandCount();
        int fileRecords = labelRecords + imageRecords + totalTableRecords;
        (*m_label)["FILE_RECORDS"].setValue(toString(fileRecords));

        if(!m_detachedLabel) {
          (*m_label)["^IMAGE"].setValue(toString(labelRecords + 1));
        }
      }
      if(m_label->format() != NULL) {
        os << *m_label << m_label->format()->formatEOL();
      }
      else {
        os << *m_label << endl;
      }
      if(m_pdsFileType != ProcessExportPds::JP2Image) {
        for(int i = LabelSize(); i < labelRecords * lineBytes; ++i) os << ' ';
      }
    }

  }


  /**
   * Updates the CHECKSUM value on the label and rewrites to the output file.
   *
   * @param std::ofstream &pdsFileStream Output file to write the label to.
   */
  void ProcessExportPds::updateChecksumInLabel(std::ofstream &pdsFileStream) {
    // This occurs after application has called its StartProcess and the checksum has been
    // generated. We need to seek to the top of the file to rewrite the label. Since the
    // CHECKSUM is MD5, we set aside 32 characters for the value of this keyword. Since
    // OuputLabel() has already created the label and necessary padding, we can simply update
    // the CHECKSUM value to the generated checksum and re-write the entire label.
    pdsFileStream.seekp(0);
    (*m_label)["CHECKSUM"].setValue(checksum().toStdString());
    pdsFileStream << *m_label;
  }


  /**
   * This method will add a table to be exported to PDS. If the output PDS
   * labels are set to detached, the PDS table will be written to a detached
   * file in this method. If the output PDS product is set to attached, the
   * table will be added to a buffer and written to the PDS file when the
   * StartProcess() method is called. Attached tables are written to the file
   * after the image data.
   *
   * Warning: If attached tables are exported and the labels of these tables
   * are modified, the start byte value of the labels will need to be
   * updated. There is no existing method (UpdateAttachedTableLabels) to do
   * this in ProcessExportPds. If this functionality is needed, this class
   * will need to be modified accordingly.
   *
   * @throw IException::Unknown - "The output PDS file has been set to
   *                attached and a detached PDS table file name has been
   *                given. If detached is preferred, set the process to
   *                detached SetDetached() and call StandardPdsLabel() before
   *                calling ExportTable()."
   * @throw IException::Unknown - "The output PDS file has been set to
   *                detached. A file name for the detached ouput PDS table
   *                file is required. If an attached output file is prefered,
   *                use the method ProcessExportPds::SetAttached() before
   *                calling ExportTable()."

   * @param isisTable The ISIS Table object to be exported to PDS.
   * @param detachedPdsTableFileName The name of the exported PDS table file,
   *                                 if detached. This value should not
   *                                 include a path.  The path from the label
   *                                 file will be used.
   */
  void ProcessExportPds::ExportTable(Table isisTable, QString detachedPdsTableFileName) {

    if(Attached() && detachedPdsTableFileName != "") {
      std::string msg = "The output PDS file has been set to attached and a "
                    "detached PDS table file name has been given. If detached "
                    "is preferred, set the process to detached SetDetached() "
                    "and call StandardPdsLabel() before calling ExportTable().";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if(Detached() && detachedPdsTableFileName == "") {
      std::string msg = "The output PDS file has been set to detached. A file name "
                    "for the detached ouput PDS table file is required. "
                    "If an attached output file is prefered, use the method "
                    "ProcessExportPds::SetAttached() before calling ExportTable().";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // create an ExportPdsTable to fill file stream with PDS Table info
    ExportPdsTable pdsTable(isisTable);
    int fileRecordBytes = LineBytes();
    // return metadata pvl containing needed information for the output label.

    char *tableBuffer = new char[isisTable.Records() * fileRecordBytes];
    PvlObject metadata = pdsTable.exportTable(tableBuffer,
                                              fileRecordBytes,
                                              QString::fromStdString(ByteOrderName(p_endianType)));
    QString pdsTableName = pdsTable.formatPdsTableName();
    Pvl &mainPvl = *m_label;
    if (Attached()) {
      m_tableBuffers.push_back(tableBuffer);
      int labSize = LabelSize(); // labSize will be the old label size with "?"
      int labelRecords = (int)ceil((double)labSize / (double)fileRecordBytes);
      int imageRecords = InputCubes[0]->lineCount()
                         * InputCubes[0]->bandCount();
      int totalTableRecords = 0;
      for (unsigned int i = 0; i < m_tableRecords.size(); i++) {
        totalTableRecords += m_tableRecords[i];
      }
      // for start record values, indexing begins with 1
      int tableStartRecord = 1 + labelRecords + imageRecords + totalTableRecords;
      mainPvl += PvlKeyword("^" + pdsTableName.toStdString(), toString(tableStartRecord));
    }
    else {
      mainPvl += PvlKeyword("^" + pdsTableName.toStdString(), detachedPdsTableFileName.toStdString());
      FileName labelFile(m_detachedPdsLabelFile.toStdString());
      QString tableFileWithPath = QString::fromStdString(labelFile.path()) + "/"
                                  + detachedPdsTableFileName;
      ofstream os(tableFileWithPath.toLatin1().data());
      os.write(tableBuffer, isisTable.Records() * fileRecordBytes);
      os.close();
    }
    mainPvl.addObject(metadata);
    m_tableRecords.push_back(isisTable.Records());
    return;
  }

  /**
   * Mutator method to set the output PDS file to detached. In this case,
   * there will be separate output files containing the PDS label, image
   * data, and any PDS tables that are added.
   *
   * @param detachedLabelFile A string containing the name of the detached
   *                          PDS label file
   */
  void ProcessExportPds::SetDetached(QString detachedLabelFile) {
    m_detachedLabel = true;
    m_detachedPdsLabelFile = detachedLabelFile;
    return;
  }

  /**
   * Mutator method to set the output PDS file to attached. In this case,
   * there will be a single output file containing the PDS label, image
   * data, and any PDS tables that are added.
   */
  void ProcessExportPds::SetAttached() {
    m_detachedLabel = false;
    m_detachedPdsLabelFile = "";
  }

  /**
   * Accessor function returns true if the output PDS file is set to detached.
   *
   * @return @b bool Indicates whether the PDS file is detached.
   */
  bool ProcessExportPds::Detached()      {
    return m_detachedLabel;
  }

  /**
   * Accessor function returns true if the output PDS file is set to attached.
   *
   * @return @b bool Indicates whether the PDS file is attached.
   */
  bool ProcessExportPds::Attached()      {
    return !m_detachedLabel;
  }

  /**
   * Mutator method to set the output PDS image resolution to meters per pixel
   * or kilometers per pixel.
   *
   * @param resolutionUnits Enumerated value for the units type to be set.
   */
  void ProcessExportPds::SetPdsResolution(PdsResolution resolutionUnits) {
    m_exportResolution = resolutionUnits;
  }

  /**
   * Mutator method to set the output PDS image record type to stream or
   * fixed.
   *
   * @param recordFormat Enumerated value for the record type of the exported
   *                     PDS file.
   */
  void ProcessExportPds::SetExportType(PdsExportType recordFormat) {
    m_exportType = recordFormat;
  }

  /**
   * Mutator method to set how the the BANDS keyword will be handled. If false,
   * the BANDS keyword will be removed from the IMAGE object of the PDS labels.
   * This member variable defaults to true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to keep the BANDS
   *              keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceBands(bool force) {
    m_forceBands = force;
  }

  /**
   * Mutator method to set how the BAND_NAME keyword will be handled. If
   * false and the BAND_NAME keyword exists in the IMAGE object of the PDS
   * labels, the keyword will be removed. This member variable defaults to
   * true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to keep the BAND_NAME
   *              keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceBandName(bool force) {
    m_forceBandName = force;
  }

  /**
   * Mutator method to set how the CENTER_FILTER_WAVELENGTH keyword will be
   * handled. If false and the CENTER_FILTER_WAVELENGTH keyword exists in the
   * IMAGE object of the PDS labels, the keyword will be removed. This
   * member variable defaults to true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to keep the
   *              CENTER_FILTER_WAVELENGTH keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceCenterFilterWavelength(bool force) {
    m_forceCenterFilterWavelength = force;
  }

  /**
   * Mutator method to set how the BANDWIDTH keyword will be handled. If
   * false and the BANDWIDTH keyword exists in the IMAGE object of the PDS
   * labels, the keyword will be removed. This member variable defaults to
   * true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to keep the
   *              BANDWIDTH keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceBandwidth(bool force) {
    m_forceBandwidth = force;
  }

  /**
   * Mutator method to set how the BAND_STORAGE_TYPE keyword will be
   * handled. If true, the BAND_STORAGE_TYPE keyword will be added to the
   * IMAGE object of the PDS labels. This member variable defaults to true in
   * the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              BAND_STORAGE_TYPE keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceBandStorageType(bool force) {
    m_forceBandStorageType = force;
  }

  /**
   * Mutator method to set how the OFFSET keyword will be handled. If true,
   * the OFFSET keyword will be added to the IMAGE object of the PDS labels.
   * This member variable defaults to true in the ProcessExportPds
   * constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              OFFSET keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceOffset(bool force) {
    m_forceOffset = force;
  }

  /**
   * Mutator method to set how the SCALING_FACTOR keyword will be handled. If
   * true, the SCALING_FACTOR keyword will be added to the IMAGE object of
   * the PDS labels. This member variable defaults to true in the
   * ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              SCALING_FACTOR keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceScalingFactor(bool force) {
    m_forceScalingFactor = force;
  }

  /**
   * Mutator method to set how the SAMPLE_BITS keyword will be handled. If
   * true, the SAMPLE_BITS keyword will be added to the IMAGE object of
   * the PDS labels. This member variable defaults to true in the
   * ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              SAMPLE_BITS keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceSampleBits(bool force) {
    m_forceSampleBits = force;
  }

  /**
   * Mutator method to set how the SAMPLE_BIT_MASK keyword will be handled. If
   * true, the SAMPLE_BIT_MASK keyword will be added to the IMAGE object of
   * the PDS labels. This member variable defaults to true in the
   * ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              SAMPLE_BIT_MASK keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceSampleBitMask(bool force) {
    m_forceSampleBitMask = force;
  }

  /**
   * Mutator method to set how the SAMPLE_TYPE keyword will be handled. If
   * true, the SAMPLE_TYPE keyword will be added to the IMAGE object of the
   * PDS labels. This member variable defaults to true in the
   * ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              SAMPLE_TYPE keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceSampleType(bool force) {
    m_forceSampleType = force;
  }

  /**
   * Mutator method to set how the CORE_NULL keyword will be handled. If
   * true, the CORE_NULL keyword will be added to the IMAGE object of the
   * PDS labels. This member variable defaults to true in the
   * ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              CORE_NULL keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceCoreNull(bool force) {
    m_forceCoreNull = force;
  }

  /**
   * Mutator method to set how the CORE_LOW_REPR_SATURATION keyword will be
   * handled. If true, the CORE_LOW_REPR_SATURATION keyword will be added
   * to the IMAGE object of the PDS labels. This member variable defaults
   * to true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              CORE_LOW_REPR_SATURATION keyword in the PDS image labels.
   */
  void ProcessExportPds::ForceCoreLrs(bool force) {
    m_forceCoreLrs = force;
  }

  /**
   * Mutator method to set how the CORE_LOW_INSTR_SATURATION keyword will be
   * handled. If true, the CORE_LOW_INSTR_SATURATION keyword will be added
   * to the IMAGE object of the PDS labels. This member variable defaults
   * to true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              CORE_LOW_INSTR_SATURATION keyword in the PDS image
   *              labels.
   */
  void ProcessExportPds::ForceCoreLis(bool force) {
    m_forceCoreLis = force;
  }

  /**
   * Mutator method to set how the CORE_HIGH_REPR_SATURATION keyword will be
   * handled. If true, the CORE_HIGH_REPR_SATURATION keyword will be added
   * to the IMAGE object of the PDS labels. This member variable defaults to
   * true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              CORE_HIGH_REPR_SATURATION keyword in the PDS image
   *              labels.
   */
  void ProcessExportPds::ForceCoreHrs(bool force) {
    m_forceCoreHrs = force;
  }

  /**
   * Mutator method to set how the CORE_HIGH_INSTR_SATURATION keyword will be
   * handled. If true, the CORE_HIGH_INSTR_SATURATION keyword will be added
   * to the IMAGE object of the PDS labels. This member variable defaults to
   * true in the ProcessExportPds constructor.
   *
   * @param force Indicates whether to force the process to add the
   *              CORE_HIGH_INSTR_SATURATION keyword in the PDS image
   *              labels.
   */
  void ProcessExportPds::ForceCoreHis(bool force) {
    m_forceCoreHis = force;
  }

  /**
   * This method fills the image data of the PDS file using the parent class
   * ProcessExport::StartProcess, then appends any attached tables.
   *
   * @param fout Output file stream to be filled with the PDS data.
   *
   */
  void ProcessExportPds::StartProcess(std::ofstream &fout) {
    ProcessExport::StartProcess(fout);
    if (!m_detachedLabel) {
      for (unsigned int i = 0; i < m_tableBuffers.size(); i++) {
        if (m_tableBuffers[i] == NULL) {
          std::string msg = "Unable to add tables to PDS output file.";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        // write each table buffer to fout.
        // For each table, use (number of records)*(bytes per record) to
        // determine how many bytes to write out.
        fout.write(m_tableBuffers[i], m_tableRecords[i]*LineBytes());
      }
    }
    return;
  }

} // End of Isis namespace
