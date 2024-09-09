/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: CsvReaderStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "CsvReaderStrategy.h"

// other ISIS
#include "CSVReader.h"
#include "IException.h"
#include "PvlFlatMap.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   *  Default Constructor
   */
  CsvReaderStrategy::CsvReaderStrategy() : Strategy("CsvReader", "CsvReader"), 
                                           m_hasHeader(false), 
                                           m_ignoreComments(false),
                                           m_delimiter(","), 
                                           m_rowBase("Row") { 
  }
  

  /**
   * @brief Constructor loads from a Strategy object CsvReader definition
   *
   * This constructor loads and retains processing parameters from the CsvReader
   * Strategy object definition as (typically) read form the configuration file.
   *
   * @param definition CsvReader Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  CsvReaderStrategy::CsvReaderStrategy(const PvlObject &definition, 
                                       const ResourceList &globals) : 
                                       Strategy(definition, globals),
                                       m_hasHeader(false), m_ignoreComments(false),
                                       m_delimiter(","), m_rowBase("Row") {
  
    //  Want to do this to check validity at creation time - not run time
    PvlFlatMap parms( getDefinitionMap() );
    m_hasHeader      = toBool(parms.get("HasHeader", "false").toStdString());
    m_ignoreComments = toBool(parms.get("IgnoreComments", "false").toStdString());
    m_skipLines      = toInt(parms.get("SkipLines", "0").toStdString());
    m_delimiter      = parms.get("Delimiter", ",");
    m_rowBase        = parms.get("RowBaseName", "Row"); 
  
    //  Check for valid delimiter
    if ( m_delimiter.size() != 1 ) {
      std::string mess = "Delimiter value (" + m_delimiter.toStdString() + ") must be one and only"
                     " one value - try again";
      throw IException(IException::User, mess, _FILEINFO_);
    }
    return;
  }
  

  /** 
   *  Destructor
   */
  CsvReaderStrategy::~CsvReaderStrategy() {
  }

 
  /**
   * @brief Creates Resources from the rows of the CSV file
   *
   * Creates Resources from the rows of the CSV file with the keyword names from
   * the header, normally the header of the CSV file. The keyword values are the
   * values in the corresponding columns of each row.
   *
   * @param resources The ResourceList to add the created resources to.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int Returns the number of Resources created.
   */
  int CsvReaderStrategy::apply(ResourceList &resources, const ResourceList &globals) { 
  
    CSVReader csv;
  
    // Apply conditions
    csv.setComment(m_ignoreComments);
    csv.setSkip(m_skipLines);
    csv.setHeader(m_hasHeader);
    csv.setDelimiter(m_delimiter[0].toLatin1());
    if ( ' ' == m_delimiter[0] ) csv.setSkipEmptyParts();  // needed if space
  
    //  Fetch input file name
    QString fname = translateKeywordArgs("CsvFile", globals);
    if ( isDebug() ) { 
      cout << "CsvReader::Filename = " << fname.toStdString() << "\n"; 
    }
    try {
      csv.read(fname);
    } 
    catch (IException &ie) {
      std::string mess =  "Could not read CSV file \'" + fname.toStdString() + "\'";
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }
  
    //  Get the header and index mapping
    ColumnHeader header = makeHeader(&csv);
  
    //  Now ready to read all row/columns and convert to Resources
    int nrows = csv.rows();
    int ncols = header.size();
    if ( isDebug() ) {
      cout << "CsvReader::Rows(" << nrows << "), Columns(" << ncols << ")\n";
    }
    for ( int row = 0 ; row < nrows ; row++ ) {
      CSVReader::CSVAxis csvrow = csv.getRow(row);
  
      // Create new Resource
      QString rowId(m_rowBase + QString::number(row));
      SharedResource rowsrc(new Resource(rowId));
  
      // Populate resource
      for ( int column = 0 ; column < ncols ; column++ ) {
        int index = header[column].second;
        if ( index < csvrow.dim() ) {
          rowsrc->add(header[column].first, csvrow[index]); 
          if ( isDebug() ) {
            cout << "CsvReader::Column::" << header[column].first.toStdString() << "[" 
                 << index << "] = " << csvrow[index].toStdString() << "\n";
          }
        }
      }
  
      //  Now determine Identity
      QString identity = translateKeywordArgs("Identity", getGlobals(rowsrc, globals));
      if ( identity.isEmpty() ) {
         identity = rowId;
      }

      rowsrc->setName(identity); 
      if ( isDebug() ) { 
       cout << "  CsvReader::Resource::" << rowId.toStdString() << "::Identity = " 
             << identity.toStdString() << "\n"; 
      }
  
      // Import geometry w/exception handling
      //  Check for Geometry.  May want to remove it after parsing/conversion.  
      //  These text geometries tend to be huge and consume lots of memory.
      try {
        importGeometry(rowsrc, getGlobals(rowsrc, globals));
      }
      catch (IException &ie) {
        std::string mess = "Geometry conversion failed horribly for Resource(" + 
                       identity.toStdString() + ")";
        throw IException(ie, IException::User, mess, _FILEINFO_);
      }
  
      // Export the current Row Resource for subsequent processing
      resources.append(rowsrc);
    }
  
    return (nrows);
  } 
  

  /**
   * @brief Create the Resource keyword names from the header of the CSV file
   *
   * Creates the keyword names for the Resources, usually form the header of the
   * CSV file header. If the CSV file has a header then the keywords are created
   * from the colums of that header. If the CSV file does not have a header then
   * the keyword names are created from the base column name, which defaults to
   * "Column", and a number, such as "Column1", "Column2", etc.
   * 
   * @author 2012-07-15 Kris Becker 
   *
   * @param csv The CSVReader which has read the contents of the CSV file 
   *
   * @return CsvReaderStrategy::ColumnHeader Returns the header
   */
  CsvReaderStrategy::ColumnHeader CsvReaderStrategy::makeHeader(
                                                            const CSVReader *csv
                                                               ) const {
  
    PvlFlatMap keys( getDefinitionMap() );  // Flatten parameter keywords
  
    //  Now get size of columns and declare return header/index
    int ncols = csv->columns(); 
    ColumnHeader header;
  
    // Now need to resolve column headers as they now serve as keyword names in
    // Resources generated from each row.
    if ( keys.exists("Header") ) {
      QStringList names = keys.allValues("Header");
      if (keys.exists("Index") ) {
        QStringList indexes = keys.allValues("Index");
        if ( names.size() != indexes.size() ) {
          std::string mess = "Size of Header (" + toString(names.size()) +
                         " does not match size of Index (" +
                          toString(indexes.size()) + ")";
          throw IException(IException::User, mess, _FILEINFO_);
        }
        
        for ( int i = 0 ;  i < names.size() ; i++ ) {
          int index = indexes[i].toInt();
          if ( (index < 0) || (index >= ncols) ) {
            std::string mess = "Column " + names[i].toStdString() + " index (" + indexes[i].toStdString() +
                           ") exceeds input column size (" + 
                           toString(ncols) + ")";
            // throw IException(IException::User, mess, _FILEINFO_);
            // Not an error if we handle the input conditions properly
            if ( isDebug() ) {
              cout << mess << "\n"; 
            }
          }
  
          header.push_back(qMakePair(names[i], index));
        }
      }
      else {
        if ( names.size() > ncols ) {
          std::string mess = "Size of Header (" + toString(names.size()) +
                          ") exceeds input column size (" + 
                           toString(ncols) + 
                          ") - must provide Index otherwise";
          // throw IException(IException::User, mess, _FILEINFO_);
          // Not an error if we handle the input conditions properly
          if ( isDebug() ) {
            cout << mess << "\n"; 
          }
        }
  
        for ( int i = 0 ; i < names.size() ; i++ ) {
          header.push_back(qMakePair(names[i], i));
        }
        
      }
    }
    else if ( m_hasHeader ) {
      CSVReader::CSVAxis head = csv->getHeader();
      for ( int i = 0 ; i < head.dim() ; i++ ) {
        header.push_back(qMakePair(head[i].remove(QChar(' ')), i));
      }
    }
    else { 
     // No header in CSV, must create column header/keyword names
      QString base = keys.get("ColumnBaseName", "Column");
      for ( int i = 0 ;  i < ncols ; i++) {
        header.push_back(qMakePair(base + QString::number(i), i));
      }
    }
  
    return (header);
    
  }
  
  
  /**
   * @brief Finds a name in a header.
   *
   * Returns the index of a name in the column header. If the name is not found it
   * returns -1. The name is not case-sensitive.
   *
   * @param name   The name of the column to find.
   * @param header The column header to search in.
   *
   * @return int Returns the index of name in ColumnHeader. If the name is not
   *             found it returns -1.
   */
  int CsvReaderStrategy::findColumnHeader(const QString &name, 
                                          const CsvReaderStrategy::ColumnHeader &header) 
                                          const {
    QString column = name.toLower();
    for ( int i = 0 ;  i <  header.size() ; i++ ) {
      if ( header[i].first.toLower() == column ) {
        return (i);
      }
    }
  
    //  If not found
    return (-1);
  }

}  //namespace Isis
