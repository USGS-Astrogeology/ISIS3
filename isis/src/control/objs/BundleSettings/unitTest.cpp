/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QByteArray>
#include <QDataStream>
#include <QDebug>
#include <QFile>
#include <QList>
#include <QRegExp>
#include <QString>
#include <QtDebug>
#include <QXmlStreamWriter>
#include <QXmlInputSource>

#include "BundleObservationSolveSettings.h"
#include "BundleSettings.h"
#include "FileName.h"
#include "IException.h"
#include "MaximumLikelihoodWFunctions.h"
#include "Preference.h"
#include "PvlObject.h"


using namespace std;
using namespace Isis;

template<class T>
void printXml(const T &);

/**
  * Test BundleSettings
  *
  *
  * @author 2014-05-14 Jeannie Backer
  * @internal
  *   @history 2014-05-14 Jeannie Backer - Original version. Code test coverage:
  *                           Scope (100%), Line (100%), Function(100%)
  *   @history 2016-07-07 Jeannie Backer - Updated to include TargetBody tests.
  *                           Note: This is still incomplete until we refactor how TargetBody
  *   @history 2016-08-12 Jeannie Backer - Removed references to solve method. References #4162.
  *   @history 2016-08-18 Jeannie Backer - Removed references to BundleSettings solve method.
  *                           References #4162.
  *   @history 2016-10-13 Ian Humphrey - Replaced setInstrumentId() with addObservationNumber()
  *                           to associate the bundle observation solve settings with an
  *                           observation number. References #4293.
  *   @history 2016-10-24 Makayla Shepherd - Commented out the test for setting the default settings
  *                           when reading from an empty XML. This test was failing on prog24 and we
  *                           are not sure if we are going to allow XML reading/writing as we are
  *                           most likely going to move to HDF5. Fixes #4327.
  *   @history 2017-04-24 Ian Humphrey - Removed pvlObject() and replaced with the XML save().
  *                           Fixes #4797.
  *   @history 2018-05-24 Ken Edmundson - Updated truth data to indicate default value for
  *                                       m_createInverseMatrix was changed from true to false.
  *   @history 2018-06-04 Debbie A. Cook - (added to BundleXYZ branch on 2017-06-25) Updated to
  *                           reflect added argument for control pointcoordinate type.  References
  *                           #4649 and #501.
  *
  *   @todo Truth updated so that the name of the BundleObservationSolveSettings object is Null,
  *         this should be fixed as part of #4292.
  *   @todo Test hdf5 methods when added.
  *   @todo Test setBundleTargetBody()
  *   @todo Test non-null bundleTargetBody()
  *
  */


namespace Isis {
  /**
   * Child class of BundleSettings used to test the embedded XML handler class.
   *
   * @author 2014-05-14 Jeannie Backer
   * @internal
   *   @history 2014-05-14 Jeannie Backer - Original version.
   */
  class BundleSettingsXmlHandlerTester : public BundleSettings {
    public:
      /**
       * Constructs BundleSettings using XML handler.
       *
       * @param reader A pointer to a QXmlStreamReader.
       * @param xmlFile The name of the XML file to be used to create a
       *                BundleSettings object.
       *
       *
       * @throw Isis::Exception::Io "Unable to open XML file with read access."
       * @throw Isis::Exception::Unknown "Failed to parse XML file."
       */
      BundleSettingsXmlHandlerTester(QXmlStreamReader *reader,
                                     FileName xmlFile) : BundleSettings() {

        QString xmlPath(QString::fromStdString(xmlFile.expanded()));
        QFile file(xmlPath);

        if (!file.open(QFile::ReadOnly) ) {
          throw IException(IException::Io,
                           "Unable to open xml file, ["+xmlPath.toStdString()+"],  with read access",
                           _FILEINFO_);
        }

        if (reader->readNextStartElement()) {
          if (reader->name() == "bundleSettings") {
            readBundleSettings(reader);
          }
          else {
            reader->raiseError(QObject::tr("Incorrect file"));
          }
        }

      }

      ~BundleSettingsXmlHandlerTester() {
      }

  };
}


