/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "cnetstats.h"
#include "Application.h"
#include "ControlNetFilter.h"
#include "ControlNetStatistics.h"
#include "IException.h"
#include "PvlGroup.h"
#include "Progress.h"

using namespace std;
namespace Isis{
  void ReadDefFile(ControlNetFilter & pcNetFilter, Pvl & pvlDefFile);
  void (ControlNetFilter::*GetPtr2Filter(const QString sFilter)) (const PvlGroup & pvlGrp, bool pbLastFilter);

    /**
    * Default method for cnetstats that takes a UI object from the
    * application, parses the necessary UI elements and prints stats
    * on the control network.
    *
    *
    * @param ui UserInterface object generated using the cnetstats.xml file.
    *
    * @param progress A progress object for the UI to get progress reports
    *                 through.
    */
  void cnetstats(UserInterface &ui, Pvl *log) {
    ControlNet innet(ui.GetFileName("CNET"));
    QString inlist(ui.GetFileName("FROMLIST"));

    cnetstats(innet, inlist, ui, log);
  }

  /**
    * Given some control network and criteria passed in through the UI,
    * return some stats on the control network and its images.
    *
    * @param net A control network object.
    *
    * @param serialNumFile A QString of the name of the file containing the serial
    *        numbers for all cubes within the network often generated from
    *        a filelist.
    *
    * @param ui UserInterface object generated using the cnetwinnow.xml file.
    *
    * @param log A pvl object used to store log information.
    *
    */
  void cnetstats(ControlNet &innet, QString &serialNumFile, UserInterface &ui, Pvl *log) {
    try {
      // Get the DefFile
      QString sDefFile = "";
      QString sOutFile = "";
      Pvl pvlDefFile;
      if (ui.WasEntered("DEFFILE")) {
        sDefFile = ui.GetFileName("DEFFILE");
        sOutFile = ui.GetFileName("FLATFILE");
        pvlDefFile = Pvl(sDefFile);

        // Log the DefFile - Cannot log Object... only by Group
        for (int i=0; i<pvlDefFile.objects(); i++) {
          PvlObject pvlObj = pvlDefFile.object(i);
          for (int j=0; j<pvlObj.groups(); j++) {
            Application::AppendAndLog(pvlObj.group(j), log);
          }
        }

        // Verify DefFile comparing with the Template
        Pvl pvlTemplate("$ISISROOT/appdata/templates/cnetstats/cnetstats.def");
        Pvl pvlResults;
        pvlTemplate.validatePvl(pvlDefFile, pvlResults);
        if(pvlResults.objects() != 0 || pvlResults.groups() != 0 || pvlResults.keywords() != 0){
          for (int i=0; i<pvlResults.objects(); i++) {
            PvlObject pvlObj = pvlResults.object(i);
            for (int j=0; j<pvlObj.groups(); j++) {
              Application::AppendAndLog(pvlObj.group(j), log);
            }
          }
          QString sErrMsg = "Invalid Deffile\n";
          throw IException(IException::User, sErrMsg, _FILEINFO_);
        }
      }

      // Get the Image Stats File
      QString sImageFile= "";
      if (ui.WasEntered("CREATE_IMAGE_STATS") && ui.GetBoolean("CREATE_IMAGE_STATS")) {
        sImageFile = ui.GetFileName("IMAGE_STATS_FILE");
      }

      // Get the Point Stats File
      QString sPointFile="";
      if (ui.WasEntered("CREATE_POINT_STATS") && ui.GetBoolean("CREATE_POINT_STATS")) {
        sPointFile = ui.GetFileName("POINT_STATS_FILE");
      }

       // Get the original control net internalized
      Progress progress;
      // ControlNet cNet(ui.GetFileName("CNET"), &progress);

      Progress statsProgress;
      ControlNetFilter cNetFilter(&innet, serialNumFile, &statsProgress);

      // Log the summary of the input Control Network
      PvlGroup statsGrp;
      cNetFilter.GenerateControlNetStats(statsGrp);
      Application::AppendAndLog(statsGrp, log);

      // Run Filters using Deffile
      if (ui.WasEntered("DEFFILE")) {
        cNetFilter.SetOutputFile(sOutFile);
        ReadDefFile(cNetFilter, pvlDefFile);
      }

      // Run Image Stats
      if (ui.WasEntered("CREATE_IMAGE_STATS") && ui.GetBoolean("CREATE_IMAGE_STATS")) {
        cNetFilter.GenerateImageStats();
        cNetFilter.PrintImageStats(sImageFile);
      }

      // Run Point Stats
      if (ui.WasEntered("CREATE_POINT_STATS") && ui.GetBoolean("CREATE_POINT_STATS")) {
        cNetFilter.GeneratePointStats(sPointFile);
      }
    } // REFORMAT THESE ERRORS INTO ISIS TYPES AND RETHROW
    catch(IException &) {
      throw;
    }
  }

