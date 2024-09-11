/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProjectItemModel.h"
#include <QDebug>
#include <QItemSelection>
#include <QList>
#include <QMessageBox>
#include <QMimeData>
#include <QModelIndex>
#include <QObject>
#include <QRegExp>
#include <QRegExpValidator>
#include <QStandardItemModel>
#include <QString>
#include <QValidator>

#include "BundleSolutionInfo.h"
#include "Control.h"
#include "ControlList.h"
#include "FileItem.h"
#include "GuiCameraList.h"
#include "ImageList.h"
#include "Project.h"
#include "ProjectItem.h"
#include "ShapeList.h"
#include "TargetBodyList.h"
#include "TemplateList.h"


namespace Isis {
  /**
   * Constructs an empty model.
   *
   * @param parent (QObject *) The parent.
   */
  ProjectItemModel::ProjectItemModel(QObject *parent) : QStandardItemModel(parent) {
    m_selectionModel = new QItemSelectionModel(this, this);
    connect(m_selectionModel, SIGNAL(selectionChanged(const QItemSelection &,
                                                      const QItemSelection &) ),
            this, SLOT(onSelectionChanged(const QItemSelection &, const QItemSelection &) ) );

    connect( this, SIGNAL(rowsInserted(const QModelIndex &, int, int)),
             this, SLOT(onRowsInserted(const QModelIndex &, int, int)) );

    connect( this, SIGNAL(rowsAboutToBeRemoved(const QModelIndex &, int, int)),
             this, SLOT(onRowsRemoved(const QModelIndex &, int, int)) );

  }


  /**
   * Destructs the model.
   */
  ProjectItemModel::~ProjectItemModel() {

  }


  /**
   * You cannot drop mime data into the ProjectItemModel.
   *
   * @see ProjectItemProxyModel
   *
   * @param data (const QMimeData *) The data to drop
   * @param action (Qt::DropAction) The drop action
   * @param row (int) ???
   * @param column (int) ???
   * @param parent (const QModelIndex &) The index of the data's parent
   *
   * @return @b bool False
   */
  bool ProjectItemModel::canDropMimeData(const QMimeData *data,
                                         Qt::DropAction action,
                                         int row, int column,
                                         const QModelIndex &parent) const {
    return false;
  }


  /**
   * Returns the internal selection model.
   *
   * @return @b QItemSelectionModel* The selection model.
   */
  QItemSelectionModel *ProjectItemModel::selectionModel() {
    return m_selectionModel;
  }


  /**
   * Adds a Project to the model. An item is created that corresponds to the
   * Project as well as children items that correspond to various parts of the
   * Project.
   *
   * @param project (Project *) The Project to be added.
   *
   * @return @b ProjectItem* The new item that corresponds to the Project.
   */
  ProjectItem *ProjectItemModel::addProject(Project *project) {

    connect(project, SIGNAL( nameChanged(QString) ),
            this, SLOT( onNameChanged(QString) ) );
    connect(project, SIGNAL( bundleSolutionInfoAdded(BundleSolutionInfo *) ),
            this, SLOT( onBundleSolutionInfoAdded(BundleSolutionInfo *) ) );
    connect(project, SIGNAL( controlAdded(Control *) ),
            this, SLOT( onControlAdded(Control *) ) );
    connect(project, SIGNAL( controlListAdded(ControlList *) ),
            this, SLOT( onControlListAdded(ControlList *) ) );
    connect(project, SIGNAL( imagesAdded(ImageList *) ),
            this, SLOT( onImagesAdded(ImageList *) ) );
    connect(project, SIGNAL( shapesAdded(ShapeList *) ),
            this, SLOT( onShapesAdded(ShapeList *) ) );
    connect(project, SIGNAL( targetsAdded(TargetBodyList *) ),
            this, SLOT( onTargetsAdded(TargetBodyList *) ) );
    connect(project, SIGNAL( templatesAdded(TemplateList *)),
            this, SLOT( onTemplatesAdded(TemplateList *)));
    connect(project, SIGNAL( guiCamerasAdded(GuiCameraList *) ),
            this, SLOT( onGuiCamerasAdded(GuiCameraList *) ) );
    ProjectItem *projectItem = new ProjectItem(project);
    appendRow(projectItem);
//  qDebug()<<"ProjectItem::addProject after appendRow projectItem rowCount = "<<rowCount();
    return projectItem;
  }


