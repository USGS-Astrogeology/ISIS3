#ifndef ProcessExport_h
#define ProcessExport_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Process.h"

#include <fstream>
#include <iostream>
#include <string>

#include <QCryptographicHash>
#include <QString>

#include "Buffer.h"
#include "BufferManager.h"
#include "IEndian.h"
#include "EndianSwapper.h"
#include "SpecialPixel.h"
#include "Stretch.h"
#include "UserInterface.h"

namespace Isis {
  /**
   * @brief Process class for exporting cubes
   *
   * This class allows a programmer to develop applications which export Isis
   * cubes into another format. For example, isis2jpg or isis2tif. It is highly
   * recommended that this object be utilized when developing export applications
   * to ensure a consistent look-and-feel for Isis users. The class operates by
   * passing the programmer a line of cube data at a time. It is up to the
   * programmer to write this data to the foreign output format. An ability exists
   * to stretch the data supplied to the programmer in one of three ways. Either
   * an automatic linear stretch, an automatic piecewise stretch, or an manual
   * linear stretch. There are various methods which specify how the input pixels
   * are to be stretched to an output range. You can examine the isis2jpg
   * application code as a guide for writing an export program. Currently this
   * class only allows for one band of a cube to be exported.
   *
   * If you would like to see ProcessExport being used in implementation,
   * see isis2jpg.cpp
   *
   * @ingroup HighLevelCubeIO
   *
   * @author 2003-03-31 Jeff Anderson
   *
   * @internal
   *  @history 2003-04-03 Jeff Anderson - Added unit test
   *  @history 2003-04-04 Jeff Anderson - Updated documentation for
   *                          SetInputRange methods
   *  @history 2003-05-16 Stuart Sides - Modified schema from astrogeology...
   *                          isis.astrogeology...
   *  @history 2005-01-07 Stuart Sides - Added CreateWorldFile method
   *  @history 2005-02-08 Elizabeth Ribelin - Modified file to support Doxygen
   *                          documentation
   *  @history 2005-06-14 Drew Davidson - Overloaded StartProcess method to output
   *                          directly to a stream.
   *  @history 2005-06-15 Drew Davidson - Updated to support multi-band output.
   *  @history 2006-02-27 Jacob Danton - Added Multiple input cube support
   *  @history 2006-05-08 Elizabeth Miller - Modified SetInputRange() to get the
   *                          min and max percent values from the userinterface
   *                          (0.5 and 99.5 are still the default values)
   *  @history 2006-05-15 Jeff Anderson - Fixed bug with multiple input cube
   *                          support when the programmer didn't set a input
   *                          minimum/maximum
   *  @history 2006-05-23 Jacob Danton - Added seperate MIN/MAX values for each
   *                          input channel
   *  @history 2006-08-30 Jeff Anderson - Fixed memory leak
   *  @history 2007-12-17 Christopher Austin - Added processes for BIL
   *                          and BIP, leaving BSQ as the default, as well as
   *                          fixed rounding accuracy.
   *  @history 2008-05-12 Steven Lambright - Removed references to CubeInfo
   *  @history 2008-06-18 Steven Koechle - Fixed Documentation Errors
   *  @history 2008-08-14 Christopher Austin - Added the Destructor to fix
   *                          memory leaks, as well as changed the
   *                          EndianSwapper::Float() call to
   *                          EndianSwapper::ExportFloat() in isisOut32() to fix
   *                          bad float casting.
   *  @history 2008-12-17 Steven Lambright - Changed SetOutputRange calls to use
   *                          constants (i.e. instead of 65535 VALID_MAX2 is used).
   *  @history 2009-07-27 Steven Lambright - Piecewise stretch backs off to linear
   *                          if Median() == MINPCT or Median() == MAXPCT
   *  @history 2010-02-24 Janet Barrett - Added code to support JPEG2000
   *  @history 2012-10-04 Jeannie Backer - Added documentation and fixed
   *                          indentation of history entries. No mantis ticket.
   *  @history 2013-06-05 Jeannie Backer - Replaced redundant code in
   *                          InitProcess() with accessor methods for
   *                          OutputNull(), et al. Changed local variable names
   *                          in ProcessCubes for clarity. References #1380.
   *  @history 2015-01-15 Sasha Brownsberger - Added virtual keyword to several
   *                          functions to ensure successful
   *                          inheritance between Process and its
   *                          child classes.  Added virtual keyword
   *                          to destructor.  References #2215.
   *  @history 2016-04-21 Makayla Shepherd - Added UnsignedWord pixel type handling.
   *  @history 2017-05-17 Makayla Shepherd - Added setCanGenerateChecksum(), canGenerateChecksum(),
   *                          and checksum(). Added m_cryptographicHash and m_canGenerateChecksum.
   *                          This allows an MD5 checksum to be generated when exporting an image.
   *                          This checksum is generated based on the image data. Fixes #1013.
   *  @todo 2005-02-09 Stuart Sides - write documentation for CreateWorldFile
   *                          method
   *  @todo 2005-02-09 Jeff Anderson - add coded example to class file and
   *                          implementation examples
   *  @history 2018-09-28 Kaitlyn Lee - Added (char) cast to fix implicit conversion. Split up
   *                          "-(short)32768" into two lines. Fixes build warnings on MacOS 10.13.
   *                          Updated code up to standards. References #5520.
   */
  class ProcessExport : public Isis::Process {

