/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: CsvWriterStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "CsvWriterStrategy.h"

// std library
#include <iomanip>
#include <iostream>

// Qt library
#include <QByteArray>

// geos library
#include <geos_c.h>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "FileName.h"
#include "GisTopology.h"
#include "IException.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   * Constructs a Strategy object whose name and type are "CsvWriter". 
   */ 
  CsvWriterStrategy::CsvWriterStrategy() : Strategy("CsvWriter", "CsvWriter"),
                                           m_keywords(),
                                           m_header(true), m_delimiter(","),
                                           m_default("NULL"), 
                                           m_skipEmptyLists(false),
                                           m_gisKey(), m_gisType("wkb") { 
  }
  

  /**
   * @brief Constructor loads from a Strategy object CsvWriter definition
   *  
   * This constructor loads and retains processing parameters from the CsvWriter
   * Strategy object definition as (typically) read from the configuration file.
   * Defaults to GIS type of well-known binary. 
   *  
   * @param definition CsvWriter Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  CsvWriterStrategy::CsvWriterStrategy(const PvlObject &definition, 
                                       const ResourceList &globals) : 
                                       Strategy(definition, globals),
                                       m_keywords(),
                                       m_header(true), m_delimiter(","),
                                       m_default("NULL"), 
                                       m_skipEmptyLists(false),
                                       m_gisKey(), m_gisType("wkb") {
    PvlFlatMap parms( getDefinitionMap() );
    m_mode           = parms.get("Mode", "Create").toLower();
    m_keywords       = parms.allValues("Keywords");
    m_header         = toBool(parms.get("Header", "true"));
    m_delimiter      = parms.get("Delimiter", ",");
    m_default        = parms.get("DefaultValue", "NULL");
    m_skipEmptyLists = toBool(parms.get("SkipEmptyLists", "false"));

    if ( parms.exists("GisGeometryRef") ) {
      m_gisKey = parms.get("GisGeometryRef").toLower();
    }
    if ( parms.exists("GisGeometryKey") ) {
      m_gisKey = parms.get("GisGeometryKey").toLower();
    }

    m_gisType = parms.get("GisType", "wkb").toLower();

  }
  

  /** 
   * Destructor. 
   */ 
  CsvWriterStrategy::~CsvWriterStrategy() {
  }


  /**
   * @brief Writes the Resources to the CSV file as rows.
   *
   * Writes the keyword values of the Resources as rows to the CSV file.
   * If specified, the columns of the first row, known as the header, will
   * contain the keyword names. For each resource, the rows will contain
   * corresponding keyword values in each column. Fields are delimited by the
   * delimiter character and records are delimited by a new line.
   *
   * @param resources ResourceList of the Resources to be written to the CSV
   *                  file.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int The number of Resources written to the CSV.
   * @throw IException::Programmer "CsvWriter::Cannot open/create output file."
   */
  int CsvWriterStrategy::apply(ResourceList &resources, const ResourceList &globals) {
  
    //  Check for handling of empty lists
    if ( ( true == m_skipEmptyLists) ) {
      if ( resources.size() <= 0 ) {
        return (0);
      }
    }
    //  Check for argument replacement
    QString fname = translateKeywordArgs("CsvFile", globals);
    if ( isDebug() ) { 
      cout << "CsvWriter::Filename = " << fname << "\n"; 
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
      std::string mess = "CsvWriter::Cannot open/create output file [" + fname +
                      "].";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
  
    //  Write out header if requested
    if ( m_header ) {
      QString delimiter = "";
      BOOST_FOREACH ( QString column, m_keywords ) {
        os << delimiter << column;
        delimiter = m_delimiter;
      }
      os << "\n";
    }
  
  
    // Write out each keyword value to column
    initProgress( ( isApplyToDiscarded() ) ? resources.size() : countActive(resources) ); 
    int result = 0;
     BOOST_FOREACH ( SharedResource resource, resources ) {
      if ( resource->isDiscarded() ) {
        if ( isApplyToDiscarded() ) {
          csvwrite(os, resource, m_keywords, m_delimiter, m_default,
                   m_gisKey, m_gisType, globals);
          result++;
          processed();
        }
      }
      else {
        csvwrite(os, resource, m_keywords, m_delimiter, m_default,
                 m_gisKey, m_gisType, globals);
        result++;
        processed();
      }
    }
  
    //  Done with the file
    os.close();
    return (result);
  }
  

  /**
   * @brief Write a single Resource to a row in the CSV file.
   *
   * Writes a Resource to a CSV formatted file as a row. The specified keyword
   * values of the Resource are wriiten as columns in the row. If a specified
   * keyword cannot be found in the Resource the default value is written
   * instead. If the geometry keyword is specified and the resource does not
   * already have a keyword with the same name, the geometry of the resource
   * will be written to that column in a text format. The columns of the
   * written row are delimited by the delimiter character and the row is
   * terminated with a new line.
   *
   * @param os        ofstream of the file to write the row to.
   * @param resource  SharedResource containing the keyword values.
   * @param keywords  QStringList of the keywords whose corresponding values
   *                  are to be writen to the CSV file.
   * @param delimiter The delimeter used to separate columns in the row.
   * @param defValue  The default value used if a keyword in the header does
   *                  not exist in the Resource
   * @param gisKey    QString of the keyword used to reference the geometry.
   * @param gisType   QString of the text format to write the geometry in.
   */
  void CsvWriterStrategy::csvwrite(ofstream &os, SharedResource &resource, 
                                   const QStringList &keywords, 
                                   const QString &delimiter, 
                                   const QString &defValue,
                                   const QString &gisKey,
                                   const QString &gisType,
                                   const ResourceList &globals) {
    QString myDelimiter = "";

    // Initialize geometry if it exists
    const GEOSGeometry *geos = 0;
    if ( resource->hasGeometry() ) {
      if ( resource->geometry()->isDefined() ) {
        geos = resource->geometry()->geometry();
      }
    }

    // Initialize resources
    GisTopology *gis(GisTopology::instance());
    ResourceList parameters;
    parameters.push_back(resource);
    parameters.append(globals);

    // Get values for all fields
    BOOST_FOREACH ( QString field, keywords ) {
      QString value = defValue;

      // First check for a geometry key to process as it would take precedence
      // in case another keyword of the same name exists in the resource
      if ( field.toLower() == gisKey ) {
        if ( geos ) {
          if ("wkt" == gisType) { 
            value = gis->wkt(geos, GisTopology::PreserveGeometry);
          }
          else if ("wkb" == gisType) {
            value = gis->wkb(geos, GisTopology::PreserveGeometry);
          }
          else {
            std::string mess = "Unsupported geometry type: " + gisType;
            throw IException(IException::User, mess, _FILEINFO_);
          }
        }
      }
      else {
        // Get the keyword from the Resource/global parameters
        value = findReplacement(field, parameters, 0, defValue);
      }

      os << myDelimiter << value;
      myDelimiter = delimiter;
    }

    os << "\n";
    return;
  }

}  //namespace Isis
