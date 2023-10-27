#define GUIHELPERS

#include "Isis.h"

#include <QTime>

#include "Chip.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlNet.h"
#include "Cube.h"
#include "IException.h"
#include "iTime.h"
#include "Latitude.h"
#include "Longitude.h"
#include "PointPlot.h"
#include "Process.h"
#include "Progress.h"
#include "PvlGroup.h"
#include "SerialNumber.h"
#include "SmtkMatcher.h"
#include "SmtkPoint.h"
#include "Statistics.h"
#include "SurfacePoint.h"
#include "Target.h"
#include "TileManager.h"
#include "UserInterface.h"

using namespace std;
using namespace Isis;

//helper button functins in the code
void helperButtonLog();

map <QString,void*> GuiHelpers(){
  map <QString,void*> helper;
  helper ["helperButtonLog"] = (void*) helperButtonLog;
  return helper;
}

/** Function to create a Control point from an SmtkPoint */
ControlPoint CreatePoint(const SmtkPoint &spnt, const QString &pid,
                         const QString &lcn, const QString &rcn) {
  ControlPoint pnt(pid);
  Coordinate geom = spnt.getGeometry().getLeft();
  SurfacePoint surpnt(Latitude(geom.getLatitude(), Angle::Degrees),
                      Longitude(geom.getLongitude(), Angle::Degrees),
                      Distance(1.0, Distance::Kilometers));  // Kludge radius
  pnt.SetAprioriSurfacePoint(surpnt);

  ControlMeasure left, right;
  left.SetCoordinate(spnt.getLeft().getSample(), spnt.getLeft().getLine(),
                     ControlMeasure::Candidate);
  left.SetCubeSerialNumber(lcn);
  left.SetChooserName("smtk");

  right.SetCoordinate(spnt.getRight().getSample(), spnt.getRight().getLine(),
                      ControlMeasure::RegisteredSubPixel);
  right.SetCubeSerialNumber(rcn);
  right.SetChooserName("SmtkMatcher");
  right.SetLogData(ControlMeasureLogData(ControlMeasureLogData::GoodnessOfFit,
                                         spnt.GoodnessOfFit()));


  // Add measures and set reference
  pnt.Add(new ControlMeasure(left));
  pnt.Add(new ControlMeasure(right));
  pnt.SetRefMeasure(lcn);

  return (pnt);
}


/** Function that creates and writes a control network from a SmtkQStack */
void WriteCnet(const QString &netfile, SmtkQStack &points,
               const Pvl &label, const QString &lcn,
               const QString &rcn) {
   // Initialize control point network
  ControlNet cn;
  //  cn.SetType(ControlNet::ImageToImage);
  cn.SetTarget(label);
  cn.SetUserName(Application::UserName());
  cn.SetCreatedDate(iTime::CurrentLocalTime());
  SmtkQStackIter pnt = points.begin();
  for (int i = 0 ; pnt != points.end() ; i++, ++pnt) {
    QString pntid = "Point_" + toString(i);
    cn.AddPoint(new ControlPoint(CreatePoint(pnt.value(), pntid, lcn, rcn)));
  }

  cn.Write(netfile);
  return;
}


