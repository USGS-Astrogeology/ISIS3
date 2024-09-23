/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Application.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"
#include "PvlGroup.h"
#include "Camera.h"
#include "MeasureValidationResults.h"
#include "Portal.h"
#include "SpecialPixel.h"
#include "Pvl.h"
#include "SerialNumberList.h"

#include "CnetRefByResolution.h"

using namespace std;

namespace Isis {
  /**
   * CnetRefByResolution constructor with the def file
   *
   * @author Sharmila Prasad (5/25/2010)
   *
   * @param pPvlDef         - Pvl Definition File
   * @param psSerialNumfile - Serial Number file attached to the ControlNet
   * @param peType          - Resolution Type
   * @param pdResValue      - Resolution value
   * @param pdMinRes        - Min Resolution value
   * @param pdMaxRes        - Max Resolution value
   */
  CnetRefByResolution::CnetRefByResolution(Pvl *pPvlDef, QString psSerialNumfile,
      ResolutionType peType, double pdResValue, double pdMinRes, double pdMaxRes)
    : ControlNetValidMeasure(pPvlDef) {
    mdResValue = pdResValue;
    mdMinRes   = pdMinRes;
    mdMaxRes   = pdMaxRes;
    meType     = peType;
    ReadSerialNumbers(psSerialNumfile);
    SetCameraRequiredFlag(true);
  }

