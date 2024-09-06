#ifndef EndianSwapper_h
#define EndianSwapper_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "IException.h"
#include <QString>

namespace Isis {

  /**
   * @brief Byte swapper
   *
   * This class is used to swap bytes on data that is from a different machine
   * architecture.
   *
   * @ingroup Utility
   *
   * @author 2002-07-10 Tracie Sucharski
   *
   * @internal
   *   @todo This class needs an example.
   *   @history 2003-05-16 Stuart Sides modified schema from
   *   astrogeology...isis.astrogeology.
   *   @history 2004-03-18 Stuart Sides used Endian.h instead of the linux gcc
   *   endian.h to figure the system's endian type.
   *   @history 2008-08-14 Christopher Austin - Added ExportFloat() for exporting
   *   real data to the non-native endians. i.e. exporting to msb on a lsb system
   *   @history 2009-04-16 Steven Lambright - Added Int and LongLongInt. Long was
   *            not added because it is 4 bytes on 32-bit linux and 8 bytes on
   *            64-bit linux.
   *   @history 2016-04-21 Makayla Shepherd - Added UnsignedWord pixel type handling.
   *   @history 2018-01-29 Adam Goins - Added uint32_t behavior to EndianSwapper.
   */
  class EndianSwapper {
    private:
      //! Indicates whether bytes need to be swapped.
      bool p_needSwap;
      /**
       *  Indicates which direction to increment the pointer for swapping.
       *  (Possible values: -1,1)
       */
      int p_swapDirection;

      /**
       * Union containing the output double precision value, floating point
       * value, short integer value, unsigned short integer value and
       * byte format - all with swapped bytes.
       */
      union {
        //! Union containing the output uint32_t value with swapped bytes.
        uint32_t p_uint32;
        //! Union containing the output double precision value with swapped bytes.
        double p_double;
        //! Union containing the output floating point value with swapped bytes.
        float p_float;
        //! Union containing the output 4 byte integer value with swapped bytes.
        int p_int;
        //! Union containing the output 8 byte integer value with swapped bytes.
        long long int p_longLongInt;
        //! Union containing the output 2 byte integer value with swapped bytes.
        short int p_shortInt;
        /**
         * Union containing the output unsigned short integer value with swapped
         * bytes.
         */
        unsigned short int p_uShortInt;
        //! Union containing the output value in byte format.
        char p_char[8];
      } p_swapper;

    public:
      EndianSwapper(QString inputEndian);
      ~EndianSwapper();
      double Double(void *buf);
      float Float(void *buf);
      int ExportFloat(void *buf);
      int Int(void *buf);
      uint32_t Uint32_t(void *buf);
      long long int LongLongInt(void *buf);
      short int ShortInt(void *buf);
      unsigned short int UnsignedShortInt(void *buf);
      bool willSwap() const {
        return p_needSwap;
      }
  };
};

#endif
