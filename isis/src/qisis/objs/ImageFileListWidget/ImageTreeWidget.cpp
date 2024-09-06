#include "ImageTreeWidget.h"

#include <algorithm>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDropEvent>
#include <QMenu>
#include <QProgressBar>
#include <QSettings>
#include <QScrollBar>
#include <QTime>

#include "Directory.h"
#include "IException.h"
#include "Image.h"
#include "ImageDisplayProperties.h"
#include "ImageList.h"
#include "ImageTreeWidgetItem.h"
#include "ProgressBar.h"

namespace Isis {

  /**
   * ImageTreeWidget constructor.
   * ImageTreeWidget is derived from QTreeWidget
   *
   *
   * @param parent
   */
  ImageTreeWidget::ImageTreeWidget(Directory *directory, QWidget *parent) : QTreeWidget(parent) {
    m_queuedSelectionChanged = false;
    m_directory = directory;

    QStringList header;

    ImageTreeWidgetItem::TreeColumn col = ImageTreeWidgetItem::NameColumn;
    while(col < ImageTreeWidgetItem::BlankColumn) {
      header << ImageTreeWidgetItem::treeColumnToString(col);
      col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
    }

    setHeaderLabels(header);

    hideColumn(ImageTreeWidgetItem::ImageColumn);
    hideColumn(ImageTreeWidgetItem::LabelColumn);
    hideColumn(ImageTreeWidgetItem::ResolutionColumn);
    hideColumn(ImageTreeWidgetItem::EmissionAngleColumn);
    hideColumn(ImageTreeWidgetItem::IncidenceAngleColumn);
    hideColumn(ImageTreeWidgetItem::PhaseAngleColumn);
    hideColumn(ImageTreeWidgetItem::AspectRatioColumn);
    hideColumn(ImageTreeWidgetItem::SampleResolutionColumn);
    hideColumn(ImageTreeWidgetItem::LineResolutionColumn);
    hideColumn(ImageTreeWidgetItem::NorthAzimuthColumn);
    hideColumn(ImageTreeWidgetItem::BlankColumn);

    // Read and apply default visibilities from config, if different than defaults
    QSettings settings(
        QString::fromStdString(FileName("$HOME/.Isis/" + QApplication::applicationName().toStdString() + "/fileList.config")
          .expanded()),
        QSettings::NativeFormat);
    settings.beginGroup("ColumnsVisible");

    col = ImageTreeWidgetItem::FootprintColumn;
    while(col < ImageTreeWidgetItem::BlankColumn) {
      bool visible = !isColumnHidden(col);

      if (settings.value(QString("%1Visible").arg(ImageTreeWidgetItem::treeColumnToString(col)),
                         visible).toBool() != visible) {
        if (visible)
          hideColumn(col);
        else
          showColumn(col);
      }

      col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
    }
    settings.endGroup();

    setContextMenuPolicy(Qt::DefaultContextMenu);

    setSortingEnabled(true);
    sortItems(ImageTreeWidgetItem::NameColumn, Qt::AscendingOrder);

    connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
            this, SLOT(onItemChanged(QTreeWidgetItem *, int)));
    connect(this, SIGNAL(itemSelectionChanged()),
            this, SLOT(onSelectionChanged()));

    // This is set up to do a single selection changed after all selections have changed, instead
    //   of 1 selection changed per 1 changed item.
    connect(this, SIGNAL(queueSelectionChanged()),
            this, SLOT(onQueuedSelectionChanged()), Qt::QueuedConnection);

