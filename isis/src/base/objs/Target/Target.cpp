/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Target.h"

#include "Angle.h"
#include "Distance.h"
#include "EllipsoidShape.h"
#include "FileName.h"
#include "IException.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "ShapeModelFactory.h"
#include "SpecialPixel.h"
#include "Spice.h"


using namespace std;

namespace Isis {

  /**
   * Constructs a Target object and loads target information.
   *
   * @param lab Label containing Instrument and Kernels groups.
   *
   * @author 2012-03-20 Debbie A. Cook
   *
   * @internal
   * @history 2012-10-11 Debbie A. Cook - Original version
   */

  // TODO: what is needed to initialize?
  Target::Target(Spice *spice, Pvl &lab) {

    // Initialize members
    init();
    m_bodyCode = new SpiceInt;
    m_systemCode = new SpiceInt;
    m_radii.resize(3, Distance());

    m_spice = spice;

    // If we get this far, we know we have a kernels group.  Spice requires it.
    PvlGroup &kernels = lab.findGroup("Kernels", Pvl::Traverse);

    PvlGroup &inst = lab.findGroup("Instrument", Pvl::Traverse);
    m_name = new QString;
    *m_name = QString::fromStdString(inst["TargetName"][0]);
    QString trykey = "NaifIkCode";

    m_systemName = new QString;

    if (kernels.hasKeyword("NaifFrameCode")) {
      trykey = "NaifFrameCode";
    }

    if (name().toUpper() == "SKY") {
      m_radii[0] = m_radii[1] = m_radii[2] = Distance(1000.0, Distance::Meters);
      m_sky = true;
      int ikCode = std::stoi(kernels[trykey.toStdString()][0]);
      *m_bodyCode  = ikCode / 1000;
      // Check for override in kernel group
      if (kernels.hasKeyword("NaifSpkCode")) {
        *m_bodyCode = (int) kernels["NaifSpkCode"];
      }

      *m_systemCode = -1;
      (*m_systemName).append("THE COSMOS");

    }
    else {
      *m_bodyCode = lookupNaifBodyCode(lab);
      m_sky = false;

      *m_systemCode = (*m_bodyCode/100)*100 + 99;

      SpiceChar naifBuf[40];
      SpiceBoolean found;
      bodc2n_c((SpiceInt) *m_systemCode, sizeof(naifBuf), naifBuf, &found);
      NaifStatus::CheckErrors();
      string s(naifBuf);
      (*m_systemName).append(s.c_str());

      // QString radiiKey = "BODY" + QString((BigInt) naifBodyCode()) + "_RADII";
      // m_radii[0] = Distance(getDouble(radiiKey, 0), Distance::Kilometers);
      // m_radii[1] = Distance(getDouble(radiiKey, 1), Distance::Kilometers);
      // m_radii[2] = Distance(getDouble(radiiKey, 2), Distance::Kilometers);
    }
    // Override it if it exists in the labels
    if (kernels.hasKeyword("NaifBodyCode")) {
      *m_bodyCode = (int) kernels["NaifBodyCode"];
    }
    m_shape = ShapeModelFactory::create(this, lab);
  }


  /**
   * Construct a Target without SPICE data.
   * The label should contain an Instrument group with a TargetName and
   * a Kernels group with a ShapeModel.
   *
   * @param label Label containing information about the Target.
   */
  Target::Target(Pvl &label) {
    // Initialize everything to null
    m_bodyCode = new SpiceInt;
    m_systemCode = new SpiceInt;
    m_name = NULL;
    m_systemName = NULL;
    m_spice = NULL;
    m_radii.resize(3, Distance());
    init();

    PvlGroup &inst = label.findGroup("Instrument", Pvl::Traverse);
    QString targetName = QString::fromStdString(inst["TargetName"][0]);
    setName(targetName);

    PvlGroup &kernels = label.findGroup("Kernels", Pvl::Traverse);

    std::string trykey = "NaifIkCode";

    m_systemName = new QString;

    if (kernels.hasKeyword("NaifFrameCode")) {
      trykey = "NaifFrameCode";
    }

    if (name().toUpper() == "SKY" && kernels.hasKeyword(trykey)) {
      m_radii[0] = m_radii[1] = m_radii[2] = Distance(1000.0, Distance::Meters);
      m_sky = true;
      int ikCode = std::stoi(kernels[trykey][0]);
      *m_bodyCode  = ikCode / 1000;
      // Check for override in kernel group
      if (kernels.hasKeyword("NaifSpkCode")) {
        *m_bodyCode = (int) kernels["NaifSpkCode"];
      }

      *m_systemCode = -1;
      (*m_systemName).append("THE COSMOS");
    }

    m_shape = ShapeModelFactory::create(this, label);
  }


