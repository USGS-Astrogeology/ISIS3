/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Gui.h"

#include <clocale>

#include <sstream>
#include <string>

#include <QApplication>
#include <QCoreApplication>
#include <QDesktopWidget>
#include <QFont>
#include <QFrame>
#include <QIcon>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSplitter>
#include <QScrollArea>
#include <QStatusBar>
#include <QToolBar>
#include <QWhatsThis>
#include <QWidget>

#include "Application.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "ProgramLauncher.h"
#include "PvlObject.h"
#include "PvlGroup.h"
#include "Pvl.h"
#include "SessionLog.h"
#include "UserInterface.h"

#ifdef Q_OS_LINUX
#include <X11/Xlib.h>
#endif

namespace Isis {

  //! Singleton
  Gui *Gui::p_gui = NULL;

  /**
   * check to see if X is available
   */
  void Gui::checkX11() {
    // Many users who run xorg compatible servers on windows like to forget to
    // start their Xhack software before launching X clients.
    // The standard "cannot connect to X server" message that Qt gives is not
    // enough to explain what the problem is, because we keep getting bug
    // reports for this.  Hopefully detecting this ourselves and printing the
    // following message will help.  If not then yes, this is the message that
    // needs changing...

    #ifdef Q_OS_LINUX
    Display *xDisplay = XOpenDisplay(NULL);
    if (!xDisplay) {
      std::cerr << "cannot connect to X server...\n\n"
          "Do you have an X server running?\n\n"
          "If yes then...\n\n"
          "  If you are trying to run this program remotely using ssh, then "
          "did you enable X11 forwarding?\n\n"
          "If the possible causes cited above have been ruled out and this "
          "problem persists, then check your X settings or contact your "
          "system administrator.\n\n";

      abort();
    }
    else {
      XCloseDisplay(xDisplay);
    }
    #endif
  }

  Gui *Gui::Create(Isis::UserInterface &ui, int &argc, char *argv[]) {
    // Don't recreate
    if(p_gui != NULL) return p_gui;

    // Get preferences
    PvlGroup &uiPref = Preference::Preferences().findGroup("UserInterface");
    // Create the application
    new QApplication(argc, argv);
    // When QApplication is initialized, it will reset the locale to the shells locale. As a result
    // the locale needs to be reset after QApplications initialization.
    setlocale(LC_ALL, "en_US");

    QApplication::setQuitOnLastWindowClosed(true);
    QApplication::setApplicationName(QString::fromStdString(FileName(argv[0]).baseName()));


    // Qt is smart enough to use the style of the system running the program.
    // However, Isis supports overriding this with a setting in IsisPreferences.
    // Here we check to see if this has been done and force the style if needed.
    if(uiPref.hasKeyword("GuiStyle")) {
      QString style = QString::fromStdString(uiPref["GuiStyle"]);
      QApplication::setStyle(style);
    }


    if (uiPref.hasKeyword("GuiFontName")) {
      QString fontString = QString::fromStdString(uiPref["GuiFontName"]);
      QFont font = QFont(fontString);

      if (uiPref.hasKeyword("GuiFontSize")) {
        int pointSize = uiPref["GuiFontSize"];
        font.setPointSize(pointSize);
      }

      QApplication::setFont(font);
    }


    // Create the main window
    p_gui = new Gui(ui);
    p_gui->show();

    return p_gui;
  }

