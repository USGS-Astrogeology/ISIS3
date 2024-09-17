#ifndef MdisCalUtils_h
#define MdisCalUtils_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <string>
#include <vector>

#include "Camera.h"
#include "CSVReader.h"
#include "FileName.h"
#include "IString.h"
#include "iTime.h"
#include "NaifStatus.h"
#include "Spice.h"

/**
 * @author ????-??-?? Kris Becker
 *
 * @internal
 *   @history 2015-08-31 Jeannie Backer - Changed method name from loadContaminationEvent() to
 *                           loadEmpiricalCorrection(). Brought code closer to coding standards.
 */
namespace Isis {
  /**
   * @brief Helper function to convert values to doubles
   *
   * @param T Type of value to convert
   * @param value Value to convert
   *
   * @return double Converted value
   */
  template <typename T> double toDouble(const T &value) {
    return toDouble(QString(value).trimmed());
  }

  template <typename T> int toInteger(const T &value) {
    return QString(value).trimmed().toInt();
  }

  inline QString quote(const QString &value) {
    if (value.isEmpty()) return (value);
    if (value[0] == '"') return (value);
    return (QString('"' + value + '"'));
  }

  /**
   * @brief Load required NAIF kernels required for timing needs
   *
   * This method maintains the loading of kernels for MESSENGER timing and
   * planetary body ephemerides to support time and relative positions of planet
   * bodies.
   */
  static void loadNaifTiming() {
    static bool naifLoaded = false;
    if (!naifLoaded) {
      //  Load the NAIF kernels to determine timing data
      Isis::FileName leapseconds("$base/kernels/lsk/naif????.tls");
      leapseconds = leapseconds.highestVersion();

      Isis::FileName sclk("$messenger/kernels/sclk/messenger_????.tsc");
      sclk = sclk.highestVersion();

      Isis::FileName pck("$base/kernels/spk/de???.bsp");
      pck = pck.highestVersion();

      //  Load the kernels
      QString leapsecondsName(QString::fromStdString(leapseconds.expanded()));
      QString sclkName(QString::fromStdString(sclk.expanded()));
      QString pckName(QString::fromStdString(pck.expanded()));
      furnsh_c(leapsecondsName.toLatin1().data());
      furnsh_c(sclkName.toLatin1().data());
      furnsh_c(pckName.toLatin1().data());

      //  Ensure it is loaded only once
      naifLoaded = true;
    }
    return;
  }


  /**
   * @brief Computes the distance from the Sun to the observed body
   *
   * This method requires the appropriate NAIK kernels to be loaded that
   * provides instrument time support, leap seconds and planet body ephemeris.
   *
   * @return double Distance in AU between Sun and observed body
   */
  static bool sunDistanceAU(const QString &scStartTime,
                            const QString &target,
                            double &sunDist,
                            Cube *cube) {
    try {
      Camera *cam;
      cam = cube->camera();
      cam->SetImage(0.5, 0.5);
      sunDist = cam->sunToBodyDist() / 1.49597870691E8;
    }
    catch (IException &e) {
      try {
        //  Ensure NAIF kernels are loaded
        NaifStatus::CheckErrors();
        loadNaifTiming();
        sunDist = 1.0;
        
        //  Determine if the target is a valid NAIF target
        SpiceInt tcode;
        SpiceBoolean found;
        bodn2c_c(target.toLatin1().data(), &tcode, &found);
        if (!found) return (false);
        
        //  Convert starttime to et
        double obsStartTime;
        scs2e_c(-236, scStartTime.toLatin1().data(), &obsStartTime);
        NaifStatus::CheckErrors();
        
        //  Get the vector from target to sun and determine its length
        double sunv[3];
        double lt;
        spkpos_c(target.toLatin1().data(), obsStartTime, "J2000", "LT+S", "sun",
                        sunv, &lt);
        double sunkm = vnorm_c(sunv);
        
        //  Return in AU units
        sunDist = sunkm / 1.49597870691E8;
      } 
      catch (IException &e) {
        std::string message = "Unable to determine the sun-target distance.";
        throw IException(e, IException::Unknown, message, _FILEINFO_);
      }
    }
    return (true);
  }


