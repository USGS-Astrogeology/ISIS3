/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessImport.h"

#include <float.h>
#include <iostream>
#include <QString>
#include <sstream>

#include "Application.h"
#include "BoxcarCachingAlgorithm.h"
#include "Brick.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "IString.h"
#include "JP2Decoder.h"
#include "LineManager.h"
#include "PixelType.h"
#include "Process.h"
#include "Pvl.h"
#include "SpecialPixel.h"
#include "UserInterface.h"

#define EXPONENT_MASK ((char) 0x7F)


using namespace std;
namespace Isis {

  //! Constructs an Import object.
  ProcessImport::ProcessImport() : Isis::Process() {

    p_progress->SetText("Importing");

    //  Initialize input file information
    p_inFile = "";
    p_pixelType = Isis::None;
    p_suffixPixelType = Isis::None;
    p_ns = p_nl = p_nb = 0;
    p_byteOrder = Isis::NoByteOrder;
    p_fileHeaderBytes = 0;
    p_dataHeaderBytes = 0;
    p_dataTrailerBytes = 0;
    p_dataPreBytes = 0;
    p_dataPostBytes = 0;
    p_suffixData = 0;
    p_organization = ProcessImport::BSQ;

    p_base.push_back(0.0);
    p_mult.push_back(1.0);
    // Make all special pixels invalid
    SetNull(DBL_MAX, -DBL_MAX);
    SetHRS(DBL_MAX, -DBL_MAX);
    SetLRS(DBL_MAX, -DBL_MAX);
    SetHIS(DBL_MAX, -DBL_MAX);
    SetLIS(DBL_MAX, -DBL_MAX);

    p_saveFileHeader = false;
    p_saveDataHeader = false;
    p_saveDataTrailer = false;
    p_saveDataPre = false;
    p_saveDataPost = false;
    p_saveFileTrailer = false;
    p_vax_convert = false;

    p_fileHeader = NULL;
    p_fileTrailer = NULL;
  }


  //!  Destroys the Import object
  ProcessImport::~ProcessImport() {
    if (p_fileHeader) {
      delete [] p_fileHeader;
      p_fileHeader = NULL;
    }

    for(unsigned int i = 0; i < p_dataHeader.size(); i++) {
      delete [] p_dataHeader[i];
    }

    for(unsigned int i = 0; i < p_dataTrailer.size(); i++) {
      delete [] p_dataTrailer[i];
    }

    vector<char *> temp;
    for(unsigned int j = 0; j < p_dataPre.size(); j++) {
      temp = p_dataPre[j];
      for(unsigned int i = 0; i < temp.size(); i++) delete [] temp[i];
    }

    for(unsigned int j = 0; j < p_dataPost.size(); j++) {
      temp = p_dataPost[j];
      for(unsigned int i = 0; i < temp.size(); i++) delete [] temp[i];
    }

    if (p_fileTrailer) {
      delete p_fileTrailer;
    }
  }


  /**
   * Determines if the VAX encoded pixel value is special or not
   *
   * @param vax A pointer to a VAX pixel
   * @param pix An enumerated value indicating the VAX pixel type
   * @return bool Returns true if the bit pattern for the enumerated
   *         type matches that pointed to by vax.  Returns false
   *         otherwise.
   */
  bool ProcessImport::IsVAXSpecial(unsigned int *vax, VAXSpecialPixel pix) {

    unsigned int VAX_NULL = 0xFFFFFFFF;
    unsigned int VAX_MIN  = 0xFFEFFFFF;
    unsigned int VAX_LRS  = 0xFFFEFFFF;
    unsigned int VAX_LIS  = 0xFFFDFFFF;
    unsigned int VAX_HIS  = 0xFFFCFFFF;
    unsigned int VAX_HRS  = 0xFFFBFFFF;
    int n;

    unsigned int x;
    memcpy(&x, vax, sizeof(unsigned int));

    switch(pix) {
      case VAX_NULL4:
        n = memcmp(&VAX_NULL, &x, sizeof(unsigned int));
        if (n == 0) return true;
        break;
      case VAX_LRS4:
        n = memcmp(&VAX_LRS, &x, sizeof(unsigned int));
        if (n == 0) return true;
        break;
      case VAX_LIS4:
        n = memcmp(&VAX_LIS, &x, sizeof(unsigned int));
        if (n == 0) return true;
        break;
      case VAX_HIS4:
        n = memcmp(&VAX_HIS, &x, sizeof(unsigned int));
        if (n == 0) return true;
        break;
      case VAX_HRS4:
        n = memcmp(&VAX_HRS, &x, sizeof(unsigned int));
        if (n == 0) return true;
        break;
      case VAX_MIN4:
        n = memcmp(&VAX_MIN, &x, sizeof(unsigned int));
        if (n == 0) return true;
        break;
      default:
        return false;
      }

    return false;

 }


  /**
   * Conversion routine which translates VAX_REAL to IEEE_REAL
   *
   * @param ibuf Memory buffer of input data to be converted
   * @return double the converted value
   */
  double ProcessImport::VAXConversion(void *ibuf) {

    float result;
    double dresult;
    bool swap_bytes = false;
    bool swap_words = true;
    int exp_adjust = -1;
    int exp_mask = 0;
    int exp_word = 1;
    int exp_byte;
    Isis::ByteOrder in_order = p_byteOrder;
    Isis::ByteOrder out_order;

    unsigned int *oli, *ili;   //4-byte buffer io ptrs
    unsigned short *osi;       //2-byte buffer io ptrs
    char *oci;                 //1-byte buffer io ptrs

    if ( Isis::IsLsb() ) {
      exp_byte  = 1;
      out_order = Isis::Lsb;
    }
    else {
      exp_byte = 0;
      out_order = Isis::Msb;
    }

    if (in_order != out_order) {
      swap_bytes =true;
    }

    oli = (unsigned int * ) ibuf;
    ili = (unsigned int * ) ibuf;

    if (IsVAXSpecial(oli, ProcessImport::VAX_NULL4) ) {
      return Isis::NULL8;
    }

    if (IsVAXSpecial(oli, ProcessImport::VAX_LIS4) ) {
      return Isis::LOW_INSTR_SAT8;
    }

    if (IsVAXSpecial(oli, ProcessImport::VAX_LRS4) ) {
      return Isis::LOW_REPR_SAT8;
    }

    if (IsVAXSpecial(oli, ProcessImport::VAX_HIS4) ) {
      return Isis::HIGH_INSTR_SAT8;
    }

    if (IsVAXSpecial(oli, ProcessImport::VAX_HRS4) ) {
      return Isis::HIGH_REPR_SAT8;
    }

    if (IsVAXSpecial(oli, ProcessImport::VAX_MIN4) ) {
      return Isis::VALID_MIN8;
    }

    //test for word swapping
    if (swap_words) {
      *oli = (*ili <<16) | (*ili >> 16);
    }

    osi = (unsigned short* ) oli;

    //test for byte swapping

    if (swap_bytes) {
      osi[0] = (osi[0] >> 8 ) | (osi[0] << 8);
      osi[1] = (osi[1] >> 8 ) | (osi[1] << 8);
    }

    //Isolate the exponent and do the conversion
    oci = (char *) &osi[exp_word];

    if ( (oci[exp_byte] & EXPONENT_MASK) != exp_mask) {
      oci[exp_byte] += exp_adjust;
    }

    result = *(float *)oli;
    dresult = static_cast<double>(result);
    return dresult;

   }


