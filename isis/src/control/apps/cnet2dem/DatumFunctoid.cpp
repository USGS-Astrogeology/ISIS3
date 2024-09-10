/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <sstream>

#include <QtGlobal>
#include <QCoreApplication>
#include <QScopedPointer>
#include <QString>
#include <QStringList>

#include <boost/foreach.hpp>

#include "DatumFunctoid.h"
#include "FileName.h"
#include "IString.h"
#include "IException.h"
#include "PvlKeyword.h"
#include "PvlObject.h"

using namespace std;

namespace Isis {

DatumFunctoidFactory *DatumFunctoidFactory::m_maker = 0;

DatumFunctoidFactory::DatumFunctoidFactory() {
//  This ensures this singleton is shut down when the application exists
   qAddPostRoutine(DieAtExit);
   init();
   return;
}

DatumFunctoidFactory::~DatumFunctoidFactory() { }

DatumFunctoidFactory *DatumFunctoidFactory::getInstance() {
  if (!m_maker) {
    m_maker = new DatumFunctoidFactory();
  }
  return (m_maker);
}



QStringList DatumFunctoidFactory::algorithms() const {
  QStringList functoids;

  BOOST_FOREACH ( SharedDatumFunctoid d, m_functoids ) {
    functoids.push_back( d->name() );
  }
  return (functoids);
}

DatumFunctoidList DatumFunctoidFactory::create(const QString &specs,
                                               const bool &errorIfEmpty)
                                               const {
  DatumFunctoidList functoids;

  // Individual algorithm specifications are separated by vertical bars
  QStringList algorithms = specs.split(",", Qt::SkipEmptyParts);
  // cout << "create: got " << algorithms.size() << " algorithms...\n";
  if ( algorithms.size() == 0 ) {
    if ( errorIfEmpty ) {
      std::string mess = "No functoid algorithm specifictions found!";
      throw IException(IException::User, mess, _FILEINFO_);
    }
    return ( functoids );
  }

  IException ie;
  int nerrors(0);
  BOOST_FOREACH ( QString functoid, algorithms ) {
    SharedDatumFunctoid f( make(functoid) );
    if ( f.isNull() ) {
      std::string mess = "Algorithm \"" + functoid.toStdString() + "\" was not found!";
      ie.append(IException(IException::User, mess, _FILEINFO_));
      nerrors++;
    }
    else {
      functoids.append(f);
    }
  }

  // Toss errors if they occurred
  if ( nerrors > 0 ) { throw ie; }

  // return the list
  return ( functoids );
}


DatumFunctoid *DatumFunctoidFactory::make(const QString &funcspec) const {
  PvlFlatMap parms = parseParameters(funcspec);
  QString algorithm = parms.get("Functoid","").toLower();
  BOOST_FOREACH ( SharedDatumFunctoid d, m_functoids ) {
    if ( d->name().toLower() == algorithm ) {
      return ( d->clone(parms) );
    }
  }

  // Did not find a functoid
  return ( 0 );
}

/**
 * @brief Parse a parameter string for values and return in parameter map
 *
 * @author 2015-09-08 Kris Becker
 *
 * @param parameters
 *
 * @return PvlFlatMap
 */
PvlFlatMap DatumFunctoidFactory::parseParameters(const QString &parameters)
                                                    const {
  PvlFlatMap pmap;

  // If the string is empty, return an empty paramter list
  QStringList parts = parameters.split("@");
  if ( parts.size() == 0 ) { return (pmap); }

  // Pull the first value and name it the Functoid
  QString parmtag = parts.takeFirst();
  pmap.add("Functoid", parmtag);

  // All good so far, parse each parameter string
  BOOST_FOREACH ( QString specs, parts ) {

    // Is it valid?
    QStringList parmvaltag = specs.split(":");
    if ( 2 != parmvaltag.size() ) {
      std::string mess = "Invalid parameter at or near [" + specs.toStdString() + "] in \"" +
                     parameters.toStdString() + "\" - must be of the form "
                     "\"name@parm1:value1@parm2:value2...\"";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    // Looks good, set the parameters
    pmap.add(parmvaltag[0], parmvaltag[1]);
  }

  // All done...
  return ( pmap );
}


 /**
   * @brief Exit termination routine
   *
   * This (static) method ensure this object is destroyed when Qt exits.
   *
   * Note that it is error to add this to the system _atexit() routine because
   * this object utilizes Qt classes.  At the time the atexit call stack is
   * executed, Qt is long gone resulting in Very Bad Things.  Fortunately, Qt has
   * an exit stack function as well.  This method is added to the Qt exit call
   * stack.
   */
  void DatumFunctoidFactory::DieAtExit() {
    delete  m_maker;
    m_maker = 0;
    return;
  }


void DatumFunctoidFactory::init() {
  m_functoids.clear();

  m_functoids.append( SharedDatumFunctoid(new AverageRadius()) );
  m_functoids.append( SharedDatumFunctoid(new MedianRadius()) );
  m_functoids.append( SharedDatumFunctoid(new StandardDeviationRadius()) );
  m_functoids.append( SharedDatumFunctoid(new MaximumRadius()) );
  m_functoids.append( SharedDatumFunctoid(new MinimumRadius()) );
  m_functoids.append( SharedDatumFunctoid(new AverageDistance()) );
  m_functoids.append( SharedDatumFunctoid(new MedianDistance()) );
  m_functoids.append( SharedDatumFunctoid(new StandardDeviationDistance()) );
  m_functoids.append( SharedDatumFunctoid(new MaximumDistance()) );
  m_functoids.append( SharedDatumFunctoid(new MinimumDistance()) );
  m_functoids.append( SharedDatumFunctoid(new TypeCount()) );
  m_functoids.append( SharedDatumFunctoid(new ShepardsRadius()) );
  m_functoids.append( SharedDatumFunctoid(new FrankeNelsonRadius()) );
  m_functoids.append( SharedDatumFunctoid(new NaturalNeighborRadius()) );
  return;
}

} // namespace Isis
