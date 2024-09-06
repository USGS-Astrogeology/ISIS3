#ifndef Spice_h
#define Spice_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <QVariant>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Pvl.h"
#include "ShapeModel.h"
#include "SpicePosition.h"
#include "SpiceRotation.h"

namespace Isis {
  class Cube;
  class iTime;
  class Distance;
  class EllipsoidShape;
  class Longitude;
  class Target;

  /**
   * @brief Obtain SPICE information for a spacecraft
   *
   * This class initializes standard NAIF SPICE kernels in order to allow queries
   * of a spacecraft's position and attitude at a given time. It also allows for
   * access to the position of the sun and transformation matrices from J2000 to a
   * body-fixed reference frame for a given target (e.g., Mars). The constructor
   * for this class expects a PVL object with the following minimum information:
   *   @code
   *     Group = Instrument
   *       TargetName = Mars
   *     EndGroup
   *     Group = Kernels
   *       NaifFrameCode       = -94030
   *       LeapSecond          = naif0007.tls
   *       TargetAttitudeShape = pck00006.tpc
   *       TargetPosition      = de405.bsp
   *       InstrumentPointing  = (mgs_sc_ab1.bc,
   *       Instrument          = moc13.ti
   *       SpacecraftClock     = MGS_SCLKSCET.00045.tsc
   *       InstrumentPosition  = mgs_ab1.bsp
   *       InstrumentAddendum  = mocAddendum.ti
   *     EndGroup
   *   @endcode
   * This group is typically found in the image labels after it has been run
   * through the program "spiceinit" It is recommended you read NAIF documentation
   * to obtain a better understanding about the various types of SPICE kernels.
   * The NAIF toolkit accesses information from kernels on a
   * last-in-first-out (LIFO) basis. This means that the creation of a second
   * object can cause problems with the first object. To alleviate this problem we
   * have supplied the CreateCache method which should be invoked immediately
   * after the object is constructed. This caches information (spacecraft
   * position, pointing, etc) internally in the object and unloads all NAIF
   * kernels.
   *
   * @ingroup SpiceInstrumentsAndCameras
   *
   * @author 2003-03-13 Jeff Anderson
   *
   * @internal
   *   @history 2003-05-16 Stuart Sides - Modified schema from astrogeology...
   *                           isis.astrogeology...
   *   @history 2003-10-15 Jeff Anderson - Added requirement for frame kernel in
   *                           labels
   *   @history 2003-10-28 Jeff Anderson - Changed SpaceCraft to Spacecraft in
   *                           labels and method names
   *   @history 2003-11-03 Jeff Anderson - Added SubSolarPoint and
   *                           SubSpacecraftPoint methods
   *   @history 2003-11-12 Jeff Anderson - Added Target method
   *   @history 2004-01-14 Jeff Anderson - Changed how the SPK, CK, and
   *                           Instrument codes where handled. The instrument
   *                           code must be in the labels as NaifFrameCode and
   *                           then the other two can be automatically computed.
   *   @history 2004-02-18 Jeff Anderson - Modified to ignore kernel labels which
   *                           were blank
   *   @history 2004-03-25 Jeff Anderson - Modified NaifBodyCode method to convert
   *                           Jupiter target code of 599 to 5
   *   @history 2004-03-25 Jeff Anderson - Fixed bug in destructor and added
   *                           GetString method.
   *   @history 2005-02-15 Elizabeth Ribelin - Modified file to support Doxygen
   *                           documentation
   *   @history 2005-02-24 Jeff Anderson - Modified SubSolarPoint and
   *                           SubSpacecraftPoint to return positive
   *                           longitudes only
   *   @history 2005-09-12 Jeff Anderson - Check for case-insensitive values for
   *                           TargetName of SKY
   *   @history 2005-09-20 Jeff Anderson - Added IsSky method
   *   @history 2006-01-05 Debbie A. Cook - Added units to comments
   *   @history 2006-03-28 Jeff Anderson - Refactored using SpiceRotation and
   *                           SpicePosition classes.  Added Tables
   *                           and nadir kernel information.
   *   @history 2006-03-31 Elizabeth Miller - Added TargetCenterDistance method
   *   @history 2006-04-19 Elizabeth Miller - Added SolarLongitude method
   *   @history 2007-01-30 Tracie Sucharski - Throw error in the load method before
   *                           calling furnish if the file does not exist.
   *   @history 2007-07-09 Steven Lambright - Frame kernel is now optional,
   *                           added Extra kernel support.
   *   @history 2007-07-10 Debbie A. Cook - Modified method
   *                           ComputeSolarLongitude to use pxform instead of
   *                           tipbod to get body-fixed to J2000 rotation matrix
   *                           so that the correct frame will be used. If the
   *                           frame is different from the default IAU frame,
   *                           the correct frame should be set in the iak file
   *                           (see frames.req).  Also modified setting of
   *                           m_bodyRotation frameCode.  The old code forced
   *                           the IAU_ frame.  The new code uses the Naif
   *                           routine cidfrm to get the frame associated with
   *                           the body id.  These change will recognize any
   *                           frame changes made in the iak file.
   *   @history 2007-08-10 Steven Lambright - Added support for Nadir keyword in
   *                           InstrumentPointing group to not be the first
   *                           element in the PvlKeyword.
   *   @history 2007-08-24 Debbie A. Cook - Removed p_sB so it is recalculated
   *                           every time it is used insuring that any updates
   *                           to the position or rotation are applied. Also
   *                           removed p_BP since it is no longer used
   *   @history 2008-02-13 Steven Lambright - Added StartPadding and EndPadding
   *                           caching capabilties
   *   @history 2008-02-13 Steven Lambright - Added Support Check for
   *                           StartPadding and EndPadding caching capabilties;
   *                           An clarified exception is thrown if a framing
   *                           camera tries to use time padding
   *   @history 2008-02-27 Kris Becker - Modified so that planetary ephemeris SPKs
   *                           are loaded before spacecraft SPKs so that
   *                           missions that augment planet ephemerides will
   *                           take precidence.
   *   @history 2008-06-23 Steven Lambright - Added NaifStatus error checking
   *   @history 2008-06-25 Debbie A. Cook - Added method InstrumentVelocity to
   *                           support miniRF
   *   @history 2008-11-28 Debbie A. Cook - Added method hasKernels()
   *   @history 2009-03-18 Tracie Sucharski - Cleaned up some unnecessary,
   *                           obsolete code.  Make sure the table is used if
   *                           the kernel names follow the "Table" keyword
   *                           value, due to change made to spiceinit to retain
   *                           kernel names if the spice is written to blob.
   *   @history 2009-06-18 Debbie A. Cook - Modified to downsize instrument
   *                           rotation table when loading cache
   *   @history 2009-07-01 Debbie A. Cook - Modified to downsize body rotation,
   *                           and sun position tables when loading cache
   *   @history 2009-08-03 Debbie A. Cook - Added tolerance argument to method
   *                           CreateCache to allow downsizing of instrument
   *                           position Spice table.
   *   @history 2009-08-21 Kris Becker - Moved the NAIF code methods to public
   *                           scope.
   *   @history 2010-01-29 Debbie A. Cook - Redid Tracie's change to make sure
   *                           the table is loaded instead of the kernels if the
   *                           kernel keyword value lists "Table" before the
   *                           kernel files.
   *   @history 2010-03-19 Debbie A. Cook - Added constructor and moved common
   *                           constructor initialization into new method Init.
   *                           Also added parameter notab to method Load.
   *   @history 2010-04-09 Debbie A. Cook - Moved the loading of the "extra"
   *                           kernel(s) from the middle of the loads to the end.
   *   @history 2011-02-08 Jeannie Walldren - Added documentation to methods and
   *                           private variables. Commented out
   *                           createCache(double,double) since it appears that
   *                           this method is not needed. Initialize pointers to
   *                           NULL in Init() method.
   *   @history 2011-02-09 Steven Lambright - Refactored to use iTime where
   *                           possible. Changed p_radii to a Distance so the
   *                           units are no longer ambiguous. These changes were
   *                           meant for readability and reducing the likelyhood
   *                           of future code having bugs due to unit mismatches.
   *   @history 2011-02-11 Jeannie Walldren - Changed documentation references to
   *                           SetEphemerisTime() method (these were replaced
   *                           with references to setTime()). Added missing
   *                           documentation to new methods.
   *   @history 2011-05-03 Jeannie Walldren - Added Isis Disclaimer to files.
   *   @history 2011-05-25 Janet Barrett and Steven Lambright - Added API that
   *                           stores naif values and arbitrary computations so
   *                           that the text kernels do not have to be
   *                           furnished. This makes the Camera instantiation
   *                           much, much quicker. Text kernels are no longer
   *                           furnished when their data has been stored in the
   *                           labels.
   *   @history 2011-05-26 Debbie A. Cook - Put back the code for spkwriter
   *                           that was checked in May 25 but disappeared in the
   *                           May 26 build.  This code turns aberration
   *                           corrections off for the instrument position if
   *                           the spk file was created by spkwriter.
   *   @history 2011-07-08 Jeff Anderson - Fixed Init method to record the
   *                           integer body frame code in the labels of the
   *                           cube. Vesta exposed this problem because it was
   *                           not a instrinsic body in the NAIF toolkit
   *                           version 63.
   *   @history 2011-07-11 Jeff Anderson - Added private copy constructors and
   *                           operator= methods
   *   @history 2011-09-19 Debbie Cook - Added cubes with Ideal Cameras to the
   *                           exclusion list for reading instrument keywords
   *                           from the label.  The Ideal Camera has variable
   *                           values for the affine coefficients that are set
   *                           in the camera itself and not read from a kernel.
   *                           The camera puts these values into the Naif kernel
   *                           pool.
   *   @history 2012-07-06 Debbie A. Cook, Updated Spice members to be more
   *                           compliant with Isis coding standards. References
   *                           #972.
   *   @history 2012-09-10 Steven Lambright - Undid Debbie's change from
   *                           2011-09-19 because the Ideal camera now supports
   *                           putting those keywords in the label on the fly.
   *                           References #1094.
   *   @history 2012-10-11 Debbie A. Cook - Deleted deprecated createCache code
   *                           already commented out for over a year.  Updated
   *                           to use new Target and ShapeModel classes. Added
   *                           resolution method needed for Target and its
   *                           ShapeModel. Changed private member names from
   *                           p_ to m_ to comply with coding standards.
   *                           References Mantis tickets #775 and #1114.
   *   @history 2012-10-25 Jeannie Backer - Added accesssor method to return the
   *                           BODY_FRAME_CODE value found in the init() method.
   *                           Improved unitTest coverage in all areas. New
   *                           method has 100% scope, line, function coverage.
   *                           Improved padding around control statements  and
   *                           indentation of history entries to be more
   *                           compliant with standards. Moved accessor method
   *                           implementations to cpp file. Changed Resolution()
   *                           method to lower camel case. Added documentation.
   *                           Fixes #1181.
   *   @history 2012-10-31 Kris Becker - Added implementation for swapping of
   *                           observer/target and light time correction to
   *                           surface. Fixes (mostly) #0909, #1136 and #1223.
   *   @history 2012-12-10 Kris Becker - A newly designed class,
   *                           SpacecraftPosition, is now being instantiated
   *                           instead of SpicePosition specifically to properly
   *                           order NAIF observer and target codes (to more
   *                           accurately determine the target body position at
   *                           the time the observation was initiated) and help
   *                           determine light time correction to the surface of
   *                           the target body instead of the center of the body.
   *                           See the documention in SpacecraftPosition.h for a
   *                           more detailed description of these changes.
   *                           References #909, #1136 and #1223.
   *   @history 2013-01-09 Steven Lambright and Mathew Eis - Fixed a possible crash
   *                           condition that never exhibited any problems, but valgrind
   *                           caught it. This was discovered when adding support for Mac
   *                           OSX 10.8. References #1354.
   *   @history 2013-09-26 Tracie Sucharski - If the Target is Saturn and the shape model
   *                           is the ring plane, load an extra kernel, saturnRings_v001.tpc,
   *                           which changes the prime meridian to {0, 0, 0}.  This insures
   *                           the longitude values are calculated in the inertial system
   *                           relative to the ascending node of the ring plane.  Fixes #1757.
   *   @history 2013-12-17 Janet Barrett - Added the instrumentBodyFixedPosition and
   *                           instrumentBodyFixedVelocity methods. Fixes #1684.
   *   @history 2015-04-30 Stuart - Added an error check around the NAIF SPICE call scs2e_c in
   *                           getClockTime. Avoids a segfault. Fixes #2247.
   *   @history 2015-07-21 Kristin Berry - Added additional NaifStatus::CheckErrors() to see if any
   *                           NAIF errors were signaled. References #2248.
   *   @history 2016-05-18 Jeannie Backer and Stuart Sides - Moved the construction of the Target
   *                           after the NAIF kernels have been loaded or the NAIF keywords
   *                           have been pulled from the cube labels, so we can find target
   *                           body codes that are defined in kernels and not just body codes
   *                           build into spicelib. References #3934
   *   @history 2016-10-19 Kristin Berry - Added exception to Spice::time() to throw if m_et is
   *                           NULL. Also added isTimeSet(), a function that will return true if
   *                           m_et is set. References #4476.
   *   @history 2016-10-21 Jeannie Backer - Reorder method signatures and member variable
   *                           declarations to fit ISIS coding standards. References #4476.
   *   @history 2018-06-07 Debbie A Cook - Added BODY_CODE to Naif keywords.  This code
   *                           is used in the target body radii keyword name.  Isis retrieves this code
   *                           from the standard PCK.  Because target bodies new to Naif are not
   *                           included in the standard PCK, missions create a special body-specific
   *                           PCK to define the new body, including its body code.  This PCK is only
   *                           loaded in spiceinit so the code needs to be saved so that the radii
   *                           keyword can be created to retrieve the target radii.
   *  @history 2019-04-16 Kristin Berry - Added a parameter to getClockTime called clockTicks which
   *                           defaults to false. When set to true, this indicates that the input value
   *                           is in encoded clock ticks, rather than a full spacecraft clock time
   *                           string. As such, when used sct2e_c is used to convert to an ET rather
   *                           than scs2e_c.
   *  @history 2021-02-17 Kristin Berry, Jesse Mapel, and Stuart Sides - Made several methods virtual,
   *                           moved several member variables to protected, and added initialization
   *                           path for a sensor model without SPICE data.
   */
  class Spice {
    public:
      // constructors
      Spice(Cube &cube);
      Spice(Pvl &lab, nlohmann::json);

