/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Preference.h"

#include "Application.h"
#include "IException.h"
#include "IString.h"
#include "Portal.h"
#include "ProcessMosaic.h"
#include "Pvl.h"
#include "SerialNumber.h"
#include "SpecialPixel.h"
#include "Table.h"
#include "TrackingTable.h"

using namespace std;

namespace Isis {
  /**
   * This is the name of the table in the Cube which will contain the image file names.
   */
  const char *ProcessMosaic::TRACKING_TABLE_NAME = "InputImages";

  /**
   * ProcessMosaic Contructor
   *
   * Initialize the class members to default
   */
  ProcessMosaic::ProcessMosaic() {
    // Set the BandBin Match
    SetBandBinMatch(true);

    // Initialize the structure Track Info
    m_trackingEnabled    = false;
    m_trackingCube = NULL;
    m_createOutputMosaic   = false;
    m_bandPriorityBandNumber  = 0;
    m_bandPriorityKeyName  = "";
    m_bandPriorityKeyValue = "";
    m_bandPriorityUseMaxValue = false;

    // Initialize the Special Pixel Flags
    m_placeHighSatPixels = false;
    m_placeLowSatPixels = false;
    m_placeNullPixels = false;

    // Default Priority OnTop
    m_imageOverlay = PlaceImagesOnTop;

    m_enforceMatchDEM = false;

    // Initialize the data members
    m_iss = -1;
    m_isl = -1;
    m_isb = -1;
    m_ins = -1;
    m_inl = -1;
    m_inb = -1;
    m_oss = -1;
    m_osl = -1;
    m_osb = -1;
    m_onb = -1;
  }


  //!  Destroys the Mosaic object. It will close all opened cubes.
  ProcessMosaic::~ProcessMosaic() {
    if (m_trackingCube) {
      m_trackingCube->close();
      delete m_trackingCube;
      m_trackingCube = NULL;
    }
  }


