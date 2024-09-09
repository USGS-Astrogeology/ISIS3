#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Progress.h"
#include "Application.h"
#include "Preference.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a Progress object.
   *
   * @throws Isis::iException::User
   */
  Progress::Progress() {
    // Get user preferences
    int percent;
    bool printPercent;
    Isis::PvlGroup &group = Isis::Preference::Preferences().findGroup("UserInterface");
    percent = group["ProgressBarPercent"];
    QString temp = QString::fromStdString(group["ProgressBar"]);
    printPercent = temp.toUpper() == "ON";

    // Check for an error
    if((percent != 1) && (percent != 2) &&
        (percent != 5) && (percent != 10)) {
      string m = "Invalid preference for [ProgressBarPercent] in ";
      m += "group [UserInterface] must be 1, 2, 5, or 10";
      throw IException(IException::User, m, _FILEINFO_);
    }

    // Initialize private variables
    p_text = "Working";
    p_maximumSteps = 0;
    p_currentStep = 0;
    p_currentPercent = 0;
    p_percentIncrement = percent;
    p_printPercent = printPercent;
    p_autoDisplay = true;
  }

  //! Destroys the Progress object
  Progress::~Progress() {
  }

  /**
   * Changes the value of the text string reported just before 0% processed. This
   * text is only output under the following condition. After SetMaximumSteps is
   * called, the first call to CheckStatus will immediately output the text. By
   * default this is set to "Working".
   *
   * @param text Text to output.
   */
  void Progress::SetText(const QString &text) {
    p_text = text;
  }

  /**
   * Returns the text to output. Generally, this in not needed except rare
   * circumstances where an application has multiple steps and the text string
   * needs to be saved and restored.
   *
   * @return string
   */
  QString Progress::Text() const {
    return p_text;
  }

  /**
   * This sets the maximum number of steps in the process. Whenever this is
   * invoked it also resets the counters to their initial states. This allows for
   * programs which have multiple steps.
   *
   * @param steps Maximum number of steps
   *
   * @throws IException::Programmer Invalid value for step (must be >0)
   */
  void Progress::SetMaximumSteps(const int steps) {
    if(steps < 0) {
      string m = "Value for [steps] must be greater than ";
      m += "or equal to zero in [Progress::SetMaximumSteps]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }
    p_maximumSteps = steps;
    p_currentStep = 0;
    p_currentPercent = 0;
  }

  /**
   * Checks and updates the status. The first time this is invoked, it outputs
   * the text from SetText and 0% processed. It should then be invoked for each
   * step, for example, a step could be considered processing a line for NL lines
   * in the image. If you do not call this enough times you will not reached 100%
   * processed.
   *
   * @throws IException::Programmer Step exceeds maximumSteps
   */
  void Progress::CheckStatus() {
    // Error check
    //std::cout << p_currentStep << " / " << p_maximumSteps << std::endl;
    if(p_currentStep > p_maximumSteps) {
      string m = "Step exceeds maximumSteps in [Progress::CheckStatus]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    if(p_currentStep == 0) {
      if(Isis::iApp != NULL) {
        if (p_autoDisplay) {
          Isis::iApp->UpdateProgress(p_text, p_printPercent);
        }
      }
      else {
        if(p_printPercent && p_autoDisplay) {
          cout << p_text.toStdString() << endl;
        }
      }
    }

    // See if the percent processed needs to be updated
    while(100.0 * p_currentStep / p_maximumSteps >= p_currentPercent) {
      if(Isis::iApp != NULL) {
        if (p_autoDisplay) {
          Isis::iApp->UpdateProgress(p_currentPercent, p_printPercent);
        }
      }
      else {
        if(p_printPercent && p_autoDisplay) {
          if(p_currentPercent < 100) {
            cout << p_currentPercent << "% Processed\r" << flush;
          }
          else {
            cout << p_currentPercent << "% Processed" << endl;
          }
        }
      }
      p_currentPercent += p_percentIncrement;
    }

    if(p_autoDisplay && Isis::iApp != NULL) {
      Isis::iApp->ProcessGuiEvents();
    }

    // Increment to the next step
    p_currentStep++;

    return;
  }


  /**
   * Turns off updating the Isis Gui when CheckStatus() is called. You must use
   *   RedrawProgress() to visually update the current progress.
   */
  void Progress::DisableAutomaticDisplay() {
    p_autoDisplay = false;
  }

  
  /**
   * Returns the maximum number of steps of the progress.
   *
   * @see SetMaximumSteps()
   * @return The maximum number of steps of the progress.
   */
  int Progress::MaximumSteps() const {
    return p_maximumSteps;
  }


  /**
   * Returns the current step of the progress. This value should always be in the range of:
   *   [0, MaximumSteps()]. CheckStatus() increments the current step.
   *
   * @see CheckStatus()
   *
   * @return The current step of the progress
   */
  int Progress::CurrentStep() const {
    return p_currentStep;
  }


  /**
   * If the initial step size was a guess, it can be modified using this method.
   * For example, if you SetMaximumSteps(11) then call AddSteps(1) then the new
   * MaximumSteps is 12. The progress bar will not go backwards (it will not drop
   * from 10% to 5%). "steps" can be negative to remove steps.
   *
   *
   * @param steps Amount to adjust the MaximumSteps by
   */
  void Progress::AddSteps(const int steps) {
    if(steps == 0) {
      string m = "Value for [steps] must not be zero in [Progress::AddSteps]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    p_maximumSteps += steps;

    if(p_currentStep > p_maximumSteps) {
      string m = "Step exceeds maximumSteps in [Progress::AddSteps]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    if(p_maximumSteps <= 0) {
      string m = "Maximum steps must be greater than zero in [Progress::AddSteps]";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }
  }
} // end namespace isis
