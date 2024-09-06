/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "GisTopology.h"

// std library
#include <cstdio>

// Qt library
#include <QByteArray>
#include <QCoreApplication>
#include <QString>

// geos library
#include <geos_c.h>

// other ISIS
#include "IException.h"

using namespace std;

namespace Isis {

  /**
   * A static instance of the GIS topology class. It is initialized to NULL and instantiated
   * when the instance() method is called.
   */
  GisTopology *GisTopology::m_gisfactory = 0;
  

  /** 
   * Private default constructor so that this class is only instatiated through the instance()
   * method. This ensures that only a singleton object is constructed. The GEOS readers/writers are
   * set to null and the GEOS C API is initialized.
   */  
  GisTopology::GisTopology() : m_WKTreader(0), m_WKTwriter(0), 
                               m_WKBreader(0), m_WKBwriter(0) {
    geosInit();
    //  This ensures this singleton is shut down when the application exists,
    //  so the GEOS system can be shut down cleanly.
    qAddPostRoutine(dieAtExit);
  }
  

  /** 
   * Destroy the GisTopology object. The GEOS readers/writers are destroyed and 
   * set to null and the GEOS C API is shut down. 
   */  
  GisTopology::~GisTopology() {
    if (m_WKTreader) {  
      GEOSWKTReader_destroy(m_WKTreader);   
      m_WKTreader = 0;
    }
  
    if (m_WKTwriter) {  
      GEOSWKTWriter_destroy(m_WKTwriter);   
      m_WKTwriter = 0;
    }
  
    if (m_WKBreader) {  
      GEOSWKBReader_destroy(m_WKBreader);   
      m_WKBreader = 0;
    }
  
    if (m_WKBwriter) {  
      GEOSWKBWriter_destroy(m_WKBwriter);   
      m_WKBwriter = 0;
    }
  
    geosFinish();
  }
  

  /** 
   * Gets the singleton instance of this class. If it has not been instantiated yet, 
   * the default constructor is called.
   *  
   * @return GisTopology A pointer to the GisTopology singleton object.
   */  
  GisTopology *GisTopology::instance() { 
    if (!m_gisfactory) {
      m_gisfactory = new GisTopology();
    }
    return (m_gisfactory);
  }
  

  /** 
   * Reads in the geometry from the given well-known binary formatted string. 
   *  
   * @param wkb The well-known binary formatted string containing the geometry 
   *            information to be parsed.
   *  
   * @return GEOSGeometry* A pointer to a GEOSGeometry object created from the parsed wkb. 
   * @throw IException::Programmer "Unable convert the given well-known binary to a GEOSGeometry.
   */  
  GEOSGeometry *GisTopology::geomFromWKB(const QString &wkb) {
  #if 0
    QByteArray wkb_data = wkb.toLatin1();
    const unsigned char *footy = reinterpret_cast<const unsigned char *> (wkb_data.data());
    GEOSGeometry *geom = GEOSWKBReader_readHEX(wkbReader(), footy, wkb.size()); 
  #else
    GEOSGeometry *geom = GEOSWKBReader_readHEX(wkbReader(),
                 reinterpret_cast<const unsigned char *> (wkb.toLatin1().data()),
                                               wkb.size());
  #endif
    if (!geom) {
      std::string mess = "Unable convert the given WKB string [" + wkb.toStdString() + "] to a GEOSGeometry";
      throw IException(IException::Programmer, mess, _FILEINFO_);
  
    }
    return (geom);
  }
  

  /** 
   * Reads in the geometry from the given well-known text formatted string. 
   *  
   * @param wkt The well-known text formatted string containing the geometry 
   *            information to be parsed.
   *  
   * @return GEOSGeometry* A pointer to a GEOSGeometry object created from the parsed wkt.
   * @throw IException::Programmer "Unable convert the given well-known text to a GEOSGeometry.
   */  
  GEOSGeometry *GisTopology::geomFromWKT(const QString &wkt) {
    QByteArray wkt_data = wkt.toLatin1();
    const char *footy = wkt_data.data();
    GEOSGeometry *geom = GEOSWKTReader_read(wktReader(), footy);
    if (!geom) {
      std::string mess = "Unable convert the given WKT string [" + wkt.toStdString() + "] to a GEOSGeometry";
      throw IException(IException::Programmer, mess, _FILEINFO_);
  
    }
    return (geom);
  }
  

