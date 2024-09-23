#include "ImageFileListWidget.h"

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFileDialog>
#include <QInputDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPoint>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSize>
#include <QToolBar>
#include <QVBoxLayout>
#include <QXmlStreamWriter>

#include "Directory.h"
#include "FileName.h"
#include "IException.h"
#include "Image.h"
#include "ImageTreeWidget.h"
#include "ImageTreeWidgetItem.h"
#include "IString.h"
#include "ProgressBar.h"
#include "Project.h"
#include "PvlObject.h"
#include "Shape.h"
#include "TextFile.h"

namespace Isis {

  /**
   * Constructor. This method sets up the QWidget to show the ImageTreeWidget that is created
   * using directory.
   *
   * @param directory The directory where you want the ImageTreeWidget to be made
   * @param parent The QWidget parent
   */
  ImageFileListWidget::ImageFileListWidget(Directory *directory,
                                           QWidget *parent) : QWidget(parent) {
    QWidget *treeWidget = new QWidget();
    m_directory = directory;
    QHBoxLayout *layout = new QHBoxLayout();

    m_tree = new ImageTreeWidget(directory);
    m_tree->setObjectName("Tree");
    layout->addWidget(m_tree);

    layout->setContentsMargins(0, 0, 0, 0);

    setWhatsThis("This is the image file list. Opened "
        "cubes show up here. You can arrange your cubes into groups (that you "
        "name) to help keep track of them. Also, you can configure multiple "
        "files at once. Finally, you can sort your files by any of the visible "
        "columns (use the view menu to show/hide columns of data).");

    treeWidget->setLayout(layout);

    m_progress = new ProgressBar;
    m_progress->setVisible(false);

    m_searchToolbar = new QToolBar("Search Tool", this);
    m_searchToolbar->setObjectName("Search Tool");
    m_searchToolbar->setWhatsThis("This contains all the fields for searching the active file list");

    m_searchLineEdit = new QLineEdit();
    QPushButton *okButton = new QPushButton("Search");
    connect(okButton, SIGNAL(clicked()), this, SLOT(filterFileList()));
    QPushButton *clearButton = new QPushButton("Clear");
    connect(clearButton, SIGNAL(clicked()), this, SLOT(clear()));
    QHBoxLayout *actionLayout = new QHBoxLayout();
    m_fileCount = new QLabel("File Matches: 0");
    actionLayout->addWidget(m_searchLineEdit);
    actionLayout->addWidget(okButton);
    actionLayout->addWidget(clearButton);
    actionLayout->addWidget(m_fileCount);
    actionLayout->addStretch(1);
    actionLayout->setMargin(0);
    QWidget *toolBarWidget = new QWidget;
    toolBarWidget->setLayout(actionLayout);
    m_searchToolbar->addWidget(toolBarWidget);
    QVBoxLayout *fileListWidgetLayout = new QVBoxLayout();
    fileListWidgetLayout->addWidget(m_searchToolbar);
    fileListWidgetLayout->addWidget(treeWidget);
    setLayout(fileListWidgetLayout);
  }

  /**
   * Destructor
   */
  ImageFileListWidget::~ImageFileListWidget() {
    delete m_progress;
    m_directory = NULL;
  }

  /**
   * This method returns the progress bar.
   *
   * @return @b QProgressBar * Returns the ProgressBar
   */
  QProgressBar *ImageFileListWidget::getProgress() {
    return m_progress;
  }

