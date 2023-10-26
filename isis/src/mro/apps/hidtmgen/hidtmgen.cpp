/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QDir>
#include <QString>
#include <QStringList>

#include "hidtmgen.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "FileList.h"
#include "IString.h"
#include "ProcessExportPds.h"
#include "TProjection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlFormatPds.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "SpecialPixel.h"
#include "Statistics.h"
#include "UserInterface.h"

using namespace std;


namespace Isis{
  /**
   * Indicates the type of file currently being processed for export to PDS.
   */
  enum FileType { DTM,           /**< The output file for the current process is a DTM.*/
                  Orthorectified /**< The output file for the current process is an Ortho.*/
  };


  /**
   * This enumeration is used to determine how to set the special pixel types
   * selected by the user.
   *
   * If the output data is non-negative, then Both is set. This means that Null,
   * Lrs, and Lis (if selected) will be given the values of the lower boundary
   * (i.e. beginning at 0) and that His and Hrs (if selected) will be given the
   * values of the upper boundary (i.e. for 8bit, values near 255 and for 16bit,
   * values near 65535).
   *
   * If the output data is signed integer data, then Negative is set. This means
   * that all special pixels (if selected) will be given values of the lower
   * boundary (i.e. beginning -32768).
   *
   * If the output data is real-valued, then Default is set. This means that the
   * special pixels defined in Isis::SpecialPixel will be used.
   *
   */
  enum SpecialPixelBoundary { Both,     /**< Both the upper and lower boundaries may be used to save
                                             off special pixel values. This option is used for
                                             unsigned bit integer valued output data types
                                             (i.e. 8bit and unsigned 16bit)*/
                              Negative, /**< Only the lower (negative) boundary may be used to save
                                             off special pixel values. This option is used for signed
                                             16 bit integer valued output data.*/
                              Default   /**< This option is used for real valued output data types
                                             (i.e. 32bit). In this case pre-defined
                                             Isis::SpecialPixel values are dedicated to the selected
                                             pixel types.*/
  };


  // -------------------------Function Prototypes ------------------------------------
  // cube processing
  void setUpProcessPixels(const UserInterface &ui,
                          ProcessExportPds &pdsExportProcess,
                          FileType fileType);
  void setRangeAndPixels(const UserInterface &ui,
                         ProcessExportPds &pdsExportProcess,
                         double &min,
                         double &max,
                         SpecialPixelBoundary ptype);
  void processCube(ProcessExportPds &pdsExportProcess,
                   const FileName  &outputPdsFile);
  // projection information and viewing parameters
  void setProjectionInformation(Cube *inCube,
                                Pvl &pdsLabel,
                                PvlObject &mappingObject,
                                const QString &projectionType);
  void setEquirectangularRadii(Cube *inCube,
                               PvlObject &mappingObject);
  double polarStereoGraphicNorthAzimuth(const PvlObject &mappingObject);
  // find codes needed for product id
  char mapScaleCode(double scale);
  QString dtmSourceOrbitAndTargetCodes(const PvlKeyword &sourceKeyword);
  QString versionNumber(const Pvl &paramsPvl,
                        const UserInterface &ui);
  QString producingInstitution(const Pvl &paramsPvl,
                               const UserInterface &ui);
  QString orthoContentColorCode(const FileName &orthoFileName);
  // find/set values needed for output labels
  void setIdentificationInformation(Pvl &pdsLabel,
                                    const QString &productId,
                                    PvlKeyword sourceProductId,
                                    const Pvl &paramsPvl,
                                    const UserInterface &ui);
  // dtm specific calls
  void verifyDTM(Cube *inCube,
                 const FileName &inputCubeFile);
  void customizeDtmLabels(Cube *inCube,
                          Pvl &dtmPdsLabel,
                          PvlObject &mappingObject);
  // ------------------------- end Function Prototypes ---------------------------------


