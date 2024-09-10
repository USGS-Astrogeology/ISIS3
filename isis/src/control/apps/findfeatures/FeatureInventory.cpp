/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include <boost/foreach.hpp>

#include <QRegularExpression>
#include <QStringList>
#include <QSet>

#include "opencv2/opencv.hpp"
#include "opencv2/xfeatures2d.hpp"

#include "FeatureInventory.h"

namespace Isis {
  FeatureInventory::FeatureInventory() : m_theFeatureCreator(),
                                         m_theMatcherCreator()  { }


  FeatureInventory::~FeatureInventory() { }


  int FeatureInventory::add(const QString &name,
                             FeatureInventory::FeatureCreator maker,
                             const QStringList &aliases) {

    // Add the fundamental
    int v_made(0);
    // There cannot be withspace at the begging or end of the name - its okay
    // to embed them.
    QString v_name = name.toLower().trimmed();
    m_theFeatureCreator.insert(v_name, maker);
    v_made++;

    // Add all aliases. Expected to of form "detector.name, extractor.name",
    // etc....
    BOOST_FOREACH ( QString a_name, aliases ) {
      if ( !a_name.isEmpty() ) {
       QString l_name = a_name.toLower().trimmed();
       m_theFeatureCreator.insert(l_name, maker);
       v_made++;
       if ( l_name.contains(QRegularExpression("^detector\\.*",
                                               QRegularExpression::CaseInsensitiveOption) ) ) {
         m_detectorNames.append( l_name.toLower() );
       }
       if ( a_name.contains(QRegularExpression("^extractor\\.*",
                                               QRegularExpression::CaseInsensitiveOption) ) ) {
         m_extractorNames.append( l_name.toLower() );
       }
      }
    }

    return ( v_made );
  }


  int FeatureInventory::add(const QString &name,
                             FeatureInventory::MatcherCreator maker,
                             const QStringList &aliases) {
    int v_made(0);
    // There cannot be whitespace at the begining or end of the name - its okay
    // to embed them.
    QString v_name = name.toLower().trimmed();
    m_theMatcherCreator.insert(v_name, maker);
    m_matcherNames.append( name.toLower() );
    v_made++;

    // Add all aliases. Expected to of form "detector.name, extractor.name",
    // etc....
    BOOST_FOREACH ( QString a_name, aliases ) {
      QString l_name = a_name.toLower().trimmed();
      if ( !l_name.isEmpty() ) {
        m_theMatcherCreator.insert(l_name, maker);
      }
      v_made++;
    }

    return ( v_made );
  }


