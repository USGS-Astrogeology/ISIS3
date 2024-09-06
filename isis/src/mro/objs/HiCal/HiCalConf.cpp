/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

#include <QString>

#include <SpiceUsr.h>

#include "Brick.h"
#include "Camera.h"
#include "Cube.h"
#include "FileName.h"
#include "HiCalConf.h"
#include "HiCalUtil.h"
#include "IString.h"
#include "IException.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

bool HiCalConf::_naifLoaded = false;

/**
 * @brief Default constructor for HiCalConf
 */
  HiCalConf::HiCalConf() : DbAccess() {
    _profName.clear();
    init();
  }


  /**
   * @brief Construct from a HiRISE label
   * @param label Label from HiRISE cube file
   */
  HiCalConf::HiCalConf(Pvl &label) : DbAccess() {
    _profName.clear();
    init(label);
  }

  /**
   * @brief Construct from HiRISE label and configuration file
   *
   * @param label Label from HiRISE cube file
   * @param conf Name of configuration file to use
   */
  HiCalConf::HiCalConf(Pvl &label, const QString &conf) :
       DbAccess(Pvl(filepath(conf).toStdString()).findObject("Hical", PvlObject::Traverse)) {
    _profName.clear();
    init(label);
  }

  /**
   * @brief Define label to initialize parameters from
   *
   * @param label Label from HiRISE cube file
   */
  void HiCalConf::setLabel(Pvl &label) {
    init(label);
    return;
  }

  /**
   * @brief Resolve a file path validating existance
   *
   * This method determines if a filepath is versioned (as indicated with one or
   * more '?') and returns the expanded file name \b only.  It does not expand
   * the path portion of the filespec.  This is so to make use of the results of
   * this method in reporting files in labels...to make it tidy.
   *
   * @param fname  File specification with or without versioning patterns
   *
   * @return QString Expanded filename but not the filepath
   */
  QString HiCalConf::filepath(const QString &fname) const {
    FileName efile(fname);
    if (efile.isVersioned()) {
      QString path(efile.originalPath());
      if (!path.isEmpty()) path += "/";

      efile = efile.highestVersion();

      return (path + efile.name());
    }
    return (fname);
  }


  /**
   * @brief Establish the configuration file used for calibration parameters
   *
   * This file can be established at any point in the processing as parameters
   * are resolved as needed (lazy instantiation).  One must be established
   * before any calibration can take place.
   *
   * @param conf Name of configuration file to use
   */
  void HiCalConf::setConf(const QString &conf) {
    load((Pvl(filepath(conf).toStdString()).findObject("Hical", PvlObject::Traverse)));
  }

  /**
   * @brief Selects a profile other than the default
   *
   * Use of this method is to explicitly select a named profile in the
   * configuration file.  If this is used, additional profile options are not
   * loaded.
   *
   * @param profile Name of existing profile in the configuration file
   */
  void HiCalConf::selectProfile(const QString &profile) {
    _profName = profile;
    return;
  }

  /**
   * @brief Returns the selected profile name
   *
   * This method returns the selected profile name.
   *
   * @return QString Selected profile name
   */
  QString HiCalConf::getProfileName() const {
    return (_profName);
  }

  /**
   * @brief Returns the named expanded matrix file reference
   *
   * This method returns the name of a matrix file reference variable from the
   * \b Matrices keyword as determined from the fully option profile. It then
   * expands the filename portion (not the filepath!) and returns the result as
   * a QString for subsequent use.  See also getMatrixList and getMatrix.
   *
   * @param name  Name of Matrix to resolve.  It must be one in the \b Matrices
   *              keyword from the configuration file
   *
   * @return QString The expanded file name reference for the matrix
   */
  QString HiCalConf::getMatrixSource(const QString &name) const {
    return (getMatrixSource(name, getMatrixProfile()));
  }

   /**
   * @brief Returns the named expanded matrix file reference
   *
   * This method returns the name of a matrix file reference variable from the
   * \b Matrices keyword as determined from the specified profile. It then
   * expands the filename portion (not the filepath!) and returns the result as
   * a QString for subsequent use.  See also getMatrixList and getMatrix.
   *
   * @param name  Name of Matrix to resolve.  It must be one in the \b Matrices
   *              keyword from the configuration file
   * @param matconf Profile to extract the named matrix source from
   *
   * @return QString The expanded file name reference for the matrix
   */
  QString HiCalConf::getMatrixSource(const QString &name,
                                         const DbProfile &matconf) const {

    QString mfile = parser(matconf.value(name),
                               getList(matconf,"OptionKeywords"),
                               matconf);

//  Translate and return
    return (filepath(mfile));
  }

  HiVector HiCalConf::getMatrix(const QString &name,
                                int expected_size) const {
    return (getMatrix(name,getMatrixProfile(), expected_size));
  }


  QString HiCalConf::resolve(const QString &composite,
                                 const DbProfile &matconf) const {
    return (parser(composite,getList(matconf,"OptionKeywords"), matconf));
  }



  /**
   * @brief Returns the named matrix from the specified file reference
   *
   * The matrix specified in the name parameter must exist in the \b Matrices
   * keyword in the \b Hical object of the configuration file.  A fully option
   * profile is created within this method and the keyword of the named variable
   * is retreived from it.  The fully qualified filepath is generated to
   * finally resolve to a multi-band, single line, Isis cube file.  A specific
   * band is extracted from the cube file as determined by the CCD channel
   * (provide by the getMatrixBand method) and returned in a HiVector data
   * array.
   *
   * Note that the caller can specify the number of samples expected from the
   * matrix cube in the expected_size parameter if it is non-zero.  Once the
   * matrix cube is opened, the number of samples is checked against this
   * parameter and if they do not match, an exception is thrown.
   *
   * @param name Name of matrix to retreive
   * @param profile Specfied profile providing all parameters
   * @param expected_size Expected number of samples in the matrix cube
   *
   * @return HiCalConf::HiVector Returns the extracted band from the cube
   */
  HiVector HiCalConf::getMatrix(const QString &name,
                                const DbProfile &profile,
                                int expected_size) const {

    QString mfile = getMatrixSource(name, profile);

// Crack the file and read the appropriate band
    Cube cube;
    cube.open(mfile);

//  Check for proper size if specifeid
    if (expected_size != 0) {
      if (cube.sampleCount() != expected_size) {
        ostringstream mess;
        mess << "Specifed matrix  (" << name
             << ") from file \"" << mfile
             << "\" does not have expected samples (" << expected_size
             << ") but has " << cube.sampleCount();
        cube.close();
        throw IException(IException::User, mess.str(), _FILEINFO_);
      }
    }

//  Read the specifed region
    Brick bandio(cube.sampleCount(), 1, 1, Real);
    bandio.SetBasePosition(1,1,getMatrixBand(profile));
    cube.read(bandio);

//  Now create the output buffer with some TNT funny business
    HiVector temp(cube.sampleCount(), bandio.DoubleBuffer());
    HiVector out(cube.sampleCount());
    out.inject(temp);
    cube.close();
    return (out);
  }

  /**
   * @brief Returns a named scalar constant
   *
   * This method returns a named scalar constant parameter retrieved from the
   * configuration file through a fully optioned profile.  This keyword does not
   * necessarily have to exist in the \b Scalars keyword in the \b Hical object,
   * but is required to be a floating point value (or values).  The result is
   * returned as a HiVector data array.
   *
   * @param name Name of scalar constant to return
   * @param expected_size Expected size of constant if non-zero
   *
   * @return HiCalConf::HiVector Values of scalar constants
   */
  HiVector HiCalConf::getScalar(const QString &name,
                                const DbProfile &profile,
                                int expected_size) const {
    int nvals = profile.count(name);

    //  Check for proper size if specifeid
    if (expected_size != 0) {
      if (nvals != expected_size) {
        ostringstream mess;
        mess << "Specifed scalar (" << name
             << ") does not have expected size (" << expected_size
             << ") but has " << nvals;
        throw IException(IException::User, mess.str(), _FILEINFO_);
      }
    }
//  All is OK
    HiVector mtx(nvals, Null);
    for (int i = 0  ; i < nvals ; i++) {
      mtx[i] = ToDouble(profile(name, i));
    }
    return (mtx);
  }

  /**
   * @brief Computes the distance from the Sun to the observed body
   *
   * This method requires the appropriate NAIK kernels to be loaded that
   * provides instrument time support, leap seconds and planet body ephemeris.
   *
   * @return double Distance in AU between Sun and observed body
   */
  double HiCalConf::sunDistanceAU(Cube *cube) {
    double sunkm = 0.0;
    try {
      Camera *cam;
      cam = cube->camera();
      cam->SetImage(0.5, 0.5);
      sunkm = cam->sunToBodyDist();
      NaifStatus::CheckErrors();
    }
    catch (IException &e) {
      try {
        loadNaifTiming();

        QString scStartTime = QString::fromStdString(getKey("SpacecraftClockStartCount", "Instrument"));
        double obsStartTime;
        NaifStatus::CheckErrors();
        scs2e_c (-74999,scStartTime.toLatin1().data(),&obsStartTime);

        QString targetName = QString::fromStdString(getKey("TargetName", "Instrument"));
        if (targetName.toLower() == "sky" ||
            targetName.toLower() == "cal" ||
            targetName.toLower() == "phobos" ||
            targetName.toLower() == "deimos") {
          targetName = "Mars";
        }
        double sunv[3];
        double lt;
        (void) spkpos_c(targetName.toLatin1().data(), obsStartTime, "J2000", "LT+S", "sun",
                        sunv, &lt);
        sunkm = vnorm_c(sunv);

        NaifStatus::CheckErrors();
      }
      catch(IException &e) {
        std::string msg = "Unable to determine the distance from the target to the sun";
        throw IException(e, IException::User, msg, _FILEINFO_);
      }
    }

    //  Return in AU units
    return (sunkm/1.49597870691E8);
  }


  /**
   * @brief Returns the band number of a Matrix file given CCD, Channel number
   *
   * The ordering of HiRISE Isis matrix cubes are required to be ordered by CCD
   * and channel number.  The band number is computed as:
   * @code
   * band =  1 + ccd * 2 + channel
   * @endcode
   *
   * Note that this method extracts the values from a stored cube label.  It
   * must be initialized with a label or an error will occur.
   *
   * @return int Band number of matrix cube
   */
  int HiCalConf::getMatrixBand() const {
    Pvl label = _label;
    DbProfile parms = makeParameters(label);
    return (getMatrixBand(parms));
  }

  /**
   * @brief Returns the band number of a Matrix file from a profile
   *
   * The ordering of HiRISE Isis matrix cubes are required to be ordered by CCD
   * and CHANNEL number.  The band number is computed as:
   * @code
   * band =  1 + ccd * 2 + channel
   * @endcode
   *
   * @param  DbProfile Profile to get CCD and CHANNEL values from
   * @return int Band number of matrix cube
   */

  int HiCalConf::getMatrixBand(const DbProfile &p) const {
    return (getChannelIndex(ToInteger(p("CCD")), ToInteger(p("CHANNEL"))));
  }

  /**
   * @brief Generic profile keyword value extractor
   *
   * This method retrieves a profile keyword from the given profile and returns
   * all its values a a list of QStrings.  An exception will be thrown
   * incidentally if the keyword does not exist.
   *
   * @param profile Profile containing the keyword to extract
   * @param key  Name of keyword to retrieve
   *
   * @return std::vector<QString> List of values from profile keyword
   */
  HiCalConf::ValueList HiCalConf::getList(const DbProfile &profile,
                                          const QString &key) const {
    ValueList slist;

//  Get keyword parameters
    if ( profile.exists(key) ) {
      int nvals = profile.count(key);
      for (int i = 0 ; i < nvals ; i++) {
        slist.push_back(profile.value(key, i));
      }
    }
    return (slist);
  }

