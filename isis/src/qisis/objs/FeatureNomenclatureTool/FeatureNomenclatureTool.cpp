#include "FeatureNomenclatureTool.h"

#include <QAction>
#include <QApplication>
#include <QBitmap>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QPainter>
#include <QUrl>

#include <geos/geom/Coordinate.h>
#include <geos/geom/CoordinateSequence.h>
#include <geos/geom/MultiPolygon.h>

#include "Distance.h"
#include "FileName.h"
#include "ImagePolygon.h"
#include "Latitude.h"
#include "Longitude.h"
#include "MdiCubeViewport.h"
#include "NomenclatureToolConfigDialog.h"
#include "PolygonTools.h"
#include "Target.h"
#include "ToolPad.h"

using namespace std;

namespace Isis {

  /**
   * This instantiates a FeatureNomenclatureTool. This will read this tool's
   *   saved settings and potentially automatically enable itself.
   * @param parent
   */
  FeatureNomenclatureTool::FeatureNomenclatureTool(QWidget *parent) :
      Tool(parent) {
    m_foundNomenclature = NULL;
    m_nomenclatureSearchers = NULL;
    m_findNomenclatureCheckBox = NULL;
    m_foundFeaturesCombo = NULL;
    m_nomenclatureCenterBtn = NULL;
    m_nomenclatureOptionsBtn = NULL;
    m_disclaimerBtn = NULL;
    m_queryingProgress = NULL;
    m_nomenclatureEnabled = false;
    m_fontColor = NULL;

    m_foundNomenclature = new QList<ViewportFeatureDisplay>;
    m_nomenclatureSearchers =
        new QMap< MdiCubeViewport *, FeatureNomenclature *>;

    m_fontSize = 12;
    m_fontColor = new QColor(237, 170, 171);
    m_defaultEnabled = false;
    m_disclaimedAlready = false;
    m_showApprovedOnly = true;
    m_extentType = None;

    m_disclaimerText = "The nomenclature qview tool will label named features "
        "in your opened cube files. This tool <strong>requires</strong> an "
        "active internet connection, projection or camera information, and a "
        "calculatable ground range to function. The larger the ground range ("
        "covered area on a planet), the longer it will take to populate the "
        "nomenclature for a particular cube.<br/><br/>"
        "<font color='red'>**WARNING**</font> The accuracy of this tool is not "
        "perfect, features <strong>can and will be mislabeled</strong> if you "
        "have not properly controlled your images to the control network that "
        "identifies the latitude/longitude values of a feature. Please use the "
        "nomenclature website to verify a label is correct for a feature. "
        "<br/><br/>See the IAU Gazetteer of Planetary Nomenclature website for "
        "more information.<br/>"
        "<a href='http://planetarynames.wr.usgs.gov/'>"
        "http://planetarynames.wr.usgs.gov/</a>";

    connect(this, SIGNAL(toolActivated()),
            this, SLOT(onToolActivated()));

    readSettings();

    m_nomenclatureEnabled = m_defaultEnabled;
  }


  /**
   * Cleans up memory allocated by this tool.
   */
  FeatureNomenclatureTool::~FeatureNomenclatureTool() {
    delete m_foundNomenclature;
    m_foundNomenclature = NULL;

    foreach (FeatureNomenclature *nomenclature, *m_nomenclatureSearchers) {
      delete nomenclature;
    }

    delete m_nomenclatureSearchers;
    m_nomenclatureSearchers = NULL;
  }


  /**
   * Add the 'Show Nomenclature' option to the options menu.
   *
   * @param menu Menu to add 'Show Nomenclature' to.
   */
  void FeatureNomenclatureTool::addTo(QMenu *menu) {
    m_action = menu->addAction("Show Nomenclature");
    m_action->setCheckable(true);
    m_action->setChecked(m_nomenclatureEnabled);

    connect(m_action, SIGNAL(triggered(bool)),
            m_findNomenclatureCheckBox, SLOT(setChecked(bool)));
  }


  /**
   * Paint features on the given viewport.
   *
   * @param vp The viewport that needs painted
   * @param painter The painter to use for painting
   */
  void FeatureNomenclatureTool::paintViewport(MdiCubeViewport *vp,
                                              QPainter *painter) {
    if (m_nomenclatureEnabled && viewportFeaturesFound(vp)) {
      ViewportFeatureDisplay &display = *viewportFeatureDisplay(vp);

      QFont fontToUse;
      fontToUse.setPointSize(m_fontSize);
      painter->setFont(fontToUse);
      painter->setPen(QPen(*m_fontColor));
      display.paint(painter, (m_extentType != None), m_extentType, m_showApprovedOnly);
    }
  }


  /**
   * Is this tool enabled by default? (i.e. on program start)
   *
   * @return True if this tool is automatically enabled
   */
  bool FeatureNomenclatureTool::defaultEnabled() const {
    return m_defaultEnabled;
  }


  /**
   * What is the font color to use?
   *
   * @return Color that is used for painting features
   */
  QColor FeatureNomenclatureTool::fontColor() const {
    return *m_fontColor;
  }


  /**
   * Retrieve the font size of the features in this tool.
   *
   * @return Font point size used to render feature names
   */
  int FeatureNomenclatureTool::fontSize() const {
    return m_fontSize;
  }


  /**
   * Show approved features only?
   *
   * @return True if we're showing only approved features.
   */
  bool FeatureNomenclatureTool::showApprovedOnly() const {
    return m_showApprovedOnly;
  }


  /**
   * Draw vectors to the extents of features?
   *
   * @return True if we're drawing vectors to the extents of features.
   */
  FeatureNomenclatureTool::VectorType FeatureNomenclatureTool::vectorType() const {
    return m_extentType;
  }


  /**
   * Set whether this tool is enabled by default.
   *
   * @param defaultEnabled True to enable by default
   */
  void FeatureNomenclatureTool::setDefaultEnabled(bool defaultEnabled) {
    if (m_defaultEnabled != defaultEnabled) {
      m_defaultEnabled = defaultEnabled;
      writeSettings();
    }
  }


  /**
   * Set the color to use for drawing on the viewport. This takes effect
   *   immediately.
   *
   * @param color The color to use for drawing
   */
  void FeatureNomenclatureTool::setFontColor(QColor color) {
    if (*m_fontColor != color) {
      *m_fontColor = color;
      writeSettings();

      foreach (MdiCubeViewport *vp, *cubeViewportList())
        vp->viewport()->update();
    }
  }


  /**
   * Set the font point size to use for drawing text on the viewport. This takes
   *   effect immediately.
   *
   * @param newFontSize The font point size to use for rendering text
   */
  void FeatureNomenclatureTool::setFontSize(int newFontSize) {
    if (m_fontSize != newFontSize) {
      m_fontSize = newFontSize;
      writeSettings();

      nomenclaturePositionsOutdated();

      foreach (MdiCubeViewport *vp, *cubeViewportList())
        vp->viewport()->update();
    }
  }


  /**
   * Set whether to show approved features and exclude unapproved features.
   *
   * @param approvedOnly True to show only appproved features
   */
  void FeatureNomenclatureTool::setShowApprovedOnly(bool approvedOnly) {
    if (m_showApprovedOnly != approvedOnly) {
      m_showApprovedOnly = approvedOnly;
      writeSettings();
      rebuildFeaturesCombo();
      nomenclaturePositionsOutdated();

      foreach (MdiCubeViewport *vp, *cubeViewportList())
        vp->viewport()->update();
    }
  }


