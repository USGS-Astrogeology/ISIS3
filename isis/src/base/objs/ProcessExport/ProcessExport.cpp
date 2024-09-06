/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <iomanip>
#include <QCryptographicHash>
#include <QString>

#include "ProcessExport.h"
#include "Preference.h"
#include "IException.h"
#include "LineManager.h"
#include "BandManager.h"
#include "SpecialPixel.h"
#include "Histogram.h"
#include "Stretch.h"
#include "Application.h"
#include "EndianSwapper.h"
#include "Projection.h"

using namespace std;
namespace Isis {

  //! Constructs an Export object
  ProcessExport::ProcessExport() : Isis::Process() {

    p_outputMinimum = 0.0;
    p_outputMiddle = 0.5;
    p_outputMaximum = 1.0;

    p_inputMinimum.clear();
    p_inputMiddle.clear();
    p_inputMaximum.clear();

    p_endianSwap = NULL;

    setFormat(BSQ);
    SetOutputEndian(Isis::IsLsb() ? Isis::Lsb : Isis::Msb);
    SetOutputType(Isis::Real);

    p_Null_Set = false;
    p_Lis_Set = false;
    p_Lrs_Set = false;
    p_His_Set = false;
    p_Hrs_Set = false;

    m_cryptographicHash = new QCryptographicHash(QCryptographicHash::Md5);
    m_canGenerateChecksum = false;

    p_progress->SetText("Exporting");
  }


  //! Destructor
  ProcessExport::~ProcessExport() {
    if (p_endianSwap != NULL) {
      delete p_endianSwap;
    }
    for (unsigned int i = 0; i < p_str.size(); i++) {
      delete p_str[i];
    }
    p_str.clear();

    delete m_cryptographicHash;
  }


  /**
   * @brief Set input pixel range from to a linear stretch
   *
   * This method allows the programmer to define what range of input pixels in
   * the input cube get mapped to the output range in the Buffer. By default the
   * output range is 0.0 to 1.0 (can be overridden using the SetOutputRange
   * method). This version of SetInputRange allows the programmer to perform a
   * simple linear stretch. That is, "minimum:0.0 maximum:1.0" or minimum is
   * mapped to 0.0, maximum is mapped to 1.0 everything inbetween is mapped
   * linearly (e.g., (minimum+maximum)/2.0:0.5). Also, everything less than the
   * minimum is mapped to 0.0 and everything more than the maximum is mapped to
   * 1.0.  If you are uncertain about how stretches operate see the Stretch
   * object. If the input range is never set, no stretch will occur.
   *
   * @param minimum Minimum pixel value in the input cube to be mapped to the
   *                minimum value in the Buffer
   *
   * @param maximum Maximum pixel value in the input cube to be mapped to the
   *                maximum value in the Buffer
   *
   * @throws Isis::iException::Message
   */
  void ProcessExport::SetInputRange(const double minimum, const double maximum) {
    double middle = (minimum + maximum) / 2.0;
    SetInputRange(minimum, middle, maximum);
  }

  /**
  * @brief Set input pixel range from to a linear stretch
  *
  * This method allows the programmer to define what range of input pixels in
  * the input cube get mapped to the output range in the Buffer. By default the
  * output range is 0.0 to 1.0 (can be overridden using the SetOutputRange
  * method). This version of SetInputRange allows the programmer to perform a
  * simple linear stretch. That is, "minimum:0.0 maximum:1.0" or minimum is
  * mapped to 0.0, maximum is mapped to 1.0 everything inbetween is mapped
  * linearly (e.g., (minimum+maximum)/2.0:0.5). Also, everything less than the
  * minimum is mapped to 0.0 and everything more than the maximum is mapped to
  * 1.0.  If you are uncertain about how stretches operate see the Stretch
  * object. If the input range is never set, no stretch will occur.
  *
  * @param minimum Minimum pixel value in the input cube to be mapped to the
  *                minimum value in the Buffer
  *
  * @param maximum Maximum pixel value in the input cube to be mapped to the
  *                maximum value in the Buffer
  *
  * @param index The index of the channel you are setting
  *
  * @throws Isis::iException::Message
  */
  void ProcessExport::SetInputRange(const double minimum, const double maximum, const int index) {
    double middle = (minimum + maximum) / 2.0;
    SetInputRange(minimum, middle, maximum, index);
  }

