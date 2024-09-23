#include "StretchTool.h"

#include <sstream>

#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QPushButton>
#include <QSizePolicy>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QValidator>
#include <QInputDialog>

#include "AdvancedStretchDialog.h"
#include "Brick.h"
#include "Blob.h"
#include "CubeViewport.h"
#include "Histogram.h"
#include "IException.h"
#include "IString.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "RubberBandTool.h"
#include "Statistics.h"
#include "Stretch.h"
#include "ToolPad.h"
#include "ViewportBuffer.h"
#include "ViewportMainWindow.h"
#include "Workspace.h"

#include "CubeStretch.h"

using namespace std;

namespace Isis {
  /**
   * StretchTool constructor
   *
   *
   * @param parent
   */
  StretchTool::StretchTool(QWidget *parent) : Tool::Tool(parent) {
    m_chipViewportStretch = NULL;
    m_preGlobalStretches = NULL;
    m_advancedStretch = NULL;

    m_chipViewportStretch = new Stretch;

    m_advancedStretch = new AdvancedStretchDialog(parent);
    connect(m_advancedStretch, SIGNAL(stretchChanged()),
            this, SLOT(advancedStretchChanged()));
    connect(m_advancedStretch, SIGNAL(visibilityChanged()),
            this, SLOT(updateTool()));
    connect(m_advancedStretch, SIGNAL(saveToCube()),
            this, SLOT(saveStretchToCube()));
    connect(m_advancedStretch, SIGNAL(deleteFromCube()),
            this, SLOT(deleteFromCube()));
    connect(m_advancedStretch, SIGNAL(loadStretch()),
            this, SLOT(loadStretchFromCube()));

    QPushButton *hiddenButton = new QPushButton();
    hiddenButton->setVisible(false);
    hiddenButton->setDefault(true);

    m_stretchGlobal = new QAction(parent);
    m_stretchGlobal->setShortcut(Qt::CTRL + Qt::Key_G);
    m_stretchGlobal->setText("Global Stretch");
    connect(m_stretchGlobal, SIGNAL(triggered()), this, SLOT(stretchGlobal()));

    m_stretchRegional = new QAction(parent);
    m_stretchRegional->setShortcut(Qt::CTRL + Qt::Key_R);
    m_stretchRegional->setText("Regional Stretch");
    connect(m_stretchRegional, SIGNAL(triggered()), this, SLOT(stretchRegional()));

    // Emit a signal when an exception occurs and connect to the Warning object
    // to display Warning icon and the message
    ViewportMainWindow *parentMainWindow = qobject_cast<ViewportMainWindow *>(parent);
    if (parentMainWindow) {
      connect(this, SIGNAL(warningSignal(std::string &, const std::string)),
              parentMainWindow, SLOT(displayWarning(std::string &, const std::string &)));
    }
  }


  /**
   * Destructor
   */
  StretchTool::~StretchTool() {
    delete [] m_preGlobalStretches;
    m_preGlobalStretches = NULL;

    delete m_chipViewportStretch;
    m_chipViewportStretch = NULL;
  }


  /**
   * Adds the stretch action to the toolpad.
   *
   *
   * @param pad
   *
   * @return QAction*
   */
  QAction *StretchTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon(QPixmap(toolIconDir() + "/stretch_global.png"));
    action->setToolTip("Stretch (S)");
    action->setShortcut(Qt::Key_S);
    QString text  =
      "<b>Function:</b>  Change the stretch range of the cube.\
      <p><b>Shortcut:</b>  S</p> ";
    action->setWhatsThis(text);

