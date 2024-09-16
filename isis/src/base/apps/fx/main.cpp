/**
 * @file
 * $Revision: 1.8 $
 * $Date: 2010/04/08 15:28:20 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are public
 *   domain. See individual third-party library and package descriptions for
 *   intellectual property information,user agreements, and related information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or implied,
 *   is made by the USGS as to the accuracy and functioning of such software
 *   and related material nor shall the fact of distribution constitute any such
 *   warranty, and no responsibility is assumed by the USGS in connection
 *   therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html in a browser or see
 *   the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "Isis.h"

#include "Camera.h"
#include "CubeCalculator.h"
#include "CubeInfixToPostfix.h"
#include "FileList.h"
#include "FileName.h"
#include "ProcessByLine.h"

using namespace std;
using namespace Isis;

Isis::CubeCalculator c;

// Prototypes
void Evaluate(vector<Buffer *> &input, vector<Buffer *> &output);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  ProcessByLine p;
  QVector<Cube *> cubes;
  Cube *outCube;
  Cube *inCube;
  int bands = 1;

  if (ui.GetString("MODE") == "CUBES") {
    // Require atleast one file to be specified
    inCube = p.SetInputCube("F1", Isis::AllMatchOrOne);
    cubes.push_back(inCube);
    if (ui.WasEntered("F2")){
        inCube = p.SetInputCube("F2", Isis::AllMatchOrOne);
        cubes.push_back(inCube);
    }
    if (ui.WasEntered("F3")){
        inCube = p.SetInputCube("F3", Isis::AllMatchOrOne);
        cubes.push_back(inCube);
    }
    if (ui.WasEntered("F4")){
        inCube = p.SetInputCube("F4", Isis::AllMatchOrOne);
        cubes.push_back(inCube);
    }
    if (ui.WasEntered("F5")){
        inCube = p.SetInputCube("F5", Isis::AllMatchOrOne);
        cubes.push_back(inCube);
    }
    bands = cubes[0]->bandCount();
    outCube = p.SetOutputCube("TO");
  }
  else if (ui.GetString("MODE") == "LIST") {
    FileList list(ui.GetFileName("FROMLIST").toStdString());

    // Run through file list and set its entries as input cubes
    for (int i = 0; i < list.size(); i++) {
      CubeAttributeInput att(list[i].original());
      inCube = p.SetInputCube(QString::fromStdString(list[i].original()), att, Isis::AllMatchOrOne);
      cubes.push_back(inCube);
    }
    bands = cubes[0]->bandCount();
    outCube = p.SetOutputCube("TO");
  }
  else {
    int lines = ui.GetInteger("LINES");
    int samples = ui.GetInteger("SAMPLES");
    bands = ui.GetInteger("BANDS");
    outCube = p.SetOutputCube("TO", samples, lines, bands);
  }

  CubeInfixToPostfix infixToPostfix;
  c.prepareCalculations(infixToPostfix.convert(ui.GetString("EQUATION")), cubes, outCube);
  p.StartProcess(Evaluate);
  p.EndProcess();
}


/**
 * Take in the input buffer, apply the user-defined equation to
 * it, then write the results to the output buffer
 *
 * @param input The input buffer vector
 * @param output The output buffer
 */
void Evaluate(vector<Buffer *> &input, vector<Buffer *> &output) {
  Buffer &outBuffer = *output[0];

  QVector<Buffer *> inputCopy;

  for (int i = 0; i < (int)input.size(); i++) {
    inputCopy.push_back(input[i]);
  }

  QVector<double> results = c.runCalculations(inputCopy,
                            outBuffer.Line(), outBuffer.Band());

  // If final result is a scalar, set all pixels to that value.
  if (results.size() == 1) {
    for (int i = 0; i < (int)outBuffer.size(); i++) {
      outBuffer[i] = results[0];
    }
  }
  // Final result is a valid vector, write to output buffer
  else {
    for (int i = 0; i < (int)results.size(); i++) {
      outBuffer[i] = results[i];
    }
  }

}

