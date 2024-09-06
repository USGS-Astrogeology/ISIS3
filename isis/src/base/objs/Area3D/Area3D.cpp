/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Area3D.h"

#include <algorithm>

#include <QObject>
#include <QString>

#include "Displacement.h"
#include "Distance.h"
#include "IException.h"
#include "IString.h"

namespace Isis {
  /**
   * The empty constructor creates an invalid 3D area.
   */
  Area3D::Area3D() {
    nullTheData();
  }


  /**
   * Creates a 3D area based on the supplied dimensions.
   *
   * @param startX the leftmost X position
   * @param startY the topmost Y position
   * @param startZ the frontmost Z position
   * @param width the width of the X dimension
   * @param height the height of the Y dimension
   * @param depth the depth of the Z dimension
   */
  Area3D::Area3D(const Displacement &startX, const Displacement &startY,
                 const Displacement &startZ,
                 const Distance &width, const Distance &height,
                 const Distance &depth) {
    nullTheData();
    setArea(startX, startY, startZ,
            startX + width, startY + height, startZ + depth);
  }


  /**
   * Creates a 3D area based on the supplied start and end points.
   *
   * @param startX the leftmost X position
   * @param startY the topmost Y position
   * @param startZ the frontmost Z position
   * @param endX the rightmost X position
   * @param endY the bottommost Y position
   * @param endZ the backmost Z position
   */
  Area3D::Area3D(const Displacement &startX, const Displacement &startY,
                 const Displacement &startZ,
                 const Displacement &endX, const Displacement &endY,
                 const Displacement &endZ) {
    nullTheData();
    setArea(startX, startY, startZ, endX, endY, endZ);
  }


  /**
   * The copy constructor.
   *
   * @param areaToCopy the 3D area to copy into this
   */
  Area3D::Area3D(const Area3D &areaToCopy) {
    nullTheData();

    if(areaToCopy.m_startX) {
      m_startX = new Displacement(*areaToCopy.m_startX);
    }

    if(areaToCopy.m_startY) {
      m_startY = new Displacement(*areaToCopy.m_startY);
    }

    if(areaToCopy.m_startZ) {
      m_startZ = new Displacement(*areaToCopy.m_startZ);
    }

    if(areaToCopy.m_endX) {
      m_endX = new Displacement(*areaToCopy.m_endX);
    }

    if(areaToCopy.m_endY) {
      m_endY = new Displacement(*areaToCopy.m_endY);
    }

    if(areaToCopy.m_endZ) {
      m_endZ = new Displacement(*areaToCopy.m_endZ);
    }
  }


  /**
   * The destructor frees allocated memory.
   */
  Area3D::~Area3D() {
    deleteTheData();
  }


  /**
   * Returns the leftmost X position of the 3D area.
   * 
   * @return Displacement The leftmost X position of the 3D area
   */
  Displacement Area3D::getStartX() const {
    Displacement startX;

    if(m_startX)
      startX = *m_startX;

    return startX;
  }


  /**
   * Returns the topmost Y position of the 3D area.
   * 
   * @return Displacement The topmost Y position of the 3D area
   */
  Displacement Area3D::getStartY() const {
    Displacement startY;

    if(m_startY)
      startY = *m_startY;

    return startY;
  }


  /**
   * Returns the frontmost Z position of the 3D area.
   * 
   * @return Displacement The frontmost Z position of the 3D area
   */
  Displacement Area3D::getStartZ() const {
    Displacement startZ;

    if(m_startZ)
      startZ = *m_startZ;

    return startZ;
  }


  /**
   * Returns the width (in the X dimension) of the 3D area.
   * 
   * @return Displacement The width (in the X dimension) of the 3D area
   */
  Distance Area3D::getWidth() const {
    return Distance(
        (getEndX() - getStartX()).meters(),
        Distance::Meters);
  }


  /**
   * Returns the height (in the Y dimension) of the 3D area.
   * 
   * @return Displacement The height (in the Y dimension) of the 3D area
   */
  Distance Area3D::getHeight() const {
    return Distance(
        (getEndY() - getStartY()).meters(),
        Distance::Meters);
  }


  /**
   * Returns the depth (in the Z dimension) of the 3D area.
   * 
   * @return Displacement The depth (in the Z dimension) of the 3D area
   */
  Distance Area3D::getDepth() const {
    return Distance(
        (getEndZ() - getStartZ()).meters(),
        Distance::Meters);
  }


  /**
   * Returns the rightmost X position of the 3D area.
   * 
   * @return Displacement The rightmost X position of the 3D area
   */
  Displacement Area3D::getEndX() const {
    Displacement endX;

    if(m_endX)
      endX = *m_endX;

    return endX;
  }


