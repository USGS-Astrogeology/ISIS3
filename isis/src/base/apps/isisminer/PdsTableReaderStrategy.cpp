/**                                                                       
 * @file                                                                  
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: PdsTableReaderStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
#include "PdsTableReaderStrategy.h"

// std library
#include <iomanip>
#include <iostream>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "PdsColumn.h"
#include "Pvl.h"
#include "PvlObject.h"
#include "Resource.h"
#include "Strategy.h"

using namespace std;

namespace Isis {

  /** 
   * Default constructor.
   */  
  PdsTableReaderStrategy::PdsTableReaderStrategy() : 
                                  Strategy("PdsTableReader", "PdsTableReader"), 
                                  ImportPdsTable(), m_resources(), m_globals(), 
                                  m_delimiter(","){ }
  
  
  /**
   * @brief Constructor loads from a Strategy object 
   *        PdsTableReader definition
   *  
   * This constructor loads and retains processing parameters from 
   * the PdsTableReader Strategy object definition as (typically)
   * read from the configuration file. 
   *  
   * @param definition PdsTableReader Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   * 
   * @author 2012-07-15 Kris Becker
   */ 
  PdsTableReaderStrategy::PdsTableReaderStrategy(const PvlObject &definition, 
                                                  const ResourceList &globals) : 
                                                  Strategy(definition, globals), 
                                                  ImportPdsTable(),
                                                  m_resources(), m_globals(),   
                                                  m_delimiter(",") {
  
    // Verify input parameters
    PvlFlatMap parms(getDefinitionMap());
    m_delimiter = translateKeywordArgs("Delimiter", globals,"");
    m_identity  = translateKeywordArgs("Identity", globals,"Row");
    m_table  = translateKeywordArgs("TableName", globals,"");
    if ( !m_table.isEmpty() ) ImportPdsTable::setName(m_table);
    m_useFormatted = toBool(parms.get("UseFormattedName", "true"));
    return;
  }
  
  
  /** 
   * Destructor.
   */  
  PdsTableReaderStrategy::~PdsTableReaderStrategy() { 
  }
  
  
  /** 
   * Opens the PDS table file, converts all column resources to 
   * PdsColumns, and writes all active resources to the file. 
   * Resource entires are delimited by the delimiter specified. 
   *  
   * @param resources The resources that are used to create the 
   *                  PDS Table.
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return int The number of rows in the PDS table.
   *  
   * @author 2012-07-15 Kris Becker 
   */  
  int PdsTableReaderStrategy::apply(ResourceList &resources, 
                                    const ResourceList &globals) {
  
    QString lblfile = translateKeywordArgs("PdsLabelFile", globals);
    if ( isDebug() ) { 
      cout << "PdsTableReader::PdsLabelFile = " << lblfile << "\n"; 
    }
      //  Check for argument replacement
    QString tblfile = translateKeywordArgs("PdsTableFile", globals,"");
    if ( isDebug() ) { 
      cout << "PdsTableReader::PdsIndexFile = " << tblfile << "\n"; 
    }
  
    //  Now open the filename
    m_resources.clear();
    m_globals = globals;
    int nrows(0);
    try {
      QString tblName  = translateKeywordArgs("TableName", globals, m_table);
      load(lblfile, tblfile, tblName); 
      nrows = m_resources.size();
      resources.append(m_resources);
      m_resources.clear();
      m_globals.clear();
    }
    catch ( IException &ie ) {
      m_resources.clear();
      m_globals.clear();
      std::string mess = "Failed to read PDS label/table " + lblfile + "," + tblfile;
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }
 
    return ( nrows );
  }
  

  
  /**
   * Throws an error if a single resource is entered. If this 
   * error is thrown, the Resource is kept. 
   *  
   * @param resource A single resource. 
   *  
   * @return This function will throw an error if called.
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @author 2012-07-15 Kris Becker 
   */  
  int PdsTableReaderStrategy::apply(SharedResource &resource,
                                     const ResourceList &globals) {
    throw IException(IException::Programmer,
                     "Should not be calling apply(SharedResource &resource)!",
                     _FILEINFO_);
    //return (0);
  }

bool PdsTableReaderStrategy::processRow(const int &row, 
                                        const QString &rowdata) {

  PvlFlatMap parms( getDefinitionMap() );
  QStringList columns(getColumnNames(m_useFormatted));
  if ( parms.exists("Columns") ) {
    columns = parms.allValues("Columns");
  }

  QString crow = QString::number(row);
  SharedResource resource(new Resource(crow));
  resource->add("Row", crow);
  BOOST_FOREACH (QString column, columns ) {
    ColumnDescr *description = findColumn(column);
    PvlKeyword colkey = PvlKeyword(column.toStdString());
    if ( 0 != description ) {
      QStringList fields = getColumnFields(rowdata, *description, m_delimiter);
      BOOST_FOREACH (QString field, fields ) {
        colkey.addValue(field.trimmed().toStdString());
      }
    }
    resource->add(colkey);
  }

  // Determine identity - use row number if not specified
  QString id = translateKeywordArgs("Identity", 
                                    getGlobals(resource, getGlobalDefaults()), 
                                    crow);
  resource->setName(id);
  m_resources.append(resource);
  return (true);

}

}  //namespace Isis
