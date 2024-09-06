#ifndef Module_h
#define Module_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "HiCalTypes.h"
#include "HiCalUtil.h"
#include "HiCalConf.h"
#include "LoadCSV.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"
#include "IException.h"

namespace Isis {

  class PvlGroup;

  /**
   * @brief Module manages HiRISE calibration vectors from various sources
   *
   * @ingroup Utility
   * @author 2007-10-05 Kris Becker
   * @internal
   *   @history 2010-04-16 Kris Becker Added load method for standardized
   *            access to CSV files; renamed to Module from Component
   */
  class Module {
    public:
      friend std::ostream &operator<<(std::ostream &o, const Module &c) {
        c.printOn(o);
        return (o);
      }

    public:
      //  Constructors and Destructor
      Module() : _name("Module"), _csvFile(""), _data(), _history(),
                  _fmtWidth(DefaultWidth),  _fmtPrecision(DefaultPrecision) { }
      Module(const QString &name) : _name(name), _csvFile(""), _data(),
                 _history(),_fmtWidth(DefaultWidth),
                 _fmtPrecision(DefaultPrecision) { }
      Module(const QString &name, const HiHistory &history) :
                _name(name), _csvFile(""), _data(), _history(history),
                _fmtWidth(DefaultWidth),  _fmtPrecision(DefaultPrecision) { }
      Module(const QString &name, const Module &c) : _name(name),
                _csvFile(c._csvFile), _data(c._data), _history(c._history),
                _fmtWidth(c._fmtWidth),_fmtPrecision(c._fmtPrecision) { }
      Module(const Module &c) : _name(c._name), _csvFile(c._csvFile),
                                      _data(c._data), _history(c._history),
                                      _fmtWidth(c._fmtWidth),
                                      _fmtPrecision(c._fmtPrecision) { }


      /** Destructor */
      virtual ~Module() { }

      /** Returns name of component */
      inline QString name() const { return (_name); }
      /** Returns expanded name of last CSV file loaded by loadCsv */
      inline QString getcsvFile() const { return (_csvFile); }
      /** Returns the size (number of elements) of data array */
      inline int size() const { return (_data.dim()); }

      /** Invokes the process method on the Module vector */
      virtual void Process(const Module &c) {
        Process(c.ref());
      }

      /** Default processing behavior makes a reference copy of data array */
      virtual void Process(const HiVector &v) {
        _data = v;
        return;
      }

      /**
       * @brief Provide generic loading of CSV file for all modules
       *
       * This method provides generalized access to CSV files through the
       * standardized format.
       *
       * @author Kris Becker - 4/16/2010
       *
       * @param cvsBase Name of base keyword for CSV file
       * @param conf    Configuration parameters
       * @param prof    Module profile parameters
       * @param samples Number of expect elements to be read from CSV file
       *
       * @return HiVector
       */
      HiVector loadCsv(const QString &csvBase, const HiCalConf &conf,
                       const DbProfile &prof, const int &elements = 0)  {
        LoadCSV csv(csvBase, conf, prof);
        _csvFile = csv.filename();
        if (elements != 0) csv.validateSize(elements, true);
        csv.History(_history);
        return (csv.getVector());
      }

      /** Return data via a const reference */
      const HiVector &ref() const { return (_data); }
      /** Return individual element of the data vector */
      inline double operator()(int index) const { return (_data[index]);}

      /** Return recorded history of events */
      const HiHistory &History() const { return (_history); }

      /** Record history in Pvl group object */
      virtual void record(PvlGroup &pvl,
                          const QString keyname = "ModuleHistory")
                          const {
        pvl += _history.makekey(keyname);
        return;
      }

      /**
       * @brief Dumps the component to a specified file
       *
       * @param fname  Name of file to dump contents to
       */
      void Dump(const QString &fname) const {
        FileName dumpc(fname);
        QString dumpcFile = dumpc.expanded();
        std::ofstream ofile(dumpcFile.toLatin1().data(), std::ios::out);
        if (!ofile) {
          std::string mess = "Unable to open/create module dump file " +
                             dumpc.expanded();
          throw IException(IException::User, mess, _FILEINFO_);
        }
        ofile << *this;
        ofile.close();
        return;
      }


    protected:
      enum { DefaultWidth = 10, DefaultPrecision = 6};

      QString   _name;         //!< Name of component
      QString   _csvFile;      //!< Fully expanded name of CSV file if present
      HiVector      _data;         //!< Data vector
      HiHistory     _history;      //!< Hierarchial component history
      int           _fmtWidth;     //!< Default field with of double
      int           _fmtPrecision; //!< Default field with of double


      /**
       * @brief Properly format values that could be special pixels
       *
       * This method applies ISIS special pixel value conventions to properly
       * print pixel values.
       *
       * @param[in] (double) value Input value to test for specialness and print
       *                           as requested by caller
       * @param[in] (int) width Width of field in which to print the value
       * @param[in] (int) prec  Precision used to format the value
       * @return (string) Formatted double value
       */
      inline QString formatDbl(const double &value) const {
        std::ostringstream ostr;
        if (IsSpecial(value)) {
          ostr << std::setw(_fmtWidth) << PixelToString(value);
          return (QString(ostr.str().c_str()));
        }
        else {
        // Its not special so format to callers specs
          ostr << std::setw(_fmtWidth) << std::setprecision(_fmtPrecision) << value;
          return (QString(ostr.str().c_str()));
        }
      }

      /** Default printing of data in module */
      virtual void printOn(std::ostream &o) const {
        o << "#  History = " << _history << std::endl;
        o << "#  Count =   " << _data.dim() << std::endl;
        for (int i = 0 ; i < _data.dim() ; i++) {
          o << formatDbl(_data[i]) << std::endl;
        }
        return;
      }

  };

}     // namespace Isis
#endif
