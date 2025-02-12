#include <QTemporaryFile>
#include <QString>
#include <iostream>

#include "Blob.h"

#include "TempFixtures.h"
#include "TiffFixtures.h"
#include "TestUtilities.h"

#include "gmock/gmock.h"

using namespace Isis;

class DefaultBlob : public TempTestingFiles {

  protected:
    Blob *testBlob;
    QString testBlobPath = tempDir.path() + "/junk_blob.pvl";

    void SetUp() override {
      TempTestingFiles::SetUp();

      testBlob = new Blob("UnitTest", "Blob");
      char buf[] = {"ABCD"};
      testBlob->setData(buf, 4);
      testBlob->Write(testBlobPath);
    }

    void TearDown() override {
      delete testBlob;
    }
  
};

TEST_F(TempTestingFiles, TestBlobDefault) {
  Blob b("UnitTest", "Blob");
  char buf[] = {"ABCD"};
  b.setData(buf, 4);
  b.Write("junk");

  EXPECT_EQ("UnitTest", b.Name());
  EXPECT_EQ(4, b.Size());
  EXPECT_EQ("Blob", b.Type());
}

TEST_F(DefaultBlob, TestBlobRead) {
  Blob b("UnitTest", "Blob", testBlobPath);
  
  EXPECT_EQ("UnitTest", b.Name());
  EXPECT_EQ(4, b.Size());
  EXPECT_EQ("Blob", b.Type());

  std::string buffStr(b.getBuffer());
  EXPECT_EQ("ABCD", buffStr);
}

TEST_F(DefaultBlob, TestBlobWriteExisting) {
  Isis::Pvl pvl(testBlobPath);
  std::fstream strm;
  strm.open(testBlobPath.toStdString(), std::ios::binary | std::ios::out);
  char buf[] = {"ABCD"};
  testBlob->setData(buf, 3);
  testBlob->Write(pvl, strm);
  strm.seekp(0, std::ios::beg);
  strm << pvl;
  EXPECT_EQ("UnitTest", testBlob->Name());
  EXPECT_EQ(3, testBlob->Size());
  EXPECT_EQ("Blob", testBlob->Type());
  strm.close();

  Blob b("UnitTest", "Blob", testBlobPath);

  EXPECT_EQ("UnitTest", b.Name());
  EXPECT_EQ(3, b.Size());
  EXPECT_EQ("Blob", b.Type());
}

TEST_F(DefaultBlob, TestBlobWriteExistingEOF) {
  Isis::Pvl pvl("junk");
  std::fstream strm;
  strm.open(testBlobPath.toStdString(), std::ios::binary | std::ios::out);
  char buf[] = {"ABCD"};
  testBlob->setData(buf, 4);
  testBlob->Write(pvl, strm);
  strm.seekp(0, std::ios::beg);
  strm << pvl;
  EXPECT_EQ("UnitTest", testBlob->Name());
  EXPECT_EQ(4, testBlob->Size());
  EXPECT_EQ("Blob", testBlob->Type());
  strm.close();

  Blob b("UnitTest", "Blob", testBlobPath);

  EXPECT_EQ("UnitTest", b.Name());
  EXPECT_EQ(4, b.Size());
  EXPECT_EQ("Blob", b.Type());
}

TEST_F(ReadWriteTiff, TestBlobWriteReadGdal) {
  createTiff(UnsignedByte, false);
  GDALDataset *dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_Update));

  Blob writeBlob("UnitTest", "Blob");
  char buf[] = {"ABCD"};
  writeBlob.setData(buf, 4);
  writeBlob.WriteGdal(dataset);

  GDALClose(dataset);
  dataset = GDALDataset::FromHandle(GDALOpen(path.toStdString().c_str(), GA_Update));

  Blob readBlob("UnitTest", "Blob");
  readBlob.ReadGdal(dataset);

  EXPECT_EQ(writeBlob.Name(), readBlob.Name());
  EXPECT_EQ(writeBlob.Size(), readBlob.Size());
  EXPECT_EQ(writeBlob.Type(), readBlob.Type());

  std::string writeBuff(writeBlob.getBuffer());
  std::string readBuff(readBlob.getBuffer());

  EXPECT_EQ(writeBuff, readBuff);

  GDALClose(dataset);
}