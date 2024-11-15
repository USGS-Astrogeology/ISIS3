/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include <QFile>

#include "Pipeline.h"
#include "PipelineApplication.h"
#include "ProgramLauncher.h"
#include "IException.h"
#include "Application.h"
#include "Preference.h"
#include "Progress.h"
#include "FileList.h"
#include "FileName.h"

using namespace Isis;
using namespace std;

namespace Isis {


  /**
   * This is the one and only Pipeline constructor. This will initialize a
   * pipeline object.
   *
   * @param procAppName This is an option parameter that gives the pipeline a name
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added initialization for new member
   *                                       p_pausePosition
   */
  Pipeline::Pipeline(const QString &procAppName) {
    p_pausePosition = -1;
    p_procAppName = procAppName;
    p_addedCubeatt = false;
    p_outputListNeedsModifiers = false;
    p_continue = false;
  }


  /**
   * This destroys the pipeline
   *
   */
  Pipeline::~Pipeline() {
    for (int i = 0; i < (int)p_apps.size(); i++) {
      delete p_apps[i];
    }

    p_apps.clear();
  }


  /**
   * This method is the core of the pipeline class. This method tells each
   * PipelineApplication to learn about itself and calculate necessary filenames,
   * execution calls, etc... Pipeline error checking happens in here, so if a
   * Pipeline is invalid this method will throw the IException.
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *                                       vector since a [ause is added as a
   *                                       NULL pointer.
   */
  void Pipeline::Prepare() {
    // Nothing in the pipeline? quit
    if (p_apps.size() == 0) return;

    // We might have to modify the pipeline and try again, so keep track of if this is necessary
    bool successfulPrepare = false;

    while (!successfulPrepare) {
      // Assume we'll be successful
      successfulPrepare = true;
      bool foundFirst = false;

      // Keep track of whether or not we must remove virtual bands
      bool mustElimBands = false;

      // Look to see if we need to eliminate virtual bands...
      for (unsigned int i = 0; i < p_virtualBands.size(); i++) {
        mustElimBands |= !p_virtualBands[i].isEmpty();
      }

      // Keep track of temp files for conflicts
      vector<QString> tmpFiles;

      // Loop through all the pipeline apps, look for a good place to remove virtual
      //   bands and tell the apps the prepare themselves. Double check the first program
      //   is not branched (expecting multiple inputs).
      for (int i = 0; i < (int)p_apps.size() && successfulPrepare; i++) {
        if (p_apps[i] == NULL) continue;
        if (mustElimBands && p_apps[i]->SupportsVirtualBands()) {
          if (i != 0 && p_virtualBands.size() != 1) {
            QString message = "If multiple original inputs were set in the pipeline, the first application must support virtual bands.";
            throw IException(IException::Programmer, message, _FILEINFO_);
          }

          p_apps[i]->SetVirtualBands(p_virtualBands);
          mustElimBands = false;

          // We might have added the "cubeatt" program to eliminate bands,
          //   remove it if we found something else to do the virtual bands.
          // **This causes a failure in our calculations, start over.
          if (p_addedCubeatt && i != (int)p_apps.size() - 1) {
            delete p_apps[p_apps.size() - 1];
            p_apps.resize(p_apps.size() - 1);
            p_appIdentifiers.resize(p_appIdentifiers.size() - 1);
            p_apps[p_apps.size() - 1]->SetNext(NULL);
            p_addedCubeatt = false;
            successfulPrepare = false;
            continue;
          }
        }
        else {
          // Pipeline is responsible for the virtual bands, reset any apps
          //   who have an old virtual bands setting.
          vector<QString> empty;
          p_apps[i]->SetVirtualBands(empty);
        }

        // This instructs the pipeline app to prepare itself; all previous pipeline apps must
        //   be already prepared. Future pipeline apps do not have to be.
        p_apps[i]->BuildParamString();

        // keep track of tmp files
        vector<QString> theseTempFiles = p_apps[i]->TemporaryFiles();
        for (int tmpFile = 0; tmpFile < (int)theseTempFiles.size(); tmpFile++) {
          // no need to delete blank files
          if (theseTempFiles[tmpFile].contains("blank")) {
            tmpFiles.push_back(theseTempFiles[tmpFile]);
          }
        }

        if (!foundFirst && p_apps[i]->Enabled()) {
          foundFirst = true;

          if (p_apps[i]->InputBranches().size() != OriginalBranches().size()) {
            QString msg = "The program [" + p_apps[i]->Name() + "] can not be the first in the pipeline";
            msg += " because it must be run multiple times with unspecified varying inputs";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        }
      }

      // Make sure we found an app!
      if (!foundFirst) {
        string msg = "No applications are enabled in the pipeline";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      // Make sure all tmp files are unique!
      for (int i = 0; successfulPrepare && i < (int)tmpFiles.size(); i++) {
        for (int j = i + 1; j < (int)tmpFiles.size(); j++) {
          if (tmpFiles[i] == tmpFiles[j]) {
            QString msg = "There is a conflict with the temporary file naming. The temporary file [";
            msg += tmpFiles[i] + "] is created twice.";
            throw IException(IException::Programmer, msg, _FILEINFO_);
          }
        }
      }

      // We failed at eliminating bands, add stretch to our programs and try again
      if (successfulPrepare && mustElimBands) {
        AddToPipeline("cubeatt", "~PIPELINE_RESERVED_FOR_BANDS~");
        Application("~PIPELINE_RESERVED_FOR_BANDS~").SetInputParameter("FROM", true);
        Application("~PIPELINE_RESERVED_FOR_BANDS~").SetOutputParameter("TO", "final");
        p_addedCubeatt = true;
        successfulPrepare = false;
      }

      int lastApp = p_apps.size()-1;
      if (p_apps[p_apps.size()-1] == NULL)
        lastApp = p_apps.size() - 2;

      if (p_apps[lastApp]->GetOutputs().size() == 0) {
        string msg = "There are no outputted files in the pipeline. At least one program must generate an output file.";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
  }


  /**
   * This method executes the pipeline. When you're ready to start your proc
   * program, call this method.
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *                                       vector since a pause is added as a
   *                                       NULL pointer.  Also adjusted
   *                                       application loops for potential
   *                                       pauses.
   */
  void Pipeline::Run() {
    // Prepare the pipeline programs
    Prepare();

    // Get the starting point
    p_pausePosition++;

    Progress pipelineProg;
    pipelineProg.SetText(p_procAppName);
    pipelineProg.SetMaximumSteps(1);
    pipelineProg.CheckStatus();

    // Go through these programs, executing them
    for (int i = p_pausePosition; i < Size(); i++) {

      // Return to caller for a pause
      if (p_apps[i] == NULL) {
        p_pausePosition = i;
        return;
      }

      if (Application(i).Enabled()) {
        Progress appName;
        appName.SetText("Running " + Application(i).Name());
        appName.SetMaximumSteps(1);
        appName.CheckStatus();

        // grab the sets of parameters this program needs to be run with
        const vector<QString> &params = Application(i).ParamString();
        for (int j = 0; j < (int)params.size(); j++) {

          // check for non-program run special strings
          QString special(params[j].mid(0, 7));

          // If ">>LIST", then we need to make a list file
          if (special == ">>LIST ") {
            QString cmd = params[j].mid(7);

            QStringList listData = cmd.split(" ");
            QString listFileName = listData.takeFirst();
            FileList listFile;

            while (!listData.isEmpty()) {
              listFile.push_back(listData.takeFirst());
            }

            listFile.write(listFileName);
          }
          else {
            // Nothing special is happening, just execute the program
            try {
              ProgramLauncher::RunIsisProgram(Application(i).Name(), params[j]);
            }
            catch (IException &e) {
              if (!p_continue && !Application(i).Continue()) {
                throw;
              }
              else {
                e.print();
                cerr << "Continuing ......" << endl;
              }
            }
          }
        }
      }
    }

    // Remove temporary files now
    if (!KeepTemporaryFiles()) {
      for (int i = 0; i < Size(); i++) {
        if (p_apps[i] == NULL) continue;
        if (Application(i).Enabled()) {
          vector<QString> tmpFiles = Application(i).TemporaryFiles();
          for (int file = 0; file < (int)tmpFiles.size(); file++) {
            QFile::remove(tmpFiles[file]);
          }
        }
      }
    }

    // Reset pause position
    p_pausePosition = -1;
  }


  /**
   * This method is used to set the original input file. This file is the first
   * program's input, and the virtual bands will be taken directly from this
   * parameter.
   *
   * @param inputParam The parameter to get from the user interface that contains
   *                   the input file
   */
  void Pipeline::SetInputFile(const char *inputParam) {
    SetInputFile(QString(inputParam));
  }


  /**
   * This method is used to set the original input file. This file is the first
   * program's input, and the virtual bands will be taken directly from this
   * parameter.
   *
   * @param inputParam The parameter to get from the user interface that contains
   *                   the input file
   * @history 2010-12-20 Sharmila Prasad - Changed p_originalBranches array
   *                                       to p_inputBranches
   */
  void Pipeline::SetInputFile(const QString &inputParam) {
    UserInterface &ui = Application::GetUserInterface();
    p_originalInput.push_back(ui.GetCubeName(inputParam));
    p_inputBranches.push_back(inputParam);
    p_virtualBands.push_back(ui.GetInputAttribute(inputParam).toString());
  }


  /**
   * This method is used to set the original input file. No virtual bands will
   * be read, and the inputFile parameter will be read as a path to a file instead
   * of as a parameter.
   *
   * @param inputParam A filename object containing the location of the input file
   * @history 2010-12-20 Sharmila Prasad - Changed p_originalBranches array
   *                                       to p_inputBranches
   */
  void Pipeline::SetInputFile(const FileName &inputFile) {
    p_originalInput.push_back(inputFile.original());
    p_inputBranches.push_back(inputFile.original());
    p_virtualBands.push_back("");
  }


  /**
   * This method is used to set the original input files. These files are the
   * first program's input, a branch will be added for every line in the file, and
   * the virtual bands will be taken directly from this parameter.
   *
   * @param inputParam The parameter to get from the user interface that contains
   *                   the input file
   */
  void Pipeline::SetInputListFile(const char *inputParam) {
    SetInputListFile(QString(inputParam));
  }


  /**
   * This method is used to set the original input files. These files are the
   * first program's input, a branch will be added for every line in the file, and
   * the virtual bands will be taken directly from this parameter.
   *
   * @param inputParam The parameter to get from the user interface that contains
   *                   the input file
   * @history 2010-12-20 Sharmila Prasad - Changed p_originalBranches array
   *                                       to p_inputBranches
   */
  void Pipeline::SetInputListFile(const QString &inputParam) {
    UserInterface &ui = Application::GetUserInterface();
    FileName inputFileName = FileName(ui.GetFileName(inputParam));

    SetInputListFile(inputFileName);
  }


  /**
   * This method is used to set the original input files. These files are the
   * first program's input, a branch will be added for every line in the file.
   *
   * @param inputParam The filename of the list file contains the input files
   * @history 2010-12-20 Sharmila Prasad - Changed p_originalBranches array
   *                                       to p_inputBranches
   */
  void Pipeline::SetInputListFile(const FileName &inputFileName) {
    FileList filelist(inputFileName.expanded());
    FileName filename;
    int branch = 1;

    while (!filelist.isEmpty()) {
      filename = filelist.takeFirst();
      p_originalInput.push_back(filename.expanded());
      p_inputBranches.push_back(inputFileName.name() + toString(branch));
      p_virtualBands.push_back("");
      p_finalOutput.push_back(filename.name());

      branch ++;
    }

    p_outputListNeedsModifiers = true;
  }


  /**
   * This method is used to set the original input file. This file is the first
   * program's input.
   *
   * @param inputParam The parameter to get from the user interface that contains
   *                   the input file
   * @param virtualBandsParam The parameter to get from the user interface that
   *                          contains the virtual bands list; internal default is
   *                          supported. Empty string if no virtual bands
   *                          parameter exists.
   */
  void Pipeline::SetInputFile(const char *inputParam, const char *virtualBandsParam) {
    SetInputFile(QString(inputParam), QString(virtualBandsParam));
  }


  /**
   * This method is used to set the original input file. This file is the first
   * program's input.
   *
   * @param inputParam The parameter to get from the user interface that contains
   *                   the input file
   * @param virtualBandsParam The parameter to get from the user interface that
   *                          contains the virtual bands list; internal default is
   *                          supported. Empty string if no virtual bands
   *                          parameter exists.
   * @history 2010-12-20 Sharmila Prasad - Changed p_originalBranches array
   *                                       to p_inputBranches
   */
  void Pipeline::SetInputFile(const QString &inputParam, const QString &virtualBandsParam) {
    UserInterface &ui = Application::GetUserInterface();
    p_originalInput.push_back(ui.GetAsString(inputParam));
    p_inputBranches.push_back(inputParam);

    if (!virtualBandsParam.isEmpty() && ui.WasEntered(virtualBandsParam)) {
      p_virtualBands.push_back(ui.GetAsString(virtualBandsParam));
    }
    else {
      p_virtualBands.push_back("");
    }
  }


  /**
   * This method is used to set the final output file. If no programs generate
   * output, the final output file will not be used. If the output file was not
   * entered, one will be generated automatically and placed into the current
   * working folder.
   *
   * @param outputParam The parameter to get from the user interface that contains
   *                    the output file; internal default is supported.
   */
  void Pipeline::SetOutputFile(const char *outputParam) {
    SetOutputFile(QString(outputParam));
  }


  /**
   * This method is used to set the final output file. If no programs generate
   * output, the final output file will not be used. If the output file was not
   * entered, one will be generated automatically and placed into the current
   * working folder.
   *
   * @param outputParam The parameter to get from the user interface that contains
   *                    the output file; internal default is supported.
   */
  void Pipeline::SetOutputFile(const QString &outputParam) {
    UserInterface &ui = Application::GetUserInterface();
    p_finalOutput.clear();

    if (ui.WasEntered(outputParam)) {
      p_finalOutput.push_back(ui.GetAsString(outputParam));
    }
  }


  /**
   * This method is used to set the final output file. If no programs generate
   * output, the final output file will not be used. If the output file was not
   * entered, one will be generated automatically and placed into the current
   * working folder.
   *
   * @param outputFile The filename of the output file; NOT the parameter name.
   */
  void Pipeline::SetOutputFile(const FileName &outputFile) {
    p_finalOutput.clear();
    p_finalOutput.push_back(outputFile.expanded());
  }


  /**
   * This method is used to set an output list file. Basically, this means the
   * output filenames are specified in the list file. Internal defaults/automatic
   * name calculations are supported.
   *
   * @param outputFileNameList Parameter name containing the path to the output
   *                           list file
   */
  void Pipeline::SetOutputListFile(const char *outputFileNameParam) {
    SetOutputListFile(QString(outputFileNameParam));
  }


  /**
   * This method is used to set an output list file. Basically, this means the
   * output filenames are specified in the list file. Internal defaults/automatic
   * name calculations are supported.
   *
   * @param outputFileNameList Parameter name containing the path to the output
   *                           list file
   */
  void Pipeline::SetOutputListFile(const QString &outputFileNameParam) {
    UserInterface &ui = Application::GetUserInterface();

    if (ui.WasEntered(outputFileNameParam)) {
      SetOutputListFile(FileName(ui.GetFileName(outputFileNameParam)));
    }
    else {
      p_finalOutput.clear();

      // Calculate output files
      for (unsigned int i = 0; i < p_originalInput.size(); i++) {
        p_finalOutput.push_back(FileName(p_originalInput[i]).name());
      }

      p_outputListNeedsModifiers = true;
    }
  }


  /**
   * This method is used to set an output list file. Basically, this means the
   * output filenames are specified in the list file.
   *
   * @param outputFileNameList List file with output cube names
   */
  void Pipeline::SetOutputListFile(const FileName &outputFileNameList) {
    p_finalOutput.clear();

    FileList filelist(outputFileNameList.expanded());
    QString filename;

    while (!filelist.isEmpty()) {
      filename = filelist.takeFirst().expanded();
      p_finalOutput.push_back(filename);
    }

    p_outputListNeedsModifiers = false;
  }


  /**
   * Set whether or not to keep temporary files (files generated in the middle of
   * the pipeline that are neither the original input nor the final output).
   *
   * @param keep True means don't delete, false means delete.
   */
  void Pipeline::KeepTemporaryFiles(bool keep) {
    p_keepTemporary = keep;
  }


  /**
   * Add a pause to the pipeline.
   *
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Original version
   */
  void Pipeline::AddPause() {
    // Add the pause
    QString pauseAppId = "";
    PipelineApplication *pauseApp = NULL;

    p_apps.push_back(pauseApp);
    p_appIdentifiers.push_back(pauseAppId);
  }


  /**
   * Add a new program to the pipeline. This method must be called before calling
   * Pipeline::Application(...) to access it. The string identifier will access
   * this program.
   *
   * @param appname The name of the new application
   * @param identifier The program's identifier for when calling
   *                   Pipeline::Application
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  void Pipeline::AddToPipeline(const QString &appname, const QString &identifier) {
    // Check uniqueness first
    for (unsigned int appIdentifier = 0; appIdentifier < p_appIdentifiers.size(); appIdentifier++) {
      if (p_appIdentifiers[appIdentifier] == identifier) {
        QString message = "The application identifier [" + identifier + "] is not unique. " +
                          "Please providing a unique identifier";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
    }

    // If we've got cubeatt on our list of applications for band eliminating, take it away temporarily
    PipelineApplication *cubeAtt = NULL;
    QString cubeAttId = "";
    if (p_addedCubeatt) {
      cubeAtt = p_apps[p_apps.size()-1];
      cubeAttId = p_appIdentifiers[p_appIdentifiers.size()-1];
      p_apps.resize(p_apps.size() - 1);
      p_appIdentifiers.resize(p_appIdentifiers.size() - 1);
      p_apps[p_apps.size()-1]->SetNext(NULL);
    }

    //Check for non nulls instead of size ie. find last non null or use this
    QString pauseAppId = "";

    for (int iapp = 0; iapp < (int) p_apps.size(); iapp++)

    // Add the new application
    if (p_apps.size() == 0) {
      p_apps.push_back(new PipelineApplication(appname, this));
    }
    else {
      if (p_apps[p_apps.size()-1] != NULL)
        p_apps.push_back(new PipelineApplication(appname, p_apps[p_apps.size()-1]));
      else // We know we have app NULL before this new app
        p_apps.push_back(new PipelineApplication(appname, p_apps[p_apps.size()-2]));
    }

    p_appIdentifiers.push_back(identifier);

    // If we have stretch, put it back where it belongs
    if (cubeAtt) {
      p_apps[p_apps.size()-1]->SetNext(cubeAtt);
      cubeAtt->SetPrevious(p_apps[p_apps.size()-1]);
      p_apps.push_back(cubeAtt);
      p_appIdentifiers.push_back(cubeAttId);
    }
  }


  /**
   * Add a new program to the pipeline. This method must be called before calling
   * Pipeline::Application(...) to access it. The string used to access this
   * program will be the program's name.
   *
   * @param appname The name of the new application
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  void Pipeline::AddToPipeline(const QString &appname) {
    // Check uniqueness first
    for (unsigned int appIdentifier = 0; appIdentifier < p_appIdentifiers.size(); appIdentifier++) {
      if (p_appIdentifiers[appIdentifier] == appname) {
        QString message = "The application identifier [" + appname + "] is not unique. Please use " +
                          "the other AddToPipeline method providing a unique identifier";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
    }

    // If we've got cubeatt on our list of applications for band eliminating, take it away temporarily
    PipelineApplication *cubeAtt = NULL;
    QString cubeAttId = "";
    if (p_addedCubeatt) {
      cubeAtt = p_apps[p_apps.size()-1];
      cubeAttId = p_appIdentifiers[p_appIdentifiers.size()-1];
      p_apps.resize(p_apps.size() - 1);
      p_appIdentifiers.resize(p_appIdentifiers.size() - 1);
      p_apps[p_apps.size()-1]->SetNext(NULL);
    }

    // Add the new application
    if (p_apps.size() == 0) {
      p_apps.push_back(new PipelineApplication(appname, this));
    }
    else {
      if (p_apps[p_apps.size()-1] != NULL)
        p_apps.push_back(new PipelineApplication(appname, p_apps[p_apps.size()-1]));
      else // We know we have app NULL before this new app
        p_apps.push_back(new PipelineApplication(appname, p_apps[p_apps.size()-2]));
    }

    p_appIdentifiers.push_back(appname);

    // If we have stretch, put it back where it belongs
    if (cubeAtt) {
      p_apps[p_apps.size()-1]->SetNext(cubeAtt);
      cubeAtt->SetPrevious(p_apps[p_apps.size()-1]);
      p_apps.push_back(cubeAtt);
      p_appIdentifiers.push_back(cubeAttId);
    }
  }


  /**
   * This is an accessor to get a specific PipelineApplication. This is the
   * recommended accessor.
   *
   * @param identifier The identifier (usually name) of the application to access,
   *                such as "spiceinit"
   *
   * @return PipelineApplication& The application's representation in the pipeline
   */
  PipelineApplication &Pipeline::Application(const QString &identifier) {
    int index = 0;
    bool found = false;

    while (!found && index < Size()) {
      if (p_appIdentifiers[index] == identifier) {
        found = true;
      }
      else {
        index ++;
      }
    }

    if (!found) {
      QString msg = "Application identified by [" + identifier + "] has not been added to the pipeline";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *p_apps[index];
  }


  /**
   * This is an accessor to get a specific PipelineApplication. This accessor is
   * mainly in place for the output operator; it is not recommended.
   *
   * @param index The index of the pipeline application
   *
   * @return PipelineApplication& The pipeline application
   */
  PipelineApplication &Pipeline::Application(const int &index) {
    if (index > Size()) {
      QString msg = "Index [" + QString(index) + "] out of bounds";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *p_apps[index];
  }


  /**
   * This method disables all applications up to this one. Effectively, the
   * application specified becomes the first application. This is not a guarantee
   * that the application specified is enabled, it only guarantees no applications
   * before it will be run.
   *
   * @param appname The program to start with
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  void Pipeline::SetFirstApplication(const QString &appname) {
    int appIndex = 0;
    for (appIndex = 0; appIndex < (int)p_apps.size() &&
                      p_apps[appIndex]->Name() != appname; appIndex++) {
      if (p_apps[appIndex] == NULL) continue;
      p_apps[appIndex]->Disable();
    }
    // for (appIndex = 0; appIndex < (int)p_apps.size() && p_apps[appIndex]->Name() != appname; appIndex++) {
    //   p_apps[appIndex]->Disable();
    // }

    if (appIndex >= (int)p_apps.size()) {
      QString msg = "Pipeline could not find application [" + appname + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method disables all applications after to this one. Effectively, the
   * application specified becomes the last application. This is not a guarantee
   * that the application specified is enabled, it only guarantees no applications
   * after it will be run.
   *
   * @param appname The program to end with
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  void Pipeline::SetLastApplication(const QString &appname) {
    int appIndex = p_apps.size() - 1;
    for (appIndex = p_apps.size() - 1; appIndex >= 0 && p_apps[appIndex]->Name() != appname; appIndex --) {
      if (p_apps[appIndex] == NULL) continue;
      p_apps[appIndex]->Disable();
    }

    if (appIndex < 0) {
      QString msg = "Pipeline could not find application [" + appname + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This gets the final output for the specified branch; this is necessary for
   * the PipelineApplications to calculate the correct outputs to use.
   *
   * @param branch Branch index to get the final output for
   * @param addModifiers Whether or not to add the last name modifier
   *
   * @return QString The final output string
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  QString Pipeline::FinalOutput(int branch, bool addModifiers) {
    QString output = ((p_finalOutput.size() != 0) ? p_finalOutput[0] : "");

    if (p_apps.size() == 0) return output;

    if (p_finalOutput.size() > 1) {
      if ((unsigned int)branch >= p_finalOutput.size()) {
        QString msg = "Output not set for branch [" + QString(branch) + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if (!p_outputListNeedsModifiers) {
        return p_finalOutput[branch];
      }
      else {
        output = p_finalOutput[branch];
        addModifiers = true;
      }
    }

    PipelineApplication *last = p_apps[p_apps.size()-1];
    if (last == NULL) last = p_apps[p_apps.size()-2];
    if (!last->Enabled()) last = last->Previous();

    if (output == "" || p_finalOutput.size() > 1) {
      if (output == "") {
        output = "./" + FileName(p_originalInput[0]).baseName();
      }
      else {
        output = "./" + FileName(p_originalInput[branch]).baseName();
      }

      // Base filename off of first input file
      if (!addModifiers || last->OutputBranches().size() == 1) {
        if (addModifiers && p_finalOutput.size() > 1)
          output += "." + last->OutputNameModifier();

        output += "." + last->OutputExtension();
      }
      else {
        // If we have multiple final outputs, rely on them to
        //   differentiate the branches
        if (p_finalOutput.size() <= 1) {
          output += "." + last->OutputBranches()[branch];
        }

        if (addModifiers && p_finalOutput.size() > 1)
          output += "." + last->OutputNameModifier();

        output += "." + last->OutputExtension();
      }
    }
    else if (addModifiers) {
      PipelineApplication *last = p_apps[p_apps.size()-1];
      if (!last->Enabled()) last = last->Previous();

      output = FileName(p_finalOutput[0]).path() + "/" +
               FileName(p_finalOutput[0]).baseName() + "." +
               last->OutputBranches()[branch] + ".";

      if (p_finalOutput.size() > 1) {
        output += last->OutputNameModifier() + ".";
      }

      output += last->OutputExtension();
    }

    return output;
  }


  /**
   * This method returns the user's temporary folder for temporary files. It's
   * simply a conveinient accessor to the user's preferences.
   *
   * @return QString The temporary folder
   */
  QString Pipeline::TemporaryFolder() {
    Pvl &pref = Preference::Preferences();
    return pref.findGroup("DataDirectory")["Temporary"];
  }


  /**
   * This method re-enables all applications. This resets the effects of
   * PipelineApplication::Disable, SetFirstApplication and SetLastApplication.
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  void Pipeline::EnableAllApplications() {
    for (int i = 0; i < Size(); i++) {
      if (p_apps[i] != NULL) p_apps[i]->Enable();
    }
  }


  /**
   * This is the output operator for a Pipeline, which enables things such as:
   * @code
   *   Pipeline p;
   *   cout <<  p << endl;
   * @endcode
   *
   * @param os The output stream (usually cout)
   * @param pipeline The pipeline object to output
   *
   * @return ostream& The modified output stream
   *
   * @internal
   *   @history 2011-08-15 Debbie A. Cook Added check for NULL pointers in p_apps
   *
   */
  ostream &operator<<(ostream &os, Pipeline &pipeline) {
    pipeline.Prepare();

    if (!pipeline.Name().isEmpty()) {
      os << "PIPELINE -------> " << pipeline.Name() << " <------- PIPELINE" << endl;
    }

    for (int i = 0; i < pipeline.Size(); i++) {
      if (pipeline.Application(i).Enabled()) {
        const vector<QString> &params = pipeline.Application(i).ParamString();
        for (int j = 0; j < (int)params.size(); j++) {
          QString special(params[j].mid(0, 7));
          if (special == ">>LIST ") {
            QString cmd = params[j].mid(7);

            QStringList listFileData = cmd.split(" ");
            QString file = listFileData.takeFirst();
            os << "echo -e \"" << listFileData.join("\\n") << "\" > " << file << endl;
          }
          else {
            os << pipeline.Application(i).Name() << " " << params[j] << endl;
          }
        }
      }
    }

    if (!pipeline.KeepTemporaryFiles()) {
      for (int i = 0; i < pipeline.Size(); i++) {
        if (pipeline.Application(i).Enabled()) {
          vector<QString> tmpFiles = pipeline.Application(i).TemporaryFiles();
          for (int file = 0; file < (int)tmpFiles.size(); file++) {
            if (!tmpFiles[file].contains("blank")) {
              os << "rm " << tmpFiles[file] << endl;
            }
          }
        }
      }
    }

    if (!pipeline.Name().isEmpty()) {
      os << "PIPELINE -------> " << pipeline.Name() << " <------- PIPELINE" << endl;
    }

    return os;
  }
}
