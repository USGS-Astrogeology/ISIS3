/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "BulletDskShape.h"

#include <iostream>
#include <iomanip>
#include <numeric>
#include <sstream>

#include "NaifDskApi.h"

#include <QMutexLocker>
#include <QTime>

#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Pvl.h"
#include "NaifDskPlateModel.h"
#include "NaifStatus.h"

using namespace std;

namespace Isis {

  /**
   * Default empty constructor.
   */
  BulletDskShape::BulletDskShape() :  m_mesh() { }


  /**
   * Construct a BulletDskShape from a DSK file.
   *
   * @param dskfile The DSK file to load into a Bullet target shape.
   */
  BulletDskShape::BulletDskShape(const QString &dskfile) : m_mesh()  {
    loadFromDsk(dskfile);
    setMaximumDistance();
  }


  /**
   * Desctructor
   */
  BulletDskShape::~BulletDskShape() {
    // Bullet does not clean up the mesh automatically, so we need to delete it manually
    if (m_mesh) {
      for (int i = 0; i < m_mesh->getIndexedMeshArray().size(); i++) {
        btIndexedMesh &v_mesh = m_mesh->getIndexedMeshArray()[i];
        delete[] v_mesh.m_triangleIndexBase;
        v_mesh.m_triangleIndexBase = nullptr;
        delete[] v_mesh.m_vertexBase;
        v_mesh.m_vertexBase = nullptr;
      }
    }
  }


  /**
   * Return the number of triangles in the shape
   *
   * @return @b int The number of triangles. If nothing has been loaded, then 0 is returned.
   */
  int BulletDskShape::getNumTriangles() const {
    size_t num_triangles = 0;

    if (m_mesh) {
      for(int i = 0; i < m_mesh->getIndexedMeshArray().size(); i++) {
        num_triangles += m_mesh->getIndexedMeshArray()[i].m_numTriangles;
      }
    }

    return num_triangles;
  }


  /**
   * Return the number of verticies in the shape
   *
   * @return @b int The number of verticies. If nothing has been loaded, then 0 is returned.
   */
  int BulletDskShape::getNumVertices() const {
    size_t num_vertices = 0;

    if (m_mesh) {
      for(int i = 0; i < m_mesh->getIndexedMeshArray().size(); i++) {
        num_vertices += m_mesh->getIndexedMeshArray()[i].m_numVertices;
      }
    }

    return num_vertices;
  }


  /**
  * @brief Return normal for a given triangle index
  *
  * This method is particularly useful to return the normal of a triangle plate
  * in a mesh-based target body.
  *
  * @author 2017-03-28 Kris Becker
  *
  * @param indexId The index of the triangle in the mesh.
  *
  * @return @b btVector3 The local normal for the triangle.
  */
  btVector3 BulletDskShape::getNormal(const int indexId, const int segment) const {
    btMatrix3x3 triangle = getTriangle(indexId, segment);
    btVector3 edge1 = triangle.getRow(1) - triangle.getRow(0);
    btVector3 edge2 = triangle.getRow(2) - triangle.getRow(0);
    return ( edge1.cross( edge2 ) );
  }