  /**
   * Returns the bottommost Y position of the 3D area.
   * 
   * @return Displacement The bottommost Y position of the 3D area
   */
  Displacement Area3D::getEndY() const {
    Displacement endY;

    if(m_endY)
      endY = *m_endY;

    return endY;
  }


  /**
   * Returns the backmost Z position of the 3D area.
   * 
   * @return Displacement The backmost Z position of the 3D area
   */
  Displacement Area3D::getEndZ() const {
    Displacement endZ;

    if(m_endZ)
      endZ = *m_endZ;

    return endZ;
  }


  /**
   * Sets the leftmost X position. This method may change the width of the 3D
   *   area.
   *
   * @param startX the new leftmost X position
   */
  void Area3D::setStartX(const Displacement &startX) {
    setArea(startX, getStartY(), getStartZ(),
            getEndX(), getEndY(), getEndZ());
  }


  /**
   * Sets the topmost Y position. This method may change the height of the 3D
   *   area.
   *
   * @param startY the new topmost Y position
   */
  void Area3D::setStartY(const Displacement &startY) {
    setArea(getStartX(), startY, getStartZ(),
            getEndX(), getEndY(), getEndZ());
  }


  /**
   * Sets the frontmost Z position. This method may change the depth of the 3D
   *   area.
   *
   * @param startZ the new frontmost Z position
   */
  void Area3D::setStartZ(const Displacement &startZ) {
    setArea(getStartX(), getStartY(), startZ,
            getEndX(), getEndY(), getEndZ());
  }


  /**
   * Moves the leftmost X position of the 3D area. This method will not change
   *   the width of the 3D area.
   *
   * @param startX the new leftmost X position
   */
  void Area3D::moveStartX(const Displacement &startX) {
    setArea(startX, getStartY(), getStartZ(),
            getEndX() + (startX - getStartX()), getEndY(), getEndZ());
  }


  /**
   * Moves the topmost Y position of the 3D area. This method will not change
   *   the height of the 3D area.
   *
   * @param startY the new topmost Y position
   */
  void Area3D::moveStartY(const Displacement &startY) {
    setArea(getStartX(), startY, getStartZ(),
            getEndX(), getEndY() + (startY - getStartY()), getEndZ());
  }


  /**
   * Moves the frontmost Z position of the 3D area. This method will not change
   *   the depth of the 3D area.
   *
   * @param startZ the new frontmost Z position
   */
  void Area3D::moveStartZ(const Displacement &startZ) {
    setArea(getStartX(), getStartY(), startZ,
            getEndX(), getEndY(), getEndZ() + (startZ - getStartZ()));
  }


  /**
   * Changes the width of the 3D area.
   *
   * @param width the new width of the 3D area
   */
  void Area3D::setWidth(const Distance &width) {
    setArea(getStartX(), getStartY(), getStartZ(),
            getStartX() + width, getEndY(), getEndZ());
  }


  /**
   * Changes the height of the 3D area.
   *
   * @param height the new height of the 3D area
   */
  void Area3D::setHeight(const Distance &height) {
    setArea(getStartX(), getStartY(), getStartZ(),
            getEndX(), getStartY() + height, getEndZ());
  }


  /**
   * Changes the depth of the 3D area.
   *
   * @param depth the new depth of the 3D area
   */
  void Area3D::setDepth(const Distance &depth) {
    setArea(getStartX(), getStartY(), getStartZ(),
            getEndX(), getEndY(), getStartZ() + depth);
  }


  /**
   * Sets the rightmost X position. This method may change the width of the 3D
   *   area.
   *
   * @param endX the new rightmost X position
   */
  void Area3D::setEndX(const Displacement &endX) {
    setArea(getStartX(), getStartY(), getStartZ(),
            endX, getEndY(), getEndZ());
  }


  /**
   * Sets the bottommost Y position. This method may change the height of the
   *   3D area.
   *
   * @param endY the new bottommost Y position
   */
  void Area3D::setEndY(const Displacement &endY) {
    setArea(getStartX(), getStartY(), getStartZ(),
            getEndX(), endY, getEndZ());
  }


  /**
   * Sets the backmost Z position. This method may change the depth of the
   *   3D area.
   *
   * @param endZ the new backmost Z position
   */
  void Area3D::setEndZ(const Displacement &endZ) {
    setArea(getStartX(), getStartY(), getStartZ(),
            getEndX(), getEndY(), endZ);
  }


  /**
   * Moves the rightmost X position of the 3D area. This method will not change
   *   the width of the 3D area.
   *
   * @param endX the new rightmost X position
   */
  void Area3D::moveEndX(const Displacement &endX) {
    setArea(getStartX() + (endX - getEndX()), getStartY(), getStartZ(),
            endX, getEndY(), getEndZ());
  }


