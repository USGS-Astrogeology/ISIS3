#include "Shape.h"

#include <QBuffer>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QMutexLocker>
#include <QScopedPointer>
#include <QString>
#include <QUuid>
#include <QXmlStreamWriter>

#include <geos/geom/MultiPolygon.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>

#include "Angle.h"
#include "CameraFactory.h"
#include "ControlPoint.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "DisplayProperties.h"
#include "Distance.h"
#include "FileName.h"
#include "IException.h"
#include "ImagePolygon.h"
#include "IString.h"
#include "PolygonTools.h"
#include "Project.h"
#include "ProjectionFactory.h"
#include "SerialNumber.h"
#include "ShapeDisplayProperties.h"
#include "Target.h"

namespace Isis {
  /**
   * Create an Shape from a cube file on disk.
   *
   * @param ShapeFileName The name of a cube on disk - /work/users/.../blah.cub
   * @param parent The Qt-relationship parent
   */
  Shape::Shape(QString imageFileName, QObject *parent) : QObject(parent) {

    m_fileName = imageFileName;

    initMemberData();
    cube();
    initShape();
  }


  /**
   * Create an shape from a cube file on disk.
   *
   * @param shapeFileName The name of a cube on disk - /work/users/.../blah.cub
   * @param parent The Qt-relationship parent
   */
  Shape::Shape(Cube *shapeCube, QObject *parent) : QObject(parent) {

    m_fileName = shapeCube->fileName();

    initMemberData();
    m_cube = shapeCube;
    initShape();
  }


  /**
   * Clean up this shape. If you haven't saved this shape, all of its settings will be lost.
   */
  Shape::~Shape() {
    delete m_bodyCode;
    m_bodyCode = NULL;

    delete m_cube;
    m_cube = NULL;

    delete m_footprint;
    m_footprint = NULL;

    delete m_id;
    m_id = NULL;

    //  Shape is a "Qt" parent of display properties, so the Shape QObject
    //    destructor will take care of deleting the display props. See call to
    //    DisplayProperties' constructor.
    m_displayProperties = NULL;
  }


  void Shape::initMemberData() {

    m_bodyCode = NULL;
    m_cube = NULL;
    m_displayProperties = NULL;
    m_footprint = NULL;
    m_id = NULL;

    m_aspectRatio = Null;
    m_resolution = Null;
    m_lineResolution = Null;
    m_sampleResolution = Null;

    m_targetName = Null;
    m_projectionName = Null;
    m_pixelResolution = Null;
    m_scale = Null;
  }


  void Shape::initShape() {

    m_displayProperties = new ShapeDisplayProperties(FileName(m_fileName).name(), this);
    m_id = new QUuid(QUuid::createUuid());
    m_serialNumber = SerialNumber::Compose(m_fileName, true);

    m_radiusSource = ControlPoint::RadiusSource::None;

    if (cube()->hasTable("ShapeModelStatistics")) {
      m_surfacePointSource = ControlPoint::SurfacePointSource::Basemap;
      m_radiusSource = ControlPoint::RadiusSource::DEM;
      m_shapeType = Dem;
    }
    // Is this a level 1 or level 2?
    else {
      try {
        ProjectionFactory::CreateFromCube(*(cube()));
        m_surfacePointSource = ControlPoint::SurfacePointSource::Basemap;
        m_radiusSource = ControlPoint::RadiusSource::Ellipsoid;
        m_shapeType = Basemap;
      }
      catch (IException &) {
        // TODO  Determine if unprojected shape has been bundle adjusted. Otherwise, ??
        try {
          CameraFactory::Create(*(cube()));
          m_surfacePointSource = ControlPoint::SurfacePointSource::Reference;

          PvlGroup kernels = cube()->group("Kernels");
          if (kernels.hasKeyword("ShapeModel")) {
            QString shapeFile = kernels["ShapeModel"];
            if (shapeFile.contains("dem")) {
              m_radiusSource = ControlPoint::RadiusSource::DEM;
            }
            else {
              m_radiusSource = ControlPoint::RadiusSource::Ellipsoid;
            }
          }
          m_shapeType = Unprojected;
        }
        catch (IException &e) {
          m_surfacePointSource = ControlPoint::SurfacePointSource::None;
          m_radiusSource = ControlPoint::RadiusSource::None;
          m_shapeType = Unknown;
          QString message = "Cannot create either Camera or Projections "
            "for the ground source file [" + displayProperties()->displayName() + "].  "
            "Check the validity of the  cube labels.  The cube must either be projected or "
            " run through spiceinit.";
          throw IException(e, IException::Io, message, _FILEINFO_);
        }
      }
    }

    try {
      if (m_shapeType == Unprojected) {
        initCamStats();
      }
      else if (m_shapeType == Basemap || m_shapeType == Dem) {
        initMapStats();
        if (m_shapeType == Dem) {
          initDemStats();
        }
      }
    }
    catch (IException &e) {
      QString message = "Cannot initialize the camera, map or dem statistics for this shape file [" +
          displayProperties()->displayName() + "]. Check the validity of the  cube labels.  The "
          "cube must either be projected or run through spiceinit. \n";
      message += e.toString();
      QMessageBox::warning((QWidget *) parent(), "Warning", message);
    }

    try {
      initQuickFootprint();
    }
    catch (IException &e) {

    }
  }


