#include <iostream>
#include "Apollo.h"
#include "IException.h"
#include "FileName.h"
#include "IString.h"

#include "Preference.h"

using namespace Isis;
using namespace std;

int main(int argc, char *argv[]) {

  Preference::Preferences(true);

  try {

    cout << "Unit test for Apollo" << endl;

    QString fname("AS15-M-1450");

    Apollo *apollo;

    apollo = new Apollo(fname);

    cout << "IsMetric: " << apollo->IsMetric() << endl;
    cout << "IsPanoramic: " << apollo->IsPanoramic() << endl;
    cout << "IsHasselblad: " << apollo->IsHasselblad() << endl;
    cout << "IsApollo15: " << apollo->IsApollo15() << endl;
    cout << "IsApollo16: " << apollo->IsApollo16() << endl;
    cout << "IsApollo17: " << apollo->IsApollo17() << endl;
    cout << "Width: " << apollo->Width() << endl;
    cout << "Height: " << apollo->Height() << endl;
    cout << "Bands: " << apollo->Bands() << endl;
    cout << "ReseauDimension: " << apollo->ReseauDimension() << endl;
    cout << "PixelPitch: " << apollo->PixelPitch() << endl;
    cout << "SpacecraftName: " << apollo->SpacecraftName().toStdString() << endl;
    cout << "InstrumentId: " << apollo->InstrumentId().toStdString() << endl;
    cout << "NaifFrameCode: " << apollo->NaifFrameCode().toStdString() << endl;
    cout << "TargetName: " << apollo->TargetName().toStdString() << endl;
    QString time=(apollo->LaunchDate().UTC());
    cout << "Time: " << time.toStdString() << endl;

  }
  catch(IException &e) {
    e.print();
  }
}