/**
 * @brief Load required NAIF kernels required for timing needs
 *
 * This method maintains the loading of kernels for HiRISE timing and planetary
 * body ephemerides to support time and relative positions of planet bodies.
 */
void HiCalConf::loadNaifTiming( ) {
  NaifStatus::CheckErrors();
  if (!_naifLoaded) {
//  Load the NAIF kernels to determine timing data
    Isis::FileName leapseconds("$base/kernels/lsk/naif????.tls");
    leapseconds = leapseconds.highestVersion();

    Isis::FileName sclk("$mro/kernels/sclk/MRO_SCLKSCET.?????.65536.tsc");
    sclk = sclk.highestVersion();

    Isis::FileName pck("$base/kernels/spk/de???.bsp");
    pck = pck.highestVersion();

    Isis::FileName sat("$base/kernels/spk/mar???.bsp");
    sat = sat.highestVersion();

//  Load the kernels
    QString lsk = leapseconds.expanded();
    QString sClock = sclk.expanded();
    QString pConstants = pck.expanded();
    QString satConstants = sat.expanded();
    furnsh_c(lsk.toLatin1().data());
    furnsh_c(sClock.toLatin1().data());
    furnsh_c(pConstants.toLatin1().data());
    furnsh_c(satConstants.toLatin1().data());
    NaifStatus::CheckErrors();

//  Ensure it is loaded only once
    _naifLoaded = true;
  }
  return;
}