/** The ISIS smtk main application */
void IsisMain() {
  // 2016-10-14 Ian Humphrey -
  // Set the QHash seed, otherwise output is ALWAYS slightly different.
  // Note that in Qt4, the seed was constant, but in Qt5, the seed is created differently for each
  // process. Fixes #4058.
  qSetGlobalQHashSeed(1031);
  UserInterface &ui = Application::GetUserInterface();

  // Open the first cube.  It is the left hand image.
  Cube lhImage;
  lhImage.open(ui.GetCubeName("FROM"),"r");

  // Open the second cube, it is geomertricallty altered.  We will be matching the
  // first to this one by attempting to compute a sample/line offsets
  Cube rhImage;
  rhImage.open(ui.GetCubeName("MATCH"),"r");

  // Ensure only single bands
  if (lhImage.bandCount() != 1 || rhImage.bandCount() != 1) {
    QString msg = "Input Cubes must have only one band!";
    throw IException(IException::User,msg,_FILEINFO_);
  }

  //  Both images must have a Camera and can also have a Projection.  We will
  //  only deal with a Camera, however as a projected, non-mosaicked image
  //  uses a Projection internal to the Camera object.
  Camera *lhCamera = NULL;
  Camera *rhCamera = NULL;
  try {
    lhCamera = lhImage.camera();
    rhCamera = rhImage.camera();
  }
  catch (IException &ie) {
    QString msg = "Both input images must have a camera";
    throw IException(ie, IException::User, msg, _FILEINFO_);
  }

  //  Since we are generating a DEM, we must turn off any existing
  //  DEM that may have been initialized with spiceinit.
  lhCamera->IgnoreElevationModel(true);
  rhCamera->IgnoreElevationModel(true);

  // Get serial number
  QString serialLeft = SerialNumber::Compose(lhImage, true);
  QString serialRight = SerialNumber::Compose(rhImage, true);

//  This still precludes band to band registrations.
  if (serialLeft == serialRight) {
    QString sLeft = FileName(lhImage.fileName()).name();
    QString sRight = FileName(rhImage.fileName()).name();
    if (sLeft == sRight) {
      QString msg = "Cube Serial Numbers must be unique - FROM=" + serialLeft +
                   ", MATCH=" + serialRight;
      throw IException(IException::User,msg,_FILEINFO_);
    }
    serialLeft = sLeft;
    serialRight = sRight;
  }

  Progress prog;
  prog.SetText("Finding Initial Seeds");

  int nl = lhImage.lineCount();
  int ns = lhImage.sampleCount();
  BigInt numAttemptedInitialPoints = 0;

  //  Declare Gruen matcher
  SmtkMatcher matcher(ui.GetFileName("REGDEF"), &lhImage, &rhImage);

  // Get line/sample linc/sinc parameters
  int space   = ui.GetInteger("SPACE");
  int linc (space), sinc(space);

  // Do we have a seed points from a control net file?
  bool useseed = ui.WasEntered("CNET");

  // Base points on an input cnet
  SmtkQStack gstack;
  double lastEigen(0.0);
  if (useseed) {
    ControlNet cnet(ui.GetFileName("CNET"));
    prog.SetMaximumSteps(cnet.GetNumPoints());
    prog.CheckStatus();

    gstack.reserve(cnet.GetNumPoints());

    for (int cpIndex = 0; cpIndex < cnet.GetNumPoints(); cpIndex ++) {
      ControlPoint *cp = cnet.GetPoint(cpIndex);

      if (!cp->IsIgnored()) {
        ControlMeasure *cmLeft(0), *cmRight(0);
        for (int cmIndex = 0; cmIndex < cp->GetNumMeasures(); cmIndex ++) {
          ControlMeasure *cm = cp->GetMeasure(cmIndex);
          if (!cm->IsIgnored()) {
            if (cm->GetCubeSerialNumber() == serialLeft)
              cmLeft = cp->GetMeasure(cmIndex);
            if (cm->GetCubeSerialNumber() == serialRight)
              cmRight = cp->GetMeasure(cmIndex);
          }
        }

        //  If we have both left and right images in the control point, save it
        if ( (cmLeft != 0) && (cmRight != 0) ) {
          Coordinate left = Coordinate(cmLeft->GetLine(), cmLeft->GetSample());
          Coordinate right = Coordinate(cmRight->GetLine(), cmRight->GetSample());
          SmtkPoint spnt = matcher.Create(left, right);

          // Insert the point (unregistered)
          if ( spnt.isValid() ) {
            int line = (int) cmLeft->GetLine();
            int samp = (int) cmLeft->GetSample();
            matcher.isValid(spnt);
            gstack.insert(qMakePair(line, samp), spnt);
            lastEigen = spnt.GoodnessOfFit();
          }
        }
      }

      prog.CheckStatus();
    }
  }
  else {
  // We want to create a grid of control points that is N rows by M columns.

    int rows = (lhImage.lineCount() + linc - 1)/linc;
    int cols = (lhImage.sampleCount() + sinc - 1)/sinc;

    prog.SetMaximumSteps(rows * cols);
    prog.CheckStatus();

    // First pass stack and eigen value statistics
    SmtkQStack fpass;
    fpass.reserve(rows * cols);
    Statistics temp_mev;

    // Loop through grid of points and get statistics to compute
    // initial set of points
    for (int line = linc / 2 + 1; line < nl; line += linc) {
      for (int samp = sinc / 2 + 1 ; samp < ns; samp += sinc) {
        numAttemptedInitialPoints ++;
        SmtkPoint spnt = matcher.Register(Coordinate(line,samp));
        if ( spnt.isValid() ) {
          matcher.isValid(spnt);
          fpass.insert(qMakePair(line, samp), spnt);
          temp_mev.AddData(spnt.GoodnessOfFit());
        }
        prog.CheckStatus();
      }
    }

    //  Now select a subset of fpass points as the seed points
    cout << "Number of Potential Seed Points: " << fpass.size() << "\n";
    cout << "Min / Max Eigenvalues Matched: " << temp_mev.Minimum() << ", "
         << temp_mev.Maximum() << "\n";

    // How many seed points are requested
    double nseed = ui.GetDouble("NSEED");
    int inseed;
    if (nseed >= 1.0) inseed = (int) nseed;
    else if (nseed > 0.0) inseed = (int) (nseed * (double) (fpass.size()));
    else inseed = (int) ((double) (fpass.size()) * 0.05);

    double seedsample = ui.GetDouble("SEEDSAMPLE");

    // Generate a new stack
    gstack.reserve(inseed);
    while ((gstack.size() < inseed) && (!fpass.isEmpty() )) {
      SmtkQStack::iterator bestm;
      if (seedsample <= 0.0) {
        bestm = matcher.FindSmallestEV(fpass);
      }
      else {
        bestm = matcher.FindExpDistEV(fpass, seedsample, temp_mev.Minimum(),
                                      temp_mev.Maximum());
      }

      //  Add point to stack
      if (bestm != fpass.end()) {
        Coordinate right = bestm.value().getRight();
        matcher.isValid(bestm.value());
        gstack.insert(bestm.key(), bestm.value());
        lastEigen = bestm.value().GoodnessOfFit();
        fpass.erase(bestm);
      }
    }

    // If a user wants to see the seed network, write it out here
    if (ui.WasEntered("OSEEDNET")) {
      WriteCnet(ui.GetFileName("OSEEDNET"), gstack,
                *lhImage.label(), serialLeft, serialRight);
    }

  }

  ///////////////////////////////////////////////////////////////////////
  // All done with seed points.  Sanity check ensures we actually found
  // some.
  ///////////////////////////////////////////////////////////////////////
  if (gstack.size() <= 0) {
    QString msg = "No seed points found - may need to check Gruen parameters.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  //  Report seed point status
  if (!useseed) {
    cout << "Number of Seed Points used: " << gstack.size() << "\n";
    cout << "EV of last Seed Point:      " << lastEigen << "\n";
  }
  else {
    cout << "Number of Manual Seed Points:   " << gstack.size() << "\n";
  }

  // Use seed points (in stack) to grow
  SmtkQStack bmf;
  bmf.reserve(gstack.size());  // Probably need much more but for starters...

  BigInt numOrigPoints = gstack.size();
  BigInt passpix2 = 0;

  int subcbox = ui.GetInteger("SUBCBOX");
  int halfBox((subcbox-1)/2);
  while (!gstack.isEmpty()) {

    SmtkQStackIter cstack = matcher.FindSmallestEV(gstack);

    // Print number on stack
    if ((gstack.size() % 1000) == 0) {
      cout << "Number on Stack: " << gstack.size()
           << ". " << cstack.value().GoodnessOfFit() << "\n";
    }

    // Test to see if already determined
    SmtkQStackIter bmfPt = bmf.find(cstack.key());
    if (bmfPt == bmf.end()) {
      // Its not in the final stack, process it

      //  Retrieve the point
      SmtkPoint spnt = cstack.value();
      //  Register if its not already registered
      if (!spnt.isRegistered()) {
        spnt = matcher.Register(spnt, spnt.getAffine());
      }

      // Still must check for validity if the point was just registered,
      // otherwise should be good
      if ( spnt.isValid() ) {
        passpix2++;
        bmf.insert(cstack.key(), spnt);  // inserts (0,0) offset excluded below
        int line   = cstack.key().first;
        int sample = cstack.key().second;

        //  Determine match points
        double eigen(spnt.GoodnessOfFit());
        for (int sampBox = -halfBox ; sampBox <= halfBox ; sampBox++ ) {
          int csamp = sample + sampBox;
          for (int lineBox = -halfBox ; lineBox <= halfBox ; lineBox++) {
            int cline = line + lineBox;
            if ( !( (sampBox == 0) && (lineBox == 0)) ) {// Already added above
              SmtkQPair dupPair(cline, csamp);
              SmtkQStackIter temp = bmf.find(dupPair);
              SmtkPoint bmfpnt;
              if (temp != bmf.end()) {
                if (temp.value().GoodnessOfFit() > eigen) {
                  // Create cloned point with better fit
                  bmfpnt = matcher.Clone(spnt, Coordinate(cline,csamp));
                }
              }
              else {  // ISIS2 is BMF(SAMP,LINE,7) .EQ VALID_MAX4)
                // Clone new point for insert
                bmfpnt = matcher.Clone(spnt, Coordinate(cline,csamp));
              }

              //  Add if good point
              if (bmfpnt.isValid()) {
                bmf.insert(dupPair, bmfpnt);
              }
            }
          }
        }

        // Grow stack with spacing adding info to stack
        for (int i = -1 ; i <= 1 ; i ++) {  // Sample
          for (int j = -1 ; j <= 1 ; j ++) {  // Line
            // Don't re-add the original sample, line
            if ( !((i == 0) && (j == 0)) ) {
              //  Grow based upon spacing
              double ssamp = sample + (i * space);
              double sline = line   + (j * space);
              Coordinate pnt = Coordinate(sline, ssamp);
              SmtkPoint gpnt = matcher.Clone(spnt, pnt);

              if ( gpnt.isValid() ) {
                SmtkQPair growpt((int) sline, (int) ssamp);

                // double check we don't have a finalized result at this position
                SmtkQStackIter temp = bmf.find(growpt);
                if (temp == bmf.end()) {
                  gstack.insert(growpt, gpnt);
                }
              }
            }
          }
        }
      }
    }

    // Remove the current point from the grow stack (hole)
    gstack.erase(cstack);
  }

/////////////////////////////////////////////////////////////////////////
// All done with creating points.  Perform output options.
/////////////////////////////////////////////////////////////////////////

  // If a TO parameter was specified, create DEM with errors
  if (ui.WasEntered("TO")) {
    //  Create the output DEM
    cout << "\nCreating output DEM from " << bmf.size() << " points.\n";
    Process  p;
    Cube *icube = p.SetInputCube("FROM");
    Cube *ocube = p.SetOutputCube("TO", icube->sampleCount(),
                                  icube->lineCount(), 3);
    p.ClearInputCubes();

    int boxsize = ui.GetInteger("BOXSIZE");
    double plotdist = ui.GetDouble("PLOTDIST");

    TileManager dem(*ocube), eigen(*ocube), stErr(*ocube);
    dem.SetTile(1, 1);      //  DEM Data/elevation
    stErr.SetTile(1, 2);    //  Error in stereo computation
    eigen.SetTile(1, 3);    //  Eigenvalue of the solution

    int nBTiles(eigen.Tiles()/3);  // Total tiles / 3 bands

    prog.SetText("Creating DEM");
    prog.SetMaximumSteps(nBTiles);
    prog.CheckStatus();

    Statistics stAng;
    while ( !eigen.end() ) {   // Must use the last band for this!!
      PointPlot tm = for_each(bmf.begin(), bmf.end(), PointPlot(dem, plotdist));
      tm.FillPoints(*lhCamera, *rhCamera, boxsize, dem, stErr, eigen, &stAng);

      ocube->write(dem);
      ocube->write(stErr);
      ocube->write(eigen);

      dem.next();
      stErr.next();
      eigen.next();

      prog.CheckStatus();
    }

    //  Report Stereo separation angles
    PvlGroup stresultsPvl("StereoSeparationAngle");
    stresultsPvl += PvlKeyword("Minimum", std::to_string(stAng.Minimum()), "deg");
    stresultsPvl += PvlKeyword("Average", std::to_string(stAng.Average()), "deg");
    stresultsPvl += PvlKeyword("Maximum", std::to_string(stAng.Maximum()), "deg");
    stresultsPvl += PvlKeyword("StandardDeviation", std::to_string(stAng.StandardDeviation()), "deg");
    Application::Log(stresultsPvl);

    // Update the label with BandBin keywords
    PvlKeyword filter("FilterName", "Elevation", "meters");
    filter.addValue("ElevationError", "meters");
    filter.addValue("GoodnessOfFit", "unitless");
    PvlKeyword center("Center", "1.0");
    center.addValue("1.0");
    center.addValue("1.0");

    PvlGroup &bandbin = ocube->label()->findGroup("BandBin", PvlObject::Traverse);
    bandbin.addKeyword(filter, PvlContainer::Replace);
    bandbin.addKeyword(center, PvlContainer::Replace);
    center.setName("Width");
    bandbin.addKeyword(center, PvlContainer::Replace);


    p.EndProcess();
  }

  // If a cnet file was entered, write the ControlNet pvl to the file
  if (ui.WasEntered("ONET")) {
    WriteCnet(ui.GetFileName("ONET"), bmf, *lhImage.label(), serialLeft,
              serialRight);
  }

  // Create output data
  PvlGroup totalPointsPvl("Totals");
  totalPointsPvl += PvlKeyword("AttemptedPoints", std::to_string(numAttemptedInitialPoints));
  totalPointsPvl += PvlKeyword("InitialSuccesses", std::to_string(numOrigPoints));
  totalPointsPvl += PvlKeyword("GrowSuccesses", std::to_string(passpix2));
  totalPointsPvl += PvlKeyword("ResultingPoints", std::to_string(bmf.size()));

  Application::Log(totalPointsPvl);

  Pvl arPvl = matcher.RegistrationStatistics();
  PvlGroup smtkresultsPvl("SmtkResults");
  smtkresultsPvl += PvlKeyword("SpiceOffImage", std::to_string(matcher.OffImageErrorCount()));
  smtkresultsPvl += PvlKeyword("SpiceDistanceError", std::to_string(matcher.SpiceErrorCount()));
  arPvl.addGroup(smtkresultsPvl);

  for (int i = 0; i < arPvl.groups(); i++) {
    Application::Log(arPvl.group(i));
  }

  // add the auto registration information to print.prt
  PvlGroup autoRegTemplate = matcher.RegTemplate();
  Application::Log(autoRegTemplate);

  // Don't need the cubes opened anymore
  lhImage.close();
  rhImage.close();
}


//Helper function to output the regdeft file to log.
void helperButtonLog () {
  UserInterface &ui = Application::GetUserInterface();
  QString file(ui.GetFileName("REGDEF"));
  Pvl p;
  p.read(file.toStdString());
  Application::GuiLog(p);
}
//...........end of helper function ........
