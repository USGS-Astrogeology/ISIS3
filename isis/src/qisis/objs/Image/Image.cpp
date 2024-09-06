#include "Image.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QScopedPointer>
#include <QString>
#include <QUuid>
#include <QXmlStreamWriter>

#include <geos/geom/MultiPolygon.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>

#include "Angle.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "DisplayProperties.h"
#include "Distance.h"
#include "FileName.h"
#include "ImageDisplayProperties.h"
#include "ImagePolygon.h"
#include "IString.h"
#include "ObservationNumber.h"
#include "PolygonTools.h"
#include "Project.h"
#include "SerialNumber.h"
#include "Target.h"

namespace Isis {

  /**
   * @brief Create an image from a cube file on disk.
   * @param imageFileName The name of a cube on disk - /work/users/.../blah.cub
   * @param parent The Qt-relationship parent
   */
  Image::Image(QString imageFileName, QObject *parent) : QObject(parent) {
    m_bodyCode = NULL;
    m_cube = NULL;
    m_displayProperties = NULL;
    m_footprint = NULL;
    m_id = NULL;

    m_aspectRatio = Null;
    m_resolution = Null;
    m_lineResolution = Null;
    m_sampleResolution = Null;

    m_fileName = imageFileName;

    cube();

    initCamStats();

    try {
      initQuickFootprint();
    }
    catch (IException &) {
    }

    m_displayProperties = new ImageDisplayProperties(QString::fromStdString(FileName(m_fileName.toStdString()).name()), this);

    m_id = new QUuid(QUuid::createUuid());
  }


  /**
   * @brief Create an image from a cube file on disk.
   * @param imageFileName The name of a cube on disk - /work/users/.../blah.cub
   * @param parent The Qt-relationship parent
   */
  Image::Image(Cube *imageCube, QObject *parent) : QObject(parent) {
    m_fileName = imageCube->fileName();

    m_bodyCode = NULL;
    m_cube = imageCube;
    m_displayProperties = NULL;
    m_footprint = NULL;
    m_id = NULL;

    m_aspectRatio = Null;
    m_resolution = Null;
    m_lineResolution = Null;
    m_sampleResolution = Null;

    initCamStats();

    try {
      initQuickFootprint();
    }
    catch (IException &e) {
    }

    m_displayProperties = new ImageDisplayProperties(QString::fromStdString(FileName(m_fileName.toStdString()).name()), this);

    m_id = new QUuid(QUuid::createUuid());
  }


  /**
   * @brief Create an image from a cube file on disk including the footprint
   * @param imageFileName The name of a cube on disk - /work/users/.../blah.cub
   * @param footprint The calculated footprint
   * @param parent The Qt-relationship parent
   */
  Image::Image(Cube *imageCube, geos::geom::MultiPolygon *footprint, QString id, QObject *parent) :
      QObject(parent) {
    m_fileName = imageCube->fileName();

    m_bodyCode = NULL;
    m_cube = imageCube;
    m_displayProperties = NULL;
    m_id = NULL;

    m_aspectRatio = Null;
    m_resolution = Null;
    m_lineResolution = Null;
    m_sampleResolution = Null;

    initCamStats();

    m_footprint = footprint;

    m_displayProperties = new ImageDisplayProperties(QString::fromStdString(FileName(m_fileName.toStdString()).name()), this);

    setId(id);
  }


  /**
   * @brief Clean up this image. If you haven't saved this image, all of its settings will be lost.
   */
  Image::~Image() {
    delete m_bodyCode;
    m_bodyCode = NULL;

    delete m_cube;
    m_cube = NULL;

    delete m_footprint;
    m_footprint = NULL;

    delete m_id;
    m_id = NULL;

    //  Image is a "Qt" parent of display properties, so the Image QObject
    //  destructor will take care of deleting the display props. See call to
    //  DisplayProperties' constructor.
    m_displayProperties = NULL;
  }


