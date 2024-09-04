#include "ProjectionConfigDialog.h"

#include <sstream>

#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>

#include "IException.h"
#include "MosaicSceneWidget.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "Pvl.h"

using std::stringstream;

namespace Isis {
  /**
   * Create a projection configuration dialog.
   *
   * @param scene The mosaic scene for which we're going to set the projection, this must not be
   *               NULL.
   * @param parent The Qt-parent for this dialog.
   */
  ProjectionConfigDialog::ProjectionConfigDialog(MosaicSceneWidget *scene, QWidget *parent) :
      QDialog(parent) {
    m_scene = scene;
    m_dirty = false;
    m_quick = false;

    QGridLayout *mainLayout = new QGridLayout;

    /*
     *  The layout is shown below:
     *
     *  |--------------------------------------------------------------|
     *  | Header                                                       |
     *  |--------------------------------------------------------------|
     *  | Description                                                  |
     *  |--------------------------------------------------------------|
     *  | Load From File... Save To File...                            |
     *  |--------------------------------------------------------------|
     *  | Text Edit                                                    |
     *  |--------------------------------------------------------------|
     *  | Errors                                                       |
     *  |--------------------------------------------------------------|
     *  | Show Errors                            Ok Apply Cancel       |
     *  |--------------------------------------------------------------|
     */

    int row = 0;
    QLabel *headerLabel = new QLabel("<h3>Configure Projection/Mapping Parameters</h3>");
    mainLayout->addWidget(headerLabel, row, 0);
    row++;

    QLabel *descriptionLabel = new QLabel("The projection determines how the footprints will be "
        "shown on the scene. This projection will be used to convert from latitude/longitude to "
        "scene coordinates (x, y).<br/><br/>Please keep in mind:<br/><b>Load Map File...</b> will "
        "read all of the keywords in the mapping group from the input file (unnecessary keywords "
        "included).<br/>"
        "<b>Save Map File...</b> will save what's currently in the display (unnecessary keywords "
        "included).<br/>"
        "<b>Ok and Apply</b> will remove all unnecessary or unknown keywords immediately.<br/>");
    descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(descriptionLabel, row, 0);
    row++;

    QHBoxLayout *loadSaveLayout = new QHBoxLayout;

    QPushButton *saveToFile = new QPushButton("&Save Map File...");
    connect(saveToFile, SIGNAL(clicked()),
            this, SLOT(saveToFile()));
    loadSaveLayout->addWidget(saveToFile);

    m_readFromFileButton = new QPushButton("&Load Map File...");
    connect(m_readFromFileButton, SIGNAL(clicked()),
            this, SLOT(loadFromFile()));
    loadSaveLayout->addWidget(m_readFromFileButton);

    loadSaveLayout->addStretch();
    mainLayout->addLayout(loadSaveLayout, row, 0);
    row++;

    QFont font("Monospace");
    font.setStyleHint(QFont::TypeWriter);

    m_mapFileEdit = new QTextEdit;
    m_mapFileEdit->setFont(font);
    connect(m_mapFileEdit, SIGNAL(textChanged()),
            this, SLOT(refreshWidgetStates()));
    mainLayout->addWidget(m_mapFileEdit, row, 0);
    row++;

    m_stateLabel = new QLabel;
    mainLayout->addWidget(m_stateLabel, row, 0);
    row++;
    
    m_errorsLabel = new QLabel;
    m_errorsLabel->setWordWrap(true);
    mainLayout->addWidget(m_errorsLabel, row, 0);
    row++;

    QHBoxLayout *applyButtonsLayout = new QHBoxLayout;

    QCheckBox *showErrorsCheckBox = new QCheckBox(tr("Show Errors"));
    connect(showErrorsCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(showErrors(int)));
    showErrors(showErrorsCheckBox->isChecked());
    applyButtonsLayout->addWidget(showErrorsCheckBox);

    applyButtonsLayout->addStretch();

    m_okayButton = new QPushButton("&Ok");
    m_okayButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(applySettings()));
    connect(m_okayButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    applyButtonsLayout->addWidget(m_okayButton);

    m_applyButton = new QPushButton("&Apply");
    m_applyButton->setIcon(QIcon::fromTheme("dialog-ok-apply"));
    connect(m_applyButton, SIGNAL(clicked()),
            this, SLOT(applySettings()));
    applyButtonsLayout->addWidget(m_applyButton);

    QPushButton *cancelButton = new QPushButton("&Cancel");
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    applyButtonsLayout->addWidget(cancelButton);

    QWidget *applyButtonsWrapper = new QWidget;
    applyButtonsWrapper->setLayout(applyButtonsLayout);
    mainLayout->addWidget(applyButtonsWrapper, row, 0);
    row++;

    setLayout(mainLayout);

    readSettings();

    connect(this, SIGNAL(shown()),
            this, SLOT(beginQuickLoad()),
            Qt::QueuedConnection);
  }


