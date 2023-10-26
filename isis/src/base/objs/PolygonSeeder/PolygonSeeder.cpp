/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PolygonSeeder.h"

#include "IException.h"
#include "LeastSquares.h"
#include "Plugin.h"
#include "PolygonTools.h"
#include "PolynomialBivariate.h"
#include "LeastSquares.h"
#include "FileName.h"
#include "ProjectionFactory.h"
#include "Pvl.h"
#include "PvlGroup.h"


namespace Isis {
  /**
   * Create PolygonSeeder object.  Because this is a pure virtual class you can
   * not create an PolygonSeeder class directly.  Instead, see the
   * PolygonSeederFactory class.
   *
   * @param pvl  A pvl object containing a valid PolygonSeeder specification
   *
   */
  PolygonSeeder::PolygonSeeder(Pvl &pvl) {
    invalidInput = NULL;
    invalidInput = new Pvl(pvl);

    p_algorithmName = "Unknown";

    Parse(pvl);
  }


  /**
   * @brief Copy constructor.
   *  
   * Create PolygonSeeder object by copying the algorithm name, the minimum 
   * thickness, and the minimum area of an existing PolygonSeeder object.
   *
   * @param other The other PolygonSeeder object that will be copied.
   * 
   */
  PolygonSeeder::PolygonSeeder(const PolygonSeeder &other) {
    p_algorithmName = other.p_algorithmName;
    p_minimumThickness = other.p_minimumThickness;
    p_minimumArea = other.p_minimumArea;
  }


  /**
   * Destroys the PolygonSeeder object
   */
  PolygonSeeder::~PolygonSeeder() {
    if(invalidInput) {
      delete invalidInput;
      invalidInput = NULL;
    }
  }


