/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Pixel.h"
#include <QString>

using namespace std;

namespace Isis {

  /**
   * @brief Constructs an empty Pixel
   *
   * @author 2015-08-05 Kristin Berry
   */
  Pixel::Pixel() {
    m_line = 0;
    m_sample = 0;
    m_band = 0;
    m_DN = Isis::Null;
  }


  /**
   * @brief Constructs a Pixel given a line, sample, band, and DN.
   *
   * @author 2015-05-08 Kristin Berry
   *
   * @param line line coordinate
   * @param sample sample coordinate
   * @param band band coordinate
   * @param DN data value for the pixel
   */
  Pixel::Pixel(int sample, int line, int band, double DN) {
    m_sample = sample;
    m_line = line;
    m_band = band;
    m_DN = DN;
  }


  /**
   * @brief Constructs a Pixel, given a Pixel.
   *
   * @author 2015-08-05 Kristin Berry
   *
   * @param pixel Pixel to copy
   */
  Pixel::Pixel(const Pixel& pixel) {
    m_sample = pixel.sample();
    m_line = pixel.line();
    m_band = pixel.band();
    m_DN = pixel.DN();
  }


  /**
   * Copy assignment operator
   *
   * @param other Pixel the Pixel to be copied
   *
   * @return Pixel Pointer to self
   */
  Pixel &Pixel::operator=(const Pixel& other) {
    m_line = other.line();
    m_sample = other.sample();
    m_band = other.band();
    m_DN = other.DN();
    return *this;
  }


  //!Default destructor
  Pixel::~Pixel() {}


  //! @return int The line coordinate of the Pixel
  int Pixel::line() const {
    return m_line;
  }


  //! @return int The sample coordinate of the Pixel
  int Pixel::sample() const {
    return m_sample;
  }


  //! @return int The band coordinate of the Pixel
  int Pixel::band() const {
    return m_band;
  }


  //! @return double The DN value of the Pixel
  double Pixel::DN() const {
    return m_DN;
  }


  /**
   * Converts double pixels to unsigned char pixels with special pixel
   * translations
   *
   * @param d Double pixel value to be converted to a double
   *
   * @return unsigned char The unsigned char pixel value
   */
  unsigned char Pixel::To8Bit(const double d) {
    if(d < VALID_MIN8) {
      if(d == NULL8)                return NULL1;
      else if(d == LOW_INSTR_SAT8)  return LOW_INSTR_SAT1;
      else if(d == LOW_REPR_SAT8)   return LOW_REPR_SAT1;
      else if(d == HIGH_INSTR_SAT8) return HIGH_INSTR_SAT1;
      else if(d == HIGH_REPR_SAT8)  return HIGH_REPR_SAT1;
      else                           return LOW_REPR_SAT1;
    }
    else {
      if(d < VALID_MIN1 - 0.5)      return LOW_REPR_SAT1;
      else if(d > VALID_MAX1 + 0.5) return HIGH_REPR_SAT1;

      else {
        int itemp = (int)(d + 0.5);
        if(itemp < VALID_MIN1)      return LOW_REPR_SAT1;
        else if(itemp > VALID_MAX1) return HIGH_REPR_SAT1;
        else                         return (unsigned char)(d + 0.5);
      }
    }
  }


  /**
   * Converts internal pixel value to an unsigned char pixel with
   * special pixel translations
   *
   * @return unsigned char The unsigned char pixel value
  */
  unsigned char Pixel::To8Bit() {
    return To8Bit(m_DN);
  }


