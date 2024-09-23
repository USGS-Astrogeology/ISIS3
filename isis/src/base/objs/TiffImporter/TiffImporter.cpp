/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>

#include <QDebug>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>

#include "TiffImporter.h"

#include "Angle.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlToPvlTranslationManager.h"
#include "TProjection.h"

namespace Isis {
  /**
   * Construct the importer.
   *
   * @param inputName The name of the input image
   */
  TiffImporter::TiffImporter(FileName inputName) : ImageImporter(inputName) {

    // Open the TIFF image
    m_image = NULL;
    if ((m_image = XTIFFOpen(inputName.expanded().c_str(), "r")) == NULL) {
      throw IException(IException::Programmer, "Could not open TIFF image [" + inputName.expanded() + "]", _FILEINFO_);
    }

    // Get its constant dimensions.  Note, height seems to get reset to 0 if
    // called before setting width.
    uint32_t height;
    TIFFGetField(m_image, TIFFTAG_IMAGELENGTH, &height);
    setLines(height);

    uint32_t width;
    TIFFGetField(m_image, TIFFTAG_IMAGEWIDTH, &width);
    setSamples(width);

    TIFFGetField(m_image, TIFFTAG_SAMPLESPERPIXEL, &m_samplesPerPixel);

    // Setup the width and height of the image
    unsigned long imagesize = lines() * samples();
    m_raster = NULL;
    if ((m_raster = (uint32_t *) malloc(sizeof(uint32_t) * imagesize)) == NULL) {
      throw IException(IException::Programmer,
          "Could not allocate enough memory", _FILEINFO_);
    }

    // Read the image into the memory buffer
    if (TIFFReadRGBAImage(m_image, samples(), lines(), m_raster, 0) == 0) {
      throw IException(IException::Programmer,
          "Could not read image", _FILEINFO_);
    }

    // Deal with photometric interpretations
    if (TIFFGetField(m_image, TIFFTAG_PHOTOMETRIC, &m_photo) == 0) {
      throw IException(IException::Programmer,
          "Image has an undefined photometric interpretation", _FILEINFO_);
    }

    // Get the geotiff info (if available)
    m_geotiff = NULL;
    m_geotiff = GTIFNew(m_image);

    setDefaultBands();
  }


  /**
   * Destruct the importer.
   */
  TiffImporter::~TiffImporter() {
    free(m_raster);
    m_raster = NULL;

    GTIFFree(m_geotiff);
    m_geotiff = NULL;

    XTIFFClose(m_image);
    m_image = NULL;
  }