    public:

      //! Storage order enumeration
      enum ExportFormat {
        BSQ, //!< Band sequential
        BIL, //!< Band interleaved by line
        BIP, //!< Band interleaved by pixel
        JP2  //!< Compressed JPEG2000
      };

      ProcessExport();
      virtual ~ProcessExport();
      using Isis::Process::StartProcess;  // make parents virtual function visable
      virtual void StartProcess(void funct(Isis::Buffer &in));
      virtual void StartProcess(void funct(std::vector<Isis::Buffer *> &in));
      virtual void StartProcess(std::ofstream &fout);
      void SetOutputRange(const double minimum, const double maximum);
      void SetOutputNull(const double value);
      void SetOutputLis(const double value);
      void SetOutputLrs(const double value);
      void SetOutputHis(const double value);
      void SetOutputHrs(const double value);
      double OutputNull();
      double OutputLis();
      double OutputLrs();
      double OutputHis();
      double OutputHrs();
      bool HasInputRange() const;
      void SetInputRange();
      void SetInputRange(UserInterface &ui);
      void SetInputRange(const double minimum, const double maximum);
      void SetInputRange(const double minimum, const double maximum, const int index);
      void SetInputRange(const double minimum, const double middle,
                         const double maximum);
      void SetInputRange(const double minimum, const double middle,
                         const double maximum, const int index);
      void CreateWorldFile(const QString &worldFile);
      void SetOutputEndian(enum ByteOrder endianness);
      void SetOutputType(Isis::PixelType pixelIn);

      void setCanGenerateChecksum(bool flag);
      bool canGenerateChecksum();
      QString checksum();

      double GetInputMinimum(unsigned int n=0) const;
      double GetInputMaximum(unsigned int n=0) const;

      //! Get the valid minimum pixel value to be written to the output file
      double GetOutputMinimum() {
        return (p_outputMinimum);
      };
      //! Get the valid maximum pixel value to be written to the output file
      double GetOutputMaximum() {
        return (p_outputMaximum);
      };

      //! Sets the storage order of the output file
      void setFormat(ExportFormat format) {
        p_format = format;
      };


      template <typename Functor> void ProcessCubes(const Functor & functor) {

        int samples = InputCubes[0]->sampleCount();
        int length = 0;
        if (p_format == BIP) {
          length = InputCubes[0]->bandCount();
        }
        else {
          length = InputCubes[0]->lineCount();
        }

        // Loop and let the app programmer fiddle with the lines
        std::vector<BufferManager *> imgrs = GetBuffers();
        for (int k = 1; k <= length; k++) {
          std::vector<Buffer *> ibufs;

          for (unsigned int cubeIndex = 0; cubeIndex < InputCubes.size(); cubeIndex++) {
            // Read a line of data from this cube
            InputCubes[cubeIndex]->read(*imgrs[cubeIndex]);

            // Stretch the pixels into the desired range
            for (int sampleIndex = 0; sampleIndex < samples; sampleIndex++) {
              (*imgrs[cubeIndex])[sampleIndex] =
                  p_str[cubeIndex]->Map((*imgrs[cubeIndex])[sampleIndex]);
            }

            ibufs.push_back(imgrs[cubeIndex]);
          }

          // Invoke the user function
          functor(ibufs);

          for (unsigned int i = 0; i < imgrs.size(); i++) imgrs[i]->next();
          p_progress->CheckStatus();
        }
      }

