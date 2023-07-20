#ifndef Fixtures_h
#define Fixtures_h

#include "gtest/gtest.h"

#include <QString>
#include <QTemporaryDir>
#include <QString>

#include "Blob.h"
#include "Pvl.h"
#include "PvlKeyword.h"

#include "TempFixtures.h"
#include "CameraFixtures.h"
#include "CsmFixtures.h"
#include "CubeFixtures.h"
#include "NetworkFixtures.h"

namespace Isis {


class TgoCassisModuleKernels : public ::testing::Test {

  protected:
    // You can define per-test set-up logic as usual.
    void SetUp() override;

    // You can define per-test tear-down logic as usual.
    void TearDown() override;

    QTemporaryDir kernelPrefix;

    QVector<QString> binaryCkKernels;
    QVector<QString> binarySpkKernels;

    QString binaryCkKernelsAsString;
    QString binarySpkKernelsAsString;
  };


class HistoryBlob : public TempTestingFiles {
  protected:
    Blob historyBlob;
    PvlObject historyPvl;

    void SetUp() override;
  };
}

#endif
