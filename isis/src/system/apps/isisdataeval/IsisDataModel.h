/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#ifndef IsisDataModel_h
#define IsisDataModel_h

#include <map>
#include <set>
#include <vector>
#include <tuple>
#include <algorithm>
#include <iomanip>
#include <filesystem>

#include <nlohmann/json.hpp>
using isisdata_json = nlohmann::ordered_json;

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>
#include <QFileInfo>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "iTime.h"
#include "Preference.h"
#include "Pvl.h"
#include "PvlKeyword.h"
#include "PvlGroup.h"

namespace Isis {
  namespace Data {


    /** Generic key (header) retrival from a JSON object */
    inline QStringList header_from_json( const isisdata_json &dbjson )  {

      QStringList header;
      for ( auto &jkey : dbjson.items() ) {
        header.append( QString::fromStdString( jkey.key() ) );
      }

      return ( header );
    }

    /** Generic value retrival from a JSON object */
     inline QStringList values_from_json( const isisdata_json &dbjson )  {

        QStringList values;
        for ( auto &jdata : dbjson.items() ) {
          values.append( QString::fromStdString( to_string( jdata.value() ) ).remove('\"') );
        }

        return ( values );
      }

    /** Declare a single constant for database  */
    inline QString db_null( ) {
      return ( QString("null") );
    }

    /** Declare a single constant for database  */
    inline isisdata_json json_null( ) {
      return ( isisdata_json() );
    }

    /** Declare a single constant for database  */
    inline QString json_bool( const bool &t_or_f ) {
      if ( true == t_or_f ) return ( QString("true") );
      return ( QString("false") );
    }


    /**
     * @brief Generic DB container/column algorithm usage
     *
     * This is designed to be used as part of the IsisData model container
     * system. It may be sufficient to use as generic column container.
     *
     * @tparam Key    Key type that is designed to be sortable on operator<()
     * @tparam Datum  Stored data
     */
    template<typename Key, typename Datum>
      class DBContainer {
        public:
          QString m_name;
          QString m_header;

          Key     m_key;
          Datum   m_data;

          DBContainer() : m_name("column"), m_header(""),
                          m_key(), m_data() { }
          DBContainer(const QString &column,
                      const QString &header = "" ) :
                      m_name(column), m_header(header),
                      m_key(), m_data() { }
          DBContainer(const QString &column,
                      const Key &key, const Datum &datum,
                      const QString &header = "" ) :
                      m_name(column), m_header ( header ),
                      m_key( key ), m_data( datum ) { }

          virtual ~DBContainer() { }

          const QString &name( ) const {
            return ( m_name );
          }

          void setName( const QString &name ) {
            m_name = name;
          }

          const QString &header( ) const {
            if ( m_header.isEmpty() ) return ( name() );
            return ( m_header );
          }

          const QString &status() const {
            return ( header() );
          }

          const Key &key() const {
            return ( m_key );
          }

          const Datum &datum() const {
            return ( m_data );
          }

          Datum &datum() {
            return ( m_data );
          }

          bool operator<( const DBContainer &dbcontainer) const {
            return ( this->compare( this->key(), dbcontainer.key() ) );
          }

          bool operator==( const DBContainer &dbcontainer) const {
            bool isEqual = ( !this->compare( this->key(), dbcontainer.key() ) ) &&
                           ( !this->compare( dbcontainer.key(), this->key() ) );
            return ( isEqual );
          }

          bool compare( const Key &key1, const Key &key2 ) const {
            return ( key1 < key2);
          }
      };


    /**
     * @brief DBTimeCoverage provides that time span of a KernelDB entry
     *
     * This class is a container for the Time keywords that are found in
     * the Selection groups of ISIS kernel config and DB files.
     *
     */
    class DBTimeCoverage {
      public:
        // Datum
        iTime  m_starttime;
        iTime  m_stoptime;
        bool   m_isvalid;

        DBTimeCoverage();
        DBTimeCoverage( const iTime &start_t,   const iTime &stop_t );
        DBTimeCoverage( const QString &start_s, const QString &stop_s );
        DBTimeCoverage( const iTime &start_t );
        DBTimeCoverage( const QString &start_s );
        virtual ~DBTimeCoverage() { }

