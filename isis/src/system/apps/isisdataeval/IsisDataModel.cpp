/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>
#include <tuple>
#include <algorithm>
#include <iomanip>

#include <Qt>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QCryptographicHash>

#include "FileName.h"
#include "IException.h"
#include "IsisDataModel.h"
#include "IString.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"

namespace Isis {
  namespace Data {


    //***********************************************************************************
    // DBTimeCoverage Implementations
    //***********************************************************************************
    DBTimeCoverage::DBTimeCoverage() : m_starttime( ), m_stoptime( ),
                                       m_isvalid ( false ) { }
    DBTimeCoverage::DBTimeCoverage( const iTime &start_t, const iTime &stop_t ) :
                                    m_starttime( start_t ), m_stoptime( stop_t ),
                                    m_isvalid ( true ) { }

    DBTimeCoverage::DBTimeCoverage( const QString &start_s, const QString &stop_s ) :
                                    m_starttime( ), m_stoptime( ),
                                    m_isvalid ( false ) {

      QString start_t = ( !start_s.isEmpty() ) ? start_s : stop_s;
      QString stop_t  = ( !stop_s.isEmpty() )  ? stop_s : start_t;

      if ( !start_t.isEmpty() && !stop_t.isEmpty() ) {
        setTime( iTime( start_t) , iTime( stop_t ) );
      }
    }

    DBTimeCoverage::DBTimeCoverage( const iTime &start_t ) :
                                    m_starttime( start_t ), m_stoptime( start_t ),
                                    m_isvalid ( true ) { }
    DBTimeCoverage::DBTimeCoverage( const QString &start_s ) :
                                    m_starttime(  ), m_stoptime(  ),
                                    m_isvalid ( false ) {
      if ( !start_s.isEmpty() ) {
        iTime time_s( start_s );
        setTime( time_s, time_s );
      }
    }

      /** Check if a time is in the coverage timestamp */
    bool DBTimeCoverage::contains( const DBTimeCoverage &t_time,
                                   const bool partial ) const {
      if ( t_time.isvalid() ) {
        if ( this->isvalid() ) {
          if ( t_time.starttime().Et() <= this->stoptime().Et() ) {
            if ( t_time.stoptime().Et() >= this->starttime().Et() ) {
              if ( partial ) {
                return ( true );
              }
              else {
                if ( t_time.starttime().Et()   >= this->starttime().Et() ) {
                  if ( t_time.stoptime().Et() <= this->stoptime().Et() ) {
                    return ( true );
                  }
                }
              }
            }
          }
        }
      }

      return ( false );
    }

    isisdata_json DBTimeCoverage::to_json( const DBTimeCoverage &dbtimecoverage ) {
      isisdata_json js_dbtcov;

      if ( dbtimecoverage.isvalid() ) {
        js_dbtcov["starttime"]   = dbtimecoverage.starttime().UTC().toStdString();
        js_dbtcov["stoptime"]    = dbtimecoverage.stoptime().UTC().toStdString();
        js_dbtcov["starttimeet"] = dbtimecoverage.starttime().Et();
        js_dbtcov["stoptimeet"]  = dbtimecoverage.stoptime().Et();
      }
      else {
        isisdata_json is_null;
        js_dbtcov["starttime"]   = is_null;
        js_dbtcov["stoptime"]    = is_null;
        js_dbtcov["starttimeet"] = is_null;
        js_dbtcov["stoptimeet"]  = is_null;
      }

      return ( js_dbtcov );
    }


    //***********************************************************************************
    // DBFileStatus Implementations
    //***********************************************************************************
    DBFileStatus::DBFileStatus() : m_file() { }
    DBFileStatus::DBFileStatus( const DBFileStatus::DBFileData &dbfile ) : m_file ( dbfile ) { }
    DBFileStatus::DBFileStatus( const FileName &dbfile, const QFileInfo &dbfileinfo ) :
                                m_file(dbfile.original(), dbfile , dbfileinfo, "DBFileStatus" ) { }

    DBFileStatus::DBFileStatus( const FileName &dbfile, const bool versionIt ) :
                                m_file( dbfile.original(), dbfile, QFileInfo( dbfile.expanded() ), "DBFileStatus") {
      if ( versionIt ) versionize();
    }

