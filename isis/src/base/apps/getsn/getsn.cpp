#include "getsn.h"

#include <iostream>
#include <sstream>
#include <QString>

#include "Application.h"
#include "Blob.h"
#include "Cube.h"
#include "History.h"
#include "ObservationNumber.h"
#include "Preference.h"
#include "Pvl.h"
#include "SerialNumber.h"
#include "SessionLog.h"
#include "TextFile.h"

using namespace Isis;
using namespace std;


namespace Isis {

  void getsn( UserInterface &ui, Pvl *log ) {
    // Open the input cube
    Cube *cube = new Cube( ui.GetCubeName("FROM").toStdString(), "r");
    
    getsn( cube, ui, log );

  }


  void getsn( Cube *cube, UserInterface &ui, Pvl *log ) {

    // Determine if output should be written base on parameters
    bool WriteFile = ui.GetBoolean("FILE");
    bool WriteSN = ui.GetBoolean("SN");
    bool WriteObservation = ui.GetBoolean("OBSERVATION");
    
    QString from = cube->fileName();    
    QString format = ui.GetString("FORMAT");
    bool pvl;
    if (format == "PVL") {
      pvl = true;
    }
    else {
      pvl = false;
    }

    // Extract label from cube file
    Pvl *label = cube->label();

    PvlGroup sn("Results");

    if (WriteFile) sn += PvlKeyword("Filename", from.toStdString());
    if (WriteSN) sn += PvlKeyword( "SerialNumber", SerialNumber::Compose( *label, ui.GetBoolean("DEFAULT") ).toStdString() );
    if (WriteObservation) sn += PvlKeyword( "ObservationNumber", ObservationNumber::Compose( *label, ui.GetBoolean("DEFAULT") ).toStdString() );

    if ( ui.WasEntered("TO") ) {
      // PVL option
      if (pvl) {
	// Create a serial number and observation number for this cube & put it in a pvlgroup for output
	Pvl pvl;
	pvl.addGroup(sn);
	if ( ui.GetBoolean("APPEND") )
	  pvl.append( ui.GetFileName("TO").toStdString() );
	else
	  pvl.write( ui.GetFileName("TO").toStdString() );
      }
      // FLAT option
      else {
	bool append = ui.GetBoolean("APPEND");
	// Open in append or overwrite
	TextFile txt(ui.GetFileName("TO"), append ? "append" : "overwrite");

	// Build QString
	QString line = "";
	for (int i = 0; i < sn.keywords(); i++) {
	  if (i != 0)
	    line += ",";
	  line += QString::fromStdString(sn[i][0]);
	}
	txt.PutLine(line);
      }
            
    }
    else {
      for (int i = 0; i < sn.keywords(); i++) {
	cout << sn[i][0] << endl;
      }
    }
    if (ui.IsInteractive()) {
      Application::AppendAndLog(sn, log);
    }
    else {
      log->addGroup(sn);
    }
  }
}