int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cout.precision(6);

  try {
    qDebug() << "Unit test for BundleSettings...";
    qDebug() << "Printing PVL group with settings from the default constructor...";
    BundleSettings settings;
    // tested fully by each call to pvlObject()
    //      bool validateNetwork() const;
    //      bool solveObservationMode() const;
    //      bool solveRadius() const;
    //      bool updateCubeLabel() const;
    //      bool errorPropagation() const;
    //      bool outlierRejection() const;
    //      double outlierRejectionMultiplier() const;
    //      double globalLatitudeAprioriSigma() const;
    //      double globalLongitudeAprioriSigma() const;
    //      double globalRadiusAprioriSigma() const;
    //      ConvergenceCriteria convergenceCriteria() const;
    //      double convergenceCriteriaThreshold() const;
    //      int convergenceCriteriaMaximumIterations() const;
    //      QString outputFilePrefix() const;
    //      int numberSolveSettings();
    //      int numberTargetBodyParameters()
    //      bool solveTargetBody()
    //      bool solvePoleRA()
    //      bool solvePoleRAVelocity()
    //      bool solvePoleDec()
    //      bool solvePoleDecVelocity()
    //      bool solvePM()
    //      bool solvePMVelocity()
    //      bool solvePMAcceleration()
    //      bool solveTriaxialRadii()
    //      bool solveMeanRadius()

    // Default bundle settings
    printXml<BundleSettings>(settings);


    qDebug() << "Testing copy constructor...";
    BundleSettings copySettings(settings);
    printXml<BundleSettings>(copySettings);

    qDebug() << "Testing assignment operator to set this equal to itself...";
    {
      BundleSettings &s = settings;
      settings = s;
    }

    printXml<BundleSettings>(settings);

    qDebug() << "Testing assignment operator to create a new settings object...";
    BundleSettings assignmentOpSettings;
    assignmentOpSettings = settings;
    printXml<BundleSettings>(assignmentOpSettings);

    qDebug() << "Testing mutator methods...";
    // reset all...
    // validate the network
    copySettings.setValidateNetwork(true);
    // Assume for now the user interface will prevent any invalid coordinate types in the
    // set the solve options
    copySettings.setSolveOptions(true, true, true, true, SurfacePoint::Rectangular,
                                  SurfacePoint::Rectangular, 1000.0, 2000.0, 3000.0);
    // set outlier rejection
    copySettings.setOutlierRejection(true, 4.0);
    // create and fill the list of observation solve settings... then set
    BundleObservationSolveSettings boss1;
    boss1.addObservationNumber("Instrument1");
    boss1.setInstrumentPositionSettings(BundleObservationSolveSettings::AllPositionCoefficients,
                                        5, 6, true, 7000.0, 8000.0, 9000.0);
    boss1.setInstrumentPointingSettings(BundleObservationSolveSettings::NoPointingFactors,
                                        10, 11, true, true, 12.0, 13.0, 14.0);
    QList<BundleObservationSolveSettings> observationSolveSettings;
    observationSolveSettings.append(boss1);
    boss1.addObservationNumber("Instrument2");
    boss1.setInstrumentPositionSettings(BundleObservationSolveSettings::PositionOnly,
                                        15, 16, true, 17000.0, 18000.0, 19000.0);
    boss1.setInstrumentPointingSettings(BundleObservationSolveSettings::AllPointingCoefficients,
                                        20, 21, true, true, 22.0, 23.0, 24.0);
    observationSolveSettings.append(boss1);
    copySettings.setObservationSolveOptions(observationSolveSettings);
    // set convergence criteria values
    copySettings.setConvergenceCriteria(
                     BundleSettings::stringToConvergenceCriteria("parametercorrections"), 0.25, 26);
    // set maximum likelihood models
    copySettings.addMaximumLikelihoodEstimatorModel(MaximumLikelihoodWFunctions::Huber, 0.27);
    copySettings.addMaximumLikelihoodEstimatorModel(MaximumLikelihoodWFunctions::Welsch, 28);
    copySettings.addMaximumLikelihoodEstimatorModel(MaximumLikelihoodWFunctions::HuberModified, 29);
    copySettings.addMaximumLikelihoodEstimatorModel(MaximumLikelihoodWFunctions::Chen, 30);
    // set target body
    // TODO
    // set output file options
    copySettings.setOutputFilePrefix("TestFilePrefix");
    printXml<BundleSettings>(copySettings);

    // now for test coverage, call some more resets
    // SolveObservationMode = UpdateCubeLabel = ErrorPropagation = true and
    // SolveRadius = OutlierRejection = false
    settings.setSolveOptions(true, true, true, false, SurfacePoint::Latitudinal);
    settings.setOutlierRejection(false);
    settings.setOutputFilePrefix("TestFilePrefix");
    printXml<BundleSettings>(settings);

    // reset output options - test coverage
    settings.setOutputFilePrefix("TestFilePrefix");
    printXml<BundleSettings>(settings);

    qDebug() << "Testing accessor methods...";
    BundleObservationSolveSettings boss2 = copySettings.observationSolveSettings("Instrument1");
    qDebug() << "Get BundleObservationSolveSettings with name InstrumentId = Instrument1";
    printXml<BundleObservationSolveSettings>(boss2);

    boss2 = copySettings.observationSolveSettings(1);
    qDebug() << "Now get BundleObservationSolveSettings at index 1";
    printXml<BundleObservationSolveSettings>(boss2);

    QList< QPair< MaximumLikelihoodWFunctions::Model, double > > models
            = copySettings.maximumLikelihoodEstimatorModels();
    for (int i = 0; i < models.size(); i++) {
      qDebug() << MaximumLikelihoodWFunctions::modelToString(models[i].first)
               << QString::number(models[i].second);
    }
    qDebug() << "";

    qDebug() << "Testing static enum-to-string and string-to-enum methods...";
    qDebug() << BundleSettings::convergenceCriteriaToString(
                               BundleSettings::stringToConvergenceCriteria("SIGMA0"));
    qDebug() << BundleSettings::convergenceCriteriaToString(
                               BundleSettings::stringToConvergenceCriteria("PARAMETERCORRECTIONS"));
    qDebug() << "";

    qDebug() << "Testing XML serialization 1: write XML from BundleSettings object...";
    // write xml to test output file for comparison
    qDebug() << "Serializing test XML object to file:";
    printXml<BundleSettings>(settings);
    // now write the object to serialization file
    FileName xmlFile("./BundleSettings2.xml");
    std::string xmlPath = xmlFile.expanded();
    QFile qXmlFile(QString::fromStdString(xmlPath));
    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    QXmlStreamWriter writer(&qXmlFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    Project *project = NULL;
    settings.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();

    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }

    // read serialized xml into object and then write object to log file
    qDebug() << "Testing XML: Object deserialized as (should match object above):";
    QXmlStreamReader reader(&qXmlFile);
    BundleSettingsXmlHandlerTester bsFromXml(&reader, xmlFile);
    printXml<BundleSettings>(bsFromXml);
    qXmlFile.close();

    qDebug() << "Testing XML serialization 2: write XML from BundleSettings object...";
    // for test coverage, read/write the copySettings object with
    // solveRadius=true, outlierRejection=true, no observationSolveSettings,
    // globalRadiusAprioriSigma != N/A, outlierRejectionMultiplier != N/A
    if (!qXmlFile.open(QIODevice::WriteOnly|QIODevice::Text)) {
      throw IException(IException::Io,
                       "Unable to open xml file, ["+xmlPath+"],  with write access",
                       _FILEINFO_);
    }
    // write xml to test output file for comparison
    qDebug() << "Serializing test XML object to file:";
    printXml<BundleSettings>(copySettings);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    copySettings.save(writer, project);
    writer.writeEndDocument();
    qXmlFile.close();

    if(!qXmlFile.open(QFile::ReadOnly | QFile::Text)){
      throw IException(IException::Unknown,
                        "Failed to parse xml file, ["+qXmlFile.fileName().toStdString()+"]",
                        _FILEINFO_);
    }

    // read serialized xml into object and then write object to log file
    qDebug() << "Testing XML: Object deserialized as (should match object above):";
    QXmlStreamReader reader2(&qXmlFile);
    BundleSettingsXmlHandlerTester bsFromXml2(&reader2, xmlFile);
    printXml<BundleSettings>(bsFromXml2);
    qXmlFile.close();
    qXmlFile.remove();


    qDebug() << "Testing error throws..."; // ??? weird error if i move this after read from empty???
    try {
      settings.observationSolveSettings("UnassociatedObservationNumber");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      settings.observationSolveSettings(32);
    }
    catch (IException &e) {
      e.print();
    }
    try {
      settings.observationSolveSettings(-1.0);
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleSettings::stringToConvergenceCriteria("Pickles");
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleSettings::convergenceCriteriaToString(BundleSettings::ConvergenceCriteria(33));
    }
    catch (IException &e) {
      e.print();
    }
    try {
      BundleSettings invalidMaxLikelihoodModel1;
      invalidMaxLikelihoodModel1.addMaximumLikelihoodEstimatorModel(
          MaximumLikelihoodWFunctions::Chen, 33);
    }
    catch (IException &e) {
      e.print();
    }

    /*
     * Commenting out this test because it is causing an error on prog24 and we are not sure
     * if we are going to include XML reading/writing
     *
     * qDebug() << "";    // read xml with no attributes or values
     * qDebug() << "Testing XML: read XML with no attributes or values to object...";
     * FileName emptyXmlFile("./unitTest_NoElementValues.xml");
     * BundleSettingsXmlHandlerTester bsFromEmptyXml(project, &reader, emptyXmlFile);
     * pvl = bsFromEmptyXml.pvlObject("DefaultBundleSettingsFromEmptyXml");
     * cout << pvl << endl << endl;
    */

  }
  catch (IException &e) {
    e.print();
  }
}


/**
 * Prints the serialized BundleSettings as XML.
 *
 * @param const T &printable The BundleSettings to print.
 */
template<class T>
void printXml(const T &printable) {
  QString output;
  QXmlStreamWriter writer(&output);
  writer.setAutoFormatting(true);
  printable.save(writer, NULL);
  // needed to remove UUids from sub-object serialization.
  output.remove(QRegExp("<id>[^<]*</id>"));
  qDebug().noquote() << output << Qt::endl << Qt::endl;
}