        /** Validity is tracked and status provided by this method */
        inline bool isvalid() const {
          return ( m_isvalid );
        }

        /** Set the time with iTimes */
        inline void setTime( const iTime &starttime,  const iTime &endtime ) {
          m_starttime = starttime;
          m_stoptime   = endtime;
          m_isvalid = true;
        }

        /** Get the start time as an iTime */
        inline const iTime &starttime() const {
          return ( m_starttime );
        }
        /** Get the stop time as an iTime */
        inline const iTime &stoptime() const {
          return ( m_stoptime );
        }

        /** Check if a time is fully in the coverage timespan */
        bool contains( const DBTimeCoverage &t_time,
                       const bool partial = false ) const;

        /** Check if a time is partiallty in the coverage timespan */
        inline bool intersects( const DBTimeCoverage &t_time ) const {
          const bool includePartial = true;
          return ( contains( t_time, includePartial ) );
        }

        static isisdata_json to_json( const DBTimeCoverage &dbtimecoverage );

        /** Get the JSON structure of this object */
        inline isisdata_json to_json( ) const {
          return ( DBTimeCoverage::to_json ( *this ) );
        }

        /** Get the header for this object */
        inline QStringList header( ) const {
          return ( header_from_json ( this->to_json()) );
        }

        /** Get the values for this object */
        inline QStringList values( ) const {
          return ( values_from_json ( this->to_json() ) );
        }
    };


    /**
     * @brief DBFileStatus contains the state of a file from a storage resource
     *
     * This container provides details regarding any file or directory from a
     * disk resource.
     *
     */
    class DBFileStatus {
      public:
        // Types
        typedef DBContainer<FileName, QFileInfo>       DBFileData;
        typedef DBContainer<DBFileStatus, QByteArray>  DBFileHash;

        // Datum
        DBFileData   m_file;

        DBFileStatus();
        DBFileStatus( const DBFileData &dbfile );
        DBFileStatus( const FileName &dbfile, const QFileInfo &dbfileinfo );
        DBFileStatus( const FileName &dbfile, const bool versionIt = false );
        DBFileStatus( const QString &dbfile,  const bool versionIt = false );
        virtual ~DBFileStatus() { }


        inline const DBFileData &data() const {
          return ( m_file );
        }

        inline const FileName &file() const {
          return ( m_file.key() );
        }

        inline const QFileInfo &info() const {
          return ( m_file.datum() );
        }

        inline const QString &name() const {
          return ( m_file.name() );
        }

        void setName( const QString &name ) {
          m_file.setName( name );
        }

        inline bool exists() const {
          return ( info().exists() );
        }

        inline size_t size() const {
          return ( info().size() );
        }

        inline bool isDirectory() const {
          return ( info().isDir() );
        }

        inline bool isSymbolicLink() const {
          return ( info().isSymLink() );
        }

        inline QString original() const {
          return ( QString::fromStdString(file().original()) );
        }

        inline QString expanded() const {
          return ( QString::fromStdString(file().expanded()) );
        }

        inline QString absolute() const {
          return ( info().absoluteFilePath() );
        }

        inline QString target() const {
          QString symlink = info().symLinkTarget();
          if ( symlink.isEmpty() ) symlink = absolute();
          return ( symlink );
        }

        inline QDateTime created() const {
          return ( info().birthTime() );
        }

        inline QDateTime modified() const {
          return ( info().lastModified() );
        }

        static QString toUTC( const QDateTime ftime ) {
          QString format = "yyyy-MM-ddThh:mm:ss.zzz";
          return ( ftime.toUTC().toString( format ) );
        }

        bool versionize( );

        static DBFileStatus versionize( const FileName &dbname );

        static DBFileStatus translate( const PvlKeyword &key,
                                       const PvlGroup &prefdata,
                                       const bool doVersioning = true );


        static bool compare( const DBFileStatus &db1,
                             const DBFileStatus &db2 ) {
          return ( db1 < db2 );
        }

