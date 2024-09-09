#include "cubeit.h"

#include "Application.h"
#include "ProcessByLine.h"
#include "ProcessMosaic.h"
#include "IException.h"
#include "FileList.h"
#include "SpecialPixel.h"

using namespace std;
namespace Isis {
  void NullBand(Buffer &out);

  void cubeit(UserInterface &ui, Pvl *log) {
    // Get the list of cubes to stack
    Process p;
    FileList cubeList(ui.GetFileName("FROMLIST").toStdString());
    FileList newcubeList; //cubes with at least 1 non-TRACKING band
    QList<vector<QString> > newVirtualBands; //non-TRACKING bands to propagate

    //Results group to contain information about unpropagated TRACKING bands
    PvlGroup results("Results");

    // Loop through the list
    int nsamps(0), nlines(0), nbands(0);
    PvlGroup outBandBin("BandBin");
    try {
      for(int i = 0; i < cubeList.size(); i++) {
        vector<QString> newBands;
        Cube cube;
        CubeAttributeInput inatt(cubeList[i].original());
        vector<QString> bands = inatt.bands();
        cube.setVirtualBands(bands);
        cube.open(QString::fromStdString(cubeList[i].toString()));

        if( cube.hasTable("InputImages") ) {
          //search through band bin group of input cube for "TRACKING"
          PvlObject cubeLabel =cube.label()->findObject("IsisCube");
          PvlGroup bandbin = cubeLabel.findGroup("BandBin");

          //Different cubes use either FilterName or FilterNumber in the BandBin group
          //to refer to the same thing: a list of the numbers/names of each band, in order
          PvlKeyword filterName;
          if( bandbin.hasKeyword("FilterName") ){
            filterName = bandbin.findKeyword("FilterName");
          }
          else if ( bandbin.hasKeyword("FilterNumber")) {
            filterName = bandbin.findKeyword("FilterNumber");
          }
          else {
              std::string msg = "The BandBin group of a cube with tracking information [" +
                          cubeList[i].toString() + "] does not have a FilterName or a FilterNumber.";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }

          for (int j = 0; j < filterName.size(); j++) {
            if (filterName[j] != "TRACKING") {
              newBands.push_back(QString::number(j+1));
            }
            else {
              std::string msg = "TRACKING band not propagated from " + cubeList[i].toString();
              results += PvlKeyword("UnpropagatedBand", msg);
            }
          }

          //if there are some bands that aren't TRACKING, set the cube to use those
          if (newBands.size() > 0) {
            cube.close();
            cube.setVirtualBands(newBands);
            cube.open(QString::fromStdString(cubeList[i].toString()));
          }
          //if the only provided bands are TRACKING, don't use this cube at all
          else {
            cube.close();
            continue;
          }
        }

        //initialize ns, nl, nb if we're at our first non-tracking cube
        if(newcubeList.size() == 0) {
          nsamps = cube.sampleCount();
          nlines = cube.lineCount();
          nbands = cube.bandCount();
        }
        else {
          // Make sure they are all the same size
          if((nsamps != cube.sampleCount()) || (nlines != cube.lineCount())) {
            std::string msg = "Spatial dimensions of cube [" +
                          cubeList[i].toString() + "] does not match other cubes in list";
            throw IException(IException::User, msg, _FILEINFO_);
          }
          // Get the total number of bands
          nbands += cube.bandCount();
        }

        // Build up the band bin group
        PvlObject &isiscube = cube.label()->findObject("IsisCube");
        if(isiscube.hasGroup("BandBin")) {
          PvlGroup &inBandBin = isiscube.findGroup("BandBin");
          for(int key = 0; key < inBandBin.keywords(); key++) {
            PvlKeyword &inKey = inBandBin[key];
            if(!outBandBin.hasKeyword(inKey.name())) {
              outBandBin += inKey;
            }
            else {
              PvlKeyword &outKey = outBandBin[inKey.name()];
              for(int index = 0; index < (int)inKey.size(); index++) {
                outKey.addValue(inKey[index], inKey.unit(index));
              }
            }
          }
        }
        cube.close();
        newVirtualBands.append(newBands);
        newcubeList.push_back(cubeList[i]);
      }
      //Only write out results group if we added something to it.
      if (results.hasKeyword("UnpropagatedBand")) {
        Application::Log(results);
      }
    }
    catch(IException &e) {
      std::string msg = "Invalid cube in list file [" + ui.GetFileName("FROMLIST").toStdString() + "]";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    //if literally everything is a TRACKING band, throw an error, since we don't prop. TRACKING bands
    if (newcubeList.size() == 0) {
      std::string msg = "Only TRACKING bands supplied in [" + ui.GetFileName("FROMLIST").toStdString() + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Setup to propagate from the first input cube
    ProcessByLine p2;
    CubeAttributeInput inatt;

    int index = 0;
    if(ui.WasEntered("PROPLAB")) {
      bool match = false;
      QString fname = ui.GetCubeName("PROPLAB");
      for(int i = 0; i < cubeList.size(); i++) {
        if(fname == QString::fromStdString(cubeList[i].toString())) {
          index = i;
          match = true;
          break;
        }
      }
      if(!match) {
        std::string msg = "FileName [" + ui.GetCubeName("PROPLAB").toStdString() +
                      "] to propagate labels from is not in the list file [" +
                      ui.GetFileName("FROMLIST").toStdString() + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    p2.SetInputCube(QString::fromStdString(newcubeList[index].toString()), inatt);

    // Create the output cube
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *ocube = p2.SetOutputCube(ui.GetCubeName("TO"), att, nsamps, nlines, nbands);
    p2.ClearInputCubes();

    p2.Progress()->SetText("Allocating cube");
    p2.StartProcess(NullBand);

    // Add the band bin group if necessary
    if(outBandBin.keywords() > 0) {
      ocube->putGroup(outBandBin);
    }

    // Delete any tracking tables from the input label if necessary
    ocube->deleteBlob("InputImages", "Table");

    // Delete the Tracking group if it exists (3.6.0 and up)
    // The tracking group could be transfered from the first input cube, but it does not
    // represent the images used in any other band after cubeit.
    if(ocube->hasGroup("Tracking")) {
      ocube->deleteGroup("Tracking");
    }

    p2.EndProcess();

   // Now loop and mosaic in each cube
    int sband = 1;
    for(int i = 0; i < newcubeList.size(); i++) {
      ProcessMosaic m;
      m.SetBandBinMatch(false);

      Progress *prog = m.Progress();
      prog->SetText("Adding bands from Cube " + QString::number((int)i + 1) +
                    " of " + newcubeList.size());
      m.SetOutputCube("TO", ui);

      //update attributes to the input cube
      CubeAttributeInput attrib;

      if (newVirtualBands.at(i).size() == 0) {

        attrib.addAttributes(newcubeList[i]);

      } else {

        for(unsigned k=0; k < newVirtualBands.at(i).size(); k++) {
          attrib.addAttribute(newVirtualBands.at(i)[k]);
        }
      }

      Cube *icube = m.SetInputCube(QString::fromStdString(newcubeList[i].toString()), attrib, 1, 1, 1, -1, -1, -1);

      // Delete any tracking tables from the input cube if necessary
      icube->deleteBlob("InputImages", "Table");

      m.SetImageOverlay(ProcessMosaic::PlaceImagesOnTop);
      m.StartProcess(1, 1, sband);
      sband += icube->bandCount();
      m.EndProcess();
    }
  }

  // Line processing routine
  void NullBand(Buffer &out) {
    for(int i = 0; i < out.size(); i++) {
      out[i] = NULL8;
    }
  }
}