  /**
   * @brief Read the image settings from a Pvl.  The Pvl file looks like this:
   *
   * <pre>
   *   Object = Image
   *     FileName = ...
   *     ID = ...
   *   EndObject
   * </pre>
   *
   * @param pvl The PvlObject that contains image information.
   * @throws IException::Unknown "Tried to load Image with properties/information"
   */
  void Image::fromPvl(const PvlObject &pvl) {
    QString pvlFileName = ((IString)pvl["FileName"][0]).ToQt();
    if (m_fileName != pvlFileName) {
      throw IException(IException::Unknown,
          tr("Tried to load Image [%1] with properties/information from [%2].")
            .arg(m_fileName).arg(pvlFileName),
          _FILEINFO_);
    }

    displayProperties()->fromPvl(pvl.findObject("DisplayProperties"));

    if (pvl.hasKeyword("ID")) {
      QByteArray hexValues(QString::fromStdString(pvl["ID"][0]).toLatin1());
      QDataStream valuesStream(QByteArray::fromHex(hexValues));
      valuesStream >> *m_id;
    }
  }


  /**
   * @brief Convert this Image to PVL.
   *
   * The output looks like this:
   * <pre>
   *   Object = Image
   *     FileName = ...
   *     ID = ...
   *   EndObject
   * </pre>
   *
   * @return @b PvlObject A PvlObject that contains image information.
   */
  PvlObject Image::toPvl() const {
    PvlObject output("Image");

    output += PvlKeyword("FileName", m_fileName.toStdString());

    // Do m_id
    QBuffer dataBuffer;
    dataBuffer.open(QIODevice::ReadWrite);

    QDataStream idStream(&dataBuffer);
    idStream << *m_id;

    dataBuffer.seek(0);

    output += PvlKeyword("ID", std::string(dataBuffer.data().toHex()));

    output += displayProperties()->toPvl();

    return output;
  }


  /**
   * @brief Test to see if it's possible to create a footprint from this image.
   * This may not give an accurate answer if the cube isn't open.
   * @return @b bool Returns True if it is possible, False if it is not.
   */
  bool Image::isFootprintable() const {
    bool result = false;

    if (m_footprint) {
      result = true;
    }

    if (!result && m_cube) {
      Blob example = ImagePolygon().toBlob();

      QString blobType = QString::fromStdString(example.Type());
      QString blobName = example.Name();

      Pvl &labels = *m_cube->label();

      for (int i = 0; i < labels.objects(); i++) {
        PvlObject &obj = labels.object(i);

        if (obj.isNamed(blobType.toStdString()) && obj.hasKeyword("Name") && QString::fromStdString(obj["Name"][0]) == blobName)
          result = true;
      }
    }

    return result;
  }


  /**
   * @brief Get the Cube pointer associated with this display property.
   *
   * This will allocate the Cube pointer if one is not already present.
   * @throws IException::Programmer "Cube cannot be created"
   * @return @b (Cube *) A pointer to the image cube.
   */
  Cube *Image::cube() {
    if (!m_cube) {
      try {
        m_cube = new Cube(m_fileName.toStdString());
      }
      catch (IException &e) {
        throw IException(e, IException::Programmer, "Cube cannot be created", _FILEINFO_);
      }
    }

    return m_cube;
  }


  /**
   * @brief Cleans up the Cube pointer.
   *
   * You want to call this once you are sure you are done
   * with the Cube because the OS will limit how many of these we have open.
   */
  void Image::closeCube() {
    if (m_cube) {
      delete m_cube;
      m_cube = NULL;
    }
  }


  /**
   * @brief Get the display (GUI) properties (information) associated with this image.
   * @return @b (ImageDisplayProperties *) Returns a poniter to an ImageDisplayProperties
   * that describes how to view this image.
   */
  ImageDisplayProperties *Image::displayProperties() {
    return m_displayProperties;
  }


