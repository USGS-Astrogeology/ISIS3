/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef cam2cam_h
#define cam2cam_h

#include <memory>

#include "Transform.h"

#include "Application.h"
#include "Camera.h"
#include "Cube.h"
#include "Distance.h"
#include "UserInterface.h"

namespace Isis {
  extern void cam2cam(Cube* inCube, Cube* matchCube, UserInterface &ui);
  extern void cam2cam(UserInterface &ui);


  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class cam2camXform : public Isis::Transform {
    private:
      Isis::Camera *p_incam;
      Isis::Camera *p_outcam;
      int p_inputSamples;
      int p_inputLines;
      int p_outputSamples;
      int p_outputLines;
      bool p_offbody;
      bool p_trim;

    public:
      // constructor
      cam2camXform(const int inputSamples, const int inputLines, Isis::Camera *incam,
                   const int outputSamples, const int outputLines, Isis::Camera *outcam,
                   const bool offbody = false, const bool trim = true) {

        p_inputSamples = inputSamples;
        p_inputLines = inputLines;
        p_incam = incam;
    
        p_outputSamples = outputSamples;
        p_outputLines = outputLines;
        p_outcam = outcam;
        p_offbody = offbody;
        p_trim = trim;
      }
      
      // destructor
      virtual ~cam2camXform() {};

      /** Transform method mapping output line/samps to lat/lons to input line/samps */
      inline bool Xform(double &inSample, double &inLine,
                        const double outSample, const double outLine) {
                
       // See if the output image coordinate converts to lat/lon
       if ( p_outcam->SetImage(outSample, outLine) )  {
         // Get the universal lat/lon and see if it can be converted to input line/samp
         double lat = p_outcam->UniversalLatitude();
         double lon = p_outcam->UniversalLongitude();
         Distance rad = p_outcam->LocalRadius();
         if (rad.isValid()) {
           if(!p_incam->SetUniversalGround(lat, lon, rad.meters())) return false;
         }
         else {
           if(!p_incam->SetUniversalGround(lat, lon)) return false;
         }
       }
       else if ( p_offbody ) {
         double ra = p_outcam->RightAscension();
         double dec = p_outcam->Declination();
         if ( !p_incam->SetRightAscensionDeclination(ra,dec) ) return false;
         if ( p_trim ) {
           if ( p_incam->SetImage(p_incam->Sample(), p_incam->Line()) ) return (false);
         }
       }
       else {
         return false;
       }
      
       // Make sure the point is inside the input image
       if (p_incam->Sample() < 0.5) return false;
       if (p_incam->Line() < 0.5) return false;
       if (p_incam->Sample() > p_inputSamples + 0.5) return false;
       if (p_incam->Line() > p_inputLines + 0.5) return false;
      
       // Everything is good
       inSample = p_incam->Sample();
       inLine = p_incam->Line();
       return true;
       }
      
      
       inline int OutputSamples() const {
         return p_outputSamples;
       }
      
      
       inline int OutputLines() const {
         return p_outputLines;
       }
    
  };


 
  

}

#endif