    return action;
  }


  /**
   * Adds the stretch action to the given menu.
   *
   *
   * @param menu
   */
  void StretchTool::addTo(QMenu *menu) {
    menu->addAction(m_stretchGlobal);
    menu->addAction(m_stretchRegional);
  }


  /**
   * Creates the widget to add to the tool bar.
   *
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *StretchTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);

    QToolButton *butt = new QToolButton(hbox);
    butt->setAutoRaise(true);
    butt->setIconSize(QSize(22, 22));
    butt->setIcon(QPixmap(toolIconDir() + "/regional_stretch-2.png"));
    butt->setToolTip("Stretch");
    QString text  =
      "<b>Function:</b> Automatically compute min/max stretch using viewed \
      pixels in the band(s) of the active viewport.  That is, only pixels \
      that are visible in the viewport are used. \
      If the viewport is in RGB color all three bands will be stretched. \
      <p><b>Shortcut:</b>  Ctrl+R</p> \
      <p><b>Mouse:</b>  Left click \
      <p><b>Hint:</b>  Left click and drag for a local stretch.  Uses only \
      pixels in the red marquee</p>";
    butt->setWhatsThis(text);
    connect(butt, SIGNAL(clicked()), this, SLOT(stretchRegional()));
    m_stretchRegionalButton = butt;

    m_stretchBandComboBox = new QComboBox(hbox);
    m_stretchBandComboBox->setEditable(false);
    m_stretchBandComboBox->addItem("Red Band", Red);
    m_stretchBandComboBox->addItem("Green Band", Green);
    m_stretchBandComboBox->addItem("Blue Band", Blue);
    m_stretchBandComboBox->addItem("All Bands", All);
    m_stretchBandComboBox->setToolTip("Select Color");
    text =
      "<b>Function:</b> Selecting the color will allow the appropriate \
      min/max to be seen and/or edited in text fields to the right.";

//      The All option implies the same min/max will be applied
//      to all three colors (RGB) if either text field is edited";
    m_stretchBandComboBox->setWhatsThis(text);
    m_stretchBand = All;
    m_stretchBandComboBox->setCurrentIndex(
        m_stretchBandComboBox->findData(m_stretchBand));
    connect(m_stretchBandComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(stretchBandChanged(int)));

    // Create drop-down selection option to set min/max type
    p_minMaxTypeSelection = new QComboBox();
    p_minMaxTypeSelection->setToolTip("Min/Max Type");
    text =
      "<b>Function:</b> Select the minimum & maximum value types to \
      set the stretch to. The four options are: \
      <p>- Default: Min and max values are set to the \
        0.5 and 99.5 percentiles, respectively. \
      <p>- Best: The better of the absolute min/max or the \
        Chebyshev min/max. The better value is considered the value \
        closest to the mean. \
      <p>- Absolute: The absolute min/max value of all valid pixels. \
      <p>- Chebyshev: The min/max value such that a certain percentage \
        of data will fall within K standard deviations of the average \
        (Chebyshev's Theorem). It can be used to obtain a value that \
        does not include statistical outliers.";
    p_minMaxTypeSelection->setWhatsThis(text);
    p_minMaxTypeSelection->addItem("Default",      0);
    p_minMaxTypeSelection->addItem("Best",         1);
    p_minMaxTypeSelection->addItem("Absolute",     2);
    p_minMaxTypeSelection->addItem("Chebyshev",    3);

    connect(p_minMaxTypeSelection, SIGNAL(currentIndexChanged(int)), this, SLOT(changeStretch()));

    QDoubleValidator *dval = new QDoubleValidator(hbox);
    m_stretchMinEdit = new QLineEdit(hbox);
    m_stretchMinEdit->setValidator(dval);
    m_stretchMinEdit->setToolTip("Minimum");
    text =
      "<b>Function:</b> Shows the current minimum pixel value.  Pixel values \
      below minimum are shown as black.  Pixel values above the maximum \
      are shown as white or the highest intensity of red/green/blue \
      if in color. Pixel values between the minimum and maximum are stretched \
      linearly between black and white (or color component). \
      <p><b>Hint:</b>  You can manually edit the minimum but it must be \
      less than the maximum.";
    m_stretchMinEdit->setWhatsThis(text);
    m_stretchMinEdit->setMaximumWidth(100);
    connect(m_stretchMinEdit, SIGNAL(returnPressed()),
            this, SLOT(changeStretch()));

    m_stretchMaxEdit = new QLineEdit(hbox);
    m_stretchMaxEdit->setValidator(dval);
    m_stretchMaxEdit->setToolTip("Maximum");
    text =
      "<b>Function:</b> Shows the current maximum pixel value.  Pixel values \
      below minimum are shown as black.  Pixel values above the maximum \
      are shown as white or the highest intensity of red/green/blue \
      if in color. Pixel values between the minimum and maximum are stretched \
      linearly between black and white (or color component). \
      <p><b>Hint:</b>  You can manually edit the maximum but it must be \
      greater than the minimum";
    m_stretchMaxEdit->setWhatsThis(text);
    m_stretchMaxEdit->setMaximumWidth(100);
    connect(m_stretchMaxEdit, SIGNAL(returnPressed()), this, SLOT(changeStretch()));

    // Create the two menus that drop down from the buttons
    QMenu *copyMenu = new QMenu();
    QMenu *globalMenu = new QMenu();

    m_copyBands = new QAction(parent);
    m_copyBands->setText("to All Bands");
    connect(m_copyBands, SIGNAL(triggered(bool)), this, SLOT(setStretchAcrossBands()));

    QAction *copyAll = new QAction(parent);
    copyAll->setIcon(QPixmap(toolIconDir() + "/copy_stretch.png"));
    copyAll->setText("to All Viewports");
    connect(copyAll, SIGNAL(triggered(bool)), this, SLOT(setStretchAllViewports()));

    copyMenu->addAction(copyAll);
    copyMenu->addAction(m_copyBands);

    m_copyButton = new QToolButton();
    m_copyButton->setAutoRaise(true);
    m_copyButton->setIconSize(QSize(22, 22));
    m_copyButton->setIcon(QPixmap(toolIconDir() + "/copy_stretch.png"));
    m_copyButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_copyButton->setMenu(copyMenu);
    m_copyButton->setDefaultAction(copyAll);
    m_copyButton->setToolTip("Copy");
    text  =
      "<b>Function:</b> Copy the current stretch to all the \
      active viewports. Or use the drop down menu to copy the current stretch \
      to all the  bands in the active viewport. \
      <p><b>Hint:</b>  Can reset the stretch to an automaticaly computed \
      stretch by using the 'Reset' stretch button option. </p>";
    m_copyButton->setWhatsThis(text);

    QAction *currentView = new QAction(parent);
    currentView->setText("Active Viewport");
    currentView->setIcon(QPixmap(toolIconDir() + "/global_stretch.png"));
    globalMenu->addAction(currentView);
    connect(currentView, SIGNAL(triggered(bool)), this, SLOT(stretchGlobal()));

    QAction *globalAll = new QAction(parent);
    globalAll->setText("All Viewports");
    globalMenu->addAction(globalAll);
    connect(globalAll, SIGNAL(triggered(bool)), this, SLOT(stretchGlobalAllViewports()));

    QAction *globalBands = new QAction(parent);
    globalBands->setText("All Bands");
    globalMenu->addAction(globalBands);
    connect(globalBands, SIGNAL(triggered(bool)), this, SLOT(stretchGlobalAllBands()));

    m_globalButton = new QToolButton(); //basically acts as a 'reset'
    m_globalButton->setAutoRaise(true);
    m_globalButton->setIconSize(QSize(22, 22));
    m_globalButton->setPopupMode(QToolButton::MenuButtonPopup);
    m_globalButton->setMenu(globalMenu);
    m_globalButton->setDefaultAction(currentView);
    m_globalButton->setToolTip("Reset");
    text  =
      "<b>Function:</b> Reset the stretch to be automatically computed "
      "using the statisics from the entire image. Use the drop down menu "
      "to reset the stretch for all the bands in the active viewport or "
      "to reset the stretch for all the viewports.";
    m_globalButton->setWhatsThis(text);

    QPushButton *advancedButton = new QPushButton("Advanced");
    connect(advancedButton, SIGNAL(clicked()), this, SLOT(showAdvancedDialog()));

    m_flashButton = new QPushButton("Show Global");
    text  =
      "<b>Function:</b> While this button is pressed down, the visible stretch "
      "will be the automatically computed stretch using the statisics from the "
      "entire image. The original stretch is restored once you let up on this "
      "button.";
    m_flashButton->setWhatsThis(text);
    connect(m_flashButton, SIGNAL(pressed()), this, SLOT(stretchChanged()));
    connect(m_flashButton, SIGNAL(released()), this, SLOT(stretchChanged()));

    // Buttons migrated out of Advanced Stretch Tool
    QPushButton *saveToCubeButton = new QPushButton("Save");
    connect(saveToCubeButton, SIGNAL(clicked(bool)), this, SLOT(saveStretchToCube()));

    QPushButton *deleteFromCubeButton = new QPushButton("Delete");
    connect(deleteFromCubeButton, SIGNAL(clicked(bool)), this, SLOT(deleteFromCube()));

    QPushButton *loadStretchButton = new QPushButton("Restore");
    connect(loadStretchButton, SIGNAL(clicked(bool)), this, SLOT(loadStretchFromCube()));

    QHBoxLayout *layout = new QHBoxLayout(hbox);
    layout->setMargin(0);
    layout->addWidget(m_copyButton);
    layout->addWidget(m_globalButton);
    layout->addWidget(m_stretchRegionalButton);
    layout->addWidget(m_stretchBandComboBox);
    layout->addWidget(p_minMaxTypeSelection);
    layout->addWidget(m_stretchMinEdit);
    layout->addWidget(m_stretchMaxEdit);
    layout->addWidget(advancedButton);
    layout->addWidget(m_flashButton);

    // should only display if gray stretch
    // Save/Restore strech only supported for Grayscale images. Hide buttons if in RGB.
    layout->addWidget(saveToCubeButton);
    layout->addWidget(deleteFromCubeButton);
    layout->addWidget(loadStretchButton);

    layout->addStretch(); // Pushes everything else left in the menu bar
    hbox->setLayout(layout);

    return hbox;
  }


  /**
   * This updates the visible histograms in the advanced stretch,
   * if present.
   */
  void StretchTool::updateHistograms() {
    if(m_advancedStretch->isVisible()) {
      MdiCubeViewport *cvp = cubeViewport();

      if(!cvp) return;

      if(cvp->isGray() && !cvp->grayBuffer()->working()) {
        if(m_advancedStretch->isRgbMode()) {
          updateTool();
        }
        else {
          Histogram hist(histFromBuffer(cvp->grayBuffer()));

          if(hist.ValidPixels() > 0) {
            m_advancedStretch->updateHistogram(hist);
          }
        }
      }
      // Otherwise it is in color mode
      else if(!cvp->isGray() &&
              !cvp->redBuffer()->working() &&
              !cvp->greenBuffer()->working() &&
              !cvp->blueBuffer()->working()) {
        if(!m_advancedStretch->isRgbMode()) {
          updateTool();
        }
        else {
          Histogram redHist(histFromBuffer(cvp->redBuffer()));
          Histogram grnHist(histFromBuffer(cvp->greenBuffer()));
          Histogram bluHist(histFromBuffer(cvp->blueBuffer()));

          if(redHist.ValidPixels() > 0 &&
              grnHist.ValidPixels() > 0 &&
              bluHist.ValidPixels() > 0) {
            m_advancedStretch->updateHistograms(redHist, grnHist, bluHist);
          }
        }
      }
    }
  }


  /**
   * Update the streches and corresponding histograms for all the
   * colors Red, Green and Blue for Stretch All Mode.
   *
   * @author Sharmila Prasad (3/14/2011)
   */
  void StretchTool::updateAdvStretchDialogforAll(void)
  {
    if(m_advancedStretch->isVisible()) {
      MdiCubeViewport *cvp = cubeViewport();

      if(!cvp->isGray() &&
         !cvp->redBuffer()->working() &&
         !cvp->greenBuffer()->working() &&
         !cvp->blueBuffer()->working()) {

        Histogram redHist(histFromBuffer(cvp->redBuffer()));
        Histogram grnHist(histFromBuffer(cvp->greenBuffer()));
        Histogram bluHist(histFromBuffer(cvp->blueBuffer()));
        Stretch redStretch(cvp->redStretch());
        Stretch grnStretch(cvp->greenStretch());
        Stretch bluStretch(cvp->blueStretch());

        m_advancedStretch->updateForRGBMode(redStretch, redHist,
                                            grnStretch, grnHist,
                                            bluStretch, bluHist);
      }
    }
  }


  /**
   * Restores a saved stretch from the cube
   */
  void StretchTool::loadStretchFromCube(){
    MdiCubeViewport *cvp = cubeViewport();
    Cube* icube = cvp->cube();
    Pvl* lab = icube->label();

    QStringList namelist;

    // Create a list of existing Stretch names
    if (cvp->isGray()) {
      PvlObject::PvlObjectIterator objIter;
      for (objIter=lab->beginObject(); objIter<lab->endObject(); objIter++) {
        if (objIter->name() == "Stretch") {
          PvlKeyword tempKeyword = objIter->findKeyword("Name");
          int bandNumber = int(objIter->findKeyword("BandNumber"));
          if (cvp->grayBand() == bandNumber) {
            QString tempName = QString::fromStdString(tempKeyword[0]);
            namelist.append(tempName);
          }
        }
      }
    }
    else {
      int redBandNumber = cvp->redBand();
      int greenBandNumber = cvp->greenBand();
      int blueBandNumber = cvp->blueBand();

      QMap<QString, QList<int>> tempNameMap;
      PvlObject::PvlObjectIterator objIter;
      for (objIter=lab->beginObject(); objIter<lab->endObject(); objIter++) {
        if (objIter->name() == "Stretch") {
          PvlKeyword tempKeyword = objIter->findKeyword("Name");
          int bandNumber = int(objIter->findKeyword("BandNumber"));
          if (bandNumber == redBandNumber || bandNumber == greenBandNumber
              || bandNumber == blueBandNumber) {
            QString tempName = QString::fromStdString(tempKeyword[0]);
            if (tempNameMap.contains(tempName)) {
              tempNameMap[tempName].append(bandNumber);
            }
            else
            {
              tempNameMap[tempName] = {bandNumber};
            }
          }
        }
      }
      QMap<QString, QList<int>>::const_iterator i = tempNameMap.constBegin();
      while (i != tempNameMap.constEnd()) {
        if (i.value().contains(redBandNumber) && i.value().contains(greenBandNumber) &&
            i.value().contains(blueBandNumber) ){
          namelist.append(i.key());
        }
        ++i;
      }
    }

    bool ok = false;
    QString stretchName;
    // Only display load stretch dialog if there are stretches saved to the cube
    if (namelist.size() >=1) {
      stretchName = QInputDialog::getItem((QWidget *)parent(), tr("Load Stretch"),
                                          tr("Name of Stretch to Load:"), namelist, 0,
                                          false, &ok);
    }
    else {
      QMessageBox::information((QWidget *)parent(), "Information",
                                   "There are no saved stretches to restore.");
    }

    if (ok) {
      if (cvp->isGray()) {
        CubeStretch cubeStretch = icube->readCubeStretch(stretchName);
        if (m_advancedStretch->isVisible()) {
          m_advancedStretch->restoreGrayStretch(cubeStretch);
        }
        // Get the current cube stretche and copy the new stretch pairs over so that the
        // special pixel values set in the viewport are maintained.
        CubeStretch grayOriginal = cvp->grayStretch();
        grayOriginal.CopyPairs(cubeStretch);
        cvp->stretchGray(grayOriginal);
      }
      else {
        std::vector<PvlKeyword> keywordValueRed;
        keywordValueRed.push_back(PvlKeyword("BandNumber",  toString(cvp->redBand())));

        std::vector<PvlKeyword> keywordValueGreen;
        keywordValueGreen.push_back(PvlKeyword("BandNumber", toString(cvp->greenBand())));

        std::vector<PvlKeyword> keywordValueBlue;
        keywordValueBlue.push_back(PvlKeyword("BandNumber", toString(cvp->blueBand())));

        CubeStretch redStretch = icube->readCubeStretch(stretchName, keywordValueRed);
        CubeStretch greenStretch = icube->readCubeStretch(stretchName, keywordValueGreen);
        CubeStretch blueStretch = icube->readCubeStretch(stretchName, keywordValueBlue);

        if (m_advancedStretch->isVisible()) {
          m_advancedStretch->restoreRgbStretch(redStretch, greenStretch, blueStretch);
        }

        // Get the current cube stretches and copy the new stretch pairs over so that the
        // special pixel values set in the viewport are maintained.
        CubeStretch redOriginal = cvp->redStretch();
        CubeStretch greenOriginal = cvp->greenStretch();
        CubeStretch blueOriginal = cvp->blueStretch();

        redOriginal.CopyPairs(redStretch);
        greenOriginal.CopyPairs(greenStretch);
        blueOriginal.CopyPairs(blueStretch);

        cvp->stretchRed(redOriginal);
        cvp->stretchGreen(greenOriginal);
        cvp->stretchBlue(blueOriginal);
      }
      emit stretchChanged();
    }
  }


  /**
   * Deletes a saved stretch from the cube
   */
  void StretchTool::deleteFromCube() {
    MdiCubeViewport *cvp = cubeViewport();
    Cube* icube = cvp->cube();
    Pvl* lab = icube->label();

    // Create a list of existing Stretch names
    QStringList namelist;
    PvlObject::PvlObjectIterator objIter;
    for (objIter=lab->beginObject(); objIter<lab->endObject(); objIter++) {
      if (objIter->name() == "Stretch") {
        PvlKeyword tempKeyword = objIter->findKeyword("Name");
        int bandNumber = int(objIter->findKeyword("BandNumber"));
        if (cvp->grayBand() == bandNumber) {
          QString tempName = QString::fromStdString(tempKeyword[0]);
          namelist.append(tempName);
        }
      }
    }

    bool ok = false;
    QString toDelete;
    // Only display list of stretches to delete if there are stretches saved to the cube
    if (namelist.size() >= 1) {
      toDelete = QInputDialog::getItem((QWidget *)parent(), tr("Delete Stretch"),
                                       tr("Name of Stretch to Delete:"), namelist, 0,
                                       false, &ok);
    }
    else {
      QMessageBox::information((QWidget *)parent(), "Information",
                               "There are no saved stretches to delete.");
    }

    if (ok) {
      if (icube->isReadOnly()) {
        try {
          cvp->cube()->reopen("rw");
        }
        catch(IException &) {
          cvp->cube()->reopen("r");
          QMessageBox::information((QWidget *)parent(), "Error",
                                   "Cannot open cube read/write to delete stretch");
          return;
        }
      }

      bool cubeDeleted = icube->deleteBlob(toDelete, "Stretch");

      if (!cubeDeleted) {
        QMessageBox msgBox;
        msgBox.setText("Stretch Could Not Be Deleted!");
        msgBox.setInformativeText("A stretch with name: \"" + toDelete +
            "\" Could not be found, so there was nothing to delete from the Cube.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
      }

      // Don't leave open rw -- not optimal.
      cvp->cube()->reopen("r");
    }
  }


  /**
   * Saves a strech to the cube.
   */
  void StretchTool::saveStretchToCube() {
    MdiCubeViewport *cvp = cubeViewport();
    Cube* icube = cvp->cube();
    Pvl* lab = icube->label();

    // Create a list of existing Stretch names
    QStringList namelist;
    PvlObject::PvlObjectIterator objIter;
    for (objIter=lab->beginObject(); objIter<lab->endObject(); objIter++) {
      if (objIter->name() == "Stretch") {
        PvlKeyword tempKeyword = objIter->findKeyword("Name");
        QString tempName = QString::fromStdString(tempKeyword[0]);
        namelist.append(tempName);
      }
    }

    bool ok;
    QString name;

    //
    // At this time, it is NOT possible to save an RGB stretch with the same band number
    // multiple times, if the saved stretch for each is different. If the saved stretch is the same
    // this is okay.
    //
    // For example, r=1, g=1, b=1, with the same stretch for each is okay
    //              r=1, g=1, b=2, where the stretches for band 1 are the same, is okay
    //              r=1, g=1, b=1, where the the stretches for band 1 are different (red stretch !=
    //                                                                               green stretch)
    //
    if (!cvp->isGray()) {
      CubeStretch redStretch, greenStretch, blueStretch;
      if (m_advancedStretch->isVisible()) {
        redStretch = m_advancedStretch->getRedStretch();
        greenStretch = m_advancedStretch->getGrnStretch();
        blueStretch = m_advancedStretch->getBluStretch();
      }
      else {
        redStretch = cvp->redStretch();
        greenStretch = cvp->greenStretch();
        blueStretch = cvp->blueStretch();
      }

      int redBand = cvp->redBand();
      int greenBand = cvp->greenBand();
      int blueBand = cvp->blueBand();

      if (((redBand == greenBand) && !(redStretch == greenStretch)) ||
          ((redBand == blueBand)  && !(redBand == blueBand)) ||
          ((greenBand == blueBand) && !(greenBand == blueBand))) {
        QMessageBox::information((QWidget *)parent(), "Error", "Sorry, cannot save RGB stretches which include the same band multiple times, but have different stretches for each");
        return;
      }
    }

    // "Get the name for the stretch" dialog
    QString text = QInputDialog::getText(m_advancedStretch, tr("Save Stretch"),
                                         tr("Enter a name to save the stretch as:"), QLineEdit::Normal,
                                         "stretch", &ok);

    if (ok) {
      // Stretch Name Already Exists Dialog!
      if (namelist.contains(text)) {
        QMessageBox msgBox;
        msgBox.setText(tr("Stretch Name Already Exists!"));
        msgBox.setInformativeText("A stretch pair with name: \"" + text + "\" already exists and "
                                  "the existing saved data will be overwritten. Are you sure you "
                                  "wish to proceed?");
        msgBox.setStandardButtons(QMessageBox::Save | QMessageBox::Cancel);
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int ret = msgBox.exec();

      switch (ret) {
      case QMessageBox::Save:
        break;
      case QMessageBox::Cancel:
          // Cancel was clicked, exit this function
        return;
        break;
      default:
          // should never be reached
        return;
        break;
        }
      }

      if (icube->isReadOnly()) {
        //  reOpen cube as read/write
        try {
          icube->reopen("rw");
        }
        catch(IException &) {
          icube->reopen("r");
          QMessageBox::information((QWidget *)parent(), "Error", "Cannot open cube read/write to save stretch");
          return;
        }
      }

      // If gray, only one stretch to save
      if (cvp->isGray()) {
        CubeStretch stretch;
        if (m_advancedStretch->isVisible()) {
          stretch = m_advancedStretch->getGrayStretch();
        }
        else {
          stretch = cvp->grayStretch();
        }

        // Write single stretch to cube
        stretch.setName(text);
        stretch.setBandNumber(cvp->grayBand());

        // Overwrite an existing stretch with the same name if it exists. The user was warned
        // and decided to overwrite.
        icube->write(stretch);
      }
      else {
        CubeStretch redStretch, greenStretch, blueStretch;
        if (m_advancedStretch->isVisible()) {
          redStretch = m_advancedStretch->getRedStretch();
          greenStretch = m_advancedStretch->getGrnStretch();
          blueStretch = m_advancedStretch->getBluStretch();
        }
        else {
          redStretch = cvp->redStretch();
          greenStretch = cvp->greenStretch();
          blueStretch = cvp->blueStretch();
        }

        redStretch.setName(text);
        redStretch.setBandNumber(cvp->redBand());
        Blob stretchBlob = redStretch.toBlob();
        icube->write(stretchBlob, false);

        greenStretch.setName(text);
        greenStretch.setBandNumber(cvp->greenBand());
        stretchBlob = greenStretch.toBlob();
        icube->write(stretchBlob, false);

        blueStretch.setName(text);
        blueStretch.setBandNumber(cvp->blueBand());
        stretchBlob = blueStretch.toBlob();
        icube->write(stretchBlob, false);
      }

      // Don't leave open rw -- not optimal.
      cvp->cube()->reopen("r");
    }
  }


  /**
   * This is called when the visible area changes.
   */
  void StretchTool::screenPixelsChanged() {
    updateHistograms();
  }


  /**
   * This updates the advanced stretch to use the given viewport
   *
   * @param cvp
   */
  void StretchTool::setCubeViewport(CubeViewport *cvp) {
    if(m_advancedStretch->isVisible()) {
      m_advancedStretch->enable(true);
      //If the viewport is in gray mode
      if(cvp->isGray() && !cvp->grayBuffer()->working()) {
        Histogram hist(histFromBuffer(cvp->grayBuffer()));
        Stretch stretch(cvp->grayStretch());
        m_advancedStretch->enableGrayMode(stretch, hist);
      }
      //Otherwise it is in color mode
      else if(!cvp->isGray() &&
              !cvp->redBuffer()->working() &&
              !cvp->greenBuffer()->working() &&
              !cvp->blueBuffer()->working()) {
        Histogram redHist(histFromBuffer(cvp->redBuffer()));
        Histogram grnHist(histFromBuffer(cvp->greenBuffer()));
        Histogram bluHist(histFromBuffer(cvp->blueBuffer()));
        Stretch redStretch(cvp->redStretch());
        Stretch grnStretch(cvp->greenStretch());
        Stretch bluStretch(cvp->blueStretch());
        m_advancedStretch->enableRgbMode(redStretch, redHist,
                                         grnStretch, grnHist,
                                         bluStretch, bluHist);
      }
      else {
        m_advancedStretch->enable(false);
      }
    }
    else {
      m_advancedStretch->enable(false);
    }
  }


  /**
   * Updates the stretch tool.
   *
   */
  void StretchTool::updateTool() {
    CubeViewport *cvp = cubeViewport();

    if(cvp == NULL) {
      //If the current viewport is NULL and the advanced dialog is visible, hide it
      if(m_advancedStretch->isVisible()) {
        m_advancedStretch->hide();
      }
    }
    else {
      if(!m_advancedStretch->enabled() ||
          m_advancedStretch->isRgbMode() != !cvp->isGray()) {
        setCubeViewport(cvp);
      }
    }

    if(cvp && cvp->isGray()) {
      m_copyBands->setEnabled(true);
      m_stretchBandComboBox->setVisible(false);
      m_stretchMinEdit->show();
      m_stretchMaxEdit->show();
    }
    else if(cvp) {
      m_copyBands->setEnabled(true);
      m_stretchBandComboBox->setVisible(true);
      stretchBandChanged(0);
    }
    else {
      m_copyBands->setEnabled(false);
      m_stretchBandComboBox->setVisible(false);
    }

    if(m_advancedStretch->isVisible()) {
      m_stretchMinEdit->setEnabled(false);
      m_stretchMaxEdit->setEnabled(false);
    }
    else {
      m_stretchMinEdit->setEnabled(true);
      m_stretchMaxEdit->setEnabled(true);
    }

    stretchChanged();
    updateHistograms();
  }


  /**
   * The cube viewport requested a stretch at this time, give it
   * to the viewport
   *
   * @param cvp
   * @param bandId
   */
  void StretchTool::stretchRequested(MdiCubeViewport *cvp, int bandId) {
    // Yeah this is a hack... but it's necessary to make this tool
    // do anything while its not the active tool.
    connect(cvp, SIGNAL(screenPixelsChanged()), this, SLOT(updateHistograms()));
    QRect rect(0, 0, cvp->viewport()->width(), cvp->viewport()->height());

    if(bandId == (int)Gray) {
      if(cvp->grayBuffer() && cvp->grayBuffer()->hasEntireCube()) {
        Stretch newStretch = cvp->grayStretch();
        newStretch.CopyPairs(stretchBuffer(cvp->grayBuffer(), rect));
        cvp->stretchGray(newStretch);
      }
      else {
        Stretch newStretch = stretchBand(cvp, (StretchBand)bandId);
        cvp->stretchGray(newStretch);
      }
    }
    else
    {
      if(bandId == (int)Red || bandId == (int)All) {
        if(cvp->redBuffer() && cvp->redBuffer()->hasEntireCube()) {
          Stretch newStretch = cvp->redStretch();
          newStretch.CopyPairs(stretchBuffer(cvp->redBuffer(), rect));
          cvp->stretchRed(newStretch);
        }
        else {
          Stretch newStretch = stretchBand(cvp, Red);
          cvp->stretchRed(newStretch);
        }
      }
      if(bandId == (int)Green || bandId == (int)All) {
        if(cvp->greenBuffer() && cvp->greenBuffer()->hasEntireCube()) {
          Stretch newStretch = cvp->greenStretch();
          newStretch.CopyPairs(stretchBuffer(cvp->greenBuffer(), rect));
          cvp->stretchGreen(newStretch);
        }
        else {
          Stretch newStretch = stretchBand(cvp, Green);
          cvp->stretchGreen(newStretch);
        }
      }
      if(bandId == (int)Blue || bandId == (int)All) {
        if(cvp->blueBuffer() && cvp->blueBuffer()->hasEntireCube()) {
          Stretch newStretch = cvp->blueStretch();
          newStretch.CopyPairs(stretchBuffer(cvp->blueBuffer(), rect));
          cvp->stretchBlue(newStretch);
        }
        else {
          Stretch newStretch = stretchBand(cvp, Blue);
          cvp->stretchBlue(newStretch);
        }
      }
    }
    stretchChanged();
  }


  /**
   * This method is called when the stretch has changed and sets the min/max
   * text fields to the correct values.
   *
   */
  void StretchTool::stretchChanged() {
    MdiCubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    if(m_flashButton->isDown()) {
      if(!m_preGlobalStretches) {
        m_preGlobalStretches = new Stretch[4];
        m_preGlobalStretches[0] = cvp->grayStretch();
        m_preGlobalStretches[1] = cvp->redStretch();
        m_preGlobalStretches[2] = cvp->greenStretch();
        m_preGlobalStretches[3] = cvp->blueStretch();
      }

      cvp->stretchKnownGlobal();
      return;
    }
    else if(m_preGlobalStretches) {
      if(cvp->isGray()) {
        cvp->stretchGray(m_preGlobalStretches[0]);
      }
      else {
        cvp->stretchRed(m_preGlobalStretches[1]);
        cvp->stretchGreen(m_preGlobalStretches[2]);
        cvp->stretchBlue(m_preGlobalStretches[3]);
      }

      delete [] m_preGlobalStretches;
      m_preGlobalStretches = NULL;
    }

    double min = 0, max = 0;
    //If the viewport is in gray mode
    if(cvp->isGray()) {
      //Get the min/max from the current stretch
      Stretch stretch = cvp->grayStretch();
      min = stretch.Input(0);
      max = stretch.Input(stretch.Pairs() - 1);
    }

    //Otherwise it is in color mode
    else {
      Stretch rstretch = cvp->redStretch();
      Stretch gstretch = cvp->greenStretch();
      Stretch bstretch = cvp->blueStretch();

      //Get the min/max from the current stretch
      if(m_stretchBand == Red) {
        min = rstretch.Input(0);
        max = rstretch.Input(rstretch.Pairs() - 1);
      }
      else if(m_stretchBand == Green) {
        min = gstretch.Input(0);
        max = gstretch.Input(gstretch.Pairs() - 1);
      }
      else if(m_stretchBand == Blue) {
        min = bstretch.Input(0);
        max = bstretch.Input(bstretch.Pairs() - 1);
      }
    }

    //Set the min/max text fields
    if(m_stretchBand != All || cvp->isGray()) {
      QString strMin;
      strMin.setNum(min);
      m_stretchMinEdit->setText(strMin);

      QString strMax;
      strMax.setNum(max);
      m_stretchMaxEdit->setText(strMax);
    }

    if(m_advancedStretch->isVisible()) {
      if(m_stretchBand == All){
        updateAdvStretchDialogforAll();
      }
      m_advancedStretch->updateStretch(cvp);
    }
  }


  /**
   * This is called when one of the advanced stretches changed.
   * Give the stretch to the viewport.
   */
  void StretchTool::advancedStretchChanged() {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    if(!m_advancedStretch->isRgbMode()) {
      Stretch grayStretch = cvp->grayStretch();
      grayStretch.ClearPairs();
      grayStretch.CopyPairs(m_advancedStretch->getGrayStretch());
      cvp->stretchGray(grayStretch);

      // send the stretch to any ChipViewports that want to listen
      *m_chipViewportStretch = grayStretch;
      emit stretchChipViewport(m_chipViewportStretch, cvp);
    }
    else {
      Stretch redStretch = cvp->redStretch();
      redStretch.ClearPairs();
      redStretch.CopyPairs(m_advancedStretch->getRedStretch());
      cvp->stretchRed(redStretch);

      Stretch grnStretch = cvp->greenStretch();
      grnStretch.ClearPairs();
      grnStretch.CopyPairs(m_advancedStretch->getGrnStretch());
      cvp->stretchGreen(grnStretch);

      Stretch bluStretch = cvp->blueStretch();
      bluStretch.ClearPairs();
      bluStretch.CopyPairs(m_advancedStretch->getBluStretch());
      cvp->stretchBlue(bluStretch);
    }
    stretchChanged();
  }


  /**
   * This method is called when the stretch has changed and sets the min/max
   * text fields to the correct values.
   *
   *  Does not effect All as the min/max lineedits are hidden for this option
   */
  void StretchTool::changeStretch() {
    MdiCubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    // Set the min/max based on selected type
    stretchMinMaxType(cvp);
    
    // Make sure the user didn't enter bad min/max and if so fix it
    double min = m_stretchMinEdit->text().toDouble();
    double max = m_stretchMaxEdit->text().toDouble();

    if(min >= max || m_stretchMinEdit->text() == "" ||
        m_stretchMaxEdit->text() == "") {
      updateTool();
      return;
    }

    //The viewport is in gray mode
    if(cvp->isGray()) {
      Stretch stretch = cvp->grayStretch();
      stretch.ClearPairs();
      stretch.AddPair(min, 0.0);
      stretch.AddPair(max, 255.0);

      // send the stretch to any ChipViewports that want to listen
      *m_chipViewportStretch = stretch;
      emit stretchChipViewport(m_chipViewportStretch, cvp);

      cvp->stretchGray(stretch);
    }
    //Otherwise the viewport is in color mode
    else {
      Stretch redStretch = cvp->redStretch();
      Stretch greenStretch = cvp->greenStretch();
      Stretch blueStretch = cvp->blueStretch();

      if(m_stretchBand == Red) {
        redStretch.ClearPairs();
        redStretch.AddPair(min, 0.0);
        redStretch.AddPair(max, 255.0);
      }
      if(m_stretchBand == Green) {
        greenStretch.ClearPairs();
        greenStretch.AddPair(min, 0.0);
        greenStretch.AddPair(max, 255.0);
      }
      if(m_stretchBand == Blue) {
        blueStretch.ClearPairs();
        blueStretch.AddPair(min, 0.0);
        blueStretch.AddPair(max, 255.0);
      }

      cvp->stretchRed(redStretch);
      cvp->stretchGreen(greenStretch);
      cvp->stretchBlue(blueStretch);
    }

    stretchChanged();
  }

  /**
   * Sets stretch for current band in active viewport given
   * the min/max type selection.
  */
  void StretchTool::stretchMinMaxType(CubeViewport *cvp) {
    // Get current band
    int bandId = m_stretchBandComboBox->currentIndex();
    int bandNum = cvp->grayBand();
    if(bandId == (int)Red) {
      bandNum = cvp->redBand();
    }
    else if(bandId == (int)Green) {
      bandNum = cvp->greenBand();
    }
    else if(bandId == (int)Blue) {
      bandNum = cvp->blueBand();
    }

    // Get current band statistics
    Statistics stats = statsFromCube(cvp->cube(), bandNum);
    Histogram hist = histFromCube(cvp->cube(), bandNum, stats.BestMinimum(), stats.BestMaximum());
    
    // Set min/max given ComboBox selection
    int minMaxIndex = p_minMaxTypeSelection->currentIndex();
    double selectedMin = 0;
    double selectedMax = 0;

    if (minMaxIndex == 0) {
      selectedMin = hist.Percent(0.5);
      selectedMax = hist.Percent(99.5);
    } else if (minMaxIndex == 1) {
      // Best
      selectedMin = stats.BestMinimum();
      selectedMax = stats.BestMaximum();
    } else if (minMaxIndex == 2) {
      // Absolute
      selectedMin = stats.Minimum();
      selectedMax = stats.Maximum();
    } else if (minMaxIndex == 3) {
      // Chebyshev
      selectedMin = stats.ChebyshevMinimum();
      selectedMax = stats.ChebyshevMaximum();
    }

    QString qMin = QString::number(selectedMin);
    QString qMax = QString::number(selectedMax);

    // Set the min/max text
    m_stretchMinEdit->setText(qMin);
    m_stretchMaxEdit->setText(qMax);
  }


  /**
   * This methods shows and updates the advanced dialog.
   *
   */
  void StretchTool::showAdvancedDialog() {
    if(m_advancedStretch->isVisible()) return;

    if(cubeViewport()) {
      m_advancedStretch->updateStretch(cubeViewport());
      m_advancedStretch->show();
    }
    updateTool();
  }


  /**
   * Does a global stretch for the active viewport.
   *
   */
  void StretchTool::stretchGlobal() {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    stretchGlobal(cvp);
  }


  /**
   * This resets the stretch across all bands
   */
  void StretchTool::stretchGlobalAllBands() {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    cvp->forgetStretches();
    stretchGlobal(cvp);
  }


  /**
   * Does a global stretch for the specified viewport.
   *
   */
  void StretchTool::stretchGlobal(CubeViewport *cvp) {
    cvp->stretchKnownGlobal();
    stretchChanged();
  }


  /**
   * Does a global stretch for all the viewports.
   *
   */
  void StretchTool::stretchGlobalAllViewports() {
    for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
      CubeViewport *cvp = cubeViewportList()->at(i);

      stretchGlobal(cvp);
    }
  }


  /**
   * Does a regional stretch for the active viewport.
   *
   */
  void StretchTool::stretchRegional() {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    stretchRegional(cvp);
  }

  /**
   * Does a regional stretch for the specified viewport.
   *
   */
  void StretchTool::stretchRegional(CubeViewport *cvp) {
    try {
      QRect rect(0, 0, cvp->viewport()->width(), cvp->viewport()->height());

      stretchRect(cvp, rect);
    }
    catch (IException &e) {
      QString message = "Cannot stretch while the cube is still loading";
      QMessageBox::warning((QWidget *)parent(), "Warning", message);
      return;
    }
  }


  /**
   * This method is called when the RubberBandTool is complete. It
   * will get a rectangle from the RubberBandTool and stretch
   * accordingly.
   *
   */
  void StretchTool::rubberBandComplete() {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;
    if(!rubberBandTool()->isValid()) return;

    QRect rubberBandRect = rubberBandTool()->rectangle();
    //Return if the width or height is zero
    if(rubberBandRect.width() == 0 || rubberBandRect.height() == 0) return;

    stretchRect(cvp, rubberBandRect);
  }


  /**
   * stretch the specified CubeViewport with the given rect
   *
   * @param cvp The CubeViewport to stretch
   * @param rect The rect with which to stretch the CubeViewport
   */
   void StretchTool::stretchRect(CubeViewport *cvp, QRect rect) {
     Stretch newStretch;
     if(cvp->isGray()) {
       newStretch = cvp->grayStretch();
       newStretch.ClearPairs();
       newStretch.CopyPairs(stretchBuffer(cvp->grayBuffer(), rect));
       cvp->stretchGray(newStretch);

       // send the stretch to any ChipViewports that want to listen
       *m_chipViewportStretch = newStretch;
       emit stretchChipViewport(m_chipViewportStretch, cvp);
     }
     else {
       if (m_stretchBand==Red || m_stretchBand==All) {
         newStretch = cvp->redStretch();
         newStretch.ClearPairs();
         newStretch.CopyPairs(stretchBuffer(cvp->redBuffer(), rect));
         cvp->stretchRed(newStretch);
       }
       if (m_stretchBand==Green || m_stretchBand==All){
         newStretch = cvp->greenStretch();
         newStretch.ClearPairs();
         newStretch.CopyPairs(stretchBuffer(cvp->greenBuffer(), rect));
         cvp->stretchGreen(newStretch);
       }
       if (m_stretchBand==Blue || m_stretchBand==All){
         newStretch = cvp->blueStretch();
         newStretch.ClearPairs();
         newStretch.CopyPairs(stretchBuffer(cvp->blueBuffer(), rect));
         cvp->stretchBlue(newStretch);
       }
       if(m_stretchBand != Red && m_stretchBand != Blue &&
          m_stretchBand != Green && m_stretchBand != All) {
           throw IException(IException::Programmer,
                            "Unknown stretch band",
                            _FILEINFO_);
       }
     }

     stretchChanged();
   }


  /**
   * This method will call a global stretch if the right mouse
   * button is released.
   *
   * @param start
   * @param s
   */
  void StretchTool::mouseButtonRelease(QPoint start, Qt::MouseButton s) {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    // Call the parent Tool function to reset the Warning as different activity is
    // taking place
    Tool::mouseButtonRelease(start, s);

    if(s == Qt::RightButton) {
      stretchGlobal(cvp);

      // notify any ChipViewports listening that the CubeViewport was stretched
      // back to global
      emit stretchChipViewport(NULL, cvp);

      // Resets the RubberBandTool on screen.
      enableRubberBandTool();
    }
  }

  /**
   * This method enables the RubberBandTool.
   *
   */
  void StretchTool::enableRubberBandTool() {
    rubberBandTool()->enable(RubberBandTool::RectangleMode);
    rubberBandTool()->setDrawActiveViewportOnly(true);
  }


  /**
   * Sets the stretch for all the bands in the active viewport to
   * the current stretch
   *
   */
  void StretchTool::setStretchAcrossBands() {
    CubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    double min = m_stretchMinEdit->text().toDouble();
    double max = m_stretchMaxEdit->text().toDouble();

    Stretch stretch;
    if(cvp->isGray()) {
      stretch = cvp->grayStretch();
      stretch.ClearPairs();
      stretch.AddPair(min, 0.0);
      stretch.AddPair(max, 255.0);
    }
    else if(m_stretchBand == Red) {
      stretch = cvp->redStretch();
      stretch.ClearPairs();
      stretch.AddPair(min, 0.0);
      stretch.AddPair(max, 255.0);
      cvp->stretchGreen(stretch);
      cvp->stretchBlue(stretch);
    }
    else if(m_stretchBand == Green) {
      stretch = cvp->greenStretch();
      stretch.ClearPairs();
      stretch.AddPair(min, 0.0);
      stretch.AddPair(max, 255.0);
      cvp->stretchRed(stretch);
      cvp->stretchBlue(stretch);
    }
    else if(m_stretchBand == Blue) {
      stretch = cvp->blueStretch();
      stretch.ClearPairs();
      stretch.AddPair(min, 0.0);
      stretch.AddPair(max, 255.0);
      cvp->stretchRed(stretch);
      cvp->stretchGreen(stretch);
    }
    else {
      return;
    }

    cvp->setAllBandStretches(stretch);
  }


  /**
   * Sets the stretch for all the viewports to the current
   * stretch in the active viewport.
   *
   */
  void StretchTool::setStretchAllViewports() {
    CubeViewport *thisViewport = cubeViewport();

    if(thisViewport == NULL) return;

    for(int i = 0; i < (int)cubeViewportList()->size(); i++) {
      CubeViewport *cvp = cubeViewportList()->at(i);

      if(thisViewport->isGray() && cvp->isGray()) {
        Stretch newStretch(cvp->grayStretch());
        newStretch.CopyPairs(thisViewport->grayStretch());
        cvp->stretchGray(newStretch);
      }
      else if(!thisViewport->isGray() && !cvp->isGray()) {
        Stretch newStretchRed(cvp->redStretch());
        newStretchRed.CopyPairs(thisViewport->redStretch());
        cvp->stretchRed(newStretchRed);

        Stretch newStretchGreen(cvp->greenStretch());
        newStretchGreen.CopyPairs(thisViewport->greenStretch());
        cvp->stretchGreen(newStretchGreen);

        Stretch newStretchBlue(cvp->blueStretch());
        newStretchBlue.CopyPairs(thisViewport->blueStretch());
        cvp->stretchBlue(newStretchBlue);
      }
      else if(!thisViewport->isGray() && cvp->isGray()) {
        // don't copy rgb to gray
      }
      else if(thisViewport->isGray() && !cvp->isGray()) {
        // don't copy gray stretches to rgb
      }
    }

    stretchChanged();
  }


  /**
   * This method computes the stretch over a region using the viewport buffer.
   *
   * @param buffer
   * @param rect
   */
  Stretch StretchTool::stretchBuffer(ViewportBuffer *buffer, QRect rect) {
    //Get the statistics and histogram from the region
    Statistics stats = statsFromBuffer(buffer, rect);
    Stretch stretch;
    if(stats.ValidPixels() > 1 &&
        fabs(stats.Minimum() - stats.Maximum()) > DBL_EPSILON) {
      Histogram hist = histFromBuffer(buffer, rect,
                                            stats.BestMinimum(), stats.BestMaximum());

      if(fabs(hist.Percent(0.5) - hist.Percent(99.5)) > DBL_EPSILON) {
        stretch.AddPair(hist.Percent(0.5), 0.0);
        stretch.AddPair(hist.Percent(99.5), 255.0);
      }
    }

    if(stretch.Pairs() == 0) {
      stretch.AddPair(-DBL_MAX, 0.0);
      stretch.AddPair(DBL_MAX, 255.0);
    }

    return stretch;
  }


  /**
   * This method computes the stretch over the entire cube.
   *
   * @param cvp
   * @param band Band to stretch
   */
  Stretch StretchTool::stretchBand(CubeViewport *cvp, StretchBand band) {

    int bandNum = cvp->grayBand();
    Stretch stretch = cvp->grayStretch();
    if(band == Red) {
      bandNum = cvp->redBand();
      stretch = cvp->redStretch();
    }
    else if(band == Green) {
      bandNum = cvp->greenBand();
      stretch = cvp->greenStretch();
    }
    else if(band == Blue) {
      bandNum = cvp->blueBand();
      stretch = cvp->blueStretch();
    }

    Statistics stats = statsFromCube(cvp->cube(), bandNum);
    Histogram  hist = histFromCube(cvp->cube(), bandNum,
                                         stats.BestMinimum(), stats.BestMaximum());

    stretch.ClearPairs();
    if(fabs(hist.Percent(0.5) - hist.Percent(99.5)) > DBL_EPSILON) {
      stretch.AddPair(hist.Percent(0.5), 0.0);
      stretch.AddPair(hist.Percent(99.5), 255.0);
    }
    else {
      stretch.AddPair(-DBL_MAX, 0.0);
      stretch.AddPair(DBL_MAX, 255.0);
    }

    return stretch;
  }

  /**
   * This method will calculate and return the statistics for a given cube and
   * band.
   *
   * @param cube
   * @param band
   *
   * @return Statistics
   */
  Statistics StretchTool::statsFromCube(Cube *cube, int band) {
    Statistics stats;
    Brick brick(cube->sampleCount(), 1, 1, cube->pixelType());

    for(int line = 0; line < cube->lineCount(); line++) {
      brick.SetBasePosition(0, line, band);
      cube->read(brick);
      stats.AddData(brick.DoubleBuffer(), cube->sampleCount());
    }

    return stats;
  }


  /**
   * This method will calculate and return the statistics for a given region and
   * viewport buffer.
   *
   * @param buffer
   * @param rect
   *
   * @return Statistics
   */
  Statistics StretchTool::statsFromBuffer(ViewportBuffer *buffer,
      QRect rect) {
    if(buffer->working()) {
      throw IException(IException::User,
                       "Cannot stretch while the cube is still loading",
                       _FILEINFO_);
    }

    QRect dataArea = QRect(buffer->bufferXYRect().intersected(rect));
    Statistics stats;

    for(int y = dataArea.top();
        !dataArea.isNull() && y <= dataArea.bottom();
        y++) {
      const std::vector<double> &line = buffer->getLine(y - buffer->bufferXYRect().top());

      for(int x = dataArea.left(); x < dataArea.right(); x++) {
        stats.AddData(line[x - buffer->bufferXYRect().left()]);
      }
    }

    return stats;
  }

  /**
   * This method will calculate and return the histogram for a given cube and
   * band.
   *
   * @param cube
   * @param band
   * @param min
   * @param max
   *
   * @return Histogram
   */
  Histogram StretchTool::histFromCube(Cube *cube, int band,
      double min, double max) {
    Histogram hist(min, max);
    Brick brick(cube->sampleCount(), 1, 1, cube->pixelType());

    for(int line = 0; line < cube->lineCount(); line++) {
      brick.SetBasePosition(0, line, band);
      cube->read(brick);
      hist.AddData(brick.DoubleBuffer(), cube->sampleCount());
    }
 
    return hist;
  }


  /**
   * Given a viewport buffer, this calculates a histogram
   *
   * @param buffer
   *
   * @return Histogram
   */
  Histogram StretchTool::histFromBuffer(ViewportBuffer *buffer) {
    Statistics stats = statsFromBuffer(buffer, buffer->bufferXYRect());
    return histFromBuffer(buffer, buffer->bufferXYRect(),
        stats.BestMinimum(), stats.BestMaximum());

  }


  /**
   * This method will calculate and return the histogram for a given region and
   * viewport buffer.
   *
   * @param buffer
   * @param rect
   * @param min
   * @param max
   *
   * @return Histogram
   */
  Histogram StretchTool::histFromBuffer(ViewportBuffer *buffer,
      QRect rect, double min, double max) {
    QRect dataArea = QRect(buffer->bufferXYRect().intersected(rect));

    try {
      Histogram hist(min, max);

      for(int y = dataArea.top(); !dataArea.isNull() && y <= dataArea.bottom(); y++) {
        const std::vector<double> &line = buffer->getLine(y - buffer->bufferXYRect().top());
        hist.AddData(&line.front() + (dataArea.left() - buffer->bufferXYRect().left()), dataArea.width());
      }

      return hist;
    }
    catch(IException &e) {
      // get the min and max DN values of the data area
      IString sMin(min);
      IString sMax(max);
      std::string msg = "Insufficient data Min [" + sMin + "], Max [" + sMax + "]";
      msg += " in the stretch area.";

      // Emit signal to the parent tool to display Warning object with the warning message
      //emit warningSignal(msg, e.Errors());

      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }
  }



  /**
   * The selected band for stretching changed.
   */
  void StretchTool::stretchBandChanged(int) {

    m_stretchBand = (StretchBand) m_stretchBandComboBox->itemData(
                      m_stretchBandComboBox->currentIndex()
                    ).toInt();

    if(m_stretchBand == All) {
      m_stretchMinEdit->hide();
      m_stretchMaxEdit->hide();
    }
    else {
      m_stretchMinEdit->show();
      m_stretchMaxEdit->show();
    }
    stretchChanged();
  }


  /**
   * This method is called when the flash button is pressed on the advanced dialog
   * and sets the viewport's stretch to the global stretch.
   *
   *
  void StretchTool::flash()
  {
    CubeViewport *cvp = cubeViewport(); if(cvp != NULL) {
      if(cvp->isGray()) {
        m_viewportMap[cvp][cvp->grayBand()-1]->currentStretch = m_viewportMap[cvp][cvp->grayBand()-1]->stretch->stretch();
      }
      else {
        m_viewportMap[cvp][cvp->redBand()-1]->currentStretch = m_viewportMap[cvp][cvp->redBand()-1]->stretch->stretch();
        m_viewportMap[cvp][cvp->greenBand()-1]->currentStretch = m_viewportMap[cvp][cvp->greenBand()-1]->stretch->stretch();
        m_viewportMap[cvp][cvp->blueBand()-1]->currentStretch = m_viewportMap[cvp][cvp->blueBand()-1]->stretch->stretch();
      }

      //Update the stretch
      stretchChanged();
    }
  }*/


  /**
   * This method will save the current stretch's stretch pairs to the specified
   * file name.
   *
   *
  void StretchTool::savePairsAs()
  {
    QString fn = QFileDialog::getSaveFileName((QWidget *)parent(),
                 "Choose filename to save under",
                 ".",
                 "Text Files (*.txt)");
    QString filename;

    //Make sure the filename is valid
    if(!fn.isEmpty())
    {
      if(!fn.endsWith(".txt"))
      {
        filename = fn + ".txt";
      }
      else
      {
        filename = fn;
      }
    }
    //The user cancelled, or the filename is empty
    else
    {
      return;
    }

    if(m_advancedStretch->isRgbMode())
    {
      FileName redFileName(filename.toStdString());
      FileName grnFileName(filename.toStdString());
      FileName bluFileName(filename.toStdString());

      redFileName.addExtension("red");
      grnFileName.addExtension("grn");
      bluFileName.addExtension("blu");

      QFile redFile(QString::fromStdString(redFileName.expanded()));
      QFile grnFile(QString::fromStdString(grnFileName.expanded()));
      QFile bluFile(QString::fromStdString(bluFileName.expanded()));
      bool success = redFile.open(QIODevice::WriteOnly) &&
                     grnFile.open(QIODevice::WriteOnly) &&
                     bluFile.open(QIODevice::WriteOnly);
      if(!success)
      {
        QMessageBox::critical((QWidget *)parent(),
                              "Error", "Cannot open file, please check permissions");
        return;
      }

      QTextStream redStream(&redFile);
      QTextStream grnStream(&grnFile);
      QTextStream bluStream(&bluFile);

      Stretch red = m_advancedStretch->getRedStretch();
      Stretch grn = m_advancedStretch->getGrnStretch();
      Stretch blu = m_advancedStretch->getBluStretch();

      redStream << red.Text().c_str() << endl;
      grnStream << grn.Text().c_str() << endl;
      bluStream << blu.Text().c_str() << endl;

      redFile.close();
      grnFile.close();
      bluFile.close();
    }
    else
    {
      QFile file;
      bool success = file.open(QIODevice::WriteOnly);
      if(!success)
      {
        QMessageBox::critical((QWidget *)parent(),
                              "Error", "Cannot open file, please check permissions");
        return;
      }

      QTextStream stream(&file);

      Stretch stretch = m_advancedStretch->getGrayStretch();

      //Add the pairs to the file
      stream << stretch.Text().c_str() << endl;

      file.close();
    }
  }*/
}