      // destructor
      virtual ~Spice();

      // Methods
      virtual void setTime(const iTime &time);
      void instrumentPosition(double p[3]) const;
      virtual void instrumentBodyFixedPosition(double p[3]) const;
      virtual void sunPosition(double p[3]) const;
      virtual double targetCenterDistance() const;
      virtual double sunToBodyDist() const;

      virtual Longitude solarLongitude();
      virtual void instrumentBodyFixedVelocity(double v[3]) const;
      virtual iTime time() const;

      void radii(Distance r[3]) const;

      virtual void createCache(iTime startTime, iTime endTime,
                               const int size, double tol);
      virtual iTime cacheStartTime() const;
      virtual iTime cacheEndTime() const;

      virtual void subSpacecraftPoint(double &lat, double &lon);
      virtual void subSolarPoint(double &lat, double &lon);

      virtual Target *target() const;
      QString targetName() const;

      virtual iTime getClockTime(QString clockValue,
                                 int sclkCode = -1,
                                 bool clockTicks=false);
      SpiceDouble getDouble(const QString &key, int index = 0);
      SpiceInt getInteger(const QString &key,   int index = 0);
      QString getString(const QString &key,     int index = 0);

      virtual SpicePosition *sunPosition() const;
      virtual SpicePosition *instrumentPosition() const;
      virtual SpiceRotation *bodyRotation() const;
      virtual SpiceRotation *instrumentRotation() const;