  /**
   * Converts double pixels to short int pixels with special pixel translations
   *
   * @param d Double pixel value to be converted to a double
   *
   * @return short int The short int pixel value
   */
  short int Pixel::To16Bit(const double d) {
    if(d < VALID_MIN8) {
      if(d == NULL8)                return NULL2;
      else if(d == LOW_INSTR_SAT8)  return LOW_INSTR_SAT2;
      else if(d == LOW_REPR_SAT8)   return LOW_REPR_SAT2;
      else if(d == HIGH_INSTR_SAT8) return HIGH_INSTR_SAT2;
      else if(d == HIGH_REPR_SAT8)  return HIGH_REPR_SAT2;
      else                           return LOW_REPR_SAT2;
    }
    else {
      if(d < VALID_MIN2 - 0.5)      return LOW_REPR_SAT2;
      else if(d > VALID_MAX2 + 0.5) return HIGH_REPR_SAT2;

      else {
        int itemp;
        if(d < 0.0) {
          itemp = (int)(d - 0.5);
        }
        else {
          itemp = (int)(d + 0.5);
        }

        if(itemp < VALID_MIN2)      return LOW_REPR_SAT2;
        else if(itemp > VALID_MAX2) return HIGH_REPR_SAT2;
        else if(d < 0.0)            return (short)(d - 0.5);
        else                         return (short)(d + 0.5);
      }
    }
  }


  /**
   * Converts internal pixel value to a short int pixel with
   * special pixel translations
   *
   * @return short int The short int pixel value
   */
  short int Pixel::To16Bit() {
    return To16Bit(m_DN);
  }

  /**
   * Converts double pixels to short unsigned int pixels with special pixel translations
   *
   * @param d Double pixel value to be converted to a double
   *
   * @return short unsigned int The short int pixel value
   */
  short unsigned int Pixel::To16UBit(const double d) {
    if(d < VALID_MIN8) {
      if(d == NULL8)                return NULLU2;
      else if(d == LOW_INSTR_SAT8)  return LOW_INSTR_SATU2;
      else if(d == LOW_REPR_SAT8)   return LOW_REPR_SATU2;
      else if(d == HIGH_INSTR_SAT8) return HIGH_INSTR_SATU2;
      else if(d == HIGH_REPR_SAT8)  return HIGH_REPR_SATU2;
      else                           return LOW_REPR_SATU2;
    }
    else {
      if(d < VALID_MIN2 - 0.5)      return LOW_REPR_SATU2;
      else if(d > VALID_MAX2 + 0.5) return HIGH_REPR_SATU2;

      else {
        int itemp;
        if(d < 0.0) {
          itemp = (int)(d - 0.5);
        }
        else {
          itemp = (int)(d + 0.5);
        }

        if(itemp < VALID_MIN2)      return LOW_REPR_SATU2;
        else if(itemp > VALID_MAX2) return HIGH_REPR_SATU2;
        else if(d < 0.0)            return (short)(d - 0.5);
        else                         return (short)(d + 0.5);
      }
    }
  }


  /**
   * Converts internal pixel value to a short int pixel with
   * special pixel translations
   *
   * @return short unsigned int The short int pixel value
   */
  short unsigned int Pixel::To16Ubit() {
    return To16UBit(m_DN);
  }

  /**
   * Converts double pixels to float pixels with special pixel translations
   *
   * @param d Double pixel value to be converted to a double
   *
   * @return float The float pixel value
   */
  float Pixel::To32Bit(const double d) {
    if(d < (double) VALID_MIN8) {
      if(d == NULL8)                 return(NULL4);
      else if(d == LOW_REPR_SAT8)    return(LOW_REPR_SAT4);
      else if(d == LOW_INSTR_SAT8)   return(LOW_INSTR_SAT4);
      else if(d == HIGH_INSTR_SAT8)  return(HIGH_INSTR_SAT4);
      else if(d == HIGH_REPR_SAT8)   return(HIGH_REPR_SAT4);
      else                            return(LOW_REPR_SAT4);
    }
    else if(d > (double) VALID_MAX8) return(HIGH_REPR_SAT8);
    else                              return((float) d);
  }

  /**
   * Converts internal pixel value to float with special pixel
   * translations
   *
   * @return float The float pixel value
   */
  float Pixel::To32Bit() {
    return To32Bit(m_DN);
  }