/**
 * @brief Intialization of object variables
 */
void HiCalConf::init() {
  _label.clear();
  return;
}

/**
 * @brief Initialization of object using HiRISE label
 *
 * This method initializes the object from a HiRISE label.  Note that a copy of
 * the label is created so that subsequent operations can be supported and it is
 * not dependant upon callers behaviour.
 *
 * @param label Pvl label of HiRISE cube
 */
void HiCalConf::init(Pvl &label) {
  init();
  _label = label;
  return;
}

/**
 * @brief Get a label keyword
 *
 * Retreives a keyword from a HiRISE label and returns a reference to it.  If it
 * does not exist, an exception is thrown incidentally.
 *
 * @param key Name of keyword in label to retrieve
 * @param group Optional group name to use if non-empty.
 *
 * @return PvlKeyword& Reference to retrieved label keyword
 */
PvlKeyword &HiCalConf::getKey(const QString &key,
                              const QString &group) {
  if (!group.isEmpty()) {
    PvlGroup &grp = _label.findGroup(group.toStdString(), Pvl::Traverse);
    return (grp[key.toStdString()]);
  }
  else {
    return (_label.findKeyword(key.toStdString()));
  }
}

/**
 * @brief Returns the Matrix profile
 *
 * This method constructs a fully optioned matrix profile from the
 * configuration file.  If the caller has designated a specific named profile,
 * the options profiles are not loaded.
 *
 * Options profiles are read from the \b ProfileOptions configuration keyword
 * and resolved through pattern replacement of FILTER, TDI, BIN, CCD and CHANNEL
 * values are determined from the HiRISE label.  If named profiles exist after
 * the textual substitutions of these values they are added to the base default
 * profile.  This process will replace existing default keywords of the same
 * name in subseqent optional keywords.
 *
 * @param profile Optional name of the profile to retrieve.  It will use the
 *                default profile if this parameter is not provided (empty).
 * @return DbProfile  Returns a fully optioned profile
 */