      bool isUsingAle();
      bool hasKernels(Pvl &lab);
      bool isTimeSet();

      SpiceInt naifBodyCode() const;
      SpiceInt naifSpkCode() const;
      SpiceInt naifCkCode() const;
      SpiceInt naifIkCode() const;
      SpiceInt naifSclkCode() const;
      SpiceInt naifBodyFrameCode() const;

      PvlObject getStoredNaifKeywords() const;
      virtual double resolution();

    protected:
      /**
       * NAIF value primitive type
       *
       */
      enum SpiceValueType {
        SpiceDoubleType,  //!< SpiceDouble type
        SpiceStringType,  //!< SpiceString type
        SpiceIntType,     //!< SpiceInt type
        SpiceByteCodeType //!< SpiceByteCode type
      };

      QVariant readValue(QString key, SpiceValueType type, int index = 0);

      void storeResult(QString name, SpiceValueType type, QVariant value);
      QVariant getStoredResult(QString name, SpiceValueType type);

      void storeValue(QString key, int index, SpiceValueType type,
                      QVariant value);
      QVariant readStoredValue(QString key, SpiceValueType type, int index);
      virtual void computeSolarLongitude(iTime et);

      // Leave these protected so that inheriting classes don't
      // have to convert between double and spicedouble
      // None of the below data elements are usable (except
      // m_radii) until SetEphemerisTime is invoked
      SpiceDouble m_uB[3];    /**< This contains the sun position (u) in the
                                  bodyfixed reference frame (B). It is left
                                  protected so that conversions between double
                                  and SpiceDouble do not have to occur in
                                  inheriting classes. Units are km */
      SpiceDouble m_BJ[3][3]; /**< This contains the transformation matrix from
                                  J2000 (J) to Body fixed (B). Recall that the
                                  transpose of this matrix JB will convert from
                                  body-fixed to J2000. It is left in protected
                                  space so that conversions between double and
                                  SpiceDouble do not have to occur in inheriting
                                  classes.*/
      Target *m_target; //!< Target of the observation
      iTime *m_et; //!< Ephemeris time (read NAIF documentation for a detailed description)
      Longitude *m_solarLongitude; //!< Body rotation solar longitude value

