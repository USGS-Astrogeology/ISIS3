/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include "IException.h"
#include "Preference.h"
#include "Spectel.h"
#include "SpectralDefinition1D.h"

using std::cerr;
using std::endl;

using namespace Isis;

int main(int argc, char *argv[]) {
  Preference::Preferences(true);

  cerr.precision(14);

  cerr << "----- Testing Constructors -----" << endl << endl;

  try {
    cerr << "Constructor given a 20 row csv" << endl;
    SpectralDefinition1D oneD("assets/test-target.csv");
    cerr << "The number of sections is : " << oneD.sectionCount() << endl; 
    for (int band=1; band<=20; ++band) {
        Spectel spec = oneD.findSpectel(0, 0, band);
        cerr << "Spectel at band: " << band << " : Wavelength=" <<
                 spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    }
  }
  catch (IException &e) {
    e.print();
  }

  cerr << endl << "----- Testing searching -----" << endl << endl;
  // TODO: Needs fixing once getSpectelByWavelength is fully implemented
  try {
    SpectralDefinition1D oneD("assets/test-target.csv");
    cerr << "The number of sections is : " << oneD.sectionCount() << endl; 
    Spectel spec = oneD.findSpectelByWavelength(9.1, 0);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
  }
  catch (IException &e) {
    e.print();
  }

  cerr << endl << "----- Testing multiple sections -----" << endl << endl;
  try {
    SpectralDefinition1D oneD("assets/test-target-sections.csv");
    cerr << "The number of sections is : " << oneD.sectionCount() << endl; 
    Spectel spec;
    spec = oneD.findSpectelByWavelength(5.2, 3);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    //findSpectel
    Spectel spec2;
    spec2 = oneD.findSpectel(spec, oneD.sectionNumber(spec.sample(), spec.line(), spec.band()));
    cerr << "Spectel at band: " << spec2.band() << " : Wavelength=" <<
            spec2.centerWavelength() << " Width=" << spec2.filterWidth() << endl;
    spec = oneD.findSpectelByWavelength(5.2, 1);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    spec = oneD.findSpectelByWavelength(5.2, 0);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    spec = oneD.findSpectelByWavelength(5.2, 2);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
   for (int band=1; band<=29; ++band) {
     spec = oneD.findSpectel(0, 0, band);
     cerr << "Spectel at band: " << band << " : Wavelength=" <<
                 spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
   }

    cerr << "Section number is: " << oneD.sectionNumber(1,1,1) << endl; 
    cerr << "Section number is: " << oneD.sectionNumber(10,3,10) << endl; 
  }
  catch (IException &e) {
    e.print();
  }

  cerr << endl << "----- Testing multiple sections in opposite order-----" << endl << endl;
  try {
    SpectralDefinition1D oneD("assets/test-target-sections-desc.csv");
    cerr << oneD.toString().toStdString(); 
    cerr << "The number of sections is : " << oneD.sectionCount() << endl; 
    Spectel spec;
    spec = oneD.findSpectelByWavelength(5.2, 3);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    //findSpectel
    Spectel spec2;
    spec2 = oneD.findSpectel(spec, oneD.sectionNumber(spec.sample(), spec.line(), spec.band()));
    cerr << "Spectel at band: " << spec2.band() << " : Wavelength=" <<
            spec2.centerWavelength() << " Width=" << spec2.filterWidth() << endl;
    spec = oneD.findSpectelByWavelength(5.2, 1);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    spec = oneD.findSpectelByWavelength(5.2, 0);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
    spec = oneD.findSpectelByWavelength(5.2, 2);
    cerr << "Spectel at band: " << spec.band() << " : Wavelength=" <<
            spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
   for (int band=1; band<=29; ++band) {
     spec = oneD.findSpectel(0, 0, band);
     cerr << "Spectel at band: " << band << " : Wavelength=" <<
                 spec.centerWavelength() << " Width=" << spec.filterWidth() << endl;
   }
    cerr << "Section number is: " << oneD.sectionNumber(1,1,1) << endl; 
    cerr << "Section number is: " << oneD.sectionNumber(10,3,10) << endl; 
    cerr << endl << "---- Testing toString --- " << endl << endl; 
    cerr << oneD.toString().toStdString(); 
  }
  catch (IException &e) {
    e.print();
  }

  cerr << endl << "---- Testing for errors --- " << endl << endl; 
  try {
    SpectralDefinition1D oneD("assets/test-target-sections-desc.csv");
    oneD.findSpectelByWavelength(5.2,6); 
  }
  catch (IException &e) {
    e.print();
  }
  try {
    SpectralDefinition1D oneD("assets/broken.csv");
  }
  catch (IException &e) {
    e.print();
  }
  try {
    SpectralDefinition1D oneD("assets/morebroken.csv");
  }
  catch (IException &e) {
    e.print();
  }

  cerr << endl << "----- Testing outside of definition errors -----" << endl << endl;

  //TODO: Wait until isis exception error TODOs are done in 2D
  //try {
  //  cerr << "Constructor given a 5x2x20 cube" << endl;
  //  SpectralDefinition2D twoD("assets/calibration-test.cub");
  //  cerr << "The number of sections is : " << twoD.sectionCount() << endl; 
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