  /**
   * @brief Get a non-mutable (const) the display (GUI) properties (information)
   * associated with this image.
   * @return @b (ImageDisplayProperties *) A pointer to a non-mutable ImageDisplayProperties
   * object that describes how to view this image.
   */
  const ImageDisplayProperties *Image::displayProperties() const {
    return m_displayProperties;
  }


  /**
   * @brief Get the file name of the cube that this image represents.
   * @return @b QString A string containing the path to the cube data associated with this image.
   */
  QString Image::fileName() const {
    return m_fileName;
  }


  /**
   * @brief Returns the observation number of the Cube
   * @return QString A string representation of the observation number of the cube.
   */
  QString Image::observationNumber() {
    if (m_observationNumber.isEmpty()) {
      m_observationNumber = ObservationNumber::Compose(*(cube()));
    }
    return m_observationNumber;
  }


  /**
   * @brief Returns the serial number of the Cube
   * @return @b QString  A string representation of the serial number of the cube.
   */
  QString Image::serialNumber() {
    if (m_serialNumber.isEmpty()) {
      m_serialNumber = SerialNumber::Compose(*(cube()));
    }
    return m_serialNumber;
  }


  /**
   * @brief Get the footprint of this image (if available).
   * @return @b (geos::geom::MultiPolygon *) A pointer to a lat/lon footprint of this image,
   * or NULL if unavailable.
   */
  geos::geom::MultiPolygon *Image::footprint() {
    return m_footprint;
  }


  /**
   * @brief Override the automatically generated ID with the given ID.
   * @param id The id tjat overrides the automatically generated id.
   */
  void Image::setId(QString id) {
    m_id = new QUuid(id);
  }


  /**
   * @brief Get the non-mutable (const) footprint of this image (if available).
   * @return @b geos::geom::MultiPolygon A non-mutable (const) lat/lon footprint of this image,
   * or NULL if unavailable.
   */
  const geos::geom::MultiPolygon *Image::footprint() const {
    return m_footprint;
  }


  /**
   * @brief Calculate a footprint for this image.
   *
   * If the footprint is already stored inside the cube, that will be used instead.
   * If no footprint can be found, this throws an exception.
   * @param cameraMutex A pointer to the camera mutex to lock the camera resource while a footprint
   * is created.
   * @throws IException::Io  "Could not read the footprint from cube [$cube].  Please make sure
   * footprintinit has been run"
   * @return @b bool Returns True if there is a footprint stored in the Cube, False otherwise.
   */
  bool Image::initFootprint(QMutex *cameraMutex) {
    if (!m_footprint) {
      try {
        initQuickFootprint();
      }
      catch (IException &e) {
        try {
          m_footprint = createFootprint(cameraMutex);
        }
        catch (IException &e) {
          IString msg = "Could not read the footprint from cube [" +
              displayProperties()->displayName() + "]. Please make "
              "sure footprintinit has been run";
          throw IException(e, IException::Io, msg, _FILEINFO_);
        }
      }
    }

    // I'm not sure how this could ever be NULL. -SL
    return (m_footprint != NULL);
  }


  /**
   * @brief Get the aspect ratio of this image, as calculated and attached by camstats.
   * @return @b double The aspect ratio if available, otherwise Null is returned.
   */
  double Image::aspectRatio() const {
    return m_aspectRatio;
  }


  /**
   * @brief Get a unique, identifying string associated with this image.
   * @return @b QString A unique ID for this image.
   */
  QString Image::id() const {
    return m_id->toString().remove(QRegExp("[{}]"));
  }


  /**
   * @brief Get the resolution of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b double The resolution if available, otherwise Null is returned.
   */
  double Image::resolution() const {
    return m_resolution;
  }


  /**
   * @brief Get the emission angle of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b Angle The emission angle if available, otherwise an invalid angle is returned.
   */
  Angle Image::emissionAngle() const {
    return m_emissionAngle;
  }


  /**
   * @brief Get the incidence angle of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b Angle The incidence angle if available, otherwise an invalid angle is returned.
   */
  Angle Image::incidenceAngle() const {
    return m_incidenceAngle;
  }


