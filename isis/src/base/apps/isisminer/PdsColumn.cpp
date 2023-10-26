/**                                                                       
 * @file                                                                  
 * $Revision: 6172 $
 * $Date: 2015-05-06 15:52:18 -0700 (Wed, 06 May 2015) $
 * $Id: PdsColumn.cpp 6172 2015-05-06 22:52:18Z kbecker@GS.DOI.NET $
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
#include "PdsColumn.h"

// Qt library
#include <QRegExp>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "PvlContainer.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {

  /** 
   * Constructs a PDS column resource object. The name is set to "Resource" and
   * discard is set to false. The keyword and asset lists are left empty. 
   */  
  PdsColumn::PdsColumn() : Resource() { 
  }
  
  
  /** 
   * Constructs a PDS column resource object using the given name. Discard is 
   * set to false. The keyword and asset lists are left empty.
   *  
   * @param name  A string containing the name of the PDS column resource.
   */  
  PdsColumn::PdsColumn(const QString &name) : Resource(name)  { 
  }
  
  
  /** 
   * Copy constructor for a PDS column resource object. This constructor simply 
   * calls the parent copy constructor. 
   *
   * @param other  Reference to the Resource to be copied.
   */  
  PdsColumn::PdsColumn(const Resource &resource) : Resource(resource) { 
  }
  

  /** 
   * Constructs a PDS column resource using the given PVL container describing 
   * the column. Discard is set to false. The asset list is left empty. 
   *  
   * The given container must have keywords for NAME, COLUMN_NUMBER, DATA_TYPE, 
   * START_BYTE, BYTES, and DESCRIPTION. The name of this resource is read from 
   * the "NAME" keyword in the container. 
   *  
   * @param column  A PVL container of keywords that define the column. The of 
   *                this column must be specified in the container using a
   *                "Name" keyword.
   */
  PdsColumn::PdsColumn(const PvlContainer &column)  : Resource("PdsColumn", column)  {
    setName(value("Name"));
  }
  
  
  /** 
   * Destroys the PDS column resource.
   */  
  PdsColumn::~PdsColumn() { 
  }
  
  
  /** 
   * Determines whether this PDS column resource is valid. The resource is not 
   * valid if it is missing any of the required keywords: 
   * <ul> 
   *   <li>COLUMN_NUMBER</li>
   *   <li>NAME</li>
   *   <li>DATA_TYPE</li>
   *   <li>START_BYTE</li>
   *   <li>BYTES</li>
   *   <li>DESCRIPTION</li>
   * </ul>
   * @return bool
   */  
  bool PdsColumn::isValid() const {
    if ( !exists("COLUMN_NUMBER") ) return (false);
    if ( !exists("NAME") ) return (false);
    if ( !exists("DATA_TYPE") ) return (false);
    if ( !exists("START_BYTE") ) return (false);
    if ( !exists("BYTES") ) return (false);
    // if ( !exists("FORMAT") ) return (false);
    if ( !exists("DESCRIPTION") ) return (false);
    return (true);
  }
  
  
  /** 
   * Gets the number of bytes allocated for this PDS column resource. 
   *  
   * @see bytes()
   *  
   * @return int The size (i.e. number of bytes) of this PDS column.
   */  
  int PdsColumn::size() const {
    return ( bytes() );
  }
  
  
  /** 
   * Sets the number of bytes allocated for this PDS column resource to the 
   * given value. 
   *  
   * @param bytes The size (i.e. number of bytes) to be allocated for this PDS 
   *              column.
   */  
  void PdsColumn::setBytes(const int &bytes) {
    add("BYTES", toString(bytes));
    return;
  }
  
  
  /** 
   * This method returns the value for the "BYTES" keyword in this PDS column 
   * resource's PvlFlatMap. If the keyword does not exist in the map, a default 
   * of "0" will be returned. 
   *  
   * @return int The number of bytes, specified in the PvlFlatMap, for this PDS 
   *         column.
   */  
  int PdsColumn::bytes() const {
    return ( toInt(value("BYTES", "0")));
  }
  
  
  /** 
   * Determines the number of bytes to be allocated for the PDS column, given 
   * list of resources.
   *  
   * This method searches the given list for active resources that contain a 
   * keyword matching the name of this PDS column. From these, the maximum 
   * string length of the keyword values is determined. If this length is 
   * greater than the number of bytes specified in this PDS column's "BYTES" 
   * keyword, then it is returned. Otherwise, the "BYTES" value is returned.
   *  
   * @param resources A list of Resources that contain data for this PDS column.
   *  
   * @return int The number of bytes to be allocated for the column.
   */  
  int PdsColumn::bytes(ResourceList &resources) const {
    int maxSize = bytes();
    if ( 0 <= maxSize) {
      BOOST_FOREACH(SharedResource resource, resources) {
        if ( !resource->isDiscarded() ) {
          if (resource->exists(name())) {
            maxSize = std::max(maxSize, resource->value(name(), "NULL").size() );
          }
        }
      }
    }
    return (maxSize);
  }
  
  
  /** 
   * This method looks for the "DATA_TYPE" keyword in this PDS column resource's
   * PvlFlatMap. If the keyword has multiple values, only the first is returned.
   *  
   * @return QString The data type for this PDS column that is specified 
   *         in the PvlFlatMap.
   */  
  QString PdsColumn::dataType() const {
    return ( value("DATA_TYPE") );
  }
  
  
  /** 
   * Determines whether the data type for this PDS column resources is quoted. 
   * This method returns 1 for true and 0 for false.  If the DATA_TYPE keyword 
   * value contains the substring "character", then the data for this PDS 
   * column will be quoted when formated. 
   *  
   * @see dataType() 
   *  
   * @return int Indicates 1 if the data type contains the substring 
   *         "character" and 0 otherwise.
   */  
  int PdsColumn::isQuoted() const {
    QString datatype = dataType();
    if ( datatype.contains("character", Qt::CaseInsensitive) ) return (1);
    return (0);
  }
  
  
  /** 
   * Sets the start byte for this PDS column resource to the given value. 
   *  
   * @param bytes An integer indicating the location of the first byte of this 
   *              PDS column resource.
   */  
  void PdsColumn::setStartByte(const int &bytes) {
    add("START_BYTE", toString(bytes));
    return;
  }
  
  
  /** 
   * This method returns the value for the "START_BYTE" keyword in this PDS 
   * column resource's PvlFlatMap. If the keyword does not exist in the map, a 
   * default of "0" will be returned. 
   *  
   * @return int The location, specified in the PvlFlatMap, of the first byte 
   *         for this PDS column.
   */  
  int  PdsColumn::startByte() const {
    return ( toInt( value("START_BYTE", "0") ) );
  }
  
  
  /** 
   * This method returns the location of the end byte for the PDS column 
   * resource. If the PDS column resource is quoted, then the sum of 
   * the start byte and number of bytes allocated is returned. Otherwise, 
   * 1 less than the sum is returned. 
   *  
   * @see startByte() 
   * @see bytes() 
   * @see isQuoted() 
   *  
   * @return int The location of the last byte for this PDS column. 
   */  
  int PdsColumn::endByte() const {
    return ( startByte()+bytes()+isQuoted() - 1 );
  }
  
  
  /** 
   * Sets the format for this PDS column resource to the given value. The given 
   * format value will be converted to all uppercased letters. 
   *  
   * @param format A string containing the format of this PDS column resource.
   */  
  void PdsColumn::setFormat(const QString &format) {
    add("FORMAT", format.toUpper());
    return;
  }
  
  
  /** 
   * Formats the data from the given resource that corresponds to this PDS 
   * column. This method searches for the keyword in the given resource whose 
   * name matches the name of this PDS column.  If this PDS column is quoted, 
   * then the value is left justified and double quotes are added to the value 
   * before returning it. If the PDS column is not quoted, then the value is 
   * right justified and returned. If no matching keyword is not found in the 
   * resource, then the given default string is returned. 
   *  
   * @see isQuoted() 
   *  
   * @param resource A reference to a shared Resource pointer containing data 
   *                 for this PDS column.
   * @param defstring A string containing default value. This value is returned 
   *                  if the given resource does not have a keyword matching the
   *                  name of this PDS column.
   *  
   * @return QString The formatted value of the data from the given Resource for
   *         this PDS column.
   */  
  QString PdsColumn::formattedValue(SharedResource &resource,
                                      const QString &defstring) const {
  
    QString coldata = resource->value(name(), defstring); 
    int strsize = bytes();
    if ( isQuoted() ) {
       QString temp = coldata.leftJustified(strsize, ' ');
       coldata = "\"" + temp + "\"";
    }
    else {
      coldata = coldata.rightJustified(strsize, ' ');
    }
    return (coldata);
  }
  
  