    DBFileStatus::DBFileStatus( const QString &dbfile, const bool versionIt ) : m_file() {
      FileName v_file( dbfile );
      m_file = DBFileData( dbfile, v_file, QFileInfo( v_file.expanded() ), "DBFileStatus");
      if ( versionIt ) versionize();
    }

    bool DBFileStatus::versionize( ) {

      if ( !file().isVersioned() ) return ( false );

      // Attempt versioning
      bool isversioned( false) ;
      try {
        m_file.m_key = file().highestVersion();
        m_file.m_data.setFile( file().expanded() );
        isversioned = true;
      }
      catch (...) {
        // NOOP - return expanded version
        isversioned = false;

      }

      return ( isversioned );
    }

    DBFileStatus versionize( const FileName &dbname ) {
      const bool versionIt = true;
      return ( DBFileStatus( dbname, versionIt ) );
    }

    /**
     * @brief Translates a File keyword using ISIS rules returning status
     *
     * @param key
     * @param prefdata
     * @param doVersioning
     * @return DBFileStatus
     */
    DBFileStatus DBFileStatus::translate( const PvlKeyword &key,
                                          const PvlGroup &prefdata,
                                          const bool doVersioning ) {
      QString fname, dbfile;
      bool isGoodToGo( doVersioning );
      try {
        if ( key.size() == 2 ) {
          dbfile = fname = "$" + QString::fromStdString(key[0]) + "/" + QString::fromStdString(key[1]);  // In case the translation fails...
          dbfile = QString::fromStdString(prefdata[key[0]][0]) + "/" + QString::fromStdString(key[1]);
        }
        else if ( key.size() == 1 ) {
          dbfile = fname = QString::fromStdString(key[0]);
        }
        else {
          // Ill-formed string, prepare a return string
          QString bad;
          for ( int v = 0 ; v < key.size() ; v++) {
            bad += "[" + QString::fromStdString(key[v]) + "]";
          }

          // If its an empty PvlKeyword...
          fname  = bad;
          dbfile = bad;
          isGoodToGo = false;
        }
      } catch ( ... ) {
        // This can only occur because the missing data lookup failed.
        // The $ above indicates its a failed mission translation in Preferences
        isGoodToGo = false;
      }

      DBFileStatus f_status( dbfile, isGoodToGo );
      f_status.setName( fname );

      return ( f_status );
    }

    isisdata_json DBFileStatus::to_json( const DBFileStatus &dbfilestatus ) {

      isisdata_json js_dbfile;
      isisdata_json is_null;


      if ( !dbfilestatus.file().toString().isEmpty() ) {
        js_dbfile["filespec"] = dbfilestatus.name().toStdString();
        js_dbfile["filepath"] = dbfilestatus.expanded();
        js_dbfile["exists"]   = json_bool( dbfilestatus.exists() ).toStdString();
      }
      else {
        js_dbfile["filespec"] = is_null;
        js_dbfile["filepath"] = is_null;
        js_dbfile["exists"]   = is_null;
      }

      if ( dbfilestatus.exists() ) {
        // js_dbfile["file"]     = dbfilestatus.absolute().toStdString();
        js_dbfile["file"]     = dbfilestatus.info().absoluteFilePath().toStdString();
        js_dbfile["symlink"]  = json_bool( dbfilestatus.isSymbolicLink() ).toStdString();
        js_dbfile["target"]   = dbfilestatus.target().toStdString();

        // This will ensure both are valid times
        DBTimeCoverage  created_t( DBFileStatus::toUTC( dbfilestatus.created() ) );
        DBTimeCoverage modified_t( DBFileStatus::toUTC( dbfilestatus.modified() ) );

        if ( created_t.isvalid() ) {
          js_dbfile["created"]    = created_t.starttime().UTC().toStdString();
          js_dbfile["createdet"]  = created_t.starttime().Et();
        }
        else {
          js_dbfile["created"]    = is_null;
          js_dbfile["createdet"]  = is_null;
        }

        if ( modified_t.isvalid() ) {
          js_dbfile["modified"]   = modified_t.starttime().UTC().toStdString();
          js_dbfile["modifiedet"] = modified_t.starttime().Et();
        }
        else {
          js_dbfile["modified"]   = is_null;
          js_dbfile["modifiedet"] = is_null;
        }

        js_dbfile["size"]       = dbfilestatus.info().size();
      }
      else {
        isisdata_json is_null;
        js_dbfile["file"]       = is_null;
        js_dbfile["symlink"]    = is_null;
        js_dbfile["target"]     = is_null;
        js_dbfile["created"]    = is_null;
        js_dbfile["createdet"]  = is_null;
        js_dbfile["modified"]   = is_null;
        js_dbfile["modifiedet"] = is_null;
        js_dbfile["size"]       = is_null;
      }

      return ( js_dbfile );
    }

