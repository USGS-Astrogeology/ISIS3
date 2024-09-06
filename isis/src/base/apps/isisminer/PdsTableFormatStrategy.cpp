/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: PdsTableFormatStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
 * 
 *   Unless noted otherwise, the portions of Isis written by the USGS are 
 *   public domain. See individual third-party library and package descriptions 
 *   for intellectual property information, user agreements, and related  
 *   information.                                                         
 *                                                                        
 *   Although Isis has been used by the USGS, no warranty, expressed or   
 *   implied, is made by the USGS as to the accuracy and functioning of such 
 *   software and related material nor shall the fact of distribution     
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.                                        
 *                                                                        
 *   For additional information, launch                                   
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html                
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.                                    
 */ 
#include "PdsTableFormatStrategy.h"

// other ISIS
#include "IException.h"
#include "PdsColumn.h"
#include "Pvl.h"
#include "PvlFormatPds.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"

using namespace std;

namespace Isis {

  /** 
   * Default constructor.
   */ 
  PdsTableFormatStrategy::PdsTableFormatStrategy() : 
                                  Strategy("PdsTableFormat", "PdsTableFormat"), 
                                  m_parameters(), m_columns(), 
                                  m_delimiter(",") { 
  }
  

  /**
   * @brief Constructor loads from a Strategy object 
   *        PdsTableFormat definition
   *  
   * This constructor loads and retains processing parameters from 
   * the PdsTableFormat Strategy object definition as (typically) 
   * read from the configuration file. 
   *  
   * @author 2012-07-15 Kris Becker
   * 
   * @param definition PdsTableFormat Strategy PVL object 
   *                   definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  PdsTableFormatStrategy::PdsTableFormatStrategy(const PvlObject &definition, 
                                                 const ResourceList &globals) : 
                                                 Strategy(definition, globals), 
                                                 m_parameters(new Resource("PdsTableFormat",definition)), 
                                                 m_columns(),  m_delimiter(",") {
  
    // Verify input parameters
    makeColumns(m_parameters);
    m_columns.clear();
    m_delimiter = m_parameters->value("Delimiter", ",");
    return;
  }
  

  /** 
   * Destructor.
   */ 
  PdsTableFormatStrategy::~PdsTableFormatStrategy() {
  }


  /**
   * @brief Creates columns according to the byte size of the 
   *        resources and saves that data to a PDS table format
   *        file.
   *
   * Creates columns according to the byte size of the resources 
   * and saves the format to the PDS table format file. Column 
   * entries are delimited by the delimiter. 
   *
   * @author 2012-07-15 Kris Becker
   *
   * @param resources The list of Resources that determine column 
   *                  size.
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return int The size of the Resources written to the PDS 
   *         table 
   */
  int PdsTableFormatStrategy::apply(ResourceList &resources, 
                                    const ResourceList &globals) {
  
    QString fmtfile = translateKeywordArgs("PdsFormatFile", 
                                           getGlobals(m_parameters, globals));

    QString keyfmt = translateKeywordArgs("KeywordFormatFile", 
                                          getGlobals(m_parameters, globals),
                                          "$ISISROOT/appdata/translations/pdsExportRootGen.typ");


    // Remake new columns each time
    m_columns.clear();
    makeColumns(m_parameters);
  
    int pos = 1;
    QString separator("");

    Pvl fmtpvl;
    // Apply only if provided
    if ( !keyfmt.isEmpty() ) {
      fmtpvl.setFormat(new PvlFormatPds(keyfmt.toStdString())); 
    }
    fmtpvl.setTerminator("END");

    for (int c = 0 ; c < m_columns.size() ; c++) {
      PdsColumn *column = PdsColumn::promote(m_columns[c]);
    
      // Get the size of the column
      int datalen = column->bytes();
      if ( 0 <= datalen ) {
        datalen = column->bytes(resources); 
        column->setBytes(datalen);
      }
  
      // Determine formatted data size
      int nquotes = column->isQuoted();
      pos += (separator.size() + nquotes);
      column->setStartByte(pos);
  
      // Update for size
      pos += (datalen + nquotes);
      separator = m_delimiter;
  
      fmtpvl.addObject(column->toPvl());
    }
  
    // Now write the Pvl column file
    fmtpvl.write(fmtfile.toStdString());
  
    return (resources.size());
  }
  
