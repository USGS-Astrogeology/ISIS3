/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessMapMosaic.h"

#include <QTime>
#include <QDebug>

#include "Application.h"
#include "Displacement.h"
#include "IException.h"
#include "ProcessByLine.h"
#include "Preference.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "RingPlaneProjection.h"
#include "SpecialPixel.h"
#include "TProjection.h"
#include "UniqueIOCachingAlgorithm.h"

using namespace std;


namespace Isis {
  //! Constructs a Mosaic object
  ProcessMapMosaic::ProcessMapMosaic() {
    p_createMosaic = true;
  }


  //! Destructor
  ProcessMapMosaic::~ProcessMapMosaic() { }


 /**
  * Input cube cannot be set here
  */
  Isis::Cube *ProcessMapMosaic::SetInputCube() {
    throw IException(IException::Programmer,
                     "ProcessMapMosaic does not support the SetInputCube method",
                     _FILEINFO_);
  }


  /**
   * Mosaic Processing method, returns false if the cube is not inside the mosaic
   */
  bool ProcessMapMosaic::StartProcess(QString inputFile) {
    if (InputCubes.size() != 0) {
      QString msg = "Input cubes already exist; do not call SetInputCube when using ";
      msg += "ProcessMosaic::StartProcess(QString)";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (OutputCubes.size() == 0) {
      QString msg = "An output cube must be set before calling StartProcess";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    CubeAttributeInput inAtt(inputFile);
    Cube *inCube = ProcessMosaic::SetInputCube(inputFile, inAtt);
    inCube->addCachingAlgorithm(new UniqueIOCachingAlgorithm(2));

    Cube *mosaicCube = OutputCubes[0];
    Projection *iproj = inCube->projection();
    Projection *oproj = mosaicCube->projection();
    int nsMosaic = mosaicCube->sampleCount();
    int nlMosaic = mosaicCube->lineCount();

    if (*iproj != *oproj) {
      QString msg = "Mapping groups do not match between cube [" + inputFile + "] and mosaic";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    int outSample, outSampleEnd, outLine, outLineEnd;
    
    
    if (oproj->ToWorldX(iproj->ToProjectionX(1.0)) < 0) {
      outSample = (int)(oproj->ToWorldX(iproj->ToProjectionX(1.0)) - 0.5);
    }
    else {
      outSample = (int)(oproj->ToWorldX(iproj->ToProjectionX(1.0)) + 0.5);
    }
    if (oproj->ToWorldY(iproj->ToProjectionY(1.0)) < 0) {
      outLine   = (int)(oproj->ToWorldY(iproj->ToProjectionY(1.0)) - 0.5);
    }
    else {
      outLine   = (int)(oproj->ToWorldY(iproj->ToProjectionY(1.0)) + 0.5);
    }
    
    int ins = InputCubes[0]->sampleCount();
    int inl =  InputCubes[0]->lineCount();
    outSampleEnd = outSample + ins;
    outLineEnd   = outLine + inl;

    bool wrapPossible = iproj->IsEquatorialCylindrical();
    int worldSize = 0;
    if (wrapPossible) {
      // Figure out how many samples 360 degrees is
      wrapPossible = wrapPossible && oproj->SetUniversalGround(0, 0);
      int worldStart = (int)(oproj->WorldX() + 0.5);
      wrapPossible = wrapPossible && oproj->SetUniversalGround(0, 180);
      int worldEnd = (int)(oproj->WorldX() + 0.5);

      worldSize = abs(worldEnd - worldStart) * 2;

      wrapPossible = wrapPossible && (worldSize > 0);

      // This is EquatorialCylindrical, so shift to the left all the way
      if (wrapPossible) {
        // While some data would still be put in the mosaic, move left
        //  >1 for end because 0 still means no data, whereas 1 means 1 line of data
        while (outSampleEnd - worldSize > 1) {
          outSample -= worldSize;
          outSampleEnd -= worldSize;
        }
        // Now we have the sample range to the furthest left
      }
    }

    // Check overlaps of input image along the mosaic edges before
    // calling ProcessMosaic::StartProcess
    // Left edge
    if (outSample < 1) {
      ins = ins + outSample - 1;
    }

    // Top edge
    if (outLine < 1) {
      inl = inl + outLine - 1;
    }

    // Right edge
    if ((outSample + ins - 1) > nsMosaic) {
      ins = nsMosaic - outSample + 1;
    }

    // Bottom edge
    if ((outLine + inl - 1) > nlMosaic) {
      inl = nlMosaic - outLine + 1;
    }

    if (outSampleEnd < 1 || outLineEnd < 1 || outSample > nsMosaic || outLine > nlMosaic || ins < 1 || inl < 1) {
      // Add a PvlKeyword naming which files are not included in output mosaic
      ClearInputCubes();
      return false;
    }
    else {
      // Place the input in the mosaic
      Progress()->SetText("Mosaicking " + FileName(inputFile).name());

      try {
        do {
          int outBand = 1;
          
          ProcessMosaic::StartProcess(outSample, outLine, outBand);
          // Reset the creation flag to ensure that the data within the tracking cube written from
          // this call of StartProcess isn't over-written in the next. This needs to occur since the 
          // tracking cube is created in ProcessMosaic if the m_createOutputMosaic flag is set to 
          // true and the cube would then be completely re-created (setting all pixels to Null).
          ProcessMosaic::SetCreateFlag(false);

          // Increment for projections where occurrances may happen multiple times
          outSample += worldSize;
          outSampleEnd += worldSize;
        }
        while (wrapPossible && outSample < nsMosaic);
      }
      catch (IException &e) {
        QString msg = "Unable to mosaic cube [" + FileName(inputFile).name() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
    }

    WriteHistory(*mosaicCube);

    // Don't propagate any more histories now that we've done one
    p_propagateHistory = false;

    ClearInputCubes();

    return true;
  }


  //*************************************************************************************************
  /**
   * Set the output cube to specified file name and specified input images
   * and output attributes
   */
  Isis::Cube *ProcessMapMosaic::SetOutputCube(FileList &propagationCubes, CubeAttributeOutput &oAtt,
      const QString &mosaicFile) {
    int bands = 0;
    double xmin = DBL_MAX;
    double xmax = -DBL_MAX;
    double ymin = DBL_MAX;
    double ymax = -DBL_MAX;
    double slat = DBL_MAX;
    double elat = -DBL_MAX;
    double slon = DBL_MAX;
    double elon = -DBL_MAX;
    bool latlonflag = true;

    TProjection *proj = NULL;

    if (propagationCubes.size() < 1) {
      std::string msg = "The list does not contain any data";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    for (int i = 0; i < propagationCubes.size(); i++) {
      // Open the cube and get the maximum number of band in all cubes
      Cube cube;
      cube.open(propagationCubes[i].toString());
      bands = max(bands, cube.bandCount());

      // See if the cube has a projection and make sure it matches
      // previous input cubes
      TProjection *projNew =
        (TProjection *) Isis::ProjectionFactory::CreateFromCube(*(cube.label()));
      if ((proj != NULL) && (*proj != *projNew)) {
        QString msg = "Mapping groups do not match between cubes [" +
                     propagationCubes[0].toString() + "] and [" + propagationCubes[i].toString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Figure out the x/y range as it may be needed later
      double x = projNew->ToProjectionX(0.5);
      double y = projNew->ToProjectionY(0.5);
      if (x < xmin) xmin = x;
      if (y < ymin) ymin = y;
      if (x > xmax) xmax = x;
      if (y > ymax) ymax = y;

      x = projNew->ToProjectionX(cube.sampleCount() + 0.5);
      y = projNew->ToProjectionY(cube.lineCount() + 0.5);
      if (x < xmin) xmin = x;
      if (y < ymin) ymin = y;
      if (x > xmax) xmax = x;
      if (y > ymax) ymax = y;

      if (projNew->MinimumLatitude() == 0.0 && projNew->MaximumLatitude() == 0.0 &&
          projNew->MinimumLongitude() == 0.0 && projNew->MaximumLongitude() == 0.0) {
        latlonflag = false;
      }

      slat = min(slat, projNew->MinimumLatitude());
      elat = max(elat, projNew->MaximumLatitude());
      slon = min(slon, projNew->MinimumLongitude());
      elon = max(elon, projNew->MaximumLongitude());

      // Cleanup
      cube.close();
      if (proj) delete proj;
      proj = projNew;
    }

    if (proj) delete proj;

    return SetOutputCube(propagationCubes[0].toString(), xmin, xmax, ymin, ymax,
                         slat, elat, slon, elon, bands, oAtt, mosaicFile, latlonflag);
  }


  //*************************************************************************************************
  /**
   * Set the output cube to specified file name and specified input images
   * and output attributes
   */
  Isis::Cube *ProcessMapMosaic::RingsSetOutputCube(FileList &propagationCubes, CubeAttributeOutput &oAtt,
      const QString &mosaicFile) {
    int bands = 0;
    double xmin = DBL_MAX;
    double xmax = -DBL_MAX;
    double ymin = DBL_MAX;
    double ymax = -DBL_MAX;
    double srad = DBL_MAX;  // starting ring radius
    double erad = -DBL_MAX; // ending ring radius
    double saz = DBL_MAX;   // starting azimuth (ring longitude)
    double eaz = -DBL_MAX;  // ending azimuth (ring longitude)

    RingPlaneProjection *proj = NULL;

    if (propagationCubes.size() < 1) {
      std::string msg = "The list does not contain any data";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    for (int i = 0; i < propagationCubes.size(); i++) {
      // Open the cube and get the maximum number of band in all cubes
      Cube cube;
      cube.open(propagationCubes[i].toString());
      bands = max(bands, cube.bandCount());

      // See if the cube has a projection and make sure it matches
      // previous input cubes
      RingPlaneProjection *projNew =
        (RingPlaneProjection *) Isis::ProjectionFactory::CreateFromCube(*(cube.label()));
      if ((proj != NULL) && (*proj != *projNew)) {
        QString msg = "Mapping groups do not match between cubes [" +
                     propagationCubes[0].toString() + "] and [" + propagationCubes[i].toString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Figure out the x/y range as it may be needed later
      double x = projNew->ToProjectionX(0.5);
      double y = projNew->ToProjectionY(0.5);
      if (x < xmin) xmin = x;
      if (y < ymin) ymin = y;
      if (x > xmax) xmax = x;
      if (y > ymax) ymax = y;

      x = projNew->ToProjectionX(cube.sampleCount() + 0.5);
      y = projNew->ToProjectionY(cube.lineCount() + 0.5);
      if (x < xmin) xmin = x;
      if (y < ymin) ymin = y;
      if (x > xmax) xmax = x;
      if (y > ymax) ymax = y;

      srad = min(srad, projNew->MinimumRingRadius());
      erad = max(erad, projNew->MaximumRingRadius());
      saz = min(saz, projNew->MinimumRingLongitude());
      eaz = max(eaz, projNew->MaximumRingLongitude());

      // Cleanup
      cube.close();
      if (proj) delete proj;
      proj = projNew;
    }

    if (proj) delete proj;

    return RingsSetOutputCube(propagationCubes[0].toString(), xmin, xmax, ymin, ymax,
                         srad, erad, saz, eaz, bands, oAtt, mosaicFile);
  }


  //*************************************************************************************************
  /**
   * Set the output cube to specified file name and specified input images
   * and output attributes and lat,lons
   */
  Isis::Cube *ProcessMapMosaic::SetOutputCube(FileList &propagationCubes,
      double slat, double elat, double slon, double elon,
      CubeAttributeOutput &oAtt, const QString &mosaicFile) {
    if (propagationCubes.size() < 1) {
      std::string msg = "The list does not contain any data";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    int samples, lines, bands = 0;
    Pvl label;
    label.read(propagationCubes[0].toString().toStdString());
    PvlGroup mGroup = label.findGroup("Mapping", Pvl::Traverse);

    // All mosaicking programs use only the upper left x and y to determine where to
    // place an image into a mosaic. For clarity purposes, the mosaic programs do
    // not use lat/lon ranges for anything except creating the mosaic. By specifying
    // the lat/lon range of the mosaic, we compute the upper left x/y of the mosaic.
    // All map projected cubes must have an upper left x/y and do not require a lat/lon
    // range. If the current values for the latitude and longitude range are out of
    // order or equal, then we don't write them to the labels.
    if (slat < elat && slon < elon) {
      mGroup.addKeyword(PvlKeyword("MinimumLatitude", std::to_string(slat)), Pvl::Replace);
      mGroup.addKeyword(PvlKeyword("MaximumLatitude", std::to_string(elat)), Pvl::Replace);
      mGroup.addKeyword(PvlKeyword("MinimumLongitude", std::to_string(slon)), Pvl::Replace);
      mGroup.addKeyword(PvlKeyword("MaximumLongitude", std::to_string(elon)), Pvl::Replace);
    }

    if (mGroup.hasKeyword("UpperLeftCornerX"))
      mGroup.deleteKeyword("UpperLeftCornerX");

    if (mGroup.hasKeyword("UpperLeftCornerY"))
      mGroup.deleteKeyword("UpperLeftCornerY");

    Pvl mapPvl;
    mapPvl += mGroup;

    // Use CreateForCube because our range differs from any of the cubes (manually specified)
    Projection *proj = Isis::ProjectionFactory::CreateForCube(mapPvl, samples, lines, false);

    double xmin, xmax, ymin, ymax;
    proj->XYRange(xmin, xmax, ymin, ymax);

    // The xmin/ymax should be rounded for the labels
    xmin = mapPvl.findGroup("Mapping")["UpperLeftCornerX"];
    ymax = mapPvl.findGroup("Mapping")["UpperLeftCornerY"];

    for (int i = 0; i < propagationCubes.size(); i++) {
      Cube cube;
      cube.open(propagationCubes[i].toString());
      bands = max(cube.bandCount(), bands);

      // See if the cube has a projection and make sure it matches
      // previous input cubes
      Projection *projNew =
          Isis::ProjectionFactory::CreateFromCube(*(cube.label()));

      if (proj == NULL) {
      }
      else if (*proj != *projNew) {
        std::string msg = "Mapping groups do not match between cube [" + propagationCubes[i].toString().toStdString() +
                     "] and [" + propagationCubes[0].toString().toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      if (proj) delete proj;
      proj = projNew;
    }

    if (proj) delete proj;

    return SetOutputCube(propagationCubes[0].toString(), xmin, xmax, ymin, ymax,
                         slat, elat, slon, elon, bands, oAtt, mosaicFile);
  }


  //*************************************************************************************************
  /**
   * Set the output cube to specified file name using the specified input 
   * images, output attributes, ring radii values and ring longitude 
   * values. 
   *  
   * @param propagationCubes List of input images
   * @param srad Start ring radius
   * @param erad End ring radius
   * @param saz Start ring longitude (azimuth)
   * @param eaz End ring longitude (azimuth)
   * @param oAtt Output attributes
   * @param mosaicFile Name of the output mosaic file
   */
  Isis::Cube *ProcessMapMosaic::RingsSetOutputCube(FileList &propagationCubes,
      double srad, double erad, double saz, double eaz,
      CubeAttributeOutput &oAtt, const QString &mosaicFile) {
    if (propagationCubes.size() < 1) {
      std::string msg = "The list does not contain any data";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    int samples, lines, bands = 0;
    Pvl label;
    label.read(propagationCubes[0].toString().toStdString());
    PvlGroup mGroup = label.findGroup("Mapping", Pvl::Traverse);
    mGroup.addKeyword(PvlKeyword("MinimumRingRadius", std::to_string(srad)), Pvl::Replace);
    mGroup.addKeyword(PvlKeyword("MaximumRingRadius", std::to_string(erad)), Pvl::Replace);
    mGroup.addKeyword(PvlKeyword("MinimumRingLongitude", std::to_string(saz)), Pvl::Replace);
    mGroup.addKeyword(PvlKeyword("MaximumRingLongitude", std::to_string(eaz)), Pvl::Replace);

    if (mGroup.hasKeyword("UpperLeftCornerX"))
      mGroup.deleteKeyword("UpperLeftCornerX");

    if (mGroup.hasKeyword("UpperLeftCornerY"))
      mGroup.deleteKeyword("UpperLeftCornerY");

    Pvl mapPvl;
    mapPvl += mGroup;

    // Use CreateForCube because our range differs from any of the cubes (manually specified)
    Projection *proj = Isis::ProjectionFactory::RingsCreateForCube(mapPvl, samples, lines, false);

    double xmin, xmax, ymin, ymax;
    proj->XYRange(xmin, xmax, ymin, ymax);

    // The xmin/ymax should be rounded for the labels
    xmin = mapPvl.findGroup("Mapping")["UpperLeftCornerX"];
    ymax = mapPvl.findGroup("Mapping")["UpperLeftCornerY"];

    for (int i = 0; i < propagationCubes.size(); i++) {
      Cube cube;
      cube.open(propagationCubes[i].toString());
      bands = max(cube.bandCount(), bands);

      // See if the cube has a projection and make sure it matches
      // previous input cubes
      Projection *projNew = Isis::ProjectionFactory::RingsCreateFromCube(*(cube.label()));

      if (proj == NULL) {
      }
      else if (*proj != *projNew) {
        std::string msg = "Mapping groups do not match between cube [" + propagationCubes[i].toString().toStdString() +
                     "] and [" + propagationCubes[0].toString().toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      if (proj) delete proj;
      proj = projNew;
    }

    if (proj) delete proj;

    return RingsSetOutputCube(propagationCubes[0].toString(), xmin, xmax, ymin, ymax,
                         srad, erad, saz, eaz, bands, oAtt, mosaicFile);
  }


  //NExt
  //*************************************************************************************************

  /**
   * Set the output cube to specified file name and specified input images
   * and output attributes and lat,lons
   */
  Isis::Cube *ProcessMapMosaic::SetOutputCube(const QString &inputFile,
      double xmin, double xmax, double ymin, double ymax,
      double slat, double elat, double slon, double elon, int nbands,
      CubeAttributeOutput &oAtt, const QString &mosaicFile, bool latlonflag) {
    Pvl fileLab(inputFile.toStdString());
    PvlGroup &mapping = fileLab.findGroup("Mapping", Pvl::Traverse);

    // All mosaicking programs use only the upper left x and y to determine where to
    // place an image into a mosaic. For clarity purposes, the mosaic programs do
    // not use lat/lon ranges for anything except creating the mosaic. By specifying
    // the lat/lon range of the mosaic, we compute the upper left x/y of the mosaic.
    // All map projected cubes must have an upper left x/y and do not require a lat/lon
    // range. If the current values for the latitude and longitude range are out of
    // order or equal, then we don't write them to the labels.
    if (latlonflag && slat < elat && slon < elon) {
      mapping.addKeyword(PvlKeyword("MinimumLatitude", std::to_string(slat)), Pvl::Replace);
      mapping.addKeyword(PvlKeyword("MaximumLatitude", std::to_string(elat)), Pvl::Replace);
      mapping.addKeyword(PvlKeyword("MinimumLongitude", std::to_string(slon)), Pvl::Replace);
      mapping.addKeyword(PvlKeyword("MaximumLongitude", std::to_string(elon)), Pvl::Replace);
    }
    else {
      if (mapping.hasKeyword("MinimumLatitude")) {
        mapping.deleteKeyword("MinimumLatitude");
      }
      if (mapping.hasKeyword("MaximumLatitude")) {
        mapping.deleteKeyword("MaximumLatitude");
      }
      if (mapping.hasKeyword("MinimumLongitude")) {
        mapping.deleteKeyword("MinimumLongitude");
      }
      if (mapping.hasKeyword("MaximumLongitude")) {
        mapping.deleteKeyword("MaximumLongitude");
      }
    }

    Projection *firstProj = ProjectionFactory::CreateFromCube(fileLab);
    firstProj->SetUpperLeftCorner(Displacement(xmin, Displacement::Meters),
                                  Displacement(ymax, Displacement::Meters));

    int samps = (int)(ceil(firstProj->ToWorldX(xmax) - firstProj->ToWorldX(xmin)) + 0.5);
    int lines = (int)(ceil(firstProj->ToWorldY(ymin) - firstProj->ToWorldY(ymax)) + 0.5);

    if (p_createMosaic) {
      Pvl newMap;
      newMap.addGroup(firstProj->Mapping());

      // Initialize the mosaic
      CubeAttributeInput inAtt;

      ProcessByLine p;
      p.SetInputCube(inputFile, inAtt);
      p.PropagateHistory(false);
      p.PropagateLabels(false);
      p.PropagateTables(false);
      p.PropagatePolygons(false);
      p.PropagateOriginalLabel(false);

      // For average priority, get the new band count
      if (GetImageOverlay() == AverageImageWithMosaic) {
        nbands *= 2;
      }

      Cube *ocube = p.SetOutputCube(mosaicFile, oAtt, samps, lines, nbands);
      p.Progress()->SetText("Initializing mosaic");
      p.ClearInputCubes();
      p.StartProcess(ProcessMapMosaic::FillNull);

      // CreateForCube created some keywords in the mapping group that needs to be added
      ocube->putGroup(newMap.findGroup("Mapping", Pvl::Traverse));
      p.EndProcess();
    }
    delete firstProj;

    Cube *mosaicCube = new Cube();
    mosaicCube->open(mosaicFile, "rw");
    mosaicCube->addCachingAlgorithm(new UniqueIOCachingAlgorithm(2));

    AddOutputCube(mosaicCube);
    return mosaicCube;
  }


  //NExt
  //*************************************************************************************************

  /**
   * Set the output cube to specified file name using the specified input
   * file name, output attributes, ring radii values and ring longitude 
   * values. 
   *  
   * @param inputFile Name of input file 
   * @param xmin Minimum x-value
   * @param xmax Maximum x-value
   * @param ymin Minimum y-value
   * @param ymax Maximum y-value
   * @param srad Start ring radius
   * @param erad End ring radius
   * @param saz Start ring longitude (azimuth)
   * @param eaz End ring longitude (azimuth)
   * @param oAtt Output attributes
   * @param mosaicFile Name of the output mosaic file
   */
  Isis::Cube *ProcessMapMosaic::RingsSetOutputCube(const QString &inputFile,
      double xmin, double xmax, double ymin, double ymax,
      double srad, double erad, double saz, double eaz, int nbands,
      CubeAttributeOutput &oAtt, const QString &mosaicFile) {
    Pvl fileLab(inputFile.toStdString());
    PvlGroup &mapping = fileLab.findGroup("Mapping", Pvl::Traverse);

    mapping["UpperLeftCornerX"] = std::to_string(xmin);
    mapping["UpperLeftCornerY"] = std::to_string(ymax);
    mapping.addKeyword(PvlKeyword("MinimumRingRadius", std::to_string(srad)), Pvl::Replace);
    mapping.addKeyword(PvlKeyword("MaximumRingRadius", std::to_string(erad)), Pvl::Replace);
    mapping.addKeyword(PvlKeyword("MinimumRingLongitude", std::to_string(saz)), Pvl::Replace);
    mapping.addKeyword(PvlKeyword("MaximumRingLongitude", std::to_string(eaz)), Pvl::Replace);

    Projection *firstProj = ProjectionFactory::RingsCreateFromCube(fileLab);
    int samps = (int)(ceil(firstProj->ToWorldX(xmax) - firstProj->ToWorldX(xmin)) + 0.5);
    int lines = (int)(ceil(firstProj->ToWorldY(ymin) - firstProj->ToWorldY(ymax)) + 0.5);
    delete firstProj;

    if (p_createMosaic) {
      Pvl newMap;
      newMap.addGroup(mapping);

      // Initialize the mosaic
      CubeAttributeInput inAtt;

      ProcessByLine p;
      p.SetInputCube(inputFile, inAtt);
      p.PropagateHistory(false);
      p.PropagateLabels(false);
      p.PropagateTables(false);
      p.PropagatePolygons(false);
      p.PropagateOriginalLabel(false);

      // For average priority, get the new band count
      if (GetImageOverlay() == AverageImageWithMosaic) {
        nbands *= 2;
      }

      Cube *ocube = p.SetOutputCube(mosaicFile, oAtt, samps, lines, nbands);
      p.Progress()->SetText("Initializing mosaic");
      p.ClearInputCubes();
      p.StartProcess(ProcessMapMosaic::FillNull);

      // CreateForCube created some keywords in the mapping group that needs to be added
      ocube->putGroup(newMap.findGroup("Mapping", Pvl::Traverse));
      p.EndProcess();
    }

    Cube *mosaicCube = new Cube();
    mosaicCube->open(mosaicFile, "rw");
    mosaicCube->addCachingAlgorithm(new UniqueIOCachingAlgorithm(2));

    AddOutputCube(mosaicCube);
    return mosaicCube;
  }


  //*************************************************************************************************

  /**
   * Set the output cube to specified file name and specified input images
   * and output attributes and lat,lons
   */
  Isis::Cube *ProcessMapMosaic::SetOutputCube(const QString &inputFile, PvlGroup mapping,
      CubeAttributeOutput &oAtt, const QString &mosaicFile) {
    if (OutputCubes.size() != 0) {
      std::string msg = "You can only specify one output cube and projection";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (mapping.hasKeyword("UpperLeftCornerX"))
      mapping.deleteKeyword("UpperLeftCornerX");

    if (mapping.hasKeyword("UpperLeftCornerY"))
      mapping.deleteKeyword("UpperLeftCornerY");

    if (p_createMosaic) {
      Pvl newMap;
      newMap.addGroup(mapping);
      int samps, lines, bands;
      delete ProjectionFactory::CreateForCube(newMap, samps, lines, false);

      // Initialize the mosaic
      ProcessByLine p;
      CubeAttributeInput inAtt(inputFile);
      Cube *propCube = p.SetInputCube(inputFile, inAtt);
      bands = propCube->bandCount();

      // For average priority, get the new band count
      if (GetImageOverlay() == AverageImageWithMosaic) {
        bands *= 2;
      }

      p.PropagateHistory(false);
      p.PropagateLabels(false);
      Cube *ocube = p.SetOutputCube(mosaicFile, oAtt, samps, lines, bands);
      p.Progress()->SetText("Initializing mosaic");
      p.ClearInputCubes();

      p.StartProcess(ProcessMapMosaic::FillNull);

      // CreateForCube created some keywords in the mapping group that needs to be added
      ocube->putGroup(newMap.findGroup("Mapping", Pvl::Traverse));
      p.EndProcess();
    }

    Cube *mosaicCube = new Cube();
    AddOutputCube(mosaicCube);
    mosaicCube->open(mosaicFile, "rw");
    mosaicCube->addCachingAlgorithm(new UniqueIOCachingAlgorithm(2));

    return mosaicCube;
  }


  //*************************************************************************************************

  /**
   * Set the output cube to specified file name and specified input images
   * and output attributes and lat,lons
   */
  Isis::Cube *ProcessMapMosaic::RingsSetOutputCube(const QString &inputFile, PvlGroup mapping,
      CubeAttributeOutput &oAtt, const QString &mosaicFile) {
    if (OutputCubes.size() != 0) {
      QString msg = "You can only specify one output cube and projection";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (mapping.hasKeyword("UpperLeftCornerX"))
      mapping.deleteKeyword("UpperLeftCornerX");

    if (mapping.hasKeyword("UpperLeftCornerY"))
      mapping.deleteKeyword("UpperLeftCornerY");

    if (p_createMosaic) {
      Pvl newMap;
      newMap.addGroup(mapping);
      int samps, lines, bands;
      delete ProjectionFactory::RingsCreateForCube(newMap, samps, lines, false);

      // Initialize the mosaic
      ProcessByLine p;
      CubeAttributeInput inAtt(inputFile);
      Cube *propCube = p.SetInputCube(inputFile, inAtt);
      bands = propCube->bandCount();

      // For average priority, get the new band count
      if (GetImageOverlay() == AverageImageWithMosaic) {
        bands *= 2;
      }

      p.PropagateHistory(false);
      p.PropagateLabels(false);
      Cube *ocube = p.SetOutputCube(mosaicFile, oAtt, samps, lines, bands);
      p.Progress()->SetText("Initializing mosaic");
      p.ClearInputCubes();

      p.StartProcess(ProcessMapMosaic::FillNull);

      // CreateForCube created some keywords in the mapping group that needs to be added
      ocube->putGroup(newMap.findGroup("Mapping", Pvl::Traverse));
      p.EndProcess();
    }

    Cube *mosaicCube = new Cube();
    AddOutputCube(mosaicCube);
    mosaicCube->open(mosaicFile, "rw");
    mosaicCube->addCachingAlgorithm(new UniqueIOCachingAlgorithm(2));

    return mosaicCube;
  }


  //*************************************************************************************************

  /**
   * Mosaic output method for Mosaic Processing Method, this will use an existing mosaic
   */
  Cube *ProcessMapMosaic::SetOutputCube(const QString &mosaicFile) {
    p_createMosaic = false;
    Cube mosaic;
    mosaic.open(mosaicFile);

    PvlGroup &mapping = mosaic.label()->findGroup("Mapping", Pvl::Traverse);
    CubeAttributeOutput oAtt;
    // The other SetOutput will not use the attribute or filename
    Cube *ocube = SetOutputCube("", mapping, oAtt, mosaicFile);
    p_createMosaic = true;

    return ocube;
  }


  //*************************************************************************************************

  /**
   * Mosaic output method for Mosaic Processing Method, this will use an existing mosaic
   */
  Cube *ProcessMapMosaic::RingsSetOutputCube(const QString &mosaicFile) {
    p_createMosaic = false;
    Cube mosaic;
    mosaic.open(mosaicFile);

    PvlGroup &mapping = mosaic.label()->findGroup("Mapping", Pvl::Traverse);
    CubeAttributeOutput oAtt;
    // The other SetOutput will not use the attribute or filename
    Cube *ocube = RingsSetOutputCube("", mapping, oAtt, mosaicFile);
    p_createMosaic = true;

    return ocube;
  }


  //*************************************************************************************************
  /**
   * Reset the buffer with NULL pixels
   */
  void ProcessMapMosaic::FillNull(Buffer &data) {
    for (int i = 0; i < data.size(); i++) data[i] = Isis::Null;
  }
} // end namespace isis
