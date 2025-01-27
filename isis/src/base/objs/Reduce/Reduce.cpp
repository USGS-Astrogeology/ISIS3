/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Reduce.h"
#include "IString.h"
#include "SpecialPixel.h"
#include "SubArea.h"

using namespace std;

namespace Isis {
  /**
   * Reduce Constructor
   * 
   * @author Sharmila Prasad (4/26/2011)
   * 
   * @param pInCube     - Input cube
   * @param psBands     - Bands list
   * @param sampleScale - Sample scale
   * @param lineScale   - Line scale
   *  
   * @internal 
   *   @history 2013-01-16  Tracie Sucharski - Use rounding instead of ceil function to prevent
   *                           errors that were caused by round off.  The reduce application
   *                           would get ons, onl from the user, calculate scale and pass the scale
   *                           in to this class, which would calculate ons, onl, which could be
   *                           different from the user entered values.
   */
  Reduce::Reduce(Isis::Cube *pInCube, const double sampleScale, const double lineScale)
  {
    // Input Cube
    mInCube = pInCube;
    
    mdLine      = 1;
    miBandIndex = 1;
    // Set input image area to defaults
    miStartSample = 1;
    miEndSample   = mInCube->sampleCount();
    miStartLine   = 1;
    miEndLine     = mInCube->lineCount();
  
    miInputSamples= mInCube->sampleCount();
    miInputLines  = mInCube->lineCount();
    miInputBands  = mInCube->bandCount();
      
    // Save off the sample and mdLine magnification
    mdSampleScale = sampleScale;
    mdLineScale   = lineScale;

    // Calculate output size based on the sample and line scales
    miOutputSamples = (int)((double)miInputSamples / mdSampleScale + 0.5);
    miOutputLines   = (int)((double)miInputLines / mdLineScale + 0.5);
    
    // Initialize the input portal
    m_iPortal = new Isis::Portal(miInputSamples, 1, mInCube->pixelType());
  }
  
  /**
   * Destructor
   * 
   * @author Sharmila Prasad (5/11/2011)
   */
  Reduce::~Reduce(){
    delete(m_iPortal);
  }
  
  /**
   * Parameters to input image sub area to be reduced
   * 
   * @author Sharmila Prasad (5/11/2011)
   * 
   * @param startSample - input image start sample
   * @param endSample   - input image end sample
   * @param startLine   - input image start line
   * @param endLine     - input image end line 
   *  
   * @internal 
   *   @history 2013-01-16  Tracie Sucharski - Use rounding instead of ceil function to prevent
   *                           errors that were caused by round off.  The reduce application
   *                           would get ons, onl from the user, calculate scale and pass the scale
   *                           in to this class, which would calculate ons, onl, which could be
   *                           different from the user entered values.
   */
  void Reduce::setInputBoundary(int startSample, int endSample, int startLine, int endLine){
    miStartSample  = startSample;
    miEndSample    = endSample;
    miInputSamples = endSample - startSample + 1;

    miStartLine  = startLine;
    miEndLine    = endLine;
    miInputLines = endLine - startLine + 1;
    mdLine       = miStartLine;  

    // Calculate output size based on the sample and line scales
    miOutputSamples = (int)((double)miInputSamples / mdSampleScale + 0.5);
    miOutputLines   = (int)((double)miInputLines / mdLineScale + 0.5);
  }
  
  /**
   * Update the Mapping, Instrument, and AlphaCube groups in the output
   * cube label
   * 
   * @param pOutCube  - Resulting reduced output cube
   * @param pResultsGrp - This is the Results group that will go into the application
   *                 log file. This group must be created by the calling application.
   *                 Information will be added to it if the Mapping or Instrument
   *                 groups are deleted from the output image label
   */
  Isis::PvlGroup Reduce::UpdateOutputLabel(Isis::Cube *pOutCube)
  {
    // Construct a label with the results
    // This is the Results group that will go into the application
    // log file. This group must be created by the calling application.
    // Information will be added to it if the Mapping or Instrument
    // groups are deleted from the output image label
    PvlGroup resultsGrp("Results");
    resultsGrp += PvlKeyword("InputLines",      toString(miInputLines));
    resultsGrp += PvlKeyword("InputSamples",    toString(miInputSamples));
    resultsGrp += PvlKeyword("StartingLine",    toString(miStartLine));
    resultsGrp += PvlKeyword("StartingSample",  toString(miStartSample));
    resultsGrp += PvlKeyword("EndingLine",      toString(miEndLine));
    resultsGrp += PvlKeyword("EndingSample",    toString(miEndSample));
    resultsGrp += PvlKeyword("LineIncrement",   toString(mdLineScale));
    resultsGrp += PvlKeyword("SampleIncrement", toString(mdSampleScale));
    resultsGrp += PvlKeyword("OutputLines",     toString(miOutputLines));
    resultsGrp += PvlKeyword("OutputSamples",   toString(miOutputSamples));
   
    Isis::SubArea subArea;
    subArea.SetSubArea(mInCube->lineCount(), mInCube->sampleCount(), miStartLine, miStartSample, 
                       miEndLine, miEndSample, mdLineScale, mdSampleScale);
    subArea.UpdateLabel(mInCube, pOutCube, resultsGrp);
    
    return resultsGrp;
  }
  