DbProfile HiCalConf::getMatrixProfile(const QString &profile) const {
  QString myprof = (!profile.isEmpty()) ? profile : _profName;
  DbProfile matconf = getProfile(myprof);
  if (!matconf.isValid()) {
    ostringstream mess;
    mess << "Specifed matrix profile (" << matconf.Name()
         << ") does not exist or is invalid!";
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }

  //  Profile the label and merge them.  Order is important.
  matconf = DbProfile(getLabelProfile(matconf), matconf, matconf.Name());

  //  Add special parameters.  Again, order is important.
  matconf = DbProfile(matconf, makeParameters(matconf), matconf.Name());

//  Load any optional ones
  ValueList profkeys = getList(matconf,"OptionKeywords");
  ValueList proforder = getList(matconf,"ProfileOptions");
  QString pName(matconf.Name());
  for (unsigned int i = 0 ; i < proforder.size() ; i++) {
    QString profile = parser(proforder[i], profkeys, matconf);
    if (profileExists(profile)) {
      pName += "+[" + profile + "]";
      matconf = DbProfile(matconf,getProfile(profile), pName);
    }
  }
  return (matconf);
}


DbProfile HiCalConf::getLabelProfile(const DbProfile &profile) const {
  DbProfile lblprof("Label");
  if ( profile.exists("LabelGroups") ) {
    int ngroups = profile.count("LabelGroups");
    Pvl label = _label;
    for ( int g = 0 ; g < ngroups ; g++ ) {
      QString group = profile("LabelGroups", g);
      PvlGroup grp = label.findGroup(group.toStdString(), Pvl::Traverse);
      lblprof = DbProfile(lblprof,DbProfile(grp));
    }
  }
  return (lblprof);
}

