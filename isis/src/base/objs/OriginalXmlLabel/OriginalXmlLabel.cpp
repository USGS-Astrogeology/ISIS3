/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <fstream>
#include <sstream>
#include <string>

#include <QByteArray>
#include <QFile>

#include "OriginalXmlLabel.h"
#include "Application.h"
#include "PvlObject.h"

using namespace std;
namespace Isis {
  /**
   * Constructors a default OriginalXmlLabel with an empty label.
   */
  OriginalXmlLabel::OriginalXmlLabel() {
  }


  /**
   * Constructs an OriginalXmlLabel from a cube label file.
   *
   * @param file Xml file to read labels from
   */
  OriginalXmlLabel::OriginalXmlLabel(const QString &file) {
    Blob blob = Blob("IsisCube", "OriginalXmlLabel");
    blob.Read(file);
    fromBlob(blob);
  }

  /**
   * Constructs an OriginalXmlLabel from a blob
   *
   * @param blob Blob from which to create the OriginalXmlLabel
   */
  OriginalXmlLabel::OriginalXmlLabel(Isis::Blob &blob) {
    fromBlob(blob);
  }


  /**
   * Destructor
   */
  OriginalXmlLabel::~OriginalXmlLabel() {
  }


  /*
   * Load blob data into m_originalLabel
   */
  void OriginalXmlLabel::fromBlob(Isis::Blob blob) {
    QString errorMessage;
    int errorLine = 0;
    int errorColumn = 0;

    if ( !m_originalLabel.setContent( QByteArray(blob.getBuffer(), blob.Size()) ) ) {
      QString msg = "XML read/parse error when parsing original label. "
                    "Error at line [" + toString(errorLine) +
                    "], column [" + toString(errorColumn) +
                    "]. Error message: " + errorMessage;
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }


  /**
   * Serialize the OriginalXmlLabel to a Blob.
   *
   * @return @b Blob
   */
  Blob OriginalXmlLabel::toBlob() const {
    std::stringstream sstream;
    sstream << m_originalLabel.toString();
    string orglblStr = sstream.str();
    Isis::Blob blob("IsisCube", "OriginalXmlLabel");
    blob.setData((char*)orglblStr.data(), orglblStr.length());
    blob.Label() += Isis::PvlKeyword("ByteOrder", "NULL");
    if (Isis::IsLsb()) {
      blob.Label()["ByteOrder"] = Isis::ByteOrderName(Isis::Lsb).toStdString();
    }
    else {
      blob.Label()["ByteOrder"] = Isis::ByteOrderName(Isis::Msb).toStdString();
    }
    return blob;
  }


  /**
   * Read the original label from an Xml file.
   *
   * @param FileName The Xml file containing the original label.
   *
   * @throws IException::Io "Could not open label file."
   * @throws IException::Unknown "XML read/parse error in file."
   */
  void OriginalXmlLabel::readFromXmlFile(const FileName &xmlFileName) {
    QFile xmlFile(xmlFileName.expanded());
     if ( !xmlFile.open(QIODevice::ReadOnly) ) {
       QString msg = "Could not open label file [" + xmlFileName.expanded() +
                     "].";
       throw IException(IException::Io, msg, _FILEINFO_);
     }

     QString errmsg;
     int errline, errcol;
     if ( !m_originalLabel.setContent(&xmlFile, false, &errmsg, &errline, &errcol) ) {
       xmlFile.close();
       QString msg = "XML read/parse error in file [" + xmlFileName.expanded()
            + "] at line [" + toString(errline) + "], column [" + toString(errcol)
            + "], message: " + errmsg;
       throw IException(IException::Unknown, msg, _FILEINFO_);
     }

     xmlFile.close();
  }


  /**
   * Returns the original Xml label.
   *
   * @return @b QDomDocument The parsed original label
   */
  const QDomDocument &OriginalXmlLabel::ReturnLabels() const{
    return m_originalLabel;
  }
}