    protected:

      //! Current storage order
      ExportFormat p_format;

      std::vector<BufferManager *> GetBuffers();
      std::vector<BufferManager *> GetBuffersBSQ();
      std::vector<BufferManager *> GetBuffersBIL();
      std::vector<BufferManager *> GetBuffersBIP();

      double p_outputMinimum; //!< Desired minimum pixel value in the Buffer
      double p_outputMiddle;  /**< Middle pixel value (minimum+maximun)/2.0 in
                                   the Buffer */
      double p_outputMaximum; //!< Desired maximum pixel value in the Buffer

      std::vector<double> p_inputMinimum; /**< Minimum pixel value in the input
                                          cube to be mapped to the minimum
                                          value in the Buffer */
      std::vector<double> p_inputMiddle;  /**< Middle pixel value in the input
                                          cube to be mapped to the
                                          (minimum+maximum)/2.0 value in the
                                          Buffer */
      std::vector<double> p_inputMaximum; /**< Maximum pixel value in the input
                                          cube to be mapped to the maximum
                                          value in the Buffer */
      EndianSwapper *p_endianSwap; /**< Object to swap the endianness of the
                                        raw output to either MSB or LSB */
      ByteOrder p_endianType; //!< The byte order of the output file

      PixelType p_pixelType;  /**< The bits per pixel of the output image*/

      std::vector<Stretch *> p_str; /**< Stretch object to ensure a reasonable
                                         range of pixel values in the output
                                         data**/

      double p_Null; /**< The output value for pixels whose input DNs are Null values.*/
      double p_Lis;  /**< The output value for pixels whose input DNs are
                          Low Instrument Saturation values.*/
      double p_Lrs;  /**< The output value for pixels whose input DNs are
                          Low Representation Saturation values.*/
      double p_His;  /**< The output value for pixels whose input DNs are
                          High Instrument Saturation values.*/
      double p_Hrs;  /**< The output value for pixels whose input DNs are
                          High Representation Saturation values.*/

      bool p_Null_Set; /**< Indicates whether p_Null has been set
                            (i.e. if setNull() has been called).*/
      bool p_Lis_Set;  /**< Indicates whether p_Lis has been set
                            (i.e. if setLis() has been called).*/
      bool p_Lrs_Set;  /**< Indicates whether p_Lrs has been set
                            (i.e. if setLrs() has been called).*/
      bool p_His_Set;  /**< Indicates whether p_His has been set
                            (i.e. if setHis() has been called).*/
      bool p_Hrs_Set;  /**< Indicates whether p_Hrs has been set
                            (i.e. if setHrs() has been called).*/

      QCryptographicHash *m_cryptographicHash; /**< A cryptographic hash that will generate an MD5
                                                    checksum of the image data. */
      bool m_canGenerateChecksum;  /**< Flag to determine if a file checksum will be generated. */

    private:
      //!Method for writing 8-bit unsigned pixel data to a file stream
      void isisOut8(Buffer &in, std::ofstream &fout);

      //!Method for writing 16-bit signed pixel data to a file stream
      void isisOut16s(Buffer &in, std::ofstream &fout);

      //!Method for writing 16-bit unsigned pixel data to a file stream
      void isisOut16u(Buffer &in, std::ofstream &fout);

      /**Method for writing 32-bit signed floating point pixels data to a
      file stream*/
      void isisOut32(Buffer &in, std::ofstream &fout);

      /**Method for writing 64-bit signed double precision floating point pixels
      data to a file stream*/
      void isisOut64(Buffer &in, std::ofstream &fout);

      /** Convenience method that checks to make sure the user is only using
      valid input to the StartProcess method. Also sets the cube up to be
      processed by performing the necessary stretches.*/
      void InitProcess();


  };
};

#endif