  /**
   * Constructs an empty Target object
   *
   * @author 2012-03-20 Debbie A. Cook
   *
   * @internal
   * @history 2012-08-29 Debbie A. Cook - Original version
   */
  Target::Target() {
    m_bodyCode = NULL;
    m_systemCode = NULL;
    m_name = NULL;
    m_systemName = NULL;
    m_spice = NULL;
    init();
  }


  /**
   * Initialize member variables
   *
   * @author 2012-03-20 Debbie A. Cook
   *
   * @internal
   * @history 2012-08-31 Debbie A. Cook - Original version
   */
  void Target::init() {
    m_shape = NULL;
    m_originalShape = NULL;
    m_sky = false;
  }


  /**
   * Destroys the Target object
   */
  Target::~Target() {
    NaifStatus::CheckErrors();

    delete m_bodyCode;
    m_bodyCode = NULL;

    delete m_systemCode;
    m_systemCode = NULL;

    delete m_name;
    m_name = NULL;

    delete m_systemName;
    m_systemName = NULL;

    if (m_radii.size() != 0) {
      m_radii.clear();
    }

    delete m_originalShape;
    m_originalShape = NULL;

    delete m_shape;
    m_shape = NULL;
  }


  //! Return if our target is the sky
  bool Target::isSky() const {
    return m_sky;
  }


  /**
   * This returns the NAIF body code of the target indicated in the labels.
   *
   * @return @b SpiceInt NAIF body code
   *
   */
  SpiceInt Target::lookupNaifBodyCode(Pvl &lab) const {
    SpiceInt code;
    try {
      code = lookupNaifBodyCode(*m_name);
      return code;
    }
    catch (IException &e) {
      try {
        // Get body code from Isis Naif object if it exists or Naif data pool
        if (m_spice) {
          code = m_spice->getInteger("BODY_CODE", 0);
          return code;
        }
        // getInteger automatically calls Spice::readValue which looks in the NaifKeywords
        else if (lab.hasObject("NaifKeywords")
                 && lab.findObject("NaifKeywords").hasKeyword("BODY_CODE") ) {
          code = int(lab.findObject("NaifKeywords").findKeyword("BODY_CODE"));
          return code;
        }
        else {
          throw IException(e,
                           IException::Unknown,
                           "BODY_CODE not found for this Target.",
                           _FILEINFO_);
        }
      }
      catch (IException &e2) {
        e.append(e2);
        throw IException(e,
                         IException::Unknown,
                         "Unable to look up NAIF body code for this Target.",
                           _FILEINFO_);
      }
    }
  }


