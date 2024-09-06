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
#include "ImportRegistrationTemplateWorkOrder.h"

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QRegularExpression>
#include <QDebug>

#include "Project.h"
#include "ProjectItemModel.h"
#include "Template.h"
#include "TemplateList.h"

namespace Isis {
  /**
   * Creates a work order to import registration templates.
   *
   * @param *project Pointer to the project this work order belongs to
   */
  ImportRegistrationTemplateWorkOrder::ImportRegistrationTemplateWorkOrder(Project *project) :
      WorkOrder(project) {

    m_isUndoable = true;
    m_list = NULL;

    QAction::setText(tr("Import Registration Templates..."));
    QUndoCommand::setText(tr("Import Registration Templates..."));
    setModifiesDiskState(true);

  }


  /**
   * Creates a copy of the other ImportRegistrationTemplateWorkOrder
   *
   * @param &other ImportRegistrationTemplateWorkOrder to copy the state from
   */
  ImportRegistrationTemplateWorkOrder::ImportRegistrationTemplateWorkOrder(const ImportRegistrationTemplateWorkOrder &other) :
      WorkOrder(other) {
  }


  /**
   * Destructor
   */
  ImportRegistrationTemplateWorkOrder::~ImportRegistrationTemplateWorkOrder() {
    m_list = NULL;
  }


  /**
   * This method clones the current ImportRegistrationTemplateWorkOrder and returns it.
   *
   * @return ImportRegistrationTemplateWorkOrder Clone
   */
  ImportRegistrationTemplateWorkOrder *ImportRegistrationTemplateWorkOrder::clone() const {
    return new ImportRegistrationTemplateWorkOrder(*this);
  }


  /**
   * This method returns true if the user clicked on a project tree node with the text
   * "Registrations".
   * This is used by Directory::supportedActions(DataType data) to determine what actions are
   * appended to context menus.
   *
   * @param item The ProjectItem that was clicked
   *
   * @return bool True if the user clicked on a project tree node named "Registrations"
   */
  bool ImportRegistrationTemplateWorkOrder::isExecutable(ProjectItem *item) {
    QString itemType = item->text();
    setInternalData(QStringList(itemType));

    return (itemType == "Registrations");
  }


  /**
   * @brief Sets up the work order for execution.
   *
   * This method prompts the user for a template to open.
   *
   * @see WorkOrder::setupExecution()
   *
   * @return bool Returns true if we have at least one template file name.
   */
  bool ImportRegistrationTemplateWorkOrder::setupExecution() {

    WorkOrder::setupExecution();

    QStringList templateFileNames;

    templateFileNames = QFileDialog::getOpenFileNames(
        qobject_cast<QWidget *>(parent()),
        "Import Registration Templates",
        QString(),
        "Registrations (*.def);; All Files (*)");

    if (!templateFileNames.isEmpty()) {
      QUndoCommand::setText(tr("Import %1 Template(s)").arg(templateFileNames.count()));
    }
    else {
      return false;
    }

    setInternalData(templateFileNames);

    return true;
  }


  /**
   * @brief Imports the templates
   *
   * This method copies the template files into the appropriate directory. If the file already 
   * exists in the chosen directory, it will not be copied over.
   */
  void ImportRegistrationTemplateWorkOrder::execute() {
    QDir templateFolder = project()->addTemplateFolder("registrations/import");
    QStringList templateFileNames = internalData();

    m_list = new TemplateList(templateFolder.dirName(), 
                              "registrations", 
                              "registrations/" + templateFolder.dirName() );

    for (const QString &str : templateFileNames) {
      FileName filename(str.toStdString());
      QFile::copy(QString::fromStdString(filename.expanded()), templateFolder.path() + "/" + QString::fromStdString(filename.name()));
      m_list->append(new Template(templateFolder.path() + "/" + QString::fromStdString(filename.name()), 
                                  "registrations", 
                                  templateFolder.dirName()));
    }

    if (!m_list->isEmpty()) {
      project()->addTemplates(m_list);
      project()->setClean(false);
    }

  }


  /**
   * @brief Deletes the previously imported templates
   *
   * This method deletes the templates from both the directory they were copied to
   * and the ProjectItemModel
   */
  void ImportRegistrationTemplateWorkOrder::undoExecution() {
    if (m_list && project()->templates().size() > 0) {
      m_list->deleteFromDisk( project() );
      ProjectItem *currentItem =
          project()->directory()->model()->findItemData(QVariant::fromValue(m_list));
      project()->directory()->model()->removeItem(currentItem);
    }
    foreach ( Template *currentTemplate, *m_list) {
      delete currentTemplate;
    }
    delete m_list;
    m_list = NULL;
  }

}
