#include <cstdio>
#include <string>

#include "ProcessImportPds.h"
#include "LineManager.h"
#include "UserInterface.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "IException.h"
#include "iTime.h"
#include "ProcessByBrick.h"
#include "Brick.h"
#include "OriginalLabel.h"

#include "thm2isis.h"

using namespace std;

namespace Isis { 
  static vector<Cube *> outputCubes;
  static int frameletLines;
  static void TranslateLabels(Pvl &labelFile, Pvl &isis3, int numBands, UserInterface &ui);
  static void separateFrames(Buffer &in);


  void thm2isis(UserInterface &ui) {
    // Grab the file to import
    ProcessImportPds p;
    outputCubes.clear();
    frameletLines = 192;

    FileName in = ui.GetFileName("FROM");

    // Make sure it is a Themis EDR/RDR
    bool projected;
    try {
      Pvl lab(in.expanded().toStdString());
      projected = lab.hasObject("IMAGE_MAP_PROJECTION");
      QString id;
      id = QString::fromStdString(lab["DATA_SET_ID"]);
      id = id.simplified().trimmed();
      if(!id.startsWith("ODY-M-THM")) {
        QString msg = "Invalid DATA_SET_ID [" + id + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    }
    catch(IException &e) {
      QString msg = "Input file [" + in.expanded() +
                    "] does not appear to be " +
                    "in Themis EDR/RDR format";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    //Checks if in file is rdr
    if(projected) {
      QString msg = "[" + in.name() + "] appears to be an rdr file.";
      msg += " Use pds2isis.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Ok looks good ... set it as the PDS file
    Pvl pdsLab;
    p.SetPdsFile(in.expanded(), "", pdsLab);

    OriginalLabel origLabels(pdsLab);

    Pvl isis3Lab;
    TranslateLabels(pdsLab, isis3Lab, p.Bands(), ui);

    // Set up the output cube
    FileName outFile(ui.GetCubeName("TO"));
    PvlGroup &inst = isis3Lab.findGroup("Instrument", Pvl::Traverse);
    CubeAttributeOutput outAttr = ui.GetOutputAttribute("to");
    
    if(QString::fromStdString(inst["InstrumentId"]) == "THEMIS_VIS") {
      Cube *even = new Cube();
      Cube *odd = new Cube();

      even->setDimensions(p.Samples(), p.Lines(), p.Bands());
      odd->setDimensions(p.Samples(), p.Lines(), p.Bands());

      QString evenFile = outFile.path() + "/" + outFile.baseName() + ".even.cub";
      QString oddFile = outFile.path() + "/" + outFile.baseName() + ".odd.cub";

      even->create(evenFile, outAttr);
      odd->create(oddFile, outAttr);

      frameletLines = 192 / ((int)inst["SpatialSumming"]);

      outputCubes.push_back(odd);
      outputCubes.push_back(even);
    }
    else {
      Cube *outCube = new Cube();
      outCube->setDimensions(p.Samples(), p.Lines(), p.Bands());

      outCube->create(outFile.expanded(), outAttr);
      outputCubes.push_back(outCube);
    }

    // Import the file and then translate labels
    p.StartProcess(separateFrames);
    p.EndProcess();

    for(int i = 0; i < (int)outputCubes.size(); i++) {
      for(int grp = 0; grp < isis3Lab.groups(); grp++) {

        // vis image?
        if(outputCubes.size() != 1) {
          int numFramelets = p.Lines() / frameletLines;
          isis3Lab.findGroup("Instrument").addKeyword(
            PvlKeyword("NumFramelets", std::to_string(numFramelets)), Pvl::Replace
          );

          QString frameletType = ((i == 0) ? "Odd" : "Even");
          isis3Lab.findGroup("Instrument").addKeyword(
            PvlKeyword("Framelets", frameletType.toStdString()), Pvl::Replace
          );
        }

        outputCubes[i]->putGroup(isis3Lab.group(grp));
      }
      p.WriteHistory(*outputCubes[i]);
      outputCubes[i]->write(origLabels);
      outputCubes[i]->close();
      delete outputCubes[i];
    }

    outputCubes.clear();
  }

  //! Separates each of the individual VIS frames into their own file
  void separateFrames(Buffer &in) {
    // (line-1)/frameletHeight % numOutImages
    int outputCube = (in.Line() - 1) / frameletLines % outputCubes.size();
    LineManager mgr(*outputCubes[outputCube]);
    mgr.SetLine(in.Line(), in.Band());

    // mgr.Copy(in); doesn't work because the raw buffers dont match
    for(int i = 0; i < mgr.size(); i++)
      mgr[i] = in[i];

    outputCubes[outputCube]->write(mgr);

    // Null out every other cube
    for(int i = 0; i < (int)outputCubes.size(); i++) {
      if(i == outputCube) continue;

      LineManager mgr(*outputCubes[i]);
      mgr.SetLine(in.Line(), in.Band());

      for(int j = 0; j < mgr.size(); j++) {
        mgr[j] = Isis::Null;
      }

      outputCubes[i]->write(mgr);
    }
  }

  void TranslateLabels(Pvl &pdsLab, Pvl &isis3, int numBands, UserInterface &ui) {
    // Create the Instrument Group
    PvlGroup inst("Instrument");
    inst += PvlKeyword("SpacecraftName", "MARS_ODYSSEY");
    QString instId = QString::fromStdString(pdsLab["InstrumentId"]) + "_" + QString::fromStdString(pdsLab["DetectorId"]);
    inst += PvlKeyword("InstrumentId", instId.toStdString());
    inst += PvlKeyword("TargetName", pdsLab["TargetName"]);
    inst += PvlKeyword("MissionPhaseName", pdsLab["MissionPhaseName"]);
    inst += PvlKeyword("StartTime", pdsLab["StartTime"]);
    inst += PvlKeyword("StopTime", pdsLab["StopTime"]);
    inst += PvlKeyword("SpacecraftClockCount", pdsLab["SpacecraftClockStartCount"]);

    PvlObject &sqube = pdsLab.findObject("SPECTRAL_QUBE");
    if(instId == "THEMIS_IR") {
      inst += PvlKeyword("GainNumber", sqube["GainNumber"]);
      inst += PvlKeyword("OffsetNumber", sqube["OffsetNumber"]);
      inst += PvlKeyword("MissingScanLines", sqube["MissingScanLines"]);
      inst += PvlKeyword("TimeDelayIntegration",
                        sqube["TimeDelayIntegrationFlag"]);
      if(sqube.hasKeyword("SpatialSumming")) {
        inst += PvlKeyword("SpatialSumming", sqube["SpatialSumming"]);
      }
    }
    else {
      inst += PvlKeyword("ExposureDuration", sqube["ExposureDuration"]);
      inst += PvlKeyword("InterframeDelay", sqube["InterframeDelay"]);
      inst += PvlKeyword("SpatialSumming", sqube["SpatialSumming"]);
    }

    // Add at time offset to the Instrument group
    
    double spacecraftClockOffset = ui.GetDouble("TIMEOFFSET");
    inst += PvlKeyword("SpacecraftClockOffset", std::to_string(spacecraftClockOffset), "seconds");

    isis3.addGroup(inst);

    // Create the Band bin Group
    PvlGroup bandBin("BandBin");

    // The original band is the original ISIS cube band number upon ingestion
    PvlKeyword originalBand("OriginalBand");
    for(int i = 1; i <= numBands; i++) {
      originalBand.addValue(std::to_string(i));
    }
    bandBin += originalBand;

    bandBin += sqube.findGroup("BandBin")["BandBinCenter"];
    bandBin["BandBinCenter"].setName("Center");

    bandBin += sqube.findGroup("BandBin")["BandBinWidth"];
    bandBin["BandBinWidth"].setName("Width");

    // The FilterNumber keyword is a list indicating the time-ordered filter number of the
    // corresponding ISIS band. These values also indicate the physical order of the bands in the
    // detector array. They are numbered by ascending times (or starting detector lines).
    bandBin += sqube.findGroup("BandBin")["BandBinFilterNumber"];
    bandBin["BandBinFilterNumber"].setName("FilterNumber");

    // The BandNumber keyword is a list of wavelength-ordered band
    // numbers corresponding to filter numbers for each ISIS band.
    //
    // For IR, BandNumber always matches filter number since the filters are found
    // on the ccd in ascending wavelength order.
    //
    // For VIS, we have the following one to one correspondence of these keywords:
    // BandNumber   {1, 2, 3, 4, 5}
    // FilterNumber {2, 5, 3, 4, 1}
    //
    // Note that the BandNumber will match the OriginalBand only if the image
    // to be imported contains band number 1 and is not missing consecutive band numbers.
    bandBin += sqube.findGroup("BandBin")["BandBinBandNumber"];
    bandBin["BandBinBandNumber"].setName("BandNumber");

    isis3.addGroup(bandBin);

    // Create the archive Group
    PvlGroup arch("Archive");
    arch += PvlKeyword("DataSetId", pdsLab["DataSetId"]);
    arch += PvlKeyword("ProducerId", pdsLab["ProducerId"]);
    arch += PvlKeyword("ProductId", pdsLab["ProductId"]);
    arch += PvlKeyword("ProductCreationTime",
                      pdsLab["ProductCreationTime"]);
    arch += PvlKeyword("ProductVersionId", pdsLab["ProductVersionId"]);
  //  arch += PvlKeyword("ReleaseId",(string)pdsLab["ReleaseId"]);
    arch += PvlKeyword("OrbitNumber", pdsLab["OrbitNumber"]);

    arch += PvlKeyword("FlightSoftwareVersionId",
                      sqube["FlightSoftwareVersionId"]);
    arch += PvlKeyword("CommandSequenceNumber",
                      sqube["CommandSequenceNumber"]);
    arch += PvlKeyword("Description", sqube["Description"]);
    isis3.addGroup(arch);

    // Create the Kernel Group
    PvlGroup kerns("Kernels");
    if(instId == "THEMIS_IR") {
      kerns += PvlKeyword("NaifFrameCode", std::to_string(-53031));
    }
    else {
      kerns += PvlKeyword("NaifFrameCode", std::to_string(-53032));
    }
    isis3.addGroup(kerns);
  }
}