  ProjectionConfigDialog::~ProjectionConfigDialog() {
    m_scene = NULL;
  }


  /**
   * Enable/disable minimal interaction mode. This is defaulted to false. Setting this option will
   *   remove most mouse clicks from the interface and try to immediately use a projection that the
   *   user is expected to select from a file.
   *
   * @param quick True to minimize clicks/interaction, false (default) to use standard behavior.
   */
  void ProjectionConfigDialog::setQuickConfig(bool quick) {
    m_quick = quick;
  }


  /**
   * Take the settings that have been configured and apply them to the mosaic scene.
   */
  void ProjectionConfigDialog::applySettings() {
    try {
      if (m_scene && m_dirty) {
        m_scene->setProjection(createProjection());
      }
    }
    catch (IException &) {
    }

    readSettings();
  }


  /**
   * Update the current widgets' states with the current settings in the
   *   mosaic scene.
   */
  void ProjectionConfigDialog::readSettings() {
    if (m_scene && m_scene->getProjection()) {
      Pvl mapFilePvl;
      mapFilePvl += m_scene->getProjection()->Mapping();

      stringstream mapFileStringStream;
      mapFileStringStream << mapFilePvl;

      m_mapFileEdit->setText(QString::fromStdString(mapFileStringStream.str()));
    }

    refreshWidgetStates();

    m_dirty = false;
  }



  void ProjectionConfigDialog::showEvent(QShowEvent *event) {
    QDialog::showEvent(event);

    emit shown();
  }


  /**
   * Get a modified mapping pvl that the mosaic scene will be compatible with
   */
  Pvl ProjectionConfigDialog::addMissingKeywords(Pvl mappingPvl) {
    PvlGroup &mapping = mappingPvl.findGroup("Mapping", Pvl::Traverse);

    if(!mapping.hasKeyword("MinimumLatitude"))
      mapping += PvlKeyword("MinimumLatitude", "-90");

    if(!mapping.hasKeyword("MaximumLatitude"))
      mapping += PvlKeyword("MaximumLatitude", "90");

    if(!mapping.hasKeyword("MinimumLongitude")) {
      if(mapping["LongitudeDomain"][0] == "360")
        mapping += PvlKeyword("MinimumLongitude", "0");
      else
        mapping += PvlKeyword("MinimumLongitude", "-180");
    }

    if(!mapping.hasKeyword("MaximumLongitude")) {
      if(mapping["LongitudeDomain"][0] == "360")
        mapping += PvlKeyword("MaximumLongitude", "360");
      else
        mapping += PvlKeyword("MaximumLongitude", "180");
    }

    return mappingPvl;
  }


  /**
   * Convert the current text in the text edit to a projection.
   *
   * @return The projection specified in the text edit
   */
  Projection *ProjectionConfigDialog::createProjection() {
    stringstream mapFileStringStream;
    mapFileStringStream.str(m_mapFileEdit->toPlainText().toStdString());

    Pvl mapFilePvl;
    mapFileStringStream >> mapFilePvl;
    mapFilePvl = addMissingKeywords(mapFilePvl);

    return ProjectionFactory::Create(mapFilePvl);
  }


  /**
   * If using quick load, this will prompt the user for an input file right after the show event.
   *   We can't do this inside the show event because we can't accept() or reject() in the
   *   show event.
   */
  void ProjectionConfigDialog::beginQuickLoad() {
    if (m_quick && m_readFromFileButton) {
      m_readFromFileButton->click();
    }
  }


