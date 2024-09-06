#include "lrowacphomap.h"

#include <memory>
#include <vector>

#include <QList>
#include <QString>

#include "Buffer.h"
#include "Cube.h"
#include "CubeAttribute.h"
#include "FileName.h"
#include "HapkeLRO.h"
#include "HapkeLROC.h"
#include "IException.h"
#include "PhotometricFunction.h"
#include "ProcessByBrick.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "SpecialPixel.h"
#include "UserInterface.h"

namespace Isis {
  /**
   * @brief Apply Hapke photometric correction to a WAC cube.
   * 
   * This is the programmatic interface to the lrowacphomap application.
   * 
   * @author 2021-08-02 Cordell Michaud
   * 
   * @internal
   *   @history 2021-08-02 Cordell Michaud - Code adapted from lrowacphomap by Kris Becker
   *   @history 2024-04-02 Cordell Michaud - Changed icube to be a regular Cube object rather than 
   *                           a pointer
   * 
   * @param ui the user interface to parse parameters from
  */
  Pvl lrowacphomap(UserInterface &ui) {
    // Set up the input cube
    Cube icube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      icube.setVirtualBands(inAtt.bands());
    }
    icube.open(ui.GetCubeName("FROM"));

    Pvl log = lrowacphomap(&icube, ui);

    icube.close();

