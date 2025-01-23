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


  /**
   * @brief Standalone cam2cam pixel mapper functor
   * 
   * This class provides an instance of a standalone implementation of the
   * cam2cam mapping algorithm. It is designed to provide single pixel mapping
   * of the match/to output image to the from image. Note that the MATCH and TO
   * cubes have exactly the same geometry so TO is not required for this 
   * implementation.
   * 
   * The cam2cam algorithm is output driven by mapping every sample/line pixel
   * coordinate in the match image back into the from image FOV.
   * 
   * This class functor is designed to just compute the coordinate in the FROM
   * image and determine how it is mapped into the FROM/MATCH paramterization.
   * This is well suited for testing the algorithm without significant overhead
   * of processing, typically large, ISIS cubes.
   * 
   * It does not create an output cube or interpolation algorithms. Selection of
   * input bands is entirely managed by the owner of this object.
   * 
   * Note this should *not* be used as the processing algorithm directly in
   * cam2cam because it does not use the RubberSheet mapping algorithm as the
   * main app uses. This essentially uses the RubberSheet::SlowGeom() method
   * (w/out interpolation!), which is the least efficicent algorithm.
   * However it may be applicable in a threaded implementation.
   * 
   * @author 2024-10-27 Kris J. Becker
   */
  class Cam2CamMapper {
    public:
      Cam2CamMapper( ) : m_from( nullptr ), m_match( nullptr ),
                         m_offbody( false ),
                         m_offbodytrim( true ),
                         m_transform( nullptr ) {  } 
      Cam2CamMapper( Cube *from, Cube *match,
                     const bool offbody = false, 
                     const bool offbodytrim = true ) :
                     m_from( from ), m_match( match ),
                     m_offbody( offbody ),
                     m_offbodytrim( offbodytrim ),
                     m_transform( make_mapper() ) {  }
      ~Cam2CamMapper() { }


      inline bool IsValid() const {
        return ( ( nullptr != m_from ) && ( nullptr != m_match ) && ( nullptr != m_transform.get() ) );
      }

      inline Cube *from( ) {
        return ( m_from );
      }     

      inline Cube *match( ) {
        return ( m_match );
      }     

      inline Camera *from_camera( ) {
        return ( m_from->camera() );
      }     

      inline Camera *match_camera( ) {
        return ( m_match->camera() );
      }     

      inline Transform *transform( ) {
        return ( m_transform.get() );
      }     

      inline bool offbody() const {
        return ( m_offbody );
      }

      inline bool offbodytrim() const {
        return ( m_offbodytrim );
      }

      /** Allocate a custom cam2map mapper transform */
      inline Transform *make_mapper( const bool offbody, const bool offbodytrim) const {
        if ( nullptr == m_from ) {
          throw IException( IException::Programmer, "FROM cube is not valid!", _FILEINFO_ );
        }

        if ( nullptr == m_match ) {
          throw IException( IException::Programmer, "MATCH cube is not valid!", _FILEINFO_ );
        }        

        // Compute the transform
        Transform *v_t = new cam2camXform( m_from->sampleCount(),
                                           m_from->lineCount(),
                                           m_from->camera(),
                                           m_match->sampleCount(),
                                           m_match->lineCount(),
                                           m_match->camera(),
                                           offbody,
                                           offbodytrim );

        // Check if we were successful
        if ( nullptr == v_t ) {
          QString mess = "Unable to allocate cam2map mapper for FROM (" + m_from->fileName() + 
                         ") and MATCH (" + m_match->fileName() + ")";
          throw IException( IException::Programmer, mess, _FILEINFO_ );
        }

        return ( v_t );        
      }

      /** Allocate a cam2map mapper transform with configured state */
      inline  Transform *make_mapper( ) const {
        return ( make_mapper( m_offbody, m_offbodytrim ) );
      }

      /** Mapping method for (out)MATCH->(in)FROM pixel coordinates */
      inline bool mapit( double &inSample, double &inLine,
                         const double outSample, const double outLine ) {
        return ( m_transform->Xform( inSample, inLine, outSample, outLine ) );
      }

    private:
      Cube                      *m_from;
      Cube                      *m_match;
      bool                       m_offbody;
      bool                       m_offbodytrim;
      std::shared_ptr<Transform> m_transform;
  };
  

}

#endif