  // Main
  void hidtmgen(UserInterface &ui) {
    try {
      // -------------------------------------------------------------------------//
      // Get required global ui...
      // -------------------------------------------------------------------------//
      Pvl paramsPvl(ui.GetFileName("PARAMSPVL"));
      bool defaultNames = ui.GetBoolean("DEFAULTNAMES");

      // parameters:
      // * DTM (not required) if entered, set as input cube in process
      // * ORTHOFROMLIST (not required) if entered, set as input cubes in process
      //
      // * DEFAULTNAMES (required) if true, generates output file names and product ids
      // * OUTPUTDIR (not required) only used (still not required) if defaultnames=true
      // * DTMTO (not required) required when defaultnames=false and dtm was given.
      // * ORTHOTOLIST (not required) required when DEFAULTNAMES=false and ORTHOFROMLIST was given.
      //               if entered, size must match ORTHOFROMLIST
      //
      // * PARAMSPVL (required)
      // * DTM_PRODUCT_ID (not required) required if DTM given and DEFAULTNAMES=false
      // * ORTHOPRODUCTIDLIST (not required) required if DTM given and DEFAULTNAMES=false
      //                      if entered, size must match ORTHOFROMLIST
      // * ORTHOSEQUENCENUMBERLIST (not required) required if DTM given and DEFAULTNAMES=false
      //                      if entered, size must match ORTHOFROMLIST
      //
      // * ENDIAN (required)
      //
      // * DTMBITTYPE required if DTM
      // * ORTHOBITTYPE required if ORTHOFROMLIST
      // * NULL (not required)
      // * LRS (not required)
      // * LIS (not required)
      // * HIS (not required)
      // * HRS (not required)


      // -------------------------------------------------------------------------//
      // Set up Process...
      // -------------------------------------------------------------------------//
      ProcessExportPds pdsExportProcess;
      // Set ExportType and Endian for all output
      pdsExportProcess.SetExportType(ProcessExportPds::Fixed);
      if (ui.GetString("ENDIAN") == "MSB") {
        pdsExportProcess.SetOutputEndian(Isis::Msb);
      }
      else if (ui.GetString("ENDIAN") == "LSB") {
        pdsExportProcess.SetOutputEndian(Isis::Lsb);
      }
      // -------------------------------------------------------------------------//
      // -------------------------------------------------------------------------//

      // The output directory will be used for DTM and ortho images when defaultNames=true
      // this directory won't be used if defaultNames=false but will be used for
      // both DTM and ortho if defaultNames=true.
      FileName outFile;
      QString outDirString;
      FileName outDir = FileName(ui.GetString("OUTPUTDIR"));
      if (!outDir.fileExists()) {
        outDir.dir().mkpath(".");
      }
      outDirString = outDir.expanded();
      if (outDirString.mid(outDirString.size()-1, 1) != "/") {
        outDirString += "/";
        outDir = FileName(outDirString);
      }
      outDirString = outDir.expanded();

      // if DTM provided, PRODUCT_ID will also be used for ortho SOURCE_PRODUCT_IDs
      QString dtmProductId = "";
      if (ui.WasEntered("DTM")) {
        setUpProcessPixels(ui, pdsExportProcess, DTM);
        // set the input cube to process
        CubeAttributeInput inAttribute;
        Cube *inCube = pdsExportProcess.SetInputCube(ui.GetCubeName("DTM"), inAttribute);
        verifyDTM(inCube,  ui.GetCubeName("DTM"));

        // These are our output labels, will be modifying heavily
        Pvl &pdsLabel = pdsExportProcess.StandardPdsLabel(ProcessExportPds::Image);
        PvlObject &mappingObject = pdsLabel.findObject("IMAGE_MAP_PROJECTION");
        QString projectionType = mappingObject["MAP_PROJECTION_TYPE"][0];
        setProjectionInformation(inCube, pdsLabel, mappingObject, projectionType);
        customizeDtmLabels(inCube,  pdsLabel,  mappingObject);

        // if the DTM was given, use the SOURCE_PRODUCT_ID supplied by the user.
        PvlKeyword source = paramsPvl.findKeyword("DTM_SOURCE_PRODUCT_ID", PvlObject::Traverse);
        source.setName("SOURCE_PRODUCT_ID");

        if (defaultNames) {
          dtmProductId =  "DT";
          dtmProductId += "E";// for now this is hard-coded to E for elevations (see xml doc)
          dtmProductId += projectionType[0].toUpper();
          dtmProductId += mapScaleCode(mappingObject.findKeyword("MAP_SCALE"));
          dtmProductId += "_";
          dtmProductId += dtmSourceOrbitAndTargetCodes(source);
          // Get the 1-character producing institution code from the PARAMSPVL and add it
          dtmProductId += producingInstitution(paramsPvl, ui);
          // Get the 2-character version number from the PARAMSPVL and add it
          dtmProductId += versionNumber(paramsPvl, ui);

          outFile = FileName(outDirString + dtmProductId + ".IMG");
        }
        else {
          dtmProductId = ui.GetString("DTM_PRODUCT_ID");
          outFile = FileName(ui.GetFileName("DTMTO"));
        } // End scope of defaultNames true

        // identification labels that are pretty set in stone
        setIdentificationInformation(pdsLabel, dtmProductId, source, paramsPvl, ui);
        processCube(pdsExportProcess, outFile);
        if (!outFile.fileExists()) {
          throw IException(IException::Unknown,
                           QString("DTM file [%1] failed to be created.").arg(outFile.expanded()),
                           _FILEINFO_);
        }
      } // end if DTM was entered
      else if (!ui.WasEntered("ORTHOFROMLIST")) {
        throw IException(IException::User,
                         "User must supply DTM or ORTHOFROMLIST or both.",
                         _FILEINFO_);
      }

      /*
       * End of DTM work. Labels are complete, names complete, it's written out.
       */


      // Now we take care of ortho images, if given.
      // Get the list of ortho input cubes
      if (ui.WasEntered("ORTHOFROMLIST")) {

        FileList orthoFromList;
        orthoFromList.read(FileName(ui.GetFileName("ORTHOFROMLIST")));
        if(orthoFromList.size() == 0) {
          throw IException(IException::User, "Input ortho list is empty.", _FILEINFO_);
        }

        // check corresponding input lists for matching sizes...
        FileList orthoToList, orthoProductIdList, orthoSequenceNumberList;
        if (defaultNames) {
          // if creating default output file names and product ids then we need to get the list of
          // ortho sequence numbers
          orthoSequenceNumberList.read(FileName(ui.GetFileName("ORTHOSEQUENCENUMBERLIST")));
          if(orthoFromList.size() != orthoSequenceNumberList.size()) {
            throw IException(IException::User, "Output sequence number list must "
                                               "correspond to the input ortho list.", _FILEINFO_);
          }
        }
        else {
          // if not creating default names, get the lists of ortho output cube names and product ids
          orthoToList.read(FileName(ui.GetFileName("ORTHOTOLIST")));
          orthoProductIdList.read(FileName(ui.GetFileName("ORTHOPRODUCTIDLIST")));
          if(orthoFromList.size() != orthoToList.size()
             || orthoFromList.size() != orthoProductIdList.size()) {
            throw IException(IException::User,
                             "Output ortho list and product id list must "
                             "correspond to the input ortho list.",
                             _FILEINFO_);
          }
        }

        ProcessExportPds orthoExportProcess;
        // Set ExportType and Endian for all output
        orthoExportProcess.SetExportType(ProcessExportPds::Fixed);
        if (ui.GetString("ENDIAN") == "MSB") {
          orthoExportProcess.SetOutputEndian(Isis::Msb);
        }
        else if (ui.GetString("ENDIAN") == "LSB") {
          orthoExportProcess.SetOutputEndian(Isis::Lsb);
        }

        setUpProcessPixels(ui, orthoExportProcess, Orthorectified);

        // Loop through all ortho images
        for (int i = 0; i < orthoFromList.size(); i++) {

          // set the input cube to process
          CubeAttributeInput att(orthoFromList[i]);
          Cube *inCube = orthoExportProcess.SetInputCube(orthoFromList[i].expanded(), att);

          // get the cube label and set identification info
          Pvl &pdsLabel  = orthoExportProcess.StandardPdsLabel(ProcessExportPds::Image);

          // set map projection information
          PvlObject &mappingObject = pdsLabel.findObject("IMAGE_MAP_PROJECTION");
          QString projectionType = mappingObject["MAP_PROJECTION_TYPE"][0];
          setProjectionInformation(inCube, pdsLabel, mappingObject, projectionType);

          QString productId = "";
          QString orthoId = "";
          if (defaultNames) {
            orthoId = orthoFromList[i].baseName().left(15);
            productId = orthoId;
            productId += "_";
            productId += orthoContentColorCode(orthoFromList[i]);
            productId += "_";
            productId += mapScaleCode(mappingObject.findKeyword("MAP_SCALE"));
            productId += "_";
            productId += orthoSequenceNumberList[i].expanded();
            productId += "_";
            productId += "ORTHO";

            // output file path is the same as the dtm
            outFile = FileName(outDirString + productId + ".IMG");
          }
          else {
            productId = orthoProductIdList[i].expanded();
            outFile = orthoToList[i];
            orthoId = productId;
          }

          // for ortho images, source product ID is the DTM product ID followed by the
          // ORTHO product ID
          PvlKeyword source("SOURCE_PRODUCT_ID");
          if (dtmProductId.isEmpty()) {
            source += paramsPvl["ORTHO_SOURCE_DTM_ID"];
          }
          else {
            source += dtmProductId;
          }
          source += orthoId;
          setIdentificationInformation(pdsLabel, productId, source, paramsPvl, ui);

          processCube(orthoExportProcess, outFile);
          if (!outFile.fileExists()) {
            throw IException(IException::Unknown,
                             QString("DTM file [%1] failed to be created.").arg(outFile.expanded()),
                             _FILEINFO_);
          }

        } // End scope of for loop for ortho images
      }
    }
    catch (IException &e) {
      throw IException(e, IException::Unknown, "hidtmgen: Unable to HiRISE generate pds products.", _FILEINFO_);
    }
  }// end main


