#ifndef Endian_h
#define Endian_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IString.h"

namespace Isis {
  /**
   * @brief Tests the current architecture for byte order
   *
   * Allows ISIS applications and objects to test the architecture's byte order.
   * Little Endian or Big Endian. Note: Middle Endian is not supported at this
   * time.
   *
   * @ingroup Utility
   *
   * @author 2002-12-01 Tracie Sucharski
   *
   * @internal
   *   @history 2003-02-11 Stuart Sides - Documented and created an object
   *                                      unittest
   *   @history 2003-05-16 Stuart Sides - Modified schema from astrogeology...
   *                                      isis.astrogeology...
   *   @history 2003-10-03 Stuart Sides - Added the byte order enumeration and
   *                                      functions to convert the enumeration
   *                                      to a string and from a string to an
   *                                      enumeration. Also deprecated the
   *                                      LittleEndian and BigEndian functions.
   *                                      They were replaced by Lsb and Msb
   *                                      functions.
   *   @history 2004-03-18 Stuart Sides - Added to macros for case where we need
   *                                      to know at compile time if a system is
   *                                      LSB or MSB. The macros are
   *                                      ISIS_LITTLE_ENDIAN and ISIS_BIG_ENDIAN
   */

  enum ByteOrder {
    NoByteOrder = 0,
    Lsb,
    Msb
  };

  inline std::string ByteOrderName(Isis::ByteOrder byteOrder) {
    if(byteOrder == Isis::NoByteOrder) return "None";
    if(byteOrder == Isis::Lsb) return "Lsb";
    if(byteOrder == Isis::Msb) return "Msb";
    return "Error";
  }

  inline Isis::ByteOrder ByteOrderEnumeration(const std::string &order) {
    std::string temp = IString::UpCase(order);
    if(temp == "LSB") return Isis::Lsb;
    if(temp == "MSB") return Isis::Msb;
    return Isis::NoByteOrder;
  }

  /**
   * Return true if this host is an LSB first machine and false if it is not
   *
   * @return bool - Returns true if host is LSB and false if it is MSB
   */
  inline bool IsLsb() {
    union {
      short a;
      char b[2];
    } test;

    test.a = 1;
    if(test.b[0] == 0) return false;
    return true;
  }

  /**
   * Return true if this host is an MSB first machine and false if it is not
   *
   * @return bool - Returns true if host is MSB and false if it is LSB
   */
  inline bool IsMsb() {
    return !Isis::IsLsb();
  }

  /**
   * @deprecated
   *
   * Test the architecture the application is running on.
   *
   * @return True if it's LittleEndian, False if it's not.
   */
  inline bool IsLittleEndian() {
    return IsLsb();
  }

  /**
   * @deprecated
   *
   * Test the architecture the application is running on.
   *
   * @return True if it's BigEndian, False if it's not.
   */
  inline bool IsBigEndian() {
    return !Isis::IsLsb();
  }
}

#endif