  /**
   * @brief Throws an error if SharedResource &resource is 
   *        entered.
   *
   * Throws an error if SharedResource &resource is entered. 
   * If this error is thrown, the Resource is kept.
   *
   * @author 2012-07-15 Kris Becker
   *
   * @param resource A Resource that if passed in with throw an 
   *                 error.
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return int 0, which keeps the Resource.
   * @throw IException::Programmer "Should not be calling apply(SharedResource &resource)."
   */
  int PdsTableFormatStrategy::apply(SharedResource &resource, const ResourceList &globals) { 
    throw IException(IException::Programmer, 
                     "Should not be calling apply(SharedResource &resource).",
                     _FILEINFO_);
  
    //  Keeps the Resource
    return (0);
  }
  

  /**
   * @brief Goes through the Pds file and saves the column 
   *        information into the Pds format file.
   *
   * Goes through the Pds file and saves the column information 
   * into the Pds format file. Column entries are delimited by the 
   * delimiter. 
   *
   * @author 2012-07-15 Kris Becker
   *
   * @param definition PdsTableFormat Strategy PVL object 
   *                   definition
   *  
   */
  void PdsTableFormatStrategy::makeColumns(SharedResource &definition) {
  
    int nColumns(definition->count("Column"));
    validate("DataType", definition, nColumns, nColumns);
    validate("Bytes", definition, nColumns, 0);
    int nfmt = validate("Format", definition, nColumns, 0);
    int nunit = validate("Unit", definition, nColumns, 0);
    validate("DataDescription", definition, nColumns, 0);
  
    for ( int i = 0 ; i < nColumns ; i++) {
      QString name = definition->value("Column", i);
      SharedResource column(new PdsColumn(name));
      column->add("COLUMN_NUMBER", toString(i+1));
  
      column->add("NAME", name);
  
      column->add("DATA_TYPE", definition->value("DataType", i));
  
      column->add("BYTES", definition->value(name+"_Bytes", 
                                             definition->value("BYTES", "0", i)));
  
      if ( nfmt > i ) { 
        column->add("FORMAT", definition->value("Format", i));
      }
      if ( nunit > i ) { 
        column->add("UNIT", definition->value("Unit", i)); 
      }
  
      column->add("DESCRIPTION", definition->value("DataDescription", "NULL", i)); 
      m_columns.append(column);
    } 
  }
  
   
  /**
   * @brief Validates that the number of entries for a given 
   *        resource type is within the maximum and minimum values
   *        set.
   *
   * Validates that the number of entries for a given resource 
   * type is within the maximum and minimum values set. If 
   * throwOnError is true and the count of keynam is equal to 
   * maxcols or mincols, an error is thrown. If an error isn't 
   * thrown,validate returns the count of keynam if it is equal to
   * mincols or maxcols, otherwise -1 is returned. 
   *
   * @author 2012-07-15 Kris Becker
   *  
   * @param keynam A keyword that contains the name of a resource 
   *               type within definition.
   * @param definition PdsTableFormat Strategy PVL object 
   *                   definition
   * @param maxcols An int containing the maximum number of 
   *                columns.
   * @param mincols An int containing the minimum number of 
   *                columns.
   * @param throwOnError A boolean value that allows the user to 
   *                     determine if they want an error thrown if
   *                     certain conditions are met.
   *  
   * @return int Returns the count of keynam, if it is equal to
   *             mincols or maxcols. Otherwise an error is thrown or
   *             -1 is returned.
   * @throw IException::User "The keyword count for the given 
   *                   Resource definition is invalid. Must equal
   *                   maxcols or mincols."
   */
  int PdsTableFormatStrategy::validate(const QString &keynam, 
                                        SharedResource &definition, 
                                        const int &maxcols, const int &mincols,
                                        const bool &throwOnError) {
  
    int nvals = definition->count(keynam);
    if ( !((maxcols == nvals) || (mincols == nvals)) ) {
      if ( throwOnError ) {
        std::string mess = "The keyword count [" + QString::number(nvals) + "] for the given " + keynam
                       + " Resource definition is invalid. Must equal maxcols [" 
                       + QString::number(maxcols) + "or mincols [" 
                       + QString::number(mincols) + "]."; 
        throw IException(IException::User, mess, _FILEINFO_);
      }
      return (-1);
    }
  
    // Has either maxcols or mincols
    return (nvals);
  }

}  //namespace Isis