  /**
   * Set whether to draw vectors from the feature center to the feature extents
   *   on the viewport. This takes effect immediately.
   *
   * @param show True to show the vectors
   */
  void FeatureNomenclatureTool::setVectorType(VectorType show) {
    if (m_extentType != show) {
      m_extentType = show;
      writeSettings();

      for (int i = 0; i < m_foundNomenclature->count(); i++) {
        (*m_foundNomenclature)[i].applyExtentType(m_extentType);
      }

      nomenclaturePositionsOutdated();

      foreach (MdiCubeViewport *vp, *cubeViewportList())
        vp->viewport()->update();
    }
  }


  /**
   * This is the name of the menu that should be passed into "addTo()"
   *
   * @return The options menu's name
   */
  QString FeatureNomenclatureTool::menuName() const {
    return "&Options";
  }


  /**
   * Creates the widget that goes on the tool bar when this tool is active.
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *FeatureNomenclatureTool::createToolBarWidget(
      QStackedWidget *parent) {
    QWidget *wrapperWidget = new QWidget;

    m_findNomenclatureCheckBox = new QCheckBox;
    m_findNomenclatureCheckBox->setText("Name Features");
    m_findNomenclatureCheckBox->setChecked(m_nomenclatureEnabled);
    connect(m_findNomenclatureCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(findNomenclatureStateChanged(int)));

    QLabel *foundFeaturesLabel = new QLabel("Found Features:");
    m_foundFeaturesCombo = new QComboBox;
    m_foundFeaturesCombo->setSizeAdjustPolicy(
        QComboBox::AdjustToContents);
    m_foundFeaturesCombo->addItem("");

    m_nomenclatureCenterBtn = new QPushButton("Center on Feature");
    m_nomenclatureCenterBtn->setEnabled(false);

    connect(m_nomenclatureCenterBtn, SIGNAL(clicked()),
            this, SLOT(centerOnSelectedFeature()));

    m_nomenclatureOptionsBtn = new QPushButton("Tool Options");
    connect(m_nomenclatureOptionsBtn, SIGNAL(clicked()),
            this, SLOT(configure()));

    m_disclaimerBtn = new QPushButton("Disclaimer");
    connect(m_disclaimerBtn, SIGNAL(clicked()),
            this, SLOT(showDisclaimer()));

    connect(m_foundFeaturesCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(featureSelected()));

    m_queryingProgress = new QProgressBar;
    m_queryingProgress->setObjectName("nomenclatureQueryProgress");
    m_queryingProgress->setVisible(false);
    m_queryingProgress->setRange(0, 0);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_findNomenclatureCheckBox);
    layout->addWidget(foundFeaturesLabel);
    layout->addWidget(m_foundFeaturesCombo);
    layout->addWidget(m_nomenclatureCenterBtn);
    layout->addWidget(m_nomenclatureOptionsBtn);
    layout->addWidget(m_disclaimerBtn);
    layout->addWidget(m_queryingProgress);
    layout->addStretch(1);
    wrapperWidget->setLayout(layout);
    return wrapperWidget;
  }


  /**
   * Add this tool's action to the toolpad. This defines the name and hotkey
   *   of this tool.
   *
   * @param toolpad
   *
   * @return QAction*
   */
  QAction *FeatureNomenclatureTool::toolPadAction(ToolPad *toolpad) {
    QAction *action = new QAction(toolpad);

    action->setIcon(QPixmap(toolIconDir() + "/nomenclature.png"));
    action->setToolTip("Nomenclature (N)");
    action->setShortcut(Qt::Key_N);
    action->setObjectName("nomenclatureToolButton");

    QString text  =
      "<b>Function:</b>  Display nomenclature on the visible images.\n"
      "<p/><b>Hint:</b>  While this tool is active, you can left and right "
      "click on any of the named features for additional options."
      "<p/><b>Shortcut:</b> N";
    action->setWhatsThis(text);

    return action;
  }


  /**
   * This handles a mouse release on one of the cube viewports when this tool is
   *   active. The responsibilities are forwarde to
   *   ViewportFeatureDisplay::handleMouseClicked().
   *
   * @param p The point in the viewport where the mouse was released
   * @param s The mouse buttons that were released
   */
  void FeatureNomenclatureTool::mouseButtonRelease(
      QPoint p, Qt::MouseButton s) {
    if (m_nomenclatureEnabled &&
        viewportFeatureDisplay(cubeViewport())) {
      viewportFeatureDisplay(cubeViewport())->handleMouseClicked(this, p, s);
    }
  }


  /**
   * Updates the state of the current tool. This will find any missing
   *   nomenclature if appropriate.
   */
  void FeatureNomenclatureTool::updateTool() {
    findMissingNomenclature();
  }


  /**
   * Center the relevent viewport (and any viewports linked to it) on the
   *   feature selected in the feature selection combo box.
   */
  void FeatureNomenclatureTool::centerOnSelectedFeature() {
    QVariant variant = m_foundFeaturesCombo->itemData(
        m_foundFeaturesCombo->currentIndex());

    if (variant.toMap()["Viewport"].value<MdiCubeViewport *>() != NULL) {
      MdiCubeViewport *vp =
          variant.toMap()["Viewport"].value<MdiCubeViewport *>();
      FeatureNomenclature::Feature feature =
          variant.toMap()["Feature"].value<FeatureNomenclature::Feature>();

      // We need to find the appropriate viewport also...
      centerOnFeature(vp, feature);
    }
  }


  /**
   * Give a configuration dialog for the options available in this tool.
   */
  void FeatureNomenclatureTool::configure() {
    NomenclatureToolConfigDialog *configDialog =
        new NomenclatureToolConfigDialog(this,
                                         qobject_cast<QWidget *>(parent()));
    configDialog->setAttribute(Qt::WA_DeleteOnClose);
    configDialog->show();
  }


  /**
   * This handles a feature being selected in the feature list combo box.
   *
   * The 'Center' button's enabled state is set to reflect if a feature is
   *   selected and the tool is enabled.
   */
  void FeatureNomenclatureTool::featureSelected() {
    QVariant variant = m_foundFeaturesCombo->itemData(
        m_foundFeaturesCombo->currentIndex());

    bool featureSelected =
        variant.toMap()["Viewport"].value<MdiCubeViewport *>() != NULL;

    m_nomenclatureCenterBtn->setEnabled(
        m_nomenclatureEnabled && featureSelected);
  }


  /**
   * A feature nomenclature has finished querying... we need to translate the
   *   features into visible names.
   */
  void FeatureNomenclatureTool::featuresIdentified(
      FeatureNomenclature *searcher) {
    MdiCubeViewport *viewport = m_nomenclatureSearchers->key(searcher);

    // The viewport could have gone away when we were still querying... handle
    //   that case.
    bool vpValid = cubeViewportList()->contains(viewport);
    if (vpValid)
      featuresForViewportFound(viewport);

    if (m_nomenclatureEnabled) {
      viewportDone(viewport);
    }

    if (vpValid)
      viewport->viewport()->update();
  }


  /**
   * The 'Name Features' check box has changed state. Handle identifying any
   *   missing features and naming any already-found features if enabling, hide
   *   all names if disabling.
   */
  void FeatureNomenclatureTool::findNomenclatureStateChanged(int newState) {
    if (newState == Qt::Unchecked) {
      m_nomenclatureEnabled = false;
      toolStateChanged();
    }
    else if (newState == Qt::Checked) {
      m_nomenclatureEnabled = true;
      toolStateChanged();

      findMissingNomenclature();
    }

    foreach (MdiCubeViewport *vp, *cubeViewportList())
      vp->viewport()->update();
  }


  /**
   * Update the screen coordinates of the named features because the viewport
   *   has changed it's mappings. This uses the already found cube sample, line
   *   positions... it just needs to do the appropriate transformations from
   *   cube to viewport. This method does not cause a repaint.
   */
  void FeatureNomenclatureTool::nomenclaturePositionsOutdated() {
    if (m_nomenclatureEnabled) {

      for (int i = 0; i < m_foundNomenclature->count(); i++) {
        (*m_foundNomenclature)[i].handleViewChanged(this);
      }
    }
  }


