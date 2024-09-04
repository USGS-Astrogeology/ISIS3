/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <fstream>
#include <sstream>
#include <string>
#include "OriginalLabel.h"
#include "Application.h"
#include "PvlObject.h"

using namespace std;
namespace Isis {
  /**
   * Constructor for creating an original blob with a given name
   */
  OriginalLabel::OriginalLabel() {
    m_originalLabel.setTerminator("");
  }


  /**
   * Constructor for creating an original label from a blob object.
   */
  OriginalLabel::OriginalLabel(Isis::Blob &blob) {
    fromBlob(blob);
  }


  /**
   * Constructor for creating an original blob with a given name and file to
   * read labels from.
   *
   * @param file File to read labels from
   */
  OriginalLabel::OriginalLabel(const QString &file){
    Blob blob = Blob("IsisCube", "OriginalLabel");
    blob.Read(file.toStdString());
    fromBlob(blob);
  }


  /**
   * Constructor for creating an original blob with a given name and Pvl
   * container.
   *
   * @param pvl  Pvl containing labels of the source
   */
  OriginalLabel::OriginalLabel(Pvl pvl){
    m_originalLabel = pvl;
  }

  // Destructor
  OriginalLabel::~OriginalLabel() {
  }


  /**
   * Initialize the OriginalLabel from a Blob.
   *
   * @param blob The Blob to extract data from
   */
  void OriginalLabel::fromBlob(Isis::Blob blob) {
    Pvl pvl;
    stringstream os;
    char *buff = blob.getBuffer();
    for(int i = 0; i < blob.Size(); i++){
      os << buff[i];
    }
    os >> pvl;
    m_originalLabel = pvl;
  }


  /**
   * Serialize the OriginalLabel data to a Blob.
   *
   * @return @b Blob
   */
  Isis::Blob OriginalLabel::toBlob() {
    std::stringstream sstream;
    sstream << m_originalLabel;
    string orglblStr = sstream.str();
    Isis::Blob blob("IsisCube", "OriginalLabel");
    blob.setData(orglblStr.c_str(), orglblStr.size());
    return blob;
  }


  /**
   * Returns the labels in a Pvl object
   *
   * @return (Isis::Pvl) original labels
   */
  Pvl OriginalLabel::ReturnLabels() const {
    return m_originalLabel;
  }
}
