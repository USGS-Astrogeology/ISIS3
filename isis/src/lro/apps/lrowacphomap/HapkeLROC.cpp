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

#include "Projection.h"
#include "Camera.h"
#include "HapkeLROC.h"
#include "PhotoModelFactory.h"
#include "PhotoModel.h"
#include "PvlObject.h"
#include "Brick.h"

using namespace std;

namespace Isis {

  /**
   * Create an HapkeLROC photometric object
   * 
   * @author 2021-08-09 Cordell Michaud
   * 
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Code adapted from HapkeLROC written by Dan Clarke
   *   @history 2021-07-19 Cordell Michaud - Code adapted from PhotometricFunction written by Kris Becker
   * 
   * @param pvl       Photometric parameter files
   * @param cube      Input cube file
   * @param useCamera Enable using camera from input cube
   * @param paramMap  Parameter cube
   */
  HapkeLROC::HapkeLROC(PvlObject &pvl, Cube &cube, bool useCamera, Cube *paramMap)
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
        HapkeLROC::Parameters parms;

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

    m_photoThetaold = -999.0;
    m_oldPhase = -9999;
    m_oldIncidence = -9999;
    m_oldEmission = -9999;

    m_hfunc = "HG";
    if (phoModel.hasKeyword("Hfunc")) {
      m_hfunc = QString::fromStdString(phoModel.findKeyword("Hfunc")[0]).toUpper();
    }

