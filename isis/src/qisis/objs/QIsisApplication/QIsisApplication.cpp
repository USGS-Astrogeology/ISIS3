#include "QIsisApplication.h"

#include <locale.h>

#include <QDesktopServices>
#include <QFileInfo>
#include <QObject>
#include <QMessageBox>
#include <QUrl>

#include "FileName.h"
#include "Preference.h"
#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * Constructor
   *
   * @param argc Pass this in from main(argc, argv)
   * @param argv Pass this in from main(argc, argv)
   *
   * @internal
   * @history 2017-10-06 Adam Goins - QIsisApplication now checks for a "-pref" flag
   *                        in the command-line arguments and loads the following
   *                        preference file if it exists. Fixes # 814
   */
  QIsisApplication::QIsisApplication(int &argc, char *argv[]) :
    QApplication(argc, argv) {
    // try to use US locale for numbers so we don't end up printing "," instead
    //   of "." where it might count.

    for (int i = 1; i < argc; i++) {
        QString arg(argv[i]);
        if (arg.startsWith("-pref")) {

            // So that we can grab the file located after the current '-pref' flag.
            int nextIndex = i + 1;

            if (nextIndex < argc) {
                FileName preferenceFile(argv[nextIndex]);
                std::string filePath = preferenceFile.expanded();
                Preference::Preferences().clear();
                Preference::Preferences().Load(filePath);
            }
            else {
                QMessageBox::warning(NULL, "Warning", "Preference flag set but no preference file given.");
            }
        }
    }
    setlocale(LC_NUMERIC, "en_US");

    QDesktopServices::setUrlHandler("http", this, "openUrl");
  }


  /**
  * notify - this function overrides the QApplication notify as
  * QT event handlers do not handle exceptions. QIsisApplication
  * handles the exception by catching it and displaying the
  * MessageBox Warning.
  *
  * @author Sharmila Prasad (11/24/2009)
  *
  * @param rec - QObject where the exception occured
  * @param ev  - Event where the exception occured
  *
  * @return bool
  *
  * @internal
  * @history 2011-03-11 Tracie Sucharski - Create dialog with thrown errors
  *                        instead of ignoring and clearing.
  */
  bool QIsisApplication::notify(QObject *rec, QEvent *ev) {
    try {
      return QApplication::notify(rec, ev);
    }
    catch(IException &e) {
      QMessageBox::critical(NULL, "Error", e.what());
    }
    return false;
  }


  /**
   * Open a URL in the browser specified by Isis.
   */
  void QIsisApplication::openUrl(QUrl url) {
     QDesktopServices::openUrl(url);
  }
}
