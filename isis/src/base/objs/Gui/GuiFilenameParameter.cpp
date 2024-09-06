/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QFileDialog>
#include <QFontMetrics>
#include <QHBoxLayout>

#include "GuiFilenameParameter.h"
#include "IString.h"
#include "UserInterface.h"


namespace Isis {

  /**
   * Construct a GuiFileNameParameter object
   *
   * @param grid Pointer to QGridLayout
   * @param ui User interface object
   * @param group Index of group
   * @param param Index of parameter
   *
   * @internal
   */
  GuiFileNameParameter::GuiFileNameParameter(QGridLayout *grid, UserInterface &ui,
      int group, int param) :
    GuiParameter(grid, ui, group, param) {
    connect(p_lineEdit, SIGNAL(textChanged(const QString &)), this, SIGNAL(ValueChanged()));
    grid->addWidget(p_lineEdit, param, 2);

    grid->addWidget(p_fileButton, param, 3);

    QAction *action = new QAction(this);
    QString file = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/view_tree.png").expanded());
    action->setIcon(QPixmap(file));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(SelectFile()));

    p_fileButton->setIconSize(QSize(22, 22));
    p_fileButton->setIcon(QPixmap(file));
    p_fileButton->setDefaultAction(action);
    p_fileButton->setToolTip("Select file");
    QString fileButtonWhatsThisText = "<p><b>Function:</b> \
            Opens a file chooser window to select a file from</p>";
    p_fileButton->setWhatsThis(fileButtonWhatsThisText);

    if(p_ui->HelpersSize(group, param) != 0) {
      grid->addWidget(AddHelpers(p_lineEdit), param, 4);
    }

    RememberWidget(p_lineEdit);
    RememberWidget(p_fileButton);

    p_type = FileNameWidget;
  }


  /**
   * Destructor of GuiFileNameParameter object
   */
  GuiFileNameParameter::~GuiFileNameParameter() {}


  /**
   * Sets the line edit text box to value passed in by this method
   *
   * @param newValue
   */
  void GuiFileNameParameter::Set(QString newValue) {
    p_lineEdit->setText(newValue);
  }


  /**
   * Gets the value found in the line edit text box.
   *
   * @return @b IString Value found in line edit text box
   */
  QString GuiFileNameParameter::Value() {
    return p_lineEdit->text();
  }

  /**
   * Gets an input/output file from a GUI filechooser or typed in filename.
   *
   * This method determines which directory to look in, sets up extension
   * filters, and gets the filename.
   *
   * @internal
   *   @history  2007-05-16 Tracie Sucharski - For files located in CWD, do not
   *                           include path in the lineEdit.
   *   @history  2007-06-05 Steven Koechle - Corrected problem where output
   *                           FileName was being opened not saved. Defaulted to
   *                           output mode if not specified.
   *   @history  2009-11-02 Mackenzie Boyd - Corrected building of filter string
   *                           to match GuiCubeParameter.cpp and, if no filters
   *                           specified, to specify "Any(*)" not ";;Any(*)" as
   *                           it had been doing.
   *   @history  2010-07-19 Jeannie Walldren - Modified to allow users to
   *                           view files in the directory.
   */
  void GuiFileNameParameter::SelectFile() {
    // What directory do we look in?
    QString dir;
    if((p_lineEdit->text().length() > 0) &&
        (p_lineEdit->text() != p_ui->ParamInternalDefault(p_group, p_param))) {
      Isis::FileName fname(p_lineEdit->text().toStdString());
      dir = QString::fromStdString(fname.expanded());
    }
    else if(p_ui->ParamPath(p_group, p_param).length() > 0) {
      Isis::FileName fname(p_ui->ParamPath(p_group, p_param).toStdString());
      dir = QString::fromStdString(fname.expanded());
    }

    // Set up the filter
    QString filter = p_ui->ParamFilter(p_group, p_param);
    if(filter.isEmpty()) {
      filter = "Any(*)";
    }
    else {
      filter += ";;Any(*)";
    }

    // Get the filename
    QString fnameQString;

    if(p_ui->ParamFileMode(p_group, p_param) == "input") {
      fnameQString = QFileDialog::getOpenFileName(p_fileButton, "Select file", dir, filter);
    }
    else {
      // The IsisPreference file has groups "FileCustomization" and
      // "CubeCustomization" with keyword "Overwrite" and possible values
      // "Allow" or "Error".
      // Qt does not provide an option to disallow overwrites, so we are unable
      // to handle this preference here.  Instead, the IsisAml class and the
      // Cube class CubeIoHandler checks these preferences, respectively, and
      // throws an error if overwrites are not allowed
      // 2010-07-15 Jeannie Walldren
      QFlags<QFileDialog::Option> options(QFileDialog::DontConfirmOverwrite);
      fnameQString = QFileDialog::getSaveFileName(p_fileButton, "Select file", dir, filter, 0, options);
    }
    if(fnameQString != "") {
      Isis::FileName fname(fnameQString.toStdString());
      if(fname.dir() == std::filesystem::current_path()) {
        fnameQString = QString::fromStdString(fname.name());
      }
      Set(fnameQString);
    }
  }
}

