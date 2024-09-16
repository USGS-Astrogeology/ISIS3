#include "Isis.h"

#include <QDomDocument>
#include <QFile>
#include <QTextStream>

#include "IException.h"
#include "Pvl.h"
#include "OriginalLabel.h"
#include "OriginalXmlLabel.h"

using namespace Isis;
using namespace std;

void IsisMain() {
  // QDomDocument makes use of QHashes to store attributes.
  // This sets the QHash seed so that the attributes always come out in the
  // same order.
  qSetGlobalQHashSeed(1031);

  // Get user entered file name & mode
  UserInterface &ui = Application::GetUserInterface();
  QString file = ui.GetCubeName("FROM");

  Pvl fromLabel(file.toStdString());
  if ( fromLabel.hasObject("OriginalLabel") ) {
    OriginalLabel origLab(file);
    Pvl pvl = origLab.ReturnLabels();
    if (ui.IsInteractive()) {
      Application::GuiLog(pvl);
    }
    else if (ui.WasEntered("TO")) {
      if (ui.GetBoolean("APPEND")) {
        pvl.append(FileName(ui.GetFileName("TO").toStdString()).expanded());
      }
      else {
        pvl.write(FileName(ui.GetFileName("TO").toStdString()).expanded());
      }
    } 
    else {
      cout << pvl << endl;
    }
  }
  else if ( fromLabel.hasObject("OriginalXmlLabel") ) {
    OriginalXmlLabel origLab(file);
    QDomDocument origXml = origLab.ReturnLabels();
    if ( ui.IsInteractive() ) {
      Application::GuiLog( origXml.toString(2) );
    }
    else if (ui.WasEntered("TO")) {
      // Open the output file
      QFile outFile(QString::fromStdString(FileName(ui.GetFileName("TO").toStdString()).expanded()));
      QIODevice::OpenMode openMode;
      if (ui.GetBoolean("APPEND")) {
        openMode = QIODevice::WriteOnly | QIODevice::Append;
      }
      else {
        openMode = QIODevice::WriteOnly | QIODevice::Truncate;
      }
      if ( !outFile.open(openMode) ) {
        std::string msg = "Unable to open output file [" +
                      FileName( ui.GetFileName("TO").toStdString() ).expanded() +
                      "] with write permissions.";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Write to the output file
      QTextStream outputStream(&outFile);
      outputStream << origXml.toString(2);

      // Close the output file
      outFile.close();
    }
    else {
      cout << origXml.toString(2).toStdString() << endl;
    }
  }
  else {
    std::string msg = "Could not find OriginalLabel or OriginalXmlLabel "
                  "in input file [" + file.toStdString() + "].";
    throw IException(IException::User, msg, _FILEINFO_);
  }
}
