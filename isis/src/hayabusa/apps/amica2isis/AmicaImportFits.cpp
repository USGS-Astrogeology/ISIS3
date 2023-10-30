/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AmicaImportFits.h"

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>

#include <QDebug>
#include <QString>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "PvlObject.h"


using namespace std;

namespace Isis {


  /**
   * Default constructor. This constructor initializes the FITS object
   *
   */
  AmicaImportFits::AmicaImportFits() {
    init();
  }

  /**
   * @brief This constructor automatically loads the given label and table files.
   *
   * This constructor takes the name of the label file describing the PDS
   * table, the table data file name, and the name of the PDS table object.
   * It will extract the description of the columns and read the contents of the
   * table data file.
   *
   * If no table file is given or an empty QString is given for the table file,
   * the table location will be read from the label file.
   *
   * If no table name is given, the default name for the object is TABLE.
   *
   * This constructor may be used for ASCII or BINARY PDS tables.
   *
   * @param pdsLabFile Name of table label file
   * @param pdsTableName The name of the table object in the PDS file.
   */
  AmicaImportFits::AmicaImportFits(const FileName &fitsFile,
                                 const QString &fitsLabelName) {
    init();
    load(fitsFile.expanded(), fitsLabelName);
  }

  /**
   * Destructs the AmicaImportFits object.
   */
  AmicaImportFits::~AmicaImportFits() {
  }

  /** Return the number of samples in the FITS image */
  int AmicaImportFits::samples() const {
    return (m_samples);
  }

  /** Return the number of lines in the FITS image */
  int AmicaImportFits::lines() const {
    return (m_lines);
  }

  /** Return the number of bands in the FITS image */
  int AmicaImportFits::bands() const {
    return (m_bands);
  }

  /** Returns the FITS label in a group name specified in the constructor */
  PvlGroup AmicaImportFits::label() const {
    return (m_label);
  }


  /**
   * @brief Loads a PDS table label and (optional) data file
   *
   * This method will load a PDS table dataset using a label file describing the
   * contents of the table data.  The caller can provide the table data file,
   * otherwise, the location of the table data is extracted from the ^TABLE_NAME
   * keyword in the provided labels.  The table data is then loaded.
   *
   * This method needs to be called if the default constructor is used.
   * Otherwise, it is invoked in the constructor that takes the label, table
   * file, and table name. This method may be used to overwrite the label and
   * table file used. When it is invoked, the current contents of the object are
   * discarded.
   *
   * This method is used for ASCII or BINARY PDS tables.
   *
   * @param pdsLabFile Name of PDS table label file
   * @param pdsTableFile Name of PDS table data file to be imported into Isis
   *                (optional)
   */
  void AmicaImportFits::load(const QString &fitsfile,
                       const QString &fitsLabelName) {

    init();

    //  Set up file state
    m_file = fitsfile;

      // Setup to read headers/labels
    ifstream input;
    input.open(m_file.expanded().toLatin1().data(), ios::in | ios::binary);

  // Check stream open status
    if ( !input.is_open() ) {
      QString msg = "Cannot open input file [" + fitsfile + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    //  Parse the labels
    m_label = parseLabel(input, fitsLabelName);

    //  Get data dimensions
    int naxis = std::stoi(m_label["NAXIS"][0]);
    if (naxis == 2) {
      m_samples = std::stoi(m_label["NAXIS1"][0]);
      m_lines = std::stoi(m_label["NAXIS2"][0]);
      m_bands = 1;
    }
    else if (naxis == 3) {
      m_samples = std::stoi(m_label["NAXIS1"][0]);
      m_lines = std::stoi(m_label["NAXIS2"][0]);
      m_bands = std::stoi(m_label["NAXIS3"][0]);
    }
    else {
      std::string msg = "NAXIS count of [" + m_label["NAXIS"][0] +
                    "] is not supported at this time";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    return;
  }


  /**
   * Initialize object variables
   *
   * This method is reentrant.
   *
   */
  void AmicaImportFits::init() {
    m_file = "";
    m_lines = m_samples = m_bands = 0;
    m_label = PvlGroup("FitsLabel");
    return;
  }

/**
 * @brief Import a FITS label in to PvlGroup
 *
 * @author 2013-11-07 kbecker
 *
 * @param input        Input stream to read label from
 * @param fitLabelName Name of group to parse and store FITS label into
 *
 * @return PvlGroup Returns the FITS keywords in this group
 */
  PvlGroup AmicaImportFits::parseLabel(std::ifstream &input,
                                  const QString &fitLabelName) {

    char reading[81];
    IString line = "";
    unsigned int place = 0;
    PvlGroup labels(fitLabelName.toStdString());

    // Load first line
    input.seekg(0);
    input.read(reading, 80);
    reading[80] = '\0';
    line = reading;
    place += 80;

    // Read in and place in PvlKeywords and a PvlGroup
    while(line.substr(0, 3) != "END") {
      // Check for blank lines
      if(line.substr(0, 1) != " " && line.substr(0, 1) != "/") {
        // Name of keyword
        PvlKeyword label(line.Token(" ="));
        // Remove up to beginning of data
        line.TrimHead(" ='");
        line.TrimTail(" ");
        if(label.name() == "COMMENT" || label.name() == "HISTORY") {
          label += line;
        }
        else {
          // Access the data without the comment if there is one
          IString value = line.Token("/");
          // Clear to end of data, including single quotes
          value.TrimTail(" '");
          label += value;
          line.TrimHead(" ");
          // If the remaining line QString has anything, it is comments.
          if(line.size() > 0) {
            label.addComment(line);
            // A possible format for units, other possiblites exist.
            if(line != line.Token("[")) {
              label.setUnits(line.Token("[").Token("]"));
            }
          }
        }
        labels += label;
      }
      // Load next line
      input.seekg(place);
      input.read(reading, 80);
      reading[80] = '\0';
      place += 80;
      line = reading;
    }

    return (labels);
  }

}  //  namespace Isis
