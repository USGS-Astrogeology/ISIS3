/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */


#include "Isis.h"

//C++ standard libraries if needed
#include <math.h>

//QT libraries if needed if needed

//third party libraries if needed
#include "NaifStatus.h"

//Isis Headers if needed
#include "ApolloPanoramicCamera.h"
#include "ApolloPanoramicDetectorMap.h"
#include "AutoReg.h"
#include "AutoRegFactory.h"
#include "Camera.h"
#include "CameraDetectorMap.h"
#include "Centroid.h"
#include "CentroidApolloPan.h"
#include "Chip.h"
#include "Cube.h"
#include "FileName.h"
#include "IString.h"
#include "iTime.h"
#include "JP2Decoder.h"
#include "ProcessImport.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "PvlTranslationTable.h"
#include "Spice.h"
#include "SpicePosition.h"
#include "SpiceRotation.h"
#include "Table.h"
#include "UserInterface.h"


#define FIDL  26.72093    //spacing between fiducial marks in mm
#define ROLLC  74.0846291699105  //constant to convert from V/H to roll speed in rads/sec

#define NODES  87  //must be odd!!!  number of nodes to put in tables

//constants for 5 micro resolution scans
#define SCALE   10.0   //scale used for down sizingS patternS an search chips
#define SEARCHh 1400.0  //number of lines (in 5-micron-pixels) in search space for the first fiducial
#define SEARCHc  350.0  //number of samples per edge(in 5-micron-pixels) in each sub-search area
#define AVERs  5286.0  //average smaples (in 5-micron-pixels) between fiducials
#define AVERl  23459.0  //average diference (in 5-micron-pixels) between the top and bottom fiducials
#define TRANS_N 28520.0  //nomimal dx between scan lines, scani x + 28532 = (approx) scani+1 x--also
                         //  the size of the search area for the first fiducial



using namespace std;
using namespace Isis;


void Load_Kernel(Isis::PvlKeyword &key);
void crossp(double v1[3], double v2[3], double v1cv2[3]);
void Geographic2GeocentricLunar(double geographic[3], double geocentric[3]);
void MfromLeftEulers(double M[3][3], double omega, double phi, double kappa);
void MfromVecLeftAngle(double M[3][3], double vec[3], double angle);
void M2Q(double M[3][3], double Q[4]);