  /**
   * Returns the current item of the internal selection model.
   *
   * @return @b ProjectItem* The current item.
   */
  ProjectItem *ProjectItemModel::currentItem() {

    ProjectItem *item = itemFromIndex( selectionModel()->currentIndex() );

    // We do this because if the user was in a footprint or cubeDN view, then
    // There is no valid currentIndex(). In that case, we grab whichever item
    // was right clicked that triggered this call.
    if (item == NULL) {
      item = selectedItems().at(0);
    }
    return item;
  }


  /**
   * Returns a list of the selected items of the internal selection model.
   *
   * @return @b QList<ProjectItem*> The list of selected items.
   */
  QList<ProjectItem *> ProjectItemModel::selectedItems() {
    QItemSelection selection = selectionModel()->selection();
    QList<ProjectItem *> items;


    foreach ( QModelIndex index, selection.indexes() ) {
      items.append( itemFromIndex(index) );
    }

    return items;
  }
  /**
   * @brief ProjectItemModel::selectedBOSSImages
   * @return This is a refinement of the selectedItems function which
   * was needed to display a subset of Images/ImageLists in the
   * Bundle Observation Solve Settings (BOSS) tab of the JigsawSetupDialog widget.
   * The primary consumer of the selected images is going to be the SortFilterProxyModel
   * class.
   */
  QList<ProjectItem *> ProjectItemModel::selectedBOSSImages() {

    QItemSelection selection = selectionModel()->selection();
    QList<ProjectItem *> items;
    QModelIndexList indices = selection.indexes();

    // If nothing is selected, fill items with all image lists and images.
    if (indices.size() == 0) {
      ProjectItem *imageRoot = findItemData(QVariant("Images"), 0);
      items.append(imageRoot);
      for (int i = 0; i < imageRoot->rowCount(); i++) {
        ProjectItem *imglistItem = imageRoot->child(i);
        items.append(imglistItem);
        for (int j = 0; j < imglistItem->rowCount(); j++) {
          ProjectItem *imgItem = imglistItem->child(j);
          if (imgItem->isImage()) {
            items.append(imgItem);
          }
        }
      }
      return items;
    }

    //Query the selected items to see if they have children
    foreach ( QModelIndex ix, indices ) {

      ProjectItem *item = this->itemFromIndex(ix);

      //Anything that is not an image or an image list does
      //not make sense to display in the BOSS treeview tab,
      //so we need to exclude these items.
      if (item->isImageList() || item->isImage() ) {
        items.append( item );
      }
      else {
        return items;
      }

      //If the selected ImageList has children, we have to handle
      //the case where some of the children are selected, or
      //the possibility that the user wants all of the children selected.
      if (this->hasChildren(ix)) {

        //If the node has children, loop through all of them
        //and add them to selected items.
        bool childrenSelected(false);
        int numChildren = this->rowCount(ix);

        //First loop through the children to see if any of them are also selected
        for (int i = 0; i < numChildren;i++) {
          QModelIndex ixchild = this->index(i,0,ix);
          if (indices.contains(ixchild) ){
              childrenSelected=true;
              break;
          }
        }
          //If they are, then add them to selected items
          if (childrenSelected) {
            for (int i =0;i < numChildren;i++) {
              QModelIndex ixchild = this->index(i,0,ix);
              if (indices.contains(ixchild))
                items.append(this->itemFromIndex(ixchild ));
            }
          }
          //No children selected, so we are assuming that the user
          //wanted to select all of the children under the parent.
          else {
            for (int i =0;i < numChildren;i++) {
               QModelIndex ixchild = this->index(i,0,ix);
               items.append(this->itemFromIndex(ixchild ));
            }

          }

      }//end if

      //Append the parent of any selected child.  This is so
      //the children aren't hanging on the tree without
      //a collapsible parent node.
      if( item->parent() ->hasChildren()) {
        ProjectItem * parent = item->parent();
        if (!items.contains(parent)){
          items.append(parent);
        }// end inner if
      }//end outer if
      //Also include the grandparent.  This handles the event
      //that we may have multiple image lists selected to the treeview
      //and we need a grandparent node attached to group them under.
      if (this->itemFromIndex(ix)->parent()->parent() ){
        ProjectItem *grandparent = this->itemFromIndex(ix)->parent()->parent();
        if (!items.contains(grandparent)) {
          items.append(grandparent);
        } //end inner if

      } //end outer if

  }// end foreach

    return items;

  }