  //! Constructor
  Gui::Gui(Isis::UserInterface &ui) : QMainWindow(0, Qt::Window) {
    // Create the toolbar and menu and populate them with actions
    CreateAreas();

    // Set title
    QWidget::setWindowTitle(QApplication::applicationName());

    // Add parameters to the main area
    for(int group = 0; group < ui.NumGroups(); group++) {
      for(int param = 0; param < ui.NumParams(group); param++) {
        p_parameters.push_back(AddParameter(ui, group, param));
      }
    }

    // Load the values from the UI into the GUI
    for(int p = 0; p < (int)p_parameters.size(); p++) {
      GuiParameter *param = p_parameters[p];
      param->Update();
      connect(param, SIGNAL(ValueChanged()), this, SLOT(UpdateCommandLine()));
    }


    // Make the horizontal direction in the scrolling widget non-stretchable
    p_scrollLayout->addStretch(1);

    // Setup status bar
    p_progressBar = new QProgressBar();
    p_progressBar->setMinimum(0);
    p_progressBar->setMaximum(100);
    p_progressBar->setValue(0);
    p_progressBar->setMinimumWidth(200);

    p_statusText = new QLabel("Ready");

    statusBar()->setSizeGripEnabled(true);
    statusBar()->addWidget(p_progressBar, 0);
    statusBar()->addWidget(p_statusText, 3);

    // Setup the current history pointer
    p_historyEntry = -1;


  }

  //! Destructor
  Gui::~Gui() {
    for(unsigned int i = 0; i < p_parameters.size(); i++) {
      delete p_parameters[i];
    }

    p_parameters.clear();
  }

  // Create the main widget, menus, toolbars, status, actions
  void Gui::CreateAreas() {
    // Create the main area
    QSplitter *split = new QSplitter(Qt::Vertical, this);

    // Add a scrolled area for the parameters to the splitter
    p_scrollArea = new QScrollArea();
    p_scrollWidget = new QWidget();
    p_scrollLayout = new QVBoxLayout;
    p_scrollWidget->setLayout(p_scrollLayout);
    p_scrollArea->setWidget(p_scrollWidget);
    p_scrollArea->setWidgetResizable(true);

    // Set the scroll area size
    int height = QApplication::desktop()->height();

    // Add the log area to the bottom of the spliter
    p_log = new GuiLog();
    p_log->setMinimumHeight(10);
    p_log->resize(p_log->width(), 250);

    split->addWidget(p_scrollArea);
    split->addWidget(p_log);
    split->setChildrenCollapsible(false);
    split->setStretchFactor(0, 3);
    split->setStretchFactor(1, 0);
    setCentralWidget(split);
    resize(720, (int)(height / 2) + 350);

    // Create all the actions for menus, toolbars...
    p_processAction = CreateProcessAction();
    p_stopAction = CreateStopAction();
    p_exitAction = CreateExitAction();

    p_previousHistoryAction = CreatePreviousHistoryAction();
    p_nextHistoryAction = CreateNextHistoryAction();
    p_resetAction = CreateResetAction();

    p_saveLogAction = CreateSaveLogAction();
    p_clearLogAction = CreateClearLogAction();

    QAction *whatsThisAction = CreateWhatsThisAction();

    // Create the File menu
    QMenu *fileMenu = menuBar()->addMenu("&File");
    fileMenu->addAction(p_processAction);
    fileMenu->addAction(p_stopAction);
    fileMenu->addAction(p_exitAction);

    // Create the Options menu
    QMenu *optionsMenu = menuBar()->addMenu("&Options");
    optionsMenu->addAction(p_resetAction);
    optionsMenu->addAction(p_previousHistoryAction);
    optionsMenu->addAction(p_nextHistoryAction);
    optionsMenu->addAction(p_saveLogAction);
    optionsMenu->addAction(p_clearLogAction);

    // Create the Controls Toolbar
    QToolBar *tb = addToolBar("Controls");
    tb->setIconSize(QSize(22, 22));
    tb->addAction(p_processAction);
    tb->addAction(p_stopAction);
    tb->addAction(p_exitAction);
    tb->addSeparator();

    tb->addAction(p_previousHistoryAction);
    tb->addAction(p_nextHistoryAction);
    tb->addAction(p_resetAction);
    tb->addSeparator();

    tb->addAction(p_saveLogAction);
    tb->addAction(p_clearLogAction);
    tb->addSeparator();

    tb->addAction(whatsThisAction);

    QAction *showControls = new QAction(this);
    showControls->setText("Controls");
    showControls->setCheckable(true);
    connect(showControls, SIGNAL(toggled(bool)), tb, SLOT(setVisible(bool)));

    tb->installEventFilter(this);

    // Create the command line toolbar
    tb = new QToolBar("Command Line");
    addToolBar(Qt::BottomToolBarArea, tb);
    tb->setIconSize(QSize(22, 22));
    tb->setAllowedAreas(Qt::BottomToolBarArea);
    p_commandLineEdit = new QLineEdit(tb);
    p_commandLineEdit->setReadOnly(true);
    tb->addWidget(p_commandLineEdit);
    QAction *showCommandLine = new QAction(this);
    showCommandLine->setText("Command Line");
    showCommandLine->setCheckable(true);
    connect(showCommandLine, SIGNAL(toggled(bool)), tb, SLOT(setVisible(bool)));
    //tb->hide();

    // Create the view menu
    QMenu *viewMenu = menuBar()->addMenu("&View");
    viewMenu->addAction(showControls);
    viewMenu->addAction(showCommandLine);
    showControls->setChecked(true);
    showCommandLine->setChecked(true);

    // Create the Help menu
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    helpMenu->addAction(whatsThisAction);

    QAction *aboutProgram = new QAction(this);
    aboutProgram->setMenuRole(QAction::AboutRole);
    aboutProgram->setText("About this program");
    aboutProgram->setShortcut(Qt::CTRL + Qt::Key_H);
    helpMenu->addAction(aboutProgram);
    connect(aboutProgram, SIGNAL(triggered(bool)), this, SLOT(AboutProgram()));

    QAction *aboutIsis = new QAction(this);
    aboutIsis->setMenuRole(QAction::NoRole);
    aboutIsis->setText("About Isis");
    aboutIsis->setShortcut(Qt::CTRL + Qt::Key_I);
    helpMenu->addAction(aboutIsis);
    connect(aboutIsis, SIGNAL(triggered(bool)), this, SLOT(AboutIsis()));
  }


