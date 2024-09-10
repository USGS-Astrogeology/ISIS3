#ifndef QDebugLogger_h
#define QDebugLogger_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdio>

#include <QDebug>
#include <QFile>
#include <QList>
#include <QString>
#include <QSharedPointer>
#include <QTextStream>

#include "FileName.h"
#include "FeatureMatcherTypes.h"

namespace Isis {

//  Define components for the logger. Note that QTextStream and QDebug classes
//  are known to work as a QDebugStreamType. Unfortunatenly, users of these
//  types must explicitly flush the stream in order to see the output
//  immediately.
class QDebugLogger;
#define STRING_DEBUG_SUPPORTED       1
#define STRING_OMODE_OK              1
typedef QTextStream                  QDebugStreamType;
typedef QSharedPointer<QDebugLogger> QDebugStream;

/**
 * @brief Specialized class to provide consistent interface to debug logger
 *
 * This interface provides some flexibility in constructing an easy to use
 * interface to a generic output streams.  The methods available here are
 * designed to enforce a shared pointer API. This is necessary as QFILE and
 * QDebugStreamType must coexist. Scoped pointers enforce this requirement.
 *
 * Some QDebugStreamTypes may require flushing after writing in order
 * immediately to see the output.
 *
 * @author 2015-09-09 Kris Becker
 * @internal
 *   @history 2015-09-09 Kris Becker - Original Version
 *   @history 2016-04-26 Ian Humphrey - Modified open mode for /dev/null so that QIODevice::Append
 *                           is not set. This causes a seek to occur (to end of device) on a
 *                           sequential device (see Qt5 documentation on QIODevice::isSequential),
 *                           which Qt5 does not allow.
 */

class QDebugLogger {
  public:
    /** Release in specific order */
    ~QDebugLogger() {
      m_dbuglog.reset();
      m_dbugfile.reset();
    }

    /** Map files to the logger using this method */
    static QDebugStream create(const QString &filename,
                               const QIODevice::OpenMode &omode =
                                                        (QIODevice::WriteOnly |
                                                         QIODevice::Append |
                                                         QIODevice::Text |
                                                         QIODevice::Unbuffered) ) {
      // Default condition is to write to std::cout
      if ( filename.isEmpty() ) {  return ( toStdOut() );  }

      // Set up file access logging
      FileName t_fname(filename.toStdString());
      QScopedPointer<QFile> t_dbugfile( QDebugLogger::open( QString::fromStdString(t_fname.expanded()), omode) );
      QScopedPointer<QDebugStreamType> t_dbuglog( new QDebugStreamType( t_dbugfile.data() ) );
      return ( QDebugStream( new QDebugLogger( t_dbugfile.take(), t_dbuglog.take() ) ) );
    }

    /** Map streams like stdout, stderr, etc..., using this method */
    static QDebugStream create(FILE *fh, const QIODevice::OpenMode &omode =
                                                        (QIODevice::WriteOnly |
                                                         QIODevice::Append |
                                                         QIODevice::Text |
                                                         QIODevice::Unbuffered) ) {
       QScopedPointer<QFile> t_dbugfile( new QFile() );
       t_dbugfile->open( fh, omode );
       QScopedPointer<QDebugStreamType> t_dbuglog( new QDebugStreamType( t_dbugfile.data() ) );
       return ( QDebugStream( new QDebugLogger( t_dbugfile.take(), t_dbuglog.take() ) ) );
    }


    /** Map strings to debugger output device using this method. Set
     *  appropriate parameters for proper compilation above. */
    static QDebugStream create(QString *dbstring,
                               const QIODevice::OpenMode &omode = QIODevice::WriteOnly ) {

      // Check for string support in debugger
#if ( STRING_DEBUG_SUPPORT == 0 )
       throw IException(IException::Programmer,
                        "QDebugLogger does not support strings as an output device!",
                        _FILEINFO_);
#endif

       QScopedPointer<QFile> t_dbugfile( 0 );
#if ( STRING_OMODE_OK == 0 )
       QScopedPointer<QDebugStreamType> t_dbuglog( new QDebugStreamType( dbstring, omode ) );
#else
       QScopedPointer<QDebugStreamType> t_dbuglog( new QDebugStreamType( dbstring ) );
#endif

       return ( QDebugStream( new QDebugLogger( t_dbugfile.take(), t_dbuglog.take() ) ) );
    }