  /**
   * Get the vertices of a triangle in the mesh.
   *
   * @param index The index of the triangle in the mesh.
   *
   * @return @b btMatrix3x3 Matrix with each row containing the coordinate of a
   *                        vertex. The vertices are ordered counter-clockwise
   *                        around the surface normal of the triangle.
   */
  btMatrix3x3 BulletDskShape::getTriangle(const int index, const int segment) const {
    btAssert ( index >= 0 );
    btAssert ( index < getIndexedMeshArray()[segment].m_numTriangles );

    btAssert ( segment >= 0 );
    btAssert ( segment < getIndexedMeshArray().size());

     // Set up pointers to triangle indexes
    const btIndexedMesh &v_mesh = m_mesh->getIndexedMeshArray()[segment];

    const int *t_index = static_cast<int32_t *> ((void *) v_mesh.m_triangleIndexBase);
    int p_index = 3 * index;
    int vndx0 = t_index[p_index];
    int vndx1 = t_index[p_index+1];
    int vndx2 = t_index[p_index+2];

    const btScalar *t_vertex = static_cast<const btScalar *> ((void *) v_mesh.m_vertexBase);

    btMatrix3x3 triangle(t_vertex[vndx0+0], t_vertex[vndx0+1], t_vertex[vndx0+2],
                         t_vertex[vndx1+0], t_vertex[vndx1+1], t_vertex[vndx1+2],
                         t_vertex[vndx2+0], t_vertex[vndx2+1], t_vertex[vndx2+2]);
    return ( triangle );
  }


/**
 * @brief Load the contents of a NAIF DSK and create a Bullet triangle mesh
 *
 * @author 2017-03-28 Kris Becker
 *
 * @param dskfile The DSK file to load.
 */
  void BulletDskShape::loadFromDsk(const QString &dskfile) {

    /** NAIF DSK parameter setup   */
    SpiceInt                   handle;   //!< The DAS file handle of the DSK file.

    // Sanity check
    FileName dskFile(dskfile.toStdString());
    if ( !dskFile.fileExists() ) {
      std::string mess = "NAIF DSK file [" + dskfile.toStdString() + "] does not exist.";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    // Open the NAIF Digital Shape Kernel (DSK)
    dasopr_c( dskFile.expanded().c_str(), &handle );
    NaifStatus::CheckErrors();

    // Search to the first DLA segment
    SpiceBoolean  found;
    SpiceDLADescr segment;
    dlabfs_c( handle, &segment, &found );
    NaifStatus::CheckErrors();
    if ( !found ) {
      std::string mess = "No segments found in DSK file " + dskfile.toStdString() ;
      throw IException(IException::User, mess, _FILEINFO_);
    }

    std::vector<SpiceDLADescr> segments;
    segments.push_back(segment);

    // Iterate until you find no more segments.
    while(found) {
      dlafns_c(handle, &segments.back(), &segment, &found);
      NaifStatus::CheckErrors();
      if (found)
        segments.push_back(segment);
    }

    // dskgd_c( v_handle, &v_dladsc, &v_dskdsc );
    // NaifStatus::CheckErrors();

    // Now allocate a new indexed mesh to contain all the DSK data
    m_mesh.reset( new btTriangleIndexVertexArray());

    for (size_t i = 0; i < segments.size(); i++) {
      SpiceInt nplates;
      SpiceInt nvertices;

      btIndexedMesh i_mesh;

      // Get size/counts
      dskz02_c( handle, &segments[i], &nvertices, &nplates);
      NaifStatus::CheckErrors();

      m_mesh->addIndexedMesh(i_mesh, PHY_INTEGER);

      // Get internal mesh reference and set parameters appropriately
      btIndexedMesh &v_mesh = m_mesh->getIndexedMeshArray()[i];
      v_mesh.m_vertexType = PHY_DOUBLE;

      // Set and allocate data for triangle indexes
      v_mesh.m_numTriangles = nplates;
      v_mesh.m_triangleIndexBase = new unsigned char[nplates * 3 * sizeof(int)];
      v_mesh.m_triangleIndexStride = (sizeof(int) * 3);

      // Set and allocate vertex data
      v_mesh.m_numVertices = nvertices;
      v_mesh.m_vertexBase = new unsigned char[nvertices * 3 * sizeof(double)];
      v_mesh.m_vertexStride = (sizeof(double) * 3);

      SpiceInt n;
      (void) dskv02_c(handle, &segments[i], 1, nvertices, &n,
                      ( SpiceDouble(*)[3] ) (v_mesh.m_vertexBase));
      NaifStatus::CheckErrors();

      // Read the indexes from the DSK
      (void) dskp02_c(handle, &segments[i], 1, nplates, &n,
                      ( SpiceInt(*)[3] ) (v_mesh.m_triangleIndexBase));
      NaifStatus::CheckErrors();

      // Got to reset the vertex indexes to 0-based
      int *pindex = static_cast<int *> ((void *) v_mesh.m_triangleIndexBase);
      int nverts = nplates * 3;
      for (int i = 0 ; i < nverts ; i++) {
        pindex[i] -= 1;
        btAssert ( pindex[i] >= 0 );
        btAssert ( pindex[i] < nvertices );
      }
    }

    // Close DSK
    dascls_c(handle);

    bool useQuantizedAabbCompression = true;
    // bool useQuantizedAabbCompression = false;
    btBvhTriangleMeshShape *v_triShape = new btBvhTriangleMeshShape(m_mesh.data(),
                                                                    useQuantizedAabbCompression);
    v_triShape->setUserPointer(this);
    btCollisionObject *vbody = new btCollisionObject();
    vbody->setCollisionShape(v_triShape);
    setTargetBody(vbody);

    return;

  }

}  // namespace Isis