    // Similar issue, but not as bad of a slow-down. Still enough to warrant the effort though.
    connect(this, SIGNAL(queueReadDisplayProperties()),
            this, SLOT(onQueuedReadDisplayProperties()), Qt::QueuedConnection);

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropOverwriteMode(false);
  }


  ImageTreeWidget::~ImageTreeWidget() {
    QAction *contextAction;
    foreach(contextAction, actions()) {
      removeAction(contextAction);
    }
  }


  QList<QAction *> ImageTreeWidget::actions() {

    QList<QAction *> results;
    if (!m_setFileListColsAct) {
      m_setFileListColsAct = new QAction("Set Current File List &Columns as Default", this);
      m_setFileListColsAct->setWhatsThis(tr("Use the currently visible columns in the file list as "
            "the default when no project has been opened"));
      connect(m_setFileListColsAct, SIGNAL(triggered(bool)),
              this, SLOT(setDefaultFileListCols()));
    }
    results.append(m_setFileListColsAct);

    return results;
  }

  
  QTreeWidgetItem *ImageTreeWidget::addGroup(QString imageListName, QString groupName, int index) {
    if (index >= 0) {
      disableSort();
    }
    QTreeWidgetItem *imageList = imageListTreeItem(imageListName);
    QTreeWidgetItem *group = createGroup(imageList, groupName, index);

    verticalScrollBar()->setMaximum(verticalScrollBar()->maximum() + 25);
    return group;
  }


  QTreeWidgetItem *ImageTreeWidget::createGroup(QTreeWidgetItem *imageListItem, QString groupName,
                                                int index) {
    if (!imageListItem)
      imageListItem = invisibleRootItem();

    if (groupName.isEmpty()) {
      groupName = QString("Group %1").arg(imageListItem->childCount() + 1);
    }

    QTreeWidgetItem *result = new QTreeWidgetItem;
    result->setText(0, groupName);
    result->setFlags(Qt::ItemIsEditable |
                    Qt::ItemIsUserCheckable |
                    Qt::ItemIsEnabled |
                    Qt::ItemIsSelectable |
                    Qt::ItemIsDragEnabled |
                    Qt::ItemIsDropEnabled);
    result->setData(0, Qt::UserRole, ImageGroupType);

    if (index == -1) {
      index = imageListItem->childCount();
    }

    imageListItem->insertChild(index, result);

    // This needs to be after the insert child call
    result->setExpanded(true);

    return result;
  }


  QTreeWidgetItem *ImageTreeWidget::createImageListNameItem(QString name) {
    QTreeWidgetItem *result = new QTreeWidgetItem;
    result->setText(0, name);
    result->setData(0, Qt::UserRole, ImageListNameType);
    result->setExpanded(true);

    return result;
  }


  void ImageTreeWidget::refit() {
    ImageTreeWidgetItem::TreeColumn col = ImageTreeWidgetItem::NameColumn;
    while(col < ImageTreeWidgetItem::BlankColumn) {
      // Resize the name column to the expanded version's data.
      if (col == ImageTreeWidgetItem::NameColumn) {
        QList<bool> expandedStates;
        for (int groupIndex = 0; groupIndex < topLevelItemCount(); groupIndex++) {
          expandedStates.append(topLevelItem(groupIndex)->isExpanded());
          topLevelItem(groupIndex)->setExpanded(true);
        }

        resizeColumnToContents(col);

        for (int groupIndex = 0; groupIndex < topLevelItemCount(); groupIndex++) {
          topLevelItem(groupIndex)->setExpanded(expandedStates[groupIndex]);
        }
      }
      else {
        resizeColumnToContents(col);
      }

      col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
    }
  }


  void ImageTreeWidget::disableSort() {
    sortItems(ImageTreeWidgetItem::BlankColumn, Qt::AscendingOrder);
  }


  void ImageTreeWidget::enqueueReadDisplayProperties(ImageTreeWidgetItem *item) {
    m_queuedReadDisplayPropertiesItems.append(item);

    if (m_queuedReadDisplayPropertiesItems.count() == 1) {
      emit queueReadDisplayProperties();
    }
  }


  QList<QAction *> ImageTreeWidget::getViewActions() {
    if (m_viewActs.empty()) {
      ImageTreeWidgetItem::TreeColumn col =
          ImageTreeWidgetItem::FootprintColumn;

      while(col < ImageTreeWidgetItem::BlankColumn) {
        QAction *showHide = new QAction("", this);
        showHide->setData(col);

        connect(showHide, SIGNAL(triggered()),
                this, SLOT(toggleColumnVisible()));

        m_viewActs.append(showHide);

        col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
      }

      updateViewActs();
    }

    return m_viewActs;
  }


  void ImageTreeWidget::updateViewActs() {
    if (!m_viewActs.empty()) {
      int viewActIndex = 0;
      ImageTreeWidgetItem::TreeColumn col =
          ImageTreeWidgetItem::FootprintColumn;

      while(col < ImageTreeWidgetItem::BlankColumn) {
        bool visible = !isColumnHidden(col);
        QAction *showHide = m_viewActs[viewActIndex];

        if (!visible) {
          showHide->setText("Show " +
              ImageTreeWidgetItem::treeColumnToString(col) + " Column");
        }
        else {
          showHide->setText("Hide " +
              ImageTreeWidgetItem::treeColumnToString(col) + " Column");
        }

        col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
        viewActIndex ++;
      }
    }
  }


  /** 
   *  
   */  
  void ImageTreeWidget::setDefaultFileListCols() {
    QSettings settings(
        QString::fromStdString(FileName("$HOME/.Isis/" +  QApplication::applicationName().toStdString() + "/fileList.config")
          .expanded()),
        QSettings::NativeFormat);
    settings.beginGroup("ColumnsVisible");

    ImageTreeWidgetItem::TreeColumn col =
        ImageTreeWidgetItem::FootprintColumn;

    while(col < ImageTreeWidgetItem::BlankColumn) {
      bool visible = !isColumnHidden(col);

      settings.setValue(QString("%1Visible").arg(ImageTreeWidgetItem::treeColumnToString(col)),
                        visible);

      col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
    }

    settings.endGroup();
  }


  ImageTreeWidgetItem *ImageTreeWidget::prepCube(ImageList *imageList, Image *image) {
    try {
      ImageTreeWidgetItem *item = new ImageTreeWidgetItem(imageList, image);

      connect(image->displayProperties(), SIGNAL(destroyed(QObject *)),
              this, SLOT(imageDeleted(QObject *)));
      connect(image->displayProperties(), SIGNAL(propertyChanged(DisplayProperties *)),
              this, SLOT(propertiesChanged(DisplayProperties *)));

      m_displayPropsToTreeItemLookup[image->displayProperties()] = item;

      return item;
    }
    catch(IException &e) {
      e.print();
    }

    return NULL;
  }


  ImageList ImageTreeWidget::imagesInView() {
    ImageList results;

    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::UserFlag);

    while(*it) {
      QTreeWidgetItem *item = *it;

      if (item->type() == QTreeWidgetItem::UserType) {
        ImageTreeWidgetItem *mosItem = (ImageTreeWidgetItem *)item;
        results.append(mosItem->image());
      }

      it ++;
    }

    return results;
  }


  ImageTreeWidget::ImagePosition::ImagePosition() {
    init();
  }


  ImageTreeWidget::ImagePosition::ImagePosition(int outerIndex, int innerIndex) {
    init();

    setPosition(outerIndex, innerIndex);
  }


  ImageTreeWidget::ImagePosition::ImagePosition(const ImagePosition &other) :
      m_group(other.m_group), m_index(other.m_index) {
  }


  ImageTreeWidget::ImagePosition::~ImagePosition() {
    init();
  }


  void ImageTreeWidget::ImagePosition::setPosition(int group, int index) {
    if (group >= 0 && index >= 0) {
      m_group = group;
      m_index = index;
    }
    else {
      init();
    }
  }


  int ImageTreeWidget::ImagePosition::group() const {
    return m_group;
  }


  int ImageTreeWidget::ImagePosition::index() const {
    return m_index;
  }


  bool ImageTreeWidget::ImagePosition::isValid() const {
    return (m_group != -1 && m_index != -1);
  }


  void ImageTreeWidget::ImagePosition::swap(ImagePosition &other) {
    std::swap(m_group, other.m_group);
    std::swap(m_index, other.m_index);
  }


  bool ImageTreeWidget::ImagePosition::operator<(const ImagePosition &rhs) {
    bool lessThan = false;
    if (group() < rhs.group()) {
      lessThan = true;
    }
    else if (group() == rhs.group() && index() < rhs.index()) {
      lessThan = true;
    }

    return lessThan;
  }


  ImageTreeWidget::ImagePosition &ImageTreeWidget::ImagePosition::operator=(
      const ImagePosition &rhs) {
    ImagePosition copy(rhs);
    swap(copy);

    return *this;
  }


  void ImageTreeWidget::ImagePosition::init() {
    m_group = -1;
    m_index = -1;
  }

  void ImageTreeWidget::dropEvent(QDropEvent *event) {
    QTreeWidgetItem *droppedAtItem = itemAt(event->pos());

    if(droppedAtItem) {
      disableSort();
      QTreeWidgetItem *droppedAtGroup = droppedAtItem;

      if(droppedAtGroup->type() != QTreeWidgetItem::Type)
        droppedAtGroup = droppedAtItem->parent();

      bool draggedGroup = groupInList(selectedItems());
      bool draggedItem = mosaicItemInList(selectedItems());

      // Moving items around...
      if(draggedItem && !draggedGroup) {
        int insertPosition = 0;

        if(droppedAtGroup != droppedAtItem) {
          insertPosition = droppedAtGroup->indexOfChild(droppedAtItem) + 1;
        }

        QTreeWidgetItem *toBeMoved;
        foreach(toBeMoved, selectedItems()) {
          if(toBeMoved != droppedAtItem) {
            QTreeWidgetItem *parent = toBeMoved->parent();

            // We need to make sure we handle moving a child within the
            //   same group and how that effects the insert position
            int childOrigIndex = parent->indexOfChild(toBeMoved);

            parent->takeChild(childOrigIndex);

            int actualInsertPos = insertPosition;

            if(parent == droppedAtGroup && childOrigIndex < insertPosition)
              actualInsertPos --;

            droppedAtGroup->insertChild(actualInsertPos, toBeMoved);

            // This makes multiple items dragged to bottom of group work
            if(insertPosition != droppedAtGroup->childCount())
              insertPosition ++;
          }
        }
      }
      else if(!draggedItem && draggedGroup) {
        QTreeWidgetItem *toBeMoved;

        foreach(toBeMoved, selectedItems()) {
          if(toBeMoved != droppedAtGroup) {
            int dropPosition = indexOfTopLevelItem(droppedAtGroup);

            takeTopLevelItem(indexOfTopLevelItem(toBeMoved));
            insertTopLevelItem(dropPosition, toBeMoved);
          }
        }
      }
    }
  }

  /**
   * This is why we needed to subclass the QTreeWidget class.
   * We needed our own dropEvent for the dragging and dropping
   * of the tree widget items. 
   *  
   * @internal 
   *   @history 2013-07-02 Tracie Sucharski - Replaced this method with the old drop method
   *                           from MosaiceTreeWidget.cpp.  This method (and class) needs some
   *                           refactoring to work with both qmos and a.out.
   */
