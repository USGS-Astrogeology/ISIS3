/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AlphaCube.h"

#include "Cube.h"

using namespace std;

namespace Isis {
  /**
   * Constructs an AlphaCube object using a PVL object.
   * 
   * @param cube The cube to write to
   */
  AlphaCube::AlphaCube(Cube &cube) {
    Isis::PvlObject &isiscube = cube.label()->findObject("IsisCube");
    if(isiscube.hasGroup("AlphaCube")) {
      Isis::PvlGroup &alpha = isiscube.findGroup("AlphaCube");
      p_alphaSamples        = alpha["AlphaSamples"];
      p_alphaLines          = alpha["AlphaLines"];
      p_alphaStartingSample = alpha["AlphaStartingSample"];
      p_alphaStartingLine   = alpha["AlphaStartingLine"];
      p_alphaEndingSample   = alpha["AlphaEndingSample"];
      p_alphaEndingLine     = alpha["AlphaEndingLine"];
      p_betaSamples         = alpha["BetaSamples"];
      p_betaLines           = alpha["BetaLines"];
    }
    else {
      p_alphaSamples        = cube.sampleCount();
      p_alphaLines          = cube.lineCount();
      p_alphaStartingSample = 0.5;
      p_alphaStartingLine   = 0.5;
      p_alphaEndingSample   = (double) p_alphaSamples + 0.5;
      p_alphaEndingLine     = (double) p_alphaLines + 0.5;
      p_betaSamples         = p_alphaSamples;
      p_betaLines           = p_alphaLines;
    }

    ComputeSlope();
  }

  /** 
   * Constructs an AlphaCube object with a basic mapping from corner-to-corner,
   * beta 0.5,0.5 maps to alpha 0.5,0.5 and beta ns+0.5,nl+0.5 maps to alpha
   * ns+0.5, nl+0.5.
   * 
   * @param alphaSamples Number of alpha samples in the cube
   * @param alphaLines Number of alpha lines in the cube
   * @param betaSamples Number of beta samples in the cube
   * @param betaLines Number of beta lines in the cube
   * @param alphaSs Starting alpha sample
   * @param alphaSl Starting alpha line
   * @param alphaEs Ending alpha sample
   * @param alphaEl Ending alpha line
   */
  AlphaCube::AlphaCube(int alphaSamples, int alphaLines,
                       int betaSamples, int betaLines,
                       double alphaSs, double alphaSl,
                       double alphaEs, double alphaEl) {
    p_alphaSamples        = alphaSamples;
    p_alphaLines          = alphaLines;
    p_alphaStartingSample = alphaSs;
    p_alphaStartingLine   = alphaSl;
    p_alphaEndingSample   = alphaEs;
    p_alphaEndingLine     = alphaEl;

    p_betaSamples = betaSamples;
    p_betaLines = betaLines;

    ComputeSlope();
  }

  /** 
   * Constructs an AlphaCube object given alphaSamples, alphaLines,
   * betaSamples and betaLines.
   * 
   * @param alphaSamples Number of alpha samples in the cube
   * @param alphaLines Number of alpha lines in the cube
   * @param betaSamples Number of beta samples in the cube
   * @param betaLines Number of beta lines in the cube
   */
  AlphaCube::AlphaCube(int alphaSamples, int alphaLines,
                       int betaSamples, int betaLines) {
    p_alphaSamples        = alphaSamples;
    p_alphaLines          = alphaLines;
    p_alphaStartingSample = 0.5;
    p_alphaStartingLine   = 0.5;
    p_alphaEndingSample   = (double) alphaSamples + 0.5;
    p_alphaEndingLine     = (double) alphaLines + 0.5;

    p_betaSamples = betaSamples;
    p_betaLines = betaLines;

    ComputeSlope();
  }

