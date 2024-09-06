#include "MainWindow.h"

#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * Mainwindow constructor
   *
   *
   * @param title
   * @param parent
   * @param flags
   */
  MainWindow::MainWindow(QString title, QWidget *parent, Qt::WindowFlags flags) :
      QMainWindow(parent, flags) {
    //qDebug()<<"MainWindow::MainWindow";
    setWindowTitle(title);
  }


  /**
   * Free allocated memory by from this instance.
   */
  MainWindow::~MainWindow() {
    if (isVisible())
      close();
  }


  /**
   * This method is overridden so that we can be sure to write the
   * current settings of the Main window.
   *
   * @param event
   */
  void MainWindow::closeEvent(QCloseEvent *event) {
    writeSettings();
    QMainWindow::closeEvent(event);
  }


  QString MainWindow::settingsFileName(QString objectTitle) {
    if (QCoreApplication::applicationName() == "") {
      throw IException(IException::Programmer, "You must set QApplication's "
          "application name before using the Isis::MainWindow class. Window "
          "state and geometry can not be saved and restored", _FILEINFO_);
    }

    if (objectTitle == "") {
      throw IException(IException::Programmer,
          tr("You must provide a valid objectTitle to MainWindow::settingsFileName"),
          _FILEINFO_);
    }

    QDir programSettings =
        QDir(QString::fromStdString(FileName("$HOME/.Isis/" + QCoreApplication::applicationName().toStdString() + "/").path()));
    QString windowSettings = programSettings.filePath(objectTitle + ".config");

    return windowSettings;
  }


  /**
   * This method ensure that the settings get written even if the
   * Main window was only hidden, not closed.
   *
   * @param event
   */
//   void MainWindow::hideEvent(QHideEvent *event) {
//     writeSettings();
//   }


  /**
   * This method is called from the constructor so that when the
   * Main window is created, it know's it's size and location.
   *
   */
  void MainWindow::readSettings(QSize defaultSize) {
    //qDebug()<<"MainWindow::readSettings";
    QSettings settings(settingsFileName(), QSettings::NativeFormat);
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // The geom/state isn't enough for main windows to correctly remember
    //   their position and size, so let's restore those on top of
    //   the geom and state.
    if (!settings.value("pos").toPoint().isNull())
      move(settings.value("pos").toPoint());

    resize(settings.value("size", defaultSize).toSize());
  }


  QString MainWindow::settingsFileName() const {
    if (QCoreApplication::applicationName() == "") {
      throw IException(IException::Programmer, "You must set QApplication's "
          "application name before using the Isis::MainWindow class. Window "
          "state and geometry can not be saved and restored", _FILEINFO_);
    }

    if (objectName() == "") {
      throw IException(IException::Programmer,
          tr("You must set the objectName of the widget titled [%1] before "
          "using the instance. Window state and geometry can not be saved and "
          "restored").arg(windowTitle()), _FILEINFO_);
    }

    QDir programSettings =
        QDir(QString::fromStdString(FileName("$HOME/.Isis/" + QCoreApplication::applicationName().toStdString() + "/").path()));
    QString windowSettings = programSettings.filePath(objectName() + ".config");
    //qDebug()<<"MainWindow::settingsFileName windowSettings = "<<windowSettings;
    return windowSettings;
  }


  /**
   * This method is called when the Main window is closed or
   * hidden to write the size and location settings to a config
   * file in the user's home directory.
   *
   */
  void MainWindow::writeSettings() const {
    //qDebug()<<"MainWindow::writeSettings";
    QSettings settings(settingsFileName(), QSettings::NativeFormat);

    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("size", size());
    settings.setValue("pos", pos());
  }
}