    /** Default constructor to null device */
    static QDebugStream create() {
      return ( null() );
    }

    static QDebugStream toStdOut() {
      return ( create( stdout ) );
    }

    static QDebugStream null() {
      return ( create( "/dev/null", QIODevice::WriteOnly | QIODevice::Unbuffered ) );
    }


    /** Returns underlying logger for additional capabilities */
    QDebugStreamType &dbugout() const {
      return ( *m_dbuglog );
    }

    QDebugStreamType &flush() const {
      dbugout().flush();
      return ( dbugout() );
    }

  private:
    // Do not allow direct creation or copying for now. Use the enforced
    // shared pointer API
    /** Default constructor defined here to deny direct instantiation. This
     *  helps enforces the shared pointer API */
    QDebugLogger() {
       m_dbugfile.reset( QDebugLogger::open("/dev/null", QIODevice::WriteOnly |
                                                         QIODevice::Unbuffered) );
       m_dbuglog.reset( new QDebugStreamType( m_dbugfile.data() ) );
    }

    /** No implementation provided to prevent copying */
    QDebugLogger(const QDebugLogger &other);
    /** No implementation provided to prevent copying */
    QDebugLogger operator=(const QDebugLogger &other);

    /** Direct instantiation of prepared components for a logger stream   */
    QDebugLogger(QFile *logfile, QDebugStreamType *logger) :
                 m_dbugfile(logfile), m_dbuglog(logger) { }

    // These variables consistently manage logging activities
    QScopedPointer<QFile>            m_dbugfile;
    QScopedPointer<QDebugStreamType> m_dbuglog;

    /** Method creates a QFile from a filename */
    static QFile *open(const QString &filename,
                       const QIODevice::OpenMode &omode) {
      FileName t_fname(filename.toStdString());
      QScopedPointer<QFile> t_dbugfile(new QFile( QString::fromStdString(t_fname.expanded()) ) );
      if ( !t_dbugfile->open(omode) ) {
        std::string mess = "Unable to open/create debug log stream for file: " +
                       filename.toStdString();
        throw IException(IException::User, mess, _FILEINFO_);
      }
      return ( t_dbugfile.take() );
    }
};


/**
 * @brief API for indepent logger usage
 *
 * Users of the QDebugLogger system can inherit from this class and include the
 * interface in their classes seamlessly. Or can make an instance of it and use
 * it directly as a member class.
 *
 * @author 2015-09-09 Kris Becker
 * @internal
 *   @history 2015-09-09 Kris Becker - Original Version
 */
class QLogger {
  public:
    QLogger() : m_logger( QDebugLogger::null() ), m_debug( false ) { }
    explicit QLogger(QDebugStream logger, const bool &debug = true) :
                      m_logger( logger ), m_debug( debug ) { }
    virtual ~QLogger() { }


    void setDebugLogger(QDebugStream logger = QDebugLogger::null(),
                        const bool &debug = true) {
      m_logger = logger;
      m_debug = debug;
      return;
    }

    void setDebugOff() {
      m_debug = false;
      return;
    }
    void setDebugOn() {
      m_debug = true;
      return;
    }

    inline bool isDebug() const {  return ( m_debug );  }

    inline QDebugStreamType &logger() const {
      return ( m_logger->dbugout() );
    }

    inline QDebugStream stream() const {
      return ( m_logger );
    }

  private:
    QDebugStream m_logger;   // Debuging stream
    bool         m_debug;    // Turn on/off real time debugging statements

};


}  // namespace Isis
#endif
