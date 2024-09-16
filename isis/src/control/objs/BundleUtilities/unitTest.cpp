/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Angle.h"
#include "BundleControlPoint.h"
#include "BundleImage.h"
#include "BundleMeasure.h"
#include "IsisBundleObservation.h"
#include "BundleObservationSolveSettings.h"
#include "BundleObservationVector.h"
#include "BundleSettings.h"
#include "BundleTargetBody.h"
#include "Camera.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "ControlMeasure.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "Latitude.h"
#include "LinearAlgebra.h"
#include "Longitude.h"
#include "Preference.h"
#include "PvlObject.h"
#include "SpecialPixel.h"
#include "Spice.h"
#include "SurfacePoint.h"
#include "Target.h"

#include <QByteArray>
#include <QDebug>
#include <QDataStream>
#include <QFile>
#include <QIODevice>
#include <QString>
#include <QXmlInputSource>
#include <QXmlStreamWriter>

#include <boost/numeric/ublas/symmetric.hpp>
#include <boost/numeric/ublas/vector.hpp>

using namespace std;
using namespace Isis;

void printBundleMeasure(BundleMeasure &);
void printXml(const BundleObservationSolveSettings &);

/**
 * This class is used to test loading objects from xml files.
 *
 * @author 2014 Jeannie Backer
 *
 * @internal
 *   @history 2018-07-03 Debbie A Cook - Removed target radii. References #5457.
 *   @history 2014-12-11 - Original version.
 *   @history 2018-09-06 Debbie A. Cook - Merged dev into BundleXYZ branch
 *                               Original branch history entry on 2017-06-26
 *                               Updated to reflect changes made to BundleControlPoint.
 *                               Some tests were no longer valid and new tests were added to exercise
 *                               the new option of adjusting in Rectangular coordinates.  The
 *                               Latitudinal covariance was being populated only along the diagonal
 *                               (using the latitudinal sigmas).  This produced inaccurate results.
 *                               Now it is created by converting  the rectangular covariance
 *                               matrix to latitudinal.  References #4649 and #501.
 *   @history 2018 -09-06 Debbie A. Cook - Merged dev into BundleXYZ branch
 *                               Original branch history entry on 2017-11-29 - Updated to reflect
 *                               changes made to units of covariance matrix in SurfacePoint methods
 *                               and removal of SurfacePoint::SetRadii method.
 *  @history 2018-09-28 Debbie A. Cook - Removed metersToRadians argument from
 *                               constructor because we are now using the local radius instead of
 *                               the target body equatorial radius to convert meters to radians.  To
 *                               work, the apriori coordinates must be set.  Some of the tests do not
 *                               require the apriori coordinates to be set, so those tests were
 *                               modified.
 */
namespace Isis {
  class XmlHandlerTester : public BundleObservationSolveSettings {
    public:
      XmlHandlerTester(QXmlStreamReader *reader, FileName xmlFile)
          : BundleObservationSolveSettings() {

        QString xmlPath(QString::fromStdString(xmlFile.expanded()));
        QFile file(xmlPath);

        if (!file.open(QFile::ReadOnly) ) {
          throw IException(IException::Io,
                           "Unable to open xml file, ["+xmlPath.toStdString()+"],  with read access",
                           _FILEINFO_);
        }

        if (reader->readNextStartElement()) {
          if (reader->name() == "bundleObservationSolveSettings") {
            readSolveSettings(reader);
          }
          else {
            reader->raiseError(QObject::tr("Incorrect file"));
          }
        }

      }

      ~XmlHandlerTester() {
      }

  };
}


/**
 * Unit test for BundleObservationSolveSettings.
 *
 * @internal
 *   @history 2014-12-11 Jeannie Backer - Added test for BundleControlPoint.
 *                           Updated truth file, improved overall test coverage.
 *   @history 2016-08-10 Jeannie Backer - Replaced boost vector with Isis::LinearAlgebra::Vector.
 *                           References #4163.
 *   @history 2016-08-18 Jeannie Backer - Removed references to BundleSettings solve method.
 *                           References #4162.
 *   @history 2016-10-13 Ian Humphrey - Removed references to setFromPvl(), as
 *                           BundleObservationSolveSettings::setFromPvl() was moved to jigsaw as
 *                           setSettingsFromPvl(). References #4293.
 *   @history 2016-12-01 Ian Humphrey - Added extra qDebug() stream so the "apply param
 *                           corrections successful?" string will be in the unitTest output.
 *   @history 2017-04-24 Ian Humphrey - Replaced pvlObject() with XML save(). Fixes #4797.
 *   @history 2017-03-05 Debbie A. Cook - updated to conform to changes made to
 *                            BundleControlPointConstructor.  Fixed test
 *                             "Modify FreePoint - setWeights() - solveRadius=true, apriori lat/lon/rad <= 0"
 *                             to output radius type as free instead of N/A under Inital Accuracy column and
 *                             fixed weight value to be 0.  Corrections were made by creating a new contol
 *                             point when the settings were changed instead of just calling setWeights.
 *                             Deleted tests "Modified FreePoint - setWeights, "Modify FixedPoint -
 *                             setWeights() and ModifyConstrainedPoint - setWeights()" since
 *                             setWeights is always called in the constructor now.  References
 *                             #4649 and #501
 */