  void setUpProcessPixels(const UserInterface &ui,
                          ProcessExportPds &pdsExportProcess,
                          FileType fileType) {
    double min = -DBL_MAX;
    double max = DBL_MAX;

    // Setup output type
    QString parameterPrefix = "DTM";
    if (fileType == Orthorectified) {
      parameterPrefix = "ORTHO";
    }
    QString bitType = ui.GetString(parameterPrefix + "BITTYPE");
    if (bitType == "8BIT") {
      pdsExportProcess.SetOutputType(Isis::UnsignedByte);
      min = 0.0;
      max = 255.0;
      setRangeAndPixels(ui, pdsExportProcess, min, max, Both);
    }
    else if (bitType == "S16BIT") {
      pdsExportProcess.SetOutputType(Isis::SignedWord);
      min = -32768.0;
      max = 32767.0;
      setRangeAndPixels(ui, pdsExportProcess, min, max, Negative);
    }
    else if (bitType == "U16BIT") {
      pdsExportProcess.SetOutputType(Isis::UnsignedWord);
      min = 0.0;
      max = 65535.0;
      setRangeAndPixels(ui, pdsExportProcess, min, max, Both);
    }
    else { // default 32 bit
      pdsExportProcess.SetOutputType(Isis::Real);
      pdsExportProcess.SetOutputNull(Isis::NULL4);
      pdsExportProcess.SetOutputLrs(Isis::LOW_REPR_SAT4);
      pdsExportProcess.SetOutputLis(Isis::LOW_INSTR_SAT4);
      pdsExportProcess.SetOutputHrs(Isis::HIGH_REPR_SAT4);
      pdsExportProcess.SetOutputHis(Isis::HIGH_INSTR_SAT4);
      setRangeAndPixels(ui, pdsExportProcess, min, max, Default);
    }

  }