  /**
   * This returns the NAIF body code of the target indicated in the labels.
   *
   * @return @b SpiceInt NAIF body code
   *
   */
  SpiceInt Target::lookupNaifBodyCode(QString name) {

    NaifStatus::CheckErrors();
    SpiceInt code;
    SpiceBoolean found;
    bodn2c_c(name.toLatin1().data(), &code, &found);
    if (!found) {
      QString msg = "Could not convert Target [" + name +
                   "] to NAIF body code";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    NaifStatus::CheckErrors();
    return  code;

  }


  /**
   * This method returns a Mapping group containing  TargetName,
   * EquatorialRadius, and PolarRadius in addition to all of the keywords
   * that are in the given mapGroup.
   *
   * @param cubeLab Pvl labels for the image.
   * @param mapGroup A const reference to a PvlGroup that contains
   *                 mapping parameters for the projection.
   *
   * @return PvlGroup The Mapping Group for the projection including the
   *                 keywords TargetName, EquatorialRadius, and
   *                 PolarRadius.
   *
   */
  PvlGroup Target::radiiGroup(Pvl &cubeLab, const PvlGroup &mapGroup) {
    PvlGroup mapping = mapGroup;

    // Check to see if the mapGroup already has the target radii.
    // If BOTH radii are already in the mapGroup then just return the given
    // mapping group as is.
    if (mapping.hasKeyword("EquatorialRadius")
        && mapping.hasKeyword("PolarRadius")) {
      return mapping;
    }

    // If radii values are not in the given mapping group, we will get the target from the mapping
    // group or cube label and attempt to use NAIF routines to find the radii.
    QString target = "";
    try {
      if (mapping.hasKeyword("TargetName")) {
        target = QString::fromStdString(mapping["TargetName"][0]);
      }

      // if target name not found in mapping group or value was empty string, try instrument group
      if (target.isEmpty()) {
        bool hasInstrumentGroup = cubeLab.findObject("IsisCube").hasGroup("Instrument");
        if (hasInstrumentGroup) {
          PvlGroup inst = cubeLab.findGroup("Instrument", Pvl::Traverse);
          if (inst.hasKeyword("TargetName")) {
            target = QString::fromStdString(inst["TargetName"][0]);
            mapping.addKeyword( PvlKeyword("TargetName", target.toStdString()), PvlContainer::Replace );
          }
        }
      }

      // target name still not found, throw error
      if (target.isEmpty()) {
        throw IException(IException::Unknown,
                         "Unable to find a TargetName keyword in the given PVL.",
                         _FILEINFO_);
      }

      // first, attempt to use cached values or run NAIF routine on the target name to get the
      // radii values. if this fails, the exception will be caught and we will try to find
      // radii in the NaifKeywords object of the labels
      PvlGroup radii = Target::radiiGroup(target);

      // Successfully found radii using target name.
      // Copy the EquatorialRadius and PolorRadius and we are done.
      mapping.addKeyword( radii.findKeyword("EquatorialRadius"), PvlContainer::Replace );
      mapping.addKeyword( radii.findKeyword("PolarRadius"),      PvlContainer::Replace );
      return mapping;
    }
    catch (IException &e) {
      // If all previous attempts fail, look for the radii using the body frame
      // code in the NaifKeywords object.
      // Note: We will only look in the given label for the values after SPICELIB
      // routines have failed, to preserve backwards compatibility (since this
      // label check is new).
      if (cubeLab.hasObject("NaifKeywords")) {

        PvlObject naifKeywords = cubeLab.findObject("NaifKeywords");

        // Try using the target bodycode_RADII keyword in the NaifKeywords PVL object

        try {

          SpiceInt bodyCode = 0;
          try {
            // Try using the target bodycode_RADII keyword in the NaifKeywords PVL object
            bodyCode = lookupNaifBodyCode(target);
          }
          catch (IException &e2) {
            throw IException(e, IException::Unknown, e2.what(), _FILEINFO_);
          }
          std::string radiiKeyword = "BODY" + std::to_string(int(bodyCode)) + "_RADII";

          if (naifKeywords.hasKeyword(radiiKeyword)) {
            PvlKeyword radii =  naifKeywords.findKeyword(radiiKeyword);
            mapping.addKeyword( PvlKeyword("EquatorialRadius",
                                           std::to_string(std::stod(radii[0]) * 1000.0), "meters"),
                                PvlContainer::Replace);
            mapping.addKeyword( PvlKeyword("PolarRadius",
                                           std::to_string(std::stod(radii[2]) * 1000.0), "meters"),
                                PvlContainer::Replace);
            return mapping;
          }
        }
        catch (IException &e) {
        // Try using the value of the BODY_FRAME_CODE keyword in the NaifKeywords PVL object
          if (naifKeywords.hasKeyword("BODY_FRAME_CODE")) {

            PvlKeyword bodyFrame = naifKeywords.findKeyword("BODY_FRAME_CODE");
            QString radiiKeyword = "BODY" + QString::fromStdString(bodyFrame[0]) + "_RADII";

            if (naifKeywords.hasKeyword(radiiKeyword.toStdString())) {
              PvlKeyword radii =  naifKeywords.findKeyword(radiiKeyword.toStdString());
              mapping.addKeyword( PvlKeyword("EquatorialRadius",
                                             std::to_string(std::stod(radii[0]) * 1000.0),
                                             "meters"),
                                  PvlContainer::Replace);
              mapping.addKeyword( PvlKeyword("PolarRadius",
                                             std::to_string(std::stod(radii[2]) * 1000.0),
                                             "meters"),
                                  PvlContainer::Replace);
              return mapping;
            }
          }
        }
      }

      // If we get this far, we know the cube has no NaifKeywords object and previous attempts to
      // find radii in the mapping group or using spice IDs have failed
      QString msg = "Unable to find Equatorial and Polar radii for target [" + target + "].";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * Creates a Pvl Group with keywords TargetName, EquitorialRadius, and
   * PolarRadius. The values for the radii will be retrieved from the most
   * recent Target Attitude and Shape Naif kernel available in the Isis data
   * area.
   *
   * @param target The name of the body for which the radii will be retrieved.
   *
   * @throw IException::Io - "Could not convert target name to NAIF code."
   *
   * @return PvlGroup Group named "Mapping" with keywords TargetName,
   *             EquatorialRadius, and PolarRadius.
   */
  PvlGroup Target::radiiGroup(QString target) {

    if (target.isEmpty()) {
      throw IException(IException::Unknown,
                       "Unable to find TargetRadii. The given TargetName is empty.",
                       _FILEINFO_);
    }

    static QMap<QString, PvlGroup> cachedResults;

    PvlGroup mapping("Mapping");
    if (cachedResults.contains(target)) {
      mapping = cachedResults[target];
    }
    else {

      SpiceInt bodyCode = 0;
      try {
        bodyCode = lookupNaifBodyCode(target);
      }
      catch (IException &e) {
        QString msg = "Unable to find target radii for given target ["
                      + target + "].";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      PvlGroup radiiGroup = Target::radiiGroup(int(bodyCode));
      mapping += PvlKeyword("TargetName",  target.toStdString());
      mapping += radiiGroup.findKeyword("EquatorialRadius");
      mapping += radiiGroup.findKeyword("PolarRadius");
      cachedResults[target] = mapping;
    }

    return mapping;
  }


  /**
   * Convenience method called by the public radii() methods to
   * compute the target radii using a body code recognized by NAIF.
   *
   * The PVL group contains only the EquatorialRadius and PolarRadius
   * keywords. This group does not contain the Target keyword.
   *
   * @param bodyFrameCode A recognized NAIF code that represents the target body.
   *
   * @return PvlGroup containing EquatorialRadius and PolarRadius keywords.
   */
  PvlGroup Target::radiiGroup(int bodyCode) {

    // Load the most recent target attitude and shape kernel for NAIF
    static bool pckLoaded = false;

    NaifStatus::CheckErrors();

    FileName kern("$base/kernels/pck/pck?????.tpc");
    kern = kern.highestVersion();
    QString kernName = kern.expanded();

    if(!pckLoaded) {
      furnsh_c(kernName.toLatin1().data());
      pckLoaded = true;
    }

    // Get the radii from NAIF
    SpiceInt n;
    SpiceDouble radii[3];
    bodvar_c(bodyCode, "RADII", &n, radii);

    try {
      NaifStatus::CheckErrors();
    }
    catch (IException &e) {
      std::string msg = "Unable to find radii for target code [" + std::to_string(bodyCode)
                    + "]. Target code was not found in furnished kernels.";

      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    PvlGroup radiiGroup;
    radiiGroup += PvlKeyword("EquatorialRadius", std::to_string(radii[0] * 1000.0), "meters");
    radiiGroup += PvlKeyword("PolarRadius", std::to_string(radii[2] * 1000.0), "meters");

    return radiiGroup;

  }


  /**
   * This returns the NAIF body code of the target
   *
   * @return @b SpiceInt NAIF body code
   *
   */
  SpiceInt Target::naifBodyCode() const {
    return *m_bodyCode;
  }


  /**
   * This returns the NAIF planet system body code of the target
   *
   * @return @b SpiceInt NAIF system body code
   *
   * e.g. Enceladus is in the Saturn system
   *
   */
  SpiceInt Target::naifPlanetSystemCode() const {
    return *m_systemCode;
  }


  //! Return target name
  QString Target::name() const {
    return *m_name;
  }


  //! Return planet system name
  QString Target::systemName() const {
    return *m_systemName;
  }


  /**
   * Returns the radii of the body in km. The radii are obtained from the
   * appropriate SPICE kernel for the body specified by TargetName in the
   * Instrument group of the labels.
   */
  std::vector<Distance> Target::radii() const {
    return m_radii;
  }


  int Target::frameType() {
    return spice()->bodyRotation()->getFrameType();
  }


  std::vector<Angle> Target::poleRaCoefs() {
    return spice()->bodyRotation()->poleRaCoefs();
  }


  std::vector<Angle> Target::poleDecCoefs() {
    return spice()->bodyRotation()->poleDecCoefs();
  }


  std::vector<Angle> Target::pmCoefs() {
    return spice()->bodyRotation()->pmCoefs();
  }


  std::vector<double> Target::poleRaNutPrecCoefs() {
    return spice()->bodyRotation()->poleRaNutPrecCoefs();
  }


  std::vector<double> Target::poleDecNutPrecCoefs() {
    return spice()->bodyRotation()->poleDecNutPrecCoefs();
  }


  std::vector<double> Target::pmNutPrecCoefs() {
    return spice()->bodyRotation()->pmNutPrecCoefs();
  }


  std::vector<Angle> Target::sysNutPrecConstants() {
    return spice()->bodyRotation()->sysNutPrecConstants();
  }


  std::vector<Angle> Target::sysNutPrecCoefs() {
    return spice()->bodyRotation()->sysNutPrecCoefs();
  }


  /**
   * Restores the shape to the original after setShapeEllipsoid has overridden it.
   */
  void Target::restoreShape() {
    if (m_shape->name()  != "Ellipsoid") {
      // Nothing to do
      return;
    }
    // If we don't have a saved original shape, just stick to the current shape
    else if (m_originalShape == NULL) {
      // Nothing to do
      return;
    }
    m_shape = m_originalShape;
    m_originalShape = NULL;
  }


  /**
   * Set the shape to the ellipsoid and save the original shape.
   */
  void Target::setShapeEllipsoid() {
    // Save the current shape to restore later
    m_originalShape = m_shape;
    m_shape = new EllipsoidShape(this);
  }


  /**
   * Sets the radii of the body.
   *
   * @param r[] Radii of the target in kilometers
   */
  void Target::setRadii(std::vector<Distance> radii) {
    if (m_radii.size() < 3) {
      m_radii.resize(3, Distance());
    }
    m_radii[0] = radii[0];
    m_radii[1] = radii[1];
    m_radii[2] = radii[2];
  }


  /**
   * Set the name for the Target.
   * This function should be used if the target name is not available on the label
   * originally used to initialize the Target.
   *
   * @param name The new name of the Target
   */
  void Target::setName(QString name) {
    if (m_name == NULL) {
      m_name = new QString;
    }
    *m_name = name;
  }


  /**
   * Set the Spice pointer for the Target.
   * This function should be used if the Target was initialized without SPICE
   * data but is still needed by a sensor model.
   *
   * @param spice A pointer to the new Spice object
   */
  void Target::setSpice(Spice *spice) {
    m_spice = spice;
  }


  /**
   * Return the shape
   */
  ShapeModel *Target::shape() const {
    return m_shape;
  }


  /**
   * Return the spice object
   */
  Spice *Target::spice() const {
    return m_spice;
  }
}
