#ifndef PvlFlatMap_h
#define PvlFlatMap_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
// Qt library
#include <QMap>
#include <QString>
#include <QStringList>

// PvlConstraints type
#include "PvlKeyword.h"

namespace Isis {

  class FileName;
  class PvlContainer;
  class PvlGroup;
  class PvlObject;

  /**
   * This class can be used to define import/export behavior of Pvl structures 
   * when used in the PvlFlatMap class. 
   *  
   * The include/exclude features are applied to PvlObject and PvlGroup 
   * structures. 
   *  
   * The keylists apply only to keywords. 
   *  
   * Behavior beyond what is provided here is the burden of the programmer.
   *  
   * A complete Pvl structure can be provided in the PvlFlatMap class along with a 
   * defined PvlConstraints class containing objects/groups that are included or 
   * excluded from import operations.  Additionally, users can restrict the 
   * contents of all keyword containers to a list of keywords as provided in the 
   * keylist in this class.  It applies to all PVL keyword containers so ensure 
   * the list is comprehensive to include all desired keywords from all 
   * containers. 
   *  
   * Note if any include containers names are specified, all others are excluded 
   * by definition.  Likewise, if any containers are excluded, all other keywords 
   * containers are included by definition.  This behavior is defined to be 
   * constrained to the PvlObject and/or PvlGroup levels. 
   *  
   * A special mode can be defined by using both the include and exclude feature 
   * in combination. Include names are preimenent in this mode including only 
   * those PVL structures in the include list and excluding PVL structures in the 
   * exclude list. If any keywords exist in the keylist, then they are the only 
   * keywords mapped from the  PVL stuctures in the include list. 
   *  
   * Note that if a PvlGroup/PvlObject is excluded, then every PvlGroup/PvlObject 
   * contained within it are also excluded. Similarly, in order for a 
   * PvlGroup/PvlObject to be included, then any PvlObject that it is contained in 
   * must also be included. 
   *  
   * Example PVL:
   *  
   * @code 
   * Object = IsisCube
   *   Object = Core
   *     StartByte   = 65537
   *     Format      = Tile
   *     TileSamples = 128
   *     TileLines   = 128
   * 
   *     Group = Dimensions
   *       Samples = 704
   *       Lines   = 3640
   *       Bands   = 5
   *     End_Group
   * 
   *     Group = Pixels
   *       Type       = Real
   *       ByteOrder  = Lsb
   *       Base       = 0.0
   *       Multiplier = 1.0
   *     End_Group
   *   End_Object
   * 
   *   Group = Instrument
   *     SpacecraftName            = "LUNAR RECONNAISSANCE ORBITER"
   *     InstrumentId              = WAC-VIS
   *     TargetName                = Moon
   *     StartTime                 = 2009-09-15T07:27:49.230
   *   End_Group
   * 
   *   Group = Archive
   *     DataSetId            = LRO-L-LROC-2-EDR-V1.0
   *     ProducerId           = LRO_LROC_TEAM
   *     ProductId            = M107638937CE
   *   End_Group
   * 
   *   Group = BandBin
   *     FilterNumber = (3, 4, 5, 6, 7)
   *     Center       = (415, 566, 604, 643, 689)
   *   End_Group
   * 
   * End_Object
   * @endcode 
   *  
   * For the example PVL above, we can do the following: 
   *  
   * 1) Exclude the Pixels group 
   *  
   * @code 
   * PvlConstraints constraints; 
   * constraints.addExclude("Pixels"); 
   * @endcode 
   *  
   * 2) Include the Pixels group. 
   *  
   * @code 
   * PvlConstraints constraints; 
   * constraints.addInclude("IsisCube"); 
   * constraints.addInclude("Core"); 
   * constraints.addInclude("Pixels"); 
   * @endcode 
   *  
   * 3) The following will include everything in the Core object (Pixels, 
   * Dimensions, and Core keywords). 
   *  
   * @code 
   * PvlConstraints constraints; 
   * constraints.addInclude("IsisCube"); 
   * constraints.addInclude("Core"); 
   * @endcode 
   *  
   * 4) In some cases we will have a mix of includes/excludes.
   *  
   * @code 
   * PvlConstraints constraints; 
   * constraints.addInclude("IsisCube"); 
   * constraints.addInclude("Core"); 
   * constraints.addExclude("Pixels"); 
   * @endcode 
  *  
   * @author 2012-07-15 Kris Becker 
   * @internal 
   *   @history 2012-07-15 Kris Becker - Original version.
   *   @history 2015-03-20 Ian Humphrey - Updated documentation. Added
   *                           loadContainer method as helper method to
   *                           loadContainers to make exluding groups function
   *                           correctly.
  *    @history 2016-02-24 Jeannie Backer - Added unit test. Updated
  *                            documentation. Fixes #2399
   */
  class PvlConstraints {
    public:
      PvlConstraints();
      PvlConstraints(const QString &keyListFile);
      virtual ~PvlConstraints();
  