  /**
   * Converts unsigned char pixels to double pixels with special pixel
   * translations
   *
   * @param d Unsigned char pixel value to be converted to a double
   *
   * @return double The double pixel value
   */
  double Pixel::ToDouble(const unsigned char d) {
    if(d < VALID_MIN1) {
      if(d == NULL1)                return(NULL8);
      else if(d == LOW_REPR_SAT1)   return(LOW_REPR_SAT8);
      else if(d == LOW_INSTR_SAT1)  return(LOW_INSTR_SAT8);
      else if(d == HIGH_REPR_SAT1)  return(HIGH_REPR_SAT8);
      else if(d == HIGH_INSTR_SAT1) return(HIGH_INSTR_SAT8);
      else                           return(LOW_REPR_SAT8);
    }
    else if(d > VALID_MAX1)         return(HIGH_REPR_SAT8);
    else                             return((double) d);
  }


  /**
   * Converts short int pixels to double pixels with special pixel translations
   *
   * @param d Short int pixel value to be converted to a double
   *
   * @return double The double pixel value
   */
  double Pixel::ToDouble(const short int d) {
    if(d < VALID_MIN2) {
      if(d == NULL2)                 return(NULL8);
      else if(d == LOW_REPR_SAT2)    return(LOW_REPR_SAT8);
      else if(d == LOW_INSTR_SAT2)   return(LOW_INSTR_SAT8);
      else if(d == HIGH_REPR_SAT2)   return(HIGH_REPR_SAT8);
      else if(d == HIGH_INSTR_SAT2)  return(HIGH_INSTR_SAT8);
      else                            return(LOW_REPR_SAT8);
    }
    else                              return((double) d);
  }

  /**
   * Converts short unsigned int pixels to double pixels with special pixel translations
   *
   * @param d Short unsigned int pixel value to be converted to a double
   *
   * @return double The double pixel value
   */
  double Pixel::ToDouble(const short unsigned int d) {
    if(d < VALID_MINU2) {
      if(d == NULLU2)                 return(NULL8);
      else if(d == LOW_REPR_SATU2)    return(LOW_REPR_SAT8);
      else if(d == LOW_INSTR_SATU2)   return(LOW_INSTR_SAT8);
      else if(d == HIGH_REPR_SATU2)   return(HIGH_REPR_SAT8);
      else if(d == HIGH_INSTR_SATU2)  return(HIGH_INSTR_SAT8);
      else                            return(LOW_REPR_SAT8);
    }
    else                              return((double) d);
  }


  /**
   * Converts float pixels to double pixels with special pixel translations
   *
   * @param d Float pixel value to be converted to a double
   *
   * @return double The double pixel value
   */
  double Pixel::ToDouble(const float d) {
    if(d < VALID_MIN4) {
      if(d == NULL4)                return(NULL8);
      else if(d == LOW_REPR_SAT4)   return(LOW_REPR_SAT8);
      else if(d == LOW_INSTR_SAT4)  return(LOW_INSTR_SAT8);
      else if(d == HIGH_REPR_SAT4)  return(HIGH_REPR_SAT8);
      else if(d == HIGH_INSTR_SAT4) return(HIGH_INSTR_SAT8);
      else                           return(LOW_REPR_SAT8);
    }
    else if(d > VALID_MAX4)         return(HIGH_REPR_SAT8);
    else                             return((double) d);
  }


  /**
   * Converts stored pixel value to a double.
   *
   * @return double The double pixel value
   */
  double Pixel::ToDouble() {
    return m_DN;
  }


  /**
   * Converts unsigned char to float with pixel translations
   * and care for overflows (underflows are assumed to cast to
   * 0!)
   *
   * @param t Unsigned char pixel value to be converted to a
   *          float
   *
   * @return float The float pixel value
   */
  float Pixel::ToFloat(const unsigned char t) {
    if(t < (double) VALID_MIN1) {
      if(t == NULL1)                 return(NULL4);
      else if(t == LOW_REPR_SAT1)    return(LOW_REPR_SAT4);
      else if(t == LOW_INSTR_SAT1)   return(LOW_INSTR_SAT4);
      else if(t == HIGH_INSTR_SAT1)  return(HIGH_INSTR_SAT4);
      else if(t == HIGH_REPR_SAT1)   return(HIGH_REPR_SAT4);
      else                            return(LOW_REPR_SAT4);
    }
    else if(t > (double) VALID_MAX1) return(HIGH_REPR_SAT8);
    else                              return((float) t);
  }

