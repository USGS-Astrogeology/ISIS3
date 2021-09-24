#ifndef TiffImporter_h
#define TiffImporter_h

/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ImageImporter.h"

#include "geotiff.h"
#include "xtiffio.h"
#include "tiffio.h"

#include "PvlGroup.h"

namespace Isis {

  class Pvl;

  /**
   * @brief Imports TIFF images as Isis cubes.
   *
   * Takes a TIFF input image and imports it into Isis in the cube format.
   * Unlike Qt's facilities for importing TIFFs, this class is capable of
   * importing images >2GB, provided the user's machine has sufficient RAM to
   * hold such an image in memory.
   *
   * @ingroup HighLevelCubeIO
   *
   * @author 2012-03-16 Travis Addair
   *
   * @internal
   *   @todo Read chunks of the image into memory at a time, not the entire
   *         image.
   *   @todo Add all ISIS projection to the convertProjection member.
   *
   *   @history 2012-03-28 Travis Addair - Added documentation.
   *   @history 2013-12-11 Stuart Sides - Added new member convterProjection. This is for converting
   *                            GeoTiff projection tags to standard ISIS Mapping lables.
   *
   */
  class TiffImporter : public ImageImporter {
    public:
      TiffImporter(FileName inputName);
      virtual ~TiffImporter();

      virtual PvlGroup convertProjection() const;

      virtual bool isGrayscale() const;
      virtual bool isRgb() const;
      virtual bool isArgb() const;

    protected:
      int samplesPerPixel() const;

      virtual void updateRawBuffer(int line, int band) const;
      virtual int getPixel(int s, int l) const;

      virtual int getGray(int pixel) const;
      virtual int getRed(int pixel) const;
      virtual int getGreen(int pixel) const;
      virtual int getBlue(int pixel) const;
      virtual int getAlpha(int pixel) const;

    private:
      //! LibTIFF representation of the input image.
      TIFF *m_image;

      //! Buffer holding the raw TIFF image in memory.
      uint32_t *m_raster;

      //! The enumerated photometric interpretation of the input image.
      uint16_t m_photo;

      //! The number of "samples" (bands in Isis terms) in the input image.
      uint16_t m_samplesPerPixel;

      //! GeoTiff hanele
      GTIF *m_geotiff;

      //! ISIS Mapping group
      PvlGroup m_map;

      Pvl gdalItems(const Pvl &outPvl) const;
      Pvl upperLeftXY(const Pvl &inLab) const;
      Pvl resolution(const Pvl &inLab) const;

  };
};


#endif
