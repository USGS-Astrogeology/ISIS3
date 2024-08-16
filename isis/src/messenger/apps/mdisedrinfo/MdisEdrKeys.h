#ifndef MdisEdrKeys_h
#define MdisEdrKeys_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "Pvl.h"
#include "CollectorMap.h"
#include "MdisGeometry.h"
#include "OriginalLabel.h"
#include "IException.h"

namespace Isis {

  /**
   * @brief MDIS EDR keyword container class
   *
   * This class ingests a PDS EDR label and stages all keywords found in a
   * table.  Keywords can be added through the Pvl class or updated explicitly
   * by adding a PvlKeyword.
   *
   * Any add or update operation will replace existing completely so no more
   * than on keyword will exist in the table at a time.
   *
   * Keyword names are not case sensitive.  Also, any Object or Group hierarchy
   * is not honored in this class.  Meaning that if a keyword of the same name
   * exists in more than one Object or Group, only the last occurring one is
   * retained.
   *
   * @ingroup Utility
   * @author 2007-08-14 Kris Becker
   *
   * @internal
   *   @history 2008-09-22 Kris Becker - corrected formatting of units that were
   *                           missing in some cases where the first value units
   *                           is not defined.
   */
  class MdisEdrKeys {
    public:
      /** Benign constuctor */
      MdisEdrKeys() { }
      /** Constuctor adds a Pvl label to the table */
      MdisEdrKeys(Pvl &edrlab) : _edrLabel(edrlab), _keys() {
        LoadKeys(edrlab, _keys);
      }

      /**
       * @brief Add a Pvl file (label) to the table
       *
       * This constuctor will ingest a Pvl labelled file to the table.  It can
       * be any Pvl file that is supported by the Pvl class, including most all
       * PDS formatted labels.  This particular interface assumes a MDIS PDS EDR
       * label as its parameter.
       *
       * @param edrfile  File containing the PDS EDR label
       */
      MdisEdrKeys(const QString &edrfile) {
        _edrLabel = Pvl(edrfile.toStdString());
        LoadKeys(_edrLabel, _keys);
      }

      /**
       * @brief Construct from an Isis Cube file
       *
       * This constructor will ingest an ISIS cube file label.  Note that there
       * is no regard for the Object/Group structure so all keywords are
       * included and keywords that repeat in other Objects or Groups replace
       * any existing definition.
       *
       * @param cube  A valid ISIS Cube object with a label
       */
      MdisEdrKeys(Cube &cube) {
        _edrLabel = OriginalLabel(cube.fileName()).ReturnLabels();
        LoadKeys(_edrLabel, _keys);
      }

      /** Destroys the object */
      virtual ~MdisEdrKeys() { }

      /**
       * @brief Returns the number of keywords in the container
       *
       * @return int Number of keys in container
       */
      inline int size() const {
        return _keys.size();
      }

      /**
       * @brief Adds a keyword to the container directly
       *
       * This method is provided for adding keywords directly to the contained
       * that may not be contained within a Pvl object.
       *
       * @param key PvlKeyword to add to the container
       * @param name  Optional name other than what the keyword is named
       */
      void add(const PvlKeyword &key, const QString &name = "") {
        if(name.isEmpty()) {
          _keys.add(key.name(), key);
        }
        else {
          _keys.add(name, key);
        }
        return;
      }

      /**
       * @brief Returns the nth keyword in the container
       *
       * This method allows one to retrieve the nth keyword in the container as
       * a means to iterate through the complete contents for direct
       * interogation/use.
       *
       * If the keyword at specified index does not exist, an exception is
       * thrown.
       *
       * @param index Index of the keyword ranging from 0 to size()-1
       *
       * @return const PvlKeyword&  Returns a reference to the keyword at index
       */
      const PvlKeyword &operator[](const int index) const {
        return (_keys.getNth(index));
      }

      /**
       * @brief Return the specified keyword by name
       *
       * Retrieves a keyword by name without regard for case.  An exception is
       * thrown if it does not exist.
       *
       * @param name  Name of keyword to retrieve
       *
       * @return PvlKeyword& Reference to named keyword
       */
      PvlKeyword &get(const QString &name) {
        return (_keys.get(name));
      }

      /**
       * @brief Returns the specified keyword by name
       *
       * Retrieves a keyword by name without regard for case.  An exception is
       * thrown if it does not exist.
       *
       * @param name  Name of keyword to retrieve
       *
       * @return const PvlKeyword& Returns a const reference to the named
       *         keyword
       */
      const PvlKeyword &get(const QString &name) const {
        return (_keys.get(name));
      }

      /**
       * @brief Replaces or adds keywords in a Pvl class
       *
       * This method will add all the keywords found in the Pvl keys object to
       * the container.  Any keywords that already exist in the container are
       * replaced with the new one.
       *
       * @param keys Pvl class containing the keywords to add
       */
      void updateKeys(Pvl &keys) {
        LoadKeys(keys, _keys);
      }

