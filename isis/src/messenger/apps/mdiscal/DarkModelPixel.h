#ifndef DarkModelPixel_h
#define DarkModelPixel_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>
#include <string>

#include <tnt_array1d.h>
#include <tnt_array1d_utils.h>

#include "CSVReader.h"
#include "FileName.h"
#include "MdisCalUtils.h"

namespace Isis {

  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   *   @history 2015-09-01 Jeannie Backer - Brought code closer to coding standards.
   */
  class DarkModelPixel {
    public:
      DarkModelPixel() : m_scale(1), m_ccdTemp(0.0), m_expTime(0.0), m_coefs(8, 0.0),
        m_filename() { }
      DarkModelPixel(int pixelBinning) : m_scale(1), m_ccdTemp(0.0), m_expTime(0.0),
        m_coefs(8, 0.0), m_filename() {
        setPixelBinning(pixelBinning);
      }

      DarkModelPixel(int pixelBinning, double ccdTemp, double expTime) :
        m_scale(1), m_ccdTemp(ccdTemp), m_expTime(expTime),
        m_coefs(8, 0.0) {
        setPixelBinning(pixelBinning);
        setCCDTemperature(ccdTemp);
        setExposureTime(expTime);
      }

      ~DarkModelPixel() { }

      void setPixelBinning(int pxlBin) {
        if (pxlBin > 0) {
          m_scale = (int) std::pow(2.0, pxlBin);
        }
        else {
          m_scale = 1;
        }
        return;
      }
      void setCCDTemperature(double ccdTemp) {
        m_ccdTemp = ccdTemp;
        return;
      }

      /**
       * @brief Set exposure time
       * @param expTime Exposure time of image in seconds
       */
      void setExposureTime(const double &expTime) {
        m_expTime = expTime * 1000.0;
      }


      QString loadCoefficients(bool isNac, bool isFpuBinned) {
        QString filename = "$messenger/calibration/DARK_MODEL/MDIS";
        // FileName consists of binned/notbinned, camera, and filter
        filename += (isNac) ? "NAC" : "WAC";
        filename += (isFpuBinned) ? "_BINNED_" : "_NOTBIN_";
        filename += "DARKMODEL_?.TAB";

        FileName finalName(filename.toStdString());
        finalName = finalName.highestVersion();
        filename = QString::fromStdString(finalName.originalPath() + "/" + finalName.name());
        m_filename = filename;

        //  Open the CSV file
        CSVReader csv(QString::fromStdString(finalName.expanded()));
        DVector coefs(8);
        double ccdTempSqrd = m_ccdTemp * m_ccdTemp;
        double ccdTempCubed = ccdTempSqrd * m_ccdTemp;
        for (int i = 0; i < 8; i++) {
          CSVReader::CSVAxis row = csv.getRow(i);
          DVector values(4);
          for (int v = 0; v < 4; v++) {
            values[v] = toDouble(row[v]);
          }

          // temps are in order in the file
          coefs[i] = values[0] +
                     values[1] * m_ccdTemp +
                     values[2] * ccdTempSqrd +
                     values[3] * ccdTempCubed;
        }

        //  All done, save off coefficients and return filename
        m_coefs = coefs;
        return (filename);
      }

      QString getFileName() const {
        return (m_filename);
      }

      double getDarkPixel(int sample, int line) const {
        double sum = 0.0;
        double npixels = 0.0;
        int line0 = line * m_scale;
        for (int l = 0; l < m_scale; l++, line0++) {
          int samp0 = sample * m_scale;
          for (int s = 0; s < m_scale; s++, samp0++) {
            // ctemp + dtemp*exposure
            double acoef = m_coefs[0] + m_coefs[1] * m_expTime;
            double bcoef = m_coefs[2] + m_coefs[3] * m_expTime;
            double mcoef = m_coefs[4] + m_coefs[5] * m_expTime;
            double ncoef = m_coefs[6] + m_coefs[7] * m_expTime;

            double alphacoef = acoef + bcoef * line0;
            double betacoef  = mcoef + ncoef * line0;

            sum += (alphacoef + betacoef * samp0);
            npixels += 1.0;
          }
        }

        //  Return the average
        return (sum / npixels);
      }

    private:
      typedef TNT::Array1D<double> DVector;       //!<  1-D Buffer
      int m_scale;
      double m_ccdTemp;
      double m_expTime;
      DVector m_coefs;
      QString m_filename;
  };

};
#endif