  /**
   * @brief Get the line resolution of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b double The line resolution if available, otherwise Null.
   */
  double Image::lineResolution() const {
    return m_lineResolution;
  }


  /**
   * @brief Get the local radius of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b The local radius if available, otherwise an invalid Distance.
   */
  Distance Image::localRadius() const {
    return m_localRadius;
  }


  /**
   * @brief Get the north azimuth of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b Angle The north azimuth if available, otherwise an invalid angle
   */
  Angle Image::northAzimuth() const {
    return m_northAzimuth;
  }


  /**
   * @brief Get the phase angle of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b Angle The phase angle if available, otherwise an invalid angle is returned.
   */
  Angle Image::phaseAngle() const {
    return m_phaseAngle;
  }


  /**
   * @brief Get the sample resolution of this image, as calculated and attached by camstats.
   * This is the image-wide average.
   * @return @b double The sample resolution if available, otherwise Null is returned.
   */
  double Image::sampleResolution() const {
    return m_sampleResolution;
  }


  /**
   * @brief Copy the cub/ecub files associated with this image into the new project.
   * @param newProjectRoot  The root directory where the project is stored.
   */
  void Image::copyToNewProjectRoot(const Project *project, FileName newProjectRoot) {
    if (FileName(newProjectRoot) != FileName(project->projectRoot().toStdString())) {
      Cube origImage(m_fileName.toStdString());

      // The imageDataRoot will either be PROJECTROOT/images or PROJECTROOT/results/bundle/timestamp/images,
      // depending on how the newProjectRoot points to.
      FileName newExternalLabelFileName(Project::imageDataRoot(QString::fromStdString(newProjectRoot.toString())).toStdString() + "/" +
          FileName(m_fileName.toStdString()).dir().filename().string() + "/" + FileName(m_fileName.toStdString()).name());

      if (m_fileName != QString::fromStdString(newExternalLabelFileName.toString())) {
        // This cube copy creates a filename w/ecub extension in the new project root, but looks to
        // be a cube(internal vs external). It changes the DnFile pointer to the old ecub,
        // /tmp/tsucharski_ipce/tmpProject/images/import1/AS15-.ecub, but doing a less on file
        // immediately after the following call indicates it is a binary file.
        QScopedPointer<Cube> newExternalLabel(
            origImage.copy(newExternalLabelFileName, CubeAttributeOutput("+External")));

        // If this is an ecub (it should be) and is pointing to a relative file name,
        //   then we want to copy the DN cube also.
        if (!origImage.storesDnData() ) {
          if (origImage.externalCubeFileName().path() == ".") {
            Cube dnFile(
                FileName(m_fileName.toStdString()).path() + "/" + origImage.externalCubeFileName().name());
            FileName newDnFileName = newExternalLabelFileName.setExtension("cub");
            QScopedPointer<Cube> newDnFile(dnFile.copy(newDnFileName, CubeAttributeOutput()));
            newDnFile->close();
            // Changes the ecube's DnFile pointer in the labels.
            newExternalLabel->relocateDnData(newDnFileName.name());
          }
          else {
            //  If the the ecub's external cube is pointing to the old project root, update to new
            //  project root.
            if (QString::fromStdString(origImage.externalCubeFileName().toString()).contains(project->projectRoot())) {
              QString newExternalCubeFileName = QString::fromStdString(origImage.externalCubeFileName().toString());
              newExternalCubeFileName.replace(project->projectRoot(), project->newProjectRoot());
              newExternalLabel->relocateDnData(newExternalCubeFileName.toStdString());
            }
            else {
              newExternalLabel->relocateDnData(origImage.externalCubeFileName());
            }
          }
        }
      }
    }
  }


