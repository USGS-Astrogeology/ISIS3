/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "PixelType.h"
#include "Buffer.h"
#include "IException.h"
#include "Message.h"

#include <iostream>

using namespace std;

namespace Isis {

  /**
   * @brief Default constructor for proper initialization purposes
   *
   * This constructor is mostly useless but is needed so that it protects
   * against improper usage.
   */
  Buffer::Buffer() : p_sample(0), p_nsamps(0), p_line(0), p_nlines(0),
    p_band(0), p_nbands(0), p_npixels(0), p_buf(0),
    p_pixelType(None), p_rawbuf(0) { }


  /**
   * Creates a Buffer object.  Note that the number of pixels in the buffer
   * can not exceed 2GB.
   *
   * @param nsamps Number of samples in shape.
   * @param nlines Number of lines in shape.
   * @param nbands Number of bands in shape.
   * @param type Raw buffer pixel type
   *
   * @throws Isis::iException::Programmer - Invalid value for a dimension
   */
  Buffer::Buffer(const int nsamps, const int nlines,
                 const int nbands, const Isis::PixelType type) :
    p_nsamps(nsamps), p_nlines(nlines),
    p_nbands(nbands), p_pixelType(type) {

    p_sample = p_line = p_band = 0;

    if(p_nsamps <= 0) {
      string message = "Invalid value for sample dimensions (nsamps)";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    if(p_nlines <= 0) {
      string message = "Invalid value for line dimensions (nlines)";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    if(p_nbands <= 0) {
      string message = "Invalid value for band dimensions (nbands)";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    p_npixels = p_nsamps * p_nlines * p_nbands;

    Allocate();
  }


  //! Destroys the Buffer object and frees shape buffer.
  Buffer::~Buffer() {
    try {
      if(p_buf) {
        delete [] p_buf;
      }
      if(p_rawbuf) {
        delete [](char *) p_rawbuf;
      }
    }
    catch(...) {

    }
  }


  /**
   * @brief Assign the entire buffer to a constant double value
   * @param d Value to assign to the buffer
   *
   * @return the current Buffer
   */
  Buffer &Buffer::operator=(const double &d) {
    for(int i = 0 ; i < p_npixels ; i++) {
      p_buf[i] = d;
    }
    return (*this);
  }


  /**
   * This method is used to set the base position of the shape buffer.  In general
   * it is used by BufferManager objects to progress sequentially through
   * a cube by line, tile, boxcar, etc.
   *
   * @param start_sample Starting sample to set.
   * @param start_line Starting line to set.
   * @param start_band Starting band to set.
   */
  void Buffer::SetBasePosition(const int start_sample,
                               const int start_line,
                               const int start_band) {
    SetBaseSample(start_sample);
    SetBaseLine(start_line);
    SetBaseBand(start_band);
  }


  /**
   * Returns the sample position associated with a shape buffer index.  The
   * shape buffer is one dimensional.  Let us assume a nsamps=2, nlines=3,
   * and nbands=2.  Therefore the total size of the shape buffer is 12 and valid
   * index values are 0-11.  Sample(0), Sample(2), Sample(4), etc will return a
   * 1 while Sample(1), Sample(3), Sample(5), etc will return a 2.
   *
   * @param index  Shape buffer index to map to a sample position.
   *               Defaults to 0.
   *
   * @return int The absolute sample number based on the buffer index.
   */
  int Buffer::Sample(const int index) const {
    return (index % p_nsamps) + p_sample;
  }


  /**
   * Returns the line position associated with a shape buffer index.  The
   * shape buffer is one dimensional.  Let us assume a nsamps=2, nlines=3, and
   * nbands=2.  Therefore the total size of the shape buffer is 12 and valid
   * index values are 0-11.  Line(0), Line(1), Line(6) and Line(7), will return
   * a 1, Line(2), Line(3), Line(8) and Line(9) will return a 2, and Line(4),
   * Line(5), Line(10), and Line(11) will return a 3.
   *
   * @param index  Shape buffer index to map to a line position.
   *               Defaults to 0
   *
   * @return int The absolute line number based on the buffer index
   */
  int Buffer::Line(const int index) const {
    int sub_index = index % (p_nsamps * p_nlines);
    return sub_index / p_nsamps + p_line;
  }


  /**
   * Returns the band position associated with a shape buffer index.  The
   * shape buffer is one dimensional.  Let us assume a nsamps=2, nlines=3, and
   * nbands=2.  Therefore the total size of the shape buffer is 12 and valid
   * index values are 0-11.  Indexes 0-5 will return band 1 and 6-11 will return
   * band 2.
   *
   * @param index Shape buffer index to map to a band position.  Defaults to 0.
   *
   * @return int The absolute band number based on the buffer index.
   */
  int Buffer::Band(const int index) const {
    return index / (p_nsamps * p_nlines) + p_band;
  }


  /**
   * Returns the sample, line, and band position associated with a
   * shape buffer index.  Performs the same function as the Sample, Line,
   * and Band methods.
   *
   * @param index Shape buffer index to map to a band position.
   * @param i_samp Sample position in shape buffer at index.
   * @param i_line Line position in shape buffer at index
   * @param i_band Band position in shape buffer at index.
   */
  void Buffer::Position(const int index, int &i_samp, int &i_line,
                        int &i_band) const {
    i_samp = Sample(index);
    i_line = Line(index);
    i_band = Band(index);
  }


  /**
   * Given a sample, line, and band position, this returns the appropriate index
   * in the shape buffer.
   *
   * @param i_samp Sample position.
   * @param i_line Line position.
   * @param i_band Band position.
   *
   * @return int Index at the specified sample, line and band.
   *
   * @throws Isis::iException::Programmer - Array Subscript not in range
   */
  int Buffer::Index(const int i_samp, const int i_line, const int i_band)
      const {
    if((i_samp < p_sample) || (i_samp > (p_sample + p_nsamps - 1))) {
      std::string message = Message::ArraySubscriptNotInRange(i_samp);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    if((i_line < p_line) || (i_line > (p_line + p_nlines - 1))) {
      std::string message = Message::ArraySubscriptNotInRange(i_line);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    if((i_band < p_band) || (i_band > (p_band + p_nbands - 1))) {
      std::string message = Message::ArraySubscriptNotInRange(i_band);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    //  Got a valid reference location so compute the index and return
    int index = (i_band - p_band) * (p_nlines * p_nsamps) +
                (i_line - p_line) * (p_nsamps) +
                (i_samp - p_sample);
    return (index);
  }


  /**
   * Returns the value in the shape buffer at the given index.
   *
   * @param index Index position in buffer. Out of bounds index is trapped.
   *
   * @return double Buffer value at index
   *
   * @throws Isis::iException::Programmer - Array Subscript not in range
   */
  double Buffer::at(const int index) const {
    if(index < 0) {
      std::string message = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    else if(index >= p_npixels) {
      std::string message = Message::ArraySubscriptNotInRange(index);
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    return p_buf[index];
  }


  /**
   * Allows copying of the buffer contents to another Buffer
   *
   * @param in The Buffer to be copied.
   * @param includeRawBuf Whether to include raw dm read from disk
   *
   * @throws Isis::iException::Programmer - Input and Output buffers are not the same size
   * @throws Isis::iException::Programmer - Input and Output buffers do not use the same pixel
   *                                        type
   */
  void Buffer::Copy(const Buffer &in, bool includeRawBuf) {
    if(p_npixels != in.size()) {
      string message = "Input and output buffers are not the same size";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    if(includeRawBuf && p_pixelType != in.PixelType()) {
      string message = "Input and output buffers are not the same pixel type";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    size_t n = sizeof(double);
    n = n * (size_t) p_npixels;
    memcpy(p_buf, in.p_buf, n);

    if (includeRawBuf) {
      n = Isis::SizeOf(p_pixelType);
      n = n * (size_t) p_npixels;
      memcpy(p_rawbuf, in.p_rawbuf, n);
    }
  }


  /**
   * Allows copying of the buffer contents of a larger buffer to another same size or smaller
   *   Buffer, using their base positions to relate data. This does not copy the raw buffer.
   *
   * @param in The Buffer to be copied.
   * @return The operation was successful (the buffers overlapped)
   */
  bool Buffer::CopyOverlapFrom(const Buffer &in) {
    bool isSubareaOfIn = (p_npixels <= in.size());
    isSubareaOfIn &= (p_sample >= in.p_sample);
    isSubareaOfIn &= (p_line >= in.p_line);
    isSubareaOfIn &= (p_band >= in.p_band);

    int endSample = p_sample + p_nsamps - 1;
    int otherEndSample = in.p_sample + in.p_nsamps - 1;

    int endLine = p_line + p_nlines - 1;
    int otherEndLine = in.p_line + in.p_nlines - 1;

    int endBand = p_band + p_nbands - 1;
    int otherEndBand = in.p_band + in.p_nbands - 1;

    isSubareaOfIn &= (endSample <= otherEndSample);
    isSubareaOfIn &= (endLine <= otherEndLine);
    isSubareaOfIn &= (endBand <= otherEndBand);

    if (isSubareaOfIn) {
      for (int i = 0; i < size(); i++) {
        (*this)[i] = in[in.Index(Sample(i), Line(i), Band(i))];
      }
    }

    return isSubareaOfIn;
  }


  /**
   * The copy constructor. Allows a new Buffer object to be created using
   * an existing Buffer object.
   *
   * @param rhs The Buffer to be used to create the new buffer
   */
  Buffer::Buffer(const Buffer &rhs) :
    p_nsamps(rhs.p_nsamps), p_nlines(rhs.p_nlines),
    p_nbands(rhs.p_nbands), p_pixelType(rhs.p_pixelType) {

    p_sample = rhs.p_sample;
    p_line = rhs.p_line;
    p_band = rhs.p_band;

    p_npixels = rhs.p_npixels;

    Allocate();
    Copy(rhs);
  }


  /**
   * Size or resize the memory buffer.
   *
   * @throws Isis::iException::System - Memory allocation failed
   */
  void Buffer::Allocate() {
    p_buf = NULL;
    p_rawbuf = NULL;
    try {
      p_buf = new double [p_npixels];
      size_t n = Isis::SizeOf(p_pixelType);
      n = n * (size_t) p_npixels;
      p_rawbuf = new char[n];
    }
    catch(...) {
      try {
        if(p_buf) {
          delete [] p_buf;
          p_buf = NULL;
        }
        if(p_rawbuf) {
          delete [](char *)p_rawbuf;
          p_rawbuf = NULL;
        }
      }
      catch(...) {
        p_buf = NULL;
        p_rawbuf = NULL;
      }
      QString message = Message::MemoryAllocationFailed();
      throw IException(IException::Unknown, message, _FILEINFO_);
    }
  }
}
