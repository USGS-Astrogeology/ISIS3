/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "EmbreeTargetShape.h"

#include <iostream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "NaifDskApi.h"

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "Pvl.h"

namespace Isis {

  /**
   * Default constructor for RTCMultiHitRay.
   */
  RTCMultiHitRay::RTCMultiHitRay() {
    ray.tnear   = 0.0;
    ray.tfar    = std::numeric_limits<float>::infinity(); // Should be INF
    ray.mask    = 0xFFFFFFFF;
    lastHit = -1;
    hit.u       = 0.0;
    hit.v       = 0.0;
    hit.geomID  = RTC_INVALID_GEOMETRY_ID;
    hit.primID  = RTC_INVALID_GEOMETRY_ID;
    hit.instID[0]  = RTC_INVALID_GEOMETRY_ID;
    ray.org_x = 0.0;
    ray.org_y = 0.0;
    ray.org_z = 0.0;
    ray.dir_x = 0.0;
    ray.dir_y = 0.0;
    ray.dir_z = 0.0;
    hit.Ng_x = 0.0;
    hit.Ng_y = 0.0;
    hit.Ng_z = 0.0;
  }


  /**
   * Constructor for RTCMultiHitRay given a ray origin and look direction.
   * 
   * @param origin The body-fixed (x, y, z) origin of the ray in kilometers.
   * @param direction The unit look direction vector in body-fixed (x, y, z).
   */
  RTCMultiHitRay::RTCMultiHitRay(const std::vector<double> &origin,
                                 const std::vector<double> &direction) {
    ray.tnear   = 0.0;
    ray.tfar    = std::numeric_limits<float>::infinity(); // Should be INF
    ray.mask    = 0xFFFFFFFF;
    lastHit = -1;
    hit.u       = 0.0;
    hit.v       = 0.0;
    hit.geomID  = RTC_INVALID_GEOMETRY_ID;
    hit.primID  = RTC_INVALID_GEOMETRY_ID;
    hit.instID[0]  = RTC_INVALID_GEOMETRY_ID;
    ray.org_x = origin[0];
    ray.org_y = origin[1];
    ray.org_z = origin[2];
    ray.dir_x = direction[0];
    ray.dir_y = direction[1];
    ray.dir_z = direction[2];
    hit.Ng_x = 0.0;
    hit.Ng_y = 0.0;
    hit.Ng_z = 0.0;
  }


  /**
   * Constructor for RTCMultiHitRay given a ray origin and look direction as
   * ISIS linear algebra vectors.
   * 
   * @param origin The body-fixed (x, y, z) origin of the ray in kilometers.
   * @param direction The unit look direction vector in body-fixed (x, y, z).
   */
  RTCMultiHitRay::RTCMultiHitRay(LinearAlgebra::Vector origin,
                                 LinearAlgebra::Vector direction) {
    ray.tnear   = 0.0;
    ray.tfar    = std::numeric_limits<float>::infinity(); // Should be INF
    ray.mask    = 0xFFFFFFFF;
    lastHit = -1;
    hit.u       = 0.0;
    hit.v       = 0.0;
    hit.geomID  = RTC_INVALID_GEOMETRY_ID;
    hit.primID  = RTC_INVALID_GEOMETRY_ID;
    hit.instID[0]  = RTC_INVALID_GEOMETRY_ID;
    ray.org_x = origin[0];
    ray.org_y = origin[1];
    ray.org_z = origin[2];
    ray.dir_x = direction[0];
    ray.dir_y = direction[1];
    ray.dir_z = direction[2];
    hit.Ng_x = 0.0;
    hit.Ng_y = 0.0;
    hit.Ng_z = 0.0;
  }


