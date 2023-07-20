#include <QTextStream>
#include <QUuid>

#include "CubeAttribute.h"
#include "FileName.h"

#include "Blob.h"
#include "Brick.h"
#include "Fixtures.h"
#include "Portal.h"
#include "LineManager.h"
#include "SpecialPixel.h"
#include "TestUtilities.h"
#include "ControlNet.h"

namespace Isis {


  void TgoCassisModuleKernels::SetUp() {
    QVector<QString> ckKernels = {QString("data/tgoCassis/mapProjectedReingested/em16_tgo_cassis_tel_20160407_20221231_s20220316_v01_0_sliced_-143410.xc"),
                                  QString("data/tgoCassis/mapProjectedReingested/em16_tgo_cassis_tel_20160407_20221231_s20220316_v01_1_sliced_-143410.xc"),
                                  QString("data/tgoCassis/mapProjectedReingested/em16_tgo_sc_ssm_20180501_20180601_s20180321_v01_0_sliced_-143000.xc"),
                                  QString("data/tgoCassis/mapProjectedReingested/em16_tgo_sc_ssm_20180501_20180601_s20180321_v01_1_sliced_-143000.xc"),
                                  QString("data/tgoCassis/em16_tgo_cassis_tel_20160407_20221231_s20220402_v01_0_sliced_-143410.xc"),
                                  QString("data/tgoCassis/em16_tgo_cassis_tel_20160407_20221231_s20220402_v01_1_sliced_-143410.xc"),
                                  QString("data/tgoCassis/em16_tgo_sc_spm_20161101_20170301_s20191109_v01_0_sliced_-143000.xc"),
                                  QString("data/tgoCassis/em16_tgo_sc_spm_20161101_20170301_s20191109_v01_1_sliced_-143000.xc"),
                                  QString("data/tgoCassis/singleFrameletProj/em16_tgo_cassis_tel_20160407_20221231_s20220402_v01_0_sliced_-143410.xc"),
                                  QString("data/tgoCassis/singleFrameletProj/em16_tgo_cassis_tel_20160407_20221231_s20220402_v01_1_sliced_-143410.xc"),
                                  QString("data/tgoCassis/singleFrameletProj/em16_tgo_sc_spm_20161101_20170301_s20191109_v01_0_sliced_-143000.xc"),
                                  QString("data/tgoCassis/singleFrameletProj/em16_tgo_sc_spm_20161101_20170301_s20191109_v01_1_sliced_-143000.xc")};
    QVector<QString> tempCkKernels;
    QVector<QString> spkKernels = {QString("data/tgoCassis/mapProjectedReingested/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1_0.xsp"),
                                   QString("data/tgoCassis/mapProjectedReingested/CAS-M01-2018-05-05T23.11.48.767-RED-01029-B1_1.xsp"),
                                   QString("data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381_0.xsp"),
                                   QString("data/tgoCassis/CAS-MCO-2016-11-26T22.50.27.381_1.xsp"),
                                   QString("data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583_0.xsp"),
                                   QString("data/tgoCassis/singleFrameletProj/CAS-MCO-2016-11-26T22.58.02.583_1.xsp")};
    QVector<QString> tempSpkKernels;

    for (int i = 0; i < ckKernels.size(); i++) {
      QString kernelFile = ckKernels[i];
      QString kernelExtension = kernelFile.split('.').last();
      QString targetFile = kernelPrefix.path() + "/" + QString::number(i) + '.' + kernelExtension;
      QFile::copy(kernelFile, targetFile);
      tempCkKernels.append(targetFile);
    }

    for (int i = 0; i < spkKernels.size(); i++) {
      QString kernelFile = spkKernels[i];
      QString kernelExtension = kernelFile.split('.').last();
      QString targetFile = kernelPrefix.path() + "/" + QString::number(i) + '.' + kernelExtension;
      QFile::copy(kernelFile, targetFile);
      tempSpkKernels.append(targetFile);
    }

    // variables defined in TgoCassisModuleTests
    if (binaryCkKernels.size() == 0) {
      binaryCkKernels = generateBinaryKernels(tempCkKernels);
      binarySpkKernels = generateBinaryKernels(tempSpkKernels);

      binaryCkKernelsAsString = fileListToString(binaryCkKernels);
      binarySpkKernelsAsString = fileListToString(binarySpkKernels);
    }
  }


  void TgoCassisModuleKernels::TearDown() {
    binaryCkKernels = {};
    binarySpkKernels = {};
    binaryCkKernelsAsString = "";
    binarySpkKernelsAsString = "";
  }


  void HistoryBlob::SetUp() {
    TempTestingFiles::SetUp();

    std::istringstream hss(R"(
      Object = mroctx2isis
        IsisVersion       = "4.1.0  | 2020-07-01"
        ProgramVersion    = 2016-06-10
        ProgramPath       = /Users/acpaquette/repos/ISIS3/build/bin
        ExecutionDateTime = 2020-07-01T16:48:40
        HostName          = Unknown
        UserName          = acpaquette
        Description       = "Import an MRO CTX image as an Isis cube"

        Group = UserParameters
          FROM    = /Users/acpaquette/Desktop/J03_045994_1986_XN_18N282W.IMG
          TO      = /Users/acpaquette/Desktop/J03_045994_1986_XN_18N282W_isis.cub
          SUFFIX  = 18
          FILLGAP = true
        End_Group
      End_Object)");

    hss >> historyPvl;

    std::ostringstream ostr;
    ostr << historyPvl;
    std::string histStr = ostr.str();

    historyBlob = Blob("IsisCube", "History");
    historyBlob.setData(histStr.c_str(), histStr.size());
  }
}
