#include "Isis.h"
#include "tnt_array2d.h"
#include "PrincipalComponentAnalysis.h"
#include "ProcessBySpectra.h"
#include "Statistics.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace std;
using namespace Isis;

void PCA(Buffer &in);
void Transform(Buffer &in, Buffer &out);
void Inverse(Buffer &in, Buffer &out);

PrincipalComponentAnalysis pca(0);

int numDimensions;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  ProcessByBrick p;
  Cube *icube = p.SetInputCube("FROM");
  p.SetBrickSize(128, 128, icube->bandCount());

  // The output cube with no attributes and real pixel type
  Isis::CubeAttributeOutput cao;
  cao.setPixelType(Isis::Real);

  // Start the sample processing
  if(ui.GetString("MODE") == "TRANSFORM") {
    Cube *ocube = p.SetOutputCube(ui.GetAsString("TO"), cao, icube->sampleCount(), icube->lineCount(), icube->bandCount());
    numDimensions = icube->bandCount();
    pca = Isis::PrincipalComponentAnalysis(numDimensions);
    ProcessByBrick p2;
    p2.SetBrickSize(128, 128, icube->bandCount());
    p2.SetInputCube("FROM");
    p2.Progress()->SetText("Computing Transform");
    p2.StartProcess(PCA);
    p2.EndProcess();
    pca.ComputeTransform();
    TNT::Array2D<double> transform = pca.TransformMatrix();

    // Add a table to the output cube that contains the transform matrix
    //   This allows us to invert the cube back from pc space
    Isis::TableField field("Columns", Isis::TableField::Double, transform.dim2());
    Isis::TableRecord record;
    record += field;
    Isis::Table table("Transform Matrix", record);
    for(int i = 0; i < transform.dim1(); i++) {
      std::vector<double> row;
      for(int j = 0; j < transform.dim2(); j++) {
        row.push_back(transform[i][j]);
      }
      record[0] = row;
      table += record;
    }

    p.Progress()->SetText("Transforming Cube");
    p.StartProcess(Transform);
    ocube->write(table);
    p.EndProcess();
  }
  else if(ui.GetString("MODE") == "INVERSE") {
    if(!(icube->hasTable("Transform Matrix"))) {
      std::string m = "The input cube has not been transformed into its principal components";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }
    Isis::Table table = icube->readTable("Transform Matrix");
    numDimensions = table.Records();
    TNT::Array2D<double> transform(numDimensions, numDimensions);
    for(int i = 0; i < numDimensions; i++) {
      std::vector<double> row = table[i]["Columns"];
      for(int j = 0; j < numDimensions; j++) {
        transform[i][j] = row[j];
      }
    }

    pca = Isis::PrincipalComponentAnalysis(transform);
    Cube *ocube = p.SetOutputCube(ui.GetAsString("TO"), cao, icube->sampleCount(), icube->lineCount(), numDimensions);
    Pvl *label = ocube->label();
    // remove the transform matrix table from the cube
    for(int i = 0; i < label->objects(); i++) {
      if(label->object(i).hasKeyword("Name")
          && label->object(i)["Name"].isEquivalent("Transform Matrix")) label->deleteObject(i);
    }
    p.Progress()->SetText("Inverting Cube");
    p.StartProcess(Inverse);
    p.EndProcess();
  }
  else {
    std::string m = "Invalid option for MODE [" + ui.GetString("MODE") + "]";
    throw IException(IException::Programmer, m, _FILEINFO_);
  }
}

// Processing routine for the pca with one input cube
void Transform(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.SampleDimension(); i++) {
    for(int j = 0; j < in.LineDimension(); j++) {
      TNT::Array2D<double> pre(1, in.BandDimension());
      for(int k = 0; k < pre.dim2(); k++) {
        int index = i + j * in.SampleDimension() + k * in.SampleDimension() * in.LineDimension();
        pre[0][k] = in[index];
      }

      TNT::Array2D<double> post = pca.Transform(pre);

      for(int k = 0; k < post.dim2(); k++) {
        int index = i + j * in.SampleDimension() + k * in.SampleDimension() * in.LineDimension();
        out[index] = post[0][k];
      }
    }
  }
}

// Processing routine for the pca with two input cubes
void Inverse(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.SampleDimension(); i++) {
    for(int j = 0; j < in.LineDimension(); j++) {
      TNT::Array2D<double> pre(1, in.BandDimension());
      for(int k = 0; k < pre.dim2(); k++) {
        int index = i + j * in.SampleDimension() + k * in.SampleDimension() * in.LineDimension();
        if(k + 1 > in.size()) pre[0][k] = 0.0;
        else pre[0][k] = in[index];
      }

      TNT::Array2D<double> post = pca.Inverse(pre);

      for(int k = 0; k < post.dim2(); k++) {
        int index = i + j * in.SampleDimension() + k * in.SampleDimension() * in.LineDimension();
        out[index] = post[0][k];
      }
    }
  }
}

// Analyze the cube and compute the principal components
void PCA(Buffer &in) {
  pca.AddData(in.DoubleBuffer(), in.size() / in.BandDimension());
}
