/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "ProcessImportVicar.h"

#include <iostream>
#include <QString>
#include <sstream>

#include "Preference.h"
#include "IException.h"
#include "LineManager.h"
#include "Pvl.h"
#include "PixelType.h"
#include "SpecialPixel.h"
#include "IString.h"
#include "UserInterface.h"

using namespace std;

namespace Isis {
  /**
   * Opens a vicar file which can then be immediately imported by invoking the
   * inherited StartProcess method.
   *
   * @param vicarFile Name of the vicar file to open
   * @param vicarLab A PVL object which will contain the vicar labels.
   */
  void ProcessImportVicar::SetVicarFile(const QString &vicarFile, Pvl &vicarLab) {
    //  Open vicar file
    ifstream vicFile(vicarFile.toLatin1().data(), ios::in);

    if(!vicFile) {
      QString msg = "Cannot open vicar file [" + vicarFile + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    try {
      // get the starting VICAR label and convert to PVL
      IString vicLabels = ExtractPvlLabel(0, vicFile);

      // Fill temp Pvl label for ProcessImport startprocess
      stringstream lbl;
      lbl << vicLabels << " End" << endl;
      Pvl vLab;
      lbl >> vLab;
      vicarLab = vLab;

      //  Set the fileHeaderBytes
      SetFileHeaderBytes(vLab["LBLSIZE"]);

      //  Set the dataHeaderBytes
      SetDataHeaderBytes((int) vLab["NLB"] * (int)vLab["RECSIZE"]);

      //  Are there binary prefix bytes on each image line?
      SetDataPrefixBytes(vLab["NBB"]);
      SetDataSuffixBytes(0);

      SetDimensions(vLab["NS"], vLab["NL"], vLab["NB"]);

      QString pixType = QString::fromStdString(vLab["FORMAT"]);
      Isis::PixelType pixelType = None;
      if(pixType == "BYTE") pixelType = UnsignedByte;
      if(pixType == "WORD") pixelType = UnsignedWord;
      if(pixType == "HALF") pixelType = SignedWord;
      if(pixType == "REAL") pixelType = Real;
      if(pixelType == None) {
        QString msg = "Unsupported pixel type [FORMAT=" + pixType + "]";
        throw IException(IException::Io, msg, _FILEINFO_);
      }
      SetPixelType(pixelType);

      QString order = QString::fromStdString(vLab["INTFMT"]);
      if(order == "LOW") {
        SetByteOrder(Lsb);
      }
      else {
        SetByteOrder(Msb);
      }

      QString organization = QString::fromStdString(vLab["ORG"]);
      if(organization == "BSQ") {
        SetOrganization(ProcessImport::BSQ);
      }
      else if(organization == "BIL") {
        SetOrganization(ProcessImport::BIL);
      }
      else if(organization == "BIP") {
        SetOrganization(ProcessImport::BIP);
      }
      else {
        QString msg = "Unsupported file organization [" + organization + "]";
        throw IException(IException::Io, msg, _FILEINFO_);
      }

      // See if there is end-of-dataset labels
      // If so read them and merge
      if(vLab.hasKeyword("EOL")) {
        if((int) vLab["EOL"] == 1) {
          int startByte = (int) vLab["LBLSIZE"] +
                          (int) vLab["NLB"] * (int) vLab["RECSIZE"] +
                          (int) vLab["NL"] * (int) vLab["NB"] *
                          (int) vLab["RECSIZE"];
          ifstream vicFile(vicarFile.toLatin1().data(), ios::in);

          QString endPvlLabel = ExtractPvlLabel(startByte, vicFile);
          stringstream lbl;
          lbl << endPvlLabel;

          Pvl endLab;
          lbl >> endLab;
          vicFile.close();

          for(int k = 0; k < endLab.keywords(); k++) {
            vicarLab += endLab[k];
          }
        }
      }
    }
    catch(IException &e) {
      QString msg = "Input file [" + vicarFile + "] does not appear to be a vicar file";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    SetInputFile(vicarFile);
  }

  /**
   * Returns a valid PVL label based on the start position in the VICAR file.
   *
   * @param startPos the position to start reading the label
   * @param vicarFile the file stream to read from
   *
   * @return a valid PVL label
   */
  QString ProcessImportVicar::ExtractPvlLabel(int startPos, std::ifstream &vicarFile) const {
    vicarFile.seekg(startPos, ios::beg);

    // convert the LBLSIZE to an integer
    char *lblSizeValue = new char [1024];
    vicarFile.seekg(QString("LBLSIZE=").size(), ios_base::cur);

    for(int pos = 0; pos < 1024 - 1; pos++) {
      if(!vicarFile.good())
        break;

      if(vicarFile.peek() == ' ')
        break;

      lblSizeValue[pos] = vicarFile.get();
      lblSizeValue[pos + 1] = '\0';

      // we're totally lost at this point
      if(pos == 1023) {
        QString msg = "Cannot find label size in VICAR file";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    int lblSize = IString(lblSizeValue).ToInteger();
    delete [] lblSizeValue;
    lblSizeValue = NULL;

    char *buf = new char[lblSize+1];

    //  Read end vicar label
    vicarFile.seekg(startPos, ios::beg);
    vicarFile.read(buf, lblSize);
    buf[lblSize] = '\0';
    vicarFile.close();

    // Transform the vicar labels into valid pvl labels
    QString vicLabels = buf;

    bool inQuote = false;
    for(int pos = 0; pos < vicLabels.size(); pos++) {
      if(vicLabels[pos] == '\'' || vicLabels[pos] == '"') {
        inQuote = !inQuote;
      }

      if(!inQuote && vicLabels[pos] == ' ') {
        vicLabels[pos] = '\n';
      }
    }

    return vicLabels;
  }
} // end namespace Isis
