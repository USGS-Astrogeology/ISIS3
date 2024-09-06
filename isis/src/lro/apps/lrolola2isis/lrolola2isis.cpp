#include "lrolola2isis.h"

using namespace std;

namespace Isis {

  struct LidarCube {
    FileName name;
    QString sn;
    iTime startTime;
    iTime endTime;
  };

  
  void lrolola2isis(UserInterface &ui) {
    FileList filelist;
    
    // Get the Lidar csv data from either/or both FROM and FROMLIST
    if (ui.WasEntered("FROM")) filelist.push_back(FileName(ui.GetFileName("FROM")));
    if (ui.WasEntered("FROMLIST")) filelist.read(FileName(ui.GetFileName("FROMLIST")));
    
    if (filelist.size() < 1) {
      std::string msg =
        "Input CSV files must be specified in FROM and/or FROMLIST - no files were found.";
      throw IException(IException::User,msg,_FILEINFO_);
    }

    SerialNumberList cubeList = SerialNumberList(ui.GetFileName("CUBES"));
    double threshold = ui.GetDouble("THRESHOLD");
    double rangeSigma = ui.GetDouble("POINT_RANGE_SIGMA");

    double latSigma = Isis::Null;
    double lonSigma = Isis::Null;
    double radiusSigma = Isis::Null;

    if (ui.WasEntered("POINT_LATITUDE_SIGMA")) {
      latSigma = ui.GetDouble("POINT_LATITUDE_SIGMA");
    }
    if (ui.WasEntered("POINT_LONGITUDE_SIGMA")) {
      lonSigma = ui.GetDouble("POINT_LONGITUDE_SIGMA");
    }
    if (ui.WasEntered("POINT_RADIUS_SIGMA")) {
      radiusSigma = ui.GetDouble("POINT_RADIUS_SIGMA");
    }

    // Read necessary label information from cubes
    QList<LidarCube> images;
    
    for (int i = 0; i < cubeList.size(); i++) {
      LidarCube lidarCube;
      QString serialNumber = cubeList.serialNumber(i);
      FileName fileName = FileName(cubeList.fileName(serialNumber));
      Cube cube(fileName);

      lidarCube.name = fileName;
      lidarCube.sn = serialNumber;
      std::pair< double, double> startEndTime = cube.camera()->StartEndEphemerisTimes();
      lidarCube.startTime = iTime(startEndTime.first);
      lidarCube.endTime = iTime(startEndTime.second);
      
      images.append(lidarCube);
    }
    
    LidarData lidarDataSet;
    CubeManager cubeMgr;
    Distance  majorAx;
    Distance minorAx;
    Distance polarAx;
    
    // Set up an automatic id generator for the point ids
    ID pointId = ID(ui.GetString("POINTID"));

    // Loop through Lidar csv data file(s) and load the data into a single LidarData object, LidarDataSet
    for (int ifile = 0; ifile < filelist.size(); ifile++) {
      CSVReader lidarDataFile;
      lidarDataFile.read(filelist[ifile].expanded());
       
      // Start at 1 to skip the header. TODO actually set a header in lidarDataFile
      for (int i = 1; i < lidarDataFile.rows(); i++) {
        CSVReader::CSVAxis row = lidarDataFile.getRow(i);
        
        iTime time(row[0]);
        Latitude lat(row[2].toDouble(), Angle::Units::Degrees);
        Longitude lon(row[1].toDouble(), Angle::Units::Degrees);
        Distance radius(row[3].toDouble(), Distance::Units::Kilometers);
        double range = row[4].toDouble();
        //     QString quality = row[]; //TODO figure out how/where to find this value
        

        LidarControlPoint *lidarPoint = new LidarControlPoint;
        lidarPoint->SetId(pointId.Next()); 
        lidarPoint->setTime(time);
        lidarPoint->setRange(range);
        lidarPoint->setSigmaRange(rangeSigma);

        // Set the point coordinates and their sigmas and add to the Lidar Control Point
        SurfacePoint spoint(lat, lon, radius);
        spoint.SetSphericalSigmasDistance(
                                          Distance(latSigma, Distance::Units::Meters),
                                          Distance(lonSigma, Distance::Units::Meters),
                                          Distance(radiusSigma, Distance::Units::Meters));
        lidarPoint->SetAprioriSurfacePoint(spoint);

        // Loop through images to set measures in the Lidar Control Point
        for (int j = 0; j < images.size(); j++) {
          Cube *cube = cubeMgr.OpenCube(images[j].name.expanded());
          
          if (cube != NULL) {
            Camera *camera = cube->camera();
            
            if (camera != NULL) {
              if (camera->SetGround(spoint)) {
                double samp = camera->Sample();
                double line = camera->Line();
                
                if (samp > 0.5 - threshold   &&   line > 0.5 - threshold
                    && samp < camera->Samples() + .5  &&  line < camera->Lines() + .5) {
                  ControlMeasure *measure = new ControlMeasure;
                  measure->SetCoordinate(camera->Sample(), camera->Line()); 
                  measure->SetCubeSerialNumber(images[j].sn);
                  lidarPoint->Add(measure);
                  if (time >= images[j].startTime && time <= images[j].endTime) {
                    QString newSerial = measure->GetCubeSerialNumber();
                    lidarPoint->addSimultaneous(newSerial);
                  }
                }
              }
            }
            else {
              std::string msg = "Unable to create a camera from " + images[j].name.expanded();
              throw IException(IException::Unknown, msg, _FILEINFO_);
            }
          }
          else {
            std::string msg = "Unable to open a cube from " + images[j].name.expanded();
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }        
        }
        // end image loop

        if (lidarPoint->GetNumMeasures() <= 0 ||
            lidarPoint->snSimultaneous().size() <=0) {
          continue;
        }
      
        lidarDataSet.insert(QSharedPointer<LidarControlPoint>(lidarPoint));
      } // End loop on lidarDataFile rows
      lidarDataFile.clear();
    } // End loop on lidar data file list

    if (ui.GetString("OUTPUTTYPE") == "JSON") {
      lidarDataSet.write(ui.GetFileName("TO"), LidarData::Format::Json);
    }
    else if (ui.GetString("OUTPUTTYPE") == "TEST") {
      lidarDataSet.write(ui.GetFileName("TO"), LidarData::Format::Test);
    }
    else {
      lidarDataSet.write(ui.GetFileName("TO"), LidarData::Format::Binary);
    }
  }
}
