/**                                                                       
 * @file                                                                  
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: PdsTableCreatorStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
#include "PdsTableCreatorStrategy.h"

// std library
#include <iomanip>
#include <iostream>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "FileName.h"
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
  PdsTableCreatorStrategy::PdsTableCreatorStrategy() : 
                                  Strategy("PdsTableCreator", "PdsTableCreator"), 
                                  m_parameters(), m_columns(),  
                                  m_delimiter(","), m_null("NULL") { 
  }
  
  
  /**
   * @brief Constructor loads from a Strategy object 
   *        PdsTableCreator definition
   *  
   * This constructor loads and retains processing parameters from 
   * the PdsTableCreator Strategy object definition as (typically)
   * read from the configuration file. 
   *  
   * @param definition PdsTableCreator Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   * 
   * @author 2012-07-15 Kris Becker
   */ 
  PdsTableCreatorStrategy::PdsTableCreatorStrategy(const PvlObject &definition, 
                                                  const ResourceList &globals) : 
                                                  Strategy(definition, globals), 
                                                  m_parameters(new Resource("PdsTableCreator",definition)), 
                                                  m_columns(),  m_delimiter(","), 
                                                  m_null("NULL") {
  
    // Verify input parameters
    m_delimiter = m_parameters->value("Delimiter", ",");
    m_mode      = m_parameters->value("Mode", "Create").toLower();
    m_null      = m_parameters->value("DefaultValue", "NULL");
    return;
  }
  
  
  /** 
   * Destructor.
   */  
  PdsTableCreatorStrategy::~PdsTableCreatorStrategy() { 
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
  int PdsTableCreatorStrategy::apply(ResourceList &resources, 
                                    const ResourceList &globals) {
  
    QString fmtfile = translateKeywordArgs("PdsFormatFile", getGlobals(m_parameters, globals));
    if ( isDebug() ) { 
      cout << "PdsTableCreator::PdsFormatFile = " << fmtfile.toStdString() << "\n"; 
    }
    Pvl fmtpvl(fmtfile.toStdString());
    readColumns(fmtpvl);
  
      //  Check for argument replacement
    QString fname = translateKeywordArgs("PdsTableFile", getGlobals(m_parameters, globals));
    if ( isDebug() ) { 
      cout << "PdsTableCreator::PdsTableFile = " << fname.toStdString() << "\n"; 
    }
  
    //  Now open the filename
    ofstream os;
    QString ofFile = QString::fromStdString(FileName(fname.toStdString()).expanded());
    QByteArray qofFile = ofFile.toLatin1();
    if ( "append" == m_mode ) {
      os.open(qofFile.data(), ios::out | ios::app);
    }
    else {
      os.open(qofFile.data(), ios::out | ios::trunc);
    }
  
    if ( !os.is_open() ) {
      std::string mess = "PdsTableCreator::Cannot open/create output file (" + 
                     fname.toStdString() + ")";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  
    // For convenience, convert all column resources to PdsColumns
    QList<PdsColumn *> columns;
    BOOST_FOREACH ( SharedResource colresource, m_columns ) {
      columns.append(PdsColumn::promote(colresource));
    }
  
    // Process all active Resources
    int nrows = 0;
    BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( !resource->isDiscarded() ) {
        QString separator = "";
  
        QString row = "";
        for (int c = 0; c < columns.size(); c++) {
          PdsColumn *col = columns[c];
          row += separator + col->formattedValue(resource, m_null);
          separator = m_delimiter;
        }
        os << row.toStdString() << "\n";
        nrows++;
      }
    }
  
    os.close();
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
  int PdsTableCreatorStrategy::apply(SharedResource &resource,
                                     const ResourceList &globals) {
    throw IException(IException::Programmer,
                     "Should not be calling apply(SharedResource &resource)!",
                     _FILEINFO_);
    //return (0);
  }


  /** 
   * Reads the Column resources from the input pvl and stores them 
   * in the global variable m_columns. 
   *  
   * @param pvl The pvl files that contains column resources.
   *  
   * @return int Returns the size of m_columns. 
   *  
   * @author 2012-07-15 Kris Becker 
   */  
  int PdsTableCreatorStrategy::readColumns(PvlObject &pvl) {
    QString colobj = m_parameters->value("ColumnObject", "");
    PvlObject &obj = ( colobj.isEmpty() ? pvl : pvl.findObject(colobj.toStdString(), PvlObject::Traverse) );
  
    PvlObject::PvlObjectIterator pvlcol = obj.beginObject();
    while ( pvlcol != obj.endObject() ) {
      if ( pvlcol->isNamed("COLUMN") ) {
        m_columns.append(SharedResource(new PdsColumn(*pvlcol)));
      }
      ++pvlcol; 
    }
    return ( m_columns.size() );
  }


  /** 
   * Determines the size of the delimiter between columns. 
   *  
   * @param columns The column resources in the file. 
   *  
   * @return int Returns the size of the largest delimiter. 
   *  
   * @author 2012-07-15 Kris Becker 
   */  
  int PdsTableCreatorStrategy::delimiterSize(ResourceList &columns) const {
    int dsize = 0;
    for ( int i = 1 ; i < columns.size() ; i++) {
      PdsColumn *col0 = PdsColumn::promote(columns[i-1]);
      PdsColumn *col1 = PdsColumn::promote(columns[i]);
  
      dsize = std::max(dsize, 
                       (col1->startByte()-col0->endByte()-col1->isQuoted())); 
    }
    return (dsize); 
  }

}  //namespace Isis
