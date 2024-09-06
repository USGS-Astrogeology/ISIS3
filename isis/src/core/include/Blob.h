#ifndef Blob_h
#define Blob_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>

#include "PvlObject.h"

namespace Isis {
  bool IsBlob(PvlObject &obj);
  class Pvl;
  /**
   * @brief Interface for pvl cube data reading and write.
   *
   * @author ????-??-?? Unknown
   *
   * @internal
   *   @history 2006-04-04 Jeff Anderson Class was not overwriting existing
   *                           blobs correctly.
   *   @history 2007-02-20 Elizabeth Miller Fixed bug with
   *                           OriginalLabel naming and modified to be backwards
   *                           compatible
   *   @history ????-??-?? Steven Lambright Added copy constructor,
   *                           assignment operator
   *   @history 2010-05-15 Steven Lambright Changed Read to use an
   *                           istream instead of an fstream
   *   @history 2011-05-25 Janet Barrett and Steven Lambright Added a Read
   *                           method that takes the pvl labels so they do not
   *                           have to be re-read, which is a very expensive
   *                           operation.
   *   @history 2012-10-04 Jeannie Backer Added include due to forward
   *                           declaration in TableField. Ordered includes and
   *                           added forward declaration. Fixed header
   *                           definition statement. Moved method implementation
   *                           to cpp and reordered methods in cpp. Added
   *                           documentation. Improved test coverage in all
   *                           categories. Added padding to control statements.
   *                           References #1169.
   *   @history 2013-01-11 Steven Lambright and Tracie Sucharski - Fixed support of writing blobs
   *                           in very large cubes.  This was caused by calling the wrong number
   *                           to string conversion function.  Introduced when refactoring the
   *                           IString class.  Fixes #1388.
   *
   * @todo Write class description, history, etc.
   */
  class Blob {
    public:
      Blob(const std::string &name, const std::string &type);
      Blob(const std::string &name, const std::string &type,
           const std::string &file);
      Blob(const Blob &other);
      Blob() = default;
      Blob &operator=(const Blob &other);

      virtual ~Blob();

      std::string Type() const;
      std::string Name() const;
      int Size() const;
      PvlObject &Label();

      void Read(const std::string &file, const std::vector<PvlKeyword>
                keywords=std::vector<PvlKeyword>());
      void Read(const std::string &file, const Pvl &pvlLabels,
                const std::vector<PvlKeyword> keywords = std::vector<PvlKeyword>());
      virtual void Read(const Pvl &pvl, std::istream &is,
                        const std::vector<PvlKeyword> keywords = std::vector<PvlKeyword>());

      void Write(const std::string &file);
      void Write(Pvl &pvl, std::fstream &stm,
                 const std::string &detachedFileName = "", bool overwrite=true);


      char *getBuffer();
      void setData(const char *buffer, int nbytes);
      void takeData(char *buffer, int nbytes);

    protected:
      void Find(const Pvl &pvl, const std::vector<PvlKeyword> keywords = std::vector<PvlKeyword>());
      virtual void ReadInit();
      virtual void ReadData(std::istream &is);
      virtual void WriteInit();
      virtual void WriteData(std::fstream &os);

      PvlObject p_blobPvl;     //!< Pvl Blob object
      std::string p_blobName;  //!< Name of the Blob object

      char *p_buffer;          //!< Buffer blob data is stored in
      BigInt p_startByte;      //!< Byte blob data starts at in buffer
      int p_nbytes;            //!< Size of blob data (in bytes)
      std::string p_type;      //!< Type of data stored in the buffer
      std::string p_detached;  //!< Used for reading detached blobs
      std::string p_labelFile; //!< The file containing the labels
  };
};

#endif