  /**
   * When this tool is activated (clicked on in the tool bar), turn ourselves on
   *   immediately.
   */
  void FeatureNomenclatureTool::onToolActivated() {
    if (!m_findNomenclatureCheckBox->isChecked())
      m_findNomenclatureCheckBox->setChecked(true);
  }


  /**
   * Show the user our nomenclature disclaimer and make note that we have shown
   *   the disclaimer.
   */
  void FeatureNomenclatureTool::showDisclaimer() {
    QMessageBox::warning(qobject_cast<QWidget *>(parent()),
                             "Nomenclature Disclaimer", m_disclaimerText);
    m_disclaimedAlready = true;
    writeSettings();
  }


  /**
   * Center the given and any linked viewports (which contain the same feature)
   *   on the given feature. This also prioritizes the feature to display on
   *   top of the other features in the viewport(s) which center on the feature.
   *
   * @param vp The viewport to center (and to use for finding linked viewports)
   * @param feature The feature to center on the viewport
   */
  void FeatureNomenclatureTool::centerOnFeature(MdiCubeViewport *vp,
      FeatureNomenclature::Feature feature) {
    foreach (MdiCubeViewport *viewport, viewportsWithFoundNomenclature()) {
      if (viewport == vp ||
          (vp->isLinked() && viewport->isLinked())) {
        viewportFeatureDisplay(viewport)->centerFeature(feature);
      }
    }
  }


  /**
   * Move the features from a searching state to a found state for the given
   *   viewport. The FeatureNomenclature class must have emitted
   *   featuresIdentified before calling this. The viewport must be a valid
   *   pointer.
   *
   * @param vp The viewport which we're changing the nomenclature state of.
   */
  void FeatureNomenclatureTool::featuresForViewportFound(MdiCubeViewport *vp) {
    if (vp) {
      QList<FeatureNomenclature::Feature> features;
      if (m_nomenclatureSearchers->find(vp) != m_nomenclatureSearchers->end()) {
        if ((*m_nomenclatureSearchers)[vp]->hasResult())
          features.append((*m_nomenclatureSearchers)[vp]->features());
        else
          m_findNomenclatureCheckBox->setChecked(false);
      }

      if (!viewportFeatureDisplay(vp)) {
        m_foundNomenclature->append(ViewportFeatureDisplay(this, vp, features, m_extentType));
      }

      features = viewportFeatureDisplay(vp)->features();

      QProgressDialog updatingFeaturesProgress(
          tr("Projecting Features for [%1]").arg(vp->cube()->fileName().section('/',-1)),
          QString(), 0, 100);

      updatingFeaturesProgress.setWindowModality(Qt::WindowModal);

      FeatureNomenclature::Feature feature;

      for (int i = 0; i < features.count(); i++) {
        feature = features[i];

        int progress = floor(100 * (double)i / (double)features.count());

        if (progress != updatingFeaturesProgress.value())
          updatingFeaturesProgress.setValue(progress);

        if (updatingFeaturesProgress.wasCanceled()) {
          m_nomenclatureSearchers->clear();
          m_foundNomenclature->clear();
          m_foundFeaturesCombo->clear();
          m_findNomenclatureCheckBox->setChecked(false);
          break;
        }

        if ( !m_showApprovedOnly ||
            (m_showApprovedOnly && feature.status() == FeatureNomenclature::Approved) ) {

          QString displayName = feature.cleanName() +
              " (" + FileName(vp->cube()->fileName()).name() + ")";

          QString targetName = feature.target().toUpper();

          // never insert above the blank (at 0)
          int insertPos = 1;

          bool foundInsertPos = m_foundFeaturesCombo->count() == 0;

          while (!foundInsertPos) {
            if (insertPos >= m_foundFeaturesCombo->count()) {
              foundInsertPos = true;
            }
            else {
              QVariant insetPosData = m_foundFeaturesCombo->itemData(
                  insertPos);
              QString insertPosTarget = insetPosData.toMap()["Target"].toString();

              if (targetName < insertPosTarget) {
                foundInsertPos = true;
              }
              else if (targetName == insertPosTarget) {
                if (!insetPosData.toMap()["Viewport"].isNull()) {
                  foundInsertPos = displayName.compare(
                      m_foundFeaturesCombo->itemText(insertPos),
                      Qt::CaseInsensitive) < 0;
                }
              }
            }

            if (!foundInsertPos)
              insertPos++;
          }

          if (m_foundFeaturesCombo->itemData(insertPos - 1).toMap()[
                  "Target"].toString() != targetName) {
            QMap<QString, QVariant> data;
            data["Target"] = targetName;

            QString controlNet = feature.controlNet();
            if (controlNet != "")
              controlNet = " (" + controlNet + ")";

            m_foundFeaturesCombo->insertItem(insertPos,
                                            targetName + controlNet,
                                            data);
            QVariant font = m_foundFeaturesCombo->itemData(insertPos,
                                                          Qt::ForegroundRole);
            m_foundFeaturesCombo->setItemData(insertPos, QColor(Qt::gray),
                                              Qt::ForegroundRole);
            insertPos++;

            m_foundFeaturesCombo->insertItem(insertPos,
                                            "-----------",
                                            data);
            m_foundFeaturesCombo->setItemData(insertPos, QColor(Qt::gray),
                                              Qt::ForegroundRole);
            insertPos++;
          }

          QMap<QString, QVariant> data;
          data["Feature"] = QVariant::fromValue<FeatureNomenclature::Feature>(
              feature);
          data["Viewport"] = QVariant::fromValue(vp);
          data["Target"] = QVariant::fromValue(targetName);

          m_foundFeaturesCombo->insertItem(insertPos, displayName,
              QVariant::fromValue(data));
        }
      }
     updatingFeaturesProgress.setValue( features.count() );
    }
  }


  /**
   * Update this tool's nomenclature data based on this tool's enabled state and
   *   the current viewport list. This will always remove results of viewports
   *   that no longer exist. If displaying nomenclature is enabled, then this
   *   will also look for features on any viewports that don't have results and
   *   aren't currently querying.
   */
  void FeatureNomenclatureTool::findMissingNomenclature() {
    if (m_nomenclatureEnabled) {
      // We're looking for viewports with nomenclature results that no longer
      //   exist with removedViewports.
      QList<MdiCubeViewport *> removedViewports =
          viewportsWithFoundNomenclature();

      foreach (MdiCubeViewport *vp, *cubeViewportList()) {
        removedViewports.removeOne(vp);

        QMap< MdiCubeViewport *,
            FeatureNomenclature *> &finding = *m_nomenclatureSearchers;

        if (!viewportFeaturesFound(vp) && finding.find(vp) == finding.end()) {
          findMissingNomenclature(vp);
        }
      }

      bool removedAViewport = false;
      foreach (MdiCubeViewport *vp, removedViewports) {
        // A viewport disappeared; let's remove all references of it.
        removeFeatureDisplay(vp);
        removedAViewport = true;
      }

      if (removedAViewport) {
        // Rebuild the combo box...
        rebuildFeaturesCombo();
      }
    }
  }


