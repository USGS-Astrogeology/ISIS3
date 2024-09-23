/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ProcessImportPds.h"
#include "FileName.h"
#include "Brick.h"
#include "ProcessByBrick.h"
#include "OriginalLabel.h"
#include "IException.h"
#include "marciflip.h"

using namespace std;

Isis::Cube *outputCube = NULL;
int currentLine;
int filterHeight = 16;

void flipCube(Isis::Buffer &data);

namespace Isis {

    void marciflip(UserInterface &ui) {

        Isis::ProcessByBrick p;
        QString cubeFn;
        Isis::Cube *icube;
        Isis::CubeAttributeInput inAtt;

        cubeFn = ui.GetCubeName("FROM");
        icube = new Cube(cubeFn.toStdString());
        inAtt = ui.GetInputAttribute("FROM");

        p.SetInputCube(cubeFn, inAtt);

        filterHeight = 16 / (int)icube->group("Instrument")["SummingMode"];

        p.SetBrickSize(icube->sampleCount(), filterHeight, icube->bandCount());

        currentLine = icube->lineCount();

        outputCube = new Cube();
        outputCube->setDimensions(icube->sampleCount(), icube->lineCount(), icube->bandCount());

        outputCube->create(ui.GetCubeName("TO"));

        if(icube->hasGroup("Instrument")) {
            PvlGroup inst = icube->group("Instrument");

            // change flipped keyword
            inst["DataFlipped"] = Isis::toString(((int)inst["DataFlipped"] + 1) % 2);

            outputCube->label()->findObject("IsisCube").addGroup(inst);
        }

        if(icube->hasGroup("BandBin")) {
            outputCube->label()->findObject("IsisCube").addGroup(
                icube->group("BandBin"));
        }

        if(icube->label()->hasObject("OriginalLabel")) {
            OriginalLabel origLabel = icube->readOriginalLabel();
            outputCube->write(origLabel);
        }

        p.StartProcess(flipCube);
        p.EndProcess();

        outputCube->close();
        delete outputCube;
    }
}

void flipCube(Isis::Buffer &data) {
    currentLine -= filterHeight;
    Isis::Brick outBrick(data.SampleDimension(), data.LineDimension(), data.BandDimension(), data.PixelType());
    outBrick.Copy(data);
    outBrick.SetBasePosition(1, currentLine + 1, data.Band());
    outputCube->write(outBrick);
}