        /** Provides sorting capabilties */
        inline bool operator<( const DBFileStatus &other ) const {
          return ( this->original() < other.original() );
        }

        bool operator==( const DBFileStatus &dbfile_lhs ) const {
          return (this->original() == dbfile_lhs.original() );
        }

        static isisdata_json to_json( const DBFileStatus &dbfilestatus );

        inline isisdata_json to_json( ) const {
          return ( DBFileStatus::to_json ( *this ) );
        }

        inline QStringList header( ) const {
          return ( header_from_json ( this->to_json() ) );
        }

        inline QStringList values( ) const {
          return ( values_from_json ( this->to_json() ) );
        }

        static DBFileHash compute_hash( const QString &dbfile,
                                        const QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Md5 );
        QByteArray hash( const QCryptographicHash::Algorithm hashAlgorithm = QCryptographicHash::Md5 ) const;
    };

    // Some DBFileStatus definitions
    typedef std::vector<DBFileStatus>                   DBFileStatusList;
    typedef DBContainer<QString, DBFileStatus>          DBFileDisposition;
    typedef std::vector<DBFileDisposition>              DBFileDispositionList;

    typedef std::map<std::string, DBFileStatus>         DBFileStatusSet;
    typedef DBContainer<DBFileStatus, DBFileStatusSet>  DBDirectory;

    typedef DBContainer<QString, DBFileDispositionList> DBConfigStatus;
    typedef DBContainer<QString, DBFileStatusList>      DBFileList;
    typedef DBContainer<DBFileStatus, DBFileStatusList> DBFileSet;

    typedef std::map<QString, DBFileStatusList>         DBFileListMap;
    typedef DBContainer<QString, DBFileListMap>         DBFileStatusMap;
    typedef DBContainer<DBFileStatus, DBFileStatusMap>  DBFileMap;

    /**
     * @brief Class DBMatch maintains Match occurrances in DB Selection groups
     *
     * This class is a container for the Match keywords found in all Selection
     * groups contained in a kernel DB. This functions on both the DB and config
     * specifications.
     *
     * Note it could also support any other scalar valued ISIS keyword in a pinch.
     */
    class DBMatch {
      public:
        // Datum
        QString  m_group;
        QString  m_keyword;
        QString  m_match;
        QString  m_id;

        DBMatch();
        DBMatch( const QString &groupname, const QString &keyword,
                 const QString &match);
        virtual ~DBMatch() { }

        static DBMatch fromMatchid( const QString &mid );

        /** Determines if the Match is valid */
        inline bool isvalid() const {
          // Group could be empty and still valid...theoretically
          if ( m_keyword.isEmpty() ) return (false );
          if ( m_group.isEmpty()   ) return (false );
          if ( m_match.isEmpty()   ) return (false );
          return ( true );
        }

        inline const QString &group() const {
          return ( m_group );
        }

        inline const QString &keyword() const {
          return ( m_keyword );
        }

        inline const QString &value() const {
          return ( m_match );
        }

        QString makeMatchid() const;

        inline const QString &matchid() const {
          return ( m_id );
        }

        bool operator==( const DBMatch &other ) const {
          return ( other.matchid() == this->matchid() );
        }

        static isisdata_json to_json( const DBMatch &dbmatch );

        inline isisdata_json to_json( ) const {
          return ( DBMatch::to_json ( *this ) );
        }

        inline QStringList header( ) const {
          return ( header_from_json ( this->to_json() ) );
        }

        inline QStringList values( ) const {
          return ( values_from_json ( this->to_json() ) );
        }
    };

    /**
     * @brief DBSelection contains the contents of a kernel DB Selection group
     *
     * This class is a container for the contents of a Selection group found in
     * both ISISDATA DB and config files.
     *
     */
    class DBSelection {
      public:
        // Datum
        typedef std::vector<DBMatch>      DBMatchList;

        DBFileStatus      m_source;
        DBFileStatusList  m_kernelset;
        DBTimeCoverage    m_coverage;
        DBMatchList       m_matches;
        QString           m_type;

