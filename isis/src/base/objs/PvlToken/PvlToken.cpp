/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlToken.h"
#include "IException.h"
#include "IString.h"
#include "Message.h"

using namespace std;
namespace Isis {
  /**
   * Constructs a Token with k for keyword and NULL for the value list.
   *
   * @param k Value of the keyword
   */
  PvlToken::PvlToken(const std::string k) {
    setKey(k);
    valueClear();
  }

  //! Constructs a Token with NULL for both the keyword and value list.
  PvlToken::PvlToken() {
    valueClear();
  }

  //! Destroys the Token object
  PvlToken::~PvlToken() {
    m_key.clear();
    valueClear();
  }

  /**
   * Set the token keyword.
   *
   * @param k std::string to load into the token keyword
   */
  void PvlToken::setKey(const std::string k) {
    m_key = k;
  };

  /**
   * Returns the token keyword
   *
   * @return std::string
   */
  std::string PvlToken::key() const {
    return m_key;
  };

  /**
   * Returns the token keyword in all uppercase characters
   *
   * @return std::string
   */
  std::string PvlToken::keyUpper() const {
    std::string keyUpper(m_key);
    std::transform(keyUpper.begin(), keyUpper.end(), keyUpper.begin(), ::toupper);
    return keyUpper;
  }

  //! Removes all elements from the value-vector
  void PvlToken::valueClear() {
    m_value.clear();
  }

  /**
   * Returns the number of elements in the value-vector
   *
   * @return int
   */
  int PvlToken::valueSize() const {
    return m_value.size();
  }

  /**
   * Adds a value to the value-vector. Successive calls add values to the end of
   * the vector.
   *
   * @param v IString add to the value-vector list
   */
  void PvlToken::addValue(const std::string v) {
    m_value.push_back(v);
  }

  /**
   * Returns one element of the value-vector.
   *
   * @param index Zero-based index of vector element to return. Defaults to 0
   *
   * @return std::string
   *
   * @throws Isis::IException::Programmer
   */
  std::string PvlToken::value(const int index) const {
    if((index < 0) || (index >= (int) m_value.size())) {
      std::string message = Isis::Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    return m_value[index];
  }

  /**
   * Returns one element of the value-vector in uppercase.
   *
   * @param index Zero-based index of vector element to return. Defaults to 0
   *
   * @return std::string
   *
   * @throws Isis::IException::Programmer
   */
  std::string PvlToken::valueUpper(int index) const {
    if((index < 0) || (index >= (int) m_value.size())) {
      std::string message = Isis::Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    
    std::string valueUpper(m_value[index]);
    std::transform(valueUpper.begin(), valueUpper.end(), valueUpper.begin(), ::toupper);
    return valueUpper;
  }
} // end namespace isis
