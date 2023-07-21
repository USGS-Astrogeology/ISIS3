/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

extern int errno;

#include <fstream>

#include <iostream>
#include <sstream>
#include <locale.h>

#include <QApplication>
#include <QCoreApplication>
#include <QLocalSocket>
#include <QString>
#include <QTime>

#include "Application.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "Preference.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"
#include "SessionLog.h"
#include "TextFile.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {
  Application *iApp = NULL;
  bool Application::p_applicationForceGuiApp = false;

  /**
   * Constuctor for the Application object
   *
   * @param argc Number of arguments in argv[].  This must be passed by
   *             reference!!
   *
   * @param *argv[] An array containing the command line arguments
   *
   * @throws Isis::iException::Io - FileOpen error
   */
  Application::Application(int &argc, char *argv[]) {
    p_ui = NULL;
    p_connectionToParent = NULL;

    // Save the application name
    p_appName = argv[0];

    // Get the starting wall clock time
    // p_datetime = DateTime(&p_startTime);

    // Init
    p_startClock = 0;
    p_startDirectIO = 0;
    p_startPageFaults = 0;
    p_startProcessSwaps = 0;
    p_BatchlistPass = 0;

    // try to use US locale for numbers so we don't end up printing "," instead
    //   of "." where it might count.
    setlocale(LC_ALL, "en_US");

    char env[1024];
    strncpy(env, "LANG=en_US", 1023);
    putenv(env);

    // add qt path to 3rdParty so no default is taken from enviroment
    QString pluginPath = getenv("ISISROOT");
    pluginPath.append("/3rdParty/lib/");
    QCoreApplication::addLibraryPath(pluginPath);

    // Get the starting cpu time, direct I/Os, page faults, and swaps
    //p_startClock = clock();
    p_startDirectIO = DirectIO();
    p_startPageFaults = PageFaults();
    p_startProcessSwaps = ProcessSwaps();

    // Create user interface and log
    try {
      FileName f(QString(argv[0]) + ".xml");

      // Create preferences
      Preference::Preferences(f.name() == "unitTest.xml");

      if (!f.fileExists()) {
        f = "$ISISROOT/bin/xml/" + f.name();
        if (!f.fileExists()) {
          QString message = Message::FileOpen(f.expanded());
          throw IException(IException::Io, message, _FILEINFO_);
        }
      }
      QString xmlfile = f.expanded();

      p_ui = new UserInterface(xmlfile, argc, argv);

      if (!p_ui->IsInteractive()) {
        // Get the starting wall clock time
        p_datetime = DateTime(&p_startTime);
        m_connectTime.start();
        p_startClock = clock();

        if (p_applicationForceGuiApp) {
          new QApplication(argc, argv);
          // When QApplication is initialized, it will reset the locale to the shells locale. As a result
          // the locale needs to be reset after QApplications initialization.
          setlocale(LC_ALL, "en_US");
        }
        else {
          new QCoreApplication(argc, argv);
          // When QCoreApplication is initialized, it will reset the locale to the shells locale. As a result
          // the locale needs to be reset after QCoreApplications initialization.
          setlocale(LC_ALL, "en_US");
        }

        QCoreApplication::setApplicationName(FileName(p_appName).baseName());
      }
    }
    catch (IException &e) {
      e.print();
      exit(e.errorType());
    }

    iApp = this;

    // If we were run by another Isis app, connect to it
    if (GetUserInterface().ParentId()) {
      p_connectionToParent = new QLocalSocket;

      QString serverName = "isis_" + UserName() +
          "_" + toString(iApp->GetUserInterface().ParentId());

      p_connectionToParent->connectToServer(serverName);
      if (!p_connectionToParent->waitForConnected()) {
        delete p_connectionToParent;
        p_connectionToParent = NULL;
      }
    }
  }

  //! Destroys the Application object
  Application::~Application() {
    if (p_ui) {
      delete p_ui;
      p_ui = NULL;
    }
  }

  /**
   * Runs the program defined in the function funct.
   *
   * @param *funct
   *
   * @return int Status of the function execution
   */
  int Application::Run(void (*funct)()) {
    int status = 0;
    try {
      if (p_ui->IsInteractive()) {
        p_ui->TheGui()->Exec(funct);
      }
      else {
        if (p_ui->BatchListSize() > 0) {
          for (int i = 0; i < p_ui->BatchListSize(); i++) {
            try {
              p_ui->SetBatchList(i);

              if (i != 0) {
                p_datetime = DateTime(&p_startTime);
                m_connectTime.start();
                p_startClock = clock();
                p_startDirectIO = DirectIO();
                p_startPageFaults = PageFaults();
                p_startProcessSwaps = ProcessSwaps();
                SessionLog::TheLog(true);
              }

              funct();
              Application::FunctionCleanup();
              p_BatchlistPass++;
            }
            catch (IException &e) {
              p_ui->SetErrorList(i);
              status = Application::FunctionError(e);
              if (p_ui->AbortOnError()) {
                for (int j = (i + 1); j < p_ui->BatchListSize(); j++) {
                  p_ui->SetErrorList(j);
                  p_BatchlistPass++;
                }
                break;
              }
            }
          }
        }
        else {
          p_ui->SaveHistory();
          // The gui checks everything but not the command line mode so
          // verify if necessary. Batchlist verifies on SetBatchList
          p_ui->VerifyAll();
          funct();
          Application::FunctionCleanup();
        }
      }
    }
    catch (IException &e) {
      status = Application::FunctionError(e);
    }

#if 0
    catch (exception &e) {
      QString message = e.what();
      Isis::iExceptionSystem i(message, _FILEINFO_);
      status = i.Report();
    }
    catch (...) {
      QString message = "Unknown error expection";
      Isis::iExceptionSystem i(message, _FILEINFO_);
      status = i.Report();
    }
#endif

    return status;
  }

  /**
   * Creates an application history PvlObject
   *
   * @return PvlObject
   */
  PvlObject Application::History() {
    if (p_ui->IsInteractive()) {
      p_startClock = clock();
      p_datetime = DateTime(&p_startTime);
      m_connectTime.start();
      //cerr << "History GUI start clock=" << p_startClock << " time=" << p_startTime << endl;
    }
    PvlObject history(p_ui->ProgramName());
    history += PvlKeyword("IsisVersion", Version());
    history += PvlKeyword("ProgramVersion", p_ui->Version());
    QString path = QCoreApplication::applicationDirPath();
    history += PvlKeyword("ProgramPath", path);
    history += PvlKeyword("ExecutionDateTime", p_datetime);
    history += PvlKeyword("HostName", HostName());
    history += PvlKeyword("UserName", UserName());
    history += PvlKeyword("Description", p_ui->Brief());

    // Add the user parameters
    Pvl pvl;
    p_ui->CommandLine(pvl);
    history.addGroup(pvl.findGroup("UserParameters"));

    return history;
  }
