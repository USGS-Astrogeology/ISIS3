/**
 * @file
 * $Date$
 * $Revision$
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

#include "ProjectItem.h"

#include <QBrush>
#include <QIcon>
#include <QStandardItem>
#include <QVariant>
#include <QDebug>

#include "BundleResults.h"
#include "BundleSolutionInfo.h"
#include "Control.h"
#include "ControlList.h"
#include "CorrelationMatrix.h"
#include "Image.h"
#include "ImageList.h"
#include "Project.h"
#include "ProjectItemModel.h"
#include "Shape.h"
#include "ShapeList.h"
#include "Template.h"
#include "TemplateList.h"

namespace Isis {
  /**
   * Constructs an item without children, a parent, or a model.
   */
  ProjectItem::ProjectItem() : QStandardItem() {
    setTextColor(Qt::black);
    setEditable(false);
  }


  /**
   * Contructs a copy of another item. The copy will have the same text,
   * icon, and data, and copies of the children. The copy will not have a
   * parent or a model.
   *
   * @param[in] item (ProjectItem *) The item to copy.
   */
  ProjectItem::ProjectItem(ProjectItem *item) {
    setTextColor(Qt::black);
    item->setEditable(false);
    setProjectItem(item);
    for (int i=0; i < item->rowCount(); i++) {
      appendRow(new ProjectItem( item->child(i) ) );
    }
  }


  /**
   * Constructs an item representing a file in the filesystem.
   *
   * @param[in] filename The full path to the file in the filesystem
   * @param[in] treetext The name displayed in the project tree
   * @param[in] filename A icon to display next to the treetext
   */
  ProjectItem::ProjectItem(FileItemQsp filename, QString treeText, QIcon icon) {
    setTextColor(Qt::black);
    setEditable(false);
    setData(QVariant::fromValue<FileItemQsp>(filename));
    setText(treeText);
    setIcon(icon);
  }


  /**
   * Constructs an item representing a file in the filesystem.
   *
   * @param[in] filename The full path to the file in the filesystem
   * @param[in] treetext The name displayed in the project tree
   * @param[in] filename A icon to display next to the treetext
   */
  ProjectItem::ProjectItem(FileItemQsp filename, QString treeText, QString toolTipText,
                           QIcon icon) {
    setTextColor(Qt::black);
    setEditable(false);
    setData(QVariant::fromValue<FileItemQsp>(filename));
    setText(treeText);
    setToolTip(toolTipText);
    setIcon(icon);
  }


  /**
   * Constructs an item from a BundleResults.
   *
   * @param[in] bundleResults (BundleResults) The BundleResults to
   *                                          construct from.
   */
  ProjectItem::ProjectItem(BundleResults bundleResults) {
    setTextColor(Qt::black);
    setEditable(false);
    setBundleResults(bundleResults);
  }


  /**
   * Constructs an item from a BundleSettings.
   *
   * @param[in] bundleSettings (BundleSettingsQsp) The BundleSettings to
   *                                               construct from.
   */
  ProjectItem::ProjectItem(BundleSettingsQsp bundleSettings) {
    setTextColor(Qt::black);
    setEditable(false);
    setBundleSettings(bundleSettings);
  }


  /**
   * Constructs an item from a BundleSolutionInfo.
   *
   * @param[in] bundleSolutionInfo (BundleSolutionInfo *) The BundleSolutionInfo
   *                                                      to construct from.
   */
  ProjectItem::ProjectItem(BundleSolutionInfo *bundleSolutionInfo) {
    setTextColor(Qt::black);
    setEditable(false);
    setBundleSolutionInfo(bundleSolutionInfo);

    appendRow( new ProjectItem( bundleSolutionInfo->bundleSettings() ) );
    appendRow( new ProjectItem(bundleSolutionInfo->control()) );
    appendRow( new ProjectItem( bundleSolutionInfo->bundleResults() ) );
    if (!bundleSolutionInfo->adjustedImages().isEmpty()) {
      appendRow( new ProjectItem( bundleSolutionInfo->adjustedImages() ) );
    }
  }


  /**
   * Constructs an item from a Control.
   *
   * @param[in] control (Control *) The Control to construct from.
   */
  ProjectItem::ProjectItem(Control *control) {
    setTextColor(Qt::black);
    setEditable(false);
    setControl(control);
  }


  /**
   * Constructs an item from a ControlList.
   *
   * @param[in] controlList (ControlList *) The ControlList to construct from.
   */
  ProjectItem::ProjectItem(ControlList *controlList) {
    setTextColor(Qt::black);
    setEditable(false);
    setControlList(controlList);
    foreach (Control *control, *controlList) {
      appendRow( new ProjectItem(control) );
    }
  }


  /**
   * Constructs an item from a list of ControlList.
   *
   * @param[in] controls (QList<ControlList *>) The list to construct from.
   */
  ProjectItem::ProjectItem(QList<ControlList *> controls) {
    setTextColor(Qt::black);
    setEditable(false);
    setControls();
    foreach (ControlList *controlList, controls) {
      appendRow( new ProjectItem(controlList) );
    }
  }


  /**
   * Constructs an item from a CorrelationMatrix.
   *
   * @param[in] correlationMatrix (CorrelationMatrix) The CorrelationMatrix to
   *                                                  construct from.
   */
  ProjectItem::ProjectItem(CorrelationMatrix correlationMatrix) {
    setTextColor(Qt::black);
    setEditable(false);
    setCorrelationMatrix(correlationMatrix);
  }


  /**
   * Constructs an item from an Image.
   *
   * @param[in] image (Image *) The Image to construct from.
   */
  ProjectItem::ProjectItem(Image *image) {
    setTextColor(Qt::black);
    setEditable(true);
    setImage(image);
  }


  /**
   * Constructs an item from an ImageList.
   *
   * @param[in] imageList (ImageList *) The ImageList to construct from.
   */
  ProjectItem::ProjectItem(ImageList *imageList) {
    setTextColor(Qt::black);
    setEditable(true);
    setImageList(imageList);
    foreach (Image *image, *imageList) {
      appendRow( new ProjectItem(image) );
    }
  }


  /**
   * Constructs an item from a list of ImageList.
   *
   * @param[in] images (QList<ImageList *>) The list to construct from.
   */
  ProjectItem::ProjectItem(QList<ImageList *> images) {
    setTextColor(Qt::black);
    setEditable(false);
    setImages();
    foreach (ImageList *imageList, images) {
      appendRow( new ProjectItem(imageList) );
    }
  }


  /**
   * Constructs an item from an Shape.
   *
   * @param[in] shape (Shape *) The Shape to construct from.
   */
  ProjectItem::ProjectItem(Shape *shape) {
    setTextColor(Qt::black);
    setEditable(false);
    setShape(shape);
  }


  /**
   * Constructs an item from an ShapeList.
   *
   * @param[in] shapeList (ShapeList *) The ShapeList to construct from.
   */
  ProjectItem::ProjectItem(ShapeList *shapeList) {
    setTextColor(Qt::black);
    setEditable(false);
    setShapeList(shapeList);
    foreach (Shape *shape, *shapeList) {
      appendRow(new ProjectItem(shape));
    }
  }


  /**
   * Constructs an item from a list of ShapeList.
   *
   * @param[in] shapes (QList<ShapeList *>) The list to construct from.
   */
  ProjectItem::ProjectItem(QList<ShapeList *> shapes) {
    setTextColor(Qt::black);
    setEditable(false);
    setShapes();
    foreach (ShapeList *shapeList, shapes) {
      appendRow( new ProjectItem(shapeList) );
    }
  }


  /**
   * Constructs an item from a Template.
   *
   * @param[in] template (Template *) The Template to construct from.
   */
  ProjectItem::ProjectItem(Template *newTemplate) {
    setTextColor(Qt::black);
    setEditable(false);
    setTemplate(newTemplate);
  }


  /**
   * Constructs an item from an TemplateList.
   *
   * @param[in] templateList (TemplateList *) The TemplateList to construct from.
   */
  ProjectItem::ProjectItem(TemplateList *templateList) {

    setTextColor(Qt::black);
    setEditable(false);
    setTemplateList(templateList);
    foreach (Template *currentTemplate, *templateList) {
      appendRow(new ProjectItem(currentTemplate));
    }
  }


  /**
   * Constructs an item from a list of TemplateList.
   *
   * @param[in] shapes (QList<TemplateList *>) The list to construct from.
   */
  ProjectItem::ProjectItem(QList<TemplateList *> templates) {
    setTextColor(Qt::black);
    setEditable(false);
    setTemplates();
    foreach (TemplateList *templateList, templates) {
      appendRow( new ProjectItem(templateList) );
    }
  }

  /**
   * Constructs an item from a GuiCameraQsp
   *
   * @param[in] guiCamera (GuiCameraQsp) The camera to construct from.
   */
  ProjectItem::ProjectItem(GuiCameraQsp guiCamera) {
    setTextColor(Qt::black);
    setEditable(false);
    setGuiCamera(guiCamera);
  }


  /**
   * Constructs an item from a GuiCameraList.
   *
   * @param[in] guiCameraList (GuiCameraList *) The list of cameras to
   *                                            construct from.
   */
  ProjectItem::ProjectItem(GuiCameraList *guiCameraList) {
    setTextColor(Qt::black);
    setEditable(false);
    setGuiCameraList();
    foreach (GuiCameraQsp guiCamera, *guiCameraList) {
      appendRow( new ProjectItem(guiCamera) );
    }
  }


  /**
   * Constructs an item from a Project.
   *
   * @param[in] project (Project *) The Project to construct from.
   */
  ProjectItem::ProjectItem(Project *project) {
    setTextColor(Qt::black);
    setProject(project);
    appendRow( new ProjectItem( project->controls() ) );
    appendRow( new ProjectItem( project->images() ) );
    appendRow( new ProjectItem( project->shapes() ) );

    appendRow( new ProjectItem( project->templates() ) );

    ProjectItem *targetBodyListItem = new ProjectItem();
    targetBodyListItem->setTargetBodyList();
    appendRow(targetBodyListItem);

    ProjectItem *guiCameraListItem = new ProjectItem();
    guiCameraListItem->setGuiCameraList();
    appendRow(guiCameraListItem);

    ProjectItem *spaceCraftItem = new ProjectItem();
    spaceCraftItem->setSpacecraft();
    appendRow(spaceCraftItem);

    appendRow( new ProjectItem( project->bundleSolutionInfo() ) );
  }


  /**
   * Constructs an item from a list of BundleSolutionInfo.
   *
   * @param[in] results (QList<BundleSolutionInfo *>) The list to construct
   *                                                  from.
   */
  ProjectItem::ProjectItem(QList<BundleSolutionInfo *> results) {
    setTextColor(Qt::black);
    setEditable(false);
    setResults();
    foreach (BundleSolutionInfo *bundleSolutionInfo, results) {
      appendRow( new ProjectItem( bundleSolutionInfo) );
    }
  }


  /**
   * Constructs an item from a TargetBodyQsp.
   *
   * @param[in] targetBody (TargetBodyQsp) The target body to construct from.
   */
  ProjectItem::ProjectItem(TargetBodyQsp targetBody) {
    setTextColor(Qt::black);
    setEditable(false);
    setTargetBody(targetBody);
  }


  /**
   * Constructs an item from a TargetBodyList.
   *
   * @param[in] targetBodyList (TargetBodyList *) The list to construct from.
   */
  ProjectItem::ProjectItem(TargetBodyList *targetBodyList) {
    setTextColor(Qt::black);
    setEditable(false);
    setTargetBodyList();
    foreach (TargetBodyQsp targetBody, *targetBodyList) {
      appendRow( new ProjectItem(targetBody) );
    }
  }


  /**
   * Destructs a ProjectItem.
   */
  ProjectItem::~ProjectItem() {}


  /**
   * Returns the BundleResults stored in the data of the item.
   *
   * @return @b BundleResults The BundleResults of the item.
   */
  BundleResults ProjectItem::bundleResults() const {
    return data().value<BundleResults>();
  }


  /**
   * Returns the BundleSettings stored in the data of the item.
   *
   * @return @b BundleSettingsQsp The BundleSettings of the item.
   */
  BundleSettingsQsp ProjectItem::bundleSettings() const {
    return data().value<BundleSettingsQsp>();
  }


  /**
   * Returns the BundleSolutionInfo stored in the data of the item.
   *
   * @return @b BundleSolutionInfo* The BundleSolutionInfo of the item.
   */
  BundleSolutionInfo *ProjectItem::bundleSolutionInfo() const {
    return data().value<BundleSolutionInfo *>();
  }


  /**
   * Returns the Image stored in the data of the item.
   *
   * @return @b Image* The Image of the item.
   */
  Image *ProjectItem::image() const {
    return data().value<Image *>();
  }


  /**
   * Returns the ImageList stored in the data of the item.
   *
   * @return @b ImageList* The ImageList of the item.
   */
  ImageList *ProjectItem::imageList() const {
    return data().value<ImageList *>();
  }


  /**
   * Returns the Shape stored in the data of the item.
   *
   * @return (Shape *) The Shape of the item.
   */
  Shape *ProjectItem::shape() const {
    return data().value<Shape *>();
  }


  /**
   * Returns the ShapeList stored in the data of the item.
   *
   * @return (ShapeList *) The ShapeList of the item.
   */
  ShapeList *ProjectItem::shapeList() const {
    return data().value<ShapeList *>();
  }


  /**
   * Returns the Template stored in the data of the item.
   *
   * @return (Template *) The Template of the item.
   */
  Template *ProjectItem::getTemplate() const {
    return data().value<Template *>();
  }


  /**
   * Returns the TemplateList stored in the data of the item.
   *
   * @return (TemplateList *) The TemplateList of the item.
   */
  TemplateList *ProjectItem::templateList() const {
    return data().value<TemplateList *>();
  }


  /**
   * Returns the Control stored in the data of the item.
   *
   * @return @b Control* The Control of the item.
   */
  Control *ProjectItem::control() const {
    return data().value<Control *>();
  }


  /**
   * Returns the ControlList stored in the data of the item.
   *
   * @return @b ControlList* The ControlList of the item.
   */
  ControlList *ProjectItem::controlList() const {
    return data().value<ControlList *>();
  }


  /**
   * Returns the CorrelationMatrix stored the item.
   *
   * @return @b CorrelationMatrix* The CorrelationMatrix of the item.
   */
  CorrelationMatrix ProjectItem::correlationMatrix() const {
    return data().value<CorrelationMatrix>();
  }


  /**
   * Returns the Project stored in the data of the item.
   *
   * @return @b Project* The Project of the item.
   */
  Project *ProjectItem::project() const {
    return data().value<Project *>();
  }


  /**
   * Returns the GuiCameraQsp stored in the data of the item.
   *
   * @return @b GuiCameraQsp The camera stored in the item.
   */
  GuiCameraQsp ProjectItem::guiCamera() const {
    return data().value<GuiCameraQsp>();

  }


  /**
   * Returns the TargetBodyQsp stored in the data of the item.
   *
   * @return @b TargetBodyQsp The target body stored in the item.
   */
  TargetBodyQsp ProjectItem::targetBody() const {
    return data().value<TargetBodyQsp>();
  }


  /**
   * Returns the FileItemQsp stored in the data of the item.
   *
   * @return @b FileItemQsp The filename stored in the item.
   */
  FileItemQsp ProjectItem::fileItem() const {
    return data().value<FileItemQsp>();
  }


  bool ProjectItem::isTemplate() const {
    return data().canConvert<Template *>();
  }


  /**
   * Returns true if BundleResults are stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If BundleResults are stored in the data of the item or not.
   */
  bool ProjectItem::isBundleResults() const {
    return data().canConvert<BundleResults>();
  }


  /**
   * Returns true if BundleSettings are stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If BundleSettings are stored in the data of the item or not.
   */
  bool ProjectItem::isBundleSettings() const {
    return data().canConvert<BundleSettingsQsp>();
  }


  /**
   * Returns true if a BundleSolutionInfo is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If a BundleSolutionInfo is stored in the data of the item or not.
   */
  bool ProjectItem::isBundleSolutionInfo() const {
    return data().canConvert<BundleSolutionInfo *>();
  }


  /**
   * Returns true if an Image is stored in the data of the item. Returns false
   * otherwise.
   *
   * @return @b bool If an Image is stored in the data of the item or not.
   */
  bool ProjectItem::isImage() const {
    return data().canConvert<Image *>();
  }


  /**
   * Returns true if an ImageList is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If an ImageList is stored in the data of the item or not.
   */
  bool ProjectItem::isImageList() const {
    return data().canConvert<ImageList *>();
  }


  /**
   * Returns true if an Shape is stored in the data of the item. Returns false
   * otherwise.
   *
   * @return (bool) If an Shape is stored in the data of the item or not.
   */
  bool ProjectItem::isShape() const {
    return data().canConvert<Shape *>();
  }


  /**
   * Returns true if an ShapeList is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return (bool) If an ShapeList is stored in the data of the item or not.
   */
  bool ProjectItem::isShapeList() const {
    return data().canConvert<ShapeList *>();
  }


  /**
   * Returns true if a Control is stored in the data of the item. Returns false
   * otherwise.
   *
   * @return @b bool If a Control is stored in the data of the item or not.
   */
  bool ProjectItem::isControl() const {
    return data().canConvert<Control *>();
  }


  /**
   * Returns true if a ControlList is stored in the data of the item.
   * Returns false
   * otherwise.
   *
   * @return @b bool If a ControlList is stored in the data of the item or not.
   */
  bool ProjectItem::isControlList() const {
    return data().canConvert<ControlList *>();
  }


  /**
   * Returns true if a CorrelationMatrix is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If a CorrelationMatrix is stored in the data of the item or not.
   */
  bool ProjectItem::isCorrelationMatrix() const {
    return data().canConvert<CorrelationMatrix>();
  }


  /**
   * Returns true if a Project is stored in the data of the item. Returns false
   * otherwise.
   *
   * @return @b bool If a Project is stored in the data of the item or not.
   */
  bool ProjectItem::isProject() const {
    return data().canConvert<Project *>();
  }


  /**
   * Returns true if a GuiCameraQsp is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If a GuiCameraQsp is stored in the item or not.
   */
  bool ProjectItem::isGuiCamera() const {
    return data().canConvert<GuiCameraQsp>();
  }


  /**
   * Returns true if a TargetBodyQsp is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If a TargetBodyQsp is stored in the item or not.
   */
  bool ProjectItem::isTargetBody() const {
    return data().canConvert<TargetBodyQsp>();
  }


  /**
   * Returns true if a FileItemQsp is stored in the data of the item.
   * Returns false otherwise.
   *
   * @return @b bool If a FileItemQsp is stored in the item or not.
   */
  bool ProjectItem::isFileItem() const {
    return data().canConvert<FileItemQsp>();
  }


  /**
   * Sets the text, icon, and data to those of another item.
   *
   * @param[in] item (ProjectItem *) The other item.
   */
  void ProjectItem::setProjectItem(ProjectItem *item) {
    setTextColor(Qt::black);
    setText( item->text() );
    setIcon( item->icon() );
    setData( item->data() );
    setEditable(item->isEditable());
  }


  /**
   * Sets the text, icon, and data corresponding to BundleResults.
   *
   * @param[in] bundleResults (BundleResults) The BundleResults.
   */
  void ProjectItem::setBundleResults(BundleResults bundleResults) {
    setTextColor(Qt::black);
    setText("Statistics");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/kchart.png")
                           .expanded())));
    setData( QVariant::fromValue<BundleResults>(bundleResults) );
  }


  /**
   * Sets the text, icon, and data corresponding to BundleSettings.
   *
   * @param[in] bundleSettings (BundleSettingsQsp) The BundleSettingsQsp.
   */
  void ProjectItem::setBundleSettings(BundleSettingsQsp bundleSettings) {
    setTextColor(Qt::black);
    setText("Settings");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/applications-system.png")
                           .expanded())));
    setData( QVariant::fromValue<BundleSettingsQsp>(bundleSettings) );
  }


  /**
   * Sets the text, icon, and data corresponding to a BundleSolutionInfo.
   *
   * @param[in] bundleSolutionInfo (BundleSolutionInfo *) The BundleSolutionInfo.
   */
  void ProjectItem::setBundleSolutionInfo(BundleSolutionInfo *bundleSolutionInfo) {
    setTextColor(Qt::black);
    if (bundleSolutionInfo->name() != "") {
      setText( bundleSolutionInfo->name() );
    }
    else {
      setText( bundleSolutionInfo->runTime() );
    }
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/kchart.png")
                           .expanded())));
    setData( QVariant::fromValue<BundleSolutionInfo *>(bundleSolutionInfo) );
  }


  /**
   * Sets the text, icon, and data corresponding to an Image.
   *
   * @param[in] image (Image *) The Image.
   */
  void ProjectItem::setImage(Image *image) {
    setTextColor(Qt::black);
    setText( QFileInfo( image->fileName() ).fileName() );
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/view-preview.png")
                           .expanded())));
    setData( QVariant::fromValue<Image *>(image) );
  }


  /**
   * Sets the text, icon, and data corresponding to an ImageList.
   *
   * @param[in] imageList (ImageList *) The ImageList.
   */
  void ProjectItem::setImageList(ImageList *imageList) {
    setTextColor(Qt::black);
    if (imageList->name() != "") {
      setText( imageList->name() );
    }
    else {
      setText( imageList->path() );
    }
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-image.png")
                           .expanded())));
    setData( QVariant::fromValue<ImageList *>(imageList) );
  }


  /**
   * Sets the text, icon, and data corresponding to a list of ImageList.
   */
  void ProjectItem::setImages() {
    setTextColor(Qt::black);
    setText("Images");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-image.png")
                           .expanded())));
    setData( QVariant() );
  }


  /**
   * Sets the text, icon, and data corresponding to an Shape.
   *
   * @param[in] shape (Shape *) The Shape.
   */
  void ProjectItem::setShape(Shape *shape) {
    setTextColor(Qt::black);
    setText( QFileInfo( shape->fileName() ).fileName() );
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/rating.png")
                           .expanded())));
    setData( QVariant::fromValue<Shape *>(shape) );
  }


  /**
   * Sets the text, icon, and data corresponding to an ShapeList.
   *
   * @param[in] shapeList (ShapeList *) The ShapeList.
   */
  void ProjectItem::setShapeList(ShapeList *shapeList) {
    setTextColor(Qt::black);
    if (shapeList->name() != "") {
      setText( shapeList->name() );
    }
    else {
      setText( shapeList->path() );
    }
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-orange.png")
                           .expanded())));
    setData( QVariant::fromValue<ShapeList *>(shapeList) );
  }


  /**
   * Sets the text, icon, and data corresponding to a list of ShapeList.
   */
  void ProjectItem::setShapes() {
    setTextColor(Qt::black);
    setText("Shapes");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-red.png")
                           .expanded())));
    setData( QVariant() );
  }


  /**
   * Sets the text, icon, and data corresponding to a Template.
   *
   * @param[in] shape (Shape *) The Shape.
   */
  void ProjectItem::setTemplate(Template *newTemplate) {
    setTextColor(Qt::black);
    setText( QFileInfo( newTemplate->fileName() ).fileName() );
    setIcon( QIcon(":folder"));
    setData( QVariant::fromValue<Template *>(newTemplate) );
  }


  /**
   * Sets the text, icon, and data corresponding to an TemplateList.
   *
   * @param[in] templateList (TemplateList *) The TemplateList.
   */
  void ProjectItem::setTemplateList(TemplateList *templateList) {
    setTextColor(Qt::black);
    if (templateList->name() != "") {
      setText( templateList->name() );
    }
    else {
      setText( templateList->path() );
    }
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-orange.png")
                           .expanded())));
    setData( QVariant::fromValue<TemplateList *>(templateList) );
  }


  /**
   * Sets the text, icon, and data corresponding to a list of TemplateList.
   */
  void ProjectItem::setTemplates() {
    setText("Templates");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-red.png")
                           .expanded())));
    setData( QVariant() );

    ProjectItem *mapsItem = new ProjectItem();
    mapsItem->setText("Maps");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-red.png")
                           .expanded())));
    mapsItem->setData( QVariant() );
    appendRow(mapsItem);


    ProjectItem *registrationsItem = new ProjectItem();
    registrationsItem->setText("Registrations");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-red.png")
                           .expanded())));
    registrationsItem->setData( QVariant() );
    appendRow(registrationsItem);
  }


  /**
   * Sets the text, icon, and data corresponding to a Control.
   *
   * @param[in] control (Control *) The Control.
   */
  void ProjectItem::setControl(Control *control) {
    setTextColor(Qt::black);
    setText( QFileInfo( control->fileName() ).fileName() );
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/network-server-database.png")
                           .expanded())));
    setData( QVariant::fromValue<Control *>(control) );
  }


  /**
   * Sets the text, icon, and data corresponding to a ControlList.
   *
   * @param[in] controlList (ControlList *) The ControlList.
   */
  void ProjectItem::setControlList(ControlList *controlList) {
    setTextColor(Qt::black);
    setText( controlList->name() );
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder.png")
                           .expanded())));
    setData( QVariant::fromValue<ControlList *>(controlList) );
  }


  /**
   * Sets the text, icon, and data corresponding to a list of ControlList.
   */
  void ProjectItem::setControls() {
    setTextColor(Qt::black);
    setText("Control Networks");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-remote.png")
                           .expanded())));
    setData( QVariant() );
  }


  /**
   * Sets the text, icon, and data corresponding to a CorrelationMatrix.
   *
   * @param[in] correlationMatrix (CorrelationMatrix) The CorrelationMatrix.
   */
  void ProjectItem::setCorrelationMatrix(CorrelationMatrix correlationMatrix) {
    setTextColor(Qt::black);
    setText("Correlation Matrix");
    setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/network-server-database.png")
                           .expanded())));
    setData( QVariant::fromValue<CorrelationMatrix>(correlationMatrix) );
  }


  /**
   * Sets the text, icon, and data corresponding to a Project.
   *
   * @param[in] project (Project *) The Project.
   * @internal
   *     @history  2016-11-10 - Tyler Wilson  Changed the reference
   *               to the icon from ':data' to ':data-management'
   *               due to a naming conflict causing strange errors
   *               to appear on the command line.  Fixes #3982.
   */
  void ProjectItem::setProject(Project *project) {
    setTextColor(Qt::black);
    setText( project->name() );
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/folder-activities.png")
                           .expanded())));
    setData( QVariant::fromValue<Project *>(project) );
    setToolTip(project->projectRoot());
  }


  /**
   * Sets the text, icon, and data corresponding to a list of Results.
   */
  void ProjectItem::setResults() {
    setTextColor(Qt::black);
    setText("Results");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/kchart.png")
                           .expanded())));
    setData( QVariant() );
  }


  /**
   * Sets the text, icon, and data corresponding to a GuiCameraQsp.
   *
   * @param[in] guiCamera (GuiCameraQsp) The GuiCameraQsp.
   */
  void ProjectItem::setGuiCamera(GuiCameraQsp guiCamera) {
    setTextColor(Qt::black);
    setText( guiCamera->displayProperties()->displayName() );
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/camera-photo.png")
                           .expanded())));
    setData( QVariant::fromValue<GuiCameraQsp>(guiCamera) );
  }


  /**
   * Sets the text, icon, and data corresponding to a GuiCameraQsp.
   */
  void ProjectItem::setGuiCameraList() {
    setTextColor(Qt::black);
    setText("Sensors");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/camera-photo.png")
                           .expanded())));
    setData( QVariant() );
  }


  /**
   * Sets the text, icon, and data corresponding to SpaceCraft.
   */
  void ProjectItem::setSpacecraft() {
    setTextColor(Qt::black);
    setText("Spacecraft");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/preferences-desktop-launch-feedback.png")
                           .expanded())));
    setData( QVariant() );
  }



  /**
   * Sets the text, icon, and data corresponding to a TargetBodyQsp.
   *
   * @param targetBody Target body to set data from
   */
  void ProjectItem::setTargetBody(TargetBodyQsp targetBody) {
    setTextColor(Qt::black);
    setText( targetBody->displayProperties()->displayName() );
    if (targetBody->displayProperties()->displayName() == "MOON")
      setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/weather-clear-night.png")
                             .expanded())));
    else if (targetBody->displayProperties()->displayName() == "Enceladus")
      setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/nasa_enceladus.png")
                             .expanded())));
    else if (targetBody->displayProperties()->displayName() == "Mars")
      setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/nasa_mars.png")
                             .expanded())));
    else if (targetBody->displayProperties()->displayName() == "Titan")
      setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/nasa_titan.png")
                             .expanded())));
    else
      setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/view-web-browser-dom-tree.png")
                             .expanded())));

    setData( QVariant::fromValue<TargetBodyQsp>(targetBody) );
  }


  /**
   * Sets the text, icon, and data corresponding to a TargetBodyList.
   */
  void ProjectItem::setTargetBodyList() {
    setTextColor(Qt::black);
    setText("Target Body");
    setIcon( QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/view-web-browser-dom-tree.png")
                           .expanded())));
    setData( QVariant() );
  }


  /**
   * Finds and returns the first item in the model that contains the
   * data in the role.
   *
   * @param[in] value (const QVariant &) The data.
   * @param[in] role (int) The role.
   *
   * @return @b ProjectItem* The found item.
   */
  ProjectItem *ProjectItem::findItemData(const QVariant &value, int role) {
    if ( data(role) == value ) {
      return this;
    }

    for (int i=0; i<rowCount(); i++) {
      ProjectItem *item = child(i)->findItemData(value, role);
      if (item) {
        return item;
      }
    }

    return 0;
  }


  /**
   * Appends an item to the children of this item.
   *
   * @param[in] item (ProjectItem *) The item to append.
   */
  void ProjectItem::appendRow(ProjectItem *item) {
    setTextColor(Qt::black);
    QStandardItem::appendRow(item);
  }


  /**
   * Returns the child item at a given row.
   *
   * @param[in] row (int) The row.
   *
   * @return @b ProjectItem* The child item.
   */
  ProjectItem *ProjectItem::child(int row) const {
    return static_cast<ProjectItem *>( QStandardItem::child(row) );
  }


  /**
   * Inserts an item to the children of this item at the row.
   *
   * @param[in] row (int) The row.
   * @param[in] item (ProjectItem *) The item.
   */
  void ProjectItem::insertRow(int row, ProjectItem *item) {
    setTextColor(Qt::black);
    QStandardItem::insertRow(row, item);
  }


  /**
   * Returns the ProjectItemModel associated with this item.
   *
   * @return @b ProjectItemModel* The model.
   */
  ProjectItemModel *ProjectItem::model() const {
    return static_cast<ProjectItemModel *>( QStandardItem::model() );
  }


  /**
   * Returns the parent item of this item.
   *
   * @return @b ProjectItem* The parent item.
   */
  ProjectItem *ProjectItem::parent() const {
    return static_cast<ProjectItem *>( QStandardItem::parent() );
  }


  /**
   * Sets the child at the given row to an item.
   *
   * @param[in] row (int) The row.
   * @param[in] item (ProjectItem *) The item.
   */
  void ProjectItem::setChild(int row, ProjectItem *item) {
    setTextColor(Qt::black);
    QStandardItem::setChild(row, item);
  }


  /**
   * Removes the child item at the given row and returns the removed item.
   *
   * @param[in] row (int) The row.
   *
   * @return @b ProjectItem* The item.
   */
  ProjectItem *ProjectItem::takeChild(int row) {
    QList<QStandardItem *> items = QStandardItem::takeRow(row);

    if ( items.isEmpty() ) {
      return 0;
    }

    return static_cast<ProjectItem *>( items.first() );
  }


  void ProjectItem::setTextColor(Qt::GlobalColor color) {
    setForeground(QBrush(color));
  }
}
