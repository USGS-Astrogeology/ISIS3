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
#include "TemplateEditViewWorkOrder.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QtDebug>

#include "Directory.h"
#include "IException.h"
#include "Project.h"
#include "ProjectItem.h"
#include "Template.h"
#include "TemplateEditorWidget.h"

namespace Isis {

/**
   * @brief Creates a WorkOrder that will retrieve Target info.
   *
   * Facilitates creating a view for the target information. This work order is not
   * undoable.
   *
   * @param project  The Project that this work order should be interacting with.
   */
  TemplateEditViewWorkOrder::TemplateEditViewWorkOrder(Project *project) :
      WorkOrder(project) {
    QAction::setText(tr("Edit template...") );
    QUndoCommand::setText(tr("Edit Template"));

    // This work order is not undoable
    m_isUndoable = false;
    m_isSavedToHistory = false;
  }


  /**
   * @brief Copies the 'other' WorkOrder instance into this new instance.
   * @param other The WorkOrder being copied.
   */
  TemplateEditViewWorkOrder::TemplateEditViewWorkOrder(const TemplateEditViewWorkOrder &other) :
      WorkOrder(other) {
  }


  /**
   * @brief The Destructor.
   */
  TemplateEditViewWorkOrder::~TemplateEditViewWorkOrder() {
  }


  /**
   * @brief Returns a copy of this TemplateEditViewWorkOrder instance.
   * @return @b (TemplateEditViewWorkOrder *) A pointer to a copy of this WorkOrder.
   */
  TemplateEditViewWorkOrder *TemplateEditViewWorkOrder::clone() const {
    return new TemplateEditViewWorkOrder(*this);
  }


  /**
   * @brief Determines if we can get target info.
   *
   * Determines if we already have a view for the target.  If we do, then we
   * do not need to redisplay the object.
   *
   * @param targetBody The target body to check for.
   *
   * @return bool Returns true if a view for the target already exists, false otherwise.
   */
  bool TemplateEditViewWorkOrder::isExecutable(ProjectItem *projectitem) {
    if (!projectitem)
      return false;

    if (projectitem->isTemplate()) {

      Template *currentTemplate = projectitem->getTemplate();
      // if we already have a view for this target, don't redisplay
      QList<TemplateEditorWidget *> existingViews = project()->directory()->templateEditorViews();
      for (int i = 0; i < existingViews.size(); i++) {
        if (existingViews.at(i)->objectName() == currentTemplate->fileName() )
          return false;
      }
      return true;
    }

    return false;
  }


  /**
   * @brief Attempt to retrieve the Target info.
   *
   * Attempts to retrieve the target body information to prepare for execution.
   *
   * @return bool Returns true if successful, false otherwise.
   */
  bool TemplateEditViewWorkOrder::setupExecution() {
    bool success = WorkOrder::setupExecution();

    if (success) {
      // This class relies on the parent class, WorkOrder to set m_template with its setData method.
      // We can retrieve this template for ourselves using getTemplate(), a method of WorkOrder's.
      QString templateFileName = getTemplate()->fileName();
      QStringList internalData;
      internalData.append(templateFileName);
      setInternalData(internalData);
    }
    return success;
  }


  /**
   * @brief Executes this work order.
   *
   * Adds a target info view to the project; i.e., displays the target info widget.
   */
  void TemplateEditViewWorkOrder::execute() {

    TemplateEditorWidget *templateEditorWidget =
        project()->directory()->addTemplateEditorView(getTemplate());

    if (!templateEditorWidget) {
      std::string msg = "error displaying target info";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * @brief Determines whether another WorkOrder depends upon TemplateEditViewWorkOrder.
   * @param other  The WorkOrder being checked for dependency.
   * @return @b bool  True if there is a dependency, False otherwise.
   */
  bool TemplateEditViewWorkOrder::dependsOn(WorkOrder *other) const {
    // depend on types of ourselves.
    return dynamic_cast<TemplateEditViewWorkOrder *>(other);
  }
}