        DBSelection();
        DBSelection( const DBFileStatus &dbsource );
        DBSelection( const DBFileStatus &dbsource,
                     const DBFileStatusList &dbfilestatus,
                     const DBTimeCoverage &dbcoverage,
                     const DBMatchList &dbmatches,
                     const QString &ktype = db_null() );
        virtual ~DBSelection() { }

        static DBSelection read( const PvlGroup &selection,
                                 const DBFileStatus &dbsource,
                                 const PvlGroup &prefdata );

        static DBSelection read( const PvlGroup &selection,
                                 const DBFileStatus &dbsource );

        inline size_t size() const {
          return ( m_kernelset.size() );
        }

        inline void setSource( const DBFileStatus &dbsource ) {
          m_source = dbsource;
        }

        inline const DBFileStatus &source( ) const {
          return ( m_source );
        }

        inline void addFile( const DBFileStatus &dbfilestatus ) {
          m_kernelset.push_back( dbfilestatus );
        }

        inline const DBFileStatusList &files() const {
          return ( m_kernelset );
        }

        inline size_t sizeMatches() const {
          return ( matches().size() );
        }

        inline bool hasMatches() const {
          return ( sizeMatches() != 0);
        }

        inline void addMatch( const DBMatch &dbmatch ) {
          m_matches.push_back( dbmatch);
        }

        inline const DBMatchList &matches() const {
          return ( m_matches );
        }

        inline bool hasTime( ) const {
          return ( m_coverage.isvalid() );
        }

        inline void setTime( const iTime &starttime,  const iTime &endtime ) {
          m_coverage.setTime( starttime, endtime );
        }

        inline void setTime( const DBTimeCoverage &dbcoverage ) {
          m_coverage = dbcoverage;
        }

        inline const DBTimeCoverage &time( ) const {
          return ( m_coverage );
        }

        inline void setType( const QString &ktype ) {
          m_type = ktype;
        }

        inline const QString &type( ) const {
          return ( m_type );
        }

        int validate( DBFileDispositionList &dbstatus,
                      const DBFileStatusSet &inventory = DBFileStatusSet() ) const;

        static isisdata_json to_json( const DBSelection &dbselection,
                                     const bool addSource = false );

        inline isisdata_json to_json( const bool addSource = false ) const {
          return ( DBSelection::to_json ( *this, addSource ) );
        }

        inline QStringList header( const bool addSource = false ) const {
          return ( header_from_json ( this->to_json( addSource ) ) );
        }

        inline QStringList values( const bool addSource = false ) const {
          return ( values_from_json ( this->to_json( addSource ) ) );
        }


    };

    /**
     * @brief DBKernelDb contains the contents of kernel DB/conf files
     *
     * This class provides an internalization of the contents of a ISISDATA
     * kernel database (kernels.????.db) or configuration (kernels.????.conf)
     * file.
     */
    class DBKernelDb {
      public:
        // Datum
        typedef std::vector<DBSelection> DBSelectionList;

        DBFileStatus    m_kerneldb;
        QString         m_category;
        DBTimeCoverage  m_runtime;
        DBSelectionList m_selections;

        DBKernelDb();
        DBKernelDb( const QString &category );
        DBKernelDb( const DBFileStatus &dbkerneldbfile,
                    const QString &category = "db" );
        virtual ~DBKernelDb() { }

        static DBFileStatus dbFileStatus( const DBFileStatus &dbdir );

        static DBKernelDb read( const DBFileStatus &dbkernel,
                                const PvlGroup &prefdata,
                                const QString &source = "" );

        inline bool isvalid() const {
          return ( m_kerneldb.exists() );
        }

        inline const DBFileStatus &kerneldb() const {
          return ( m_kerneldb );
        }

        inline size_t size() const {
          return ( m_selections.size() );
        }

        inline virtual void addSelection( const DBSelection &selection ) {
          m_selections.push_back ( selection );
        }

        inline const DBSelectionList &selections() const {
          return ( m_selections );
        }

        inline void setCategory( const QString &category ) {
          m_category = category;
        }

        inline const QString &category() const {
          return ( m_category );
        }

