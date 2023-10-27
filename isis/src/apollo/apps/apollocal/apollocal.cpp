/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubeAttribute.h"
#include "ProcessByTile.h"
#include "Pvl.h"
#include "PvlTranslationTable.h"
#include "SpecialPixel.h"
#include "UserInterface.h"

#include "apollocal.h"

using namespace std;


namespace Isis {
  void cal(vector<Buffer *> &in,
         vector<Buffer *> &out);


  void apollocal(UserInterface &ui) {
    Cube cube(ui.GetCubeName("FROM"), "r");
    apollocal(&cube, ui);
  }


  void apollocal(Cube *inCube, UserInterface &ui) {
    // We will be processing by line
    ProcessByTile p;
    p.SetTileSize(128, 128);

    p.SetInputCube(inCube);

    PvlGroup &dataDir =
        Preference::Preferences().findGroup("DataDirectory");
    PvlTranslationTable tTable("$ISISROOT/appdata/translations/MissionName2DataDir.trn");
    QString missionDir = QString::fromStdString(
      dataDir[tTable.Translate("MissionName", QString::fromStdString(
        (inCube->group("Instrument")).findKeyword("SpacecraftName")[0])).toStdString()][0]);
    QString camera =
        QString::fromStdString((inCube->group("Instrument")).findKeyword("InstrumentId")[0]);

    CubeAttributeInput cai;
    p.SetInputCube(missionDir + "/calibration/" + camera + "_flatfield.cub", cai);

    CubeAttributeOutput cao;
    cao.setPixelType(Real);
    p.SetOutputCube(
        FileName(ui.GetAsString("TO")).expanded(),
        cao, inCube->sampleCount(), inCube->lineCount(),
        inCube->bandCount());

    p.StartProcess(cal);
    p.EndProcess();
  }


  void cal(vector<Buffer *> &in, vector<Buffer *> &out) {
    Buffer &inp = *in[0];      // Input Cube
    Buffer &fff = *in[1];      // Flat Field File
    Buffer &outp = *out[0];    // Output Cube


    // Loop for each pixel in the line.
    for (int i=0; i<inp.size(); i++) {
      if (IsSpecial(inp[i])) {
        outp[i] = inp[i];
      }
      else if (IsSpecial(fff[i])) {
        outp[i] = Null;
      }
      else {
        // Log Filter the film negative (and multiply by 2^16/16 to maintain the range of values)
        outp[i] = 65535.0*(1.0 - log(65536 - inp[i])/log(2.0)/16.0);
        outp[i] -=  1300.0;    // subtract dark current
        outp[i] /= fff[i];    // divide flat field to remove vignetting effects
      }
    }
  }
}