//
  /**
   * Creates accounting PvlGroup
   *
   * @return PvlGroup Accounting Group
   */
  PvlGroup Application::Accounting() {
    double seconds = m_connectTime.elapsed() / 1000.0;
    int minutes = (int)(seconds / 60.0);
    seconds = seconds - minutes * 60.0;
    int hours = minutes / 60;
    minutes = minutes - hours * 60;
    char temp[80];
    snprintf(temp, sizeof(temp), "%02d:%02d:%04.1f", hours, minutes, seconds);
    QString conTime = temp;

    //cerr << "Accounting GUI end time=" << endTime <<  " start time=" << p_startTime << "  total=" << seconds << endl;

    // Grab the ending cpu time to compute total cpu time
    clock_t endClock = clock();
    seconds = (double(endClock) - (double)p_startClock) / CLOCKS_PER_SEC;
    minutes = (int)(seconds / 60.0);
    seconds = seconds - minutes * 60.0;
    hours = minutes / 60;
    minutes = minutes - hours * 60;
    snprintf(temp, sizeof(temp), "%02d:%02d:%04.1f", hours, minutes, seconds);
    QString cpuTime = temp;

    // Add this information to the log
    PvlGroup acct("Accounting");
    acct += PvlKeyword("ConnectTime", conTime);
    acct += PvlKeyword("CpuTime", cpuTime);

    // Not sure if these are really valuable.  If deemed so then
    // uncomment and complete private methods (DirectIO, Pagefaults, and
    // ProcessSwaps).
    //int directIO = DirectIO();
    //int pageFaults = PageFaults();
    //int processSwaps = ProcessSwaps();
    //acct += Isis::PvlKeyword("DirectIo",directIO);
    //acct += Isis::PvlKeyword("PageFaults",pageFaults);
    //acct += Isis::PvlKeyword("ProcessSwaps",processSwaps);

    return acct;
  }

  /**
   * Returns the current number of I/O's
   *
   * @return int The current number of I/O's
   */
  int Application::DirectIO() {
    return 0 - p_startDirectIO;
  }

  /**
   * Returns the current number of faults
   *
   * @return The current number of page faults
   */
  int Application::PageFaults() {
    return 0 - p_startPageFaults;
  }

  /**
   * Returns the current number of swaps
   *
   * @return The current number of process swaps
   */
  int Application::ProcessSwaps() {
    return 0 - p_startProcessSwaps;
  }

  /**
   * Writes Pvl results to sessionlog and printfile.
   *
   * @param results PvlGroup of results to add to the session log
   */
  void Application::Log(PvlGroup &results) {
    // Add it to the log file
    static bool blankLine = false;
    if (iApp) {
      SessionLog::TheLog().AddResults(results);
    }

    // See if the log file will be written to the terminal/gui
    // The results group of the Sessoion::Log will be written later
    // in Application::FunctionCleanup if TerminalOutput is on
    if (iApp && SessionLog::TheLog().TerminalOutput()) return;

    // See if we should write the info to our parents gui
    if (iApp && HasParent()) {
      ostringstream ostr;
      if (blankLine) ostr << endl;
      ostr << results << endl;
      QString data = ostr.str().c_str();
      iApp->SendParentData("LOG", data);
    }

    // Otherwise see if we need to write to our gui
    else if (iApp && iApp->GetUserInterface().IsInteractive()) {
      ostringstream ostr;
      if (blankLine) ostr << endl;
      ostr << results << endl;
      iApp->GetUserInterface().TheGui()->Log(ostr.str().c_str());
      iApp->GetUserInterface().TheGui()->ShowLog();
    }

    // Otherwise its command line mode
    else {
      if (blankLine) cout << endl;
      cout << results << endl;
    }
    blankLine = true;
  }

  /**
   * Writes the pvl group results to both a passed in Pvl log and
   * the applications log (either GUI or command line)
   *
   * @param results Pvl containing the results to add to the session log
   * @param log App log for running in code
   *
   */
  void Application::AppendAndLog(PvlGroup &results, Pvl *log) {
    Application::Log(results);
    if (log) {
      log->addGroup(results);
    }
  }

  /**
   * Writes the Pvl results to the sessionlog, but not to the printfile
   *
   * @param results Pvl containing the results to add to the session log
   */
  void Application::GuiLog(const Pvl &results) {
    // See if we should write the info to our parents gui
    Pvl copy(results);

    if (HasParent()) {
      ostringstream ostr;
      ostr << copy << endl;
      QString data = ostr.str().c_str();
      iApp->SendParentData("GUILOG", data);
    }

    // Otherwise see if we need to write to our gui
    else if (iApp->GetUserInterface().IsInteractive()) {
      ostringstream ostr;
      ostr << copy << endl;
      iApp->GetUserInterface().TheGui()->Log(ostr.str().c_str());
      iApp->GetUserInterface().TheGui()->ShowLog();
    }
  }

  /**
   * Writes the PvlGroup results to the sessionlog, but not to the printfile
   *
   * @param results PvlGroup containing the results to add to the session log
   */
  void Application::GuiLog(const PvlGroup &results) {
    // See if we should write the info to our parents gui
    PvlGroup copy(results);

    if (HasParent()) {
      ostringstream ostr;
      ostr << copy << endl;
      QString data = ostr.str().c_str();
      iApp->SendParentData("GUILOG", data);
    }

    // Otherwise see if we need to write to our gui
    else if (iApp->GetUserInterface().IsInteractive()) {
      ostringstream ostr;
      ostr << copy << endl;
      iApp->GetUserInterface().TheGui()->Log(ostr.str().c_str());
      iApp->GetUserInterface().TheGui()->ShowLog();
    }
  }

  /**
   * Writes the results to the sessionlog, but not to the printfile
   *
   * @param results QString containing the results to add to the session log
   */
  void Application::GuiLog(const QString &results) {
    // See if we should write the info to our parents gui
    if (HasParent()) {
      iApp->SendParentData("GUILOG", results);
    }

    // Otherwise see if we need to write to our gui
    else if (iApp->GetUserInterface().IsInteractive()) {
      iApp->GetUserInterface().TheGui()->Log(results);
      iApp->GetUserInterface().TheGui()->ShowLog();
    }
  }

  /**
   * Returns the UserInterface object
   *
   * @return A pointer to the UserInterface object
   */
  Isis::UserInterface &Application::GetUserInterface() {
    return *iApp->p_ui;
  }

  /**
   * Returns whether the application has a parent or not
   *
   * @return bool Returns true if it has a parent, and false if it does not
   */
  bool Application::HasParent() {
    if (iApp == NULL) return false;
    if (iApp->p_ui == NULL) return false;
    if (iApp->p_ui->ParentId() == 0) return false;
    return true;
  }

  /**
   * Sends errors to the parent
   *
   * @param &errors A PvlObject of the errors
   */
  void Application::SendParentErrors(Isis::PvlObject &errors) {
    if (!HasParent()) return;

    for (int i = 0; i < errors.groups(); i++) {
      ostringstream ostr;
      ostr << errors.group(i) << endl;
      QString data = ostr.str().c_str();
      iApp->SendParentData("ERROR", data);
    }
  }


  /**
   * @param code
   * @param message
   */
  void Application::SendParentData(const QString code,
      const QString &message) {
    // See if we need to connect to the parent
    if (p_connectionToParent == NULL) {
      QString msg = "This process (program) was executed by an existing Isis "
          "process. However, we failed to establish a communication channel "
          "with the parent (launcher) process. The parent process has a PID of "
          "[" + toString(iApp->GetUserInterface().ParentId()) + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    // Have connection so build data QString and send it
    QString data = code;
    data += char(27);
    data += message;
    data += char(27);
    data += '\n';

    if (p_connectionToParent->write(data.toLatin1().data(), data.toLatin1().size()) == -1) {
      QString msg = "This process (program) was executed by an exiting Isis "
          "process. A communication channel was established with the parent "
          "(launcher) process, but when we tried to send data to the parent "
          "process an error occurred. The parent process has a PID of [" +
          QString(iApp->GetUserInterface().ParentId()) + "]";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    p_connectionToParent->waitForBytesWritten(-1);
  }


  /**
   * Cleans up after the function by writing the log, saving the history, and
   * either sending the log to the parent if it has one, printing the log data
   * to the terminal or showing the log in the gui.
   */
  void Application::FunctionCleanup() {

    SessionLog::TheLog().Write();

    if (SessionLog::TheLog().TerminalOutput()) {
      if (HasParent()) {
        ostringstream ostr;
        ostr << SessionLog::TheLog() << endl;
        QString data = ostr.str().c_str();
        iApp->SendParentData("LOG", data);
      }
      else if (p_ui->IsInteractive()) {
        ostringstream ostr;
        ostr << SessionLog::TheLog() << endl;
        p_ui->TheGui()->Log(ostr.str().c_str());
        p_ui->TheGui()->ShowLog();
      }
      else {
        cout << SessionLog::TheLog() << endl;
      }
    }

    // If debugging flag on write debugging log
    if (p_ui->GetInfoFlag()) {
      QString filename = p_ui->GetInfoFileName();
      Pvl log;
      QString app = (QString)QCoreApplication::applicationDirPath() + "/" + p_appName;
      if (p_BatchlistPass == 0) {
        stringstream ss ;
        ss << SessionLog::TheLog();
        ss.clear();
        ss >> log;
        PvlGroup uname = GetUnameInfo();
        PvlGroup env = GetEnviromentInfo();
        log.addGroup(uname);
        log.addGroup(env);
      }

      // Write to file
      if (filename.compare("") != 0) {

        if (p_BatchlistPass == 0) {
          ofstream debugingLog(filename.toLatin1().data());
          if (!debugingLog.good()) {
            QString msg = "Error opening debugging log file [" + filename + "]";
            throw IException(IException::Io, msg, _FILEINFO_);
          }
          debugingLog << log << endl;
          debugingLog << "\n############### User Preferences ################\n" << endl;
          debugingLog << Preference::Preferences();
          debugingLog << "\n############## System Disk Space ################\n" << endl;
          debugingLog << GetSystemDiskSpace() << endl;
          debugingLog << "\n############ Executable Information #############\n" << endl;
          debugingLog << GetLibraryDependencies(app) << endl;
          debugingLog.close();
        }
        else {
          ofstream debugingLog(filename.toLatin1().data(), ios_base::app);
          debugingLog << SessionLog::TheLog() << endl;
          debugingLog.close();
        }
      }
      else {   // Write to std out
        if (p_BatchlistPass == 0) {
          cout << log << endl;
          cout << "\n############### User Preferences ################\n" << endl;
          cout << Preference::Preferences();
          cout << "\n############## System Disk Space ################\n" << endl;
          cout << GetSystemDiskSpace() << endl;
          cout << "\n############ Executable Information #############\n" << endl;
          cout << GetLibraryDependencies(app) << endl;
        }
        else {
          cout << SessionLog::TheLog() << endl;
        }
      }
    }


  }

  /**
   * Adds the error to the session log, sends the error to the parent if it has
   * one, loads the error message into the gui or prints it to the command line,
   * gets the error type and returns it.
   *
   * @param e The Isis::iException
   *
   * @return int The Error Type
   */
  int Application::FunctionError(IException &e) {
    Pvl errors = e.toPvl();
    for (int i = 0; i < errors.groups(); i++) {
      PvlGroup &group = errors.group(i);
      if (group.isNamed("Error")) {
        group += PvlKeyword("Program", Application::Name());
      }
    }
    SessionLog::TheLog().AddError(errors);
    SessionLog::TheLog().Write();

    if (HasParent()) {
      SendParentErrors(errors);
    }
    else if (p_ui->IsInteractive()) {
      p_ui->TheGui()->LoadMessage(Application::formatError(e));
    }
    else if (SessionLog::TheLog().TerminalOutput()) {
      cerr << SessionLog::TheLog() << endl;
    }
    else {
      cerr << Application::formatError(e) << endl;
    }

    // If debugging flag on write debugging log
    if (p_ui->GetInfoFlag()) {
      QString filename = p_ui->GetInfoFileName();
      Pvl log;
      QString app = (QString)QCoreApplication::applicationDirPath() + "/" + p_appName;
      if (p_BatchlistPass == 0) {
        stringstream ss ;
        ss << SessionLog::TheLog();
        ss.clear();
        ss >> log;
        PvlGroup uname = GetUnameInfo();
        PvlGroup env = GetEnviromentInfo();
        log.addGroup(uname);
        log.addGroup(env);
      }

      // Write to file
      if (filename.compare("") != 0) {
        if (p_BatchlistPass == 0) {
          ofstream debugingLog(filename.toLatin1().data());
          if (!debugingLog.good()) {
            QString msg = "Error opening debugging log file [" + filename + "]";
            throw IException(IException::Io, msg, _FILEINFO_);
          }
          debugingLog << log << endl;
          debugingLog << "\n############### User Preferences ################\n" << endl;
          debugingLog << Preference::Preferences();
          debugingLog << "\n############ System Disk Space #############\n" << endl;
          debugingLog << GetSystemDiskSpace() << endl;
          debugingLog << "\n############ Executable Information #############\n" << endl;
          debugingLog << GetLibraryDependencies(app) << endl;
          debugingLog.close();
        }
        else {
          ofstream debugingLog(filename.toLatin1().data(), ios_base::app);
          debugingLog << SessionLog::TheLog() << endl;
          debugingLog.close();
        }
      }
      else {   // Write to std out
        if (p_BatchlistPass == 0) {
          cout << log << endl;
          cout << "\n############### User Preferences ################\n" << endl;
          cout << Preference::Preferences();
          cout << "\n############ System Disk Space #############\n" << endl;
          cout << GetSystemDiskSpace() << endl;
          cout << "\n############ Executable Information #############\n" << endl;
          cout << GetLibraryDependencies(app) << endl;
        }
        else {
          cout << SessionLog::TheLog() << endl;
        }
      }
    }

    return (int)e.errorType();
  }

  /**
   * Loads the error message into the gui, but does not write it to the session
   *  log.
   *
   * @param e The Isis::iException
   */
  void Application::GuiReportError(IException &e) {
    QString errorMessage = Application::formatError(e);
    if (errorMessage == "") {
      p_ui->TheGui()->ProgressText("Stopped");
    }
    else {
      p_ui->TheGui()->LoadMessage(errorMessage);
      p_ui->TheGui()->ProgressText("Error");
    }

    if (p_ui->TheGui()->ShowWarning())
      exit(0);
  }

  QString Application::formatError(IException &e) {
    stringstream stringStream;
    QString stringErrors = e.toString(Preference::Preferences().reportFileLine());

    if (Preference::Preferences().outputErrorAsPvl()) {
      stringStream << stringErrors;
      Pvl errors;
      stringStream >> errors;
      for (int i = 0; i < errors.groups(); i++) {
        PvlGroup &group = errors.group(i);
        if (group.isNamed("Error")) {
          group += PvlKeyword("Program", Application::Name());
        }
      }
      stringStream.str(std::string());
      stringStream.clear();
      stringStream << errors;
      stringErrors = stringStream.str().c_str();
    }

    return stringErrors;
  }

  QString Application::p_appName("Unknown"); //!<
  /**
   * Returns the name of the application.  Returns 'Unknown' if the application
   * or gui equal NULL
   *
   * @return QString The application name
   */
  QString Application::Name() {
    return p_appName;
  }

  /**
   * Updates the progress bar in the gui.
   *
   * @param text Progress text
   *
   * @param print
   */
  void Application::UpdateProgress(const QString &text, bool print) {
    if (HasParent() && print) {
      iApp->SendParentData(QString("PROGRESSTEXT"), text);
    }
    else if (p_ui->IsInteractive()) {
      p_ui->TheGui()->ProgressText(text);
    }
    else if (print) {
      QString msg = p_ui->ProgramName() + ": " + text;
      cout << msg << endl;
    }

    ProcessGuiEvents();
  }

  /**
   * Updates the progress bar percent.
   *
   * @param percent The percent of the application that is complete
   *
   * @param print
   */
  void Application::UpdateProgress(int percent, bool print) {
    if (HasParent() && print) {
      QString data = toString(percent);
      iApp->SendParentData(QString("PROGRESS"), data);
    }
    else if (p_ui->IsInteractive()) {
      p_ui->TheGui()->Progress(percent);
    }
    else if (print) {
      if (percent < 100) {
        cout << percent << "% Processed\r" << flush;
      }
      else {
        cout << percent << "% Processed" << endl;
      }
    }
  }

  /**
   * Processes the gui events. If the event is cancel, it throws a cancel
   * exception
   *
   * @throws Isis::iException::Cancel - The event was cancelled
   */
  void Application::ProcessGuiEvents() {
    if (p_ui->IsInteractive()) {
      if (p_ui->TheGui()->ProcessEvents()) {
        throw IException();
      }
    }
  }


  /**
   * Returns the date and time as a QString.
   *
   * @param *curtime
   *
   * @return QString The date and time
   */
  QString Application::DateTime(time_t *curtime) {
    time_t startTime = time(NULL);
    if (curtime != 0) *curtime = startTime;
    struct tm *tmbuf = localtime(&startTime);
    char timestr[80];
    strftime(timestr, 80, "%Y-%m-%dT%H:%M:%S", tmbuf);
    return(QString) timestr;
  }

  /**
   * Returns the user name. Returns 'Unknown' if it cannot find the user name.
   *
   * @return QString User Name
   */
  QString Application::UserName() {
    return userName();
  }

  /**
   * Returns the host name.  Returns 'Unknown' if it cannot find the host name.
   *
   * @return QString Host Name
   */
  QString Application::HostName() {
    return hostName();
  }

  /**
   * The Isis Version for this application.
   * @return @b QString
   *
   */
  QString Application::Version() {
    return isisVersion();
  }


  /**
   * Runs various system specific uname commands and returns the results
   *
   * @return PvlGroup containing uname information
   */
  PvlGroup Application::GetUnameInfo() {
    // Create a temporary file to store console output to
    FileName temp = FileName::createTempFile("$temporary/UnameConsoleInfo.txt");
    QString tempFile = temp.expanded();

    // Uname commands output to temp file with each of the following
    // values on its own line in this order:
    // machine hardware name, processor, hardware platform name,
    // operating system, kernel name, kernel version, kernel release, all
    PvlGroup unameGroup("UNAME");
    ifstream readTemp;

#if defined(__linux__)
    // Write uname outputs to temp file
    ProgramLauncher::RunSystemCommand("uname -m > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -p > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -i > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -o > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -s > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -v > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -r > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -a > " + tempFile);
    // Read data from temp file
    char value[256];
    readTemp.open(tempFile.toLatin1().data(), ifstream::in);
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("MachineHardware", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("Processor", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("HardwarePlatform", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("OperatingSystem", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("KernelName", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("KernelVersion", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("KernelRelease", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("FullUnameString", value));

#elif defined(__APPLE__)
    // Write uname outputs to temp file
    ProgramLauncher::RunSystemCommand("uname -m > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -p > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -s > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -v > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -r > " + tempFile);
    ProgramLauncher::RunSystemCommand("uname -a > " + tempFile);

    // Read data from temp file
    char value[256];
    readTemp.open(tempFile.toLatin1().data(), ifstream::in);
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("MachineHardware", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("Processor", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("OperatingSystem", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("OperatingSystemVersion", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("OperatingSystemRelease", value));
    readTemp.getline(value, 256);
    unameGroup.addKeyword(PvlKeyword("FullUnameString", value));
#endif

    // remove temp file and return
    remove(tempFile.toLatin1().data());
    return unameGroup;
  }

  /**
   * Runs some printenv commands that return Isis related Enviroment Variables.
   *
   * @return PvlGroup containing Enviroment information
   * @todo Replace printenv commands with c library getenv
   * @todo
   */
  PvlGroup Application::GetEnviromentInfo() {
    // Create a temporary file to store console output to
    FileName temp = FileName::createTempFile("$temporary/EnviromentInfo.txt");
    QString tempFile = temp.expanded();
    PvlGroup envGroup("EnviromentVariables");
    ifstream readTemp;

    QString env1 = "printenv SHELL >| " + tempFile;
    QString env2 = "printenv HOME >> " + tempFile;
    QString env3 = "printenv PWD >> " + tempFile;
    QString env5 = "printenv ISISROOT >> " + tempFile;
    QString env6 = "printenv ISISDATA >> " + tempFile;
    ProgramLauncher::RunSystemCommand(env1);
    ProgramLauncher::RunSystemCommand(env2);
    ProgramLauncher::RunSystemCommand(env3);
    ProgramLauncher::RunSystemCommand(env5);
    ProgramLauncher::RunSystemCommand(env6);
    // Read data from temp file
    char value[511];
    readTemp.open(tempFile.toLatin1().data(), ifstream::in);
    readTemp.getline(value, 255);
    envGroup.addKeyword(PvlKeyword("Shell", value));
    readTemp.getline(value, 255);
    envGroup.addKeyword(PvlKeyword("Home", value));
    readTemp.getline(value, 255);
    envGroup.addKeyword(PvlKeyword("Pwd", value));
    readTemp.getline(value, 255);
    envGroup.addKeyword(PvlKeyword("ISISROOT", value));
    readTemp.getline(value, 255);
    envGroup.addKeyword(PvlKeyword("ISISDATA", value));

    // remove temp file and return
    QString cleanup = "rm -f " + tempFile;
    ProgramLauncher::RunSystemCommand(cleanup);
    return envGroup;
  }

  /**
   * Runs df to see the disk space availability
   *
   * @return QString containing df results
   */
  QString Application::GetSystemDiskSpace() {
    FileName temp = FileName::createTempFile("$temporary/SystemDiskSpace.txt");
    QString tempFile = temp.expanded();
    ifstream readTemp;
    QString diskspace = "df >| " + tempFile;
    ProgramLauncher::RunSystemCommand(diskspace);
    readTemp.open(tempFile.toLatin1().data(), ifstream::in);

    QString results = "";
    char tmp[512];
    while (!readTemp.eof()) {
      readTemp.getline(tmp, 512);
      results += tmp;
      results += "\n";
    }

    // remove temp file and return
    QString cleanup = "rm -f " + tempFile;
    ProgramLauncher::RunSystemCommand(cleanup);
    return results;
  }

  /**
   * Runs ldd on linux and sun and otool on macs to get information about the applicaiton run
   *
   * @return QString containing application information
   */
  QString Application::GetLibraryDependencies(QString file) {
    FileName temp = FileName::createTempFile("$temporary/LibraryDependencies.txt");
    QString tempFile = temp.expanded();
    ifstream readTemp;
    QString dependencies = "";
#if defined(__linux__)
    dependencies = "ldd -v " + file + " >| " + tempFile;
#elif defined(__APPLE__)
    dependencies = "otool -L " + file + " >| " + tempFile;
#elif defined (__sun__)
    dependencies = "ldd -v " + file + " >| " + tempFile;
#endif
    ProgramLauncher::RunSystemCommand(dependencies);
    readTemp.open(tempFile.toLatin1().data(), ifstream::in);

    QString results = "";
    char tmp[512];
    while (!readTemp.eof()) {
      readTemp.getline(tmp, 512);
      results += tmp;
      results += "\n";
    }

    // remove temp file and return
    QString cleanup = "rm -f " + tempFile;
    ProgramLauncher::RunSystemCommand(cleanup);
    return results;
  }
}  //end namespace isis
