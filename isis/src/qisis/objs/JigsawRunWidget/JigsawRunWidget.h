#ifndef JigsawRunWidget_h
#define JigsawRunWidget_h

#include <QDialog>
#include <QDir>
#include <QDockWidget>
#include <QFrame>
#include <QMessageBox>
#include <QPointer>
#include <QWidget>

#include "BundleSettings.h"
#include "IException.h"

namespace Ui {
  class JigsawRunWidget;
}

class QString;
class QThread;

namespace Isis {
  class BundleAdjust;
  class BundleSolutionInfo;
  class Control;
  class Cube;
  class Directory;
  class FileName;
  class Project;

  /**
   * This dialog allows the user to select the bundle adjust parameters, run the bundle, and view
   * the results.
   *
   * @author 2014-??-?? Ken Edmundson
   *
   * @internal
   *   @history 2014-09-18 Kimberly Oyama - Added code to thread the bundle run. It is currently
   *                           commented out but it works.
   *   @history 2015-02-20 Jeannie Backer - Replaced BundleResults references with
   *                           BundleSolutionInfo and BundleStatistics references with BundleResults
   *                           due to class name changes.
   *   @history 2015-09-03 Jeannie Backer - Modified to create JigsawSetupDialog object using the
   *                           value for the useLastSettings checkbox. When the Run button is
   *                           clicked, the run time will now be used to create a uniquely named
   *                           directory to contain the output files for the bundle solution.
   *   @history 2017-04-17 Ian Humphrey - Added second constructor that can be used when the
   *                           JigsawWorkOrder initially creates a setup dialog so it can pass
   *                           information to this dialog. Added init() delegate method for
   *                           constructors to use to reduce code duplication. Modified
   *                           notifyThreadFinished to update the Run button. References #4748.
   *   @history 2017-04-18 Ian Humphrey - Added members for an Accept, Reject, and Close button
   *                           to determine when to save a successful bundle adjust's results
   *                           to the project (or when to discard the results). Added placeholder
   *                           private slots for accepting and rejecting the results. Removed
   *                           default OK and Cancel buttons from UI file. Fixes #4781.
   *   @history 2017-04-18 Tracie Sucharski - Write the updated control net to the runtime
   *                           folder under results folder.  Fixes #4783.
   *   @history 2017-04-25 Ian Humphrey - Updated the setup clicked slot to load the current
   *                           settings into the setup dialog if we are not using the last
   *                           (most recent) settings in the project. Fixes #4817.
   *   @history 2017-04-26 Ian Humphrey - Added updateScrollBar() and clearDialog() to reduce
   *                           code duplication. Modified the run clicked slot to clear the
   *                           dialog display anytime that a bundle adjust is re-ran. Fixes #4808.
   *   @history 2017-04-27 Ian Humphrey - Modified to track the last used control net to properly
   *                           update the jigsaw setup dialog's cnet combo box. References #4817.
   *   @history 2017-05-04 Ian Humphrey & Makayla Shepherd - Updated acceptBundleResults()
   *                           to concurrently save the bundled images (ecub's) to the project.
   *                           Fixes #4804, #4837.
   *   @history 2017-05-04 Ian Humphrey & Makayla Shepherd - Removed connection handling deleting the
   *                           bundle adjust later. This prevents a segfault from occuring when the
   *                           bundle adjust was accessed in the acceptBundleResults() slot, Since
   *                           its memory may have been deleted by then. Now manually managing the
   *                           memory for m_bundleAdjust. Fixes #4849.
   *   @history 2017-05-15 Tracie Sucharski - Commented out code in acceptBundleResults which was
   *                           not being used and causing compile warnings.  Add creation of
   *                           BundleSolutionInfo folder to the acceptBundleResults method.
   *   @history 2017-05-16 Tracie Sucharski - Disable Close button if jigsaw has been run, force
   *                           selection of accept or reject.  Once a choice has been made,
   *                           re-enable the Close button.
   *   @history 2017-06-14 Ken Edmundson - Write text summary file.
   *   @history 2017-07-27 Cole Neubauer - Added a project->setClean call if the bundle results are
   *                           accepted. Fixes #4960
   *   @history 2017-11-01 Ian Humphrey - Create ecubs in the bundle results directory which contain
   *                           updated SPICE.  Fixes #4804, #4849.
   *   @history 2018-03-22 Ken Edmundson - Added member variable QString m_outputControlName. Added
   *                           argument QString outputControlFileName to constructor. Modified
   *                           acceptBundleResults method to take output control network filename
   *                           from the JigsawSetupDialog.
   *   @history 2018-05-22 Ken Edmundson - Modified init() method to not set m_BundleSolutionInfo to
   *                           NULL because JigsawDialog no longer owns it. Modified destructor to
   *                           not delete m_BundleSolutionInfo or set it to NULL. Note this is NOT
   *                           ideal, m_BundleSolutionInfo should be a QSharedPointer, not a raw
   *                           pointer.
   *   @history 2018-05-31 Christopher Combs - Name changed from JigsawDialog to JigsawRunWidget.
   *                           Now inherits from QFrame instead of QDialog. Added support for new
   *                           workflow in which JigsawSetupDialog is only ever called from a
   *                           button on this widget. Fixes #5428. 
   *   @history 2018-06-14 Christopher Combs - Made changes according to new design mockup. Added 
   *                           status bar, control net info, and rms adj point sigmas sections. 
   *                           Removed buttons for close and reject. Now inherits from QDockWidget
   *                           instead of QFrame, and handles close event if a bundle is running. 
   *   @history 2018-06-15 Christopher Combs - Implemented "Write detached labels" checkbox. 
   *                           made changes to on_JigsawAcceptButton_clicked to reflect this.
   *   @history 2018-07-26 Tracie Sucharski - Reformated the widget to get rid of fixed sizes and
   *                           use layouts to handle sizing instead.  Also put entire widget in
   *                           a scrolled area.
   */
  class JigsawRunWidget : public QDockWidget {
    Q_OBJECT

