/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "PipelineApplication.h"
#include "Pipeline.h"
#include "IException.h"
#include "Application.h"

using namespace Isis;
using namespace std;

namespace Isis {
  /**
   * Constructs the first pipeline application.
   *
   * @param appName The name of this application
   * @param pipe The pipeline
   * @history 2010-12-20 Sharmila Prasad Initialise & enable the branches
   */
  PipelineApplication::PipelineApplication(QString appName, Pipeline *pipe) {
    p_name = appName;
    p_enabled = true;
    p_previous = NULL;
    p_next = NULL;
    p_pipeline = pipe;
    p_continue = false;

    if(p_pipeline->OriginalBranches().size() == 1) {
      p_inBranches.push_back("");
      p_outBranches.push_back("");
      p_enableBranch.push_back(true);
    }
    else {
      p_inBranches = p_pipeline->OriginalBranches();
      p_outBranches = p_pipeline->OriginalBranches();
      for(int i=0; i<(int)p_outBranches.size(); i++) {
        p_enableBranch.push_back(true);
      }
    }
  };


  /**
   * Constructs subsequent pipeline applications
   *
   * @param appName The name of this application
   * @param previous The previously last pipeline application
   * @history 2010-12-20 Sharmila Prasad Initialise & enable the branches
   */
  PipelineApplication::PipelineApplication(QString appName, PipelineApplication *previous) {
    p_name = appName;
    p_enabled = true;
    p_previous = previous;
    p_next = NULL;
    p_pipeline = p_previous->GetPipeline();

    p_inBranches   = p_previous->OutputBranches();
    p_outBranches  = p_previous->OutputBranches();
    for(int i=0; i<(int)p_outBranches.size(); i++) {
      p_enableBranch.push_back(true);
    }
    p_previous->SetNext(this);
  }


  /**
   * Set the input parameter for this application and whether or not this
   * application supports the virtual bands functionality. It supports the virtual
   * bands functionality if the input is an Isis cube.
   *
   * @param inputParamName Name of the input parameter, typically "FROM"
   * @param supportsVirtualBands True if this application supports virtual bands
   */
  void PipelineApplication::SetInputParameter(const QString &inputParamName, bool supportsVirtualBands) {
    p_input.clear();
    p_input.push_back(PipelineParameter(inputParamName));
    p_supportsVirtualBands = supportsVirtualBands;
  }


  /**
   * Set the input parameter for this application and whether or not this
   * application supports the virtual bands functionality. It supports the virtual
   * bands functionality if the input is an Isis cube.
   *
   * @param inputParamName Name of the input parameter, typically "FROM"
   * @param value Custom parameter value; Recommended to use an alternate
   *              SetInputParameter instead of specifying LastOutput
   * @param supportsVirtualBands True if this application supports virtual bands
   */
  void PipelineApplication::SetInputParameter(const QString &inputParamName, CustomParameterValue value, bool supportsVirtualBands) {
    if(value == LastAppOutputList) {
      // Merge
      p_outBranches.clear();
      p_outBranches.push_back("");
    }
    else if(value == LastAppOutputListNoMerge) {
      // No merge
      value = LastAppOutputList;
    }

    p_input.push_back(PipelineParameter(inputParamName, value));
    p_supportsVirtualBands = supportsVirtualBands;
  }


  /**
   * Set the output parameter for a branch of this application and it's naming
   * convention. This is meant for an application that splits the input into two
   * output files via two output parameters.
   *
   * @param branch Branch this output parameter applies to
   * @param outputParamName Name of the output parameter
   * @param outNameModifier Modifier to add to the cube name, such as "lev1"
   * @param outFileExtension Extension of the output file (usually "cub" for cube)
   */
  void PipelineApplication::SetOutputParameter(const QString &branch, const QString &outputParamName,
      const QString &outNameModifier, const QString &outFileExtension) {
    p_output.push_back(PipelineParameter(FindBranch(branch, false), outputParamName));
    p_outputMod = outNameModifier;
    p_outputExtension = outFileExtension;
  }