  /**
   * Moves the bottommost Y position of the 3D area. This method will not
   *   change the height of the 3D area.
   *
   * @param endY the new bottommost Y position
   */
  void Area3D::moveEndY(const Displacement &endY) {
    setArea(getStartX(), getStartY() + (endY - getEndY()), getStartZ(),
            getEndX(), endY, getEndZ());
  }


  /**
   * Moves the backmost Z position of the 3D area. This method will not
   *   change the depth of the 3D area.
   *
   * @param endZ the new backmost Z position
   */
  void Area3D::moveEndZ(const Displacement &endZ) {
    setArea(getStartX(), getStartY(), getStartZ() + (endZ - getEndZ()),
            getEndX(), getEndY(), endZ);
  }


  /**
   * Sets the X dimension of the 3D area. This method may change the X
   *   positions and the width of the 3D area.
   *
   * @param startX the new leftmost X position
   * @param width the new width of the 3D area
   */
  void Area3D::setXDimension(const Displacement &startX,
                             const Distance &width) {
    setArea(startX, getStartY(), getStartZ(),
            startX + width, getEndY(), getEndZ());
  }


  /**
   * Sets the Y dimension of the 3D area. This method may change the Y
   *   positions and the height of the 3D area.
   *
   * @param startY the new topmost Y position
   * @param height the new height of the 3D area
   */
  void Area3D::setYDimension(const Displacement &startY,
                             const Distance &height) {
    setArea(getStartX(), startY, getStartZ(),
            getEndX(), startY + height, getEndZ());
  }


  /**
   * Sets the Z dimension of the 3D area. This method may change the Z
   *   positions and the depth of the 3D area.
   *
   * @param startZ the new frontmost Z position
   * @param depth the new depth of the 3D area
   */
  void Area3D::setZDimension(const Displacement &startZ,
                             const Distance &depth) {
    setArea(getStartX(), getStartY(), startZ,
            getEndX(), getEndY(), startZ + depth);
  }


  /**
   * Returns the intersection of this 3D area with another 3D area. If there is
   *   no intersection, an invalid 3D area will be returned.
   *
   * @param otherArea the area to intersect this 3D area with
   * @return the 3D area that is the intersection
   */
  Area3D Area3D::intersect(const Area3D &otherArea) const {
    Area3D result;

    // Check validity because of comparison operators.
    if (isValid() && otherArea.isValid()) {
      Displacement startX(std::max(getStartX(), otherArea.getStartX()));
      Displacement startY(std::max(getStartY(), otherArea.getStartY()));
      Displacement startZ(std::max(getStartZ(), otherArea.getStartZ()));
      Displacement endX(std::min(getEndX(), otherArea.getEndX()));
      Displacement endY(std::min(getEndY(), otherArea.getEndY()));
      Displacement endZ(std::min(getEndZ(), otherArea.getEndZ()));


      if (startX <= endX && startY <= endY && startZ <= endZ)
        result = Area3D(startX, startY, startZ, endX, endY, endZ);
    }

    return result;
  }


  /**
   * Returns true if all of the positions of the 3D area are
   *   valid (i.e. they are non-NULL).
   * 
   * @return bool True if all of the positions of the 3D area are valid (i.e. they are non-NULL).
   */
  bool Area3D::isValid() const {
    bool valid = true;

    if(!m_startX)
      valid = false;
    else if(!m_startY)
      valid = false;
    else if(!m_startZ)
      valid = false;
    else if(!m_endX)
      valid = false;
    else if(!m_endY)
      valid = false;
    else if(!m_endZ)
      valid = false;

    return valid;
  }


  /**
   * Compares two areas with the == operator. Two uninitialized areas
   *   are equal to each other.
   *
   * @param otherArea This is the area we're comparing to, i.e. on
   *     the right hand side of the operator when used
   * @return True if this area is the same as the given area
   */
  bool Area3D::operator ==(const Area3D &otherArea) const {
    return getStartX() == otherArea.getStartX() &&
            getStartY() == otherArea.getStartY() &&
            getStartZ() == otherArea.getStartZ() &&
            getEndX() == otherArea.getEndX() &&
            getEndY() == otherArea.getEndY() &&
            getEndZ() == otherArea.getEndZ();
  }


  /**
   * Compares two areas with the != operator. Two uninitialized areas
   *   are equal to each other.
   *
   * @param otherArea This is the area we're comparing to, i.e. on
   *     the right hand side of the operator when used
   * @return True if this area is not the same as the given area
   */
  bool Area3D::operator !=(const Area3D &otherArea) const {
    return !(*this == otherArea);
  }


