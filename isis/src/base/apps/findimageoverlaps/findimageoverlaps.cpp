#include "findimageoverlaps.h"

#include "Application.h"
#include "FileList.h"
#include "ImageOverlapSet.h"
#include "Pvl.h"
#include "SerialNumber.h"
#include "SerialNumberList.h"

using namespace std;

namespace Isis {

  void findimageoverlaps(UserInterface &ui, bool useThread, Pvl *log) {
    FileList images(ui.GetFileName("FROMLIST"));

    findimageoverlaps(images, ui, useThread, log);
  }

  void findimageoverlaps(FileList &images, UserInterface &ui, bool useThread, Pvl *log) {
    // list of sns/filenames sorted by serial number
    SerialNumberList serialNumbers(true);

    vector< pair<QString, QString> > sortedList;

    if (images.size() == 1) {
      throw IException(IException::User, "The list [" + ui.GetFileName("FROMLIST") +
                       "] only contains one image.", _FILEINFO_);
    }

    // We want to sort the input data by serial number so that the same
    //   results are produced every time this program is run with the same
    //   images. This is a modified insertion sort.
    for (int image = 0; image < images.size(); image++) {
      unsigned int insertPos = 0;
      QString sn = SerialNumber::Compose(images[image].toString());
      for (insertPos = 0; insertPos < sortedList.size(); insertPos++) {
        if (sn.compare(sortedList[insertPos].first) < 0) break;
      }
      pair<QString, QString> newPair = pair<QString, QString>(sn, images[image].toString());
      sortedList.insert(sortedList.begin() + insertPos, newPair);
    }

    // Add the serial numbers in sorted order now
    for (unsigned int i = 0; i < sortedList.size(); i++) {
      serialNumbers.add(sortedList[i].second);
    }

    // Now we want the ImageOverlapSet to calculate our overlaps
    ImageOverlapSet overlaps(true, useThread);

    // Use multi-threading to create the overlaps
    overlaps.FindImageOverlaps(serialNumbers,
                               FileName(ui.GetFileName("OVERLAPLIST")).expanded());

    // This will only occur when "CONTINUE" is true, so we can assume "ERRORS" was
    //   an entered parameter.
    if (overlaps.Errors().size() != 0 && ui.WasEntered("ERRORS")) {
      Pvl outFile;

      bool filenamesOnly = !ui.GetBoolean("DETAILED");

      vector<PvlGroup> errorList = overlaps.Errors();

      for (unsigned int err = 0; err < errorList.size(); err++) {
        if (!filenamesOnly) {
          outFile += errorList[err];
        }
        else if (errorList[err].hasKeyword("FileNames")) {
          PvlGroup origError = errorList[err];
          PvlGroup err("ImageOverlapError");

          for (int keyword = 0; keyword < origError.keywords(); keyword++) {
            if (origError[keyword].name() == "FileNames") {
              err += origError[keyword];
            }
          }

          outFile += err;
        }
      }

      outFile.write(FileName(ui.GetFileName("ERRORS")).expanded());
    }

    PvlGroup results("Results");
    results += PvlKeyword("ErrorCount", toString((BigInt)overlaps.Errors().size()));

    Application::Log(results);
  }
}
