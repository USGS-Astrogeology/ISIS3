/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubeStretch.h"

namespace Isis {

  /**
   * Constructs a CubeStretch object with default mapping of special pixel values to
   * themselves and a provided name, and a provided stretch type
   *
   * @param name Name to use for Stretch
   * @param type Type of stretch
   */
  CubeStretch::CubeStretch(QString name, QString stretchType, int bandNumber) : m_name(name),
    m_type(stretchType), m_bandNumber(bandNumber) {
  }


  /**
   * Copy constructor for a CubeStretch
   */
  CubeStretch::CubeStretch(CubeStretch const& stretch): Stretch(stretch) {
    m_name = stretch.getName();
    m_type = stretch.getType();
    m_bandNumber = stretch.getBandNumber();
  }


  /**
   * Constructs a CubeStretch object from a normal Stretch.
   *
   * @param Stretch Stretch to construct the CubeStretch from.
   */
  CubeStretch::CubeStretch(Stretch const& stretch): Stretch(stretch) {
    m_name = "DefaultStretch";
    m_bandNumber = 1;
    m_type = "Default";
  }


  /**
   * Constructs a CubeStretch object from a normal Stretch.
   *
   * @param Stretch Stretch to construct the CubeStretch from.
   */
  CubeStretch::CubeStretch(Stretch const& stretch, QString stretchType): Stretch(stretch), m_type(stretchType) {
    m_name = "DefaultName";
    m_bandNumber = 1;
  }


  /**
   * Constructs a CubeStretch from a Blob.
   *
   * @param blob The Blob to read data from.
   */
  CubeStretch::CubeStretch(Blob blob) : Stretch() {
    char *buff = blob.getBuffer();
    std::string stringFromBuffer(buff, blob.Size());
    setName(QString::fromStdString(blob.Label()["Name"][0]));
    setType(QString::fromStdString(blob.Label()["StretchType"][0]));
    Parse(QString::fromStdString(stringFromBuffer));
    setBandNumber(std::stoi(blob.Label()["BandNumber"][0]));
  }


  // CubeStretch destructor
  CubeStretch::~CubeStretch() {
  }


  /**
   * Serialize the CubeStretch to a Blob.
   *
   * The stretch will be serialized as a string. See Stretch::Text for more information.
   *
   * @return @b Blob a Blob containing the stretch data.
   */
  Isis::Blob CubeStretch::toBlob() const {
    Isis::Blob blob("CubeStretch", "Stretch");

    blob.Label()["Name"] = getName().toStdString();
    blob.Label() += PvlKeyword("StretchType", getType().toStdString());
    blob.Label() += PvlKeyword("BandNumber", std::to_string(getBandNumber()));
    std::string blobString = Text().toStdString();
    blob.setData(blobString.c_str(), blobString.size());
    return blob;
  }


  /**
   * Check if the CubeStretches are equal
   *
   * @param stretch2 The stretch to compare with
   *
   * @return bool True if stretches are equal. Else, false.
   */
  bool CubeStretch::operator==(CubeStretch& stretch2) {
    return (getBandNumber() == stretch2.getBandNumber()) &&
           (getName() == stretch2.getName()) &&
           (Text() == stretch2.Text());
  }


  /**
   * Get the Type of Stretch.
   *
   * @return QString Type of Stretch.
   */
  QString CubeStretch::getType() const{
    return m_type;
  }


  /**
   * Set the type of Stretch.
   *
   * @param QString Type of Stretch.
   */
  void CubeStretch::setType(QString stretchType){
    m_type = stretchType;
  }


  /**
   * Set the Stretch name.
   *
   * @param QString name for stretch
   */
  void CubeStretch::setName(QString name){
    m_name = name;
  }


  /**
   * Get the Stretch name.
   *
   * @return QString name of stretch
   */
  QString CubeStretch::getName() const{
    return m_name;
  }


  /**
   * Get the band number for the stretch.
   *
   * @return int band number
   */
  int CubeStretch::getBandNumber() const{
    return m_bandNumber;
  }


  /**
   * Set the band number for the stretch.
   *
   * @param int band number
   */
  void CubeStretch::setBandNumber(int bandNumber) {
    m_bandNumber = bandNumber;
  }
} // end namespace isis