  /**
   * FindCnetRef traverses all the control points and measures in the network and checks for
   * valid Measure which passes the Emission Incidence Angle, DN value tests and chooses the
   * measure with the best Resolution criteria as the reference. Creates a new control network
   * with these adjustments.
   *
   * @author Sharmila Prasad (5/25/2010)
   * @history 2010-10-04 Sharmila Prasad - Modified for Binary CNet (Edit Lock)
   *
   * @param pNewNet   - Modified output Control Net
   *
   */
  void CnetRefByResolution::FindCnetRef(ControlNet &pNewNet) {
    // Process each existing control point in the network
    int iPointsModified = 0;
    int iMeasuresModified = 0;
    int iRefChanged = 0;

    //Status Report
    mStatus.SetText("Choosing Reference by Resolution...");
    mStatus.SetMaximumSteps(pNewNet.GetNumPoints());
    mStatus.CheckStatus();

    //mPvlLog += GetStdOptions();
    for (int point = 0; point < pNewNet.GetNumPoints(); ++point) {
      ControlPoint *newPnt = pNewNet.GetPoint(point);
      bool bError = false;

      // Create a copy of original control point
      const ControlPoint origPnt(*newPnt);

      mdResVector.clear();

      // Logging
      PvlObject pvlPointObj("PointDetails");
      pvlPointObj += Isis::PvlKeyword("PointId", newPnt->GetId().toStdString());

      // Edit Lock Option
      bool bPntEditLock = newPnt->IsEditLocked();
      if (!bPntEditLock) {
        newPnt->SetDateTime(Application::DateTime());
      }
      else {
        pvlPointObj += Isis::PvlKeyword("Reference", "No Change, PointEditLock");
      }

      int iNumMeasuresLocked = newPnt->GetNumLockedMeasures();
      bool bRefLocked = newPnt->GetRefMeasure()->IsEditLocked();
      int numMeasures = newPnt->GetNumMeasures();

      int iRefIndex = -1;
      if (newPnt->IsReferenceExplicit())
        iRefIndex = newPnt->IndexOfRefMeasure();
      QString istrTemp;

      std::vector <PvlGroup> pvlGrpVector;
      int iBestIndex = 0;

      // Only perform the interest operation on points of type "Free" and
      // Points having atleast 1 measure and Point is not Ignored
      // Check for EditLock in the Measures and also verfify that
      // only a Reference Measure can be Locked else error
      if (!newPnt->IsIgnored() && newPnt->GetType() == ControlPoint::Free && numMeasures > 0 &&
          (iNumMeasuresLocked == 0 || (iNumMeasuresLocked > 0 && bRefLocked))) {
        int iNumIgnore = 0;
        QString istrTemp;
        for (int measure = 0; measure < newPnt->GetNumMeasures(); ++measure) {
          ControlMeasure *newMsr = newPnt->GetMeasure(measure);
          bool bMeasureLocked = newMsr->IsEditLocked();
          double dSample      = newMsr->GetSample();
          double dLine        = newMsr->GetLine();
          QString sn      = newMsr->GetCubeSerialNumber();

          if (!bPntEditLock && !bMeasureLocked) {
            newMsr->SetDateTime(Application::DateTime());
            newMsr->SetChooserName("Application cnetref(Resolution)");
          }

          // Log
          PvlGroup pvlMeasureGrp("MeasureDetails");
          pvlMeasureGrp += Isis::PvlKeyword("SerialNum", sn.toStdString());
          pvlMeasureGrp += Isis::PvlKeyword("OriginalLocation", LocationString(dSample, dLine).toStdString());

          if (bMeasureLocked)
            pvlMeasureGrp += Isis::PvlKeyword("EditLock", "True");

          if (!newMsr->IsIgnored()) {
            Cube *measureCube = mCubeMgr.OpenCube(mSerialNumbers.fileName(sn));

            MeasureValidationResults results =
              ValidStandardOptions(newMsr, measureCube, &pvlMeasureGrp);
            if (!results.isValid()) {
              if (bPntEditLock) {
                pvlMeasureGrp += Isis::PvlKeyword("UnIgnored", "Failed Validation Test but not Ignored as Point EditLock is True");
              }
              else if (bMeasureLocked) {
                pvlMeasureGrp += Isis::PvlKeyword("UnIgnored", "Failed Validation Test but not Ignored as Measure EditLock is True");
              }
              else {
                pvlMeasureGrp += Isis::PvlKeyword("Ignored", "For point [" + newPnt->GetId().toStdString() + "] and measure [" + sn.toStdString() +
                                                  "], point failed to intersect body.");
                newMsr->SetIgnored(true);
                iNumIgnore++;
              }
            } // valid measure
            else {
              if (!bPntEditLock && !bRefLocked) {
                newMsr->SetType(ControlMeasure::Candidate);
                newMsr->SetIgnored(false);
                mdResVector.push_back(mdResolution);
              }
            }
          } // Ignore == false
          else {
            pvlMeasureGrp += Isis::PvlKeyword("Ignored", "Originally Ignored");
            iNumIgnore++;
          }
          if (newMsr != origPnt.GetMeasure(measure)) {
            iMeasuresModified++;
          }

          //newPnt.UpdateMeasure(newMsr); // Redesign fixed this
          pvlGrpVector.push_back(pvlMeasureGrp);
        }// end Measure

        if ((newPnt->GetNumMeasures() - iNumIgnore) < 2) {
          if (bPntEditLock) {
            pvlPointObj += Isis::PvlKeyword("UnIgnored", "Good Measures less than 2 but not Ignored as Point EditLock is True");
          }
          else {
            newPnt->SetIgnored(true);
            pvlPointObj += Isis::PvlKeyword("Ignored", "Good Measures less than 2");
          }
        }
        // Set the Reference if the Point is unlocked and Reference measure is unlocked
        if (!newPnt->IsIgnored() && !bPntEditLock && !bRefLocked) {
          iBestIndex = GetReferenceByResolution(newPnt);
          if (iBestIndex >= 0 && !newPnt->GetMeasure(iBestIndex)->IsIgnored()) {
            newPnt->SetRefMeasure(iBestIndex);
            //newPnt.UpdateMeasure(cm); // Redesign fixed this
            pvlGrpVector[iBestIndex] += Isis::PvlKeyword("Reference", "true");
          }
          else {
            if (iBestIndex < 0 && meType == Range) {
              pvlPointObj += Isis::PvlKeyword("NOTE", "No Valid Measures within the Resolution Range. Reference defaulted to the first Measure");
            }
            iBestIndex = 0;
            newPnt->SetRefMeasure(iBestIndex);
            //newPnt.UpdateMeasure(cm); // Redesign fixed this

            // Log info, if Point not locked, apriori source == Reference and a new reference
            if (iRefIndex != iBestIndex &&
                newPnt->GetAprioriSurfacePointSource() == ControlPoint::SurfacePointSource::Reference) {
              pvlGrpVector[iBestIndex] += Isis::PvlKeyword("AprioriSource", "Reference is the source and has changed");
            }
          }
        }

        for (int i = 0; i < newPnt->GetNumMeasures(); i++) {
          pvlPointObj += pvlGrpVector[i];
        }
      } // end Free
      else {
        int iComment = 0;
        if (numMeasures == 0) {
          std::string sComment = "Comment";
          sComment += Isis::toString(++iComment);
          pvlPointObj += Isis::PvlKeyword(sComment, "No Measures in the Point");
        }

        if (newPnt->IsIgnored()) {
          std::string sComment = "Comment";
          sComment += Isis::toString(++iComment);
          pvlPointObj += Isis::PvlKeyword(sComment, "Point was originally Ignored");
        }

        if (newPnt->GetType() == ControlPoint::Fixed) {
          std::string sComment = "Comment";
          sComment += Isis::toString(++iComment);
          pvlPointObj += Isis::PvlKeyword(sComment, "Fixed Point");
        }
        else if (newPnt->GetType() == ControlPoint::Constrained) {
          std::string sComment = "Comment";
          sComment += Isis::toString(++iComment);
          pvlPointObj += Isis::PvlKeyword(sComment, "Constrained Point");
        }

        if (iNumMeasuresLocked > 0 && !bRefLocked) {
          pvlPointObj += Isis::PvlKeyword("Error", "Point has a Measure with EditLock set to true "
                                          "but the Reference is not Locked");
          bError = true;
        }
        else {
          for (int measure = 0; measure < newPnt->GetNumMeasures(); measure++) {
            ControlMeasure *cm = newPnt->GetMeasure(iBestIndex);
            cm->SetDateTime(Application::DateTime());
            cm->SetChooserName("Application cnetref(Resolution)");
            //newPnt.UpdateMeasure(cm); // Redesign fixed this
          }
        }
      }

      if (*newPnt != origPnt) {
        iPointsModified++;
      }

      if (!bError && !newPnt->IsIgnored() && newPnt->IsReferenceExplicit() && iBestIndex != iRefIndex
          && !bPntEditLock && !bRefLocked) {
        iRefChanged++;
        PvlGroup pvlRefChangeGrp("ReferenceChangeDetails");
        if (iRefIndex >= 0) {
          pvlRefChangeGrp += Isis::PvlKeyword("PrevSerialNumber",
              origPnt.GetMeasure(iRefIndex)->GetCubeSerialNumber().toStdString());
          pvlRefChangeGrp += Isis::PvlKeyword("PrevResolution",   Isis::toString(mdResVector[iRefIndex]));

          istrTemp = QString((int)origPnt.GetMeasure(iRefIndex)->GetSample());
          istrTemp += ",";
          istrTemp += QString((int)origPnt.GetMeasure(iRefIndex)->GetLine());
          pvlRefChangeGrp += Isis::PvlKeyword("PrevLocation",     istrTemp.toStdString());
        }
        else {
          pvlRefChangeGrp += Isis::PvlKeyword("PrevReference", "Not Set");
        }

        pvlRefChangeGrp += Isis::PvlKeyword("NewSerialNumber",
            newPnt->GetMeasure(iBestIndex)->GetCubeSerialNumber().toStdString());
        QString sKeyName = "NewHighestResolution";
        if (meType == Low) {
          sKeyName = "NewLeastResolution";
        }
        else if (meType == Mean) {
          pvlRefChangeGrp += Isis::PvlKeyword("MeanResolution",  Isis::toString(GetMeanResolution()));
          sKeyName = "NewResolutionNeartoMean";
        }
        else if (meType == Nearest) {
          sKeyName = "NewResolutionNeartoValue";
        }
        else if (meType == Range) {
          sKeyName = "NewResolutionInRange";
        }
        pvlRefChangeGrp += Isis::PvlKeyword(sKeyName.toStdString(),  Isis::toString(mdResVector[iBestIndex]));

        istrTemp = QString((int)newPnt->GetMeasure(iBestIndex)->GetSample());
        istrTemp += ",";
        istrTemp += QString((int)newPnt->GetMeasure(iBestIndex)->GetLine());
        pvlRefChangeGrp += Isis::PvlKeyword("NewLocation", istrTemp.toStdString());

        pvlPointObj += pvlRefChangeGrp;
      }
      else {
        pvlPointObj += Isis::PvlKeyword("Reference", "No Change");
      }

      //pNewNet.UpdatePoint(newPnt); // Redesign fixed this
      mPvlLog += pvlPointObj;
      mStatus.CheckStatus();
    }// end Point

    // CnetRef Change Statistics
    mStatisticsGrp += Isis::PvlKeyword("PointsModified",   Isis::toString(iPointsModified));
    mStatisticsGrp += Isis::PvlKeyword("ReferenceChanged", Isis::toString(iRefChanged));
    mStatisticsGrp += Isis::PvlKeyword("MeasuresModified", Isis::toString(iMeasuresModified));

    mPvlLog += mStatisticsGrp;
  }