  /**
   * @brief Delete the image data from disk. The cube() will no longer be accessible
   * until you call updateFileName().
   * @throws IException::Io "Could not remove file [$filename]"
   */
  void Image::deleteFromDisk() {
    bool deleteCubAlso = (cube()->externalCubeFileName().path() == ".");
    closeCube();

    if (!QFile::remove(m_fileName)) {
      throw IException(IException::Io,
                       tr("Could not remove file [%1]").arg(m_fileName),
                       _FILEINFO_);
    }

    if (deleteCubAlso) {
      FileName cubFile = FileName(m_fileName.toStdString()).setExtension("cub");
      if (!QFile::remove(QString::fromStdString(cubFile.expanded()) ) ) {
        throw IException(IException::Io,
                         tr("Could not remove file [%1]").arg(m_fileName),
                         _FILEINFO_);
      }
    }

    // If we're the last thing in the folder, remove the folder too.
    QDir dir;
    dir.rmdir(QString::fromStdString(FileName(m_fileName.toStdString()).path()));
  }


  /**
   * @brief Write the Image properties out to an XML file.
   * @param stream The output data stream.
   * @param project The project this image is contained within.
   * @param newProjectRoot The path/filename we are writing to.
   *
   * Output format:
   *
   *
   * <image id="..." fileName="...">
   *   ...
   * </image>
   *
   * (fileName attribute is just the base name)
   */
  void Image::save(QXmlStreamWriter &stream, const Project *project, FileName newProjectRoot)
      const {
    stream.writeStartElement("image");

    stream.writeAttribute("id", m_id->toString());
    stream.writeAttribute("fileName", QString::fromStdString(FileName(m_fileName.toStdString()).name()));
    stream.writeAttribute("instrumentId", m_instrumentId);
    stream.writeAttribute("spacecraftName", m_spacecraftName);

    if (!IsSpecial(m_aspectRatio) ) {
      stream.writeAttribute("aspectRatio", IString(m_aspectRatio).ToQt());
    }

    if (!IsSpecial(m_resolution) ) {
      stream.writeAttribute("resolution", IString(m_resolution).ToQt());
    }

    if (m_emissionAngle.isValid() ) {
      stream.writeAttribute("emissionAngle", IString(m_emissionAngle.radians()).ToQt());
    }

    if (m_incidenceAngle.isValid() ) {
      stream.writeAttribute("incidenceAngle", IString(m_incidenceAngle.radians()).ToQt());
    }

    if (!IsSpecial(m_lineResolution) ) {
      stream.writeAttribute("lineResolution", IString(m_lineResolution).ToQt());
    }

    if (m_localRadius.isValid() ) {
      stream.writeAttribute("localRadius", IString(m_localRadius.meters()).ToQt());
    }

    if (m_northAzimuth.isValid() ) {
      stream.writeAttribute("northAzimuth", IString(m_northAzimuth.radians()).ToQt());
    }

    if (m_phaseAngle.isValid() ) {
      stream.writeAttribute("phaseAngle", IString(m_phaseAngle.radians()).ToQt());
    }

    if (!IsSpecial(m_sampleResolution) ) {
      stream.writeAttribute("sampleResolution", IString(m_sampleResolution).ToQt());
    }

    if (m_footprint) {
      stream.writeStartElement("footprint");

      geos::io::WKTWriter wktWriter;
      stream.writeCharacters(QString::fromStdString(wktWriter.write(m_footprint)));

      stream.writeEndElement();
    }

    m_displayProperties->save(stream, project, newProjectRoot);

    stream.writeEndElement();
  }


  /**
   * @brief Change the on-disk file name for this cube to be where the image ought to be in
   * the given project.
   * @param project The project that this image is stored in.
   */
  void Image::updateFileName(Project *project) {
    closeCube();

    FileName original(m_fileName.toStdString());
    FileName newName(project->imageDataRoot().toStdString() + "/" +
                     original.dir().filename().string() + "/" + original.name());
    m_fileName = QString::fromStdString(newName.expanded());
  }