  /**
   * Returns the first item found that contains the given data in the given
   * role or a null pointer if no item is found.
   *
   * @param data (const QVariant &) The data contained in the item.
   *
   * @param role (int) The role of the data (see Qt::ItemDataRole).
   *
   * @return @b ProjectItem* First project item found.
   */
  ProjectItem *ProjectItemModel::findItemData(const QVariant &data, int role) {

//  qDebug()<<"ProjectItemModel::findItemData  incoming data = "<<data;
    for (int i=0; i<rowCount(); i++) {
//    qDebug()<<"ProjectItemModel::findItemData  BEFORE call: item(i)->findItemData...";
      ProjectItem *projectItem = item(i)->findItemData(data, role);
//    qDebug()<<"ProjectItemModel::findItemData  AFTER call: item(i)->findItemData...";
      if (projectItem) {
        return projectItem;
      }
    }

    return 0;
  }


  /**
   * Removes an item and its children from the model.
   *
   * @param item (ProjectItem *) The item to be removed.
   *
   * @internal
   *   @history 2017-08-08 Marjorie Hahn - Added a check so that if the item to be removed
   *                           has any children then they can be removed first. Fixes #5074.
   */
  void ProjectItemModel::removeItem(ProjectItem *item) {
    if (!item) {
      return;
    }

    // remove any children the item has first
    if (item->hasChildren()) {
      for (int row = (item->rowCount() - 1); row >= 0; row--) {
        removeRow(item->child(row)->row(), item->index());
      }
    }

    if (ProjectItem *parentItem = item->parent()) {
      // remove the item from its parent
      removeRow(item->row(), parentItem->index());
    }
    else {
      removeRow(item->row());
    }
  }


  /**
   * Removes a list of items and their children from the model.
   *
   * @param items (ProjectItem *) The items to be removed.
   */
  void ProjectItemModel::removeItems(QList<ProjectItem *> items) {
    foreach (ProjectItem *item, items) {
      removeItem(item);
    }
  }


  /**
   * Appends a top-level item to the model.
   *
   * @param item (ProjectItem *) The item to append.
   */
  void ProjectItemModel::appendRow(ProjectItem *item) {
    QStandardItemModel::appendRow(item);
  }


  /**
   * Returns the QModelIndex corresponding to a given ProjectItem.
   *
   * @param item (const ProjectItem *) The item.
   *
   * @return @b QModelIndex The index of the item.
   */
  QModelIndex ProjectItemModel::indexFromItem(const ProjectItem *item) {
    return QStandardItemModel::indexFromItem(item);
  }


  /**
   * Inserts a top-level item at the given row.
   *
   * @param row (int) The row where the item will be inserted.
   * @param item (ProjectItem *) The item to insert.
   */
  void ProjectItemModel::insertRow(int row, ProjectItem *item) {
    QStandardItemModel::insertRow(row, item);
  }


  /**
   * Returns the top-level item at the given row.
   *
   * @param row (int) The row of the item.
   *
   * @return @b ProjectItem* The item at the row.
   */
  ProjectItem *ProjectItemModel::item(int row) {
    return static_cast<ProjectItem *>( QStandardItemModel::item(row) );
  }


  /**
   * Returns the ProjectItem corresponding to a given QModelIndex.
   *
   * @param index (const QModelIndex &) The index of the item.
   *
   * @return @b ProjectItem* The item.
   */
  ProjectItem *ProjectItemModel::itemFromIndex(const QModelIndex &index) {
    return static_cast<ProjectItem *>( QStandardItemModel::itemFromIndex(index) );
  }


