/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "FileName.h"
#include "GuiEditFile.h"
#include "UserInterface.h"

#include <QDir>
#include <QFileDialog>
#include <QList>
#include <QPushButton>
#include <QStatusBar>
#include <QTextEdit>
#include <QTextStream>
#include <QMenuBar>
#include <QMessageBox>

using namespace Isis;
using namespace std;

namespace Isis {

  // Singleton
  GuiEditFile* GuiEditFile::m_instance = NULL;

  /**
   * Creates a single instance of the GuiEditFile. If already an instance,
   * display the object
   *
   * @author Sharmila Prasad (5/20/2011)
   *
   * @param pUI    - User Interface of parent app
   * @param psFile - File to edit
   */
  void GuiEditFile::EditFile(UserInterface & pUI, QString psFile) {
    if (m_instance == NULL) {
      m_instance = new GuiEditFile(pUI, psFile);
    }
    else {
      m_instance->showWindow(psFile);
    }
  }

  /**
   * Display the window if there is already an instance of this object
   *
   * @author Sharmila Prasad (5/23/2011)
   *
   * @param psFile - File to edit
   */
  void GuiEditFile::showWindow(QString psFile) {
    m_fileName = QString(psFile);
    m_editWin->setWindowTitle (m_fileName);
    OpenFile(m_fileName);
    m_editWin->show();
  }