#if 0
  /** 
   * @brief Clone this PDS column resource for additional specialized use.
   *  
   * This clone method will create a new PdsColumn resource with or without Assets. 
   * Keywords are full propagated.  The GisGeometry is also propagated as it can 
   * easily be reset. The discard status is reset to false.
   * 
   * @param name        A string containing the name of the new cloned resource.
   * @param withAssets  Specify to also copy out the Asset list.
   * 
   * @return Resource* A pointer to the new PDS column resource clone.
   */  
//  Resource *PdsColumn::clone(const QString &name, 
//                             const bool &withAssets) const {
//    QScopedPointer<PdsColumn> column(new PdsColumn(*this));
//    column->add("NAME", name);
//    column->setName(name);
//    if ( !withAssets ) column->clearAssets();
//    return ( column.take() ); 
//  }
#endif  
  
  /** 
   * @brief Transfer all keywords in this PDS column resource's PvlFlatMap to a 
   *        PvlObject.
   *  
   * In addition to all of the required keywords described in the isValid() 
   * method, the UNIT and FORMAT keywords will be added to the output PvlObject. 
   *  
   * @param pvlName Name of the PVL object to create and fill with map keywords
   * 
   * @return PvlObject An object with all keywords in the map
   */  
  PvlObject PdsColumn::toPvl(const QString &object) const {
    PvlObject column(object.toUpper().toStdString());
    
    column.addKeyword(PvlKeyword("COLUMN_NUMBER", value("COLUMN_NUMBER").toStdString()));
    column.addKeyword(PvlKeyword("NAME", value("NAME").toStdString()));
    column.addKeyword(PvlKeyword("DATA_TYPE", value("DATA_TYPE").toStdString()));
  
    // Check for optional parameters
    if ( exists("UNIT") ) {
      QString unit = value("UNIT");
      if ( unit.size() > 0 ) {
        column.addKeyword(PvlKeyword("UNIT", value("UNIT").toStdString())); 
      }
    }
  
    column.addKeyword(PvlKeyword("START_BYTE", value("START_BYTE").toStdString()));
    column.addKeyword(PvlKeyword("BYTES", value("BYTES").toStdString()));
  
    if ( exists("FORMAT") ) {
      column.addKeyword(PvlKeyword("FORMAT", format().toStdString())); 
    }
  
    column.addKeyword(PvlKeyword("DESCRIPTION", value("DESCRIPTION").toStdString()));
  
  
    return (column);
  }
  
  
  /** 
   * This method returns an uppercase string containing the value for the
   * "FORMAT" keyword in this PDS column resource's PvlFlatMap. This is a 
   * private method and the PvlFlatMap will throw an error if the 
   * "FORMAT" keyword does not exist in this resource. 
   *  
   * @return QString The format, specified in the PvlFlatMap, for this PDS column.
   */  
  QString PdsColumn::format() const {
    return ( value("FORMAT").toUpper() );
  }
  
  
  /** 
   * If the given format is not recognized, 0 is returned. 
   *  
   * @param fmt A string containing the format whose size is to be found.
   *  
   * @return int 
   */  
  int PdsColumn::formatSize(const QString &fmt) const {
    QRegExp rx("[AEFI]\\d+(\\.\\d+){0,1}$");
    int pos = rx.indexIn(fmt.toUpper());
    if ( pos < 0 ) { 
      return (0); 
    }
    return (toInt(rx.cap(2)));
  }
  
  /**
   * @brief Promote a resource pointer to a PdsColumn pointer 
   *  
   * This will fail if it was not derived from PdsColumn. 
   * 
   * @author 2014-11-30 Kris Becker
   * 
   * @param resource Shared pointer to a Resource (PdsColumn base class)
   * 
   * @return PdsColumn* A reinterpreted pointer to the PdsColumn
   * @throw IException::Programmer "Could not cast Resource to a PdsColumn pointer."
   */
  PdsColumn *PdsColumn::promote(SharedResource &resource) {
    PdsColumn *column = dynamic_cast<PdsColumn *> (resource.data());
    if ( 0 == column ) {
      QString mess = "Could not cast Resource [" + resource->name() +
                     "] to a PdsColumn pointer.";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }
    return (column); 
  }

}  //namespace Isis
