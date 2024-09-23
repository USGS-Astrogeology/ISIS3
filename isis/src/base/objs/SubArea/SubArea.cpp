/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <float.h>
#include <iostream>
#include <string>
#include <sstream>

#include "SubArea.h"
#include "AlphaCube.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "TProjection.h"
#include "IException.h"
#include "IString.h"

using namespace std;
namespace Isis {

  /**
   * Defines the subarea.
   *
   * @param orignl This is the number of lines in the original image.
   *
   * @param origns This is the number of samples in the original image.
   *
   * @param sl This is the line in the original image where the subarea will
   *           start at. This value must be greater than or equal to 1 and
   *           less than or equal to the number of lines in the original
   *           image.
   *
   * @param ss This is the sample in the original image where the subarea
   *           will start at. This value must be greater than or equal to 1
   *           and less than or equal to the number of samples in the
   *           original image.
   *
   * @param el This is the ending line of the subarea to extract from the
   *           original image. This value must be greater than or equal to
   *           sl and less than or equal to the number of lines in the
   *           original image. The actual number of lines that will be in
   *           the subarea will be (el-sl+1)/linc.
   *
   * @param es This is the ending sample of the subarea to extract from the
   *           original image. This value must be greater than or equal to
   *           ss and less than or equal to the number of samples in the
   *           original image. The actual number of samples that will be in
   *           the subarea will be (es-ss+1)/sinc.
   *
   * @param linc This is the line increment that will be used to extract the
   *           subarea from the original image. It must be greater than 0.
   *
   * @param sinc This is the sample increment that will be used to extract
   *           the subarea from the original image. It must be greater than
   *           0.
   *
   */
  void SubArea::SetSubArea(const int orignl, const int origns,
                           const int sl, const int ss, const int el,
                           const int es, const double linc, const double sinc) {

    // Save size of original image file
    p_nl = orignl;
    p_ns = origns;

    // Save the subarea information
    p_sl = sl;
    p_ss = ss;
    p_el = el;
    p_es = es;

    if(p_sl > p_el) {
      string msg = "Invalid start/end line range [sl,el] specified for subarea";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if(p_ss > p_es) {
      string msg = "Invalid start/end sample range [ss,es] specified for subarea";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    p_linc = linc;
    if(p_linc <= 0.0) {
      string msg = "Invalid line increment [linc] specified for subarea";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    p_sinc = sinc;
    if(p_sinc <= 0.0) {
      string msg = "Invalid sample increment [sinc] specified for subarea";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  /**
   * Modifies a label for a file containing a subarea. The AlphaCube, Mapping,
   * and Instrument groups are all affected when a subarea is extracted from
   * another file. If the linc is not equal to the sinc, then the Instrument
   * and Mapping groups will be removed from the label because they will no
   * longer be valid. If the linc is equal to the sinc and they are not equal
   * to 1, then the map scale and resolution in the Mapping group needs to be
   * updated. The latitude and longitude ranges become invalid when the subarea
   * does not cover the entire sample and line range of the original cube.
   * Update the upper left corner x,y values if the projection is still valid
   * and the starting line and/or starting sample have been changed from their
   * location in the original file.
   *
   * @param icube This is the input cube that will have the subarea
   *           extracted from it. The label of this cube will be used to
   *           create updated Mapping, Instrument, and AlphaCube groups
   *           for the label of the output cube containing the subarea.
   *
   * @param ocube This is the output cube file containing the subarea. The
   *           label of this cube will be modified by extracting the Mapping,
   *           Instrument, and AlphaCube groups from the input label and
   *           putting them in this label.
   *
   * @param results This is the Results group that will go into the application
   *           log file. This group must be created by the calling application.
   *           Information will be added to it if the Mapping or Instrument
   *           groups are deleted from the output image label.
   *
   */
  void SubArea::UpdateLabel(Cube *icube, Cube *ocube, PvlGroup &results) {

    Pvl inlabel = *icube->label();

    // If the linc and sinc are not equal, then the Instrument and
    // Mapping groups are no longer valid.
    if(p_linc != p_sinc) {
      if(inlabel.findObject("IsisCube").hasGroup("Mapping")) {
        inlabel.findObject("IsisCube").deleteGroup("Mapping");
        results += PvlKeyword("MappingGroupDeleted", "True");

        // We don't want to think our projected cube is unprojected, so if we
        //   delete a mapping group and we have a camera there is a problem.
        //   Remove the camera.
        if(inlabel.findObject("IsisCube").hasGroup("Instrument")) {
          inlabel.findObject("IsisCube").deleteGroup("Instrument");
          results += PvlKeyword("InstrumentGroupDeleted", "True");
        }
      }
    }

    if(inlabel.findObject("IsisCube").hasGroup("Mapping")) {
      // Update the upper left corner X,Y values if the starting line or
      // starting sample are changed.
      TProjection *proj = (TProjection *) icube->projection();
      if(p_sl != 1 || p_ss != 1) {
        proj->SetWorld(p_ss - 0.5, p_sl - 0.5);
        PvlGroup &mapgroup = inlabel.findObject("IsisCube").findGroup("Mapping", Pvl::Traverse);
        mapgroup.addKeyword(PvlKeyword("UpperLeftCornerX", Isis::toString(proj->XCoord())),
                            Pvl::Replace);
        mapgroup.addKeyword(PvlKeyword("UpperLeftCornerY", Isis::toString(proj->YCoord())),
                            Pvl::Replace);
      }

      // If the linc and sinc are not equal to 1, then update the
      // mapping scale and resolution.
      if(p_linc == p_sinc && p_linc != 1.0) {
        PvlGroup &mapgroup = inlabel.findObject("IsisCube").findGroup("Mapping", Pvl::Traverse);
        QString pixresUnit = QString::fromStdString(mapgroup["PixelResolution"].unit());
        double pixres = Isis::toDouble(mapgroup["PixelResolution"][0]);
        mapgroup["PixelResolution"] = Isis::toString(pixres * p_linc);
        mapgroup["PixelResolution"].setUnits(pixresUnit.toStdString());
        QString scaleUnit = QString::fromStdString(mapgroup["Scale"].unit());
        double scale = mapgroup["Scale"];
        mapgroup["Scale"] = Isis::toString(scale / p_linc);
        mapgroup["Scale"].setUnits(scaleUnit.toStdString());
      }

      // If the outer bounds of the image are changed, then the
      // latitude,longitude range is no longer valid.
      if(p_sl != 1 || p_ss != 1 || p_el != p_nl || p_es != p_ns) {
        PvlGroup &mapgroup = inlabel.findObject("IsisCube").findGroup("Mapping", Pvl::Traverse);
        if (proj->IsEquatorialCylindrical()) {
          double minlat, maxlat;
          double minlon, maxlon;
          proj->SetWorld(p_ss-.5, p_sl-.5);
          if (proj->IsGood()) {
            maxlat = proj->UniversalLatitude();
            if (proj->IsPlanetographic()) {
              maxlat = proj->ToPlanetographic(maxlat);
            }
            if (proj->IsPositiveEast()) {
              minlon = proj->UniversalLongitude();
              if (proj->Has180Domain()) {
                minlon = proj->To180Domain(minlon);
              }
            }
            else {
              if (proj->Has360Domain()) {
                minlon = proj->ToPositiveWest(proj->Longitude(), 360);
              }
              else {
                minlon = proj->ToPositiveWest(proj->Longitude(), 180);
              }
            }
            proj->SetWorld(p_es+.5, p_el+.5);
            if (proj->IsGood()) {
              minlat = proj->UniversalLatitude();
              if (proj->IsPlanetographic()) {
                minlat = proj->ToPlanetographic(minlat);
              }
              if (proj->IsPositiveEast()) {
                maxlon = proj->UniversalLongitude();
                if (proj->Has180Domain()) {
                  maxlon = proj->To180Domain(maxlon);
                }
              }
              else {
                if (proj->Has360Domain()) {
                  maxlon = proj->ToPositiveWest(proj->Longitude(), 360);
                }
                else {
                  maxlon = proj->ToPositiveWest(proj->Longitude(), 180);
                }
              }
              mapgroup.addKeyword(PvlKeyword("MinimumLatitude",Isis::toString(minlat)),Pvl::Replace);
              mapgroup.addKeyword(PvlKeyword("MaximumLatitude",Isis::toString(maxlat)),Pvl::Replace);
              mapgroup.addKeyword(PvlKeyword("MinimumLongitude",Isis::toString(minlon)),Pvl::Replace);
              mapgroup.addKeyword(PvlKeyword("MaximumLongitude",Isis::toString(maxlon)),Pvl::Replace);
            }
          }
        }
        else {
          if(mapgroup.hasKeyword("MinimumLatitude")) {
            mapgroup.deleteKeyword("MinimumLatitude");
          }
          if(mapgroup.hasKeyword("MaximumLatitude")) {
            mapgroup.deleteKeyword("MaximumLatitude");
          }
          if(mapgroup.hasKeyword("MinimumLongitude")) {
            mapgroup.deleteKeyword("MinimumLongitude");
          }
          if(mapgroup.hasKeyword("MaximumLongitude")) {
            mapgroup.deleteKeyword("MaximumLongitude");
          }
        }
      }
    }

    // Make changes to the output cube label
    if(ocube->hasGroup("Instrument")) {
      ocube->deleteGroup("Instrument");
    }
    if(inlabel.findObject("IsisCube").hasGroup("Instrument")) {
      PvlGroup inst;
      inst = inlabel.findObject("IsisCube").findGroup("Instrument");
      ocube->putGroup(inst);
    }

    if(ocube->hasGroup("Mapping")) {
      ocube->deleteGroup("Mapping");
    }
    if(inlabel.findObject("IsisCube").hasGroup("Mapping")) {
      PvlGroup mapgrp;
      mapgrp = inlabel.findObject("IsisCube").findGroup("Mapping");
      ocube->putGroup(mapgrp);
    }

    // Update the AlphaCube group - this group will only be updated if
    // a Mapping group does not exist in the labels.
    AlphaCube aCube(p_ns, p_nl, ocube->sampleCount(), ocube->lineCount(),
                    p_ss - 0.5, p_sl - 0.5, p_es + 0.5, p_el + 0.5);
    aCube.UpdateGroup(*ocube);
  }
}