  ControlPoint::SurfacePointSource::Source Shape::surfacePointSource() {
    return m_surfacePointSource;
  }


  ControlPoint::RadiusSource::Source Shape::radiusSource() {
    return m_radiusSource;
  }


  Shape::ShapeType Shape::shapeType() {
    return m_shapeType;
  }


  /**
   * Read the shape settings from a Pvl.
   *
   * <pre>
   *   Object = Shape
   *     FileName = ...
   *     ID = ...
   *   EndObject
   * </pre>
   *
   * @param pvl The PvlObject that contains shape information.
   */
  void Shape::fromPvl(const PvlObject &pvl) {
    QString pvlFileName = ((IString)pvl["FileName"][0]).ToQt();
    if (m_fileName != pvlFileName) {
      throw IException(IException::Unknown,
          tr("Tried to load Shape [%1] with properties/information from [%2].")
            .arg(m_fileName).arg(pvlFileName),
          _FILEINFO_);
    }

    displayProperties()->fromPvl(pvl.findObject("DisplayProperties"));

    if (pvl.hasKeyword("ID")) {
      QByteArray hexValues(pvl["ID"][0].toLatin1());
      QDataStream valuesStream(QByteArray::fromHex(hexValues));
      valuesStream >> *m_id;
    }
  }


  /**
   * Convert this Shape to PVL.
   *
   * The output looks like this:
   * <pre>
   *   Object = Shape
   *     FileName = ...
   *     ID = ...
   *   EndObject
   * </pre>
   *
   * @return A PvlObject that contains shape information.
   */
  PvlObject Shape::toPvl() const {
    PvlObject output("Shape");

    output += PvlKeyword("FileName", m_fileName);

    // Do m_id
    QBuffer dataBuffer;
    dataBuffer.open(QIODevice::ReadWrite);

    QDataStream idStream(&dataBuffer);
    idStream << *m_id;

    dataBuffer.seek(0);

    output += PvlKeyword("ID", QString(dataBuffer.data().toHex()));

    output += displayProperties()->toPvl();

    return output;
  }


  /**
   * Test to see if it's possible to create a footprint from this shape. This may not give an
   *   accurate answer if the cube isn't open.
   */
  bool Shape::isFootprintable() const {
    bool result = false;

    if (m_footprint)
      result = true;

    if (!result && m_cube) {
      Blob example = ImagePolygon().toBlob();

      QString blobType = example.Type();
      QString blobName = example.Name();

      Pvl &labels = *m_cube->label();

      for (int i = 0; i < labels.objects(); i++) {
        PvlObject &obj = labels.object(i);

        if (obj.isNamed(blobType) && obj.hasKeyword("Name") && obj["Name"][0] == blobName)
          result = true;
      }
    }

    return result;
  }


  /**
   * Get the Cube * associated with this display property. This will allocate
   *   the Cube * if one is not already present.
   */
  Cube *Shape::cube() {
    if (!m_cube) {
      try {
        m_cube = new Cube(m_fileName);
      }
      catch (IException &e) {
        throw IException(e, IException::Programmer, "Cube cannot be created", _FILEINFO_);
      }
    }

    return m_cube;
  }


  /**
   * Cleans up the Cube *. You want to call this once you're sure you are done
   *   with the Cube because the OS will limit how many of these we have open.
   */
  void Shape::closeCube() {
    if (m_cube) {
      delete m_cube;
      m_cube = NULL;
    }
  }