int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout.precision(6);
  qDebug() << "Unit test for BundleUtilities...";
  qDebug() << "";

  try {
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing BundleObservationSolveSettings...";
    qDebug() << "";
    // default constructor
    qDebug() << "Printing PVL group with settings from the default constructor...";
    BundleObservationSolveSettings boss;
    printXml(boss);

    qDebug() << "Testing copy constructor...";
    BundleObservationSolveSettings copySettings(boss);
    printXml(copySettings);

    qDebug() << "Testing assignment operator to set this equal to itself...";
    {
      BundleObservationSolveSettings &tboss = boss;
      boss = tboss;
    }
    printXml(boss);

    qDebug() << "Testing assignment operator to create a new settings object...";
    BundleObservationSolveSettings assignmentOpSettings;
    assignmentOpSettings = boss;
    printXml(assignmentOpSettings);

    qDebug() << "Testing mutator methods...";
    qDebug() << "setInstrument(), setInstrumentPointingSettings(), setInstrumentPositionSettings()";
    boss.setInstrumentId("MyInstrumentId");
    boss.setInstrumentPointingSettings(BundleObservationSolveSettings::AnglesVelocity, true, 1, 2,
                                       false, 3.0, 4.0, 5.0);
    boss.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionOnly, 6, 7,
                                       true, 800.0, 900.0, 1000.0);
    printXml(boss);

    BundleObservationSolveSettings solveNone;
    solveNone.setInstrumentPointingSettings(BundleObservationSolveSettings::NoPointingFactors,
                                               true);
    solveNone.setInstrumentPositionSettings(BundleObservationSolveSettings::NoPositionFactors);
    printXml(solveNone);

    BundleObservationSolveSettings solveAnglesPositions;
    solveAnglesPositions.setInstrumentPointingSettings(BundleObservationSolveSettings::AnglesOnly,
                                                       true);
    solveAnglesPositions.setInstrumentPositionSettings(
        BundleObservationSolveSettings::PositionOnly);
    printXml(solveAnglesPositions);

    BundleObservationSolveSettings solveVelocities;
    solveVelocities.setInstrumentPointingSettings(BundleObservationSolveSettings::AnglesVelocity,
                                                  true);
    solveVelocities.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionVelocity);
    printXml(solveVelocities);

    BundleObservationSolveSettings solveAccelerations;
    solveAccelerations.setInstrumentPointingSettings(
        BundleObservationSolveSettings::AnglesVelocityAcceleration, true);
    solveAccelerations.setInstrumentPositionSettings(
        BundleObservationSolveSettings::PositionVelocityAcceleration);
    printXml(solveAccelerations);

    boss.setInstrumentPointingSettings(BundleObservationSolveSettings::AllPointingCoefficients,
                                       false, 4, 5, true, 1.0, -1.0, 3.0);
    boss.setInstrumentPositionSettings(BundleObservationSolveSettings::AllPositionCoefficients,
                                       6, 7, true, 8.0, 9.0, -1.0);
    printXml(boss);

    qDebug() << "Testing static unused enum-to-string and string-to-enum methods...";
    qDebug() << BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
                                                                              "NOPOINTINGFACTORS"));
    qDebug() << BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
                                                                                     "anglesonly"));
    qDebug() << BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
                                                                              "AnglesAndVelocity"));
    qDebug() << BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
                                                                  "AnglesVelocityAndAcceleration"));
    qDebug() << BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPointingSolveOption(
                                                                      "AllPolynomialCoefficients"));
    qDebug() << BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
                                                                              "NOPOSITIONFACTORS"));
    qDebug() << BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
                                                                                   "positiononly"));
    qDebug() << BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
                                                                            "PositionAndVelocity"));
    qDebug() << BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
                                                                "PositionVelocityAndAcceleration"));
    qDebug() << BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
                BundleObservationSolveSettings::stringToInstrumentPositionSolveOption(
                                                                      "AllPolynomialCoefficients"));
    qDebug() << "";


    qDebug() << "Testing XML: write XML from BundleObservationSolveSettings object...";
    // write xml
    FileName xmlFile("./BundleObservationSolveSettings.xml");
    std::string xmlPath = xmlFile.expanded();
    QFile qXmlFile(QString::fromStdString(xmlPath));

    // For test coverage, we need to write/read BundleObservationSolveSettings objects
    // with 0,1,2,3 apriori sigmas and an empty xml
    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    QXmlStreamWriter writer(&qXmlFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    Project *project = NULL;
    solveNone.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml
    qDebug() << "Testing XML: read XML to BundleObservationSolveSettings object...";
    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }
    QXmlStreamReader reader2(&qXmlFile);
    XmlHandlerTester bsFromXml1(&reader2, xmlFile);
    printXml(bsFromXml1);
    qXmlFile.close();

    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    solveAnglesPositions.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml
    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }
    QXmlStreamReader reader3(&qXmlFile);
    qDebug() << "Testing XML: read XML to BundleObservationSolveSettings object...";
    XmlHandlerTester bsFromXml2(&reader3, xmlFile);
    printXml(bsFromXml2);
    qXmlFile.close();

    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    solveVelocities.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml
    qDebug() << "Testing XML: read XML to BundleObservationSolveSettings object...";
    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }
    QXmlStreamReader reader4(&qXmlFile);
    XmlHandlerTester bsFromXml3(&reader4, xmlFile);
    printXml(bsFromXml3);
    qXmlFile.close();

    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    solveAccelerations.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml
    qDebug() << "Testing XML: read XML to BundleObservationSolveSettings object...";
    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }
    QXmlStreamReader reader5(&qXmlFile);
    XmlHandlerTester bsFromXml4(&reader5, xmlFile);
    printXml(bsFromXml4);
    qXmlFile.close();

    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    boss.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();
    // read xml
    qDebug() << "Testing XML: read XML to BundleObservationSolveSettings object...";
    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }
    QXmlStreamReader reader6(&qXmlFile);
    XmlHandlerTester bossToFill(&reader6, xmlFile);
    printXml(bossToFill);
    qXmlFile.close();

    // read xml with no attributes or values
    qDebug() << "Testing XML: read XML with no attributes or values to object...";
    FileName emptyXmlFile("./unitTest_NoElementValues.xml");
    QFile xml(QString::fromStdString(emptyXmlFile.expanded()));
    if(!xml.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+xml.fileName().toStdString()+"]",
                        _FILEINFO_);
    }

    QXmlStreamReader reader7(&xml);
    XmlHandlerTester bsFromEmptyXml(&reader7, emptyXmlFile);
    printXml(bsFromEmptyXml);
    xml.close();

    qXmlFile.remove();

    //bool deleted = qXmlFile.remove();
    //if (!deleted) {
    //  std::string msg = "Unit Test failed. XML file [" + xmlPath + "not deleted.";
    //  throw IException(IException::Io, msg, _FILEINFO_);
    //}

    qDebug() << "Testing error throws...";
    try {
      BundleObservationSolveSettings::stringToInstrumentPointingSolveOption("Nonsense");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleObservationSolveSettings::instrumentPointingSolveOptionToString(
          BundleObservationSolveSettings::InstrumentPointingSolveOption(-1));
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleObservationSolveSettings::stringToInstrumentPositionSolveOption("Nonsense");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleObservationSolveSettings::instrumentPositionSolveOptionToString(
          BundleObservationSolveSettings::InstrumentPositionSolveOption(-2));
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing BundleImage...";
    #if 0
    TEST COVERAGE (SCOPE) FOR THIS SOURCE FILE: 100%
    #endif
    qDebug() << "Constructing BundleImage object from null camera...";
    Camera *camera = NULL;
    BundleImage bi(camera, "TestImageSerialNumber", "TestImageFileName");
    qDebug() << "setting null parentBundleObservation to BundleImage...";
    IsisBundleObservationQsp parentObservation;
    bi.setParentObservation(parentObservation);
    qDebug() << "Access camera and parentObservation ...";
//    Camera *cam = bi.camera();
//    BundleObservation *parent = bi.parentObservation();
    qDebug() << "serial number = " << bi.serialNumber();
    qDebug() << "file name     = " << bi.fileName();
    qDebug() << "Testing copy constructor...";
    BundleImageQsp bi2 = BundleImageQsp( new BundleImage(bi) );
    qDebug() << "serial number = " << bi2->serialNumber();
    qDebug() << "file name     = " << bi2->fileName();
    qDebug() << "Testing assignment operator to set this equal to itself...";
    {
      BundleImage &tbi = bi;
      bi = tbi;
    }
    qDebug() << "serial number = " << bi.serialNumber();
    qDebug() << "file name     = " << bi.fileName();
    qDebug() << "Testing assignment operator to create a new object...";
    BundleImage bi3(camera, "TestImage2SerialNumber", "TestImage2FileName");
    bi3 = bi;
    qDebug() << "serial number = " << bi3.serialNumber();
    qDebug() << "file name     = " << bi3.fileName();
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing IsisBundleObservation...";
    qDebug() << "Constructing empty IsisBundleObservation object...";
    /**
    TEST COVERAGE (SCOPE) FOR THIS SOURCE FILE: ??% (need to re-run SquishCoco)
    NEED:
        1) construct with image containing camera that is not null
          1a) construct also with non-null target body qsp
          1b) test initilaizeBodyRotation()
        2) NOT POSSIBLE - setSolveSettings - initParameterWeights() returns false
        3) initializeExteriorOrientation - positionOption!=No, pointingOption=No
        4) applyParameterCorrections -     positionOption!=No, pointingOption=No
          4a) Test the second exception thrown by applyParameterCorrections
          4b) Test successful call to applyParameterCorrections (#4249).
        5) initParameterWeights - possigma[0] == 0, possigma[1] == 0, possigma[2]  > 0
                                                    pntsigma[1]  > 0, pntsigma[2] == 0
        6) formatBundleOutputString - with instrumentPosition/instrumentRotation not NULL
    */
    IsisBundleObservation bo;
    BundleTargetBodyQsp bundleTargetBody = BundleTargetBodyQsp(new BundleTargetBody);
    qDebug() << "Constructing IsisBundleObservation object from BundleImage...";
    IsisBundleObservation bo2(bi2,
                          "ObservationNumber2",
                          "InstrumentId2",
                          bundleTargetBody);

    BundleImageQsp nullImage;
    IsisBundleObservation nullBO(nullImage,
                             "NullObservationNumber",
                             "NullInstrumentId",
                             bundleTargetBody);

    qDebug() << "Testing assignment operator to set this equal to itself...";
    {
      IsisBundleObservation &tbo2 = bo2;
      bo2 = tbo2;
    }
    qDebug() << "Testing assignment operator to create a new object...";
    bo = bo2;
    qDebug() << "Testing copy constructor...";
    IsisBundleObservation bo3(bo);

    qDebug() << "Testing mutators and accessors...";
    qDebug() << "    Set/get solve settings using with CAMESOLVE=NONE...";
    bo2.setSolveSettings(solveNone);
    BundleObservationSolveSettings bossFromBo = *bo2.solveSettings();
    printXml(bossFromBo);
    qDebug() << "    output bundle observation...";
    qDebug().noquote() << bo2.bundleOutputCSV(true);
    qDebug().noquote() << bo2.bundleOutputCSV(false);
    std::stringstream fpOut1;
    bo2.bundleOutputString(fpOut1, false);
    qDebug().noquote() << QString::fromStdString(fpOut1.str());
    std::stringstream fpOut2;
    bo2.bundleOutputString(fpOut2, true);
    qDebug().noquote() << QString::fromStdString(fpOut2.str());

    qDebug() << "    Set solve settings using with TWIST=FALSE...";
    bo2.setSolveSettings(bossToFill);
    bossFromBo = *bo2.solveSettings();
    printXml(bossFromBo);
    qDebug() << "    output bundle observation...";
    qDebug().noquote() << bo2.bundleOutputCSV(true);
    qDebug().noquote() << bo2.bundleOutputCSV(false);
    std::stringstream fpOut3;
    bo2.bundleOutputString(fpOut3, false);
    qDebug().noquote() << QString::fromStdString(fpOut3.str());
    std::stringstream fpOut4;
    bo2.bundleOutputString(fpOut4, true);
    qDebug().noquote() << QString::fromStdString(fpOut4.str());

    qDebug() << "    Set solve settings using with CAMSOLVE=ALL and TWIST=TRUE...";
    bo3.setSolveSettings(bsFromEmptyXml);

    bossFromBo = *bo3.solveSettings();
    printXml(bossFromBo);
    bo3.setIndex(1);
    qDebug() << "index = " << QString::number(bo3.index());
    qDebug() << "instrument id = " << bo3.instrumentId();
    qDebug() << "number parameters =     " << QString::number(bo3.numberParameters());
    qDebug() << "parameter list: " << bo3.parameterList();
    qDebug() << "image names:    " << bo3.imageNames();

    LinearAlgebra::Vector paramWts = bo3.parameterWeights();
    LinearAlgebra::Vector paramCor = bo3.parameterCorrections();
    LinearAlgebra::Vector aprSigma = bo3.aprioriSigmas();
    LinearAlgebra::Vector adjSigma = bo3.adjustedSigmas();
    QString vectors = "parameter weights :     ";
    for (unsigned int i = 0; i < paramWts.size(); i++) {
      vectors.append(QString::number(paramWts[i]));
      vectors.append("     ");
    }
    vectors.append("\nparameter corrections : ");
    for (unsigned int i = 0; i < paramCor.size(); i++) {
      vectors.append(QString::number(paramCor[i]));
      vectors.append("     ");
    }
    vectors.append("\napriori sigmas :        ");
    for (unsigned int i = 0; i < aprSigma.size(); i++) {
      vectors.append(QString::number(aprSigma[i]));
      vectors.append("     ");
    }
    vectors.append("\nadjusted sigmas :       ");
    for (unsigned int i = 0; i < adjSigma.size(); i++) {
      vectors.append(QString::number(adjSigma[i]));
      vectors.append("     ");
    }
    qDebug().noquote() << vectors;

    // initializeBodyRotation (verify???)
    // bo3.initializeBodyRotation(); //Seg fault

    qDebug() << "    output bundle observation...";
    qDebug().noquote() << bo3.bundleOutputCSV(false);
    std::stringstream fpOut5;
    bo3.bundleOutputString(fpOut5, false);
    qDebug().noquote() << QString::fromStdString(fpOut5.str());
    qDebug().noquote() << bo3.bundleOutputCSV(true);
    qDebug() << "init exterior orientiation successful?  "
             << QString::number(bo3.initializeExteriorOrientation());
    //TODO: We should not have to catch an exception here, we need to use an observation
    //      with a better (i.e. non-null) Camera. See ticket #4249.
    try {
      qDebug() << "apply param corrections successful?";
      qDebug() << QString::number(bo3.applyParameterCorrections(paramCor));
    }
    catch (IException &e) {
      e.print();
    }

    qDebug() << "";

    // spiceRotation and spicePosition (verify???)
    //SpiceRotation *rotation =
    bo3.spiceRotation();
    //SpicePosition *position =
    bo3.spicePosition();

    qDebug() << "    add another image...";
    bo3.append(
      BundleImageQsp(
        new BundleImage(camera, "TestImage2SerialNumber", "TestImage2FileName")));
    qDebug() << "    access images by serial number...";
    qDebug().noquote() << bo3.imageByCubeSerialNumber("TestImageSerialNumber")->fileName();
    qDebug().noquote() << bo3.imageByCubeSerialNumber("TestImage2SerialNumber")->fileName();
    qDebug() << "";

    //  See BundleObservation::applyParameterCorrections last catch (exception NOT thrown)
    qDebug() << "Testing exceptions...";
    BundleObservationSolveSettings bo3SettingsCopy(*(bo3.solveSettings()));
    try {
      bo3SettingsCopy.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionOnly);
      nullBO.setSolveSettings(bo3SettingsCopy);
      nullBO.applyParameterCorrections(paramCor);
    }
    catch (IException &e) {
      e.print();
    }
    try {
      bo3.applyParameterCorrections(paramCor);
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing BundleObservationVector...";
    qDebug() << "";
    /*
    #if 0
    TEST COVERAGE (SCOPE) FOR THIS SOURCE FILE: 21%
    Need:
      1) addNew - settings->solveObservationMode() == false
                  map.contains(obsNumber)
      2) addNew - settings->solveObservationMode()
                  map.contains(obsNumber) == false
      3) addNew - settings->solveObservationMode()
                  map.contains(obsNumber)
                  bo.instId() == this.instId
      4) addNew - settings->solveObservationMode()
                  map.contains(obsNumber)
                  bo.instId() != this.instId
                  bundleObservation != null
                  bundleSettings.numberSolveSettings() == 1
      5) addNew - settings->solveObservationMode()
                  map.contains(obsNumber)
                  bo.instId() != this.instId
                  bundleObservation != null
                  bundleSettings.numberSolveSettings() != 1
      6) addNew -
                  map.contains(obsNumber)
                  bo.instId() != this.instId
                  bundleObservation == null
      7) initializeExteriorOrientation, numberPositionParameters, numberPointingParameters - size > 0
      8) getObsByCubeSerialNumber - map.contains(sn)
      9) getObsByCubeSerialNumber - map.contains(sn) == false

    #endif
    */
    BundleObservationVector bov;
    BundleSettingsQsp bundleSettings = BundleSettingsQsp(new BundleSettings);
    qDebug() << "number of parameters: " << QString::number(bov.numberParameters());
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing BundleControlPoint...";
    // #1 Test free point with default settings (solveRadius=false), apriori coordinates set, but no
    //       sigmas (other settings: observation mode = false, update =false, errorProp = false)
    qDebug() << "BCP test 1 - Create FreePoint with free point containing 2 measures "
          "(note that first measure is ignored, second measure is not ignored)";
    qDebug()  << "     and no apriori or adjusted coordinate values or sigmas set...";
    ControlPoint *freePoint = new ControlPoint("FreePoint");
    ControlMeasure *cm1 = new ControlMeasure;
    cm1->SetCubeSerialNumber("Ignored");
    cm1->SetIgnored(true);
    freePoint->Add(cm1);
    ControlMeasure *cm2 = new ControlMeasure;
    cm2->SetCubeSerialNumber("NotIgnored");
    cm2->SetIgnored(false);
    cm2->SetCoordinate(1.0, 2.0);
    cm2->SetResidual(-3.0, 4.0);
    freePoint->Add(cm2);
// Moved these lines up from below (DAC 2-25-2017)
    BundleSettingsQsp settings = BundleSettingsQsp(new BundleSettings);
    BundleControlPoint bcp1(settings, freePoint);
    bool errorProp = false;

    qDebug() << "Type of BundleControlPoint 1:" << bcp1.type();

    bcp1.setRejected(true);
    qDebug() << "Set BundleControlPoint 1 to rejected - is rejected?"
        << QString::number(bcp1.isRejected());
    bcp1.setRejected(false);
    qDebug() << "Set BundleControlPoint 1 to non-rejected - is rejected?"
        << QString::number(bcp1.isRejected());

    qDebug() << "Number of rejected measures:" << bcp1.numberOfRejectedMeasures();
    bcp1.setNumberOfRejectedMeasures(2);
    qDebug() << "Set number of rejected measures:" << bcp1.numberOfRejectedMeasures();
    bcp1.zeroNumberOfRejectedMeasures();
    qDebug() << "Zero out number of rejected measures:" << bcp1.numberOfRejectedMeasures();

    // ??? these print outs are not pretty... fix???
    qDebug().noquote() << bcp1.formatBundleOutputSummaryString(errorProp);
    // ??? these print outs are not pretty... fix??? improved somewhat 6-9-2017
    qDebug().noquote() << bcp1.formatBundleOutputDetailString(errorProp);
    // Test free point.  Settings same, but errorProp = true)
    errorProp = true;
    // ??? these print outs are not pretty... fix???
    qDebug().noquote() << bcp1.formatBundleOutputSummaryString(errorProp);
    // ??? these print outs are not pretty... fix???
    qDebug().noquote() << bcp1.formatBundleOutputDetailString(errorProp);
    qDebug() << "";

// #2 Same as test 1, but assign coordinate values (0., 0., 10.) to adjusted surface points of FREE
//       point with solve radius still false -- radius weight is fixed (1.0e+50).  Other coordinates are free
    qDebug() << "BCP test 2 - Modify FreePoint - setAdjustedSurfacePoint(0,0,10) and addMeasure()";
    SurfacePoint sp1(Latitude(0.0, Angle::Degrees),
                     Longitude(0.0, Angle::Degrees),
                     Distance(10.0, Distance::Meters));
    bcp1.setAdjustedSurfacePoint(sp1);
    // ??? this appears to do nothing! measure is added to the internal QVector of measures,
    // not the member control point...
    // probably need to fix the format string methods to use "this" instead of member control point
    // and accessor methods???
    BundleMeasure bcm = *(bcp1.addMeasure(cm1));
    // ??? these print outs are not pretty... fix???
    qDebug().noquote() << bcp1.formatBundleOutputSummaryString(errorProp);
    // ??? these print outs are not pretty... fix???
    qDebug().noquote() << bcp1.formatBundleOutputDetailString(errorProp);

    // Same but look at values of aprioriSigmas, weights, corrections, etc.
    // qDebug() << "Modify FreePoint - setWeights() - solveRadius=false";
    // Deleted summary and detail output here since it is duplicated now that the BCP and the
    // constructor always calls setWeights now.;
// Before the change, test2 checked default settings and default weights.  Omitted test checked
// BCP creation using a blank settings (equivalent to no settings) and a setWeights call that only
// specifies solveRadius to false, which is the same as the default.
// default solveRadius=false

    // These now commented lines were moved to before creation of bcp1 and
    //   setWeights now happens in BundleControlPoint constructor (DAC 2-25-2017)
    // BundleSettingsQsp settings = BundleSettingsQsp(new BundleSettings);
    // double metersToRadians = 1.0 / radiansToMeters;
    // bcp1.setWeights(settings, metersToRadians);
    // qDebug().noquote() << bcp1.formatBundleOutputDetailString(errorProp, radiansToMeters);
    boost::numeric::ublas::bounded_vector< double, 3 > aprioriSigmas = bcp1.aprioriSigmas();
    boost::numeric::ublas::bounded_vector< double, 3 > weights = bcp1.weights();
    //??? never set 000??? init to 1.0e+50???
    boost::numeric::ublas::bounded_vector< double, 3 > corrections = bcp1.corrections();
    //??? never set 000??? 1.0e+50???
    boost::numeric::ublas::bounded_vector< double, 3 > adjustedSigmas = bcp1.adjustedSigmas();
    //??? never set 000c??? 1.0e+50???
    boost::numeric::ublas::bounded_vector< double, 3 > nicVector = bcp1.nicVector();
    SparseBlockRowMatrix qMatrix = bcp1.cholmodQMatrix(); //??? empty matrix...
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "N/A" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "N/A" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "N/A" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "corrections:    " << corrections[0] << corrections[1] << corrections[2];
    qDebug() << "adjustedSigmas: "
               << (Isis::IsSpecial(adjustedSigmas[0]) ? "N/A" : QString::number(adjustedSigmas[0]))
               << (Isis::IsSpecial(adjustedSigmas[1]) ? "N/A" : QString::number(adjustedSigmas[1]))
               << (Isis::IsSpecial(adjustedSigmas[2]) ? "N/A" : QString::number(adjustedSigmas[2]));
    qDebug() << "nicVector:      " << nicVector[0] << nicVector[1] << nicVector[2];
    qDebug() << "qMatrix:";
    qDebug() << qMatrix;

    qDebug() << "Residual rms:" << bcp1.residualRms();
    qDebug() << "";
// end test 2

    // #3 Test free point with solveRadius=true (default), no corrections or valid sigmas
    qDebug() << "BCP test 3 - Create FreePoint - identical to previous, but with solveRadius=true";
    qDebug() << " and apriori lat/lon/rad <= 0.  Test adding a measure to a BundleControlPoint.";
//  Create a new BundleControlPoint like the previous one with settings to indicate radius is free
    settings->setSolveOptions(false, false, false, true, SurfacePoint::Latitudinal,
                              SurfacePoint::Latitudinal, Isis::Null);
    BundleControlPoint bcp1a(settings, freePoint);
    bcp1a.setAdjustedSurfacePoint(sp1);
    BundleMeasure bcm1a = *(bcp1a.addMeasure(cm1));
//  Note:  This test was abusing the setWeights method of BundleControlPoint.
//  It was using setWeights to update bcp1 with new BundleSettings; in particular,
//  it was changing the solveRadius bool from false to true.  This did not work
//  properly because setWeights was designed to update weights after they were
//  initialized by the constructor.  The radius weight was set to 1.0e+50 when bcp1
//  was created with solveRadius set to false.  When setWeights was used to update
//  the settings, the radius weight was not changed back.  setWeights should NOT
//  be used to update BundleSettings.  This is likely only a test issue.
    qDebug().noquote() << bcp1a.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp1a.formatBundleOutputDetailString(errorProp, true);
    aprioriSigmas = bcp1a.aprioriSigmas();
    weights = bcp1a.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "N/A" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "N/A" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "N/A" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// #4 Test with global sigmas now.  Everything else is the same as test #3.
    qDebug() << "BCP test 4 - Create FreePoint - solveRadius=true, apriori lat/lon/rad > 0 ";
    qDebug() << "                    from globals - coordinate type = Latitudinal";
    settings->setSolveOptions(false, false, false, true, SurfacePoint::Latitudinal,
                              SurfacePoint::Latitudinal, 2.0, 3.0, 4.0);
    freePoint->SetAprioriSurfacePoint(sp1);
    BundleControlPoint bcp1b(settings, freePoint);
    bcp1b.setAdjustedSurfacePoint(sp1);
    BundleMeasure bcm1b = *(bcp1b.addMeasure(cm1));
    qDebug().noquote() << bcp1b.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp1b.formatBundleOutputDetailString(errorProp);
    aprioriSigmas = bcp1b.aprioriSigmas();
    weights = bcp1b.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "N/A" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "N/A" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "N/A" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

    ControlPoint *cp = bcp1b.rawControlPoint();
    qDebug() << "Raw control point equal to original?    " << QString::number(*cp == *freePoint);
    qDebug() << "Raw control point is rejected?          " << QString::number(bcp1b.isRejected());
    SurfacePoint sp = bcp1b.adjustedSurfacePoint();
    qDebug() << "Adjusted SurfacePoint (Lat, Lon, Rad) = "
               << QString::number(sp.GetLatitude().degrees())
               << QString::number(sp.GetLongitude().degrees())
               << QString::number(sp.GetLocalRadius().meters());
    qDebug() << "";

// Testing of Free point settings is complete

//  Fixed point tests
    qDebug() << "BCP test 5 - Create FixedPoint from empty fixed point, solveRadius = F"
                            " adjusted surface point (90, 180, 10)...";
    ControlPoint *fixedPoint = new ControlPoint("FixedPoint");
    fixedPoint->SetType(ControlPoint::Fixed);
    settings->setSolveOptions(false, false, false, false, SurfacePoint::Latitudinal,
                              SurfacePoint::Latitudinal, Isis::Null);
    BundleControlPoint *bcp3a = new BundleControlPoint(settings, fixedPoint);
    SurfacePoint sp2(Latitude(90.0, Angle::Degrees),
                     Longitude(180.0, Angle::Degrees),
                     Distance(10.0, Distance::Meters));
    bcp3a->setAdjustedSurfacePoint(sp2);
    qDebug().noquote() << bcp3a->formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp3a->formatBundleOutputDetailString(errorProp);

    qDebug() << "BCP test 6 - Create FixedPoint from empty fixed point, solveRadius = True";
    qDebug() << " adjusted surface point (90, 180, 10)...";
settings->setSolveOptions(false, false, false, true, SurfacePoint::Latitudinal,
                          SurfacePoint::Latitudinal, Isis::Null);
    BundleControlPoint *bcp3b = new BundleControlPoint(settings, fixedPoint);
    bcp3b->setAdjustedSurfacePoint(sp2);
    qDebug().noquote() << bcp3b->formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp3b->formatBundleOutputDetailString(errorProp, true);
    aprioriSigmas = bcp3b->aprioriSigmas();
    weights = bcp3b->weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// Testing of FixedPoint output completed

// #7 ConstrainedPoint test with surface point with coordinates only.  SolveRadius is false.
     qDebug() << "BCP test 7 - Create ConstrainedPoint with solveRadius=false and adjusted "
                            "surface point (0, 0, 10), no constraints set, coordType=Latitudinal ...";
    ControlPoint *constrainedPoint = new ControlPoint("ConstrainedPoint");
    constrainedPoint->SetType(ControlPoint::Constrained);
    settings->setSolveOptions(false, false, false, false);
    BundleControlPoint bcp4a(settings, constrainedPoint);
    bcp4a.setAdjustedSurfacePoint(sp1);
    qDebug().noquote() << bcp4a.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp4a.formatBundleOutputDetailString(errorProp);
    aprioriSigmas = bcp4a.aprioriSigmas();
    weights = bcp4a.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// #8 ConstrainedPoint with no constraints, but solveRadius=true
    qDebug() << "BCP test 8 - Create ConstrainedPoint - no constraints, solveRadius=true, "
               "apriori lat/lon/rad <= 0, ";
    qDebug() << "     and adjustedsurface point (0, 0, 10)";
    settings->setSolveOptions(false, false, false, true);
    BundleControlPoint bcp4b(settings, constrainedPoint);
    bcp4b.setAdjustedSurfacePoint(sp1);
    qDebug().noquote() << bcp4b.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp4b.formatBundleOutputDetailString(errorProp, true);
    aprioriSigmas = bcp4b.aprioriSigmas();
    weights = bcp4b.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// #9 ConstrainedPoint with no constraints set, solveRadius=true, and valid global sigmas.
    qDebug() << "BCP test 9 - Create ConstrainedPoint - no constraints,  solveRadius=true,"
                " apriori lat/lon/rad > 0 (valid global sigmas)";
    settings->setSolveOptions(false, false, false, true, SurfacePoint::Latitudinal,
                               SurfacePoint::Latitudinal, 2.0, 3.0, 4.0);
    constrainedPoint->SetAprioriSurfacePoint(sp1);
    BundleControlPoint bcp4c(settings, constrainedPoint);
    bcp4c.setAdjustedSurfacePoint(sp1);
    qDebug().noquote() << bcp4c.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp4c.formatBundleOutputDetailString(errorProp);
    aprioriSigmas = bcp4c.aprioriSigmas();
    weights = bcp4c.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    if (!IsSpecial(weights[0]) && !IsSpecial(weights[1]) && !IsSpecial(weights[2])) {
      qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    }
    else {
      qDebug() << "weights:        NA";
    }

    qDebug() << "";
    qDebug() << "";

// #10 ConstrainedPoint with apriori and adjusted surface points fully set and solveRadius=F.
    qDebug() << "BCP test 10 - Create ConstrainedPoint from constrained point with adjusted  ";
    qDebug() << "    pt (32, 120, 1000) & apriori pt with constraints from covar, solveRadius=F...";
    SurfacePoint aprioriSurfPt;
    boost::numeric::ublas::symmetric_matrix<double, boost::numeric::ublas::upper> covar;
    covar.resize(3);
    covar.clear();
    covar(0,0) = 100.;
    covar(1,1) = 2500.;
    covar(2,2) = 400.;
    aprioriSurfPt.SetRectangular(Displacement(-424.024048, Displacement::Meters),
                                 Displacement(734.4311949, Displacement::Meters),
                                 Displacement(529.919264, Displacement::Meters), covar);
// Extract the covar matrix converted to latitudinal coordinates now to use for test 10.
// Usage note:  In order to get accurate results, the full correlation matrix should be
// used as opposed to only setting the diagonal elements with the sigmas.
    boost::numeric::ublas::symmetric_matrix<double, boost::numeric::ublas::upper> covarLat(3);
    covarLat.clear();
    covarLat = aprioriSurfPt.GetSphericalMatrix();

// These results match what is being set in adjusted surface point.
    Angle latSigma = aprioriSurfPt.GetLatSigma();
    Angle lonSigma = aprioriSurfPt.GetLonSigma();
    Distance localRad = aprioriSurfPt.GetLocalRadiusSigma();
    constrainedPoint->SetAprioriSurfacePoint(aprioriSurfPt);
    settings->setSolveOptions(false, false, false, false);
    BundleControlPoint bcp5a(settings, constrainedPoint);
    SurfacePoint adjustedSurfPt(constrainedPoint->GetAdjustedSurfacePoint());
    adjustedSurfPt.SetSphericalCoordinates(Latitude(32., Angle::Degrees),
                                Longitude(120., Angle::Degrees),
                                           Distance(1000., Distance::Meters));
    adjustedSurfPt.SetSphericalMatrix(covarLat);
                                // Angle(1.64192315,Angle::Degrees),
                                // Angle(1.78752107, Angle::Degrees),
                                // Distance(38.454887335682053718134171237789, Distance::Meters));
    bcp5a.setAdjustedSurfacePoint(adjustedSurfPt);
    qDebug().noquote() << bcp5a.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp5a.formatBundleOutputDetailString(errorProp);

// #11 ConstrainedPoint with apriori and adjusted surface points fully set and solveRadius=T.
    qDebug() << "BCP test 11 - Create ConstrainedPoint from constrained point with adjusted  surface"
                " pt (32, 120, 1000) ";
    qDebug() << "     & apriori pt with constraints from covar, solveRadius=T...";
    settings->setSolveOptions(false, false, false, true);
    BundleControlPoint bcp5b(settings, constrainedPoint);
    qDebug().noquote() << bcp5b.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp5b.formatBundleOutputDetailString(errorProp);
    aprioriSigmas = bcp5b.aprioriSigmas(); // these values were verified by comparing against
                                          // SurfacePoint truth data
    weights = bcp5b.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// #12 FreePoint - test copy constructor by copying bcp1b
    qDebug() << "BCP test 12 - Create copy of FreePoint using copy constructor...";
// Should we use bundleSettings or settings here?
    BundleControlPoint bcp2(bcp1b);
    qDebug().noquote() << bcp2.formatBundleOutputSummaryString(errorProp);
    //solveForRadius = false by default in formatBundleDetailString
    qDebug() << "Output for formatBundleOutputDetailString(...) with solveForRadius = false:";
    qDebug().noquote() << bcp2.formatBundleOutputDetailString(errorProp);

    //solveForRadius = true
    qDebug() << "BCP test 13 - Output for formatBundleOutputDetailString(...) with "
                           "solveForRadius = true:";
    qDebug().noquote() << bcp2.formatBundleOutputDetailString(errorProp, true);

    qDebug() << "";

    qDebug() << "BCP test 14 - Overwrite existing object with FixedPoint information...";
    bcp2.copy(*bcp3b);
    qDebug().noquote() << bcp2.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp2.formatBundleOutputDetailString(errorProp);
    qDebug() << "";

    qDebug() << "BCP test 15 - Coordtype=Rect, Free, solveRad=F";
    settings->setSolveOptions(false, false, false, false, SurfacePoint::Rectangular,
                          SurfacePoint::Rectangular);
    BundleControlPoint bcp1c(settings, freePoint);
    qDebug().noquote() << bcp1c.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp1c.formatBundleOutputDetailString(errorProp);
    weights = bcp1c.weights();
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

    qDebug() << "BCP test 16 - Coordtype=Rect, Free, solveRad=T";
    settings->setSolveOptions(false, false, false, true, SurfacePoint::Rectangular,
                              SurfacePoint::Rectangular, 2.0, 3.0, 4.0);
    BundleControlPoint bcp1d(settings, freePoint);
    qDebug().noquote() << bcp1d.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp1d.formatBundleOutputDetailString(errorProp);
    weights = bcp1d.weights();
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";
    qDebug() << "";

    qDebug() << "BCP test 17 - Coordtype=Rect, Fixed, solveRad=F";
    settings->setSolveOptions(false, false, false, false, SurfacePoint::Rectangular,
                              SurfacePoint::Rectangular, 2.0, 3.0, 4.0);
    sp2.SetRectangular(Displacement(0.0, Displacement::Meters),
                     Displacement(0.0, Displacement::Meters),
                     Displacement(1000.0, Displacement::Meters));
    BundleControlPoint *bcp3c = new BundleControlPoint(settings, fixedPoint);
    bcp3c->setAdjustedSurfacePoint(sp2);
    qDebug().noquote() << bcp3c->formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp3c->formatBundleOutputDetailString(errorProp);
    weights = bcp3c->weights();
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";
    qDebug() << "";

// #18 ConstrainedPoint test with surface point with coordinates only.  SolveRadius is false.
     qDebug() << "BCP test 18 - Create ConstrainedPoint with solveRadius=false and adjusted "
                            "surface point (0, 0, 1000), no constraints set, and coordType = Rect ...";
    SurfacePoint sp3(Displacement(0.0, Displacement::Meters),
                     Displacement(0.0, Displacement::Meters),
                     Displacement(1000.0, Displacement::Meters));
    ControlPoint *constrainedPointRect = new ControlPoint("ConstrainedPoint");
    constrainedPointRect->SetType(ControlPoint::Constrained);
    settings->setSolveOptions(false, false, false, false, SurfacePoint::Rectangular,
                          SurfacePoint::Rectangular);
    BundleControlPoint bcp4d(settings, constrainedPointRect);
    bcp4d.setAdjustedSurfacePoint(sp3);
    qDebug().noquote() << bcp4d.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp4d.formatBundleOutputDetailString(errorProp);
    aprioriSigmas = bcp4d.aprioriSigmas();
    weights = bcp4d.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// #19 ConstrainedPoint test with surface point with coordinates only.  SolveRadius is false.
     qDebug() << "BCP test 19 - Create ConstrainedPoint with solveRadius=false and adjusted "
                            "surface point (0, 0, 1000), valid globals, and coordType = Rect ...";
    // SurfacePoint sp3(Displacement(0.0, Displacement::Meters),
    //                  Displacement(0.0, Displacement::Meters),
    //                  Displacement(1000.0, Displacement::Meters));
    // ControlPoint *constrainedPointRect = new ControlPoint("ConstrainedPoint");
    // constrainedPointRect->SetType(ControlPoint::Constrained);
    settings->setSolveOptions(false, false, false, false, SurfacePoint::Rectangular,
                              SurfacePoint::Rectangular, 2.0, 3.0, 4.0);
    BundleControlPoint bcp4e(settings, constrainedPointRect);
    bcp4e.setAdjustedSurfacePoint(sp3);
    qDebug().noquote() << bcp4e.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp4e.formatBundleOutputDetailString(errorProp);
    aprioriSigmas = bcp4e.aprioriSigmas();
    weights = bcp4e.weights();
    qDebug() << "aprioriSigmas:  "
               << (Isis::IsSpecial(aprioriSigmas[0]) ? "NULL" : QString::number(aprioriSigmas[0]))
               << (Isis::IsSpecial(aprioriSigmas[1]) ? "NULL" : QString::number(aprioriSigmas[1]))
               << (Isis::IsSpecial(aprioriSigmas[2]) ? "NULL" : QString::number(aprioriSigmas[2]));
    qDebug() << "weights:        " << weights[0] << weights[1] << weights[2];
    qDebug() << "";

// #20 ConstrainedPoint with apriori and adjusted surface points fully set and solveRadius=F.
    qDebug() << "BCP test 20 - Create ConstrainedPoint from constrained point with adjusted  ";
    qDebug() << " pt (32, 120, 1000) & apriori pt from Test 10 with constraints from covar, solveRadius=F, ";
    qDebug() << "coordType=Rectangular...";
    qDebug() << "";
// This test uses an apriori surface point set with rectangular coordinates and sigmas.  The adjusted
// surface point is set with latitudinal coordinates equivalent to the apriori surface point coordinates.
// The covar for the adjusted surface point is generated from the apriori covar converted to latitudinal
// coordinates.  Using just the sigmas to set the diagonal elements of the covar is not accurate.
    constrainedPointRect->SetAprioriSurfacePoint(aprioriSurfPt);
settings->setSolveOptions(false, false, false, false, SurfacePoint::Rectangular, SurfacePoint::Rectangular);
    BundleControlPoint bcp5c(settings, constrainedPointRect);
    bcp5c.setAdjustedSurfacePoint(adjustedSurfPt);
    qDebug().noquote() << bcp5c.formatBundleOutputSummaryString(errorProp);
    qDebug().noquote() << bcp5c.formatBundleOutputDetailString(errorProp);

// #21 Test error condition - invalid BundleControlPoint coordinate type
    qDebug() << "BCP test 21 - Test invalid coordinate type  ";
    qDebug() << "";
    settings->setSolveOptions(false, false, false, false, SurfacePoint::CoordinateType(3));

    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing BundleMeasure...";

    // TEST COVERAGE (SCOPE) FOR THIS SOURCE FILE: 86% //TODO update when SquishCoco works again
    BundleMeasure bundleMeasure(cm2, bcp3b);

    try {
      bundleMeasure.observationSolveSettings();
    }
    catch (IException &e) {
      e.print();
    }
    bundleMeasure.setParentObservation(IsisBundleObservationQsp(new IsisBundleObservation(bo2)));
    // const BundleObservationSolveSettings *solveSettings =
    bundleMeasure.observationSolveSettings();
    // Camera *cam =
    bundleMeasure.camera();
    // BundleObservation  *parentObs =
    bundleMeasure.parentBundleObservation();
    BundleControlPoint *parentBCP = bundleMeasure.parentControlPoint();
    qDebug() << "parent control point id" << parentBCP->id();
    bundleMeasure.setParentImage(BundleImageQsp(new BundleImage(bi)));
    BundleImageQsp parentImage = bundleMeasure.parentBundleImage();
    qDebug() << "parent image id" << parentImage->serialNumber();

    // Copy and =
    BundleMeasure bundleMeasureRejected(bundleMeasure); // We will use this to test setRejected.
    BundleMeasure bundleMeasureEq = bundleMeasure;

    // Test setRejected(true)
    bundleMeasureRejected.setRejected(true);

    // Test self-assignment
    {
      BundleMeasure &tbundleMeasure = bundleMeasure;
      bundleMeasure = tbundleMeasure;
    }

    qDebug() << "";
    // Verify state and copies
    printBundleMeasure(bundleMeasure);
    printBundleMeasure(bundleMeasureRejected);
    printBundleMeasure(bundleMeasureEq);

    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "";

    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
    qDebug() << "Testing BundleTargetBody...";
    qDebug() << "";

    qDebug() << "Create an empty BundleTargetBody";
    qDebug() << "";
    BundleTargetBody btb1;

    qDebug().noquote() << btb1.formatBundleOutputString(false);

    //TODO test creating a BundleTargetBody from a target

    qDebug() << "";
    qDebug() << "Convert strings to TargetRadiiSolveMethods and back";
    qDebug() << "";

    BundleTargetBody::TargetRadiiSolveMethod targetRadiiSolveMethod;
    targetRadiiSolveMethod = BundleTargetBody::stringToTargetRadiiOption("none");
    qDebug() << targetRadiiSolveMethod;
    qDebug() << BundleTargetBody::targetRadiiOptionToString(targetRadiiSolveMethod);
    targetRadiiSolveMethod = BundleTargetBody::stringToTargetRadiiOption("mean");
    qDebug() << targetRadiiSolveMethod;
    qDebug() << BundleTargetBody::targetRadiiOptionToString(targetRadiiSolveMethod);
    targetRadiiSolveMethod = BundleTargetBody::stringToTargetRadiiOption("all");
    qDebug() << targetRadiiSolveMethod;
    qDebug() << BundleTargetBody::targetRadiiOptionToString(targetRadiiSolveMethod);

    qDebug() << "";
    qDebug() << "Setup the BundleTargetBody to solve for everything but mean radius";
    qDebug() << "";
    // We do not test solving for acceleration as it is not implemented.
    std::set<int> targetParameterSolveCodes;
    targetParameterSolveCodes.insert(BundleTargetBody::PoleRA);
    targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleRA);
