#include "Isis.h"

#include <QString>
#include <vector>

#include "ProcessByLine.h"

#include "Pvl.h"
#include "MultivariateStatistics.h"

using namespace std;
using namespace Isis;

std::vector< vector<double> > covariance;
std::vector< vector<double> > correlation;
MultivariateStatistics stats;

void MakeStats(vector<Buffer *> &in, vector<Buffer *> &out);
void WriteText(int size, QString filename);
void WriteCube(Buffer &inout);

void IsisMain() {

  //Check to see if an output file was specified
  UserInterface &ui = Application::GetUserInterface();
  if(!ui.WasEntered("CUBE") && !ui.WasEntered("FLATFILE")) {
    std::string message = "At least one output file must be entered";
    throw IException(IException::User, message, _FILEINFO_);
  }

  QString file = ui.GetCubeName("FROM");

  //Use a Process to get the number of bands in the input cube
  Process q;
  Cube *icube = q.SetInputCube("FROM");
  int bands = icube->bandCount();

  //Check to see if the input cube has enough bands
  if(bands < 2) {
    std::string message = "Input band must have at least two bands!";
    throw IException(IException::User, message, _FILEINFO_);
  }

  //Set the matrices according to the number of bands in the input cube
  covariance.resize(bands);
  correlation.resize(bands);
  for(int i = 0; i < bands ; ++i) {
    covariance[i].resize(bands);
    correlation[i].resize(bands);
  }

  //Loop through the bands, systematically ensuring that
  //each band is compared against each other band.
  for(int i = 1 ; i <= bands ; ++i) {
    for(int j = i ; j <= bands ; j++) {
      //Reset Stat accumulators and set the Progress Display Text
      stats.Reset();
      QString progText = "Band " + QString::number(i) +
                        " vs. " +
                        "Band " +  QString::number(j);

      //Cube will be processed by line
      ProcessByLine p;

      //Set CubeAttributeInputs to tell the ProcessByLine which
      //bands to compare
      CubeAttributeInput band_a("d+" + Isis::toString(icube->physicalBand(i)));
      CubeAttributeInput band_b("d+" + Isis::toString(icube->physicalBand(j)));

      //Set Input files and process, to accumulate the statistics
      p.SetInputCube(file, band_a);
      p.SetInputCube(file, band_b);
      Progress *progress = p.Progress();
      progress->SetText(progText);
      p.StartProcess(MakeStats);
      p.EndProcess();

      //If the bands are the same, use the Variance of one, and a Correlation
      //of 1, for speed and simplicity
      if(i == j) {
        covariance[i-1][j-1] = stats.X().Variance();
        correlation[i-1][j-1] = 1.0;
      }
      //Otherwise, set the matrix to the appropriate value
      else {
        covariance[i-1][j-1] = stats.Covariance();
        correlation[i-1][j-1] = stats.Correlation();
      }
    }
  }
  //Mirror the matrices to create a full representation, instead
  //of half-matrices
  for(int i = 0; i < bands; ++i) {
    for(int j = 0; j < bands; ++j) {
      covariance[j][i] = covariance[i][j];
      correlation[j][i] = correlation[i][j];
    }
  }
  //Write the output file(s)
  if(ui.WasEntered("FLATFILE")) {
    WriteText(bands, ui.GetFileName("FLATFILE"));
  }
  if(ui.WasEntered("CUBE")) {
    //Set the Band Names
    PvlKeyword name("Name");
    name += "Correlation";
    name += "Covariance";
    PvlGroup bandBin("BandBin");
    bandBin += name;

    //Set up the Process and the OutputCube, and Process
    ProcessByLine p;
    CubeAttributeOutput set;
    set.setPixelType(Real);
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("CUBE"),
                                  set, bands, bands, 2);
    p.StartProcess(WriteCube);
    ocube->putGroup(bandBin);
    p.EndProcess();
  }
}

//Function to gather the data and feed them to a
//MultivariateStatistics container
void MakeStats(vector<Buffer *> &in, vector<Buffer *> &out) {
  double *x = in[0]->DoubleBuffer();
  double *y = in[1]->DoubleBuffer();

  stats.AddData(x, y, in[0]->size());
}

//Function to generate a flatfile to represent the matrices
void WriteText(int size, QString filename) {
  ofstream outputFile;
  outputFile.open(filename.toLatin1().data());
  QString line = " ";
  outputFile << "Correlation:" << endl << endl;
  for(int i = 0; i < size; ++i) {
    for(int j = 0; j < size; ++j) {
      line += " " + QString::number(correlation[i][j]) + " ";
    }
    outputFile << line.toStdString() << endl;
    line = " ";
  }

  outputFile << endl << endl << "Covariance:" << endl << endl;
  for(int i = 0; i < size; ++i) {
    for(int j = 0; j < size; ++j) {
      line += " " + QString::number(covariance[i][j]) + " ";
    }
    outputFile << line.toStdString() << endl;
    line = " ";
  }
  outputFile.close();
}

//Function to write the 2 band cube containing representions
//of the two matrices
void WriteCube(Buffer &inout) {
  if(inout.Band() == 1) {
    for(int i = 0; i < inout.size(); ++i) {
      inout[i] = correlation[inout.Line()-1][i];
    }
  }
  else if(inout.Band() == 2) {
    for(int i = 0; i < inout.size(); ++i) {
      inout[i] = covariance[inout.Line()-1][i];
    }
  }
}

