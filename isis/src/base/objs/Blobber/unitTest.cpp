/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "Preference.h"
#include "SpecialPixel.h"
#include "Blobber.h"
#include "IException.h"
#include "Cube.h"

using namespace std;
using namespace Isis;

int main(int argc, char *argv[]) {
  int nfailed(0);
  try {

    cout << "*** Blobber unitTest ***\n";
    Preference::Preferences(true);
    QString fname = "$ISISTESTDATA/isis/src/base/unitTestData/blobTruth.cub";
    cout << "\nFile Source: " << fname.toStdString() << endl;

// This is assumed to be Hirise cube source
    QString blobName  = "HiRISE Calibration Ancillary";
    QString fieldName = "DarkPixels";
    QString name      = "Blobber";

// Test access from Cube object class
    cout << "\n** Test Cube Class access...\n";
    Cube cube;
    cube.open(fname);
    Blobber blob1(cube, blobName, fieldName, name);
    cout << "Blob Name:           " << blob1.getName().toStdString() << endl;
    cout << "Object Name:         " << blob1.getBlobName().toStdString() << endl;
    cout << "Field Name:          " << blob1.getFieldName().toStdString() << endl;
    cout << "Number Blob Lines:   " << blob1.Lines() << endl;
    cout << "Number Blob Samples: " << blob1.Samples() << endl;
    cout << "Total Pixels:        " << blob1.size() << endl;

    BigInt lines(blob1.Lines()), samples(blob1.Samples()),
           ntotal(blob1.size());

//  Ensure product of lines & sample are equal to size()
    if(ntotal != (lines * samples)) {
      cout << "--> FAILED < Total Blob Size (" << ntotal
                << ") does not equal product of Lines, Samples ("
                << (lines * samples) << ")!\n";
      nfailed++;
    }

//  Test access to pixels
    double pixel0 = blob1[0][0];
    double pixelN = blob1[lines-1][samples-1];
    double pixelx = blob1[lines/2][samples/2];

    cout << "\n** Blob Values...\n";
    cout << "Pixel[0][0] = " << PixelToString(pixel0) << endl;
    cout << "Pixel[" << (lines / 2) << "][" << (samples / 2) << "] = "
              << PixelToString(pixelx) << endl;
    cout << "Pixel[" << (lines - 1) << "][" << (samples - 1) << "] = "
              << PixelToString(pixelN) << endl;

//  Now reload using Cube explicit method
    blob1.load(cube);
    if(pixel0 != blob1[0][0]) {
      cout << "--> FAILED < Pixel[Line=0][Sample=0] = " << pixel0
                << "  != " << blob1[0][0] << " after reload!!" << endl;
      nfailed++;
    }

    if(pixelx != blob1[lines/2][samples/2]) {
      cout << "--> FAILED < Pixel[Line=" << (lines / 2)
                << "][Sample=" << (samples / 2)
                << "] = " << pixelx << "  != "
                << blob1[lines/2][samples/2]
                << " after reload!!" << endl;
      nfailed++;
    }


    if(pixelN != blob1[lines-1][samples-1]) {
      cout << "--> FAILED < Pixel[Line=" << (lines - 1)
                << "][Sample=" << (samples - 1)
                << "] = " << pixelN << "  != "
                << blob1[lines-1][samples-1]
                << " after reload!!" << endl;
      nfailed++;
    }

// Test Cube access to blob
    cout << "\n** Test Blobber direct access (via filename)...\n";
    Blobber blob3(blobName, fieldName, name);
    blob3.load(fname);

    if(pixelx != blob3[lines/2][samples/2]) {
      cout << "--> FAILED < Pixel[Line=" << (lines / 2)
                << "][Sample=" << (samples / 2)
                << "] = " << pixelx << "  != "
                << blob1[lines/2][samples/2]
                << " from named file source!!" << endl;
      nfailed++;
    }

// Test copying
    cout << "\n** Test Blobber(Blobber) constructor...\n";
    Blobber blob4(blob1);
    if(pixelx != blob4[lines/2][samples/2]) {
      cout << "--> FAILED < Pixel[Line=" << (lines / 2)
                << "][Sample=" << (samples / 2)
                << "] = " << pixelx << "  != "
                << blob1[lines/2][samples/2]
                << " from Blobber(blob) constructor!!" << endl;
      nfailed++;

    }

    cout << "\n** Test Blobber copy assignment (constructor)...\n";
    Blobber blob5 = blob1;
    if(pixelx != blob5[lines/2][samples/2]) {
      cout << "--> FAILED < Pixel[Line=" << (lines / 2)
                << "][Sample=" << (samples / 2)
                << "] = " << pixelx << "  != "
                << blob1[lines/2][samples/2]
                << " from Blobber blob = oblob constructor!!" << endl;
      nfailed++;
    }


//  Test deep copy
    cout << "\n** Test Blobber deep copy...\n";
    Blobber blob6;
    blob6 = blob1.deepcopy();
    if(pixelx != blob6[lines/2][samples/2]) {
      cout << "--> FAILED < Pixel[Line=" << (lines / 2)
                << "][Sample=" << (samples / 2)
                << "] = " << pixelx << "  != "
                << blob1[lines/2][samples/2]
                << " from deepcopy!!" << endl;
      nfailed++;
    }

//  Now change pixel in copied version
    blob6[lines/2][samples/2] += 1.0;
    if(blob1[lines/2][samples/2] == blob6[lines/2][samples/2]) {
      cout << "--> FAILED < Pixel[Line=" << (lines / 2)
                << "][Sample=" << (samples / 2)
                << "] = " << pixelx << "  == "
                << blob1[lines/2][samples/2]
                << " deepcopy failed to make separate copy!!" << endl;
      nfailed++;
    }
  }
  catch(Isis::IException &e) {
    e.print();
  }

  //  Return status
  return (nfailed);
}