  /**
   * Get the display (GUI) properties (information) associated with this shape.
   *
   * @return An ShapeDisplayProperties that describes how to view this shape.
   */
  ShapeDisplayProperties *Shape::displayProperties() {
    return m_displayProperties;
  }


  /**
   * Get a non-mutable (const) the display (GUI) properties (information) associated with this
   *   shape.
   *
   * @return A non-mutable ShapeDisplayProperties that describes how to view this shape.
   */
  const ShapeDisplayProperties *Shape::displayProperties() const {
    return m_displayProperties;
  }


  /**
   * Get the file name of the cube that this shape represents.
   *
   * @return A string containing the path to the cube data associated with this shape.
   */
  QString Shape::fileName() const {
    return m_fileName;
  }


  /**
   * Get the serial number.
   * @return SerialNumber The cube's serial number.
   */
  QString Shape::serialNumber() {
    return m_serialNumber;
  }

  /**
   * Get the footprint of this shape (if available).
   *
   * @return A lat/lon footprint of this shape, or NULL if unavailable.
   */
  geos::geom::MultiPolygon *Shape::footprint() {
    return m_footprint;
  }


  /**
   * Override the automatically generated ID with the given ID.
   */
  void Shape::setId(QString id) {
    *m_id = QUuid(QString("{%1}").arg(id));
  }


  /**
   * Get the non-mutable (const) footprint of this shape (if available).
   *
   * @return A non-mutable (const) lat/lon footprint of this shape, or NULL if unavailable.
   */
  const geos::geom::MultiPolygon *Shape::footprint() const {
    return m_footprint;
  }


