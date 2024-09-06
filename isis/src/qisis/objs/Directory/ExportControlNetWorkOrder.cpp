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
#include "ExportControlNetWorkOrder.h"

#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>

#include "Control.h"
#include "ControlList.h"
#include "ControlNet.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Project.h"

namespace Isis {

  /**
   * Creates a work order for exporting a control network from the project. This work
   * order is asynchronous and is not undoable.
   *
   * @param Project *project Pointer to the project to export from (the project this work order
   * belongs to).
   */
  ExportControlNetWorkOrder::ExportControlNetWorkOrder(Project *project) :
      WorkOrder(project) {
    m_isSynchronous = false;
    m_isUndoable = false;
    QAction::setText(tr("&Export Control Network..."));
    QUndoCommand::setText(tr("Export Control Network..."));
  }


  /**
   * @brief Copy constructor.
   *
   * Copies the work order.
   *
   * @param ExportControlNetWorkOrder &other The other work order to copy state from.
   */
  ExportControlNetWorkOrder::ExportControlNetWorkOrder(const ExportControlNetWorkOrder &other) :
      WorkOrder(other) {
  }


  /**
   * @brief Destructor.
   *
   * Default destructor to clean up any memory this work order might allocate.
   */
  ExportControlNetWorkOrder::~ExportControlNetWorkOrder() {

  }


  /**
   * @brief Clones this work order.
   *
   * Allocate a new work order using this work order's state.
   *
   * @return ExportControlNetWorkOrder* Returns a pointer to the newly cloned work order.
   */
  ExportControlNetWorkOrder *ExportControlNetWorkOrder::clone() const {
    return new ExportControlNetWorkOrder(*this);
  }


  /**
   * @brief Determines if we can export a control net.
   *
   * Currently, this work order only works with either no data (file menu) or with a
   * single control network.
   *
   * @param ControList *controls The current context we're inquiring about.
   *
   * @return bool Returns true if this work order functions with the given control list. Right now,
   * true indicates that there is one control list in the project.
   */
  bool ExportControlNetWorkOrder::isExecutable(ControlList *controls) {

    if (controls) {
      return (controls->count() == 1);
    }
    return false;
  }


  /**
   * @brief Prepares for exporting a control net by soliciting information from the user.
   *
   * Prompts the user for input. If there is no context, we ask the user to select a
   * control. Once we have a control (via context or asking the user), we then ask for a output cnet
   * file name. The relevant data is stored in internalData(). The internal data will contain
   * the control net id and the destination to export to.
   *
   * @return bool Returns true if the setup is successful.
   */
  bool ExportControlNetWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      QStringList internalData;

      Control *control = NULL;

      // See if there are any other control lists in the project and give these to the user as
      // choices for control nets they can export.
      if (project()) {

        Project *proj = project();

        QList<ControlList *> controls = proj->controls();
        if (controls.count() > 0) {
          ControlList *l = controls.first();
          WorkOrder::setData(l);
          control = controlList()->first();
        }
        else {

          QMap<Control *, QString> cnetChoices;
          foreach (ControlList *list, project()->controls()) {
            foreach (Control *control, *list) {
              cnetChoices[control] = tr("%1/%2").arg(list->name())
                  .arg(control->displayProperties()->displayName());
            }
          }

          QStringList cnetNames = cnetChoices.values();
          cnetNames.sort();

          QString choice = QInputDialog::getItem(NULL, tr("Select Control"),
              tr("Please choose a control to export."), cnetNames, 0, false, &success);

          control = cnetChoices.key(choice);
          internalData.append(control->id());
        }
      }


      QString destination =
          QFileDialog::getSaveFileName(NULL, QString("Export Control Network"),
          "./" + QString::fromStdString(FileName(control->fileName().toStdString()).name()));

      if (destination.isEmpty()) {
        success = false;
      }
      internalData.append(destination);

      setInternalData(internalData);
    }

    return success;
  }


  /**
   * @brief Executes the work order.
   *
   * Uses internalData() and writes the control network into the output file. Stores
   * errors in m_warning which will be reported in postExecution().
   *
   * @see WorkOrder::execute()
   */
  void ExportControlNetWorkOrder::execute() {
    QString destination;
    Control *control = NULL;

    if (controlList()->isEmpty()) {
      destination = internalData()[1];

      QString controlId = internalData()[0];
      control = project()->control(controlId);
    }
    else {
      destination = internalData()[0];
      control = controlList()->first();
    }

    QString currentLocation = control->fileName();
    if (!QFile::copy(currentLocation, destination) ) {
      m_warning = "Error saving control net.";
    }
  }


  /**
   * @brief Display any warnings that occurred during the asynchronous computations.
   *
   * These warnings will be attached to the project.
   *
   * @see WorkOrder::postExecution()
   */
  void ExportControlNetWorkOrder::postExecution() {
    if (!m_warning.isEmpty()) {
      project()->warn(m_warning);
      m_warning.clear();
    }
  }
}
