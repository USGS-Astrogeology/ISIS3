#ifndef Apollo_h
#define Apollo_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "iTime.h"
#include "IString.h"
#include "IException.h"

namespace Isis {
/**
 * @brief Reads user Apollos from a data file.
 *
 * This class is used to
 *
 * @author 2007-02-07 Jacob Danton
 *
 * @internal
 *   @history 2007-02-07 Jacob Danton - Original version
 *   @history 2017-06-28 Makayla Shepherd - Updated documentation. References #4807.
 */

  class Apollo {
    public:

      /**
       * Constructor. Calls initialize() to set variables based on the QStrings passed in.
       *
       * @param spacecraft The name of the spacecraft
       * @param instrument The name of the instrument
       *
       * @see initialize(QString spacecraft, QString instrument)
       */
      Apollo (QString spacecraft, QString instrument) {
        initialize(spacecraft.toUpper(), instrument.toUpper());
      };


      /**
       * Constructor. Parses the filename passed in and sets the spacecraft name and instrument.
       * Calls initialize() to set variables based on the spacecraft and instrument.
       *
       * @param filename The image filename that will be parsed to get the spacecraft and instrument
       *
       * @see initialize(QString spacecraft, QString instrument)
       */
      Apollo(QString filename) {
        QString spacecraft, instrument;
        if (filename.mid(0,4) == "AS15") spacecraft = "APOLLO 15";
        else if (filename.mid(0,4) == "AS16") spacecraft = "APOLLO 16";
        else if (filename.mid(0,4) == "AS17") spacecraft = "APOLLO 17";
        // throw an error
        else {
          std::string msg = "The image filename does not match the required formatting.";
          throw IException(IException::User,msg,_FILEINFO_);
        }

        if (filename.mid(5,1) == "M") instrument = "METRIC";
        else if (filename.mid(5,1) == "P") instrument = "PANORAMIC";
        else if (filename.mid(5,1) == "H") instrument = "HASSELBLAD";
        // throw an error
        else {
          std::string msg = "The image filename does not match the required formatting.";
          throw IException(IException::User,msg,_FILEINFO_);
        }

        initialize(spacecraft, instrument);
      };


      //! Destroys the Apollo object
      ~Apollo () {};

      /**
       * Checks if the instrument is an Apollo Metric camera.
       *
       * @return bool True if the instrument is Metric
       */
      bool IsMetric () {return p_instrumentId == "METRIC";}


      /**
       * Checks if the instrument is an Apollo Panoramic camera.
       *
       * @return bool True if the instrument is Panoramic
       */
      bool IsPanoramic () {return p_instrumentId == "PANORAMIC";}


      /**
       * Checks if the instrument is an Apollo Hasselblad camera.
       *
       * @return bool True if the instrument is Hasselblad
       */
      bool IsHasselblad () {return p_instrumentId == "HASSELBLAD";}


      /**
       * Checks if the spacecraft is Apollo 15
       *
       * @return bool True if the spacecraft is Apollo 15
       */
      bool IsApollo15 () {return p_spacecraftName == "APOLLO 15";}


      /**
       * Checks if the spacecraft is Apollo 16
       *
       * @return bool True if the spacecraft is Apollo 16
       */
      bool IsApollo16 () {return p_spacecraftName == "APOLLO 16";}


      /**
       * Checks if the spacecraft is Apollo 17
       *
       * @return bool True if the spacecraft is Apollo 17
       */
      bool IsApollo17 () {return p_spacecraftName == "APOLLO 17";}


      /**
       * Returns the width of the image. The image width is set in initialize() based on the
       * instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return int Width of the image.
       */
      int Width () {return p_imageWidth;};


      /**
       * Returns the height of the image. The image height is set in initialize() based on the
       * instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return int Height of the image.
       */
      int Height () {return p_imageHeight;};


      /**
       * Returns number of bands in the image. The number of bands is set in initialize() based on
       * the instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return int Bands of the image.
       */
      int Bands () { return p_imageBands;};


      /**
       * Returns the reseau dimension of the image. The reseau dimenstion is set in initialize()
       * based on the instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return int Reseau dimension of the image.
       */
      int ReseauDimension () {return p_reseauDimension;};