  /**
   * Sets the pixel type of the input file.
   *
   * @param type The enumerated pixel type Isis::Real, Isis::SignedWord, or
   *             Isis::UnsignedByte. This must be invoked prior to setOutputCube
   *
   * @throws Isis::iException::Message "Unsupported pixel type."
   */
  void ProcessImport::SetPixelType(const Isis::PixelType type) {

    if ((type == Isis::Double) || (type == Isis::Real) || (type == Isis::SignedWord) ||
        (type == Isis::UnsignedWord) || (type == Isis::UnsignedByte) ||
        (type == Isis::SignedInteger) || type==Isis::UnsignedInteger) {
      p_pixelType = type;
    }
    else {
      QString msg = "Unsupported pixel type [" +
                   Isis::PixelTypeName(type) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }

  void ProcessImport::SetSuffixPixelType(const Isis::PixelType type) {

    if ((type == Isis::Double) || (type == Isis::Real) || (type == Isis::SignedWord) ||
        (type == Isis::UnsignedWord) || (type == Isis::UnsignedByte)) {
      p_suffixPixelType = type;
    }
    else {
      QString msg = "Unsupported pixel type [" +
                   Isis::PixelTypeName(type) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Sets the physical size of the input cube. This must be invoked prior to
   * setOutputCube.
   *
   * @param ns Number of samples
   *
   * @param nl Number of lines
   *
   * @param nb Number of bands
   *
   * @throws Isis::iException::Message "Illegal dimension."
   */
  void ProcessImport::SetDimensions(const int ns, const int nl, const int nb) {
    if (ns > 0 && nl > 0 && nb > 0) {
      p_ns = ns;
      p_nl = nl;
      p_nb = nb;
    }
    else {
      QString msg = "Illegal dimension [" + toString(ns) + ", " +
                   toString(nl) + ", " + toString(nb) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Sets the byte order of the input file. This must be invoked prior to
   * SetOutputCube.
   *
   * @param order The enumerated byte order Isis::Lsb or Isis::Msb
   */
  void ProcessImport::SetByteOrder(const Isis::ByteOrder order) {
    p_byteOrder = order;
  }


  /**
   * This method sets the number of bytes in the header of a file.
   * The file header is a block of non-image data at the beginning
   * of the entire file.  This does not include any section
   * headers, such as band headers or line prefixes. Typically it
   * is used to delineate the boundary of the binary or ascii
   * labels. It must be called before StartProcess(). This data
   * will only be available if saved via SaveFileHeader() and may
   * be accessed via FileHeader() after StartProcess() has
   * returned and before EndProcess() is called. If not invoked
   * the default is 0.
   *
   * @param bytes Number of bytes
   *
   * @throws Isis::iException::Message "Illegal file header size."
   *             (when bytes < 0)
   * @see SaveFileHeader()
   * @see FileHeader()
   *
   */
  void ProcessImport::SetFileHeaderBytes(const int bytes) {
    if (bytes >= 0) {
      p_fileHeaderBytes = bytes;
    }
    else {
      QString msg = "Illegal file header size [" + toString(bytes) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
  * This method sets the number of bytes in the trailer of a file.
  * The trailer is a block of non-image data at the end of the
  * entire file.   This does not include any section trailers,
  * such as band trailers or line suffixes.  It must be called
  * before StartProcess(). This data will only be available if
  * saved via SaveFileTrailer() and may be accessed via
  * FileTrailer() after StartProcess() has returned and before
  * EndProcess() is called. If not invoked the default is 0.
  *
  * @param bytes Number of bytes
  *
  * @throws Isis::iException::Message "Illegal file trailer size."
  *             (when bytes < 0)
  * @see SaveFileTrailer()
  * @see FileTrailer()
  * @history 2008-08-12 Jeannie Walldren, Method created
  *
  */
  void ProcessImport::SetFileTrailerBytes(const int bytes) {
    if (bytes >= 0) {
      p_fileTrailerBytes = bytes;
    }
    else {
      QString msg = "Illegal file trailer size [" + toString(bytes) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method sets the number of bytes in the header of each
   * datablock of a file. A data header is a block of non-image
   * data at the beginning of each data block (for BSQ data blocks
   * are bands).  This is not included in the file header and does
   * not include any record headers, such as line prefixes.  It
   * must be called before StartProcess(). This data will only be
   * available if saved via SaveDataHeader() and may be accessed
   * via DataHeader() after StartProcess() has returned and before
   * EndProcess() is called. If not invoked the default is 0.
   *
   * @param bytes Number of bytes
   *
   * @throws Isis::iException::Message "Illegal data header size."
   *             (when bytes < 0)
   * @see SaveDataHeader()
   * @see DataHeader()
   */
  void ProcessImport::SetDataHeaderBytes(const int bytes) {
    if (bytes >= 0) {
      p_dataHeaderBytes = bytes;
    }
    else {
      QString msg = "Illegal data header size [" + toString(bytes) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  void ProcessImport::SetSuffixOffset(int samples, int lines, int coreBands, int itemBytes) {
    p_suffixData = samples*lines*coreBands*itemBytes;
  }


  /**
   * This method sets the number of bytes in the trailer of each
   * datablock of a file. A data trailer is a block of non-image
   * data at the end of each data block (for BSQ data blocks
   * are bands).  This is not included in the file trailer and
   * does not include any record trailers, such as line suffixes.
   * It must be called before StartProcess(). This data will only
   * be available if saved via SaveDataTrailer() and may be
   * accessed via DataTrailer() after StartProcess() has returned
   * and before EndProcess() is called. If not invoked the default
   * is 0.
   *
   * @param bytes Number of bytes
   *
   * @throws Isis::iException::Message "Illegal data trailer
   *             size." (when bytes < 0)
   * @see SaveDataTrailer()
   * @see DataTrailer()
   */
  void ProcessImport::SetDataTrailerBytes(const int bytes) {

    if (bytes >= 0) {
      p_dataTrailerBytes = bytes;
    }
    else {
      QString msg = "Illegal data trailer size [" + toString(bytes) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method sets the number of bytes at the beginning of each
   * data record of a file. A data prefix is a block of non-image
   * data at the beginning of each data record (for BSQ data
   * records are lines).  This is not included in the file header
   * or the data header. It must be called before StartProcess().
   * This data will only be available if saved via
   * SaveDataPrefix() and may be accessed via DataPrefix() after
   * StartProcess() has returned and before EndProcess() is
   * called. If not invoked the default is 0.
   *
   * @param bytes Number of bytes
   *
   * @throws Isis::iException::Message "Illegal data prefix size."
   *             (when bytes < 0)
   * @see SaveDataPrefix()
   * @see DataPrefix()
   */
  void ProcessImport::SetDataPrefixBytes(const int bytes) {

    if (bytes >= 0) {
      p_dataPreBytes = bytes;
    }
    else {
      QString msg = "Illegal data prefix size [" + toString(bytes) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * This method sets the number of bytes at the end of each
   * data record of a file. A data suffix is a block of non-image
   * data at the end of each data record (for BSQ data records are
   * lines).  This is not included in the file trailer or the data
   * trailer. It must be called before StartProcess(). This data
   * will only be available if saved via SaveDataSuffix() and may be
   * accessed via DataSuffix() after StartProcess() has returned
   * and before EndProcess() is called. If not invoked the default
   * is 0.
   *
   * @param bytes Number of bytes
   *
   * @throws Isis::iException::Message "Illegal data suffix size."
   *             (when bytes < 0)
   * @see SaveDataSuffix()
   * @see DataSuffix()
   */
  void ProcessImport::SetDataSuffixBytes(const int bytes) {

    if (bytes >= 0) {
      p_dataPostBytes = bytes;
    }
    else {
      QString msg = "Illegal data suffix size [" + toString(bytes) + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
  * This method marks the file header to be saved.  A file
  * header is a block of non-image data at the beginning of the
  * entire file.   This does not include any section headers, such
  * as band headers or line prefixes.  The file header will be
  * saved inside StartProcess(). It must be called after setting
  * the size with SetFileHeaderBytes() and before StartProcess()
  * is called. This data will only be available via FileHeader()
  * after StartProcess() has returned and before EndProcess() is
  * called.
  *
  * @throws Isis::iException::Message "File header bytes equals
  *              0.  There is nothing to save.  Use
  *              SetFileHeaderBytes() first."
  * @see SetFileHeaderBytes()
  * @see FileHeader()
  */
  void ProcessImport::SaveFileHeader() {
    if (p_fileHeaderBytes == 0) {
      QString msg = "File header bytes equals 0.  There is nothing to save.  "
                    "Use SetFileHeaderBytes() first.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    p_saveFileHeader = true;
  }


  /**
   * This method marks the file trailer to be saved.  A file
   * trailer is a block of non-image data at the end of the entire
   * file.   This does not include any section trailers, such as
   * band trailers or line suffixes.  The file trailer will be
   * saved inside StartProcess(). It must be called after setting
   * the size with SetFileTrailerBytes() and before StartProcess()
   * is called. This data will only be available via FileTrailer()
   * after StartProcess() has returned and before EndProcess() is
   * called.
   *
   * @throws Isis::iException::Message "File trailer bytes equals
   *              0.  There is nothing to save.  Use
   *              SetFileTrailerBytes() first."
   * @see SetFileTrailerBytes()
   * @see FileTrailer()
   */
  void ProcessImport::SaveFileTrailer() {
    if (p_fileTrailerBytes == 0) {
      QString msg = "File trailer bytes equals 0.  There is nothing to save.  "
                    "Use SetFileTrailerBytes() first.";
      throw IException(IException::Programmer, msg,
                                _FILEINFO_);
    }
    p_saveFileTrailer = true;
  }


  /**
   * This method marks the data block headers to be saved. A data
   * header is a block of non-image data at the beginning of each
   * data block (for BSQ data blocks are bands).  This is not
   * included in the file header and does not include any record
   * headers, such as line prefixes. Each of the header blocks
   * will be saved inside StartProcess().  It must be called after
   * setting the size with SetDataHeaderBytes() and before
   * StartProcess() is called. This data will only be available
   * via DataHeader() after StartProcess() has returned and before
   * EndProcess() is called.
   *
   * @throws Isis::iException::Message "Data header bytes equals
   *              0.  There is nothing to save.  Use
   *              SetDataHeaderBytes() first."
   * @see SetDataHeaderBytes()
   * @see DataHeader()
   */
  void ProcessImport::SaveDataHeader() {
    if (p_dataHeaderBytes == 0) {
      QString msg = "Data header bytes equals 0.  There is nothing to save.  "
                    "Use SetDataHeaderBytes() first.";
      throw IException(IException::Programmer, msg,
                                _FILEINFO_);
    }
    p_saveDataHeader = true;
  }


  /**
   * This method marks the data block trailers to be saved. A data
   * trailer is a block of non-image data at the end of each data
   * block (for BSQ data blocks are bands).  This is not included
   * in the file trailer and does not include any record trailers,
   * such as line suffixes. Each of the trailer blocks will be
   * saved inside StartProcess(). It must be called after setting
   * the size with SetDataTrailerBytes() and before StartProcess()
   * is called. This data will only be available via DataTrailer()
   * after StartProcess() has returned and before EndProcess() is
   * called.
   *
   * @throws Isis::iException::Message "Data trailer bytes equals
   *             0.  There is nothing to save.  Use
   *             SetDataTrailerBytes() first."
   * @see SetDataTrailerBytes()
   * @see DataTrailer()
   */
  void ProcessImport::SaveDataTrailer() {
    if (p_dataTrailerBytes == 0) {
      QString msg = "Data trailer bytes equals 0.  There is nothing to save.  "
                    "Use SetDataTrailerBytes() first.";
      throw IException(IException::Programmer, msg,
                                _FILEINFO_);
    }
    p_saveDataTrailer = true;
  }


  /**
   * This method marks the data prefix to be saved.  A data prefix
   * is a block of non-image data at the beginning of each data
   * record (for BSQ data records are lines).  This is not
   * included in the file header or the data header.  Each of the
   * data prefixes will be saved inside StartProcess().  It must
   * be called after setting the size with SetDataPrefixBytes()
   * and before StartProcess() is called. This data will only be
   * available via DataPrefix() after StartProcess() has returned
   * and before EndProcess() is called.
   *
   *  @throws Isis::iException::Message "Data prefix bytes equals
   *              0.  There is nothing to save.  Use
   *              SetDataPrefixBytes() first."
   * @see SetDataPrefixBytes()
   * @see DataPrefix()
   */
  void ProcessImport::SaveDataPrefix() {
    if (p_dataPreBytes == 0) {
      QString msg = "Data prefix bytes equals 0.  There is nothing to save.  "
                    "Use SetDataPrefixBytes() first.";
      throw IException(IException::Programmer, msg,
                                _FILEINFO_);
    }
    p_saveDataPre = true;
  }


  /**
   * This method marks the data suffix to be saved.  A data
   * suffix is a block of non-image data at the end of each data
   * record (for BSQ data records are lines).  This is not
   * included in the file trailer or the data trailer. Each of
   * the data suffixes will be saved inside StartProcess().  It
   * must be called after setting the size with
   * SetDataPrefixBytes() and before StartProcess() is called.
   * This data will only be available via DataPrefix() after
   * StartProcess() has returned and before EndProcess() is
   * called.
   *
   * @throws Isis::iException::Message "Data suffix bytes equals
   *             0.  There is nothing to save.  Use
   *             SetDataSuffixBytes() first."
   * @see SetDataSuffixBytes()
   * @see DataSuffix()
   */
  void ProcessImport::SaveDataSuffix() {
    if (p_dataPostBytes == 0) {
      QString msg = "Data suffix bytes equals 0.  There is nothing to save.  "
                    "Use SetDataSuffixBytes() first.";
      throw IException(IException::Programmer, msg,
                                _FILEINFO_);
    }
    p_saveDataPost = true;
  }


  /**
   * This method returns the number of file header bytes
   */
  int ProcessImport::FileHeaderBytes() const {

    return p_fileHeaderBytes;
  }


  /**
   * This method returns the number of file trailer bytes
   */
  int ProcessImport::FileTrailerBytes() const {
    return p_fileTrailerBytes;
  }


  /**
   * This method returns the number of data header bytes
   */
  int ProcessImport::DataHeaderBytes() const {
    return p_dataHeaderBytes;
  }


  /**
   * This method returns the number of data trailer bytes
   */
  int ProcessImport::DataTrailerBytes() const {
    return p_dataTrailerBytes;
  }


  /**
   * This method returns the number of data prefix bytes
   */
  int ProcessImport::DataPrefixBytes() const {
    return p_dataPreBytes;
  }


  /**
   * This method returns the number of data duffix bytes
   */
  int ProcessImport::DataSuffixBytes() const {

    return p_dataPostBytes;
  }


  /**
   * This method returns a pointer to the file header.  A file
   * header is a block of non-image data at the beginning of the entire
   * file.  This does not include any section headers, such
   * as band headers or line prefixes.  In order to retrieve this
   * data, the file header bytes must be set and the file header
   * must be saved via SetFileHeaderBytes() and SaveFileHeader(),
   * respectively. This method must be called only after
   * StartProcess() has returned and before EndProcess() is
   * called.
   *
   * @return <B>char*</B> Pointer to file header, if saved.
   * @throws Isis::iException::Message "File header was not saved.
   *              Use SaveFileHeader()."
   *
   * @see SetFileHeaderBytes()
   * @see SaveFileHeader()
   */
  char *ProcessImport::FileHeader() {
    if (p_saveFileHeader) {
      return p_fileHeader;
    }
    QString msg = "File header was not saved.  Use SaveFileHeader().";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   * This method returns a pointer to the file trailer.  A file
   * trailer is a block of non-image data at the end of the entire
   * file.  This does not include any section trailers, such as
   * band trailers or line suffixes.  In order to retrieve this
   * data, the file trailer bytes must be set and the file trailer
   * must be saved via SetFileTrailerBytes() and
   * SaveFileTrailer(), respectively. This method must be called
   * only after StartProcess() has returned and before
   * EndProcess() is called.
   *
   * @return <B>char*</B> Pointer to file trailer, if saved.
   * @throws Isis::iException::Message "File trailer was not
   *              saved. Use SaveFileTrailer()."
   * @see SetFileTrailerBytes()
   * @see SaveFileTrailer()
   */
  char *ProcessImport::FileTrailer() {
    if (p_saveFileTrailer) {
      return p_fileTrailer;
    }
    QString msg = "File trailer was not saved.  Use SaveFileTrailer()";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   * This method returns a pointer to the data header.  A data
   * header is a block of non-image data at the beginning of each
   * data block (for BSQ data blocks are bands).  This is not
   * included in the file header and does not include any record
   * headers, such as line prefixes.  In order to retrieve this
   * data, the data header bytes must be set and the data header
   * must be saved via SetDataHeaderBytes() and SaveDataHeader(),
   * respectively. This method must be called only after
   * StartProcess() has returned and before EndProcess() is
   * called.
   *
   * @return <B>vector <char*> </B> Vector of pointers to data
   *         headers for each block, if saved.
   * @throws Isis::iException::Message "Data header was not saved.
   *              Use SaveDataHeader()."
   * @see SetDataHeaderBytes()
   * @see SaveDataHeader()
   */
  std::vector<char *> ProcessImport::DataHeader() {
    if (p_saveDataHeader) {
      return p_dataHeader;
    }
    QString msg = "Data header was not saved.  Use SaveDataHeader()";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   * This method returns a pointer to the data trailer.  A data
   * trailer is a block of non-image data at the end each data
   * block (for BSQ data blocks are bands).  This is not included
   * in the file trailer and does not include any record trailers,
   * such as line suffixes.  In order to retrieve this data, the
   * data trailer bytes must be set and the data trailer must be
   * saved via SetDataTrailerBytes() and SaveDataTrailer(),
   * respectively. This method must be called only after
   * StartProcess() has returned and before EndProcess() is
   * called.
   *
   * @return <B>vector <char*> </B> Vector of pointers to data
   *         trailers for each block, if saved.
   * @throws Isis::iException::Message "Data trailer was not
   *              saved. Use SaveDataTrailer()."
   * @see SetDataTrailerBytes()
   * @see SaveDataTrailer()
   */
  std::vector<char *> ProcessImport::DataTrailer() {
    if (p_saveDataTrailer) {
      return p_dataTrailer;
    }
    QString msg = "Data trailer was not saved.  Use SaveDataTrailer()";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   *  This method returns a pointer to the data prefix.  A data
   *  prefix is a block of non-image data at the beginning of each
   * data record (for BSQ data records are lines).  This is not
   *  included in the file header or the data header.  In order to
   *  retrieve this data, the data prefix bytes must be set and
   *  the data prefix must be saved via SetDataPrefixBytes() and
   *  SaveDataPrefix(), respectively. This method must be called
   *  only after StartProcess() has returned and before
   *  EndProcess() is called.
   *
   * @return <B>vector <vector <char*> > </B> Two-dimensional
   *         vector of pointers to data prefixes for each record,
   *         if saved.
   * @throws Isis::iException::Message "Data prefix was not saved.
   *              Use SaveDataPrefix()."
   * @see SetDataPrefixBytes()
   * @see SaveDataPrefix()
   */
  std::vector<std::vector<char *> > ProcessImport::DataPrefix() {
    if (p_saveDataPre) {
      return p_dataPre;
    }
    QString msg = "Data prefix was not saved.  Use SaveDataPrefix()";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   * This method returns a pointer to the data suffix.  A data
   * suffix is a block of non-image data at the end of each data
   * record (for BSQ data records are lines).  This is not
   * included in the file trailer or the data trailer. In order to
   * retrieve this data, the data suffix bytes must be set and the
   * data suffix must be saved via SetDataSuffixBytes() and
   * SaveDataSuffix(), respectively. This method must be called
   * only after StartProcess() has returned and before
   * EndProcess() is called.
   *
   * @return <B>vector <vector <char*> > </B> Two-dimensional
   *         vector of pointers to data suffixes for each record,
   *         if saved.
   * @throws Isis::iException::Message "Data suffix was not saved.
   *              Use SaveDataSuffix()."
   * @see SetDataSuffixBytes()
   * @see SaveDataSuffix()
   */
  std::vector<std::vector<char *> > ProcessImport::DataSuffix() {
    if (p_saveDataPost) {
      return p_dataPost;
    }
    QString msg = "Data suffix was not saved.  Use SaveDataSuffix()";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   * Sets the organization of the input cube. If not invoked it is assumed
   * Isis::Import::Bsq.
   *
   * @param org The enumerated organizations either Isis::Import::Bsq
   *            Isis::Import::Bil or Isis::Import::Bip
   */
  void ProcessImport::SetOrganization(const ProcessImport::Interleave org) {
    p_organization = org;
  }


  /**
   * Sets the VAX flag of the input cube.  If true, then the core pixel type of
   * the input cube is VAX, and VAX conversion routines need to be run to
   * convert the pixels to IEEE format.
   *
   * @param vax_convert Flag indicating whether or not to run VAX conersion methods
   */
  void ProcessImport::SetVAXConvert(const bool vax_convert) {
    p_vax_convert = vax_convert;
  }


  /**
   * Gets the organization of the input cube.
   *
   */
  ProcessImport::Interleave ProcessImport::Organization() const {
    return p_organization;
  }


  /**
   * Sets the core base of the input cube. Defaults to zero if not invoked.
   *
   * @param base Core base
   */
  void ProcessImport::SetBase(const double base) {
    p_base.clear();
    p_base.push_back(base);
  }


  /**
   * Sets band dependent core base
   *
   * @param base A vector containing the base for each band.
   */
  void ProcessImport::SetBase(const std::vector<double> base) {
    p_base = base;
  }


  /**
   * Sets the core multiplier of the input cube. Defaults to one if not invoked.
   *
   * @param mult Core base
   */
  void ProcessImport::SetMultiplier(const double mult) {
    p_mult.clear();
    p_mult.push_back(mult);
  }


  /**
   * Sets band dependent core multiplier
   *
   * @param mult A vector containing the multiplier for each band.
   */
  void ProcessImport::SetMultiplier(const std::vector<double> mult) {
    p_mult = mult;
  }


  /**
   * Sets a mapping of input pixel values to output special pixel values.
   *
   * @param null The double precision value used to identify null pixels in the
   *             image being imported.
   *
   * @param lrs The double precisiion value used to identify low representation
   *            saturation LRS pixels in the image being imported.
   *
   * @param lis The double precision value used to identify low instrument
   *            saturation LIS pixels in the image being imported.
   *
   * @param hrs The double precision value used to identify high representation
   *            saturation HRS pixels in the image being imported.
   *
   * @param his The double precision value used to identify high instrument
   *            saturation HIS pixels in the image being imported.
   */
  void ProcessImport::SetSpecialValues(const double null, const double lrs,
                                       const double lis, const double hrs,
                                       const double his) {
    SetNull(null, null);
    SetLRS(lrs, lrs);
    SetLIS(lis, lis);
    SetHRS(hrs, hrs);
    SetHIS(his, his);
  }


  /**
  * Sets the range that will be considered Isis::Null
  *
  * @param null_min The double precision value used to identify
  *             the min range of null pixels in the image being
  *             imported.
  * @param null_max The double precision value used to identify
  *             the max range of null pixels in the image being
  *             imported.
  */
  void ProcessImport::SetNull(const double null_min, const double null_max) {
    CheckPixelRange("Null", null_min, null_max);
    p_null_min = null_min;
    p_null_max = null_max;
  }


  /**
  * Sets the range that will be considered Isis::Null
  *
  * @param lrs_min The double precision value used to identify
  *             the min range of lrs pixels in the image being
  *             imported.
  * @param lrs_max The double precision value used to identify
  *             the max range of lrs pixels in the image being
  *             imported.
  */
  void ProcessImport::SetLRS(const double lrs_min, const double lrs_max) {
    CheckPixelRange("LRS", lrs_min, lrs_max);
    p_lrs_min = lrs_min;
    p_lrs_max = lrs_max;
  }


  /**
  * Sets the range that will be considered Isis::Null
  *
  * @param lis_min The double precision value used to identify
  *             the min range of lis pixels in the image being
  *             imported.
  * @param lis_max The double precision value used to identify
  *             the max range of lis pixels in the image being
  *             imported.
  */
  void ProcessImport::SetLIS(const double lis_min, const double lis_max) {
    CheckPixelRange("LIS", lis_min, lis_max);
    p_lis_min = lis_min;
    p_lis_max = lis_max;
  }


  /**
  * Sets the range that will be considered Isis::Null
  *
  * @param hrs_min The double precision value used to identify
  *             the min range of hrs pixels in the image being
  *             imported.
  * @param hrs_max The double precision value used to identify
  *             the max range of hrs pixels in the image being
  *             imported.
  */
  void ProcessImport::SetHRS(const double hrs_min, const double hrs_max) {
    CheckPixelRange("HRS", hrs_min, hrs_max);
    p_hrs_min = hrs_min;
    p_hrs_max = hrs_max;
  }


  /**
  * Sets the range that will be considered Isis::Null
  *
  * @param his_min The double precision value used to identify
  *             the min range of his pixels in the image being
  *             imported.
  * @param his_max The double precision value used to identify
  *             the max range of his pixels in the image being
  *             imported.
  */
  void ProcessImport::SetHIS(const double his_min, const double his_max) {
    CheckPixelRange("HIS", his_min, his_max);
    p_his_min = his_min;
    p_his_max = his_max;
  }


  /**
   * Checks the special pixel range of the given against all other special pixel
   * value ranges, making sure none overlap.
   *
   * @param pixelName Name of the special pixel type to be displayed in the
   *               error message.
   * @param pixelMin The minimum value of the special pixel range
   * @param pixelMax The maximum value of the special pixel range
   */
  void ProcessImport::CheckPixelRange(QString pixelName, double pixelMin,
                                      double pixelMax) {
    if (pixelMin == DBL_MAX || pixelMax == -DBL_MAX) return;

    if (p_null_min != DBL_MAX && p_null_max != -DBL_MAX && ( //-null has been set
          (pixelMin > p_null_min && pixelMin < p_null_max) ||  // --min crossing
          (pixelMax > p_null_min && pixelMax < p_null_max) ||  // --max crossing
          (pixelMin < p_null_min && pixelMax > p_null_max))) { // --straddling
                                                               //   values
      QString msg = "The " + pixelName + " range [" + toString(pixelMin) +
                   "," + toString(pixelMax) + "] overlaps the NULL range [" +
                   toString(p_null_min) + "," + toString(p_null_max) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (p_lrs_min != DBL_MAX && p_lrs_max != -DBL_MAX && (
          (pixelMin > p_lrs_min && pixelMin < p_lrs_max) ||
          (pixelMax > p_lrs_min && pixelMax < p_lrs_max) ||
          (pixelMin < p_lrs_min && pixelMax > p_lrs_max))) {
      QString msg = "The " + pixelName + " range [" + toString(pixelMin) +
                   "," + toString(pixelMax) + "] overlaps the LRS range [" +
                   toString(p_lrs_min) + "," + toString(p_lrs_max) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (p_lis_min != DBL_MAX && p_lis_max != -DBL_MAX && (
          (pixelMin > p_lis_min && pixelMin < p_lis_max) ||
          (pixelMax > p_lis_min && pixelMax < p_lis_max) ||
          (pixelMin < p_lis_min && pixelMax > p_lis_max))) {
      QString msg = "The " + pixelName + " range [" + toString(pixelMin) +
                   "," + toString(pixelMax) + "] overlaps the LIS range [" +
                   toString(p_lis_min) + "," + toString(p_lis_max) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (p_hrs_min != DBL_MAX && p_hrs_max != -DBL_MAX && (
          (pixelMin > p_hrs_min && pixelMin < p_hrs_max) ||
          (pixelMax > p_hrs_min && pixelMax < p_hrs_max) ||
          (pixelMin < p_hrs_min && pixelMax > p_hrs_max))) {
      QString msg = "The " + pixelName + " range [" + toString(pixelMin) +
                   "," + toString(pixelMax) + "] overlaps the HRS range [" +
                   toString(p_hrs_min) + "," + toString(p_hrs_max) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    if (p_his_min != DBL_MAX && p_his_max != -DBL_MAX && (
          (pixelMin > p_his_min && pixelMin < p_his_max) ||
          (pixelMax > p_his_min && pixelMax < p_his_max) ||
          (pixelMin < p_his_min && pixelMax > p_his_max))) {
      QString msg = "The " + pixelName + " range [" + toString(pixelMin) +
                   "," + toString(pixelMax) + "] overlaps the HIS range [" +
                   toString(p_his_min) + "," + toString(p_his_max) + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

  }


  /**
  * Tests the pixel. If it is valid it will return the dn value,
  * otherwise it will return the Isis special pixel value that
  * corresponds to it
  *
  * @param pixel The double precision value that represents a
  *              pixel.
  * @return double  The double precision value representing the
  *         pixel will return as a valid dn or changed to an isis
  *         special pixel.
  */
  double ProcessImport::TestPixel(const double pixel) {
    if (pixel <= p_null_max && pixel >= p_null_min) {
      return Isis::NULL8;
    }
    else if (pixel <= p_hrs_max && pixel >= p_hrs_min) {
      return Isis::HIGH_REPR_SAT8;
    }
    else if (pixel <= p_lrs_max && pixel >= p_lrs_min) {
      return Isis::LOW_REPR_SAT8;
    }
    else if (pixel <= p_his_max && pixel >= p_his_min) {
      return Isis::HIGH_INSTR_SAT8;
    }
    else if (pixel <= p_lis_max && pixel >= p_lis_min) {
      return Isis::LOW_INSTR_SAT8;
    }
    else {
      return pixel;
    }
  }


  /**
   * Given a CubeAttributeOutput object, set min/max to propagate if
   * propagating min/max attributes was requested and set the pixel
   * type to propagate if pixel type propagation was requested.
   *
   * @param parameter The parameter name that holds the output file name.
   *
   * @throws Isis::iException::Message "Unsupported pixel type."
   */
  void ProcessImport::SetAttributes(CubeAttributeOutput &att) {
    if (att.propagateMinimumMaximum()) {
      double min, max;
      if ((p_pixelType == Isis::Double) ||
          (p_pixelType == Isis::Real) ||
          (p_base.size() > 1) || (p_mult.size() > 1)) {
        min = Isis::VALID_MIN4;
        max = Isis::VALID_MAX4;
      }
      else if (p_pixelType == Isis::SignedInteger) {
        min = Isis::IVALID_MIN4;
        max = Isis::IVALID_MAX4;
      }

      else if (p_pixelType == Isis::UnsignedInteger) {
        min = Isis::VALID_MINUI4;
        max = Isis::VALID_MAXUI4;
      }
      else if (p_pixelType == Isis::SignedWord) {
        min = Isis::VALID_MIN2 * p_mult[0] + p_base[0];
        max = Isis::VALID_MAX2 * p_mult[0] + p_base[0];
      }
      else if (p_pixelType == Isis::UnsignedWord) {
        min = Isis::VALID_MINU2 * p_mult[0] + p_base[0];
        max = Isis::VALID_MAXU2 * p_mult[0] + p_base[0];
      }
      else if (p_pixelType == Isis::UnsignedByte) {
        min = Isis::VALID_MIN1 * p_mult[0] + p_base[0];
        max = Isis::VALID_MAX1 * p_mult[0] + p_base[0];
      }
      else {
        QString msg = "Unsupported pixel type [" +
                     Isis::PixelTypeName(p_pixelType) + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
      att.setMinimum(min);
      att.setMaximum(max);
    }

    if (att.propagatePixelType()) {
      if ((p_base.size() > 1) || (p_mult.size() > 1)) {
        att.setPixelType(Isis::Real);
      }
      else if (p_pixelType == Isis::Double || p_pixelType == Isis::SignedInteger) {
        att.setPixelType(Isis::Real);
      }
      else {
        att.setPixelType(p_pixelType);
      }
    }
  }


  /**
   * Create the output file. Note that all the appropriate calls to at least
   * SetDimensions and SetPixelType should be made prior to calling this method.
   *
   * @param parameter The parameter name that holds the output file name.
   *
   * @return @b Isis::Cube Output cube.
   *
   * @throws Isis::iException::Message "Unsupported pixel type."
   */
  Isis::Cube *ProcessImport::SetOutputCube(const QString &parameter) {
    CubeAttributeOutput &att =
      Application::GetUserInterface().GetOutputAttribute(parameter);

    SetAttributes(att);

    return Process::SetOutputCube(Application::GetUserInterface().GetCubeName(parameter), att, p_ns, p_nl, p_nb);
  }


  /**
   * Create the output file.
   *
  */
  Isis::Cube *ProcessImport::SetOutputCube(const QString &parameter, UserInterface &ui){
    CubeAttributeOutput &att = ui.GetOutputAttribute(parameter);
    SetAttributes(att);
    return Isis::Process::SetOutputCube(ui.GetCubeName(parameter), att, p_ns, p_nl, p_nb);
  }

  /**
   * Create the output file. Note that all the appropiate calls to at least
   * SetDimensions should be invoked prior to calling this method.
   *
   * @param fname The output file name.
   *
   * @param att An output cube attribute to define the characteristics of the
   *            output cube.
   *
   * @return @b Isis::Cube Output cube.
   * @throws Isis::iException::Message "File is not in a supported
   *             organization."
   */
  Isis::Cube *ProcessImport::SetOutputCube(const QString &fname,
      Isis::CubeAttributeOutput &att) {
    SetAttributes(att);
    return Isis::Process::SetOutputCube(fname, att, p_ns, p_nl, p_nb);
  }


  //! Process the input file and write it to the output.
  void ProcessImport::StartProcess() {
    if (p_organization == ProcessImport::JP2) {
      ProcessJp2();
    }
    else if (p_organization == ProcessImport::BSQ) {
      ProcessBsq();
    }
    else if (p_organization == ProcessImport::BIL) {
      ProcessBil();
    }
    else if (p_organization == ProcessImport::BIP) {
      ProcessBip();
    }
    else {
      QString msg = "File [" + p_inFile
                   + "] is not in a supported organization.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Process the input file and send data to method
   *
   * @param funct Method that accepts Isis::Buffer as an input
   *              parameter, processes the image, and has no
   *              return value.
   * @throws Isis::iException::Message "File is not a supported
   *             organization."
   */
  void ProcessImport::StartProcess(void funct(Isis::Buffer &out)) {
    if (p_organization == ProcessImport::JP2) {
      ProcessJp2(funct);
    }
    else if (p_organization == ProcessImport::BSQ) {
      ProcessBsq(funct);
    }
    else if (p_organization == ProcessImport::BIL) {
      ProcessBil(funct);
    }
    else if (p_organization == ProcessImport::BIP) {
      ProcessBip(funct);
    }
    else {
      QString msg = "File [" + p_inFile + "] is not in a supported organization.";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  /**
   * Process the import data as a band sequential file.
   *
   * @param funct Method that accepts Isis::Buffer as an input
   *              parameter, processes the image, and has no
   *              return value.
   *
   * @throws Isis::iException::Message "Cannot open input file."
   * @throws Isis::iException::Message "Cannot read file.
   *             Position[]. Byte count[]"
   */
  void ProcessImport::ProcessBsq(void funct(Isis::Buffer &out)) {
    // Figure out the number of bytes to read for a single line
    int readBytes = Isis::SizeOf(p_pixelType);
    readBytes = readBytes * p_ns;
    char *in = new char [readBytes];

    // Set up an Isis::EndianSwapper object
    QString tok(Isis::ByteOrderName(p_byteOrder));
    tok = tok.toUpper();
    Isis::EndianSwapper swapper(tok);

    ifstream fin;
    // Open input file
    Isis::FileName inFile(p_inFile);
    QString inFileName(inFile.expanded());
    fin.open(inFileName.toLatin1().data(), ios::in | ios::binary);
    if (!fin.is_open()) {
      QString msg = "Cannot open input file [" + p_inFile + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Handle the file header
    streampos pos = fin.tellg();
    if (p_saveFileHeader) {
      p_fileHeader = new char[p_fileHeaderBytes];
      fin.read(p_fileHeader, p_fileHeaderBytes);
      fin.seekg(p_suffixData+p_fileHeaderBytes, ios_base::beg);
    }
    else {
      fin.seekg(p_fileHeaderBytes+p_suffixData, ios_base::beg);
    }

    // Check the last io
    if (!fin.good()) {
      QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                   toString((int)pos) + "]. Byte count [" +
                   toString(p_fileHeaderBytes) + "]" ;
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Construct a line buffer manager
    Isis::Buffer *out = NULL;

    if (funct != NULL) {
      out = new Isis::Brick(p_ns, 1, 1, p_pixelType);
    }
    else {
      out = new Isis::LineManager(*OutputCubes[0]);
    }

    // Loop once for each band in the image
    p_progress->SetMaximumSteps(p_nl * p_nb);
    p_progress->CheckStatus();

    // Loop for each band
    for(int band = 0; band < p_nb; band++) {
      // Set the base multiplier
      double base, mult;
      if (p_base.size() > 1) {
        base = p_base[band];
        mult = p_mult[band];
      }
      else {
        base = p_base[0];
        mult = p_mult[0];
      }

      // Handle any data headers (e.g., the data at the beginning of each band)
      pos = fin.tellg();
      if (p_saveDataHeader) {
        p_dataHeader.push_back(new char[p_dataHeaderBytes]);
        fin.read(p_dataHeader.back(), p_dataHeaderBytes);
      }
      else {
        fin.seekg(p_dataHeaderBytes, ios_base::cur);
       }

      // Check the last io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_dataHeaderBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Space for storing prefix and suffix data pointers
      vector<char *> tempPre, tempPost;

      // Loop for each line in a band
      for(int line = 0; line < p_nl; line++) {

        // Handle any line prefix bytes
        pos = fin.tellg();
        if (p_saveDataPre) {
          tempPre.push_back(new char[p_dataPreBytes]);
          fin.read(tempPre.back(), p_dataPreBytes);
        }
        else {
          fin.seekg(p_dataPreBytes, ios_base::cur);
        }

        // Check the last io
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(p_dataPreBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }

        // Get a line of data from the input file
        pos = fin.tellg();
        fin.read(in, readBytes);
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(readBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }

        // Swap the bytes if necessary and convert any out of bounds pixels
        // to special pixels
        for(int samp = 0; samp < p_ns; samp++) {
          switch(p_pixelType) {
            case Isis::UnsignedByte:
              (*out)[samp] = (double)((unsigned char *)in)[samp];
              break;
            case Isis::UnsignedWord:
              (*out)[samp] =
                (double)swapper.UnsignedShortInt((unsigned short int *)in+samp);
              break;
            case Isis::SignedWord:
              (*out)[samp] = (double)swapper.ShortInt((short int *)in+samp);
              break;
            case Isis::SignedInteger:
              (*out)[samp] = (double)swapper.Int((int *)in+samp);
              break;

          case Isis::UnsignedInteger:
            (*out)[samp] = (double)swapper.Uint32_t((unsigned int *)in+samp);
            break;
            case Isis::Real:
              if(p_vax_convert) {
                (*out)[samp]= VAXConversion( (float *)in+samp );
              }
              else {
                (*out)[samp] = (double)swapper.Float((float *)in+samp);
              }
              break;
            case Isis::Double:
              (*out)[samp] = (double)swapper.Double((double *)in+samp);
              break;
            default:
              break;
          }

          (*out)[samp] = TestPixel((*out)[samp]);

          if (Isis::IsValidPixel((*out)[samp])) {
            (*out)[samp] = mult * ((*out)[samp]) + base;
          }
        } // End sample loop

        if (funct == NULL) {
          // Set the buffer position and write the line to the output file
          ((Isis::LineManager *)out)->SetLine((band * p_nl) + line + 1);
          OutputCubes[0]->write(*out);
        }
        else {
          ((Isis::Brick *)out)->SetBaseSample(1);
          ((Isis::Brick *)out)->SetBaseLine(line + 1);
          ((Isis::Brick *)out)->SetBaseBand(band + 1);
          funct(*out);
        }

        p_progress->CheckStatus();

        // Handle any line suffix bytes
        pos = fin.tellg();
        if (p_saveDataPost) {
          tempPost.push_back(new char[p_dataPostBytes]);
          fin.read(tempPost.back(), p_dataPostBytes);
        }
        else {
          fin.seekg(p_dataPostBytes, ios_base::cur);
        }

        // Check the last io
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(p_dataPreBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }
      } // End line loop

      // Save off the prefix bytes vector
      if (p_saveDataPre) {
        p_dataPre.push_back(tempPre);
        tempPre.clear();
      }

      // Save off the suffix bytes vector
      if (p_saveDataPost) {
        p_dataPost.push_back(tempPost);
        tempPost.clear();
      }

      // Handle the band trailer
      pos = fin.tellg();
      if (p_saveDataTrailer) {
        p_dataTrailer.push_back(new char[p_dataTrailerBytes]);
        fin.read(p_dataTrailer.back(), p_dataTrailerBytes);
      }
      else {
        fin.seekg(p_dataTrailerBytes, ios_base::cur);
      }

      // Check the last io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_fileHeaderBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

    } // End band loop

    // Handle the file trailer
    pos = fin.tellg();
    if (p_saveFileTrailer) {
      fin.seekg(0, ios_base::end);
      streampos e = fin.tellg();
      p_fileTrailerBytes = (int)(e - pos + (streampos)1);
      p_fileTrailer = new char[p_fileTrailerBytes];
      fin.seekg(pos);
      fin.read(p_fileTrailer, p_fileTrailerBytes);

      // Check the io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_fileTrailerBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

    }

    // Close the file and clean up
    fin.close();
    delete [] in;
  }


  /**
   * Function to process files stored as Band Interleaved by Line
   *
   * @param funct Method that accepts Isis::Buffer as an input
   *              parameter, processes the image, and has no
   *              return value.
   * @throws Isis::iException::Message "Cannot open input file."
   * @throws Isis::iException::Message "Cannot read file.
   *             Position[]. Byte count[]"
   */
  void ProcessImport::ProcessBil(void funct(Isis::Buffer &someBuf)) {

    // Figure out the number of bytes to read for a single line
    int readBytes = Isis::SizeOf(p_pixelType);
    readBytes = readBytes * p_ns;
    char *in = new char [readBytes];

    // Set up an Isis::EndianSwapper object
    QString tok(Isis::ByteOrderName(p_byteOrder));
    tok = tok.toUpper();
    Isis::EndianSwapper swapper(tok);

    ifstream fin;
    // Open input file
    Isis::FileName inFile(p_inFile);
    QString inFileName(inFile.expanded());
    fin.open(inFileName.toLatin1().data(), ios::in | ios::binary);
    if (!fin.is_open()) {
      QString msg = "Cannot open input file [" + p_inFile + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Handle the file header
    streampos pos = fin.tellg();

    if (p_saveFileHeader) {
      p_fileHeader = new char[p_fileHeaderBytes];
      fin.read(p_fileHeader, p_fileHeaderBytes);
      fin.seekg(p_suffixData+p_fileHeaderBytes, ios_base::beg);
    }
    else {
      fin.seekg(p_fileHeaderBytes+p_suffixData, ios_base::beg);
    }

    // Check the last io
    if (!fin.good()) {
      QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                   toString((int)pos) + "]. Byte count [" +
                   toString(p_fileHeaderBytes) + "]" ;
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Construct a line buffer manager
    Isis::Buffer *out = NULL;

    if (funct != NULL) {
      out = new Isis::Brick(p_ns, p_nl, p_nb, p_ns, 1, 1, p_pixelType, true);
      ((Isis::Brick *)out)->setpos(0);
    }
    else {
      OutputCubes[0]->addCachingAlgorithm(new BoxcarCachingAlgorithm());
      out = new Isis::LineManager(*OutputCubes[0]);
    }

    // Loop once for each line in the image
    p_progress->SetMaximumSteps(p_nb * p_nl);
    p_progress->CheckStatus();

    // Loop for each line
    for(int line = 0; line < p_nl; line++) {
      // Loop for each band
      for(int band = 0; band < p_nb; band++) {
        // Set the base multiplier
        double base, mult;
        if (p_base.size() > 1) {
          base = p_base[band];
          mult = p_mult[band];
        }
        else {
          base = p_base[0];
          mult = p_mult[0];
        }

        // Check the last io
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(p_dataHeaderBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }

        // Space for storing prefix and suffix data pointers
        vector<char *> tempPre, tempPost;

        // Handle any line prefix bytes
        pos = fin.tellg();
        if (p_saveDataPre) {
          tempPre.push_back(new char[p_dataPreBytes]);
          fin.read(tempPre.back(), p_dataPreBytes);
        }
        else {
          fin.seekg(p_dataPreBytes, ios_base::cur);
        }

        // Check the last io
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(p_dataPreBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }


        // Get a line of data from the input file
        pos = fin.tellg();
        fin.read(in, readBytes);
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(readBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }

        // Swap the bytes if necessary and convert any out of bounds pixels
        // to special pixels
        for(int samp = 0; samp < p_ns; samp++) {
          switch(p_pixelType) {
            case Isis::UnsignedByte:
              (*out)[samp] = (double)((unsigned char *)in)[samp];
              break;
            case Isis::UnsignedWord:
              (*out)[samp] =
                (double)swapper.UnsignedShortInt((unsigned short int *)in+samp);
              break;
            case Isis::SignedWord:
              (*out)[samp] = (double)swapper.ShortInt((short int *)in+samp);
              break;
            case Isis::SignedInteger:
              (*out)[samp] = (double)swapper.Int((int *)in+samp);
              break;
          case Isis::UnsignedInteger:
            (*out)[samp] = (double)swapper.Uint32_t((unsigned int *)in+samp);
            break;
            case Isis::Real:
              if(p_vax_convert) {
                (*out)[samp]= VAXConversion( (float *)in+samp );
              }
              else {
                (*out)[samp] = (double)swapper.Float((float *)in+samp);
              }
              break;
            case Isis::Double:
              (*out)[samp] = (double)swapper.Double((double *)in+samp);
              break;
            default:
              break;
          }

          // Sets out to isis special pixel or leaves it if valid
          (*out)[samp] = TestPixel((*out)[samp]);

          if (Isis::IsValidPixel((*out)[samp])) {
            (*out)[samp] = mult * ((*out)[samp]) + base;
          }
        } // End sample loop

        if (funct == NULL) {
          ((Isis::LineManager *)out)->SetLine((band * p_nl) + line + 1);
          OutputCubes[0]->write(*out);
        }
        else {
          funct(*out);
          (*((Isis::Brick *)out))++;
        }

        p_progress->CheckStatus();

        // Handle any line suffix bytes
        pos = fin.tellg();
        if (p_saveDataPost) {
          tempPost.push_back(new char[p_dataPostBytes]);
          fin.read(tempPost.back(), p_dataPostBytes);
        }
        else {
          fin.seekg(p_dataPostBytes, ios_base::cur);
        }

        // Check the last io
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(p_dataPreBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }

        // Save off the prefix bytes vector
        if (p_saveDataPre) {
          p_dataPre.push_back(tempPre);
          tempPre.clear();
        }

        // Save off the suffix bytes vector
        if (p_saveDataPost) {
          p_dataPost.push_back(tempPost);
          tempPost.clear();
        }

        // Check the last io
        if (!fin.good()) {
          QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                       toString((int)pos) + "]. Byte count [" +
                       toString(p_fileHeaderBytes) + "]" ;
          throw IException(IException::Io, msg, _FILEINFO_);
        }

      } // End band loop

    } // End line loop

    // Handle the file trailer
    pos = fin.tellg();
    if (p_saveFileTrailer) {
      fin.seekg(0, ios_base::end);
      streampos e = fin.tellg();
      p_fileTrailerBytes = (int)(e - pos + (streampos)1);
      p_fileTrailer = new char[p_fileTrailerBytes];
      fin.seekg(pos);
      fin.read(p_fileTrailer, p_fileTrailerBytes);

      // Check the io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_fileTrailerBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

    }

    // Close the file and clean up
    fin.close();
    delete [] in;
  }


  /**
   * Function to process files stored as Band Interleaved by Pixel
   *
   * @param funct Method that accepts Isis::Buffer as an input
   *              parameter, processes the image, and has no
   *              return value.
   * @throws Isis::iException::Message "Cannot open input file."
   * @throws Isis::iException::Message "Cannot read file.
   *             Position[]. Byte count[]"
   */
  void ProcessImport::ProcessBip(void funct(Isis::Buffer &out)) {

    // Set up an Isis::EndianSwapper object
    QString tok(Isis::ByteOrderName(p_byteOrder));
    tok = tok.toUpper();
    Isis::EndianSwapper swapper(tok);

    ifstream fin;
    // Open input file
    Isis::FileName inFile(p_inFile);
    QString inFileName(inFile.expanded());
    fin.open(inFileName.toLatin1().data(), ios::in | ios::binary);
    if (!fin.is_open()) {
      QString msg = "Cannot open input file [" + p_inFile + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    // Handle the file header
    streampos pos = fin.tellg();
    if (p_saveFileHeader) {
      p_fileHeader = new char[p_fileHeaderBytes];
      fin.read(p_fileHeader, p_fileHeaderBytes);
      fin.seekg(p_suffixData+p_fileHeaderBytes, ios_base::beg);
    }
    else {
      fin.seekg(p_fileHeaderBytes+p_suffixData, ios_base::beg);
    }

    // Check the last io
    if (!fin.good()) {
      QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                   toString((int)pos) + "]. Byte count [" +
                   toString(p_fileHeaderBytes) + "]" ;
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    OutputCubes[0]->addCachingAlgorithm(new BoxcarCachingAlgorithm());

    // Construct a line buffer manager
    Isis::Buffer *out = NULL;

    if (funct != NULL) {
      out = new Isis::Buffer(p_ns, p_nl, p_nb, p_pixelType);
    }
    else {
      out = new Isis::LineManager(*OutputCubes[0]);
    }

    // Loop once for each line in the image
    p_progress->SetMaximumSteps(p_nl);
    p_progress->CheckStatus();

    // Figure out the number of bytes to read for a single line
    int sampleBytes = Isis::SizeOf(p_pixelType) * p_nb + p_dataPreBytes + p_dataPostBytes;
    int readBytes = p_ns * sampleBytes;
    char *in = new char [readBytes];

    // Loop for each line
    for(int line = 0; line < p_nl; line++) {
      // Check the last io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_dataHeaderBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Space for storing prefix and suffix data pointers
      vector<char *> tempPre, tempPost;

      // Check the last io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_dataPreBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Get a line of data from the input file
      pos = fin.tellg();
      fin.read(in, readBytes);
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(readBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Loop for each band
      for(int band = 0; band < p_nb; band++) {
        // Set the base multiplier
        double base, mult;
        if (p_base.size() > 1) {
          base = p_base[band];
          mult = p_mult[band];
        }
        else {
          base = p_base[0];
          mult = p_mult[0];
        }

        // Swap the bytes if necessary and convert any out of bounds pixels
        // to special pixels
        int osamp = 0;

        for(int samp = 0; samp < p_ns; samp++) {
          int bufferIndex = p_dataPreBytes + Isis::SizeOf(p_pixelType)*band + samp*sampleBytes;
          switch(p_pixelType) {
            case Isis::UnsignedByte:
              (*out)[osamp] = (double)((unsigned char *)in)[bufferIndex];
              break;
            case Isis::UnsignedWord:
              (*out)[osamp] =
                (double)swapper.UnsignedShortInt(&in[bufferIndex]);
              break;
            case Isis::SignedWord:
              (*out)[osamp] = (double)swapper.ShortInt(&in[bufferIndex]);
              break;
            case Isis::SignedInteger:
              (*out)[samp] = (double)swapper.Int(&in[bufferIndex]);
              break;
          case Isis::UnsignedInteger:
            (*out)[samp] = (double)swapper.Uint32_t(&in[bufferIndex]);
            break;
            case Isis::Real:
              if(p_vax_convert) {
                (*out)[osamp]= VAXConversion(&in[bufferIndex]);
              }
              else {
                (*out)[osamp] = (double)swapper.Float(&in[bufferIndex]);
              }
              break;
            case Isis::Double:
              (*out)[osamp] = (double)swapper.Double(&in[bufferIndex]);
              break;
            default:
              break;
          }

          // Sets out to isis special pixel or leaves it if valid
          (*out)[osamp] = TestPixel((*out)[osamp]);

          if (Isis::IsValidPixel((*out)[osamp])) {
            (*out)[osamp] = mult * ((*out)[osamp]) + base;
          }
          osamp++;
        } // End sample loop

        if (funct == NULL) {
          //Set the buffer position and write the line to the output file
          ((Isis::LineManager *)out)->SetLine((band * p_nl) + line + 1);
          OutputCubes[0]->write(*out);
        }
        else {
          funct(*out);
        }

      } // End band loop

      // Handle record prefix and suffix
      if (p_saveDataPre) {
        for(int samp = 0; samp < p_ns; samp++) {
          char *samplePrefix = new char[p_dataPreBytes];
          memcpy(samplePrefix, &in[samp*sampleBytes], p_dataPreBytes);
          tempPre.push_back(samplePrefix);
        }
        p_dataPre.push_back(tempPre);
        tempPre.clear();
      }
      if (p_saveDataPost) {
        for(int samp = 0; samp < p_ns; samp++) {
          char *sampleSuffix = new char[p_dataPostBytes];
          int suffixIndex = p_dataPreBytes + Isis::SizeOf(p_pixelType)*p_nb + samp*sampleBytes;
          memcpy(sampleSuffix, &in[suffixIndex], p_dataPostBytes);
          tempPost.push_back(sampleSuffix);
        }
        p_dataPost.push_back(tempPost);
        tempPost.clear();
      }

      // Check the last io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_dataPreBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // Handle the data trailer
      pos = fin.tellg();
      if (p_saveDataTrailer) {
        p_dataTrailer.push_back(new char[p_dataTrailerBytes]);
        fin.read(p_dataTrailer.back(), p_dataTrailerBytes);
      }
      else {
        fin.seekg(p_dataTrailerBytes, ios_base::cur);
      }

      // Check the last io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_fileHeaderBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      p_progress->CheckStatus();

    } // End line loop

    // Handle the file trailer
    pos = fin.tellg();
    if (p_saveFileTrailer) {
      fin.seekg(0, ios_base::end);
      streampos e = fin.tellg();
      p_fileTrailerBytes = (int)(e - pos + (streampos)1);
      p_fileTrailer = new char[p_fileTrailerBytes];
      fin.seekg(pos);
      fin.read(p_fileTrailer, p_fileTrailerBytes);

      // Check the io
      if (!fin.good()) {
        QString msg = "Cannot read file [" + p_inFile + "]. Position [" +
                     toString((int)pos) + "]. Byte count [" +
                     toString(p_fileTrailerBytes) + "]" ;
        throw IException(IException::Io, msg, _FILEINFO_);
      }

    }

    // Close the file and clean up
    fin.close();
    delete [] in;

  }


  /**
   * Function to process files containing compressed JPEG2000 data
   * (which is always BSQ but is processed as BIL with one or more
   * lines stored sequentially for each band). There is no need to
   * set up an Isis::EndianSwapper because JP2 data is always
   * assumed to be in MSB format. The Kakadu library does an
   * automatic byte swap for the current architecture.
   *
   * @param funct Method that accepts Isis::Buffer as an input
   *              parameter, processes the image, and has no
   *              return value.
   * @throws Isis::iException::Message "Cannot open input file."
   * @throws Isis::iException::Message "Cannot read file.
   *             Position[]. Byte count[]"
   */
  void ProcessImport::ProcessJp2(void funct(Isis::Buffer &out)) {

    // Set up an Isis::Jp2Decoder object
    JP2Decoder *JP2_decoder;
    JP2_decoder = new JP2Decoder(p_inFile);

    // Open JP2 file
    JP2_decoder->OpenFile();

    // Make sure JP2 file dimensions match PDS labels
    p_ns = JP2_decoder->GetSampleDimension();
    p_nl = JP2_decoder->GetLineDimension();
    p_nb = JP2_decoder->GetBandDimension();

    // Figure out the number of bytes to read for a single line
    // from all bands
    int sizeofpixel = Isis::SizeOf(p_pixelType);
    int startsamp = p_dataPreBytes / sizeofpixel;
    int endsamp = startsamp + p_ns;
    int readBytes = sizeofpixel * p_ns * p_nb
                    + p_dataPreBytes + p_dataPostBytes;
    char **in = new char* [p_nb];
    for(int i = 0; i < p_nb; i++) {
      in[i] = new char [readBytes];
    }

    // Construct a line buffer manager
    Isis::Buffer *out = NULL;

    if (funct != NULL) {
      out = new Isis::Buffer(p_ns, p_nl, p_nb, p_pixelType);
    }
    else {
      out = new Isis::LineManager(*OutputCubes[0]);
    }

    // Loop once for each line in the image
    p_progress->SetMaximumSteps(p_nb * p_nl);
    p_progress->CheckStatus();

    // Loop for each line
    for(int line = 0; line < p_nl; line++) {
      if (p_pixelType == Isis::UnsignedByte) {
        JP2_decoder->Read((unsigned char **)in);
      }
      else {
        JP2_decoder->Read((short int **)in);
      }
      // Loop for each band
      for(int band = 0; band < p_nb; band++) {
        // Set the base multiplier
        double base, mult;
        if (p_base.size() > 1) {
          base = p_base[band];
          mult = p_mult[band];
        }
        else {
          base = p_base[0];
          mult = p_mult[0];
        }

        // Space for storing prefix and suffix data pointers
        vector<char *> tempPre, tempPost;


        // Handle any line prefix bytes
        if (p_saveDataPre) {
          tempPre.push_back(new char[p_dataPreBytes]);
          memcpy(&tempPre[0], in[band], p_dataPreBytes);
        }

        // Swap the bytes if necessary and convert any out of bounds pixels
        // to special pixels
        for(int samp = startsamp; samp < endsamp; samp++) {
          switch(p_pixelType) {
            case Isis::UnsignedByte:
              (*out)[samp] = (double)((unsigned char *)in[band])[samp];
              break;
            case Isis::UnsignedWord:
              (*out)[samp] = (double)((unsigned short int *)in[band])[samp];
              break;
            case Isis::SignedWord:
              (*out)[samp] = (double)((short int *)in[band])[samp];
              break;
            default:
              break;
          }

          // Sets out to isis special pixel or leaves it if valid
          (*out)[samp] = TestPixel((*out)[samp]);

          if (Isis::IsValidPixel((*out)[samp])) {
            (*out)[samp] = mult * ((*out)[samp]) + base;
          }
        } // End sample loop

        if (funct == NULL) {
          //Set the buffer position and write the line to the output file
          ((Isis::LineManager *)out)->SetLine((band * p_nl) + line + 1);
          OutputCubes[0]->write(*out);
        }
        else {
          funct(*out);
        }

        p_progress->CheckStatus();

        // Handle any line suffix bytes
        if (p_saveDataPost) {
          tempPost.push_back(new char[p_dataPostBytes]);
          memcpy(&tempPost[0], &in[band][p_dataPreBytes+p_ns*sizeofpixel],
                 p_dataPostBytes);
        }

        // Save off the prefix bytes vector
        if (p_saveDataPre) {
          p_dataPre.push_back(tempPre);
          tempPre.clear();
        }

        // Save off the suffix bytes vector
        if (p_saveDataPost) {
          p_dataPost.push_back(tempPost);
          tempPost.clear();
        }

      } // End band loop

    } // End line loop

    // Close the file and clean up
    delete JP2_decoder;
    delete [] in;
  }


#if 0

  /**
   * Adds a label to the Cube section of the existing cube label using a
   * supplied Pvl
   *
   * @param label The Pvl containing the label to be added.
   */
  void ProcessImport::AddLabel(Isis::Pvl &label) {

    label.Root();

    Isis::Pvl *output = OutputCubes[0]->Label();
    output->Find("IsisCube");
    output->Insert(label);
  }

  /**
   * Adds an ImportLabel to the output cube using a supplied Pvl
   *
   * @param importlab The Pvl containing the original labels from the import
   * file.
   */
  void ProcessImport::AddImportLabel(Isis::Pvl &importLab) {

    importLab.Root();

    Isis::Pvl *output = OutputCubes[0]->Label();
    output->Find("IsisCube");
    output->addObject("ImportLabel");
    output->Insert(importLab);
  }
#endif


  /**
   * Sets the name of the input file to be read in the import StartProcess
   * method and verifies its existance.
   *
   * @param file The name of the input file to import.
   *
   * @throws Isis::iException::Message "File does not exist."
   */
  void ProcessImport::SetInputFile(const QString &file) {
    p_inFile = file;
    if (!Isis::FileName(file).fileExists()) {
      QString msg = "File [" + file + "] does not exist";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Sets the name of the input file to be read in the import StartProcess
   * method and verifies its existance.
   *
   * @param file The name of the input file to import.
   *
   * @throws Isis::iException::Message "File does not exist."
   */
  QString ProcessImport::InputFile() {
    if (p_inFile.size() <= 0) {
      QString msg = "No input file has been set";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    return p_inFile;
  }

}
