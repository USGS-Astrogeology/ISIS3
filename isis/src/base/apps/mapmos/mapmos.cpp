#include "mapmos.h"

#include <QDebug>

#include "Application.h"
#include "ProcessMapMosaic.h"
#include "PvlGroup.h"

using namespace std;
using namespace Isis;

namespace Isis {

  void mapmos(UserInterface &ui, Pvl *log) {
    Cube *inCube = new Cube( ui.GetCubeName("FROM").toStdString(), "r");
    mapmos(inCube, ui, log);
  }


 void mapmos(Cube *inCube, UserInterface &ui, Pvl *log) { 
    // Get the user interface
    ProcessMapMosaic m;

    // Get the MatchBandBin Flag
    m.SetBandBinMatch(ui.GetBoolean("MATCHBANDBIN"));

    // Get the MatchDEM Flag
    m.SetMatchDEM(ui.GetBoolean("MATCHDEM"));

    // Get the track flag
    bool bTrack = ui.GetBoolean("TRACK");
    m.SetTrackFlag(bTrack);

    // Gets the input file along with attributes
    QString sInputFile = inCube->fileName();

    ProcessMosaic::ImageOverlay overlay = ProcessMosaic::StringToOverlay( ui.GetString("PRIORITY") );

    if (overlay == ProcessMosaic::UseBandPlacementCriteria) {
      if(ui.GetString("TYPE") == "BANDNUMBER") {
        m.SetBandNumber(ui.GetInteger("NUMBER"));
      }
      else {
        // Key name & value
        m.SetBandKeyword(ui.GetString("KEYNAME"), ui.GetString("KEYVALUE"));
      }
      // Band Criteria
      m.SetBandUseMaxValue( (ui.GetString("CRITERIA") == "GREATER") );
    }

    // Priority
    m.SetImageOverlay(overlay);

    // Get the output projection set up properly
    if(ui.GetBoolean("CREATE")) {

      // Set the create flag
      m.SetCreateFlag(true);

      // Use the input projection as a starting point for the mosaic
      PvlGroup mapGroup = inCube->label()->findGroup("Mapping", Pvl::Traverse);
      inCube->close();
      
      if ( ui.WasEntered("MINLAT") ) { 
        mapGroup.addKeyword(PvlKeyword( "MinimumLatitude",  std::to_string( ui.GetDouble("MINLAT") ) ),
                            Pvl::Replace);
      }
      if ( ui.WasEntered("MAXLAT") ) {
        mapGroup.addKeyword(PvlKeyword( "MaximumLatitude",  std::to_string( ui.GetDouble("MAXLAT") ) ),
                            Pvl::Replace);
      }
      if ( ui.WasEntered("MINLON") ) {
        mapGroup.addKeyword(PvlKeyword( "MinimumLongitude", std::to_string( ui.GetDouble("MINLON") ) ),
                            Pvl::Replace);
      }
      if ( ui.WasEntered("MAXLON") ) {
        mapGroup.addKeyword(PvlKeyword( "MaximumLongitude", std::to_string( ui.GetDouble("MAXLON") ) ),
                            Pvl::Replace);
      }
      //check to make sure they're all there. If not, throw error, need to enter all.
      if (!mapGroup.hasKeyword("MinimumLongitude") || !mapGroup.hasKeyword("MaximumLongitude") ||
          !mapGroup.hasKeyword("MinimumLatitude") || !mapGroup.hasKeyword("MaximumLatitude") ) {
        std::string msg = "One of the extents is missing. Please input all extents.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      
      CubeAttributeOutput oAtt = ui.GetOutputAttribute("MOSAIC");
      m.SetOutputCube(sInputFile, mapGroup, oAtt, ui.GetCubeName("MOSAIC"));
    }
    else {
      m.SetOutputCube(ui.GetCubeName("MOSAIC"));
    }


    m.SetHighSaturationFlag(ui.GetBoolean("HIGHSATURATION"));
    m.SetLowSaturationFlag(ui.GetBoolean("LOWSATURATION"));
    m.SetNullFlag(ui.GetBoolean("NULL"));

    // Start Process  
    if(!m.StartProcess(sInputFile)) {
      // Logs the cube if it falls outside of the given mosaic
      PvlGroup outsiders("Outside");
      outsiders += PvlKeyword("File", sInputFile.toStdString());
      Application::AppendAndLog(outsiders, log);
    }
    else {
      // Logs the input file location in the mosaic
      for (int i = 0; i < m.imagePositions().groups(); i++) {
        Application::AppendAndLog(m.imagePositions().group(i), log);
      }
    }


    if(bTrack != m.GetTrackFlag()) {
      ui.Clear("TRACK");
      ui.PutBoolean("TRACK", m.GetTrackFlag());
    }

    m.EndProcess();
  }

}
