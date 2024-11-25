/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#ifndef cam2cam_h
#define cam2cam_h

#include "Transform.h"

#include "Application.h"
#include "Camera.h"
#include "Cube.h"
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
                   const bool offbody = false, const bool trim = false);

      // destructor
      ~cam2camXform() {};

      // Override parent's pure virtual members
      bool Xform(double &inSample, double &inLine,
                 const double outSample, const double outLine);
      int OutputSamples() const;
      int OutputLines() const;
  };
}

#endif
