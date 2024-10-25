#ifndef BundleSolutionInfo_h
#define BundleSolutionInfo_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QList>
#include <QObject>
#include <QString>

#include "BundleObservation.h"
#include "BundleSettings.h"
#include "LidarData.h"
#include "SurfacePoint.h"

class QDataStream;
class QUuid;
class QXmlStreamWriter;
class QXmlStreamReader;

namespace Isis {
  class BundleResults;
  class Control;
  class FileName;
  class ImageList;
  class Project;  //TODO does xml stuff need project???
  class PvlObject;

  /**
   * @brief Container class for BundleAdjustment results.
   *
   * This class includes the settings used to run the bundle adjustment, the resulting statistics
   * values, and the name of the control network used.
   *  NOTE: BundleSolutionInfo is derived from QObject as it has one slot (perhaps more signals
   *       and slots in the future? As a child of QObject it should have no copy constructor or
   *       assignment operator. See for example...
   *
   *       http://doc.qt.io/qt-5/qobject.html#no-copy-constructor
   *
   * @ingroup ControlNetworks
   *
   * @author 2014-07-08 Jeannie Backer
   *
   * @internal
   *   @history 2014-07-08 Jeannie Backer - Original version.
   *   @history 2014-07-23 Jeannie Backer - Added implementation for the QDataStream << and >>
   *                           operators and the read/write methods.
   *   @history 2014-12-04 Jeannie Backer - Renamed from BundleResults to BundleSolutionInfo.
   *   @history 2015-09-03 Jeannie Backer - Added preliminary hdf5 read/write capabilities.
   *   @history 2015-10-14 Jeffrey Covington - Declared BundleSolutionInfo * as
   *                           a Qt metatype for use with QVariant.
   *   @history 2016-06-13 Makayla Shepherd - Added updateFileName() and updated documentation.
   *                           Fixes #2298.
   *   @history 2016-08-15 Jesse Mapel - added output, outputHeader, outputText, outputPointsCSV,
   *                           and outputResiduals from BundleAdjust.  Fixes #4159.
   *   @history 2016-08-18 Jeannie Backer - Removed all references to deprecated
   *                           BundleSettings::solveMethod. References #4162.
   *   @history 2016-08-23 Jesse Mapel - Removed output() method.  Individual output
   *                           file methods must be called.  Fixes #4279.
   *   @history 2016-09-02 Jesse Mapel - Added camera point and position input parameters to output
   *                           files when using only one set of solve settings.  Fixes #4316.
   *   @history 2016-10-06  Tyler Wilson - Added methods outputImagesCSV()
   *                           and outputImagesCSVHeader which enables jigsaw users to output
   *                           the bundleout_images.csv file.  Fixes #4314.
   *   @history 2016-10-17 Jesse Mapel - Removed multiple solve settings output in accordance with
   *                           USEPVL being removed from jigsaw.  References #4316.
   *   @history 2016-10-28 Tyler Wilson - Modified outputText() to check and output if the solution
   *                           is solving for the radius.  References #4317.
   *   @history 2016-11-14 Ken Edmundson Modified the following...
   *                           -Changed column headers in images.csv to match row headers in bundleout.txt
   *                            (instead of at2, bt, c to indicate coefficients, now using t2, t1, t0, etc)
   *                           -Added output of CKDEGREE, CKSOLVEDEGREE, SPKDEGREE, SPKSOLVEDEGREE to
   *                            bundleout.txt header when CAMSOLVE=ALL and/or SPSOLVE=ALL
   *                           -Fixed typo under SPACECRAFT OPTIONS; what should have said
   *                            "SPSOLVE: All POLYNOMIAL COEFFICIENTS" was
   *                            "CAMSOLVE: All POLYNOMIAL COEFFICIENTS"
   *                           -modified output of image EO in bundleout.txt for images solved with
   *                            observation mode; previously one entry per observation was written,
   *                            now all images in the observation are written separately.
   *   @history 2016-12-01 Ian Humphrey - Modified an sprintf() call in outputImagesCSV() to
   *                           prevent a -Wformat-security warning from occurring.
   *   @history 2016-12-08 Ian Humphrey - Modified outputImagesCSVHeader() to treat TWIST the same
   *                           as the other angles when determining how many headers to create.
   *                           Fixes #4557.
   *   @history 2017-04-24 Ian Humphrey - Removed pvlObject(). Fixes #4797.
   *   @history 2017-05-01 Makayla Shepherd - Added imageList() to track and return the images used
   *                           in the bundle adjustment. These images will be displayed on the
   *                           project tree under results/bundle/<runtime> and will keep the same
   *                           structure as the input on the project tree. Fixes #4818.
   *   @history 2017-05-02 J Bonn - Fixed XML serialzation and code cleanup.  Fixes #4835.
   *   @history 2017-05-02 Tracie Sucharski - Moved XMLHandler code to bottom of file for
   *                           consistency;  all other classes have the XmlHandler at end of file.
   *                           Fixes #4822.
   *   @history 2017-05-04 Ian Humphrey & Makayla Shepherd - Modified save() to write the bundle
   *                           solution info images to the correct directory in the project on disk.
   *                           Fixes #4804, #4837.
   *   @history 2017-07-11 Makayla Shepherd - Added bundle naming capabilities. Fixes #4855.
   *   @history 2017-07-28 Makayla Shepherd - Fixed the default naming tag. Fixes #5069.
   *   @history 2017-08-09 Ian Humphrey - Added m_adjustedImages with setters and getters so the
   *                           BundleSolutionInfo can know which images have been adjusted (Updated
   *                           labels). References #4849.
   *   @history 2017-10-30 Tracie Sucharski - In ::save method, if the newProjectRoot is different
   *                           from the current projectRoot, save the cnet and csv files and
   *                           create the directory structure.
   *   @history 2017-12-20 Tracie Sucharski - Fixed bug which was saving the bundle adjust input
   *                           control net rather than the output control net.  References #4804.
   *   @history 2018-01-03 Tracie Sucharski - Changed serialization to use relative paths.
   *                           Fixes #5104.
   *   @history 2018-01-17 Tracie Sucharski - Added conditional code to check for null project in
   *                           xml serialization to allow the unitTest to use xml serialization
   *                           without having a project. References #5104.
   *   @history 2018-03-21 Ken Edmundson - Added...
   *                           1) member variable m_inputControlNetFileName, accessor method, and
   *                              serialization support. Also added input control net filename to
   *                              constructor.
   *                           2) member variable m_outputControl, associated mutator/accessor, and
   *                              serialization support.
   *                           3) member variable m_txtBundleOutputFilename and associated accessor
   *                              for bundleout.txt file.
   *   @history 2018-03-23 Ken Edmundson - modified...
   *                           1) removed serialization of output control filename
   *                           2) serialization of output control to be more robust, ensuring that
   *                              the control's id is added to project upon reading back in. Also
   *                              ensures that an open cneteditor widget containing a
   *                              bundlesolutioninfo's output control is serialized properly.
   *   @history 2018-03-26 Ken Edmundson - modified save method to properly save output control
   *                           network file.
   *   @history 2018-05-22 Ken Edmundson - changed default and copy constructors and assignment
   *                           operator to private to prevent developer from calling them. Done
   *                           because BundleSolutionInfo is derived from QObject (see comment
   *                           below). Removed copy constructor and assignment operator from cpp
   *                           file.
   *   @history 2018-06-01 Ken Edmundson - modifications to add lidar data input, output, and
   *                          serialization.
   *   @history 2018-06-01 Debbie A. Cook - ( Added 2018-02-21 to BundleXYZ branch) Added
   *                           coordinate types to report and appropriate headings for columns based
   *                           on the coordinate type.  Also added a utility method to return the
   *                           coordinate name based on coordinate type and coordinate index.
   *                           References #4649 and #501.
   *   @history 2018-09-18 Debbie A. Cook - Removed radiansToMeters argument.   References
   *                           #4649 and #501
   *   @history 2019-05-14 Tyler Wilson - Replaced call to
   *                           BundleObservation::formatBundeOutputString(...) in outputText()
   *                           file to a new function:  BundleObservation::bundleOutput
   *                           which takes as an argument the std::ostream.
   *   @history 2019-06-03 Tyler Wilson - Replaced all calls to
   *                           BundleObservation::formatBundleOutputString where it outputs a
   *                           csv file with BundleObservation::bundleOutputCSV.
   *                           BundleObservation::formatBundleOutputString was removed
   *                           from ISIS3 because it had become unmaintainable.
   *   @history 2019-06-03 Adam Paquette - Updated the header for the bundleout.txt file for
   *                           more human readable formatting in the bundleout.txt file.
   *   @history 2024-10-21 Ken Edmundson - 1) Fixed bug in the outputText method when error
   *                          propagation is on in a rectangular (XYZ) solution. In the "POINTS
   *                          UNCERTAINTY SUMMARY" section, instead of writing the RMS of
   *                          Z-coordinate sigmas, the RMS, MIN, and MAX of radius coordinates
   *                          was reported as "N/A"; 2) Modified outputHeader method
   *                          to output RADIUS: N/A under "INPUT: SOLVE OPTIONS" in a rectangular
   *                          solution. 3) Cleaned up spacing of Point Coordinate output in the
   *                          "INPUT: GLOBAL IMAGE PARAMETER UNCERTAINTIES" section. Originally
   *                          added to UofA code on 2019-07-30.
   *
   */
  class BundleSolutionInfo : public QObject {
    Q_OBJECT
    public:
      BundleSolutionInfo(BundleSettingsQsp inputSettings,
                    FileName controlNetworkFileName,
                    BundleResults outputStatistics,
                    QList<ImageList *> imgList,
                    QObject *parent = 0);
      BundleSolutionInfo(BundleSettingsQsp inputSettings,
                    FileName controlNetworkFileName,
                    FileName lidarDataFileName,
                    BundleResults outputStatistics,
                    QList<ImageList *> imgList,
                    QObject *parent = 0);
      BundleSolutionInfo(Project *project,
                         QXmlStreamReader *xmlReader,
                         QObject *parent = 0);
      void readBundleSolutionInfo(QXmlStreamReader *xmlReader);
      BundleSolutionInfo() = default;

