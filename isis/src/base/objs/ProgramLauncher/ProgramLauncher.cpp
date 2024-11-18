/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProgramLauncher.h"

#include <iostream>
#include <sstream>
#include <sys/wait.h>

#include <QLocalServer>
#include <QLocalSocket>
#include <QProcess>

#include "Application.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"

using namespace std;
namespace Isis {
  /**
   * Executes the Isis program with the given arguments. This will handle logs,
   *   GUI updates, and similar tasks. Please use this even when there is no
   *   instance of Isis::Application, so long as the thing you are running has
   *   an Isis::Application (do not use this for qview, qnet, qtie, etc for
   *   now).
   *
   * Please do not specify -pid.
   *
   * @param programName The Isis program name to be run (i.e. catlab, cubeatt)
   * @param parameters The arguments to give to the program that is being run
   */
  void ProgramLauncher::RunIsisProgram(QString programName,
                                       QString parameters) {
    FileName program(programName);
    FileName isisExecutableFileName("$ISISROOT/bin/" + program.name());
    bool isIsisProgram = false;

    if(isisExecutableFileName.fileExists()) {
      isIsisProgram = true;
      program = isisExecutableFileName;
    }

    PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
    QString tempDir = dataDir["Temporary"];

    QString command = program.expanded() + " " + parameters +
        " -pid=" + toString(getpid());

    if(!isIsisProgram) {
      QString msg = "Program [" + programName + "] does not appear to be a "
          "valid Isis program";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    QString serverName = "isis_" + Application::UserName() +
        "_" + toString(getpid());

    QLocalServer server;
    server.listen(serverName);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("TEMPORARY", tempDir);

    QProcess childProcess;
    childProcess.setProcessEnvironment(env);
    childProcess.setProcessChannelMode(QProcess::ForwardedChannels);
    childProcess.start("bash", QStringList() << "-c" << command);
    childProcess.waitForStarted();

    bool connected = false;

    while(!connected && childProcess.state() != QProcess::NotRunning) {
      // Give time for the process to connect to us or for it to finish
      // wait 30s for the new connection....
      connected = server.waitForNewConnection(30000);
      childProcess.waitForFinished(100);
    }

    if(!connected) {
      QString msg = "Isis child process failed to communicate with parent";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QLocalSocket *childSocket = server.nextPendingConnection();
    IException errors;

    // Don't return until we're done running this program
    while(childSocket->state() != QLocalSocket::UnconnectedState) {
      bool insideCode = true;
      bool messageDone = false;

      QString code;
      QString message;
      QByteArray lineData;

      if(childSocket->waitForReadyRead(1000)) {
        lineData = childSocket->read(childSocket->bytesAvailable());

        for(int i = 0; i < lineData.size(); i++) {
          if(insideCode) {
            if(lineData[i] != (char)27) {
              code += lineData[i];
            }
            else {
              insideCode = false;
            }
          }
          else {
            if(lineData[i] != (char)27) {
              message += lineData[i];
            }
            else {
              messageDone = true;
              insideCode = true;
              i ++; // skip \n that should always exist here
            }
          }

          if(messageDone) {
            errors.append(
              ProcessIsisMessageFromChild(code, message));
            code = "";
            message = "";
            messageDone = false;
          }
        }
      }
    }

    childProcess.waitForFinished();

    if(childProcess.exitCode() != 0) {
      QString msg = "Running Isis program [" + programName + "] failed with "
                    "return status [" + toString(childProcess.exitCode()) + "]";
      throw IException(errors, IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * This interprets a message sent along the pipe from a child process to us
   *   (the parent).
   *
   * @param code The text code of the message - this is used to determine what
   *             the message contains.
   * @param msg The data sent along with a code. This is a string, number,
   *            PvlGroup, Pvl, etc... really anything. It depends on the code
   *            parameter.
   */
  IException
      ProgramLauncher::ProcessIsisMessageFromChild(QString code, QString msg) {
    IException errors;

    if(code == "PROGRESSTEXT" && iApp) {
      iApp->UpdateProgress(msg, true);
    }
    else if(code == "PROGRESS" && iApp) {
      iApp->UpdateProgress(toInt(msg), true);
    }
    else if(code == "LOG" && iApp) {
      stringstream msgStream;
      msgStream << msg;
      Pvl logPvl;
      msgStream >> logPvl;

      if(logPvl.groups() == 1 &&
         logPvl.keywords() == 0 &&
         logPvl.objects() == 0) {
        iApp->Log(logPvl.group(0));
      }
    }
    else if(code == "GUILOG" && iApp) {
      iApp->GuiLog(msg);
    }
    else if(code == "ERROR") {
      stringstream msgStream;
      msgStream << msg;
      Pvl errorPvl;
      msgStream >> errorPvl;

      for(int i = 0; i < errorPvl.groups(); i++) {
        PvlGroup &g = errorPvl.group(i);
        QString emsg = g["Message"];
        int ecode = g["Code"];
        QString efile = g["File"];
        int eline = g["Line"];

        errors.append(
          IException((IException::ErrorType)ecode, emsg, efile.toLatin1().data(), eline));
      }
    }

    return errors;
  }


  /**
   * This runs arbitrary system commands. You can run programs like "qview" with
   *   this, or commands like "ls | grep *.cpp > out.txt". Please do not use
   *   this for Isis programs not in qisis.
   *
   * Example: qview should use RunIsisProgram to run camstats.
   *          camstats should use RunSystemCommand to run qview.
   *
   * @param fullCommand A string containing the command formatted like what
   *                    you would type in a terminal
   */
  void ProgramLauncher::RunSystemCommand(QString fullCommand) {
    int status = system(fullCommand.toLatin1().data());

    if(status != 0) {
      QString msg = "Executing command [" + fullCommand +
                    "] failed with return status [" + toString(status) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }
}  //end namespace isis