  //Sets up special pixels and valid pixel ranges
  void setRangeAndPixels(const UserInterface &ui,
                         ProcessExportPds &pdsExportProcess,
                         double &min, double &max,
                         SpecialPixelBoundary ptype) {

    if (ptype == Negative) {
      // if we are dedicating the passed in min to be null,
      // 1. set output null to that value
      // 2. update the min to min+1
      // i.e. for SignedWord, null=-32768.0, min=-32767.0
      if (ui.GetBoolean("NULL")) {
        pdsExportProcess.SetOutputNull(min++);
      }
      // if we are dedicating the current min value to be lrs,
      // 1. set output lrs to that value
      // 2. update the min to min+1
      if (ui.GetBoolean("LRS")) {
        pdsExportProcess.SetOutputLrs(min++);
      }
      if (ui.GetBoolean("LIS")) {
        pdsExportProcess.SetOutputLis(min++);
      }
      if (ui.GetBoolean("HIS")) {
        pdsExportProcess.SetOutputHis(min++);
      }
      if (ui.GetBoolean("HRS")) {
        pdsExportProcess.SetOutputHrs(min++);
      }
    }
    else if (ptype == Both) {
      // if we are dedicating the passed in min to be null,
      // 1. set output null to that value
      // 2. update the min to min+1
      if (ui.GetBoolean("NULL")) {
        pdsExportProcess.SetOutputNull(min++);
      }
      // if we are dedicating the current min value to be lrs,
      // 1. set output lrs to that value
      // 2. update the min to min+1
      if (ui.GetBoolean("LRS")) {
        pdsExportProcess.SetOutputLrs(min++);
      }
      // if we are dedicating the current min value to be lis,
      // 1. set output lis to that value
      // 2. update the min to min+1
      if (ui.GetBoolean("LIS")) {
        pdsExportProcess.SetOutputLis(min++);
      }
      // if we are dedicating the max value to be hrs,
      // 1. set output hrs to that value
      // 2. update the max to max-1
      if (ui.GetBoolean("HRS")) {
        pdsExportProcess.SetOutputHrs(max--);
      }
      // if we are dedicating the current max value to be his,
      // 1. set output his to that value
      // 2. update the max to max-1
      if (ui.GetBoolean("HIS")) {
        pdsExportProcess.SetOutputHis(max--);
      }
    }
    pdsExportProcess.SetOutputRange(min, max);
  }