    QByteArray DBFileStatus::hash( const QCryptographicHash::Algorithm hashAlgorithm ) const {
      if ( !exists() ) {
        return ( QByteArray() );
      }

      // File exists, lets open it and compute the hash
      QFile v_file( expanded() );
      if ( !v_file.open( QIODevice::ReadOnly ) ) {
        std::string mess = "Could not open file " + expanded() +  " to compute hash";
        throw IException( IException::User, mess, _FILEINFO_ );
      }

      // Compute the hash!
      QCryptographicHash q_hash( hashAlgorithm );
      if ( !q_hash.addData( &v_file ) ) {
        std::string mess = "Could not compute hash for  " + expanded();
        throw IException( IException::User, mess, _FILEINFO_ );
      }

      // NOTE: The QCryptograhicHash API is different in 6.x!
      return ( q_hash.result() );
    }

    DBFileStatus::DBFileHash DBFileStatus::compute_hash( const QString &dbfile,
                                                         const QCryptographicHash::Algorithm hashAlgorithm ) {

      DBFileStatus v_file( dbfile );
      return ( DBFileHash( v_file.expanded(), v_file, v_file.hash( hashAlgorithm ), "FileHash") );
    }


    //***********************************************************************************
    // DBMatch Implementations
    //***********************************************************************************
    DBMatch::DBMatch() : m_group( ), m_keyword( ), m_match ( ), m_id() { }
    DBMatch::DBMatch( const QString &groupname,
                      const QString &keyword,
                      const QString &match) :
                      m_group( groupname ),
                      m_keyword( keyword ),
                      m_match( match ),
                      m_id( db_null() ) {
      // Make the unique id
       m_id = makeMatchid();
    }

    /** Make a (unique) identifier */
    QString DBMatch::makeMatchid() const {

      // Construct only if valid
      if ( isvalid() ) {
        return ( QString( group()+"/"+keyword()+"/"+value() ).toLower() );
      }

      // If invalid, return null
      return ( db_null() );
    }

    DBMatch DBMatch::fromMatchid( const QString &mid ) {
      QStringList m_fields = mid.split( "/" );   // , Qt::SkipEmptyParts );
      if ( m_fields.size() != 3 ) return ( DBMatch() );
      return ( DBMatch( m_fields[0], m_fields[1], m_fields[2] ) );
    }

    isisdata_json DBMatch::to_json( const DBMatch &dbmatch ) {
      isisdata_json js_dbmatch;

      if ( dbmatch.isvalid() ) {
        js_dbmatch["group"]   = dbmatch.group().toStdString();
        js_dbmatch["keyword"] = dbmatch.keyword().toStdString();
        js_dbmatch["value"]   = dbmatch.value().toStdString();
        js_dbmatch["matchid"]   = dbmatch.matchid().toStdString();
      }
      else {
        isisdata_json is_null;
        js_dbmatch["group"]   = is_null;
        js_dbmatch["keyword"] = is_null;
        js_dbmatch["value"]   = is_null;
        js_dbmatch["matchid"]   = is_null;
      }
      return ( js_dbmatch );
    }