  /**
   * Convert any projection information associated with the input image to an ISIS Mapping group in
   * PVL form. Currently this routine only handles GeoTiff tags containing UTM projection
   * parameters. NOTE: As written, this only works for a few projections. The tranlsation files
   * can be extened to work for more, but the whole design needs to be generalized and thought out.
   *
   *  References: http://www.remotesensing.org/geotiff/spec/geotiffhome.html
   * @return The ISIS PVL mapping group
   *
   * @internal
   *   @todo Generalize this and make it work for all ISIS projections (preferably without changes
   *         when new projections are added to ISIS)
   */
  PvlGroup TiffImporter::convertProjection() const {

    Pvl outPvl;
    outPvl.addGroup(PvlGroup("Mapping"));

    geocode_t modelType;
    geocode_t rasterType;
    geocode_t coordSysType;

    if ((GTIFKeyGet(m_geotiff, GTModelTypeGeoKey, &modelType, 0, 1) == 1) &&
        ((modelType == 1) || (modelType == 2))) { // 1=ModelTypeProjected, 2=ModelTypeGeographic

      if ((GTIFKeyGet(m_geotiff, GTRasterTypeGeoKey, &rasterType, 0, 1) == 1) &&
          (rasterType == 1 || rasterType == 2)) { // Area || Point

        // See if this geogiff uses a coded projection. That is, the projection parameters are not
        // explicitly set. A code in the GEOTIF tag indicates a set of parameters for this
        // projection.
        if (GTIFKeyGet(m_geotiff, ProjectedCSTypeGeoKey, &coordSysType, 0, 1) == 1) {

          // Get the mapping group data for this code: proj name, clat, clon, ...
          FileName transFile("$ISISROOT/appdata/translations/" +
                                 Isis::toString(coordSysType) + ".trn");
          if (transFile.fileExists()) {
            Pvl tmp;
            tmp += PvlKeyword("Code", Isis::toString(coordSysType));
            PvlToPvlTranslationManager geoTiffCodeTranslater(tmp, QString::fromStdString(transFile.expanded()));
            geoTiffCodeTranslater.Auto(outPvl);
          }
        }

        // The following is here for when this gets generalized to handle non coded projections
        // (i.e., the real proj parameters have values and the code is not used)
        else if (1 == 0) {
          geocode_t geoCode;
          if (GTIFKeyGet(m_geotiff, GeographicTypeGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "GeographicTypeGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no GeographicTypeGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, GeogAngularUnitsGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "GeogAngularUnitsGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no GeogAngularUnitsGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, GeogEllipsoidGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "GeogEllipsoidGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no GeogEllipsoidGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, GeogSemiMajorAxisGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "GeogSemiMajorAxisGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no GeogSemiMajorAxisGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, GeogSemiMinorAxisGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "GeogSemiMinorAxisGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no GeogSemiMinorAxisGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, GeogInvFlatteningGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "GeogInvFlatteningGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no GeogInvFlatteningGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjCoordTransGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjCoordTransGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjCoordTransGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjLinearUnitsGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjLinearUnitsGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjLinearUnitsGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjStdParallel1GeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjStdParallel1GeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjStdParallel1GeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjStdParallel2GeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjStdParallel2GeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjStdParallel2GeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjNatOriginLongGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjNatOriginLongGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjNatOriginLongGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjNatOriginLatGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjNatOriginLatGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjNatOriginLatGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjFalseEastingGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjFalseEastingGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjFalseEastingGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjFalseNorthingGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjFalseNorthingGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjFalseNorthingGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjFalseOriginLongGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjFalseOriginLongGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjFalseOriginLongGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjFalseOriginLatGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjFalseOriginLatGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjFalseOriginLatGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjFalseOriginEastingGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjFalseOriginEastingGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjFalseOriginEastingGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjFalseOriginNorthingGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjFalseOriginNorthingGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjFalseOriginNorthingGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjCenterLongGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjCenterLongGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjCenterLongGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjCenterLatGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjCenterLatGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjCenterLatGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjCenterEastingGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjCenterEastingGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjCenterEastingGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjCenterNorthingGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjCenterNorthingGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjCenterNorthingGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjScaleAtNatOriginGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjScaleAtNatOriginGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjScaleAtNatOriginGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjAzimuthAngleGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjAzimuthAngleGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjAzimuthAngleGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, ProjStraightVertPoleLongGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "ProjStraightVertPoleLongGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no ProjStraightVertPoleLongGeoKey" << std::endl;
          }

          if (GTIFKeyGet(m_geotiff, VerticalUnitsGeoKey, &geoCode, 0, 1) == 1) {
            std::cout << "VerticalUnitsGeoKey = " << geoCode << std::endl;
          }
          else {
            std::cout << "no VerticalUnitsGeoKey" << std::endl;
          }
        } // End of individual GEOTIFF projection parameters

        // There are some projections parameters that are not in the GEO part of the TIFF, get them

        // Get the Tiff Tiepoint tag and convert those to Upper Left X & Y
        outPvl = upperLeftXY(outPvl);

        // Get the Tiff PixelScale tag and convert it to resolution and scale
        outPvl = resolution(outPvl);

        // Get the GDAL minimum and maximum lats and lons
        outPvl = gdalItems(outPvl);

      } // End of raster type


      // Test the projection
      // If any things goes wrong just ignore the projection and move on
      try {
        TProjection *proj = NULL;
        proj = (TProjection *) ProjectionFactory::Create(outPvl);

        PvlGroup &mapGroup = outPvl.findGroup("Mapping");
        double pixelResolution = mapGroup["PixelResolution"];
        double trueScaleLat = proj->TrueScaleLatitude();
        double localRadius = proj->LocalRadius(trueScaleLat);

        double scale = (2.0 * Isis::PI * localRadius) / (360.0 * pixelResolution);
        mapGroup += PvlKeyword("Scale", Isis::toString(scale), "pixels/degree");
      }
      catch (IException &e) {
        outPvl.findGroup("Mapping").clear();
      }
    }

    return outPvl.findGroup("Mapping");
  }


  /**
   * Convert items in the GDAL tag to the ISIS mapping group. This includes the min/max lats/lons
   *
   * @return The ISIS PVL with a mapping group
   *
   */
  Pvl TiffImporter::gdalItems(const Pvl &inLab) const {

    Pvl newLab = inLab;
    PvlGroup &map = newLab.findGroup("Mapping");

    // Get the GDALMetadata tag (42112) to get the lat/lon boundry
    char *gdalMetadataBuf;
    short int gdalMetadataCount = 0;

    if (TIFFGetField(m_image, 42112, &gdalMetadataCount, &gdalMetadataBuf) == 1) {

      QString gdalMetadataQstring(gdalMetadataBuf);

      QDomDocument gdalDoc("GDALMetaData");
      if (gdalDoc.setContent(gdalMetadataQstring)) {
        QDomElement gdalRoot = gdalDoc.documentElement();
        if (gdalRoot.tagName() == "GDALMetadata") {

          QDomNode gdalNode = gdalRoot.firstChild();
          while (!gdalNode.isNull()) {
            QDomElement gdalElement = gdalNode.toElement();
            if (!gdalElement.isNull() ) {
              if (gdalElement.tagName() == "Item") {
                if (gdalElement.attribute("name", "") == "WEST_LONGITUDE") {
                  QString westLon = gdalElement.text();
                  map += PvlKeyword("MinimumLongitude", Isis::toString(Angle(westLon).degrees()));
                }
                else if (gdalElement.attribute("name", "") == "EAST_LONGITUDE") {
                  QString eastLon = gdalElement.text();
                  map += PvlKeyword("MaximumLongitude", Isis::toString(Angle(eastLon).degrees()));
                }
                else if (gdalElement.attribute("name", "") == "SOUTH_LATITUDE") {
                  QString southLat = gdalElement.text();
                  map += PvlKeyword("MinimumLatitude", Isis::toString(Angle(southLat).degrees()));
                }
                else if (gdalElement.attribute("name", "") == "NORTH_LATITUDE") {
                  QString northLat = gdalElement.text();
                  map += PvlKeyword("MaximumLatitude", Isis::toString(Angle(northLat).degrees()));
                }
              }
            }

            gdalNode = gdalNode.nextSibling();
          }
        }
      }
    }

    return newLab;
  }


  /**
   * Convert the Tiff Tiepoint tag data to Upper Left X & Y values for the ISIS cube label mapping
   * group
   *
   * @return The ISIS PVL mapping group
   *
   */
  Pvl TiffImporter::upperLeftXY(const Pvl &inLab) const {

    Pvl newLab = inLab;
    PvlGroup &map = newLab.findGroup("Mapping");

    double *tiePoints = NULL;
    short int tieCount = 0;
    if (TIFFGetField(m_image, TIFFTAG_GEOTIEPOINTS, &tieCount, &tiePoints) == 1) {

      // The expected tiepoints are TIFF(i, j, k, x, y, z) = ISIS(sample, line, 0, X, Y, 0)
      // Make sure the (x, y) refer to the (0, 0)
      if (tiePoints[0] == 0.0 && tiePoints[1] == 0.0) {
        double x = 0.0;
        if (map.hasKeyword("FalseEasting")) {
          x = (double)map["FalseEasting"] + tiePoints[3];
          map.deleteKeyword("FalseEasting");
        }

        double y = 0.0;
        if (map.hasKeyword("FalseNorthing")) {
          y = (double)map["FalseNorthing"] + tiePoints[4];
          map.deleteKeyword("FalseNorthing");
        }

        map += PvlKeyword("UpperLeftCornerX", Isis::toString(x), "meters");
        map += PvlKeyword("UpperLeftCornerY", Isis::toString(y), "meters");
      }
      else {
        std::string msg = "The upper left X and Y can not be calculated. Unsupported tiepoint "
                      "type in Tiff file (i.e., not ( 0.0, 0.0))";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    return newLab;
  }


  /**
   * Convert the Tiff PixelScale tag data to a singe resolution for the ISIS cube label mapping
   * group
   *
   * @return The ISIS PVL mapping group
   *
   */
  Pvl TiffImporter::resolution(const Pvl &inLab) const {

    Pvl newLab = inLab;
    PvlGroup &map = newLab.findGroup("Mapping");

    // Get the Tiff PixelScale tag and convert it to resolution
    double *scales = NULL;
    short int scaleCount = 0;
    if (TIFFGetField(m_image, TIFFTAG_GEOPIXELSCALE, &scaleCount, &scales) == 1) {

      // The expected scales are TIFF(x, y, z) = ISIS(sample, line, 0)
      // Make sure the (x, y) are the same but not zero (0) for ISIS
      if ((scaleCount == 3) && (scales[0] > 0.0 && scales[1] > 0.0) && (scales[0] == scales[1])) {
        map += PvlKeyword("PixelResolution", Isis::toString(scales[0]), "meters");
      }
      else {
        std::string msg = "The pixel resolution could not be retrieved from the TIFF file. Unsupported "
                      "PixelScale tag values.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    return newLab;
  }


  /**
   * The number of "samples" (bands in Isis terms) per pixel in the input image.
   * Combined with the photometric interpretation, this can be used to determine
   * the color mode of the input image.  We need both pieces of information
   * because grayscale images are not guaranteed to have only one sample per
   * pixel.
   *
   * @return The samples per pixel
   */
  int TiffImporter::samplesPerPixel() const {
    return m_samplesPerPixel;
  }


  /**
   * Tests to see if the input image has a "min is white" or "min is black"
   * photometric interpretation, implying grayscale (no RGB/A).
   *
   * @return True if the image is grayscale, false otherwise
   */
  bool TiffImporter::isGrayscale() const {
    return
      m_photo == PHOTOMETRIC_MINISWHITE ||
      m_photo == PHOTOMETRIC_MINISBLACK;
  }


  /**
   * Tests to see if the input image is neither grayscale nor has more than
   * three samples per pixel, implying RGB (no alpha).
   *
   * @return True if the image is RGB, false otherwise
   */
  bool TiffImporter::isRgb() const {
    return !isGrayscale() && samplesPerPixel() <= 3;
  }


  /**
   * Tests to see if the input image is not grayscale and has more than three
   * samples per pixel, implying RGBA.
   *
   * @return True if the image is RGBA, false otherwise
   */
  bool TiffImporter::isArgb() const {
    return !isGrayscale() && samplesPerPixel() > 3;
  }


  /**
   * Does nothing as LibTIFF reads the entire input image into memory, and
   * therefore does not need to be updated throughout the import process.
   *
   * @param line Current line of the output buffer
   * @param band Current band of the output buffer
   */
  void TiffImporter::updateRawBuffer(int line, int band) const {
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
  int TiffImporter::getPixel(int s, int l) const {
    l = lines() - l - 1;
    int index = l * samples() + s;
    return m_raster[index];
  }


  /**
   * Retrieves the gray component of the given pixel.  In LibTIFF, even
   * grayscale images are given RGB channels, so converts the RGB components
   * into grayscale regardless of input photometric interpretation.
   *
   * @param pixel Representation of a LibTIFF pixel value
   *
   * @return The gray component
   */
  int TiffImporter::getGray(int pixel) const {
    return convertRgbToGray(pixel);
  }


  /**
   * Retrieves the red component of the given pixel.
   *
   * @param pixel Representation of a LibTIFF pixel value
   *
   * @return The red component
   */
  int TiffImporter::getRed(int pixel) const {
    return TIFFGetR(pixel);
  }


  /**
   * Retrieves the green component of the given pixel.
   *
   * @param pixel Representation of a LibTIFF pixel value
   *
   * @return The green component
   */
  int TiffImporter::getGreen(int pixel) const {
    return TIFFGetG(pixel);
  }


  /**
   * Retrieves the blue component of the given pixel.
   *
   * @param pixel Representation of a LibTIFF pixel value
   *
   * @return The blue component
   */
  int TiffImporter::getBlue(int pixel) const {
    return TIFFGetB(pixel);
  }


  /**
   * Retrieves the alpha component of the given pixel.
   *
   * @param pixel Representation of a LibTIFF pixel value
   *
   * @return The alpha component
   */
  int TiffImporter::getAlpha(int pixel) const {
    return TIFFGetA(pixel);
  }
};
