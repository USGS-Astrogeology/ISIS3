/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "DisplayProperties.h"

#include <QBuffer>
#include <QDataStream>
#include <QXmlStreamWriter>
#include <QMap>
#include <QVariant>

#include "FileName.h"
#include "Pvl.h"

namespace Isis {
  /**
   * DisplayProperties constructor. This sets default values and constructs
   *   the Cube *. You cannot have much more than 1K of these without calling
   *   closeCube().
   *
   *
   * @param displayName The filename (fully expanded) of the object.
   * @param parent Qt parent object (this is destroyed when parent is destroyed)
   */
  DisplayProperties::DisplayProperties(QString displayName, QObject *parent) :
      QObject(parent) {

    m_propertiesUsed = 0;
    m_propertyValues = new QMap<int, QVariant>;

    m_displayName = displayName;
  }


  /**
   * destructor
   */
  DisplayProperties::~DisplayProperties() {
  }


  void DisplayProperties::fromPvl(const PvlObject &pvl) {
    setDisplayName(QString::fromStdString(pvl["DisplayName"][0]));

    QByteArray hexValues(QString::fromStdString(pvl["Values"][0]).toLatin1());
    QDataStream valuesStream(QByteArray::fromHex(hexValues));
    valuesStream >> *m_propertyValues;
  }


  /**
   * Convert to Pvl for project files. This stores all of the data associated
   *   with all of the properties (but not what is supported). This also stores
   *   the cube filename.
   */
  PvlObject DisplayProperties::toPvl() const {
    PvlObject output("DisplayProperties");
    output += PvlKeyword("DisplayName", displayName().toStdString());

    QBuffer dataBuffer;
    dataBuffer.open(QIODevice::ReadWrite);

    QDataStream propsStream(&dataBuffer);
    propsStream << *m_propertyValues;
    dataBuffer.seek(0);

    output += PvlKeyword("Values", std::string(dataBuffer.data().toHex()));

    return output;
  }


  /**
   * Returns the display name
   */
  QString DisplayProperties::displayName() const {
    return m_displayName;
  }

  /**
   * Sets display name
   *
   * @param displayName Display name of the object.
   */
  void DisplayProperties::setDisplayName(QString displayName) {
    m_displayName = displayName;
  }


  /**
   * Call this with every property you support, otherwise they will not
   *   communicate properly between widgets.
   *
   * @param prop The property you are adding support for
   */
  void DisplayProperties::addSupport(int property) {
    if (!supports(property)) {
      m_propertiesUsed = m_propertiesUsed | property;
      emit supportAdded(property);
    }
  }


  /**
   * Support may come later, please make sure you are connected to the
   *   supportAdded signal.
   *
   * @returns True if the property has support, false otherwise
   */
  bool DisplayProperties::supports(int property) {
    return (m_propertiesUsed & property) == property;
  }


  /**
   * This is the generic mutator for properties. Given a value, this will
   *   change it and emit propertyChanged if its different and supported.
   */
  void DisplayProperties::setValue(int property, QVariant value) {
    if ((*m_propertyValues)[property] != value) {
      (*m_propertyValues)[property] = value;

      if (supports(property)) {
        emit propertyChanged(this);
      }
    }
  }


  /**
   * Get a property's associated data.
   *
   * @param prop The property
   */
  QVariant DisplayProperties::getValue(int property) const {
    return (*m_propertyValues)[property];
  }


  /**
   * Output format:
   *
   * <displayProperties displayName="...">
   *   Hex-encoded data
   * </displayProperties>
   */
  void DisplayProperties::save(QXmlStreamWriter &stream, const Project *project,
                               FileName newProjectRoot) const {
    stream.writeStartElement("displayProperties");

    stream.writeAttribute("displayName", displayName());

    // Get hex-encoded data
    QBuffer dataBuffer;
    dataBuffer.open(QIODevice::ReadWrite);
    QDataStream propsStream(&dataBuffer);
    propsStream << *m_propertyValues;
    dataBuffer.seek(0);

    stream.writeCharacters(dataBuffer.data().toHex());

    stream.writeEndElement();
  }
}