      /**
       * @brief Return a formatted list of keyword values
       *
       * This method takes in a list of named keywords and formats them
       * according to PDS standards in a comma delineated string.  This is the
       * defined format of the team for its population of PDS EDR label
       * keywords.
       *
       * If any one of the keywords is not found, an exception is thrown.
       *
       * @param keylist List of keywords to extract
       * @param NAstr String used to specify no value (used mainly so a unit
       *              qualifer is not added to the value)
       * @return QString Returns a comma delineated string of keyword values
       */
      QString extract(const QStringList &keylist,
                      const QString &NAstr, PvlGroup *group = 0) {
        std::ostringstream out;

        QString loopSep("");
        int nbad(0);
        IException errors;
        for(int i = 0 ; i < keylist.size() ; i++) {
          QString keyname(keylist[i]);
          keyname = keyname.trimmed();
          try {
            PvlKeyword &key = _keys.get(keyname);
            if(group) group->addKeyword(key);
            if((key.size() == 0) || (key.isNull())) {
              out << loopSep << "NULL";
            }
            else if(key.size() == 1) {
              out << loopSep << key[0] << formatUnit(QString::fromStdString(key.unit(0)));
            }
            else {
              out << loopSep << "(";
              QString vsep("");
              for(int iv = 0 ; iv < key.size() ; iv++) {
                out << vsep << key[iv];
                if(QString::fromStdString(key[iv]) != NAstr) {
                  out << formatUnit(QString::fromStdString(key.unit(iv)));
                }
                vsep = ",";
              }
              out << ")";
            }
            loopSep = ";";
          }
          catch(IException &ie) {
            nbad++;
            QString mess = "Keyword \"" + keyname + "\" does not exist!";
            errors.append(
                IException(IException::User, mess, _FILEINFO_));
          }
        }

        // Check to see if all keywords are found
        if(nbad > 0) {
          QString mess = "One or more keywords in list do not exist!";
          throw IException(errors, IException::User, mess, _FILEINFO_);
        }

        return (out.str().c_str());
      }

    private:
//!< Define the keyword container for systematic processing
      typedef CollectorMap<IString, PvlKeyword, NoCaseStringCompare> KeyList;
      Pvl     _edrLabel;  //!< Label used to populate the container
      KeyList _keys;      //!<  The container

      /**
       * @brief Parse the contents of a (generic) container
       *
       * This method iterates through all keywords in the PvlContainer and adds
       * them KeyList.
       *
       * @param p  Pvl container of keywords
       * @param keys Container to receive the keywords
       */
      void MapKeys(PvlContainer &p, KeyList &keys,
                   const QString prefix = "") {
        QString prekey(prefix);
        if(!prefix.isEmpty()) prekey += "/";
        PvlContainer::PvlKeywordIterator keyIter = p.begin();
        for(; keyIter != p.end() ;  ++keyIter) {
          QString keyname = prefix + QString::fromStdString(keyIter->name());
          PvlKeyword key = *keyIter;
          key.setName(keyname.toStdString());
          keys.add(keyname, key);
        }
        return;
      }

      /**
       * @brief Adds keywords in groups within an object
       *
       * This metod iterates through all the keywords in the given group and
       * addes them to the keys container.
       *
       * @param obj  Object containing Groups to add to container
       * @param keys  Container to add the group keywords to
       */
      void LoadGroups(PvlObject &obj, KeyList &keys) {
        PvlObject::PvlGroupIterator current = obj.beginGroup();
        for(; current != obj.endGroup() ; ++current) {
          MapKeys(*current, keys);
        }
        return;
      }

      /**
       * @brief Add object keywords and all keywords in the Object hierachy
       *
       * This method adds all keywords found in the Object and then any Groups
       * that exist in the object.
       *
       * @param Object to extract keywords from and add to container
       * @param keys The container to add keywords to
       */
      void LoadKeys(PvlObject &obj, KeyList &keys,
                    const QString prefix = "") {
        MapKeys(obj, keys, prefix);   // Load object level keywords
        LoadGroups(obj, keys);  // Load all groups in this object

        //  Now load all the rest of the object keywords ingnoring
        //  all SUBFRAME[12345]_PARAMETERS since they are unsupported.
        PvlObject::PvlObjectIterator objIter = obj.beginObject();
        for(; objIter != obj.endObject() ; ++objIter) {
          QString objname(QString::fromStdString(objIter->name()));
          objname = objname.toUpper();
          int gotSubframe = objname.indexOf("SUBFRAME");
          if(gotSubframe != -1) {
            LoadKeys(*objIter, keys, QString::fromStdString(objIter->name()));
          }
          else {
            LoadKeys(*objIter, keys);
          }
        }
        return;
      }

      /**
       * @brief Returns a properly formated unit
       *
       *
       * @param unit value of unit.  If empty returns empty string otherwise
       *             places "<" and ">" on the ends.
       *
       * @return QString  The formatted unit
       */
      QString formatUnit(const QString &unit) const {
        if(!unit.isEmpty()) {
          return (" <" + unit + ">");
        }
        return (unit);
      }
  };

}     // namespace Isis
#endif
