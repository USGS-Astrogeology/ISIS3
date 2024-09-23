/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <cmath>

#include "MiCalibration.h"

using namespace std;
namespace Isis {
  namespace Mer {

    //Construct with cube and calkernel(filename)
    MiCalibration::MiCalibration(Cube &image, Pvl &kernel) {
      ReadLabels(image);
      ReadKernel(kernel);
      SetCCDTemperature(p_CCDTemperature);
      SetPCBTemperature(p_PCBTemperature);
      SetOmegaNaught();
      SetReferencePixelModel();
      SetZeroExposureValue();
      SetActiveAreaValue();
    }

    /**
     * Get keyword values for the label of the input cube that are
     * needed for the calibration equations.
     */

    void MiCalibration::ReadLabels(Cube &image) {
      PvlGroup labelgrp =
          image.label()->findGroup("Instrument", Pvl::Traverse);
      p_exposureDuration = labelgrp["ExposureDuration"];
      p_instrumentSerialNumber = labelgrp["InstrumentSerialNumber"];
      p_CCDTemperature = Isis::toDouble(labelgrp["InstrumentTemperature"][6]);
      p_PCBTemperature = Isis::toDouble(labelgrp["InstrumentTemperature"][7]);
      p_OffsetModeId = labelgrp["OffsetModeID"];
      p_shuttereffectcorrectionflag =  QString::fromStdString(labelgrp["ShutterEffectCorrectionFlag"]);
      p_filterName =  QString::fromStdString(labelgrp["FilterName"]);
      p_startTime =  QString::fromStdString(labelgrp["StartTime"]);
    }
    /**
     * Get values from the calibration kernel
     * use the instrumentSerialnumber to determain what rover
     * the image is from that get the values for that rover.
     *
     * @param kernel - the kernel from the  data area or user
     *               entered.
     */
    void MiCalibration::ReadKernel(Pvl &kernel) {
      QString rover = "MI_" + QString::number(p_instrumentSerialNumber);
      PvlGroup kernelgrp = kernel.findGroup(rover.toStdString(), Pvl::Traverse);
      p_DELCCDTa = kernelgrp["DELCCDTa"];
      p_DELCCDTb = kernelgrp["DELCCDTb"];
      p_RPVOFFa = kernelgrp["RPVOFFa"];
      p_RPVOFFb = kernelgrp["RPVOFFb"];
      p_RPPCBTa = kernelgrp["RPPCBTa"];
      p_RPPCBTb = kernelgrp["RPPCBTb"];
      p_RPPCBTc = kernelgrp["RPPCBTc"];
      p_RPCCDTa = kernelgrp["RPCCDTa"];
      p_RPCCDTb = kernelgrp["RPCCDTb"];
      p_RPCCDTc = kernelgrp["RPCCDTc"];
      p_ZEROEXPa = kernelgrp["ZEROEXPa"];
      p_ZEROEXPb = kernelgrp["ZEROEXPb"];
      p_ACTAREAa = kernelgrp["ACTAREAa"];
      p_ACTAREAb = kernelgrp["ACTAREAb"];
      p_temperatureOffset = kernelgrp["TemperatureOffset"];
      p_transfertime = kernelgrp["TransferTime"];
      p_ReferencePixelImage = QString::fromStdString(kernelgrp["ReferencePixelImage"]);
      p_ZeroExposureImage = QString::fromStdString(kernelgrp["ZeroExposureImage"]);
      p_ActiveAreaImage =  QString::fromStdString(kernelgrp["ActiveAreaImage"]);
      p_FlatImageOpen =  QString::fromStdString(kernelgrp["FlatImageOpen"]);
      p_FlatImageClosed =  QString::fromStdString(kernelgrp["FlatImageClosed"]);
    }
    /**
     * Get the CCD temperature and output a corrected
     * CCDtemperatrue. Values used in the calculation come from the
     * calibration kernel
     */
    void MiCalibration::SetCCDTemperature(double temperature) {
      p_CCDTemperature = temperature;
      p_CCDTemperatureCorrect = (p_CCDTemperature + p_temperatureOffset) +
                                p_DELCCDTa *
                                (1 - exp(p_exposureDuration / p_DELCCDTb));
    }

    /**
     * Returns the PCB temperature
     * @param temperature
     */
    void MiCalibration::SetPCBTemperature(double temperature) {
      p_PCBTemperature = temperature;
    }

    /**
     * Calculates the Omega Naught.  Is dependent on which rover the
     * data is from.
     */
    void MiCalibration::SetOmegaNaught() {
      if(p_instrumentSerialNumber == 105) {
        p_OmegaNaught = 8.53e+05 - 2.50e+03 * p_CCDTemperatureCorrect;
      }
      else if(p_instrumentSerialNumber == 110) {
        p_OmegaNaught = 8.21e+05 - 2.99e03 * p_CCDTemperatureCorrect;
      }
    }

    /**
     * Calculates the Reference Pixel Model value.  The inputs to
     * the algarithem come for the image labels and calibration
     * kernel.
     */
    void MiCalibration::SetReferencePixelModel() {
      p_ReferencePixelModel = (p_RPVOFFa - p_OffsetModeId) * p_RPVOFFb +
                              (p_RPPCBTa + p_RPPCBTb * p_exposureDuration) *
                              exp(p_RPPCBTc * p_PCBTemperature) +
                              (p_RPCCDTa + p_RPCCDTb * p_exposureDuration) *
                              exp(p_RPCCDTc * p_CCDTemperatureCorrect);

    }

    /**
     * Calculates the zero exposure value.  The inputs come from the
     * image label and calibration kernel.
     */
    void MiCalibration::SetZeroExposureValue() {
      p_ZeroExposureValue =
        p_ZEROEXPa * exp(p_ZEROEXPb * p_CCDTemperatureCorrect);
    }

    /**
     * Caluculate the active aere value.  the inputs come from the
     * image label and calibration kernel.
     */
    void MiCalibration::SetActiveAreaValue() {
      p_ActiveAreaValue = p_exposureDuration * p_ACTAREAa *
                          exp(p_ACTAREAb * p_CCDTemperatureCorrect);
    }


  }
}