  /**
   * @brief Set input pixel range from to a piecewise linear stretch
   *
   * This method allows the programmer to define what range of input pixels in
   * the input cube get mapped to the output range in the Buffer. By default the
   * output range is 0.0 to 1.0 (can be overridden using the SetOutputRange
   * method). This version of SetInputRange allows the programmer to perform a
   * piecewise linear stretch. That is,"minimum:0.0 middle:0.5 maximum:1.0". The
   * pixels from the input cube between minimum and middle are mapped to 0.0 and
   * 0.5 linearly, while pixels between middle and maximum are mapped to 0.5 and
   * 1.0 linearly. Those outside the range of minimum and maximum are mapped to
   * 0.0 and 1.0 respectively. If you are uncertain about how stretches operate
   * see the Stretch object. If the input range is never set, no stretch will
   * occur.
   *
   * @param minimum Minimum pixel value in the input cube to be mapped to the
   *                minimum value in the Buffer
   *
   * @param middle Middle pixel value in the input cube to be mapped to the
   *               (minimum+maximum)/2.0 value in the Buffer
   *
   * @param maximum Maximum pixel value in the input cube to be mapped to the
   *                maximum value in the Buffer
   *
   * @throws Isis::IException::Message
   */
  void ProcessExport::SetInputRange(const double minimum, const double middle,
                                    const double maximum) {
    if (minimum >= middle) {
      string message =
        "minimum must be less than the middle [ProcessExport::SetInputRange]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    if (middle >= maximum) {
      string message =
        "middle must be less than the maximum [ProcessExport::SetInputRange]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    p_inputMinimum.clear();
    p_inputMinimum.resize(InputCubes.size(), minimum);
    p_inputMiddle.clear();
    p_inputMiddle.resize(InputCubes.size(), middle);
    p_inputMaximum.clear();
    p_inputMaximum.resize(InputCubes.size(), maximum);
  }

  /**
  * @brief Set input pixel range from to a piecewise linear stretch
  *
  * This method allows the programmer to define what range of input pixels in
  * the input cube get mapped to the output range in the Buffer. By default the
  * output range is 0.0 to 1.0 (can be overridden using the SetOutputRange
  * method). This version of SetInputRange allows the programmer to perform a
  * piecewise linear stretch. That is,"minimum:0.0 middle:0.5 maximum:1.0". The
  * pixels from the input cube between minimum and middle are mapped to 0.0 and
  * 0.5 linearly, while pixels between middle and maximum are mapped to 0.5 and
  * 1.0 linearly. Those outside the range of minimum and maximum are mapped to
  * 0.0 and 1.0 respectively. If you are uncertain about how stretches operate
  * see the Stretch object. If the input range is never set, no stretch will
  * occur.
  *
  * @param minimum Minimum pixel value in the input cube to be mapped to the
  *                minimum value in the Buffer
  *
  * @param middle Middle pixel value in the input cube to be mapped to the
  *               (minimum+maximum)/2.0 value in the Buffer
  *
  * @param maximum Maximum pixel value in the input cube to be mapped to the
  *                maximum value in the Buffer
  *
  * @param index The index of the channel you are setting
  *
  * @throws Isis::iException::Message
  */
  void ProcessExport::SetInputRange(const double minimum, const double middle,
                                    const double maximum, const int index) {
    if (minimum >= middle) {
      string message =
        "minimum must be less than the middle [ProcessExport::SetInputRange]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    if (middle >= maximum) {
      string message =
        "middle must be less than the maximum [ProcessExport::SetInputRange]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    if (index >= (int)InputCubes.size() || index < 0) {
      string message =
        "index out of bounds";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    p_inputMinimum.resize(index + 1, minimum);
    p_inputMiddle.resize(index + 1, middle);
    p_inputMaximum.resize(index + 1, maximum);
    p_inputMinimum[index] = minimum;
    p_inputMiddle[index] = middle;
    p_inputMaximum[index] = maximum;
  }

  /**
   * @brief Set input pixel range from user
   *
   * This method performs the same function as SetInputRange(min,max) and
   * SetInputRange(min,mid,max). However, the information for min/mid/max is
   * obtained from the user via the command line. Therefore you must include
   * the following parameter group in your application XML file:
   *      @code
   *       <group name="Stretch Options">
   *         <parameter name="STRETCH">
   *           <type>string</type>
   *           <default>
   *             <item>LINEAR</item>
   *           </default>
   *           <brief>Type of stretch</brief>
   *           <description>
   *             This parameter is used to select one of three ways to stretch
   *             (or map) output pixels.  They are LINEAR, PIECEWISE, or MANUAL.
   *           </description>
   *
   *           <list>
   *             <option value="LINEAR">
   *               <brief> Linear stretch </brief>
   *               <description>
   *                 A minimum and maximum are automatically computed based on
   *                 the statistics of the selected band.  A histogram of the
   *                 band is used to pick the minimum at 0.5% of the data and
   *                 the maximum at 99.5% of the data. Input pixels less than
   *                 or equal to the minimum are stretched to black while
   *                 pixels greater than the or equal to the maximum are
   *                 stretched to white. A linear mapping occurs between the
   *                 minimum and maximum.
   *               </description>
   *               <inclusions>
   *                 <item>MINPERCENT</item>
   *                 <item>MAXPERCENT</item>
   *               </inclusions>
   *               <exclusions>
   *                 <item>MINIMUM</item>
   *                 <item>MAXIMUM</item>
   *               </exclusions>
   *             </option>
   *             <option value="PIECEWISE">
   *               <brief> Piecewise-linear stretch </brief>
   *               <description>
   *                 This option is similar to the LINEAR option. A minimum and
   *                 maximum are automatically computed. Additionally, the
   *                 median is computed and it is mapped to the middle gray
   *                 value (halfway between white and black). Therefore, it is
   *                 a piecewise-linear stretch with input pixels mapped
   *                 linearly between either 1) the minimum/median or 2) the
   *                 median/maximum. This option is  useful for contrast
   *                 stretching cubes with skewed histograms to ensure a
   *                 balanced contrast.
   *               </description>
   *               <inclusions>
   *                 <item>MINPERCENT</item>
   *                 <item>MAXPERCENT</item>
   *               </inclusions>
   *               <exclusions>
   *                 <item>MINIMUM</item>
   *                 <item>MAXIMUM</item>
   *               </exclusions>
   *             </option>
   *             <option value="MANUAL">
   *               <brief> Manual stretch </brief>
   *               <description>
   *                 This option allows you to pick your own stretch.  You must
   *                 enter a value for MINIMUM and MAXIMUM
   *               </description>
   *               <inclusions>
   *                 <item>MINIMUM</item>
   *                 <item>MAXIMUM</item>
   *               </inclusions>
   *               <exclusions>
   *                 <item>MINPERCENT</item>
   *                 <item>MAXPERCENT</item>
   *               </exclusions>
   *             </option>
   *           </list>
   *         </parameter>
   *
   *         <parameter name="MINIMUM">
   *           <type>double</type>
   *           <brief>Minimum pixel value</brief>
   *           <description>
   *             The minimum input pixel value which will be mapped to black.
   *           </description>
   *           <lessThan>
   *            <item>MAXIMUM</item>
   *           </lessThan>
   *         </parameter>
   *
   *         <parameter name="MAXIMUM">
   *           <type>double</type>
   *           <brief>Maximum pixel value</brief>
   *           <description>
   *             The maximum input pixel value which will be mapped to white.
   *           </description>
   *           <greaterThan>
   *             <item>MINIMUM</item>
   *           </greaterThan>
   *         </parameter>
   *
   *         <parameter name="MINPERCENT">
   *           <type>double</type>
   *           <brief>Minimum Percent</brief>
   *           <description>
   *             The percentage of data in the histogram used to compute
   *             the minimum pixel value in the stretch.
   *           </description>
   *           <default><item>0.5</item></default>
   *           <lessThan>
   *             <item>MAXPERCENT</item>
   *           </lessThan>
   *         </parameter>
   *
   *         <parameter name="MAXPERCENT">
   *           <type>double</type>
   *           <brief>Maximum Percent</brief>
   *           <description>
   *             The percentage of data in the histogram used to compute
   *             the maximum pixel value in the stretch.
   *           </description>
   *           <default><item>99.5</item></default>
   *           <greaterThan>
   *             <item>MINPERCENT</item>
   *           </greaterThan>
   *         </parameter>
   *       </group>
   *  @endcode
   */
  void ProcessExport::SetInputRange(){
    SetInputRange(Application::GetUserInterface());
  }

  void ProcessExport::SetInputRange(UserInterface &ui) {
    p_inputMinimum.clear();
    p_inputMiddle.clear();
    p_inputMaximum.clear();

    for (unsigned int i = 0; i < InputCubes.size(); i++) {
      // Get the manual stretch parameters if needed
      QString strType = ui.GetString("STRETCH");
      if (strType == "MANUAL") {
        p_inputMinimum.push_back(ui.GetDouble("MINIMUM"));
        p_inputMaximum.push_back(ui.GetDouble("MAXIMUM"));

        p_inputMiddle.push_back(Isis::NULL8);
      }

      // Or get the automatic parameters
      else if (strType != "NONE") {
        Isis::Histogram *hist = InputCubes[i]->histogram(0);
        p_inputMinimum.push_back(hist->Percent(
                                   ui.GetDouble("MINPERCENT")));
        p_inputMaximum.push_back(hist->Percent(
                                    ui.GetDouble("MAXPERCENT")));
        p_inputMiddle.push_back(Isis::NULL8);
        ui.Clear("MINIMUM");
        ui.Clear("MAXIMUM");
        ui.PutDouble("MINIMUM", p_inputMinimum[i]);
        ui.PutDouble("MAXIMUM", p_inputMaximum[i]);

        if (strType == "PIECEWISE") {
          p_inputMiddle[i] = hist->Median();

          // If the median is the min or max, back off to linear
          if (p_inputMiddle[i] == p_inputMinimum[i] ||
              p_inputMiddle[i] == p_inputMaximum[i]) {
            p_inputMiddle[i] = Isis::NULL8;
          }
        }

        // Make sure the image isn't constant
        if (p_inputMinimum[i] == p_inputMaximum[i]) {
          p_inputMaximum[i] = p_inputMinimum[i] + 1.0;
          if (strType == "PIECEWISE") p_inputMiddle[i] = p_inputMinimum[i] + 0.5;
        }
      }
    }
  }



   bool ProcessExport::HasInputRange() const {
     return p_inputMinimum.size() > 0;
   }


   //! Get the valid minimum pixel value for the Nth input cube
   double ProcessExport::GetInputMinimum(unsigned int n) const {
     if (n >= p_inputMinimum.size())
       throw IException(IException::Programmer,
           "There is no input minimum for channel " + IString((int) n),
           _FILEINFO_);

     return p_inputMinimum[n];
   }


   //! Get the valid maximum pixel value for the Nth input cube
   double ProcessExport::GetInputMaximum(unsigned int n) const {
     if (n >= p_inputMaximum.size())
       throw IException(IException::Programmer,
           "There is no input maximum for channel " + IString((int) n),
           _FILEINFO_);

     return p_inputMaximum[n];
   }


  /**
   * @brief Set output pixel range in Buffer
   *
   * This method allows the programmer to specify the acceptable range of values
   * contained in the Buffer. If this method is never invoked, all pixel values
   * received in the Buffer of the export function will be in the range of 0.0
   * to 1.0. However, this can be overridden, for example, to 0.0 and 255.0, by
   * invoking this method.
   *
   * @param minimum Desired minimum pixel value in the Buffer
   *
   * @param maximum Desired maximum pixel value in the Buffer
   *
   * @throws Isis::iException::Message
   */
  void ProcessExport::SetOutputRange(const double minimum, const double maximum) {
    if (minimum >= maximum) {
      string message =
        "minimum must be less than the maximum [ProcessExport::SetOutputRange]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    p_outputMinimum = minimum;
    p_outputMaximum = maximum;
    p_outputMiddle = (p_outputMinimum + p_outputMaximum) / 2.0;
  }


  /**
   * Set output special pixel value for NULL
   *
   * Sets the value for output special pixel NULLs. NULL pixels values from the
   * input cube will be set to this value. Be default this value will be set to
   * the minimum out value set with SetOutputRange
   *
   * @param value The output pixel value for all NULL pixels
   */
  void ProcessExport::SetOutputNull(const double value) {
    p_Null = value;
    p_Null_Set = true;
  }


  /**
   * Set output special pixel value for LIS
   *
   * Sets the value for output special pixel LISs. LIS pixels values from the
   * input cube will be set to this value. Be default this value will be set to
   * the minimum out value set with SetOutputRange
   *
   * @param value The output pixel value for all LIS pixels
   */
  void ProcessExport::SetOutputLis(const double value) {
    p_Lis = value;
    p_Lis_Set = true;
  }


  /**
   * Set output special pixel value for LRS
   *
   * Sets the value for output special pixel LRSs. LRS8 pixels values from the
   * input cube will be set to this value. Be default this value will be set to
   * the minimum out value set with SetOutputRange
   *
   * @param value The output pixel value for all LRS pixels
   */
  void ProcessExport::SetOutputLrs(const double value) {
    p_Lrs = value;
    p_Lrs_Set = true;
  }


  /**
   * Set output special pixel value for HIS
   *
   * Sets the value for output special pixel HISs. HIS pixels values from the
   * input cube will be set to this value. Be default this value will be set to
   * the maximum out value set with SetOutputRange
   *
   * @param value The output pixel value for all HIS pixels
   */
  void ProcessExport::SetOutputHis(const double value) {
    p_His = value;
    p_His_Set = true;
  }


  /**
   * Set output special pixel value for HRS
   *
   * Sets the value for output special pixel HRSs. HRS pixels values from the
   * input cube will be set to this value. Be default this value will be set to
   * the maximum out value set with SetOutputRange
   *
   * @param value The output pixel value for all HRS pixels
   */
  void ProcessExport::SetOutputHrs(const double value) {
    p_Hrs = value;
    p_Hrs_Set = true;
  }


  /**
   * Return the output special pixel value for NULL
   */
  double ProcessExport::OutputNull() {
    return p_Null_Set ? p_Null : p_outputMinimum;
  }


  /**
   * Return the output special pixel value for LIS
   */
  double ProcessExport::OutputLis() {
    return p_Lis_Set ? p_Lis : p_outputMinimum;
  }


  /**
   * Return the output special pixel value for LRS
   */
  double ProcessExport::OutputLrs() {
    return p_Lrs_Set ? p_Lrs : p_outputMinimum;
  }


  /**
   * Return the output special pixel value for HIS
   */
  double ProcessExport::OutputHis() {
    return p_His_Set ? p_His : p_outputMaximum;
  }


  /**
   * Return the output special pixel value for HRS
   */
  double ProcessExport::OutputHrs() {
    return p_Hrs_Set ? p_Hrs : p_outputMaximum;
  }


  /**
  * @brief Set output pixel bit type in Buffer
  *
  * This method specifies the type of pixel data that is going to be output.
  * Essentially, it is a convenience method that will automatically calculate
  * the necessary output range based on the minimum and maximum values of the
  * bit type that is specified. Currently, the method only supported data types
  * are Isis::UnsignedByte (Range of 0 to 255), Isis::SignedWord (Range of
  * -32768 to 32767), Isis::UnsignedWord (Range of 0 to 65535), and Isis::Real
  * (Range from the minimum floating-point value to the maximum floating-point
  * value supported in C++; -FLT_MAX to FLT_MAX), since these are the only
  * formats that can be output by ProcessExport. If neither this method nor the
  * SetOutputRange method is invoked, all pixel values received in the Buffer
  * of the export function will be in the range of 0.0 to 1.0.
  *
  * NOTE: You must set the format type of the output data with SetFormat before
  * calling this method. Otherwise, you will get an error.
  *
  * @param pixelIn this is an enumeration of the different pixel
  *                types. The only values that are recognized as valid
  *                are Isis::UnsignedByte, Isis::SignedWord,
  *                Isis::UnsignedWord, and Isis::Real.
  *
  *
  * @throws Isis::iException::Message
  */
  void ProcessExport::SetOutputType(Isis::PixelType pixelIn) {
    p_pixelType = pixelIn;

    if (p_format < 0 || p_format > 3) {
      string message =
        "Format of the output file must be set prior to calling this method [ProcessExport::SetOutputType]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
    if (pixelIn == Isis::UnsignedByte)
      SetOutputRange((double)VALID_MIN1, (double)VALID_MAX1);
    else if (pixelIn == Isis::UnsignedWord)
      SetOutputRange((double)VALID_MINU2, (double)VALID_MAXU2);
    else if (pixelIn == Isis::SignedWord)
      SetOutputRange((double)VALID_MIN2, (double)VALID_MAX2);
    else if (pixelIn == Isis::Real)
      if (p_format == JP2) {
        string message =
          "Unsupported bit type for JP2 formatted files [ProcessExport::SetOutputType]";
        throw IException(IException::Programmer, message, _FILEINFO_);
      }
      else {
        SetOutputRange(-DBL_MAX, DBL_MAX);
      }
    else {
      string message =
        "Unsupported bit type [ProcessExport::SetOutputType]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }
  }


  /**
  * @brief Set byte endianness of the output cube
  *
  * This method allows the programmer to specify whether the first byte of data
  * output from the ProcessExport will be the most significant byte or the least
  * significant byte. If the user does not explicitly set the endianness, it
  * will default to that of the current system architecture
  *
  * @param byteOrderIn enumeration of the endianness (MSB or LSB)
  */
  void ProcessExport::SetOutputEndian(enum ByteOrder byteOrderIn) {
    if (p_endianSwap != NULL) {
      delete p_endianSwap;
    }
    p_endianType = byteOrderIn;
    if (byteOrderIn == Isis::NoByteOrder) {
      p_endianSwap = new EndianSwapper("NoByteOrder");
    }
    else if (byteOrderIn == Isis::Lsb) {
      p_endianSwap = new EndianSwapper("LSB");
    }
    else if (byteOrderIn == Isis::Msb) {
      p_endianSwap = new EndianSwapper("MSB");
    }
  }


  /**
   * @description Set m_canGenerateChecksum which determines if we can generate a MD5 checksum on
   * the image data.
   *
   * @param flag boolean to generate the checksum or not
   *
   */
  void ProcessExport::setCanGenerateChecksum(bool flag) {
    m_canGenerateChecksum = flag;
  }


  /**
   * @description Return if we can generate a checksum
   *
   * @return Boolean to generate the checksum or not
   *
   */
  bool ProcessExport::canGenerateChecksum() {
    return m_canGenerateChecksum;
  }


  /**
   * @description Generates a file checksum. This must be called after StartProcess.
   *
   * @return QString Returns a QString of the checksum.
   */
  QString ProcessExport::checksum() {

    QString checksum;

    if (!m_canGenerateChecksum) {
      std::string msg = "Cannot generate a checksum. Call setGenerateChecksum(true) before startProcess";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
    else {
      if (m_cryptographicHash == NULL) {
        std::string msg = "Unable to generate a checksum. Did you call startProcess?";
        throw IException(IException::Programmer, msg, _FILEINFO_);
        return checksum;
      }

      checksum = m_cryptographicHash->result().toHex();
    }

    return checksum;
  }


  /**
  * @brief Set cube up for processing
  *
  * This method is called from startProcess() to validate the input cube
  * before processing and to ready the input cube for reading line
  * by line in the cases of BSQ and BIL, or reading band by band
  * in the case of BIP.
  *
  * @throws Isis::iException::Message - No input cube was specified
  */
  void ProcessExport::InitProcess() {
    if (InputCubes.size() < 1) {
      string m = "You have not specified any input cubes";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    // TODO this really belongs here, but causes problems because of its
    // coupling with an application User Interface that contains a STRETCH
    // parameter
    //if (!HasInputRange()) SetInputRange();

    // Construct a line buffer manager
    if (p_format == BIP) {
      p_progress->SetMaximumSteps((InputCubes[0]->sampleCount()) * (InputCubes[0]->lineCount()));
    }
    else {
      p_progress->SetMaximumSteps((InputCubes[0]->lineCount()) * (InputCubes[0]->bandCount()));
    }


    // Setup a stretch object
    p_str.clear();
    for (unsigned int i = 0; i < InputCubes.size(); i++) {
      p_str.push_back(new Stretch());
      if (p_inputMinimum.size() > 0) {
        if (Isis::IsValidPixel(p_inputMinimum[i])) {
          p_str[i]->AddPair(p_inputMinimum[i], p_outputMinimum);
          if (Isis::IsValidPixel(p_inputMiddle[i])) {
            p_str[i]->AddPair(p_inputMiddle[i], p_outputMiddle);
          }
          p_str[i]->AddPair(p_inputMaximum[i], p_outputMaximum);
        }
      }

      p_str[i]->SetNull(OutputNull());
      p_str[i]->SetLis(OutputLis());
      p_str[i]->SetLrs(OutputLrs());
      p_str[i]->SetHis(OutputHis());
      p_str[i]->SetHrs(OutputHrs());
    }

    p_progress->CheckStatus();
    return;
  }


  /**
  *
  * This method invokes the process operation over a single input
  * cube.  In the cases of BSQ and BIL this is a process by line.
  * In the case of BIP, this is a process by band.  A single
  * buffer of input data will be padd to the buffer processing
  * function.  Note the data will be stretched based on the
  * invocations of the SetInputRange and SetpOutputRange methods.
  *
  * @param funct (Isis::Buffer &b) Name of your buffer processing
  *                                function. The buffer in will
  *                                contain stretched input cube
  *                                pixels for an entire buffer.
  *                                These pixels must be written to
  *                                the foreign output file (e.g,
  *                                jpg, tif, etc).
  *
  */
  void ProcessExport::StartProcess(void funct(Isis::Buffer &in)) {
    InitProcess();

    Isis::BufferManager *buff;
    if (p_format == BSQ) {
      buff = new Isis::LineManager(*InputCubes[0]);
    }
    else if (p_format == BIL || p_format == JP2) {
      buff = new Isis::LineManager(*InputCubes[0], true);
    }
    else if (p_format == BIP) {
      buff = new Isis::BandManager(*InputCubes[0]);
    }
    else {
      string m = "Invalid storage order.";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    // Loop and let the app programmer fiddle with the buffers
    for (buff->begin(); !buff->end(); buff->next()) {
      // Read a line of data
      InputCubes[0]->read(*buff);
      // Stretch the pixels into the desired range
      for (int i = 0; i < buff->size(); i++) {
        (*buff)[i] = p_str[0]->Map((*buff)[i]);
      }
      // Invoke the user function
      funct(*buff);
      p_progress->CheckStatus();
    }
  }


  /**
  *
  * This method invokes the process operation over a single input
  * cube.  In the cases of BSQ and BIL this is a process by line.
  * In the case of BIP, this is a process by band.  A single
  * buffer of input data will be padd to the buffer processing
  * function.  Note the data will be stretched based on the
  * invocations of the SetInputRange and SetpOutputRange methods.
  *
  * @param funct (Isis::Buffer &b) Name of your buffer processing
  *                                function. The buffer in will
  *                                contain stretched input cube
  *                                pixels for an entire buffer.
  *                                These pixels must be written to
  *                                the foreign output file (e.g,
  *                                jpg, tif, etc).
  *
  */
  void ProcessExport::StartProcess(void funct(vector<Buffer *> &in)) {
    int length = (p_format == BIP) ?
      InputCubes[0]->bandCount() : InputCubes[0]->lineCount();

    // Loop and let the app programmer fiddle with the lines
    vector<BufferManager *> imgrs = GetBuffers();
    for (int k = 1; k <= length; k++) {
      vector<Buffer *> ibufs;

      for (unsigned int j = 0; j < InputCubes.size(); j++) {
        // Read a line of data
        InputCubes[j]->read(*imgrs[j]);

        // Stretch the pixels into the desired range
        for (int i = 0; i < InputCubes[0]->sampleCount(); i++)
          (*imgrs[j])[i] = p_str[j]->Map((*imgrs[j])[i]);

        ibufs.push_back(imgrs[j]);
      }

      // Invoke the user function
      funct(ibufs);

      for (unsigned int i = 0; i < imgrs.size(); i++) imgrs[i]->next();
      p_progress->CheckStatus();
    }
  }


  vector<BufferManager *> ProcessExport::GetBuffers() {
    InitProcess();
    vector<BufferManager *> imgrs;
    if (p_format == BSQ) {
      imgrs = GetBuffersBSQ();
    }
    else if (p_format == BIL || p_format == JP2) {
      imgrs = GetBuffersBIL();
    }
    else if (p_format == BIP) {
      imgrs = GetBuffersBIP();
    }
    else {
      string m = "Invalid storage order.";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }
    return imgrs;
  }


  /**
  * A single line of input data from each input cube will be
  * passed to the line processing function. Note the data will be
  * stretched based on the invocations of the SetInputRange and
  * SetOutputRange methods.
  *
  * @param funct (Isis::Buffer &b) Name of your line processing function. The
  *                                buffer in will contain stretched input cube
  *                                pixels for an entire line of
  *                                each input cube. These pixels
  *                                must be written to the foreign
  *                                output file (e.g, jpg, tif,
  *                                etc).
  *
  */
  vector<BufferManager *> ProcessExport::GetBuffersBSQ() {
    int samples = InputCubes[0]->sampleCount();
    int lines = InputCubes[0]->lineCount();

    vector<BufferManager *> imgrs;
    for (unsigned int i = 0; i < InputCubes.size(); i++) {
      if ((InputCubes[i]->sampleCount() == samples) &&
          (InputCubes[i]->lineCount() == lines)) {

        Isis::LineManager *iline = new Isis::LineManager(*InputCubes[i]);
        iline->begin();
        imgrs.push_back(iline);
      }
      else {
        string m = "All input cubes must have the same dimensions";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }

    return imgrs;
  }


  /**
   * A single line of input data from each input cube will be
   * passed to the line processing function. Note the data will be
   * stretched based on the invocations of the SetInputRange and
   * SetOutputRange methods.
   *
   * @param funct (Isis::Buffer &b) Name of your line processing
   *                                function. The buffer in will
   *                                contain stretched input cube
   *                                pixels for an entire line of
   *                                each input cube. These pixels
   *                                must be written to the foreign
   *                                output file (e.g, jpg, tif,
   *                                etc).
   *
   */
  vector<BufferManager *> ProcessExport::GetBuffersBIL() {
    int samples = InputCubes[0]->sampleCount();
    int lines = InputCubes[0]->lineCount();

    vector<BufferManager *> imgrs;
    for (unsigned int i = 0; i < InputCubes.size(); i++) {
      if ((InputCubes[i]->sampleCount() == samples) &&
          (InputCubes[i]->lineCount() == lines)) {

        Isis::LineManager *iline = new Isis::LineManager(*InputCubes[i], true);
        iline->begin();
        imgrs.push_back(iline);
      }
      else {
        string m = "All input cubes must have the same dimensions";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }

    return imgrs;
  }

  /**
   * A single band of input data from each input cube will be
   * passed to the band processing function. Note the data will be
   * stretched based on the invocations of the SetInputRange and
   * SetOutputRange methods.
   *
   * @param funct (Isis::Buffer &b) Name of your band processing
   *                                function. The buffer in will
   *                                contain stretched input cube
   *                                pixels for an entire line of
   *                                each input cube. These pixels
   *                                must be written to the foreign
   *                                output file (e.g, jpg, tif,
   *                                etc).
   *
   */
  vector<BufferManager *> ProcessExport::GetBuffersBIP() {
    int bands = InputCubes[0]->bandCount();
    int samples = InputCubes[0]->sampleCount();

    vector<BufferManager *> imgrs;
    for (unsigned int i = 0; i < InputCubes.size(); i++) {
      if ((InputCubes[i]->bandCount() == bands) && (InputCubes[i]->sampleCount() == samples)) {
        Isis::BandManager *iband = new Isis::BandManager(*InputCubes[i]);
        iband->begin();
        imgrs.push_back(iband);
      }
      else {
        string m = "All input cubes must have the same dimensions";
        throw IException(IException::Programmer, m, _FILEINFO_);
      }
    }

    return imgrs;
  }




  /**
  * @brief Write an entire cube to an output file stream
  *
  * Just as with the other invocation of the StartProcess method, this will
  * process an input cube buffer by buffer. Unlike the other invocation, this
  * method takes care of writing the input data to an output file stream
  * specified by the user instead of relying on an external function.
  *
  * @param &fout An open stream to which the pixel data will be written. After
  *                       calling this method once, the stream will contain all
  *                       of  the pixel data from the input cube.
  */
  void ProcessExport::StartProcess(std::ofstream &fout) {
    InitProcess();

    Isis::BufferManager *buff;
    if (p_format == BSQ) {
      buff = new Isis::LineManager(*InputCubes[0]);
    }
    else if (p_format == BIL) {
      buff = new Isis::LineManager(*InputCubes[0], true);
    }
    else if (p_format == BIP) {
      buff = new Isis::BandManager(*InputCubes[0]);
    }
    else {
      string m = "Output stream cannot be generated for requested storage order type.";
      throw IException(IException::Programmer, m, _FILEINFO_);
    }

    //This if is the changed one
    if (m_canGenerateChecksum) {
      // Loop for each line of data
      for (buff->begin(); !buff->end(); buff->next()) {
        // Read a line of data
        InputCubes[0]->read(*buff);
        QByteArray byteArray;
        // Stretch the pixels into the desired range
        for (int i = 0; i < buff->size(); i++) {
          (*buff)[i] = p_str[0]->Map((*buff)[i]);
          byteArray.append((*buff)[i]);
        }
        if (p_pixelType == Isis::UnsignedByte)
          isisOut8(*buff, fout);
        else if (p_pixelType == Isis::UnsignedWord)
          isisOut16u(*buff, fout);
        else if (p_pixelType == Isis::SignedWord)
          isisOut16s(*buff, fout);
        else if (p_pixelType == Isis::Real)
          isisOut32(*buff, fout);
        p_progress->CheckStatus();
        m_cryptographicHash->addData(byteArray);
      }
    }
    else {
      for (buff->begin(); !buff->end(); buff->next()) {
        // Read a line of data
        InputCubes[0]->read(*buff);
        // Stretch the pixels into the desired range
        for (int i = 0; i < buff->size(); i++) {
          (*buff)[i] = p_str[0]->Map((*buff)[i]);
        }
        if (p_pixelType == Isis::UnsignedByte)
          isisOut8(*buff, fout);
        else if (p_pixelType == Isis::UnsignedWord)
          isisOut16u(*buff, fout);
        else if (p_pixelType == Isis::SignedWord)
          isisOut16s(*buff, fout);
        else if (p_pixelType == Isis::Real)
          isisOut32(*buff, fout);
        p_progress->CheckStatus();
      }
    }
    delete buff;
    return;
  }


  /**
  * @brief Write a buffer of 8-bit pixel data to a stream
  *
  * This method takes a buffer of data and assumes that it is 8 bit pixel data.
  * It will apply the necessary endian swap to the data and write it out to the
  * output file buffer that the user specifies. The user can only expect to
  * access this method indirectly by calling the StartProcess method
  *
  * @param &in Reference to a single buffer of pixel data. Note
  *            that this buffer will already have had the necessary
  *            stretching operations, though it will have the
  *            native endianness of the system.
  *
  * @param &fout  Name of the file stream to which the buffer
  *               of pixel data will be written.
  */
  void ProcessExport::isisOut8(Buffer &in, std::ofstream &fout) {
    char *out8 = new char[in.size()];
    for (int samp = 0; samp < in.size(); samp++) {
      double pixel = in[samp];
      if (pixel <= 0.0) {
        out8[samp] = 0;
      }
      else if (pixel >= 255.0) {
        out8[samp] = (char)255;
      }
      else {
        out8[samp] = (char)(in[samp] + 0.5);  //Rounds
      }
    }
    fout.write(out8, in.size());
    delete[] out8;
    return;
  }


  /**
  * @brief Write a buffer of 16-bit signed pixel data to a stream
  *
  * This method takes a buffer of data and assumes that it is
  * 16-bit signed pixel data. It will apply the necessary endian
  * swap to the data and write it out to the output file buffer
  * that the user specifies. The user can only expect to access
  * this method indirectly by calling the StartProcess method.
  *
  * @param &in Reference to a single buffer of pixel data. Note
  *            that this buffer will already have had the
  *            necessary stretching operations, though it will
  *            have the native endianness of the system.
  *
  * @param &fout Name of the file stream to which the buffer of pixel data will
  *              be written.
  */
  void ProcessExport::isisOut16s(Buffer &in, std::ofstream &fout) {
    short *out16s = new short[in.size()];
    for (int samp = 0; samp < in.size(); samp++) {
      double pixel = in[samp];
      short tempShort;
      if (pixel <= -32768.0) {
        tempShort = (short)32768;
        tempShort = -1 * tempShort;
      }
      else if (pixel >= 32767.0) {
        tempShort = (short)32767;
      }
      else {
        //Rounds
        if (in[samp] < 0.0) {
          tempShort = (short)(in[samp] - 0.5);
        }
        else {
          tempShort = (short)(in[samp] + 0.5);
        }
      }
      void *p_swap = &tempShort;
      out16s[samp] = p_endianSwap->ShortInt(p_swap);
    }
    fout.write((char *)out16s, in.size() * 2);
    delete[] out16s;
    return;
  }


  /**
  * @brief Write a bufferof 16-bit unsigned pixel data to a stream
  *
  * This method takes a buffer of data and assumes that it is 16-bit
  * unsigned pixel data. It will apply the necessary endian swap
  * to the data and write it out to the output file buffer that
  * the user specifies. The user can only expect to access this
  * method indirectly by calling the StartProcess method.
  *
  * @param &in Reference to a single buffer of pixel data. Note that this buffer
  *          will already have had the necessary stretching operations, though
  *          it will have the native endianness of the system.
  *
  * @param &fout Name of the file stream to which the buffer of pixel data will
  *              be written.
  *
  */
  void ProcessExport::isisOut16u(Buffer &in, std::ofstream &fout) {
    unsigned short *out16u = new unsigned short[in.size()];
    for (int samp = 0; samp < in.size(); samp++) {
      double pixel = in[samp];
      unsigned short tempShort;
      if (pixel <= 0.0) {
        tempShort = 0;
      }
      else if (pixel >= 65535.0) {
        tempShort = 65535;
      }
      else {
        tempShort = (unsigned short)(in[samp] + 0.5); //Rounds
      }
      unsigned short *p_swap = &tempShort;
      out16u[samp] = p_endianSwap->UnsignedShortInt(p_swap);
    }

    fout.write((char *)out16u, in.size() * 2);
    delete[] out16u;
    return;
  }


  /**
  * @brief Write a buffer of 32-bit floating point pixel data to a stream
  *
  * This method takes a buffer of data and assumes that it is 32-bit floating
  * point pixel data. It will apply the necessary endian swap to the data and
  * write it out to the output file buffer that the user specifies. The user can only
  * expect to access this method indirectly by calling the StartProcess method.
  *
  * @param &in Reference to a single buffer of pixel data. Note that this buffer
  *          will already have had the necessary stretching operations, though
  *          it will have the native endianness of the system.
  *
  * @param &fout Name of the file stream to which the buffer of pixel data will
  *              be written.
  *
  */
  void ProcessExport::isisOut32(Buffer &in, std::ofstream &fout) {
    int *out32 = new int[in.size()];
    for (int samp = 0; samp < in.size(); samp++) {
      double pixel = in[samp];
      float tempFloat;
      if (pixel <= -((double)FLT_MAX)) {
        tempFloat = -((double)FLT_MAX);
      }
      else if (pixel >= (double)FLT_MAX) {
        tempFloat = (double)FLT_MAX;
      }
      else {
        tempFloat = (double)in[samp];
      }
      void *p_swap = &tempFloat;
      out32[samp] = p_endianSwap->ExportFloat(p_swap);
    }
    fout.write((char *)out32, in.size() * 4);
    delete[] out32;
    return;
  }

  /**
  * @brief Create a standard world file for the input cube
  *
  * This method creates a standard world file from the mapping group of the
  * input cube.
  *
  * @param worldFile [in] Reference to a string containing the name of a file
  *          to write the world information to.
  *
  */
  void ProcessExport::CreateWorldFile(const QString &worldFile) {
    try {
      Projection *proj = InputCubes[0]->projection();
      proj->SetWorld(1.0, 1.0);
      ofstream os;
      os.open(worldFile.toLatin1().data(), ios::out);

      // X resolution
      os << std::fixed << setprecision(15)
         << proj->Resolution() << endl;
      // scale and rotation
      os << 0.0 << endl;
      os << 0.0 << endl;

      // Y resolution
      os << std::fixed << setprecision(15)
         << -proj->Resolution() << endl;

      // Upper left x at pixel middle
      os << std::fixed << setprecision(15)
         << proj->XCoord() << endl;

      // Upper left y at pixel middle
      os << std::fixed << setprecision(15)
         << proj->YCoord() << endl;

      os.close();
    }
    catch(IException &e) {
    }
  }
}