  /** 
   * Clones the given GEOSGeometry pointer. 
   *  
   * @param geom A pointer to the GEOSGeometry object to be cloned.
   *  
   * @return GEOSGeometry* A clone of the given GEOSGeometry pointer. 
   */  
  GEOSGeometry *GisTopology::clone(const GEOSGeometry *geom) const {
    if (!geom) return (0);
    return (GEOSGeom_clone(geom));
  }
  
  
  /** 
   * Gets a GEOSPreparedGeometry from the given GEOSGeometry. 
   *  
   * @param geom A pointer to a GEOSGeometry object to be converted.
   *  
   * @return GEOSPreparedGeometry A pointer to a prepared geometry from the given geometry pointer.
   * @throw IException::Programmer "Unable to convert the given GEOSGeometry to 
   *                                a GEOSPreparedGeometry."
   */  
  const GEOSPreparedGeometry *GisTopology::preparedGeometry(const GEOSGeometry *geom) const {
    const GEOSPreparedGeometry *ppgeom = GEOSPrepare(geom);
    if (!ppgeom) {
      throw IException(IException::Programmer, 
                       "Unable convert the given GEOSGeometry to a GEOSPreparedGeometry",
                       _FILEINFO_);
  
    }
    return (ppgeom);
  }
  

  /** 
   * Writes a well-known text string from the given geometry. This method will 
   * destroy the given GEOSGeometry pointer if so indicated by the given 
   * Disposition enumeration. 
   * 
   * @param geom A pointer to the geometry to be represented as a WKT string
   * @param disp A reference to a topology disposition enumeration indicating 
   *             whether to preserve or destroy the passed in geometry.
   * 
   * @return QString A well-known text string containg the geometry information.
   */  
  QString GisTopology::wkt(const GEOSGeometry *geom, 
                           const GisTopology::Disposition &disp) {
    char *wkt_h = GEOSWKTWriter_write(wktWriter(), geom);
    QString thegeom = QString::fromLatin1(reinterpret_cast<const char *> (wkt_h));
  
    if (disp == DestroyGeometry) { 
      destroy(geom); 
    }
    destroy(wkt_h);
  
    return (thegeom);
  }
  

  /** 
   * Writes a well-known binary string from the given geometry. This method will
   * destroy the given GEOSGeometry pointer if so indicated by the given 
   * Disposition enumeration. 
   * 
   * @param geom A pointer to the geometry to be represented as a WKT string
   * @param disp A reference to a topology disposition enumeration indicating 
   *             whether to preserve or destroy the passed in geometry.
   * 
   * @return QString A well-known binary string containg the geometry 
   *         information.
   */  
  QString GisTopology::wkb(const GEOSGeometry *geom, 
                           const GisTopology::Disposition &disp) {
    size_t length;
    unsigned char *wkt_h = GEOSWKBWriter_writeHEX(wkbWriter(), geom, &length);
    QString thegeom = QString::fromLatin1(reinterpret_cast<const char *> (wkt_h), length);
  
    if (disp == DestroyGeometry) { 
      destroy(geom); 
    }
    destroy(wkt_h);
  
    return (thegeom);
  }
  
  
  /** 
   * Destroys the given GEOS geometry.  
   *  
   * @param geom A pointer to the GEOSGeometry to be destroyed.
   */  
  void GisTopology::destroy(GEOSGeometry *geom) const {
    if (geom) { 
      GEOSGeom_destroy(geom); 
    }
    return;
  }
  

  /** 
   * Destroys the given GEOS geometry.  
   *  
   * @param geom A pointer to the GEOSGeometry to be destroyed.
   */  
  void GisTopology::destroy(const GEOSGeometry *geom) const {
    if (geom) { 
      destroy(const_cast<GEOSGeometry *> (geom)); 
    }
    return;
  }
  

  /** 
   * Destroys the given prepared GEOS geometry.  
   *  
   * @param geom A pointer to the GEOSPreparedGeometry to be destroyed.
   */  
  void GisTopology::destroy(const GEOSPreparedGeometry *geom) const {
    if (geom) { 
      GEOSPreparedGeom_destroy(geom); 
    }
    return;
  }
  

