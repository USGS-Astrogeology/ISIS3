/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Hillshade.h"

#include <algorithm>

#include <QDebug>
#include <QObject>
#include <QString>

#include "Angle.h"
#include "Buffer.h"
#include "SpecialPixel.h"

namespace Isis {
  /**
   * Create a default-constructed Hillshade. You must use mutators to initialize this instance
   *   before it can be used to calculate shaded values.
   */
  Hillshade::Hillshade() {
    m_azimuth = NULL;
    m_zenith = NULL;
    m_pixelResolution = Null;
  }


  /**
   * Construct and fully initialize a Hillshade. This can immediately calculate shaded values.
   *
   * @see setAzimuth
   * @see setZenith
   * @see setResolution
   */
  Hillshade::Hillshade(Angle azimuth, Angle zenith, double resolution) {
    m_azimuth = NULL;
    m_zenith = NULL;
    m_pixelResolution = Null;

    setAzimuth(azimuth);
    setZenith(zenith);
    setResolution(resolution);
  }


  /**
   * Copy constructor
   */
  Hillshade::Hillshade(const Hillshade &other) {
    m_azimuth = NULL;
    m_zenith = NULL;
    m_pixelResolution = other.m_pixelResolution;

    if (other.m_azimuth)
      m_azimuth = new Angle(*other.m_azimuth);

    if (other.m_zenith)
      m_zenith = new Angle(*other.m_zenith);
  }


  Hillshade::~Hillshade() {
    delete m_azimuth;
    m_azimuth = NULL;

    delete m_zenith;
    m_zenith = NULL;

    m_pixelResolution = Null;
  }


  /**
   * The azimuth is the direction of the light. 0 is north; this angle rotates the sun.
   *
   * An invalid angle will silently fail; if shadedValue() is called without a valid azimuth angle
   *   then an exception will be thrown.
   */
  void Hillshade::setAzimuth(Angle azimuth) {
    delete m_azimuth;
    m_azimuth = NULL;

    if (azimuth.isValid()) {
      m_azimuth = new Angle(azimuth);
    }
  }


  /**
   * The zenith is the altitude/solar elevation of the light. 0 is directly above and 90 is the
   *     horizon; this angle raises and lowers the sun.
   *
   * An invalid angle will silently fail; if shadedValue() is called without a valid zenith angle
   *   then an exception will be thrown.
   */
  void Hillshade::setZenith(Angle zenith) {
    delete m_zenith;
    m_zenith = NULL;

    if (zenith.isValid()) {
      m_zenith = new Angle(zenith);
    }
  }


  /**
   * The resolution is the meters per pixel of the input to shadedValue().
   *
   * A special pixel value will silently fail; if shadedValue() is called without a valid resolution
   *   angle then an exception will be thrown.
   */
  void Hillshade::setResolution(double resolution) {
    m_pixelResolution = resolution;
  }


  /**
   * Get the current azimuth angle.
   *
   * @see setAzimuth()
   */
  Angle Hillshade::azimuth() const {
    Angle result;

    if (m_azimuth)
      result = *m_azimuth;

    return result;
  }


  /**
   * Get the current zenith angle.
   *
   * @see setZenith()
   */
  Angle Hillshade::zenith() const {
    Angle result;

    if (m_zenith)
      result = *m_zenith;

    return result;
  }


  /**
   * Get the current resolution (meters per pixel).
   *
   * @see setResolution()
   */
  double Hillshade::resolution() const {
    return m_pixelResolution;
  }


