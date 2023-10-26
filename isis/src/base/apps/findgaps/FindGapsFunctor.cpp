/**
 * @file
 * $Revision: 1.19 $
 * $Date: 2010/03/22 19:44:53 $
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */

#include "FindGapsFunctor.h"

#include "Buffer.h"
#include "IString.h"
#include "MultivariateStatistics.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Statistics.h"

namespace Isis {

  
  /**
   * Constructs a findGapsFunctor, initializing all of the private member variables.
   * 
   * @param inputLineCount the number of lines in the input cube
   * @param correlationTol the correlation tolerance used to find the gaps
   * @param borderSizeBeforeGap the number of lines to set to null before each gap if an output
   *                            cube is specified.
   * @param borderSizeAfterGap the number of lines to set to null after each gap if an output
   *                            cube is specified.
   */
  FindGapsFunctor::FindGapsFunctor(int inputLineCount, double correlationTol,
                                   int borderSizeBeforeGap, int borderSizeAfterGap) {
    m_previous = NULL;
    m_inGap = NULL;
    m_gap = NULL;
    m_gaps = NULL;

    m_corTol = correlationTol;
    m_inGap = new bool(false);
    m_gap = new PvlGroup("Gap");
    m_gaps = new Pvl;
    m_gaps->addKeyword(PvlKeyword("Modification", "None"));
    m_lineCount = inputLineCount;
    m_bufferSizeBeforeGap = borderSizeBeforeGap;
    m_bufferSizeAfterGap = borderSizeAfterGap;
  }

  
  /**
   * Destructor
   */
  FindGapsFunctor::~FindGapsFunctor() {
    delete m_previous;
    m_previous = NULL;

    delete m_inGap;
    m_inGap = NULL;
    
    delete m_gap;
    m_gap = NULL;

    delete m_gaps;
    m_gaps = NULL;
  }

  
  /**
   * True if the last gap extends to the end of the cube
   * 
   * @return whether or not the line is in the gap after processing
   */
  bool FindGapsFunctor::endsInGap() {
    return *m_inGap;
  }

  
  /**
   * Accessor for the current gap
   * 
   * @return the group that contains the information of one gap
   */
  PvlGroup FindGapsFunctor::gap() {
    return *m_gap;
  }

  
  /**
   * Accessor for the pvl, the list of gaps to display
   * 
   * @return Pvl group of gaps to be displayed
   */
  Pvl FindGapsFunctor::gaps() {
    return *m_gaps;
  }

  
  /**
   * Accessor for the pvl, the list of gaps to display
   *
   * @return Pvl group of gaps to be displayed
   */
  void FindGapsFunctor::setModification(QString newModValue) {
    m_gaps->findKeyword("Modification").setValue(newModValue.toStdString());
  }

  
  /**
   * Find the gaps in the image by comparing each line with the previous line. If they do not
   * correlate, the current line is considered the start of a gap. The end of the gap is found
   * in the same way unless the gap reaches the end of the image. In that case the last line of
   * the image is the last line of the gap.
   * 
   */
  void FindGapsFunctor::operator()(Buffer &in) const {
    
    // Copys line 1 into previous since it is the top of the Band
    if (in.Line() == 1) {
      const_cast<FindGapsFunctor *>(this)->m_previous = new Brick(in.SampleDimension(),
                                                                  in.LineDimension(),
                                                                  in.BandDimension(),
                                                                  in.PixelType());
      copy(in);
    }
    else {

      // Uses MultivariateStatistics to compare the last line with the current
      MultivariateStatistics multivarStats;

      multivarStats.AddData(m_previous->DoubleBuffer(), in.DoubleBuffer(), in.size());

      double correlation = multivarStats.Correlation();

      if (std::fabs(correlation) < m_corTol  ||  correlation == Isis::Null) {

        // Then current line is a Gap, and acts accordingly
        if( !(*m_inGap) ) {

          *m_inGap = true;
          m_gap->addKeyword(PvlKeyword("NewGapInBand", std::to_string(in.Band())));
          m_gap->addKeyword(PvlKeyword("StartLine", std::to_string(in.Line())));
          
          if(correlation == Isis::Null) {
            correlation = 0.0;
          }
          m_gap->addKeyword(PvlKeyword("Correlation", std::to_string(correlation)));
        }
        
        if (in.Line() == m_lineCount) {
          m_gap->addKeyword(PvlKeyword("LastGapLine", std::to_string(in.Line())));
          m_gap->addKeyword(PvlKeyword("ToEndOfBand", std::to_string(m_lineCount)));

          addGapToGroup();
        }
      }
      else if (*m_inGap) {

        /*
         * Then it was the last line of the gap 2 lines ago, since this line and its pervious line
         * correlate. Or this line is the last line and it is in the gap.
         */

        m_gap->addKeyword(PvlKeyword("LastGapLine", std::to_string(in.Line() - 2)));
        addGapToGroup();
      }
      
      // Sets up previous for next pass
      copy(in);
    }
  }

  
  /**
   * Write an output cube that is a copy of the input with a null buffer before and
   * after the gaps the sizes of which will be determined by the user.
   *
   */
  void FindGapsFunctor::operator()(Buffer &in, Buffer &out) const {

    bool nulledLine = false;

    for (int i = 0; i < m_gaps->groups(); i++) {

      int gapBand = std::stoi((*m_gaps).group(i).findKeyword(" NewGapInBand")[0]);
      int gapStart = std::stoi((*m_gaps).group(i).findKeyword("StartLine")[0]);
      int gapEnd = std::stoi((*m_gaps).group(i).findKeyword("LastGapLine")[0]);

      if ( (in.Line() >= gapStart - m_bufferSizeBeforeGap) &&
           (in.Line() <= gapEnd + m_bufferSizeAfterGap) &&
           (in.Band() == gapBand) ) {

        for (int i = 0; i < in.SampleDimension(); i++) {
          out[i] = Null;
        }
        nulledLine= true;
      }
    }
      
    if (!nulledLine) {
      out.Copy(in);
    } 
  }
  

  /**
   * Copies the given buffer and its position information into m_previous.
   *
   * @param in The input buffer to be copied into m_previous
   */
  void FindGapsFunctor::copy(Buffer &in) const {
    m_previous->SetBasePosition(in.Sample(), in.Line(), in.Band());
    m_previous->Copy(in);
  }


  /**
   * Add a gap to the group of gaps that will be output. Setup the gap to accept another set of
   * gap information
   */
  void FindGapsFunctor::addGapToGroup() const {

    *m_inGap = false;

    /*
    * In case the gap is a flase positive. The end of the gap will end up being the line
    * before the start of the gap.
    */
    if (std::stoi(m_gap->findKeyword("StartLine")[0]) <
        std::stoi(m_gap->findKeyword("LastGapLine")[0])) {

      m_gaps->addGroup(*m_gap);
    }
    *m_gap = PvlGroup("Gap");
  }
  
}
