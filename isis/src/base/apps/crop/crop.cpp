#include <cmath>

#include "crop.h"

using namespace std;

namespace Isis {



  /**
   * Crop a cube along a line, sample range. This is the programmatic interface to
   * the ISIS stats application.
   *
   * @param ui The User Interface to parse the parameters from
   */
  PvlGroup crop(UserInterface &ui) {
    Cube *icube = new Cube();
    icube->open(ui.GetCubeName("FROM"));
    return crop(icube, ui);
  }


  /**
   * Compute the stats for an ISIS cube. This is the programmatic interface to
   * the ISIS stats application.
   *
   * @param cube input cube to be cropped
   * @param ui The User Interface to parse the parameters from
   */
  PvlGroup crop(Cube *cube, UserInterface &ui) {
    // Globals and prototypes
    int ss, sl, sb;
    int ns, nl, nb;
    int sinc, linc;

    LineManager *in = NULL;

    // Line processing routine
    auto cropProccess = [&](Buffer &out)->void {
      // Read the input line
      int iline = sl + (out.Line() - 1) * linc;
      in->SetLine(iline, sb);
      cube->read(*in);

      // Loop and move appropriate samples
      for(int i = 0; i < out.size(); i++) {
        out[i] = (*in)[(ss - 1) + i * sinc];
      }

      if(out.Line() == nl) sb++;
    };

    ProcessByLine p;

    // Open the input cube
    QString from = ui.GetAsString("FROM");
    CubeAttributeInput inAtt(from.toStdString());
    cube = new Cube();
    cube->setVirtualBands(inAtt.bands());
    from = ui.GetCubeName("FROM");
    cube->open(from);

    // Determine the sub-area to extract
    ss = ui.GetInteger("SAMPLE");
    sl = ui.GetInteger("LINE");
    sb = 1;

    int origns = cube->sampleCount();
    int orignl = cube->lineCount();
    int es = cube->sampleCount();
    if (ui.WasEntered("NSAMPLES")) es = ss + ui.GetInteger("NSAMPLES") - 1;
    int el = cube->lineCount();
    if (ui.WasEntered("NLINES")) el = sl + ui.GetInteger("NLINES") - 1;
    int eb = cube->bandCount();

    sinc = ui.GetInteger("SINC");
    linc = ui.GetInteger("LINC");

    // Make sure starting positions fall within the cube
    if (ss > cube->sampleCount()) {
      cube->close();
      std::string msg = "[SAMPLE] exceeds number of samples in the [FROM] cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (sl > cube->lineCount()) {
      cube->close();
      std::string msg = "[LINE] exceeds number of lines in the [FROM] cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Make sure the number of elements do not fall outside the cube
    if (es > cube->sampleCount()) {
      cube->close();
      std::string msg = "[SAMPLE+NSAMPLES-1] exceeds number of ";
      msg += "samples in the [FROM] cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (el > cube->lineCount()) {
      cube->close();
      std::string msg = "[LINE+NLINES-1] exceeds number of ";
      msg += "lines in the [FROM] cube";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Determine the size of the output cube and then set the output image size
    ns = ceil((double)(es - ss + 1) / sinc);
    nl = ceil((double)(el - sl + 1) / linc);
    nb = eb;
    //if (ns == 0) ns = 1;
    //if (nl == 0) nl = 1;
    es = ss + (ns - 1) * sinc;
    el = sl + (nl - 1) * linc;

    // Allocate the output file and make sure things get propogated nicely
    CubeAttributeInput &inputAtt =ui.GetInputAttribute("FROM");
    p.SetInputCube(ui.GetCubeName("FROM"), inputAtt);
    CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), att, ns, nl, nb);
    p.PropagateTables(false);
    p.ClearInputCubes();

    // propagate tables manually
    Pvl &inLabels = *cube->label();

    // Loop through the labels looking for object = Table
    for(int labelObj = 0; labelObj < inLabels.objects(); labelObj++) {
      PvlObject &obj = inLabels.object(labelObj);

      if(obj.name() != "Table") continue;

      // If we're not propagating spice data, dont propagate the following tables...
      if(!ui.GetBoolean("PROPSPICE")) {
        if((IString)obj["Name"][0] == "InstrumentPointing") continue;
        if((IString)obj["Name"][0] == "InstrumentPosition") continue;
        if((IString)obj["Name"][0] == "BodyRotation") continue;
        if((IString)obj["Name"][0] == "SunPosition") continue;
      }

      // Read the table into a table object
      Table table(obj["Name"], from.toStdString());

      // We are not going to bother with line/sample associations; they apply
      //   only to the alpha cube at this time. I'm leaving this code here for the
      //   equations in case we try our hand at modifying these tables at a later date.

      /* Deal with associations, sample first
      if(table.IsSampleAssociated()) {
        int numDeleted = 0;
        for(int samp = 0; samp < cube->sampleCount(); samp++) {
          // This tests checks to see if we would include this sample.
          //   samp - (ss-1)) / sinc must be a whole number less than ns.
          if((samp - (ss-1)) % sinc != 0 || (samp - (ss-1)) / sinc >= ns || (samp - (ss-1)) < 0) {
            table.Delete(samp-numDeleted);
            numDeleted ++;
          }
        }
      }

      // Deal with line association
      if(table.IsLineAssociated()) {
        int numDeleted = 0;
        for(int line = 0; line < cube->lineCount(); line++) {
          // This tests checks to see if we would include this line.
          //   line - (sl-1)) / linc must be a whole number less than nl.
          if((line - (sl-1)) % linc != 0 || (line - (sl-1)) / linc >= nl || (line - (sl-1)) < 0) {
            table.Delete(line-numDeleted);
            numDeleted ++;
          }
        }
      }*/

      // Write the table
      ocube->write(table);
    }

    Pvl &outLabels = *ocube->label();
    if(!ui.GetBoolean("PROPSPICE") && outLabels.findObject("IsisCube").hasGroup("Kernels")) {
      PvlGroup &kerns = outLabels.findObject("IsisCube").findGroup("Kernels");

      QString tryKey = "NaifIkCode";
      if(kerns.hasKeyword("NaifFrameCode")) {
        tryKey = "NaifFrameCode";
      }

      if(kerns.hasKeyword(tryKey.toStdString())) {
        PvlKeyword ikCode = kerns[tryKey.toStdString()];
        kerns = PvlGroup("Kernels");
        kerns += ikCode;
      }
    }

    // Create a buffer for reading the input cube
    in = new LineManager(*cube);

    // Crop the input cube
    p.StartProcess(cropProccess);

    delete in;
    in = NULL;

    // Construct a label with the results
    PvlGroup results("Results");
    results += PvlKeyword("InputLines", Isis::toString(orignl));
    results += PvlKeyword("InputSamples", Isis::toString(origns));
    results += PvlKeyword("StartingLine", Isis::toString(sl));
    results += PvlKeyword("StartingSample", Isis::toString(ss));
    results += PvlKeyword("EndingLine", Isis::toString(el));
    results += PvlKeyword("EndingSample", Isis::toString(es));
    results += PvlKeyword("LineIncrement", Isis::toString(linc));
    results += PvlKeyword("SampleIncrement", Isis::toString(sinc));
    results += PvlKeyword("OutputLines", Isis::toString(nl));
    results += PvlKeyword("OutputSamples", Isis::toString(ns));

    // Update the Mapping, Instrument, and AlphaCube groups in the output
    // cube label
    SubArea *s;
    s = new SubArea;
    s->SetSubArea(orignl, origns, sl, ss, el, es, linc, sinc);
    s->UpdateLabel(cube, ocube, results);
    delete s;
    s = NULL;

    // Cleanup
    p.EndProcess();
    cube->close();
    delete cube;
    cube = NULL;

    // Write the results to the log
    return results;
  }
}
