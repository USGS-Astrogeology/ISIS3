#include <cmath>
#include <algorithm>

#include "QTemporaryDir"
#include "IsisDataFixtures.h"


#include "IsisDataModel.h"
#include "isisdataeval.h"
#include "CSVReader.h"

#include "TempFixtures.h"
#include "TestUtilities.h"
#include "gtest/gtest.h"

using namespace Isis;

inline CSVReader load_isisdata_csv( const QString &csvfile ) {
  const bool hasHeader = true;
  return ( CSVReader( csvfile, hasHeader ) );
}

inline QStringList row_from_csv( const CSVReader::CSVAxis &row  ) {
  QStringList rdata;
  for ( int i = 0 ; i < row.dim1() ; i++ ) {
    rdata.append( row[i].simplified() );
  }

  return ( rdata );
}

inline QStringList get_row( const CSVReader::CSVTable &table, const int rindex = 0 ) {
  return ( row_from_csv( table[rindex] ) );
}

inline QStringList get_header( const CSVReader &csv  ) {
  return ( row_from_csv( csv.getHeader() ) );
}

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/isisdataeval.xml").expanded());

TEST_F( IsisDataInventory, ConfirmIsisDataInventory ) {
  Pvl appLog;
  const QString isisinventoryfile = tempDir.path() + "/isisdata_inventory.csv";
  const QString isiserrorsfile    = tempDir.path() + "/isisdata_errors.csv";
  const QString isisissuesfile    = tempDir.path() + "/isisdata_issues.csv";

  QVector<QString> args = {"isisdata=" + isisdatadir(),
                           "datadir="  + isisdatadir(),
                           "verify=true",
                           "toinventory=" + isisinventoryfile,
                           "toissues=" + isisissuesfile,
                           "toerrors=" + isiserrorsfile };

  UserInterface options(APP_XML, args);
  try {
    isisdataeval(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to process ISISDATA directory:  " << isisdatadir().toStdString()  << std::endl;
  }

  // Get Results group from output
  const PvlGroup &results = appLog.findGroup( "Results" );
  EXPECT_EQ( (int) results["TotalDataFiles"],      this->size() );
  EXPECT_EQ( (int) results["EmptyKernelDBs"],      1 );
  EXPECT_EQ( (int) results["MissingKernelDBs"],    4 );
  EXPECT_EQ( (int) results["ExternalKernelFiles"], 3 );
  EXPECT_EQ( (int) results["SymlinkKernelFiles"],  0 );
  EXPECT_EQ( (int) results["TotalDBConfigFiles"],  1 );
  EXPECT_EQ( (int) results["TotalKernelDBFiles"],  27 );
  EXPECT_EQ( (int) results["TotalDirectories"],    43 );
  EXPECT_EQ( (int) results["TotalDataFiles"],      176 );
  EXPECT_EQ( (int) results["TotalInstallSize"],    118961);

  // Load the inventory file
  CSVReader  csv_inventory = load_isisdata_csv( isisinventoryfile );
  EXPECT_EQ( csv_inventory.rows() , this->size() );

  int n_issues = (int) results["EmptyKernelDBs"] +
                 (int) results["MissingKernelDBs"] +
                 (int) results["ExternalKernelFiles"] +
                 (int) results["SymlinkKernelFiles"];

  FileName issues( isisissuesfile.toStdString() );
  if ( issues.fileExists() ) {
    CSVReader  csv_issues = load_isisdata_csv( isisissuesfile );
    EXPECT_EQ( csv_issues.rows() ,n_issues );


    // Now loop to determine if isisdataeval correctly identified the issues
    int n_empty_found     = 0;
    int n_missing_found   = 0;
    int n_external_found  = 0;
    int n_symlinks_found  = 0;
    int n_errors_found    = 0;
    int n_undefined_found = 0;

    QStringList issues_header = get_header ( csv_issues );
    int         status_t      = issues_header.indexOf( "status" );
    int         filespec_t    = issues_header.indexOf( "filespec" );

    int error_n = csv_issues.rows();
    for ( int i = 0 ; i < error_n ; i++ ) {

      QStringList rowdata = get_row( csv_issues.getTable(), i );

      const QString status      = rowdata[status_t];
      QString isisdata_filename = rowdata[filespec_t];

      FileName  fnFile( isisdata_filename.toStdString() );
      // if ( fnFile.isVersioned() ) fnFile.highestVersion();
      QFileInfo qfFile( QString::fromStdString(fnFile.expanded()) );

      if ( "empty" == status ) {
        n_empty_found++;
        EXPECT_EQ ( (int) qfFile.size(), 0 );
      }
      else if ( "missing" == status ) {
        n_missing_found++;
        EXPECT_EQ( fnFile.fileExists(), false );
      }
      else if ( "external" == status ) {
        n_external_found++;
      }
      else if ( "symlink" == status ) {
        n_symlinks_found++;
      }
      else if ( "error" == status ) {
        n_errors_found++;
      }
      else {
        n_undefined_found++;
      }

    }

    // Test for contents being consistent with reported status
    EXPECT_EQ( (int) results["EmptyKernelDBs"],      n_empty_found);
    EXPECT_EQ( (int) results["MissingKernelDBs"],    n_missing_found);
    EXPECT_EQ( (int) results["ExternalKernelFiles"], n_external_found);
    EXPECT_EQ( (int) results["SymlinkKernelFiles"],  n_symlinks_found);
    EXPECT_EQ( (int) results["ErrorKernelFiles"],    n_errors_found);
    EXPECT_EQ( n_undefined_found,   0);
  }

  // There are no errors for this case
  FileName errors( isiserrorsfile.toStdString() );
  EXPECT_EQ( errors.fileExists() , true );
  if ( errors.fileExists() ) {
    CSVReader  csv_errors = load_isisdata_csv( isiserrorsfile );
    EXPECT_EQ( csv_errors.rows() ,  (int) results["ErrorInInventory"]);
  }

  // Process the ISISDATA inventory
  int nrows              = std::min( (int) csv_inventory.rows(), (int) this->size() );
  QStringList inv_header = get_header (csv_inventory );
  int target_t           = inv_header.indexOf( "target" );

  for ( int i = 0 ; i < nrows ; i++ ) {
    QStringList rowdata = get_row( csv_inventory.getTable(), i );
    QString isisdata_filename  = rowdata[target_t];
    isisdata_filename.replace( isisdata_path() , "." );

    EXPECT_EQ( inventory().contains( isisdata_filename ), true );
  }
}

TEST_F( IsisDataInventory, CompareHashDataInIsisDataInventory ) {

  const size_t MaxFileHashSize = 100 * 1024;

  Pvl appLog;
  QVector<QString> args = {"isisdata=" + isisdatadir(),
                           "datadir="  + isisdatadir(),
                           "verify=true" };

  UserInterface options(APP_XML, args);
  try {
    isisdataeval(options, &appLog);
  }
  catch (IException &e) {
    FAIL() << "Unable to process ISISDATA directory:  " << isisdatadir().toStdString()  << std::endl;
  }

  int n_md5_compared    = 0;
  int n_sha1_compared   = 0;
  int n_sha256_compared = 0;

  // Gota do it this way if its NOT the only test run to find the real ISISDATA
  PvlGroup &dataDir = Preference::Preferences(true).findGroup("DataDirectory");
  if ( dataDir.hasKeyword( "ISISDATA") ) dataDir.deleteKeyword( "ISISDATA" );

  IsisDataInventoryMap::const_iterator fileinfo = inventory().begin();
  while ( fileinfo != inventory().end() ) {

    if ( fileinfo.value().info().exists() ) {

      const json_t &jdata = fileinfo.value().data();
      QString source = QString::fromStdString( jdata["source"] );

      // Not all of these files will exist in the currently available ISISDATA
      // To save time only get the smaller ones as set above!
      Data::DBFileStatus real_isis_file_info( source );
      if ( real_isis_file_info.exists() && ( real_isis_file_info.size() < MaxFileHashSize ) ) {

        int filesize = jdata["filesize"];
        EXPECT_EQ( filesize, real_isis_file_info.size() );

        if ( jdata.contains( "md5hash" ) ) {
          QString md5hash = real_isis_file_info.hash( QCryptographicHash::Md5 ).toHex();
          EXPECT_EQ( jdata["md5hash"], md5hash.toStdString() );
          n_md5_compared++;
        }

        if ( jdata.contains( "sha1hash" ) ) {
          QString sha1hash = real_isis_file_info.hash( QCryptographicHash::Sha1 ).toHex();
          EXPECT_EQ( jdata["sha1hash"], sha1hash.toStdString() );
          n_sha1_compared++;

        }

        if ( jdata.contains( "sha256hash" ) ) {
          QString sha256hash = real_isis_file_info.hash( QCryptographicHash::Sha256 ).toHex();
          EXPECT_EQ( jdata["sha256hash"], sha256hash.toStdString() );
          n_sha256_compared++;
        }
      }

    }

    // Next inventory file
    ++fileinfo;
  }

  EXPECT_NE( n_md5_compared,    0);
  EXPECT_NE( n_sha1_compared,   0);
  EXPECT_NE( n_sha256_compared, 0);
}

TEST_F( IsisDataInventory, IsisDataEvalBadIsisDataDir ) {
   Pvl appLog;
   QString bad_isisdata_path = tempDir.path() + "/DirDoesNotExist";
   QVector<QString> args = {"isisdata=" + bad_isisdata_path,
                            "datadir="  + bad_isisdata_path };

   UserInterface options(APP_XML, args);

   // TEST: Bad ISISDATA directory
   try {
     isisdataeval(options, &appLog);
     FAIL() << "Fails when ISISDATA/DATADIR do not exist!" <<std::endl;
   }
   catch (IException &e ) {
     EXPECT_THAT(e.what(), testing::HasSubstr(" is not a directory!"));
   }
   catch ( ... ) {
     FAIL() << "Expected error: ... isisdata directory not found!";
   }
}