  /**
   * Query for nomenclature on the given viewport.
   *
   * @param vp A viewport that both has no results and is not currently querying
   *     the nomenclature database.
   */
  void FeatureNomenclatureTool::findMissingNomenclature(MdiCubeViewport *vp) {
    try {
      // verify we can project before anything else
      UniversalGroundMap *ugm = vp->universalGroundMap();

      if (!ugm)
        throw IException();

      QString target;
      if (vp->camera()) {
        target = vp->camera()->target()->name();
      }
      else if (vp->projection()) {
        PvlGroup mappingGrp = vp->projection()->Mapping();

        if (mappingGrp.hasKeyword("TargetName"))
          target = QString::fromStdString(mappingGrp["TargetName"][0]);
      }

      Latitude minLat;
      Latitude maxLat;
      Longitude minLon;
      Longitude maxLon;
      if (ugm->GroundRange(vp->cube(), minLat, maxLat, minLon, maxLon) &&
          target != "") {
        FeatureNomenclature *searcher = new FeatureNomenclature;
        connect(searcher, SIGNAL(featuresIdentified(FeatureNomenclature *)),
                this, SLOT(featuresIdentified(FeatureNomenclature *)));

        (*m_nomenclatureSearchers)[vp] = searcher;
        toolStateChanged();
        (*m_nomenclatureSearchers)[vp]->queryFeatures(target.toUpper(),
                                                      minLat, minLon,
                                                      maxLat, maxLon);
      }
      else {
        viewportDone(vp);
      }
    }
    catch (IException &) {
      viewportDone(vp);
    }
  }


  /**
   * Rebuild m_foundFeaturesCombo's data from scratch. This should only be
   *   necessary when viewports are lost.
   */
  void FeatureNomenclatureTool::rebuildFeaturesCombo() {
    if (m_foundFeaturesCombo) {
      m_foundFeaturesCombo->clear();
      m_foundFeaturesCombo->addItem("");

      foreach (MdiCubeViewport *vp, viewportsWithFoundNomenclature()) {
        featuresForViewportFound(vp);
      }
    }
  }


  /**
   * Remove knowledge of features on the given viewport. The viewport does not
   *   have to be a valid (allocated) pointer.
   *
   * @param vp The viewport to forget.
   */
  void FeatureNomenclatureTool::removeFeatureDisplay(MdiCubeViewport *vp) {
    for (int i = m_foundNomenclature->count() - 1; i >= 0; i--) {
      if (m_foundNomenclature->at(i).sourceViewport() == vp)
        m_foundNomenclature->removeAt(i);
    }
  }


  /**
   * Show a dialog with full feature details of a given feature.
   *
   * @param feature The feature to describe
   */
  void FeatureNomenclatureTool::showFeatureDetails(
      FeatureNomenclature::Feature feature) {
    QDialog *detailsDialog = new QDialog(qobject_cast<QWidget *>(parent()));
    detailsDialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    detailsDialog->setLayout(mainLayout);

    mainLayout->addWidget(feature.toWidget());

    QWidget *buttonsAreaWrapper = new QWidget;

    QHBoxLayout *buttonsAreaLayout = new QHBoxLayout;
    buttonsAreaWrapper->setLayout(buttonsAreaLayout);

    buttonsAreaLayout->addStretch();
    QPushButton *okayBtn = new QPushButton("&Ok");
    okayBtn->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(okayBtn, SIGNAL(clicked()),
            detailsDialog, SLOT(accept()));
    buttonsAreaLayout->addWidget(okayBtn);

    mainLayout->addWidget(buttonsAreaWrapper);

    detailsDialog->show();
  }


  /**
   * Show a web view pointed to the feature's web page.
   *
   * @param feature The feature to bring up in the web view
   */
  void FeatureNomenclatureTool::showFeatureWebsite(
      FeatureNomenclature::Feature feature) {
    QDesktopServices::openUrl(feature.referenceUrl());
  }


  /**
   * This should be called any time this tool's enabled or searching state could
   *   have changed. This enabled/disables and shows/hides widgets appropriately
   *   to match the current state.
   */
  void FeatureNomenclatureTool::toolStateChanged() {
    bool isCurrentlyLoading = (m_nomenclatureSearchers->keys().count() > 0);

    if (isCurrentlyLoading) {
      m_findNomenclatureCheckBox->setEnabled(false);
      m_queryingProgress->setVisible(true);
    }
    else {
      m_findNomenclatureCheckBox->setEnabled(true);
      m_queryingProgress->setVisible(false);
      m_nomenclatureCenterBtn->setEnabled(m_nomenclatureEnabled);

      if (m_nomenclatureEnabled && !m_disclaimedAlready) {
        showDisclaimer();
      }
    }

    if (m_action) {
      m_action->setChecked(m_nomenclatureEnabled);
    }

    featureSelected();
  }


  /**
   * Finalize the search results for the given viewport. The viewport doesn't
   *   have to be a valid (allocated) pointer. This will destroy any searchers
   *   with the given viewport, and if there are no found results for the
   *   viewport this will create a blank one as a place holder. This prevents
   *   searching for features on the viewport again.
   *
   * @param vp The viewport to finalize the search results for
   */
  void FeatureNomenclatureTool::viewportDone(MdiCubeViewport *vp) {
    if (m_nomenclatureSearchers->find(vp) != m_nomenclatureSearchers->end()) {
      if ((*m_nomenclatureSearchers)[vp]) {
        (*m_nomenclatureSearchers)[vp]->deleteLater();
      }

      m_nomenclatureSearchers->remove(vp);
    }

    if (viewportFeatureDisplay(vp) == NULL) {
      m_foundNomenclature->append(
          ViewportFeatureDisplay(this, vp,
                                 QList<FeatureNomenclature::Feature>(), m_extentType));
    }
    else {
      connect(vp, SIGNAL(screenPixelsChanged()),
              this, SLOT(nomenclaturePositionsOutdated()));
    }

    toolStateChanged();
  }


  /**
   * Map from viewport to feature display.
   *
   * @param vp The viewport to map
   * @return NULL if there is no found nomenclature for the viewport, otherwise
   *         the appropriate feature display.
   */
  FeatureNomenclatureTool::ViewportFeatureDisplay *
      FeatureNomenclatureTool::viewportFeatureDisplay(MdiCubeViewport *vp) {
    for (int i = 0; i < m_foundNomenclature->count(); i++) {
      if (m_foundNomenclature->at(i).sourceViewport() == vp)
        return &(*m_foundNomenclature)[i];
    }

    return NULL;
  }


  /**
   * Map from viewport to feature display.
   *
   * @param vp The viewport to map
   * @return NULL if there is no found nomenclature for the viewport, otherwise
   *         the appropriate feature display.
   */
  const FeatureNomenclatureTool::ViewportFeatureDisplay *
      FeatureNomenclatureTool::viewportFeatureDisplay(MdiCubeViewport *vp)
        const {
    for (int i = 0; i < m_foundNomenclature->count(); i++) {
      if (m_foundNomenclature->at(i).sourceViewport() == vp)
        return &(*m_foundNomenclature)[i];
    }

    return NULL;
  }


  /**
   * Test if features have already been found for a given viewport.
   *
   * @param vp The viewport to test
   * @return True if features have been found, false if we're either still
   *         searching for features or haven't started searching for features
   *         on the viewport.
   */
  bool FeatureNomenclatureTool::viewportFeaturesFound(MdiCubeViewport *vp)
      const {
    return (viewportFeatureDisplay(vp) != NULL);
  }


  /**
   * Get a list of viewports with found nomenclature. This may include viewports
   *   that no longer exist but haven't been cleaned up already.
   *
   * @return A list of viewports with a feature display.
   */
  QList<MdiCubeViewport *>
      FeatureNomenclatureTool::viewportsWithFoundNomenclature() {
    QList<MdiCubeViewport *> result;

    for (int i = 0; i < m_foundNomenclature->count(); i++) {
      if (m_foundNomenclature->at(i).sourceViewport() != NULL)
        result.append(m_foundNomenclature->at(i).sourceViewport());
    }

    return result;
  }