  /**
   * Calculate a footprint for this shape. If the footprint is already stored inside the cube, that
   *   will be used instead. If no footprint can be found, this throws an exception.
   */
  bool Shape::initFootprint(QMutex *cameraMutex) {
    if (!m_footprint) {
      try {
        initQuickFootprint();
      }
      catch (IException &e) {
        try {
          m_footprint = createFootprint(cameraMutex);
        }
        catch(IException &e) {
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
   * Get the aspect ratio of this shape, as calculated and attached by camstats.
   *
   * @return The aspect ratio if available, otherwise Null
   */
  double Shape::aspectRatio() const {
    return m_aspectRatio;
  }


  /**
   * Get a unique, identifying string associated with this shape.
   *
   * @return A unique ID for this shape
   */
  QString Shape::id() const {
    return m_id->toString().remove(QRegExp("[{}]"));
  }


  /**
   * Get the resolution of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The resolution if available, otherwise Null
   */
  double Shape::resolution() const {
    return m_resolution;
  }


  /**
   * Get the emission angle of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The emission angle if available, otherwise an invalid angle
   */
  Angle Shape::emissionAngle() const {
    return m_emissionAngle;
  }


  /**
   * Get the incidence angle of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The incidence angle if available, otherwise an invalid angle
   */
  Angle Shape::incidenceAngle() const {
    return m_incidenceAngle;
  }


  /**
   * Get the line resolution of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The line resolution if available, otherwise Null
   */
  double Shape::lineResolution() const {
    return m_lineResolution;
  }


  /**
   * Get the local radius of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The local radius if available, otherwise an invalid Distance
   */
  Distance Shape::localRadius() const {
    return m_localRadius;
  }


  /**
   * Get the north azimuth of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The north azimuth if available, otherwise an invalid angle
   */
  Angle Shape::northAzimuth() const {
    return m_northAzimuth;
  }


  /**
   * Get the phase angle of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The phase angle if available, otherwise an invalid angle
   */
  Angle Shape::phaseAngle() const {
    return m_phaseAngle;
  }


  /**
   * Get the sample resolution of this shape, as calculated and attached by camstats. This is the
   *   shape-wide average.
   *
   * @return The sample resolution if available, otherwise Null
   */
  double Shape::sampleResolution() const {
    return m_sampleResolution;
  }


  /**
   * Copy the cub/ecub files associated with this shape into the new project.
   */
  void Shape::copyToNewProjectRoot(const Project *project, FileName newProjectRoot) {
    if (FileName(newProjectRoot) != FileName(project->projectRoot())) {
      Cube origShape(m_fileName);

      FileName newExternalLabelFileName(Project::shapeDataRoot(newProjectRoot.toString()) + "/" +
          FileName(m_fileName).dir().dirName() + "/" + FileName(m_fileName).name());

      QScopedPointer<Cube> newExternalLabel(
          origShape.copy(newExternalLabelFileName, CubeAttributeOutput("+External")));

      // If this is an ecub (it should be) and is pointing to a relative file name,
      //   then we want to copy the DN cube also.
      if (!origShape.storesDnData()) {
        if (origShape.externalCubeFileName().path() == ".") {
          Cube dnFile(
              FileName(m_fileName).path() + "/" + origShape.externalCubeFileName().name());

          FileName newDnFileName = newExternalLabelFileName.setExtension("cub");

          QScopedPointer<Cube> newDnFile(dnFile.copy(newDnFileName, CubeAttributeOutput()));
          newDnFile->close();

          newExternalLabel->relocateDnData(newDnFileName.name());
        }
        else {
          newExternalLabel->relocateDnData(origShape.externalCubeFileName());
        }
      }
    }
  }


  /**
   * Delete the shape data from disk. The cube() will no longer be accessible until you call
   *   updateFileName().
   */
  void Shape::deleteFromDisk() {
    bool deleteCubAlso = (cube()->externalCubeFileName().path() == ".");
    closeCube();

    if (!QFile::remove(m_fileName)) {
      throw IException(IException::Io,
                       tr("Could not remove file [%1]").arg(m_fileName),
                       _FILEINFO_);
    }

    if (deleteCubAlso) {
      FileName cubFile = FileName(m_fileName).setExtension("cub");
      if (!QFile::remove(cubFile.expanded())) {
        throw IException(IException::Io,
                         tr("Could not remove file [%1]").arg(m_fileName),
                         _FILEINFO_);
      }
    }

    // If we're the last thing in the folder, remove the folder too.
    QDir dir;
    dir.rmdir(FileName(m_fileName).path());
  }


  /**
   * Change the on-disk file name for this cube to be where the shape ought to be in the given
   *   project.
   *
   * @param project The project that this shape is stored in
   */
  void Shape::updateFileName(Project *project) {
    closeCube();

    FileName original(m_fileName);
    FileName newName(project->shapeDataRoot() + "/" +
                     original.dir().dirName() + "/" + original.name());
    m_fileName = newName.expanded();
  }


  /**
   * Calculate a footprint for an Shape using the camera or projection information.
   *
   * @param cameraMutex A mutex that guarantees us serial access to the camera/projection classes
   * @return The resulting footprint
   */
  geos::geom::MultiPolygon *Shape::createFootprint(QMutex *cameraMutex) {
    QMutexLocker lock(cameraMutex);

    // We need to walk the shape to create the polygon...
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
   * TODO
   */
  void Shape::initCamStats() {
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
      catch (IException &e) {
        e.print();
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
            m_spacecraftName = obj.findGroup("Instrument")["SpacecraftName"][0];

          if (instGroup.hasKeyword("InstrumentId"))
            m_instrumentId = obj.findGroup("Instrument")["InstrumentId"][0];
        }
      }
      catch (IException &e) {
        e.print();
      }
    }
  }


  void Shape::initMapStats() {

    Pvl &label = *cube()->label();
    for (int i = 0; i < label.objects(); i++) {
      PvlObject &obj = label.object(i);
      try {
        if (obj.hasGroup("Instrument")) {
          PvlGroup instGroup = obj.findGroup("Instrument");

          if (instGroup.hasKeyword("SpacecraftName"))
            m_spacecraftName = obj.findGroup("Instrument")["SpacecraftName"][0];

          if (instGroup.hasKeyword("InstrumentId"))
            m_instrumentId = obj.findGroup("Instrument")["InstrumentId"][0];
        }

        if (obj.hasGroup("Mapping")) {
          PvlGroup mapGroup = obj.findGroup("Mapping");

          if (mapGroup.hasKeyword("TargetName"))
            m_targetName = obj.findGroup("Mapping")["TargetName"][0];

          if (mapGroup.hasKeyword("ProjectionName"))
            m_projectionName = obj.findGroup("Mapping")["ProjectionName"][0];

          if (mapGroup.hasKeyword("CenterLongitude"))
            m_centerLongitude = Longitude(toDouble(obj.findGroup("Mapping")["CenterLongitude"][0]),
                                          mapGroup, Angle::Degrees);

          if (mapGroup.hasKeyword("CenterLatitude"))
            m_centerLatitude = Latitude(toDouble(obj.findGroup("Mapping")["CenterLatitude"][0]),
                                        mapGroup, Angle::Degrees);

          if (mapGroup.hasKeyword("MinimumLatitude"))
            m_minimumLatitude = Latitude(toDouble(obj.findGroup("Mapping")["MinimumLatitude"][0]),
                                         mapGroup, Angle::Degrees);

          if (mapGroup.hasKeyword("MaximumLatitude"))
            m_maximumLatitude = Latitude(toDouble(obj.findGroup("Mapping")["MaximumLatitude"][0]),
                                         mapGroup, Angle::Degrees);

          if (mapGroup.hasKeyword("MinimumLongitude"))
            m_minimumLongitude = Longitude(toDouble(obj.findGroup("Mapping")["MinimumLongitude"][0]),
                                           mapGroup, Angle::Degrees);

          if (mapGroup.hasKeyword("MaximumLongitude"))
            m_maximumLongitude = Longitude(toDouble(obj.findGroup("Mapping")["MaximumLongitude"][0]),
                                           mapGroup, Angle::Degrees);

          if (mapGroup.hasKeyword("PixelResolution"))
            m_pixelResolution = obj.findGroup("Mapping")["PixelResolution"];

          if (mapGroup.hasKeyword("Scale"))
            m_scale = obj.findGroup("Mapping")["Scale"];
        }
      }
      catch (IException &e) {
        e.print();
      }
    }
  }


  void Shape::initDemStats() {


  }


  void Shape::initQuickFootprint() {
    ImagePolygon poly = cube()->readFootprint();
    m_footprint = PolygonTools::MakeMultiPolygon(poly.Polys()->clone().release());
  }


  /**
   * Output format:
   *
   *
   * <shape id="..." fileName="...">
   *   ...
   * </shape>
   *
   * (fileName attribute is just the base name)
   */
  void Shape::save(QXmlStreamWriter &stream, const Project *project, FileName newProjectRoot)
      const {

    stream.writeStartElement("shape");

    stream.writeAttribute("id", m_id->toString());
    stream.writeAttribute("fileName", FileName(m_fileName).name());
    stream.writeAttribute("serialNumber", m_serialNumber);

    QString type;
    if (m_shapeType == Unprojected) {
      type = "Unprojected";
    }
    else if (m_shapeType == Basemap) {
      type = "Basemap";
    }
    else {
      type = "Dem";
    }
    stream.writeAttribute("shapeType", type);
    stream.writeAttribute("surfacePointSource",
               ControlPoint::SurfacePointSourceToString(m_surfacePointSource));
    stream.writeAttribute("radiusSource",
               ControlPoint::RadiusSourceToString(m_radiusSource));

    if (m_shapeType == Unprojected) {
      stream.writeAttribute("instrumentId", m_instrumentId);
      stream.writeAttribute("spacecraftName", m_spacecraftName);

      if (!IsSpecial(m_aspectRatio)) {
        stream.writeAttribute("aspectRatio", IString(m_aspectRatio).ToQt());
      }

      if (!IsSpecial(m_resolution)) {
        stream.writeAttribute("resolution", IString(m_resolution).ToQt());
      }

      if (m_emissionAngle.isValid()) {
        stream.writeAttribute("emissionAngle", IString(m_emissionAngle.radians()).ToQt());
      }

      if (m_incidenceAngle.isValid()) {
        stream.writeAttribute("incidenceAngle", IString(m_incidenceAngle.radians()).ToQt());
      }

      if (!IsSpecial(m_lineResolution)) {
        stream.writeAttribute("lineResolution", IString(m_lineResolution).ToQt());
      }

      if (m_localRadius.isValid()) {
        stream.writeAttribute("localRadius", IString(m_localRadius.meters()).ToQt());
      }

      if (m_northAzimuth.isValid()) {
        stream.writeAttribute("northAzimuth", IString(m_northAzimuth.radians()).ToQt());
      }

      if (m_phaseAngle.isValid()) {
        stream.writeAttribute("phaseAngle", IString(m_phaseAngle.radians()).ToQt());
      }

      if (!IsSpecial(m_sampleResolution)) {
        stream.writeAttribute("sampleResolution", IString(m_sampleResolution).ToQt());
      }
    }
    else if (m_shapeType == Basemap) {

    }
    else if (m_shapeType == Dem) {

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
}
