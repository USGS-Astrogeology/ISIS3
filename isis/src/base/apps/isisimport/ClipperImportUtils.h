#ifndef CLIPPER_H
#define CLIPPER_H

#include "iTime.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace std;

namespace Isis {
  
    void clipperEisPBCreateLineTable(Cube *cube) {
      TableField ephTimeField("EphemerisTime", TableField::Double);
      TableField expTimeField("ExposureTime", TableField::Double);
      TableField lineStartField("LineStart", TableField::Integer);

      TableRecord timesRecord;
      timesRecord += ephTimeField;
      timesRecord += expTimeField;
      timesRecord += lineStartField;

      Table timesTable("LineScanTimes", timesRecord);

      Pvl *inputCubeLabel = cube->label();

      PvlGroup instGroup;
      try {
        instGroup = inputCubeLabel->findGroup("Instrument", PvlObject::FindOptions::Traverse);
      }
      catch(IException &e) {
        QString msg = "Unable to find instrument group in [" + cube->fileName() + "]";
        throw IException(e, IException::User, msg, _FILEINFO_); 
      }

      QString startTime = QString(instGroup.findKeyword("StartTime"));
      iTime time(startTime);
      int lines = dimGroup.findKeyword("Lines");
      PvlKeyword exposureDuration = instGroup.findKeyword("ExposureDuration");
      double lineDuration = (double)exposureDuration;
      if (exposureDuration.unit() == "ms") {
        lineDuration /= 1000;
      }
      lineDuration /= cube->lineCount();

      timesRecord[0] = time.Et();
      timesRecord[1] = lineDuration;
      timesRecord[2] = 1;
      timesTable += timesRecord;

      cube->write(timesTable);
    }
  
  }

#endif