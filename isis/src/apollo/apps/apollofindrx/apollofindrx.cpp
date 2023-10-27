/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "PvlGroup.h"
#include "UserInterface.h"
#include "Cube.h"
#include "Chip.h"
#include "PolynomialBivariate.h"
#include "LeastSquares.h"
#include "Progress.h"
#include "IException.h"
#include "Apollo.h"
#include "History.h"

#include "apollofindrx.h"

using namespace std;

namespace Isis {

    int MAX_DISPX = 10,
        MAX_DISPY = 10,
        MIN_DISP = 10;

    bool Walk();
    double Register();
    void Refine();

    double tolerance = 0.0;

    Chip chip, subChip, fitChip;

    double bestSample = 0.0,
                bestLine = 0.0;

    double GoodnessOfFit = 1.0;

    void apollofindrx(UserInterface &ui) {
        Cube *cube = new Cube(ui.GetCubeName("FROM"), "rw");
        apollofindrx(cube, ui);
    }

    void apollofindrx(Cube *cube, UserInterface &ui) {
        // Import cube data & PVL information

        tolerance = ui.GetDouble("TOLERANCE");
        int patternSize = ui.GetInteger("PATTERNSIZE");
        MAX_DISPX = ui.GetInteger("DELTAX");
        MAX_DISPY = ui.GetInteger("DELTAY");

        PvlGroup &reseaus = cube->label()->findGroup("Reseaus",Pvl::Traverse);
        QString mission = QString::fromStdString((cube->label()->findGroup("Instrument",Pvl::Traverse))["SpacecraftName"]);
        QString instrument = QString::fromStdString((cube->label()->findGroup("Instrument",Pvl::Traverse))["InstrumentId"]);
        Apollo apollo(mission, instrument);
        if (mission.mid(0,6) != "APOLLO") {
            std::string msg = "This application is for use with Apollo spacecrafts only.";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // If the Keyword sizes don't match up, throw errors.
        int nres = reseaus["Line"].size();
        if (nres != reseaus["Sample"].size()) {
            std::string msg = "Sample size incorrect [Sample size " +
                            std::to_string(reseaus["Sample"].size()) + " != " + " Line size " +
                            std::to_string(reseaus["Line"].size()) + "]";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        if (nres != reseaus["Type"].size()) {
            std::string msg = "Type size incorrect [Type size " +
                            std::to_string(reseaus["Type"].size()) + " != " + " Line size " +
                            std::to_string(reseaus["Line"].size()) + "]";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        if (nres != reseaus["Valid"].size()) {
            std::string msg = "Valid size incorrect [Valid size " +
                            std::to_string(reseaus["Valid"].size()) + " != " + " Line size " +
                            std::to_string(reseaus["Line"].size()) + "]";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // Display the progress
        Progress prog;
        prog.SetMaximumSteps(nres);
        prog.CheckStatus();

        PolynomialBivariate sampPoly(1),
            linePoly(1);
        LeastSquares sampFunc(sampPoly),
                            lineFunc(linePoly);

        // Find the first good reseau (the first reseau within tolerance)
        //  then use it and the label data to
        int ds = MAX_DISPX,
            dl = MAX_DISPY;

        subChip.SetSize(patternSize, patternSize);

        int currentSample = 0 ,
            currentLine = 0;
        int dim = (int)sqrt(nres);
        int validReseaus = 0;
        // for (int res=0; res<nres; res++)
        for (int res=0; res<nres; res++) {
            currentLine = (int)(std::stod(reseaus["Line"][res])+0.5);
            currentSample = (int)(std::stod(reseaus["Sample"][res])+0.5);

            // Output chips
            chip.SetSize(patternSize + 2*ds , patternSize + 2*dl);
            chip.TackCube( currentSample, currentLine);
            chip.Load(*cube);

            if  (Walk() ) {
                double dx = 0,
                            dy = 0;
                if (res%dim > 0 && reseaus["Valid"][res-1] == "1") dy = currentLine - patternSize/2 - dl + bestLine-1 - std::stod(reseaus["Line"][res]);
                if (res/dim > 0 && reseaus["Valid"][res - dim] == "1") dx = currentSample - patternSize/2 - ds + bestSample-1 - std::stod(reseaus["Sample"][res]);
                double horizontalShift = currentSample - patternSize/2 - ds + bestSample-1 - std::stod(reseaus["Sample"][res]) - dx,
                            verticalShift = currentLine - patternSize/2 - dl + bestLine-1 - std::stod(reseaus["Line"][res]) - dy;
                for (int i=res; i<nres; i++) {
                    reseaus["Sample"][i] = std::to_string(std::stod(reseaus["Sample"][i]) + horizontalShift + ((i/dim) - (res/dim) + 1)*dx);
                    reseaus["Line"][i] = std::to_string(std::stod(reseaus["Line"][i]) + verticalShift + ((i%dim) - (res%dim) + 1)*dy);
                }
                reseaus["Valid"][res] = "1";
                validReseaus++;

                std::vector< double > xy;
                xy.push_back(res%(int)sqrt(nres));
                xy.push_back(res/(int)sqrt(nres));
                sampFunc.AddKnown(xy, std::stod(reseaus["Sample"][res]));
                lineFunc.AddKnown(xy, std::stod(reseaus["Line"][res]));

                ds = (int)(MIN_DISP+ abs(dx) + abs(horizontalShift));
                dl = (int)(MIN_DISP + abs(dy) + abs(verticalShift));
            }
            else {
                reseaus["Valid"][res] = "0";
            }

            prog.CheckStatus();
        }

        if (validReseaus > 2) {
            sampFunc.Solve();
            lineFunc.Solve();

            // for invalid reseaus, refine the estimated locations
            for (int res=0; res<nres; res++) {
                if (std::stoi(reseaus["Valid"][res])==0) {
                    std::vector< double > xy;
                    xy.push_back(res%(int)sqrt(nres));
                    xy.push_back(res/(int)sqrt(nres));
                    reseaus["Sample"][res] = std::to_string(sampFunc.Evaluate(xy));
                    reseaus["Line"][res] = std::to_string(lineFunc.Evaluate(xy));
                }
            }

            // Change status to "Refined", corrected!
            reseaus["Status"] = "Refined";
        }
        else {
            std::string msg = "No Reseaus located. Labels will not be changed.";
            msg += "Try changing the registration parameters.";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        if (cube->label()->hasObject("History")) {
            // Record apollofindrx history to the cube
            QString histName = QString::fromStdString(cube->label()->findObject("History")["Name"]);
            // create a History Blob with value found in the History PvlObject's Name keyword
            // read cube's History PvlObject data into the History Blob
            History hist = cube->readHistory(histName);
            // add apollofindrx History PvlObject into the History Blob and write to cube
            hist.AddEntry();
            cube->write(hist);
            cube->close();
        }
    }

    bool Walk() {
        // create a chip to store registration values
        fitChip.TackCube(chip.TackSample(), chip.TackLine());
        fitChip.SetSize(chip.Samples(), chip.Lines());

        double bestFit = Isis::Null;
        for (int s = (subChip.Samples()+1)/2; s <= chip.Samples()-(subChip.Samples()+1)/2+1; s++) {
            for (int l = (subChip.Lines()+1)/2; l <= chip.Lines()-(subChip.Lines()+1)/2+1; l++) {
                subChip = chip.Extract(subChip.Samples(), subChip.Lines(), s, l);
                double fit = Register();
                if (fit !=Isis::Null && (bestFit == Isis::Null || fit < bestFit)) {
                    bestSample = s;
                    bestLine = l;
                    bestFit = fit;
                }
                fitChip.SetValue(s, l, fit);
            }
        }

        if (bestFit == Isis::Null || bestFit > tolerance) return false;

        GoodnessOfFit = bestFit;
        Refine();
        return true;
    }

    // Refine the registration results to sub-pixel accuracy
    //  if it does not work, keep old results
    void Refine() {
        // if the best fit is on an edge, it cannot be refined
        if (bestSample==1 || bestLine ==1 || bestSample==fitChip.Samples() || bestLine==fitChip.Lines()) return;

        PolynomialBivariate p(2);
        LeastSquares lsq(p);
        for (int i=-1; i<=1; i++) {
            for (int j=-1; j<=1; j++) {
            int x = (int)(bestSample+i),
                y = (int)(bestLine + j);
            if (fitChip.GetValue(x, y) == Isis::Null) continue;
            std::vector<double> xy;
            xy.push_back(x);
            xy.push_back(y);
            lsq.AddKnown(xy, fitChip.GetValue(x, y) );
            }
        }
        try {
            lsq.Solve();
        } catch (IException &) {
            return;
        }

        // Get coefficients (don't need a)
        double b = p.Coefficient(1);
        double c = p.Coefficient(2);
        double d = p.Coefficient(3);
        double e = p.Coefficient(4);
        double f = p.Coefficient(5);

        // Compute the determinant
        double det = 4.0*d*f - e*e;
        if (det == 0.0) return;

        // Compute our chip position to sub-pixel accuracy
        double refinedSample = (c*e - 2.0*b*f) / det;
        double refinedLine   = (b*e - 2.0*c*d) / det;
        if ( abs(bestSample-refinedSample) < 1 && abs(bestLine-refinedLine) < 1 ) {
            bestSample = refinedSample;
            bestLine = refinedLine;
        }

        return;
    }

    double Register() {
        double sum = 0.0;
        int count = 0;

        for (int l=1; l<=subChip.Lines(); l++) {
            double min = DBL_MAX,
                        max = DBL_MIN;
            for (int i=-2; i<=2; i++) {
                double dn = subChip.GetValue( (subChip.Samples()+1)/2+i, (int)l );
                if (dn < min) min = dn;
                if (dn > max) max = dn;
            }
            if (max == min) continue;
                sum += (subChip.GetValue( (subChip.Samples()+1)/2, (int)l) - min)/(max - min);
                count++;
            }

            for (int s=1; s<=subChip.Samples(); s++) {
                double min = DBL_MAX,
                            max = DBL_MIN;
                for (int i=-2; i<=2; i++) {
                    double dn = subChip.GetValue( (int)s, (subChip.Lines()+1)/2+i );
                    if (dn < min) min = dn;
                    if (dn > max) max = dn;
                }
                if (max == min) continue;
                sum += (subChip.GetValue( (int)s, (subChip.Lines()+1)/2 ) - min)/(max - min);
                count++;
            }

        if (count < 1) return Isis::Null;
        return sum/count;
    }
}
