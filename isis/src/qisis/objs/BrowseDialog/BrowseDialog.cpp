/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BrowseDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QPushButton>

#include <iostream>

namespace Isis {

  /**
   * BrowseDialog constructor
   *
   *
   * @param title
   * @param filterList
   * @param directory
   * @param parent
   */
  BrowseDialog::BrowseDialog(QString title, QStringList &filterList, QDir &directory, QWidget *parent) :
    FileDialog(title, filterList, directory, parent), p_dir(directory) {

    /*This returns a list of all the buttons in QFileDialog. This way I can
      disconnect the buttons from the old signals and substitue my own.  The
      text on the buttons can also be changed*/
    QList<QPushButton *> allPButtons = this->findChildren<QPushButton *>();
    /*Do stuff to the first (Open) button.*/
    allPButtons[0]->setText("View");
    disconnect(allPButtons[0], 0, 0, 0);
    connect(allPButtons[0], SIGNAL(pressed()), this, SLOT(displayCube()));
    /*Do stuff to the second (Cancel) button.*/
    allPButtons[1]->setText("Close");

    this->setFileMode(QFileDialog::ExistingFile);

  }


  /**
   * This method is called after the user has selected a file
   * either by double clicking on the file name or by pressing the
   * 'view' button.  A signal is emitted which eventually calls
   * the addBrowseView function from the Workspace object.
   *
   */
  void BrowseDialog::displayCube() {
    p_dir = this->directory();
    QStringList selectedFiles = this->selectedFiles();

    /*If the user did not select a file warn them.*/
    if(selectedFiles.size() < 1) {
      QString message = tr("\nFile not found.\nPlease verify the "
                           "correct file name was given");
      QMessageBox::warning(this, "File Not Found", message);

    }
    else {

      emit fileSelected(selectedFiles[0]);
    }

  }

  /**
   * This is an overridden method from QDialog.  QDialog closes
   * the dialog. We want to leave the box open and display the
   * cube in the view port.
   *
   * @param r
   */
  void BrowseDialog::done(int r) {
    displayCube();
  }

}