  /**
   * This method loads the state of this class from the pvl.
   *
   * @param pvl The pvl that we are reading the state from.
   *
   * @throws IException::Io "Unable to read image file's list widget settings from Pvl"
   */
  void ImageFileListWidget::fromPvl(PvlObject &pvl) {
    if (pvl.name() == "ImageFileList") {
      m_serialized.reset(new PvlObject(pvl));
      ImageTreeWidgetItem::TreeColumn col =
          ImageTreeWidgetItem::FootprintColumn;
      while (col < ImageTreeWidgetItem::BlankColumn) {
        IString key = ImageTreeWidgetItem::treeColumnToString(col).toStdString() + "Visible";
        key = key.Convert(" ", '_');

        if (pvl.hasKeyword(key)) {
          bool visible = toBool(QString::fromStdString(pvl[key][0]).toStdString());

          if (visible) {
            m_tree->showColumn(col);
          }
          else {
            m_tree->hideColumn(col);
          }
        }

        col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
      }

      m_tree->updateViewActs();
      m_tree->sortItems(pvl["SortColumn"], Qt::AscendingOrder);

      QList<QTreeWidgetItem *> allCubes;

      // Take all of the cubes out of the tree
      while (m_tree->topLevelItemCount() > 0) {
        QTreeWidgetItem *group = m_tree->takeTopLevelItem(0);
        allCubes.append(group->takeChildren());

        delete group;
        group = NULL;
      }

      // Now re-build the tree items
      for (int cubeGrp = 0; cubeGrp < pvl.objects(); cubeGrp ++) {
        PvlObject &cubes = pvl.object(cubeGrp);

        QTreeWidgetItem *newCubeGrp = m_tree->addGroup("", QString::fromStdString(cubes.name()));

        if (cubes.hasKeyword("Expanded")) {
          bool expanded = (cubes["Expanded"][0] != "No");
          newCubeGrp->setExpanded(expanded);
        }
      }

      if (allCubes.size()) {
        m_tree->addGroup("", "Unknown")->addChildren(allCubes);
      }
    }
    else {
      throw IException(IException::Io, "Unable to read image file's "
          "list widget settings from Pvl", _FILEINFO_);
    }
  }

  /**
   * This method writes the state of this class to a pvl.
   *
   * @return @b PvlObject The pvl that we are writing too.
   */
  PvlObject ImageFileListWidget::toPvl() const {
    PvlObject output("ImageFileList");

    ImageTreeWidgetItem::TreeColumn col =
        ImageTreeWidgetItem::FootprintColumn;
    while (col < ImageTreeWidgetItem::BlankColumn) {
      IString key = ImageTreeWidgetItem::treeColumnToString(col).toStdString() + "Visible";
      key = key.Convert(" ", '_');
      bool visible = !m_tree->isColumnHidden(col);

      output += PvlKeyword(key, toString((int)visible));
      col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
    }

    output += PvlKeyword("SortColumn", toString(m_tree->sortColumn()));

    // Now store groups and the cubes that are in those groups
    for (int i = 0; i < m_tree->topLevelItemCount(); i++) {
      QTreeWidgetItem *group = m_tree->topLevelItem(i);
      PvlObject cubeGroup(
          group->text(ImageTreeWidgetItem::NameColumn).toStdString());
      cubeGroup += PvlKeyword("Expanded", group->isExpanded() ? "Yes" : "No");

      for (int j = 0; j < group->childCount(); j++) {
        QTreeWidgetItem *item = group->child(j);

        if (item->type() == QTreeWidgetItem::UserType) {
          ImageTreeWidgetItem *cubeItem = (ImageTreeWidgetItem *)item;

          cubeGroup += PvlKeyword("Image", cubeItem->image()->id().toStdString());
        }
      }

      output += cubeGroup;
    }

    return output;
  }

  /**
   * This method calls ImageTreeWidget::actions() which sets up a QAction
   * that sets a default for the file list columns and returns a QList of
   * QPointers that point to the QAction.
   *
   * @return @b QList<QAction *> The QList of QPointers that point to the QAction set up by
   *            ImageTreeWidget::actions().
   */
  QList<QAction *> ImageFileListWidget::actions() {
    return m_tree->actions();
  }

  /**
   * This method calls ImageTreeWidget::getViewActions() which returns a
   * list of FootprintColumns.
   *
   * @return @b QList<QAction *> The QList of FootprintColumns.
   */
  QList<QAction *> ImageFileListWidget::getViewActions() {
    return m_tree->getViewActions();
  }

  /**
   * This method creates a new QAction which is connected and when triggered
   * will save the cube list.
   *
   * @return @b QList<QAction *> The QList of the new QAction.
   */
  QList<QAction *> ImageFileListWidget::getExportActions() {
    QList<QAction *> exportActs;

    QAction *saveList = new QAction(this);
    saveList->setText(
        "Save Entire Cube List (ordered by &file list/groups)...");
    connect(saveList, SIGNAL(triggered()), this, SLOT(saveList()));

    exportActs.append(saveList);

    return exportActs;
  }