  /**
   * Read this tool's preserved state. This uses the current state as defaults,
   *   so please make sure your variables are initialized before calling this
   *   method.
   */
  void FeatureNomenclatureTool::readSettings() {
    FileName config("$HOME/.Isis/qview/nomenclature.config");
    QSettings settings(
        config.expanded(), QSettings::NativeFormat);

    m_fontSize = settings.value("fontSize", m_fontSize).toInt();
    *m_fontColor = settings.value("fontColor", *m_fontColor).value<QColor>();
    m_defaultEnabled =
        settings.value("defaultEnabled", m_defaultEnabled).toBool();
    m_disclaimedAlready =
        settings.value("disclaimerShown", m_disclaimedAlready).toBool();
    m_showApprovedOnly =
        settings.value("showApprovedOnly", m_showApprovedOnly).toBool();
    m_extentType =
        VectorType(settings.value("vectorsShown", m_extentType).toInt());
  }


  /**
   * Write out this tool's preserved state between runs. This is NOT called on
   *   close, so you should call this any time you change the preserved state.
   */
  void FeatureNomenclatureTool::writeSettings() {
    FileName config("$HOME/.Isis/qview/nomenclature.config");
    QSettings settings(
        config.expanded(), QSettings::NativeFormat);
    settings.setValue("fontSize", m_fontSize);
    settings.setValue("fontColor", QVariant::fromValue(*m_fontColor));
    settings.setValue("defaultEnabled", m_defaultEnabled);
    settings.setValue("disclaimerShown", m_disclaimedAlready);
    settings.setValue("showApprovedOnly", m_showApprovedOnly);
    settings.setValue("vectorsShown", m_extentType);
  }


  /**
   * Instiantiates a feature position with no data.
   */
  FeatureNomenclatureTool::FeaturePosition::FeaturePosition() {
    m_centerLine = Null;
    m_centerSample = Null;
    m_featureEdgeLineSamples = NULL;
    m_gmap = NULL;

    m_featureEdgeLineSamples = new QList< QPair<double, double> >;
  }


  /**
   * Instiantiates a feature position. This will calculate the line/sample
   *   coordinates of the feature.
   *
   * @param vp The viewport to use for ground information
   * @param feature The feature with nomenclature database lat/lon values to be
   *            translated into cube line/sample coordinates.
   * @param vectorType The type of extent vector to display
   */
  FeatureNomenclatureTool::FeaturePosition::FeaturePosition(MdiCubeViewport *vp,
      FeatureNomenclature::Feature feature, VectorType vectorType) {
    m_centerLine = Null;
    m_centerSample = Null;
    m_featureEdgeLineSamples = NULL;
    m_gmap = NULL;

    m_featureEdgeLineSamples = new QList< QPair<double, double> >;

    m_feature = feature;

    if (vp) {
      m_gmap = vp->universalGroundMap();
      Latitude centerLat = m_feature.centerLatitude();
      Longitude centerLon = m_feature.centerLongitude();
      if (m_gmap && m_gmap->SetGround(centerLat, centerLon)) {
        m_centerSample = m_gmap->Sample();
        m_centerLine = m_gmap->Line();

        applyExtentType(vectorType);
      }
    }
  }


  /**
   * Copies a feature position.
   *
   * @param other The feature position to copy
   */
  FeatureNomenclatureTool::FeaturePosition::FeaturePosition(
      const FeaturePosition &other) {
    m_centerLine = other.m_centerLine;
    m_centerSample = other.m_centerSample;
    m_featureEdgeLineSamples = NULL;
    m_gmap = NULL;

    m_featureEdgeLineSamples = new QList< QPair<double, double> >(
        *other.m_featureEdgeLineSamples);

    m_feature = other.m_feature;

    if (other.m_gmap)
      m_gmap = new UniversalGroundMap(*other.m_gmap);
  }


  /**
   * Cleans up allocated memory.
   */
  FeatureNomenclatureTool::FeaturePosition::~FeaturePosition() {
    m_centerLine = Null;
    m_centerSample = Null;
    m_gmap = NULL;

    delete m_featureEdgeLineSamples;
    m_featureEdgeLineSamples = NULL;
  }


  /**
   * Test if sample/line coordinates could be found for this feature.
   *
   * @return True if this feature has a sample/line position.
   */
  bool FeatureNomenclatureTool::FeaturePosition::isValid() const {
    return (m_centerSample != Null && m_centerLine != Null);
  }


  /**
   * Get the center sample/line position of the feature.
   *
   * @return first = sample, second = line.
   */
  QPair<double, double> FeatureNomenclatureTool::FeaturePosition::center()
      const {
    return QPair<double, double>(m_centerSample, m_centerLine);
  }


  /**
   * Get the edge sample/line positions of the feature.
   *
   * @return In the pair, first = sample, second = line.
   */
  QList< QPair<double, double> >
      FeatureNomenclatureTool::FeaturePosition::edges() const {
    return *m_featureEdgeLineSamples;
  }


  /**
   * Get the feature associated with this feature position.
   *
   * @return The feature
   */
  FeatureNomenclature::Feature &
      FeatureNomenclatureTool::FeaturePosition::feature() {
    return m_feature;
  }

  /**
   * Applies the type of extents to the feature.
   *
   * 4 Arrows - N, S, E, W
   * 8 Arrows - N, NE, NW, E, W, S, SE, SW
   * Box - Corners at: NE, NW, SE, SW
   *
   * @param vectorType The type of extents that will be drawn with the feature
   */
  void FeatureNomenclatureTool::FeaturePosition::applyExtentType(VectorType vectorType) {

    Latitude centerLat = m_feature.centerLatitude();
    Longitude centerLon = m_feature.centerLongitude();

    m_featureEdgeLineSamples->clear();

    if (vectorType == FeatureNomenclatureTool::Arrows8) {

      // We're going to permute the edge lats/lons excluding the center, so
      //   these lists are independent of each other.
      QList<Latitude> edgeLats;
      QList<Longitude> edgeLons;

      edgeLats.append(m_feature.northernLatitude());
      edgeLats.append(m_feature.centerLatitude());
      edgeLats.append(m_feature.southernLatitude());

      edgeLons.append(m_feature.easternLongitude());
      edgeLons.append(m_feature.centerLongitude());
      edgeLons.append(m_feature.westernLongitude());

      int edgeLatCount = edgeLats.count();
      int edgeLonCount = edgeLons.count();

      for (int latIndex = 0; latIndex < edgeLatCount; latIndex++) {
        for (int lonIndex = 0; lonIndex < edgeLonCount; lonIndex++) {
          Latitude &lat = edgeLats[latIndex];
          Longitude &lon = edgeLons[lonIndex];

          if (lat.isValid() && lon.isValid() &&
              (lat != centerLat || lon != centerLon) &&
              m_gmap->SetGround(lat, lon)) {
            m_featureEdgeLineSamples->append(
              QPair<double, double>(m_gmap->Sample(), m_gmap->Line()));
          }
        }
      }
    }

    else {
      QList< QPair<Latitude, Longitude> > edgeLatLons;

      if (vectorType == FeatureNomenclatureTool::Arrows4) {
        edgeLatLons.append(qMakePair(m_feature.northernLatitude(),
                                      m_feature.centerLongitude()));
        edgeLatLons.append(qMakePair(m_feature.centerLatitude(),
                                      m_feature.westernLongitude()));
        edgeLatLons.append(qMakePair(m_feature.centerLatitude(),
                                      m_feature.easternLongitude()));
        edgeLatLons.append(qMakePair(m_feature.southernLatitude(),
                                      m_feature.centerLongitude()));
      }

      if (vectorType == FeatureNomenclatureTool::Box) {
        edgeLatLons.append(qMakePair(m_feature.northernLatitude(),
                                      m_feature.easternLongitude()));
        edgeLatLons.append(qMakePair(m_feature.northernLatitude(),
                                      m_feature.westernLongitude()));
        edgeLatLons.append(qMakePair(m_feature.southernLatitude(),
                                      m_feature.westernLongitude()));
        edgeLatLons.append(qMakePair(m_feature.southernLatitude(),
                                      m_feature.easternLongitude()));
      }

      int edgeLatLonCount = edgeLatLons.count();

      for (int edgeIndex = 0; edgeIndex < edgeLatLonCount; edgeIndex++) {
        Latitude &lat = edgeLatLons[edgeIndex].first;
        Longitude &lon = edgeLatLons[edgeIndex].second;

        if (lat.isValid() && lon.isValid() &&
            (lat != centerLat || lon != centerLon) &&
            m_gmap->SetGround(lat, lon)) {
          m_featureEdgeLineSamples->append(
            QPair<double, double>(m_gmap->Sample(), m_gmap->Line()));
        }
      }
    }
  }