        inline void setRuntime( const DBTimeCoverage &runtime ) {
          m_runtime = DBTimeCoverage( runtime );
        }

        inline void setRuntime( const iTime &runtime ) {
          m_runtime = DBTimeCoverage( runtime );
        }

        inline const DBTimeCoverage &coverage() const {
          return ( m_runtime );
        }

        static isisdata_json to_json( const DBKernelDb &dbkerneldb );

        inline isisdata_json to_json( ) const {
          return ( DBKernelDb::to_json( *this ) );
        }

        /** Confirm valid selection*/
        int validate( DBFileDispositionList &dbstatus,
                      const DBFileStatusSet &inventory = DBFileStatusSet() ) const;
        int validate( const DBSelection &dbselection,
                      DBFileDispositionList &dbstatus,
                      const DBFileStatusSet &inventory = DBFileStatusSet() ) const;

        static bool compare( const DBKernelDb &db1,
                             const DBKernelDb &db2 ) {

          return ( DBFileStatus::compare( db1.kerneldb(), db2.kerneldb() ) );
        }
    };

    /**
     * @brief A specialization of an ISISDATA kernel config file
     *
     * This class is designed to manage the more complex ISISDATA kernel
     * config (kernels.????.conf) files.
     *
     */
    class DBKernelConf {
      public:
        // Datum
        typedef std::vector<DBKernelDb>  DBKernelDbList;
        typedef struct dbSelectionSet {
                  dbSelectionSet(const DBSelection &dbselection,
                                 const DBKernelDbList &dbkernellist) :
                                 m_selection( dbselection),
                                 m_kerneldbs( dbkernellist ) { }
                  ~dbSelectionSet( ) { }
                  DBSelection    m_selection;
                  DBKernelDbList m_kerneldbs;
                } DBSelectionSet;

        typedef std::vector<DBSelectionSet> DBSelectionKernels;

        DBKernelDb         m_kerneldb;   /// The kernel.????.conf contents
        DBSelectionKernels m_selectionsets;


        DBKernelConf();
        DBKernelConf( const DBFileStatus &dbconfig );
        DBKernelConf( const DBKernelDb &dbkernel,
                      const PvlGroup &preferences = Preference::Preferences().findGroup("DataDirectory") );

        virtual ~DBKernelConf() { }

        static DBKernelConf read( const DBFileStatus &dbconfig,
                                  const PvlGroup &preferences = Preference::Preferences().findGroup("DataDirectory"));


        inline bool isvalid() const {
          return ( this->config().isvalid() );
        }

        inline const DBKernelDb &config() const {
          return ( m_kerneldb );
        }

        inline size_t size() const {
          return ( m_selectionsets.size() );
        }

        inline const DBSelectionKernels &kernelsets() const {
          return ( m_selectionsets );
        }


        void clear() {
          m_kerneldb = DBKernelDb( db_null() );
          m_selectionsets.clear( );
        }

        static DBFileStatus dbFileStatus( const DBFileStatus &dbdir );

        void addConfigSelection( const DBSelection &dbselection,
                                 const PvlGroup &preferences = Preference::Preferences().findGroup("DataDirectory") );

        static isisdata_json to_json( const DBKernelConf &dbkernelconfig );

        static bool compare( const DBKernelConf &db1,
                             const DBKernelConf &db2 ) {

          return ( DBKernelDb::compare( db1.config(),  db2.config() ) );
        }

        /** Confirm valid selection*/
        int validate( DBFileDispositionList &dbstatus,
                      const DBFileStatusSet &inventory = DBFileStatusSet() )const;
        int validate( const DBSelectionSet &dbset, DBFileDispositionList &dbstatus,
                      const DBFileStatusSet &inventory = DBFileStatusSet() ) const;

      protected:
        void addKernelDb( const DBKernelDb &dbconfig,
                          const PvlGroup &preferences = Preference::Preferences().findGroup("DataDirectory") );

    };

    /**
     * @brief ISISDATA model defining kernel structure
     *
     * This class contains the complete contents of the ISISDATA directory
     * structure. It provides algorithms to traverse and construct the
     * contents of the complete ISISDATA directory structure. It also
     * provides analysis and search algorithms to help categorize and
     * evaluate the kernel data maps.
     *
     */
    class IsisDataModel {