  /**
   * Default constructor for RTCOcclussionRay.
   */
  RTCOcclusionRay::RTCOcclusionRay() {
    ray.tnear   = 0.0;
    ray.tfar    = std::numeric_limits<float>::infinity(); // Should be INF
    ray.mask    = 0xFFFFFFFF;
    lastHit = -1;
    hit.u       = 0.0;
    hit.v       = 0.0;
    ignorePrimID = -1;
    hit.geomID  = RTC_INVALID_GEOMETRY_ID;
    hit.primID  = RTC_INVALID_GEOMETRY_ID;
    hit.instID[0]  = RTC_INVALID_GEOMETRY_ID;
    ray.org_x = 0.0;
    ray.org_y = 0.0;
    ray.org_z = 0.0;
    ray.dir_x = 0.0;
    ray.dir_y = 0.0;
    ray.dir_z = 0.0;
    hit.Ng_x = 0.0;
    hit.Ng_y = 0.0;
    hit.Ng_z = 0.0;
  }


  /**
   * Constructor for RTCOcclusionRay given a ray origin and look direction.
   * 
   * @param origin The body-fixed (x, y, z) origin of the ray in kilometers.
   * @param direction The unit look direction vector in body-fixed (x, y, z).
   */
  RTCOcclusionRay::RTCOcclusionRay(const std::vector<double> &origin,
                                 const std::vector<double> &direction) {
    ray.tnear   = 0.0;
    ray.tfar    = std::numeric_limits<float>::infinity(); // Should be INF
    ray.mask    = 0xFFFFFFFF;
    lastHit = -1;
    hit.u       = 0.0;
    hit.v       = 0.0;
    hit.geomID  = RTC_INVALID_GEOMETRY_ID;
    ignorePrimID  = -1;
    hit.instID[0]  = RTC_INVALID_GEOMETRY_ID;
    ray.org_x = origin[0];
    ray.org_y = origin[1];
    ray.org_z = origin[2];
    ray.dir_x = direction[0];
    ray.dir_y = direction[1];
    ray.dir_z = direction[2];
    hit.Ng_x = 0.0;
    hit.Ng_y = 0.0;
    hit.Ng_z = 0.0;
  }

  /**
   * Constructor for RTCOcclusionRay given a ray origin and look direction as
   * ISIS linear algebra vectors.
   * 
   * @param origin The body-fixed (x, y, z) origin of the ray in kilometers.
   * @param direction The unit look direction vector in body-fixed (x, y, z).
   */
  RTCOcclusionRay::RTCOcclusionRay(LinearAlgebra::Vector origin,
                                   LinearAlgebra::Vector direction) {
    ray.tnear   = 0.0;
    ray.tfar    = std::numeric_limits<float>::infinity(); // Should be INF
    ray.mask    = 0xFFFFFFFF;
    lastHit = -1;
    hit.u       = 0.0;
    hit.v       = 0.0;
    hit.geomID  = RTC_INVALID_GEOMETRY_ID;
    ignorePrimID  = -1;
    hit.instID[0]  = RTC_INVALID_GEOMETRY_ID;
    ray.org_x = origin[0];
    ray.org_y = origin[1];
    ray.org_z = origin[2];
    ray.dir_x = direction[0];
    ray.dir_y = direction[1];
    ray.dir_z = direction[2];
    hit.Ng_x = 0.0;
    hit.Ng_y = 0.0;
    hit.Ng_z = 0.0;
  }


  /**
   * Default constructor for RayHitInformation
   */
  RayHitInformation::RayHitInformation() {
    intersection = LinearAlgebra::vector(0, 0, 0);
    surfaceNormal = LinearAlgebra::vector(0, 0, 0);
    primID = 0;
  }


  /**
   * Constructor for RayHitInformation given an intersection and unit surface
   * normal.
   * 
   * @param location The body-fixed (x, y, z) location of the intersection in kilometers.
   * @param normal The unit surface normal vector in body-fixed (x, y, z). 
   * @param primID The primitive ID
   */
  RayHitInformation::RayHitInformation(LinearAlgebra::Vector &location,
                                       LinearAlgebra::Vector &normal,
                                       int prim)
      : intersection(location),
        surfaceNormal(normal) ,
        primID(prim) { }