  /**
   * Load WAC CSV.
   */
  std::vector<double> loadWACCSV(const QString &fname, int filter,
                                 int nvalues, bool header=true, int skip=0) {
    //  Open the CSV file
    FileName csvfile(fname.toStdString());
    CSVReader csv(QString::fromStdString(csvfile.expanded()), header, skip);
    for (int i = 0; i < csv.rows(); i++) {
      CSVReader::CSVAxis row = csv.getRow(i);
      if (toInteger(row[0]) == filter) {
        if ((row.dim1() - 1) < nvalues) {
          std::string mess = "Number values (" + toString(row.dim1() - 1) +
                             ") in file " + fname.toStdString() +
                             " less than number requested (" +
                             toString(nvalues) + ")!";
          throw IException(IException::User, mess, _FILEINFO_);
        }

        std::vector<double> rsp;
        for (int c = 0; c < nvalues; c++) {
          rsp.push_back(toDouble(row[1+c]));
        }
        return (rsp);
      }
    }

    // If it reaches here, the filter was not found
    std::ostringstream mess;
    mess << "CSV Vector MDIS filter " << toString(filter) <<  ", not found in file "
         << fname.toStdString() << "!";
    throw IException(IException::User, mess.str(), _FILEINFO_);
  }


  /**
   * Load NAC CSV.
   */
  std::vector<double> loadNACCSV(const QString &fname, int nvalues,
                                 bool header=true, int skip=0) {
    //  Open the CSV file
    FileName csvfile(fname.toStdString());
    CSVReader csv(QString::fromStdString(csvfile.expanded()), header, skip);
    CSVReader::CSVAxis row = csv.getRow(0);
    if (row.dim1() < nvalues) {
      std::string mess = "Number values (" + toString(row.dim1()) +
                         ") in file " + fname.toStdString() + " less than number requested (" +
                         toString(nvalues) + ")!";
      throw IException(IException::User, mess, _FILEINFO_);

    }
    std::vector<double> rsp;
    for (int i = 0; i < nvalues; i++) {
      rsp.push_back(toDouble(row[i]));
    }
    return (rsp);
  }


  /**
   * Load responsivity 
   */
  std::vector<double> loadResponsivity(bool isNAC, bool binned, int filter,
                                       QString &fname) {

    FileName resfile(fname.toStdString());
    if (fname.isEmpty()) {
      QString camstr = (isNAC) ? "NAC" : "WAC";
      QString binstr = (binned)       ? "_BINNED" : "_NOTBIN";
      QString base   = "$messenger/calibration/RESPONSIVITY/";
      resfile = base.toStdString() + "MDIS" + camstr.toStdString() + binstr.toStdString() + "_RESP_?.TAB";
      resfile = resfile.highestVersion();
      fname = QString::fromStdString(resfile.originalPath() + "/" + resfile.name());
    }

    // Unfortunately NAC has a slightly different format so must do it
    //  explicitly
    if (isNAC) {
      return (loadNACCSV(fname, 4, false, 0));
    }
    else {
      // Load the WAC parameters
      return (loadWACCSV(fname, filter, 4, false, 0));
    }
  }


  /**
   * Load solar irradiation
   */
  std::vector<double> loadSolarIrr(bool isNAC, bool binned, int filter,
                                   QString &fname)  {

    FileName solfile(fname.toStdString());
    if (fname.isEmpty()) {
      QString camstr = (isNAC) ? "NAC" : "WAC";
      QString base   = "$messenger/calibration/SOLAR/";
      solfile = base.toStdString() + "MDIS" + camstr.toStdString() + "_SOLAR_?.TAB";
      solfile = solfile.highestVersion();
      fname = QString::fromStdString(solfile.originalPath() + "/" + solfile.name());
    }

    if (isNAC) {
      return (loadNACCSV(fname, 3, false, 0));
    }
    else {
      return (loadWACCSV(fname, filter, 3, false, 0));
    }
  }


