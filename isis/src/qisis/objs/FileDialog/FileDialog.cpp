#include "FileDialog.h"

#include <iostream>

#include <QComboBox>
#include <QFile>
#include <QLineEdit>
#include <QPushButton>

#include "FileName.h"
#include "IString.h"

namespace Isis {
  FileDialog::FileDialog(QString title, QStringList &filterList, QDir &directory, QWidget *parent) :
    QFileDialog(parent), p_filterList(filterList), p_dir(directory) {

    p_parent = parent;

    // We want to use Qt's dialog so we can find the children widgets
    this->setOptions(QFileDialog::DontUseNativeDialog);

    this->setWindowTitle(title);
    this->setFileMode(QFileDialog::ExistingFiles);
    if(parent != 0) {
      parent->installEventFilter(this);
      p_appName = parent->windowTitle();
    }
    connect(this, SIGNAL(accepted()), this, SLOT(sendSignal()));

    //this->setFilters(p_filterList);
    this->setNameFilters(p_filterList);

    this->setViewMode(QFileDialog::Detail);
    if(directory.exists()) {
      this->setDirectory(directory);
    }
    else {
      this->setDirectory(QDir::current());
    }

    p_comboBoxes = this->findChildren<QComboBox *>();
    p_comboBoxes[0]->setEditable(true);
    p_comboBoxes[1]->setEditable(true);

    QLineEdit *lineEdit = p_comboBoxes[1]->lineEdit();
    //p_comboBoxes[1]->setEditText("Choose a filter or create your own.");
    disconnect(lineEdit, 0, 0, 0);
    connect(lineEdit, SIGNAL(textChanged(const QString &)), p_comboBoxes[1],
            SIGNAL(activated(const QString &)));
    connect(lineEdit, SIGNAL(editingFinished()), this,
            SLOT(saveFilter()));


    p_allPButtons = this->findChildren<QPushButton *>();
    for(int i = 0; i < p_allPButtons.size(); i++) {
      //Disconnecting both buttons from all their old connection so we have complete control.
      disconnect(p_allPButtons[i], 0, 0, 0);
      if(p_allPButtons[i]->text().contains("Open", Qt::CaseInsensitive)) {
        connect(p_allPButtons[i], SIGNAL(clicked()), this, SLOT(done()));
      }
      if(p_allPButtons[i]->text().contains("Cancel", Qt::CaseInsensitive)) {
        ///I had to disconnect this buttons signal because I overwrote the
        //done method from QDialog which is what this used to be connected to.
        connect(p_allPButtons[i], SIGNAL(clicked()), this, SLOT(cancel()));
      }
    }

    readSettings();
  }


  /**
   * This is where we actually set the user editable filters and
   * remember them
   *
   */
  void FileDialog::saveFilter() {
    p_allPButtons[0]->setDefault(false);
    if(!p_filterList.contains(p_comboBoxes[1]->currentText())) {
      p_filterList.insert(0, p_comboBoxes[1]->currentText());
      this->setNameFilters(p_filterList);
    }

  }


  /**
   * This saves the directory that the user selected the file from
   * so it can open to this directory next time.
   * Also, emits the signal to open the selected file.
   */
  void FileDialog::sendSignal() {
    p_dir = this->directory();
    QStringList fileList = this->selectedFiles();
    QStringList::const_iterator it = fileList.begin();
    while(it != fileList.end()) {
      if(!(*it).isEmpty()) {
        emit fileSelected(*it);
        ++it;
      }
    }
    //p_fileList = fileList;
  }


  /**
   * This method is overridden so that we can be sure to write the
   * current settings of the Main window.
   *
   * @param event
   */
  void FileDialog::closeEvent(QCloseEvent *event) {
    writeSettings();

  }


  /**
   * Called when the user presses OK.
   *
   */
  void FileDialog::done() {
    close();
    sendSignal();
  }

  /**
   * Called when the user presses OK.
   *
   */
  void FileDialog::done(int r) {
    QFileDialog::done(r);
  }


  /**
   * Called when user presses cancel.
   *
   */
  void FileDialog::cancel() {
    close();
    p_dir = this->directory();
  }

  /**
   * This method is called from the constructor so that when the
   * Main window is created, it know's it's size and location.
   *
   */
  void FileDialog::readSettings() {
    if(p_appName == "") {
      p_appName = this->windowTitle();
    }

    QString instanceName = this->windowTitle();
    // FileName is a QFileDialog enum for DialogLabel
    Isis::FileName config("$HOME/.Isis/" + p_appName.toStdString() + "/" + instanceName.toStdString() + ".config");
    QSettings settings(QString::fromStdString(config.expanded()), QSettings::NativeFormat);
    QPoint pos = settings.value("pos", QPoint(300, 100)).toPoint();
    QSize size = settings.value("size", QSize(355, 350)).toSize();
    resize(size);
    move(pos);
  }

  /**
   * This method is called when the File Dialog is closed or
   * hidden to write the size and location settings to a config
   * file in the user's home directory.
   *
   */
  void FileDialog::writeSettings() {
    /*We do not want to write the settings unless the window is
      visible at the time of closing the application*/
    if(!this->isVisible()) return;

    if(p_appName == "") {
      p_appName = this->windowTitle();
    }

    QString instanceName = this->windowTitle();
    // FileName is a QFileDialog enum for DialogLabel
    Isis::FileName config("$HOME/.Isis/" + p_appName.toStdString() + "/" + instanceName.toStdString() + ".config");
    QSettings settings(QString::fromStdString(config.expanded()), QSettings::NativeFormat);
    settings.setValue("pos", pos());
    settings.setValue("size", size());

  }

  /**
  * This event filter is installed on the parent of this window.
  * When the user closes the main window of the application, the
  * FileDialog will write their settings even though they did not
  * receive the close event themselves.
  *
  * @param o
  * @param e
  *
  * @return bool
  */
  bool FileDialog::eventFilter(QObject *o, QEvent *e) {

    switch(e->type()) {
      case QEvent::Close: {
          writeSettings();

        }
      case QEvent::Hide: {
          writeSettings();

        }
      default: {
          return false;
        }
    }
  }

}