//     targetParameterSolveCodes.insert(BundleTargetBody::AccelerationPoleRA);
    targetParameterSolveCodes.insert(BundleTargetBody::PoleDec);
    targetParameterSolveCodes.insert(BundleTargetBody::VelocityPoleDec);
//     targetParameterSolveCodes.insert(BundleTargetBody::AccelerationPoleDec);
    targetParameterSolveCodes.insert(BundleTargetBody::PM);
    targetParameterSolveCodes.insert(BundleTargetBody::VelocityPM);
//     targetParameterSolveCodes.insert(BundleTargetBody::AccelerationPM);
    targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusA);
    targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusB);
    targetParameterSolveCodes.insert(BundleTargetBody::TriaxialRadiusC);
    double poleRA               = -2.0;
    double poleRASigma          = -0.2;
    double poleRAVelocity       = -3.0;
    double poleRAVelocitySigma  = -3.5;
    double poleDec              = -4.0;
    double poleDecSigma         = -5.0;
    double poleDecVelocity      = -6.0;
    double poleDecVelocitySigma = -7.0;
    double pm                   = -8.0;
    double pmSigma              = -9.0;
    double pmVelocity           = -10.0;
    double pmVelocitySigma      = -11.0;
    double aRadius              = 12.0;
    double aRadiusSigma         = 13.0;
    double bRadius              = 14.0;
    double bRadiusSigma         = 0.001;
    double cRadius              = 15.0;
    double cRadiusSigma         = 17.0;
    double meanRadius           = 20.0;
    double meanRadiusSigma      = 21.0;
    btb1.setSolveSettings(
        targetParameterSolveCodes,
        Angle(poleRA, Angle::Degrees),             Angle(poleRASigma, Angle::Degrees),
        Angle(poleRAVelocity, Angle::Degrees),     Angle(poleRAVelocitySigma, Angle::Degrees),
        Angle(poleDec, Angle::Degrees),            Angle(poleDecSigma, Angle::Degrees),
        Angle(poleDecVelocity, Angle::Degrees),    Angle(poleDecVelocitySigma, Angle::Degrees),
        Angle(pm, Angle::Degrees),                 Angle(pmSigma, Angle::Degrees),
        Angle(pmVelocity, Angle::Degrees),         Angle(pmVelocitySigma, Angle::Degrees),
        (BundleTargetBody::TargetRadiiSolveMethod)2,
        Distance(aRadius, Distance::Kilometers),   Distance(aRadiusSigma, Distance::Kilometers),
        Distance(bRadius, Distance::Kilometers),   Distance(bRadiusSigma, Distance::Kilometers),
        Distance(cRadius, Distance::Kilometers),   Distance(cRadiusSigma, Distance::Kilometers),
        Distance(meanRadius ,Distance::Kilometers),Distance(meanRadiusSigma, Distance::Kilometers));
    qDebug().noquote() << btb1.formatBundleOutputString(false);
    qDebug() << "";
    qDebug().noquote() << btb1.formatBundleOutputString(true);
    LinearAlgebra::Vector btb1Weights = btb1.parameterWeights();
    QString btb1WString;
    for (size_t i = 0; i < btb1Weights.size(); i++) {
      btb1WString.append(QString::number(btb1Weights[i]));
      if (i < btb1Weights.size() - 1) {
        btb1WString.append(", ");
      }
    }
    qDebug().noquote() << btb1WString;

    qDebug() << "";
    qDebug() << "Apply some corrections";
    qDebug() << "";

    LinearAlgebra::Vector btb1CumCorrections (btb1.numberParameters());
    btb1CumCorrections = btb1.parameterCorrections();
    QString btb1CString;
    for (size_t i = 0; i < btb1CumCorrections.size(); i++) {
      btb1CString.append(QString::number(btb1CumCorrections[i]));
      if (i < btb1CumCorrections.size() - 1) {
        btb1CString.append(", ");
      }
    }
    qDebug().noquote() << btb1CString;
    qDebug() << "";
    LinearAlgebra::Vector btb1Corrections (btb1.numberParameters());
    for (size_t i = 0; i < btb1Corrections.size(); i++) {
      btb1Corrections[i] = pow(-0.7, i);
    }
    btb1.applyParameterCorrections(btb1Corrections);
    qDebug().noquote() << btb1.formatBundleOutputString(true);
    btb1Weights = btb1.parameterWeights();
    btb1WString.clear();
    for (size_t i = 0; i < btb1Weights.size(); i++) {
      btb1WString.append(QString::number(btb1Weights[i]));
      if (i < btb1Weights.size() - 1) {
        btb1WString.append(", ");
      }
    }
    qDebug().noquote() << btb1WString;
    qDebug() << "";
    btb1CumCorrections = btb1.parameterCorrections();
    btb1CString.clear();
    for (size_t i = 0; i < btb1CumCorrections.size(); i++) {
      btb1CString.append(QString::number(btb1CumCorrections[i]));
      if (i < btb1CumCorrections.size() - 1) {
        btb1CString.append(", ");
      }
    }
    qDebug().noquote() << btb1CString;
    qDebug() << "";
    for (size_t i = 0; i < btb1Corrections.size(); i++) {
      btb1Corrections[i] = pow(1.1, i);
    }
    btb1.applyParameterCorrections(btb1Corrections);
    qDebug().noquote() << btb1.formatBundleOutputString(true);
    btb1Weights = btb1.parameterWeights();
    btb1WString.clear();
    for (size_t i = 0; i < btb1Weights.size(); i++) {
      btb1WString.append(QString::number(btb1Weights[i]));
      if (i < btb1Weights.size() - 1) {
        btb1WString.append(", ");
      }
    }
    qDebug().noquote() << btb1WString;
    qDebug() << "";
    btb1CumCorrections = btb1.parameterCorrections();
    btb1CString.clear();
    for (size_t i = 0; i < btb1CumCorrections.size(); i++) {
      btb1CString.append(QString::number(btb1CumCorrections[i]));
      if (i < btb1CumCorrections.size() - 1) {
        btb1CString.append(", ");
      }
    }
    qDebug().noquote() << btb1CString;

    qDebug() << "";
    qDebug() << "Test acccessors";
    qDebug() << "";
    qDebug() << "Pole Right Ascension";
    qDebug() << btb1.solvePoleRA() << btb1.solvePoleRAVelocity() << btb1.solvePoleRAAcceleration();
    qDebug() << "Pole Declination";
    qDebug() << btb1.solvePoleDec() << btb1.solvePoleDecVelocity() << btb1.solvePoleDecAcceleration();
    qDebug() << "Prime Meridian";
    qDebug() << btb1.solvePM() << btb1.solvePMVelocity() << btb1.solvePMAcceleration();
    qDebug() << "Radii";
    qDebug() << btb1.solveTriaxialRadii() << btb1.solveMeanRadius();
    qDebug() << "Parameter Count";
    qDebug() << btb1.numberRadiusParameters() << btb1.numberParameters();
    qDebug() << "Parameter Solutions";
    LinearAlgebra::Vector btb1Solutions = btb1.parameterSolution();
    QString btb1SString;
    for (size_t i = 0; i < btb1Solutions.size(); i++) {
      btb1SString.append(QString::number(btb1Solutions[i]));
      if (i < btb1Solutions.size() - 1) {
        btb1SString.append(", ");
      }
    }
    qDebug().noquote() << btb1SString;
    qDebug() << "Apriori Sigmas";
    LinearAlgebra::Vector btb1Apriori = btb1.aprioriSigmas();
    QString btb1AprioriString;
    for (size_t i = 0; i < btb1Apriori.size(); i++) {
      btb1AprioriString.append(QString::number(btb1Apriori[i]));
      if (i < btb1Apriori.size() - 1) {
        btb1AprioriString.append(", ");
      }
    }
    qDebug().noquote() << btb1AprioriString;
    qDebug() << "Adjusted Sigmas";
    LinearAlgebra::Vector btb1Adjusted = btb1.adjustedSigmas();
    QString btb1AdjustedString;
    for (size_t i = 0; i < btb1Adjusted.size(); i++) {
      btb1AdjustedString.append(QString::number(btb1Adjusted[i]));
      if (i < btb1Adjusted.size() - 1) {
        btb1AdjustedString.append(", ");
      }
    }
    qDebug().noquote() << btb1AdjustedString;
    qDebug() << "Pole Right Ascension Coefficients";
    std::vector<Angle> btb1RACoefs = btb1.poleRaCoefs();
    QString btb1RACoefString;
    for (size_t i = 0; i < btb1RACoefs.size(); i++) {
      btb1RACoefString.append(btb1RACoefs[i].toString());
      if (i < btb1RACoefs.size() - 1) {
        btb1RACoefString.append(", ");
      }
    }
    qDebug().noquote() << btb1RACoefString;
    qDebug() << "Pole Declination Coefficients";
    std::vector<Angle> btb1DecCoefs = btb1.poleDecCoefs();
    QString btb1DecCoefString;
    for (size_t i = 0; i < btb1DecCoefs.size(); i++) {
      btb1DecCoefString.append(btb1DecCoefs[i].toString());
      if (i < btb1DecCoefs.size() - 1) {
        btb1DecCoefString.append(", ");
      }
    }
    qDebug().noquote() << btb1DecCoefString;
    qDebug() << "Prime Meridian Coefficients";
    std::vector<Angle> btb1PMCoefs = btb1.pmCoefs();
    QString btb1PMCoefString;
    for (size_t i = 0; i < btb1PMCoefs.size(); i++) {
      btb1PMCoefString.append(btb1PMCoefs[i].toString());
      if (i < btb1PMCoefs.size() - 1) {
        btb1PMCoefString.append(", ");
      }
    }
    qDebug().noquote() << btb1PMCoefString;
    qDebug() << "VtPV";
    qDebug() << btb1.vtpv();
    qDebug() << "Local Radius";
    qDebug().noquote()
      << btb1.localRadius(Latitude(15, Angle::Degrees), Longitude(15, Angle::Degrees)).toString();

    qDebug() << "";
    qDebug() << "Test copy constructor";
    qDebug() << "";
    BundleTargetBody btb3(btb1);
    qDebug().noquote() << btb3.formatBundleOutputString(true);

    qDebug() << "Switch free and valid sigmas";
    qDebug() << "";
    poleRASigma          = 0.2;
    poleRAVelocitySigma  = 3.5;
    poleDecSigma         = 5.0;
    poleDecVelocitySigma = 7.0;
    pmSigma              = 9.0;
    pmVelocitySigma      = 11.0;
    aRadiusSigma         = 0.0;
    bRadiusSigma         = 0.0;
    cRadiusSigma         = 0.0;
    btb1.setSolveSettings(
        targetParameterSolveCodes,
        Angle(poleRA, Angle::Degrees),             Angle(poleRASigma, Angle::Degrees),
        Angle(poleRAVelocity, Angle::Degrees),     Angle(poleRAVelocitySigma, Angle::Degrees),
        Angle(poleDec, Angle::Degrees),            Angle(poleDecSigma, Angle::Degrees),
        Angle(poleDecVelocity, Angle::Degrees),    Angle(poleDecVelocitySigma, Angle::Degrees),
        Angle(pm, Angle::Degrees),                 Angle(pmSigma, Angle::Degrees),
        Angle(pmVelocity, Angle::Degrees),         Angle(pmVelocitySigma, Angle::Degrees),
        (BundleTargetBody::TargetRadiiSolveMethod)2,
        Distance(aRadius, Distance::Kilometers),   Distance(aRadiusSigma, Distance::Kilometers),
        Distance(bRadius, Distance::Kilometers),   Distance(bRadiusSigma, Distance::Kilometers),
        Distance(cRadius, Distance::Kilometers),   Distance(cRadiusSigma, Distance::Kilometers),
        Distance(meanRadius ,Distance::Kilometers),Distance(meanRadiusSigma, Distance::Kilometers));
    qDebug().noquote() << btb1.formatBundleOutputString(true);
    btb1Weights = btb1.parameterWeights();
    btb1WString.clear();
    for (size_t i = 0; i < btb1Weights.size(); i++) {
      btb1WString.append(QString::number(btb1Weights[i]));
      if (i < btb1Weights.size() - 1) {
        btb1WString.append(", ");
      }
    }
    qDebug().noquote() << btb1WString;

    qDebug() << "";
    qDebug() << "Test assignment operator";
    qDebug() << "";
    qDebug() << "Self assignment";
    {
      BundleTargetBody &tbtb3 = btb3;
      btb3 = tbtb3;
    }
    qDebug().noquote() << btb3.formatBundleOutputString(true);
    qDebug() << "Assignment to other";
    btb3 = btb1;
    qDebug().noquote() << btb3.formatBundleOutputString(true);

    qDebug() << "Setup a BundleTargetBody that solves for only mean radius";
    qDebug() << "";
    BundleTargetBody btb2;
    targetParameterSolveCodes.clear();
    targetParameterSolveCodes.insert(BundleTargetBody::MeanRadius);
    btb2.setSolveSettings(
        targetParameterSolveCodes,
        Angle(poleRA, Angle::Degrees),             Angle(poleRASigma, Angle::Degrees),
        Angle(poleRAVelocity, Angle::Degrees),     Angle(poleRAVelocitySigma, Angle::Degrees),
        Angle(poleDec, Angle::Degrees),            Angle(poleDecSigma, Angle::Degrees),
        Angle(poleDecVelocity, Angle::Degrees),    Angle(poleDecVelocitySigma, Angle::Degrees),
        Angle(pm, Angle::Degrees),                 Angle(pmSigma, Angle::Degrees),
        Angle(pmVelocity, Angle::Degrees),         Angle(pmVelocitySigma, Angle::Degrees),
        (BundleTargetBody::TargetRadiiSolveMethod)1,
        Distance(aRadius, Distance::Kilometers),   Distance(aRadiusSigma, Distance::Kilometers),
        Distance(bRadius, Distance::Kilometers),   Distance(bRadiusSigma, Distance::Kilometers),
        Distance(cRadius, Distance::Kilometers),   Distance(cRadiusSigma, Distance::Kilometers),
        Distance(meanRadius ,Distance::Kilometers),Distance(meanRadiusSigma, Distance::Kilometers));
    qDebug().noquote() << btb2.formatBundleOutputString(true);
    LinearAlgebra::Vector btb2Weights = btb2.parameterWeights();
    QString btb2WString;
    for (size_t i = 0; i < btb2Weights.size(); i++) {
      btb2WString.append(QString::number(btb2Weights[i]));
      if (i < btb2Weights.size() - 1) {
        btb2WString.append(", ");
      }
    }
    qDebug().noquote() << btb2WString;
    qDebug() << "";
    qDebug().noquote() << btb2.meanRadius().toString();
    qDebug() << "";
    qDebug() << "Switch free and valid sigmas";
    qDebug() << "";
    meanRadiusSigma = 0;
    btb2.setSolveSettings(
        targetParameterSolveCodes,
        Angle(poleRA, Angle::Degrees),             Angle(poleRASigma, Angle::Degrees),
        Angle(poleRAVelocity, Angle::Degrees),     Angle(poleRAVelocitySigma, Angle::Degrees),
        Angle(poleDec, Angle::Degrees),            Angle(poleDecSigma, Angle::Degrees),
        Angle(poleDecVelocity, Angle::Degrees),    Angle(poleDecVelocitySigma, Angle::Degrees),
        Angle(pm, Angle::Degrees),                 Angle(pmSigma, Angle::Degrees),
        Angle(pmVelocity, Angle::Degrees),         Angle(pmVelocitySigma, Angle::Degrees),
        (BundleTargetBody::TargetRadiiSolveMethod)1,
        Distance(aRadius, Distance::Kilometers),   Distance(aRadiusSigma, Distance::Kilometers),
        Distance(bRadius, Distance::Kilometers),   Distance(bRadiusSigma, Distance::Kilometers),
        Distance(cRadius, Distance::Kilometers),   Distance(cRadiusSigma, Distance::Kilometers),
        Distance(meanRadius ,Distance::Kilometers),Distance(meanRadiusSigma, Distance::Kilometers));
    qDebug().noquote() << btb2.formatBundleOutputString(true);
    btb2Weights = btb2.parameterWeights();
    btb2WString.clear();
    for (size_t i = 0; i < btb2Weights.size(); i++) {
      btb2WString.append(QString::number(btb2Weights[i]));
      if (i < btb2Weights.size() - 1) {
        btb2WString.append(", ");
      }
    }
    qDebug().noquote() << btb2WString;

    qDebug() << "";
    qDebug() << "Test reading from a PvlObject";
    qDebug() << "";
    PvlGroup goodRAGroup;
    goodRAGroup += PvlKeyword("Ra", "velocity");
    goodRAGroup += PvlKeyword("RaValue", "15");
    goodRAGroup += PvlKeyword("RaSigma", "0.487");
    goodRAGroup += PvlKeyword("RaVelocityValue", "10");
    goodRAGroup += PvlKeyword("RaVelocitySigma", "1.01");
    PvlGroup goodDecGroup;
    goodDecGroup += PvlKeyword("Dec", "velocity");
    goodDecGroup += PvlKeyword("DecValue", "25");
    goodDecGroup += PvlKeyword("DecSigma", "2.3");
    goodDecGroup += PvlKeyword("DecVelocityValue", "5");
    goodDecGroup += PvlKeyword("DecVelocitySigma", "0.03");
    PvlGroup goodPMGroup;
    goodPMGroup += PvlKeyword("PM", "velocity");
    goodPMGroup += PvlKeyword("PmValue", "20");
    goodPMGroup += PvlKeyword("PmSigma", "2.4");
    goodPMGroup += PvlKeyword("PmVelocityValue", "30");
    goodPMGroup += PvlKeyword("pmVelocitySigma", "10");
    PvlGroup goodRadiiGroup;
    goodRadiiGroup += PvlKeyword("RadiiSolveOption", "triaxial");
    goodRadiiGroup += PvlKeyword("RadiusAValue", "2");
    goodRadiiGroup += PvlKeyword("RadiusASigma", "0.2");
    goodRadiiGroup += PvlKeyword("RadiusBValue", "3");
    goodRadiiGroup += PvlKeyword("RadiusBSigma", "0.3");
    goodRadiiGroup += PvlKeyword("RadiuscValue", "4");
    goodRadiiGroup += PvlKeyword("RadiuscSigma", "0.4");
    PvlObject goodBTBObject;
    goodBTBObject += goodRAGroup;
    goodBTBObject += goodDecGroup;
    goodBTBObject += goodPMGroup;
    goodBTBObject += goodRadiiGroup;
    btb3.readFromPvl(goodBTBObject);
    qDebug().noquote() << btb3.formatBundleOutputString(true);

    qDebug() << "Test error throws";
    qDebug() << "";

    // Correction errors (parameters and corrections mismatch)
    try {
      btb1.applyParameterCorrections(LinearAlgebra::Vector(btb1.numberParameters() + 1));
    }
    catch (IException &e) {
      e.print();
    }

    // Internal correction errors - the corrections vector contains a non-Null special Isis
    // pixel value (e.g. Hrs, Lrs...), and one of our parameter solve codes is for an angle.
    // This causes the Angle(double, Angle::Radians) to throw an exception.
    try {
      LinearAlgebra::Vector hasSpecial(btb1.numberParameters());
      hasSpecial[0] = Isis::Lrs;
      btb1.applyParameterCorrections(hasSpecial);
    }
    catch (IException &e) {
      e.print();
    }

    // Radii solve method errors
    try {
      BundleTargetBody::stringToTargetRadiiOption("Invalid Method");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleTargetBody::targetRadiiOptionToString((BundleTargetBody::TargetRadiiSolveMethod)-1);
    }
    catch (IException &e) {
      e.print();
    }

    // Radii accessor errors
    try {
      btb2.radii();
    }
    catch (IException &e) {
      e.print();
    }
    try {
      btb1.meanRadius();
    }
    catch (IException &e) {
      e.print();
    }

    // local radius error
    try {
      btb2.localRadius(Latitude(15, Angle::Degrees), Longitude(15, Angle::Degrees));
    }
    catch (IException &e) {
      e.print();
    }

    // Read Pvl errors
    PvlObject badBTBObject;
    PvlGroup badRaPosGroup;
    badRaPosGroup += PvlKeyword("RaValue", "Not a double");
    badBTBObject += badRaPosGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRaPosSigGroup;
    badRaPosSigGroup += PvlKeyword("RaSigma", "Also not a double");
    badBTBObject += badRaPosSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRaVelGroup;
    badRaVelGroup += PvlKeyword("RaVelocityValue", "Still not a double");
    badBTBObject += badRaVelGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRaVelSigGroup;
    badRaVelSigGroup += PvlKeyword("RaVelocitySigma", "Definitely not a double");
    badBTBObject += badRaVelSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRaAccGroup;
    badRaAccGroup += PvlKeyword("RaAccelerationValue", "A string");
    badBTBObject += badRaAccGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRaAccSigGroup;
    badRaAccSigGroup += PvlKeyword("RaAccelerationSigma", "Also a string");
    badBTBObject += badRaAccSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badDecPosGroup;
    badDecPosGroup += PvlKeyword("DecValue", "Another string");
    badBTBObject += badDecPosGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badDecPosSigGroup;
    badDecPosSigGroup += PvlKeyword("DecSigma", "The seventh string");
    badBTBObject += badDecPosSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badDecVelGroup;
    badDecVelGroup += PvlKeyword("DecVelocityValue", "The loneliest string");
    badBTBObject += badDecVelGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badDecVelSigGroup;
    badDecVelSigGroup += PvlKeyword("DecVelocitySigma", "The happy string");
    badBTBObject += badDecVelSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badDecAccGroup;
    badDecAccGroup += PvlKeyword("DecAccelerationValue", "The fast string");
    badBTBObject += badDecAccGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badDecAccSigGroup;
    badDecAccSigGroup += PvlKeyword("DecAccelerationSigma", "The wobbling string");
    badBTBObject += badDecAccSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badPMGroup;
    badPMGroup += PvlKeyword("PmValue", "Or are they char arrays?");
    badBTBObject += badPMGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badPMSigGroup;
    badPMGroup += PvlKeyword("PmSigma", "This one is");
    badBTBObject += badPMGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badPMVelGroup;
    badPMVelGroup += PvlKeyword("PmVelocityValue", "This is also a char array");
    badBTBObject += badPMVelGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badPMVelSigGroup;
    badPMVelSigGroup += PvlKeyword("pmVelocitySigma", "These still aren't doubles");
    badBTBObject += badPMVelSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badPMAccGroup;
    badPMAccGroup += PvlKeyword("PmAccelerationValue", "And that's what matters");
    badBTBObject += badPMAccGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badPMAccSigGroup;
    badPMAccSigGroup += PvlKeyword("PmAccelerationSigma", "The eighteenth not double");
    badBTBObject += badPMAccSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRadAGroup;
    badRadAGroup += PvlKeyword("RadiusAValue", "The twentieth not double");
    badBTBObject += badRadAGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negRadAGroup;
    negRadAGroup += PvlKeyword("RadiusAValue", "-8");
    badBTBObject += negRadAGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRadASigGroup;
    badRadASigGroup += PvlKeyword("RadiusASigma", "The true twentieth not double");
    badBTBObject += badRadASigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negRadASigGroup;
    negRadASigGroup += PvlKeyword("RadiusASigma", "-7");
    badBTBObject += negRadASigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRadBGroup;
    badRadBGroup += PvlKeyword("RadiusBValue", "Only five more");
    badBTBObject += badRadBGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negRadBGroup;
    negRadBGroup += PvlKeyword("RadiusBValue", "-6");
    badBTBObject += negRadBGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRadBSigGroup;
    badRadBSigGroup += PvlKeyword("RadiusBSigma", "Only four more");
    badBTBObject += badRadBSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negRadBSigGroup;
    negRadBSigGroup += PvlKeyword("RadiusBSigma", "-5");
    badBTBObject += negRadBSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRadCGroup;
    badRadCGroup += PvlKeyword("RadiusCValue", "Only three more");
    badBTBObject += badRadCGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negRadCGroup;
    negRadCGroup += PvlKeyword("RadiusCValue", "-4");
    badBTBObject += negRadCGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badRadCSigGroup;
    badRadCSigGroup += PvlKeyword("RadiusCSigma", "Only two more");
    badBTBObject += badRadCSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negRadCSigGroup;
    negRadCSigGroup += PvlKeyword("RadiusCSigma", "-3");
    badBTBObject += negRadCSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badMeanRadGroup;
    badMeanRadGroup += PvlKeyword("MeanRadiusValue", "Only one more");
    badBTBObject += badMeanRadGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negMeanRadGroup;
    negMeanRadGroup += PvlKeyword("MeanRadiusValue", "-2");
    badBTBObject += negMeanRadGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup badMeanRadSigGroup;
    badMeanRadSigGroup += PvlKeyword("MeanRadiusSigma", "The end");
    badBTBObject += badMeanRadSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    badBTBObject.clear();
    PvlGroup negMeanRadSigGroup;
    negMeanRadSigGroup += PvlKeyword("MeanRadiusSigma", "-1");
    badBTBObject += negMeanRadSigGroup;
    try {
      btb3.readFromPvl(badBTBObject);
    }
    catch (IException &e) {
      e.print();
    }
    qDebug() << "";
    qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
  }
  catch (IException &e) {
    e.print();
  }

}