  /**
   * Trade member data with other. This should never throw an exception.
   *
   * @param other The instance to trade state with
   */
  void FeatureNomenclatureTool::FeaturePosition::swap(FeaturePosition &other) {
    std::swap(m_centerLine, other.m_centerLine);
    std::swap(m_centerSample, other.m_centerSample);
    std::swap(m_featureEdgeLineSamples, other.m_featureEdgeLineSamples);
    std::swap(m_feature, other.m_feature);
    std::swap(m_gmap, other.m_gmap);
  }


  /**
   * Assign rhs to this. We'll be taking on (mirroring) the state of the other
   *   feature position.
   *
   * @param rhs The FeaturePosition on the right hand side of the '=' operator
   *
   * @return *this
   */
  FeatureNomenclatureTool::FeaturePosition &
      FeatureNomenclatureTool::FeaturePosition::operator=(
        const FeaturePosition &rhs) {
    FeaturePosition copy(rhs);
    swap(copy);
    return *this;
  }


  /**
   * Get the feature associated with this feature position.
   *
   * @return The feature
   */
  const FeatureNomenclature::Feature &
      FeatureNomenclatureTool::FeaturePosition::feature() const {
    return m_feature;
  }


  /**
   * Instantiate a blank feature display position.
   */
  FeatureNomenclatureTool::FeatureDisplayPosition::FeatureDisplayPosition() {
    m_textRect = NULL;
    m_fullDisplayRect = NULL;
    m_edgePoints = NULL;

    m_textRect = new QRect;
    m_fullDisplayRect = new QRect;
    m_edgePoints = new QList<QPoint>;
  }


  /**
   * Instantiate a feature display position with the given data.
   *
   * @param textRect The screen pixel rect in viewport screen coordinates that
   *     ought to be filled with the textual name.
   * @param fullDisplayRect The screen pixel rect in viewport screen coordinates
   *     that encapsulates the entire feature.
   * @param edgePoints The edge screen pixel points in viewport screen
   *     coordinates that circle the feature.
   */
  FeatureNomenclatureTool::FeatureDisplayPosition::FeatureDisplayPosition(
      QRect textRect, QRect fullDisplayRect, QList<QPoint> edgePoints) {
    m_textRect = NULL;
    m_fullDisplayRect = NULL;
    m_edgePoints = NULL;

    m_textRect = new QRect(textRect);
    m_fullDisplayRect = new QRect(fullDisplayRect);
    m_edgePoints = new QList<QPoint>(edgePoints);
  }


  /**
   * Copy a feature display position.
   * @param other The position to copy
   */
  FeatureNomenclatureTool::FeatureDisplayPosition::FeatureDisplayPosition(
      const FeatureDisplayPosition &other) {
    m_textRect = NULL;
    m_fullDisplayRect = NULL;
    m_edgePoints = NULL;

    m_textRect = new QRect(*other.m_textRect);
    m_fullDisplayRect = new QRect(*other.m_fullDisplayRect);
    m_edgePoints = new QList<QPoint>(*other.m_edgePoints);
  }


  /**
   * Free the allocated memory.
   */
  FeatureNomenclatureTool::FeatureDisplayPosition::~FeatureDisplayPosition() {
    delete m_textRect;
    m_textRect = NULL;

    delete m_fullDisplayRect;
    m_fullDisplayRect = NULL;

    delete m_edgePoints;
    m_edgePoints = NULL;
  }


  /**
   * Get the screen pixel rect in viewport screen coordinates that ought to be
   *   filled with the textual name.
   *
   * @return Screen pixel rect in viewport screen coordinates
   */
  QRect FeatureNomenclatureTool::FeatureDisplayPosition::textArea() const {
    return *m_textRect;
  }


  /**
   * Get the screen pixel rect in viewport screen coordinates that encapsulates
   *   the entire feature.
   *
   * @return Screen pixel rect in viewport screen coordinates
   */
  QRect FeatureNomenclatureTool::FeatureDisplayPosition::displayArea() const {
    return *m_fullDisplayRect;
  }


  /**
   * Get the edge screen pixel points in viewport screen coordinates that
   *   circle the feature.
   *
   * @return Screen pixel edge points in viewport screen coordinates
   */
  QList<QPoint> FeatureNomenclatureTool::FeatureDisplayPosition::edgePoints()
      const {
    return *m_edgePoints;
  }


  /**
   * Swap member data with another instance of this class. This trades the state
   *   of this with other and will never throw an exception.
   *
   * @param other The instance to trade state with
   */
  void FeatureNomenclatureTool::FeatureDisplayPosition::swap(
      FeatureDisplayPosition &other) {
    std::swap(m_textRect, other.m_textRect);
    std::swap(m_fullDisplayRect, other.m_fullDisplayRect);
    std::swap(m_edgePoints, other.m_edgePoints);
  }


  /**
   * Assign the state of rhs to this.This is exception-safe.
   *
   * @param rhs The position on the right hand side of the '=' operator
   *
   * @return *this
   */
  FeatureNomenclatureTool::FeatureDisplayPosition &
      FeatureNomenclatureTool::FeatureDisplayPosition::operator=(
        const FeatureDisplayPosition &rhs) {
    FeatureDisplayPosition copy(rhs);
    swap(copy);
    return *this;
  }


  /**
   * Create a blank feature display.
   */
  FeatureNomenclatureTool::ViewportFeatureDisplay::ViewportFeatureDisplay() {
    m_sourceViewport = NULL;
    m_features = NULL;
    m_featureScreenAreas = NULL;
    m_viewportCubeRange = NULL;

    m_features = new QList<FeaturePosition>;
    m_featureScreenAreas = new QList<FeatureDisplayPosition>;
    m_viewportCubeRange = new QPair<QPointF, QPointF>;
  }


  /**
   * Create a feature display for a given viewport.
   *
   * @param tool The tool that has the appropriate view settings
   * @param sourceViewport The viewport that this display will be used for
   * @param features The named features that are in the image in the viewport
   * @param vectorType The type of extent vector to display
   */
  FeatureNomenclatureTool::ViewportFeatureDisplay::ViewportFeatureDisplay(
      FeatureNomenclatureTool *tool, MdiCubeViewport *sourceViewport,
      QList<FeatureNomenclature::Feature> features, VectorType vectorType) {
    m_sourceViewport = sourceViewport;
    m_features = NULL;
    m_featureScreenAreas = NULL;
    m_viewportCubeRange = NULL;

    m_features = new QList<FeaturePosition>;
    m_featureScreenAreas = new QList<FeatureDisplayPosition>;
    m_viewportCubeRange = new QPair<QPointF, QPointF>;

    sort(features.begin(), features.end(),
          &FeatureNomenclature::featureDiameterGreaterThan);

    for (int i = 0; i < features.count(); i++) {
      FeaturePosition display(sourceViewport, features[i], vectorType);
      if (display.isValid()) {
        m_features->append(display);
      }
    }

    handleViewChanged(tool);
  }