  /**
   * This method creates a QWidget that displays a long help message explaining the tool.
   *
   * @return @b QWidget Returns the message QWidget that was made.
   */
  QWidget *ImageFileListWidget::getLongHelp(QWidget *fileListContainer) {
    QScrollArea *longHelpWidgetScrollArea = new QScrollArea;

    QWidget *longHelpWidget = new QWidget;
    longHelpWidgetScrollArea->setWidget(longHelpWidget);

    QVBoxLayout *longHelpLayout = new QVBoxLayout;
    longHelpLayout->setSizeConstraint(QLayout::SetFixedSize);
    longHelpWidget->setLayout(longHelpLayout);

    QLabel *title = new QLabel("<h2>Image File List</h2>");
    longHelpLayout->addWidget(title);

    QPixmap preview;
    if (!fileListContainer) {
//      QWeakPointer<ImageFileListWidget> tmp = new ImageFileListWidget;
//      tmp.data()->resize(QSize(500, 200));
//      preview = tmp.data().grab();
//      delete tmp.data();
      ImageFileListWidget *tmp = new ImageFileListWidget;
      tmp->resize(QSize(500, 200));
      preview = tmp->grab();
      delete tmp;
    }
    else {
      QPixmap previewPixmap = fileListContainer->grab().scaled(
          QSize(500, 200), Qt::KeepAspectRatio, Qt::SmoothTransformation);

      QLabel *previewWrapper = new QLabel;
      previewWrapper->setPixmap(previewPixmap);
      longHelpLayout->addWidget(previewWrapper);
    }

    QLabel *previewWrapper = new QLabel;
    previewWrapper->setPixmap(preview);
    longHelpLayout->addWidget(previewWrapper);

    QLabel *overview = new QLabel(tr("The mosaic file list is designed to help "
        "to organize your files within the %1 project. The file list supports changing multiple "
        "files simultaneously using the right-click menus after selecting "
        "several images or groups.<br>"
        "<h3>Groups</h3>"
            "<p>Every cube must be inside of a group. These groups can be "
            "renamed by double clicking on them. To move a cube between groups, "
            "click and drag it to the group you want it in. This works "
            "for multiple cubes also. You can change all of the cubes in a "
            "group by right clicking on the group name. You can add a group "
            "by right clicking in the white space below the last cube or on "
            "an existing group.</p>"
        "<h3>Columns</h3>"
            "Show and hide columns by using the view menu. These "
            "columns show relevant data about the cube, including statistical "
            "information. Some of this information will be blank if you do "
            "not run the application, <i>camstats</i>, before opening the cube."
        "<h3>Sorting</h3>"
            "Sort cubes within each group in ascending or descending order "
            "by clicking on the column "
            "title of the column that you want to sort on. Clicking on the "
            "title again will reverse the sorting order. You can also drag and "
            "drop a cube between two other cubes to change where it is in the "
            "list.").arg(QApplication::applicationName()));
    overview->setWordWrap(true);

    longHelpLayout->addWidget(overview);
    longHelpLayout->addStretch();

    return longHelpWidgetScrollArea;
  }

