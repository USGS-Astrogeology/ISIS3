#include <iostream>
#include <time.h>

#include <QRegExp>
#include <QString>
#include <nlohmann/json.hpp>

#include "CubeFixtures.h"
#include "md5wrapper.h"
#include "OriginalLabel.h"
#include "OriginalXmlLabel.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"

#include "isisexport.h"

#include "gmock/gmock.h"

using namespace Isis;
using json = nlohmann::json;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isisexport.xml").expanded();

TEST_F(SmallCube, FunctionalTestIsisexportMainLabel) {
  PvlGroup testGroup("TestGroup");
  PvlKeyword testKey("TestValue", "a");
  testGroup += testKey;
  testCube->putGroup(testGroup);

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{MainLabel.IsisCube.TestGroup.TestValue.Value}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ(testKey[0], line);
}

TEST_F(SmallCube, FunctionalTestIsisexportOriginalLabel) {
  Pvl testLabel;
  PvlKeyword testKey("TestValue", "a");
  testLabel += testKey;
  OriginalLabel testOrigLab(testLabel);
  testCube->write(testOrigLab);

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{OriginalLabel.TestValue.Value}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ(testKey[0], line);
}

TEST_F(SmallCube, FunctionalTestIsisexportNoOriginalLabel) {
  QString templateFile = tempDir.path()+"/bad_value.tpl";
  QString renderedFile = tempDir.path()+"/bad_value.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{OriginalLabel.TestValue.Value}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  EXPECT_ANY_THROW(isisexport(testCube, options)); }

TEST_F(SmallCube, FunctionalTestIsisexportOriginalXmlLabel) {

  QString labelFileName = tempDir.path()+"/originallabel.xml";
  std::ofstream ofxml;
  ofxml.open(labelFileName.toStdString());
  ofxml << R"(<Outside> <name>Something</name> </Outside>)";
  ofxml.close();
  OriginalXmlLabel origLabel;
  origLabel.readFromXmlFile(labelFileName);
  testCube->write(origLabel);

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{OriginalLabel.Outside.name}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ("Something", line);
}

TEST_F(SmallCube, FunctionalTestIsisexportExtraPvl) {
  QString pvlFile = tempDir.path()+"/extra.pvl";
  Pvl testPvl;
  PvlKeyword testKey("TestValue", "a");
  testPvl += testKey;
  testPvl.write(pvlFile.toStdString());

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{ExtraPvl.TestValue.Value}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile,
                           "to=" + renderedFile,
                           "extrapvl=" + pvlFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ(testKey[0], line);
}

TEST_F(SmallCube, FunctionalTestIsisexportMultipleExtraPvl) {
  QString pvlFile1 = tempDir.path()+"/extra1.pvl";
  Pvl testPvl1;
  PvlKeyword testKey1("TestValue", "a");
  PvlKeyword safeKey("SafeValue", "true");
  testPvl1 += testKey1;
  testPvl1 += safeKey;
  testPvl1.write(pvlFile1.toStdString());

  QString pvlfile2 = tempDir.path()+"/extra2.pvl";
  Pvl testPvl2;
  PvlKeyword duplicateKey("TestValue", "b");
  PvlKeyword testKey2("AnotherValue", "10");
  testPvl2 += duplicateKey;
  testPvl2 += testKey2;
  testPvl2.write(pvlfile2.toStdString());

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{ExtraPvl.TestValue.Value}}\n"
     << "{{ExtraPvl.AnotherValue.Value}}\n"
     << "{{ExtraPvl.SafeValue.Value}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile,
                           "to=" + renderedFile,
                           "extrapvl=(" + pvlFile1 + "," + pvlfile2 + ")"};
  UserInterface options(APP_XML, args);
  Pvl log;

  isisexport(testCube, options, &log);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ(duplicateKey[0], line);
  std::getline(renderedStream, line);
  EXPECT_EQ(testKey2[0], line);
  std::getline(renderedStream, line);
  EXPECT_EQ(safeKey[0], line);

  // The duplicate key should generate a warning
  EXPECT_TRUE(log.hasGroup("Warning"));
}

TEST_F(SmallCube, FunctionalTestIsisexportExtraJson) {
  QString jsonFile = tempDir.path()+"/extra.json";
  json testJson;
  testJson["TestValue"] = "a";
  std::ofstream jsonStream;
  jsonStream.open(jsonFile.toStdString());
  jsonStream << testJson.dump();
  jsonStream.close();

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{ExtraJson.TestValue}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile,
                           "to=" + renderedFile,
                           "extrajson=" + jsonFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ(testJson["TestValue"], line);
}