  /**
   * @brief  Calculates a footprint for an Image using the camera or projection information.
   * @param cameraMutex A mutex that guarantees us serial access to the camera/projection classes
   * @return @b (geos::geom::MultiPolygon *) The resulting footprint.
   */
  geos::geom::MultiPolygon *Image::createFootprint(QMutex *cameraMutex) {
    QMutexLocker lock(cameraMutex);

    // We need to walk the image to create the polygon...
    ImagePolygon imgPoly;

    int sampleStepSize = cube()->sampleCount() / 10;
    if (sampleStepSize <= 0) sampleStepSize = 1;

    int lineStepSize = cube()->lineCount() / 10;
    if (lineStepSize <= 0) lineStepSize = 1;

    imgPoly.Create(*cube(), sampleStepSize, lineStepSize);

    IException e = IException(IException::User,
        tr("Warning: Polygon re-calculated for [%1] which can be very slow")
          .arg(displayProperties()->displayName()),
        _FILEINFO_);
    e.print();

    return PolygonTools::MakeMultiPolygon(imgPoly.Polys()->clone().release());
  }


  /**
   * @brief Checks to see if the Cube label contains Camera Statistics.  If it does,
   * then we attempt to grab data from the label to populate the private members variables.
   */
  void Image::initCamStats() {
    bool hasCamStats = false;

    Pvl &label = *cube()->label();
    for (int i = 0; !hasCamStats && i < label.objects(); i++) {
      PvlObject &obj = label.object(i);

      try {
        if (obj.name() == "Table") {
          if (obj["Name"][0] == "CameraStatistics") {
            hasCamStats = true;
          }
        }
      }
      catch (IException &) {
      }
    }

    if (hasCamStats) {
      Table camStatsTable("CameraStatistics", m_fileName, label);

      int numRecords = camStatsTable.Records();
      for (int recordIndex = 0; recordIndex < numRecords; recordIndex++) {
        TableRecord &record = camStatsTable[recordIndex];

        // The TableField class gives us a std::string with NULL (\0) characters... be careful not
        //   to keep them when going to QString.
        std::string recordName(record["Name"]);
        double avgValue = (double)record["Average"];

        if (recordName == "AspectRatio") {
          m_aspectRatio = avgValue;
        }
        else if (recordName == "Resolution") {
          m_resolution = avgValue;
        }
        else if (recordName == "EmissionAngle") {
          m_emissionAngle = Angle(avgValue, Angle::Degrees);
        }
        else if (recordName == "IncidenceAngle") {
          m_incidenceAngle = Angle(avgValue, Angle::Degrees);
        }
        else if (recordName == "LineResolution") {
          m_lineResolution = avgValue;
        }
        else if (recordName == "LocalRadius") {
          m_localRadius = Distance(avgValue, Distance::Meters);
        }
        else if (recordName == "NorthAzimuth") {
          m_northAzimuth = Angle(avgValue, Angle::Degrees);
        }
        else if (recordName == "PhaseAngle") {
          m_phaseAngle = Angle(avgValue, Angle::Degrees);
        }
        else if (recordName == "SampleResolution") {
          m_sampleResolution = avgValue;
        }
      }
    }

    for (int i = 0; i < label.objects(); i++) {
      PvlObject &obj = label.object(i);
      try {
        if (obj.hasGroup("Instrument")) {
          PvlGroup instGroup = obj.findGroup("Instrument");

          if (instGroup.hasKeyword("SpacecraftName"))
            m_spacecraftName = QString::fromStdString(obj.findGroup("Instrument")["SpacecraftName"][0]);

          if (instGroup.hasKeyword("InstrumentId"))
            m_instrumentId = QString::fromStdString(obj.findGroup("Instrument")["InstrumentId"][0]);
        }
      }
      catch (IException &) {
      }
    }
  }


  /**
   * @brief Creates a default ImagePolygon option which is read into the Cube.
   * @see Isis::ImagePolygon
   */
  void Image::initQuickFootprint() {
    ImagePolygon poly = cube()->readFootprint();
    m_footprint = PolygonTools::MakeMultiPolygon(poly.Polys()->clone().release());
  }
}