double R_MOON[3];

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  double  time0,//start time
          time1,//end time
          alti,  //altitude of the spacecraftmore
          fmc,  //forward motion compensation rad/sec
          horV,  //horizontal velocity km/sec
          radV,  //radial velocity km/sec
          rollV,//roll speed in rad/sec
          led;  //line exposure duration in seconds

  Cube  panCube;
  iTime  isisTime;
  QString iStrTEMP;

  int i,j,k,scFrameCode,insCode;

  QString mission;

  SpicePosition *spPos;
  SpiceRotation *spRot;

  //int nlines,nsamples,nbands;

  double deg2rad = acos(-1.0)/180.0;

  ProcessImport jp;
  FileName transFile("$ISISROOT/appdata/translations/ApolloPanInit.trn");
  PvlTranslationTable transTable(transFile);
  PvlGroup kernels_pvlG;

  //scFrameCode and insCode from user input
  mission = ui.GetString("MISSION");
  if (mission == "APOLLO12") scFrameCode = -912000;
  if (mission == "APOLLO14") scFrameCode = -914000;
  if (mission == "APOLLO15") scFrameCode = -915000;
  if (mission == "APOLLO16") scFrameCode = -916000;
  if (mission == "APOLLO17") scFrameCode = -917000;

  insCode = scFrameCode - 230;

  try {
    panCube.open(ui.GetCubeName("FROM"),"rw");
  }
  catch (IException &e) {
    throw IException(IException::User,
                     "Unable to open the file [" + ui.GetCubeName("FROM") + "] as a cube.",
                     _FILEINFO_);
  }

  ////////////////////////////////////////////build the cube header instrament group
  PvlGroup inst_pvlG("Instrument");

  PvlKeyword keyword;

  //four that are the same for every panaramic mission
  keyword.setName("SpacecraftName");
  keyword.setValue(mission.toStdString());
  inst_pvlG.addKeyword(keyword);

  keyword.setName("InstrumentName");
  keyword.setValue(transTable.Translate("InstrumentName","whatever").toStdString());
  inst_pvlG.addKeyword(keyword);

  keyword.setName("InstrumentId");
  keyword.setValue(transTable.Translate("InstrumentId","whatever").toStdString());
  inst_pvlG.addKeyword(keyword);

  keyword.setName("TargetName");
  keyword.setValue(transTable.Translate("TargetName","whatever").toStdString());
  inst_pvlG.addKeyword(keyword);

  //three that need to be calculated from input values
  horV = ui.GetDouble("VEL_HORIZ");
  radV = ui.GetDouble("VEL_RADIAL");
  alti = ui.GetDouble("CRAFT_ALTITUDE");

  //caculate the LineExposureDuration (led)
  if( ui.WasEntered("V/H_OVERRIDE") )
    fmc = ui.GetDouble("V/H_OVERRIDE")/1000.0;
  else
    //forward motion compensation is directly equivalent to V/H
    fmc = sqrt(horV*horV + radV*radV)/alti;
  rollV = fmc*ROLLC;  //roll angular velcoity is equal to  V/H * constant    (units rad/sec)
  //led = rad/mm * sec/rad = radians(2.5)/FIDL / rollV    (final units: sec/mm)
  led = (2.5*acos(-1.0)/180.0)/rollV/FIDL;

  //use led and the number of mm to determine the start and stop times
  isisTime = ui.GetString("GMT");

  //calculate starting and stoping times
  time0 = isisTime.Et() - led*FIDL*21.5;
  time1 = time0 + led*FIDL*43;

  isisTime = time0;
  keyword.setName("StartTime");
  keyword.setValue(isisTime.UTC().toStdString());
  inst_pvlG.addKeyword(keyword);

  isisTime = time1;
  keyword.setName("StopTime");
  keyword.setValue(isisTime.UTC().toStdString());
  inst_pvlG.addKeyword(keyword);

  keyword.setName("LineExposureDuration");
  //converted led to msec/mm--negative sign to account for the anti-parallel time and line axes
  keyword.setValue(std::to_string(-led),"sec/mm");
  inst_pvlG.addKeyword(keyword);

  panCube.putGroup(inst_pvlG);

  ///////////////////////////////////The kernels group
  kernels_pvlG.setName("Kernels");
  kernels_pvlG.clear();

  keyword.setName("NaifFrameCode");
  keyword.setValue(std::to_string(insCode));
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("LeapSecond");
  keyword.setValue( transTable.Translate("LeapSecond","File1").toStdString() );
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("TargetAttitudeShape");
  keyword.setValue( transTable.Translate("TargetAttitudeShape", "File1").toStdString() );
  keyword.addValue( transTable.Translate("TargetAttitudeShape", "File2").toStdString() );
  keyword.addValue( transTable.Translate("TargetAttitudeShape", "File3").toStdString() );
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("TargetPosition");
  keyword.setValue("Table");
  keyword.addValue( transTable.Translate("TargetPosition", "File1").toStdString() );
  keyword.addValue( transTable.Translate("TargetPosition", "File2").toStdString() );
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("ShapeModel");
  keyword.setValue( transTable.Translate("ShapeModel", "File1").toStdString() );
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("InstrumentPointing");
  keyword.setValue("Table");
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("InstrumentPosition");
  keyword.setValue("Table");
  kernels_pvlG.addKeyword(keyword);

  keyword.setName("InstrumentAddendum");
  keyword.setValue( transTable.Translate("InstrumentAddendum", mission).toStdString());
  kernels_pvlG.addKeyword(keyword);

  panCube.putGroup(kernels_pvlG);

  //Load all the kernels
  Load_Kernel(kernels_pvlG["TargetPosition"]);
  Load_Kernel(kernels_pvlG["TargetAttitudeShape"]);
  Load_Kernel(kernels_pvlG["LeapSecond"]);

  //////////////////////////////////////////attach a target rotation table
  char frameName[32];
  SpiceInt frameCode;
  SpiceBoolean found;
  //get the framecode from the body code (301=MOON)
  cidfrm_c(301, sizeof(frameName), &frameCode, frameName, &found);
  if(!found) {
    QString naifTarget = QString("IAU_MOOM");
    namfrm_c(naifTarget.toLatin1().data(), &frameCode);
    if(frameCode == 0) {
      QString msg = "Can not find NAIF code for [" + naifTarget + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
  }
  spRot = new SpiceRotation(frameCode);
  //create a table from starttime to endtime (streched by 3%) with NODES entries
  spRot->LoadCache(time0-0.015*(time1-time0), time1+0.015*(time1-time0), NODES);
  Table tableTargetRot = spRot->Cache("BodyRotation");
  tableTargetRot.Label() += PvlKeyword("Description", "Created by apollopaninit");
  panCube.write(tableTargetRot);


  //////////////////////////////////////////////////attach a sun position table
  spPos = new SpicePosition(10,301);  //Position of the sun (10) WRT to the MOON (301)
  //create a table from starttime to endtime (stretched by 3%) with NODES entries
  spPos->LoadCache(time0-0.015*(time1-time0), time1+0.015*(time1-time0), NODES);
  Table tableSunPos = spPos->Cache("SunPosition");
  tableSunPos.Label() += PvlKeyword("Description", "Created by apollopaninit");
  panCube.write(tableSunPos);  //attach the table to the cube


  /////////////Finding the principal scan line position and orientation
  //get the radii of the MOON
  SpiceInt tempRadii = 0;
  bodvcd_c(301,"RADII",3,&tempRadii,R_MOON);  //units are km
  double  omega,phi,kappa;

  std::vector<double>  posSel;  //Seleno centric position
  std::vector<double> sunPos;  //sunPosition used to transform to J2000
  std::vector<double> posJ20;  //camera position in J2000
  posSel.resize(3);
  sunPos.resize(3);
  posJ20.resize(3);

  double  temp,
          vel[3] = { 0.0, 0.0, 0.0 },  //the total velocity vector (combined Horizonatal and normal components)
                   //  in km/sec
          M[3][3] = { { 0.0, 0.0, 0.0 },
                      { 0.0, 0.0, 0.0 },
                      { 0.0, 0.0, 0.0 } },    //rotation matrix
          zDir[] = { 0.0, 0.0, 1.0 },  //selenographic Z axis
          northPN[3]  = { 0.0, 0.0, 0.0 }, //normal to the plane containing all the north/south directions,
                      //  that is plane containing
                      //  the origin, the z axis, and the primary point of intersection
          northL[3] = { 0.0, 0.0, 0.0 },    //north direction vector in local horizontal plane
          azm[3] = { 0.0, 0.0, 0.0 },   //azm direction of the veclocity vector in selenographic coordinates
          azmP[3] = { 0.0, 0.0, 0.0 },  //azm rotated (partially) and projected into the image plane
          norm[3] = { 0.0, 0.0, 0.0 },  //normal to the local horizontal plane
          look[3] = { 0.0, 0.0, 0.0 };  //unit direction vector in the pincipal cameral look direction,
                    //  parallel to the vector from the center of the moon through the spacecraft

  double  pos0[3] = { 0.0, 0.0, 0.0 },  //coordinate of the camera position
          pInt[3] = { 0.0, 0.0, 0.0 };  //coordinate of the principle intersection point

  /////////////////calculating the camera position for the center (principal scan line)
  pos0[1] = ui.GetDouble("LON_NADIR")*deg2rad;
  pos0[0] = ui.GetDouble("LAT_NADIR")*deg2rad;
  pos0[2] = ui.GetDouble("CRAFT_ALTITUDE");  //units are km
  Geographic2GeocentricLunar(pos0,pos0);    //function is written so the input can also be the
                                            //  output

  /////////////////////calculating the camera orientation for the center (principal) scan line
  pInt[1] = ui.GetDouble("LON_INT")*deg2rad;
  pInt[0] = ui.GetDouble("LAT_INT")*deg2rad;
  pInt[2] = 0.0;
  Geographic2GeocentricLunar(pInt,pInt); //function is written so the input can also be the output
  //calculate the unit look direction vector in object space
  look[0] = -pos0[0] + pInt[0];
  look[1] = -pos0[1] + pInt[1];
  look[2] = -pos0[2] + pInt[2];
  temp = sqrt(look[0]*look[0] + look[1]*look[1] + look[2]*look[2]);
  look[0] /= temp;
  look[1] /= temp;
  look[2] /= temp;
  //the local normal vector is equal to pInt0/|pInt0|
  temp = sqrt(pInt[0]*pInt[0] + pInt[1]*pInt[1] + pInt[2]*pInt[2]);
  norm[0] = pInt[0]/temp;
  norm[1] = pInt[1]/temp;
  norm[2] = pInt[2]/temp;
  //omega and phi are defined so that M(phi)M(omega)look = [0 0 -1]  leaving only the roation
  //  around z axis to be found
  omega = -atan2(look[1], look[2]);  //omega rotation to zero look[1]
  phi   = atan2(-look[0], sin(omega)*look[1] - cos(omega)*look[2]);  //phi rotation to zero look[0]
  //use the horizontal velocity vector direction to solve for the last rotation; we will make the
  //  image x axis parallel to the in-image-plane projection of the horizontal direction of flight.
  //  The local normal cross the selenogrpahic z gives northPN (normal to the plane containing all
  //  the north/south directions), that is, the plane containing the origin, the z axis, and the
  //  primary point of intersection.
  crossp(norm, zDir, northPN);

  //The normal to the plane containing all the north/south directions cross the local normal
  //  direction gives the local north/south direction in the local normal plane
  crossp(northPN,norm,northL);
  if (northL[2] < 0) {  //if by chance we got the south direction change the signs
    northL[0] = -northL[0];
    northL[1] = -northL[1];
    northL[2] = -northL[2];
  }
  //define the rotation matrix to convert northL to the azimuth of flight.
  //  A left handed rotation of "VEL_AZM" around the positive normal direction will convert northL
  //  to azm
  MfromVecLeftAngle(M,norm,ui.GetDouble("VEL_AZM")*deg2rad);
  azm[0] = M[0][0]*northL[0] + M[0][1]*northL[1] + M[0][2]*northL[2];
  azm[1] = M[1][0]*northL[0] + M[1][1]*northL[1] + M[1][2]*northL[2];
  azm[2] = M[2][0]*northL[0] + M[2][1]*northL[1] + M[2][2]*northL[2];
  //apply the two rotations we already know
  MfromLeftEulers(M,omega,phi,0.0);
  azmP[0] = M[0][0]*azm[0] + M[0][1]*azm[1] + M[0][2]*azm[2];
  azmP[1] = M[1][0]*azm[1] + M[1][1]*azm[1] + M[1][2]*azm[2];
  azmP[2] = M[2][0]*azm[2] + M[2][1]*azm[1] + M[2][2]*azm[2];
  //subtract that portion of the azm that is perpindicular to the image plane (also the portion
  //  which is parallel to look) making azm a vector parrallel to the image plane
  //  Further, since we're now rotated into some coordinate system that differs from
  //  the image coordinate system by only a kappa rotation making the vector parrallel to the
  //  image plan is as simple as zeroing the z component (and as pointless to further calculations
  //  as a nat's fart in hurricane) nevertheless it completes the logical transition
  azmP[2] = 0.0;

  //finally the kappa rotation that will make azmP parallel (including sign) to the camera x axis
  kappa = -atan2(-azmP[1], azmP[0]);


  ////////////////////Add an instrument position table
  //Define the table records
  TableRecord recordPos;  // reacord to be added to table
  // add x,y,z position labels and ephemeris time et to record
  TableField x("J2000X", TableField::Double);
  TableField y("J2000Y", TableField::Double);
  TableField z("J2000Z", TableField::Double);
  TableField t("ET", TableField::Double);
  recordPos += x;
  recordPos += y;
  recordPos += z;
  recordPos += t;
  Table tablePos("InstrumentPosition", recordPos);
  //now that the azm and norm vectors are defined
  //  the total velocity vector can be calcualted (km/sec)
  vel[0] = horV*azm[0] + radV * norm[0];
  vel[1] = horV*azm[1] + radV * norm[1];
  vel[2] = horV*azm[2] + radV * norm[2];
  //we'll provide a two ellement table (more is redundant because the motion is modeled as linear
  //  at this point)  we'll extend the nodes 3% beyond the edges of the images to be sure
  //  rounding errors don't cause problems
  temp = 0.515*(time1-time0);  //3% extension
  posSel[0] = pos0[0] - temp*vel[0];    //selenocentric coordinate calculation
  posSel[1] = pos0[1] - temp*vel[1];
  posSel[2] = pos0[2] - temp*vel[2];
  //converting to J2000
  temp = time0 - 0.005*(time1-time0);  //et just before the first scan line
  spPos->SetEphemerisTime(temp);
  spRot->SetEphemerisTime(temp);
  //Despite being labeled as J2000, the coordinates for the instrument position are in fact in
  //  target centric coordinated rotated to a system centered at the target with aces parallel
  //  to J2000, whatever that means
  posJ20 = spRot->J2000Vector(posSel); //J2000Vector calls rotates the position vector into J2000,
                                       //  completing the transformation
  recordPos[0] = posJ20[0];
  recordPos[1] = posJ20[1];
  recordPos[2] = posJ20[2];
  recordPos[3] = temp;  //temp = et (right now anyway)
  tablePos += recordPos;
  tablePos.Label() += PvlKeyword("SpkTableStartTime",std::to_string(temp));
  //now the other node
  temp = 0.515*(time1-time0);      //3% extension
  posSel[0] = pos0[0] + temp*vel[0];    //selenocentric coordinate calculation
  posSel[1] = pos0[1] + temp*vel[1];
  posSel[2] = pos0[2] + temp*vel[2];
  //converting to J2000
  temp = time1 + 0.015*(time1-time0);  //et just after the last scan line
  spPos->SetEphemerisTime(temp);
  spRot->SetEphemerisTime(temp);
  //Despite being labeled as J2000, the coordinates for the instrument position are in fact
  //  in target centric coordinated rotated to a system centered at the target with aces
  //  parallel to J2000, whatever that means
  posJ20 = spRot->J2000Vector(posSel); //J2000Vector calls rotates the position vector into J2000,
                                       //  completing the transformation
  recordPos[0] = posJ20[0];
  recordPos[1] = posJ20[1];
  recordPos[2] = posJ20[2];
  recordPos[3] = temp;  //temp = et (right now anyway)
  tablePos += recordPos;
  tablePos.Label() += PvlKeyword("SpkTableEndTime",std::to_string(temp));
  tablePos.Label() += PvlKeyword("CacheType","Linear");
  tablePos.Label() += PvlKeyword("Description","Created by apollopaninit");
  panCube.write(tablePos);  //now attach it to the table

  /////////////////////////////attach a camera pointing table
  double  cacheSlope,  //time between epoches in the table
          rollComb,  //magnitude of roll relative to the center in the middle of the epoch
          relT,  //relative time at the center of each epoch
          Q[NODES][5],  //NODES four ellement unit quarternions and et (to be calculated).
          gimVec[3],  //the direction of the gimbal rotation vector (to the cameras persepective
                      //  this is always changing because the camera is mounted to the roll frame
                      //  assembly which is mounted to the gimbal)
          M0[3][3],  //rotation matrix of the previous epoch
          Mtemp1[3][3],  //intermediate step in the multiplication of rotation matricies
          Mtemp2[3][3],  //intermediate step in the multiplication of rotation matricies
          Mdg[3][3],  //incremental rotation due the the gimbal motion in the camera frame
          Mdr[3][3];  //the contribution of the roll motion in the camera frame during time
                      //  cacheSlope
  std::vector <double> M_J2toT;  //rotation matrix from J2000 to the target frame
  M_J2toT.resize(9);
  //Table Definition
  TableField q0("J2000Q0", TableField::Double);
  TableField q1("J2000Q1", TableField::Double);
  TableField q2("J2000Q2", TableField::Double);
  TableField q3("J2000Q3", TableField::Double);
  TableField et("ET", TableField::Double);
  TableRecord recordRot;
  recordRot += q0;
  recordRot += q1;
  recordRot += q2;
  recordRot += q3;
  recordRot += et;
  Table tableRot("InstrumentPointing",recordRot);
  //From the cameras perspective the gimbal motion is around a constantly changing axis,
  //  this is handled by combining a series of incremental rotations
  MfromLeftEulers(M0, omega, phi, kappa);  //rotation matrix in the center Q[(NOPDES-1)/2]
  spRot->SetEphemerisTime(isisTime.Et());
  M_J2toT = spRot->Matrix();   //this actually gives the rotation from J2000 to target centric
  for(j=0; j<3; j++)    //reformating M_J2toT to a 3x3
    for(k=0; k<3; k++)
      Mtemp1[j][k] = M_J2toT[3*j+k];
  mxm_c(M0, Mtemp1, Mtemp2);
  M2Q(Mtemp2, Q[(NODES-1)/2]);  //save the middle scan line quarternion

  Q[(NODES-1)/2][4] = (time1 + time0)/2.0;  //time in the center of the image
  //the total time is scaled up slightly so that nodes will extend just beyond the edge of the image
  cacheSlope = 1.03*(time1 - time0)/(NODES-1);
  //Mdr is constant for all the forward time computations
  MfromLeftEulers(Mdr,cacheSlope*rollV,0.0,0.0);
  for (i=(NODES-1)/2+1; i<NODES; i++) {    //moving foward in time first
    Q[i][4] = Q[i-1][4] + cacheSlope;    //new time epoch
    //epoch center time relative to the center line
    relT = double(i - (NODES-1)/2 - 0.5)*cacheSlope;
    rollComb = relT*rollV;
    gimVec[0] = 0.0;      //gimbal rotation vector direction in the middle of the epoch
    gimVec[1] =  cos(rollComb);
    gimVec[2] = -sin(rollComb);
    //incremental rotation due to the gimbal (forward motion compensation)
    MfromVecLeftAngle(Mdg, gimVec, fmc*cacheSlope);
    //the new rotation matrix is Transpose(Mdr)*Transpose(Mdg)*M0--NOTE the order swap and
    //  transposes are needed because both Mdr and Mdg were caculated in image space and need to be
    //  transposed to apply to object space
    mtxm_c(Mdg, M0, Mtemp1);
    //M0 is now what would typically be considered the rotation matrix of an image.  It rotates a
    //  vector from the target centric space into camera space.  However, what is standard to
    //  include in the cube labels is a rotation from camera space to J2000.  M0 is therefore the
    //  transpose of the first part of this rotation.  Transpose(M0) is the rotation from camera
    //  space to target centric space
    mtxm_c(Mdr, Mtemp1, M0);
    //now adding the rotation from the target frame to J2000
    spRot->SetEphemerisTime(Q[i][4]);
    //this actually gives the rotation from J2000 to target centric--hence the mxmt_c function being
    //  used later
    M_J2toT = spRot->Matrix();
    for(j=0; j<3; j++)  //reformating M_J2toT to a 3x3
      for(k=0; k<3; k++)
        Mtemp1[j][k] = M_J2toT[3*j+k];
    mxm_c(M0, Mtemp1, Mtemp2);
    M2Q(Mtemp2, Q[i]);    //convert to a quarterion
  }

  MfromLeftEulers(M0, omega, phi, kappa);  //rotation matrix in the center Q[(NOPDES-1)/2]
  //Mdr is constant for all the backward time computations
  MfromLeftEulers(Mdr, -cacheSlope*rollV, 0.0, 0.0);
  for (i=(NODES-1)/2-1; i>=0; i--) {  //moving backward in time
    Q[i][4] = Q[i+1][4] - cacheSlope;  //new time epoch
    //epoch center time relative to the center line
    relT = double(i  - (NODES-1)/2 + 0.5)*cacheSlope;
    rollComb = relT*rollV;
    gimVec[0] = 0.0;      //gimbal rotation vector direction in the middle of the epoch
    gimVec[1] =  cos(rollComb);
    gimVec[2] = -sin(rollComb);
    //incremental rotation due to the gimbal (forward motion compensation)
    MfromVecLeftAngle(Mdg, gimVec, -fmc*cacheSlope);
    //the new rotation matrix is Transpose(Mdr)*Transpose(Mdg)*M0    NOTE the order swap and
    //  transposes are needed because both Mdr and Mdg were caculated in image space and need to be
    //  transposed to apply to object space
    mtxm_c(Mdg, M0, Mtemp1);
    //M0 is now what would typically be considered the rotation matrix of an image.  It rotates a
    //  vector from the target centric space into camera space.  However, what is standard to
    //  include in the cube labels is a rotation from camera space to J2000.  M0 is therefore the
    //  transpose of the first part of this rotation.  Transpose(M0) is the rotation from camera
    //  space to target centric space
    mtxm_c(Mdr, Mtemp1, M0);
    //now adding the rotation from the target frame to J2000
    spRot->SetEphemerisTime(Q[i][4]);
    M_J2toT = spRot->Matrix();
    for(j=0; j<3; j++)  //reformating M_J2toT to a 3x3
      for(k=0; k<3; k++)
        Mtemp1[j][k] = M_J2toT[3*j+k];
    mxm_c(M0, Mtemp1, Mtemp2);
    M2Q(Mtemp2, Q[i]);    //convert to a quarterion
  }
  //fill in the table
  for (i=0; i<NODES; i++) {
    recordRot[0] = Q[i][0];
    recordRot[1] = Q[i][1];
    recordRot[2] = Q[i][2];
    recordRot[3] = Q[i][3];
    recordRot[4] = Q[i][4];
    tableRot += recordRot;
  }
  tableRot.Label() += PvlKeyword("CkTableStartTime", std::to_string(Q[0][4]));
  tableRot.Label() += PvlKeyword("CkTableEndTime", std::to_string(Q[NODES-1][4]));
  tableRot.Label() += PvlKeyword("Description", "Created by appollopan2isis");

  keyword.setName("TimeDependentFrames");
  keyword.setValue(std::to_string(scFrameCode));
  keyword.addValue("1");
  tableRot.Label() += keyword;

  keyword.setName("ConstantFrames");
  keyword.setValue(std::to_string(insCode));
  keyword.addValue(std::to_string(scFrameCode));
  tableRot.Label() += keyword;

  keyword.setName("ConstantRotation");
  keyword.setValue("1");
  for (i=1;i<9;i++)
    if (i%4 == 0) keyword.addValue("1");
    else keyword.addValue("0");
  tableRot.Label() += keyword;
  panCube.write(tableRot);


  /////////////////////////Attach a table with all the measurements of the fiducial mark locations.
  Chip patternS,searchS;   //scaled pattern and search chips
  Cube  fidC;  //Fiducial image

  //line and sample coordinates for looping through the panCube
  double l=1,s=1,sample,line,sampleInitial=1,lineInitial=1,play;

  int  regStatus,
       fidn,
       panS,
       refL,  //number of lines in the patternS
       refS;  //number of samples in the patternS
  Pvl pvl;

  bool foundFirst=false;

  QString fileName;

  panS = panCube.sampleCount();

  //Table definition
  TableRecord recordFid;
  TableField indexFid("FID_INEX",TableField::Integer);
  TableField xFid("X_COORD",TableField::Double);
  TableField yFid("Y_COORD",TableField::Double);
  recordFid += indexFid;
  recordFid += xFid;
  recordFid += yFid;
  Table tableFid("Fiducial Measurement",recordFid);

  //read the image resolutions and scale the constants acordingly
  double  resolution = ui.GetDouble("MICRONS"),    //pixel size in microns
          scale            = SCALE  *5.0/resolution,  //reduction scale for fast autoregistrations
          searchHeight     = SEARCHh*5.0/resolution,  //number of lines (in 5-micron-pixels) in
                                                      //  search space for the first fiducial
          searchCellSize   = SEARCHc*5.0/resolution,  //height/width of search chips block
          averageSamples   = AVERs  *5.0/resolution,  //scaled smaples between fiducials
          averageLines     = AVERl  *5.0/resolution;  //scaled average distance between the top and
                                                      //bottom fiducials

  if( 15.0/resolution < 1.5) play=1.5;
  else play = 15.0/resolution;

  //copy the patternS chip (the entire ApolloPanFiducialMark.cub)
  FileName fiducialFileName("$apollo15/calibration/ApolloPanFiducialMark.cub");
  fidC.open(fiducialFileName.expanded(),"r");
  if( !fidC.isOpen() ) {
    std::string msg = "Unable to open the fiducial patternS cube: ApolloPanFiducialMark.cub\n";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  refL = fidC.lineCount();
  refS = fidC.sampleCount();
  //scaled pattern chip for fast matching
  patternS.SetSize(int((refS-2)/SCALE), int((refL-2)/SCALE));
  patternS.TackCube((refS-1)/2, (refL-1)/2);
  patternS.Load(fidC, 0, SCALE);

  //parameters for maximum correlation autoregestration
  // see:  file:///usgs/pkgs/isis3nightly2011-09-21/isis/doc/documents/patternSMatch/patternSMatch.html#DistanceTolerance
  FileName fiducialPvl("$ISISROOT/appdata/templates/apollo/PanFiducialFinder.def");
  pvl.read(fiducialPvl.expanded().toStdString());  //read in the autoreg parameters
  AutoReg *arS = AutoRegFactory::Create(pvl);

  *arS->PatternChip()   = patternS;  //patternS chip is constant

  //set up a centroid measurer
  CentroidApolloPan centroid(resolution);
  Chip inputChip,selectionChip;
  inputChip.SetSize(int(ceil(200*5.0/resolution)), int(ceil(200*5.0/resolution)));
  fileName = ui.GetCubeName("FROM");
  if( panCube.pixelType() == 1)  //UnsignedByte
    centroid.setDNRange(12, 1e99);  //8 bit bright target
  else
    centroid.setDNRange(3500, 1e99);  //16 bit bright target

  Progress progress;
  progress.SetText("Locating Fiducials");
  progress.SetMaximumSteps(91);

  //Search for the first fiducial, search sizes are constanst
  searchS.SetSize(int(searchCellSize/scale),int(searchCellSize/scale));
  //now start searching along a horizontal line for the first fiducial mark
  for(l = searchCellSize/2;
      l<searchHeight+searchCellSize/2.0 && !foundFirst;
      l+=searchCellSize-125*5.0/resolution) {
    for (s = searchCellSize/2;
         s < averageSamples + searchCellSize/2.0 && !foundFirst;
         s += searchCellSize-125*5.0/resolution) {
      searchS.TackCube(s, l);
      searchS.Load(panCube, 0, scale);
      *arS->SearchChip() = searchS;
      regStatus = arS->Register();
      if (regStatus == AutoReg::SuccessPixel) {
        inputChip.TackCube(arS->CubeSample(), arS->CubeLine());
        inputChip.Load(panCube, 0, 1);
        inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
        //continuous dynamic range selection
        centroid.selectAdaptive(&inputChip, &selectionChip);
        //elliptical trimming/smoothing
        if (centroid.elipticalReduction(&selectionChip, 95, play, 2000)) {
          //center of mass to reduce selection to a single measure
          centroid.centerOfMass(&selectionChip, &sample, &line);
          inputChip.SetChipPosition(sample, line);
          sampleInitial = inputChip.CubeSample();
          lineInitial   = inputChip.CubeLine();
          foundFirst = true;  //once the first fiducial is found stop
        }
      }
    }
  }
  if(s>=averageLines+searchCellSize/2.0) {
     QString msg = "Unable to locate a fiducial mark in the input cube [" + fileName +
                  "].  Check FROM and MICRONS parameters.";
     throw IException(IException::Io, msg, _FILEINFO_);
     return;
  }
  progress.CheckStatus();

  //record first fiducial measurement in the table
  recordFid[0] = 0;
  recordFid[1] = sampleInitial;
  recordFid[2] = lineInitial;
  tableFid += recordFid;
  for (s= sampleInitial, l=lineInitial, fidn=0;  s<panS;  s+=averageSamples, fidn++) {
     //corrections for half spacing of center fiducials
     if (fidn == 22) s -= averageSamples/2.0;
     if (fidn == 23) s -= averageSamples/2.0;

     //look for the bottom fiducial
     searchS.TackCube(s,l+averageLines);
     searchS.Load(panCube, 0, scale);
     *arS->SearchChip()   = searchS;
     regStatus = arS->Register();
     if (regStatus == AutoReg::SuccessPixel) {
       inputChip.TackCube(arS->CubeSample(), arS->CubeLine());
       inputChip.Load(panCube,0,1);
       inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
     }
     else {  //if autoreg is unsuccessful, a larger window will be used
       inputChip.TackCube(s, l+averageLines);
       inputChip.Load(panCube, 0, 1);
       inputChip.SetCubePosition(s, l+averageLines);
     }
     centroid.selectAdaptive(&inputChip, &selectionChip);  //continuous dynamic range selection
     //elliptical trimming/smoothing... if this fails move on
     if (centroid.elipticalReduction(&selectionChip, 95, play, 2000) != 0 ) {
       //center of mass to reduce selection to a single measure
       centroid.centerOfMass(&selectionChip, &sample, &line);
       inputChip.SetChipPosition(sample, line);
       sample = inputChip.CubeSample();
       line   = inputChip.CubeLine();
       recordFid[0] = fidn*2+1;
       recordFid[1] = sample;
       recordFid[2] = line;
       tableFid += recordFid;
     }
     progress.CheckStatus();

     //look for the top fiducial
     if (s == sampleInitial) //first time through the loop?
       continue;  //then the top fiducial was already found
     searchS.TackCube(s, l);
     searchS.Load(panCube, 0, scale);
     *arS->SearchChip()   = searchS;
     regStatus = arS->Register();
     if (regStatus == AutoReg::SuccessPixel) {
       inputChip.TackCube(arS->CubeSample(), arS->CubeLine());
       inputChip.Load(panCube, 0, 1);
       inputChip.SetCubePosition(arS->CubeSample(), arS->CubeLine());
     }
     else {  //if autoreg is unsuccessful, a larger window will be used
       inputChip.TackCube(s, l);
       inputChip.Load(panCube, 0, 1);
       inputChip.SetCubePosition(s, l);
     }
     centroid.selectAdaptive(&inputChip, &selectionChip);//continuous dynamic range selection
     //inputChip.Write("inputTemp.cub");//debug
     //selectionChip.Write("selectionTemp.cub");//debug
     //elliptical trimming/smoothing... if this fails move on
     if (centroid.elipticalReduction(&selectionChip, 95, play, 2000) !=0) {
       //center of mass to reduce selection to a single measure
       centroid.centerOfMass(&selectionChip, &sample, &line);
       inputChip.SetChipPosition(sample, line);
       //when finding the top fiducial both s and l are refined for a successful measurement,
       //  this will help follow trends in the scaned image
       s = inputChip.CubeSample();
       l = inputChip.CubeLine();
       recordFid[0] = fidn*2;
       recordFid[1] = s;
       recordFid[2] = l;
       tableFid += recordFid;
     }
     progress.CheckStatus();
  }

  panCube.write(tableFid);
  //close the new cube
  panCube.close(false);
  panCube.open(ui.GetCubeName("FROM"),"rw");

  delete spPos;
  delete spRot;

  //now instantiate a camera to make sure all of this is working
  ApolloPanoramicCamera* cam = (ApolloPanoramicCamera*)(panCube.camera());
  //log the residual report from interior orientation
  PvlGroup residualStats("InteriorOrientationStats");
  residualStats += PvlKeyword("FiducialsFound",  std::to_string(tableFid.Records()));
  residualStats += PvlKeyword("ResidualMax",  std::to_string(cam->intOriResidualMax()),"pixels");
  residualStats += PvlKeyword("ResidualMean", std::to_string(cam->intOriResidualMean()),"pixels");
  residualStats += PvlKeyword("ResidualStdev", std::to_string(cam->intOriResidualStdev()),"pixels");

  Application::Log( residualStats );


  return;
}

//function largely copied from the spice class because it was private and I couldn't access it
//  without shoe-horning the input to please the rest of the Spice::Init() funciton
void Load_Kernel(Isis::PvlKeyword &key) {

  //Load all the kernel files (file names are stored as values of the PvlKeyword)
  NaifStatus::CheckErrors();

  for(int i = 0; i < key.size(); i++) {
     if(key[i] == "") continue;
     if(QString::fromStdString(key[i]).toUpper() == "NULL") break;
     if(QString::fromStdString(key[i]).toUpper() == "NADIR") break;
     //Table was left as the first value of these keywords because one is about to be attached,
     //  still though it needs to be skipped in this loop
     if(QString::fromStdString(key[i]).toUpper() == "TABLE") continue;
     Isis::FileName file(QString::fromStdString(key[i]));
     if(!file.fileExists()) {
       QString msg = "Spice file does not exist [" + file.expanded() + "]";
       throw IException(IException::Io, msg, _FILEINFO_);
     }
     QString fileName(file.expanded());
     furnsh_c(fileName.toLatin1().data());
  }

  NaifStatus::CheckErrors();
}

void crossp(double v1[3],double v2[3],double v1cv2[3])
{
  //calcuate v1 cross v2
  v1cv2[0] = v1[1]*v2[2] - v1[2]*v2[1];
  v1cv2[1] = v1[2]*v2[0] - v1[0]*v2[2];
  v1cv2[2] = v1[0]*v2[1] - v1[1]*v2[0];
  return;
}

void Geographic2GeocentricLunar(double geographic[3], double geocentric[3])
/*
  given
  geographic -> (Lat, Lon, H)

  calc
  geocentric <- (X, Y, Z)

  Lat and Lon should be in radians
*/
{
  double r,cosl;

  r = R_MOON[0] + geographic[2];

  cosl = cos(geographic[0]);

  geocentric[2] = r*sin(geographic[0]);
  geocentric[0] = r*cosl*cos(geographic[1]);
  geocentric[1] = r*cosl*sin(geographic[1]);
}

void MfromLeftEulers(double M[3][3], double omega, double phi, double kappa)
{
  /*given three left handed Euler angles compute M 3x3 orthogonal rotation matrix

    M    <-  3x3 rotation matrix
    omega  ->  left handed rotation (rad) around the x axis
    phi    ->  left handed rotation (rad) around the once rotated y axis
    kappa  ->  left handed rotation (rad) around the twice rotated z axis
  */

  M[0][0] = cos(phi) * cos(kappa);
  M[0][1] = sin(omega) * sin(phi) * cos(kappa) + cos(omega) * sin(kappa);
  M[0][2] = -cos(omega) * sin(phi) * cos(kappa) + sin(omega) * sin(kappa);
  M[1][0] = -cos(phi) * sin(kappa);
  M[1][1] = -sin(omega) * sin(phi) * sin(kappa) + cos(omega) * cos(kappa);
  M[1][2] = cos(omega) * sin(phi) * sin(kappa) + sin(omega) * cos(kappa);
  M[2][0] = sin(phi);
  M[2][1] = -sin(omega) * cos(phi);
  M[2][2] = cos(omega) * cos(phi);

}

void MfromVecLeftAngle(double M[3][3], double vec[3], double angle)
{
  /*Define a roation matrix from a vector and a left handed angle
    M    <- orthogonal rotation matrix
    vec    -> 3 ellement vector defining the axis of rotation
    angle  -> left handed angle of rotation (rad) around vec

  */
  double vs[3],l;

  l = sqrt(vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2]);
  vs[0] = vec[0] / l;
  vs[1] = vec[1] / l;
  vs[2] = vec[2] / l;
  //change the sign of angle to match subsequent math (written for right handed angles)
  l = -angle;  //this makes this a right handed anlge
  //calculate rotation matrix
  M[0][0] = cos(l) + vs[0]*vs[0] * (1 - cos(l));
  M[0][1] = vs[0] * vs[1] * (1 - cos(l)) - vs[2] * sin(l);
  M[0][2] = vs[0] * vs[2] * (1 - cos(l)) + vs[1] * sin(l);

  M[1][0] = vs[0] * vs[1] * (1 - cos(l)) + vs[2] * sin(l);
  M[1][1] = cos(l) + vs[1]*vs[1] * (1 - cos(l));
  M[1][2] = vs[1] * vs[2] * (1 - cos(l)) - vs[0] * sin(l);

  M[2][0] = vs[0] * vs[2] * (1 - cos(l)) - vs[1] * sin(l);
  M[2][1] = vs[1] * vs[2] * (1 - cos(l)) + vs[0] * sin(l);
  M[2][2] = cos(l) + vs[2]*vs[2] * (1 - cos(l));
}