  /**
   * This method adds the new images to the tree.
   *
   * @param images ImageList containing all of the images that need to be added to the tree.
   */
  void ImageFileListWidget::addImages(ImageList *images) {
    m_progress->setText("Loading file list");
    m_progress->setRange(0, images->size() - 1);
    m_progress->setValue(0);
    m_progress->setVisible(true);

    QList<QTreeWidgetItem *> selected = m_tree->selectedItems();

    ImageList alreadyViewedImages = m_tree->imagesInView();


    // Expanded states are forgotten when removed from the tree; hence the bool.
    QList<QTreeWidgetItem *> groups;


    // It's very slow to add/insertChild() on tree items when they are in the tree, so let's
    //   take them out of the tree, call add/insertChild() over and over, then give the
    //   groups (tree items) back to the tree. Expanded states are lost... so save/restore them
    QVariant expandedStates = saveExpandedStates(m_tree->invisibleRootItem());
    while (m_tree->topLevelItemCount()) {
      groups.append(m_tree->takeTopLevelItem(0));
    }

    QTreeWidgetItem *selectedGroup = NULL;

    foreach (Image *image, *images) {
      if (alreadyViewedImages.indexOf(image) == -1) {
        ImageTreeWidget::ImagePosition pos;

        if (!m_serialized.isNull()) {
          pos = find(image);
        }

        QTreeWidgetItem *newImageItem = m_tree->prepCube(images, image);

        if (!pos.isValid()) {
          if (m_tree->groupInList(selected)) {
            QListIterator<QTreeWidgetItem *> it(selected);
            while (it.hasNext() && !selectedGroup) {
              QTreeWidgetItem *item = it.next();
              if (item->data(0, Qt::UserRole).toInt() == ImageTreeWidget::ImageGroupType) {
                selectedGroup = item;
              }
            }
          }

          if (!selectedGroup) {
            QTreeWidgetItem *imageListNameItem = NULL;
            QString groupName;

            if (images->name().isEmpty()) {
              imageListNameItem = m_tree->invisibleRootItem();
              groupName = QString("Group %1").arg(groups.count() + 1);
            }
            else {
              for (int i = 0; !imageListNameItem && i < groups.count(); i++) {
                QTreeWidgetItem *group = groups[i];
                if (group->data(0, Qt::UserRole).toInt() == ImageTreeWidget::ImageListNameType &&
                    group->text(0) == images->name()) {
                  imageListNameItem = group;
                }
              }

              if (!imageListNameItem) {
                imageListNameItem = m_tree->createImageListNameItem(images->name());
                groups.append(imageListNameItem);
              }
            }

            selectedGroup = m_tree->createGroup(imageListNameItem, groupName);
          }

          selectedGroup->addChild(newImageItem);
        }
        else {
          QTreeWidgetItem *groupItem = groups[pos.group()];
          if (groupItem->childCount() < pos.index())
            groupItem->addChild(newImageItem);
          else
            groupItem->insertChild(pos.index(), newImageItem);
        }
      }

      m_progress->setValue(m_progress->value() + 1);
    }

    foreach (QTreeWidgetItem *group, groups) {
      m_tree->addTopLevelItem(group);
    }
    restoreExpandedStates(expandedStates, m_tree->invisibleRootItem());

    if (selectedGroup)
      selectedGroup->setSelected(true);

    m_tree->refit();
    m_progress->setVisible(false);
  }

  /**
  * Removes an imagelist from the FileListWidget. This would normally be handled elsewhere
  * but for IPCE it needs a method to hide images and update the tree.
  *
  * @param ImageList containing the images to remove from the tree
  */
  void ImageFileListWidget::removeImages(ImageList* images){
    foreach (Image* image, *images) {

      for (int i = 0; i < m_tree->topLevelItemCount(); i++) {
        QTreeWidgetItem *group = m_tree->topLevelItem(i);

        for (int j = 0; j < group->childCount(); j++) {
          QTreeWidgetItem *item = group->child(j);

          if (item->type() == QTreeWidgetItem::UserType && !item->isHidden()) {
            ImageTreeWidgetItem *cubeItem = (ImageTreeWidgetItem *)item;
            if (cubeItem->image() == image){
              item->setHidden(true);
            }
          }
        }
      }
    }
    m_tree->repaint();
  }

  
  /**
   * This method takes an event and gets all of the FootprintColumns, adds them to
   * the menu, then it pops up the menu at the location of event.
   *
   * @param event A QContextMenuEvent that is used to find the position of the mouse.
   */
  void ImageFileListWidget::contextMenuEvent(QContextMenuEvent *event) {
    QMenu menu;

    QList<QAction *> actions = m_tree->getViewActions();

    foreach (QAction *act, actions) {
      if (act) {
        menu.addAction(act);
      }
      else {
        menu.addSeparator();
      }
    }

    menu.exec(event->globalPos());
  }

  /**
   * This method saves the list to the choosen output file.
   */
  void ImageFileListWidget::saveList() {
    QString output =
        QFileDialog::getSaveFileName((QWidget *)parent(),
          "Choose output file",
          QDir::currentPath() + "/files.lis",
          QString("List File (*.lis);;Text File (*.txt);;All Files (*.*)"));
    if (output.isEmpty()) return;

    TextFile file(output, "overwrite");

    for (int i = 0; i < m_tree->topLevelItemCount(); i++) {
      QTreeWidgetItem *group = m_tree->topLevelItem(i);

      for (int j = 0; j < group->childCount(); j++) {
        QTreeWidgetItem *item = group->child(j);

        if (item->type() == QTreeWidgetItem::UserType) {
          ImageTreeWidgetItem *cubeItem = (ImageTreeWidgetItem *)item;
          file.PutLine(cubeItem->image()->fileName());
        }
      }
    }
  }