//void ImageTreeWidget::dropEvent(QDropEvent *event) {
//  QTreeWidgetItem *droppedAtItem = itemAt(event->pos());
//
//  if (droppedAtItem) {
//    disableSort();
//    QTreeWidgetItem *droppedAtList  = droppedAtItem;
//    QTreeWidgetItem *droppedAtGroup = droppedAtItem;
//
//    // Try to figure out which list / group they dropped into
//    while (droppedAtList->data(0, Qt::UserRole).toInt() != ImageListNameType &&
//           droppedAtList != invisibleRootItem()) {
//      droppedAtList = droppedAtList->parent();
//
//      if (!droppedAtList)
//        droppedAtList = invisibleRootItem();
//    }
//
//    while (droppedAtGroup && droppedAtGroup->data(0, Qt::UserRole).toInt() != ImageGroupType) {
//      droppedAtGroup = droppedAtGroup->parent();
//    }
//
//    QString droppedListName = droppedAtList->text(0);
//
//    // Figure out from which list they're moving items from (to verify they aren't moving
//    //   between file lists)
//    QTreeWidgetItem *firstItemToBeMoved = NULL;
//    QString draggedListName;
//
//    if (selectedItems().count()) {
//      firstItemToBeMoved = selectedItems()[0];
//
//      if (firstItemToBeMoved->data(0, Qt::UserRole).toInt() == ImageGroupType) {
//        draggedListName = firstItemToBeMoved->parent()->text(0);
//      }
//      else if (firstItemToBeMoved->type() == QTreeWidgetItem::UserType) {
//        draggedListName = ((ImageTreeWidgetItem *)firstItemToBeMoved)->imageListName();
//      }
//      else {
//        firstItemToBeMoved = NULL;
//      }
//    }
//
//    bool draggedGroup = groupInList(selectedItems());
//    bool draggedItem = mosaicItemInList(selectedItems());
//
//    if (firstItemToBeMoved && (draggedListName == droppedListName || droppedListName.isEmpty())) {
//      // If they dropped images into a list item (but not a group), consider it a new group in
//      //   the list
//      if (droppedAtList && !droppedAtGroup && !draggedGroup && draggedItem) {
//        droppedAtGroup = addGroup(droppedListName);
//      }
//
//      // Moving items around...
//      if (draggedItem && !draggedGroup) {
//        int insertPosition = 0;
//
//        if (droppedAtGroup != droppedAtItem) {
//          insertPosition = droppedAtGroup->indexOfChild(droppedAtItem) + 1;
//        }
//
//        QTreeWidgetItem *toBeMoved;
//        foreach(toBeMoved, selectedItems()) {
//          if (toBeMoved != droppedAtItem) {
//            QTreeWidgetItem *parent = toBeMoved->parent();
//
//            // We need to make sure we handle moving a child within the
//            //   same group and how that effects the insert position
//            int childOrigIndex = parent->indexOfChild(toBeMoved);
//
//            parent->takeChild(childOrigIndex);
//
//            int actualInsertPos = insertPosition;
//
//            if (parent == droppedAtGroup && childOrigIndex < insertPosition)
//              actualInsertPos --;
//
//            droppedAtGroup->insertChild(actualInsertPos, toBeMoved);
//
//            // This makes multiple items dragged to bottom of group work
//            if (insertPosition != droppedAtGroup->childCount())
//              insertPosition ++;
//          }
//        }
//      }
//      else if (!draggedItem && draggedGroup && droppedAtGroup) {
//        QTreeWidgetItem *toBeMoved;
//
//        foreach(toBeMoved, selectedItems()) {
//          if (toBeMoved != droppedAtGroup) {
//            int dropPosition = droppedAtList->indexOfChild(droppedAtGroup);
//
//            droppedAtList->takeChild(droppedAtList->indexOfChild(toBeMoved));
//            droppedAtList->insertChild(dropPosition, toBeMoved);
//          }
//        }
//      }
//    }
//  }
//}


  /**
   * This is re-implemented to make right clicks on white space also unselect
   *   the current selection.
   */
  void ImageTreeWidget::mousePressEvent(QMouseEvent *event) {
    switch(event->type()) {
      case QEvent::MouseButtonPress: {
        if (event->button() == Qt::RightButton &&
           itemAt(event->pos()) == NULL) {
          setCurrentItem(NULL);
        }
        break;
      }

      default:
        break;
    }

    QTreeWidget::mousePressEvent(event);
  }


  void ImageTreeWidget::contextMenuEvent(QContextMenuEvent *event) {
    ImageList selectedCubes = selectedDisplays();

    bool selectedGroup = groupInList(selectedItems());
    bool selectedCube = mosaicItemInList(selectedItems());

    Project *project = NULL;

    if (m_directory)
      project = m_directory->project();

    QList<QAction *> supportedActions = selectedCubes.supportedActions(project);

    if (m_directory) {
      supportedActions.append(NULL);
      supportedActions.append(m_directory->supportedActions(&selectedCubes));
    }

    QMenu menu;
    foreach(QAction *action, supportedActions) {
      if (action != NULL)
        menu.addAction(action);
      else
        menu.addSeparator();
    }

    menu.addSeparator();

    if (!selectedCube && selectedGroup) {
      if (selectedItems().size() == 1 && selectedItems()[0]->childCount()) {
        QAction *close = menu.addAction("Close Cubes in Group");
        connect(close, SIGNAL(triggered()),
                this, SLOT(requestCloseSelected()));
      }

      if (selectedItems().size() == 1) {
        QAction *rename = menu.addAction("Rename Group");
        connect(rename, SIGNAL(triggered()),
                this, SLOT(renameSelectedGroup()));

        QAction *group = menu.addAction("Insert Group");
        connect(group, SIGNAL(triggered()),
                this, SLOT(addGroup()));
      }

      QAction *removeGroup = NULL;

      if (selectedItems().size() == 1)
        removeGroup = menu.addAction("Delete Group");
      else
        removeGroup = menu.addAction("Delete Groups");

      connect(removeGroup, SIGNAL(triggered()),
              this, SLOT(deleteSelectedGroups()));
    }

    if (selectedCube && !selectedGroup) {
      QAction *close = NULL;

      if (selectedItems().size() == 1)
        close = menu.addAction("Close Cube");
      else
        close = menu.addAction("Close Cubes");

      connect(close, SIGNAL(triggered()),
              this, SLOT(requestCloseSelected()));
    }

    if (!selectedCube && !selectedGroup) {
      QAction *group = menu.addAction("Add Group");

      connect(group, SIGNAL(triggered()),
              this, SLOT(addGroup()));
    }

    menu.exec(event->globalPos());
  }


  /**
  * 
  * 
  * @internal
  *   @history 2013-07-02 Tracie Sucharski - Returned to old qmos functionality.  Fixes #1697.
  */
  QTreeWidgetItem *ImageTreeWidget::addGroup() {
    int index = -1;

    QTreeWidgetItem *newGroupParent = NULL;
    QString newGroupParentText = "";

    if (selectedItems().size() == 1) {
      if (selectedItems()[0]->data(0, Qt::UserRole).toInt() == ImageGroupType) {
        newGroupParent = selectedItems()[0]->parent();
        if (newGroupParent) {
          index = newGroupParent->indexOfChild(selectedItems()[0]);
        }
        else {
          newGroupParent = invisibleRootItem();
          index = indexOfTopLevelItem(selectedItems()[0]);
        }
      }
      else if (selectedItems()[0]->data(0, Qt::UserRole).toInt() == ImageListNameType) {
        newGroupParent = selectedItems()[0];
      }
    }

    if (newGroupParent) {
      newGroupParentText = newGroupParent->text(0);
    }
    return addGroup(newGroupParentText, "", index);
  }


  void ImageTreeWidget::deleteSelectedGroups() {
    // Close the cubes in these groups
    requestCloseSelected();

    QTreeWidgetItem *toBeDeleted;
    foreach(toBeDeleted, selectedItems()) {
      if (toBeDeleted->type() == QTreeWidgetItem::Type) {
        delete toBeDeleted;
      }
    }
  }


  void ImageTreeWidget::imageDeleted(QObject *imageObj) {
    ImageDisplayProperties *imagedisp = (ImageDisplayProperties *)imageObj;
    ImageTreeWidgetItem *item = treeItem(imagedisp);

    if (item && item->parent()) {
      m_displayPropsToTreeItemLookup.remove(imagedisp);

      item->forgetImage();
      delete item->parent()->takeChild( item->parent()->indexOfChild(item) );
    }
  }


  void ImageTreeWidget::onItemChanged(QTreeWidgetItem *item, int) {
    if (item->type() == QTreeWidgetItem::UserType) {
      ((ImageTreeWidgetItem *)item)->update(true);
    }
  }


  void ImageTreeWidget::onSelectionChanged() {
    if (!m_queuedSelectionChanged) {
      emit queueSelectionChanged();
      m_queuedSelectionChanged = true;
    }
  }


  void ImageTreeWidget::onQueuedReadDisplayProperties() {
    QTreeWidgetItem *newlySelectedItem = NULL;

    foreach (ImageTreeWidgetItem *item, m_queuedReadDisplayPropertiesItems) {
      bool wasSelected = item->isSelected();
      item->update(false);

      if (!wasSelected && item->isSelected()) {
        newlySelectedItem = item;
      }
    }

    if (newlySelectedItem) {
      scrollToItem(newlySelectedItem);
    }

    m_queuedReadDisplayPropertiesItems.clear();
  }


  void ImageTreeWidget::onQueuedSelectionChanged() {
    // This boolean is very important to speed. Please test with 50,000+ images if/when considering
    //   removing it.
    m_queuedSelectionChanged = false;

    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::UserFlag);

    while(*it) {
      QTreeWidgetItem *item = *it;

      if (item->type() == QTreeWidgetItem::UserType) {
        ImageTreeWidgetItem *imageItem = (ImageTreeWidgetItem *)item;
        imageItem->update(true);
      }

      it ++;
    }

    updateDragAndDropability();
  }


  void ImageTreeWidget::propertiesChanged(DisplayProperties *changed) {
    enqueueReadDisplayProperties(treeItem(changed));
  }


  void ImageTreeWidget::renameSelectedGroup() {
    if (selectedItems().size() == 1 &&
       selectedItems()[0]->type() == QTreeWidgetItem::Type) {
      QTreeWidgetItem *groupToEdit = selectedItems()[0];
      editItem(groupToEdit);
    }
  }


  void ImageTreeWidget::requestCloseSelected() {
    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::UserFlag);

    while(*it) {
      QTreeWidgetItem *item = *it;

      // We get at least one item of the wrong type
      if (item->type() == QTreeWidgetItem::UserType) {
        if (item->isSelected() || item->parent()->isSelected()) {
          ImageTreeWidgetItem *mosItem = (ImageTreeWidgetItem *)item;
          mosItem->image()->deleteLater();
        }
      }

      it ++;
    }
  }


  void ImageTreeWidget::toggleColumnVisible() {
    int column = ((QAction *)sender())->data().toInt();
    setColumnHidden(column, !isColumnHidden(column));

    if (!m_viewActs.empty())
      updateViewActs();

    refit();
  }


  void ImageTreeWidget::updateDragAndDropability() {
    QList<QTreeWidgetItem *> selected = selectedItems();
    bool selectedGroup = groupInList(selected);
    bool selectedItem = mosaicItemInList(selected);

    if (selectedGroup && selectedItem) {
      setDragDropMode(QAbstractItemView::NoDragDrop);
    }
    else {
      setDragDropMode(QAbstractItemView::DragDrop);
    }
  }


  bool ImageTreeWidget::groupInList(QList<QTreeWidgetItem *> items) {
    QListIterator<QTreeWidgetItem *> it(items);

    while (it.hasNext()) {
      QTreeWidgetItem *item = it.next();

      if (item->type() == QTreeWidgetItem::Type)
        return true;
    }

    return false;
  }


  bool ImageTreeWidget::mosaicItemInList(QList<QTreeWidgetItem *> items) {
    QListIterator<QTreeWidgetItem *> it(items);

    while (it.hasNext()) {
      QTreeWidgetItem *item = it.next();

      if (item->type() == QTreeWidgetItem::UserType)
        return true;
    }

    return false;
  }

  ImageList ImageTreeWidget::selectedDisplays() {
    ImageList selected;
    QTreeWidgetItemIterator it(this, QTreeWidgetItemIterator::UserFlag);

    while(*it) {
      QTreeWidgetItem *item = *it;

      // We get at least one item of the wrong type
      if (item->type() == QTreeWidgetItem::UserType) {
        ImageTreeWidgetItem *mosItem = (ImageTreeWidgetItem *)item;
        if (mosItem->isSelected() || mosItem->parent()->isSelected()) {
          selected.append(mosItem->image());
        }
      }

      it ++;
    }

    return selected;
  }


  /**
   * This will get the image list tree item for the given image list (by name). This requires
   *   unique names for the image lists. If the image list name is an empty string, this will
   *   return the invisible root item (qmos). If the image list name isn't blank, but there is no
   *   such image list item in the tree, then this will create one, add it, and return it.
   *
   * @param imageListName The name of the image list to get the tree item for
   */
  QTreeWidgetItem *ImageTreeWidget::imageListTreeItem(QString imageListName) {
    QTreeWidgetItem *result = NULL;

    if (imageListName.isEmpty()) {
      result = invisibleRootItem();
    }
    else {
      QTreeWidgetItemIterator it(this);

      while(*it && !result) {
        QTreeWidgetItem *item = *it;

        if (item->data(0, Qt::UserRole).toInt() == ImageListNameType) {
          if (item->text(0) == imageListName) {
            result = item;
          }
        }

        it ++;
      }
    }

    if (!result) {
      result = createImageListNameItem(imageListName);
      addTopLevelItem(result);
    }

    return result;
  }


  ImageTreeWidgetItem *ImageTreeWidget::treeItem(DisplayProperties *displayProps) {
    return m_displayPropsToTreeItemLookup[displayProps];
  }


  ImageTreeWidgetItem *ImageTreeWidget::treeItem(Image *image) {
    return treeItem(image->displayProperties());
  }
}
