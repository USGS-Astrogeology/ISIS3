/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Enlarge.h"

#include <cmath>

#include "Cube.h"
#include "IException.h"
#include "PvlGroup.h"
#include "SubArea.h"

using namespace std;

namespace Isis {

  /**
   * Constructs an Enlarge object.
   *
   * @param pInCube       - Input cube to be enlarged
   * @param sampleScale   - Sample scale
   * @param lineScale     - Line scale
   */
  Enlarge::Enlarge(Cube *pInCube, const double sampleScale,
                   const double lineScale) {
    // Input Cube
    mInCube = pInCube;

    // Set input image area to defaults
    mdStartSample = 1;
    mdEndSample   = mInCube->sampleCount();
    mdStartLine   = 1;
    mdEndLine     = mInCube->lineCount();

    // Save off the sample and line magnification
    mdSampleScale = sampleScale;
    mdLineScale   = lineScale;

    miOutputSamples = (int)ceil(mInCube->sampleCount() * mdSampleScale);
    miOutputLines   = (int)ceil(mInCube->lineCount() * mdLineScale);
  }

  /**
   * Implementations for parent's pure virtual members
   * Convert the requested output samp/line to an input samp/line
   *
   * @param inSample  - Calculated input sample corresponding to output sample
   * @param inLine    - Calculated input line corresponding to output line
   * @param outSample - Output sample
   * @param outLine   - Output line
   *
   * @return bool
   */
  bool Enlarge::Xform(double &inSample, double &inLine,
                      const double outSample, const double outLine) {
    inSample = (outSample - 0.5) / mdSampleScale + 0.5 + (mdStartSample - 1);
    inLine   = (outLine - 0.5) / mdLineScale + 0.5 + (mdStartLine - 1);

    return true;
  }

  /**
   * Sets the sub area dimensions of the input image.
   * Default is the entire image
   *
   * @author Sharmila Prasad (4/14/2011)
   *
   * @param pdStartSample - Input start sample
   * @param pdEndSample   - Input end sample
   * @param pdStartLine   - Input start line
   * @param pdEndLine     - Input end line
   */
  void Enlarge::SetInputArea(double pdStartSample, double pdEndSample,
                             double pdStartLine, double pdEndLine) {
    // Check for the right image dimensions
    if (pdStartSample > pdEndSample || pdStartLine > pdEndLine) {
      string sErrMsg = "Error in Input Area Dimesions";
      throw IException(IException::Programmer, sErrMsg, _FILEINFO_);
    }

    if (pdStartSample >= 1) {
      mdStartSample = pdStartSample;
    }
    if (pdEndSample <= mInCube->sampleCount()) {
      mdEndSample = pdEndSample;
    }
    if (pdStartLine >= 1) {
      mdStartLine = pdStartLine;
    }
    if (pdEndLine <= mInCube->lineCount()) {
      mdEndLine = pdEndLine;
    }

    miOutputSamples = (int)ceil((mdEndSample - mdStartSample + 1) * mdSampleScale);
    miOutputLines   = (int)ceil((mdEndLine - mdStartLine + 1) * mdLineScale);
  }

  /**
   * Update the Mapping, Instrument, and AlphaCube groups in the output
   * cube label
   *
   * @param pOutCube  - Resulting enlarged output cube
   *
   * @return @b PvlGroup - This is the Results group that
   *                 will go into the application log file. This group
   *                 must be created by the calling application.
   *                 Information will be added to it if the Mapping or
   *                 Instrument groups are deleted from the output
   *                 image label
   */
  PvlGroup Enlarge::UpdateOutputLabel(Cube *pOutCube) {
    int iNumSamples= mInCube->sampleCount();
    int iNumLines  = mInCube->lineCount();
    // Construct a label with the results
    // This is the Results group that will go into the application
    // log file. This group must be created by the calling application.
    // Information will be added to it if the Mapping or Instrument
    // groups are deleted from the output image label
    PvlGroup resultsGrp("Results");
    resultsGrp += PvlKeyword("InputLines",      std::to_string(iNumLines));
    resultsGrp += PvlKeyword("InputSamples",    std::to_string(iNumSamples));
    resultsGrp += PvlKeyword("StartingLine",    std::to_string((int)mdStartLine));
    resultsGrp += PvlKeyword("StartingSample",  std::to_string((int)mdStartSample));
    resultsGrp += PvlKeyword("EndingLine",      std::to_string((int)mdEndLine));
    resultsGrp += PvlKeyword("EndingSample",    std::to_string((int)mdEndSample));
    resultsGrp += PvlKeyword("LineIncrement",   std::to_string(1. / mdLineScale));
    resultsGrp += PvlKeyword("SampleIncrement", std::to_string(1. / mdSampleScale));
    resultsGrp += PvlKeyword("OutputLines",     std::to_string(miOutputLines));
    resultsGrp += PvlKeyword("OutputSamples",   std::to_string(miOutputSamples));

    SubArea subArea;
    subArea.SetSubArea(mInCube->lineCount(), mInCube->sampleCount(), (int)mdStartLine, (int)mdStartSample,
                       (int)mdEndLine, (int)mdEndSample, 1.0 / mdLineScale, 1.0 / mdSampleScale);
    subArea.UpdateLabel(mInCube, pOutCube, resultsGrp);

    return resultsGrp;
  }
}