    //***********************************************************************************
    // DBSelection Implementations
    //***********************************************************************************
    DBSelection::DBSelection() : m_source(), m_kernelset (), m_coverage(),
                                 m_matches(), m_type( db_null() ) { }
    DBSelection::DBSelection( const DBFileStatus &dbsource ) :
                              m_source( dbsource ),
                              m_kernelset( ),
                              m_coverage( ),
                              m_matches( ),
                              m_type( db_null() )  { }
    DBSelection::DBSelection(const DBFileStatus &dbsource,
                             const DBFileStatusList &dbfilestatus,
                             const DBTimeCoverage &dbcoverage,
                             const DBMatchList &dbmatches,
                             const QString &ktype) :
                             m_source( dbsource ),
                             m_kernelset( dbfilestatus ),
                             m_coverage( dbcoverage ),
                             m_matches( dbmatches ),
                             m_type( ktype )  { }

      /** Retrive contents of a section */
    DBSelection DBSelection::read( const PvlGroup &selection,
                                   const DBFileStatus &dbsource,
                                   const PvlGroup &prefdata ) {


      DBSelection dbselection( dbsource );

      // Consolidate all coverage times to a span that my have gaps
      iTime s_starttime;
      iTime s_stoptime;
      const bool doVersioning = true;  // get highest versions of kernel names

      for ( int kndx = 0 ; kndx < selection.keywords() ; kndx++ ) {
        const PvlKeyword &key = selection[kndx];

        if ( key.isNamed("File") ) {
          dbselection.addFile( DBFileStatus::translate( key, prefdata, doVersioning ) );
        }
        else if ( key.isNamed("Time") ) {
          if ( dbselection.hasTime() == false ) {
            // Set first time coverage
            s_starttime = iTime(QString::fromStdString(key[0]));
            s_stoptime   = iTime(QString::fromStdString(key[1]));
          }
          else {
            // Test limits of current span
            iTime start_time_t(QString::fromStdString(key[0]));
            iTime  stop_time_t(QString::fromStdString(key[1]));

            if ( start_time_t < s_starttime ) s_starttime = start_time_t;
            if (  stop_time_t > s_stoptime  ) s_stoptime  = stop_time_t;
          }

          // Update the span
          dbselection.setTime( s_starttime, s_stoptime );
        }
        else if ( key.isNamed( "Match") ) {
          // Add a match keyword
          dbselection.addMatch( DBMatch(QString::fromStdString(key[0]),QString::fromStdString(key[1]),QString::fromStdString(key[2])));
        }
        else if ( key.isNamed( "Type" ) ) {
          // Update the type which will be "Reconstructed", "Smithed", etc...
          dbselection.setType(QString::fromStdString(key[0]));
        }
      }

      return ( dbselection );
    }

    DBSelection DBSelection::read( const PvlGroup &selection,
                                   const DBFileStatus &dbsource ) {
        const PvlGroup &preferences = Preference::Preferences().findGroup("DataDirectory");
        return  ( read( selection, dbsource,  preferences ) );
    }

     int DBSelection::validate( DBFileDispositionList &dbstatus,
                               const DBFileStatusSet &inventory ) const {
      int nbad = 0;

      for ( auto const &dbfile : files()  ) {

        if ( !dbfile.exists() ) {
          dbstatus.push_back( DBFileDisposition( "Missing", dbfile.name(), source(), type() ) );
          nbad++;
        }
        else {
          if ( dbfile.isSymbolicLink() ) {
            dbstatus.push_back( DBFileDisposition( "Symlink", dbfile.name(), source(), type() ) );
            nbad++;
          }

          // Check if file is in inventory
          if ( !inventory.contains( dbfile.expanded() ) ) {
            dbstatus.push_back( DBFileDisposition( "External", dbfile.expanded(), source(), type() ) );
            nbad++;
          }
        }
      }
      return ( nbad );
    }