  /**
   * Calculate the shaded value from a 3x3x1 window
   */
  double Hillshade::shadedValue(Buffer &input) const {
    if (input.SampleDimension() != 3 ||
        input.LineDimension() != 3 ||
        input.BandDimension() != 1) {
      throw IException(IException::Programmer,"Hillshade requires a 3x3x1 portal of data, but a " + std::to_string(input.SampleDimension()) + "x" + std::to_string(input.LineDimension()) + "x" + std::to_string(input.BandDimension()) + " portal of data was provided instead",
                       _FILEINFO_);
    }

    if (!m_azimuth) {
      throw IException(IException::Unknown,"Hillshade requires a valid azimuth angle (sun direction) to operate",
                       _FILEINFO_);
    }

    if (*m_azimuth < Angle(0, Angle::Degrees) || *m_azimuth > Angle::fullRotation()) {
      throw IException(IException::Unknown,"Hillshade azimuth angle [" + m_azimuth->toString().toStdString() + "] must be between 0 and 360 degrees",
                       _FILEINFO_);
    }

    if (!m_zenith) {
      throw IException(IException::Unknown,"Hillshade requires a valid zenith angle (solar elevation) to operate",
                       _FILEINFO_);
    }

    if (*m_zenith < Angle(0, Angle::Degrees) || *m_zenith > Angle(90, Angle::Degrees)) {
      throw IException(IException::Unknown,"Hillshade zenith angle [" + m_zenith->toString().toStdString() + "] must be between 0 and 90 degrees",
                       _FILEINFO_);
    }

    if (IsSpecial(m_pixelResolution)) {
      throw IException(IException::Unknown,"Hillshade requires a pixel resolution (meters/pixel) to operate",
                       _FILEINFO_);
    }

    if (qFuzzyCompare(0.0, m_pixelResolution)) {
      throw IException(IException::Unknown, "Hillshade requires a non-zero pixel resolution (meters/pixel) to operate",
                       _FILEINFO_);
    }


    // This parameter as used by the algorithm is 0-360 with 0 at 3 o'clock,
    // increasing in the clockwise direction. The value taken in from the user is
    // 0-360 with 0 at 12 o'clock increasing in the clockwise direction.
    Angle azimuthFromThree = *m_azimuth + Angle(270, Angle::Degrees);

    if (azimuthFromThree > Angle::fullRotation()) {
      azimuthFromThree -= Angle::fullRotation();
    }

    double result = Null;

    // first we just check to make sure that we don't have any special pixels
    bool anySpecialPixels = false;
    for(int i = 0; i < input.size(); ++i) {
      if(IsSpecial(input[i])) {
        anySpecialPixels = true;
      }
    }

    // if we have any special pixels we bail out (well ok just set to null)
    if (!anySpecialPixels) {
      /* if we have a 3x3 section that doesn't have an special pixels we hit that 3x3
      / against two gradients

        [-1 0 1 ]   [-1 -1 -1 ]
        [-1 0 1 ]   [ 0  0  0 ]
        [-1 0 1 ]   [ 1  1  1 ]

        These two gradients are not special in any way aside from that they are
        are orthogonal. they can be replaced if someone has reason as long as they
        stay orthogonal to eachoher. (for those that don't know orthoginal means
        for matrix A: A * transpose(A) = I where I is the identity matrix).

      */

      double p = (  (-1) * input[0] + (0) * input[1] + (1) * input[2]
                  + (-1) * input[3] + (0) * input[4] + (1) * input[5]
                  + (-1) * input[6] + (0) * input[7] + (1) * input[8]) / (3.0 * m_pixelResolution);

      double q = (  (-1) * input[0] + (-1) * input[1] + (-1) * input[2]
                  + (0) *  input[3] + (0)  * input[4] + (0) * input[5]
                  + (1) *  input[6] + (1)  * input[7] + (1) * input[8]) / (3.0 * m_pixelResolution);

      /* after we hit them by the gradients the formula in order to make the shade is

                            1 + p0*p + q0*q
      shade =  -------------------------------------------------
                 sqrt(1 + p*p + q*q) * sqrt(1 + p0*p0 + q0*q0)

      where p0 = -cos( sun azimuth ) * tan( solar elevation ) and
            q0 = -sin( sun azimuth ) * tan( solar elevation )

      and p and q are the two orthogonal gradients of the data (calculated above)

      this equation comes from
      Horn, B.K.P. (1982). "Hill shading and the reflectance map". Geo-processing, v. 2, no. 1, p. 65-146.

      It is avaliable online at http://people.csail.mit.edu/bkph/papers/Hill-Shading.pdf
      */
      double p0 = -cos(azimuthFromThree.radians()) * tan(m_zenith->radians());
      double q0 = -sin(azimuthFromThree.radians()) * tan(m_zenith->radians());

      double numerator = 1.0 + p0 * p + q0 * q;

      double denominator = sqrt(1 + p * p + q * q) * sqrt(1 + p0 * p0 + q0 * q0);
      result = numerator / denominator;
    }

    return result;
  }


  /**
   * Swap class data with other; this cannot throw an exception.
   */
  void Hillshade::swap(Hillshade &other) {
    std::swap(m_azimuth, other.m_azimuth);
    std::swap(m_zenith, other.m_zenith);
    std::swap(m_pixelResolution, other.m_pixelResolution);
  }


  /**
   * Assignment operator. This utilizes copy-and-swap.
   */
  Hillshade &Hillshade::operator=(const Hillshade &rhs) {
    Hillshade copy(rhs);
    swap(copy);
    return *this;
  }


  /**
   * Print this class out to a QDebug object
   */
  QDebug operator<<(QDebug debug, const Hillshade &hillshade) {
    QString resolution = "Null";

    if (!IsSpecial(hillshade.resolution()))
      resolution = QString::number(hillshade.resolution());

    debug << "Hillshade[ azimuth =" << hillshade.azimuth().toString().toLatin1().data()
          << "zenith =" << hillshade.zenith().toString().toLatin1().data()
          << "resolution =" << resolution.toLatin1().data() << "]";

    return debug;
  }
}