  /**
   * @brief Default empty constructor.
   * 
   * The filename defaults to an empty string and the file handle defaults to 0.
   * Counts default to 0 and the cache size defaults to 0.
   */
  EmbreeTargetShape::EmbreeTargetShape()
      : m_name(),
        m_mesh(),
        m_cloud(),
        m_device(rtcNewDevice(NULL)),
        m_scene(rtcNewScene(m_device)) 
  {
    rtcSetSceneFlags(m_scene, RTC_SCENE_FLAG_ROBUST | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION);
    rtcSetSceneBuildQuality(m_scene, RTC_BUILD_QUALITY_HIGH);
  }

  /** 
   * Constructs an EmbreeTargetShape from a PointCloudLibrary polygon mesh.
   * 
   * @param mesh The polygon mesh to construct the scene from.
   * @param name The name of the target being represented.
   */
  EmbreeTargetShape::EmbreeTargetShape(pcl::PolygonMesh::Ptr mesh, const QString &name)
      : m_name(name),
        m_mesh(),
        m_cloud(),
        m_device(rtcNewDevice(NULL)),
        m_scene(rtcNewScene(m_device))
  {
    rtcSetSceneFlags(m_scene, RTC_SCENE_FLAG_ROBUST | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION);
    rtcSetSceneBuildQuality(m_scene, RTC_BUILD_QUALITY_HIGH);
    initMesh(mesh);
  }