    isisdata_json DBSelection::to_json( const DBSelection &dbselection,
                                       const bool addSource ) {

      isisdata_json js_dbmatches = isisdata_json::array();
      for ( auto &dbmatch : dbselection.matches() ) {
        js_dbmatches.push_back( dbmatch.to_json() );
      }

      isisdata_json js_dbkernels = isisdata_json::array();
      for ( auto &dbkernel : dbselection.files() ) {
        js_dbkernels.push_back( dbkernel.to_json() );
      }

      isisdata_json js_dbselection;
      if ( addSource ) {
        js_dbselection["source"] = dbselection.source().expanded();
      }

      js_dbselection["time"]   = dbselection.time().to_json();
      js_dbselection["match"]  = js_dbmatches;
      js_dbselection["files"]  = js_dbkernels;
      js_dbselection["type"]   = dbselection.type().toStdString();

      return ( js_dbselection );
    }


    //***********************************************************************************
    // DBKernelDb Implementations
    //************]***********************************************************************
    DBKernelDb::DBKernelDb() :  m_kerneldb(), m_category( db_null() ),
                                m_runtime(), m_selections()  { }
    DBKernelDb::DBKernelDb( const QString &category ) :
                            m_kerneldb(), m_category( category ),
                            m_runtime(), m_selections() { }
    DBKernelDb::DBKernelDb( const DBFileStatus &dbkerneldbfile,
                            const QString &category ) :
                            m_kerneldb( dbkerneldbfile ),
                            m_category( category ),
                            m_runtime(),
                            m_selections() { }


    DBFileStatus DBKernelDb::dbFileStatus( const DBFileStatus &dbdir )  {
      QString kerneldb = dbdir.expanded();
      if ( dbdir.isDirectory() ) {
        kerneldb += "/kernels.????.db";
      }

      const bool doVersioning = true;
      return ( DBFileStatus( kerneldb, doVersioning ) );
    }

    DBKernelDb DBKernelDb::read( const DBFileStatus &dbfile,
                                 const PvlGroup &prefdata,
                                 const QString &source ) {

      if ( !dbfile.exists() ) {
        return ( DBKernelDb ( dbfile, source) );
      }

      // Got a kerneldb file
      Pvl db( dbfile.expanded() );

      // Check if there are any specs in the file
      if ( db.objects() < 1 ) {
        return ( DBKernelDb ( dbfile, source ) );
      }

      PvlObject &inst = db.object(0);  // Get first object of .db or .conf
      DBKernelDb dbkernel( dbfile, QString::fromStdString(inst.name()) );

      // Check for a Runtime keyword
      if ( inst.hasKeyword("Runtime") ) {
        dbkernel.setRuntime(iTime(QString::fromStdString(inst["Runtime"][0])));
      }

      // Set up the data dir translation for the files
      for ( int gndx = 0 ; gndx < inst.groups() ; gndx++ ) {
        PvlGroup &grp = inst.group(gndx);

        if ( grp.isNamed("Selection") ) {
          dbkernel.addSelection( DBSelection::read( grp, dbkernel.kerneldb(), prefdata ) );
        }
      }

      return ( dbkernel );
    }

    int DBKernelDb::validate(  const DBSelection &dbselection,
                               DBFileDispositionList &dbstatus,
                               const DBFileStatusSet &inventory ) const {
      return ( dbselection.validate( dbstatus, inventory ) );
    }

    int DBKernelDb::validate( DBFileDispositionList &dbstatus,
                               const DBFileStatusSet &inventory ) const {
      int nbad = 0;

      // Check if the kerneldbd actually exists
      if ( !isvalid() ) {
        dbstatus.push_back( DBFileDisposition( "Missing", kerneldb().name(), kerneldb(), category() ) );
        nbad++;
      }
      else {
        if ( 0 >= size() ) {
          dbstatus.push_back( DBFileDisposition( "Empty", kerneldb().name(), kerneldb().original(), category() ) );
          nbad++;
        }
        else {
          // Vaidate Selection kernels
          for ( auto const &dbselection : selections() ) {
            nbad += validate( dbselection, dbstatus, inventory );
          }
        }

        // Check the location of the DB file (it does exist at this point)
        if ( !inventory.contains( kerneldb().expanded() ) ) {
          // std::cout << "External_S, " << kerneldb().name() << ", " << source().name() << std::endl;
          dbstatus.push_back( DBFileDisposition( "External", kerneldb().expanded(), kerneldb(), category() ) );
          nbad++;
        }
      }

      return ( nbad );
    }