  /**
   * Sets the item at the top-level row.
   *
   * @param row (int) The row where the item will be set.
   * @param item (ProjectItem *) The item to set the row to.
   */
  void ProjectItemModel::setItem(int row, ProjectItem *item) {
    QStandardItemModel::setItem(row, item);
  }


  /**
   * Removes the top-level row and returns the removed item.
   *
   * @param row (int) The row of the item to remove.
   *
   * @return @b ProjectItem* The removed item.
   */
  ProjectItem *ProjectItemModel::takeItem(int row) {
    QList<QStandardItem *> items = QStandardItemModel::takeRow(row);

    if ( items.isEmpty() ) {
      return 0;
    }

    return static_cast<ProjectItem *>( items.first() );
  }


  /**
   * Slot to connect to the nameChanged() signal from a Project. Sets the name
   * of the ProjectItem that corresponds with the Project.
   *
   * @param newName (QString) The new name of the project
   */
  void ProjectItemModel::onNameChanged(QString newName) {
    Project *project = qobject_cast<Project *>( sender() );

    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        projectItem->setText(newName);
      }
    }
    project->setClean(false);
  }


  /**
   * Slot to connect to the bundleSolutionInfoAdded() signal from a
   * project. Adds a ProjectItem that corresponds to the
   * BundleSolutionInfo to the model. The item is added to the item
   * named "Results" of the item that corresponds to the Project that
   * sent the signal.
   *
   * @param bundleSolutionInfo (BundleSolutionInfo *) The BundleSolutionInfo
   *                                                      added to the Project.
   */
  void ProjectItemModel::onBundleSolutionInfoAdded(BundleSolutionInfo *bundleSolutionInfo) {
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(bundleSolutionInfo->name() );

    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *resultsItem = projectItem->child(j);
          if (resultsItem->text() == "Results") {
            ProjectItem *pItem = new ProjectItem(bundleSolutionInfo);
            resultsItem->appendRow( pItem );

            // Append text bundle summary and CSV files to the Statistics in the project
            ProjectItem *bundleSummaryItem = new ProjectItem(FileItemQsp(
               new FileItem(bundleSolutionInfo->savedBundleOutputFilename())),
                            "Summary", bundleSolutionInfo->savedBundleOutputFilename(),
                            QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/office-chart-pie.png")
                            .expanded())));
            pItem->child(2)->appendRow(bundleSummaryItem);
            ProjectItem *residualsItem = new ProjectItem(FileItemQsp(
               new FileItem(bundleSolutionInfo->savedResidualsFilename())),
                            "Measure Residuals", bundleSolutionInfo->savedResidualsFilename(),
                            QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/office-chart-pie.png")
                            .expanded())));
            pItem->child(2)->appendRow(residualsItem);
            ProjectItem *imagesItem = new ProjectItem(FileItemQsp(
               new FileItem(bundleSolutionInfo->savedImagesFilename())),
                            "Image", bundleSolutionInfo->savedImagesFilename(),
                            QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/office-chart-pie.png")
                            .expanded())));
            pItem->child(2)->appendRow(imagesItem);
            ProjectItem *pointsItem = new ProjectItem(FileItemQsp(
               new FileItem(bundleSolutionInfo->savedPointsFilename())),
                            "Control Points", bundleSolutionInfo->savedPointsFilename(),
                            QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/office-chart-pie.png")
                            .expanded())));
            pItem->child(2)->appendRow(pointsItem);
          }
        }
      }
    }
  }


  /**
   * Slot connected to the templatesAdded() signal from a project. Adds a ProjectItem for
   * each newly added template FileName to the model. The Item is added to the corresponding
   * ProjectItem under "Templates" (currently only "Maps" and "Registrations" ) and the name
   * of the TemplateList (import1, import2, etc...).
   *
   * @param templateList TemplateList of Templates being added to the project.
   */
  void ProjectItemModel::onTemplatesAdded(TemplateList *templateList) {
    Project *project = qobject_cast<Project *>( sender() );
    if (!project) { return; }

    // Start at our project's node
    // Start at our project's node
    for (int i = 0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {

        // Find the "Templates" node
        for (int j = 0; j < projectItem->rowCount(); j++) {
          ProjectItem *templatesItem = projectItem->child(j);
          if (templatesItem->text() == "Templates"){

            // Find either the "Maps" or "Registrations" node
            QString type = templateList->type();
            for (int k = 0; k < templatesItem->rowCount(); k++) {
                ProjectItem *templateType = templatesItem->child(k);
                if (templateType->text().toLower() == type) {
                  templateType->appendRow( new ProjectItem(templateList));
              }
            }
          }
        }
      }
    }
  }


  /**
   * Slot to connect to the controlAdded() signal from a project. Adds
   * a ProjectItem that corresponds to the Control to the model. The
   * item is added to the item that corresponds to the control's
   * ControlList.
   *
   * @param control (Control *) The Control added to the Project.
   */
  void ProjectItemModel::onControlAdded(Control * control) {
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(control->id() );

    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *controlsItem = projectItem->child(j);
          if (controlsItem->text() == "Control Networks") {
            for (int k=0; k < controlsItem->rowCount(); k++) {
              ProjectItem *controlListItem = controlsItem->child(k);
              ControlList *controlList = controlListItem->controlList();
              if ( controlList && controlList->contains(control) ) {
                controlListItem->appendRow( new ProjectItem(control) );
              }
            }
          }
        }
      }
    }