  // Create the "Begin/Start Processing" action
  QAction *Gui::CreateProcessAction() {
    QAction *processAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    processAction->setIcon(QPixmap(baseDir + "/guiRun.png"));
    processAction->setText("&Run");
    processAction->setToolTip("Run");
    QString processActionWhatsThisText = "<p><b>Function: </b> \
                      Runs the application with the current parameters</p> \
                      <p><b>Shortcut: </b> Ctrl+R</p>";
    processAction->setShortcut(Qt::CTRL + Qt::Key_R);
    processAction->setWhatsThis(processActionWhatsThisText);

    connect(processAction, SIGNAL(triggered(bool)), this, SLOT(StartProcess()));

    return processAction;
  }

  /**
   *  The user pressed the go button
   *
   *  @internal
   *  @history  2007-05-16 Tracie Sucharski - Change cursor to wait while
   *                                   processing.
   *  @history 2016-02-08 Ian Humphrey - Changed exit(0) to QApplication's quit(),
   *                          since this code is within the QApplication's exec event loop.
   *
   */
  void Gui::StartProcess() {
    p_processAction->setEnabled(false);
    ProgressText("Working");
    Progress(0);
    p_stop = false;

    Isis::UserInterface &ui = Isis::iApp->GetUserInterface();

    // Pull the values from the parameters and put them into the Aml
    for(int p = 0; p < (int)p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      ui.Clear(param.Name());
      if(param.IsEnabled() && param.IsModified()) {
        QString value = param.Value().simplified().trimmed();
        if(value.length() > 0) {
          ui.PutAsString(param.Name(), value);
        }
      }
    }

    // Make sure the parameters were valid
    // Call the application's main
    ProcessEvents();
    try {
      ui.VerifyAll();
      ui.SaveHistory();
      Isis::SessionLog::TheLog(true);
      QApplication::setOverrideCursor(Qt::WaitCursor);
      (*p_funct)();  // Call IsisMain
      QApplication::restoreOverrideCursor();
      Isis::iApp->FunctionCleanup();

      // Display the parameters incase the app changed one or more
      for(int p = 0; p < (int)p_parameters.size(); p++) {
        GuiParameter &param = *(p_parameters[p]);
        param.Update();
      }

      Progress(100);
      ProgressText("Done");
    }
    catch(IException &e) {
      QApplication::restoreOverrideCursor();
      if(e.toString() == "") {
        ProgressText("Stopped");
      }
      else {
        Isis::iApp->FunctionError(e);
        ProgressText("Error");
        // When the warning is rejected (i.e. Abort), clean up from within qApp's exec event loop
        if(ShowWarning()) {
          qApp->quit();
        }
      }
    }

    p_processAction->setEnabled(true);
  }