  /**
   * Merges two AlphaCube objects. This facilities combinations of programs
   * crop-enlarge, crop-crop, reduce-pad, etc.
   *
   * @param add The AlphaCube object to be merged.
   */
  void AlphaCube::Rehash(AlphaCube &add) {
    double sl = AlphaLine(add.AlphaLine(0.5));
    double ss = AlphaSample(add.AlphaSample(0.5));
    double el = AlphaLine(add.AlphaLine(add.BetaLines() + 0.5));
    double es = AlphaSample(add.AlphaSample(add.BetaSamples() + 0.5));

    p_alphaStartingLine = sl;
    p_alphaStartingSample = ss;
    p_alphaEndingLine = el;
    p_alphaEndingSample = es;
    p_betaLines = add.BetaLines();
    p_betaSamples = add.BetaSamples();

    ComputeSlope();
  }

  /**
   * Writes or update the Alpha keywords (AlphaLines, AlphaSamples,
   * AlphaStartingSamples, etc) in the proper group in a PVL object. It chooses
   * first to write and AlphaGroup if the Mapping group exists.
   * If not then is will write the keywords in the Instrument group if it exists.
   * Otherwise it will write to the AlphaCube group.
   *
   * @param &cube The cube to write to.
   *
   */
  void AlphaCube::UpdateGroup(Isis::Cube &cube) {
    // If we have a mapping group we do not want to update the alpha cube
    // group as it represents the dimensions and sub-area of the raw instrument
    // cube
    Isis::PvlObject &cubeObject = cube.label()->findObject("IsisCube");
    if(cubeObject.hasGroup("Mapping")) return;

    // Add the labels to the pvl
    if(cubeObject.hasGroup("AlphaCube")) {
      AlphaCube temp(cube);
      temp.Rehash(*this);
      *this = temp;

      Isis::PvlGroup &alpha = cubeObject.findGroup("AlphaCube");
      alpha["AlphaSamples"] = Isis::toString(p_alphaSamples);
      alpha["AlphaLines"] = Isis::toString(p_alphaLines);
      alpha["AlphaStartingSample"] = Isis::toString(p_alphaStartingSample);
      alpha["AlphaStartingLine"] = Isis::toString(p_alphaStartingLine);
      alpha["AlphaEndingSample"] = Isis::toString(p_alphaEndingSample);
      alpha["AlphaEndingLine"] = Isis::toString(p_alphaEndingLine);
      alpha["BetaSamples"] = Isis::toString(p_betaSamples);
      alpha["BetaLines"] = Isis::toString(p_betaLines);
    }
    else {
      Isis::PvlGroup alpha("AlphaCube");
      alpha += Isis::PvlKeyword("AlphaSamples", Isis::toString(p_alphaSamples));
      alpha += Isis::PvlKeyword("AlphaLines", Isis::toString(p_alphaLines));
      alpha += Isis::PvlKeyword("AlphaStartingSample", Isis::toString(p_alphaStartingSample));
      alpha += Isis::PvlKeyword("AlphaStartingLine", Isis::toString(p_alphaStartingLine));
      alpha += Isis::PvlKeyword("AlphaEndingSample", Isis::toString(p_alphaEndingSample));
      alpha += Isis::PvlKeyword("AlphaEndingLine", Isis::toString(p_alphaEndingLine));
      alpha += Isis::PvlKeyword("BetaSamples", Isis::toString(p_betaSamples));
      alpha += Isis::PvlKeyword("BetaLines", Isis::toString(p_betaLines));
      cubeObject.addGroup(alpha);
    }
  }

  /**
   * Computes the line and sample slopes. (Starting and ending
   * alpha lines/samples divided by the number of beta lines/samples).
   *
   * @internal
   * @todo Why the +0.5 and -0.5?
   */
  void AlphaCube::ComputeSlope() {
    p_lineSlope = double(p_alphaEndingLine - p_alphaStartingLine) /
                  double((p_betaLines + 0.5)       - 0.5);
    p_sampSlope = double(p_alphaEndingSample - p_alphaStartingSample) /
                  double((p_betaSamples + 0.5)       - 0.5);
  }
}