  /**
   * Initialize parameters in the PolygonSeeder class using a PVL specification.
   * An example of the PVL required for this is:
   *
   * @code
   * Object = AutoSeed
   *   Group = Algorithm
   *     Name      = Grid
   *     Tolerance = 0.7
   *   EndGroup
   * EndObject
   * @endcode
   *
   * There are many other options that can be set via the pvl and are
   * described in other documentation (see below).
   *
   * @param pvl The pvl object containing the specification
   **/
  void PolygonSeeder::Parse(Pvl &pvl) {

    QString errorSpot;

    try {
      // Get info from Algorithm group
      errorSpot = "Algorithm";
      PvlGroup &algo = pvl.findGroup("PolygonSeederAlgorithm", Pvl::Traverse);

      // algo is such a cool name for a PvlGroup that it begs to be out done
      PvlGroup &invalgo = invalidInput->findGroup("PolygonSeederAlgorithm",
                          Pvl::Traverse);

      // Set the algorithm name
      errorSpot = "Name";
      p_algorithmName = QString::fromStdString(algo["Name"]);

      if(invalgo.hasKeyword("Name"))
        invalgo.deleteKeyword("Name");

      // Set the minimum thickness (Area / max(extent X, extent Y)**2
      errorSpot = "MinimumThickness";
      p_minimumThickness = 0.0;
      if(algo.hasKeyword("MinimumThickness")) {
        p_minimumThickness = (double) algo["MinimumThickness"];
      }

      if(invalgo.hasKeyword("MinimumThickness"))
        invalgo.deleteKeyword("MinimumThickness");

      // Set the minimum area
      errorSpot = "MinimumArea";
      p_minimumArea = 0.0;
      if(algo.hasKeyword("MinimumArea")) {
        p_minimumArea = (double) algo["MinimumArea"];
      }

      if(invalgo.hasKeyword("MinimumArea"))
        invalgo.deleteKeyword("MinimumArea");
    }
    catch(IException &e) {
      std::string msg = "Improper format for PolygonSeeder PVL [";
      msg +=  pvl.fileName() + "]. Location [" + errorSpot.toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    return;
  }


  /**
   * Check the polygon to see if it meets standard criteria.
   *
   * @param xymp The multipoly containing the coordinates in x/y units instead
   *             of lon/lat
   * @param xyBoundBox The bounding box of the multipoly
   *
   * @return QString A string with an appropriate message to throw if
   *             a test was unsuccessful or an empty string if all tests
   *             passed.
   */
  QString PolygonSeeder::StandardTests(const geos::geom::MultiPolygon *xymp,
      const geos::geom::Envelope *xyBoundBox) {
    if(xymp->getArea() < MinimumArea()) {
      std::string msg = "Polygon did not meet the minimum area of [";
      msg += std::to_string(MinimumArea()) + "]";
      return QString::fromStdString(msg);
    }

    double thickness =
      xymp->getArea() /
      pow(std::max(xyBoundBox->getWidth(), xyBoundBox->getHeight()), 2.0);
    if(thickness < MinimumThickness()) {
      std::string msg = "Polygon did not meet the minimum thickness ratio of [";
      msg += std::to_string(MinimumThickness()) + "]";
      return QString::fromStdString(msg);
    }

    return "";
  }

  /**
   * The name of the algorithm, read from the Name Keyword in the 
   * PolygonSeeder Pvl passed into the constructor. 
   *  
   * @return @b QString The value of the Name Keyword in the Pvl.
   */
  QString PolygonSeeder::Algorithm() const {
    return p_algorithmName;
  }

  /**
   * Return the minimum allowed thickness of the polygon. This value is set
   * from the "MinimumThickness" keyword in the PVL. The seeding algorithm
   * will not seed polygons that have a thickness ratio less than this
   *  
   * @return @b double The value for the minimum thickness allowed. 
   */
  double PolygonSeeder::MinimumThickness() {
    return p_minimumThickness;
  }


  /**
   * Return the minimum allowed area of the polygon. This value is set
   * from the "MinimumArea" keyword in the PVL. The seeding algorithm will
   * not seed polygons that have an area less than this.
   *  
   * @return @b double The value for the minimum area allowed. 
   */
  double PolygonSeeder::MinimumArea() {
    return p_minimumArea;
  }

  /**
   * @brief Plugin parameters. 
   * This method will add the PvlKeyword values for algorithm name, minimum 
   * thickness, and minimum area of this object to a PvlGroup with the name that
   * is passed in. 
   * 
   * @param grpName A string containing the PvlGroup name.
   * 
   * @return @b PvlGroup The PvlGroup with the appropriate parameters added.
   * 
   */
  PvlGroup PolygonSeeder::PluginParameters(QString grpName) {
    PvlGroup pluginInfo(grpName.toStdString());

    PvlKeyword name("Name", p_algorithmName.toStdString());
    PvlKeyword minThickness("MinimumThickness", std::to_string(p_minimumThickness));
    PvlKeyword minArea("MinimumArea", std::to_string(p_minimumArea));

    pluginInfo.addKeyword(name);
    pluginInfo.addKeyword(minThickness);
    pluginInfo.addKeyword(minArea);

    return pluginInfo;
  }


  /**
   * This method returns a copy of the Pvl passed in by the 
   * constructor (from a def file probably) minus what was used.
   *  
   * @return @b Pvl  A copy of this pvl minus what was used.
   */
  Pvl PolygonSeeder::InvalidInput() {
    return *invalidInput;
  }

  /**
   * @brief Assignment operator.
   * Sets this PolygonSeeder object equal to another by copying the
   * algorithm name, the minimum thickness, and the minimum area of the other
   * PolygonSeeder object.
   * 
   * @param other The other PolygonSeeder object whose values will be copied.
   * @return @b PolygonSeeder This PolygonSeeder object, once the other object's
   *                values have been copied.
   * 
   */
  const PolygonSeeder &PolygonSeeder::operator=(const PolygonSeeder &other) {
    p_algorithmName = other.p_algorithmName;
    p_minimumThickness = other.p_minimumThickness;
    p_minimumArea = other.p_minimumArea;

    return *this;
  }

}