void M2Q(double M[3][3], double Q[4])
{
  /* given a rotation matrix decompose the quarternion

    M  ->  3x3 orthogonal rotation matrix
    Q  <-  four ellement unit quarternion

  */

  //following the decomposition algorithim given at: http://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation

  int index[3],i;
  double temp;

  //find the largest diagonal ellement in Md
  temp = fabs(M[0][0]);
  index[0] = 0;

  for(i=1; i<2 ; i++)
  {
    if( fabs(M[i][i] ) > temp )
    {
      temp = fabs(M[i][i]);
      index[0] = i;
    }
  }

  index[1] = index[0] + 1;
  if( index[1] > 2 )
    index[1] -= 3;

  index[2] = index[1] + 1;
  if( index[2] > 2 )
    index[2] -= 3;

  temp = sqrt(1 + M[index[0]][index[0]] - M[index[1]][index[1]] - M[index[2]][index[2]]);

  if( temp == 0 )
  {
    Q[0] = 1;
    Q[1] = 0;
    Q[2] = 0;
    Q[3] = 0;
    return;
  }

  Q[0] = (  M[index[2]][index[1]] - M[index[1]][index[2]]  ) / (2 * temp);
  Q[index[1] + 1] = (M[index[0]][index[1]] + M[index[1]][index[0]]) / (2 * temp);
  Q[index[2] + 1] = (M[index[2]][index[0]] + M[index[0]][index[2]]) / (2 * temp);

  Q[index[0] + 1] = temp / 2;
}