  /**
   * Constructor
   *
   * @author Sharmila Prasad (5/20/2011)
   *
   * @param pUI
   * @param psFile
   */
  GuiEditFile::GuiEditFile(UserInterface & pUI, QString psFile) {
    // Init data members
    m_parent   = pUI.TheGui();
    m_fileName = QString(psFile);
    m_editFile=NULL;

    // Main Window
    m_editWin = new QMainWindow(m_parent, Qt::SubWindow);
    m_editWin->setWindowTitle (m_fileName);
    m_editWin->resize (400, 600);

    // Status bar
    QStatusBar *statusBar = new QStatusBar(m_editWin);
    m_editWin->setStatusBar(statusBar);

    // Menu bar
    QMenuBar *menuBar = new QMenuBar(m_editWin);
    QMenu *fileMenu = menuBar->addMenu("&File");

    // Action File-> Open
    m_open = new QAction(menuBar);
    m_open->setShortcut(Qt::CTRL + Qt::Key_O);
    m_open->setText("&Open...");
    m_open->setToolTip("Open File");
    QString whatsThis = "Open a file to edit";
    m_open->setWhatsThis(whatsThis);
    connect(m_open, SIGNAL(triggered()), this, SLOT(open()));
    fileMenu->addAction(m_open);

    // Action File-> Save
    m_save = new QAction(menuBar);
    m_save->setShortcut(Qt::CTRL + Qt::Key_S);
    m_save->setText("&Save...");
    m_save->setToolTip("Save File");
    m_save->setWhatsThis("Save the current file");
    connect(m_save, SIGNAL(triggered()), this, SLOT(saveFile()));
    fileMenu->addAction(m_save);

    // Action File->Save As
    m_saveAs = new QAction(menuBar);
    m_saveAs->setText("Save &As...");
    m_saveAs->setShortcut(Qt::CTRL + Qt::Key_A);
    m_saveAs->setToolTip("Save As File");
    m_saveAs->setWhatsThis("Save the current file into another file");
    connect(m_saveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
    fileMenu->addAction(m_saveAs);

    // Action File->close
    m_close = new QAction(menuBar);
    m_close->setText("&Close...");
    m_close->setShortcut(Qt::CTRL + Qt::Key_C);
    m_close->setToolTip("Close File");
    m_close->setWhatsThis("Close the current file");
    connect(m_close, SIGNAL(triggered()), this, SLOT(closeFile()));
    fileMenu->addAction(m_close);

    // Action Exit
    m_exit = menuBar->addAction("&Exit");
    m_exit->setShortcut(Qt::CTRL + Qt::Key_E);
    m_exit->setText("&Exit...");
    m_exit->setToolTip("Exit");
    m_exit->setWhatsThis("Exit the Editor");
    //connect(m_exit, SIGNAL(triggered()), m_editWin, SLOT(close()));
    connect(m_exit, SIGNAL(triggered()), this, SLOT(closeWin()));

    m_editWin->setMenuBar(menuBar);

    // Text Edit
    m_textChanged = false;
    m_txtEdit = new QTextEdit(m_editWin);
    m_txtEdit->setUndoRedoEnabled(true);
    m_txtEdit->resize(400,500);
    m_editWin->setCentralWidget(m_txtEdit);
    if (m_fileName.length() > 0){
      OpenFile(m_fileName);
    }
    connect(m_txtEdit, SIGNAL(textChanged()), this, SLOT(setTextChanged()));

    m_editWin->show();
  }

  /**
   * Destructor
   *
   * @author Sharmila Prasad (5/20/2011)
   */
  GuiEditFile::~GuiEditFile() {
    delete m_editFile;
    m_editFile = NULL;

    delete m_txtEdit;
    m_txtEdit = NULL;

    delete m_editWin;
    m_editWin = NULL;

    delete m_instance;
    m_instance = NULL;
  }

  /**
   * Action Exit - Close window and clear text editor
   *
   * @author Sharmila Prasad (5/23/2011)
   */
  void GuiEditFile::closeWin() {
    m_editWin->close();
    m_txtEdit->clear();
  }

  /**
   * Flag to indicate text has changed
   *
   * @author Sharmila Prasad (5/20/2011)
   */
  void GuiEditFile::setTextChanged() {
    m_textChanged = true;
  }

  /**
   * Action File->Open
   *
   * @author Sharmila Prasad (5/20/2011)
   */
  void GuiEditFile::open() {
    //Set up the list of filters that are default with this dialog.
    //cerr << "setTextChanged=" << m_textChanged << endl;
    if (m_textChanged) {
      if(QMessageBox::question((QWidget *)parent(), tr("Save File?"),
                               tr("Are you sure you want to save this file?"),
                               tr("&Save"), tr("&Cancel"),
                               QString(), 1, 0)) {
        saveFile();
      }
    }

    QString sFilterList("All files (*)");
    QDir currDir = QDir::current();
    QString sOpen("Open");
    QFileDialog  *fileDialog = new QFileDialog((QWidget *)parent(), sOpen, currDir.dirName(), sFilterList);
    fileDialog->show();
    connect(fileDialog, SIGNAL(fileSelected(QString)), this, SLOT(OpenFile(QString)));
  }

  /**
   * Action File->close, close the opened file
   *
   * @author Sharmila Prasad (5/20/2011)
   */
  void GuiEditFile::closeFile(){
    if (m_textChanged) {
      if(QMessageBox::question((QWidget *)parent(), tr("Save File?"),
                               tr("Changes have been made to the file. Do you want to Save?"),
                               QMessageBox::Save, QMessageBox::No) == QMessageBox::Save) {
        saveFile();
      }
    }
    m_editFile->close();
    m_txtEdit->clear();
    m_textChanged = false;
    m_editWin->setWindowTitle(tr(""));
  }

  /**
   * Display the selected file
   *
   * @author Sharmila Prasad (5/20/2011)
   *
   * @param psOutFile
   */
  void GuiEditFile::OpenFile(QString psOutFile){
    //cerr << "Open File " << psOutFile.toStdString() << "\n";
    if(psOutFile.isEmpty()){
      QMessageBox::information((QWidget *)parent(), "Error", "No output file selected");
      return;
    }

    if (m_editFile != NULL) {
      m_txtEdit->clear();
      delete (m_editFile);
    }
    m_editFile = new QFile(psOutFile);
    //m_editWin->setWindowTitle(FileName(psOutFile.toStdString()).Name().c_str());
    windowTitle(psOutFile);

    if (m_editFile->open(QIODevice::ReadWrite)){
      char buf[1024];
      QString bufStr;
      qint64 lineLength = m_editFile->readLine(buf, sizeof(buf));
      while (lineLength != -1) {
        bufStr = QString(buf);
        //std::cerr << bufStr.toStdString();
        bufStr.remove('\n'); // puts an additional new line
        m_txtEdit->append(bufStr);
        lineLength = m_editFile->readLine(buf, sizeof(buf));
      }
    }
    else {
      m_txtEdit->append("\nThis file cannot be edited. Please check the file's Write permissions");
    }
    m_textChanged = false;
  }

  /**
   * Action File->Save
   *
   * @author Sharmila Prasad (5/20/2011)
   */
  void GuiEditFile::saveFile(){
    if (m_editFile){
      clearFile();
      QTextStream out(m_editFile);
      out << m_txtEdit->document()->toPlainText();
      m_textChanged = false;
    }
  }

  /**
   * Action File->Save As
   *
   * @author Sharmila Prasad (5/23/2011)
   */
  void GuiEditFile::saveAs() {
    QString sFilterList("All files (*)");
    QDir currDir = QDir::current();
    QString sOpen("Open");
    QFileDialog  *saveAsDialog = new QFileDialog((QWidget *)parent(), sOpen, currDir.dirName(), sFilterList);

    QList<QPushButton *> allPButtons = saveAsDialog->findChildren<QPushButton *>();
    // Edit the first (Open) button title.
    allPButtons[0]->setText("&Save");
    // Edit the second (Cancel) button title.
    allPButtons[1]->setText("&Close");

    saveAsDialog->show();
    connect(saveAsDialog, SIGNAL(fileSelected(QString)), this, SLOT(saveAsFile(QString)));
  }

  /**
   * Copy the current file into user selected file
   *
   * @author Sharmila Prasad (5/23/2011)
   *
   * @param psNewFile
   */
  void GuiEditFile::saveAsFile(QString psNewFile) {
    m_editFile->close();
    delete(m_editFile);
    m_editFile = new QFile(psNewFile);
    m_editFile->open(QFile::ReadWrite);
    saveFile();
    windowTitle(psNewFile);
  }

  /**
   * Delete the contents of the current file - especially if changes are made
   * and they are not to be saved.
   *
   * @author Sharmila Prasad (5/23/2011)
   */
  void GuiEditFile::clearFile(){
    if (m_editFile){
      m_editFile->close();
      //m_editFile->open(QIODevice::Truncate | QIODevice::ReadWrite);
      m_editFile->open(QFile::ReadWrite | QFile::Truncate);
    }
  }

  /**
   * Display only the base name of the file
   *
   * @author Sharmila Prasad (5/23/2011)
   *
   * @param psfile - current file
   */
  void GuiEditFile::windowTitle(QString & psfile) {
    m_editWin->setWindowTitle(QString::fromStdString(FileName(psfile.toStdString()).name()));
  }

}