  /**
   * This method takes an image and finds it's position.
   *
   * @param image The image we are trying to find.
   *
   * @return @b ImageTreeWidget::ImagePosition The position of the image.
   */
  ImageTreeWidget::ImagePosition ImageFileListWidget::find(const Image *image) const {
    QString id = image->id();

    ImageTreeWidget::ImagePosition result;
    for (int objIndex = 0; !result.isValid() && objIndex < m_serialized->objects(); objIndex++) {
      PvlObject &obj = m_serialized->object(objIndex);

      int imageKeyOffset = 0;
      for (int fileKeyIndex = 0;
           !result.isValid() && fileKeyIndex < obj.keywords();
           fileKeyIndex++) {
        PvlKeyword &key = obj[fileKeyIndex];

        if (key.isNamed("Image")) {
          if (key[0] == id.toStdString()) {
            result = ImageTreeWidget::ImagePosition(objIndex, imageKeyOffset);
          }
          else {
            imageKeyOffset++;
          }
        }
      }
    }

    return result;
  }

  /**
   * This method returns the QTreeWidgetItem to it's expanded state.
   *
   * @param expandedStates The expanded state that needs to be restored.
   * @param item The QTreeWidgetItem that will be restored to it's expanded state.
   */
  void ImageFileListWidget::restoreExpandedStates(QVariant expandedStates, QTreeWidgetItem *item) {
    QMap<QString, QVariant> states = expandedStates.toMap();

    if (states.contains("Expanded")) {
      item->setExpanded(states["Expanded"].toBool());
    }
    else {
      item->setExpanded(true);
    }

    QList<QVariant> childrenStates = states["Children"].toList();

    int i = 0;
    while (i < item->childCount() && i < childrenStates.count()) {
      restoreExpandedStates(childrenStates[i], item->child(i));
      i++;
    }

    while (i < item->childCount()) {
      restoreExpandedStates(QVariant(), item->child(i));
      i++;
    }
  }

  /**
   * This method saves the the expanded state of item.
   *
   * @param item The QTreeWidgetItem that we will save the expanded state of.
   *
   * @return @b QVariant The expanded state of item.
   */
  QVariant ImageFileListWidget::saveExpandedStates(QTreeWidgetItem *item) {
    QMap<QString, QVariant> result;

    result["Expanded"] = item->isExpanded();

    if (item->childCount()) {
      QList<QVariant> childrenStates;

      for (int i = 0; i < item->childCount(); i++) {
        childrenStates.append(saveExpandedStates(item->child(i)));
      }

      result["Children"] = childrenStates;
    }

    return result;
  }


  /**
   * This method saves the FootprintColumns in the project and the settings associated
   * with every column.
   *
   * @param stream The QXmlStreamWriter that saves the FootprintColumns.
   * @param project The current project being saved.
   * @param newProjectRoot This is not used.
   */
  void ImageFileListWidget::save(QXmlStreamWriter &stream, Project *project,
                                 FileName newProjectRoot) const {

    stream.writeStartElement("imageFileList");
    stream.writeAttribute("objectName", objectName());

    // Write QSettings
//  stream.writeStartElement("widgetGeometry");
//  QString geom = saveGeometry();
//  //qDebug()<<"ImageFileListWidget::save   geometry = "<<geom;
//  stream.writeAttribute("value", saveGeometry());
//  stream.writeEndElement();

//  stream.writeStartElement("geometry");
//  //qDebug()<<"ImageFileListWidget::save   Geometry = "<<geometry();
//  //qDebug()<<"ImageFileListWidget::save   saveGeometry = "<<saveGeometry();
//  stream.writeAttribute("x", QString::number(pos().x()));
//  stream.writeAttribute("y", QString::number(pos().y()));
//  stream.writeEndElement();
//
//  stream.writeStartElement("position");
//  //qDebug()<<"ImageFileListWidget::save   Position = "<<QVariant(pos()).toString();
//  stream.writeAttribute("x", QString::number(pos().x()));
//  stream.writeAttribute("y", QString::number(pos().y()));
//  stream.writeEndElement();
//
//  stream.writeStartElement("size");
//  //qDebug()<<"ImageFileListWidget::save   Size = "<<size();
//  stream.writeAttribute("width", QString::number(size().width()));
//  stream.writeAttribute("height", QString::number(size().height()));
//  stream.writeEndElement();
//  stream.writeEndElement();


    ImageTreeWidgetItem::TreeColumn col =
        ImageTreeWidgetItem::FootprintColumn;
    while (col < ImageTreeWidgetItem::BlankColumn) {
      bool visible = !m_tree->isColumnHidden(col);
      bool sorted = (m_tree->sortColumn() == col);

      stream.writeStartElement("column");
      stream.writeAttribute("name", ImageTreeWidgetItem::treeColumnToString(col));
      stream.writeAttribute("visible", (visible? "true" : "false"));
      stream.writeAttribute("sorted", (sorted? "true" : "false"));
      stream.writeEndElement();

      col = (ImageTreeWidgetItem::TreeColumn)(col + 1);
    }

    // Now store groups and the cubes that are in those groups
    save(stream, NULL);

    stream.writeEndElement();
  }


