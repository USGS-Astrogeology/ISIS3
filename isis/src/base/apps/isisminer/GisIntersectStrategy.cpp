/**                                                                       
 * @file                                                                  
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: GisIntersectStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
#include "GisIntersectStrategy.h"

// other ISIS
#include "IException.h"
#include "GisGeometry.h"
#include "GisTopology.h"
#include "Pvl.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   *  Creates an empty GisIntersectStrategy object
   */  
  GisIntersectStrategy::GisIntersectStrategy() : 
                          Strategy("GisIntersect", "GisIntersect"),
                          m_geom(), m_computeOverlap(false), m_ratioKey() { 
  }
  

  /** 
   *  @brief Creates a GisIntersectStrategy object using its PVL
   *         definition.
   *  
   * @param definition GisIntersectStrategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */  
  GisIntersectStrategy::GisIntersectStrategy(const PvlObject &definition, 
                                             const ResourceList &globals) : 
                                         Strategy(definition, globals),
                                         m_geom(), m_computeOverlap(false), m_ratioKey() {
  
    PvlFlatMap parms( getDefinitionMap() );
    QString gistype = parms.get("GisType").toLower();
    GisGeometry::Type gtype = GisGeometry::type(gistype);
    if ( GisGeometry::None != gtype ) {
      //in case it is a filename
      QString gsource = translateKeywordArgs("GisGeometry", globals);
      m_geom = SharedGisGeometry(new GisGeometry(gsource, gtype));
    }
    else {
      m_geom = SharedGisGeometry(geomFromPvl(parms, globals));
    }

    //  See if user wants a bounding box from the source
    if ( toBool(parms.get("BoundingBox", "false").toStdString()) ) {
      GEOSGeometry *geom = GEOSEnvelope(m_geom->geometry());
      m_geom->setGeometry(geom);
    }

    // Check for validity
    if ( !m_geom->isValid() ) {
      std::string mess = "User provided geometry for " + name().toStdString()  +
                     " is not valid!";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    if ( isDebug() ) {
       // Echo back WKT geometry
       GisTopology *gis = GisTopology::instance();
       QString wkt = gis->wkt(m_geom->geometry());
       cout << "GisGeometry = " << wkt.toStdString()  << "\n";
    }

     m_computeOverlap = toBool(parms.get("ComputeRatio", "false").toStdString());
     m_ratioKey       = parms.get("RatioRef", ""); 

     return; 
  }
  
  
  /** 
   *  @brief Destroys the GisIntersectStrategy object
   */  
  GisIntersectStrategy::~GisIntersectStrategy() { 
  }
  

  /** 
   * @brief Constructs a geometry using a PVL file. 
   *  
   * The PVL file to read from is indicated using the GisGeometry 
   * keyword in the PVL configuration file. 
   *  
   * Ex. GisGeometry = "filename.pvl"  
   *  
   * @param parms GisIntersetStrategy PVL object definition 
   * @param globals   List of global keywords to use in argument substitutions
   *  
   * @return geom GisGeometry object constructed from PVL object 
   *         definition
   */  
  GisGeometry *GisIntersectStrategy::geomFromPvl(const PvlFlatMap &parms, 
                                                 const ResourceList &globals) const {
    GisGeometry *geom(0);
  
    QString pfile = translateKeywordArgs("GisGeometry", globals); 

    Pvl pvl(pfile.toStdString()); 

    //Keywoard in IsisMiner PVL that has an assoc. geometric value in pfile
    QString key = parms.get("GisGeometryRef"); 
    QString gisgeom = QString::fromStdString(pvl.findKeyword(key.toStdString(), PvlObject::Traverse));
  
    QString gistype = parms.get("GisType"); //must be pvlWKT or pvlWKB
    gistype = gistype.remove("pvl", Qt::CaseInsensitive);
    GisGeometry::Type gtype = GisGeometry::type(gistype); 
    
    try {
      geom = new GisGeometry(gisgeom, gtype);
    }
    catch (IException &ie) {
      std::string mess = "Problems converting GIS Geometry in keyword " + key.toStdString() +
                     ", PVL source file: " + pfile.toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
  
    return (geom);
  }


  /** 
   * @brief Applies efficient overlap query to identify 
   *               intersectors.
   *  
   * After finding the intersectors, this method applies the 
   * Strategy::apply(SharedResource &) method for all Resources 
   * whose geometry intersects the given geometry. 
   *  
   * Please see Strategy::applytoIntersectedGeometry for more 
   * information.  
   *  
   * @param resources List of Resources
   * @param globals   List of global keywords to use in argument substitutions
   * @return int Number of Resources in intersection
   */ 
  int GisIntersectStrategy::apply(ResourceList &resources, 
                                  const ResourceList &globals) {
     return (applyToIntersectedGeometry(resources, *m_geom, globals));
  }
  

  /** 
   * @brief Evaluate potential intersectors with more precise/less
   * efficient intersection algorithm. 
   *  
   * Computes the intersection ratio and adds it to the Resource 
   * if m_computeOverlap is true. 
   *  
   * @param resource A single Resource 
   * @param globals   List of global keywords to use in argument substitutions
   * @return int 0 if Resource does not intersect with geometry; 1
   *         if Resource does intersect with geometry. 
   */
  int GisIntersectStrategy::apply(SharedResource &resource,
                                  const ResourceList &globals) {
    if ( !resource->geometry()->intersects(*m_geom) ) {
      resource->discard();
      return (0);
    }
    else {
      if ( m_computeOverlap ) {
        double ratio = resource->geometry()->intersectRatio(*m_geom);
        resource->add(m_ratioKey, QString::fromStdString(toString(ratio)));
      }
    }
    return (1);
  }

}  //namespace Isis