      ~BundleSolutionInfo();

      QString savedBundleOutputFilename();
      QString savedImagesFilename();
      QString savedPointsFilename();
      QString savedResidualsFilename();

      void addAdjustedImages(ImageList *images);
      void setOutputStatistics(BundleResults statisticsResults);
      void setOutputControl(Control *outputControl);
      void setOutputControlName(QString name);
      void setRunTime(QString runTime);
      void setName(QString name);

      QList<ImageList *> adjustedImages() const;
      QString id() const;
      QString inputControlNetFileName() const;
      QString outputControlNetFileName() const;
      Control *control() const;
      QString outputControlName() const;
      QString inputLidarDataFileName() const;
      BundleSettingsQsp bundleSettings();
      BundleResults bundleResults();
      QList<ImageList *> imageList();
      QString runTime() const;
      QString name() const;


      bool outputImagesCSVHeader(std::ofstream &fpOut, BundleObservationQsp observations);
      bool outputHeader(std::ofstream &fpOut);
      bool outputText();
      bool outputImagesCSV();
      bool outputPointsCSV();
      bool outputLidarCSV();
      bool outputResiduals();

      void save(QXmlStreamWriter &stream, const Project *project, FileName newProjectRoot) const;

      QString surfacePointCoordName(SurfacePoint::CoordinateType type,
                                    SurfacePoint::CoordIndex coordInx) const;