  /**
   * Read mapping parameters from a file (prompts user for the file name). This works with cubes.
   *   The mapping parameters will then be put into the text edit.
   */
  void ProjectionConfigDialog::loadFromFile() {
    QString mapFile = QFileDialog::getOpenFileName(this, tr("Select Map File"), QString("."),
        tr("Map Files (*.map *.pvl *.cub);;Text Files (*.txt);;All Files (*)"));

    if (!mapFile.isEmpty()) {
      bool success = false;

      try {
        Pvl mapFilePvl(mapFile.toStdString());
        PvlGroup &mapping = mapFilePvl.findGroup("Mapping", PvlObject::Traverse);

        Pvl trimmedMapFilePvl;
        trimmedMapFilePvl += mapping;

        stringstream trimmedMapFileStringStream;
        trimmedMapFileStringStream << trimmedMapFilePvl;
        m_mapFileEdit->setText(QString::fromStdString(trimmedMapFileStringStream.str()));

        success = true;
      }
      catch (IException &e) {
        QMessageBox::warning(this, tr("Failed to Load Map File"),
                             tr("Failed to load projection from the given file.\n") +
                             QString::fromStdString(e.toString()));
      }


      if (m_quick && m_okayButton && success) {
        try {
          delete createProjection();
          m_okayButton->click();
          close();
        }
        catch (IException &) {
        }
      }
    }

    // After we've tried one load, quick turns off. They either completed what they wanted, or
    //   need to evaluate what this dialog is telling them and modify their input.
    m_quick = false;
  }


  /**
   * Save mapping parameters to the given file. This overwrites the output file with the contents
   *   of the text edit.
   */
  void ProjectionConfigDialog::saveToFile() {
    QString mapFile = QFileDialog::getSaveFileName(this, tr("Save Map File"), QString("."),
        tr("Map Files (*.map *.pvl);;Text Files (*.txt);;All Files (*)"));

    if (!mapFile.isEmpty()) {
      QFile outputFile(mapFile);

      if (outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QString mapFileInfo = m_mapFileEdit->toPlainText() + "\n";
        if (outputFile.write(mapFileInfo.toLatin1()) == -1) {
          QMessageBox::warning(this, tr("Failed to Write Text to File"),
              tr("Failed to write the map file to [%1] due to an I/O failure").arg(mapFile));
        }
      }
      else {
        QMessageBox::warning(this, tr("Failed to Create Output File"),
                             tr("Failed to open file [%1] for writing").arg(mapFile));
      }
    }
  }


  /**
   * Update the enabled/disabled states of the various widgets based on the
   *   current user inputs' states.
   */
  void ProjectionConfigDialog::refreshWidgetStates() {
    bool projectionIsGood = true;

    if (m_mapFileEdit->toPlainText().trimmed().isEmpty()) {
      m_stateLabel->setText("<strong>Please load (or type in) a map file</strong>");
      m_errorsLabel->setText("");
      projectionIsGood = false;
    }
    else {
      try {
        delete createProjection();
        m_stateLabel->setText("<strong>The currently displayed text is valid</strong>");
        m_errorsLabel->setText("");
      }
      catch (IException &e) {
        m_stateLabel->setText("<strong>The currently displayed text is not valid"
                              "</strong>");
        m_errorsLabel->setText(
            QString("<font color='red'>&nbsp;&nbsp;") +
            QString(e.what()).replace("\n", "<br/>&nbsp;&nbsp;") +
            QString("</font>"));
        projectionIsGood = false;
      }
    }

    m_dirty = true;

    m_okayButton->setEnabled(projectionIsGood);
    m_applyButton->setEnabled(projectionIsGood);
  }


  /**
   * This is called when "Show Errors" is checked. This hides/shows the errors label depending
   *   on the given check state (0 = hide, 1 = show).
   *
   * @param shouldShowErrors 0 for hiding errors, 1 for showing
   */
  void ProjectionConfigDialog::showErrors(int shouldShowErrors) {
    m_errorsLabel->setVisible(shouldShowErrors);
  }
}