  /**
   * Copy another feature display.
   *
   * @param other The feature display to copy
   */
  FeatureNomenclatureTool::ViewportFeatureDisplay::ViewportFeatureDisplay(
      const ViewportFeatureDisplay &other) {
    m_sourceViewport = other.m_sourceViewport;
    m_features = NULL;
    m_featureScreenAreas = NULL;
    m_viewportCubeRange = NULL;

    m_features = new QList<FeaturePosition>(*other.m_features);
    m_featureScreenAreas = new QList<FeatureDisplayPosition>(
        *other.m_featureScreenAreas);
    m_viewportCubeRange = new QPair<QPointF, QPointF>(
        *other.m_viewportCubeRange);
  }


  /**
   * Cleans up memory allocated by this feature display.
   */
  FeatureNomenclatureTool::ViewportFeatureDisplay::~ViewportFeatureDisplay() {
    m_sourceViewport = NULL;

    delete m_features;
    m_features = NULL;

    delete m_featureScreenAreas;
    m_featureScreenAreas = NULL;

    delete m_viewportCubeRange;
    m_viewportCubeRange = NULL;
  }


  /**
   * Apply the extent type to all of the features for the source viewport.
   *
   * @param vectorType The type of extents to be drawn
   */
  void FeatureNomenclatureTool::ViewportFeatureDisplay::applyExtentType(VectorType vectorType) {
    for (int i = 0; i < m_features->count(); i++) {
      (*m_features)[i].applyExtentType(vectorType);
    }
  }


  /**
   * Center the viewport on this feature. This also brings the feature to the
   *   top of the drawing priority list.
   *
   * @param feature The feature to center on
   */
  void FeatureNomenclatureTool::ViewportFeatureDisplay::centerFeature(
      FeatureNomenclature::Feature feature) {
    QString displayName = feature.displayName();

    int foundIndex = -1;
    for (int i = 0; foundIndex == -1 && i < m_features->count(); i++) {
      if (displayName == m_features->at(i).feature().displayName()) {
        foundIndex = i;
      }
    }

    if (foundIndex != -1) {
      m_features->prepend(m_features->takeAt(foundIndex));
      m_featureScreenAreas->prepend(m_featureScreenAreas->takeAt(foundIndex));

      // The center() method creates artifacts/displays bad data in the viewport
      //   ... the old 'cube' before centering stays around.
      m_sourceViewport->setScale(m_sourceViewport->scale(),
                                 m_features->first().center().first,
                                 m_features->first().center().second);
      m_sourceViewport->viewport()->update();
    }
  }


  /**
   * Get a list of features available on this viewport.
   *
   * @return The features that successfully project into the image on this
   *         viewport
   */
  QList<FeatureNomenclature::Feature>
      FeatureNomenclatureTool::ViewportFeatureDisplay::features() {
    QList<FeatureNomenclature::Feature> featureList;

    for (int i = 0; i < m_features->count(); i++)
      featureList.append((*m_features)[i].feature());

    return featureList;
  }


  /**
   * Get the list of feature positions for a given display.
   *
   * @return The feature positions of the display.
   */
  QList<FeatureNomenclatureTool::FeaturePosition>
      FeatureNomenclatureTool::ViewportFeatureDisplay::featurePositions() {
    QList<FeatureNomenclatureTool::FeaturePosition> positionList;

    for (int i = 0; i < m_features->count(); i++)
      positionList.append((*m_features)[i]);

    return positionList;
  }


  /**
   * Get the viewport associated with this feature display.
   *
   * @return The viewport that this display is supposed to work with.
   */
  MdiCubeViewport *
      FeatureNomenclatureTool::ViewportFeatureDisplay::sourceViewport() const {
    return m_sourceViewport;
  }


  /**
   * Paint features onto the viewport.
   *
   * @param painter The painter to use for painting on the viewport
   * @param showVectors True if we're painting the vectors
   * @param vectorType The extent type to paint
   * @param approvedOnly True if only painting approved features
   */
  void FeatureNomenclatureTool::ViewportFeatureDisplay::paint(
      QPainter *painter, bool showVectors, VectorType vectorType, bool approvedOnly) const {

    if (viewportCubeRange() == *m_viewportCubeRange) {

      for (int i = 0; i < m_features->count() &&
                      i < m_featureScreenAreas->count(); i++) {

        FeatureNomenclature::Feature feature = (*m_features)[i].feature();
        FeatureDisplayPosition pos = (*m_featureScreenAreas)[i];
        QRect textArea = pos.textArea();
        QRect fullArea = pos.displayArea();

        if (!fullArea.isNull() && fullArea != textArea && showVectors) {
          // For efficiency's sake
          QRect startRect = textArea.adjusted(-2, -2, 2, 2);
          QLineF topTextBorder(startRect.topLeft(), startRect.topRight());
          QLineF rightTextBorder(startRect.topRight(), startRect.bottomRight());
          QLineF bottomTextBorder(startRect.bottomLeft(),
                                  startRect.bottomRight());
          QLineF leftTextBorder(startRect.topLeft(), startRect.bottomLeft());

          QList<QLine> vectors;

          if (vectorType != Box) {
            foreach (QPoint point, pos.edgePoints()) {
              QLineF fullVector(textArea.center(), point);
              QPoint newVectorStart;

              QPointF intersectionPoint;

              if (point.y() < textArea.top()) {
                if (topTextBorder.intersects(fullVector, &intersectionPoint) ==
                    QLineF::BoundedIntersection) {
                  newVectorStart = QPoint(qRound(intersectionPoint.x()),
                                          qRound(intersectionPoint.y()));
                }
              }

              if (point.x() > textArea.right()) {
                if (rightTextBorder.intersects(fullVector, &intersectionPoint) ==
                    QLineF::BoundedIntersection) {
                  newVectorStart = QPoint(qRound(intersectionPoint.x()),
                                          qRound(intersectionPoint.y()));
                }
              }

              if (point.y() > textArea.bottom()) {
                if (bottomTextBorder.intersects(fullVector, &intersectionPoint) ==
                    QLineF::BoundedIntersection) {
                  newVectorStart = QPoint(qRound(intersectionPoint.x()),
                                          qRound(intersectionPoint.y()));
                }
              }

              if (point.x() < textArea.left()) {
                if (leftTextBorder.intersects(fullVector, &intersectionPoint) ==
                    QLineF::BoundedIntersection) {
                  newVectorStart = QPoint(qRound(intersectionPoint.x()),
                                          qRound(intersectionPoint.y()));
                }
              }

              if (!newVectorStart.isNull() &&
                  QLineF(newVectorStart, point).length() > 10) {
                vectors.append(QLine(newVectorStart, point));
              }
            }

            foreach (QLine vector, vectors) {
              painter->drawLine(vector);

              //For 4 or 8 arrows
              Angle normalAngle(-1 * QLineF(vector).normalVector().angle(),
                                Angle::Degrees);

              int magnitude = 10;
              double deltaX = magnitude * cos(normalAngle.radians());
              double deltaY = magnitude * sin(normalAngle.radians());

              QPoint normalStart(vector.x2() + deltaX, vector.y2() + deltaY);
              QPoint normalEnd(vector.x2() - deltaX, vector.y2() - deltaY);
              painter->drawLine(normalStart, normalEnd);

              Angle arrowheadAngle(30, Angle::Degrees);
              Angle vectorAngle(-1 * QLineF(vector).angle(), Angle::Degrees);
              Angle leftHead = vectorAngle - arrowheadAngle;
              Angle rightHead = vectorAngle + arrowheadAngle;

              int arrowheadMag = 10;
              deltaX = arrowheadMag * cos(leftHead.radians());
              deltaY = arrowheadMag * sin(leftHead.radians());
              painter->drawLine(
                  vector.p2(), vector.p2() - QPoint(deltaX, deltaY));

              deltaX = arrowheadMag * cos(rightHead.radians());
              deltaY = arrowheadMag * sin(rightHead.radians());
              painter->drawLine(
                  vector.p2(), vector.p2() - QPoint(deltaX, deltaY));
            }
          }
          else {
            // vector type == Box, draw the box
            if (pos.edgePoints().count() == 4) {
              QPolygon boundingPoly(pos.edgePoints().toVector());
              painter->drawPolygon(boundingPoly);
            }
          }
        }

        if (!textArea.isNull()) {
          QString featureName = feature.name();
          painter->drawText(textArea, featureName);
        }
      }
    }
  }