    public slots:
      void updateFileName(Project *);

    private:

      //! A unique ID for this BundleSolutionInfo object (useful for others to reference this
      //! object when saving to disk).
      QUuid              *m_id;
      QString             m_name;                        //!< Name of the bundle. Defaults to the id
      QString             m_runTime;                     //!< Run time of the bundle adjustment
      FileName           *m_inputControlNetFileName;     //!< Input control network file name
      Control            *m_outputControl;               //!< Output control
      QString             m_outputControlName;
      FileName           *m_inputLidarDataFileName;      //!< Input lidar data file name
      LidarData          *m_outputLidarDataSet;          //!< QList of adjusted lidar points
      BundleSettingsQsp   m_settings;                    //!< Bundle settings
      BundleResults      *m_statisticsResults;           //!< Bundle statistical results
      QList<ImageList *> *m_images;                      //!< Input image list
      QList<ImageList *> *m_adjustedImages;              //!< Adjusted image list

      // In theory the path in the BundleSettings can change while running. So we save the
      // filenames actually used when the most recent save of the file was done.
      QString m_txtBundleOutputFilename;
      QString m_csvSavedImagesFilename;
      QString m_csvSavedPointsFilename;
      QString m_csvSavedResidualsFilename;

      // BundleSolutionInfo *m_xmlHandlerBundleSolutionInfo; //!< The bundleSolutionInfo object
      Project *m_xmlHandlerProject;                       // TODO does xml stuff need project???
      QString m_xmlHandlerCharacters;                     //!< List of characters that have been handled

  }; // end BundleSolutionInfo class

  void setStringAttribute(int locationId, QString locationName,
                          QString attributeName, QString attributeValue);
  QString getStringAttribute(int locationId, QString locationName, QString attributeName);
}; // end namespace Isis

Q_DECLARE_METATYPE(Isis::BundleSolutionInfo *);

#endif // BundleSolutionInfo_h