    isisdata_json DBKernelDb::to_json( const DBKernelDb &dbkerneldb ) {

      isisdata_json js_dbkerneldb;
      js_dbkerneldb             += dbkerneldb.kerneldb().to_json();
      js_dbkerneldb["category"] = dbkerneldb.category().toStdString();

      auto js_dbruntime = dbkerneldb.coverage().to_json();
      js_dbkerneldb["runtime"]    = js_dbruntime["starttime"];
      js_dbkerneldb["runtimeet"]  = js_dbruntime["starttimeet"];

      isisdata_json js_selections = isisdata_json::array();
      for ( auto &dbselection : dbkerneldb.m_selections ) {
        js_selections.push_back( dbselection.to_json() );
      }

      js_dbkerneldb["selections"] = js_selections;

      return ( js_dbkerneldb );
    }


    //***********************************************************************************
    // DBKernelConf Implementations
    //***********************************************************************************
    DBKernelConf::DBKernelConf() : m_kerneldb( "none" ), m_selectionsets() { }

    DBKernelConf::DBKernelConf( const DBFileStatus &dbconfig ) :
                                m_kerneldb( dbconfig ),
                                m_selectionsets() { }

    DBKernelConf::DBKernelConf( const DBKernelDb &dbkernel,
                                const PvlGroup &preferences ) :
                                m_kerneldb( dbkernel ),
                                m_selectionsets() {
       addKernelDb( m_kerneldb, preferences );
    }

    DBFileStatus DBKernelConf::dbFileStatus( const DBFileStatus &dbdir )  {
      QString kerneldb = dbdir.expanded();
      if ( dbdir.isDirectory() ) {
        kerneldb += "/kernels.????.conf";
      }

      const bool doVersioning = true;
      return ( DBFileStatus( kerneldb, doVersioning ) );
    }

    DBKernelConf DBKernelConf::read( const DBFileStatus &dbconfig,
                                     const PvlGroup &preferences ) {
     return ( DBKernelConf( DBKernelDb::read( dbconfig, preferences), preferences ) );
    }

    void DBKernelConf::addConfigSelection( const DBSelection &selection,
                                           const PvlGroup &preferences ) {

      // Add the contents of the referred database here
      QString source = selection.source().expanded();

      DBKernelDbList kerneldbs;
      for ( auto const &file : selection.files() ) {
        kerneldbs.push_back( DBKernelDb::read( file, preferences ) );
      }

      // Add to the selection sets
      m_selectionsets.push_back( DBSelectionSet( selection, kerneldbs ) );

      return;
    }

    isisdata_json DBKernelConf::to_json( const DBKernelConf &dbconfig ) {

      isisdata_json js_dbconfig;
      js_dbconfig            += dbconfig.config().to_json();

      isisdata_json js_selections = isisdata_json::array();
      for ( auto &dbselection : dbconfig.kernelsets() ) {
        isisdata_json js_confselect;

        js_confselect["config"] = dbselection.m_selection.to_json();

        isisdata_json js_confkernels = isisdata_json::array();
        for ( auto &db : dbselection.m_kerneldbs ) {
          js_confkernels += db.to_json();
        }

        js_confselect["kernels"] = js_confkernels;
        js_selections += js_confselect;;
      }

      js_dbconfig["selectionsets"] = js_selections;

      return ( js_dbconfig );
    }

    void DBKernelConf::addKernelDb( const DBKernelDb &dbconfig,
                                    const PvlGroup &preferences ) {

      if ( dbconfig.size() > 0 ) {
        for ( auto const &dbselection : dbconfig.selections() ) {
          addConfigSelection( dbselection, preferences );
        }
      }

      return;
    }

    int DBKernelConf::validate(const DBSelectionSet &dbset,
                               DBFileDispositionList &dbstatus,
                               const DBFileStatusSet &inventory ) const {
      int nbad = 0;
      for ( auto const &dbkernel : dbset.m_kerneldbs ) {
        nbad += dbkernel.validate( dbstatus, inventory );
      }
      return ( nbad );
    }


