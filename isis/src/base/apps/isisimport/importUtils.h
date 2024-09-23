#ifndef IMPORT_UTILS_H
#define IMPORT_UTILS_H

#include <map>
#include <iostream>
#include <sstream>
#include <vector>

#include <QString>

#include "Buffer.h"
#include "EndianSwapper.h"
#include "Table.h"
#include "SpecialPixel.h"
#include "Stretch.h"

#include "CassiniImportUtils.h"
#include "ClipperImportUtils.h"

using namespace std;

std::map<std::string, int> processMap = {
  {"cassiniIssFixDnPostProcess", 1}
};

std::map<std::string, int> ancillaryProcessMap = {
  {"cassiniIssCreateLinePrefixTable", 1},
  {"cassiniIssFixLabel", 2},
  {"clipperEisPBCreateLineTable", 3}
};

namespace Isis {

  QString vectorToString(std::vector<double> v) {
    std::stringstream ss;
    ss << "(";

    size_t i = 0;
    for(; i < v.size(); i++)
    {
      if(i != 0)
        ss << ", ";
      ss << v[i];
    }
    ss << ")";
    return QString::fromStdString(ss.str());
  };

  void applyAncillaryProcess(Cube *cube,
                             QString processFunction,
                             PvlObject translation,
                             ProcessImport *process) {
    switch (ancillaryProcessMap[processFunction.toStdString()]) {
      case 1:
        return cassiniIssCreateLinePrefixTable(cube,
                                               translation,
                                               process);
      case 2:
        return cassiniIssFixLabel(cube, translation, process);

      case 3:
        return clipperEisPBCreateLineTable(cube);
    }

    throw IException(IException::Programmer, "Unable to find prefix/suffix function [" + processFunction.toStdString() + "]", _FILEINFO_);
  };

  void runProcess(QString ioFile, PvlObject process) {
    std::string functionString = QString(process["ProcessFunction"]).toStdString();
    switch (processMap[functionString]) {
      case 1:
        cassiniIssFixDnPostProcess(ioFile, process);
        return;
    }

    throw IException(IException::Programmer, "Unable to find functor [" + functionString + "]", _FILEINFO_);
  };
}

#endif
