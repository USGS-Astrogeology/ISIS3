/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlEditDialog.h"

#include <fstream>
#include <sstream>

#include <QFileDialog>
#include <QListWidget>
#include <QString>
#include <QMessageBox>
#include <QPushButton>
#include <QTextEdit>
#include <QtGui>
#include <QVBoxLayout>

#include "IException.h"
#include "Pvl.h"

using namespace std;

namespace Isis {
  /**
   * This constructor creates a PvlEditDialog object given a
   * pointer to a Pvl object.
   *
   * @param pvl Pvl file to be viewed or edited
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version
   *   @history 2008-12-15 Jeannie Walldren - Removed unneccesary
   *            error catch.
   */
  PvlEditDialog::PvlEditDialog(Pvl &pvl, QWidget *parent) : QDialog(parent) {

    // Create text edit box and fill it with pvl file contents
    p_textEdit = new QTextEdit;
    fstream input;

    // open as input from pvl file
    input.open(pvl.fileName(), ios::in);
    string output;

    // read first line of input and write as first output line
    getline(input, output);
    while(!input.eof()) {
      // append this line of output to the QTextEdit box
      p_textEdit->append(QString::fromStdString(output));

      // read next line of input and write as next output line
      getline(input, output);
    }
    input.close();

    //  Create Close and Save buttons in an QHBoxLayout
    p_saveButton = new QPushButton("Save Changes &As...");
    p_saveButton->setEnabled(false);
    QPushButton *closeButton = new QPushButton("&Close");

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(p_saveButton);
    buttonLayout->addWidget(closeButton);

    // Add QTextEdit box and button layout to QVBoxLayout
    // and set this to the layout of the QDialog window
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(p_textEdit);
    vLayout->addLayout(buttonLayout);

    setLayout(vLayout);
    QString titleBar = "Pvl File: " + QString::fromStdString(pvl.fileName());
    setWindowTitle(titleBar);

    // Add functionality to buttons
    connect(p_textEdit, SIGNAL(textChanged()), this, SLOT(enableSaveButton()));
    connect(p_saveButton, SIGNAL(clicked()), this, SLOT(saveTextEdit()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));

  }

  /**
   * Allow the "Save Changes" button to be activated.
   *
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version.
   */
  void PvlEditDialog::enableSaveButton() {
    p_saveButton->setEnabled(true);
  }

  /**
   * Save the edited text as a new Pvl file.
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original Version.
   *   @history 2008-12-15 Jeannie Walldren - Added a verification
   *            that the edited file is in Pvl format before
   *            allowing save. Replaced error throw with
   *            QMessageBox warning.
   */
  void PvlEditDialog::saveTextEdit() {
    // Check validity of format by placing QTextEdit contents into a Pvl object
    Pvl pvl;
    stringstream ss;
    string textEditContents = p_textEdit->toPlainText().toStdString();

    //fill stringstream with contents of QTextEdit
    ss << textEditContents;

    try {
      // fill pvl with contents of stringstream
      ss >> pvl;
    }
    catch(IException &e) {
      // catch errors in Pvl format when populating pvl object
      QString message = e.toString();
      QMessageBox::warning((QWidget *)parent(), "Error", message);
      return;
    }

    // get a window to choose a name and location for the saved file
    // default: look in user's current directory for *.def or *.pvl files
    QString filter = "Select registration template (*.def *.pvl);;";
    filter += "All (*)";
    QString pvlFile = QFileDialog::getSaveFileName((QWidget *)parent(),
                      "Select a registration template",
                      ".",
                      filter);
    if(!pvlFile.isEmpty()) {
      // convert QString to std::string needed to open file stream
      QString saveFile = pvlFile;
      try {
        pvl.write(saveFile.toStdString());
      }
      catch(IException &e) {
        // report exception(s) to mesage box
        QString message = e.toString();
        QMessageBox::warning((QWidget *)parent(), "Error", message);
        return;
      }
    }

    // refresh titleBar with new file name
    setWindowTitle("Pvl File: " + pvlFile);
  }
}
