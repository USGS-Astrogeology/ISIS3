/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProcessByTile.h"
#include "SpecialPixel.h"
#include "Pvl.h"
#include "UserInterface.h"
#include "Brick.h"
#include "Apollo.h"
#include <QString>
#include <cstdlib>

#include "apolloremrx.h"

using namespace std;
namespace Isis {

  void cpy(Buffer &in, Buffer &out);


  void apolloremrx(UserInterface &ui) {
    Cube cube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      cube.setVirtualBands(inAtt.bands());
    }
    cube.open(ui.GetCubeName("FROM"));
    apolloremrx(&cube, ui);
  }


  void apolloremrx(Cube *info, UserInterface &ui) {
    int dim;
    bool resvalid;
    QString action;
    // We will be processing by line
    ProcessByTile p;
    p.SetTileSize(128, 128);

    // Setup the input and output cubes
    p.SetInputCube(info);
    PvlKeyword &status = info->group("RESEAUS")["STATUS"];
    QString in = info->fileName();

    QString spacecraft = QString::fromStdString(info->group("Instrument")["SpacecraftName"]);
    QString instrument = QString::fromStdString(info->group("Instrument")["InstrumentId"]);
    if (spacecraft.mid(0,6) != "APOLLO") {
      std::string msg = "This application is for use with Apollo spacecrafts only. ";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
    Apollo apollo(spacecraft, instrument);

    // Check reseau status and make sure it is not nominal or removed
    if ((std::string)status == "Nominal") {
      QString msg = "Input file [" + in +
            "] appears to have nominal reseau status. You must run findrx first.";
      throw IException(IException::User,msg, _FILEINFO_);
    }
    if ((std::string)status == "Removed") {
      QString msg = "Input file [" + in +
            "] appears to already have reseaus removed.";
      throw IException(IException::User,msg, _FILEINFO_);
    }

    status = "Removed";

    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    p.SetOutputCube(ui.GetCubeName("TO"), att);

    // Start the processing
    p.StartProcess(cpy);
    p.EndProcess();

    dim = apollo.ReseauDimension();

    // Get other user entered options
    QString out = ui.GetCubeName("TO");
    resvalid = ui.GetBoolean("RESVALID");
    action = ui.GetString("ACTION");

    // Open the output cube
    Cube cube;
    cube.open(out, "rw");

    PvlGroup &res = cube.label()->findGroup("RESEAUS",Pvl::Traverse);

    // Get reseau line, sample, type, and valid Keywords
    PvlKeyword lines = res.findKeyword("LINE");
    PvlKeyword samps = res.findKeyword("SAMPLE");
    PvlKeyword type = res.findKeyword("TYPE");
    PvlKeyword valid = res.findKeyword("VALID");
    int numres = lines.size();

    Brick brick(dim,dim,1,cube.pixelType());
    int width = ui.GetInteger("WIDTH");
    for (int res=0; res<numres; res++) {
      if ((resvalid == 0 || std::stoi(valid[res]) == 1)) {
        int baseSamp = (int)(std::stod(samps[res])+0.5) - (dim/2);
        int baseLine = (int)(std::stod(lines[res])+0.5) - (dim/2);
        brick.SetBasePosition(baseSamp,baseLine,1);
        cube.read(brick);
        if (action == "NULL") {
          // set the three pixels surrounding the reseau to null
          for (int i=0; i<dim; i++) {
            for (int j=(width-1)/-2; j<=width/2; j++) {
              // vertical lines
              brick[dim*i+dim/2+j] = Isis::Null;
              // horizontal lines
              brick[dim*(dim/2+j)+i] = Isis::Null;
            }
          }
        }
        else if (action == "PATCH") {
          for (int i = 0; i < dim; i++) {
            for (int j=(width-1)/-2; j<=width/2; j++) {
              // vertical lines
              brick[dim*i+dim/2+j] = (brick[dim*i+dim/2-width+j] + brick[dim*i+dim/2+width+j])/2.0;
              // horizontal lines
              brick[dim*(dim/2+j)+i] = (brick[dim*(dim/2-width+j)+i]+brick[dim*(dim/2+width+j)+i])/2.0;
            }
          }
        }
      }
      cube.write(brick);
    }
    cube.close();
  }

  // Copy the input cube to the output cube
  void cpy(Buffer &in, Buffer &out) {
    for (int i=0; i<in.size(); i++) {
      out[i] = in[i];
    }
  }
}
