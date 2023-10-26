/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Resource.h"

// Qt library
#include <QSharedPointer>
#include <QString>

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "PvlContainer.h"
#include "PvlFlatMap.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {

  /**
   * Default constructor for a Resource object. The name is set to "Resource" and discard
   * is set to false. The keyword and asset lists are left empty. 
   */
  Resource::Resource(): m_data( new ResourceData("Resource") ), 
                        m_discard( false ) { 
    setName( m_data->m_name );
  }


  /**
   * Constructs a Resource object with the given name. Discard is set to false.
   * The keyword and asset lists are left empty.
   *
   * @param name  A string containing the resource name.
   */
  Resource::Resource(const QString &name) : m_data( new ResourceData(name) ), 
                                            m_discard( false ) { 
    setName(name);
  }


  /**
   * Constructs a Resource object with the given name and profile of keywords.
   * Discard is set to false. The asset list is left empty.
   *
   * @param name     A string containing the resource name.
   * @param profile  A map between strings and PVL keywords.
   */
  Resource::Resource(const QString &name, const PvlFlatMap &profile) : 
                     m_data( new ResourceData(name, profile) ),
                     m_discard(false) { 
    setName(name);
  }


  /**
   * Constructs a Resource from another Resource 
   *
   * @param other    Resource to copy
   */
  Resource::Resource(const Resource &other) : m_data( other.m_data ),
                                              m_discard( other.m_discard ) {
  }


  /**
   * Constructs a Resource from another Resource with options to make it a deep 
   * or shallow copy of the data 
   *
   * @param other    Resource to copy
   * @param deepcopy A deep copy of the data is made if true, otherwise a 
   *                 shallow copy of the data is performed
   */
  Resource::Resource(const Resource &other, const bool deepcopy) :
                     m_data( new ResourceData( *other.m_data ) ),
                     m_discard( other.m_discard ) { 
    if ( deepcopy ) { m_data.detach(); }
  }


  /**
   * Constructs a Resource object with the given name and profile of keywords.
   * Discard is set to false. The asset list is left empty.
   *
   * @param name     A string containing the resource name.
   * @param profile  A PvlContainer of keywords.
   */
  Resource::Resource(const QString &name, const PvlContainer &profile) :
                     m_data( new ResourceData(name, PvlFlatMap(profile) ) ),
                     m_discard(false) {
    setName(name);
  }


  /**
   * Destroys the Resource object.
   */
  Resource::~Resource() {
  }


  /**
   * Accessor for a string containing the Resource's name.
   *
   * @return QString  The name of this resource.
   */
  QString Resource::name() const {
    return ( m_data->m_name );
  }


  /**
   * A mutator to set the Resource's name.
   *
   * @param identity  A string containing the name of this Resource.
   */
  void Resource::setName(const QString &identity) {
    m_data->m_name = identity;
    add("Identity", identity);
    return;
  }