  /**
  * This method invokes the process by mosaic operation over a single input cube
  * and single output cube. Unlike other Isis process objects, no application .
  * function will be called. The processing is handled entirely within the
  * mosaic object. The input cube must be pixel aligned with the output cube
  * before mosaiking. If the input cube does not overlay any of the output cube,
  * no processing takes place. There are 3 priorities input, mosaic and band. Has the
  * ability to track the origin of the mosaic if the flag is set. Some conditions apply
  * like tracking turned off for multiband input with input or mosaic priority.
  *
  * @param os The sample position of input cube starting sample relative to
  *           the output cube. The cordinate is in output cube space and may
  *           be any integer value negative or positive.
  *
  * @param ol The line position of input cube starting line relative to the
  *           output cube. The cordinate is in output cube space and may be
  *           any integer value negative or positive.
  *
  * @param ob The band position of input cube starting band relative to the
  *           output cube. The cordinate is in output cube space and must be
  *           a legal band number within the output cube.
  *
  * @throws IException::Message
  *
  * @author Sharmila Prasad (8/25/2009)
  */
  void ProcessMosaic::StartProcess(const int &os, const int &ol, const int &ob) {
    // Error checks ... there must be one input and one output
    if ((OutputCubes.size() != 1) || (InputCubes.size() != 1)) {
      QString m = "You must specify exactly one input and one output cube";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    bool bTrackExists = false;
    if (!m_createOutputMosaic) {
      bTrackExists = GetTrackStatus();
      if (m_trackingEnabled &&
            !(OutputCubes[0]->hasGroup("Tracking") || OutputCubes[0]->hasTable("InputImages"))) {
        QString m = "Cannot enable tracking while adding to a mosaic without tracking ";
        m += "information. Confirm that your mosaic was originally created with tracking enabled.";
        throw IException(IException::User, m, _FILEINFO_);
      }
    }

    int ins = m_ins;
    int inl = m_inl;
    int inb = m_inb;
    int iss = m_iss;
    int isl = m_isl;
    int isb = m_isb;

    if (ins == -1)
      ins = (int)InputCubes[0]->sampleCount();

    if (inl == -1)
      inl = (int)InputCubes[0]->lineCount();

    if (inb == -1)
      inb = (int)InputCubes[0]->bandCount();

    // Adjust the input sub-area if it overlaps any edge of the output cube
    m_oss = os;
    m_osl = ol;
    m_osb = ob;

    // Left edge
    if (m_oss < 1) {
      iss = iss - m_oss + 1;
      ins = ins + m_oss - 1;
      m_oss = 1;
    }
    // Top edge
    if (m_osl < 1) {
      isl = isl - m_osl + 1;
      inl = inl + m_osl - 1;
      m_osl = 1;
    }
    // Right edge
    if ((m_oss + ins - 1) > OutputCubes[0]->sampleCount()) {
      ins = OutputCubes[0]->sampleCount() - m_oss + 1;
    }
    // Bottom edge
    if ((m_osl + inl - 1) > OutputCubes[0]->lineCount()) {
      inl = OutputCubes[0]->lineCount() - m_osl + 1;
    }

    PvlGroup imgPosition("ImageLocation");
    imgPosition += PvlKeyword("File", InputCubes[0]->fileName().toStdString());
    imgPosition += PvlKeyword("StartSample", std::to_string(m_oss));
    imgPosition += PvlKeyword("StartLine", std::to_string(m_osl));
    m_imagePositions += imgPosition;

    // Tests for completly off the mosaic
    if ((ins < 1) || (inl < 1)) {
      QString m = "The input cube does not overlap the mosaic";
      throw IException(IException::User, m, _FILEINFO_);
    }

    // Band Adjustments
    if (m_osb < 1) {
      isb = isb - m_osb + 1;
      inb = inb + m_osb - 1;
      m_osb = 1;
    }

    p_progress->SetMaximumSteps(
        (int)InputCubes[0]->lineCount() * (int)InputCubes[0]->bandCount());
    p_progress->CheckStatus();

    // Tracking is done for:
    // (1) Band priority,
    // (2) Ontop and Beneath priority with number of bands equal to 1,
    // (3) Ontop priority with all the special pixel flags set to true. (All special pixel flags
    //      must be set to true in order to handle multiple bands since we need to force pixels
    //      from all bands in a single image to be copied to the mosaic with ontop priority so we
    //      don't have multiple input bands to track for any single pixel in our tracking band.)
    if (m_trackingEnabled) {
      if (!(m_imageOverlay == UseBandPlacementCriteria ||
          ((m_imageOverlay == PlaceImagesOnTop || m_imageOverlay == PlaceImagesBeneath) &&
           // tracking band was already created for Tracking=true
           (OutputCubes[0]->bandCount()) == 1) ||
          (m_imageOverlay == PlaceImagesOnTop && m_placeHighSatPixels && m_placeLowSatPixels &&
           m_placeNullPixels)) ){
        QString m = "Tracking cannot be True for multi-band Mosaic with ontop or beneath priority";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }

    // *******************************************************************************

    Pvl *inLab  = InputCubes[0]->label();
    // Create / Match DEM Shape Model if bMatchDEM Flag is enabled
    if (m_enforceMatchDEM){
      MatchDEMShapeModel();
    }

    // Check to make sure the bandbins match if necessary
    if (m_enforceBandBinMatch) {
      Pvl *outLab = OutputCubes[0]->label();

      if (inLab->findObject("IsisCube").hasGroup("BandBin")) {
        // Check to make sure the output cube has a bandbin group & make sure it
        // matches the input cube bandbin group
        if (!m_createOutputMosaic && outLab->findObject("IsisCube").hasGroup("BandBin")) {
          MatchBandBinGroup(isb, inb);
        }
        // Otherwise copy the input cube bandbin to the output file
        else {
          AddBandBinGroup(isb);
        }
      }
      // BandBin group is not found
      else {
        QString m = "Match BandBin cannot be True when the Image does not have the BandBin group";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }
    // Match BandBin set to false and CREATE and TRACKING is true
    else {
      if (m_createOutputMosaic) {
        if (inLab->findObject("IsisCube").hasGroup("BandBin")) {
          AddBandBinGroup(isb);
        }
        else {
          AddDefaultBandBinGroup();
        }
      }
    }

    // Even if the track flag is off, if the track table exists and "TRACKING" exists in bandbin
    // group continue tracking
    if (bTrackExists) {
      m_trackingEnabled = true;
    }

    // We don't want to set the filename in the table unless the band info is valid
    int bandPriorityInputBandNumber = -1;
    int bandPriorityOutputBandNumber = -1;
    if (m_imageOverlay == UseBandPlacementCriteria ) {
      bandPriorityInputBandNumber = GetBandIndex(true);
      bandPriorityOutputBandNumber = GetBandIndex(false);
    }

    // Set index of tracking image to the default offset of the Isis::UnsignedByte
    int iIndex = VALID_MINUI4;
    // Propogate tracking if adding to mosaic that was previouly tracked.
    if (OutputCubes[0]->hasGroup("Tracking") && !m_createOutputMosaic) {
      m_trackingEnabled = true;
    }

    // Create tracking cube if need-be, add bandbin group, and update tracking table. Add tracking
    // group to mosaic cube.
    if (m_trackingEnabled) {
      TrackingTable *trackingTable;

      if (!m_trackingCube) {
        m_trackingCube = new Cube;

        // A new mosaic cube is being created
        if (m_createOutputMosaic) {

          // Tracking cubes are always unsigned 4 byte integer
          m_trackingCube->setPixelType(PixelType::UnsignedInteger);

          // Tracking cube has the same number of lines and samples as the mosaic and 1 band
          m_trackingCube->setDimensions(OutputCubes[0]->sampleCount(),
                                        OutputCubes[0]->lineCount(),
                                        1);

          // The tracking cube file name convention is "output_cube_file_name" + "_tracking.cub"
          QString trackingBase = FileName(OutputCubes[0]->fileName()).removeExtension().expanded().split("/").last();
          m_trackingCube->create(FileName(OutputCubes[0]->fileName()).path()
                                  + "/" + trackingBase + "_tracking.cub");

          // Add the tracking group to the mosaic cube label
          Pvl *mosaicLabel = OutputCubes[0]->label();
          PvlGroup trackingFileGroup("Tracking");
          PvlKeyword trackingFileName("FileName");
          trackingFileName.setValue(trackingBase.toStdString() + "_tracking.cub");
          trackingFileGroup.addKeyword(trackingFileName);
          mosaicLabel->findObject("IsisCube").addGroup(trackingFileGroup);

          // Write the bandbin group to the tracking cube label
          Pvl *trackingLabel = m_trackingCube->label();
          PvlGroup bandBin("BandBin");
          PvlKeyword trackBand("FilterName");
          trackBand += "TRACKING";
          bandBin.addKeyword(trackBand);
          trackingLabel->findObject("IsisCube").addGroup(bandBin);

          // Write the mapping group to the tracking cube label
          if (inLab->findObject("IsisCube").hasGroup("Mapping")) {
            PvlGroup & mappingGroup = inLab->findGroup("Mapping", Pvl::Traverse);
            trackingLabel->findObject("IsisCube").addGroup(mappingGroup);
          }

          // Initialize an empty TrackingTable object to manage tracking table in tracking cube
          trackingTable = new TrackingTable();
        }

        // An existing mosaic cube is being added to
        else {
          // Confirm tracking group exists in mosaic cube to address backwards compatibility
          if ( OutputCubes[0]->hasGroup("Tracking") ) {
            QString trackingPath = FileName(OutputCubes[0]->fileName()).path();
            QString trackingFile = QString::fromStdString(OutputCubes[0]->group("Tracking").findKeyword("FileName")[0]);
            m_trackingCube->open(trackingPath + "/" + trackingFile, "rw");

            // Initialize a TrackingTable object from current mosaic
            Table *table;
            try {
              table = new Table(TRACKING_TABLE_NAME, m_trackingCube->fileName());
              trackingTable = new TrackingTable(*table);
            }
            catch (IException &e) {
              QString msg = "Unable to find Tracking Table in " + m_trackingCube->fileName() + ".";
              throw IException(IException::User, msg, _FILEINFO_);
            }
          }
          // If no tracking group exists in mosaic cube, warn user to run utility application to
          // add it and create a separate tracking cube
          else {
            std::string msg = "Tracking cannot be enabled when adding to an existing mosaic "
            "that does not already have a tracking cube. Mosaics with a tracking band must "
            "have the tracking band extracted into an external tracking cube.";
            throw IException(IException::User, msg, _FILEINFO_);
          }
        }

        // Add current file to the TrackingTable object
        iIndex = trackingTable->fileNameToPixel(InputCubes[0]->fileName(),
                                       SerialNumber::Compose(*(InputCubes[0])));

        //  Write the tracking table to the tracking cube, overwriting if need-be
        m_trackingCube->deleteBlob(Isis::trackingTableName, "Table");
        Table table = trackingTable->toTable();
        m_trackingCube->write(table);
      }

    }
    else if (m_imageOverlay == AverageImageWithMosaic && m_createOutputMosaic) {
      ResetCountBands();
    }

    m_onb = OutputCubes[0]->bandCount();

    if (m_trackingEnabled) {

      // For mosaic creation, the input is copied onto mosaic by default
      if (m_imageOverlay == UseBandPlacementCriteria && !m_createOutputMosaic) {
        BandComparison(iss, isl, ins, inl,
                       bandPriorityInputBandNumber, bandPriorityOutputBandNumber, iIndex);
      }
    }
    else if (m_imageOverlay == AverageImageWithMosaic) {
      m_onb /= 2;
      if (m_onb < 1) {
        std::string msg = "The mosaic cube needs a count band.";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }

    // Process Band Priority with no tracking
    if (m_imageOverlay == UseBandPlacementCriteria && !m_trackingEnabled ) {
      BandPriorityWithNoTracking(iss, isl, isb, ins, inl, inb, bandPriorityInputBandNumber,
                                 bandPriorityOutputBandNumber);
    }
    else {
      // Create portal buffers for the input and output files
      Portal iPortal(ins, 1, InputCubes[0]->pixelType());
      Portal oPortal(ins, 1, OutputCubes[0]->pixelType());
      Portal countPortal(ins, 1, OutputCubes[0]->pixelType());
      Portal trackingPortal(ins, 1, PixelType::UnsignedInteger);

      for (int ib = isb, ob = m_osb; ib < (isb + inb) && ob <= m_onb; ib++, ob++) {
        for (int il = isl, ol = m_osl; il < isl + inl; il++, ol++) {
          // Set the position of the portals in the input and output cubes
          iPortal.SetPosition(iss, il, ib);
          InputCubes[0]->read(iPortal);

          oPortal.SetPosition(m_oss, ol, ob);
          OutputCubes[0]->read(oPortal);

          if (m_trackingEnabled) {
            trackingPortal.SetPosition(m_oss, ol, 1);
            m_trackingCube->read(trackingPortal);
          }
          else if (m_imageOverlay == AverageImageWithMosaic) {
            countPortal.SetPosition(m_oss, ol, (ob+m_onb));
            OutputCubes[0]->read(countPortal);
          }

          bool bChanged = false;
          // Move the input data to the output
          for (int pixel = 0; pixel < oPortal.size(); pixel++) {
            // Creating Mosaic, copy the input onto mosaic
            // regardless of the priority
            if (m_createOutputMosaic) {
              oPortal[pixel] = iPortal[pixel];
              if (m_trackingEnabled) {
                trackingPortal[pixel] = iIndex;
                bChanged = true;
              }
              else if (m_imageOverlay == AverageImageWithMosaic) {
                if (IsValidPixel(iPortal[pixel])) {
                  countPortal[pixel]=1;
                  bChanged = true;
                }
              }
            }
            // Band Priority
            else if (m_trackingEnabled && m_imageOverlay == UseBandPlacementCriteria) {
              int iPixelOrigin = qRound(trackingPortal[pixel]);

              Portal iComparePortal( ins, 1, InputCubes[0]->pixelType() );
              Portal oComparePortal( ins, 1, OutputCubes[0]->pixelType() );
              iComparePortal.SetPosition(iss, il, bandPriorityInputBandNumber);
              InputCubes[0]->read(iComparePortal);
              oComparePortal.SetPosition(m_oss, ol, bandPriorityOutputBandNumber);
              OutputCubes[0]->read(oComparePortal);

              if (iPixelOrigin == iIndex) {
                if ( ( IsValidPixel(iComparePortal[pixel]) &&
                       IsValidPixel(oComparePortal[pixel]) ) &&
                     ( (!m_bandPriorityUseMaxValue &&
                        iComparePortal[pixel] < oComparePortal[pixel]) ||
                       (m_bandPriorityUseMaxValue &&
                        iComparePortal[pixel] > oComparePortal[pixel]) ) ) {

                  if ( IsValidPixel(iPortal[pixel]) ||
                       ( m_placeHighSatPixels && IsHighPixel(iPortal[pixel]) ) ||
                       ( m_placeLowSatPixels  && IsLowPixel (iPortal[pixel]) ) ||
                       ( m_placeNullPixels    && IsNullPixel(iPortal[pixel]) ) ){
                    oPortal[pixel] = iPortal[pixel];
                    bChanged = true;
                  }
                }
                else { //bad comparison
                  if ( ( IsValidPixel(iPortal[pixel]) && !IsValidPixel(oPortal[pixel]) ) ||
                       ( m_placeHighSatPixels && IsHighPixel(iPortal[pixel]) ) ||
                       ( m_placeLowSatPixels  && IsLowPixel (iPortal[pixel]) ) ||
                       ( m_placeNullPixels    && IsNullPixel(iPortal[pixel]) ) ) {
                    oPortal[pixel] = iPortal[pixel];
                    bChanged = true;
                  }
                }
              }
            }
            // OnTop/Input Priority
            else if (m_imageOverlay == PlaceImagesOnTop) {
              if (IsNullPixel(oPortal[pixel])  ||
                 IsValidPixel(iPortal[pixel]) ||
                 (m_placeHighSatPixels && IsHighPixel(iPortal[pixel])) ||
                 (m_placeLowSatPixels  && IsLowPixel(iPortal[pixel]))  ||
                 (m_placeNullPixels    && IsNullPixel(iPortal[pixel]))) {
                oPortal[pixel] = iPortal[pixel];
                if (m_trackingEnabled) {
                  trackingPortal[pixel] = iIndex;
                  bChanged = true;
                }
              }
            }
            // AverageImageWithMosaic priority
            else if (m_imageOverlay == AverageImageWithMosaic) {
              bChanged |= ProcessAveragePriority(pixel, iPortal, oPortal, countPortal);
            }
            // Beneath/Mosaic Priority
            else if (m_imageOverlay == PlaceImagesBeneath) {
              if (IsNullPixel(oPortal[pixel])) {
                oPortal[pixel] = iPortal[pixel];
                // Set the origin if number of input bands equal to 1
                // and if the track flag was set
                if (m_trackingEnabled) {
                  trackingPortal[pixel] = iIndex;
                  bChanged = true;
                }
              }
            }
          } // End sample loop
          if (bChanged) {
            if (m_trackingEnabled) {
              m_trackingCube->write(trackingPortal);
            }
            if (m_imageOverlay == AverageImageWithMosaic) {
              OutputCubes[0]->write(countPortal);
            }
          }
          OutputCubes[0]->write(oPortal);
          p_progress->CheckStatus();
        } // End line loop
      }   // End band loop
    }
    if (m_trackingCube) {
      m_trackingCube->close();
      delete m_trackingCube;
      m_trackingCube = NULL;
    }
  } // End StartProcess


  /**
   * Cleans up by closing input, output and tracking cubes
   */
  void ProcessMosaic::EndProcess() {
    if (m_trackingCube) {
      m_trackingCube->close();
      delete m_trackingCube;
      m_trackingCube = NULL;
    }
    Process::EndProcess();
  }


  /**
   * Accessor for the placed images and their locations.
   *
   * @return The list of placed images
   */
  PvlObject ProcessMosaic::imagePositions() {
    return m_imagePositions;
  }


  /**
   * Opens an input cube specified by the user. This method is overloaded and
   * adds the requirement that only one input cube can be specified.
   *
   * @return Cube*
   *
   * @param parameter User parameter to obtain file to open. Typically, the value
   *                  is "FROM". For example, the user can specify on the command
   *                  line FROM=myfile.cub and this method will attempt to open
   *                  the cube "myfile.cub" if the parameter was set to "FROM".
   *
   * @param ss The starting sample within the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to 1
   *
   * @param sl The starting line within the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to 1
   *
   * @param sb The starting band within the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to 1
   *
   * @param ns The number of samples from the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to number of samples in the cube
   *
   * @param nl The number of lines from the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to number of lines in the cube
   *
   * @param nb The number of bands from the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to number of bands in the cube
   *
   * @throws IException::Message
   */
  Cube *ProcessMosaic::SetInputCube(const QString &parameter,
                                    const int ss, const int sl, const int sb,
                                    const int ns, const int nl, const int nb) {

    // Make sure only one input is active at a time
    if (InputCubes.size() > 0) {
      QString m = "You must specify exactly one input cube";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    m_iss = ss;
    m_isl = sl;
    m_isb = sb;
    m_ins = ns;
    m_inl = nl;
    m_inb = nb;

    Cube *cInCube = Process::SetInputCube(parameter);

    //get the output label
    Pvl *cInPvl = InputCubes[0]->label();
    if (cInPvl->findGroup("Dimensions", Pvl::Traverse).hasKeyword("Bands")) {
      PvlKeyword &cBandKey = cInPvl->findGroup("Dimensions", Pvl::Traverse).findKeyword("Bands");
      QString sStr(QString::fromStdString(cBandKey[0]));
      if (toInt(sStr) < nb) {
        QString m = "The parameter number of input bands exceeds the actual number of bands in the "
                   "input cube";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }
    return cInCube;
  }


  /**
   * Opens an input cube specified by the user. This method is overloaded and
   * adds the requirement that only one input cube can be specified.
   *
   * @return Cube*
   *
   * @param fname
   *
   * @param att
   *
   * @param ss The starting sample within the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to 1
   *
   * @param sl The starting line within the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to 1
   *
   * @param sb The starting band within the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to 1
   *
   * @param ns The number of samples from the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to number of samples in the cube
   *
   * @param nl The number of lines from the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to number of lines in the cube
   *
   * @param nb The number of bands from the input cube. This allowd the
   *           application to choose a sub-area from the input cube to be place
   *           into the mosaic. Defaults to number of bands in the cube
   *
   * @throws IException
   */
  Cube *ProcessMosaic::SetInputCube(const QString &fname,
                                          CubeAttributeInput &att,
                                          const int ss, const int sl, const int sb,
                                          const int ns, const int nl, const int nb) {

    // Make sure only one input is active at a time
    if (InputCubes.size() > 0) {
      QString m = "You must specify exactly one input cube";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    m_iss = ss;
    m_isl = sl;
    m_isb = sb;
    m_ins = ns;
    m_inl = nl;
    m_inb = nb;

    Cube *cInCube = Process::SetInputCube(fname, att);

    //check if the number of bands specified is not greater than the actual number of bands in the input
    Pvl *cInPvl = InputCubes[0]->label();
    if (cInPvl->findGroup("Dimensions", Pvl::Traverse).hasKeyword("Bands")) {
      PvlKeyword &cBandKey = cInPvl->findGroup("Dimensions", Pvl::Traverse).findKeyword("Bands");
      QString sStr(QString::fromStdString(cBandKey[0]));
      if (toInt(sStr) < nb) {
        QString m = "The parameter number of input bands exceeds the actual number of bands in the input cube";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }
    return cInCube;
  }


  /**
   * Opens an output cube specified by the user. This method is overloaded and
   * adds the requirement that only one output cube can be specified. The output
   * cube must exist before calling SetOutputCube.
   *
   * @return Cube*
   *
   * @param psParameter User parameter to obtain file to open. Typically, the value
   *                    is "TO". For example, the user can specify on the command
   *                    line TO=mosaic.cub and this method will attempt to open the
   *                    cube "mosaic.cub" if the parameter was set to "TO". .
   *
   * @throws IException::Message
   */
  Cube *ProcessMosaic::SetOutputCube(const QString &psParameter) {
    return SetOutputCube(psParameter, Application::GetUserInterface());
  }


  Cube *ProcessMosaic::SetOutputCube(const QString &psParameter, UserInterface &ui) {
    QString fname = ui.GetCubeName(psParameter);

    // Make sure there is only one output cube
    if (OutputCubes.size() > 0) {
      QString m = "You must specify exactly one output cube";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    // Attempt to open a cube ... get the filename from the user parameter
    // (e.g., "TO") and the cube size from an input cube
    Cube *cube = new Cube;
    try {
      cube->open(fname, "rw");
    }
    catch (IException &) {
      delete cube;
      throw;
    }

    if (m_createOutputMosaic) {
      Pvl *outLab = cube->label();
      if (outLab->findObject("IsisCube").hasGroup("BandBin")) {
        outLab->findObject("IsisCube").deleteGroup("BandBin");
      }
    }

    // Everything is fine so save the cube on the stack
    AddOutputCube(cube);
    return cube;
  }


  /**
   *
   */
  void ProcessMosaic::SetBandBinMatch(bool enforceBandBinMatch) {
    m_enforceBandBinMatch = enforceBandBinMatch;
  }


  /**
   * Set the keyword/value to use for comparing when using band priority.
   */
  void ProcessMosaic::SetBandKeyword(QString bandPriorityKeyName, QString bandPriorityKeyValue) {
    m_bandPriorityKeyName = bandPriorityKeyName;
    m_bandPriorityKeyValue = bandPriorityKeyValue;
  }


  /**
   * Set the band to use for priorities when using band priority.
   */
  void ProcessMosaic::SetBandNumber(int bandPriorityBandNumber) {
    m_bandPriorityBandNumber = bandPriorityBandNumber;
  }


  /**
   * Set whether to take the image with the max or min value when
   *   using band priority.
   */
  void ProcessMosaic::SetBandUseMaxValue(bool useMax) {
    m_bandPriorityUseMaxValue = useMax;
  }


  /**
   * Flag to indicate that the mosaic is being newly created
   * Indication that the new label specific to the mosaic needs to be created.
   *
   * @author Sharmila Prasad (1/19/2011)
   *
   * @param createOutputMosaic - Set Create Flag True/False
   */
  void ProcessMosaic::SetCreateFlag(bool createOutputMosaic) {
    m_createOutputMosaic = createOutputMosaic;
  }


  /**
   * When true, high saturation (HRS, HIS) will be considered valid data for the purposes of
   *   placing pixels in the output mosaic.
   */
  void ProcessMosaic::SetHighSaturationFlag(bool placeHighSatPixels) {
    m_placeHighSatPixels = placeHighSatPixels;
  }


  void ProcessMosaic::SetImageOverlay(ImageOverlay placement) {
    m_imageOverlay = placement;
  }


  /**
   * When true, low saturation (LRS, LIS) will be considered valid data for the purposes of
   *   placing pixels in the output mosaic.
   */
  void ProcessMosaic::SetLowSaturationFlag(bool placeLowSatPixels) {
    m_placeLowSatPixels = placeLowSatPixels;
  }

  /**
   * Set the DEM match flag.
   * @param matchDEM If true, the match is enforced between Input & Mosaic
   */
  void ProcessMosaic::SetMatchDEM(bool matchDEM) {
    m_enforceMatchDEM = matchDEM;
  }


  /**
   * When true, Null pixels will be considered valid data for the purposes of
   *   placing pixels in the output mosaic.
   */
  void ProcessMosaic::SetNullFlag(bool placeNullPixels) {
    m_placeNullPixels = placeNullPixels;
  }


  void ProcessMosaic::SetTrackFlag(bool trackingEnabled) {
    m_trackingEnabled = trackingEnabled;
  }


  /**
   * @see SetHighSaturationFlag()
   */
  bool ProcessMosaic::GetHighSaturationFlag() const {
    return m_placeHighSatPixels;
  }


  /**
   *
   */
  ProcessMosaic::ImageOverlay ProcessMosaic::GetImageOverlay() const {
    return m_imageOverlay;
  }


  /**
   * @see SetLowSaturationFlag()
   */
  bool ProcessMosaic::GetLowSaturationFlag() const {
    return m_placeLowSatPixels;
  }


  /**
   * @see SetNullFlag()
   */
  bool ProcessMosaic::GetNullFlag() const {
    return m_placeNullPixels;
  }


  /**
   * @see SetTrackFlag()
   */
  bool ProcessMosaic::GetTrackFlag() const {
    return m_trackingEnabled;
  }


  /**
   * This is the line where the image was placed into the output mosaic.
   */
  int ProcessMosaic::GetInputStartLineInMosaic() const {
    return m_osl;
  }


  /**
   * This is the sample where the image was placed into the output mosaic.
   */
  int ProcessMosaic::GetInputStartSampleInMosaic() const {
    return m_oss;
  }


  /**
   * This is the band where the image was placed into the output mosaic.
   */
  int ProcessMosaic::GetInputStartBandInMosaic() const {
    return m_osb;
  }


  /**
   * Convert an ImageOverlay to a QString. This is used to translate between
   *   mapmos, handmos, and automos' interfaces into an ImageOverlay.
   */
  QString ProcessMosaic::OverlayToString(ImageOverlay imageOverlay) {
    QString result;

    switch (imageOverlay) {
      case PlaceImagesOnTop:
        result = "OnTop";
        break;

      case PlaceImagesBeneath:
        result = "Beneath";
        break;

      case UseBandPlacementCriteria:
        result = "Band";
        break;

      case AverageImageWithMosaic:
        result = "Average";
        break;

      case NumImageOverlayOptions:
        break;
    }

    if (result == "") {
      throw IException(IException::Unknown,
                       "Cannot convert overlay [" + std::to_string((int)imageOverlay) + "] to a string",
                       _FILEINFO_);
    }

    return result;
  }


  /**
   * Convert a QString to an ImageOverlay (case-insensitive). This is used to translate between
   *   mapmos, handmos, and automos' interfaces into an ImageOverlay.
   */
  ProcessMosaic::ImageOverlay ProcessMosaic::StringToOverlay(QString imageOverlayString) {
    QString imageOverlayStringUpper = imageOverlayString.toUpper();
    for (int i = 0; i < NumImageOverlayOptions; i++) {
      if (OverlayToString((ImageOverlay)i).toUpper() == imageOverlayStringUpper)
        return (ImageOverlay)i;
    }

    throw IException(IException::Unknown,
                     "The text [" + imageOverlayString + "] does not correspond to any known "
                     "image overlay modes (mosaic priorities)",
                     _FILEINFO_);
  }


  /**
   * Match the Shape Model for input and mosaic. If creating the mosaic,
   * copy the input ShapeModel from the input label.
   * Store only the file name of the Shape Model
   *
   * @author Sharmila Prasad (1/24/2011)
   */
  void ProcessMosaic::MatchDEMShapeModel() {
    Pvl* inLabel  = InputCubes[0]->label();
    Pvl* outLabel = OutputCubes[0]->label();

    if (outLabel->findObject("IsisCube").hasGroup("Mosaic")) {
      PvlGroup outMosaicGrp = outLabel->findObject("IsisCube").findGroup("Mosaic");
      if (outMosaicGrp.hasKeyword("ShapeModel")) {
        if (inLabel->findObject("IsisCube").hasGroup("Kernels")) {
          PvlGroup inMosaicGrp = inLabel->findObject("IsisCube").findGroup("Kernels");
          if (outMosaicGrp.hasKeyword("ShapeModel") && inMosaicGrp.hasKeyword("ShapeModel")) {
            PvlKeyword outShapeModelKey = outMosaicGrp.findKeyword("ShapeModel");
            QString sShapeModel = QString::fromStdString(inMosaicGrp.findKeyword("ShapeModel")[0]);
            int found = sShapeModel.lastIndexOf("/");
            if (found != -1) {
              sShapeModel.remove(0, found + 1);
            }
            if (sShapeModel == QString::fromStdString(outShapeModelKey[0])) {
              return;
            }
          }
        }
        QString sErrMsg = "Input and Mosaic DEM Shape Model do not match";
        throw IException(IException::User, sErrMsg, _FILEINFO_);
      }
    }
    else {
      if (m_createOutputMosaic) {
        if (inLabel->findObject("IsisCube").hasGroup("Kernels")) {
          QString sShapeModel =
              QString::fromStdString(inLabel->findObject("IsisCube").findGroup("Kernels").findKeyword("ShapeModel")[0]);
          int found = sShapeModel.lastIndexOf("/");
          if (found != -1){
            sShapeModel.remove(0, found+1);
          }
          PvlObject & outIsisCubeObj = outLabel->findObject("IsisCube");
          PvlGroup mosaicGrp("Mosaic");
          PvlKeyword shapeModelKey("ShapeModel");
          shapeModelKey += sShapeModel.toStdString();
          mosaicGrp += shapeModelKey;
          outIsisCubeObj += mosaicGrp;
        }
      }
    }
  }


  /**
   * Reset all the count bands to default at the time of mosaic creation
   *
   * @author Sharmila Prasad (1/13/2011)
   */
  void ProcessMosaic::ResetCountBands()
  {
    int iBand   = OutputCubes[0]->bandCount();
    int iLines  = OutputCubes[0]->lineCount();
    int iSample = OutputCubes[0]->sampleCount();

    Portal countPortal(iSample, 1, OutputCubes[0]->pixelType());
    int iStartCountBand = iBand/2 + 1;

    for (int band=iStartCountBand; band<=iBand; band++) {
      for (int i = 1; i <= iLines; i++) {
        countPortal.SetPosition(1, i, band);  //sample, line, band position
        OutputCubes[0]->read(countPortal);
        for (int iPixel = 0; iPixel < countPortal.size(); iPixel++) {
          countPortal[iPixel] = 0;
        }
        OutputCubes[0]->write(countPortal);
      }
    }
  }


  /**
   * Calculate DN value for a pixel for AverageImageWithMosaic priority and set the
   * Count band portal
   *
   * @author Sharmila Prasad (1/13/2011)
   *
   * @param piPixel     - Pixel index
   * @param piPortal    - Input Portal
   * @param poPortal    - Output Portal
   * @param countPortal - Count Portal
   *
   * @return bool
   */
  bool ProcessMosaic::ProcessAveragePriority(int piPixel, Portal& piPortal, Portal& poPortal,
                                             Portal& countPortal)
  {
    bool bChanged=false;
    if (IsValidPixel(piPortal[piPixel]) && IsValidPixel(poPortal[piPixel])) {
      int iCount = (int)countPortal[piPixel];
      double dNewDN = (poPortal[piPixel] * iCount + piPortal[piPixel]) / (iCount + 1);
      poPortal[piPixel] = dNewDN;
      countPortal[piPixel] =iCount +1;
      bChanged = true;
    }
    // Input-Valid, Mosaic-Special
    else if (IsValidPixel(piPortal[piPixel])) {
      poPortal[piPixel] = piPortal[piPixel];
      countPortal[piPixel] = 1;
      bChanged = true;
    }
    // Input-Special, Flags-True
    else if (IsSpecial(piPortal[piPixel])) {
      if ((m_placeHighSatPixels && IsHighPixel(piPortal[piPixel])) ||
         (m_placeLowSatPixels  && IsLowPixel (piPortal[piPixel]))  ||
         (m_placeNullPixels    && IsNullPixel(piPortal[piPixel]))) {
        poPortal[piPixel]    = piPortal[piPixel];
        countPortal[piPixel] = 0;
        bChanged = true;
      }
    }
    return bChanged;
  }


  /**
   *  This method matches the input BandBin group to the mosaic BandBin Group
   *  and allows band to be replaced in mosaic if it is NA (not assigned).
   *  It expects the bands to be contiguous
   *
   *  returns None
   *
   *  @throws IException::Message
   *
   *  @author Sharmila Prasad (9/25/2009)
   */
  void ProcessMosaic::MatchBandBinGroup(int origIsb, int &inb) {
    Pvl *inLab  = InputCubes[0]->label();
    Pvl *outLab = OutputCubes[0]->label();

    PvlGroup &inBin  = inLab->findGroup("BandBin", Pvl::Traverse);
    PvlGroup &outBin = outLab->findGroup("BandBin", Pvl::Traverse);
    if (inBin.keywords() != outBin.keywords()) {
      std::string msg = "Pvl Group [BandBin] does not match between the input and output cubes";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    //pvl - zero based
    int isb = (origIsb - 1);
    int osb = (m_osb - 1);
    int iOutBandsHalf = OutputCubes[0]->bandCount()/2;

    for (int i = 0; i < outBin.keywords(); i++) {
      PvlKeyword &outKey = outBin[i];
      QString sOutName = QString::fromStdString(outKey.name());
      if (inBin.hasKeyword(sOutName.toStdString())) {
        PvlKeyword &inKey = inBin[sOutName.toStdString()];
        for (int j = osb, k = isb; j < outKey.size() && k < inKey.size(); j++, k++) {
          if (outKey[j] == "NA") {
            outKey[j] = inKey[k];
            if (m_imageOverlay == AverageImageWithMosaic) {
              if (sOutName.contains("Filter") ||
                  sOutName.contains("Name")) {
                outKey[j+iOutBandsHalf] = inKey[k] + "_Count";
              }
              else {
                outKey[j+iOutBandsHalf] = "Avg_Count";
              }
            }
          }
          else if (outKey[j] != inKey[k]) {
            std::string msg = "The input cube [" + inLab->fileName() + "] and the base mosaic values "
                           "of the Pvl Group [BandBin] for Keyword [" + outKey.name() + "] do not "
                           "match. Base mosaic value at index [" + std::to_string(j) + "] = " +
                           outKey[j] + ". Input cube value at index [" + std::to_string(k) + "] = "
                           + inKey[k] + ". **Note: use mapmos/automos MatchBandBin = false to "
                           "override this check**";
            //QString msg = "Pvl Group [BandBin] in Key[" + outKey.name() + "] In value" + inKey[k] +
                         //"and Out value=" + outKey[j] + " do not match";
            throw IException(IException::User, msg, _FILEINFO_);
          }
        }
      }
      else {
        std::string msg = "Pvl Group [BandBin] In Keyword[" + inBin[i].name() + "] and Out Keyword[" +
                     outBin[i].name() + "] does not match";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    int inputRange = InputCubes[0]->bandCount() - isb;
    int outputRange = OutputCubes[0]->bandCount() - osb;
    inb = min(inputRange, outputRange);
  }


  /**
   *  This method adds the BandBin group to the mosaic corresponding
   *  to the actual bands in the mosaic
   *
   *  returns None
   *
   *  @author Sharmila Prasad (9/24/2009)
   *
   *  @history 2011-01-12 Sharmila Prasad Added logic for Count Bands for
   *  AverageImageWithMosaic Priority
   */
  void ProcessMosaic::AddBandBinGroup(int origIsb) {
    Pvl *inLab  = InputCubes[0]->label();
    Pvl *outLab = OutputCubes[0]->label();

    int iOutBands = OutputCubes[0]->bandCount();

    // else if (m_imageOverlay == AverageImageWithMosaic) {
    if (m_imageOverlay == AverageImageWithMosaic) {
      iOutBands /= 2;
    }

    int isb = origIsb - 1; // array zero based
    int osb = m_osb - 1;

    PvlGroup &cInBin  = inLab->findGroup("BandBin", Pvl::Traverse);
    PvlGroup cOutBin("BandBin");

    for (int i = 0; i < cInBin.keywords(); i++) {
      PvlKeyword &cInKey = cInBin[i];
      int iInKeySize = cInKey.size();
      PvlKeyword cOutKey(cInKey.name());

      for (int b = 0; b < osb; b++) {
        cOutKey += "NA";
      }
      for (int b = osb; b < iOutBands; b++) {
        if (isb < iInKeySize) {
          cOutKey += cInKey[isb++];
        }
        else {
          cOutKey += "NA";
        }
      }

      // Tag the Count Bands if priority is AverageImageWithMosaic.
      if (m_imageOverlay == AverageImageWithMosaic) {

        int iTotalOutBands = OutputCubes[0]->bandCount();
        isb = origIsb - 1; // reset the input starting band
        int iOutStartBand = iOutBands + osb;
        QString sKeyName = QString::fromStdString(cInKey.name());
        bool bFilterKey = false;
        if (sKeyName.contains("Filter")   ||
            sKeyName.contains("Original") ||
            sKeyName.contains("Name")) {
          bFilterKey = true;
        }
        for (int ob=iOutBands; ob<iTotalOutBands; ob++) {
          if (isb < iInKeySize && ob >= iOutStartBand) {
            if (bFilterKey) {
              cOutKey += cInKey[isb++] + "_Count";
            }
            else {
              cOutKey += 0;
              isb++;
            }
          }
          else {
            cOutKey += 0;
          }
        }
      }

      // Check for units and make sure output keyword units value is set to input
      // keyword units value
      if (cOutKey.unit() != cInKey.unit()) {
        cOutKey.setUnits((cInKey.unit()));
      }

      cOutBin += cOutKey;
      isb = origIsb - 1;        // reinitialize the input starting band
    }
    outLab->findObject("IsisCube").addGroup(cOutBin);
  }


  /**
   * AddDefaultBandBinGroup
   *
   * This method adds a default BandBin group on Mosaic creation
   * if the MatchBandBin Group is set to false and Tracking to set
   * to true
   *
   * Return void
   */
  void ProcessMosaic::AddDefaultBandBinGroup() {
    Pvl *outLab = OutputCubes[0]->label();

    PvlGroup cOutBin("BandBin");

    int iOutBands = OutputCubes[0]->bandCount();
    int iOutBandsTotal = iOutBands;

    if (m_trackingEnabled) {
      iOutBands--; // Leave tracking band
    }
    else if (m_imageOverlay == AverageImageWithMosaic) {
      iOutBands /= 2;
    }

    PvlKeyword cOutKey("FilterName");

    for (int i=0; i<iOutBands; i++) {
      cOutKey += "NA";
    }

    if (m_imageOverlay == AverageImageWithMosaic) {
      for (int i=iOutBands; i<iOutBandsTotal; i++) {
        cOutKey += "NA_Count";
      }
    }

    if (m_trackingEnabled) {
      cOutKey += "TRACKING";
    }

    cOutBin += cOutKey;

    outLab->findObject("IsisCube").addGroup(cOutBin);
  }


  /**
   *  Get the Band Index in an image of type (input/output)
   */
  int ProcessMosaic::GetBandIndex(bool inputFile) {
    bool bFound = false;
    int iBandIndex = 0;

    Pvl cPvlLabel;

    if (inputFile) {
      cPvlLabel = *(InputCubes[0]->label());
      if (m_bandPriorityBandNumber <= InputCubes[0]->bandCount() &&
          m_bandPriorityBandNumber > 0) {
          iBandIndex = m_bandPriorityBandNumber;
          bFound = true;
      }
    }
    else {
      cPvlLabel = *(OutputCubes[0]->label());
      if (m_bandPriorityBandNumber <= OutputCubes[0]->bandCount() &&
          m_bandPriorityBandNumber > 0) {
          iBandIndex = m_bandPriorityBandNumber;
          bFound = true;
      }
    }

    //key name
    if (!m_bandPriorityBandNumber) {
      PvlKeyword cKeyName;
      if (cPvlLabel.findGroup("BandBin", Pvl::Traverse).hasKeyword(m_bandPriorityKeyName.toStdString())) {
        cKeyName = cPvlLabel.findGroup("BandBin", Pvl::Traverse).findKeyword(m_bandPriorityKeyName.toStdString());
      }
      int iSize = cKeyName.size();
      for (int i = 0; i < iSize; i++) {
        if (m_bandPriorityKeyValue.toUpper() == QString::fromStdString(cKeyName[i]).toUpper()) {
          iBandIndex = i + 1; //1 based get key value index
          bFound = true;
          break;
        }
      }
    }
    if (!bFound) {
      std::string msg = "Invalid Band / Key Name, Value ";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    return iBandIndex;
  }


  /**
   * This method compares the specified band of the input and
   * output using the criteria (lesser or greater) to assign the
   * pixel origin(input fileindex) to the origin band. In the StartProcess(),
   * input pixel is assigned to the output if the origin pixel equals the current
   * input file index
   *
   * @param iss - Comparison start sample
   * @param isl - Comparison start line
   * @param ins - The number of samples to compare
   * @param inl - The number of lines to compare
   * @param bandPriorityInputBandNumber - The band in the input cube to use for comparison
   * @param bandPriorityOutputBandNumber - The band in the output cube to use for comparison
   * @param index - Tracking index for the input cube
   *
   * @author Sharmila Prasad (9/04/2009)
   */
  void ProcessMosaic::BandComparison(int iss, int isl, int ins, int inl,
      int bandPriorityInputBandNumber, int bandPriorityOutputBandNumber, int index) {
    //
    // Create portal buffers for the input and output files
    Portal cIportal(ins, 1, InputCubes[0]->pixelType());
    Portal cOportal(ins, 1, OutputCubes[0]->pixelType());
    Portal trackingPortal(ins, 1, PixelType::UnsignedInteger);

    for (int iIL = isl, iOL = m_osl; iIL < isl + inl; iIL++, iOL++) {
      // Set the position of the portals in the input and output cubes
      cIportal.SetPosition(iss, iIL, bandPriorityInputBandNumber);
      InputCubes[0]->read(cIportal);

      cOportal.SetPosition(m_oss, iOL, bandPriorityOutputBandNumber);
      OutputCubes[0]->read(cOportal);

      trackingPortal.SetPosition(m_oss, iOL, 1);
      m_trackingCube->read(trackingPortal);

      // Move the input data to the output
      for (int iPixel = 0; iPixel < cOportal.size(); iPixel++) {
        if ((m_placeHighSatPixels && IsHighPixel(cIportal[iPixel])) ||
            (m_placeLowSatPixels  && IsLowPixel(cIportal[iPixel])) ||
            (m_placeNullPixels    && IsNullPixel(cIportal[iPixel]))) {
          trackingPortal[iPixel] = index;
        }
        else {
          if (IsValidPixel(cIportal[iPixel])) {
            if (IsSpecial(cOportal[iPixel]) ||
                (m_bandPriorityUseMaxValue == false  && cIportal[iPixel] < cOportal[iPixel]) ||
                (m_bandPriorityUseMaxValue == true && cIportal[iPixel] > cOportal[iPixel])) {
              trackingPortal[iPixel] = index;
            }
          }
        }
      }
      m_trackingCube->write(trackingPortal);
    }
  }


  /**
   * Mosaicking for Band Priority with no Tracking
   *
   * @author Sharmila Prasad (1/4/2012)
   */
void ProcessMosaic::BandPriorityWithNoTracking(int iss, int isl, int isb, int ins, int inl,
                                                 int inb, int bandPriorityInputBandNumber,
                                                 int bandPriorityOutputBandNumber) {
    /*
     * specified band for comparison
     * Create portal buffers for the input and output files pointing to the
     */
    Portal iComparePortal( ins, 1, InputCubes[0]->pixelType() );
    Portal oComparePortal( ins, 1, OutputCubes[0]->pixelType() );
    Portal resultsPortal ( ins, 1, OutputCubes[0]->pixelType() );

    // Create portal buffers for the input and output files
    Portal iPortal( ins, 1, InputCubes[0]->pixelType() );
    Portal oPortal( ins, 1, OutputCubes[0]->pixelType() );

    for (int inLine = isl, outLine = m_osl; inLine < isl + inl; inLine++, outLine++) {
//       Set the position of the portals in the input and output cubes
      iComparePortal.SetPosition(iss, inLine, bandPriorityInputBandNumber);
      InputCubes[0]->read(iComparePortal);

      oComparePortal.SetPosition(m_oss, outLine, bandPriorityOutputBandNumber);
      OutputCubes[0]->read(oComparePortal);

      Portal iPortal( ins, 1, InputCubes[0]->pixelType() );
      Portal oPortal( ins, 1, OutputCubes[0]->pixelType() );

      bool inCopy = false;
//       Move the input data to the output
      for (int iPixel = 0; iPixel < ins; iPixel++) {
        resultsPortal[iPixel] = false;
        if (m_createOutputMosaic) {
          resultsPortal[iPixel] = true;
          inCopy = true;
        }
        else if ( IsValidPixel(iComparePortal[iPixel]) && IsValidPixel(oComparePortal[iPixel]) ) {
          if ( (m_bandPriorityUseMaxValue == false  &&
                iComparePortal[iPixel] < oComparePortal[iPixel]) ||
                (m_bandPriorityUseMaxValue == true &&
                iComparePortal[iPixel] > oComparePortal[iPixel]) ) {
            resultsPortal[iPixel] = true;
            inCopy = true;
          }
        }
        else if (IsValidPixel(iComparePortal[iPixel]) && !IsValidPixel(oComparePortal[iPixel]) ) {
          resultsPortal[iPixel] = true;
          inCopy = true;
        }
      }
      if (inCopy) {
        for (int ib = isb, ob = m_osb; ib < (isb + inb) && ob <= m_onb; ib++, ob++) {
//           Set the position of the portals in the input and output cubes
          iPortal.SetPosition(iss, inLine, ib);
          InputCubes[0]->read(iPortal);

          oPortal.SetPosition(m_oss, outLine, ob);
          OutputCubes[0]->read(oPortal);

          for (int iPixel = 0; iPixel < ins; iPixel++) {
            if (resultsPortal[iPixel]) {
              if (m_createOutputMosaic) {
                oPortal[iPixel] = iPortal[iPixel];
              }
              else if ( IsValidPixel(iPortal[iPixel]) ||
                        (m_placeHighSatPixels && IsHighPixel(iPortal[iPixel]) ) ||
                        (m_placeLowSatPixels  && IsLowPixel (iPortal[iPixel]) ) ||
                        (m_placeNullPixels    && IsNullPixel(iPortal[iPixel]) ) ) {
                oPortal[iPixel] = iPortal[iPixel];
              }
            }
            else if ( IsValidPixel(iPortal[iPixel])  && !IsValidPixel(oPortal[iPixel]) ) {
              oPortal[iPixel] = iPortal[iPixel];
            }
          }
          OutputCubes[0]->write(oPortal);
        }
      }
    }
  }


  /**
   * This method  returns the defaults(unassigned origin value)
   * depending on the pixel type.
   *
   * @No parameters
   *
   * @returns default value
   *
   * @throws IException::Message
   *
   * @author Sharmila Prasad (9/10/2009)
   */
  int ProcessMosaic::GetOriginDefaultByPixelType() {
    int iDefault;

    switch (SizeOf(OutputCubes[0]->pixelType())) {
      case 1:
        iDefault = NULL1;
        break;

      case 2:
        iDefault = NULL2;
        break;

      case 4:
        iDefault = INULL4;
        break;

      default:
        QString msg = "ProcessMosaic::GetOriginDefaultByPixelType - Invalid Pixel Type";
        throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return iDefault;
  }


  /**
   * This method searchs the mosaic label for a table with name
   * "InputFile". If found return true else false. Checks for the
   * existence of the origin table
   *
   * @returns the table index of the current image
   *
   * @author Sharmila Prasad (9/22/2009)
   */
  bool ProcessMosaic::GetTrackStatus() {
    //get the output label
    Pvl *cPvlOut = OutputCubes[0]->label();

    bool bGroupExists = false;
    PvlObject cPvlObj;

    //Check if table already exists
    if (cPvlOut->hasGroup("Tracking")) {
      bGroupExists = true;
    }

    return bGroupExists;
  }

}
