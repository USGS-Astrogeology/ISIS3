/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ShapeModelFactory.h"

#include <string>

#include "BulletShapeModel.h"
#include "BulletTargetShape.h"
#include "Cube.h"
#include "DemShape.h"
#include "EllipsoidShape.h"
#include "EmbreeShapeModel.h"
#include "EmbreeTargetManager.h"
#include "EquatorialCylindricalShape.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "NaifDskShape.h"
#include "NaifStatus.h"
#include "PlaneShape.h"
#include "Projection.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlFlatMap.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "Target.h"

using namespace std;

namespace Isis {
  /**
   * Constructor is private to avoid instantiating the class.  Use the Create method.
   *
   * @author 2010-01-29 Debbie Cook
   */
  ShapeModelFactory::ShapeModelFactory() {
  }


  //! Destructor
  ShapeModelFactory::~ShapeModelFactory() {
  }


  /**
   * Construct a valid shape model from the given target and contents of kernels
   * group. If the Kernels group does not have a ShapeModel or ElevationModel
   * specified, then the default model is an ellipsoidal shape.
   *
   * @param target Pointer to target body model.
   * @param pvl Pvl containing a Kernels group.
   *
   * @return ShapeModel* Pointer to the created ShapeModel object.
   *
   * @author 2017-03-17 Kris Becker
   */
  ShapeModel *ShapeModelFactory::create(Target *target, Pvl &pvl) {

    // get kernels and instrument Pvl groups
    PvlGroup &kernelsPvlGroup = pvl.findGroup("Kernels", Pvl::Traverse);

    // Get user preferences to determine preferred model
    PvlFlatMap parameters(kernelsPvlGroup);
    if ( Preference::Preferences().hasGroup("ShapeModel") ) {
      parameters.merge(PvlFlatMap(Preference::Preferences().findGroup("ShapeModel")));
    }

    // Do we need a sky shape model, member variable, or neither? For now treat sky as ellipsoid
    bool skyTarget = target->isSky();

    // Determine if target is a plane??? target name has rings in it?
    // Another keyword in label to indicate plane? What about lander/rovers?
    // bool planeTarget = false;

    // shape model file name
    QString shapeModelFilenames = "";

    // TODO: We differentiate between "Elevation" and "Shape" models on the
    // labels, but we assign either one to the shapeModelFilename. Do we
    // need a shapeModelFilename AND an elevationModelFilename?
    // is this historical? Interchangeable?
    if (skyTarget) {
      // Sky targets are ellipsoid shapes
    }
    else if (kernelsPvlGroup.hasKeyword("ElevationModel") &&
             !kernelsPvlGroup["ElevationModel"].isNull())  {
      shapeModelFilenames = QString::fromStdString(kernelsPvlGroup["ElevationModel"]);
    }
    else if (kernelsPvlGroup.hasKeyword("ShapeModel") &&
             !kernelsPvlGroup["ShapeModel"].isNull()) {
      shapeModelFilenames = QString::fromStdString(kernelsPvlGroup["ShapeModel"]);
    }

    // Create shape model
    ShapeModel *shapeModel = NULL;

    // TODO: If there is no shape model filename, the shape model type defaults to an
    // ellipsoid (should it?).

    // This exception will be thrown at the end of this method if no shape model is constructed.
    // More specific exceptions will be appended before throwing this error.
    IException finalError(IException::Programmer,
                          "Unable to create a shape model from given target and pvl.",
                          _FILEINFO_);

    if (shapeModelFilenames == "") {
      // No file name given.  If EllipsoidShape throws an error or returns null, the following
      // exception will be appended to the finalError.
      std::string msg = "Unable to construct an Ellipsoid shape model.";

      try {
        shapeModel = new EllipsoidShape(target);
      }
      catch (IException &e) {
        // No file name given and ellipsoid fails. Append e to new exception
        // with above message. Append this to finalError and throw.
        finalError.append(IException(e, IException::Unknown, msg, _FILEINFO_));
        throw finalError;
      }
      // in case no error was thrown, but constructor returned NULL
      finalError.append(IException(IException::Unknown, msg, _FILEINFO_));
    }
    else if (shapeModelFilenames == "RingPlane") {
      // No file name given, RingPlane indicated.  If PlaneShape throws an error or returns
      // null, the following exception will be appended to the finalError.
      std::string msg = "Unable to construct a RingPlane shape model.";

      try {
        shapeModel = new PlaneShape(target, pvl);
      }
      catch (IException &e) {
        // No file name given, RingPlane specified. Append a message to the finalError and throw it.
        finalError.append(IException(e, IException::Unknown, msg, _FILEINFO_));
        throw finalError;
      }
      // in case no error was thrown, but constructor returned NULL
      finalError.append(IException(IException::Unknown, msg, _FILEINFO_));
    }
    else { // assume shape model given is a Bullet, Embree, NAIF DSK or DEM cube file name

      QString preferred = parameters.get("RayTraceEngine", "None").toLower();
      QString onerror   = parameters.get("OnError", "Continue").toLower();
      double  tolerance = parameters.get("Tolerance", QString::number(DBL_MAX)).toDouble();

      // A file error message will be appened to the finalError, if no shape model is constructed.
      std::string fileErrorMsg = "Invalid shape model file ["
                             + shapeModelFilenames.toStdString() + "] in Kernels group.";
      IException fileError(IException::Io, fileErrorMsg, _FILEINFO_);

      //-------------- Check for bullet engine first -------------------------------//
      if ( "bullet" == preferred ) {
        // Check to see of ISIS cube DEMs get a pass
        FileName v_shapefile(shapeModelFilenames.toStdString());
        QString ext = QString::fromStdString(v_shapefile.extension()).toLower();
        // Cubes are not supported at this time.

        try {
          BulletTargetShape *bullet = BulletTargetShape::load(shapeModelFilenames);
          if ( 0 == bullet ) {

            // Bullet failed to load the kernel...test failure conditions
            if ("cub" == ext) {
              std::string mess = "Bullet could not initialize ISIS Cube DEM";
              throw IException(IException::Unknown, mess, _FILEINFO_);
            }

            // Always throw an error in this case
            std::string b_msg = "Bullet could not initialize DEM!";
            throw IException(IException::Unknown, b_msg, _FILEINFO_);
          }
          else {

            // Allocate the real shape model
            BulletShapeModel *b_model = new BulletShapeModel(bullet, target, pvl);
            b_model->setTolerance(tolerance);

            // Do this here, otherwise default behavior will ensue from here on out
            kernelsPvlGroup.addKeyword(PvlKeyword("RayTraceEngine", preferred.toStdString()), PvlContainer::Replace);
            kernelsPvlGroup.addKeyword(PvlKeyword("OnError", onerror.toStdString()), PvlContainer::Replace);
            kernelsPvlGroup.addKeyword(PvlKeyword("Tolerance", Isis::toString(tolerance)),
                                                  PvlContainer::Replace);

            return ( b_model );
          }
        } 
        catch (IException &ie) {
          fileError.append(ie);
          std::string mess = "Unable to create preferred BulletShapeModel";
          fileError.append(IException(IException::Unknown, mess, _FILEINFO_));
          if ("fail" == onerror) {
            throw fileError;
          }
        }

        // Don't have ShapeModel yet - invoke pre-exising behavior (2017-03-23)
      }

      //-------------- Check for Embree engine -------------------------------//
      if ( "embree" == preferred ) {

        // Check to see of ISIS cube DEMs get a pass
        FileName v_shapefile(shapeModelFilenames.toStdString());
        QString ext = QString::fromStdString(v_shapefile.extension()).toLower();
        // Cubes are not supported at this time

        try {

          // Allocate the shape model
          EmbreeTargetManager *targetManager = EmbreeTargetManager::getInstance();
          EmbreeShapeModel *embreeModel = new EmbreeShapeModel(target, shapeModelFilenames,
                                                               targetManager);
          embreeModel->setTolerance(tolerance);

          // Do this here, otherwise default behavior will ensue from here on out
          kernelsPvlGroup.addKeyword(PvlKeyword("RayTraceEngine", preferred.toStdString()), PvlContainer::Replace);
          kernelsPvlGroup.addKeyword(PvlKeyword("OnError", onerror.toStdString()), PvlContainer::Replace);
          kernelsPvlGroup.addKeyword(PvlKeyword("Tolerance", Isis::toString(tolerance)),
                                                PvlContainer::Replace);

          return ( embreeModel );

        } catch (IException &ie) {
          fileError.append(ie);
          std::string mess = "Unable to create preferred EmbreeShapeModel";
          fileError.append(IException(IException::Unknown, mess, _FILEINFO_));
          if ("fail" == onerror) {
            throw fileError;
          }
        }
      }

      //-------------- Is the shape model a NAIF DSK? ------------------------------//

      // If NaifDskShape throws an error or returns null and DEM construction is
      // unsuccessful, the following exception will be appended to the fileError.
      std::string msg = "The given shape model file is not a valid NAIF DSK file. "
                    "Unable to construct a NAIF DSK shape model.";
      IException dskError(IException::Unknown, msg, _FILEINFO_);

      try {
        // try to create a NaifDskShape object
        shapeModel = new NaifDskShape(target, pvl);
      }
      catch (IException &e) {
        // append a message to the fileError, but don't throw it.
        // We will make sure it's not a DEM before throwing the error.
        dskError.append(e);
      }

      if (shapeModel == NULL) {

        // in case no error was thrown, but constructor returned NULL
        fileError.append(dskError);

        //-------------- Is the shape model an ISIS DEM? ------------------------------//
        // TODO Deal with stacks -- this could be a list of DEMs
        Isis::Cube* shapeModelCube = new Isis::Cube;
        try {
          // first, try to open the shape model file as an Isis cube
          shapeModelCube->open(QString::fromStdString(FileName(shapeModelFilenames.toStdString()).expanded()), "r" );
        }
        catch (IException &e) {
          // The file is neither a valid DSK nor an ISIS cube. Append a message and throw the error.
          std::string msg = "The given shape model file is not a valid ISIS DEM. "
                        "Unable to open as an ISIS cube.";
          fileError.append(IException(e, IException::Unknown, msg, _FILEINFO_));
          finalError.append(fileError);
          throw finalError;
        }

        Projection *projection = NULL;
        try {
          // get projection of shape model cube
          projection = shapeModelCube->projection();
        }
        catch (IException &e) {
          // The file is neither a valid DSK nor a valid ISIS DEM. Append message and throw the error.
          std::string msg = "The given shape model file is not a valid ISIS DEM cube. "
                        "It is not map-projected.";
          fileError.append(IException(e, IException::Unknown, msg, _FILEINFO_));
          finalError.append(fileError);
          throw finalError;
        }

        if (projection->IsEquatorialCylindrical()) {
          // If the EquatorialCylindricalShape constructor throws an error or returns null, the
          // following exception will be appended to the fileError. (Later added to the finalError)
          std::string msg = "Unable to construct a DEM shape model from the given "
                        "EquatorialCylindrical projected ISIS cube.";

          try {
            shapeModel = new EquatorialCylindricalShape(target, pvl);
          }
          catch (IException &e) {
            // The file is an equatorial cylindrical ISIS cube. Append fileError and throw.
            fileError.append(IException(e, IException::Unknown, msg, _FILEINFO_));
            finalError.append(fileError);
            throw finalError;
          }
          // in case no error was thrown, but constructor returned NULL
          fileError.append(IException(IException::Unknown, msg, _FILEINFO_));
        }
        else {
          // If the DemShape constructor throws an error or returns null, the following
          // exception will be appended to the fileError. (Later added to the finalError)
          std::string msg = "Unable to construct a DEM shape model "
                        "from the given projected ISIS cube file.";

          try {
            shapeModel = new DemShape(target, pvl);
          }
          catch (IException &e) {
            // The file is projected ISIS cube (assumed to be DEM). Append fileError and throw.
            fileError.append(IException(e, IException::Unknown, msg, _FILEINFO_));
            finalError.append(fileError);
            throw finalError;
          }
          // in case no error was thrown, but constructor returned NULL
          fileError.append(IException(IException::Unknown, msg, _FILEINFO_));
        }

        delete shapeModelCube;

      }

      // in case no error was thrown, but DSK, Equatorial, or DEM constructor returned NULL
      finalError.append(fileError);
    }

    // TODO Add Naif DSK shape and stack?

    if (shapeModel == NULL) {
      throw finalError;
    }

    return shapeModel;
  }
} // end namespace isis
