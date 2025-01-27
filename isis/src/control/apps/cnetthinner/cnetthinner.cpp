
/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QString>

#include "Application.h"
#include "CnetManager.h"
#include "CnetSuppression.h"
#include "FileName.h"
#include "IException.h"
#include "ProcessByLine.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "UserInterface.h"

#include "cnetthinner.h"

using namespace std;

namespace Isis {

  /**
   * Computes the most efficient spatial control point distribution for each image in the input
   * control network given a maximum number of points.
   *
   * @param ui UserInterface object containing parameters
   *
   * @return Pvl results log file
   *
   * @throws IException::User "Control Net filename [FILENAME] is invalid."
   */
  Pvl cnetthinner(UserInterface &ui) {

    Progress progress;
    QSharedPointer<ControlNet> inputNet;

    try {
      inputNet.reset(new ControlNet(ui.GetFileName("CNET"), &progress));
    }
    catch (IException &e) {
      throw e;
    }

    return cnetthinner(inputNet, ui);
  }


  /**
   * Computes the most efficient spatial control point distribution for each image in the input
   * control network given a maximum number of points.
   *
   * @param cnet input ControlNet
   * @param ui UserInterface object containing parameters
   *
   * @return Pvl results log file
   *
   * @throws IException::User "TOLERANCE must be between 0.0 and 1.0"
   */
  Pvl cnetthinner(QSharedPointer<ControlNet> &cnet, UserInterface &ui) {

    // We will be processing by line
    ProcessByLine p;

    double  weight     = ui.GetDouble("WEIGHT");
    double  tolerance  = ui.GetDouble("TOLERANCE");
    int     maxpoints  = ui.GetDouble("MAXPOINTS");
    int     minpoints  = ui.GetDouble("MINPOINTS");
    QString suppressed = ui.GetString("SUPPRESSED").toLower();

    if (tolerance < 0.0 || tolerance > 1.0) {
        QString msg = "TOLERANCE must be between 0.0 and 1.0";
        throw IException(IException::User, msg, _FILEINFO_);
    }

    QScopedPointer<CnetSuppression> suppressor(new CnetSuppression(cnet, weight));
    // suppression->setEarlyTermination(true); NOTE: early termination is currently set by a hard-coded flag

    // Suppress the points
    int totalLoaded = suppressor->size();
    CnetSuppression::Results result =
        suppressor->suppress(minpoints, maxpoints, 1.5, tolerance);

    int nsaved = result.size();
    int nremoved = totalLoaded - nsaved;
    double efficiency = ( (double) nremoved/(double) totalLoaded ) * 100.0;

    if ( ui.WasEntered("ONET") ) {
      bool saveall = ( "ignore" == suppressed );
      QString netid;
      if ( ui.WasEntered("NETWORKID") ) {
        netid = ui.GetString("NETWORKID");
      }
      suppressor->write(ui.GetAsString("ONET"), result, saveall, netid);
    }

    // Report results
    Pvl log;
    PvlGroup results("Results");
    results += PvlKeyword("Points", toString(totalLoaded) );
    results += PvlKeyword("Saved",  toString(nsaved) );
    results += PvlKeyword("Suppressed",  toString(nremoved) );
    results += PvlKeyword("Efficiency", toString(efficiency, 4), "percent" );
    Application::AppendAndLog(results, &log);

    p.EndProcess();

    return log;
  }
}