  // Create the "Exit" action
  QAction *Gui::CreateExitAction() {
    QAction *exitAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    exitAction->setIcon(QPixmap(baseDir + "/guiExit.png"));
    exitAction->setText("&Exit");
    exitAction->setToolTip("Exit");
    QString exitWhatsThisText = "<p><b>Function: </b> \
               Closes the program window </p> <p><b>Shortcut: </b> Ctrl+Q</p>";
    exitAction->setWhatsThis(exitWhatsThisText);
    exitAction->setShortcut(Qt::CTRL + Qt::Key_Q);
    connect(exitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    return exitAction;
  }

  // Create the "Reset" action
  QAction *Gui::CreateResetAction() {
    QAction *resetAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    resetAction->setIcon(QPixmap(baseDir + "/guiReset.png"));
    resetAction->setText("&Reset");
    resetAction->setToolTip("Reset parameters");
    QString resetWhatsThisText = "<p><b>Function: </b> \
                Resets the application parameters to their default values</p> \
                <p><b>Shortcut: </b> F3</p>";
    resetAction->setWhatsThis(resetWhatsThisText);
    resetAction->setShortcut(Qt::Key_F3);
    connect(resetAction, SIGNAL(triggered()), this, SLOT(ResetParameters()));

    return resetAction;
  }

  // Create the "Stop" action
  QAction *Gui::CreateStopAction() {
    QAction *stopAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    stopAction->setIcon(QPixmap(baseDir + "/guiStop.png"));
    stopAction->setText("&Stop");
    stopAction->setToolTip("Stop");
    QString stopWhatsThisText = "<p><b>Function: </b> \
                Stops the application from running</p> \
                <p><b>Shortcut: </b> Ctrl+E</p>";
    stopAction->setShortcut(Qt::CTRL + Qt::Key_E);
    stopAction->setWhatsThis(stopWhatsThisText);
    connect(stopAction, SIGNAL(triggered()), this, SLOT(StopProcessing()));

    return stopAction;
  }

  // Create the "SaveLog" action
  QAction *Gui::CreateSaveLogAction() {
    QAction *saveLogAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    saveLogAction->setIcon(QPixmap(baseDir + "/guiSaveLog.png"));
    saveLogAction->setText("&Save Log...");
    saveLogAction->setToolTip("Save log");
    QString saveWhatsThisText = "<p><b>Function: </b> Saves the information \
           currently in the log area to a file <p><b>Shortcut: </b> Ctrl+S</p>";
    saveLogAction->setWhatsThis(saveWhatsThisText);
    saveLogAction->setShortcut(Qt::CTRL + Qt::Key_S);
    connect(saveLogAction, SIGNAL(triggered(bool)), p_log, SLOT(Save()));

    return saveLogAction;
  }

  // Create the "ClearLog" action
  QAction *Gui::CreateClearLogAction() {
    QAction *clearlogAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    clearlogAction->setIcon(QPixmap(baseDir + "/guiClearLog.png"));
    clearlogAction->setText("&Clear Log");
    clearlogAction->setToolTip("Clear log");
    QString clearWhatsThisText = "<p><b>Function: </b>Clears all information \
        from the log area at the bottom of the application screen</p> \
        <p><b>Shortcut: </b> Ctrl+L</p>";
    clearlogAction->setWhatsThis(clearWhatsThisText);
    clearlogAction->setShortcut(Qt::CTRL + Qt::Key_L);
    connect(clearlogAction, SIGNAL(triggered(bool)), p_log, SLOT(Clear()));

    return clearlogAction;
  }

  // Create the "Previous History" action
  QAction *Gui::CreatePreviousHistoryAction() {
    QAction *previousHistoryAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    previousHistoryAction->setIcon(QPixmap(baseDir + "/guiPrevHistory.png"));
    previousHistoryAction->setText("&Previous");
    previousHistoryAction->setToolTip("Previous parameters");
    QString previousWhatsThisText = "<p><b>Function: </b>Fills in parameter \
            values using the previous history entry</p> \
            <p><b>Shortcut: </b> F5</p>";
    previousHistoryAction->setWhatsThis(previousWhatsThisText);
    previousHistoryAction->setShortcut(Qt::Key_F5);
    connect(previousHistoryAction, SIGNAL(triggered()), this, SLOT(PreviousHistory()));

    return previousHistoryAction;
  }

  // Create the "Next History" action
  QAction *Gui::CreateNextHistoryAction() {
    QAction *nextHistoryAction = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    nextHistoryAction->setIcon(QPixmap(baseDir + "/guiNextHistory.png"));
    nextHistoryAction->setText("&Next");
    nextHistoryAction->setToolTip("Next parameters");
    QString nextWhatsThisText = "<p><b>Function: </b>Fills in parameter \
            values using the next history entry</p> \
            <p><b>Shortcut: </b>F6</p>";
    nextHistoryAction->setWhatsThis(nextWhatsThisText);
    nextHistoryAction->setShortcut(Qt::Key_F6);
    connect(nextHistoryAction, SIGNAL(triggered()), this, SLOT(NextHistory()));

    return nextHistoryAction;
  }

  // Create the Whats Action action
  QAction *Gui::CreateWhatsThisAction() {
    QAction *action = new QAction(this);
    QString baseDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    action->setIcon(QPixmap(baseDir + "/contexthelp.png"));
    action->setText("&What's This");
    action->setToolTip("What's This");
    QString whatsThisText = "<p><b>Function: </b> Use this to get longer \
         descriptions of button functions and parameter information</p> \
         <p><b>Shortcut: </b> Shift+F1</p>";
    action->setWhatsThis(whatsThisText);
    action->setShortcut(Qt::SHIFT + Qt::Key_F1);
    connect(action, SIGNAL(triggered(bool)), this, SLOT(WhatsThis()));

    return action;
  }

  // Add a new parameter to this main window
  GuiParameter *Gui::AddParameter(Isis::UserInterface &ui, int group, int param) {
    // Create the group box if this is the first parameter in the group
    QGridLayout *gridLayout = NULL;
    if(!p_grids.contains(ui.GroupName(group))) {
      // Create a new groupbox and add it to the scroll layout
      QGroupBox *groupBox = new QGroupBox(ui.GroupName(group));
      p_scrollLayout->addWidget(groupBox);
      groupBox->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
      groupBox->setAlignment(Qt::AlignHCenter);

      // Create a gridlayout for the new groupbox and save it
      gridLayout = new QGridLayout;
      gridLayout->setColumnStretch(0, 0);
      gridLayout->setColumnStretch(1, 0);
      gridLayout->setColumnMinimumWidth(1, 10);
      gridLayout->setColumnStretch(2, 10);
      groupBox->setLayout(gridLayout);
      p_grids[ui.GroupName(group)] = gridLayout;
    }
    // Find the group box for this parameter
    else {
      gridLayout = p_grids[ui.GroupName(group)];
    }

    GuiParameter *p = GuiParameterFactory::Create(gridLayout, ui, group, param);

    if (p->Type() == GuiParameter::ListWidget || p->Type() == GuiParameter::ComboWidget ||
        p->Type() == GuiParameter::BooleanWidget) {
      connect(p, SIGNAL(ValueChanged()), this, SLOT(UpdateExclusions()));
    }

    connect(p, SIGNAL(HelperTrigger(const QString &)),
            this, SLOT(InvokeHelper(const QString &)));
    return p;
  }

  //! Change progress text
  void Gui::ProgressText(const QString &text) {
    p_statusText->setText(text);
    qApp->processEvents();  // Needed when programs run programs
  }

  //! Update the progress bar
  void Gui::Progress(int percent) {
    p_progressBar->setValue(percent);
    qApp->processEvents();  // Needed when programs run programs
  }

  /**
   * Start the Gui and enter the main loop
   * This routine only returns when the program is ready to exit
   */
  int Gui::Exec(void (*funct)()) {
    p_funct = funct;
    return qApp->exec();
  }

  //! Add more information to the error message
  void Gui::LoadMessage(const QString &message) {
    // Convert newlines to breaks
    QString m = QString(message).replace("\n", "<br>");

    // If there is a set of "[]" change everything between them to red text
    if(message.contains("[") &&
       message.contains("]") &&
       (message.indexOf("[") < message.indexOf("]"))) {

      int indx = 0;
      while(m.indexOf("[", indx) != -1) {
        m.insert(m.indexOf("[", indx) + 1, "<font color=#ff0000>");
        m.insert(m.indexOf("]", indx), "</font>");
        indx = m.indexOf("]", indx) + 1;
      }
    }
    p_errorString += m;
  }

  //! Show an error message and return if the user wants to continue/abort
  int Gui::ShowWarning() {
    Isis::UserInterface &ui = Isis::iApp->GetUserInterface();
    int status = QMessageBox::warning(this,
                                      ui.ProgramName(),
                                      p_errorString,
                                      "Ok", "Abort", "", 0, 1);
    p_errorString.clear();
    return status;
  }

  //! Write text to the gui log
  void Gui::Log(const QString &text) {
    p_log->Write(text);
  }

  //! Reset the Progress bar when the user moves the mouse onto the toolbar
  bool Gui::eventFilter(QObject *o, QEvent *e) {
    if(e->type() == QEvent::Enter) {
      if(p_processAction->isEnabled()) {
        ProgressText("Ready");
        Progress(0);
      }
    }
    return false;
  }

  //! The user pressed the stop button ... see what they want to do
  void Gui::StopProcessing() {
    if(p_processAction->isEnabled()) return;

    Isis::UserInterface &ui = Application::GetUserInterface();
    switch(QMessageBox::information(this,
                                    ui.ProgramName(),
                                    QString("Program suspended, choose to ") +
                                    QString("continue processing, stop ") +
                                    QString("processing or exit the program"),
                                    "Continue",
                                    "Stop",
                                    "Exit", 0, 2)) {
      case 0: // Pressed continue
        break;

      case 1: // Pressed stop
        p_stop = true;
        break;

      case 2: // Pressed exit
        p_stop = true;
        qApp->quit();
    }
  }

  /** Let the event loop have some time to see if we need to cancel.
   * This is normally called by the Isis::Progress class.
   */
  bool Gui::ProcessEvents() {
    qApp->processEvents();
    return p_stop;
  }

  //! Reset the parameters fields to the defaults
  void Gui::ResetParameters() {
    // Clear the AML to default values
    Isis::UserInterface &ui = Application::GetUserInterface();
    for(int p = 0; p < (int)p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      ui.Clear(param.Name());
    }

    // Display the updated parameters
    for(int p = 0; p < (int)p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      param.Update();
    }
  }

  //! Goto the next history entry
  void Gui::NextHistory() {
    p_historyEntry--;
    UpdateHistory();
  }

  //! Goto the previous history entry
  void Gui::PreviousHistory() {
    p_historyEntry++;
    UpdateHistory();
  }


  /**
   * Changed the parameters based on the history pointer
   *
   * @internal
   * @history 2016-02-08 Ian Humphrey - Changed exit(0) to QApplication's quit(),
   *                          since this code is within the QApplication's exec event loop.
   */
  void Gui::UpdateHistory() {
    if(p_historyEntry < -1) {
      p_historyEntry = -1;
      QApplication::beep();
      return;
    }

    if(p_historyEntry == -1) {
      ResetParameters();
      return;
    }

    // Find out if this application has a history file
    Isis::UserInterface &ui = Application::GetUserInterface();
    Preference &p = Preference::Preferences();

    PvlGroup &grp = p.findGroup("UserInterface", Isis::Pvl::Traverse);
    Isis::FileName progHist(grp["HistoryPath"][0] + "/" + ui.ProgramName() + ".par");

    if(!progHist.fileExists()) {
      p_historyEntry = -1;
      QApplication::beep();
      return;
    }

    Isis::Pvl hist;

    try {
      hist.read(progHist.expanded().toStdString());
    }
    catch(...) {
      p_historyEntry =  -1;
      QString msg = "A corrupt parameter history file [" + QString::fromStdString(progHist.expanded()) +
                        "] has been detected. Please fix or remove this file";
      LoadMessage(msg);
      // When the warning is rejected (i.e. Abort), clean up from within qApp's exec event loop
      if (ShowWarning()) {
        qApp->quit();
      }
      return;
    }

    int entries = 0;
    for(int i = 0; i < hist.groups(); i++) {
      if(hist.group(i).isNamed("UserParameters")) entries++;
    }

    // If we are past the last entry ring the bell
    if(p_historyEntry == entries) {
      p_historyEntry = entries - 1;
      QApplication::beep();
      return;
    }

    int useEntry = entries - p_historyEntry - 1;

    try {
      //When defaults are used they do not get rewritten because they do not
      //exist in the history file to be written over. Must reset parameters first.
      ResetParameters();
      Isis::PvlGroup &up = hist.group(useEntry);
      for (int k = 0; k < up.keywords(); k++) {
        QString key = QString::fromStdString(up[k].name());
        QString val;
        // If the value has more than one element,
        // construct a string array of those elements
        if (up[k].size() > 1) {
          val = "(";
          for (int i = 0; i < up[k].size(); i++) {
            QString newVal = QString::fromStdString(up[k][i]);
            if (newVal.contains(",")) {
              newVal = '"' + newVal + '"';
            }
            if (i == up[k].size() - 1) {
              val += newVal + ")";
            }
            else {
              val += newVal + ", ";
            }
          }
        }
        // Else, return the value on its own
        else {
          QString newVal = QString::fromStdString(up[k]);
          val = newVal;
        }
        ui.Clear(key);
        ui.PutAsString(key, val);
      }

      for(unsigned int p = 0; p < p_parameters.size(); p++) {
        GuiParameter &param = *(p_parameters[p]);
        param.Update();
      }

    }
    catch(IException &e) {
      p_historyEntry = entries - 1;
      QApplication::beep();
      return;
    }
  }

  /** Grey out parameters that should be excluded for radio buttons and
   * checkboxes
   */
  void Gui::UpdateExclusions() {
    // First enable everything
    for(unsigned int p = 0; p < p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      param.SetEnabled(true);
    }

    // Now disable things
    for(unsigned int p = 0; p < p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      std::vector<QString> excludeList = param.Exclusions();
      for(int i = 0; i < (int)excludeList.size(); i++) {
        for(unsigned int e = 0; e < p_parameters.size(); e++) {
          GuiParameter &exclude = *(p_parameters[e]);
          if(exclude.Name() != excludeList[i]) continue;
          exclude.SetEnabled(false, (param.Type()==GuiParameter::ComboWidget));
        }
      }
    }
  }

  //! Update the command line toolbar
  void Gui::UpdateCommandLine() {
    QString cline = Isis::Application::GetUserInterface().ProgramName();
    for(int p = 0; p < (int)p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      if(param.IsEnabled() && param.IsModified()) {
        cline += " ";
        QString name = param.Name().toLower();
        cline += name;
        cline += "=";
        QString value = param.Value();
        if(param.Type() == GuiParameter::StringWidget) {
          if(value.contains(" ")) {
            cline += "\"" + value + "\"";
          }
          else {
            cline += value;
          }
        }
        else if(param.Type() == GuiParameter::FileNameWidget ||
                param.Type() == GuiParameter::CubeWidget) {
          cline += value;
        }
        else {
          value = value.toLower();
          cline += value;
        }
      }
    }
    p_commandLineEdit->setText(cline);
  }

  //! Update Parameters
  void Gui::UpdateParameters() {
    for(unsigned int p = 0; p < p_parameters.size(); p++) {
      GuiParameter &param = *(p_parameters[p]);
      param.Update();
    }
  }

  // Enter into what's this mode
  void Gui::WhatsThis() {
    QWhatsThis::enterWhatsThisMode();
  }

  // Show help for Isis
  void Gui::AboutIsis() {
    Isis::PvlGroup &uig = Isis::Preference::Preferences().findGroup("UserInterface");
#if defined(__linux__)
    QString command = (QString) uig["GuiHelpBrowser"] +
                          " http://isis.astrogeology.usgs.gov >> /dev/null &";
#elif defined(__APPLE__)
    QString command = "open -a" + QString::fromStdString(uig["GuiHelpBrowser"]) +
                      " http://isis.astrogeology.usgs.gov >> /dev/null &";
#endif                  
    ProgramLauncher::RunSystemCommand(command);
  }

  // Show help for the current app
  void Gui::AboutProgram() {
    Isis::PvlGroup &uig = Isis::Preference::Preferences().findGroup("UserInterface");
#if defined(__linux__)
    QString command = (QString) uig["GuiHelpBrowser"] +
                      " http://isis.astrogeology.usgs.gov/Application/presentation/Tabbed/" +
                      Isis::Application::GetUserInterface().ProgramName() + "/" +
                      Isis::Application::GetUserInterface().ProgramName() + ".html";
#elif defined(__APPLE__)
    QString command = "open -a" + QString::fromStdString(uig["GuiHelpBrowser"]) +
                      " http://isis.astrogeology.usgs.gov/Application/presentation/Tabbed/" +
                      Isis::Application::GetUserInterface().ProgramName() + "/" +
                      Isis::Application::GetUserInterface().ProgramName() + ".html";
#endif
    ProgramLauncher::RunSystemCommand(command);
  }

  //! Activate helper buttons
  void Gui::InvokeHelper(const QString &funct) {
    p_processAction->setEnabled(false);
    try {
      Isis::UserInterface &ui = Application::GetUserInterface();

      // Pull the values from the parameters and put them into the Aml
      for(int p = 0; p < (int)p_parameters.size(); p++) {
        GuiParameter &param = *(p_parameters[p]);
        ui.Clear(param.Name());
        if(param.IsEnabled() && param.IsModified()) {
          QString value = param.Value().simplified().trimmed();
          if(value.length() > 0) {
            ui.PutAsString(param.Name(), value);
          }
        }
      }

      // Get the helper function and run
      void *ptr = Isis::iApp->GetGuiHelper(funct);
      void (*helper)();
      helper = (void ( *)())ptr;
      helper();
    }
    catch(IException &e) {
      Isis::iApp->GuiReportError(e);
    }

    // Update parameters in GUI
    UpdateParameters();
    p_processAction->setEnabled(true);
  }

}
