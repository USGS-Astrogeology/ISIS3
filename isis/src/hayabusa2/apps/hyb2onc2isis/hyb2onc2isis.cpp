/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QString>

#include "FileName.h"
#include "iTime.h"
#include "OriginalLabel.h"
#include "ProcessImportFits.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "PvlObject.h"
#include "PvlToPvlTranslationManager.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {

    // Cube *outputCube = NULL;

    void hyb2onc2isis(UserInterface &ui) {
        ProcessImportFits importFits;
        importFits.setFitsFile(FileName(ui.GetFileName("FROM")));
        importFits.setProcessFileStructure(0);

        CubeAttributeOutput &att = ui.GetOutputAttribute("TO");
        Cube *outputCube = importFits.SetOutputCube(ui.GetCubeName("TO"), att);

        // Get the directory where the Hayabusa translation tables are.
        QString transDir = "$ISISROOT/appdata/translations/";

        // Create a PVL to store the translated labels in
        Pvl outputLabel;
        
        // Get the FITS label
        Pvl fitsLabel;
        fitsLabel.addGroup(importFits.fitsImageLabel(0));
        
        try {
            fitsLabel.addGroup(importFits.extraFitsLabel(0));
        }
        catch (IException &e) {
            QString msg = "Input file [" + FileName(ui.GetFileName("FROM")).expanded() +
                        "] does not appear to be a Hayabusa2/ONC label file.";
            throw IException(e, IException::Unknown, msg, _FILEINFO_);
        }

        QString instid;
        QString missid;
        try {
            instid = QString::fromStdString(fitsLabel.findGroup("FitsLabels").findKeyword("INSTRUME")[0]);
            missid = QString::fromStdString(fitsLabel.findGroup("FitsLabels").findKeyword ("SPCECRFT")[0]);
        }
        catch (IException &e) {
            QString msg = "Unable to read instrument ID, [INSTRUME], or spacecraft ID, [SPCECRFT], "
                        "from input file [" + FileName(ui.GetFileName("FROM")).expanded() + "]";
            throw IException(e, IException::Io,msg, _FILEINFO_);
        }

        missid = missid.simplified().trimmed();
        if ((QString::compare(missid, "HAYABUSA2", Qt::CaseInsensitive) != 0) && (QString::compare(missid, "HAYABUSA-2", Qt::CaseInsensitive) != 0)) {
            QString msg = "Input file [" + FileName(ui.GetFileName("FROM")).expanded() +
                        "] does not appear to be a Hayabusa2 label file.";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }
        instid = instid.simplified().trimmed();
        if (QString::compare(instid, "Optical Navigation Camera", Qt::CaseInsensitive) != 0) {
            QString msg = "Input file [" + FileName(ui.GetFileName("FROM")).expanded() +
                        "] does not appear to be a Hayabusa2/ONC label file.";
            throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        // Translate the Instrument group
        FileName transFile(transDir + "Hayabusa2OncInstrument.trn");
        PvlToPvlTranslationManager instrumentXlater (fitsLabel, transFile.expanded());
        instrumentXlater.Auto(outputLabel);

        //  Update target if user specifies it
        PvlGroup &instGrp = outputLabel.findGroup("Instrument",Pvl::Traverse);
        QString target;
        if (ui.WasEntered("TARGET")) {
            instGrp["TargetName"] = ui.GetString("TARGET").toStdString();
        }
        instGrp["ExposureDuration"].setUnits("seconds");
        outputCube->putGroup(instGrp);

        // Translate the BandBin group
        transFile = transDir + "Hayabusa2OncBandBin.trn";
        PvlToPvlTranslationManager bandBinXlater (fitsLabel, transFile.expanded());
        bandBinXlater.Auto(outputLabel);
        PvlGroup &bandGrp = outputLabel.findGroup("BandBin",Pvl::Traverse);
        if (bandGrp.hasKeyword("Width")) { // if width exists, then so must center
            bandGrp["Width"].setUnits("nanometers");
            bandGrp["Center"].setUnits("nanometers");
        }
        outputCube->putGroup(outputLabel.findGroup("BandBin",Pvl::Traverse));

        // Translate the Archive group
        transFile = transDir + "Hayabusa2OncArchive.trn";
        PvlToPvlTranslationManager archiveXlater (fitsLabel, transFile.expanded());
        archiveXlater.Auto(outputLabel);
        PvlGroup &archGrp = outputLabel.findGroup("Archive", Pvl::Traverse);
        QString source = QString::fromStdString(archGrp.findKeyword("SourceProductId")[0]);
        archGrp["SourceProductId"].setValue(FileName(source).baseName().toStdString());

        //  Create YearDoy keyword in Archive group
        iTime stime(QString::fromStdString(outputLabel.findGroup("Instrument", Pvl::Traverse)["StartTime"][0]));
        PvlKeyword yeardoy("YearDoy", std::to_string(stime.Year()*1000 + stime.DayOfYear()));
        archGrp.addKeyword(yeardoy);
        outputCube->putGroup(archGrp);


        // Create a Kernels group
        transFile = transDir + "Hayabusa2OncKernels.trn";
        PvlToPvlTranslationManager kernelsXlater(fitsLabel, transFile.expanded());
        kernelsXlater.Auto(outputLabel);
        outputCube->putGroup(outputLabel.findGroup("Kernels", Pvl::Traverse));

        // Now write the FITS augmented label as the original label
        // Save the input FITS label in the Cube original labels
        OriginalLabel originalLabel(fitsLabel);
        outputCube->write(originalLabel);

        // Convert the image data
        importFits.Progress()->SetText("Importing Hayabusa2 image");
        importFits.StartProcess();
        importFits.Finalize();
    }
}
