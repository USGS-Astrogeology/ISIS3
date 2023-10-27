/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ApolloPanoramicDetectorMap.h"
#include "Application.h"
#include "PvlGroup.h"
#include "SerialNumber.h"
#include "Table.h"
#include "iTime.h"

#include <iostream>
using namespace std;

using namespace std;
namespace Isis {
  /**
   * This method sets cube sample line coordinates from given Dector coordinates
   *
   * @param sample dector sample coordinate
   * @param line detector line coordinate
   *
   * @return bool Always returns true
   */
    bool ApolloPanoramicDetectorMap::SetDetector(const double sample, const double line) {
      //given a detector coordinate and a time (read from the 'parent' camera class)
      //  set the image coordinates
      //save the detector coordinates
      //negative signs are counterintuitive and I cannot fully explain them, I believe they are
      // because of the negative z direction in the distortion map....
      p_detectorLine = -line;
      p_detectorSample = -sample;

      //convert from detector to fiducial coordinants
      double fidL = (p_camera->time().Et() - m_etMiddle)/m_lineRate;
      //double fidS = sample;  //conversion is identity so it is skiped

      //convert from fiducial coordinates to parent image coordinates
      p_intOri.image2Machine(p_detectorSample, fidL, &p_parentSample, &p_parentLine);

      //cout << "fiducial values sent to IO: " << p_detectorSample << " " << fidL << endl; //debug
      //cout << "cube coordinates returned from IO: " << p_parentSample << " " << p_parentLine << endl;
      return true;
    }

  /**
   * This method sets dector sample line coordinates from given cube coordinates
   *
   * @param sample cube sample coordinate
   * @param line cube line coordinate
   *
   * @return bool Always returns true
   */
    bool ApolloPanoramicDetectorMap::SetParent(const double sample, const double line) {
      //Given an image (aka 'Parent', aka encoder, aka machine) coordinate set the detector
      //coordiante and the time (time is set in the 'parent' camera class)
      //save the parent data
      p_parentLine = line;
      p_parentSample = sample;
      //convert from machine to fiducial coordinates
      //cout << "cube coordinates sent to IO: " << sample << " " << line << endl; //debug

      p_intOri.machine2Image(sample,line,&p_detectorSample, &p_detectorLine);

      //cout << "fiducial coordinate return from IO: " << p_detectorSample << " " << p_detectorLine << endl;
      //convert from fiducial coordinates to detector/time coordinates
      iTime isisTime(m_etMiddle + p_detectorLine*m_lineRate);
      p_camera->setTime(isisTime);
      //This declaration may cause some debate.  Regardless it seems to that
      //  since we model the motion of the camera as continuous smooth motion
      //  (not some discretely defined series of 1 pixel or 1 mm wide 'push-frames'),
      //  and we calculate the positions, pointings, etc at the specific time implied
      //  by the sub-pixel/mm line then the line in the dector will always be the same
      //  (in this case zero)
      p_detectorLine = 0.0;

      return true;
    }

    /**
     * This method uses the ApolloPanIO class to compute transforamtion from cube to image
     * (aka fiducial cooraintes)
     *
     * @throws  IException::User "No FID_MEASURES table found in cube blobs."
     * @throws  IException::User "Less than four FID_MEASURES found in cube blobs."
     * @throws  IException::User "Insufficient Fiducial Observations for computation of the interior
     *                            orientation. At least one vertical pair must be measured, many
     *                            more is recomented."
     *
     * @returns int Returns 1 on success and -1 on failure.
     */
    int ApolloPanoramicDetectorMap::initializeInteriorOrientation() {
      int i,nrec;
      //read the fidicial measurements from the attached table in the camera labels to define a
      //  series of affine transformation between image (aka encoder aka machine) coordinates
      //  and fiducial coordinates
      Table tableFid("Fiducial Measurement", QString::fromStdString(m_lab->fileName()));

      nrec = tableFid.Records();  //get the number of records found in the cube blobs

      if( nrec <= 0) {
        throw IException(IException::User,"No FID_MEASURES table found in cube blobs.\n",
                         _FILEINFO_);
        return -1;
      }
      if( tableFid.Records() < 4) {
        throw IException(IException::User,"Less than four FID_MEASURES found in cube blobs.\n",
                         _FILEINFO_);
        return -1;
      }
      p_intOri.initialize();

      for(i=0;i<nrec;i++)  //input all the observations to the the Interior Orientation Class
        p_intOri.fiducialObservation(tableFid[i][0], tableFid[i][1], tableFid[i][2]);

      i = p_intOri.computeInteriorOrienation();

      if( i != 1) { //unsuccessful computation of the interior orienation
        throw IException(IException::User,"Insufficient Fiducial Observations for computation of "
                                          "the interior orientation.\nAt least one vertical pair "
                                          "must be measured, many more is recomented.\n",
                         _FILEINFO_);
        return -1;
      }

      return 1;
    }
} //end namespace Isis