//  qDebug()<<"ProjectItemModel::onControlAdded rowCount = "<<rowCount();
  }


  /**
   * Slot to connect to the controlListAdded() signal from a
   * Project. Adds a ProjectItem that corresponds to the ControlList
   * to the model. The item is added to the item named "Control
   * Networks" of the item that corresponds to the Project that sent
   * the signal.
   *
   * @param controlList (ControlList *) The ControlList added to the
   *                                        Project.
   */
  void ProjectItemModel::onControlListAdded(ControlList * controlList) {
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(controlList->name() );

    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *controlsItem = projectItem->child(j);
          if (controlsItem->text() == "Control Networks") {
            controlsItem->appendRow( new ProjectItem(controlList) );
          }
        }
      }
    }
  }


  /**
   * Slot to connect to the imagesAdded() signal from a Project. Adds
   * a ProjectItem that corresponds to the ImageList to the model. The
   * item is added to the item named "Images" of the item that
   * corresponds to the Project that sent the signal.
   *
   * @param imageList (ImageList *) The ImageList added to the Project.
   */
  void ProjectItemModel::onImagesAdded(ImageList * imageList) {
//  qDebug()<<"ProjectItemModel::onImagesAdded  before add rowCount = "<<rowCount();
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(imageList->name() );
    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *imagesItem = projectItem->child(j);
          if (imagesItem->text() == "Images") {
            imagesItem->appendRow( new ProjectItem(imageList) );
          }
        }
      }
    }