int HiCalConf::getChannelIndex(const int &ccd, const int &channel) const {
  return(1 + (ccd*2) + channel);
}

DbProfile HiCalConf::makeParameters(Pvl &label) const {
  PvlGroup inst = label.findGroup("Instrument", Pvl::Traverse);
  DbProfile parms("Parameters");

  int ccd = CpmmToCcd((int) inst["CpmmNumber"]);
  int channel = inst["ChannelNumber"];
  parms.add("CCD",ToString(ccd));
  parms.add("CHANNEL", ToString(channel));
  parms.add("TDI", QString::fromStdString(inst["Tdi"]));
  parms.add("BIN", QString::fromStdString(inst["Summing"]));
  parms.add("FILTER", CcdToFilter(ccd));
  parms.add("CCDCHANNELINDEX", ToString(getChannelIndex(ccd, channel)));
  return (parms);
}

DbProfile HiCalConf::makeParameters(const DbProfile &profile) const {
  DbProfile parms("Parameters");
  int ccd = CpmmToCcd(ToInteger(profile("CpmmNumber")));
  int channel = ToInteger(profile("ChannelNumber"));
  parms.add("CCD",ToString(ccd));
  parms.add("CHANNEL", ToString(channel));
  parms.add("TDI", profile("Tdi"));
  parms.add("BIN", profile("Summing"));
  parms.add("FILTER", CcdToFilter(ccd));
  parms.add("CCDCHANNELINDEX", ToString(getChannelIndex(ccd, channel)));
  return (parms);
}

QString  HiCalConf::makePattern(const QString &str) const {
  return (QString("{" + str + "}"));
}

/**
 * @brief Performs a search and replace operation for the given QString
 *
 * This method will search the input QString s for predefined keywords (FILTER,
 * TDI, BIN, CCD and CHANNEL) delimited by {} and replace these occurances with
 * the textual equivalent.
 *
 * @param s String to conduct search/replace of values
 *
 * @return QString  Results of search/replace
 */
QString HiCalConf::parser(const QString &s, const ValueList &vlist,
                          const DbProfile &prof) const {
    QString sout(s);

    ValueList::const_iterator ciVlist;
    for ( ciVlist = vlist.begin() ; ciVlist != vlist.end() ; ++ciVlist ) {
      QString str(*ciVlist);
      if ( prof.exists(str) ) {
        sout = sout.replace(makePattern(str), prof(str));
      }
    }

    return (sout);
}

}  // namespace Isis