  /**
   * Set the output parameter for this application and it's naming convention.
   *
   * @param outputParamName Name of the output parameter
   * @param outNameModifier Modifier to add to the cube name, such as "lev1"
   * @param outFileExtension Extension of the output file (usually "cub" for cube)
   */
  void PipelineApplication::SetOutputParameter(const QString &outputParamName, const QString &outNameModifier, const QString &outFileExtension) {
    p_output.clear();
    p_output.push_back(PipelineParameter(outputParamName));
    p_outputMod = outNameModifier;
    p_outputExtension = outFileExtension;
  }


  /**
   * This method adds branch to this program. A branch means with one input,
   * multiple outputs are automatically created.
   *
   * Example:
   * @code
   *   thm2isis from=input.img to=output.cub
   * @endcode
   *
   * In this code, thm2isis could actually create output.even.cub and
   * output.odd.cub. The branches are then "even" and "odd". This is used only for
   * the case where the program outputs multiple images based upon one "TO"
   * parameter.
   *
   * @param modString Branch name
   * @param type Modifier type; currently only supports constant strings
   * @history 2010-12-20 Sharmila Prasad Enable the branch at the time of creation
   */
  void PipelineApplication::AddBranch(const QString &modString, NameModifierType type) {
    if(modString == "") {
      string msg = "Can not add empty branch to pipeline";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    //if(p_inBranches.size() != 1 || p_inBranches[0] != "") {
    //  string msg = "Can not branch an already branched pipeline";
    //  throw iException::Message(iException::Programmer, msg, _FILEINFO_);
    //}

    if(p_outBranches[0] == "") {
      p_outBranches.clear();
      p_enableBranch.clear();
    }

    if(p_inBranches.size() == 1) {
      p_outBranches.push_back(modString);
      p_enableBranch.push_back(true);
    }
    else if(p_inBranches.size() == p_outBranches.size()) {
      for(int outBranch = p_outBranches.size() - 1; outBranch >= 0; outBranch --) {
        if(p_inBranches[outBranch] == p_outBranches[outBranch]) {
          p_outBranches[outBranch] = p_inBranches[outBranch] + "." + modString;
        }
        else {
          p_outBranches.push_back(p_inBranches[outBranch] + "." + modString);
          p_enableBranch.push_back(true);
        }
      }
    }
    else {
      for(unsigned int inBranch = 0; inBranch < p_inBranches.size(); inBranch ++) {
        p_outBranches.push_back(p_inBranches[inBranch] + "." + modString);
        p_enableBranch.push_back(true);
      }
    }
  }


  /**
   * This method adds knowledge of a parameter to the application. The
   * parameter value is taken directly from the user interface.
   *
   * @param inputParamName Parameter in the proc program
   * @param appParamName Parameter in this application
   */
  void PipelineApplication::AddParameter(const QString &inputParamName, const QString &appParamName) {
    UserInterface &ui = Application::GetUserInterface();

    if(ui.WasEntered(inputParamName)) {
      p_params.push_back(PipelineParameter(appParamName, ui.GetAsString(inputParamName)));
    }
  }


  /**
   * This method adds knowledge of a parameter to this application, that will only
   * affect the specified branch. The parameter value is taken directly from the
   * user interface; internal defaults are supported by not using the parameter.
   *
   * @param branch The branch this parameter affects
   * @param inputParamName Parameter in the proc program
   * @param appParamName Parameter in the this application
   */
  void PipelineApplication::AddParameter(const QString &branch, const QString &inputParamName, const QString &appParamName) {
    UserInterface &ui = Application::GetUserInterface();

    if(ui.WasEntered(inputParamName)) {
      p_params.push_back(PipelineParameter(FindBranch(branch, false), appParamName, ui.GetAsString(inputParamName)));
    }
  }


  /**
   * This method adds a parameter to this application with a known value (does not
   * get it from the user interface, must be specified).
   *
   * @param appParamName Name of the parameter
   * @param appParamValue Value of the parameter
   */
  void PipelineApplication::AddConstParameter(const QString &appParamName, const QString &appParamValue) {
    bool added = false;

    for(unsigned int i = 0; !added && i < p_params.size(); i++) {
      if(p_params[i].Name() == appParamName) {
        p_params[i] = PipelineParameter(appParamName, appParamValue);
        added = true;
      }
    }

    if(!added) {
      p_params.push_back(PipelineParameter(appParamName, appParamValue));
      added = true;
    }
  }


  /**
   * This method adds a parameter to this application with a known value (does not
   * get it from the user interface, must be specified) that only affects a single
   * branch.
   *
   * @param branch Branch this parameter affects
   * @param appParamName Name of the parameter
   * @param appParamValue Value of the parameter
   */
  void PipelineApplication::AddConstParameter(const QString &branch, const QString &appParamName, const QString &appParamValue) {
    p_params.push_back(PipelineParameter(FindBranch(branch, false), appParamName, appParamValue));
  }


  /**
   * This method adds a parameter with a calculated value (special) to this
   * application.
   *
   * @param appParamName Parameter name
   * @param value Value type
   */
  void PipelineApplication::AddParameter(const QString &appParamName, CustomParameterValue value) {
    p_params.push_back(PipelineParameter(appParamName, value));
  }


  /**
   * This method adds a parameter with a calculated value (special) to this
   * application that only affects the specified branch
   *
   * @param branch Branch this parameter affects
   * @param appParamName Parameter name
   * @param value Value type
   */
  void PipelineApplication::AddParameter(const QString &branch, const QString &appParamName, CustomParameterValue value) {
    p_params.push_back(PipelineParameter(FindBranch(branch, false), appParamName, value));
  }


  /**
   * This method calculates the inputs, outputs and necessary calls to this
   * program for the pipeline. This should only be used by Pipeline.
   *
   * @history 2010-12-20 Sharmila Prasad Set appropriate inputs to the branch taking
   *          into consideration whether the previous branch was enabled/disabled. Also
   *          set the input list accordingly if needlist is true
   */
  void PipelineApplication::BuildParamString() {
    p_paramString.clear();
    p_outputs.clear();
    p_tempFiles.clear();

    if(!Enabled()) return;

    // These are used if the pipeline needs a list file; must be out here in case multiple branches use
    //   the list file.
    bool needList = false;
    QString listFile;

    bool runOnce = Merges() && !Branches();

    /*cerr << "***BuildParamString App " << p_name << " RunOnce="<< runOnce << "***\nInput Branches Size=" << p_inBranches.size() << endl;
    for(int i=0; i<(int)p_inBranches.size(); i++) {
      cerr << "Branch" << i << " = " << p_inBranches[i] << endl;
    }

    cerr << "Output Branches Size=" << p_outBranches.size() << endl;
    for(int i=0; i<(int)p_outBranches.size(); i++) {
      cerr << "Branch" << i << " = " << p_outBranches[i] << " Enabled=" << p_enableBranch[i] << endl;
    }*/

    // Make sure we have different inputs for different runs...
    if(!runOnce && p_input.size() == 1) {
      PipelineParameter &inputParam = p_input[0];
      if(inputParam.IsSpecial() && inputParam.Special() == LastAppOutputList) {
        runOnce = true;

        for(int param = 0; param < (int)p_params.size() && runOnce; param++) {
          runOnce = (p_params[param].IsSpecial() && p_params[param].Special() == LastAppOutputList) ||
                    (!p_params[param].IsSpecial() && p_params[param].AffectsAllBranches());
        }
      }
    }

    // We need to build execute calls for all of the branches
    for(int branch = 0; branch < (int)p_inBranches.size(); branch ++) {
      if(runOnce && branch > 0) {
        return;
      }

      if(!BranchEnabled(branch)) {
        QString tmpBranch(branch);
        p_outputs.push_back(p_name + "." +tmpBranch + ".blank");
        continue;
      }


      // Figure out the input file; could throw an exception if the user didnt set it
      QString inputFile = CalculateInputFile(branch);
      // Figure out the output file; This adds the output to the output list*
      QString outputFile = CalculateOutputFile(branch);
      // This parameter gives us more detail about the input parameter
      PipelineParameter &inputParam = GetInputParameter(branch);

      QString params = "";

      // If we havent needed a list yet, let's see if we need one now
      if(!needList) {
        // We need to know if we need a list file ahead of time; look at input and parameters
        needList = (inputParam.IsSpecial() && inputParam.Special() == LastAppOutputList);

        for(int param = 0; param < (int)p_params.size() && !needList; param++) {
          needList = (p_params[param].IsSpecial() && p_params[param].Special() == LastAppOutputList);
        }

        // If we need a list file, create a parameter that starts with ">>LIST" to say it's a list file.
        //   The first element is the list file, the rest is the contents of the list file.
        if(needList) {
          QString listName = outputFile;

          if(listName.isEmpty()) {
            // This might have to become more robust in the future, we
            //   have an input list but no outputs to the program for
            //   this case. This is the naming of the list.
            listName = Name();
          }

          QString input = p_pipeline->TemporaryFolder() + "/" + QString::fromStdString(FileName(listName.toStdString()).baseName()) + ".lis";
          params = ">>LIST " + input + " ";

          PipelineApplication * prev = Previous();
          int infile = 0;
          while(prev != NULL && infile < (int)p_inBranches.size()) {
            for(int i = 0; i < (int)prev->OutputBranches().size(); i++) {
              if(prev->BranchEnabled(i)) {
                params += " " + prev->GetOutputs()[i];
                infile++;
              }
            }
            prev = prev->Previous();
          }

          p_tempFiles.push_back(input);
          p_paramString.push_back(params);
          params = "";
          listFile = input;
        }
      }

      // If the input is a list file, set it to the list file
      if(inputParam.IsSpecial() && inputParam.Special() == LastAppOutputList) {
        params = GetInputParameter(branch).Name() + "=\"" + listFile + "\"";
      }
      else {
        // Otherwise it's the input file. Try to add virtual bands.
        //   Pipeline will set p_virtualBands to an empty string if we are to not apply it.
        params = GetInputParameter(branch).Name() + "=\"" + inputFile;
        if(p_virtualBands.size() == 1) {
          params += "+" + p_virtualBands[0];
        }
        else if(p_virtualBands.size() == p_inBranches.size() && !p_virtualBands[branch].isEmpty()) {
          params += "+" + p_virtualBands[branch];
        }

        params += "\"";
      }

      // If we have output, add it on to our parameters
      if(p_output.size() != 0) {
        if(Branches() && p_output.size() != 1) {
          // Set output variable for every branch
          for(unsigned int outBranch = 0; outBranch < p_outBranches.size(); outBranch ++) {
            // Each branch should have at least 1 output file
            bool outputSet = false;

            // ... unless there is a split, look for the split (check name start)
            if(p_inBranches.size() > 1) {
              if(!StringStartsWith(p_outBranches[outBranch], p_inBranches[branch])) {
                continue;
              }
            }

            // Match the output variable with it's param name
            for(unsigned int outParam = 0; outParam < p_output.size(); outParam++) {
              if(p_output[outParam].AppliesToBranch(outBranch)) {
                params += " " + p_output[outParam].Name() + "=\"" + p_outputs[outBranch] + "\"";

                if(outputSet) {
                  std::string message = "Application [" + Name().toStdString() + "] in the pipeline branches with an ";
                  message += "output parameter for each branch, but branch [" + p_outBranches[outBranch].toStdString();
                  message += "] has multiple output files specified.";
                  throw IException(IException::Programmer, message, _FILEINFO_);
                }

                outputSet = true;
              }
            }

            if(!outputSet) {
              std::string message = "Application [" + Name().toStdString() + "] in the pipeline branches with an ";
              message += "output parameter for each branch, but branch [" + p_outBranches[outBranch].toStdString();
              message += "] has no output files specified.";
              throw IException(IException::Programmer, message, _FILEINFO_);
            }
          }
        }
        else {
          bool foundBranch = false;

          // Find the output parameter name that is acceptable. If we cant find an output branch, then there isnt one!
          for(unsigned int outputParam = 0; outputParam < p_output.size(); outputParam++) {
            if(p_output[outputParam].AppliesToBranch(branch)) {
              params += " " + p_output[0].Name() + "=\"" + outputFile + "\"";
              foundBranch = true;
            }
          }

          if(!foundBranch) continue;
        }
      }

      // Add the remaining parameters
      for(int i = 0; i < (int)p_params.size() && i < (int)p_params.size(); i++) {
        if(p_params[i].AppliesToBranch(branch)) {
          if(!p_params[i].IsSpecial()) {
            params += " " + p_params[i].Name() + "=\"" + p_params[i].Value() + "\"";
          }
          else if(p_params[i].Special() == LastOutput) {
            params += " " + p_params[i].Name() + "=\"" + GetRealLastOutput(true) + "\"";
          }
          else if(p_params[i].Special() == LastAppOutputList) {
            params += " " +  p_params[i].Name() + "=\"" + listFile + "\"";
          }
        }
      }

      if(inputFile.isEmpty()) {
        std::string message = "There was a problem with calculating the inputs for program [" + Name().toStdString();
        message += "]. Please verify your program is not setting outputs for branches that ";
        message += "don't have input.";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }

      // Remember this parameter string
      p_paramString.push_back(params);
    }
  }


  /**
   * This method calculates the input file for the specified branch.
   *
   * @param branch Branch this input file affects
   *
   * @return QString Input filename
   * @history 2010-12-20 Sharmila Prasad Get the right input for this branch if previous
   *          branch is disabled.
   */
  QString PipelineApplication::CalculateInputFile(int branch) {
    QString file = "";

    PipelineApplication *prev = Previous();

    if(prev != NULL) {
      // The last app exists, look for output on this branch
      if(branch < (int)prev->GetOutputs().size() && prev->BranchEnabled(branch)) {
        file = prev->GetOutputs()[branch];
      }
      else {
        while (prev != NULL && file=="") {
          if(prev->Branches()){
             if (!prev->BranchEnabled(branch)) {
               string msg = "Application branches but branch is disabled";
               throw IException(IException::Programmer, msg, _FILEINFO_);
             }
          }
          /*   else {
               vector<QString> inputs = prev->GetInputs();
               if((int)inputs.size() > branch) {
                 file = inputs[branch];
               }
               else {
                 file = inputs[0];
               }
               break;
             }
          } */
          //else {
            if(prev->BranchEnabled(branch)) {
              if(branch < (int)prev->GetOutputs().size()) {
                file = prev->GetOutputs()[branch];
                break;
              }
            }
         // }
          prev = prev->Previous();
        }
      }
    }

    // We're either the first program, or nothing has generated output yet.
    if(file.isEmpty()){
      file = p_pipeline->OriginalInput(branch);
      if (file.isEmpty()) {
        int index = branch / (p_pipeline->OriginalBranchesSize() / p_pipeline->OriginalInputSize());
        file = p_pipeline->OriginalInput(index);
      }
    }

    // deal with special cases
    for(int i = 0; i < (int)p_input.size(); i++) {
      if(p_input[i].AppliesToBranch(branch) && p_input[i].IsSpecial()) {
        if(p_input[i].Special() == LastOutput) {
          file = GetRealLastOutput();
          break;
        }
      }
    }
    return file;
  }


  /**
   * This method calculates the output file for the specified branch
   *
   * @param branch Branch this output file is for
   *
   * @return QString The output file
   */
  QString PipelineApplication::CalculateOutputFile(int branch) {
    QString outputFile;
    QString outFolder = p_pipeline->TemporaryFolder();

    // We need to know this to know if we actually need to add modifiers to the
    //   output name
    bool usedBranch = false;
    unsigned int usedBranchIndex = 0;
    unsigned int numUsedBranches = 0;

    for(unsigned int outputBranch = 0; outputBranch < p_outBranches.size(); outputBranch++) {
      bool outBranchUsed = false;

      for(unsigned int outputParam = 0; outputParam < p_output.size(); outputParam ++) {
        if(p_output[outputParam].AppliesToBranch(outputBranch)) {
          outBranchUsed = true;
        }
      }

      if(outBranchUsed) {
        if(outputBranch < (unsigned int)branch) {
          usedBranchIndex ++;
        }

        if((unsigned int)branch == outputBranch) {
          usedBranch = true;
        }

        numUsedBranches ++;
      }
    }

    if(!usedBranch) return "";

    if(!LastApplicationWithOutput()) {
      QString lastOutput = p_pipeline->FinalOutput(branch, false);
      outputFile = outFolder + "/" +
                   QString::fromStdString(FileName(lastOutput.toStdString()).baseName()) + "." + p_outputMod + "." + p_outputExtension;

      if(p_outputMod.isEmpty()) {
        outputFile = outFolder + "/" +
                     QString::fromStdString(FileName(lastOutput.toStdString()).baseName()) + "." + p_outputExtension;
      }
    }
    else {
      outputFile = p_pipeline->FinalOutput(branch, numUsedBranches > 1);
      outFolder = QString::fromStdString(FileName(outputFile.toStdString()).path());
    }

    if(!LastApplicationWithOutput() && numUsedBranches != 1 && !p_outputMod.isEmpty()) {
      FileName outfile(outputFile.toStdString());

      QString realOut(outFolder + "/" + QString::fromStdString(outfile.baseName()) + "." + p_outBranches[branch] + "." + p_outputExtension);

      if(usedBranch) {

        // This assumes CalculateOutputFile is called in order (branch 0,1,2...n)
        if(p_outputs.size() == usedBranchIndex) {
          p_outputs.push_back(realOut);
        }

        // If we're only run once, but we branch, we need to store the rest of these real output files...
        //   recursively call to get them stored away. Also we could be run twice, but really have 4 outputs,
        //   so always do this if we branch.
        if(branch == 0 && Branches()) {
          for(unsigned int i = 1; i < OutputBranches().size(); i++) {
            CalculateOutputFile(i);
          }
        }

        // If branches is false, then we need to tell the truth about the output file.
        //   REASONING: thm2isis needs to be lied to (Branches() == true), for example,
        //   because it modifies output names on its own.
        if(!Branches()) {
          // tell the truth
          outputFile = realOut;
        }
      }
    }
    else if(!p_outputMod.isEmpty()) {
      if(p_outputs.size() == usedBranchIndex) {
        p_outputs.push_back(outputFile);
      }

      // If we're only run once, but we branch, we need to store the rest of these real output files...
      //   recursively call to get them stored away. Also we could be run twice, but really have 4 outputs,
      //   so always do this if we branch.
      if(branch == 0 && Branches()) {
        for(unsigned int i = 1; i < OutputBranches().size(); i++) {
          CalculateOutputFile(i);
        }
      }
    }

    return outputFile;
  }


  /**
   * Returns true if this is the last application with output
   *
   *
   * @return bool False if another application later on creates output
   */
  bool PipelineApplication::LastApplicationWithOutput() {
    if(!Next() && !p_output.empty()) {
      return true;
    }
    if(!Next() && p_output.empty()) {
      return false;
    }

    // If any future app creates output, then I'm not last
    return !Next()->FutureOutputFileCreated();
  }


  /**
   * Returns true if a future application creates output
   *
   *
   * @return bool Future application creates output
   */
  bool PipelineApplication::FutureOutputFileCreated() {
    if(!p_output.empty()) {
      return true;
    }

    if(!Next() && p_output.empty()) {
      return false;
    }

    return Next()->FutureOutputFileCreated();
  }


  /**
   * This gets the input parameter for the specified branch
   *
   * @param branch Branch the input parameter is for
   *
   * @return PipelineParameter& The input parameter
   */
  PipelineParameter &PipelineApplication::GetInputParameter(int branch) {
    for(int i = 0; i < (int)p_input.size(); i++) {
      if(p_input[i].AppliesToBranch(branch)) {
        return p_input[i];
      }
    }

    if(p_inBranches[0] != "") {
      std::string msg = "Application [" + Name().toStdString() + "] in the pipeline does not have an input for branch [" + p_inBranches[branch].toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else {
      std::string msg = "Application [" + Name().toStdString() + "] in the pipeline does not have an input";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * The method, given a string, finds the index of a branch
   *
   * @param name Branch name
   * @param input True if input branch, false if output branch
   *
   * @return int Branch index
   */
  int PipelineApplication::FindBranch(QString name, bool input) {
    int branchIndex = 0;
    bool found = false;

    if(input) {
      while(!found && branchIndex < (int)p_inBranches.size()) {
        if(p_inBranches[branchIndex] == name) {
          found = true;
        }
        else {
          branchIndex ++;
        }
      }
    }
    else {
      while(!found && branchIndex < (int)p_outBranches.size()) {
        if(p_outBranches[branchIndex] == name) {
          found = true;
        }
        else {
          branchIndex ++;
        }
      }
    }

    if(!found) {
      std::string msg = "Branch [" + name.toStdString() + "] does not exist in the pipeline application [" + Name().toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return branchIndex;
  }


  /**
   * This method returns a list of the temporary files generated by this program
   *
   *
   * @return vector<QString> The temporary files
   */
  vector<QString> PipelineApplication::TemporaryFiles() {
    vector<QString> tmp;

    if(!LastApplicationWithOutput()) {
      for(int i = 0; i < (int)p_outputs.size(); i++) {
        tmp.push_back(p_outputs[i]);
      }
    }

    for(int i = 0; i < (int)p_tempFiles.size(); i++) {
      tmp.push_back(p_tempFiles[i]);
    }

    return tmp;
  }


  /**
   * This method is used to calculate the value for
   * CustomParameterValue::LastOutput
   *
   * @param skipOne Skip the very last output; this is used to skip the output of
   *                the current run
   *
   * @return QString The last output file
   */
  QString PipelineApplication::GetRealLastOutput(bool skipOne) {
    if(!skipOne) {
      return GetOutputs()[GetOutputs().size()-1];
    }

    if(p_outputs.size() > 1) {
      return GetOutputs()[GetOutputs().size()-2];
    }

    return Previous()->GetOutputs()[Previous()->GetOutputs().size()-1];
  }


  /**
   * Returns true if virtual bands are supported
   *
   *
   * @return bool Virtual bands supported
   */
  bool PipelineApplication::SupportsVirtualBands() {
    if(!Enabled()) return false;
    return p_supportsVirtualBands;
  }


  /**
   * Set the virtual bands that this application is to apply. Empty for none.
   *
   * @param bands The virtual bands string, excluding the "+". For example,
   *              "2,4-5,8"
   */
  void PipelineApplication::SetVirtualBands(vector<QString> bands) {
    p_virtualBands = bands;
  }

  //! This returns this application's output files. Only valid after BuildParamString is called.
  vector<QString> &PipelineApplication::GetOutputs() {
    if(Enabled() && p_outputs.size() != 0) {
      return p_outputs;
    }
    else if(Previous()) {
      return Previous()->GetOutputs();
    }
    else {
      // no outputs yet... not sure if an exception should be thrown, sometimes
      //   things such as list files will fail when this returns nothing.
      return p_outputs;
    }
  }
}; // end namespace Isis
