#ifndef PixelType_h
#define PixelType_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QString>

#include "gdal_priv.h"

namespace Isis {
  /**
   * @brief Enumerations for Isis Pixel Types
   *
   * This enumeration is for defining pixel types of cubes. UnsignedByte,
   * SignedWord, and Real are the only three pixel types currently supported.
   * The range for UnsignedBytes is 0 to 255, the range for SignedWord is -32768
   * to 32767, and the range for Real is -FLT_MAX to FLT_MAX as defined in
   * the include file, float.h.
   *
   * @author 2004-10-14 Stuart Sides
   *
   * @internal
   *   @history 2005-03-01 Elizabeth Ribelin - Added documentation in Doxygen
   *                                           format
   */
  enum PixelType {
    None = 0,
    UnsignedByte,
    SignedByte,
    UnsignedWord,
    SignedWord,
    UnsignedInteger,
    SignedInteger,
    Real,
    Double
  };

  /**
   * Returns the number of bytes of the specified PixelType
   *
   * @param pixelType PixelType enumeration
   *
   * @return int Size of pixel type byte
   */
  inline int SizeOf(Isis::PixelType pixelType) {
    if(pixelType == Isis::None) return 0;
    if(pixelType == Isis::UnsignedByte) return sizeof(unsigned char);
    if(pixelType == Isis::SignedByte) return sizeof(char);
    if(pixelType == Isis::UnsignedWord) return sizeof(unsigned short);
    if(pixelType == Isis::SignedWord) return sizeof(short);
    if(pixelType == Isis::UnsignedInteger) return sizeof(unsigned int);
    if(pixelType == Isis::SignedInteger) return sizeof(int);
    if(pixelType == Isis::Real) return sizeof(float);
    if(pixelType == Isis::Double) return sizeof(double);
    return -1;
  }

  /**
   * Returns string name of PixelType enumeration entered as input parameter
   *
   * @param pixelType PixelType enumeration
   *
   * @return string Name of PixelType
   */
  inline QString PixelTypeName(Isis::PixelType pixelType) {
    if(pixelType == Isis::None) return "None";
    if(pixelType == Isis::UnsignedByte) return "UnsignedByte";
    if(pixelType == Isis::SignedByte) return "SignedByte";
    if(pixelType == Isis::UnsignedWord) return "UnsignedWord";
    if(pixelType == Isis::SignedWord) return "SignedWord";
    if(pixelType == Isis::UnsignedInteger) return "UnsignedInteger";
    if(pixelType == Isis::SignedInteger) return "SignedInteger";
    if(pixelType == Isis::Real) return "Real";
    if(pixelType == Isis::Double) return "Double";
    return "Error";
  }

  /**
   * Returns PixelType enumeration given a string
   *
   * @param type QString containing the name of pixel type. Acceptable values are
   *             UnsignedByte, SignedByte, UnsignedWord, SignedWord,
   *             UnsignedInteger, SignedInteger, Read, and Double (not case
   *             sensitive)
   *
   * @return Isis::PixelType
   */
  inline Isis::PixelType PixelTypeEnumeration(const QString &type) {
    QString temp = type.toUpper();
    if(temp == "UNSIGNEDBYTE" || temp == "8BIT" || temp == "8-BIT") return Isis::UnsignedByte;
    if(temp == "SIGNEDBYTE") return Isis::SignedByte;
    if(temp == "UNSIGNEDWORD") return Isis::UnsignedWord;
    if(temp == "SIGNEDWORD") return Isis::SignedWord;
    if(temp == "UNSIGNEDINTEGER") return Isis::UnsignedInteger;
    if(temp == "SIGNEDINTEGER") return Isis::SignedInteger;
    if(temp == "REAL") return Isis::Real;
    if(temp == "DOUBLE") return Isis::Double;
    return Isis::None;
  }

  inline Isis::PixelType GdalPixelToIsis(GDALDataType type) {
    if (type == GDT_Byte) return Isis::UnsignedByte;
    if (type == GDT_Int8) return Isis::SignedByte;
    if (type == GDT_UInt16) return Isis::UnsignedWord;
    if (type == GDT_Int16) return Isis::SignedWord;
    if (type == GDT_UInt32) return Isis::UnsignedInteger;
    if (type == GDT_Int32) return Isis::SignedInteger;
    if (type == GDT_Float32) return Isis::Real;
    if (type == GDT_Float64) return Isis::Double;
    return Isis::None;
  }

  inline GDALDataType IsisPixelToGdal(Isis::PixelType type) {
    if(type == Isis::None) return GDT_Unknown;
    if(type == Isis::UnsignedByte) return GDT_Byte;
    if(type == Isis::SignedByte) return GDT_Int8;
    if(type == Isis::UnsignedWord) return GDT_UInt16;
    if(type == Isis::SignedWord) return GDT_Int16;
    if(type == Isis::UnsignedInteger) return GDT_UInt32;
    if(type == Isis::SignedInteger) return GDT_Int32;
    if(type == Isis::Real) return GDT_Float32;
    if(type == Isis::Double) return GDT_Float64;
    return GDT_Unknown;
  }
}

#endif

