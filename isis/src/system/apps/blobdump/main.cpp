/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#define GUIHELPERS

#include "Isis.h"

#include <fstream>

#include "Blob.h"
#include "Table.h"
#include "FileName.h"
#include "IString.h"
#include <sstream>

using namespace std;
using namespace Isis;

int pos = 0;
QString previousFile = "";

void helperButtonGetBlobList();

map <QString, void *> GuiHelpers() {
  map <QString, void *> helper;
  helper ["helperButtonGetBlobList"] = (void *) helperButtonGetBlobList;
  return helper;
}

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  FileName file = ui.GetCubeName("FROM");
  QString blobname = ui.GetString("NAME");
  QString blobtype = ui.GetString("TYPE");
  Blob blob(blobname, blobtype, file.expanded());
  FileName outfname = ui.GetFileName("TO");
  blob.Write(outfname.expanded());
}

// Function to find the available blob names/types and put them into the GUI
void helperButtonGetBlobList() {
  QString name, type;
  bool match = false;

  UserInterface &ui = Application::GetUserInterface();
  QString currentFile = ui.GetCubeName("FROM");
  const Pvl label(FileName(currentFile).expanded());

  // Check to see if the "FILE" parameter has changed since last press
  if(currentFile != previousFile) {
    ui.Clear("NAME");
    ui.Clear("TYPE");
    pos = 0;
    previousFile = currentFile;
  }

  // Look for blobs
  int cnt = 0;
  while(!match) {
    // If we've gone through all objects and found nothing, throw an exception
    if(cnt >= label.objects()) {
      pos = 0;
      std::string msg = "Parameter [FROM] has no blobs.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    // When the end of the objects is hit,
    // display "NAME" and "TYPE" parameters as blank
    if(pos >= label.objects()) {
      name = "";
      type = "";
      match = true;
      pos = 0;  // Prepare to start over again
    }
    // When we find a blob, fetch its name and type to stick in the parameters
    else if(label.object(pos).type() == "Object" &&
            label.object(pos).hasKeyword("Name") &&
            label.object(pos).hasKeyword("StartByte") &&
            label.object(pos).hasKeyword("Bytes")) {
      name = label.object(pos)["Name"][0];
      type = label.object(pos).name();
      match = true;
      pos++;
    }
    // Nothing's been found yet, keep looking for blobs
    else {
      pos++;
      cnt++;
    }
  }

  ui.Clear("NAME");
  ui.PutString("NAME", name);
  ui.Clear("TYPE");
  ui.PutString("TYPE", type);
}
