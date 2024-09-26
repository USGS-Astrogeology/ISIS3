/**
 * @file
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: CnetReaderStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
#include "CnetReaderStrategy.h"

// other ISIS
#include "ControlNetVersioner.h"
#include "IException.h"
#include "FileName.h"
#include "Pvl.h"
#include "PvlContainer.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /**
   * Default constructor
   */
  CnetReaderStrategy::CnetReaderStrategy() : Strategy("CnetReader", "CnetReader") {
  }


  /**
   *
   * @param definition
   * @param globals
   * @brief Constructor loads from a Strategy object CnetReader definition
   *
   * This constructor loads and retains processing parameters from the CnetReader
   * Strategy object definition as (typically) read from the configuration file.
   *
   * @author 2014-12-25 Kris Becker
   *
   * @param definition CnetReader Strategy PVL object definition
   * @param globals    Global Resource of keywords
   */
  CnetReaderStrategy::CnetReaderStrategy(const PvlObject &definition,
                                         const ResourceList &globals) :
                                         Strategy(definition, globals) {

  }


  /**
   *
   */
  CnetReaderStrategy::~CnetReaderStrategy() {
  }


  /**
   * @brief Obtains the Resources from the control network file
   *
   * This Strategy appends each of the Resources obtained from the ISIS control network file to a list
   * of Resources. The return value verifies the number of Resources that were created from the
   * ControlMeasures in the ISIS control network file.
   *
   * @author 2014-12-25 Kris Becker
   *
   * @param resources ResourceList created from the control network
   * @param globals   List of global keywords to use in argument substitutions
   *
   * @return int The number of Resources created from each ControlMeasure
   */
  int CnetReaderStrategy::apply(ResourceList &resources,
                                const ResourceList &globals) {

    ResourceList measures = cnetResource(globals);
    resources.append( measures );

    return ( measures.size() );
  }


  /**
   * @brief Creates Resources from a control network
   *
   * Helper method for apply(). Reads a control network and creates Resources from each ControlMeasure
   * group in the control network. Each Resource is created with a unique name (a serial number) and
   * the keywords in the ControlMeasure group. Specific starting name values can be
   * determined by the pointnum parameter. Default starting name value is 0.
   *
   * @author 2014-12-25 Kris Becker
   *
   * @param globals   List of global keywords to use in argument substitutions
   * @param pointnum - The row id number to start naming Resources with (default 0)
   *
   * @return ResourceList The list of Resources created
   */
  ResourceList CnetReaderStrategy::cnetResource(const ResourceList &globals,
                                                const int &pointNum) const {

    int nrows = pointNum;

    FileName netfile(translateKeywordArgs("CnetFile", globals).toStdString());
    ControlNetVersioner cnetReader(netfile);
    Pvl pvl(cnetReader.toPvl());

    PvlObject &network(pvl.findObject("ControlNetwork"));
    PvlFlatMap netkeys(loadkeys(network));

    ResourceList pointlist;
    for ( int p = 0 ; p < network.objects() ; p++ ) {
      PvlObject &point = network.object(p);

      if ( "controlpoint" == QString::fromStdString(point.name()).toLower() ) {

        PvlFlatMap netpoint(netkeys, loadkeys(point));
        for (int m = 0; m < point.groups() ; m++ ) {
          PvlGroup &measure = point.group(m);

          if ( "controlmeasure" == QString::fromStdString(measure.name()).toLower() ) {
            PvlFlatMap netmeasure(netpoint, loadkeys(measure));
            QString rowId = QString::fromStdString(toString(nrows++));
            SharedResource rowmeasure(new Resource(rowId, netmeasure));

            // Make the unique identifier (set default identity or set to specified)
            QString identity = translateKeywordArgs("Identity", getGlobals(rowmeasure, globals));
            if ( identity.isEmpty() ) {
              identity = rowId;
            }
            rowmeasure->setName(identity);
            pointlist.push_back(rowmeasure);
          }
        }
      }
    }

    return (pointlist);
  }


  /**
   * @brief Returns a more accessible PVL structure from a PvlContainer object
   *
   * Returns a more accessible PvlFlatMap from a passed PvlContainer. This PvlFlatMap provides a
   * more accessible interface to PVL keywords.
   *
   * @author 2014-12-25 Kris Becker
   *
   * @param keys - The PvlContainer object to convert to a PvlFlatMap
   *
   * @return PvlFlatMap Returns a more accessible PvlFlatMap
   */
  PvlFlatMap CnetReaderStrategy::loadkeys(const PvlContainer &keys) const {
    return (PvlFlatMap(keys));
  }

}  //namespace Isis