  /**
   * GetMeanResolution - Get Mean of all the resolutions of all the measures
   * in the Control Point
   *
   * @author Sharmila Prasad (6/1/2010)
   *
   * @return double
   */
  double CnetRefByResolution::GetMeanResolution(void) {
    double dTotal = 0;
    for (int i = 0; i < (int)mdResVector.size(); i++) {
      dTotal += mdResVector[i];
    }

    return (dTotal / mdResVector.size());
  }

  /**
   * GetReferenceByResolution - Get the Reference for each Control Point by user defined
   * Resolution criteria
   *
   * @author Sharmila Prasad (6/1/2010)
   *
   * @return int
   */
  int CnetRefByResolution::GetReferenceByResolution(ControlPoint *pNewPoint) {
    int iBestIndex = -1;
    double dBestResolution = -1;
    double dMean = 0;
    if (meType == Mean) {
      dMean = GetMeanResolution();
    }

    for (int i = 0; i < (int)mdResVector.size(); i++) {
      if (pNewPoint->GetMeasure(i)->IsIgnored()) {
        continue;
      }
      else {
        double dDiff = dBestResolution - mdResVector[i];
        if (meType == Low) {
          if (dBestResolution == -1 || dDiff < 0) {
            dBestResolution = mdResVector[i];
            iBestIndex = i;
          }
        }
        else if (meType == High) {
          double dDiff = dBestResolution - mdResVector[i];
          if (dBestResolution == -1 || dDiff > 0) {
            dBestResolution = mdResVector[i];
            iBestIndex = i;
          }
        }
        else if (meType == Mean) {
          if ((int)mdResVector.size() == 2)  {
            // Arbitrarily assign the 1st measure to be reference for a point with only 2 measures
            iBestIndex  = 0;
          }
          else {
            double dDiff = fabs(dMean - mdResVector[i]);
            if (dBestResolution == -1 || dDiff <  dBestResolution) {
              dBestResolution = dDiff;
              iBestIndex = i;
            }
          }
        }
        else if (meType == Nearest) {
          double dDiff = fabs(mdResValue - mdResVector[i]);
          if (dBestResolution == -1 || dDiff <  dBestResolution) {
            dBestResolution = dDiff;
            iBestIndex = i;
          }
        }
        else if (meType == Range) {
          if (mdResVector[i] >=  mdMinRes && mdResVector[i] <= mdMaxRes) {
            iBestIndex = i;
            return iBestIndex;
          }
        }
      }
    }
    return iBestIndex;
  }
};