  /**
   * Converts short int to float with pixel translations and
   * care for overflows (underflows are assumed to cast to 0!)
   *
   * @param t Short int pixel value to be converted to a float
   *
   * @return float The float pixel value
   */
  float Pixel::ToFloat(const short int t) {
    if(t < (double) VALID_MIN2) {
      if(t == NULL2)                 return(NULL4);
      else if(t == LOW_REPR_SAT2)    return(LOW_REPR_SAT4);
      else if(t == LOW_INSTR_SAT2)   return(LOW_INSTR_SAT4);
      else if(t == HIGH_INSTR_SAT2)  return(HIGH_INSTR_SAT4);
      else if(t == HIGH_REPR_SAT2)   return(HIGH_REPR_SAT4);
      else                            return(LOW_REPR_SAT4);
    }
    else if(t > (double) VALID_MAX2) return(HIGH_REPR_SAT8);
    else                              return((float) t);
  }

  /**
   * Converts short unsigned int to float with pixel translations and
   * care for overflows (underflows are assumed to cast to 0!)
   *
   * @param t Short unsigned int pixel value to be converted to a float
   *
   * @return float The float pixel value
   */
  float Pixel::ToFloat(const short unsigned int t) {
    if(t < (double) VALID_MINU2) {
      if(t == NULLU2)                 return(NULL4);
      else if(t == LOW_REPR_SATU2)    return(LOW_REPR_SAT4);
      else if(t == LOW_INSTR_SATU2)   return(LOW_INSTR_SAT4);
      else if(t == HIGH_INSTR_SATU2)  return(HIGH_INSTR_SAT4);
      else if(t == HIGH_REPR_SATU2)   return(HIGH_REPR_SAT4);
      else                            return(LOW_REPR_SAT4);
    }
    else if(t > (double) VALID_MAXU2) return(HIGH_REPR_SAT8);
    else                              return((float) t);
  }

  /**
   * Converts double to float with pixel translations and
   * care for overflows (underflows are assumed to cast to 0!)
   *
   * @param t Double pixel value to be converted to a float
   *
   * @return float The float pixel value
   */
  float Pixel::ToFloat(const double t) {
    if(t < (double) VALID_MIN8) {
      if(t == NULL8)                 return(NULL4);
      else if(t == LOW_REPR_SAT8)    return(LOW_REPR_SAT4);
      else if(t == LOW_INSTR_SAT8)   return(LOW_INSTR_SAT4);
      else if(t == HIGH_INSTR_SAT8)  return(HIGH_INSTR_SAT4);
      else if(t == HIGH_REPR_SAT8)   return(HIGH_REPR_SAT4);
      else                            return(LOW_REPR_SAT4);
    }
    else if(t > (double) VALID_MAX8) return(HIGH_REPR_SAT8);
    else                              return((float) t);
  }

  /**
   * Converts internal pixel value to float with pixel
   * translations and care for overflows (underflows are assumed
   * to cast to 0!)
   *
   * @return float The float pixel value
   */
  float Pixel::ToFloat() {
    return ToFloat(m_DN);
  }

  /**
   * Takes a double pixel value and returns the name of the pixel type as a
   * string
   *
   * @param d Pixel value
   *
   * @return string The name of the pixel type
   */
  string Pixel::ToString(double d) {
    if(IsSpecial(d)) {
      if(IsNull(d))     return string("Null");
      else if(IsLrs(d)) return string("Lrs");
      else if(IsHrs(d)) return string("Hrs");
      else if(IsHis(d)) return string("His");
      else if(IsLis(d)) return string("Lis");
      else               return string("Invalid");
    }

    QString result;
    return result.setNum(d).toStdString();
  }

 /**
   * Returns the name of the pixel type as a string
   *
   * @return string The name of the pixel type
   */
  string Pixel::ToString() {
    return ToString(m_DN);
  }
}