/**
 * @brief Checks for equality of another Resource. 
 *
 * This is nothing more than comparing the names of the two resources. If both 
 * resources have the same name without regard to case, they are considered the 
 * same resource. 
 *
 * @param other Resource to check for equality
 *
 * @return bool True if names are the same, false otherwise
 */
  bool Resource::isEqual(const Resource &other) const {
    return ( name().toLower() == other.name().toLower() );
  }


  /**
   * Determines whether a PVL keyword with the given name is in this Resource.
   *
   * @param keywordName  A string containing the keyword name.
   *
   * @return bool  Indicates whether a PVL keyword in this Resource's PvlFlatMap is
   *               mapped to the given name.
   */
  bool Resource::exists(const QString &keywordName) const {
    return ( m_data->m_keys.exists(keywordName) );
  }


  /**
   * Counts the number of values the PVL keyword with the given name has, if it exists
   * in this Resource. Otherwise, it returns 0.
   *
   * @param keywordName  A string containing the keyword name.
   *
   * @return int  The size of the PvlKeyword object mapped to the given name in
   *               this Resource's PvlFlatMap.
   */
  int Resource::count(const QString &keywordName) const {
    return ( m_data->m_keys.count(keywordName) );
  }


  /**
   * @brief Determines whether the PVL keyword with the given name at the given index is null.
   *
   * This method looks for the PVL keyword in this Resource's PvlFlatMap that is mapped to the
   * given name and checks whether the value at the given index is null. If no index is given, by
   * default this method checks the first value (i.e. index 0). If the keyword does not exist in
   * the map, this method returns true.
   *
   * @param keywordName  Name of the PVL keyword whose values will be accessed.
   * @param index        Zero-based index for the value to be checked.
   *
   * @return bool  Indicates whether the given PVL keyword is null at the given index.
   */
  bool Resource::isNull(const QString &keywordName, const int index) const {
    return ( m_data->m_keys.isNull(keywordName, index) );
  }


  /**
   * Accessor method for the PVL keywords associated with this Resource. This method
   * gets the PvlFlatMap, a map between a name and PvlKeyword objects.
   *
   * @return const PvlFlatMap&  A reference to the map of PVL keywords.
   */
  const PvlFlatMap &Resource::keys() const {
    return ( m_data->m_keys );
  }


  /**
   * @brief Gets the value of the PVL keyword with the given name at the given index.
   *
   * This method looks for the PVL keyword in this Resource's PvlFlatMap that is mapped to the
   * given name and accesses the value at the given index. If no index is given, by default
   * this method returns the first value (i.e. index 0). 
   *
   * Note: If the keyword does not exist in the map, PvlFlatMap throws an exception. To avoid this,
   *       the isNull() method can be called before invoking this method to verify whether a value
   *       exists at the given index. Otherwise, the value(QString, QString, int) version of this
   *       overloaded method can be called.
   *
   * @see isNull(QString, int)
   * @see value(QString, QString, int)
   *
   * @param keywordName  A string containing the name of the PVL keyword in this Resource's
   *                     list of keywords.
   * @param index        Zero-based index for the value to be accessed.
   *
   * @return QString  A string containing the PVL keyword value at the given index. 
   */
  QString Resource::value(const QString &keywordName, const int &index) const {
    return ( m_data->m_keys.get(keywordName, index) );
  }


  /**
   * @brief Gets the value of the PVL keyword at the given index, if found; otherwise it returns
   *        the given default value.
   *
   * This method looks for the PVL keyword in this Resource's PvlFlatMap that is mapped to the
   * given name and accesses the value at the given index. If no index is given, by default
   * this method returns the first value (i.e. index 0).
   *
   * If the keyword does not exist in the map, the given default value will be returned.
   *
   * @see isNull(QString, int)
   * @see value(QString, int)
   *
   * @param keywordName   A string containing the name of the PVL keyword in this Resource's
   *                      list of keywords.
   * @param defaultValue  A string containing the default value for this keyword if not found in
   *                      the PvlFlatMap.
   * @param index         Zero-based index for the value to be accessed.
   *
   * @return QString  A string containing the PVL keyword value at the given index, if it exists; 
   *                  otherwise the given default value is returned.
   */
  QString Resource::value(const QString &keywordName, const QString &defaultValue,
                          const int &index) const {
    QString keywordValue(defaultValue);
    if ( !isNull(keywordName, index) ) {
      keywordValue = value(keywordName, index);
    }
    return (keywordValue);
  }


  /**
   * Gets the PvlKeyword object with the given name, if it exists in this Resource.
   * If not, an empty PvlKeyword with the given name is returned.
   *
   * @param keywordName  A string containing the name of the PVL keyword to be accessed.
   *
   * @return PvlKeyword  The keyword mapped to the given name in this Resource's PvlFlatMap,
   *                     if it exists. Otherwise an empty PvlKeyword with the given name.
   */
  PvlKeyword Resource::keyword(const QString &keywordName) const {
    if ( m_data->m_keys.exists(keywordName) ) {
      return ( m_data->m_keys.keyword(keywordName) );
    }
    // Return empty keyword
    return (PvlKeyword(keywordName.toStdString()));
  }


  /**
   * Adds a PVL keyword with the given name and value to this Resource. A PvlKeyword object
   * is created and added to the PvlFlatMap belonging to this Resource.
   *
   * @param keywordName   A string containing the name of the keyword to be added.
   * @param keywordValue  A string containing the value of the keyword to be added.
   */
  void Resource::add(const QString &keywordName, const QString &keywordValue) {
     m_data->m_keys.add(keywordName, keywordValue);
     return;
  }


  /**
   * Adds the given PVL keyword to this Resource. The PvlKeyword object is added to
   * the PvlFlatMap belonging to this Resource.
   *
   * @param keyword  A reference to the PvlKeyword object to be added.
   */
  void Resource::add(const PvlKeyword &keyword) {
    m_data->m_keys.add(keyword);
  }


  /**
   * Adds the PVL keywords from the given map of keywords to this Resource.
   * The new keywords are inserted into this Resource's existing PvlFlatMap.
   *
   * @param keys  A PvlFlatMap containing the PvlKeywords to be inserted into the existing
   *              keyword list.
   */
  void Resource::add(const PvlFlatMap &keys) {
    BOOST_FOREACH ( PvlKeyword keyword, keys ) {
      add(keyword);
    }
    return;
  }


  /**
   * Appends the given value to the PVL keyword with the given name.
   *
   * If the keyword already exists in the PvlFlatMap, the provided value is appended to the
   * keyword's list of values. Otherwise, a new PVL keyword with the given name and value is added.
   *
   * @param keywordName   A string containing the keyword name.
   * @param keywordValue  The value to be appended to the keyword associated with the given name.
   */
  void Resource::append(const QString &keywordName, const QString &keywordValue) {
    m_data->m_keys.append(keywordName, keywordValue);
    return;
  }


  /**
   * Removes all of the PVL keywords in this Resource that are associated with the given name.
   *
   * @param keywordName  The name mapped to the PVL keywords to be removed from
   *                     this Resource's PvlFlatMap.
   *
   * @return int  The number of PVL keywords removed from this Resource's PvlFlatMap.
   */
  int Resource::erase(const QString &keywordName) {
    return ( m_data->m_keys.erase(keywordName) );
  }


  /**
   * Sets the GIS geometry for this Resource to the given value.
   *
   * Note this Resource then takes control of this pointer. Therefore, there
   * should be no other references as it is turned into a shared pointer.
   *
   * @see add(SharedGisGeometry &geom)
   *
   * @param geom  A pointer to the GisGeometry to be added to this Resource.
   */
  void Resource::add(GisGeometry *geom) {
    m_data->m_geom = SharedGisGeometry(geom);
    return;
  }


  /**
   * Sets the GIS geometry for this Resource to the given value.
   *
   * @param geom  A shared pointer to the GisGeometry to be added to this Resource.
   */
  void Resource::add(SharedGisGeometry &geom) {
    m_data->m_geom = geom;
    return;
  }


  /**
   * This method is used to determine whether the GIS geometry has been set for this Resource.
   *
   * @return bool  Indicates whether the GIS geometry has been set.
   */
  bool Resource::hasGeometry() const {
    return ( !m_data->m_geom.isNull() );
  }


  /**
   * This method is used to determine whether a valid GIS geometry has been set for this Resource.
   * If no geometry has been set or the geometry is empty, false is returned.
   *
   * @return bool  Indicates whether the GIS geometry is valid.
   */
  bool Resource::hasValidGeometry() const {
    if ( hasGeometry() ) {
      return ( !m_data->m_geom->isEmpty() );
    }

    return (false);
  }


  /**
   * Accessor method for this Resource's GIS geometry.
   *
   * @return SharedGisGeometry  A shared pointer to the GIS geometry associated with this Resource.
   */
  SharedGisGeometry Resource::geometry() const {
    return ( m_data->m_geom );
  }


  /**
   * Activate a resource.
   */
  void Resource::activate() {
    m_discard = false;
  }


  /**
   * Accessor method to determine whether this Resource is to be discarded.
   *
   * @return bool  Indicates whether to discard this Resource.
   */
  bool Resource::isActive() const {
    return ( !m_discard );
  }


  /**
   *  Discard a resource.
   */
  void Resource::discard() {
    m_discard = true;
  }


  /**
   * Accessor method to determine whether this Resource is to be discarded.
   *
   * @return bool  Indicates whether to discard this Resource.
   */
  bool Resource::isDiscarded() const {
    return ( m_discard );
  }


  /**
   * This method is used to determine whether an asset with the given name is in this Resource.
   *
   * @param assetName  A string containing the asset name.
   *
   * @return bool  Indicates whether an asset in this Resource's VariantList is
   *               mapped to the given name.
   */
  bool Resource::hasAsset(const QString &assetName) const {
    return ( m_data->m_assets.contains(assetName.toLower()) );
  }


  /**
   * Inserts an asset with the given name and value into this Resource's VariantList.
   *
   * @param assetName   A string containing the name of the asset to be added.
   * @param assetValue  A QVariant containing the value of the asset to be added.
   */
  void Resource::addAsset(const QString &assetName, QVariant &assetValue) {
    m_data->m_assets.insert(assetName.toLower(), assetValue);
    return;
  }


  /**
   * Removes all of the assets in this Resource that are mapped to the given name. 
   *
   * @param assetName  The name mapped to the assets to be removed from
   *                   this Resource's VariantList.
   *
   * @return int  The number of assets removed from this Resource's VariantList.
   */
  int Resource::removeAsset(const QString &assetName) {
    return ( m_data->m_assets.remove(assetName.toLower()) );
  }


  /**
   * Clears the assets from this Resource's VariantList.
   *
   * @return int  The number of assets that were in this Resource's VariantList before clearing it.
   */
  int Resource::clearAssets() {
    int n = m_data->m_assets.size();
    m_data->m_assets.clear();
    return (n);
  }


  /**
   * Retrieves the value of the asset in this Resource that is mapped to the given name.
   *
   * @param assetName  A string containing the name of the asset to be accessed.
   *
   * @return QVariant  The value of the asset that is mapped to the given name in
   *                   this Resource's VariantList map.
   *
   * @throw IException::Programmer "Requested asset does not exist."
   */
  QVariant Resource::asset(const QString &assetName) const {
    if ( !hasAsset(assetName) ) {
      throw IException(IException::Programmer,
                        "Requested asset " + assetName + " does not exist.", 
                       _FILEINFO_);
    }
    return ( m_data->m_assets.value(assetName.toLower()) );
  }


  /**
   * @brief Copy this resource for distinct management of its status
   *
   * This copy method will create a copy of an existing Resource and retain all
   * its current data and status. 
   *
   * Use this method when a shallow (shared data) copy of the Resource is 
   * desired so that its active status can be managed with copy of the same 
   * data source. 
   *
   * @return Resource* Pointer to new resource
   */
  Resource *Resource::copy() const {
    Resource *resource = new Resource(*this, false);
    resource->m_discard = m_discard;
    return (resource); 
  }


  /**
   * @brief Clone this resource for additional specialized use.
   *
   * This clone method will create a new Resource with or without Assets. 
   * Keywords are fully propagated.  The GisGeometry is also propagated as it 
   * can easily be reset. The resource status is activated. 
   *
   * Use this method when a deep copy of the Resource data is desired so that 
   * it can vary independently of its parent. 
   *
   * @param name        A string containing the name of the new cloned resource.
   * @param withAssets  Specify to also copy out the Asset list.
   *
   * @return Resource* Pointer to new resource
   */
  Resource *Resource::clone(const QString &name, const bool &withAssets) const {
    Resource *resource = new Resource(*this, true);
    if ( !withAssets ) { resource->clearAssets(); }
    resource->activate();
    return (resource);
  }


  /**
   * @brief Transfer all keywords in map to a PvlObject.
   *
   * Derived classes of Resource can define their own implementation if 
   * propagation of all keywords is not acceptable. 
   *
   * @param pvlName Name of the PVL object to create and fill with map keywords
   *
   * @return PvlObject An object with all keywords in the map
   */
  PvlObject Resource::toPvl(const QString &pvlName) const {
    PvlObject object(pvlName.toStdString());
    PvlFlatMap::ConstPvlFlatMapIterator key = m_data->m_keys.begin();
    while (  key != m_data->m_keys.end() ) {
      object.addKeyword(key.value());
      ++key;
    }
    return ( object );
  }

}  //namespace Isis
