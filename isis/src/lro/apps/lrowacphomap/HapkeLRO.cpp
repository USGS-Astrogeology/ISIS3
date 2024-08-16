/**
 * @file
 * $Revision$
 * $Date$
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <iostream>
#include <sstream>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "Projection.h"
#include "Camera.h"
#include "HapkeLRO.h"
#include "DbProfile.h"
#include "PvlObject.h"
#include "Brick.h"

using namespace std;

namespace Isis {

  /**
   * Create an HapkeLRO photometric object
   * 
   * @author 2021-08-02 Cordell Michaud
   * 
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Code adapted from PhotometricFunction written by Kris Becker
   * 
   * @param pvl       Photometric parameter files
   * @param cube      Input cube file
   * @param useCamera Enable using camera from input cube
   * @param paramMap  Parameter cube
   */
  HapkeLRO::HapkeLRO(PvlObject &pvl, Cube &cube, bool useCamera, Cube *paramMap)
    : PhotometricFunction(pvl, cube, useCamera) {
    
    m_paramMap = paramMap;
    m_paramProj = m_paramMap->projection();
    m_currentMapSample = 0;
    m_currentMapLine = 0;
    m_currentMapIndex = 0;

    PvlObject phoModel = pvl.findObject("PhotometricModel");
    m_iRef = std::stod(phoModel.findKeyword("Incref")[0]); //  Incidence refernce angle
    m_eRef = std::stod(phoModel.findKeyword("EmiRef")[0]); //  Emission  reference angle
    m_gRef = std::stod(phoModel.findKeyword("Pharef")[0]); //  Phase     reference angle
    if (QString::fromStdString(phoModel.findKeyword("Units")[0]).toUpper() == "DEGREES") {
      m_isDegrees = true;
    }
    else {
      m_isDegrees = false;
    }

    PvlKeyword center = cube.label()->findGroup("BandBin", Pvl::Traverse)["Center"];

    PvlKeyword paramBandNames = m_paramMap->label()->findGroup("BandBin", Pvl::Traverse).findKeyword("Name");

    // Now go through the Groups looking for all the band parameters
    for (int i = 0; i < phoModel.groups(); i++) {
      if (phoModel.group(i).isNamed("Parameters")) {
        PvlGroup paramGroup = phoModel.group(i);
        HapkeLRO::Parameters parms;

        parms.bandBinCenter = std::stod(paramGroup.findKeyword("BandBinCenter")[0]);
        for (int j = 0; j < center.size(); j++) {
          if (center[j] == paramGroup.findKeyword("BandBinCenter")[0]) {
            parms.band = j + 1;
          }
        }

        PvlKeyword bands = paramGroup.findKeyword("Bands");
        for (int j = 0; j < bands.size(); j++) {
          parms.mapBands.push_back(std::stoi(bands[j]) - 1);
          parms.names.push_back(QString::fromStdString(paramBandNames[std::stoi(bands[j]) - 1]).toUpper());
          parms.values.push_back(0.0);
        }

        m_bandParameters.push_back(parms);
      }
    }

    m_paramBrickCount = paramMap->lineCount() * paramMap->sampleCount();
    m_paramBricks = new Brick*[m_paramBrickCount];
    memset(m_paramBricks, 0, m_paramBrickCount * sizeof(Brick*));

    m_normalized = true;

    m_hfunc = "HG";
    if (phoModel.hasKeyword("Hfunc")) {
      m_hfunc = QString::fromStdString(phoModel.findKeyword("Hfunc")[0]).toUpper();
    }

    if (m_hfunc != "HG") {
      QString msg = "Invalid HFunction: " + m_hfunc;
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Destructor
   */
  HapkeLRO::~HapkeLRO() {
    for (int i = 0; i < m_paramBrickCount; i++) {
      delete m_paramBricks[i];
    }

    delete [] m_paramBricks;
  }


  bool HapkeLRO::normalized() const {
    return m_normalized;
  }


  /**
   * @brief Return photometric property given angles
   * 
   * This method computes the photometric property at the given 
   * cube location after ensuring a proper parameter container is 
   * found for the specified band.
   * 
   * @author 2021-08-18 Cordell Michaud
   * 
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Adapted code from PhotometricFunction written by Kris Becker
   * 
   * @param i     Incidence angle at cube location
   * @param e     Emission angle at cube location
   * @param g     Phase angle at cube location
   * @param band  Band number in cube (actually is band index) for lookup purposes
   * 
   * @return double Photometric correction using parameters
   */
  double HapkeLRO::photometry(double i, double e, double g, int band) const {
    double lat = m_camera->UniversalLatitude();
    double lon = m_camera->UniversalLongitude();
    
    return photometry(i, e, g, lat, lon, band);
  }


  /**
   * @brief Return photometric property given angles
   * 
   * This method computes the photometric property at the given 
   * cube location after ensuring a proepr parameter container is 
   * found for the specified band.
   * 
   * @author 2021-08-18 Cordell Michaud
   * 
   * @internal
   *   @history 2021-08-03 Cordell Michaud - Adapted code from PhotometricFunction written by Kris Becker
   * 
   * @param i     Incidence angle at cube location
   * @param e     Emission angle at cube location
   * @param g     Phase angle at cube location
   * @param lat   Latitude at cube location
   * @param lon   Longitude at cube location
   * @param band  Band number in cube (actually is band index) for lookup purposes
   * 
   * @return double Photometric correction using parameters
   */
  double HapkeLRO::photometry(double i, double e, double g, double lat, double lon, int band) const {
    m_paramProj->SetUniversalGround(lat, lon);
    int intSamp = (int) (m_paramProj->WorldX() + 0.5);
    int intLine = (int) (m_paramProj->WorldY() + 0.5);

    if (intSamp <= 0 || intSamp > m_paramMap->sampleCount()
        || intLine <= 0 || intLine > m_paramMap->lineCount()) {
      
      return 0.0;
    }

    // If the line and sample of the parameter map are different
    //   we need to update the parameters
    if (m_currentMapSample != intSamp || m_currentMapLine != intLine) {
      m_currentMapSample = intSamp;
      m_currentMapLine = intLine;
      m_currentMapIndex = 0;
      // read the data for that line/sample into a brick buffer
      int paramBrickIndex = (intLine - 1) * m_paramMap->sampleCount() + intSamp;

      // if this brick doesn't exist, create it
      if (m_paramBricks[paramBrickIndex] == NULL) {
        Brick *newBrick = new Brick(1, 1, m_paramMap->bandCount(), m_paramMap->pixelType());
        newBrick->SetBasePosition(intSamp, intLine, 1);
        m_paramMap->read(*newBrick);

        // store the Brick, so we don't have to read it from the file again
        m_paramBricks[paramBrickIndex] = newBrick;
      }
      Brick &b = *m_paramBricks[paramBrickIndex];

      // And for each parameter, copy the appropriate value
      for (unsigned int p = 0; p < m_bandParameters.size(); p++) {
        HapkeLRO::Parameters &parms = m_bandParameters[p];

        if (parms.band == band) {
          m_currentMapIndex = p;
          for (unsigned int v = 0; v < parms.values.size(); v++) {
            parms.values[v] = b[parms.mapBands[v]];
            if (IsSpecial(parms.values[v])) {
              return Isis::Null;
            }
          }
          parms.phoStd = photometry(parms, m_iRef, m_eRef, m_gRef);
        }
      }
    }

    double ph = photometry(m_bandParameters[m_currentMapIndex], i, e, g);
    if (!normalized()) {
      return ph;
    }

    return m_bandParameters[m_currentMapIndex].phoStd / ph;
  }


  /**
   * @brief Performs actual photometric correction calculations
   *
   * This routine computes photometric correction using parameters
   * for the Exponential-Buratti-Hill equation.
   *
   * @author 2021-07-29 Cordell Michaud
   * 
   * @internal
   *  @history 2021-07-19 Cordell Michaud - Code adapted from HapkeLRO written by Kris Becker
   *
   * @param parms Container of band-specific HapkeLRO parameters
   * @param i     Incidence angle in degrees
   * @param e     Emission angle in degrees
   * @param g     Phase angle in degrees
   *
   * @return double Photometric correction parameter
   */
  double HapkeLRO::photometry(HapkeLRO::Parameters &parms, double i, double e, double g) const {
    double w = parms["W"];
    double bc0 = parms["BCO"];
    double hc = parms["HC"];
    double xi = parms["XI"];

    //  Ensure problematic values are adjusted
    if (i == 0.0) {
      i = 10.E-12;
    }
    if (e == 0.0) {
      e = 10.E-12;
    }

    // Convert to radians
    i *= rpd_c();
    e *= rpd_c();

    double cosg = 0.0;
    double tang2hc = 0.0;
    if (!m_isDegrees) {
      g *= rpd_c();
      cosg = cos(g);
      tang2hc = tan(g / 2) / hc;
    }
    else {
      cosg = cos(rpd_c() * g);
      tang2hc = tan(rpd_c() * g / 2) / hc;
    }

    // Compute Lommel-Seeliger components
    double mu = cos(e);
    double mu0 = cos(i);

    double p = (1 - pow(xi, 2)) / pow(1 - 2 * xi * cosg + pow(xi, 2), 1.5);
    double Hmu = (1 + 2 * mu) / (1 + 2 * mu * pow(1 - w, 0.5));
    double Hmu0 = (1 + 2 * mu0) / (1 + 2 * mu0 * pow(1 - w, 0.5));
    double Bc = (1 + (1 - exp(-1 * tang2hc)) / (tang2hc)) / (2 * pow(1 + tang2hc, 2));

    double rcal = w / 4 * (p + Hmu0 * Hmu - 1) * (1 + bc0 * Bc);
    
    return rcal * mu0 / (mu0 + mu);
  }



  /**
   * @brief Return parameters used for all bands
   * 
   * This method creates keyword vectors of band-specific parameters used 
   * in the photometric correction.
   * 
   * @author 2021-08-03 Cordell Michaud
   * 
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Adapted code from HapkeLRO written by Kris Becker
   *   @history 2021-07-19 Cordell Michaud - Adapted code from PhotometricFunction written by Kris Becker
   * 
   * @param pvl Output PVL container write keywords
   */
  void HapkeLRO::report(PvlContainer &pvl) {
    pvl.addComment("IoF/LS = w/4 * (p(g) + H(mu0,w)*H(mu,w)-1) * (1+Bc0*Bc(g,h))");
    pvl.addComment("  where:");
    pvl.addComment("    p(g) = (1-xi^2)/(1-2*xi*cos(g) + xi^2)^(3/2)");
    pvl.addComment("    H(x,w) = (1+2*x)/(1+2*x*sqrt(1-w))");
    pvl.addComment("    Bc(g,h) = (1 + (1-exp(-tan(g/2)/h))/(tan(g/2)/h))/(2*(1+tan(g/2)/h)^2)");

    pvl += PvlKeyword("Algorithm", "HapkeLRO");
    pvl += PvlKeyword("ParameterMapCube", m_paramMap->fileName().toStdString());
    pvl += PvlKeyword("IncRef", std::to_string(m_iRef), "degrees");
    pvl += PvlKeyword("EmiRef", std::to_string(m_eRef), "degrees");
    pvl += PvlKeyword("PhaRef", std::to_string(m_gRef), "degrees");
    PvlKeyword units("Units");
    
    if (m_isDegrees) {
      units += "Degrees";
    }
    else {
      units += "Radians";
    }
    
    return;
  }


  void HapkeLRO::setNormalized(bool normalized) {
    m_normalized = normalized;
  }

} // namespace Isis
