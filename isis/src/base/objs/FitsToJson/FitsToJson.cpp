/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "FitsToJson.h"

#include <nlohmann/json.hpp>
#include <QString>
#include <QList>

#include "IException.h"
#include "IString.h"
#include "PvlObject.h"
#include "PvlGroup.h"
#include "PvlToJSON.h"

using json = nlohmann::json;

namespace Isis {

  /**
   * Convert the contents of a stream to a JSON object.
   *
   * @param the stream to convert
   * @return @b json The contents of the stream as a JSON object
   *
   * This logic was taken from ProcessImportFits::extractFitsLabels
   */
  json fitsToJson(std::ifstream &fileStream) {
    PvlObject fitsPvl;
    QList<PvlGroup *> *fitsImageLabels = new QList< PvlGroup * >;
    QList<PvlGroup *> *extraFitsLabels = new QList< PvlGroup * >;
    QList<int> *headerSizes = new QList < int >;
    QList<int> *dataStarts = new QList < int >;

    // Process each FITS label area, storing each in its own PvlGroup.
    char readBuf[81];
    IString line = "";
    unsigned int place;

    // Start at the beginning of the file for the main FITS label
    fileStream.seekg(0, std::ios_base::beg);

    // Reads each label line (80 chars)
    while (fileStream.read(readBuf, 80) && fileStream.gcount() == 80) {

      PvlGroup *fitsLabel = new PvlGroup("FitsLabels");

      readBuf[80] = '\0';
      line = readBuf;
      place = 80;

      // Process each fits label record (80 bytes) and place keyword, value pairs into PvlKeywords
      // with any associated comments.
      while (line.substr(0, 3) != "END") {

        // Check for blank lines
        if (line.substr(0, 1) != " " && line.substr(0, 1) != "/") {
          // Name of keyword
          PvlKeyword label(line.Token(" =")); // Stop on spaces OR equal sign
          if (QString::compare(QString::fromStdString(label.name()), "OBJECT", Qt::CaseInsensitive) == 0) {
            label.setName("TARGET");
            label.addComment("NOTE: This keyword name was changed from 'OBJECT' in the original "
                             "fit header file.");
          }
          // Remove up to beginning of data
          line.TrimHead(" =");
          line.TrimTail(" ");
          if (label.name() == "COMMENT" || label.name() == "HISTORY") {
            label += line;
          }
          else {
            // Check for a quoted value
            if (line.substr(0,1) == "'") {
              line.TrimHead("'");
              label += line.Token("'").TrimHead(" ").TrimTail(" ");
              line.TrimHead(" '");
            }
            else {
              // Access any remaining data without the trailing comment if there is one
              IString value = line.Token("/");
              value.TrimTail(" ");
              label += value;
              line.TrimHead(" ");
            }
            // If the line still has anything in it, treat it is as a comment.
            if (line.size() > 2) {
              line.TrimHead(" /");
              label.addComment(line);
              if (line != line.Token("[")) {
                label.setUnits(line.Token("[").Token("]"));
              }
            }
          }
          fitsLabel->addKeyword(label);
        }

        fileStream.read(readBuf, 80);
        readBuf[80] = '\0';
        line = readBuf;
        place += 80;
      }

      // Save off the PvlGroup and the number of records read from this label
      fitsPvl.addGroup(*fitsLabel);
      headerSizes->append((int)ceil(place / 2880.0));

      // Move the file pointer past the padding after the "END" (i.e., points to start of data)
      std::streamoff jump = 0;
      jump = headerSizes->last() * 2880 - place;
      fileStream.seekg(jump, std::ios_base::cur);

      dataStarts->append(fileStream.tellg());

      if (fitsLabel->hasKeyword("BITPIX") && fitsLabel->hasKeyword("NAXIS")) {

        if((int)fitsLabel->findKeyword("NAXIS") > 0) {
          int bytesPerPixel = 0;
          bytesPerPixel = (int)((*fitsLabel)["BITPIX"]);
          bytesPerPixel = std::abs(bytesPerPixel);
          bytesPerPixel /= 8;

          unsigned int axis1 = 1;
          axis1 = Isis::toInt((*fitsLabel)["NAXIS1"]);

          unsigned int axis2 = 1;
          if (fitsLabel->hasKeyword("NAXIS2")) {
            axis2 = Isis::toInt((*fitsLabel)["NAXIS2"]);
          }

          unsigned int axis3 = 1;
          if (fitsLabel->hasKeyword("NAXIS3")) {
            axis3 = Isis::toInt((*fitsLabel)["NAXIS3"]);
          }

          jump = (int)(ceil(bytesPerPixel * axis1 * axis2 * axis3 / 2880.0) * 2880.0);
          fileStream.seekg(jump, std::ios_base::cur);
        }
        else {
          // Note: this will allow us to read extra label sections that have 0 axes,
          // but has image-related info (so BITPIX and NAXIS keywords exist). This
          // includes informational labels, as seen at the beginning of hayabusa2
          // images in this case, there is NO DATA, so no jump should be needed to
          // get to the next section.
          PvlGroup *extraLabelGroup = fitsImageLabels->last();
          extraLabelGroup->setName("FitsExtras");
          extraFitsLabels->append(extraLabelGroup);

          fitsImageLabels->removeLast();
          headerSizes->removeLast();
          dataStarts->removeLast();

          fitsPvl.addGroup(*extraLabelGroup);
        }
      }
      else if (fitsImageLabels->size() > 1) {
        fitsImageLabels->removeLast();
        headerSizes->removeLast();
        dataStarts->removeLast();
        break;
      }

      else {
        std::string msg = "The FITS file does not contain a section header that appears "
                                  "to describe an image.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    return pvlObjectToJSON(fitsPvl);
  }


  /**
   * Converts a FITS file to a json object.
   *
   * @param fitsFile Path to a FITS file.
   *
   * @return json The FITS file converted to a json object.
   */
  json fitsToJson(FileName fitsFile) {
    std::ifstream fileStream;
    try {
      fileStream.open(fitsFile.expanded().c_str(), std::ios::in  | std::ios::binary);
    }
    catch (IException &e) {
      std::string msg = "Unable to open FITS formatted file " + fitsFile.toString() + ".";
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    return fitsToJson(fileStream);
  }

}
