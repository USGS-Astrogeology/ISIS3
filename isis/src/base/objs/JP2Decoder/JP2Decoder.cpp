/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <float.h>
#include <iostream>
#include <string>
#include <sstream>

#include "IException.h"
#include "IString.h"
#include "JP2Decoder.h"
#include "JP2Error.h"

using namespace std;

#if ENABLEJP2K
using namespace kdu_core;
using namespace kdu_supp;
#endif

namespace Isis {

  /**
   * Constructs a JPEG2000 decoder object
   *
   * @param jp2file The name of the JP2 file that needs to be decoded.
   *
   */
  JP2Decoder::JP2Decoder(const QString &jp2file) {

#if ENABLEJP2K
    p_jp2File = jp2file;
    p_resolutionLevel = 1;
    JP2_Source = NULL;

    // Register the Kakadu error handler
    Kakadu_Error = new JP2Error;
    kdu_customize_errors(Kakadu_Error);
#else
    std::string msg = "JPEG2000 has not been enabled with this build of ISIS3";
    throw IException(IException::Programmer, msg, _FILEINFO_);
#endif
  }

  /**
   * Open the JPEG2000 file
   *
   */
  void JP2Decoder::OpenFile() {
#if ENABLEJP2K
    // Make sure file isn't already open
    if(JP2_Source == NULL) {

      // Open the JP2 file stream
      JP2_Stream = new jp2_family_src();
      JP2_Stream->open(p_jp2File.toLatin1().data());

      // Open the JP2 source
      JP2_Source = new jp2_source();
      if(!JP2_Source->open(JP2_Stream)) {
        std::string msg = "Unable to open the decoder because the source file ";
        msg += "does not have valid JP2 format content [" + p_jp2File + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // Initialize the JP2 header boxes up to the first codestream box
      JP2_Source->read_header();

      // Open the JP2 codestream
      JPEG2000_Codestream = new kdu_codestream();
      JPEG2000_Codestream->create(JP2_Source);

      // Get the image characteristics
      // Number of components (bands)
      p_numBands = JPEG2000_Codestream->get_num_components(true);

      // Image dimensions (sample offset, line offset, number of samples,
      // number of lines) at full resolution
      JPEG2000_Codestream->get_dims(0, p_imageDims, true); //dims.pos.x, dims.size.x

      // Pixel data structure
      p_pixelBits = JPEG2000_Codestream->get_bit_depth(0, true);
      p_pixelBytes = (p_pixelBits >> 3) + ((p_pixelBits % 8) ? 1 : 0);
      if(p_pixelBytes == 3) p_pixelBytes = 4;
      if(p_pixelBits > 16 || p_pixelBytes > 2) {
        std::string msg = "The source file has unsupported pixel type ";
        msg += "[" + p_jp2File + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      p_signedData = JPEG2000_Codestream->get_signed(0, true);

      // Check all bands in the JP2 file to make sure they all have the same
      // dimensions, bit depth, and signedness
      kdu_dims dims;
      unsigned int pixel_bits;
      bool signed_data;
      for(unsigned int band = 1; band < p_numBands; ++band) {
        JPEG2000_Codestream->get_dims(band, dims, true);
        pixel_bits = JPEG2000_Codestream->get_bit_depth(band, true);
        signed_data = JPEG2000_Codestream->get_signed(band, true);
        if(dims.size.x != p_imageDims.size.x || dims.size.y != p_imageDims.size.y ||
            dims.pos.x != p_imageDims.pos.x || dims.pos.y != p_imageDims.pos.y ||
            pixel_bits != p_pixelBits || signed_data != p_signedData) {
          std::string msg = "The source file does not have bands with matching ";
          msg += "characteristics";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }

      // Get the total available resolution levels and set the effective
      // resolution and image region
      p_highestResLevel = JPEG2000_Codestream->get_min_dwt_levels() + 1;
      SetResolutionAndRegion();

      // Initialize the JP2 decoder
      // Initialize the codestream stripe decompressor
      p_decompressor.start(*JPEG2000_Codestream);

      // Determine optimum stripe heights for accessing data - the
      // optimum stripe heights are ignored. We are instead reading
      // the file a line at a time.
      p_stripeHeights = new int[p_numBands];
      p_maxStripeHeights = new int[p_numBands];
      p_precisions = new int[p_numBands];
      p_isSigned = new bool[p_numBands];
      p_decompressor.get_recommended_stripe_heights(MIN_STRIPE_HEIGHT,
          MAX_STRIPE_HEIGHT, p_stripeHeights, p_maxStripeHeights);
      for(unsigned int i = 0; i < p_numBands; i++) {
        p_precisions[i] = p_pixelBits;
        p_stripeHeights[i] = 1;
        p_isSigned[i] = p_signedData;
      }
    }
#endif
  }

  /**
   * Set resolution level of the JPEG2000 file. This class is currently set
   * up to only read a file at full resolution.
   *
   */
  void JP2Decoder::SetResolutionAndRegion() {
#if ENABLEJP2K
    // Determine size of image at requested resolution and reset requested image
    // area if it falls outside of image boundaries
    JPEG2000_Codestream->apply_input_restrictions(0, 0, p_resolutionLevel - 1, 0, NULL,
        KDU_WANT_OUTPUT_COMPONENTS);

    JPEG2000_Codestream->get_dims(0, p_imageDims, true);
    p_numSamples = p_imageDims.size.x;
    p_numLines = p_imageDims.size.y;
#endif
  }

  /**
   * Read data from JP2 file containing 8-bit data
   *
   * @param inbuf The array of pointers to byte buffers that will be used to read
   *              in the image data. One byte buffer is required for each band in
   *              the image. Kakadu reads in a BIL manner. It is up to the calling
   *              routine to do the casting necessary to convert the byte data to
   *              the actual pixel type (UnsignedByte, UnsignedWord, SignedWord).
   *
   */
  void JP2Decoder::Read(unsigned char **inbuf) {
#if ENABLEJP2K
    p_readStripes = p_decompressor.pull_stripe(inbuf, p_stripeHeights, NULL, NULL,
                    p_precisions);
#endif
  }

  /**
   * Read data from JP2 file containing 16-bit data
   *
   * @param inbuf The array of pointers to byte buffers that will be used to read
   *              in the image data. One byte buffer is required for each band in
   *              the image. Kakadu reads in a BIL manner. It is up to the calling
   *              routine to do the casting necessary to convert the byte data to
   *              the actual pixel type (UnsignedByte, UnsignedWord, SignedWord).
   *
   */
  void JP2Decoder::Read(short int **inbuf) {
#if ENABLEJP2K
    p_readStripes = p_decompressor.pull_stripe(inbuf, p_stripeHeights, NULL, NULL,
                    p_precisions, p_isSigned);
#endif
  }

  /**
   * JP2Decoder destructor
   *
   */
  JP2Decoder::~JP2Decoder() {
#if ENABLEJP2K
    // See kdu_stripe_decompressor::reset documentation:
    // "You should be sure to call this function or finish before destroying the kdu_codestream
    // inteface that was passed to start."
    // i.e. Make sure to finish the decompressor before destroying the kdu_codestream.
    p_decompressor.finish();
    if(JPEG2000_Codestream) {
      JPEG2000_Codestream->destroy();
    }
    JPEG2000_Codestream = NULL;
    if(JP2_Source) {
      JP2_Source->close();
      delete JP2_Source;
    }
    JP2_Source = NULL;
    if(JP2_Stream) {
      JP2_Stream->close();
      delete JP2_Stream;
    }
    JP2_Stream = NULL;
    if(Kakadu_Error) {
      delete Kakadu_Error;
    }
    delete [] p_stripeHeights;
    delete [] p_maxStripeHeights;
    delete [] p_precisions;
    delete [] p_isSigned;
#endif
  }


  bool JP2Decoder::IsJP2(QString filename) {
#if ENABLEJP2K
    jp2_family_src *stream = new jp2_family_src();
    stream->open(filename.toLatin1().data());
    jp2_source *source = new jp2_source();

    bool result = source->open(stream);

    source->close();
    delete source;

    stream->close();
    delete stream;

    return result;
#else
    return (false);
#endif
  }
}
