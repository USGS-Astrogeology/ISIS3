#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include "Application.h"
#include "Camera.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "FileName.h"
#include "MdiCubeViewport.h"
#include "Progress.h"
#include "QnetFileTool.h"
#include "QnetTool.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"
#include "Target.h"
#include "Workspace.h"

using namespace std;

namespace Isis {
  /**
   * Constructor
   *
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Reworded "What's this?" description
   *                          for saveAs action. Changed "Save As" action text to
   *                          match QnetTool's "Save As" action
   */
  QnetFileTool::QnetFileTool(QnetTool *qnetTool, QWidget *parent) : FileTool(parent) {
    m_qnetTool = qnetTool;

    openAction()->setText("Open control network and cube list");
    openAction()->setToolTip("Open control network and cube list");
    QString whatsThis =
      "<b>Function:</b> Open a <i>control network</i> \
       <p><b>Shortcut:</b>  Ctrl+O\n</p>";
    openAction()->setWhatsThis(whatsThis);

    saveAction()->setText("Save Control Network &As...");
    whatsThis =
      "<b>Function:</b> Save the current <i>control network</i> under chosen filename";
    saveAction()->setWhatsThis(whatsThis);
    saveAction()->setEnabled(true);

    m_isDirty = false;

    m_openGround = new QAction(parent);
    m_openGround->setText("Open &Ground Source");
    m_openGround->setStatusTip("Open a ground source for choosing ground points");
    whatsThis =
      "<b>Function:</b> Open and display a ground source for choosing ground points."
      "This can be level1, level2 or dem cube.";
    m_openGround->setWhatsThis(whatsThis);
    m_openGround->setEnabled(false);
    connect (m_openGround,SIGNAL(triggered()),this,SIGNAL(newGroundFile()));

    m_openDem = new QAction(parent);
    m_openDem->setText("Open &Radius Source");
    whatsThis =
      "<b>Function:</b> Open a DEM for determining the radius when "
      "choosing ground points.  This is not the file that will be displayed "
      "to be used for visually picking points.  This is strictly used to "
      "determine the radius value.";
    m_openDem->setWhatsThis(whatsThis);
    m_openDem->setEnabled(false);
    connect (m_openDem,SIGNAL(triggered()),this,SIGNAL(newDemFile()));
  }


  QnetFileTool::~QnetFileTool() {
  }


  void QnetFileTool::addTo(QMenu *menu) {
    menu->addAction(m_openGround);
    menu->addAction(m_openDem);
    menu->addSeparator();
    FileTool::addTo(menu);
  }


  ControlNet *QnetFileTool::controlNet() {
    return m_qnetTool->controlNet();
  }


  SerialNumberList *QnetFileTool::serialNumberList() {
    return m_qnetTool->serialNumberList();
  }


