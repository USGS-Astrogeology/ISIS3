/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProjectionFactory.h"


#include <cfloat>
#include <cmath>
#include <iomanip>

#include "Camera.h"
#include "Cube.h"
#include "Displacement.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "Plugin.h"
#include "Projection.h"
#include "RingPlaneProjection.h"
#include "TProjection.h"

using namespace std;

namespace Isis {
  Plugin ProjectionFactory::m_projPlugin;

  /**
   * This method returns a pointer to a Projection object. The projection is
   * intialized using information contained in a Label object. The information
   * must be a valid Mapping group as defined in the Isis Map Projection Users
   * Guide.
   *
   * @param label The label object containing a valid mapping group.
   *
   * @param allowDefaults If false then the projection class as indicated by the
   *                      ProjectionName keyword will require that projection
   *                      specific parameters such as CenterLatitude,
   *                      CenterLongitude, etc must be in the Pvl label object.
   *                      Otherwise if true then those parameters that are not in
   *                      the Pvl object will be initialized using the
   *                      latitude/longitude range.
   *
   * @return A pointer to a Projection object.
   *
   * @throws Isis::iException::System - Unsupported projection, unable to find
   *                                    plugin
   */
  Isis::Projection *ProjectionFactory::Create(Isis::Pvl &label,
      bool allowDefaults) {
    // Try to load a plugin file in the current working directory and then
    // load the system file
    Plugin p;

    if (m_projPlugin.fileName() == "") {
      FileName localFile("Projection.plugin");
      if (localFile.fileExists())
        m_projPlugin.read(localFile.expanded());

      FileName systemFile("$ISISROOT/lib/Projection.plugin");
      if (systemFile.fileExists())
        m_projPlugin.read(systemFile.expanded());
    }

    try {
      // Look for info in the mapping group
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      QString proj = mapGroup["ProjectionName"];

      // Now get the plugin for the projection
      QFunctionPointer ptr;
      try {
        ptr = m_projPlugin.GetPlugin(proj);
      }
      catch(IException &e) {
        QString msg = "Unsupported projection, unable to find plugin for [" +
                     proj + "]";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      // Now cast that pointer in the proper way
      Isis::TProjection * (*plugin)(Isis::Pvl & label, bool flag);
      // plugin = (Isis::TProjection * ( *)(Isis::Pvl & label, bool flag)) ptr;
      plugin = (Isis::TProjection * ( *)(Isis::Pvl & label, bool flag)) ptr;
      // Create the projection as requested
      return (Isis::Projection *) (*plugin)(label, allowDefaults);
    }
    catch(IException &e) {
      QString message = "Unable to initialize Projection information ";
      message += "from group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }


  /**
   * This method returns a pointer to a RingPlaneProjection object. The projection is
   * intialized using information contained in a Label object. The information
   * must be a valid Mapping group as defined in the Isis Map Projection Users
   * Guide.
   *
   * @param label The label object containing a valid mapping group.
   *
   * @param allowDefaults If false then the projection class as indicated by the
   *                      ProjectionName keyword will require that projection
   *                      specific parameters such as CenterRadius,
   *                      CenterRingLongitude, etc must be in the Pvl label object.
   *                      Otherwise if true then those parameters that are not in
   *                      the Pvl object will be initialized using the
   *                      latitude/longitude range.
   *
   * @return A pointer to a RingPlaneProjection object.
   *
   * @throws Isis::iException::System - Unsupported projection, unable to find
   *                                    plugin
   */
  Isis::Projection *ProjectionFactory::RingsCreate(Isis::Pvl &label,
      bool allowDefaults) {
    // Try to load a plugin file in the current working directory and then
    // load the system file
    Plugin p;

    if (m_projPlugin.fileName() == "") {
      FileName localFile("Projection.plugin");
      if (localFile.fileExists())
        m_projPlugin.read(localFile.expanded());

      FileName systemFile("$ISISROOT/lib/Projection.plugin");
      if (systemFile.fileExists())
        m_projPlugin.read(systemFile.expanded());
    }

    try {
      // Look for info in the mapping group
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      QString proj = mapGroup["ProjectionName"];

      // Now get the plugin for the projection
      QFunctionPointer ptr;
      try {
        ptr = m_projPlugin.GetPlugin(proj);
      }
      catch(IException &e) {
        QString msg = "Unsupported projection, unable to find plugin for [" +
                      proj + "]";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      // Now cast that pointer in the proper way
      Isis::RingPlaneProjection * (*plugin)(Isis::Pvl & label, bool flag);
      plugin = (Isis::RingPlaneProjection * ( *)(Isis::Pvl & label, bool flag)) ptr;
      // Create the projection as requested
      return (Projection *) (*plugin)(label, allowDefaults);
    }
    catch(IException &e) {
      QString message = "Unable to initialize Projection information ";
      message += "from group [Mapping]";
      throw IException(e, IException::Io, message, _FILEINFO_);
    }
  }


  /**
   * This method creates a map projection for a cube given a label.  The label
   * must contain all the proper mapping information (radii, projection name,
   * parameters, pixel resolution, etc).  If the labels contain a Cube group and
   * the Mapping group already has the upper left corner, then the values in the
   * label will be used to set the cube size.  If they don't exist then the
   * minimum/maximum latitude/longitude values (ground range) are expected to be
   * in the Mapping group and will be used to compute the cube size and upper
   * left corner values.
   *
   * @param label A label containing valid map projection information for a
   *              cube.
   * @param samples The number of samples.  This value is calculated in the
   *                method and returned.
   * @param lines The number of lines.  This value is calculated in the method
   *              and returned.
   * @param sizeMatch Boolean value that determines whether the method should
   *                  match the size of the output cube to the size of the input
   *                  cube.  This parameter defaults to true.
   *
   * @return A pointer to a Projection object.
   *
   */
  Isis::Projection *ProjectionFactory::CreateForCube(Isis::Pvl &label,
      int &samples, int &lines,
      bool sizeMatch) {
    // Create a temporary projection and get the radius at the special latitude
    Isis::TProjection *proj = (Isis::TProjection *) Create(label, true);
    double trueScaleLat = proj->TrueScaleLatitude();
    double localRadius = proj->LocalRadius(trueScaleLat);
    delete proj;

    IException errors;
    try {
      // Try to get the pixel resolution and then compute the scale
      double scale, pixelResolution;
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      try {
        pixelResolution = mapGroup["PixelResolution"];
        scale = (2.0 * Isis::PI * localRadius) / (360.0 * pixelResolution);
      }

      // If not get the scale and then compute the pixel resolution
      catch(IException &e) {
        errors.append(e);

        scale = mapGroup["Scale"];
        pixelResolution = (2.0 * Isis::PI * localRadius) / (360.0 * scale);
      }
      // Write out the scale and resolution with units and truescale latitude
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution", toString(pixelResolution), "meters/pixel"),
                          Isis::Pvl::Replace);
      mapGroup.addKeyword(Isis::PvlKeyword("Scale", toString(scale), "pixels/degree"), Isis::Pvl::Replace);
      //mapGroup.addKeyword(Isis::PvlKeyword ("TrueScaleLatitude", trueScaleLat),
      //                                    Isis::Pvl::Replace);

      // Get the upper left corner from the labels if possible
      // This forces an exact match of projection parameters for
      // output cubes
      bool sizeFound = false;
      double upperLeftX = Null, upperLeftY = Null;
      if (label.hasObject("IsisCube")) {
        Isis::PvlGroup &dims = label.findGroup("Dimensions", Isis::Pvl::Traverse);
        samples = dims["Samples"];
        lines = dims["Lines"];

        upperLeftX = mapGroup["UpperLeftCornerX"];
        upperLeftY = mapGroup["UpperLeftCornerY"];
        sizeFound = true;
      }
      if (!sizeMatch) sizeFound = false;

      // Initialize the rest of the projection
      proj = (Isis::TProjection *) Create(label, true);

      // Couldn't find the cube size from the labels so compute it
      if (!sizeFound) {
        if (!proj->HasGroundRange()) {
          std::string msg = "Invalid ground range [MinimumLatitude,MaximumLatitude,";
          msg += "MinimumLongitude,MaximumLongitude] missing or invalid";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        double minX, maxX, minY, maxY;
        if (!proj->XYRange(minX, maxX, minY, maxY)) {
          QString msg = "Invalid ground range [MinimumLatitude,MaximumLatitude,";
          msg += "MinimumLongitude,MaximumLongitude] cause invalid computation ";
          msg += "of image size";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }


        // Convert upperleft coordinate to units of pixel
        // Truncate it to the nearest whole pixel (floor/ceil)
        // Convert it back to meters.  But don't do this if
        // the X/Y position is already close to a whole pixel because
        // the floor/ceil function could cause an extra pixel to be added
        // just due to machine precision issues

        bool flipX = false;
        bool flipY = false;

        double minXFlipped = minX;
        double maxXFlipped = maxX;
        double minYFlipped = minY;
        double maxYFlipped = maxY;

        //New range is (-1)*[0,maxX] = [-maxX, 0]

        if (minX == 0)  {
          minXFlipped = -maxX;
          maxXFlipped = 0;
          flipX = true;

        }

          if (fabs(fmod(minXFlipped, pixelResolution)) > 1.0e-6) {
            if (pixelResolution - fabs(fmod(minXFlipped, pixelResolution)) > 1.0e-6) {            
              double sampleOffset = floor(minXFlipped / pixelResolution);

              minXFlipped = sampleOffset * pixelResolution;

            }
          }


        // make sure that the distance from minX to maxX is at least one pixel wide
        // so we have at least one sample in the created cube


        if (maxXFlipped < minXFlipped + pixelResolution) {
          maxXFlipped = minXFlipped + pixelResolution;
        }


        //New range is (-1)*[minY,0] = [0,-minY]
        if (maxY == 0) {
          maxYFlipped = -minY;
          minYFlipped = 0;
          flipY = true;

        }

        if (fabs(fmod(maxYFlipped, pixelResolution)) > 1.0e-6) {
          if (abs(pixelResolution - fabs(fmod(maxYFlipped, pixelResolution))) > 1.0e-6) {
            double lineOffset = ceil(maxYFlipped / pixelResolution);        
            maxYFlipped =  lineOffset * pixelResolution;

          }
        }
        

        // make sure that the distance from minY to maxY is at least one pixel wide
        // so we have at least one line in the created cube
        if (minYFlipped > maxYFlipped - pixelResolution) {
          minYFlipped = maxYFlipped - pixelResolution;
        }

        // Determine the number of samples and lines

         samples = (int)((maxXFlipped - minXFlipped) / pixelResolution + 0.5);
         lines = (int)((maxYFlipped - minYFlipped) / pixelResolution + 0.5);


        // Set the upper left corner and add to the labels
        if (flipX) {
         upperLeftX = 0;

        }
        else {
            upperLeftX = minXFlipped;
        }
        mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX", toString(upperLeftX)),
                            Isis::Pvl::Replace);

        if (flipY)  {
          upperLeftY = 0;

        }
        else {
           upperLeftY = maxYFlipped;

        }

        mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY", toString(upperLeftY)),
                            Isis::Pvl::Replace);

        // Write it in pixel units as well
#if 0
        lineOffset += 0.5;    // This matches the PDS definition
        sampleOffset += 0.5;  // of the offsets (center of pixel). This statement is questionable!
        mapGroup.addKeyword(Isis::PvlKeyword("LineProjectionOffset", lineOffset),
                            Isis::Pvl::Replace);
        mapGroup.addKeyword(Isis::PvlKeyword("SampleProjectionOffset", sampleOffset),
                            Isis::Pvl::Replace);
#endif
      }


      // Make sure labels have good units
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution",
                                           (QString) mapGroup["PixelResolution"],
                                           "meters/pixel"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("Scale",
                                           (QString) mapGroup["Scale"],
                                           "pixels/degree"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX",
                                           (QString) mapGroup["UpperLeftCornerX"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY",
                                           (QString) mapGroup["UpperLeftCornerY"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("EquatorialRadius",
                                           (QString) mapGroup["EquatorialRadius"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("PolarRadius",
                                           (QString) mapGroup["PolarRadius"],
                                           "meters"), Isis::Pvl::Replace);

      // Add the mapper from pixel coordinates to projection coordinates
      PFPixelMapper *pixelMapper = new PFPixelMapper(pixelResolution, upperLeftX, upperLeftY);
      proj->SetWorldMapper(pixelMapper);

      proj->SetUpperLeftCorner(Displacement(upperLeftX, Displacement::Meters),
                               Displacement(upperLeftY, Displacement::Meters));
    }
    catch(IException &e) {
      QString msg = "Unable to create projection";
      if (label.fileName() != "") msg += " from file [" + label.fileName() + "]";
      IException finalError(IException::Unknown, msg, _FILEINFO_);
      finalError.append(errors);
      finalError.append(e);
      throw finalError;
    }
    return (Isis::Projection *) proj;
  }


  /**
   * This method creates a projection for a cube to a ring plane given a label.
   * Currently this is utilized only for projecting images of rings to the ring
   * plane (i.e the equatorial plane). The label must contain all the proper
   * mapping information (radii, projection name, parameters, pixel resolution,
   * etc). If the labels contain a Cube group and the Mapping group already has
   * the upper left corner, then the values in the label will be used to set the
   * cube size.  If they don't exist then the minimum/maximum latitude/longitude
   * values (ground range) are expected to be in the Mapping group and will be
   * used to compute the cube size and upper left corner values.
   *
   * @param label A label containing valid map projection information for a
   *              cube.
   * @param samples The number of samples. This value is calculated in the
   *                method and returned.
   * @param lines The number of lines. This value is calculated in the method
   *              and returned.
   * @param sizeMatch Boolean value specifying whether the method should match
   *                  the size of the output cube to the size of the input cube.
   *                  Defaults to true.
   *
   * @return @b Projection* A pointer to a Projection object.
   *
   */
  Isis::Projection *ProjectionFactory::RingsCreateForCube(Isis::Pvl &label,
      int &samples, int &lines, bool sizeMatch) {

    // Create a temporary projection and get the radius at the special radius
    // NOTE: by "special radius" we mean that radius where the projection is
    // not distorted
    Isis::RingPlaneProjection *proj = (Isis::RingPlaneProjection *) RingsCreate(label, true);
    double localRadius = proj->TrueScaleRingRadius();
    delete proj;

    IException errors;
    try {
      // Try to get the pixel resolution and then compute the scale
      double scale, pixelResolution;
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      try {
        pixelResolution = mapGroup["PixelResolution"];
        scale = (2.0 * Isis::PI * localRadius) / (360.0 * pixelResolution);
      }
      catch(IException &e) {
        // If this fails, use the scale to compute the pixel resolution
        errors.append(e);

        scale = mapGroup["Scale"];
        pixelResolution = (2.0 * Isis::PI * localRadius) / (360.0 * scale);
      }

      // Write out the scale and resolution with units and truescale radius
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution", toString(pixelResolution),
                                           "meters/pixel"),
                          Isis::Pvl::Replace);
      mapGroup.addKeyword(Isis::PvlKeyword("Scale", toString(scale), "pixels/degree"),
                          Isis::Pvl::Replace);

      //mapGroup.AddKeyword(Isis::PvlKeyword ("TrueScaleRadius", trueScaleRadius),
      //                                    Isis::Pvl::Replace);

      // Get the upper left corner from the labels if possible
      // This forces an exact match of projection parameters for
      // output cubes
      bool sizeFound = false;
      double upperLeftX = Null, upperLeftY = Null;
      if (label.hasObject("IsisCube")) {
        Isis::PvlGroup &dims = label.findGroup("Dimensions", Isis::Pvl::Traverse);
        samples = dims["Samples"];
        lines = dims["Lines"];

        upperLeftX = mapGroup["UpperLeftCornerX"];
        upperLeftY = mapGroup["UpperLeftCornerY"];
        sizeFound = true;
      }

      if (!sizeMatch) {
        sizeFound = false;
      }

      // Initialize the rest of the projection
      proj = (Isis::RingPlaneProjection *) RingsCreate(label, true);

      // Couldn't find the cube size from the labels so compute it
      if (!sizeFound) {
        if (!proj->HasGroundRange()) {
          std::string msg = "Invalid ring range [MinimumRingRadius,MaximumRingRadius,";
          msg += "MinimumRingLongitude,MaximumRingLongitude] missing or invalid";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        double minX, maxX, minY, maxY;
        if (!proj->XYRange(minX, maxX, minY, maxY)) {
          std::string msg = "Invalid ring range [MinimumRingRadius,MaximumRingRadius,";
          msg += "MinimumRingLongitude,MaximumRingLongitude] cause invalid computation ";
          msg += "of image size";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // Convert upperleft coordinate to pixel units
        // Truncate it to the nearest whole pixel (floor/ceil)
        // Convert it back to meters. But don't do this if
        // the X/Y position is already close to a whole pixel because
        // the floor/ceil function could cause an extra pixel to be added
        // just due to machine precision issues
        if (fabs(fmod(minX, pixelResolution)) > 1.0e-6) {
          if (pixelResolution - fabs(fmod(minX, pixelResolution)) > 1.0e-6) {
            double sampleOffset = floor(minX / pixelResolution);
            minX = sampleOffset * pixelResolution;
          }
        }
        // make sure that the distance from minX to maxX is at least one pixel wide
        // so we have at least one sample in the created cube
        if (maxX < minX + pixelResolution) {
          maxX = minX + pixelResolution;
        }
        if (fabs(fmod(maxY, pixelResolution)) > 1.0e-6) {
          if (pixelResolution - fabs(fmod(maxY, pixelResolution)) > 1.0e-6) {
            double lineOffset = -1.0 * ceil(maxY / pixelResolution);
            maxY = -1.0 * lineOffset * pixelResolution;
          }
        }
        // make sure that the distance from minY to maxY is at least one pixel wide
        // so we have at least one line in the created cube
        if (minY > maxY - pixelResolution) {
          minY = maxY - pixelResolution;
        }

        // Determine the number of samples and lines
        samples = (int)((maxX - minX) / pixelResolution + 0.5);
        lines = (int)((maxY - minY) / pixelResolution + 0.5);

        // Set the upper left corner and add to the labels
        upperLeftX = minX;
        mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX", toString(upperLeftX)),
                            Isis::Pvl::Replace);

        upperLeftY = maxY;
        mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY", toString(upperLeftY)),
                            Isis::Pvl::Replace);

        // Write it in pixel units as well
#if 0
        lineOffset += 0.5;    // This matches the PDS definition
        sampleOffset += 0.5;  // of the offsets (center of pixel). This statement is questionable!
        mapGroup.AddKeyword(Isis::PvlKeyword("LineProjectionOffset", lineOffset),
                            Isis::Pvl::Replace);
        mapGroup.AddKeyword(Isis::PvlKeyword("SampleProjectionOffset", sampleOffset),
                            Isis::Pvl::Replace);
#endif
      }


      // Make sure labels have good units
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution",
                                           (QString) mapGroup["PixelResolution"],
                                           "meters/pixel"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("Scale",
                                           (QString) mapGroup["Scale"],
                                           "pixels/degree"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX",
                                           (QString) mapGroup["UpperLeftCornerX"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY",
                                           (QString) mapGroup["UpperLeftCornerY"],
                                           "meters"), Isis::Pvl::Replace);

      // Add the mapper from pixel coordinates to projection coordinates
      PFPixelMapper *pixelMapper = new PFPixelMapper(pixelResolution, upperLeftX, upperLeftY);
      proj->SetWorldMapper(pixelMapper);

      proj->SetUpperLeftCorner(Displacement(upperLeftX, Displacement::Meters),
                               Displacement(upperLeftY, Displacement::Meters));
    }
    catch(IException &e) {
      QString msg = "Unable to create projection";
      if (label.fileName() != "") msg += " from file [" + label.fileName() + "]";
      IException finalError(IException::Unknown, msg, _FILEINFO_);
      finalError.append(errors);
      finalError.append(e);
      throw finalError;
    }
    return (Isis::Projection *) proj;
  }


  /**
   * @brief Create a map projection group for a cube using a
   * camera.
   *
   * This method walks the boundary of the cube computing lat/lons
   * and then uses those lat/lon as input to the projection to
   * compute a x/y range.  This x/y range will be minimal
   * (compared to the alternate CreateForCube method and generates
   * significantly small cube size (samples,lines) depending on
   * the projection.  Projections with curved meridians and/or
   * parallels generate larger x/y ranges when only looking at the
   * ground range.
   *
   * @param label A label containing valid map projection information for a
   *              cube.
   * @param samples The number of samples.  This value is calculated in the
   *                method and returned.
   * @param lines The number of lines.  This value is calculated in the method
   *              and returned.
   * @param cam An initialized camera model
   *
   * @return @b Projection* A pointer to a Projection object.
   *
   */
  Isis::Projection *ProjectionFactory::CreateForCube(Isis::Pvl &label,
      int &samples, int &lines,
      Camera &cam) {
    // Create a temporary projection and get the radius at the special latitude
    Isis::TProjection *proj = (Isis::TProjection *) Create(label, true);
    double trueScaleLat = proj->TrueScaleLatitude();
    double localRadius = proj->LocalRadius(trueScaleLat);
    delete proj;

    try {
      // Try to get the pixel resolution and then compute the scale
      double scale, pixelResolution;
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      try {
        pixelResolution = mapGroup["PixelResolution"];
        scale = (2.0 * Isis::PI * localRadius) / (360.0 * pixelResolution);
      }

      // If not get the scale and then compute the pixel resolution
      catch(IException &) {
        scale = mapGroup["Scale"];
        pixelResolution = (2.0 * Isis::PI * localRadius) / (360.0 * scale);
      }
      // Write out the scale and resolution with units and truescale latitude
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution", toString(pixelResolution), "meters/pixel"),
                          Isis::Pvl::Replace);
      mapGroup.addKeyword(Isis::PvlKeyword("Scale", toString(scale), "pixels/degree"), Isis::Pvl::Replace);
      //mapGroup.AddKeyword(Isis::PvlKeyword ("TrueScaleLatitude", trueScaleLatitude),
      //                                    Isis::Pvl::Replace);

      // Initialize the rest of the projection
      proj = (Isis::TProjection *) Create(label, true);
      double minX = DBL_MAX;
      double maxX = -DBL_MAX;
      double minY = DBL_MAX;
      double maxY = -DBL_MAX;

      // Walk the boundaries of the camera to determine the x/y range
      int eband = cam.Bands();
      if (cam.IsBandIndependent()) eband = 1;
      for(int band = 1; band <= eband; band++) {
        cam.SetBand(band);

        // Loop for each line testing the left and right sides of the image
        for(int line = 0; line <= cam.Lines(); line++) {
          // Look for the first good lat/lon on the left edge of the image
          // If it is the first or last line then test the whole line
          int samp;
          for(samp = 0; samp <= cam.Samples(); samp++) {
            if (cam.SetImage((double)samp + 0.5, (double)line + 0.5)) {
              double lat = cam.UniversalLatitude();
              double lon = cam.UniversalLongitude();
              proj->SetUniversalGround(lat, lon);
              if (proj->IsGood()) {
                if (proj->XCoord() < minX) minX = proj->XCoord();
                if (proj->XCoord() > maxX) maxX = proj->XCoord();
                if (proj->YCoord() < minY) minY = proj->YCoord();
                if (proj->YCoord() > maxY) maxY = proj->YCoord();
                if ((line != 0) && (line != cam.Lines())) break;
              }
            }
          }

          // Look for the first good lat/lon on the right edge of the image
          if (samp < cam.Samples()) {
            for(samp = cam.Samples(); samp >= 0; samp--) {
              if (cam.SetImage((double)samp + 0.5, (double)line + 0.5)) {
                double lat = cam.UniversalLatitude();
                double lon = cam.UniversalLongitude();
                proj->SetUniversalGround(lat, lon);
                if (proj->IsGood()) {
                  if (proj->XCoord() < minX) minX = proj->XCoord();
                  if (proj->XCoord() > maxX) maxX = proj->XCoord();
                  if (proj->YCoord() < minY) minY = proj->YCoord();
                  if (proj->YCoord() > maxY) maxY = proj->YCoord();
                  break;
                }
              }
            }
          }
        }

        // Special test for ground range to see if either pole is in the image
        if (cam.SetUniversalGround(90.0, 0.0)) {
          if (cam.Sample() >= 0.5 && cam.Line() >= 0.5 &&
              cam.Sample() <= cam.Samples() + 0.5 && cam.Line() <= cam.Lines() + 0.5) {
            double lat = cam.UniversalLatitude();
            double lon = cam.UniversalLongitude();
            proj->SetUniversalGround(lat, lon);
            if (proj->IsGood()) {
              if (proj->XCoord() < minX) minX = proj->XCoord();
              if (proj->XCoord() > maxX) maxX = proj->XCoord();
              if (proj->YCoord() < minY) minY = proj->YCoord();
              if (proj->YCoord() > maxY) maxY = proj->YCoord();
            }
          }
        }

        if (cam.SetUniversalGround(-90.0, 0.0)) {
          if (cam.Sample() >= 0.5 && cam.Line() >= 0.5 &&
              cam.Sample() <= cam.Samples() + 0.5 && cam.Line() <= cam.Lines() + 0.5) {
            double lat = cam.UniversalLatitude();
            double lon = cam.UniversalLongitude();
            proj->SetUniversalGround(lat, lon);
            if (proj->IsGood()) {
              if (proj->XCoord() < minX) minX = proj->XCoord();
              if (proj->XCoord() > maxX) maxX = proj->XCoord();
              if (proj->YCoord() < minY) minY = proj->YCoord();
              if (proj->YCoord() > maxY) maxY = proj->YCoord();
            }
          }
        }

#if 0
        // Another special test for ground range as we could have the
        // 0-360 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        for(double lat = p_minlat; lat <= p_maxlat; lat += (p_maxlat - p_minlat) / 10.0) {
          if (SetUniversalGround(lat, 0.0)) {
            if (Sample() >= 0.5 && Line() >= 0.5 &&
                Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
              p_minlon = 0.0;
              p_maxlon = 360.0;
              break;
            }
          }
        }

        // Another special test for ground range as we could have the
        // -180-180 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        for(double lat = p_minlat; lat <= p_maxlat; lat += (p_maxlat - p_minlat) / 10.0) {
          if (SetUniversalGround(lat, 180.0)) {
            if (Sample() >= 0.5 && Line() >= 0.5 &&
                Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
              p_minlon180 = -180.0;
              p_maxlon180 = 180.0;
              break;
            }
          }
        }
#endif
      }

      // Convert upperleft coordinate to units of pixel
      // Truncate it to the nearest whole pixel (floor/ceil)
      // Convert it back to meters.  But don't do this if
      // the X/Y position is already close to a whole pixel because
      // the floor/ceil function could cause an extra pixel to be added
      // just due to machine precision issues
      if (fabs(fmod(minX, pixelResolution)) > 1.0e-6) {
        if (pixelResolution - fabs(fmod(minX, pixelResolution)) > 1.0e-6) {
          double sampleOffset = floor(minX / pixelResolution);
          minX = sampleOffset * pixelResolution;
        }
      }
      // make sure that the distance from minX to maxX is at least one pixel wide
      // so we have at least one sample in the created cube
      if (maxX < minX + pixelResolution) {
        maxX = minX + pixelResolution;
      }
      if (fabs(fmod(maxY, pixelResolution)) > 1.0e-6) {
        if (pixelResolution - fabs(fmod(maxY, pixelResolution)) > 1.0e-6) {
          double lineOffset = -1.0 * ceil(maxY / pixelResolution);
          maxY = -1.0 * lineOffset * pixelResolution;
        }
      }
      // make sure that the distance from minY to maxY is at least one pixel wide
      // so we have at least one line in the created cube
      if (minY > maxY - pixelResolution) {
        minY = maxY - pixelResolution;
      }

      // Determine the number of samples and lines
      samples = (int)((maxX - minX) / pixelResolution + 0.5);
      lines = (int)((maxY - minY) / pixelResolution + 0.5);

      // Set the upper left corner and add to the labels
      double upperLeftX = minX;
      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX", toString(upperLeftX)),
                          Isis::Pvl::Replace);

      double upperLeftY = maxY;
      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY", toString(upperLeftY)),
                          Isis::Pvl::Replace);

      // Make sure labels have good units
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution",
                                           (QString) mapGroup["PixelResolution"],
                                           "meters/pixel"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("Scale",
                                           (QString) mapGroup["Scale"],
                                           "pixels/degree"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX",
                                           (QString) mapGroup["UpperLeftCornerX"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY",
                                           (QString) mapGroup["UpperLeftCornerY"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("EquatorialRadius",
                                           (QString) mapGroup["EquatorialRadius"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("PolarRadius",
                                           (QString) mapGroup["PolarRadius"],
                                           "meters"), Isis::Pvl::Replace);

      // Add the mapper from pixel coordinates to projection coordinates
      PFPixelMapper *pixelMapper = new PFPixelMapper(pixelResolution, upperLeftX, upperLeftY);
      proj->SetWorldMapper(pixelMapper);

      proj->SetUpperLeftCorner(Displacement(upperLeftX, Displacement::Meters),
                               Displacement(upperLeftY, Displacement::Meters));
    }
    catch(IException &e) {
      QString msg = "Unable to create projection";
      if (label.fileName() != "") msg += " from file [" + label.fileName() + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    return (Isis::Projection *) proj;
  }


  /**
   * @brief Create a ring plane map projection group for a cube using a
   * camera.
   *
   * This method walks the boundary of the cube computing lat/lons
   * and then uses those lat/lon as input to the projection to
   * compute a x/y range.  This x/y range will be minimal
   * (compared to the alternate CreateForCube method and generates
   * significantly small cube size (samples,lines) depending on
   * the projection.  Projections with curved meridians and/or
   * parallels generate larger x/y ranges when only looking at the
   * ground range.
   *
   * @param label A label containing valid map projection information for a
   *              cube.
   * @param samples The number of samples.  This value is calculated in the
   *                method and returned.
   * @param lines The number of lines.  This value is calculated in the method
   *              and returned.
   * @param cam An initialized camera model
   *
   * @return Projection* A pointer to a Projection object.
   *
   */
  Isis::Projection *ProjectionFactory::RingsCreateForCube(Isis::Pvl &label,
      int &samples, int &lines, Camera &cam) {

    // Create a temporary projection
    Isis::RingPlaneProjection *proj = (Isis::RingPlaneProjection *) RingsCreate(label, true);
    double localRadius = proj->TrueScaleRingRadius();
    delete proj;

    IException errors;
    try {
      // Try to get the pixel resolution and then compute the scale
      double scale, pixelResolution;
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      try {
        pixelResolution = mapGroup["PixelResolution"];
        scale = (2.0 * Isis::PI * localRadius) / (360.0 * pixelResolution);
        // scale = proj->Scale();
      }

      // If not get the scale and then compute the pixel resolution
      catch(IException &e) {
        errors.append(e);
        scale = mapGroup["Scale"];
        pixelResolution = (2.0 * Isis::PI * localRadius) / (360.0 * scale);
      }
      // Write out the scale and resolution with units and truescale radius
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution", toString(pixelResolution),
                                           "meters/pixel"), Isis::Pvl::Replace);
      mapGroup.addKeyword(Isis::PvlKeyword("Scale", toString(scale), "pixels/degree"),
                          Isis::Pvl::Replace);

      // Initialize the rest of the projection
      proj = (Isis::RingPlaneProjection *) RingsCreate(label, true);
      double minX = DBL_MAX;
      double maxX = -DBL_MAX;
      double minY = DBL_MAX;
      double maxY = -DBL_MAX;

      // Walk the boundaries of the camera to determine the x/y range
      int eband = cam.Bands();
      if (cam.IsBandIndependent()) eband = 1;
      for(int band = 1; band <= eband; band++) {
        cam.SetBand(band);

        // Loop for each line testing the left and right sides of the image
        for(int line = 0; line <= cam.Lines(); line++) {
          // Look for the first good rad/az on the left edge of the image
          // If it is the first or last line then test the whole line
          int samp;
          for(samp = 0; samp <= cam.Samples(); samp++) {
            if (cam.SetImage((double)samp + 0.5, (double)line + 0.5)) {
              double radius = cam.LocalRadius().meters();
              double az = cam.UniversalLongitude();
              proj->SetGround(radius, az);
              if (proj->IsGood()) {
                if (proj->XCoord() < minX) minX = proj->XCoord();
                if (proj->XCoord() > maxX) maxX = proj->XCoord();
                if (proj->YCoord() < minY) minY = proj->YCoord();
                if (proj->YCoord() > maxY) maxY = proj->YCoord();
                if ((line != 0) && (line != cam.Lines())) break;
              }
            }
          }

          // Look for the first good rad/az on the right edge of the image
          if (samp < cam.Samples()) {
            for(samp = cam.Samples(); samp >= 0; samp--) {
              if (cam.SetImage((double)samp + 0.5, (double)line + 0.5)) {
                double radius = cam.LocalRadius().meters();
                double az = cam.UniversalLongitude();
                proj->SetGround(radius, az);
                if (proj->IsGood()) {
                  if (proj->XCoord() < minX) minX = proj->XCoord();
                  if (proj->XCoord() > maxX) maxX = proj->XCoord();
                  if (proj->YCoord() < minY) minY = proj->YCoord();
                  if (proj->YCoord() > maxY) maxY = proj->YCoord();
                  break;
                }
              }
            }
          }
        }

#if 0
        // Another special test for ground range as we could have the
        // 0-360 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        for(double rad = p_minRadius; rad <= p_maxRadius; rad += (p_maxRadius - p_minRadius) / 10.0) {
          if (SetUniversalGround(rad, 0.0)) {
            if (Sample() >= 0.5 && Line() >= 0.5 &&
                Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
              p_minaz = 0.0;
              p_maxaz = 360.0;
              break;
            }
          }
        }

        // Another special test for ground range as we could have the
        // -180-180 seam running right through the image so
        // test it as well (the increment may not be fine enough !!!)
        for(double rad = p_minrad; rad <= p_maxrad; rad += (p_maxrad - p_minrad) / 10.0) {
          if (SetUniversalGround(rad, 180.0)) {
            if (Sample() >= 0.5 && Line() >= 0.5 &&
                Sample() <= p_samples + 0.5 && Line() <= p_lines + 0.5) {
              p_minaz180 = -180.0;
              p_maxaz180 = 180.0;
              break;
            }
          }
        }
#endif
      }

      // Convert upperleft coordinate to units of pixel
      // Truncate it to the nearest whole pixel (floor/ceil)
      // Convert it back to meters.  But don't do this if
      // the X/Y position is already close to a whole pixel because
      // the floor/ceil function could cause an extra pixel to be added
      // just due to machine precision issues
      if (fabs(fmod(minX, pixelResolution)) > 1.0e-6) {
        if (pixelResolution - fabs(fmod(minX, pixelResolution)) > 1.0e-6) {
          double sampleOffset = floor(minX / pixelResolution);
          minX = sampleOffset * pixelResolution;
        }
      }
      // make sure that the distance from minX to maxX is at least one pixel wide
      // so we have at least one sample in the created cube
      if (maxX < minX + pixelResolution) {
        maxX = minX + pixelResolution;
      }

      if (fabs(fmod(maxY, pixelResolution)) > 1.0e-6) {
        if (pixelResolution - fabs(fmod(maxY, pixelResolution)) > 1.0e-6) {
          double lineOffset = -1.0 * ceil(maxY / pixelResolution);
          maxY = -1.0 * lineOffset * pixelResolution;
        }
      }
      // make sure that the distance from minY to maxY is at least one pixel wide
      // so we have at least one line in the created cube
      if (minY > maxY - pixelResolution) {
        minY = maxY - pixelResolution;
      }

      // Determine the number of samples and lines
      samples = (int)((maxX - minX) / pixelResolution + 0.5);
      lines = (int)((maxY - minY) / pixelResolution + 0.5);

      // Set the upper left corner and add to the labels
      double upperLeftX = minX;
      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX", toString(upperLeftX)),
                          Isis::Pvl::Replace);

      double upperLeftY = maxY;
      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY", toString(upperLeftY)),
                          Isis::Pvl::Replace);

      // Make sure labels have good units
      mapGroup.addKeyword(Isis::PvlKeyword("PixelResolution",
                                           (QString) mapGroup["PixelResolution"],
                                           "meters/pixel"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("Scale",
                                           (QString) mapGroup["Scale"],
                                           "pixels/degree"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerX",
                                           (QString) mapGroup["UpperLeftCornerX"],
                                           "meters"), Isis::Pvl::Replace);

      mapGroup.addKeyword(Isis::PvlKeyword("UpperLeftCornerY",
                                           (QString) mapGroup["UpperLeftCornerY"],
                                           "meters"), Isis::Pvl::Replace);

      // Add the mapper from pixel coordinates to projection coordinates
      PFPixelMapper *pixelMapper = new PFPixelMapper(pixelResolution, upperLeftX, upperLeftY);
      proj->SetWorldMapper(pixelMapper);

      proj->SetUpperLeftCorner(Displacement(upperLeftX, Displacement::Meters),
                               Displacement(upperLeftY, Displacement::Meters));
    }
    catch(IException &e) {
      QString msg = "Unable to create projection";
      if (label.fileName() != "") msg += " from file [" + label.fileName() + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    return (Isis::Projection *) proj;
  }


  /**
   * This method is a helper method. See CreateFromCube(Pvl).
   *
   * @param cube A cube containing valid map projection information
   *
   * @return A pointer to a Projection object
   */
  Isis::Projection *ProjectionFactory::CreateFromCube(Isis::Cube &cube) {
    return CreateFromCube(*cube.label());
  }


  /**
   * This method is a helper method. See RingsCreateFromCube(Pvl).
   *
   * @param cube A cube containing valid map projection information
   *
   * @return A pointer to a Projection object
   */
  Isis::Projection *ProjectionFactory::RingsCreateFromCube(Isis::Cube &cube) {
    return RingsCreateFromCube(*cube.label());
  }


  /**
   * This method loads a map projection from a cube returning a pointer to a
   * Projection object.
   *
   * @param label A label containing valid map projection information for a
   * cube.
   *
   * @return (Isis::Projection) A pointer to a Projection object.
   */
  Isis::Projection *ProjectionFactory::CreateFromCube(Isis::Pvl &label) {
    Isis::TProjection *proj;
    try {
      // Get the pixel resolution
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      double pixelResolution = mapGroup["PixelResolution"];

      // Get the upper left corner
      double upperLeftX = mapGroup["UpperLeftCornerX"];
      double upperLeftY = mapGroup["UpperLeftCornerY"];

      // Initialize the rest of the projection
      proj = (Isis::TProjection *) Create(label, true);

      // Create a mapper to transform pixels into projection x/y and vice versa
      PFPixelMapper *pixelMapper = new PFPixelMapper(pixelResolution, upperLeftX, upperLeftY);
      proj->SetWorldMapper(pixelMapper);

      proj->SetUpperLeftCorner(Displacement(upperLeftX, Displacement::Meters),
                               Displacement(upperLeftY, Displacement::Meters));
    }
    catch (IException &e) {
      QString msg = "Unable to initialize cube projection";
      if (label.fileName() != "") msg += " from file [" + label.fileName() + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    return (Isis::Projection *) proj;
  }


  /**
   * This method loads a map projection from a cube returning a pointer to a
   * Projection object.
   *
   * @param label A label containing valid map projection information for a
   * cube.
   *
   * @return (Isis::Projection) A pointer to a Projection object.
   */
  Isis::Projection *ProjectionFactory::RingsCreateFromCube(Isis::Pvl &label) {
    Isis::RingPlaneProjection *proj;
    try {
      // Get the pixel resolution
      Isis::PvlGroup &mapGroup = label.findGroup("Mapping", Isis::Pvl::Traverse);
      double pixelResolution = mapGroup["PixelResolution"];

      // Get the upper left corner
      double upperLeftX = mapGroup["UpperLeftCornerX"];
      double upperLeftY = mapGroup["UpperLeftCornerY"];

      // Initialize the rest of the projection
      proj = (Isis::RingPlaneProjection *) RingsCreate(label, true);

      // Create a mapper to transform pixels into projection x/y and vice versa
      PFPixelMapper *pixelMapper = new PFPixelMapper(pixelResolution, upperLeftX, upperLeftY);
      proj->SetWorldMapper(pixelMapper);

      proj->SetUpperLeftCorner(Displacement(upperLeftX, Displacement::Meters),
                               Displacement(upperLeftY, Displacement::Meters));
    }
    catch (IException &e) {
      QString msg = "Unable to initialize cube projection";
      if (label.fileName() != "") msg += " from file [" + label.fileName() + "]";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
    return (Isis::Projection *) proj;
  }
} //end namespace isis
