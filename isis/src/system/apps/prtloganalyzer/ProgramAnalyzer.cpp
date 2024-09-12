/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
                                                                     
#include <memory>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "ProgramAnalyzer.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlObject.h"


using namespace std;

namespace Isis {

/** Default constructor */
ProgramAnalyzer::ProgramAnalyzer() {
  init();
}

/**
 * Analyzes the given log file on construction
 *
 * @param logfile A print.prt file
 */
ProgramAnalyzer::ProgramAnalyzer(const QString &logfile) {
  init();
  add(logfile);
}

/**
 * @brief Set the list of program exclusions
 *
 *  When provided, the QString should contain names of ISIS programs that will be
 *  excluded in the analysis.  If more than one program is desired, separate
 *  them by commas.
 *
 *  The exclusion list takes precedence over any applications added in the
 *  inclusion list.  In other words, if the same program is included in both the
 *  inclusion and exclusion list, it will be excluded.
 *
 *  Note this method can be called repeatedly to add names.
 *
 * @param name Comma separated list of names of programs to exclude
 */
void ProgramAnalyzer::setExclude(const QString &name) {
  foreach (QString singleName, name.split(",")) {
    exclude(singleName);
  }
}

/**
 * @brief Loads a list of excluded program names from a file
 *
 * The file is expected to contain the name of an ISIS program, one per line.
 * These programs will be logged but not included in the analysis.
 *
 * The exclusion list takes precedence over any applications added in the
 * inclusion list.  In other words, if the same program is included in both the
 * inclusion and exclusion list, it will be excluded.
 *
 * Note this method can be called repeatedly to add names.
 *
 * @param name  Name of file containing program list to exclude
 */
void ProgramAnalyzer::exclude(const QString &name) {
  QString prog = name.trimmed();
  if ( !_excludes.exists(prog) ) {
    _excludes.add(prog, 0);
  }
  return;
}


/**
 * @brief Set the list of program inclusions
 *
 * When provided, the QString should contain names of ISIS programs that will be
 * included in the analysis.  If more than one program is desired, separate
 * them by commas.
 *
 * If this option is used, it will only included programs given in this list.
 * It operates as both an inclusive and exclusive list, so there is no need to
 * also utilize the exclude features of this class.
 *
 * However, if you do use the exclusion features of this clas, the exclusion
 * list takes precedence over any applications added in the inclusion list.  In
 * other words, if the same program is included in both the inclusion and
 * exclusion list, it will be excluded.
 *
 * Note this method can be called repeatedly to add names.
 *
 * @param name Comma separated list of names of programs to include
 */
void ProgramAnalyzer::setInclude(const QString &name) {
  foreach (QString singleName, name.split(",")) {
    include(singleName);
  }
}

/**
 * @brief Loads a list of included program names from a file
 *
 * The file is expected to contain the name of an ISIS program, one per line.
 * These programs will be included in the analysis.
 *
 * If this option is used, it will only included programs given in this list.
 * It operates as both an inclusive and exclusive list, so there is no need to
 * also utilize the exclude features of this class.
 *
 * However, if you do use the exclusion feartures, the exclusion list takes
 * precedence over any applications added in the inclusion list.  In other
 * words, if the same program is included in both the inclusion and exclusion
 * list, it will be excluded.
 *
 * Note this method can be called repeatedly to add names.
 *
 * @param name  Name of file containing program list to include
 */
void ProgramAnalyzer::include(const QString &name) {
  QString prog = name.trimmed();
  if ( !_includes.exists(prog) ) {
    _includes.add(prog, 0);
  }
  return;
}

/**
 * @brief Adds a print.prt file to the analysis
 *
 * The programs contained in the log file, assumed to be a print.prt file, will
 * be added to the list of programs to be analyzed.  They are subject to the
 * exclude and include program lists.
 *
 * @param logfile An ISIS print.prt file
 */
void ProgramAnalyzer::add(const QString &logfile) {
  Pvl plog(logfile.toStdString());
  for(int i = 0; i < plog.objects(); i++) {
    add(plog.object(i));
  }
}

/**
 * @brief Adds a program object originating from a print.prt file
 *
 * The PvlObject provided is assumed to orginate from an ISIS print.prt log
 * file.  It contains information that will be extracted and analyzed according
 * to the features of this class.
 *
 *
 * @param program Pvl object containing the output log of an ISIS application
 */
void ProgramAnalyzer::add(PvlObject &program) {
  _count++;
  QString prog(QString::fromStdString(program.name()));
  if ( _excludes.exists(prog) ) {
    _excludes.get(prog)++;
    return;
  }

  if ( _includes.size() > 0 ) {
    if ( !_includes.exists(prog)  ) {
      return;
    }
    _includes.get(prog)++;
  }

  ProgramData pdata;
  pdata.name = prog;
  pdata.runtime = getKey(program, "ExecutionDateTime");
  pdata.from = getKey(program, "From", "UserParameters");
  pdata.to   = getKey(program, "To", "UserParameters");
  accounting(program, pdata);
  analyze(pdata);
  return;
}

/**
 * @brief Reports program counters for the current state
 *
 * This method reports counts of programs as they were added to the object.  It
 * reports total programs, numbers for analyzed, included, excluded, unique,
 * valid, errors, zero CPU/Connect times and incomplete or invalid (typcially
 * negative times) for programs it evaluted.
 *
 * @param name Name of Pvl group to create
 *
 * @return PvlGroup Pvl group containing program numbers/counts
 */
PvlGroup ProgramAnalyzer::review(const QString &name) const {
  PvlGroup pvl(name.toStdString());

  pvl += PvlKeyword("Programs", std::to_string(size()));
  pvl += PvlKeyword("Unique", std::to_string(Programs()));
  pvl += PvlKeyword("Included", std::to_string(LimitTotals(_includes)));
  pvl += PvlKeyword("Excluded", std::to_string(LimitTotals(_excludes)));
  pvl += PvlKeyword("Valid", std::to_string(valid()));
  pvl += PvlKeyword("Errors", std::to_string(errors()));
  pvl += PvlKeyword("ZeroTime", std::to_string(zerotime()));
  pvl += PvlKeyword("NoData", std::to_string(nodata()));
  pvl += PvlKeyword("BadData", std::to_string(baddata()));
  pvl += PvlKeyword("Total", std::to_string(count()));
  return (pvl);
}


/**
 * @brief Reports cumulative runtime performance statistics for programs
 *
 * This method formats the contents of the program analysis in a Pvl group that
 * provides information for all programs regardin CPU, connect and I/O times.
 *
 *
 * @param name Name of Pvl group to create
 *
 * @return PvlGroup Pvl group containing cumulative program analysis
 */
PvlGroup ProgramAnalyzer::cumulative(const QString &name) const {
  return (toPvl(_totals, name));
}

/**
 * @brief Reports analysis for a specific program
 *
 * This object maintains individual statistics for each unique program.  This
 * method reports the analysis for a particular program.
 *
 *
 * @param name Name of the program to analyze
 *
 * @return PvlGroup Pvl group containing program analysis
 */
PvlGroup ProgramAnalyzer::summarize(const QString &name) const {
  return (toPvl(_programs.get(name)));
}

/**
 * @brief Reports analysis for the nth occuring application in the list
 *
 * This object maintains individual statistics for each unique program.  This
 * method reports the analysis for a program that occurs in the list at the
 * given index.

 * @param index Index of the application to summerize
 *
 * @return PvlGroup Pvl group containing the program analysis
 */
PvlGroup ProgramAnalyzer::summarize(const int index) const {
  return (toPvl(_programs.getNth(index)));
}

/**
 * @brief Writes a header in CVS format to a ostream
 *
 * @param out Output stream to write the header
 *
 * @return ostream& Returns the stream
 */
ostream &ProgramAnalyzer::header(ostream &out) const {
  out << "Program,From,To,ExecutionDateTime,ConnectTime,CpuTime,IOTime\n";
  return (out);
}

/**
 * @brief Writes the analysis to the stream in CSV format
 *
 * This method provides the analysis in CSV format for more traditional
 * manipulation.  This format is well suited to be plotted for further analysis
 * of the program/system performance.
 *
 * The columns provided are:  Program name, FROM file, TO file, runtime, connect
 * time, CPU time, and I/O time (difference in runtime and CPU time).
 *
 *
 * @param out Output stream to write data to
 *
 * @return ostream& The output stream
 */
ostream &ProgramAnalyzer::listify(ostream &out) const {
  vector<ProgramData>::const_iterator progs = _pdata.begin();
  while (progs != _pdata.end()) {
    if ( progs->status == VALID ) {
      out << format(progs->name).toStdString() << ",";
      out << format(progs->from).toStdString() << ",";
      out << format(progs->to).toStdString() << ",";
      out << format(progs->runtime).toStdString() << ",";
      out << DblToStr(progs->connectTime, 2).toStdString() << ",";
      out << DblToStr(progs->cpuTime, 2).toStdString() << ",";
      out << DblToStr(progs->connectTime-progs->cpuTime, 2).toStdString() << "\n";
    }
   ++progs;
  }
  return (out);
}


/**
 * @brief Initializes the class
 *
 * This init function is reintrant and will reset all internal parameters to the
 * empty state.
 */
void ProgramAnalyzer::init() {
  _count = 0;
  _excludes = LogList();
  _includes = LogList();
  _programs = RunList();
  _totals = RunTimeStats("Cumulative");
  _pdata.clear();
}


/**
 * @brief Provides a count of analyzed programs
 *
 * Iterates through all programs included in the analysis and provides a count
 * fo the total.  It will search through for a given status and only includes
 * those which have the indicated status.  The valid status to check are those
 * defined in the Status enum list.
 *
 * @param status Status of the program to count
 *
 * @return int
 */
int ProgramAnalyzer::getCount (ProgramAnalyzer::Status status) const {
  vector<ProgramData>::const_iterator progs = _pdata.begin();
  int n(0);
  while (progs != _pdata.end()) {
    if ( progs->status == status ) n++;
    ++progs;
  }
  return (n);
}


/**
 * @brief Extracts a keyword value from the Pvl object
 *
 *
 * @param obj Pvl object to search for the keyword
 * @param key Name of keyword to find
 * @param grp Optional group within the object to find the keyword
 *
 * @return QString Value of the keyword if the keyword exists, otherwise an empty
 *                QString is returned.
 */
QString ProgramAnalyzer::getKey(PvlObject &obj, const QString &key,
                               const QString &grp) const {

  QString value("");
  if ( !grp.isEmpty() ) {
    PvlGroup &g = obj.findGroup(grp.toStdString());
    value = findKey(g, key);
  }
  else {
    value = findKey(obj, key);
  }
  return (value);
}

/**
 * @brief Converts times represented in text to digital values
 *
 * The text QString, atime, is expected to be of the format "HH:MM:SS.sss" where
 * "HH" is hours, "MM" is minutes and "SS.sss" is seconds.milliseconds.  The
 * units returned are in seconds.
 *
 * @param atime Text QString containing time to convert
 * @param dtime Time in seconds
 *
 * @return ProgramAnalyzer::Status Returns BADDATA if the text QString is empty
 *                                 or malformed, or VALID if the conversion
 *                                 succeeds.
 */
ProgramAnalyzer::Status ProgramAnalyzer::convertTime(const QString &atime,
                                                     double &dtime) const {
  if ( atime.isEmpty() ) return (BADDATA);
  QStringList t = atime.split(":");
  if ( t.size() != 3 ) {
    return (BADDATA);
  }

  //  Convert to seconds
  double toSeconds(3600.0);
  dtime = 0.0;
  for ( unsigned int i = 0 ; i < 3 ; i++ ) {
    dtime += t[i].toDouble() * toSeconds;
    toSeconds /= 60.0;
  }

  return (VALID);
}

/**
 * @brief Compute analysis of program entry
 *
 * This method accepts a Pvl object that is assumed to orignate from an ISIS
 * print.prt log file and conforms to the format in the log file.
 *
 * Data is extracted from certain keywords in the object.  Invalid objects or
 * error conditions are detected and are indicated in the status of the program
 * analysis structure, pdata.  Other conditions of no time for runtimes or CPU
 * times is also detected and indicated.
 *
 *
 * @param obj   Object containing program data
 * @param pdata Structure to return derived values from the program data
 *
 * @return bool True if successful, false otherwise.
 */
bool ProgramAnalyzer::accounting(PvlObject &obj,
                                 ProgramAnalyzer::ProgramData &pdata) const {

  //  Assume an error occured if the Accounting group is missing
  if ( !obj.hasGroup("Accounting") ) {
    pdata.status = ERRORS;
    return (false);
  }

  PvlGroup &acc = obj.findGroup("Accounting");
  Status status = convertTime(findKey(acc,"ConnectTime"), pdata.connectTime);
  pdata.status = convertTime(findKey(acc, "CpuTime"), pdata.cpuTime);

  //  Test a few remaining times
  if ( status != VALID ) pdata.status = status;
  else if ( pdata.connectTime <= 0.0 ) pdata.status = ZEROTIME;

  return (pdata.status == VALID);
}



/**
 * @brief Performs the analysis of a program
 *
 * This method accepts a program data structure, determines validity for
 * inclusion in the analysis and computes statistics from the data content.
 *
 * @param data Structure containing program data
 *
 * @return bool True if valid and included, false otherwize
 */
bool ProgramAnalyzer::analyze(const ProgramAnalyzer::ProgramData &data) {

  bool good(false);
  if ( data.status == VALID ) {
    if ( !_programs.exists(data.name) ) {
       _programs.add(data.name, RunTimeStats(data.name));
    }
    RunTimeStats &stats = _programs.get(data.name);
    stats.contime.AddData(data.connectTime);
    stats.cputime.AddData(data.cpuTime);
    stats.iotime.AddData(data.connectTime - data.cpuTime);

    _totals.contime.AddData(data.connectTime);
    _totals.cputime.AddData(data.cpuTime);
    _totals.iotime.AddData(data.connectTime - data.cpuTime);

    good = true;
  }
  _pdata.push_back(data);
  return (good);
}

/**
 * @brief Produces report of run time statistics for the given structure
 *
 *
 * The data contained with the RunTimeStats is externalized to a Pvl group with
 * some resonable formatting.
 *
 * @param stats Run time stats for the given data
 * @param name  Optional name of the PvlGroup results
 *
 * @return PvlGroup Pvl group of runtime statistics
 */
PvlGroup ProgramAnalyzer::toPvl(const RunTimeStats &stats,
                                const QString &name) const {
  PvlGroup pvl((name.isEmpty() ? stats.pname.toStdString() : name.toStdString()));

  pvl += PvlKeyword("Hits", std::to_string(stats.contime.TotalPixels()));
  pvl += PvlKeyword("ConnectTimeMinimum", DblToStr(stats.contime.Minimum(), 2).toStdString());
  pvl += PvlKeyword("ConnectTimeMaximum", DblToStr(stats.contime.Maximum(), 2).toStdString());
  pvl += PvlKeyword("ConnectTimeAverage", DblToStr(stats.contime.Average(), 2).toStdString());
  pvl += PvlKeyword("ConnectTimeStdDev", DblToStr(stats.contime.StandardDeviation(), 4).toStdString());

  pvl += PvlKeyword("CpuTimeMinimum", DblToStr(stats.cputime.Minimum(), 2).toStdString());
  pvl += PvlKeyword("CpuTimeMaximum", DblToStr(stats.cputime.Maximum(), 2).toStdString());
  pvl += PvlKeyword("CpuTimeAverage", DblToStr(stats.cputime.Average(), 2).toStdString());
  pvl += PvlKeyword("CpuTimeStdDev", DblToStr(stats.cputime.StandardDeviation(), 4).toStdString());

  pvl += PvlKeyword("IOTimeMinimum", DblToStr(stats.iotime.Minimum(), 2).toStdString());
  pvl += PvlKeyword("IOTimeMaximum", DblToStr(stats.iotime.Maximum(), 2).toStdString());
  pvl += PvlKeyword("IOTimeAverage", DblToStr(stats.iotime.Average(), 2).toStdString());
  pvl += PvlKeyword("IOTimeStdDev", DblToStr(stats.iotime.StandardDeviation(), 4).toStdString());

  return (pvl);
}

/**
 * @brief Returns NULL for empty QStrings to ensure meaningful content
 *
 * @param s  String to test for content
 *
 * @return QString Returns existing content if present, NULL if empty
 */
QString ProgramAnalyzer::format(const QString &s) const {
 if ( s.isEmpty() )  return (QString("NULL"));
 return (s);
}