  /**
   * Loads the smear component
   */
  double loadSmearComponent(bool isNAC, int filter, QString &fname) {

    FileName smearfile(fname.toStdString());
    if (fname.isEmpty()) {
      QString camstr = (isNAC) ? "NAC" : "WAC";
      QString base   = "$messenger/calibration/smear/";
      smearfile = base.toStdString() + "MDIS" + camstr.toStdString() + "_FRAME_TRANSFER_??.TAB";
      smearfile = smearfile.highestVersion();
      fname = QString::fromStdString(smearfile.originalPath() + "/" + smearfile.name());
    }

    std::vector<double> smear;
    if (isNAC) {
      smear = loadNACCSV(fname, 1, false, 0);
    }
    else {
      smear = loadWACCSV(fname, filter, 1, false, 0);
    }
    return (smear[0]);
  }


/**
 * @brief Load and retrieve empirical correction factor
 *
 * This function determines the empirical correction factor for changes that
 * that occured on the spacecraft after Mercury orbit insertion.  The
 * affected dates are May 24, 2011 to January 3, 2012.
 *
 * The table of correction factors is expected to be stored in
 * $messenger/calibration/events/event_table_ratioed_v?.txt.  However, the
 * caller may provide a table that conforms to the expected format.  The
 * expected format for the empirical correction file is a comma separated value
 * (CSV) table that contains 13 columns of data per row.  The first column is
 * the UTC time during the event. The next 12 columns contain multiplicative
 * correction factors for each WAC filter (NAC correction factors are not
 * provided). These factors are expected to be around 1.0 (the default) as it
 * is expected to directly scale DN values.
 *
 * Below is the expected mapping of column indexes to filter numbers as
 * specfied in the BandBin/Number keyword from MDIS ISIS cube labels. Index is
 * the column index from each row for a given filter, Number is the value of
 * the BandBin/Number keyword from the label designating the filter number
 * (corresponding to the filter parameter passed to this routine) and Letter is
 * the filter letter designation used in the last alpha numeric character in
 * MDIS filenames:
 *
 *  Index   Number   Letter   Wavelength
 *    1       6        F         430 nm
 *    2       3        C         480 nm
 *    3       4        D         560 nm
 *    4       5        E         630 nm
 *    5       7        G         750 nm
 *    6      12        L         830 nm
 *    7      10        J         900 nm
 *    8       9        I        1000 nm
 *    9       1        A        Filter 1 (700 nm)
 *   10       2        B        Filter 2 (clear)
 *   11       8        H        Filter 8 (950 nm)
 *   12      11        K        Filter 11 (1010 nm)
 *
 * The UTC dates in the first column are assumed to be strictly increasing in
 * time.  The initial table (*_v2) contains dates that span the complete
 * expected timeframe of the mission (launch at 2004-08-04T10:00:00.000000,
 * termination at 2015-01-03T09:00:00.000000).
 *
 * The spacecraft clock time is provided as input (scStartTime) to this
 * function.  This value is converted to ET (SCET) and used to determine the
 * corresponding event time in the first column of the table.  The first table
 * column time is represented in UTC time.  This time is converted to ET and
 * then compared with the start time in ET.
 *
 * The algorithm searches linearly through the table essentially storing the
 * time slot prior to the SCET and the next occuring one.  Ultimately, the
 * factor returned by the algorithm is the one whose event time is closest to
 * the SCET.
 *
 * The empirical correction model and algorithm was developed by
 * Mary Ruth Keller of JHA/APL.
 *
 * @author Kris Becker - 10/23/2012
 *
 * @param scStartTime - Start time of the image in SCLK format
 * @param filter      - WAC filter number to return event correction factor for
 * @param ename       - Returns the name of the event table file if not
 *                      provided by caller.  If non-empty string is passed by
 *                      caller, it is assumed to be a fully qualified filename
 *                      of the event table.
 * @param eDate       - Returns the UTC date entry of the selected correction
 *                      event factor
 *
 * @return double     - Event correction factor at the selected time to apply
 *                      to WAC filter data.
 */
 double loadEmpiricalCorrection(const QString &scStartTime, const int filter,
                                QString &ename, QString &eDate, Cube *cube) {

   //  This table maps the filter number extracted from BandBin/Number keyword
   //  to the columns (index) in the empirical correction table
   const int filterMap[12] = { 6, 3, 4, 5, 7, 12, 10, 9, 1, 2, 8, 11 };

   //  Find the WAC filter column index
   int ncols = sizeof(filterMap)/sizeof(filterMap[0]);
   int column = -1;
   for (int c = 0; c < ncols; c++) {
     if (filterMap[c] == filter) {
       column = c + 1;  // indexes start after 1st (time) column
       break;
     }
   }

   // Ensure we have a valid filter number
   if (column <= 0) {
     std::ostringstream mess;
      mess << "Invalid MDIS WAC filter number (" << filter
           <<  " - range:1-12) for determining index into empirical correction table.";
      throw IException(IException::User, mess.str(), _FILEINFO_);
   }

   //  File name not provided by caller.  Determine the event table name
    if ( ename.isEmpty() ) {
      FileName eventfile("$messenger/calibration/events/event_table_ratioed_v?.txt");
      eventfile = eventfile.highestVersion();
      ename = QString::fromStdString(eventfile.originalPath() + "/" + eventfile.name());
    }

    //  Open/read the CSV empirical correction file
    FileName csvfile(ename.toStdString());
    const bool header = false;  // No header in file
    const int skip = 0;         // No lines to skip to data
    const int nvalues = 13;     // Expected columns in table
    CSVReader csv(QString::fromStdString(csvfile.expanded()), header, skip);
    if (csv.columns() < nvalues) {  // All rows should have same # columns
      std::string mess = "Number values (" + toString(csv.columns()) +
                         ") in file " + ename.toStdString() + " less than number requested (" +
                         toString(nvalues) + ")!";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    double obsStartTime = 0.0;
    try {
      Camera *cam = cube->camera();
      obsStartTime = cam->getClockTime(scStartTime, -236).Et();
    } 
    catch (IException &e) {
      try {
        // Ensure NAIF kernels are loaded for NAIF time computations
        NaifStatus::CheckErrors();
        loadNaifTiming();

        //  Convert s/c clock start time to et
        scs2e_c(-236, scStartTime.toLatin1().data(), &obsStartTime);
        NaifStatus::CheckErrors();

      } 
      catch (IException &e) {
        std::string message = "Could not convert spacecraft clock start count to ET.";
        throw IException(e, IException::Unknown, message, _FILEINFO_);
      }
    }

    // Set initial conditions and loop through all rows in the event table
    double evalue = 1.0;
    eDate = "N/A";  // Will attain a valid time on guaranteed first pass
    double preEventTime = 0.0;
    for (int i = 0; i < csv.rows(); i++) {
      CSVReader::CSVAxis eRow = csv.getRow(i);
      QString utcTime = eRow[0];
      double eTime;
      eTime = iTime(utcTime.toLatin1().data()).Et();
      // If current time is greater than start time this is the post event case
      if (eTime > obsStartTime) {
        //  Get closest pre or post event correction factor
        if ( fabs(obsStartTime-preEventTime) > fabs(eTime-obsStartTime) ) {
          //  Post-event time closer to SCLK than Pre-event time
          eDate = utcTime;
          evalue = toDouble(eRow[column]);
        }

        break;  //  Terminate loop and return
      }

      // Record pre-event time slot - Sets return variables as well
      eDate = utcTime;
      preEventTime = eTime;
      evalue = toDouble(eRow[column]);
    }

    // Return the factor
    return (evalue);
  }

};
#endif