  /**
   * This method saves the QTreeWidgetItem and the settings associated with the QTreeWidgetItem
   * to the stream.
   *
   * @param stream The stream that is saving the QTreeWidgetItem and the settings.
   * @param itemToWrite The QTreeWidgetItem that is being saved.
   */
  void ImageFileListWidget::save(QXmlStreamWriter &stream, QTreeWidgetItem *itemToWrite) const {
    bool done = false;

    // Start the element - image or group with attributes
    if (!itemToWrite) {
      stream.writeStartElement("treeLayout");
    }
    else if (itemToWrite->type() == QTreeWidgetItem::UserType) {
      ImageTreeWidgetItem *imageItemToWrite = (ImageTreeWidgetItem *)itemToWrite;

      stream.writeStartElement("image");
      stream.writeAttribute("id", imageItemToWrite->image()->id());
    }
    else {
      bool groupIsImageList =
            (itemToWrite->data(0, Qt::UserRole).toInt() == ImageTreeWidget::ImageListNameType);

      stream.writeStartElement("group");
      stream.writeAttribute("name", itemToWrite->text(ImageTreeWidgetItem::NameColumn));
      stream.writeAttribute("expanded", itemToWrite->isExpanded() ? "true" : "false");
      stream.writeAttribute("isImageList", groupIsImageList? "true" : "false");
    }

    // Write any child XML elements (groups in groups)
    int i = 0;
    while (!done) {
      QTreeWidgetItem *childItemToWrite = NULL;

      if (itemToWrite == NULL && i < m_tree->topLevelItemCount()) {
        childItemToWrite = m_tree->topLevelItem(i);
      }
      else if (itemToWrite != NULL && i < itemToWrite->childCount()) {
        childItemToWrite = itemToWrite->child(i);
      }

      if (childItemToWrite) {
        save(stream, childItemToWrite);
      }

      done = (childItemToWrite == NULL);
      i++;
    }

    // Close the initial image or group element
    stream.writeEndElement();
  }


  void ImageFileListWidget::filterFileList() {
    QString filterString = m_searchLineEdit->text();
    int numMatches = 0;

    for (int i = 0; i < m_tree->topLevelItemCount(); i++) {
      QTreeWidgetItem *group = m_tree->topLevelItem(i);

      for (int j = 0; j < group->childCount(); j++) {
        QTreeWidgetItem *item = group->child(j);
        group->setSelected(false);
        if (item->type() == QTreeWidgetItem::UserType) {

          ImageTreeWidgetItem *cubeItem = (ImageTreeWidgetItem *)item;
          if (cubeItem->image()->fileName().contains(filterString)){
            cubeItem->setSelected(true);
            m_tree->scrollToItem(cubeItem);
            numMatches++;
          }
          else {
            cubeItem->setSelected(false);
          }
        }
      }
    }
    m_fileCount->setText("File Matches: " + QString::number(numMatches));
  }


  void ImageFileListWidget::clear() {
    m_searchLineEdit->setText("");
    m_fileCount->setText("File Matches: 0");

    for (int i = 0; i < m_tree->topLevelItemCount(); i++) {
      QTreeWidgetItem *group = m_tree->topLevelItem(i);

      for (int j = 0; j < group->childCount(); j++) {
        QTreeWidgetItem *item = group->child(j);
        group->setSelected(false);
        if (item->type() == QTreeWidgetItem::UserType) {
          item->setSelected(false);
        }
      }
    }
  }
}