  /**
   * Near Operator () overload, parameter for StartProcessInPlace 
   * refer ProcessByLine, ProcessByBrick 
   * 
   * @param out - output buffer 
   */
  void Nearest::operator()(Isis::Buffer & out) const
  {
    int readLine = (int)(mdLine + 0.5);

    m_iPortal->SetPosition(miStartSample, readLine, miBandIndex);
    mInCube->read(*m_iPortal);
    
    // Scale down buffer
    for(int os = 0; os < miOutputSamples; os++) {
      out[os] = (*m_iPortal)[(int)((double) os * mdSampleScale)];
    }

    if(out.Line() == miOutputLines) {
      miBandIndex++;
      mdLine = 1;
    }
    else {
      mdLine += mdLineScale;
    }
  }
  
  /**
   * Average Operator () overload, parameter for StartProcessInPlace 
   * refer ProcessByLine, ProcessByBrick 
   * 
   * @param out - output buffer
   */
  void Average::operator() (Isis::Buffer & out) const
  {
    // Index of the last input line that corresponds to this output line.
    double rline = (double)out.Line() * mdLineScale;

    if(out.Line() == 1 && out.Band() == 1) {
      // Index of the last input sample that corresponds to the output sample.
      mdIncTab = new double[miOutputSamples];
      mdSum    = new double[miOutputSamples];
      mdNpts   = new double[miOutputSamples];
      mdSum2   = new double[miOutputSamples];
      mdNpts2  = new double[miOutputSamples];

      //  Fill mdIncTab and Initialize buffers for first band
      for(int osamp = 0; osamp < miOutputSamples; osamp++) {
        mdIncTab[osamp] = ((double)osamp + 1.) * mdSampleScale;
        mdSum[osamp]   = 0.0;
        mdNpts[osamp]  = 0.0;
        mdSum2[osamp]  = 0.0;
        mdNpts2[osamp] = 0.0;
      }
      mdIncTab[miOutputSamples-1] = miInputSamples;
    }

    unordered_map<QString, int> specialPixelCounts[miOutputSamples];

    while(mdLine < rline) {
      if((int)mdLine <= miInputLines) {
        m_iPortal->SetPosition(miStartSample, mdLine, miBandIndex);
        mInCube->read(*m_iPortal);
      }
      int isamp = 1;
      for(int osamp = 0; osamp < out.size(); osamp++) {
        while((double)isamp <= mdIncTab[osamp]) {
          // If Pixel is valid add it to mdSum
          if(IsValidPixel((*m_iPortal)[isamp-1])) {
            mdSum[osamp]  += (*m_iPortal)[isamp-1];
            mdNpts[osamp] += 1.0;
          }
          if (IsSpecial((*m_iPortal)[isamp-1])) {
            specialPixelCounts[osamp][PixelToString((*m_iPortal)[isamp-1])]++;
          } 
          isamp++;
        }

        double sdel = (double) isamp - mdIncTab[osamp];
        if(isamp > miInputSamples) continue;

        if(IsValidPixel((*m_iPortal)[isamp-1])) {
          mdSum[osamp]  += (*m_iPortal)[isamp-1] * (1.0 - sdel);
          mdNpts[osamp] += (1.0 - sdel);
          if(osamp + 1 < miOutputSamples) {
            mdSum[osamp+1]  += (*m_iPortal)[isamp-1] * sdel;
            mdNpts[osamp+1] += sdel;
          }
        }
        if (IsSpecial((*m_iPortal)[isamp-1])) {
          specialPixelCounts[osamp][PixelToString((*m_iPortal)[isamp-1])]++;
          if(osamp+1 <= miOutputSamples){
            specialPixelCounts[osamp+1][PixelToString((*m_iPortal)[isamp-1])]++;
          }
        } 
        isamp++;
      }
      mdLine++;
    }

    if(mdLine <= miInputLines) {
      m_iPortal->SetPosition(miStartSample, mdLine, miBandIndex);
      mInCube->read(*m_iPortal);
    }
    double ldel = (double)mdLine - rline;
    double ldel2 = 1.0 - ldel;
    int isamp = 1;
    for(int osamp = 0; osamp < miOutputSamples; osamp++) {
      while(isamp <= mdIncTab[osamp]) {
        if(IsValidPixel((*m_iPortal)[isamp-1])) {
          mdSum[osamp]   += (*m_iPortal)[isamp-1] * ldel2;
          mdNpts[osamp]  += ldel2;
          mdSum2[osamp]  += (*m_iPortal)[isamp-1] * ldel;
          mdNpts2[osamp] += ldel;
        }
        if (IsSpecial((*m_iPortal)[isamp-1])) {
          specialPixelCounts[osamp][PixelToString((*m_iPortal)[isamp-1])]++;
        } 
        isamp++;
      }

      double sdel = (double) isamp - mdIncTab[osamp];
      if(isamp > miInputSamples) continue;
      if(IsValidPixel((*m_iPortal)[isamp-1])) {
        mdSum[osamp]  += (*m_iPortal)[isamp-1] * (1.0 - sdel) * ldel2;
        mdNpts[osamp] += (1.0 - sdel) * ldel2;
        if(osamp + 1 < miOutputSamples) {
          mdSum[osamp+1]  += (*m_iPortal)[isamp-1] * sdel * ldel2;
          mdNpts[osamp+1] += sdel * ldel2;
        }
        mdSum2[osamp]  += (*m_iPortal)[isamp-1] * (1.0 - sdel) * ldel;
        mdNpts2[osamp] += (1.0 - sdel) * ldel;
        if(osamp + 1 < miOutputSamples) {
          mdSum2[osamp+1]  += (*m_iPortal)[isamp-1] * sdel * ldel;
          mdNpts2[osamp+1] += sdel * ldel;
        }
      }
      if (IsSpecial((*m_iPortal)[isamp-1])) {
        specialPixelCounts[osamp][PixelToString((*m_iPortal)[isamp-1])]++;
        if(osamp+1 <= miOutputSamples){
          specialPixelCounts[osamp+1][PixelToString((*m_iPortal)[isamp-1])]++;
        }
      } 
      isamp++;
    }

    if(mdLine < miInputLines) mdLine++;

    double npix = mdSampleScale * mdLineScale;
    for(int osamp = 0; osamp < miOutputSamples; osamp++) {
      if(mdNpts[osamp] > npix * mdValidPer) {
        out[osamp] = mdSum[osamp] / mdNpts[osamp];
      }
      else {
        if(msReplaceMode == "NEAREST") {
          out[osamp] = (*m_iPortal)[(int)(mdIncTab[osamp] + 0.5) - 1];
        }
        else if (msReplaceMode == "MAJORITY") {
          // Iterate over the map and compare element counts
          // Map iterator yields a pair:  pair.first == pixel names, pair.second == pixel count.
          auto mode = std::max_element(specialPixelCounts[osamp].begin(),
                                      specialPixelCounts[osamp].end(),
                                      [](const pair<QString, int> &a, const pair<QString, int> &b) {return a.second < b.second;});
          out[osamp] = StringToPixel(mode->first);
        }
        else {
          out[osamp] = Isis::Null;
        }
      }
      mdSum[osamp]   = mdSum2[osamp];
      mdNpts[osamp]  = mdNpts2[osamp];
      mdSum2[osamp]  = 0.0;
      mdNpts2[osamp] = 0.0;
    }

    // If this is the last line of the band, reset stats and increment band
    if(out.Line() == miOutputLines && out.Band() != miInputBands) {
      miBandIndex++;
      mdLine = 1;
      for(int osamp = 0; osamp < miOutputSamples; osamp++) {
        mdSum[osamp]   = 0.0;
        mdNpts[osamp]  = 0.0;
        mdSum2[osamp]  = 0.0;
        mdNpts2[osamp] = 0.0;
      }
    }

    if(out.Line() == miOutputLines && out.Band() == miInputBands) {
      delete [] mdIncTab;
      delete [] mdSum;
      delete [] mdNpts;
      delete [] mdSum2;
      delete [] mdNpts2;
    }
  } 
}