      int excludeSize() const;
      int includeSize() const;
      int keyListSize() const;
  
      void addExclude(const QString &name);
      void addInclude(const QString &name);
      void addKeyToList(const QString &name);
  
      void addExclude(const QStringList &other);
      void addInclude(const QStringList &other);
      void addKeyToList(const QStringList &other);
  
      void addKeyToList(const FileName &fileName);

      bool isExcluded(const QString &name) const;
      bool isIncluded(const QString &name) const;
      bool isKeyInList(const QString &name) const;
  
      const QStringList &excludes() const;
      const QStringList &includes() const;
      const QStringList &keyList()  const;
      
      static PvlConstraints withExcludes(const QStringList &excludes);
      static PvlConstraints withIncludes(const QStringList &includes);
  
    private:
      void readKeyListFile(const FileName &fname);

      QStringList m_excludes; //!< The names of objects and groups excluded (Excludes keyword)
      QStringList m_includes; //!< The names of objects and groups included (Includes keyword)
      QStringList m_keylist;  //!< The names of keywords to exclusively include (KeyListFile)
  
  };


  /**
   * @brief Provides a flat map of PvlKeywords.
   *  
   * This class provides a very fast convenient way to store PVL keywords for 
   * applications that require extensive use of them.  It is intended to provide a 
   * much easier interface to ISIS Pvl structures that represents them as a single 
   * flat lookup table. 
   *  
   * Note that if you use the methods provided in this class, then the keywords 
   * names will be converted to lowercase to provide a consistent interface.  One 
   * could choose to use the Qt QMap interface directly (which is the base class 
   * of this class) to use a case sensitive map.  Restricting use to add() and 
   * get() methods provides the case insensitive representation. 
   *  
   * @author 2012-07-15 Kris Becker 
   * @internal 
   *   @history 2012-07-15 Kris Becker - Original version.
   *   @history 2015-03-18 Jeannie Backer - Brought class files closer to ISIS coding standards.
   *   @history 2015-03-20 Ian Humphrey - Updated documentation. Modified loadGroups() and added 
   *                           loadGroup() method to correctly load PvlGroups against 
   *                           PvlConstraints.
   *   @history 2016-02-28 Jeannie Backer - Moved from isisminer app to base class area.
   *                           Brought closer to ISIS coding standards. Improved documentation.
   *                           Created unit test. Fixes #2399
   *   @history 2016-02-28 Jeannie Backer - Changed readKeyListFile() to read the keylist
   *                           in as an Isis::TextFile. References #2262
   */
  class PvlFlatMap : public QMap<QString, PvlKeyword> {
  
    public:
      //! A const iterator for the underling QMap that PvlFlatMap is built on.
      typedef QMap<QString,PvlKeyword>::const_iterator ConstPvlFlatMapIterator;
      //! An iterator for the underlying QMap that PvlFlatMap is built on.
      typedef QMap<QString,PvlKeyword>::iterator       PvlFlatMapIterator;
  
      PvlFlatMap();
      PvlFlatMap(const PvlFlatMap &other);
      PvlFlatMap(const PvlFlatMap &pmap1, 
                 const PvlFlatMap &pmap2);
      PvlFlatMap(const PvlObject &pvl, 
                 const PvlConstraints &constraints = PvlConstraints());
      PvlFlatMap(const PvlContainer &pvl, 
                 const PvlConstraints &constraints = PvlConstraints());
      virtual ~PvlFlatMap();
  
      bool exists(const QString &key) const;
      int  count(const QString &key) const;
      bool isNull(const QString &key, 
                  const int index = 0) const;

      void add(const QString &key, 
               const QString &value);
      void add(const PvlKeyword &keyword);
  
      void append(const PvlKeyword &key);
      void append(const QString &key, 
                  const QString &value);
  
      bool erase(const QString &key);
  
      QString get(const QString &key, 
                  const int &index = 0) const;
      QString get(const QString &key, 
                  const QString &defValue, 
                  const int &index = 0) const;
  
      // QString value(const QString &key) const;
      QString operator()(const QString &name) const;
      QStringList allValues(const QString &key) const;
  
      PvlKeyword keyword(const QString &key) const;
  
      int merge(const PvlFlatMap &other);
      
      static QStringList keywordValues(const PvlKeyword &keyword);
  
    private:
      int loadObject(const PvlObject &object, 
                     const PvlConstraints &constraints);
      int loadGroups(const PvlObject &object, 
                     const PvlConstraints &constraints);
      int loadGroup(const PvlGroup &group, 
                    const PvlConstraints &constraints);
      int loadKeywords(const PvlContainer &pvl, 
                       const PvlConstraints &constraints);
  };

} // Namespace Isis

#endif