      public:

        DBFileStatus m_isisdata;   // $ISISDATA
        DBFileStatus m_dataroot;   // Directory source to check

        typedef std::vector<DBKernelDb>   DBKernelDbList;
        typedef std::vector<DBKernelConf> DBKernelConfList;

        DBFileStatusSet   m_allfiles;
        DBKernelDbList    m_kerneldbs;
        DBKernelConfList  m_configs;

        IsisDataModel();
        IsisDataModel(const QString &dataroot,
                      const QString &isisdata = "$ISISDATA");
        virtual ~IsisDataModel() { }

        inline size_t allFilesCount(size_t *n_directories = nullptr  ) const {

          // Only do this if requested
          if ( nullptr != n_directories ) {
            size_t n_dirs = 0;
            for ( auto const &file : m_allfiles ) {
              std::filesystem::path filepath = file.first;
              if (std::filesystem::is_directory(filepath)) {
                n_dirs++;
              }
            }

            *n_directories = n_dirs;
          }
          return ( m_allfiles.size() );
        }

        inline size_t justFilesCount( size_t *install_size = nullptr ) const {
          size_t n_files(0);
          size_t n_bytes(0);
          for ( auto const &file : m_allfiles ) {
            std::filesystem::path filepath = file.first;
            if ( !std::filesystem::is_directory(filepath) ) {
              n_files++;
              n_bytes += file.second.size();
            }
          }

          // If they want the size, set it here
          if ( nullptr != install_size ) {
            *install_size = n_bytes;
          }
          return ( n_files );
        }

        inline size_t size() const {
          return ( m_allfiles.size() );
        }

        inline size_t dbCount() const {
          return ( m_kerneldbs.size() );
        }

        inline size_t configCount() const {
          return ( m_configs.size() );
        }

        inline const DBKernelDbList &dbs() const {
          return ( m_kerneldbs );
        }

        inline const DBKernelConfList &configs() const {
          return ( m_configs );
        }

        inline size_t installSize() const {
          size_t n_bytes(0);
          for ( auto const &file : m_allfiles ) {
            std::filesystem::path filepath = file.first;
            if ( !std::filesystem::is_directory(filepath) ) n_bytes += file.second.size();
          }
          return ( n_bytes );
        }

        inline const DBFileStatus &dataroot( ) const {
          return ( m_dataroot );
        }

        inline const DBFileStatus &isisdata( ) const {
          return ( m_isisdata );
        }

        inline bool setIsisData( const QString &isisdata ) {
          m_isisdata = DBFileStatus( isisdata );
          return ( m_isisdata.exists() );
        }

        inline QString givenPath( const QString &dbfilespec ) const {
          QString t_dbfile = QString( dbfilespec ).replace( isisdata().expanded(), isisdata().original() );
          t_dbfile.replace( dataroot().expanded(), dataroot().original() );
          return ( t_dbfile );
        }

        inline const DBFileStatusSet &allfiles() const {
          return ( m_allfiles );
        }

        inline bool hasIsisData( ) const {
          return ( m_isisdata.exists() );
        }

        inline bool setDataRoot( const QString &dataroot ) {
          m_dataroot = DBFileStatus( dataroot );
          return ( m_dataroot.exists() );
        }

        inline bool hasDataRoot( ) const  {
          return ( m_dataroot.exists() );
        }

        inline void clear() {
          m_allfiles.clear();
          m_kerneldbs.clear();
          m_configs.clear();
        }

        size_t evaluate( );

        size_t evaluate( const QString &dataroot,
                         const QString &isisdata = "$ISISDATA" ) {
          setDataRoot( dataroot );
          setIsisData( isisdata );
          return ( evaluate() );
        }

        int validate( DBFileDispositionList &dbstatus ) const;

        int validate( DBFileDispositionList &dbstatus,
                      const DBFileStatusSet &inventory ) const;

    };

  } // namespace Data
} // namespace Isis
#endif