//  qDebug()<<"ProjectItemModel::onImagesAdded  after add rowCount = "<<rowCount();
  }


  /**
   * Slot to connect to the shapesAdded() signal from a Project. Adds a ProjectItem that corresponds
   * to the ShapeList to the model. The item is added to the item named "Shape Models" of the item
   * that corresponds to the Project that sent the signal.
   *
   * @param shapes (ShapeList *) The ShapeList added to the Project.
   */
  void ProjectItemModel::onShapesAdded(ShapeList * shapes) {
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(shapes->name());

    if (!project) {
      return;
    }
    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *shapesItem = projectItem->child(j);
          if (shapesItem->text() == "Shapes") {
            shapesItem->appendRow( new ProjectItem(shapes) );
          }
        }
      }
    }
  }


  /**
   * Slot to connect to the targetsAdded() signal from a Project. Adds
   * items that correspond to the target bodies to the model. The
   * items are added to the item named "Target Body" of the item that
   * corresponds to the Project that sent the signal.
   *
   * @param targets (TargetBodyList *) The TargetBodyList of the Project.
   */
  void ProjectItemModel::onTargetsAdded(TargetBodyList *targets) {
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(targets->name() );

    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *targetsItem = projectItem->child(j);
          if (targetsItem->text() == "Target Body") {
            foreach(TargetBodyQsp target, *targets) {
              bool append = true;
              for (int k=0; k < targetsItem->rowCount(); k++) {
                ProjectItem *targetItem = targetsItem->child(k);
                if (targetItem->targetBody() == target) {
                  append = false;
                }
              }
              if (append) {
                targetsItem->appendRow( new ProjectItem(target) );
              }
            }
          }
        }
      }
    }
  }


  /**
   * Slot to connect to the guiCamerasAdded() signal from a
   * Project. Adds items that correspond to the cameras to the
   * model. The items are added to the item named "Sensors" of the
   * item that corresponds to the Project that sent the signal.
   *
   * @param cameras (GuiCameraList *) The GuiCameraList of the Project.
   */
  void ProjectItemModel::onGuiCamerasAdded(GuiCameraList *cameras) {
    Project *project = qobject_cast<Project *>( sender() );
    m_reservedNames.append(cameras->name() );

    if (!project) {
      return;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *projectItem = item(i);
      if (projectItem->project() == project) {
        for (int j=0; j < projectItem->rowCount(); j++) {
          ProjectItem *camerasItem = projectItem->child(j);
          if (camerasItem->text() == "Sensors") {
            foreach(GuiCameraQsp camera, *cameras) {
              bool append = true;
              for (int k=0; k < camerasItem->rowCount(); k++) {
                ProjectItem *cameraItem = camerasItem->child(k);
                if (cameraItem->guiCamera() == camera) {
                  append = false;
                }
              }
              if (append) {
                camerasItem->appendRow( new ProjectItem(camera) );
              }
            }
          }
        }
      }
    }
  }


  /**
   * Slot to connect to the selectionChanged() signal from a selection
   * model. Updates other factors in the model that rely on selected
   * items but do not directly correspond with the selection
   * model. Currently changes the selected property of Images that
   * correspond with selected or deselected items.
   *
   * @param selected (const QItemSelection &) The selected items.
   * @param deselected (const QItemSelection &) The deselected items.
   */
  void ProjectItemModel::onSelectionChanged(const QItemSelection &selected,
                                            const QItemSelection &deselected) {
    QList<ProjectItem *> selectedItems;
    foreach ( QModelIndex index, selected.indexes() ) {
      selectedItems.append( itemFromIndex(index) );
    }

    foreach (ProjectItem *item, selectedItems) {
      if ( item->isImage() ) {
        item->image()->displayProperties()->setSelected(true);
      }
    }

    QList<ProjectItem *> deselectedItems;
    foreach ( QModelIndex index, deselected.indexes() ) {
      deselectedItems.append( itemFromIndex(index) );
    }

    foreach (ProjectItem *item, deselectedItems) {
      if ( item->isImage() ) {
        item->image()->displayProperties()->setSelected(false);
      }
    }

  }


  /**
   * Slot to connect to the rowsInserted() signal from
   * QAbstractItemModel. Emits a corresponding itemAdded() signal for
   * each row inserted.
   *
   * @param parent (const QModelIndex &) The parent index where rows
   *                                         were inserted.
   * @param start (int) The first row inserted (inclusive).
   * @param end (int) The last row inserted (inclusive).
   */
  void ProjectItemModel::onRowsInserted(const QModelIndex &parent, int start, int end) {
    for (int row=start; row <= end; row++) {
      QModelIndex newIndex = index(row, 0, parent);
      ProjectItem *item = itemFromIndex(newIndex);
      emit itemAdded(item);
    }
  }


  /**
   * Slot to connect to the rowsAboutToBeRemoved() signal from QAbstractItemModel. Emits a
   * corresponding itemRemoved() signal for each row inserted.
   *
   * @param parent (const QModelIndex &) The parent index where rows are to be removed.
   *
   * @param start (int) The first row to be removed (inclusive).
   * @param end (int) The last row to be removed (inclusive).
   */
  void ProjectItemModel::onRowsRemoved(const QModelIndex &parent, int start, int end) {
    for (int row=start; row <= end; row++) {
      QModelIndex newIndex = index(row, 0, parent);
      ProjectItem *item = itemFromIndex(newIndex);
//    qDebug()<<"ProjectItemModel::onRowsRemoved this = "<<this<<"   item = "<<item;
      emit itemRemoved(item);
    }
//  qDebug()<<"ProjectItemModel::onRowsRemoved  Source model : "<<this<<"  row count = "<<rowCount();
  }


  /**
   * This virtual method was added to handle changing the project name by double-clicking the
   * project name on the project tree.  It was required by Qt in order to allow editing
   * capabilities.
   *
   * @see http://doc.qt.io/qt-5/modelview.html
   *
   * @param index (const QModelIndex &) Field which has been edited
   * @param value (const QVariant &) Value contained in the field
   * @param role (int) Will always be EditRole since field only contains text
   *
   * @return bool Returns true if successful; otherwise false
   */
  bool ProjectItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {

     ProjectItem *item = itemFromIndex(index);

     QString name = value.toString();

     bool rejected =rejectName(m_reservedNames,name);

     if (rejected) {
       QMessageBox nameRejected;
       nameRejected.setText("That name is already in use within this project.");
       nameRejected.exec();
       return true;
     }

     m_reservedNames.append(name);

     if (item->isProject() && role == Qt::EditRole) {
          emit projectNameEdited(name);
     }

    else if (item->isBundleSolutionInfo() && role == Qt::EditRole) {
      item->setText(name);
      item->bundleSolutionInfo()->setName(name);
      emit cleanProject(false);
    }
    else if (item->isImageList() && role == Qt::EditRole) {
      item->setText(name);
      item->imageList()->setName(name);
      emit cleanProject(false);
    }
    else if (item->isControlList() && role == Qt::EditRole) {
      item->setText(name);
      item->controlList()->setName(name);
      emit cleanProject(false);
    }
    else if (item->isShapeList() && role == Qt::EditRole) {
      item->setText(name);
      item->shapeList()->setName(name);
      emit cleanProject(false);
    }
    else if (item->isTemplate() && role == Qt::EditRole) {
      item->setText(name);
      emit cleanProject(false);
    }
    return true;
  }


  /**
   * This virtual method was added to handle changing the project name by double-clicking the
   * project name on the project tree.  It was required by Qt in order to allow editing
   * capabilities.
   *
   * @see http://doc.qt.io/qt-5/modelview.html
   *
   * @param index (const QModelIndex &) Field which has been edited
   *
   * @return Qt::ItemFlags Add the ItemIsEditable to the standard flags.
   */
  Qt::ItemFlags ProjectItemModel::flags(const QModelIndex &index) const {

    return Qt::ItemIsEditable | QStandardItemModel::flags(index);
  }



  /**
   * @brief Checks to see if we are adding a reserved name to the project
   * (ex. If we are adding an ImageList, and giving it the same name as another
   * ImageList, or ShapesList, or something else).
   * @param reserved  The list of reserved names we cannot use.
   * @param target The name we are querying to see if it is in the reserved list.
   * @return True if target is in the reserved list, False other.
   */
  bool ProjectItemModel::rejectName(QStringList &reserved, QString target) {


    QRegExpValidator valid;
    QValidator::State state;
    int pos =0;
    foreach (QString name, reserved) {

      QRegExp rx(name);
      valid.setRegExp(rx);
      state = valid.validate(target,pos);

      if (state == 2) {
        return true;
      }
    } //end for

    return false;
  }

  /**
   * Used to clean the ProjectItemModel of everything but the headers
   */
   void ProjectItemModel::clean() {
     for (int i=0; i<rowCount(); i++) {
       ProjectItem *projectItem = item(i);
       if (projectItem->project()) {
         for (int j=0; j < projectItem->rowCount(); j++) {
           if (projectItem->hasChildren()) {
             ProjectItem *subProjectItem = projectItem->child(j);

             // The header "Templates" has two subheaders that we want to keep
             if (subProjectItem->text() == "Templates") {
               if (subProjectItem->hasChildren()) {
                 for (int k=0; k < subProjectItem->rowCount(); k++) {
                   ProjectItem *tempProjectItem = subProjectItem->child(k);
                   while (tempProjectItem->hasChildren()) {
                       removeItem(tempProjectItem->child(0));
                   }
                 }
               }
             }
             else {
               while (subProjectItem->hasChildren()) {
                   removeItem(subProjectItem->child(0));
               }
             }
           }
         }
       }
     }
   }


}