  public:
    explicit JigsawRunWidget(Project *project, QWidget *parent = 0);
    explicit JigsawRunWidget(Project *project,
                          BundleSettingsQsp bundleSettings,
                          Control *selectedControl,
                          QString outputControlFileName,
                          QWidget *parent = 0);

    ~JigsawRunWidget();
    void closeEvent(QCloseEvent *event);


  public slots:
    void outputBundleStatus(QString status);
    void errorString(std::string error);
    void reportException(QString exception);
    void updateIteration(int iteration);
    void updatePoint(int point);
    void updateStatus(QString status);
    void bundleFinished(BundleSolutionInfo *bundleSolutionInfo);
    void notifyThreadFinished();

  protected:
    void init();
    BundleAdjust *m_bundleAdjust;
    Project *m_project;
    Control *m_selectedControl;
    QString m_selectedControlName;
    QString m_outputControlName;
    BundleSettingsQsp m_bundleSettings;

  private:
    bool m_bRunning; /**< Indicates whether or not the bundle adjust is running. */
    QThread *m_bundleThread; /**< separate thread for running bundle adjust calculations in. */

    /**
     * Functor used to copy images to a specified destination directory. This is used by
     * a QtConcurrent::mapped call in acceptBundleResults().
     *
     * @author 2017-05-04 Ian Humphrey
     *
     * @internal
     */
    class CopyImageToResultsFunctor :
        public std::function<Cube *(const FileName &)> {
      public:
        CopyImageToResultsFunctor(const QDir &destination);
        ~CopyImageToResultsFunctor();
        Cube *operator()(const FileName &image);
      private:
        CopyImageToResultsFunctor &operator=(const CopyImageToResultsFunctor &other);
        QDir m_destinationFolder; /**< Directory to copy the image to. */
    };

  private slots:
    void on_JigsawSetupButton_clicked();
    void on_JigsawRunButton_clicked();
    void on_JigsawAcceptButton_clicked();
    void clearDialog();
    void updateScrollBar();

  private:
    /** Captures the most recent results of a bundle. JigsawRunWidget owns this pointer. */
    BundleSolutionInfo *m_bundleSolutionInfo;
    /** Reference to self's UI generated with QtDesigner. */
    Ui::JigsawRunWidget *m_ui;
  };
};
#endif