  /**
   * Reads the DefFile having info about the different filters to
   * be used on the Control Network.
   *
   * @author Sharmila Prasad (9/7/2010)
   *
   * @param pcNetFilter
   * @param pvlDefFile
   */
  void ReadDefFile(ControlNetFilter & pcNetFilter, Pvl & pvlDefFile)
  {
    // prototype to ControlNetFilter member function
    void (ControlNetFilter::*pt2Filter)(const PvlGroup & pvlGrp, bool pbLastFilter);

    // Parse the Groups in Point Object
    PvlObject filtersObj = pvlDefFile.findObject("Filters", Pvl::Traverse);
    int iNumGroups = filtersObj.groups();

    for (int i=0; i<iNumGroups; i++) {
      PvlGroup pvlGrp = filtersObj.group(i);
      // Get the pointer to ControlNetFilter member function based on Group name
      pt2Filter=GetPtr2Filter(pvlGrp.name());
      if (pt2Filter != NULL) {
        (pcNetFilter.*pt2Filter)(pvlGrp, ((i==(iNumGroups-1)) ? true : false));
      }
    }
  }

  /**
   * Returns the pointer to filter function based on the Group name QString
   *
   * @author Sharmila Prasad (8/11/2010)
   *
   * @return void(ControlNetFilter::*GetPtr2Filter)(const PvlGroup&pvlGrp)
   */
  void (ControlNetFilter::*GetPtr2Filter(const QString psFilter)) (const PvlGroup & pvlGrp, bool pbLastFilter)
  {
    // Point Filters
    if (psFilter == "Point_PixelShift") {
      return &ControlNetFilter::PointPixelShiftFilter;
    }
    if (psFilter == "Point_EditLock")
      return &ControlNetFilter::PointEditLockFilter;

    if (psFilter == "Point_NumMeasuresEditLock") {
      return &ControlNetFilter::PointNumMeasuresEditLockFilter;
    }

    if (psFilter == "Point_ResidualMagnitude"){
      return &ControlNetFilter::PointResMagnitudeFilter;
    }

    if (psFilter == "Point_GoodnessOfFit") {
      return &ControlNetFilter::PointGoodnessOfFitFilter;
    }

    if (psFilter == "Point_IdExpression") {
      return &ControlNetFilter::PointIDFilter;
    }

    if (psFilter == "Point_NumMeasures"){
      return &ControlNetFilter::PointMeasuresFilter;
    }

    if (psFilter == "Point_Properties") {
      return &ControlNetFilter::PointPropertiesFilter;
    }

    if (psFilter == "Point_LatLon") {
      return &ControlNetFilter::PointLatLonFilter;
    }

    if (psFilter == "Point_Distance") {
      return &ControlNetFilter::PointDistanceFilter;
    }

    if (psFilter == "Point_MeasureProperties") {
      return &ControlNetFilter::PointMeasurePropertiesFilter;
    }

    if (psFilter == "Point_CubeNames") {
      return &ControlNetFilter::PointCubeNamesFilter;
    }

    // Cube Filters
    if (psFilter == "Cube_NameExpression") {
      return &ControlNetFilter::CubeNameExpressionFilter;
    }

    if (psFilter == "Cube_NumPoints") {
      return &ControlNetFilter::CubeNumPointsFilter;
    }

    if (psFilter == "Cube_Distance") {
      return &ControlNetFilter::CubeDistanceFilter;
    }

    if (psFilter == "Cube_ConvexHullRatio") {
      return &ControlNetFilter::CubeConvexHullFilter;
    }

    return NULL;
  }
}