    if (m_hfunc != "HG") {
      std::string msg = "Invalid HFunction: " + m_hfunc;
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  /**
   * Destructor
   */
  HapkeLROC::~HapkeLROC() {
    for (int i = 0; i < m_paramBrickCount; i++) {
      delete m_paramBricks[i];
    }
  
    delete [] m_paramBricks;
  };

  //           {       [      1-2 r  x            ] } -1
  //           {       |           0      / 1+x \ | }
  //  H(x)  ~= { - w x | r  + -------- ln | --- | | }
  //           {       [  0      2        \  x  / ] }
  //  where r  is,
  //         0
  //            1 - sqrt(1-w)
  //       r  = -------------   (precalculated and passed in for speed)
  //        0   1 + sqrt(1-w)
  //
  inline double HGfunc(double x, double w, double r0) {
    if (x == 0.0) return 1.0;
    x = abs(x);
    double hxw = 1.0 - w * x * (r0 + ((1.0 - 2.0 * r0 * x) * 0.5 * log((1.0 + x) / x)));

    return 1.0 / hxw;
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
   *   @history 2021-07-19 Cordell Michaud - Adapted code from HapkeLROC written by Dan Clarke
   *   @history 2021-07-19 Cordell Michaud - Adapted code from PhotometricFunction written by Kris Becker
   * 
   * @param i     Incidence angle at cube location
   * @param e     Emission angle at cube location
   * @param g     Phase angle at cube location
   * @param band  Band number in cube (actually is band index) for lookup purposes
   * 
   * @return double Photometric correction using parameters
   */
  double HapkeLROC::photometry(double i, double e, double g, int band) const {
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
   *   @history 2021-08-03 Cordell Michaud - Adapted code from HapkeLROC written by Dan Clarke
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
  double HapkeLROC::photometry(double i, double e, double g, double lat, double lon, int band) const {
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
        HapkeLROC::Parameters &parms = m_bandParameters[p];

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
   * This routine computes photometric correction using a modified
   * version of the HapkeHen model in Isis.
   *
   * @author 2021-08-19 Cordell Michaud
   * 
   * @internal
   *   @history 2021-07-19 Cordell Michaud - Adapted code from HapkeLROC written by Dan Clarke
   *
   * @param parms Container of band-specific HapkeLROC parameters
   * @param i     Incidence angle (in degrees)
   * @param e     Emission angle (in degrees)
   * @param g     Phase angle (in degrees)
   *
   * @return double Photometric correction parameter
   */
  double HapkeLROC::photometry (const HapkeLROC::Parameters &parms, double i, double e, double g) const {
    //                        cos(i )
    //                w            e
    //  r(i,e,g) = K ---- --------------- [p(g) (1 + B  B (g)) + M(i ,e )] [1 + B  B (g)] S(i,e,g)
    //                4   cos(i ) + cos(e )           S0 S          e  e         C0 C
    //                         e         e

    double w = parms["W"]; // Wh
    double b = parms["B"]; // Hg1
    double c = parms["C"]; // Hg2
    double bc0 = parms["BCO"]; // Bc0
    double hc = parms["HC"]; // hc
    double bs0 = parms["BSO"]; // B0
    double hs = parms["HS"]; // Hh
    double theta = parms["THETA"]; // Theta
    double phi = parms["PHI"]; // phi

    if (m_oldPhase == g && m_oldIncidence == i && m_oldEmission == e) {
      return m_result;
    }

    m_oldPhase = g;
    m_oldIncidence = i;
    m_oldEmission = e;

    double invPI = 1.0 / PI;

    //                          2/3
    //       -ln(1 - 1.209 / phi   )
    //  K = -------------------------
    //                       2/3
    //            1.209 / phi
    double k = 1.0;
    if (phi != 0.0) {
      double ck = 1.209 * pow(phi, 2.0/3.0);
      k = -log(1 - ck) / ck;
    }

    double incrad = i; // incidence angle in radians
    double emarad = e; // emission angle in radians

    if (m_isDegrees) {
      g *= DEG2RAD;
      incrad *= DEG2RAD;
      emarad *= DEG2RAD;

      // convert theta to radians
      theta *= DEG2RAD;
    }

    double cosg = cos(g);
    double cosi = cos(incrad);
    double cose = cos(emarad);

    double cost;
    double sint;
    double tan2t;

    if (theta != m_photoThetaold) {
      cost = cos(theta);
      sint = sin(theta);
      m_photoCott = cost / max(1.0e-10, sint);
      m_photoCot2t = m_photoCott * m_photoCott;
      m_photoTant = sint / cost;
      tan2t = m_photoTant * m_photoTant;
      m_invChiThetaP = sqrt(1.0 + PI * tan2t);
      m_chiThetaP = 1.0 / m_invChiThetaP;
      m_photoThetaold = theta;
    }

    if (i >= 90.0) {
      m_result = 0.0;
      return m_result;
    }

    //  p(g) is, (H-G phase function)
    //                                2                                2
    //             1 + c         1 - b               1 - c        1 - b 
    //      p(g) = ----- ------------------------- + ----- -------------------------
    //               2                      2 3/2      2                      2 3/2
    //                    (1 - 2b cos(g) + b )              (1 + 2b cos(g) + b )   
    double b2 = b * b;
    double pg1 = (1.0 + c) * 0.5 * (1.0 - b2) / pow((1.0 - 2.0 * b * cosg + b2), 1.5);
    double pg2 = (1.0 - c) * 0.5 * (1.0 - b2) / pow((1.0 + 2.0 * b * cosg + b2), 1.5);
    double pg = pg1 + pg2;

    //  B (g) is, (Shadow Hiding Opposition surge Effect: SHOE )
    //   S
    //        B (g) = 1 / [1 + tan(g/2) / h ]
    //         S                           s
    double tang2 = tan(g * 0.5);
    double bsg;
    if (hs == 0.0 || g == PI) {
      bsg = 0.0;
    }
    else {
      bsg = 1.0 / (1.0 + tang2 / hs);
    }

    //  B (g) is, (Coherent Backscatter Opposition surge Effect: CBOE)
    //   C
    //                            1 - exp[-tan(g/2) / h ]
    //                                                 c
    //                       1 + -------------------------
    //                                 tan(g/2) / h 
    //                                             c
    //              B (g) = -------------------------------
    //               C                               2
    //                           2[1 + tan(g/2) / h ] 
    //                                             c
    double bcg = 0.0;
    if (g == 0.0) { // if g is 0, bcg is undefined, so set it to 1
      bcg = 1.0;
    }
    else if (g != PI && hc != 0.0) {
      double tang2hc = tang2 / hc;
      bcg = (1 + ((1 - exp(-tang2hc)) / tang2hc)) / (2 * (1 + tang2hc) * (1 + tang2hc));
    }

    // If smooth Hapke is wanted then set Theta<=0.0
    if (theta <= 0.0) {
      double mie = 0.0;
      if (m_hfunc == "HG") {
        //       1 - sqrt(1 - w)
        //  r  = ---------------   (precalculated and passed in to HGfunc for speed, see comments on HGfunc)
        //   0   1 + sqrt(1 - w)
        double r0sqrt = sqrt(1.0 - w);
        double r0 = (1.0 - r0sqrt) / (1.0 + r0sqrt);

        //              (  cos(i )     )  (  cos(e )     )
        //              (       e      )  (       e      )
        //  M(i ,e ) = H( ---------, w ) H( ---------, w ) - 1
        //     e  e     (    K         )  (    K         )
        double invK = 1.0 / k;
        mie = HGfunc(cosi*invK, w, r0) * HGfunc(cose*invK, w, r0) - 1.0;
      }

      //                w       cos(i)
      //  r(i,e,g) = K ---- --------------- [p(g) (1 + B  B (g)) + M(i ,e )] [1 + B  B (g)]
      //                4   cos(i) + cos(e)             S0 S          e  e         C0 C
      m_result = k * (w / 4.0) * (cosi / (cosi + cose)) * (pg * (1.0 + bs0*bsg) + mie) * (1.0 + bc0*bcg);

      return m_result;
    }

    //             [    2       _____          ]
    //  E (i) = exp| - ---- cot(theta ) cot(i) | = e1i
    //   1         [    pi           p         ]
    //
    //             [    1      2 _____      2    ]
    //  E (i) = exp| - ---- cot (theta ) cot (i) | = e2i
    //   2         [    pi            p          ]
    double sini = sin(incrad);
    double coti = cosi / max(1.0e-10, sini);
    double e1i = exp(min(-2.0 * m_photoCott * coti * invPI, 23.0));
    double cot2i = coti * coti;
    double e2i = exp(min(-m_photoCot2t * cot2i * invPI, 23.0));


    //                       [                                 E (i)    ]
    //               _____   |                     _____        2       |
    //  eta(i) = chi(theta  )| cos(i) + sin(i) tan(theta  ) ----------- |
    //                     p |                           p   2 - E (i)  |
    //                       [                                    1     ]
    double etai = m_chiThetaP * (cosi + sini * m_photoTant * e2i / (2.0 - e1i));

    double sine = sin(emarad);
    double sinei = sine * sini;

    //              [        / psi  \  ]
    //  f(psi) = exp| -2 tan|  ----  | |
    //              [        \  2   /  ]
    //
    //              [  cos(g) - cos(e) cos(i)  ]
    //  psi = arccos| ------------------------ |
    //              [       sin(e) sin(i)      ]
    double cospsi;
    double psi;
    
    if (sinei == 0.0) {
      cospsi = 1.0;
      psi = 0.0;
    }
    else {
      cospsi = (cosg - cose * cosi) / sinei;
      if (cospsi <= -1.0) {
        psi = PI;
      }
      else if (cospsi > 1.0) {
        psi = 0.0;
      }
      else {
        psi = acos(cospsi);
      }
    }

    double halfpsi = psi * 0.5;
    double fpsi;
    if (halfpsi >= HALFPI) {
      fpsi = 0.0;
    }
    else {
      double tanhalfpsi = tan(halfpsi);
      fpsi = exp(min(-2.0 * tanhalfpsi, 23.0));
    }

    //             [    2       _____          ]
    //  E (e) = exp| - ---- cot(theta ) cot(e) | = e1e
    //   1         [    pi           p         ]
    //
    //             [    1      2 _____      2    ]
    //  E (e) = exp| - ---- cot (theta ) cot (e) | = e2e
    //   2         [    pi            p          ]
    double cote = cose / max(1.0e-10, sine);
    double e1e = exp(min(-2.0 * m_photoCott * cote * invPI, 23.0));
    double cot2e = cote * cote;
    double e2e = exp(min(-m_photoCot2t * cot2e * invPI, 23.0));


    //                       [                                 E (e)    ]
    //               _____   |                     _____        2       |
    //  eta(e) = chi(theta  )| cos(e) + sin(e) tan(theta  ) ----------- |
    //                     p |                           p   2 - E (e)  |
    //                       [                                    1     ]
    double etae = m_chiThetaP * (cose + sine * m_photoTant * e2e / (2.0 - e1e));

    double sin2psi2 = sin(halfpsi);
    sin2psi2 *= sin2psi2;
    double psiOverPI = psi * invPI;

    double mu0e;
    double mue;
    double ecei;
    double s2ei;
    double ecee;
    double s2ee;
    if (i <= e) {
      //                              [                                                  2               ]
      //                              |                              cos(psi) E (e) + sin (psi/2) E (i)  |
      //                       _____  |                     _____              2                   2     |
      //  mu   = cos(i ) = chi(theta )| cos(i) + sin(i) tan(theta ) ------------------------------------ |
      //    0e        e             p |                          p       2 - E (e) - (psi/pi) E (i)      |
      //                              [                                       1                1         ]
      //       
      //                              [                                         2                ]
      //                              |                              E (e) - sin (psi/2) E (i)   |
      //                       _____  |                     _____     2                   2      |
      //  mu   = cos(e ) = chi(theta )| cos(e) + sin(e) tan(theta ) ---------------------------- |
      //    e         e             p |                          p   2 - E (e) - (psi/pi) E (i)  |
      //                              [                                   1                1     ]
      ecei = (2.0 - e1e - psiOverPI * e1i);
      s2ei = sin2psi2 * e2i;
      mu0e = m_chiThetaP * (cosi + sini * m_photoTant * (cospsi * e2e + s2ei) / ecei);
      mue = m_chiThetaP * (cose + sine * m_photoTant * (e2e - s2ei) / ecei);
    }
    else {
      //                              [                                         2                ]
      //                              |                              E (i) - sin (psi/2) E (e)   |
      //                       _____  |                     _____     2                   2      |
      //  mu   = cos(i ) = chi(theta )| cos(i) + sin(i) tan(theta ) ---------------------------- |
      //    0e        e             p |                          p   2 - E (i) - (psi/pi) E (e)  |
      //                              [                                   1                1     ]
      //
      //                              [                                                  2               ]
      //                              |                              cos(psi) E (i) + sin (psi/2) E (e)  |
      //                       _____  |                     _____              2                   2     |
      //  mu   = cos(e ) = chi(theta )| cos(e) + sin(e) tan(theta ) ------------------------------------ |
      //    e         e             p |                          p       2 - E (i) - (psi/pi) E (e)      |
      //                              [                                       1                1         ]
      ecee = (2.0 - e1i - psiOverPI * e1e);
      s2ee = sin2psi2 * e2e;
      mu0e = m_chiThetaP * (cosi + sini * m_photoTant * (e2i - s2ee) / ecee);
      mue = m_chiThetaP * (cose + sine * m_photoTant * (cospsi * e2i + s2ee) / ecee);
    }

    double mie = 0.0;
    if (m_hfunc == "HG") {
      //       1 - sqrt(1 - w)
      //  r  = ---------------   (precalculated and passed in to HGfunc for speed, see comments on HGfunc)
      //   0   1 + sqrt(1 - w)
      double r0sqrt = sqrt(1.0 - w);
      double r0 = (1.0 - r0sqrt) / (1.0 + r0sqrt);

      //              (  cos(i )     )  (  cos(e )     )
      //              (       e      )  (       e      )
      //  M(i ,e ) = H( ---------, w ) H( ---------, w ) - 1
      //     e  e     (    K         )  (    K         )
      double invK = 1.0 / k;
      mie = HGfunc(mu0e*invK, w, r0) * HGfunc(mue*invK, w, r0) - 1.0;
    }

    //                                                 _____
    //                 mu       mu                 chi(theta )
    //                   e        0                         p
    //  S(i,e,psi) = -------- -------- -----------------------------------
    //                eta(e)   eta(i)                           _____
    //                                  1 - f(psi) + f(psi) chi(theta ) q
    //                                                               p
    double q;
    if (i <= e) {
      //  q = [mu /eta(i)]
      //         0
      q = cosi / etai;
    }
    else {
      //  q = [mu/eta(e)]
      q = cose / etae;
    }
    double s = (mue * cosi * m_chiThetaP) / (etae * etai * (1.0 - fpsi + (fpsi * m_chiThetaP * q)));

    //                        cos(i )
    //                w            e
    //  r(i,e,g) = K ---- --------------- [p(g) (1 + B  B (g)) + M(i ,e )] [1 + B  B (g)] S(i,e,g)
    //                4   cos(i ) + cos(e )           S0 S          e  e         C0 C
    //                         e         e
    m_result = k * (w / 4.0) * (mu0e / (mu0e + mue)) * (pg * (1.0 + bs0*bsg) + mie) * (1.0 + bc0*bcg) * s;

    return m_result;
  }


  bool HapkeLROC::normalized() const {
    return m_normalized;
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
   *   @history 2021-08-02 Cordell Michaud - Adapted code from PhotometricFunction written by Kris Becker
   * 
   * @param pvl Output PVL container in which to write keywords
   */
  void HapkeLROC::report(PvlContainer &pvl) {
    pvl += PvlKeyword("Algorithm", "HapkeLROC");
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


  void HapkeLROC::setNormalized(bool normalized) {
    m_normalized = normalized;
  }
} // namespace Isis