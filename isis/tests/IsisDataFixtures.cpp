#include "IsisDataFixtures.h"

#include <iostream>
#include <fstream>

#include <Qt>
#include <QDateTime>
#include <QStringList>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>

namespace Isis {

  QString IsisDataInventory::system_isisdata() const {
    return ( QString::fromStdString(m_system_isisdata.expanded()) );
  }

  QString IsisDataInventory::isisdatadir() const {
    return ( QString::fromStdString(m_isisdatadir.expanded()) );
  }

  QString IsisDataInventory::isisdata_path() const {
    QFileInfo isisdir( isisdatadir() );
    return ( isisdir.absoluteFilePath()  );
  }

  size_t IsisDataInventory::size() const {
    return ( inventory().size() );
  }

  QString IsisDataInventory::scrub_path_prefix( const QString &fname,
                                                const QString &path_prefix )
                                                const {
    QString t_dbfile = fname;
    t_dbfile.replace( isisdatadir(), path_prefix );
    return ( t_dbfile );
  }

  json_t IsisDataInventory::get_real_file_info( const QString &fname ) const {

    FileName v_fname = FileName( fname.toStdString() );
    QFileInfo qinfo( fname );
    json_t file_json;
    try {
      std::ifstream inputstream ( v_fname.expanded() );
      file_json = json_t::parse ( inputstream );
    }
    catch ( ... ) {
      file_json["source"]   = fname.toStdString();
      file_json["filesize"] = qinfo.size();
      file_json["exists"]   = v_fname.fileExists();

      if ( v_fname.fileExists() ) {
        file_json["createtime"]   = qinfo.birthTime().toString( Qt::ISODateWithMs ).toStdString();
        file_json["modifiedtime"] = qinfo.lastModified().toString( Qt::ISODateWithMs ).toStdString();
      }
    }

    return ( file_json );
  }

  void IsisDataInventory::SetUp() {
    TempTestingFiles::SetUp();

    m_system_isisdata = FileName( "$ISISDATA" ).expanded();
    m_isisdatadir     = FileName( "data/isisdata/mockup" );
    m_isisdata_inventory.clear();

    QDirIterator ddir( isisdatadir(),
                       QDir::NoDotAndDotDot | QDir::Dirs | QDir::AllDirs | QDir::Files | QDir::System,
                       QDirIterator::Subdirectories | QDirIterator::FollowSymlinks );

    while ( ddir.hasNext() ) {

      // Collect the data
      QString   ddfile   = ddir.next();
      QString   v_name   = scrub_path_prefix( ddfile );
      QFileInfo f_ddfile = ddir.fileInfo();
      if ( !f_ddfile.isDir() ) {
        json_t j_data   = get_real_file_info( f_ddfile.absoluteFilePath() );

        // Add to inventory
        m_isisdata_inventory.insert( v_name, IsisDataInventoryFile( v_name, f_ddfile, j_data ) );
      }
      else {
        QDir dirinfo( ddfile );
        json_t j_data;
        j_data["source"] = v_name.toStdString();
        j_data["files"] =  dirinfo.count();

        // Add to directory inventory
        m_isisdata_directories.insert( v_name,  IsisDataInventoryFile( v_name, f_ddfile, j_data ) );
      }
    }
  }

  void IsisDataInventory::TearDown() {
    m_system_isisdata = m_isisdatadir = FileName();
    m_isisdata_inventory.clear();
    m_isisdata_directories.clear();
  }


}