  /**
   *  Open a list of cubes
   *
   * @author  2007-05-01 Elizabeth Ribelin
   *
   * @internal
   *  @history  2007-06-07 Tracie Sucharski - Allow new network to be opened,
   *                          prompt to save old network.
   *  @history  2008-11-26 Tracie Sucharski - Remove all polygon/overlap
   *                          references, this functionality will be qmos.
   *  @history 2008-11-26 Jeannie Walldren - Uncommented "emit
   *                         controlNetworkUpdated()" line and added parameter name
   *                         defined in this method.
   *  @history 2008-12-10 Jeannie Walldren - Fixed documentation
   *  @history 2010-07-01 Jeannie Walldren - Added file extension filters for
   *                         input control network
   *  @history 2010-11-09 Tracie Sucharski - "emit" was missing from the signal
   *                         serialNumberListUpdated.
   *  @history 2011-08-08 Tracie Sucharski - If new network, set the Target
   *  @history 2011-11-01 Tracie Sucharski - save filename for the "Save" slot.
   *
   */
  void QnetFileTool::open() {

    //  If network already opened, prompt for saving
    if (serialNumberList() != NULL && m_isDirty) {
      //  If control net has been changed , prompt for user to save
      int resp = QMessageBox::warning((QWidget *)parent(), "Qnet",
          "The control network files has been modified.\n"
          "Do you want to save your changes?",
          QMessageBox::Yes | QMessageBox::Default,
          QMessageBox::No,
          QMessageBox::Cancel | QMessageBox::Escape);
      if (resp == QMessageBox::Yes) {
        saveAs();
      }
      m_isDirty = false;
    }

    QString filter = "List of cubes (*.lis *.lst *.list);;";
    filter += "Text file (*.txt);;";
    filter += "All (*)";
    QString list = QFileDialog::getOpenFileName((QWidget *)parent(),
        "Select a list of cubes",
        ".",
        filter);
    if (list.isEmpty())
      return;

    // Find directory and save for use in file dialog for net file
    FileName file(list);
    QString dir = file.path();

    QApplication::setOverrideCursor(Qt::WaitCursor);
    // Use the list to get serial numbers and polygons
    try {
      *serialNumberList() = SerialNumberList(list);
      *controlNet() = ControlNet();
    }
    catch (IException &e) {
      std::string message = "Error processing cube list.  \n";
      std::string errors = e.toString();
      message += errors;
      QMessageBox::information((QWidget *)parent(), "Error", message);
      QApplication::restoreOverrideCursor();
      return;
    }

    QApplication::restoreOverrideCursor();
    filter = "Control net (*.net *.cnet *.ctl);;";
    filter += "Pvl file (*.pvl);;";
    filter += "Text file (*.txt);;";
    filter += "All (*)";
    QString cNetFileName = QFileDialog::getOpenFileName((QWidget *)parent(),
        "Select a control network",
        dir,
        filter);
    QApplication::setOverrideCursor(Qt::WaitCursor);
    if (cNetFileName.isEmpty()) {
      controlNet()->SetUserName(Application::UserName());

      //  Determine target from first file in cube list
      QScopedPointer<Cube> cube(new Cube());
      cube->open(serialNumberList()->fileName(0));
      controlNet()->SetTarget(*cube->label());
    }
    else {
      try {
        Progress progress;
        *controlNet() = ControlNet(cNetFileName, &progress);
      }
      catch (IException &e) {
        std::string message = "Invalid control network.  \n";
        std::string errors = e.toString();
        message += errors;
        QMessageBox::information((QWidget *)parent(), "Error", message);
        QApplication::restoreOverrideCursor();
        return;
      }
    }

    //  Initialize cameras for control net
    try {
      Progress progress;
      controlNet()->SetImages(*serialNumberList(), &progress);
    }
    catch (IException &e) {
      std::string message = "Cannot initialize images in control network.  \n";
      std::string errors = e.toString();
      message += errors;
      QMessageBox::information((QWidget *)parent(), "Error", message);
      QApplication::restoreOverrideCursor();
      return;
    }

    m_openGround->setEnabled(true);
    m_openDem->setEnabled(true);

    QApplication::restoreOverrideCursor();

    m_cnetFileName = cNetFileName;
    emit serialNumberListUpdated();
    emit controlNetworkUpdated(cNetFileName);
    emit newControlNetwork(controlNet());
  }

  /**
   *  Exit the program
   *
   *  @internal
   *  @history 2018-04-24 Adam Goins - Added QCloseEvent optional parameter to
   *                          set the CloseEvent triggered by an onwindowclose
   *                          to ignore the event if the 'cancel' option was selected
   *                          after clicking the close button of the viewport window.
   *                          This fixes an issue where clicking the close button and then clicking
   *                          'cancel' from the QMessageBox would close the window but keep the
   *                          application running. Fixes #4146.
   */
  void QnetFileTool::exit(QCloseEvent *event) {
    //  If control net has been changed , prompt for user to save
    if (m_isDirty) {
      int resp = QMessageBox::warning((QWidget *)parent(), "QnetTool",
          "The control network files has been modified.\n"
          "Do you want to save your changes?",
          QMessageBox::Yes | QMessageBox::Default,
          QMessageBox::No,
          QMessageBox::Cancel | QMessageBox::Escape);
      if (resp == QMessageBox::Yes) {
        saveAs();
      }
      if (resp == QMessageBox::Cancel) {
        if (event) {
          event->setAccepted(false);
        }
        return;
      }
    }
    qApp->quit();
  }

