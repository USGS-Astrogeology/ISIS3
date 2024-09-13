/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "IException.h"
#include "Spectel.h"
#include "SpectralDefinition2D.h"
#include "Preference.h"

using std::cerr;
using std::endl;

using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cerr.precision(14);

  cerr << "----- Testing Constructors -----" << endl << endl;

  cerr << "Constructor given a 5x2x20 cube" << endl;
  
  try {
    cerr << "Constructor given a 5x2x20 cube" << endl;
    SpectralDefinition2D twoD("$ISISTESTDATA/isis/src/base/unitTestData/SpectralDefinition2D/calibration-test.cub");
    for (int samp=1; samp<=5; ++samp) {
      for (int band=1; band<=20; ++band) {
        Spectel spec = twoD.findSpectel(samp, 0, band);
        cerr << "Spectel at (s,b) (" << samp << ", " << band << ") : Wavelength=" <<
                 spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
      }
    }
  }
  catch (IException &e) {
    e.print();
  }

  cerr << "----- Testing searching -----" << endl << endl;
  // TODO: Needs fixing once getSpectelByWavelength is fully implemented
  try {
    SpectralDefinition2D twoD("$ISISTESTDATA/isis/src/base/unitTestData/SpectralDefinition2D/calibration-test-smile.cub");

    Spectel spec = twoD.findSpectel(1, 1, 1);
    cerr << "Spectel at (s,b) (" << spec.sample() << ", " << spec.band() << ") : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;

    Spectel spec2 = twoD.findSpectel(spec, 0);
    cerr << "Spectel at (s,b) (" << spec2.sample() << ", " << spec2.band() << ") : Wavelength=" <<
            spec2.centerWavelength() << " Width=" << spec2.filterWidth() << endl;

  }
  catch (IException &e) {
    e.print();
  }

  cerr << "----- Testing sections -----" << endl << endl;
  SpectralDefinition2D twoD("$ISISTESTDATA/isis/src/base/unitTestData/SpectralDefinition2D/calibration-test-sections.cub");
  cerr << "Number of section: " << twoD.sectionCount() << endl;
  for (int samp=1; samp<=5; ++samp) {
    for (int band=1; band<=20; ++band) {
      Spectel spec = twoD.findSpectel(samp, 0, band);
      cerr << "Spectel at (s,b) (" << samp << ", " << band << " ): Wavelength=" <<
      spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    }
  }
  cerr << "Section number is: " << twoD.sectionNumber(1,1,1) << endl; 
  cerr << "Section number is: " << twoD.sectionNumber(10,3,10) << endl; 

  cerr << "----- Testing toString -----" << endl << endl; 
  cerr << twoD.toString().toStdString(); 

  cerr << "----- Testing outside of definition errors -----" << endl << endl;
  //TODO: Wait until isis exception error TODOs are done in 2D
  //try {
  //  cerr << "Constructor given a 5x2x20 cube" << endl;
  //  SpectralDefinition2D twoD("$ISISTESTDATA/isis/src/base/unitTestData/SpectralDefinition2D/calibration-test.cub");
  //
  //  try {
  //    Spectel spec = twoD.getSpectel(1, 0, 21);
  //  }
  //  catch (IException &e) {
  //    e.print();
  //  }
  //
  //  try {
  //    Spectel spec = twoD.getSpectel(0, 0, 1);
  //  }
  //  catch (IException &e) {
  //    e.print();
  //  }
  //
  //}
  //catch (IException &e) {
  //  // Shouldn't get here
  //  e.print();
  //}

}