  /**
   * @brief Convert a double value to a QString subject to precision specs
   *
   * This method converts a double value to a QString that has a prefined digitis
   * of precision.  Fixed float form is used with the specified number of digits
   * of precision.
   *
   * @param value Double value to convert to QString
   *
   * @return QString Returns the converted QString
   */
  QString ProgramAnalyzer::DblToStr(const double &value, const int precision)
                                    const {
    if(IsSpecial(value)) {
      return (QString("0.0"));
    }

    //  Format the QString to specs
    ostringstream strcnv;
    strcnv.setf(std::ios::fixed);
    strcnv << setprecision(precision) << value;
    return (QString(strcnv.str().c_str()));
  }

  /**
   * @brief Returns the count of all programs in the log list
   *
   * This method computes a count of all programs that exist in the list of
   * applications that encurred a limit in the analysis.  It is not enough to
   * just report the number of entries in the list - each list contains a count
   * of occurances.  These occurances are summed and return to the caller.
   *
   * @param limit List of applications incurring a limit
   *
   * @return int Total number of occurances all programs in the list
   */
  int ProgramAnalyzer::LimitTotals(const ProgramAnalyzer::LogList &limit)
                                   const {
    int total(0);
    for ( int i = 0 ; i < limit.size() ; i++ ) {
      total += limit.getNth(i);
    }
    return (total);
  }

} // namespace Isis
