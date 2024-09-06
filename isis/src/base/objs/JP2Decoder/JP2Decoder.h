#ifndef JP2Decoder_h
#define JP2Decoder_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <QString>

#if ENABLEJP2K
#include "jp2.h"
#include "kdu_stripe_decompressor.h"
#endif

#define MIN_STRIPE_HEIGHT 256
#define MAX_STRIPE_HEIGHT 8192

namespace Isis {
  class JP2Error;

  /**
   * @brief  JPEG2000 decoder class
   *
   * This class is used to decode a JPEG2000 image.
   *
   * Here is an example of how to use JP2Decoder
   * @code
   *   JP2Decoder *JP2_decoder;
   *   JP2_decoder = new JP2Decoder(QString(ui.GetFileName("FROM")));
   *   JP2_decoder->OpenFile();
   *   int nsamps = JP2_decoder->GetSampleDimension();
   *   int nlines = JP2_decoder->GetLineDimension();
   *   int nbands = JP2_decoder->GetBandDimension();
   *   int pixelbytes = JP2_decoder->GetPixelBytes();
   *   bool is_signed = JP2_decoder->GetSignedData();
   *   delete JP2_decoder;
   *   ProcessImport jp;
   *   jp.SetDimensions(nsamps,nlines,nbands);
   *   if (pixelbytes == 1) {
   *     jp.SetPixelType(Isis::UnsignedByte);
   *   } else if (pixelbytes == 2) {
   *     if (is_signed) {
   *       jp.SetPixelType(Isis::SignedWord);
   *     } else {
   *       jp.SetPixelType(Isis::UnsignedWord);
   *     }
   *   } else {
   *     throw iException::Message(iException::User,
   *       "The file [" + ui.GetFileName("FROM") + "] contains unsupported data type.",
   *       _FILEINFO_);
   *   }
   *   jp.SetInputFile(QString(ui.GetFileName("FROM")));
   *   jp.SetOutputCube("TO");
   *   jp.SetOrganization(ProcessImport::JP2);
   *   jp.StartProcess();
   *   jp.EndProcess();
   * @endcode
   *
   * If you would like to see JP2Decoder being used in implementation,
   * see std2isis.cpp or for a class that implements JP2Decoder,
   * see ProcessImport
   *
   * @ingroup HighLevelCubeIO
   *
   * @author 2009-12-18 Janet Barrett
   *
   * @internal
   *  @history 2009-12-18 Janet Barrett - Original version.
   *  @history 2012-04-06 Kris Becker - Fixed condition compilation where
   *                        support for JP2K is disabled
   *  @history 2016-08-28 Kelvin Rodriguez - Moved member variables to be placed properly
   *                        within the if ENABLEJP2K preprocessor block in order to stop
   *                        unused member variable warnings in clang. Part of porting to OS X 10.11.
   *  @history 2017-08-21 Tyler Wilson, Ian Humphrey, Summer Stapleton - Added
   *                       support for new kakadu libraries.  References #4809.
   *  @history 2017-09-15 Ian Humphrey - Modified destructor to call finish() on the decompressor
   *                          before destroying the kdu_codestream. Caused segfault on OSX 10.11
   *                          for the JP2Importer test, and isis2std and std2isis jpeg2000 tests.
   *                          References #4809.
   */
  class JP2Decoder {
    public:
      JP2Decoder(const QString &jp2file);
      ~JP2Decoder();

      // Register with the Kakadu error facility
      JP2Error *kakadu_error() const {
        return Kakadu_Error;
      };

      // Open and initialize the JP2 file for reading
      void OpenFile();

      // Get the sample dimension of the JP2 file
      inline int GetSampleDimension() const {
        return ((int) p_numSamples);
      }

      // Get the line dimension of the JP2 file
      inline int GetLineDimension() const {
        return ((int) p_numLines);
      }

      // Get the band dimension of the JP2 file
      inline int GetBandDimension() const {
        return ((int) p_numBands);
      }

      // Get number of bytes per pixel in the JP2 file
      inline int GetPixelBytes() const {
        return (p_pixelBytes);
      }

      // Determine if data in JP2 file is signed
      inline bool GetSignedData() const {
        return (p_signedData);
      }

      // Read byte data from the JP2 file
      void Read(unsigned char **inbuf);

      // Read 16-bit data from the JP2 file
      void Read(short int **inbuf);

      static bool IsJP2(QString filename);

    private:
      QString p_jp2File;          //!<Input file name
      unsigned int p_numSamples;      //!<Number of samples in JP2 file
      unsigned int p_numLines;        //!<Number of lines in JP2 file
      unsigned int p_numBands;        //!<Number of bands in JP2 file
      unsigned int p_pixelBytes;      //!<Number of bytes per pixel in JP2 file.
      bool p_signedData;              //!<Set to true if data in JP2 file is signed.

#if ENABLEJP2K
      unsigned int p_resolutionLevel; //!<Resolution level that file will be decompressed
      //!<at. Always full resolution.
      unsigned int p_highestResLevel; //!<Total number of available resolution levels in
      //!<JP2 file.
      int *p_maxStripeHeights;        //!<Determines the maximum number of lines that can
      //!<be read at a time from the JP2 file.
      int *p_precisions;              //!<Determines the bit precision of each band in
      //!<the JP2 file.
      bool *p_isSigned;               //!<Determines if the data is signed/unsigned for
      //!<each band in the JP2 file.
      int *p_stripeHeights;           //!<Determines how many lines are read at a time
      //!<from the JP2 file.

      unsigned int p_pixelBits;       //!<Number of bits per pixel in JP2 file.
      bool p_readStripes;             //!<Number of lines read per call to Read methods


      kdu_core::kdu_dims p_imageDims;           //!<Image dimensions of JP2 file
      kdu_supp::jp2_family_src *JP2_Stream;     //!<JP2 file input stream
      kdu_supp::jp2_source *JP2_Source;         //!<JP2 content source
      kdu_core::kdu_codestream *JPEG2000_Codestream;    //!<Allow access to JP2 file codestream.
      kdu_supp::kdu_stripe_decompressor p_decompressor; //!<High level interface to decompression of
      //!<JP2 file.
#endif
      JP2Error *Kakadu_Error;         //!<JP2 Error handling facility

      void SetResolutionAndRegion();  //!<Sets resolution of data that will be decompressed.
      //!<Also determines the image dimensions at the requested
      //!<resolution.
  };
};
#endif