/**
 * Outputs the BundleMeasure state fo testing and verification
 *
 * @param m The BundleMeasure to print information on
 */
 void printBundleMeasure(BundleMeasure &m) {
   qDebug() << "rejected?" << QString::number(m.isRejected());
   qDebug() << "measure sample " << QString::number(m.sample());
   qDebug() << "measure line   " << QString::number(m.line());
   qDebug() << "sample residual" << QString::number(m.sampleResidual());
   qDebug() << "line residual" << QString::number(m.lineResidual());
   qDebug() << "residual magnitude" << QString::number(m.residualMagnitude());
   qDebug() << "measure serial number" << m.cubeSerialNumber();
   qDebug() << "focal x" << QString::number(m.focalPlaneMeasuredX());
   qDebug() << "focal y" << QString::number(m.focalPlaneMeasuredY());
   qDebug() << "computed focal x" << QString::number(m.focalPlaneComputedX());
   qDebug() << "computed focal y" << QString::number(m.focalPlaneComputedY());
   qDebug() << "observation index" << QString::number(m.observationIndex());
   qDebug() << "";
}


void printXml(const BundleObservationSolveSettings &printable) {
  QString output;
  QXmlStreamWriter writer(&output);
  writer.setAutoFormatting(true);
  printable.save(writer, NULL);
  output.remove(QRegExp("<id>[^<]*</id>"));
  qDebug().noquote() << output << Qt::endl << Qt::endl;
}