    private:
      // Don't allow copies
      Spice(const Spice &other);
      Spice &operator=(const Spice &other);

      void init(Pvl &pvl, bool noTables, nlohmann::json isd = NULL);
      void csmInit(Cube &cube, Pvl label);
      void defaultInit();

      void load(PvlKeyword &key, bool notab);

      QVector<QString> * m_kernels; //!< Vector containing kernels filenames

      // cache stuff
      iTime *m_startTime; //!< Corrected start (shutter open) time of the observation.
      iTime *m_endTime; //!< Corrected end (shutter close) time of the observation.
      SpiceDouble *m_cacheSize; //!< Cache size.  Note:  This value is 1 for Framing cameras.

      SpiceDouble *m_startTimePadding; //!< Kernels pvl group StartPadding keyword value
      SpiceDouble *m_endTimePadding; //!< Kernels pvl group EndPadding keyword value

      SpicePosition *m_instrumentPosition; //!< Instrument spice position
      SpiceRotation *m_instrumentRotation; //!< Instrument spice rotation
      SpicePosition *m_sunPosition; //!< Sun spice position
      SpiceRotation *m_bodyRotation; //!< Body spice rotation

      bool m_allowDownsizing; //!< Indicates whether to allow downsizing

      // Constants
      //      SpiceInt *m_bodyCode;        /**< The NaifBodyCode value, if it exists in the
      //                                        labels. Otherwise, if the target is sky,
      //                                        it's the SPK code and if not sky then it's
      //                                        calculated by the naifBodyCode() method.*/
      SpiceInt *m_spkCode;         //!< Spacecraft and planet ephemeris kernel (SPK) code
      SpiceInt *m_ckCode;          //!< Camera kernel (CK) code
      SpiceInt *m_ikCode;          //!< Instrument kernel (IK) code
      SpiceInt *m_sclkCode;        //!< Spacecraft clock correlation kernel (SCLK) code
      SpiceInt *m_spkBodyCode;     //!< Spacecraft and planet ephemeris kernel (SPK) body code
      SpiceInt *m_bodyFrameCode;   /**< Naif's BODY_FRAME_CODE value. It is read
                                        from the labels, if it exists. Otherwise,
                                        it's calculated by the init() method.*/

      PvlObject *m_naifKeywords; //!< NaifKeywords PvlObject from cube

      bool m_usingNaif; /**< Indicates whether we are reading values from the
                             NaifKeywords PvlObject in cube*/

      bool m_usingAle; /**< Indicate whether we are reading values from an ISD returned
                            from ALE */

  };
}

#endif
