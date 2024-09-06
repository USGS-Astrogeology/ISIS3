/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
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
#include "JigsawWorkOrder.h"

#include <QtDebug>

#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>

#include "BundleSolutionInfo.h"
#include "Control.h"
#include "Directory.h"
#include "JigsawRunWidget.h"
#include "JigsawSetupDialog.h"
#include "Project.h"

namespace Isis {

  /**
   * @brief Constructs a JigsawWorkOrder.
   *
   * This creates a work order to run bundle adjustments. Note that right now,
   * the design implemented means that this work order finishes after a JigsawRunWidget
   * is shown. This work order is synchronous and not undoable. Note is is synchronous
   * in that it simply displays a dialog. The actual bundle adjust is threaded.
   *
   * @see JigsawRunWidget
   *
   * @param project The Project that we are going to Bundle Adjust
   *
   */
  JigsawWorkOrder::JigsawWorkOrder(Project *project) :
      WorkOrder(project) {
    // This work order is synchronous and not undoable
    m_isUndoable = false;
    QAction::setText(tr("&Bundle Adjustment..."));
    QUndoCommand::setText("&Bundle Adjustment...");
    QString hoverText = "Runs a bundle adjustment. ";
    hoverText += "You must import a control net and images before you can run a bundle adjustment.";
    QAction::setToolTip(hoverText);
  }


  /**
   * @brief Copy constructor.
   *
   * Copies the state of another JigsawWorkOrder.
   */
  JigsawWorkOrder::JigsawWorkOrder(const JigsawWorkOrder &other) :
      WorkOrder(other) {
    m_bundleSettings = other.m_bundleSettings;
  }


  /**
   * Destructor
   */
  JigsawWorkOrder::~JigsawWorkOrder() {
  }


  /**
   * This method clones the JigsawViewWorkOrder
   *
   * @return JigsawWorkOrder* Returns a clone of the JigsawWorkOrder
   */
  JigsawWorkOrder *JigsawWorkOrder::clone() const {
    return new JigsawWorkOrder(*this);
  }


/**
 * This method is no longer necessary and will remain commented out until it needs to be implemented 
 *  
 * If WorkOrder:setupExecution() returns true, this creates a setup dialog.   
 *    
 * When the setup is successful (i.e. the user does not cancel the dialog), this work order   
 * will be read to execute.   
 *    
 * @return bool Returns True if setup dialog for the bundle adjustment is successful.   
 */   
// bool JigsawWorkOrder::setupExecution() {    
// }


  /**
   * This check is used by Directory::supportedActions(DataType data).
   *
   * @return bool True if the number of project controls and the number of project images
   *         is greater than 0.
   */
  bool JigsawWorkOrder::isExecutable() {
    // Is this code ever run?
    return (project()->controls().size() > 0 && 
            project()->images().size() > 0 &&
            !project()->directory()->jigsawRunWidget());
  }


 /**
  * This method returns true if other depends on a JigsawWorkOrder
  *
  * @param order the WorkOrder we want to check for dependancies
  *
  * @return bool True if WorkOrder depends on a JigsawWorkOrder
  *
  */
  bool JigsawWorkOrder::dependsOn(WorkOrder *other) const {
    // depend on types of ourselves.
    return dynamic_cast<JigsawWorkOrder *>(other);
  }


  /**
   * Executes the work order by creating a jigsaw dialog that allows the user to run or re-setup
   * the settings for a bundle adjustment.
   *
   * @see WorkOrder::execute()
   */
  void JigsawWorkOrder::execute() {
    JigsawRunWidget *runWidget = project()->directory()->addJigsawRunWidget();
    if (!runWidget) {
      std::string msg = "Unable to open Jigsaw Run Widget";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }
}