  /**
   * Assigns areaToCopy to this. areaToCopy is the right hand side of the '='.
   *
   * @param areaToCopy This is the area we're copying from
   * @return *this
   */
  Area3D &Area3D::operator =(const Area3D &areaToCopy) {
    deleteTheData();

    if(areaToCopy.m_startX) {
      m_startX = new Displacement(*areaToCopy.m_startX);
    }

    if(areaToCopy.m_startY) {
      m_startY = new Displacement(*areaToCopy.m_startY);
    }

    if(areaToCopy.m_startZ) {
      m_startZ = new Displacement(*areaToCopy.m_startZ);
    }

    if(areaToCopy.m_endX) {
      m_endX = new Displacement(*areaToCopy.m_endX);
    }

    if(areaToCopy.m_endY) {
      m_endY = new Displacement(*areaToCopy.m_endY);
    }

    if(areaToCopy.m_endZ) {
      m_endZ = new Displacement(*areaToCopy.m_endZ);
    }

    return *this;
  }


  /**
   * Sets the area. This method may be overridden in child classes if more
   *   control is needed when setting 3D area members to reflect the new area.
   *   All other setter methods call this method to change members of this
   *   class.
   *
   * This method throws an exception if inverted coordinates are given, and
   *   it makes this 3D area invalid if invalid coordinates are given.
   *
   * @param startX the new leftmost X position
   * @param startY the new topmost Y position
   * @param startZ the new frontmost Z position
   * @param endX the new rightmost X position
   * @param endY the new bottommost Y position
   * @param endZ the new backmost Z position
   */
  void Area3D::setArea(const Displacement &startX,
      const Displacement &startY, const Displacement &startZ,
      const Displacement &endX, const Displacement &endY,
      const Displacement &endZ) {
    bool startXValid = startX.isValid();
    bool endXValid = endX.isValid();
    bool startYValid = startY.isValid();
    bool endYValid = endY.isValid();
    bool startZValid = startZ.isValid();
    bool endZValid = endZ.isValid();

    deleteTheData();

    // Optimized for success.
    if(startXValid && startYValid && startZValid && endXValid && endYValid &&
       endZValid && startX <= endX && startY <= endY && startZ <= endZ) {
      m_startX = new Displacement(startX);
      m_startY = new Displacement(startY);
      m_startZ = new Displacement(startZ);
      m_endX = new Displacement(endX);
      m_endY = new Displacement(endY);
      m_endZ = new Displacement(endZ);
    }
    else {
      if(startXValid && endXValid && startX > endX) {
        std::string msg = "Cannot have a 3D area with inverted X coordinates of "
                                  "[" + std::to_string(startX.meters()) + " meters] to [" + std::to_string(endX.meters()) + " meters]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if(startYValid && endYValid && startY > endY) {
        std::string msg = "Cannot have a 3D area with inverted Y coordinates of "
                                  "[" + std::to_string(startY.meters()) + " meters] to [" + std::to_string(endY.meters()) + " meters]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if(startZValid && endZValid && startZ > endZ) {
        std::string msg = "Cannot have a 3D area with inverted Z coordinates of "
                                  "[" + std::to_string(startZ.meters()) + " meters] to [" + std::to_string(endZ.meters()) + " meters]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      if(startXValid)
        m_startX = new Displacement(startX);

      if(startYValid)
        m_startY = new Displacement(startY);

      if(startZValid)
        m_startZ = new Displacement(startZ);

      if(endXValid)
        m_endX = new Displacement(endX);

      if(endYValid)
        m_endY = new Displacement(endY);

      if(endZValid)
        m_endZ = new Displacement(endZ);
    }
  }


  /**
   * Frees all allocated memory used by this 3D area.
   */
  void Area3D::deleteTheData() {
    if(m_startX) {
      delete m_startX;
      m_startX = NULL;
    }

    if(m_startY) {
      delete m_startY;
      m_startY = NULL;
    }

    if(m_startZ) {
      delete m_startZ;
      m_startZ = NULL;
    }

    if(m_endX) {
      delete m_endX;
      m_endX = NULL;
    }

    if(m_endY) {
      delete m_endY;
      m_endY = NULL;
    }

    if(m_endZ) {
      delete m_endZ;
      m_endZ = NULL;
    }
  }


  /**
   * Nulls all of the members used by this 3D area.
   */
  void Area3D::nullTheData() {
    m_startX = NULL;
    m_startY = NULL;
    m_startZ = NULL;
    m_endX = NULL;
    m_endY = NULL;
    m_endZ = NULL;
  }
}