  /**
   * Constructs an EmbreeTargetShape from a file.
   * 
   * @param dem The file to construct the target shape from. The file type is determined
   *            based on the file extension.
   * @param conf Pvl containing configuration settings for the target shape.
   *             Currently unused.
   * 
   * @throws IException::Io
   */
  EmbreeTargetShape::EmbreeTargetShape(const QString &dem, const Pvl *conf)
      : m_name(),
        m_mesh(),
        m_cloud(),
        m_device(rtcNewDevice(NULL)),
        m_scene(rtcNewScene(m_device)) {
    rtcSetSceneFlags(m_scene, RTC_SCENE_FLAG_ROBUST | RTC_SCENE_FLAG_CONTEXT_FILTER_FUNCTION);
    rtcSetSceneBuildQuality(m_scene, RTC_BUILD_QUALITY_HIGH);
    FileName file(dem);
    pcl::PolygonMesh::Ptr mesh;
    m_name = file.baseName();

    try {
      // DEMs (ISIS cubes) TODO implement this
      if (file.extension() == "cub") {
        QString msg = "DEMs cannot be used to create an EmbreeTargetShape.";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      // DSKs
      else if (file.extension().toLower() == "bds") {
        mesh = readDSK(file);
      }
      // Let PCL try to handle other formats (obj, ply, etc.)
      else {
        mesh = readPC(file);
      }
    }
    catch (IException &e) {
      QString msg = "Failed creating an EmbreeTargetShape from ["
                    + file.expanded() + "].";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }
    initMesh(mesh);
  }


  /**
   * Read a NAIF type 2 DSK file into a PointCloudLibrary polygon mesh.
   * The vertex and plate ordering in the DSK file is maintained in the polygon mesh.
   * 
   * @param file The DSK file to load.
   * 
   * @return @b pcl::PolygonMesh::Ptr A boost shared pointer to the mesh.
   * 
   * @throws IException::User
   * @throws IException::Io
   */
  pcl::PolygonMesh::Ptr EmbreeTargetShape::readDSK(FileName file) {

    /** NAIF DSK parameter setup   */
    SpiceInt      dskHandle;      //!< The DAS file handle of the DSK file.
    SpiceDLADescr dlaDescriptor;  /**< The DLA descriptor of the DSK segment representing the 
                                       target surface.*/
    SpiceDSKDescr dskDescriptor;  //!< The DSK descriptor.
    SpiceInt      numPlates;      //!< Number of Plates in the model.
    SpiceInt      numVertices;    //!< Number of vertices in the model.

    // Sanity check
    if ( !file.fileExists() ) {
      QString mess = "NAIF DSK file [" + file.expanded() + "] does not exist.";
      throw IException(IException::User, mess, _FILEINFO_);
    }
  
    // Open the NAIF Digital Shape Kernel (DSK)
    dasopr_c( file.expanded().toLatin1().data(), &dskHandle );
    NaifStatus::CheckErrors();
  
    // Search to the first DLA segment
    SpiceBoolean found;
    dlabfs_c( dskHandle, &dlaDescriptor, &found );
    NaifStatus::CheckErrors();
    if ( !found ) {
      QString mess = "No segments found in DSK file [" + file.expanded() + "]"; 
      throw IException(IException::User, mess, _FILEINFO_);
    }

    dskgd_c( dskHandle, &dlaDescriptor, &dskDescriptor );
    NaifStatus::CheckErrors();

    // Get The number of polygons and vertices
    dskz02_c( dskHandle, &dlaDescriptor, &numVertices, &numPlates );
    NaifStatus::CheckErrors();

    // Allocate polygon and vertices arrays
    //   These arrays are actually numVertices x 3 and numPlates x 3,
    //     this allocation is easier and produces needed memory layout for dskv02_c
    //   These arrays are very large, so they need to be heap allocated.
    double *verticesArray = new double[numVertices * 3];
    int *polygonsArray = new int[numPlates * 3];

    // Read the vertices from the dsk file
    SpiceInt numRead = 0;
    dskv02_c(dskHandle, &dlaDescriptor, 1, numVertices,
             &numRead, ( SpiceDouble(*)[3] )(verticesArray) );
    NaifStatus::CheckErrors();
    if ( numRead != numVertices ) {
      QString msg = "Failed reading all vertices from the DSK file, ["
                    + toString(numRead) + "] out of ["
                    + toString(numVertices) + "] vertices read.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Read the polygons from the DSK
    numRead = 0;
    dskp02_c(dskHandle, &dlaDescriptor, 1, numPlates,
             &numRead, ( SpiceInt(*)[3] )(polygonsArray) );
    NaifStatus::CheckErrors();
    if ( numRead != numPlates ) {
      QString msg = "Failed reading all polygons from the DSK file, ["
                    + toString(numRead) + "] out of ["
                    + toString(numPlates) + "] polygons read.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Store the vertices in a point cloud
    pcl::PointCloud<pcl::PointXYZ>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZ>);
    for (int vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex) {
      cloud->push_back(pcl::PointXYZ(verticesArray[vertexIndex * 3],
                                     verticesArray[vertexIndex * 3 + 1],
                                     verticesArray[vertexIndex * 3 + 2]));
    }

    // Store the polygons as a vector of vertices
    std::vector<pcl::Vertices> polygons;
    for (int plateIndex = 0; plateIndex < numPlates; ++plateIndex) {
      pcl::Vertices vertexIndices;
      // NAIF uses 1 based indexing for the vertices, so subtract 1
      vertexIndices.vertices.push_back(polygonsArray[plateIndex * 3] - 1);
      vertexIndices.vertices.push_back(polygonsArray[plateIndex * 3 + 1] - 1);
      vertexIndices.vertices.push_back(polygonsArray[plateIndex * 3 + 2] - 1);
      polygons.push_back(vertexIndices);
    }

    // Create the mesh
    pcl::PolygonMesh::Ptr mesh(new pcl::PolygonMesh);
    mesh->polygons = polygons;
    pcl::PCLPointCloud2::Ptr cloudBlob(new pcl::PCLPointCloud2);
    pcl::toPCLPointCloud2(*cloud, *cloudBlob);
    mesh->cloud = *cloudBlob;

    // Free the vectors used to read the vertices and polygons
    delete [] verticesArray;
    delete [] polygonsArray;

    return mesh;
  }


  /**
   * Read a PointCloudLibrary file into a PointCloudLibrary polygon mesh.
   * 
   * @param file The PointCloudLibrary file to load.
   * 
   * @return @b pcl::PolygonMesh::Ptr A boost shared pointer to the mesh.
   * 
   * @throws IException::Io
   */
  pcl::PolygonMesh::Ptr EmbreeTargetShape::readPC(FileName file) {
    pcl::PolygonMesh::Ptr mesh( new pcl::PolygonMesh );

    int loadStatus = pcl::io::load(file.expanded().toStdString(), *mesh);
    if (loadStatus == -1) {
      QString msg = "Failed loading target shape file [" + file.expanded() + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    return mesh;
  }


  /**
   * Internalize a PointCloudLibrary polygon mesh in the target shape. The mesh
   * itself is stored along with a duplicate of the vertex point cloud because
   * the point cloud stored in the polygon mesh cannot be read without
   * Robot Operating System routines. The mesh is loaded into the internal
   * Embree scene and the scene is commited. Any changes made to the Embree
   * scene after this method is called will not take effect until
   * embree::rtcCommit is called again.
   * 
   * @note This method is NOT reentrant. Calling this again with a new mesh
   *       will remove the PointCloudLibrary representation of the old mesh
   *       but the Embree scene will contain all previous meshes along with
   *       the new mesh. Use embree::rtcDeleteGeometry to remove an old mesh
   *       from the scene.
   * 
   * @param mesh The mesh to be internalized.
   */
  void EmbreeTargetShape::initMesh(pcl::PolygonMesh::Ptr mesh) {
    m_mesh = mesh;

    // The points are stored in a pcl::PCLPointCloud2 object that we cannot used.
    // So, convert them into a pcl::PointCloud<pcl::PointXYZ> object that we can use.
    pcl::fromPCLPointCloud2(mesh->cloud, m_cloud);

    // Create a static geometry (the body) in our scene
    RTCGeometry rtcMesh = rtcNewGeometry(m_device, RTC_GEOMETRY_TYPE_TRIANGLE);
    // Add vertices and faces (indices)
    if (!isValid()) {
      return;
    }
    // Add the body's vertices to the Embree ray tracing device's vertex buffer
    Vertex *vertices = (Vertex *)rtcSetNewGeometryBuffer(rtcMesh, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vertex), numberOfVertices());
    for (int v = 0; v < numberOfVertices(); ++v) {
      vertices[v].x = m_cloud.points[v].x;
      vertices[v].y = m_cloud.points[v].y;
      vertices[v].z = m_cloud.points[v].z;
    }

    if (!isValid()) {
      return;
    }
    Triangle *triangles = (Triangle *)rtcSetNewGeometryBuffer(rtcMesh, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(Triangle), numberOfPolygons());
    // Add the body's face (vertex indices) to the Embree device's index buffer
    for (int t = 0; t < numberOfPolygons(); ++t) {
      triangles[t].v0 = m_mesh->polygons[t].vertices[0];
      triangles[t].v1 = m_mesh->polygons[t].vertices[1];
      triangles[t].v2 = m_mesh->polygons[t].vertices[2];
    }

    // Add the multi-hit filter
    rtcSetGeometryIntersectFilterFunction(rtcMesh, EmbreeTargetShape::multiHitFilter);

    // Add the occlusion filter
    rtcSetGeometryOccludedFilterFunction(rtcMesh, EmbreeTargetShape::occlusionFilter);

    rtcCommitGeometry(rtcMesh);
    rtcAttachGeometry(m_scene, rtcMesh);
    rtcReleaseGeometry(rtcMesh);

    // Done, now we can perform some ray tracing
    rtcCommitScene(m_scene);
  }


  /**
   * Adds the vertices from the internalized vertex point cloud to the Embree
   * scene.
   * 
   * @param geomID The Embree geometry ID of the Embree geometry that the
   *               vertices will be stored in.
   * 
   * @see EmbreeTargetShape::initMesh
   */
  void EmbreeTargetShape::addVertices(int geomID) {
    // if (!isValid()) {
    //   return;
    // }
    // // Add the body's vertices to the Embree ray tracing device's vertex buffer
    // Vertex *vertices = (Vertex *) rtcMapBuffer(m_scene, geomID, RTC_VERTEX_BUFFER);
    // for (int v = 0; v < numberOfVertices(); ++v) {
    //   vertices[v].x = m_cloud.points[v].x;
    //   vertices[v].y = m_cloud.points[v].y;
    //   vertices[v].z = m_cloud.points[v].z;
    // }
    // // Flush buffer
    // rtcUnmapBuffer(m_scene, geomID, RTC_VERTEX_BUFFER);
  }


  /**
   * Adds the polygon vertex indices from the internalized polygon mesh to the
   * Embree scene.
   * 
   * @param geomID The Embree geometry ID of the Embree geometry that the
   *               indices will be stored in.
   * 
   * @see EmbreeTargetShape::initMesh
   */
  void EmbreeTargetShape::addIndices(int geomID) {
    // if (!isValid()) {
    //   return;
    // }
    // // Add the body's face (vertex indices) to the Embree device's index buffer
    // Triangle *triangles = (Triangle *) rtcMapBuffer(m_scene, geomID, RTC_INDEX_BUFFER);
    // for (int t = 0; t < numberOfPolygons(); ++t) {
    //   triangles[t].v0 = m_mesh->polygons[t].vertices[0];
    //   triangles[t].v1 = m_mesh->polygons[t].vertices[1];
    //   triangles[t].v2 = m_mesh->polygons[t].vertices[2];
    // }
    // // Flush buffer
    // rtcUnmapBuffer(m_scene, geomID, RTC_INDEX_BUFFER);
  }


  /**
   * Desctructor. The PointCloudLibrary objects are automatically cleaned up,
   * but the Embree scene and device must be manually cleaned up.
   */
  EmbreeTargetShape::~EmbreeTargetShape() {
    rtcReleaseScene(m_scene);
    rtcReleaseDevice(m_device);
  }


  /**
   * Return the number of polygons in the target shape.
   * 
   * @return @b int The number of polygons in the polygon mesh representation
   *                of the target.
   */
  int EmbreeTargetShape::numberOfPolygons() const {
    if (isValid()) {
      return m_mesh->polygons.size();
    }
    return 0;
  }


  /**
   * Return the number of vertices in the target shape.
   * 
   * @return @b int The number of vertices in the polygon mesh representation
   *                of the target.
   */
  int EmbreeTargetShape::numberOfVertices() const {
    if (isValid()) {
      return m_mesh->cloud.height * m_mesh->cloud.width;
    }
    return 0;
  }


  /**
   * Returns the bounds of the Embree scene. If the scene has not been
   * initialized, then all bounds are returned as 0.0.
   * 
   * @return @b RTCBounds Struct containing the upper and lower bounds on each
   *                      dimension of the scene. Bounds are stored as floats
   *                      in lower_x, upper_x, lower_y, upper_y, lower_z, and
   *                      upper_z.
   *                      
   */
  RTCBounds EmbreeTargetShape::sceneBounds() const {
    RTCBounds sceneBounds;
    if (isValid()) {
      rtcGetSceneBounds(m_scene, &sceneBounds);
    }
    else {
      sceneBounds.lower_x = 0.0;
      sceneBounds.lower_y = 0.0;
      sceneBounds.lower_z = 0.0;
      sceneBounds.upper_x = 0.0;
      sceneBounds.upper_y = 0.0;
      sceneBounds.upper_z = 0.0;
    }
    return sceneBounds;
  }


  /**
   * Return the maximum distance within the scene. This is computed as the
   * length of the diagonal from once corner of the scene to the opposite.
   * 
   * @return @b double The maximum distance within the Embree scene in kilometers.
   */
  double EmbreeTargetShape::maximumSceneDistance() const {
    RTCBounds sceneBoundary = sceneBounds();
    LinearAlgebra::Vector diagonal(3);
    diagonal[0] = sceneBoundary.upper_x - sceneBoundary.lower_x;
    diagonal[1] = sceneBoundary.upper_y - sceneBoundary.lower_y;
    diagonal[2] = sceneBoundary.upper_z - sceneBoundary.lower_z;
    return LinearAlgebra::magnitude(diagonal);
  }


  /**
   * Intersect a ray with the target shape. After calling, up to 16
   * intersections will be stored within the RTCMultiHitRay. The intersection
   * information will be stored in the following arrays:
   * <ul>
   *   <li> <b>hitGeomIDs:</b>
   *         The Embree Geometry ID of the intersected geometry </li>
   *   <li> <b>hitPrimIDs:</b>
   *         The index of the intersected polygon </li>
   *   <li> <b>hitUs:</b>
   *         The Barycentric u coordinate of the intersection relative to the
   *         intersected polygon </li>
   *   <li> <b>hitVs:</b>
   *         The Barycentric V coordinate of the intersection relative to the
   *         intersected polygon </li>
   * </ul>
   * The index of the last intersection is stored in <b>lastHit</b>.
   * 
   * @note The intersection information is stored in the order that Embree
   *       finds them. This is not necessarily their order along the ray. Only
   *       the first intersection is guaranteed to be in the correct order.
   *       Other intersections may be swapped with the next, or previous,
   *       intersection relative to their order along the ray.
   * 
   * @param[in,out] ray The ray to intersect with the scene. After calling, The
   *                    intersection information will be stored in the ray.
   * 
   * @see embree::rtcIntersect
   */
  void EmbreeTargetShape::intersectRay(RTCMultiHitRay &ray) {
    if (isValid()) {
      RTCIntersectContext context;
      rtcInitIntersectContext(&context);

      rtcIntersect1(m_scene, &context, (RTCRayHit *)&ray);
    }
  }


  /**
   * Check if a ray intersects the target body.
   * 
   * @return @b bool If the ray intersects anything.
   * 
   * @see embree::rtcOccluded
   */
  bool EmbreeTargetShape::isOccluded(RTCOcclusionRay &ray) {
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);

    rtcOccluded1(m_scene,  &context, (RTCRay*)&ray);

    // rtcOccluded sets the ray.tfar to -inf if the ray hits anything
    if (isinf(ray.ray.tfar) && ray.ray.tfar < 0) {
      return true;
    }
    return false;
  }


  /**
   * Extract the intersection point and unit surface normal from an
   * RTCMultiHitRay that has been intersected with the target shape. This
   * method performs two calculations. First, it converts the intersection
   * point from barycentric coordinates relative to the intersected polygon to
   * the body-fixed (x, y, z) coordinates. Second, it computes the unit normal
   * vector of the intersected polygon. The polygon vertices are assumed to be
   * ordered counter-clockwise about the exterior surface normal as they are in
   * NAIF type 2 DSK files.
   * 
   * @param ray The ray to extract intersection information from.
   * @param hitIndex The index of the intersection to extract.
   * 
   * @return @b RayHitInformation The body-fixed intersection coordinate in
   *                              kilometers and the unit surface normal at the
   *                              intersection.
   * 
   * @throws IException::Programmer
   */
  RayHitInformation EmbreeTargetShape::getHitInformation(RTCMultiHitRay &ray, int hitIndex) {
    if (hitIndex > ray.lastHit || hitIndex < 0) {
      QString msg = "Hit index [" + toString(hitIndex) + "] is out of bounds.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    // Get the vertices of the triangle hit
    pcl::PointXYZ v0 = m_cloud.points[m_mesh->polygons[ray.hitPrimIDs[hitIndex]].vertices[0]];
    pcl::PointXYZ v1 = m_cloud.points[m_mesh->polygons[ray.hitPrimIDs[hitIndex]].vertices[1]];
    pcl::PointXYZ v2 = m_cloud.points[m_mesh->polygons[ray.hitPrimIDs[hitIndex]].vertices[2]];

    // The intersection location comes out in barycentric coordinates, (u, v, w).
    // Only u and v are returned because u + v + w = 1. If the coordinates of the
    // triangle vertices are v0, v1, and v2, then the cartesian coordinates are:
    //   w*v0 + u*v1 + v*v2
    float u = ray.hitUs[hitIndex];
    float v = ray.hitVs[hitIndex];
    float w = 1.0 - u - v;

    LinearAlgebra::Vector intersection(3);
    intersection[0] = w*v0.x + v*v1.x + u*v2.x;
    intersection[1] = w*v0.y + v*v1.y + u*v2.y;
    intersection[2] = w*v0.z + v*v1.z + u*v2.z;

    // Calculate the normal vector as (v1 - v0) x (v2 - v0) and normalize it
    // TODO This calculation assumes that the shape conforms to the NAIF dsk standard
    //      of the plate vertices being ordered counterclockwise about the normal.
    //      Check if this is true for other file types and/or make a more generic process
    LinearAlgebra::Vector surfaceNormal(3);
    surfaceNormal[0] = (v1.y - v0.y) * (v2.z - v0.z)
                       - (v1.z - v0.z) * (v2.y - v0.y);
    surfaceNormal[1] = (v1.z - v0.z) * (v2.x - v0.x)
                       - (v1.x - v0.x) * (v2.z - v0.z);
    surfaceNormal[2] = (v1.x - v0.x) * (v2.y - v0.y)
                       - (v1.y - v0.y) * (v2.x - v0.x);

    // The surface normal is not normalized so normalize it.
    surfaceNormal = LinearAlgebra::normalize(surfaceNormal);
    return RayHitInformation(intersection, surfaceNormal, ray.hitPrimIDs[hitIndex]);
  }


  /**
   * Return the name of the target shape.
   * 
   * @return @b QString The name of the target.
   */
  QString EmbreeTargetShape::name() const {
    return ( m_name );
  }


  /**
   * Return if a valid mesh is internalized and ready for use.
   * 
   * @return @b bool If a mesh is internalized and the Embree scene is ready.
   */
  bool EmbreeTargetShape::isValid() const {
    return m_mesh.get();
  }


  /**
   * Filter function for collecting multiple hits during ray intersection.
   * This function is called by the Embree library during ray tracing. Each
   * time an intersection is found, this method is called.
   * 
   * @param[in] userDataPtr Data pointer from the geometry hit. Not used.
   * @param[in,out] ray The ray being traced. Information about the
   *                    intersection will be stored in the ray.
   */
  void EmbreeTargetShape::multiHitFilter(const RTCFilterFunctionNArguments* args) {
    /* avoid crashing when debug visualizations are used */
    if (args->context == nullptr)
      return;

    assert(args->N == 1);
    int *valid = args->valid;
    if (valid[0] != -1) {
      return;
    }
    RTCMultiHitRay *ray = (RTCMultiHitRay *)args->ray;
    RTCHit *hit = (RTCHit *)args->hit;

    // Calculate the index to store the hit in
    ray->lastHit++;

    // Store the hits
    ray->hitGeomIDs[ray->lastHit] = hit->geomID;
    ray->hitPrimIDs[ray->lastHit] = hit->primID;
    ray->hitUs[ray->lastHit] = hit->u;
    ray->hitVs[ray->lastHit] = hit->v;

    // If there are less than 16 hits, continue ray tracing.
    if (ray->lastHit < 15) {
      valid[0] = 0;
      ray->hit.geomID = RTC_INVALID_GEOMETRY_ID;
      hit->geomID = RTC_INVALID_GEOMETRY_ID;
    }
  }

  /**
   * Filter function for collecting multiple primitiveIDs
   * This function is called by the Embree library during ray tracing. Each
   * time an intersection is found, this method is called.
   * 
   * @param[in] userDataPtr Data pointer from the geometry hit. Not used.
   * @param[in,out] ray The ray being traced. Information about the
   *                    intersection will be stored in the ray.
   */
  void EmbreeTargetShape::occlusionFilter(const RTCFilterFunctionNArguments* args) {
    /* avoid crashing when debug visualizations are used */
    if (args->context == nullptr)
      return;

    assert(args->N == 1);
    int *valid = args->valid;
    if (valid[0] != -1) {
      return;
    }
    RTCOcclusionRay *ray = (RTCOcclusionRay *)args->ray;
    RTCHit *hit = (RTCHit *)args->hit;
    ray->hit.primID = hit->primID;

    // This is the case where we've re-intersected the occluded plate. If this happens, ignore 
    // and keep tracing
    if (ray->hit.primID == ray->ignorePrimID) {
      valid[0] = 0;
      // ray->hit.geomID = RTC_INVALID_GEOMETRY_ID;
      // hit->geomID = RTC_INVALID_GEOMETRY_ID;
    }
  }

}  // namespace Isis
