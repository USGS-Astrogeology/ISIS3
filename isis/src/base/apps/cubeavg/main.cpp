#include "Isis.h"
#include "ProcessBySpectra.h"
#include "Statistics.h"
#include "Application.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

void cubeavg(vector<Buffer *> &in,
             vector<Buffer *> &out);

void removekeywords(PvlGroup &pvlg);

void compute(vector<double> centers,
             vector<double> widths,
             Cube *ocube);

void IsisMain() {
  ProcessBySpectra p;
  p.SetType(ProcessBySpectra::PerPixel);
  Cube *icube = p.SetInputCube("FROM");
  Cube *ocube = p.SetOutputCube("TO", icube->sampleCount(), icube->lineCount(), 1);

  //Get user parameters and sets outputcube's BandBin
  UserInterface &ui = Application::GetUserInterface();
  if(ui.GetString("BANDBIN") == "COMPUTE") {
    if(icube->hasGroup("BandBin")) {
      PvlGroup &pvlg = icube->group("BandBin");
      removekeywords(pvlg);
      if(pvlg.hasKeyword("Center")) {
        bool hasWidth = pvlg.hasKeyword("Width");
        PvlKeyword &pvlCenter = pvlg.findKeyword("Center");
        PvlKeyword *pvlWidth = NULL;
        if(hasWidth) {
          pvlWidth = & pvlg.findKeyword("Width");
        }
        std::vector<double> centers;
        centers.resize(icube->bandCount());
        std::vector<double> widths;
        widths.resize(icube->bandCount());
        for(int i = 0; i < pvlCenter.size(); i++) {
          centers[i] = std::stod(pvlCenter[i]);
          if(hasWidth)
            widths[i] = std::stod((*pvlWidth)[i]);
          else
            widths[i] = 0.0;
        }
        compute(centers, widths, ocube);
      }
      else {
        QString message = "The BandBin in your input cube does not have a Center value.";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    else {
      QString message = "There is not a BandBin Group in the input cube.";
      throw IException(IException::User, message, _FILEINFO_);
    }
  }

  else if(ui.GetString("BANDBIN") == "USER") {
    PvlGroup pvlg;
    if(!icube->hasGroup("BandBin")) {
      pvlg = PvlGroup("BandBin");
      icube->putGroup(pvlg);
    }
    else {
      pvlg = ocube->group("BandBin");
      removekeywords(pvlg);
    }
    QString Units = "";
    PvlKeyword pvlCenter;
    if(pvlg.hasKeyword("Center")) {
      pvlCenter = pvlg.findKeyword("Center");
      Units = QString::fromStdString(pvlCenter.unit());
      pvlg.deleteKeyword("Center");
    }

    pvlCenter = PvlKeyword("Center");
    pvlCenter.setValue(ui.GetAsString("CENTER").toStdString(), Units.toStdString());
    pvlg.addKeyword(pvlCenter);
    PvlKeyword pvlWidth;
    if(pvlg.hasKeyword("Width")) {
      pvlWidth = pvlg.findKeyword("Width");
      Units = QString::fromStdString(pvlWidth.unit());
      pvlg.deleteKeyword("Width");
    }

    pvlWidth = PvlKeyword("Width");
    pvlWidth.setValue(ui.GetAsString("WIDTH").toStdString(), Units.toStdString());
    pvlg.addKeyword(pvlWidth);
    //Destroys the old and adds the new BandBin Group
    if(ocube->hasGroup("BandBin")) {
      ocube->deleteGroup("BandBin");
    }
    ocube->putGroup(pvlg);
  }

  else if(ui.GetString("BANDBIN") == "DELETE") {
    if(ocube->hasGroup("BandBin")) {
      ocube->deleteGroup("BandBin");
    }
  }

  p.StartProcess(cubeavg);
  p.EndProcess();
}

// Band processing routine
void cubeavg(vector<Buffer *> &in, vector<Buffer *> &out) {
  Statistics sts;
  sts.AddData((*in[0]).DoubleBuffer() , (*in[0]).size());
  (*out[0]) = sts.Average();
}

/**
 * Removes the PvlKeywords that can't be processed
 *
 * @param pvlg the group from which the keywords are removed
 */
void removekeywords(PvlGroup &pvlg) {
  if(pvlg.hasKeyword("OriginalBand")) {
    pvlg.deleteKeyword("OriginalBand");
  }
  if(pvlg.hasKeyword("Name")) {
    pvlg.deleteKeyword("Name");
  }
}

//BandBin Computeing
void compute(vector<double> centers, vector<double> widths,
             Cube *ocube) {
  PvlGroup &pvlg = ocube->group("BandBin");
  PvlKeyword &pvlCenter = pvlg.findKeyword("Center");
  QString centerUnit = QString::fromStdString(pvlCenter.unit());
  bool hasWidth  = pvlg.hasKeyword("Width");
  double large = centers[0] + widths[0] / 2;
  double small = centers[0] - widths[0] / 2;
  for(int i = 1; i < pvlCenter.size(); i++) {
    if(large < (double)centers[i] + (double)widths[i] / 2.0) {
      large = (double)centers[i] + (double)widths[i] / 2.0;
    }
    if(small > (double)centers[i] - (double)widths[i] / 2.0) {
      small = (double)centers[i] - (double)widths[i] / 2.0;
    }
  }
  pvlCenter.setValue(std::to_string((large - small) / 2 + small), centerUnit.toStdString());
  if(hasWidth) {
    PvlKeyword &pvlWidth  = pvlg.findKeyword("Width");
    pvlWidth.setValue(std::to_string(large - small), pvlWidth.unit());
  }
  else {
    PvlKeyword pvlWidth = PvlKeyword("Width");
    pvlWidth.setValue(std::to_string(large - small), centerUnit.toStdString());
    pvlg.addKeyword(pvlWidth);
  }

}