  void processCube(ProcessExportPds &pdsExportProcess, const FileName  &outputPdsFile) {
    ofstream pdsOut(outputPdsFile.expanded().toLatin1().data());
    pdsExportProcess.OutputLabel(pdsOut);
    pdsExportProcess.StartProcess(pdsOut);
    pdsOut.close();
    pdsExportProcess.EndProcess();
    pdsExportProcess.ClearInputCubes();
  }


  void setProjectionInformation(Cube *inCube, Pvl &pdsLabel,
                                PvlObject &mappingObject, const QString &projectionType) {

    // set map projection information
    mappingObject.addKeyword(PvlKeyword("^DATA_SET_MAP_PROJECTION", "DSMAP.CAT"));
    // initialize the azimuth to a Null value so we can check whether it was set.
    double northAzimuth = Isis::Null;
    // now find the northAzimuth based on projection type
    if (QString::compare(projectionType, "EQUIRECTANGULAR", Qt::CaseInsensitive) == 0) {
      setEquirectangularRadii(inCube, mappingObject);
      mappingObject["MAP_PROJECTION_TYPE"].setValue("\"EQUIRECTANGULAR\"");
      northAzimuth = 270;
      pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHirisePdsDTMEqui.pft");
    }
    else if (QString::compare(projectionType, "POLAR STEREOGRAPHIC", Qt::CaseInsensitive) == 0) {
      northAzimuth = polarStereoGraphicNorthAzimuth(mappingObject);
      pdsLabel.setFormatTemplate("$ISISROOT/appdata/translations/MroHirisePdsDTMPolar.pft");
    }
    else {
      QString msg = "The projection type [" +
                    projectionType
                    + "] is not supported";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    PvlObject viewingParameters("VIEWING_PARAMETERS");
    if (northAzimuth != Isis::Null) {
      viewingParameters.addKeyword(PvlKeyword("NORTH_AZIMUTH", toString(northAzimuth), "DEG"));
    }
    else {
      viewingParameters.addKeyword(PvlKeyword("NORTH_AZIMUTH", "N/A"));
    }
    pdsLabel.addObject(viewingParameters);

  }


  void setEquirectangularRadii(Cube *inCube, PvlObject &mappingObject) {
    TProjection *proj = (TProjection *) ProjectionFactory::CreateFromCube(*inCube);
    double newRadius = proj->LocalRadius((double)mappingObject["CENTER_LATITUDE"]);
    newRadius /= 1000;
    mappingObject.findKeyword("A_AXIS_RADIUS").setValue(std::to_string(newRadius));
    mappingObject.findKeyword("A_AXIS_RADIUS").setUnits("KM");
    mappingObject.findKeyword("B_AXIS_RADIUS").setValue(std::to_string(newRadius));
    mappingObject.findKeyword("B_AXIS_RADIUS").setUnits("KM");
    mappingObject.findKeyword("C_AXIS_RADIUS").setValue(std::to_string(newRadius));
    mappingObject.findKeyword("C_AXIS_RADIUS").setUnits("KM");
  }


  double polarStereoGraphicNorthAzimuth(const PvlObject &mappingObject) {

    double northAzimuth = Isis::Null;
    if (mappingObject.hasKeyword("MINIMUM_LATITUDE")
        && mappingObject.hasKeyword("MAXIMUM_LATITUDE")
        && mappingObject.hasKeyword("EASTERNMOST_LONGITUDE")
        && mappingObject.hasKeyword("WESTERNMOST_LONGITUDE")) {

      // find the center latitude of this set of images
      // (not the same as the center lat for the projection)
      double clat = ((toDouble(mappingObject["MAXIMUM_LATITUDE"][0]) -
                      toDouble(mappingObject["MINIMUM_LATITUDE"][0])) / 2) +
                    toDouble(mappingObject["MINIMUM_LATITUDE"][0]);
      // find the center longitude of this set of images
      // (not the same as the center lon for the projection)
      double clon = ((toDouble(mappingObject["EASTERNMOST_LONGITUDE"][0]) -
                      toDouble(mappingObject["WESTERNMOST_LONGITUDE"][0])) / 2) +
                    toDouble(mappingObject["WESTERNMOST_LONGITUDE"][0]);

      if (clat > 0.0 && clon < 270.0) { // Northern Hemisphere, 0 to 270 lon
        northAzimuth = 270.00 - clon;
      }
      else if (clat > 0.0 && clon >= 270.0) { // Northern Hemisphere, 270 to 360 lon
        northAzimuth = 360.00 + (270.00 - clon);
      }
      else if (clat < 0.0 && clon < 90.0) {  // Southern Hemisphere, 0 to 90 lon
        northAzimuth = 270.00 + clon;
      }
      else if (clat < 0.0 && clon >= 90.0) { // Southern Hemisphere, 90 to 360 lon
        northAzimuth = -(360.00 - (270.00 + clon));
      }
    }
    return northAzimuth;

  }


  /*
   * Scale letter of the image, A = 0.25, B = 0.5, C = 1.0, and so on.
   * We are using a 10% fudge range.
   */
  char mapScaleCode(double scale) {
    int steps = 0;
    double matchNum = 0.25;
    double epsilon = matchNum * 0.1; // = 10% of matchNum

    bool bounded = false;
    while (!bounded) {
      if ((scale + epsilon) > matchNum && (scale - epsilon) < matchNum) {
        bounded = true;
      }
      else {
        steps++;
      }

      // Increase to next possible scale and increase epsilon
      matchNum *= 2;
      epsilon *= 2;

      if (matchNum > 129) { // Max allowed is J, 128, before skipping to Z
        bounded = true;
        steps = 25; // Get us to 'Z'
      }
    }
    char scaleLetter = 'A';
    // For however many steps we took, increase the letter.
    scaleLetter += steps;
    // 0.25 = A, 0.5 = B, 1.0 = C...
    return scaleLetter;
  }


  QString dtmSourceOrbitAndTargetCodes(const PvlKeyword &sourceKeyword) {
    // we use the source product id for the DTM to get the orbit IDs and target code
    // for the source products (i.e. the stereo pair)
    return sourceKeyword[0].mid(4,11) + "_" +
           sourceKeyword[1].mid(4,12) + "_";
  }


  QString versionNumber(const Pvl &paramsPvl, const UserInterface &ui) {
    double version = toDouble(paramsPvl["PRODUCT_VERSION_ID"]);

    // Format the version for the output name:
    // Only important thing to note is that a #.0 number is converted to 0#
    // for the name, otherwise, it is predictable, always 2 greatest numbers:
    // ##.
    // >10, takes first two digits
    // The number found here is used in ortho images as well.
    QString vers = "";
    if (version >= 10.0) {
      vers = toString(version).left(2);
    }
    else if (version >= 1.0) {
      vers = toString(version);
      bool wasInt = false;
      // Checking for integer values, if so, make #.0 into 0#
      // necessary because in DTMgen version 1.0 corresponded to a 01 in names.
      if (vers.size() == 3) {
        if (vers.at(2) == '0') {
          vers = "0" + vers.left(1);
          wasInt = true;
        }
      }
      // Wasn't int, make #.# into ##
      if (!wasInt) {
        vers = toString(version).remove(QChar('.'));
        if (vers.size() > 2) {
          vers = vers.left(2);
        }
      }
    }
    // 0 - <1, if 0.#, is 0#, is 0.#####, is first two ##
    else if (version >= 0.001) { // Any less and we get E... not dealing with that.
      vers = toString(version).remove(QChar('.'));
      int nonZero = vers.lastIndexOf("0");
      if (vers.size() > 2) {
        vers = vers.mid(nonZero+1, 2);
      }
    }
    // It was negative, or something else crazy?
    else {
      QString msg = "Version number [" + toString(version) + "] is invalid";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    return vers;
  }


  QString producingInstitution(const Pvl &paramsPvl, const UserInterface &ui) {
    QString producing = paramsPvl["PRODUCING_INSTITUTION"][0];
    if (producing.size() > 1) {
      QString msg = "PRODUCING_INSTITUTION value [" + producing + "] in the PARAMSPVL file must be a "
                    "single character. See hidtmgen documentation for these character codes.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    return producing;
  }


  QString orthoContentColorCode(const FileName &orthoFileName) {
    QString colorCode = "";
    Cube orthoCube(orthoFileName, "r");
    if (orthoCube.bandCount() == 1) {
      colorCode = "RED";
    }
    else if (orthoCube.bandCount() == 3) {
      colorCode = "IRB";
    }
    else {
      QString msg = "The file [" + orthoFileName.expanded() + "] found in the ORTHOFROMLIST "
                    "is not a valid orthorectified image. Band count must be 1 (RED) or 3 (color).";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    return colorCode;
  }


  void setIdentificationInformation(Pvl &pdsLabel,
                                    const QString &productId,
                                    PvlKeyword sourceProductId,
                                    const Pvl &paramsPvl,
                                    const UserInterface &ui) {
    // These come from the user (PARAMSPVL)
    pdsLabel.addKeyword(paramsPvl["DATA_SET_ID"]);
    pdsLabel.addKeyword(paramsPvl["DATA_SET_NAME"]);
    pdsLabel.addKeyword(paramsPvl["PRODUCER_INSTITUTION_NAME"]);
    pdsLabel.addKeyword(paramsPvl["PRODUCER_ID"]);
    pdsLabel.addKeyword(paramsPvl["PRODUCER_FULL_NAME"]);

    // given product id
    pdsLabel.addKeyword(PvlKeyword("PRODUCT_ID", productId));

    // This comes from the user (PARAMSPVL)
    pdsLabel.addKeyword(paramsPvl["PRODUCT_VERSION_ID"]);

    // always the same value
    pdsLabel.addKeyword(PvlKeyword("INSTRUMENT_HOST_NAME", "MARS RECONNAISSANCE ORBITER"));
    pdsLabel.addKeyword(PvlKeyword("INSTRUMENT_HOST_ID", "MRO"));
    pdsLabel.addKeyword(PvlKeyword("INSTRUMENT_NAME", "HIGH RESOLUTION IMAGING SCIENCE EXPERIMENT"));
    pdsLabel.addKeyword(PvlKeyword("INSTRUMENT_ID", "HIRISE"));

    // sourceProductId
    pdsLabel.addKeyword(sourceProductId);

    // These come from the user (PARAMSPVL)
    pdsLabel.addKeyword(paramsPvl["RATIONALE_DESC"]);
    pdsLabel.addKeyword(paramsPvl["SOFTWARE_NAME"]);
  }


  /**
   *
   * @param inCube
   *
   */
  void verifyDTM(Cube *inCube, const FileName &inputCubeFile) {
    if (inCube->bandCount() > 1 ||
        inCube->label()->hasObject("Instrument")) {
      QString msg = "Input cube [" + inputCubeFile.expanded() + "] does not appear "
                  + "to be a DTM";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   *
   * @param inCube
   * @param dtmPdsLabel
   * @param mappingObject
   *
   */
  void customizeDtmLabels(Cube *inCube, Pvl &dtmPdsLabel, PvlObject &mappingObject) {

    // sets format for dtm label
    Pvl format;
    // Have to do things this way to get an array of values
    PvlKeyword validMin("VALID_MINIMUM", "REAL");
    validMin += "2";
    format.addKeyword(validMin);
    PvlKeyword validMax("VALID_MAXIMUM", "REAL");
    validMax += "2";
    format.addKeyword(validMax);
    format.addKeyword(PvlKeyword("SAMPLE_BIT_MASK", "BINARY"));
    // Aiming for MISSING_CONSTANT = (HEX, 4); Two separate items.
    PvlKeyword mc("MISSING_CONSTANT", "HEX");
    mc += "4";
    format.addKeyword(mc);
    PvlFormat *form = dtmPdsLabel.format();
    form->add(format);
    dtmPdsLabel.setFormat(form);

    QString note = "Pixel values in this file represent elevations in meters "
        "above the martian equipotential surface (Mars 2000 Datum) defined by "
        "Smith, et al. (2001). Conversion from pixel units to geophysical "
        "units is given by the keyvalues for SCALING_FACTOR and OFFSET. This "
        "DTM was produced using ISIS and SOCET Set (copyright BAE Systems) "
        "software as described in Kirk et al. (2008).";
    dtmPdsLabel.findObject("IMAGE").addKeyword(PvlKeyword("NOTE", note));

    // Label records should always be 1, my example didn't included it, so we won't.
    dtmPdsLabel.deleteKeyword("LABEL_RECORDS");

    // Delete or change unneeded keywords in image object
    PvlObject &image = dtmPdsLabel.findObject("IMAGE");
    image.findKeyword("CORE_NULL").setName("MISSING_CONSTANT");
    image.deleteKeyword("BAND_STORAGE_TYPE");
    image.deleteKeyword("CORE_LOW_REPR_SATURATION");
    image.deleteKeyword("CORE_LOW_INSTR_SATURATION");
    image.deleteKeyword("CORE_HIGH_REPR_SATURATION");
    image.deleteKeyword("CORE_HIGH_INSTR_SATURATION");

    // Create statistics and add to image group
    Statistics *stat = inCube->statistics();
    image.addKeyword(PvlKeyword("VALID_MINIMUM", toString(stat->Minimum())));
    image.addKeyword(PvlKeyword("VALID_MAXIMUM", toString(stat->Maximum())));

    // delete unneeded keywords in map object
    mappingObject.deleteKeyword("FIRST_STANDARD_PARALLEL");
    mappingObject.deleteKeyword("SECOND_STANDARD_PARALLEL");

  }


  /*
   * For orthorectified images,
   * PRODUCT_ID = mSP_xxxxxx_xxxx_ccc_s_nn_ORTHO
   *     m is the map projection code
   *         E for equirectangular
   *         P for polar stereographic
   *     xxxxx_xxxx is the HiRISE source observation ID (mission phase orbit number target code)
   *     ccc is the color content
   *         RED for visible red, 1 band images
   *         IRB for 3 band enhanced color images (IR, RED, or BG)
   *     s is the grid spacing (i.e. map scale) code
   *         A for 0.25 m
   *         B for 0.5  m
   *         C for 1.0  m
   *         D for 2.0  m
   *     nn is the sequence number to distinguish between ortho rectified images
   *         from the same HiRISE observation that may be created from different DTMs
   *     ORTHO indicates that the image has been orthorectified
   *
   * For DTMs,
   * PRODUCT_ID = DTems_xxxxxx_xxxx_xxxxxx_xxxx_vnn
   *     e is the code for the type of elevation data
   *           E for areoid elevations
   *           R for radii
   *           (hidtmgen does not currently support this option)
   *     m is the map projection code
   *           E for equirectangular
   *           P for polar stereographic
   *     s is the grid spacing (i.e. map scale) code
   *           A for 0.25 m
   *           B for 0.5  m
   *           C for 1.0  m
   *           D for 2.0  m
   *     xxxxx_xxxx_xxxxx_xxxx is the HiRISE source observation ID for the stereo pairs
   *     v is the code for the producing institution
   *           U for USGS
   *           A for University of Arizona
   *           C for CalTech
   *           N for NASA Ames
   *           J for JPL
   *           O for Ohio State
   *           Z for other
   *     nn is the 2 digit product version ID number
   */
 }
