#include <QTextStream>
#include <QStringList>
#include <QTemporaryFile>
#include <QByteArray>
#include <QFile>
#include <QVector>
#include <QDomDocument>

#include "spiceserver.h"
#include "CameraFixtures.h"
#include "TempFixtures.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "TestUtilities.h"
#include "TextFile.h"

#include "gmock/gmock.h"

using namespace Isis;

static QString APP_XML = QString::fromStdString(FileName("$ISISROOT/bin/xml/spiceserver.xml").expanded());

class TestPayload : public DefaultCube {
    protected:

      QString asciiPayloadPath;
      QString hexPayloadPath;

      void SetUp() override {
        DefaultCube::SetUp();
        QByteArray prefix = R"(
            <input_label>
            <isis_version>
            352e312e312e3020616c706861207c20323031322d30352d3231
            </isis_version>
            <parameters>
              <cksmithed value='no' />
              <ckrecon value='yes' />
              <ckpredicted value='no' />
              <cknadir value='no' />
              <spksmithed value='no' />
              <spkrecon value='yes' />
              <spkpredicted value='no' />
              <shape value='system' />
              <startpad time='0.0' />
              <endpad time='0.0' />
            </parameters>
            <label>
         )";

        std::ostringstream labelStream;
        labelStream << *testCube->label();
        QByteArray labelBytes = labelStream.str().c_str();

        QByteArray suffix = R"(
            </label>
          </input_label>
        )";

        QByteArray asciiPayload = prefix+labelBytes.toHex()+suffix;

        QByteArray hexPayload = asciiPayload.toHex();

        asciiPayloadPath = tempDir.path() + "/asciiPayload.txt";
        hexPayloadPath = tempDir.path() + "/hexPayload.txt";

        QFile asciiFile(asciiPayloadPath);
        asciiFile.open(QIODevice::WriteOnly);
        asciiFile.write(asciiPayload);
        asciiFile.close();

        QFile hexFile(hexPayloadPath);
        hexFile.open(QIODevice::WriteOnly);
        hexFile.write(hexPayload);
        hexFile.close();
    }
};

TEST_F(TestPayload, FunctionalTestSpiceserverDefaultParameters) {
  QString outputFile = tempDir.path() + "/out.txt";

  QVector<QString> args = {"From="+hexPayloadPath, "To="+outputFile, "TEMPFILE="+tempDir.path()+"/temp.cub"};
  UserInterface options(APP_XML, args);
  Pvl appLog;

  spiceserver(options, &appLog);

  TextFile inFile(outputFile);
  QString hexCode;
  inFile.GetLine(hexCode);

  QString xml( QByteArray::fromHex( QByteArray( hexCode.toLatin1() ) ).constData() );

  QDomDocument document;
  QString error;
  Pvl kernelsLabel;
  Pvl instrumentPositionTable;

  // Use Qt's XML API to get elements we want to compare
  int errorLine, errorCol;
  if ( document.setContent(QString(xml), &error, &errorLine, &errorCol) ) {
    QDomElement rootElement = document.firstChild().toElement();
    for ( QDomNode node = rootElement.firstChild(); !node.isNull(); node = node.nextSibling() ) {

      QDomElement element = node.toElement();

      if (element.tagName() == "kernels_label") {
        QString encoded = element.firstChild().toText().data();
        std::stringstream labStream;
        labStream << QString( QByteArray::fromHex( encoded.toLatin1() ).constData() ).toStdString();
        labStream >> kernelsLabel;
      }
      else if (element.tagName() == "tables") {
        for ( QDomNode node = element.firstChild(); !node.isNull(); node = node.nextSibling() ) {
          QDomElement table = node.toElement();

          if (table.tagName() == "instrument_position") {
             QString encoded = table.firstChild().toText().data();
             std::stringstream labStream;
             labStream << QString( QByteArray::fromHex( encoded.toLatin1() ).constData() ).toStdString();
             labStream >> instrumentPositionTable;
          }
        }
      }
    }
  }
  else {
    FAIL() << "Unable to open output file, either doesn't exist for formatted incorrectly.";
  }

  PvlGroup naifKeywords = kernelsLabel.group(0);

  EXPECT_EQ((int)naifKeywords.findKeyword("NaifFrameCode"), -27002);
  EXPECT_PRED_FORMAT2(AssertStringsEqual, naifKeywords.findKeyword("TargetPosition")[0], "Table");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, naifKeywords.findKeyword("InstrumentPointing")[0], "Table");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, naifKeywords.findKeyword("InstrumentPosition")[0], "Table");

  PvlObject table = instrumentPositionTable.findObject("Table");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.findKeyword("Name")), "InstrumentPosition");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.group(0).findKeyword("Name")), "J2000X");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.group(1).findKeyword("Name")), "J2000Y");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.group(2).findKeyword("Name")), "J2000Z");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.group(3).findKeyword("Name")), "J2000XV");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.group(4).findKeyword("Name")), "J2000YV");
  EXPECT_PRED_FORMAT2(AssertStringsEqual, (table.group(5).findKeyword("Name")), "J2000ZV");
}


TEST_F(TempTestingFiles, FunctionalTestSpiceserverIsisVersion) {

  // Isis Version 3.4.1
  QByteArray badPayload = R"(
    <input_label>
      <isis_version>
    332e342e312e3020616c706861207c20323031322d30352d32310a
      </isis_version>
    </input_label>
  )";

  QString outputFile = tempDir.path() + "/out.txt";
  QString badPayloadPath = tempDir.path() + "/out.txt";

  QFile asciiFile(badPayloadPath);
  asciiFile.open(QIODevice::WriteOnly);
  asciiFile.write(badPayload.toHex());
  asciiFile.close();

  QVector<QString> args = {"From="+badPayloadPath, "To="+outputFile};
  UserInterface options(APP_XML, args);

  EXPECT_THROW({
    try{
      spiceserver(options);
      FAIL();
    }
    catch(const IException &e) {
      EXPECT_THAT(e.what(), testing::HasSubstr("The SPICE server only supports Isis versions greater than or equal to 3.5.*.*"));
      throw;
    }
  }, IException);
}
