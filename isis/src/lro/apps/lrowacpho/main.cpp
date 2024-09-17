/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// $Id$
#include "Isis.h"

#include <string>
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Hillier.h"
#include "PhotometricFunction.h"
#include "Exponential.h"
#include "HapkeExponential.h"
#include "Pvl.h"
#include "Cube.h"

#include "PvlGroup.h"
#include "IException.h"

using namespace std;
using namespace Isis;

// Global variables
PhotometricFunction *pho;

bool useDem;

void phoCal ( Buffer &in, Buffer &out );
void phoCalWithBackplane ( std::vector<Isis::Buffer *> &in, std::vector<Isis::Buffer *> &out );

void IsisMain () {
    // We will be processing by line
    ProcessByLine p;

    // set up the input cube and get camera information
    Cube *icube = p.SetInputCube("FROM");

    // Create the output cube
    Cube *ocube = p.SetOutputCube("TO");

    // set up the user interface
    UserInterface &ui = Application::GetUserInterface();

    bool useBackplane = false;

    if (ui.WasEntered("BACKPLANE")) {
        CubeAttributeInput backplaneCai = ui.GetInputAttribute("BACKPLANE");

        if ( backplaneCai.bands().size() != 3 ) {
            string msg = "Invalid Backplane: The backplane must be exactly 3 bands";
            throw IException(IException::User, msg, _FILEINFO_);
        }

        if (icube->bandCount() != 1) {
            string msg = "Invalid Image: The backplane option can only be used with a single image band at a time.";
            throw IException(IException::User, msg, _FILEINFO_);
        }

        CubeAttributeInput cai;
        cai.setAttributes("+" + backplaneCai.bands()[0].toStdString());
        p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);
        cai.setAttributes("+" + backplaneCai.bands()[1].toStdString());
        p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);
        cai.setAttributes("+" + backplaneCai.bands()[2].toStdString());
        p.SetInputCube(ui.GetCubeName("BACKPLANE"), cai);

        useBackplane = true;
    }

    // Get the name of the parameter file
    Pvl par(ui.GetFileName("PHOPAR").toStdString());

    QString algoName = PhotometricFunction::algorithmName(par);
    algoName.toUpper();

    if (algoName == "HILLIER") {
        pho = new Hillier(par, *icube, !useBackplane);
    }
    else if (algoName == "EXPONENTIAL") {
        pho = new Exponential(par, *icube, !useBackplane);
    }
    else if (algoName == "HAPKEEXPONENTIAL") {
        pho = new HapkeExponential(par, *icube, !useBackplane);
    }
    else {
        string msg = " Algorithm Name [" + algoName.toStdString() + "] not recognized. ";
        msg += "Compatible Algorithms are:\n    Hillier\n    Exponential\n    HapkeExponential";

        throw IException(IException::User, msg, _FILEINFO_);
    }

    pho->setMinimumPhaseAngle(ui.GetDouble("MINPHASE"));
    pho->setMaximumPhaseAngle(ui.GetDouble("MAXPHASE"));
    pho->setMinimumEmissionAngle(ui.GetDouble("MINEMISSION"));
    pho->setMaximumEmissionAngle(ui.GetDouble("MAXEMISSION"));
    pho->setMinimumIncidenceAngle(ui.GetDouble("MININCIDENCE"));
    pho->setMaximumIncidenceAngle(ui.GetDouble("MAXINCIDENCE"));

    // determine how photometric angles should be calculated
    useDem = ui.GetBoolean("USEDEM");

    // Start the processing
    if (useBackplane)
        p.StartProcess(phoCalWithBackplane);
    else
        p.StartProcess(phoCal);

    PvlGroup photo("Photometry");
    pho->report(photo);
    ocube->putGroup(photo);
    Application::Log(photo);
    p.EndProcess();
}

/**
 * @brief Apply Hillier photometric correction
 *
 * Short function dispatched for each line to apply the Hillier photometrc
 * correction function.
 *
 * @author kbecker (2/20/2010)
 *
 * @param in Buffer containing input data
 * @param out Buffer of photometrically corrected data
 */
void phoCal ( Buffer &in, Buffer &out ) {

    for (int i = 0; i < in.size(); i++) {
        //  Don't correct special pixels
        if (IsSpecial(in[i])) {
            out[i] = in[i];
        }
        else {
            // Get correction and test for validity
            double ph = pho->compute(in.Line(i), in.Sample(i), in.Band(i), useDem);
            out[i] = (IsSpecial(ph) ? Null : in[i] * ph);
        }
    }

    return;
}

/**
 * @brief Apply Hillier photometric correction
 *
 * Short function dispatched for each line to apply the Hillier photometrc
 * correction function.
 *
 * @author kbecker (2/20/2010)
 *
 * @param in Buffer containing input data
 * @param out Buffer of photometrically corrected data
 */
void phoCalWithBackplane ( std::vector<Isis::Buffer *> &in, std::vector<Isis::Buffer *> &out ) {

    Buffer &image = *in[0];
    Buffer &phase = *in[1];
    Buffer &emission = *in[2];
    Buffer &incidence = *in[3];
    Buffer &calibrated = *out[0];

    for (int i = 0; i < image.size(); i++) {
        //  Don't correct special pixels
        if (IsSpecial(image[i])) {
            calibrated[i] = image[i];
        }
        else {
            // Get correction and test for validity
            double ph = pho->photometry(incidence[i], emission[i], phase[i], image.Band(i));
            calibrated[i] = (IsSpecial(ph) ? Null : image[i] * ph);
        }
    }

    return;
}