  /**
   * Handle a mouse click event on the viewport.
   *
   * @param tool The feature nomenclature tool that can show the user
   *             informative dialogs and can be controlled through this action.
   * @param p The viewport screen pixel coordinates of the mouse click
   * @param s The mouse buttons that were clicked.
   */
  void FeatureNomenclatureTool::ViewportFeatureDisplay::handleMouseClicked(
      FeatureNomenclatureTool *tool, QPoint p, Qt::MouseButton s) {
    for (int i = 0; i < m_featureScreenAreas->count(); i++) {
      QRect screenArea = m_featureScreenAreas->at(i).displayArea();

      if (screenArea.contains(p)) {
        FeatureNomenclature::Feature feature = m_features->at(i).feature();
        if (s == Qt::LeftButton) {
          tool->showFeatureDetails(feature);
        }
        else if (s == Qt::RightButton) {
          QMenu menu;

          QAction *title = menu.addAction(feature.displayName());
          title->setEnabled(false);
          menu.addSeparator();

          QAction *details = menu.addAction("Details...");
          QAction *website = menu.addAction("Website...");
          menu.addSeparator();
          QAction *center = menu.addAction("Center on Feature");
          QAction *copyUrl = menu.addAction("Copy Website URL");

          // This manual adjustment of the pos is a hack... probably due to our
          //   cursors.
          QAction *selectedAction = menu.exec(
              m_sourceViewport->viewport()->mapToGlobal(p) +
              QPoint(0, 20), details);

          if (selectedAction == details) {
            tool->showFeatureDetails(feature);
          }
          else if (selectedAction == website) {
            tool->showFeatureWebsite(feature);
          }
          else if (selectedAction == center) {
            tool->centerOnFeature(m_sourceViewport, feature);
          }
          else if (selectedAction == copyUrl) {
            QApplication::clipboard()->setText(
                feature.referenceUrl().toString());
          }
        }
      }
    }
  }


  /**
   * The display options or area on the viewport has changed. We need to
   *   figure out what's visible where again.
   *
   * @param tool The nomenclature tool with the appropriate view preferences
   */
  void FeatureNomenclatureTool::ViewportFeatureDisplay::handleViewChanged(
      FeatureNomenclatureTool *tool) {
    m_featureScreenAreas->clear();

    QFont fontToUse;
    fontToUse.setPointSize(tool->fontSize());
    QFontMetrics fontMetrics(fontToUse);

    // Don't draw text that overlaps existing text.
    QList<QRect> rectsToAvoid;

    for (int i = 0; i < m_features->count(); i++) {
      FeatureNomenclature::Feature feature = (*m_features)[i].feature();

      m_featureScreenAreas->append(FeatureDisplayPosition());

      if ( !tool->m_showApprovedOnly ||
            (tool->m_showApprovedOnly && feature.status() == FeatureNomenclature::Approved) ) {

        double sample = (*m_features)[i].center().first;
        double line = (*m_features)[i].center().second;

        int viewportX;
        int viewportY;
        m_sourceViewport->cubeToViewport(sample, line,
                                        viewportX, viewportY);

        QString featureName = feature.name();
        QRect textDisplayArea(QPoint(viewportX, viewportY),
                              QSize(fontMetrics.horizontalAdvance(featureName) + 4,
                                    fontMetrics.height()));
        // Center the text on the viewportX,Y instead of starting it there...
        textDisplayArea.moveTopLeft(textDisplayArea.topLeft() -
            QPoint(textDisplayArea.width() / 2, textDisplayArea.height() / 2));

        bool canDisplay = false;
        if (textDisplayArea.left() < m_sourceViewport->width() &&
            textDisplayArea.right() > 0 &&
            textDisplayArea.top() < m_sourceViewport->height() &&
            textDisplayArea.bottom() > 0) {
          canDisplay = true;
        }

        QRect fullDisplayArea = textDisplayArea;
        QPair<double, double> edge;
        QList<QPoint> edgeScreenPoints;

        if (canDisplay && tool->vectorType() != None) {
          QList< QPair<double, double> > edges = (*m_features)[i].edges();
          foreach (edge, edges) {
            m_sourceViewport->cubeToViewport(edge.first, edge.second,
                                            viewportX, viewportY);
            edgeScreenPoints.append(QPoint(viewportX, viewportY));
          }

          if (tool->vectorType() != Box) {
            foreach (QPoint screenPoint, edgeScreenPoints) {
              fullDisplayArea = fullDisplayArea.united(
                  QRect(screenPoint.x() - 3, screenPoint.y() - 3, 6, 6));
            }
          }
          else if (edges.count() == 4) {

            QPolygon boundingPoly(edgeScreenPoints.toVector());

            if (boundingPoly.intersected(textDisplayArea) == QPolygon(textDisplayArea, true)) {
              fullDisplayArea = boundingPoly.boundingRect();
            }
          }
        }

        if (canDisplay) {
          // If we intersect another feature, do not draw
          foreach (QRect rectToAvoid, rectsToAvoid) {
            if (canDisplay && fullDisplayArea.intersects(rectToAvoid)) {
              canDisplay = false;
            }
          }
        }

        if (canDisplay) {
          rectsToAvoid.append(fullDisplayArea);
          m_featureScreenAreas->last() =  FeatureDisplayPosition(textDisplayArea, fullDisplayArea,
                                                                 edgeScreenPoints);
        }
      }
      *m_viewportCubeRange = viewportCubeRange();
    }
  }


  /**
   * Swap *this and other's member data in an exception-free way.
   *
   * @param other The instance to trade member data with
   */
  void FeatureNomenclatureTool::ViewportFeatureDisplay::swap(
      ViewportFeatureDisplay &other) {
    std::swap(m_sourceViewport, other.m_sourceViewport);
    std::swap(m_features, other.m_features);
    std::swap(m_featureScreenAreas, other.m_featureScreenAreas);
    std::swap(m_viewportCubeRange, other.m_viewportCubeRange);
  }


  /**
   * Copy the data of rhs into *this. This assignment operator is exception
   *   safe.
   *
   * @param rhs The instance on the right hand side of the '=' operator.
   * @return *this
   */
  FeatureNomenclatureTool::ViewportFeatureDisplay &
      FeatureNomenclatureTool::ViewportFeatureDisplay::operator=(
              const ViewportFeatureDisplay &rhs) {
    ViewportFeatureDisplay copy(rhs);
    swap(copy);
    return *this;
  }


  /**
   * Get the min/max cube line/sample positions of the viewport. This is
   * designed to be used to detect viewport repositioning/screen pixels changing
   * to block painting when we're out of sync.
   *
   * @return The pair of minimum coordinates
   */
  QPair<QPointF, QPointF>
      FeatureNomenclatureTool::ViewportFeatureDisplay::viewportCubeRange()
      const {
    QPointF minValues;
    sourceViewport()->viewportToCube(1, 1, minValues.rx(), minValues.ry());

    QPointF maxValues;
    sourceViewport()->viewportToCube(sourceViewport()->viewport()->width(),
                                     sourceViewport()->viewport()->height(),
                                     maxValues.rx(), maxValues.ry());

    return QPair<QPointF, QPointF>(minValues, maxValues);
  }
}