TEST_F(SmallCube, FunctionalTestIsisexportMultipleExtraJson) {
  QString jsonFile1 = tempDir.path()+"/extra1.json";
  json testJson1;
  testJson1["TestValue"] = "a";
  testJson1["SafeValue"] = "true";
  std::ofstream jsonStream1;
  jsonStream1.open(jsonFile1.toStdString());
  jsonStream1 << testJson1.dump();
  jsonStream1.close();

  QString jsonFile2 = tempDir.path()+"/extra2.json";
  json testJson2;
  testJson2["TestValue"] = "b";
  testJson2["AdditionalValue"] = "10";
  std::ofstream jsonStream2;
  jsonStream2.open(jsonFile2.toStdString());
  jsonStream2 << testJson2.dump();
  jsonStream2.close();

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{ExtraJson.TestValue}}\n"
     << "{{ExtraJson.AdditionalValue}}\n"
     << "{{ExtraJson.SafeValue}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile,
                           "to=" + renderedFile,
                           "extrajson=(" + jsonFile1 + "," + jsonFile2 + ")"};
  UserInterface options(APP_XML, args);
  Pvl log;

  isisexport(testCube, options, &log);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ(testJson2["TestValue"], line);
  std::getline(renderedStream, line);
  EXPECT_EQ(testJson2["AdditionalValue"], line);
  std::getline(renderedStream, line);
  EXPECT_EQ(testJson1["SafeValue"], line);

  // The duplicate key should generate a warning
  EXPECT_TRUE(log.hasGroup("Warning"));
}

TEST_F(SmallCube, FunctionalTestIsisexportExtraXml) {
  QString xmlFile = tempDir.path()+"/extra.xml";
  std::ofstream xmlStream;
  xmlStream.open(xmlFile.toStdString());
  xmlStream << "<TestValue>a</TestValue>";
  xmlStream.close();

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{ExtraXml.TestValue}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile,
                           "to=" + renderedFile,
                           "extraxml=" + xmlFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ("a", line);
}

TEST_F(SmallCube, FunctionalTestIsisexportMultipleExtraXml) {
  QString xmlFile1 = tempDir.path()+"/extra1.xml";
  std::ofstream xmlStream1;
  xmlStream1.open(xmlFile1.toStdString());
  xmlStream1 << "<TestValue>a</TestValue>";
  xmlStream1.close();

  QString xmlFile2 = tempDir.path()+"/extra2.xml";
  std::ofstream xmlStream2;
  xmlStream2.open(xmlFile2.toStdString());
  xmlStream2 << "<AdditionalValue>10</AdditionalValue>";
  xmlStream2.close();

  QString xmlFile3 = tempDir.path()+"/extra3.xml";
  std::ofstream xmlStream3;
  xmlStream3.open(xmlFile3.toStdString());
  xmlStream3 << "<TestValue>b</TestValue>";
  xmlStream3.close();

  QString templateFile = tempDir.path()+"/test_result.tpl";
  QString renderedFile = tempDir.path()+"/test_result.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{ExtraXml.TestValue}}\n"
     << "{{ExtraXml.AdditionalValue}}";
  of.close();

  QVector<QString> args = {"template=" + templateFile,
                           "to=" + renderedFile,
                           "extraxml=(" + xmlFile1 + "," + xmlFile2 + ", " + xmlFile3 + ")"};
  UserInterface options(APP_XML, args);
  Pvl log;

  isisexport(testCube, options, &log);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ("b", line);
  std::getline(renderedStream, line);
  EXPECT_EQ("10", line);

  // The duplicate key should generate a warning
  EXPECT_TRUE(log.hasGroup("Warning"));
}

TEST_F(SmallCube, FunctionalTestIsisexportCurrentTime) {
  QString templateFile = tempDir.path()+"/current_time.tpl";
  QString renderedFile = tempDir.path()+"/current_time.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{currentTime()}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  time_t startTime = time(NULL);
  struct tm *tmbuf = gmtime(&startTime);
  char yearstr[80];
  // Just check that the year is correct
  strftime(yearstr, 80, "%Y", tmbuf);
  EXPECT_EQ(yearstr, line.substr(0, 4));
  // Check that the rest is the correct format
  // YYYY-MM-DDTHH:MM:SS
  QRegExp timeRegex("\\d{4}-\\d{2}-\\d{2}T\\d{2}:\\d{2}:\\d{2}");
  EXPECT_TRUE(QString::fromStdString(line).contains(timeRegex)) << "String [" << line << "] "
                                                                << "does not match the time format "
                                                                << "[YYYY-MM-DDTHH:MM:SS].";
}

TEST_F(SmallCube, FunctionalTestIsisexportImageFileName) {
  QString templateFile = tempDir.path()+"/current_time.tpl";
  QString renderedFile = tempDir.path()+"/current_time.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{imageFileName()}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ("current_time.cub", line);
}

TEST_F(SmallCube, FunctionalTestIsisexportMD5Hash) {
  QString templateFile = tempDir.path()+"/current_time.tpl";
  QString renderedFile = tempDir.path()+"/current_time.txt";
  QString renderedCube = tempDir.path()+"/current_time.cub";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{md5Hash()}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  md5wrapper md5;
  EXPECT_EQ(md5.getHashFromFile(renderedCube).toStdString(), line);
}

TEST_F(SmallCube, FunctionalTestIsisexportOutputFileSize) {
  QString templateFile = tempDir.path()+"/file_size.tpl";
  QString renderedFile = tempDir.path()+"/file_size.txt";
  std::ofstream of;
  of.open(templateFile.toStdString());
  of << "{{outputFileSize()}}";
  of.close();
  QVector<QString> args = {"template=" + templateFile, "to=" + renderedFile};
  UserInterface options(APP_XML, args);

  isisexport(testCube, options);

  std::ifstream renderedStream;
  renderedStream.open(renderedFile.toStdString());
  std::string line;
  std::getline(renderedStream, line);
  EXPECT_EQ("69536", line);
}