    int DBKernelConf::validate( DBFileDispositionList &dbstatus,
                               const DBFileStatusSet &inventory )const {
      int nbad = config().validate( dbstatus, inventory );

      for ( auto const &dbset : kernelsets() ) {
        nbad += this->validate( dbset, dbstatus, inventory );
      }
      return ( nbad );
    }

    //***********************************************************************************
    // IsisDataModel Implementations
    //***********************************************************************************
    IsisDataModel::IsisDataModel() : m_isisdata(QString("$ISISDATA")),
                                     m_dataroot(QString("$ISISDATA")),
                                     m_allfiles(),
                                     m_kerneldbs(),
                                     m_configs() { }
    IsisDataModel::IsisDataModel(const QString &dataroot, const QString &isisdata ) :
                                 m_isisdata( ), m_dataroot(),
                                 m_allfiles(), m_kerneldbs(),
                                 m_configs() {
      setDataRoot( dataroot );
      setIsisData( isisdata );
    }

    size_t IsisDataModel::evaluate( ) {

      // Check for validity of datadir and isisdata
      if ( !hasIsisData() ) {
        std::string mess = "ISISDATA (" + isisdata().original() + ") does not not exist/invalid!";
        throw IException( IException::User, mess, _FILEINFO_ );
      }

      // Check for validity of datadir and isisdata
      if ( !hasDataRoot() ) {
        std::string mess = "DATAROOT (" + dataroot().original() + ") does not not exist/invalid!";
        throw IException( IException::User, mess, _FILEINFO_ );
      }

      // Check for validity of datadir and isisdata
      if ( !dataroot().isDirectory() ) {
        std::string mess = "DATAROOT (" + dataroot().original() + ") is not a directory!";
        throw IException( IException::User, mess, _FILEINFO_ );
      }

      // Set up the directory traversal
      QDirIterator ddir( dataroot().expanded(),
                          QDir::NoDotAndDotDot | QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::System,
                          QDirIterator::Subdirectories | QDirIterator::FollowSymlinks );

      // Get the DataDirectory from the Preferences file
      PvlGroup &prefdata = Preference::Preferences().findGroup("DataDirectory");

      size_t t_size(0);
      while ( ddir.hasNext() ) {
        QString ddfile = ddir.next();
        QFileInfo f_ddfile = ddir.fileInfo();
        DBFileStatus d_file( DBFileStatus::DBFileData( givenPath( ddfile ), FileName( ddfile ), f_ddfile, "inventory" ) );

        m_allfiles.insert( d_file.name(), d_file );

        // Determine any kernels that are referenced in a directory
        if ( d_file.isDirectory() ) {

          // Check for config first
          DBFileStatus configdb = DBKernelConf::dbFileStatus( d_file );
          if ( configdb.exists() ) {
            m_configs.push_back( DBKernelConf::read( configdb, prefdata ) );
          }

          // Now check for kernel DB files
          DBFileStatus kerneldb = DBKernelDb::dbFileStatus( d_file );
          if ( kerneldb.exists() ) {
            m_kerneldbs.push_back( DBKernelDb::read( kerneldb, prefdata ) );
          }
        }
        else {
          // Update total file size
          t_size += d_file.size();
        }
      }
      // QDirIterator traverses directories in random order so sort here
      // std::sort( m_allfiles.begin(), m_allfiles.end() );
      std::sort( m_kerneldbs.begin(), m_kerneldbs.end(), DBKernelDb::compare );
      std::sort( m_configs.begin(), m_configs.end(), DBKernelConf::compare );

      return ( t_size);
    }

    int IsisDataModel::validate( DBFileDispositionList &dbstatus ) const {
      return ( this->validate( dbstatus, allfiles() ) );
    }

    int IsisDataModel::validate( DBFileDispositionList &dbstatus,
                                 const DBFileStatusSet &inventory ) const {
      int n_bad = 0;

      // Do all the DB files
     for ( auto const &db : dbs() ) {
        n_bad += db.validate( dbstatus, inventory );
      }

      // Now do kernel conf dbs
      for ( auto const &dbconf : configs() ) {
        n_bad += dbconf.validate( dbstatus, inventory );
      }

      return ( n_bad );
    }

  } // namespace Data
} // namespace Isis