    /**
   *  Save control network with given file
   *  @internal
   *  @history 2010-07-01 Jeannie Walldren - Added file extension filters for
   *                         input control network
   *  @history 2011-11-01 Tracie Sucharski - emit signal to update network
   *                         information.
   *
   */
  void QnetFileTool::save() {
    controlNet()->Write(m_cnetFileName);
    m_isDirty = false;
  }



  /**
   *  Save control network with given file
   *  @internal
   *  @history 2010-07-01 Jeannie Walldren - Added file extension filters for
   *                         input control network
   *  @history 2011-11-01 Tracie Sucharski - emit signal to update network
   *                         information and save filename for the "Save" slot.
   *
   */
  void QnetFileTool::saveAs() {
    QString filter = "Control net (*.net *.cnet *.ctl);;";
    filter += "Pvl file (*.pvl);;";
    filter += "Text file (*.txt);;";
    filter += "All (*)";
    QString fn = QFileDialog::getSaveFileName((QWidget *)parent(),
        "Choose filename to save under",
        ".", filter);
    if (!fn.isEmpty()) {
      try {
        controlNet()->Write(fn);
      }
      catch (IException &e) {
        std::string message = "Error saving control network.  \n";
        std::string errors = e.toString();
        message += errors;
        QMessageBox::information((QWidget *)parent(), "Error", message);
        return;
      }
    }
    else {
      QMessageBox::information((QWidget *)parent(),
          "Error", "Saving Aborted");
    }
    m_cnetFileName = fn;
    emit controlNetworkUpdated(fn);
    m_isDirty = false;
  }

  /**
   * Load given cube in Workspace
   *
   * @param serialNumber [in]   (QString)   Serial number of cube to display
   *
   * @author  2007-05-01 Elizabeth Ribelin
   *
   * @internal
   *  @history  2008-10-08 Tracie Sucharski - Do not display cube if it is
   *                          already displayed, set as active window.
   *  @history 2008-12-10 Jeannie Walldren - Fixed documentation
   *  @history 2010-06-03 Jeannie Walldren - Removed "std::" since "using
   *                          namespace std"
   *  @history 2010-07-12 Jeannie Walldren - Updated setActiveSubWindow call due
   *                          to change in Workspace class
   *  @history 2011-04-08  Tracie Sucharski - Remove test for sn existing in
   *                          serial number list since the list is where the
   *                          serial number came from.
   */
  void QnetFileTool::loadImage(const QString &serialNumber) {

    QString tempFileName = serialNumberList()->fileName(serialNumber);
    QString filename = tempFileName;
    QVector< MdiCubeViewport * > * cvpList = m_qnetTool->workspace()->cubeViewportList();
    bool found = false;
    for (int i = 0; i < (int)cvpList->size(); i++) {
      QString sn = SerialNumber::Compose(*((*cvpList)[i]->cube()));
      if (sn == serialNumber) {
        m_qnetTool->workspace()->mdiArea()->setActiveSubWindow(
            (QMdiSubWindow *)(*cvpList)[i]->parentWidget()->parent());
        found = true;
        break;
      }
    }
    //  If viewport doesn't already exist for this serial number, emit
    //  signal so that FileTool will add a viewport.
    if (!found)
      emit fileSelected(filename);
  }

  /**
   * Load images for the given point
   *
   * @param point Control point to load
   *
   * @internal
   *   @history 2010-12-10 Tracie Sucharski - Renamed slot loadPoint to
   *                          loadPointImages.
   */
  void QnetFileTool::loadPointImages(ControlPoint *point) {
    for (int i = 0; i < point->GetNumMeasures(); i++) {
      QString cubeSN = (*point)[i]->GetCubeSerialNumber();
      loadImage(cubeSN);
    }
  }

  /**
   *  Sets save net flag to true.
   */
  void QnetFileTool::setDirty() {
    m_isDirty = true;
  }


}