  FeatureAlgorithmPtr FeatureInventory::getFeature(const QString &config) const {
    QStringList parts = parse(config, "@");

    if ( parts.isEmpty() ) {
      std::string mess = "No config string provided in FeatureInventory::getFeature";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    QString name = parts.takeFirst().toLower().trimmed();
    PvlFlatMap variables = parameters(parts);

    if ( m_theFeatureCreator.contains(name) ) {
      FeatureCreator creator = m_theFeatureCreator.value(name);
      return ( creator( variables, config) );
    }
    else {
      std::string mess = name.toStdString() + "Feature2D not found or invalid";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    // Shouldn't get here but just in case
    return ( FeatureAlgorithmPtr() );

  }


  FeatureAlgorithmPtr FeatureInventory::getDetector(const QString &config) const {
    FeatureAlgorithmPtr algo = getFeature(config);
    if ( !algo->hasDetector() ) {
      std::string mess = "Specification does not define a detector:\n" + config.toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
    return ( algo );
  }


  FeatureAlgorithmPtr FeatureInventory::getExtractor(const QString &config) const {
    FeatureAlgorithmPtr algo = getFeature(config);
    if ( !algo->hasExtractor() ) {
      std::string mess = "Specification does not define an extractor:\n" + config.toStdString();
      throw IException(IException::User, mess, _FILEINFO_);
    }
    return ( algo );
  }


  MatcherAlgorithmPtr FeatureInventory::getMatcher(const QString &config) const {
    QStringList parts = parse(config, "@");

    if ( parts.isEmpty() ) {
      std::string mess = "No config string provided in FeatureInventory::getMatcher";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    QString c_name = parts.takeFirst();
    QString name = c_name.toLower().trimmed();
    PvlFlatMap variables = parameters(parts);

    if ( m_theMatcherCreator.contains(name) ) {
      MatcherCreator matcher = m_theMatcherCreator.value(name);
      return ( matcher(variables, config));
    }
    else {
      std::string mess = c_name.toStdString() + " Matcher not found or invalid";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    // Shouldn't get here but just in case
    return ( MatcherAlgorithmPtr() );

  }


  PvlObject FeatureInventory::info(const QString &name) const {
    return ( info( allNames(), name ) );
  }


  PvlObject FeatureInventory::info(const QStringList algorithms,
                                   const QString &name) const {
    PvlObject algos(name.toStdString());

    BOOST_FOREACH ( QString algorithmName, algorithms ) {
      algos += algorithmInfo( algorithmName );
    }

    return ( algos );
  }


  PvlObject FeatureInventory::algorithmInfo(const QString algorithmName) const {
    PvlObject algorithmObject(algorithmName.toStdString());
    QString lowerName = algorithmName.toLower().trimmed();

    try {
      if ( m_theFeatureCreator.contains(lowerName) ) {
        algorithmObject = getFeature(lowerName)->info();
      }
      else if ( m_theMatcherCreator.contains(lowerName) ) {
        algorithmObject = getMatcher(lowerName)->info();
      }
      else {
        std::string mess = "Algorithm [" + algorithmName.toStdString() +
                      "] is not a supported OpenCV3 algorithm.";
        throw IException(IException::User, mess, _FILEINFO_);
      }

      algorithmObject += aliases( lowerName );
    }
    catch (IException &e) {
      algorithmObject.clear();
      algorithmObject += PvlKeyword( "Error", e.toString() );
    }

    return ( algorithmObject );
  }


  PvlKeyword FeatureInventory::aliases(const QString algorithmName) const {
    PvlKeyword aliasKey("Aliases");
    QString lowerName = algorithmName.toLower().trimmed();

    if ( m_theFeatureCreator.contains( lowerName ) ) {
      FeatureCreator creator = m_theFeatureCreator.value(lowerName);
      QStringList aliasList = m_theFeatureCreator.keys(creator);
      BOOST_FOREACH ( QString alias, aliasList) {
        aliasKey += alias.toStdString();
      }
    }
    else if ( m_theMatcherCreator.contains( lowerName ) ) {
      MatcherCreator creator = m_theMatcherCreator.value(lowerName);
      QStringList aliasList = m_theMatcherCreator.keys(creator);
      BOOST_FOREACH ( QString alias, aliasList) {
        aliasKey += alias.toStdString();
      }
    }
    else {
      std::string mess = "Algorithm [" + algorithmName.toStdString() +
                     "] is not a supported OpenCV3 algorithm.";
      throw IException(IException::Programmer, mess, _FILEINFO_);
    }

    return ( aliasKey );
  }


  QStringList FeatureInventory::detectorNames() const {
    return ( m_detectorNames );
  }


  QStringList FeatureInventory::extractorNames() const {
    return ( m_extractorNames );
  }


  QStringList FeatureInventory::feature2DNames() const {
    QSet<QString> nameSet(m_detectorNames.begin(), m_detectorNames.end());
    QSet<QString> extractorSet(m_extractorNames.begin(), m_extractorNames.end());
    nameSet.intersect(extractorSet);
    QStringList nameList(nameSet.begin(), nameSet.end());

    return ( nameList );
  }


  QStringList FeatureInventory::matcherNames() const {
    return ( m_matcherNames );
  }


  QStringList FeatureInventory::allNames() const {
    QStringList fullList = m_detectorNames;
    fullList.append(m_extractorNames);
    fullList.append(m_matcherNames);
    fullList.removeDuplicates();

    return ( fullList );
  }


  /** Split the string with the provided separator in individual parts   */
   QStringList FeatureInventory::parse(const QString &config,
                                       const QString &sep) const {
     return ( config.split(sep, Qt::SkipEmptyParts) );
   }

   PvlFlatMap FeatureInventory::parameters(const QStringList &fromConfig) const {
     PvlFlatMap parms;
     BOOST_FOREACH ( QString parm, fromConfig) {
       QStringList parts = parse(parm, ":");
       if ( parts.size() > 0 ) {
         PvlKeyword key(parts.takeFirst().trimmed().toStdString());
         BOOST_FOREACH  (QString value, parts) {
           key.addValue(value.toStdString());
         }
         parms.add(key);
       }
     }

     return ( parms );
   }

};
