/**                                                                       
 * @file                                                                  
 * $Revision: 6513 $
 * $Date: 2016-01-14 16:04:44 -0700 (Thu, 14 Jan 2016) $
 * $Id: DatabaseReaderStrategy.cpp 6513 2016-01-14 23:04:44Z kbecker@GS.DOI.NET $
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
#include "DatabaseReaderStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "Database.h"
#include "DbProfile.h"
#include "IException.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"
#include "SqlQuery.h"
#include "SqlRecord.h"

using namespace std;

// Define a local Database disconnect function that will properly shut down
// a database connection and cleans up resources when the database is closed.
// This is required to remove the profile in the Qt database system. We will not
// make DB connections persistent. This is triggered in QScopedPointer.
struct DbDisconnect {
   static inline void cleanup(Isis::Database *db) {
      QString db_name = db->Name();
      delete db;
      Isis::Database::remove(db_name);
   }
};


namespace Isis {

  /** 
   * Default Constructor 
   */  
  DatabaseReaderStrategy::DatabaseReaderStrategy() : Strategy("DatabaseReader", 
                                                              "DatabaseReader"), 
                                                     m_db(0) { 
  }
  

  /** 
   * @brief Constructor loads from a Strategy PVL object.
   *
   * This constructor loads processing parameters from a Strategy PVL object
   * as read from a configuration file.
   *
   * @param definition Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */  
  DatabaseReaderStrategy::DatabaseReaderStrategy(const PvlObject &definition, 
                                                 const ResourceList &globals) : 
                                                 Strategy(definition, globals),
                                                 m_db(0) {
  
  }
  
  
  /** 
   * Destructor
   */  
  DatabaseReaderStrategy::~DatabaseReaderStrategy() {
  }


  /**
   * Creates resources from the Query. If the target parameter from the
   * Strategy object definition is "resource", new resources are are added to
   * the list of resources. If the target is "asset", then new resources a
   * stored as assets in the resources in the list.
   *
   * @param resources ResourceList of the Resources
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int The number of rows from the Query
   */  
  int DatabaseReaderStrategy::apply(ResourceList &resources, 
                                    const ResourceList &globals) { 


    // Initiate connection with autocleaner
    QScopedPointer<Database, DbDisconnect> db(connect(globals));
    m_db = db.data();
  
    if ( isDebug()  ) {  
      cout << "Database::connected..\n"; 
    }
  
    PvlFlatMap keys( getDefinitionMap() );
    //  Check for Resource or Asset query nature
    QString target = keys.get("Target").toLower();
  
    int nrows = 0;
    // Must ensure we don't leave open DB resouce...
    try {
      if ( "resource" == target ) {
        nrows = executeQuery(resources, globals);
        if ( isDebug() ) {
          cout << "\n\nTotal Rows loaded: " << nrows << "\n";
        }
      } 
      else {  //  ("asset" == target) 
        QString assetName = keys.get("Asset");
        if ( isDebug() ) { 
          cout << "Db:LoadingAssets(" << assetName << ") \n"; 
        }
    
        BOOST_FOREACH ( SharedResource resource, resources ) {
         if ( isDebug() ) { 
           cout << "  Db:AssetsResource(" << resource->name() << ") \n"; 
         }
          ResourceList assetList;
          nrows += executeQuery(assetList, getGlobals(resource, globals));
          if ( assetList.size() > 0 ) {
            QVariant asset(QVariant::fromValue(assetList));
            resource->addAsset(assetName, asset);
          }
        }
        if ( isDebug() ) {
          cout << "\n\nTotal Asset Rows loaded: " << nrows << "\n";
        }
      }
    }
    catch ( IException &ie ) {
      std::string mess = "Query failed after " + QString::number(nrows+1) + 
                     " rows: " + ie.what();
      if ( isDebug() ) {
        cout << "Db::Error - " << mess << "\n";
      }
      m_db = 0;
      throw IException(ie, IException::Programmer, mess, _FILEINFO_);
    }
  
    // Cleanup is automatic - see DbDisconnect above.
    m_db = 0;
    return (nrows);
  }
  
  
  /** 
   * @brief Connects to the database.
   *
   * Connects to the database using the parameters of the Strategy object
   * definition.
   *
   * @param globals   List of global keywords to use in argument substitutions
   * @return Database The Database object of the connection.
   */  
  Database *DatabaseReaderStrategy::connect(const ResourceList &globals) const {
    // Access the database
    PvlFlatMap keys( getDefinitionMap() );

    QString dbFile = translateKeywordArgs("DbFile", globals);
    if ( !dbFile.isEmpty() ) {
      DbProfile profile;
      profile.add("Name", "SQLite_Profile");
      profile.add("Dbname", dbFile);
      profile.add("Type", "SQLite");
      if ( isDebug() ) {
        cout << "Database::connecting...\n";
      }
      return (new Database(profile, Database::Connect));
    }
    else {
      QString dbconfig = translateKeywordArgs("DbConfig", globals);
      if ( !dbconfig.isEmpty() ) {
        Database::addAccessConfig(dbconfig);
      }
    
      QString dbprofile = translateKeywordArgs("DbProfile", globals);
      DbProfile profile = Database::getProfile(dbprofile);
      if ( isDebug() ) {
        cout << "Database::connecting...\n";
      }
      return (new Database(profile,Database::Connect));
    }
  }
  

  /** 
   * Configures the query replacing keyword arguments and special character
   * placeholders.
   *
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return QString The query parameter with replaced arguments and
   *                 placeholders.
   */  
  QString DatabaseReaderStrategy::configureQuery(const ResourceList &globals) const {
    // Get the query and perform argument replacement
    QString query = translateKeywordArgs("Query", globals, "NULL");
  
    //  Now scan for replacement of quotes (anything else?)
    query = scanAndReplace(query, "&quot;", "\"");
    query = scanAndReplace(query, "&apos;", "\'");
    return (query);
  }
  

  /**
   * Creates the new resources from the database query. New resources will
   * contain the query if specified in the Strategy object definition.
   * Resources are named using the row base name and the row number by default,
   * or alternatly as specified by the identity parameter.
   *
   * @param resources ResourceList to append new resources to.
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int      The number of resources created.
   */  
  int DatabaseReaderStrategy::executeQuery(ResourceList &resources, 
                                           const ResourceList &globals) const { 
    // Make assets out of them
    PvlFlatMap keys( getDefinitionMap() );
  
    QString query = configureQuery(globals);
    if ( isDebug()  ) {  
      cout << "Running Query = " << query << "\n"; 
    }
    SqlQuery finder(*m_db);
    finder.setThrowOnFailure();
    finder.exec(query.toStdString());
    if ( isDebug()  ) {  
      cout << "Query done...converting...\n"; 
    }
  
  
    QString rowBase(keys.get("RowBaseName","Row"));
    QString queryStore = keys.get("QueryStore","");
  
    // For every row result from the query, create a Resource containing a
    // flat keyword interface.
    int row = 0;
    for (row = 0 ;  finder.next() ; row++ ) {
      QString rowId = rowBase + QString::number(row);
      SqlRecord record = finder.getRecord();
      SharedResource newsrc(importQuery(rowId, &record, globals));
  
      // Determine identity
      QString identity = translateKeywordArgs("Identity", getGlobals(newsrc, globals)); 
      if ( identity.isEmpty() ) { 
        identity = rowId; 
      }
      newsrc->setName(identity);
      if ( isDebug() ) { 
        cout << "  Db::Resource::" << rowId << "::Identity = " << identity << "\n";
      }
  
      // Propagated keys and store Query if requested
      BOOST_FOREACH ( SharedResource defaults, globals) {
        propagateKeys(defaults, newsrc);
      }
      if ( !queryStore.isEmpty() ) {
        newsrc->add(queryStore, scanAndReplace(query, "\"", "&quot;"));
      }
  
      resources.append(newsrc);
      // if ( isDebug() ) cout << "Processed " << row << "\r" << flush;
    }
    return (row);
  }
  

  /** 
   * Creates a new Resource from an SqlRecord.
   *
   * @param rowId  QString of the resource identity.
   * @param record SqlRecord used to populate the resource.
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return SharedResource built from the record.
   *
   * @throw IException::User "Geometry conversion failed horribly for Resource."
   */
  SharedResource DatabaseReaderStrategy::importQuery( const QString &rowId, 
                                                const SqlRecord *record,
                                                const ResourceList &globals) const {
  
    SharedResource rowrec(new Resource(rowId));
    for (int column = 0 ; column < record->size() ; column++) {
      rowrec->add(record->getFieldName(column), QString(record->getValue(column))); 
    }
  
  
    //  Check for Geometry.  May want to remove it after parsing/conversion.  
    //  These text geometries tend to be huge and consume lots of memory.
    try {
      importGeometry(rowrec, globals);
    }
    catch (IException &ie) {
      std::string mess = "Geometry conversion failed horribly for Resource [" + 
                     rowId + "].";
      throw IException(ie, IException::User, mess, _FILEINFO_);
    }

    return (rowrec);
  }
  
}  //namespace Isis