      /**
       * Returns pixel pitch for the image. The pixel patch is set in initialize() based on
       * the instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return double Pixel pitch of the image.
       */
      double PixelPitch () {return p_imagePixelPitch;};


      /**
       * Returns the spacecraft name.
       *
       * @return QString Spacecraft name.
       */
      QString SpacecraftName () {return p_spacecraftName;};


      /**
       * Returns the instrument ID.
       *
       * @return QString Instrument ID.
       */
      QString InstrumentId () {return p_instrumentId;};


      /**
       * Returns the NAIF frame code. The NAIF frame code is set in initialize() based on  the
       * spacecraft and instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return QString NAIF frame code of the image.
       */
      QString NaifFrameCode () {return p_naifFrameCode;};


      /**
       * Returns the target name which is always the Moon.
       *
       * @return QString Moon.
       */
      QString TargetName () {return "MOON";};


      /**
       * Returns the launch date of the mission. The launch date is set in initialize() based on the
       * spacecraft and instrument.
       *
       * @see initialize(QString spacecraft, QString instrument)
       *
       * @return QString Launch date of mission
       */
      iTime LaunchDate () {return p_launchDate;};

      private:

      /**
       * Sets variables based on the spacecraft name and instrument.
       *
       * @param spacecraft Spacecraft name
       * @param instrument Instrument ID
       *
       * @throws IException::Unknown "Unknown instrument"
       * @throws IException::Unknown "Unknown spacecraft"
       *
       * @return void
       */
      void initialize(QString spacecraft, QString instrument) {
        if (instrument == "METRIC") {
          p_instrumentId = "METRIC";
          p_reseauDimension = 403;
          p_imageWidth = 22900;
          p_imageHeight = 22900;
          p_imageBands = 1;
          p_imagePixelPitch = 200.5;
        }
        else if (instrument == "PANORAMIC") {
          p_instrumentId = "PANORAMIC";
          p_reseauDimension = 0;
          p_imageWidth = 231480; // 228987
          p_imageHeight = 23007;
          p_imageBands = 1;
          p_imagePixelPitch = 200.5;
        }
        else if (instrument == "HASSELBLAD") {
          p_instrumentId = "HASSELBLAD";
          p_reseauDimension = 403;
          p_imageWidth = 12800;
          p_imageHeight = 12800;
          p_imageBands = 3;
          p_imagePixelPitch = 200.5;
        }
        else {
          QString msg = "Unknown instrument: " + instrument;
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if (spacecraft == "APOLLO 15" ){
          p_spacecraftName = "APOLLO 15";
          // Apollo 15 launched 1971-07-26 at 13:34 GMT
          p_launchDate = "1971-07-26T13:33:39.11";
          if (IsMetric()) p_naifFrameCode = "-915240";
          else if (IsPanoramic()) p_naifFrameCode = "-915230";
        }
        else if (spacecraft == "APOLLO 16") {
          p_spacecraftName = "APOLLO 16";
          // Apollo 16 launched 1972-04-16 at 17:54 GMT
          p_launchDate = "1972-04-16T17:53:36.238";
          if (IsMetric()) p_naifFrameCode = "-916240";
          else if (IsPanoramic()) p_naifFrameCode = "-916230";
        }
        else if (spacecraft == "APOLLO 17") {
          p_spacecraftName = "APOLLO 17";
          // Apollo 17 launched 1972-12-07 at 05:33 GMT
          p_launchDate = "1972-12-07T05:33:00.000";
          if (IsMetric()) p_naifFrameCode = "-917240";
          else if (IsPanoramic()) p_naifFrameCode = "-917230";
        }
        else {
          QString msg = "Unknown spacecraft: " + spacecraft;
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }


      int p_imageWidth;           //!< Image width
      int p_imageHeight;          //!< Image height
      int p_imageBands;           //!< Number of bands in the image
      int p_reseauDimension;      //!< Dimensions of the reseaus
      double p_imagePixelPitch;   //!< Pixel pitch
      QString p_spacecraftName;   //!< Spacecraft name
      QString p_instrumentId;     //!< Instrument ID
      QString p_naifFrameCode;    //!< NAIF frame code
      iTime p_launchDate;         //!< Mission launch date
    };
};

#endif