  /** 
   * Destroys the given GEOS coordinate sequence.  
   *  
   * @param sequence A pointer to the GEOSCoordSequence to be destroyed.
   */  
  void GisTopology::destroy(GEOSCoordSequence *sequence) const {
    if (sequence) { 
      GEOSCoordSeq_destroy(sequence); 
    }
    return;
  }
  
  
  /** 
   * Destroys the given GEOS text geometry representation.  
   *  
   * @param geos_text A pointer to the GEOS text char to be destroyed.
   */  
  void GisTopology::destroy(const char *geos_text) const {
    if (geos_text) {
      GEOSFree(const_cast<char *> (geos_text));
    }
    return;
  }
  

  /** 
   * Destroys the given unsigned GEOS text geometry representation. 
   *  
   * @param geos_text A pointer to the GEOS text char to be destroyed.
   */  
  void GisTopology::destroy(const unsigned char *geos_text) const {
    if (geos_text) {
      GEOSFree(const_cast<unsigned char *> (geos_text));
    }
    return;
  }
  

  /** 
   * Initializes the GEOS C API
   */  
  void GisTopology::geosInit() {
    initGEOS(GisTopology::notice, GisTopology::error);
  }
  

  /** 
   * Shuts down the GEOS C API
   *  
   */  
  void GisTopology::geosFinish() {
    finishGEOS();
  }
  

  /** 
   * A static method for handling errors. 
   *  
   * @param fmt
   *  
   * @throw IException::Programmer
   */  
  void GisTopology::notice(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    throw IException(IException::Programmer, buffer, _FILEINFO_);
  }
  

  /** 
   * A static method for handling errors. 
   *  
   * @param fmt
   *  
   * @throw IException::Programmer
   */  
  void GisTopology::error(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    va_end(ap);
    throw IException(IException::Programmer, buffer, _FILEINFO_);
  }
  

  /** 
   * Accessor for the GEOS well-known text reader. This method creates the 
   * reader if it doesn't exist. 
   *  
   * @return GEOSWKTReader A pointer to a GEOS library parser for well-known text format.
   */  
  GEOSWKTReader *GisTopology::wktReader() {
    if (!m_WKTreader) {
      m_WKTreader = GEOSWKTReader_create();
    }
    return (m_WKTreader);
  }
  

  /** 
   * Accessor for the GEOS well-known text writer. This method creates the
   * writer if it doesn't exist. 
   *  
   * @return GEOSWKTWriter A pointer to a GEOS library writer for well-known text format.
   */  
  GEOSWKTWriter *GisTopology::wktWriter() {
    if (!m_WKTwriter) {
      m_WKTwriter = GEOSWKTWriter_create();
      GEOSWKTWriter_setTrim(m_WKTwriter, 1);
    }
    return (m_WKTwriter);
  }
  

  /** 
   * Accessor for the GEOS well-known binary reader. This method creates the 
   * reader if it doesn't exist. 
   *  
   * @return GEOSWKBReader A pointer to a GEOS library parser for well-known binary format.
   */  
  GEOSWKBReader *GisTopology::wkbReader() {
    if (!m_WKBreader) {
      m_WKBreader = GEOSWKBReader_create();
    }
    return (m_WKBreader);
  }
  

  /** 
   * Accessor for the GEOS well-known binary writer. This method creates the 
   * writer if it doesn't exist. 
   *  
   * @return GEOSWKBWriter A pointer to a GEOS library writer for well-known binary format.
   */  
  GEOSWKBWriter *GisTopology::wkbWriter() {
    if (!m_WKBwriter) {
      m_WKBwriter = GEOSWKBWriter_create();
    }
    return (m_WKBwriter);
  }

  /**
   * @brief Exit termination routine.
   *
   * This (static) method ensures that this object is destroyed when Qt exits.  
   *
   * Note that this should not be added to the system _atexit() routine because 
   * this object utilizes Qt classes.  At the time the atexit call stack is 
   * executed, Qt is long gone resulting in Very Bad Things.  Fortunately, Qt has 
   * an exit stack function as well.  This method is added to the Qt exit call 
   * stack. 
   */
  void GisTopology::dieAtExit() {
    delete  m_gisfactory;
    m_gisfactory = 0;
    return;
  }

} // namespace Isis