    return log;
  }

  /**
   * @brief Apply Hapke photometric correction to a WAC cube.
   * 
   * This is the programmatic interface to the lrowacphomap application.
   * 
   * @author 2021-08-02 Cordell Michaud
   * 
   * @internal
   *   @history 2021-08-02 Cordell Michaud - Code adapted from lrowacphomap by Kris Becker
   *   @history 2024-04-02 Cordell Michaud - Converted phoLro and phoLroc to smart pointers and 
   *                           added handling for new PHOALGO and PHOPARCUBE defaults
   * 
   * @param icube the input cube to apply Hapke photometric correction to
   * @param ui the user interface to parse parameters from
  */
  Pvl lrowacphomap(Cube *icube, UserInterface &ui) {
    Pvl log;

    std::shared_ptr<HapkeLRO> phoLro;
    std::shared_ptr<HapkeLROC> phoLroc;
    QString algoName;

    bool normalized;
    bool useDem;
    bool photometryOnly;

    ProcessByBrick p;

    p.SetInputCube(icube);

    // Set up the output cube
    Cube *ocube = p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"));

    bool useBackplane = false;

    if (ui.WasEntered("BACKPLANE")) {
      if (icube->bandCount() != 1) {
        std::string msg = "Invalid Image: The backplane option can only be used with a single image "
                      "band at a time.";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      CubeAttributeInput backplaneCai = ui.GetInputAttribute("BACKPLANE");

      std::vector<QString> bands = backplaneCai.bands();

      if (bands.size() == 0) {
        bands.clear();
        bands.push_back("1");
        bands.push_back("2");
        bands.push_back("3");
        bands.push_back("4");
        bands.push_back("5");
      }
      else if (bands.size() != 5) {
        std::string msg = "Invalid Backplane: The backplane must be exactly 5 bands";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      CubeAttributeInput cai;
      cai.setAttributes("+" + bands[0]);
      p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);
      cai.setAttributes("+" + bands[1]);
      p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);
      cai.setAttributes("+" + bands[2]);
      p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);
      cai.setAttributes("+" + bands[3]);
      p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);
      cai.setAttributes("+" + bands[4]);
      p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);

      useBackplane = true;
    }

    // Get the name of the parameter files
    QString algoFile = ui.GetAsString("PHOALGO");
    FileName algoFileName(algoFile);
    if (algoFileName.isVersioned()) {
      algoFileName = algoFileName.highestVersion();
    }
    if (!algoFileName.fileExists()) {
      std::string msg = algoFileName.expanded() + " does not exist.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    Pvl par(algoFileName.expanded());

    QString parCubeFile = ui.GetCubeName("PHOPARCUBE");
    FileName parCubeFileName(parCubeFile);
    if (parCubeFileName.isVersioned()) {
      parCubeFileName = parCubeFileName.highestVersion();
    }
    if (!parCubeFileName.fileExists()) {
      std::string msg = parCubeFileName.expanded() + " does not exist.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    CubeAttributeInput parCubeAtt(parCubeFileName);
    Cube parCube(parCubeFileName);
    std::vector<QString> parCubeBandsVector = parCubeAtt.bands();
    QList<QString> parCubeBands;
    parCubeBands.reserve(parCubeBandsVector.size());
    for (const QString &band : parCubeBandsVector) {
      parCubeBands.push_back(band);
    }
    if (parCubeBands.size() != 0) {
      parCube.setVirtualBands(parCubeBands);
    }

    p.SetBrickSize(128, 128, icube->bandCount());

    algoName = PhotometricFunction::algorithmName(par).toUpper();

    photometryOnly = ui.GetBoolean("PHOTOMETRYONLY");
    normalized = ui.GetBoolean("NORMALIZED");

    if (algoName == "HAPKELRO") {
      phoLro = std::make_shared<HapkeLRO>(par, *icube, !useBackplane, &parCube);
      phoLro->setMinimumPhaseAngle(ui.GetDouble("MINPHASE"));
      phoLro->setMaximumPhaseAngle(ui.GetDouble("MAXPHASE"));
      phoLro->setMinimumEmissionAngle(ui.GetDouble("MINEMISSION"));
      phoLro->setMaximumEmissionAngle(ui.GetDouble("MAXEMISSION"));
      phoLro->setMinimumIncidenceAngle(ui.GetDouble("MININCIDENCE"));
      phoLro->setMaximumIncidenceAngle(ui.GetDouble("MAXINCIDENCE"));
      
      phoLro->setNormalized(normalized);
    }
    else if (algoName == "HAPKELROC") {
      phoLroc = std::make_shared<HapkeLROC>(par, *icube, !useBackplane, &parCube);
      phoLroc->setMinimumPhaseAngle(ui.GetDouble("MINPHASE"));
      phoLroc->setMaximumPhaseAngle(ui.GetDouble("MAXPHASE"));
      phoLroc->setMinimumEmissionAngle(ui.GetDouble("MINEMISSION"));
      phoLroc->setMaximumEmissionAngle(ui.GetDouble("MAXEMISSION"));
      phoLroc->setMinimumIncidenceAngle(ui.GetDouble("MININCIDENCE"));
      phoLroc->setMaximumIncidenceAngle(ui.GetDouble("MAXINCIDENCE"));

      phoLroc->setNormalized(normalized);
    }
    else {
      std::string msg = " Algorithm Name [" + algoName + "] not recognized. ";
      msg += "Compatible Algorithms are:\n    HapkeLRO\n    HapkeLROC";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Determine how photometric angles should be calculated
    useDem = ui.GetBoolean("USEDEM");

    /**
    * @brief Apply photometric correction
    *
    * Short function dispatched for each brick to apply the photometrc
    * correction function.
    *
    * @author 2021-08-02 Cordell Michaud
    * 
    * @internal
    *   @history 2021-07-19 Cordell Michaud - Code adapted from lrowacphomap by Kris Becker
    *   @history 2024-04-02 Cordell Michaud - Changed to capture phoLro and phoLroc by value
    *
    * @param in Buffer containing input data
    * @param out Buffer of photometrically corrected data
    */
    auto phoCal = [phoLro, phoLroc, algoName, useDem, photometryOnly](
        Buffer &in, Buffer &out) -> void {
      for (int i = 0; i < in.size(); i++) {
        //  Don't correct special pixels
        if (IsSpecial(in[i])) {
          out[i] = in[i];
        }
        else {
          // Get correction and test for validity
          double ph = Null;
          if (algoName == "HAPKELRO") {
            ph = phoLro->compute(in.Line(i), in.Sample(i), in.Band(i), useDem);
          }
          else if (algoName == "HAPKELROC") {
            ph = phoLroc->compute(in.Line(i), in.Sample(i), in.Band(i), useDem);
          }
          
          if (!photometryOnly) {
            out[i] = (IsSpecial(ph) ? Null : in[i] * ph);
          }
          else {
            out[i] = (IsSpecial(ph) ? Null : ph);
          }
        }
      }

      return;
    };

    /**
    * @brief Apply photometric correction with backplane
    *
    * Short function dispatched for each brick to apply the photometrc
    * correction function with a backplane.
    *
    * @author 2021-08-02 Cordell Michaud
    * 
    * @internal
    *   @history 2021-07-19 Cordell Michaud - Code adapted from lrowacphomap by Kris Becker
    *   @history 2024-04-02 Cordell Michaud - Changed to capture phoLro and phoLroc by value
    *
    * @param in Buffer containing input data
    * @param out Buffer of photometrically corrected data
    */
    auto phoCalWithBackplane = [phoLro, phoLroc, algoName, photometryOnly](
        std::vector<Buffer *> &in, std::vector<Buffer *> &out) -> void {
      Buffer &image = *in[0];
      Buffer &phase = *in[1];
      Buffer &emission = *in[2];
      Buffer &incidence = *in[3];
      Buffer &lat = *in[4];
      Buffer &lon = *in[5];
      Buffer &calibrated = *out[0];

      for (int i = 0; i < image.size(); i++) {
        //  Don't correct special pixels
        if (IsSpecial(image[i])) {
          calibrated[i] = image[i];
        }
        else if (IsSpecial(phase[i]) || IsSpecial(emission[i]) || IsSpecial(incidence[i]) 
                 || IsSpecial(lat[i]) || IsSpecial(lon[i])) {
          calibrated[i] = Isis::Null;
        }
        else {
          // Get correction and test for validity
          double ph = Null;
          if (algoName == "HAPKELRO") {
            ph = phoLro->photometry(incidence[i], emission[i], phase[i], lat[i], lon[i], 
                                    image.Band(i));
          }
          else if (algoName == "HAPKELROC") {
            ph = phoLroc->photometry(incidence[i], emission[i], phase[i], lat[i], lon[i], 
                                     image.Band(i));
          }

          if (!photometryOnly) {
            calibrated[i] = (IsSpecial(ph) ? Null : image[i] * ph);
          }
          else {
            calibrated[i] = (IsSpecial(ph) ? Null : ph);
          }
        }
      }

      return;
    };


    // Start the processing
    if (useBackplane) {
      p.ProcessCubes(phoCalWithBackplane, false);
    }
    else {
      p.ProcessCube(phoCal, false);
    }

    PvlGroup photo("Photometry");
    if (algoName == "HAPKELRO") {
      phoLro->report(photo);
    }
    else if (algoName == "HAPKELROC") {
      phoLroc->report(photo);
    }

    ocube->putGroup(photo);
    log.addGroup(photo);

    return log;
  }
}