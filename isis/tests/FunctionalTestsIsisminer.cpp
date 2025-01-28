#include "isisminer.h"

#include "camstats.h"
#include "footprintinit.h"

#include "ControlNet.h"
#include "CSVReader.h"
#include "SqlQuery.h"

#include "TempFixtures.h"
#include "TestUtilities.h"

#include "gtest/gtest.h"

using namespace std;
using namespace Isis;

static QString APP_XML = FileName("$ISISROOT/bin/xml/isisminer.xml").expanded();
static QString APP_XML2 = FileName("$ISISROOT/bin/xml/camstats.xml").expanded();
static QString APP_XML3 = FileName("$ISISROOT/bin/xml/footprintinit.xml").expanded();

/**
   * IsisminerResourceManager
   * 
   * Class supporting IsisminerResourceManager tests.
   */
class IsisminerResourceManager : public TempTestingFiles {
  protected:

    void SetUp() override {
      TempTestingFiles::SetUp();

      // create input resourceManager_data.csv file
      ofstream of;
      of.open(tempDir.path().toStdString() + "/resourceManager_data.csv");
      of << "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationType,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214\n";
      of << "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.0458720107,-35.3972876604,Albedo,28.9490701558,64.4678332012,354.1080763404\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214\n";
      of << "2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839\n";
      of << "2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521\n";
      of << "2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.1875329109,-37.5388662682,Albedo,36.9166983086,77.264668712,334.8882754124\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214\n";
      of.close();
    }
};


/**
   * IsisminerMainProgram
   * 
   * Class supporting IsisminerMainProgram tests.
   */
class IsisminerMainProgram : public TempTestingFiles {
  protected:

    void SetUp() override {
      TempTestingFiles::SetUp();

      // create input file filter_data.csv
      ofstream of;
      of.open(tempDir.path().toStdString() + "/filter_data.csv");
      of << "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923\n";
      of << "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.68035312577266,0.71964687422734,0.94860127782928,0.15419616651216,0.56695663138626,1.6495650529206,74.978806104249,2.2364977040852,2.4816253207025,4.71812\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404\n";
      of << "2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.69201127796963,71.559210542126,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8798242820366,3.29886003388,6.17868\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.66812642316293,0.73187357683707,0.94482398919458,0.16552803241626,0.56943160918339,1.6458525862249,73.726207850557,2.2364977040852,2.4680963072552,4.70459\n";
      of << "2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.76397564900061,33.478295430042,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.8798242820366,3.3418923168616,6.22172\n";
      of << "2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.187532910945,-37.538866268171,2498082,Albedo,2012-09-01T04:53:53.9892759,399747301.17189,27,244.36392774954,244.64037529875,40.357451261299,36.91669830859,77.264668712026,334.88827541235,156.53894571568,0.80972363775171,63.31934535738,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.7393690885783,3.3233125567068,6.06268\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.34367888961712,95.942477726802,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.2364977040852,2.7206869531711,4.95718\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.74969392079315,32.478574455521,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.897658649418,3.2632828397482,6.16094\n";
      of << "2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.88243478873011,41.770435211108,0.91883649181428,0.48116350818572,0.9569036943252,0.1292889170244,0.9306297245104,1.1040554132344,112.14122128604,2.2364977040852,3.68880469938,5.9253\n";
      of << "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.7393496194796,31.754473363572,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.897658649418,3.1163576830336,6.01402\n";
      of.close();

      // create input config file mainProgram_test.conf in tempDir
      Pvl conf;
      PvlObject isisminerObject("IsisMiner");
      isisminerObject.addKeyword(PvlKeyword("Name", "FilterStrategyTest"));
      isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "inputdir"));
      
      /////////////////////////////////////////////////////////////////
      // add CsvReader strategy
      PvlObject CsvReader("Strategy");
      CsvReader.addKeyword(PvlKeyword("Name", "ReadFilterData"));
      CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
      CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/filter_data.csv\""));
      CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
      CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
      CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
      CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
      CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
      CsvReader.addKeyword(PvlKeyword("Identity", "\"input_output\""));
      CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId,StereoSource)"));
      isisminerObject.addObject(CsvReader);

      conf.addObject(isisminerObject);
      conf.write(tempDir.path() + "/mainProgram_test.conf");
    }
};


/**
   * IsisminerTestAssetSideBar
   * 
   * Tests sidebar behavior for created and non-existent assets
   * 
   * INPUT: 1) assetsidebar_assetdata.csv
   *        2) assetsidebar_resourcedata.csv
   *        3) assetsidebar_test.conf
   * 
   * OUTPUT: 1) assetsidebar_test_createdasset.csv
   *         2) assetsidebar_test_nonexistingasset.csv
   */
TEST(Isisminer, IsisminerTestAssetSideBar) {
  QTemporaryDir tempDir;

  // create input file assetsidebar_assetdata.csv
  ofstream of1;
  of1.open(tempDir.path().toStdString() + "/assetsidebar_assetdata.csv");
  of1 << "ANIMAL, COLOR\npanda, black\ncat, orange";
  of1.close();

  // create resource data file assetsidebar_resourcedata.csv
  ofstream of2;
  of2.open(tempDir.path().toStdString()+"/assetsidebar_resourcedata.csv");
  of2 << "NAME, AGE, HEIGHT\nJohn, 20, 68\nAmy, 30, 64\nBob, 25, 67";
  of2.close();

  // create input isisminer config file assetsidebar_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "AssetSidebarTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
  /////////////////////////////////////////////////////////////////
  // add CsvReader strategy
  PvlObject strategyCsvReaderTest("Strategy");
  strategyCsvReaderTest.addKeyword(PvlKeyword("Name", "CsvReaderTest"));
  strategyCsvReaderTest.addKeyword(PvlKeyword("Type", "CsvReader"));
  strategyCsvReaderTest.addKeyword(PvlKeyword("CsvFile", "\"%1/assetsidebar_resourcedata.csv\""));
  strategyCsvReaderTest.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
  strategyCsvReaderTest.addKeyword(PvlKeyword("HasHeader", "True"));
  strategyCsvReaderTest.addKeyword(PvlKeyword("SkipLines", "0"));
  strategyCsvReaderTest.addKeyword(PvlKeyword("IgnoreComments", "False"));
  strategyCsvReaderTest.addKeyword(PvlKeyword("Delimiter", ","));
  strategyCsvReaderTest.addKeyword(PvlKeyword("Identity", "\"%1\""));
  strategyCsvReaderTest.addKeyword(PvlKeyword("IdentityArgs", "\"Name\""));
  
  isisminerObject.addObject(strategyCsvReaderTest);

  /////////////////////////////////////////////////////////////////
  // add TestCreateAsset strategy
  PvlObject strategyObjectTestCreateAsset("Strategy");
  strategyObjectTestCreateAsset.addKeyword(PvlKeyword("Name", "TestCreateAsset"));
  strategyObjectTestCreateAsset.addKeyword(PvlKeyword("Type", "AssetSidebar"));
  strategyObjectTestCreateAsset.addKeyword(PvlKeyword("Asset", "CreatedAsset"));
  strategyObjectTestCreateAsset.addKeyword(PvlKeyword("Operation", "create"));
  strategyObjectTestCreateAsset.addKeyword(PvlKeyword("Description", "Adds a created asset to the loaded resources"));
  
  PvlObject isisminerSubObject1("IsisMiner");
  PvlObject strategyReadAssetTest("Strategy");
  strategyReadAssetTest.addKeyword(PvlKeyword("Name", "ReadAssetTest"));
  strategyReadAssetTest.addKeyword(PvlKeyword("Type", "CsvReader"));
  strategyReadAssetTest.addKeyword(PvlKeyword("CsvFile", "\"%1/assetsidebar_assetdata.csv\""));
  strategyReadAssetTest.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
  strategyReadAssetTest.addKeyword(PvlKeyword("HasHeader", "True"));
  strategyReadAssetTest.addKeyword(PvlKeyword("SkipLines", "0"));
  strategyReadAssetTest.addKeyword(PvlKeyword("IgnoreComments", "False"));
  strategyReadAssetTest.addKeyword(PvlKeyword("Delimiter", ","));
  strategyReadAssetTest.addKeyword(PvlKeyword("Identity", "\"%1_%2\""));
  strategyReadAssetTest.addKeyword(PvlKeyword("IdentityArgs", "(Animal, Color)"));

  isisminerSubObject1.addObject(strategyReadAssetTest);
  strategyObjectTestCreateAsset.addObject(isisminerSubObject1);
  isisminerObject.addObject(strategyObjectTestCreateAsset);

  /////////////////////////////////////////////////////////////////
  // add TestCreateAsset strategy
  PvlObject strategyObjectTestWriteExistingAsset("Strategy");
  strategyObjectTestWriteExistingAsset.addKeyword(PvlKeyword("Name", "TestWriteExistingAsset"));
  strategyObjectTestWriteExistingAsset.addKeyword(PvlKeyword("Type", "AssetSidebar"));
  strategyObjectTestWriteExistingAsset.addKeyword(PvlKeyword("Asset", "CreatedAsset"));
  strategyObjectTestWriteExistingAsset.addKeyword(PvlKeyword("Operation", "append"));
  strategyObjectTestWriteExistingAsset.addKeyword(PvlKeyword("Description", "Write the created asset"));
  
  PvlObject isisminerSubObject2("IsisMiner");
  PvlObject strategyInternalWriteTest("Strategy");
  strategyInternalWriteTest.addKeyword(PvlKeyword("Name", "InternalWriteTest"));
  strategyInternalWriteTest.addKeyword(PvlKeyword("Type", "CsvWriter"));
  strategyInternalWriteTest.addKeyword(PvlKeyword("CsvFile", "\"%1/assetsidebar_test_createdasset.csv\""));
  strategyInternalWriteTest.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
  strategyInternalWriteTest.addKeyword(PvlKeyword("Mode", "Create"));
  strategyInternalWriteTest.addKeyword(PvlKeyword("Header", "True"));
  strategyInternalWriteTest.addKeyword(PvlKeyword("Keywords", "(Animal, Color)"));
  strategyInternalWriteTest.addKeyword(PvlKeyword("Delimiter", ","));
  strategyInternalWriteTest.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));

  isisminerSubObject2.addObject(strategyInternalWriteTest);
  strategyObjectTestWriteExistingAsset.addObject(isisminerSubObject2);
  isisminerObject.addObject(strategyObjectTestWriteExistingAsset);

  /////////////////////////////////////////////////////////////////
  // add TestNonExistingAsset strategy
  PvlObject strategyObjectTestNonExistingAsset("Strategy");
  strategyObjectTestNonExistingAsset.addKeyword(PvlKeyword("Name", "TestNonExistingAsset"));
  strategyObjectTestNonExistingAsset.addKeyword(PvlKeyword("Type", "AssetSidebar"));
  strategyObjectTestNonExistingAsset.addKeyword(PvlKeyword("Asset", "Some Nonexisting Asset"));
  strategyObjectTestNonExistingAsset.addKeyword(PvlKeyword("Description", "Tries to write an asset that was never created"));
  
  PvlObject isisminerSubObject3("IsisMiner");
  PvlObject strategyWriteNonExistingAssetTest("Strategy");
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("Name", "WriteNonExistingAssetTest"));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("Type", "CsvWriter"));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("CsvFile", "\"%1/assetsidebar_test_nonexistingasset.csv\""));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("HasHeader", "True"));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("SkipLines", "0"));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("IgnoreComments", "False"));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("Delimiter", ","));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("Identity", "\"%1_%2\""));
  strategyWriteNonExistingAssetTest.addKeyword(PvlKeyword("IdentityArgs", "(Animal, Color)"));

  isisminerSubObject3.addObject(strategyWriteNonExistingAssetTest);
  strategyObjectTestNonExistingAsset.addObject(isisminerSubObject3);
  isisminerObject.addObject(strategyObjectTestNonExistingAsset);
  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/assetsidebar_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + configFilename,
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }
  
  /////////////////////////////////////////////////////////////////
  // Validate output assetsidebar_test_createdasset.csv file
  CSVReader::CSVAxis csvLine;
  
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/assetsidebar_test_createdasset.csv",
                             false, 0, ',', true, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 6);

  // Verify each line
  compareCsvLine(line.getRow(0), "Animal,Color");
  compareCsvLine(line.getRow(1), "NULL,NULL");
  compareCsvLine(line.getRow(2), "NULL,NULL");
  compareCsvLine(line.getRow(3), "NULL,NULL");
  compareCsvLine(line.getRow(4), "panda, black");
  compareCsvLine(line.getRow(5), "cat, orange");

  /////////////////////////////////////////////////////////////////
  // Validate output assetsidebar_test_nonexistingasset.csv file
  // Should be empty
  
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/assetsidebar_test_nonexistingasset.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 0);
  ASSERT_EQ(line.rows(), 0);
}


/**
   * IsisminerTestCalculator
   * 
   * Tests sidebar behavior for created and non-existent assets
   * 
   * INPUT: 1) calculator_data.csv
   *        2) calculator_test.conf
   * 
   * OUTPUT: 1) calculator_test.csv
   */
TEST(Isisminer, IsisminerTestCalculator) {
  QTemporaryDir tempDir;

  // create input file calculator_data.csv
  ofstream of1;
  of1.open(tempDir.path().toStdString() + "/calculator_data.csv");
  of1 << "x, y\n12.5, 4\n-4, 1\n# The following resource should be discarded\nFoo, Bar\n";
  of1.close();

  // create input isisminer config file calculator_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "CalculatorStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "( inputdir, outputdir )"));
  
    /////////////////////////////////////////////////////////////////
    // add ReadCalculatorData strategy
    PvlObject ReadCalculatorData("Strategy");
    ReadCalculatorData.addKeyword(PvlKeyword("Name", "ReadCalculatorData"));
    ReadCalculatorData.addKeyword(PvlKeyword("Type", "CsvReader"));
    ReadCalculatorData.addKeyword(PvlKeyword("CsvFile", "\"%1/calculator_data.csv\""));
    ReadCalculatorData.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    ReadCalculatorData.addKeyword(PvlKeyword("HasHeader", "True"));
    ReadCalculatorData.addKeyword(PvlKeyword("SkipLines", "0"));
    ReadCalculatorData.addKeyword(PvlKeyword("IgnoreComments", "True"));
    ReadCalculatorData.addKeyword(PvlKeyword("Delimiter", ","));
    isisminerObject.addObject(ReadCalculatorData);

    /////////////////////////////////////////////////////////////////
    // add TestNewFunctions strategy
    PvlObject strategyTestNewFunctions("Strategy");
    strategyTestNewFunctions.addKeyword(PvlKeyword("Type", "Calculator"));
    strategyTestNewFunctions.addKeyword(PvlKeyword("Name", "TestNewFunctions"));
    strategyTestNewFunctions.addKeyword(PvlKeyword("Description", "Test newly implemented functions"));
  
      PvlGroup initializersGroup("Initializers");
      initializersGroup.addKeyword(PvlKeyword("\"123\"", "0"));
      initializersGroup.addKeyword(PvlKeyword("\"var_x\"", "0"));
      initializersGroup.addKeyword(PvlKeyword("\"x%y\"", "0"));
      initializersGroup.addKeyword(PvlKeyword("\"pi\"", "0"));
      initializersGroup.addKeyword(PvlKeyword("\"degs(x)\"", "0"));
      initializersGroup.addKeyword(PvlKeyword("\"rads(x)\"", "0"));
      initializersGroup.addKeyword(PvlKeyword("\"e\"", "0"));
      strategyTestNewFunctions.addGroup(initializersGroup);

      PvlGroup equationsGroup("Equations");
      equationsGroup.addKeyword(PvlKeyword("\"123\"", "\"123\""));
      equationsGroup.addKeyword(PvlKeyword("\"var_x\"", "\"x\"")); // var_x is used instead of x to avoid keyword conflicts
      equationsGroup.addKeyword(PvlKeyword("\"x%y\"", "\"x%y\""));
      equationsGroup.addKeyword(PvlKeyword("\"pi\"", "\"pi\""));
      equationsGroup.addKeyword(PvlKeyword("\"degs(x)\"", "degs(x)"));
      equationsGroup.addKeyword(PvlKeyword("\"rads(x)\"", "rads(x)"));
      equationsGroup.addKeyword(PvlKeyword("\"e\"", "\"e\""));
      strategyTestNewFunctions.addGroup(equationsGroup);

    isisminerObject.addObject(strategyTestNewFunctions);

    /////////////////////////////////////////////////////////////////
    // add TestOrderOfOperations strategy
    PvlObject strategyTestOrderOfOperations("Strategy");
    strategyTestOrderOfOperations.addKeyword(PvlKeyword("Type", "Calculator"));
    strategyTestOrderOfOperations.addKeyword(PvlKeyword("Name", "TestOrderOfOperations"));
    strategyTestOrderOfOperations.addKeyword(PvlKeyword("Description", "Test order of operations"));
    strategyTestOrderOfOperations.addKeyword(PvlKeyword("Equation", "\"x-4/2^3\"")); // Simplifies to x-0.5
    strategyTestOrderOfOperations.addKeyword(PvlKeyword("Result", "\"x-4/2^3\""));
    strategyTestOrderOfOperations.addKeyword(PvlKeyword("Debug", "true"));

      PvlGroup initializersGroup1("Initializers");
      initializersGroup1.addKeyword(PvlKeyword("\"x-4/2^3\"", "0"));
      strategyTestOrderOfOperations.addGroup(initializersGroup1);

    isisminerObject.addObject(strategyTestOrderOfOperations);

    /////////////////////////////////////////////////////////////////
    // add TestEquationExecutionOrder strategy
    PvlObject strategyTestEquationExecutionOrder("Strategy");
    strategyTestEquationExecutionOrder.addKeyword(PvlKeyword("Type", "Calculator"));
    strategyTestEquationExecutionOrder.addKeyword(PvlKeyword("Name", "TestEquationExecutionOrder"));
    strategyTestEquationExecutionOrder.addKeyword(PvlKeyword("Description", "Test the correct order of equation execution."));
    strategyTestEquationExecutionOrder.addKeyword(PvlKeyword("Equation", "\"Accumulator*2\""));
    strategyTestEquationExecutionOrder.addKeyword(PvlKeyword("Result", "\"Accumulator\""));

      PvlGroup initializersGroup2("Initializers");
      initializersGroup2.addKeyword(PvlKeyword("\"Accumulator\"", "3")); // Becomes 38 if equation order is correct
      strategyTestEquationExecutionOrder.addGroup(initializersGroup2);

      PvlGroup equationsGroup2("Equations");
      equationsGroup2.addKeyword(PvlKeyword("\"Accumulator\"", "\"Accumulator^2\""));
      equationsGroup2.addKeyword(PvlKeyword("\"Accumulator\"", "\"Accumulator+2\""));
      strategyTestEquationExecutionOrder.addGroup(equationsGroup2);

    isisminerObject.addObject(strategyTestEquationExecutionOrder);

    /////////////////////////////////////////////////////////////////
    // add WriteTestInclude strategy
    PvlObject strategyWriteTestInclude("Strategy");
    strategyWriteTestInclude.addKeyword(PvlKeyword("Name", "WriteTestInclude"));
    strategyWriteTestInclude.addKeyword(PvlKeyword("Type", "CsvWriter"));
    strategyWriteTestInclude.addKeyword(PvlKeyword("CsvFile", "\"%1/calculator_test.csv\""));
    strategyWriteTestInclude.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    strategyWriteTestInclude.addKeyword(PvlKeyword("Mode", "Create"));
    strategyWriteTestInclude.addKeyword(PvlKeyword("Header", "True"));
    strategyWriteTestInclude.addKeyword(PvlKeyword("Delimiter", ","));
    strategyWriteTestInclude.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));

    QStringList keywordList = {"x", "y", "\"123\"", "\"var_x\"", "\"x%y\"", "\"pi\"", "degs(x)", "rads(x)", "\"e\"", "\"x-4/2^3\"", "\"Accumulator\""};
    PvlKeyword kw("Keywords");
    for (auto const &v : keywordList) {
      kw.addValue(v);
    }

    strategyWriteTestInclude.addKeyword(kw);

    isisminerObject.addObject(strategyWriteTestInclude);
  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/calculator_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + configFilename,
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv file
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // Validate output file calculator_test.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/calculator_test.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 11);
  ASSERT_EQ(line.rows(), 3);

  // Verify each line
  compareCsvLine(line.getRow(0), "x,y,123,var_x,x%y,pi,degs(x),rads(x),e,x-4/2^3,Accumulator");
  compareCsvLine(line.getRow(1), "12.5,4,123,12.5,0.5,3.14159,716.197,0.218166,2.71828,12,38");
  compareCsvLine(line.getRow(2), "-4,1,123,-4,0,3.14159,-229.183,-0.0698132,2.71828,-4.5,38");
}


/**
   * IsisminerTestCnetReader
   * 
   * Tests 
   * 
   * INPUT: 1) Alph_VIS.net (net and pvl files are stored in isis/tests/data/isisminer/cnetreader)
   *        2) Alph_VIS.pvl
   *        3) cnetreader_test.conf
   * 
   * OUTPUT: 1) cnetreader_no_id.csv     NOTE: these two files are identical, must be wrong
   *         2) cnetreader_with_id.csv
   */
TEST(Isisminer, IsisminerTestCnetReader) {
  QTemporaryDir tempDir;

  // create input Pvl file cnetreader_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "CnetReaderStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    ////////////////////////////////////////////////////
    // add CnetReader strategy
    PvlObject CnetReader("Strategy");
    CnetReader.addKeyword(PvlKeyword("Name", "TestWithIdentity"));
    CnetReader.addKeyword(PvlKeyword("Type", "CnetReader"));
    CnetReader.addKeyword(PvlKeyword("CnetFile", "\"%1/Alph_VIS.net\""));
    CnetReader.addKeyword(PvlKeyword("CnetFileArgs", "\"inputdir\""));
    CnetReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    CnetReader.addKeyword(PvlKeyword("IdentityArgs", "(PointId)"));
    CnetReader.addKeyword(PvlKeyword("Description", "Test the default functionality of CnetReader"));
    isisminerObject.addObject(CnetReader);

    ////////////////////////////////////////////////////
    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest1"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/cnetreader_with_id.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(ChooserName, Created, DateTime,"
                                    "Description, LastModified, Line, MeasureType,"
                                    "NetworkId, PointId, PointType, Reference, Sample,"
                                    "SerialNumber, TargetName, UserName, Version)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
    isisminerObject.addObject(CsvWriter);

    ////////////////////////////////////////////////////
    // reset for next test
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for multi-Include test"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"));
    isisminerObject.addObject(ResourceManager);

    ////////////////////////////////////////////////////
    // add 2nd CnetReader strategy
    CnetReader.addKeyword(PvlKeyword("Name", "TestNoIdentity"), Pvl::Replace);
    CnetReader.addKeyword(PvlKeyword("Description", "Test CnetReader without providing an identity value for the resource list"), Pvl::Replace);
    CnetReader.addKeyword(PvlKeyword("CnetFile", "\"%1/Alph_VIS.pvl\""), Pvl::Replace);
    CnetReader.deleteKeyword("Identity");
    CnetReader.deleteKeyword("IdentityArgs");
    isisminerObject.addObject(CnetReader);

    ////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest2"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/cnetreader_no_id.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/cnetreader_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + configFilename,
                           "parameters=inputdir:data/isisminer/cnetreader/@outputdir:"
                                       + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // Validate output file cnetreader_no_id.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/cnetreader_no_id.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 16);
  ASSERT_EQ(line.rows(), 55);

  // Verify each line
  compareCsvLine(line.getRow(0), "ChooserName,Created,DateTime,Description,LastModified,Line,MeasureType,NetworkId,PointId,PointType,Reference,Sample,SerialNumber,TargetName,UserName,Version");
  compareCsvLine(line.getRow(1), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(2), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(3), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(4), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(5), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(6), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(7), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(8), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(9), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(10), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(11), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(12), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(13), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(14), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(15), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(16), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(17), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(18), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(19), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(20), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(21), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(22), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(23), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(24), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(25), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(26), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(27), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(28), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(29), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(30), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(31), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(32), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(33), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(34), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(35), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(36), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(37), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(38), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(39), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(40), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(41), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(42), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(43), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(44), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(45), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(46), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(47), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(48), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(49), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(50), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(51), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(52), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(53), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(54), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");

  /////////////////////////////////////////////////////////////////
  // Validate output file cnetreader_with_id.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/cnetreader_with_id.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 16);
  ASSERT_EQ(line.rows(), 55);

  // Verify each line
  compareCsvLine(line.getRow(0), "ChooserName,Created,DateTime,Description,LastModified,Line,MeasureType,NetworkId,PointId,PointType,Reference,Sample,SerialNumber,TargetName,UserName,Version");
  compareCsvLine(line.getRow(1), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(2), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(3), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(4), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(5), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(6), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(7), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(8), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(9), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:41,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S127E3536_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:20:57.148489,Mars,lweller,5");
  compareCsvLine(line.getRow(10), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(11), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(12), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(13), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(14), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(15), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(16), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(17), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(18), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S120E3541_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:51.163454,Mars,lweller,5");
  compareCsvLine(line.getRow(19), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(20), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(21), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(22), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(23), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(24), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(25), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(26), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(27), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_04195S125E3541_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-09-17T02:04:39.931460,Mars,lweller,5");
  compareCsvLine(line.getRow(28), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(29), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(30), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(31), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(32), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(33), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(34), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(35), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(36), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S119E3542_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:30:52.587641,Mars,lweller,5");
  compareCsvLine(line.getRow(37), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(38), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(39), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(40), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(41), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(42), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(43), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(44), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(45), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_02362S125E3542_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-04-19T17:31:03.819694,Mars,lweller,5");
  compareCsvLine(line.getRow(46), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_UL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(47), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_UR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(48), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_LR,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(49), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_LL,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(50), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_CTR,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(51), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,75.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_1,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(52), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_2,Free,True,887.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(53), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,885.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_3,Free,True,481.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
  compareCsvLine(line.getRow(54), "seedgrid,2012-06-27T12:44:43,2012-06-27T12:44:42,Themis IR IMAGE NETWORK,2012-06-27T12:44:43,480.0,Candidate,THM_IMG_NET,MVA_2B2_01_03862S121E3536_bndry_4,Free,True,75.0,KAGUYA/MI-VIS/2008-08-20T18:21:08.380468,Mars,lweller,5");
}


/**
   * IsisminerTestCsvReader

   * Tests csv reader behavior
   * 
   * INPUT: 1) csvreader_data1.csv
   *        2) csvreader_data2.csv
   *        3) csvreader_test.conf
   * 
   * OUTPUT: 1) csvreader_test1.csv
   *         2) csvreader_test2.csv
   */
TEST(Isisminer, IsisminerTestCsvReader) {
  QTemporaryDir tempDir;

  // create input files csvreader_data1.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/csvreader_data1.csv");
  of << "This test should skip the first 4 lines, including this one and the header,\n"
     << "instead using the default column names.\n"
     << "-------------------------------------------------------------------------------\n"
     << "Respondent,Age,Gender,Height\n"
     << "1,30,NoData,60.0\n"
     << "2,NoData,M,68.2\n"
     << "3,14,F,58.1\n"
     << "4,23,F,71.9\n"
     << "5,51,M,72.0\n"
     << "6,NoData,M,63.0\n"
     << "7,32,M,55";
  of.close();

  // create input files csvreader_data2.csv
  of.open(tempDir.path().toStdString() + "/csvreader_data2.csv");
  of << "# This test should ignore comments and use a different delimiter\n"
     << "Respondent,Age,Gender,Height\n"
     << "1,30,NoData,60.0\n"
     << "2,NoData,M,68.2\n"
     << "3,14,F,58.1\n"
     << "4,23,F,71.9\n"
     << "5,51,M,72.0\n"
     << "6,NoData,M,63.0\n"
     << "# Example of commenting out a row that is likely incorrect data\n"
     << "#7,320,M,55";
  of.close();

  // create input Pvl file csvreader_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "CsvReaderStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    ////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "CsvReaderHeaderTest"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("Description", "Read a CSV file removing the header."));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/csvreader_data1.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "False"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "4"));
    CsvReader.addKeyword(PvlKeyword("ColumnBaseName", "\"Option\""));
    isisminerObject.addObject(CsvReader);

    ////////////////////////////////////////////////////
    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest1"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvreader_test1.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Option0, Option1, Option2, Option3)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
    isisminerObject.addObject(CsvWriter);

    ////////////////////////////////////////////////////
    // reset for next test
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Discard all previous resources for the next test."));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"));
    isisminerObject.addObject(ResourceManager);

    ////////////////////////////////////////////////////
    // add 2nd CsvReader strategy
    CsvReader.addKeyword(PvlKeyword("Name", "CsvReaderCommentsTest"), Pvl::Replace);
    CsvReader.addKeyword(PvlKeyword("Description", "Read a CSV file ignoring comments."), Pvl::Replace);
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/csvreader_data2.csv\""), Pvl::Replace);
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"), Pvl::Replace);
    CsvReader.deleteKeyword("SkipLines");
    CsvReader.deleteKeyword("ColumnBaseName");
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "True"));
    isisminerObject.addObject(CsvReader);

    ////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest2"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvreader_test2.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Respondent, Age, Gender, Height)"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/csvreader_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + tempDir.path() + "/csvreader_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // Validate output file csvreader_test1.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/csvreader_test1.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 8);

  // Verify each line
  compareCsvLine(line.getRow(0), "Option0,Option1,Option2,Option3");
  compareCsvLine(line.getRow(1), "1,30,NoData,60.0");
  compareCsvLine(line.getRow(2), "2,NoData,M,68.2");
  compareCsvLine(line.getRow(3), "3,14,F,58.1");
  compareCsvLine(line.getRow(4), "4,23,F,71.9");
  compareCsvLine(line.getRow(5), "5,51,M,72.0");
  compareCsvLine(line.getRow(6), "6,NoData,M,63.0");
  compareCsvLine(line.getRow(7), "7,32,M,55");

  /////////////////////////////////////////////////////////////////
  // Validate output file csvreader_test2.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/csvreader_test2.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 7);

  // Verify each line
  compareCsvLine(line.getRow(0), "Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1), "1,30,NoData,60.0");
  compareCsvLine(line.getRow(2), "2,NoData,M,68.2");
  compareCsvLine(line.getRow(3), "3,14,F,58.1");
  compareCsvLine(line.getRow(4), "4,23,F,71.9");
  compareCsvLine(line.getRow(5), "5,51,M,72.0");
  compareCsvLine(line.getRow(6), "6,NoData,M,63.0");
}


/**
   * IsisminerTestCsvWriter
   * 
   * Tests csv writer behavior
   * 
   * INPUT: 1) csvwriter_data.csv
   *        2) csvwriter_test.conf
   * 
   * OUTPUT: 1) csvwriter_test_append.csv
   *         2) csvwriter_test_defaultvalue.csv
   *         3) csvwriter_test_delimitter.csv
   *         4) csvwriter_test_wkb.csv (binary output)
   *         5) csvwriter_test_wkt.csv (text output)
   */
TEST(Isisminer, IsisminerTestCsvWriter) {
  QTemporaryDir tempDir;

  // create input file csvwriter_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/csvwriter_data.csv");
  of << "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank\n"
     << "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923\n"
     << "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721\n"
     << "2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.68035312577266,0.71964687422734,0.94860127782928,0.15419616651216,0.56695663138626,1.6495650529206,74.978806104249,2.2364977040852,2.4816253207025,4.71812\n"
     << "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404\n"
     << "2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.69201127796963,71.559210542126,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8798242820366,3.29886003388,6.17868\n"
     << "2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.66812642316293,0.73187357683707,0.94482398919458,0.16552803241626,0.56943160918339,1.6458525862249,73.726207850557,2.2364977040852,2.4680963072552,4.70459\n"
     << "2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.76397564900061,33.478295430042,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.8798242820366,3.3418923168616,6.22172\n"
     << "2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.187532910945,-37.538866268171,2498082,Albedo,2012-09-01T04:53:53.9892759,399747301.17189,27,244.36392774954,244.64037529875,40.357451261299,36.91669830859,77.264668712026,334.88827541235,156.53894571568,0.80972363775171,63.31934535738,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.7393690885783,3.3233125567068,6.06268\n"
     << "2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.34367888961712,95.942477726802,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.2364977040852,2.7206869531711,4.95718\n"
     << "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.74969392079315,32.478574455521,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.897658649418,3.2632828397482,6.16094\n"
     << "2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.88243478873011,41.770435211108,0.91883649181428,0.48116350818572,0.9569036943252,0.1292889170244,0.9306297245104,1.1040554132344,112.14122128604,2.2364977040852,3.68880469938,5.9253\n"
     << "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.7393496194796,31.754473363572,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.897658649418,3.1163576830336,6.01402\n";
  of.close();

  // create input isisminer config file csvwriter_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "IsNumericStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "( inputdir, outputdir )"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadCsvWriterData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("GisGeometry", "MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))"));
    CsvReader.addKeyword(PvlKeyword("GisType", "WKT"));
    CsvReader.addKeyword(PvlKeyword("RemoveGisKeywordAfterImport", "true"));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1_%2\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "( SourceProductId,  StereoSource )"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter strategy to test DefaultValue parameter
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "TestDefaultValue"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("Description", "Test the DefaultValue parameter. Empty is not a keyword in the Resources"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_test_defaultvalue.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(SourceProductId,  StereoSource, Empty)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"None\""));
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy to set up csv file for appending
    CsvWriter.addKeyword(PvlKeyword("Name", "SetUpAppend"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Description", "Sets up a CSV file to be appended to"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_test_append.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(SourceProductId,  StereoSource)"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 3rd CsvWriter strategy to append to csv file
    CsvWriter.addKeyword(PvlKeyword("Name", "TestAppend"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Description", "Tests appending to a CSV file."), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_test_append.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Mode", "Append"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Header", "False"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 4th CsvWriter strategy to test delimiting between fields
    CsvWriter.addKeyword(PvlKeyword("Name", "TestDelimiter"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Description", "Tests delimiting between fields."), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_test_delimiter.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Header", "True"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Delimiter", " "), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 5th CsvWriter TestWKT strategy (writes geometry in text format)
    CsvWriter.addKeyword(PvlKeyword("Name", "TestWKT"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Description", "Tests writing the geometry in well-known-text format."), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_test_wkt.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(SourceProductId, Geometry)"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ";"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("GisGeometryKey", "Geometry"));
    CsvWriter.addKeyword(PvlKeyword("GisType", "wkt"));
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 6th CsvWriter TestWKB strategy (writes geometry in binary format)
    CsvWriter.addKeyword(PvlKeyword("Name", "TestWKB"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Description", "Tests writing the geometry in well-known-binary format."), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/csvwriter_test_wkb.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("GisType", "wkb"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/csvwriter_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + tempDir.path() + "/csvwriter_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // csvwriter_test_append.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/csvwriter_test_append.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 25);

  // Verify each line
  compareCsvLine(line.getRow(0), "SourceProductId,StereoSource");
  compareCsvLine(line.getRow(1), "EN0253077000M,EN0253105125M");
  compareCsvLine(line.getRow(2), "EN0253133950M,EN0253105125M");
  compareCsvLine(line.getRow(3), "EN0254615176M,EN0253105125M");
  compareCsvLine(line.getRow(4), "EN0253077000M,EN0253133950M");
  compareCsvLine(line.getRow(5), "EN0253105125M,EN0253133950M");
  compareCsvLine(line.getRow(6), "EN0254615176M,EN0253133950M");
  compareCsvLine(line.getRow(7), "EN0253105125M,EN0253077000M");
  compareCsvLine(line.getRow(8), "EN0254962699M,EN0253077000M");
  compareCsvLine(line.getRow(9), "EN0254615176M,EN0253077000M");
  compareCsvLine(line.getRow(10), "EN0253077000M,EN0254962699M");
  compareCsvLine(line.getRow(11), "EN0254615176M,EN0254962699M");
  compareCsvLine(line.getRow(12), "EN0253077000M,EN0254615176M");
  compareCsvLine(line.getRow(13), "EN0253077000M,EN0253105125M");
  compareCsvLine(line.getRow(14), "EN0253133950M,EN0253105125M");
  compareCsvLine(line.getRow(15), "EN0254615176M,EN0253105125M");
  compareCsvLine(line.getRow(16), "EN0253077000M,EN0253133950M");
  compareCsvLine(line.getRow(17), "EN0253105125M,EN0253133950M");
  compareCsvLine(line.getRow(18), "EN0254615176M,EN0253133950M");
  compareCsvLine(line.getRow(19), "EN0253105125M,EN0253077000M");
  compareCsvLine(line.getRow(20), "EN0254962699M,EN0253077000M");
  compareCsvLine(line.getRow(21), "EN0254615176M,EN0253077000M");
  compareCsvLine(line.getRow(22), "EN0253077000M,EN0254962699M");
  compareCsvLine(line.getRow(23), "EN0254615176M,EN0254962699M");
  compareCsvLine(line.getRow(24), "EN0253077000M,EN0254615176M");

  /////////////////////////////////////////////////////////////////
  // csvwriter_test_defaultvalue.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/csvwriter_test_defaultvalue.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 3);
  ASSERT_EQ(line.rows(), 13);

  compareCsvLine(line.getRow(0), "SourceProductId,StereoSource,Empty");
  compareCsvLine(line.getRow(1), "EN0253077000M,EN0253105125M,None");
  compareCsvLine(line.getRow(2), "EN0253133950M,EN0253105125M,None");
  compareCsvLine(line.getRow(3), "EN0254615176M,EN0253105125M,None");
  compareCsvLine(line.getRow(4), "EN0253077000M,EN0253133950M,None");
  compareCsvLine(line.getRow(5), "EN0253105125M,EN0253133950M,None");
  compareCsvLine(line.getRow(6), "EN0254615176M,EN0253133950M,None");
  compareCsvLine(line.getRow(7), "EN0253105125M,EN0253077000M,None");
  compareCsvLine(line.getRow(8), "EN0254962699M,EN0253077000M,None");
  compareCsvLine(line.getRow(9), "EN0254615176M,EN0253077000M,None");
  compareCsvLine(line.getRow(10), "EN0253077000M,EN0254962699M,None");
  compareCsvLine(line.getRow(11), "EN0254615176M,EN0254962699M,None");
  compareCsvLine(line.getRow(12), "EN0253077000M,EN0254615176M,None");

  /////////////////////////////////////////////////////////////////
  // csvwriter_test_delimiter.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/csvwriter_test_delimiter.csv",
                   false, 0, ' ', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 13);

  // Verify each line
  compareCsvLine(line.getRow(0), "SourceProductId,StereoSource");
  compareCsvLine(line.getRow(1), "EN0253077000M,EN0253105125M");
  compareCsvLine(line.getRow(2), "EN0253133950M,EN0253105125M");
  compareCsvLine(line.getRow(3), "EN0254615176M,EN0253105125M");
  compareCsvLine(line.getRow(4), "EN0253077000M,EN0253133950M");
  compareCsvLine(line.getRow(5), "EN0253105125M,EN0253133950M");
  compareCsvLine(line.getRow(6), "EN0254615176M,EN0253133950M");
  compareCsvLine(line.getRow(7), "EN0253105125M,EN0253077000M");
  compareCsvLine(line.getRow(8), "EN0254962699M,EN0253077000M");
  compareCsvLine(line.getRow(9), "EN0254615176M,EN0253077000M");
  compareCsvLine(line.getRow(10), "EN0253077000M,EN0254962699M");
  compareCsvLine(line.getRow(11), "EN0254615176M,EN0254962699M");
  compareCsvLine(line.getRow(12), "EN0253077000M,EN0254615176M");

  /////////////////////////////////////////////////////////////////
  // csvwriter_test_wkb.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/csvwriter_test_wkb.csv",
                   false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 13);

  // Verify each line
  compareCsvLine(line.getRow(0), "SourceProductId,Geometry");
  compareCsvLine(line.getRow(1), "EN0253077000M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(2), "EN0253133950M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(3), "EN0254615176M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(4), "EN0253077000M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(5), "EN0253105125M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(6), "EN0254615176M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(7), "EN0253105125M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(8), "EN0254962699M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(9), "EN0254615176M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(10), "EN0253077000M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(11), "EN0254615176M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");
  compareCsvLine(line.getRow(12), "EN0253077000M,01060000000200000001030000000100000004000000000000000000444000000000000044400000000000003440000000000080464000000000008046400000000000003E4000000000000044400000000000004440010300000002000000060000000000000000003440000000000080414000000000000024400000000000003E40000000000000244000000000000024400000000000003E4000000000000014400000000000804640000000000000344000000000000034400000000000804140040000000000000000003E40000000000000344000000000000034400000000000002E40000000000000344000000000000039400000000000003E400000000000003440");

  /////////////////////////////////////////////////////////////////
  // csvwriter_test_wkt.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/csvwriter_test_wkt.csv",
                   false, 0, ';', true, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 13);

  // Verify each line
  compareCsvLineCustomDelimiter(line.getRow(1), "EN0253077000M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(2), "EN0253133950M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(3), "EN0254615176M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(4), "EN0253077000M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(5), "EN0253105125M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(6), "EN0254615176M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(7), "EN0253105125M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(8), "EN0254962699M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(9), "EN0254615176M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(10), "EN0253077000M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(11), "EN0254615176M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
  compareCsvLineCustomDelimiter(line.getRow(12), "EN0253077000M;MULTIPOLYGON (((40 40, 20 45, 45 30, 40 40)), ((20 35, 10 30, 10 10, 30 5, 45 20, 20 35), (30 20, 20 15, 20 25, 30 20)))", ";");
}


/**
   * IsisminerTestDatabaseReader
   * 
   * Tests SQLite database reader behavior
   * 
   * INPUT: 1) database_data.db
   *        2) database_test.conf
   * 
   * OUTPUT: database_test.csv
   */
TEST(Isisminer, IsisminerTestDatabaseReader) {
  QTemporaryDir tempDir;

  // create input SQLite database_data.db file in tempDir 
  FileName dbfile(tempDir.path() + "/database_data.db");
  Database db("isisminerTestdb", "SQLite");
  QString dbfileName(dbfile.expanded());
  db.setDatabaseName(dbfileName.toLatin1().data());
  if(!db.open()) {
    throw IException(IException::User, "Connection failed", _FILEINFO_);
  }

  SqlQuery query(db);
  query.setThrowOnFailure();

  string table = "CREATE TABLE Geometries ("
                 "Name char(80),"
                 "Geometry char(160));";
  query.exec(table);
  string insert = "INSERT INTO Geometries (Name, Geometry) "
                  " VALUES ('ALPHA', '01010000000000000000003E400000000000002440');";
  query.exec(insert);
  insert = "INSERT INTO Geometries (Name, Geometry) "
           " VALUES ('BRAVO', '0102000000030000000000000000003E40000000000000244000000000000024400000000000003E4000000000000044400000000000004440');";
  query.exec(insert);
  insert = "INSERT INTO Geometries (Name, Geometry) "
                  " VALUES ('CHARLIE', '010300000001000000050000000000000000003E4000000000000024400000000000004440000000000000444000000000000034400000000000004440000000000000244000000000000034400000000000003E400000000000002440');";
  query.exec(insert);
  insert = "INSERT INTO Geometries (Name, Geometry) "
           " VALUES ('DELTA', '0103000000020000000500000000000000008041400000000000002440000000000080464000000000008046400000000000002E40000000000000444000000000000024400000000000003440000000000080414000000000000024400400000000000000000034400000000000003E40000000000080414000000000008041400000000000003E40000000000000344000000000000034400000000000003E40');";
  query.exec(insert);
  db.close();

  // create input Pvl file csvreader_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "CalculatorStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));
  
  ////////////////////////////////////////////////////
  // add DatabaseReader strategy
  PvlObject DatabaseReader("Strategy");
  DatabaseReader.addKeyword(PvlKeyword("Name", "GetResources"));
  DatabaseReader.addKeyword(PvlKeyword("Type", "DatabaseReader"));
  DatabaseReader.addKeyword(PvlKeyword("DbFile", "\"%1/database_data.db\""));
  DatabaseReader.addKeyword(PvlKeyword("DbFileArgs", "inputdir"));
  DatabaseReader.addKeyword(PvlKeyword("Query", "SELECT Name, Geometry FROM Geometries"));
  DatabaseReader.addKeyword(PvlKeyword("QueryStore", "Query"));
  DatabaseReader.addKeyword(PvlKeyword("GisGeometryRef", "Geometry"));
  DatabaseReader.addKeyword(PvlKeyword("GisType", "WKB"));
  DatabaseReader.addKeyword(PvlKeyword("RemoveGisKeywordAfterImport", "True"));
  DatabaseReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
  DatabaseReader.addKeyword(PvlKeyword("IdentityArgs", "Name"));
  DatabaseReader.addKeyword(PvlKeyword("Target", "Resource"));
  isisminerObject.addObject(DatabaseReader);

  ////////////////////////////////////////////////////
  // add CsvWriter strategy
  PvlObject CsvWriter("Strategy");
  CsvWriter.addKeyword(PvlKeyword("Name", "WriteResources"));
  CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/database_test.csv\""));
  CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "outputdir"));
  CsvWriter.addKeyword(PvlKeyword("Header", "True"));
  CsvWriter.addKeyword(PvlKeyword("Keywords", "(Name, Geometry, Query)"));
  CsvWriter.addKeyword(PvlKeyword("GisGeometryKey", "Geometry"));
  CsvWriter.addKeyword(PvlKeyword("Delimiter", "\";\""));
  CsvWriter.addKeyword(PvlKeyword("GisType", "WKT"));
  isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/database_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + configFilename,
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv file
  CSVReader::CSVAxis csvLine;

  /////////////////////////////////////////////////////////////////
  // Validate output file database_test.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/database_test.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 3);
  ASSERT_EQ(line.rows(), 5);

  // Verify each line
  compareCsvLine(line.getRow(0), "Name,Geometry,Query");
  compareCsvLine(line.getRow(1), "ALPHA;POINT (30 10);SELECT Name, Geometry FROM Geometries", ';');
  compareCsvLine(line.getRow(2), "BRAVO;LINESTRING (30 10, 10 30, 40 40);SELECT Name, Geometry FROM Geometries", ';');
  compareCsvLine(line.getRow(3), "CHARLIE;POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10));SELECT Name, Geometry FROM Geometries", ';');
  compareCsvLine(line.getRow(4), "DELTA;POLYGON ((35 10, 45 45, 15 40, 10 20, 35 10), (20 30, 35 35, 30 20, 20 30));SELECT Name, Geometry FROM Geometries", ';');
}


/**
   * IsisminerTestFilter
   * 
   * Tests filter behavior
   * 
   * INPUT: 1) filter_data.csv
   *        2) filter_test.conf
   * 
   * OUTPUT: 1) filter_test_exclude.csv
   *         2) filter_test_include_exclude.csv
   *         3) filter_test_include.csv
   *         4) filter_test_multi_include.csv
   *         5) filter_test_noop.csv
   */  
TEST(Isisminer, IsisminerTestFilter) {
  QTemporaryDir tempDir;

  // create input file filter_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/filter_data.csv");
  of << "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank\n";
  of << "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923\n";
  of << "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721\n";
  of << "2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.68035312577266,0.71964687422734,0.94860127782928,0.15419616651216,0.56695663138626,1.6495650529206,74.978806104249,2.2364977040852,2.4816253207025,4.71812\n";
  of << "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404\n";
  of << "2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.69201127796963,71.559210542126,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8798242820366,3.29886003388,6.17868\n";
  of << "2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.66812642316293,0.73187357683707,0.94482398919458,0.16552803241626,0.56943160918339,1.6458525862249,73.726207850557,2.2364977040852,2.4680963072552,4.70459\n";
  of << "2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.76397564900061,33.478295430042,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.8798242820366,3.3418923168616,6.22172\n";
  of << "2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.187532910945,-37.538866268171,2498082,Albedo,2012-09-01T04:53:53.9892759,399747301.17189,27,244.36392774954,244.64037529875,40.357451261299,36.91669830859,77.264668712026,334.88827541235,156.53894571568,0.80972363775171,63.31934535738,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.7393690885783,3.3233125567068,6.06268\n";
  of << "2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.34367888961712,95.942477726802,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.2364977040852,2.7206869531711,4.95718\n";
  of << "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.74969392079315,32.478574455521,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.897658649418,3.2632828397482,6.16094\n";
  of << "2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.88243478873011,41.770435211108,0.91883649181428,0.48116350818572,0.9569036943252,0.1292889170244,0.9306297245104,1.1040554132344,112.14122128604,2.2364977040852,3.68880469938,5.9253\n";
  of << "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.7393496194796,31.754473363572,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.897658649418,3.1163576830336,6.01402\n";
  of.close();

  // create input isisminer config file csvwriter_test.conf
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "FilterStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "( inputdir, outputdir )"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadFilterData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/filter_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1_%2\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "( SourceProductId,  StereoSource )"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add TestInclude Filter strategy
    PvlObject Filter("Strategy");
    Filter.addKeyword(PvlKeyword("Name", "TestInclude"));
    Filter.addKeyword(PvlKeyword("Type", "Filter"));
    Filter.addKeyword(PvlKeyword("Description", "Keep all SourceA stereo sets"));
    Filter.addKeyword(PvlKeyword("Keyword", "SourceSet"));
    Filter.addKeyword(PvlKeyword("Include", "\"SourceA\""));
    isisminerObject.addObject(Filter);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter WriteTestInclude strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestInclude"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/filter_test_include.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));

    QStringList keywordList = {"YearDoy","File","StereoSource","SourceProductId",
                  "SourceSet","CenterLongitude","CenterLatitude","ObservationId",
                  "ObservationType","StartTime","EtStartTime","ExposureDuration",
                  "PixelResolution","MeanGroundResolution","IncidenceAngle",
                  "EmissionAngle","PhaseAngle","SubSolarGroundAzimuth",
                  "SubSpacecraftGroundAzimuth","OverlapRatio","OverlapPercentage",
                  "ParallaxHeightRatio","StereoDp","ShadowTipDistance","StereoDsh",
                  "ResolutionRatio","StereoResolutionRatio","StereoVerticalPrecision",
                  "StereoThresholds","StereoStrength","StereoPairRank"};

    PvlKeyword kw("Keywords");
    for (auto const &v : keywordList) {
      kw.addValue(v);
    }

    CsvWriter.addKeyword(kw);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // reset for next test
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for multi-Include test"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add TestExclude Filter strategy
    Filter.addKeyword(PvlKeyword("Name", "TestExclude"), Pvl::Replace);
    Filter.addKeyword(PvlKeyword("Description", "Selects EN0253077000M and EN0253133950M"), Pvl::Replace);
    Filter.addKeyword(PvlKeyword("Keyword", "SourceProductId"), Pvl::Replace);
    Filter.addKeyword(PvlKeyword("Include", "(EN0253077000M, EN0253133950M)"), Pvl::Replace);
    isisminerObject.addObject(Filter);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter WriteTestExclude strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestExclude"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/filter_test_multi_include.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // reset for next test
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for Exclude test"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add TestExclude Filter strategy
    Filter.addKeyword(PvlKeyword("Description", "Eliminate all SourceB stereo sets"), Pvl::Replace);
    Filter.addKeyword(PvlKeyword("Keyword", "SourceSet"), Pvl::Replace);
    Filter.deleteKeyword("Include");
    Filter.addKeyword(PvlKeyword("Exclude", "\"SourceB\""));
    isisminerObject.addObject(Filter);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter WriteTestExclude strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/filter_test_exclude.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // reset for next test
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for Include/Exclude test"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add Filter TestExclude strategy
    Filter.addKeyword(PvlKeyword("Description", "Selects EN0253077000M and EN0253133950M"), Pvl::Replace);
    Filter.addKeyword(PvlKeyword("Keyword", "SourceProductId"), Pvl::Replace);
    Filter.addKeyword(PvlKeyword("Include", "EN0253077000M"));
    Filter.addKeyword(PvlKeyword("Exclude", "(EN0254615176M, EN0253105125M, EN0254962699M)"), Pvl::Replace);
    isisminerObject.addObject(Filter);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter WriteTestExclude strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "%1/filter_test_include_exclude.csv"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // reset for next test
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources noop test"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add TestExclude Filter strategy
    Filter.addKeyword(PvlKeyword("Description", "Noop test"), Pvl::Replace);
    Filter.deleteKeyword("Include");
    Filter.deleteKeyword("Exclude");
    isisminerObject.addObject(Filter);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter WriteTestExclude strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/filter_test_noop.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);

  QString configFilename = tempDir.path() + "/filter_test.conf";
  conf.write(configFilename);

  QVector<QString> args = {"config=" + configFilename,
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // filter_test_exclude.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/filter_test_exclude.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 31);
  ASSERT_EQ(line.rows(), 9);

  // Verify each line
  compareCsvLine(line.getRow(0),"YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank");
  compareCsvLine(line.getRow(1),"2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923");
  compareCsvLine(line.getRow(2),"2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721");
  compareCsvLine(line.getRow(3),"2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.68035312577266,0.71964687422734,0.94860127782928,0.15419616651216,0.56695663138626,1.6495650529206,74.978806104249,2.2364977040852,2.4816253207025,4.71812");
  compareCsvLine(line.getRow(4),"2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404");
  compareCsvLine(line.getRow(5),"2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.66812642316293,0.73187357683707,0.94482398919458,0.16552803241626,0.56943160918339,1.6458525862249,73.726207850557,2.2364977040852,2.4680963072552,4.70459");
  compareCsvLine(line.getRow(6),"2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.187532910945,-37.538866268171,2498082,Albedo,2012-09-01T04:53:53.9892759,399747301.17189,27,244.36392774954,244.64037529875,40.357451261299,36.91669830859,77.264668712026,334.88827541235,156.53894571568,0.80972363775171,63.31934535738,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.7393690885783,3.3233125567068,6.06268");
  compareCsvLine(line.getRow(7),"2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.34367888961712,95.942477726802,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.2364977040852,2.7206869531711,4.95718");
  compareCsvLine(line.getRow(8),"2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.88243478873011,41.770435211108,0.91883649181428,0.48116350818572,0.9569036943252,0.1292889170244,0.9306297245104,1.1040554132344,112.14122128604,2.2364977040852,3.68880469938,5.9253");

  /////////////////////////////////////////////////////////////////
  // filter_test_include_exclude.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/filter_test_include_exclude.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 31);
  ASSERT_EQ(line.rows(), 6);

  compareCsvLine(line.getRow(0), "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank");
  compareCsvLine(line.getRow(1), "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923");
  compareCsvLine(line.getRow(2), "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721");
  compareCsvLine(line.getRow(3), "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404");
  compareCsvLine(line.getRow(4), "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.74969392079315,32.478574455521,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.897658649418,3.2632828397482,6.16094");
  compareCsvLine(line.getRow(5), "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.7393496194796,31.754473363572,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.897658649418,3.1163576830336,6.01402");

  /////////////////////////////////////////////////////////////////
  // filter_test_include.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/filter_test_include.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),31);
  ASSERT_EQ(line.rows(), 9);

  // Verify each line
  compareCsvLine(line.getRow(0), "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank");
  compareCsvLine(line.getRow(1), "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923");
  compareCsvLine(line.getRow(2), "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721");
  compareCsvLine(line.getRow(3), "2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.68035312577266,0.71964687422734,0.94860127782928,0.15419616651216,0.56695663138626,1.6495650529206,74.978806104249,2.2364977040852,2.4816253207025,4.71812");
  compareCsvLine(line.getRow(4), "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404");
  compareCsvLine(line.getRow(5), "2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.66812642316293,0.73187357683707,0.94482398919458,0.16552803241626,0.56943160918339,1.6458525862249,73.726207850557,2.2364977040852,2.4680963072552,4.70459");
  compareCsvLine(line.getRow(6), "2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.187532910945,-37.538866268171,2498082,Albedo,2012-09-01T04:53:53.9892759,399747301.17189,27,244.36392774954,244.64037529875,40.357451261299,36.91669830859,77.264668712026,334.88827541235,156.53894571568,0.80972363775171,63.31934535738,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.7393690885783,3.3233125567068,6.06268");
  compareCsvLine(line.getRow(7), "2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.34367888961712,95.942477726802,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.2364977040852,2.7206869531711,4.95718");
  compareCsvLine(line.getRow(8), "2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.88243478873011,41.770435211108,0.91883649181428,0.48116350818572,0.9569036943252,0.1292889170244,0.9306297245104,1.1040554132344,112.14122128604,2.2364977040852,3.68880469938,5.9253");

  /////////////////////////////////////////////////////////////////
  // filter_test_multi_include.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/filter_test_multi_include.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 31);
  ASSERT_EQ(line.rows(), 6);

  // Verify each line
  compareCsvLine(line.getRow(0), "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank");
  compareCsvLine(line.getRow(1), "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923");
  compareCsvLine(line.getRow(2), "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721");
  compareCsvLine(line.getRow(3), "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404");
  compareCsvLine(line.getRow(4), "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.74969392079315,32.478574455521,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.897658649418,3.2632828397482,6.16094");
  compareCsvLine(line.getRow(5), "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.7393496194796,31.754473363572,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.897658649418,3.1163576830336,6.01402");

  /////////////////////////////////////////////////////////////////
  // filter_test_noop.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/filter_test_noop.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 31);
  ASSERT_EQ(line.rows(), 13);

  // Verify each line
  compareCsvLine(line.getRow(0), "YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationId,ObservationType,StartTime,EtStartTime,ExposureDuration,PixelResolution,MeanGroundResolution,IncidenceAngle,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth,SubSpacecraftGroundAzimuth,OverlapRatio,OverlapPercentage,ParallaxHeightRatio,StereoDp,ShadowTipDistance,StereoDsh,ResolutionRatio,StereoResolutionRatio,StereoVerticalPrecision,StereoThresholds,StereoStrength,StereoPairRank");
  compareCsvLine(line.getRow(1), "2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.90365715205278,43.256000643695,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.897658649418,3.4815738199138,6.37923");
  compareCsvLine(line.getRow(2), "2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.045872010675,-35.397287660352,2367191,Albedo,2012-08-11T00:54:44.938421,397918552.12144,29,163.92192076565,164.02938781718,35.524664032576,28.949070155831,64.467833201153,354.10807634035,175.58105634295,0.68683883127001,71.9212818111,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8935249623071,3.2936875871804,6.18721");
  compareCsvLine(line.getRow(3), "2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.68035312577266,0.71964687422734,0.94860127782928,0.15419616651216,0.56695663138626,1.6495650529206,74.978806104249,2.2364977040852,2.4816253207025,4.71812");
  compareCsvLine(line.getRow(4), "2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.73698540051444,31.588978036011,0.67937697727383,0.079376977273826,0.97750461782677,0.06748614651968,0.93251291445186,1.1012306283222,454.83173081236,2.897658649418,3.3263799100669,6.22404");
  compareCsvLine(line.getRow(5), "2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.69201127796963,71.559210542126,0.61377822839461,0.013778228394615,0.99457429390277,0.016277118291676,0.99849623361299,1.0022556495805,2379.4339311391,2.8798242820366,3.29886003388,6.17868");
  compareCsvLine(line.getRow(6), "2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.28571428571429,100.0,0.66812642316293,0.73187357683707,0.94482398919458,0.16552803241626,0.56943160918339,1.6458525862249,73.726207850557,2.2364977040852,2.4680963072552,4.70459");
  compareCsvLine(line.getRow(7), "2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.230370786681,-35.113756426159,2365080,Albedo,2012-08-10T16:54:19.9408223,397889727.12385,23,163.55300250416,163.66080140544,35.305747296439,29.252112185604,64.55193066025,352.88112015209,174.35504070321,0.76397564900061,33.478295430042,0.66916812436863,0.069168124368634,0.9778916226437,0.06632513206889,0.93085692084865,1.103714618727,521.96251220714,2.8798242820366,3.3418923168616,6.22172");
  compareCsvLine(line.getRow(8), "2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.187532910945,-37.538866268171,2498082,Albedo,2012-09-01T04:53:53.9892759,399747301.17189,27,244.36392774954,244.64037529875,40.357451261299,36.91669830859,77.264668712026,334.88827541235,156.53894571568,0.80972363775171,63.31934535738,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.7393690885783,3.3233125567068,6.06268");
  compareCsvLine(line.getRow(9), "2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.34367888961712,95.942477726802,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.2364977040852,2.7206869531711,4.95718");
  compareCsvLine(line.getRow(10), "2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.74969392079315,32.478574455521,0.83127664162471,0.23127664162471,0.91811093086143,0.24566720741572,0.76420134646892,1.3536979802966,211.31743009834,2.897658649418,3.2632828397482,6.16094");
  compareCsvLine(line.getRow(11), "2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.177965207832,-37.109188070069,2473506,Monochrome,2012-08-28T04:21:50.9789346,399399778.16161,28,269.7913172311,270.54050921581,38.386785986144,50.944170749394,89.1594453885,342.97879878391,156.59153764894,0.88243478873011,41.770435211108,0.91883649181428,0.48116350818572,0.9569036943252,0.1292889170244,0.9306297245104,1.1040554132344,112.14122128604,2.2364977040852,3.68880469938,5.9253");
  compareCsvLine(line.getRow(12), "2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.849325610563,-37.367877941759,2363042,Albedo,2012-08-10T09:05:34.9362497,397861602.11929,31,180.51583980054,180.65380650435,37.661205560439,32.11185095429,69.766496871897,351.42722462143,172.90090222242,0.7393496194796,31.754473363572,0.7457095333341,0.6542904666659,0.96100358557389,0.11698924327833,0.67029494464602,1.494557583031,82.468362593112,2.897658649418,3.1163576830336,6.01402");
}


/**
   * IsisminerTestGisIntersect
   * 
   * Tests GIS intersection behavior
   * 
   * INPUT: 1) EN0240208184M.lev1.isd (in isis/tests/data/isisminer/gisintersect)
   *        2) EN0240208184M.lev1.pvl (in isis/tests/data/isisminer/gisintersect)
   *        2) gisfile.pvl
   *        3) pvlList.lis
   *        4) MercuryQuadGeoms.pvl
   *        5) gisintersect_test.conf
   * 
   * OUTPUT: 1) gisintersect_isiscube.csv
   *         2) gisintersect_test_no_overlap.csv
   *         3) gisintersect_test_overlap.csv
   *         4) gisintersect_test_wkb.csv
   *         5) gisintersect_test_wkt_bb.csv
   *         6) gisintersect_test_wkt.csv
   * 
   */  
TEST(Isisminer, IsisminerTestGisIntersect) {
  QTemporaryDir tempDir;

  // create cube from isd and label files
  FileName isdFile("$ISISROOT/../isis/tests/data/isisminer/gisintersect/EN0240208184M.lev1.isd");
  FileName labelFile("$ISISROOT/../isis/tests/data/isisminer/gisintersect/EN0240208184M.lev1.pvl");

  Cube cube;
  cube.fromIsd(tempDir.path() + "/EN0240208184M.lev1.cub", labelFile, isdFile, "rw");
  
  // run camstats and footprintinit on newly created cube
  QVector<QString> args = {"from=" + tempDir.path() + "/EN0240208184M.lev1.cub",
                                  "attach=yes",
                                  "linc=10",
                                  "sinc=10"
                          };

  UserInterface ui1(APP_XML2, args);

  try {
    Pvl camstatsLog;
    camstats(ui1, &camstatsLog);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }
  
  args = {"from=" + tempDir.path() + "/EN0240208184M.lev1.cub",
          "increaseprecision=true",
          "linc=10",
          "sinc=10",
          "maxemission=89",
          "maxincidence=89",
          "limbtest=ellipsoid",
          "testxy=false"
                          };

  UserInterface ui2(APP_XML3, args);

  try {
    Pvl fpinitLog;
    footprintinit(ui2, &fpinitLog);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // create input file gisfile.pvl
  ofstream of;
  of.open(tempDir.path().toStdString() + "/gisfile.pvl");
  of << "Object = TestGisPvl\n";
  of << "    NonIntersectingGeometry = \"POLYGON ((3 1, 4 4, 2 4, 1 2, 3 1))\"\n";
  of << "    IntersectingGeometry = \"POLYGON ((30 10, 40 40, 20 40, 10 20, 30 10))\"\n";
  of << "EndObject";
  of.close();

  // create input file MercuryQuadGeoms.pvl
  of.open(tempDir.path().toStdString() + "/MercuryQuadGeoms.pvl");
  of << "Object = MercuryQuadGeoms\n";
  of << "  Object = H1-Borealis\n";
  of << "    Keyword = \"Value\"\n";
  of << "  EndObject\n";
  of << "EndObject\n";
  of << "End\n";
  of.close();

  // create input file pvlList.lis
  of.open(tempDir.path().toStdString() + "/pvlList.lis");
  of << tempDir.path().toStdString() + "/MercuryQuadGeoms.pvl\n";
  of << tempDir.path().toStdString() + "/MercuryQuadGeoms.pvl\n";
  of.close();

  // create input isisminer config file gisintersect_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "GisIntersectStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir1, inputdir2, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add PvlReader strategies to load some Resourcess with geometries
    PvlObject PvlReader("Strategy");
    PvlReader.addKeyword(PvlKeyword("Name", "ReadGisBasicData"));
    PvlReader.addKeyword(PvlKeyword("Type", "PvlReader"));
    PvlReader.addKeyword(PvlKeyword("FromList", "\"%1/pvlList.lis\""));
    PvlReader.addKeyword(PvlKeyword("FromListArgs", "\"inputdir1\""));
    PvlReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    PvlReader.addKeyword(PvlKeyword("GisType", "WKT"));
    PvlReader.addKeyword(PvlKeyword("GisGeometry", "POLYGON ((0.00 22.50, 90.00 22.50, 90.00 65.00, 0.00 65.00, 0.00 22.50))"));
    PvlReader.addKeyword(PvlKeyword("Includes", "(Root, MercuryQuadGeoms, H1-Borealis, Keyword)"));
    isisminerObject.addObject(PvlReader);

    // add 2nd PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("GisGeometry", "POLYGON((98.4375 45.6352650622453,108.10546875 48.03962279752138,90 65,70.6640625 60.67556673490247,89.12109375 42.58223548221718,98.4375 45.6352650622453))"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);

    /////////////////////////////////////////////////////////////////
    // add GisIntersect strategy, read WKT from a pvl file
    PvlObject GisIntersect("Strategy");
    GisIntersect.addKeyword(PvlKeyword("Name", "GisIntersectTest1"));
    GisIntersect.addKeyword(PvlKeyword("Type", "GisIntersect"));
    GisIntersect.addKeyword(PvlKeyword("GisType", "pvlWKT"));
    GisIntersect.addKeyword(PvlKeyword("GisGeometry", "\"%1/gisfile.pvl\""));
    GisIntersect.addKeyword(PvlKeyword("GisGeometryArgs", "\"inputdir1\""));
    GisIntersect.addKeyword(PvlKeyword("GisGeometryRef", "\"NonIntersectingGeometry\""));
    GisIntersect.addKeyword(PvlKeyword("BoundingBox", "True"));
    GisIntersect.addKeyword(PvlKeyword("ComputeRatio", "True"));
    GisIntersect.addKeyword(PvlKeyword("RatioRef", "OverlapRatio"));
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteGisNoOverlapTest"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/gisintersect_test_no_overlap.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Keyword, OverlapRatio)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add ResourceManager strategy to restore all resources
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ReactivateAll"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Reset all discarded Resources"));
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 2nd GisIntersect strategy, pvkWKT again with non-zero
    // overlap & ComputeRatio
    GisIntersect.addKeyword(PvlKeyword("Name", "GisIntersectTest2"), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisGeometryRef", "\"IntersectingGeometry\""), Pvl::Replace);
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteGisOverlapTest"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/gisintersect_test_overlap.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 2nd ResourceManager strategy
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 3rd GisIntersect strategy, GisType = WKT, without bounding box
    GisIntersect.addKeyword(PvlKeyword("Name", "GisIntersectTestWKT"), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisType", "WKT"), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisGeometry", "POLYGON((33 19.34,40 40,20 40,10 20,33 19.34))"), Pvl::Replace);
    GisIntersect.deleteKeyword("GisGeometryArgs");
    GisIntersect.deleteKeyword("GisGeometryRef");
    GisIntersect.addKeyword(PvlKeyword("BoundingBox", "False"), Pvl::Replace);
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add 3rd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteGisIntersectBasicTest"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/gisintersect_test_wkt.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 3rd ResourceManager strategy
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 4th GisIntersect strategy, GisType = WKT, with bounding box
    GisIntersect.addKeyword(PvlKeyword("BoundingBox", "True"), Pvl::Replace);
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add 4th CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/gisintersect_test_wkt_bb.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 4th ResourceManager strategy
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 3rd PvlReader strategy, Add resource for WKB test
    PvlReader.addKeyword(PvlKeyword("GisGeometry", "POLYGON((30 10,30 -13,-20 -13,-20 10,30 10))"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);

    /////////////////////////////////////////////////////////////////
    // add 5th GisIntersect strategy, GisType = WKB
    GisIntersect.addKeyword(PvlKeyword("Name", "GisIntersectTestWKB"), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisType", "WKB"), Pvl::Replace);
    GisIntersect.deleteKeyword("BoundingBox");
    GisIntersect.addKeyword(PvlKeyword("GisGeometry", "\"000000000140000000000000004010000000000000\""), Pvl::Replace);
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add 5th CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/gisintersect_test_wkb.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 5th ResourceManager strategy
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 6th GisIntersect strategy, GisType=IsisCube
    GisIntersect.addKeyword(PvlKeyword("Name", "GisIntersectIsisCubeTest"), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisType", "IsisCube"), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisGeometryArgs", "\"inputdir1\""), Pvl::Replace);
    GisIntersect.addKeyword(PvlKeyword("GisGeometry", "\"%1/EN0240208184M.lev1.cub\""), Pvl::Replace);
    GisIntersect.deleteKeyword("ComputeRatio");
    GisIntersect.deleteKeyword("RatioRef");
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add 6th CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/gisintersect_isiscube.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Keyword)"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/gisintersect_test.conf");

  args = {"config=" + tempDir.path() + "/gisintersect_test.conf",
          "parameters=inputdir1:" + tempDir.path()
                                + "@inputdir2:data/isisminer/gisintersect"
                                + "@outputdir:"
                                + tempDir.path()
        };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // gisintersect_isiscube.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/gisintersect_isiscube.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 1);
  ASSERT_EQ(line.rows(), 1);

  // Verify each line
  compareCsvLine(line.getRow(0),"Keyword");

  /////////////////////////////////////////////////////////////////
  // gisintersect_test_no_overlap.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/gisintersect_test_no_overlap.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 1);

  // Verify each line
  compareCsvLine(line.getRow(0),"Keyword,OverlapRatio");

  /////////////////////////////////////////////////////////////////
  // gisintersect_test_overlap.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/gisintersect_test_overlap.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  // Verify each line
  compareCsvLine(line.getRow(0),"Keyword,OverlapRatio");
  compareCsvLine(line.getRow(1),"Value,0.13725490196078");
  compareCsvLine(line.getRow(2),"Value,0.13725490196078");

  /////////////////////////////////////////////////////////////////
  // gisintersect_test_wkb.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/gisintersect_test_wkb.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  // Verify each line
  compareCsvLine(line.getRow(0),"Keyword,OverlapRatio");
  compareCsvLine(line.getRow(1),"Value,0.0");
  compareCsvLine(line.getRow(2),"Value,0.0");

  /////////////////////////////////////////////////////////////////
  // gisintersect_test_wkt_bb.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/gisintersect_test_wkt_bb.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  // Verify each line
  compareCsvLine(line.getRow(0),"Keyword,OverlapRatio");
  compareCsvLine(line.getRow(1),"Value,0.13725490196078");
  compareCsvLine(line.getRow(2),"Value,0.13725490196078");

  /////////////////////////////////////////////////////////////////
  // gisintersect_test_wkt.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/gisintersect_test_wkt.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  // Verify each line
  compareCsvLine(line.getRow(0),"Keyword,OverlapRatio");
  compareCsvLine(line.getRow(1),"Value,0.09795577637315");
  compareCsvLine(line.getRow(2),"Value,0.09795577637315");
}


/**
   * IsisminerTestGisOverlap
   * 
   * Tests GIS overlap behavior
   * 
   * INPUT: 1) degaus_stereo_perimeter_gisoverlap.csv (in isis/tests/data/isisminer/gisoverlap)
   *        2) gisoverlap_test.conf
   * 
   * OUTPUT: 1) EN1021761181M.csv
   *         2) EN1021761238M.csv
   *         3) EN1021761290M.csv
   *         4) EN1036913727M.csv
   *         5) EN1036913759M.csv
   *         6) EN1036913791M.csv
   *         7) EN1036942534M.csv
   *         8) EN1036942566M.csv
   *         9) EN1036942598M.csv
   *        10) EN1052094505M.csv
   *        11) EN1052152435M.csv
   *        12) EN1052152463M.csv
   * 
   * NOTE: Original test has 78 output files, I cut it to 12, evenly
   *       distributed through original output.
   */  
TEST(Isisminer, IsisminerTestGisOverlap) {
  QTemporaryDir tempDir;

  // create input isisminer config file gisoverlap_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "GisOverlapStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadDegausPerimeter"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("Description", "Read Degaus data"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/degaus_stereo_perimeter_gisoverlap.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "\"SourceProductId\""));
    CsvReader.addKeyword(PvlKeyword("GisGeometryRef", "GisFootprint"));
    CsvReader.addKeyword(PvlKeyword("GisType", "WKB"));
    CsvReader.addKeyword(PvlKeyword("RemoveGisKeywordAfterImport", "False"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add GisOverlap strategy
    PvlObject GisOverlap("Strategy");
    GisOverlap.addKeyword(PvlKeyword("Name", "WAC_NAC_Overlaps"));
    GisOverlap.addKeyword(PvlKeyword("Type", "GisOverlap"));
    GisOverlap.addKeyword(PvlKeyword("Description", "Find all overlaps in the Degaus Crater region"));
    GisOverlap.addKeyword(PvlKeyword("OverlapMerge", "None"));
    isisminerObject.addObject(GisOverlap);

    /////////////////////////////////////////////////////////////////
    // add AssetSidebar strategy
    PvlObject AssetSidebar("Strategy");
    AssetSidebar.addKeyword(PvlKeyword("Name", "WriteOverlapFiles"));
    AssetSidebar.addKeyword(PvlKeyword("Type", "AssetSidebar"));
    AssetSidebar.addKeyword(PvlKeyword("Description", "Write CSV files of overlap data"));
    AssetSidebar.addKeyword(PvlKeyword("Asset", "GisOverlap"));

      // add IsisMiner sub object
      PvlObject isisminerSubObject("IsisMiner");

        // add CsvWriter strategy
        PvlObject CsvWriter("Strategy");
        CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
        CsvWriter.addKeyword(PvlKeyword("Name", "WriteOverlaps"));
        CsvWriter.addKeyword(PvlKeyword("Description", "Write individual files with overlap data"));
        CsvWriter.addKeyword(PvlKeyword("SkipEmptyLists", "True"));
        CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/%2.csv\""));
        CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "(\"outputdir\", \"Identity\")"));

        QStringList keywordList = {"\"SourceProductIdA\"","\"SourceProductIdB\"",
                                  "\"OverlapRatioA\"","\"OverlapRatioB\"","\"YearDoyA\"",
                                  "\"YearDoyB\"","\"CenterLongitudeA\"","\"CenterLatitudeA\"",
                                  "\"CenterLongitudeB\"","\"CenterLatitudeB\"",
                                  "\"PixelResolutionA\"","\"PixelResolutionB\"", 
                                  "\"EmissionAngleA\"","\"EmissionAngleB\"",
                                  "\"IncidenceAngleA\"","\"IncidenceAngleB\"",
                                  "\"PhaseAngleA\"","\"PhaseAngleB\""};

        PvlKeyword kw("Keywords");
        for (auto const &v : keywordList) {
          kw.addValue(v);
        }

        CsvWriter.addKeyword(kw);
        isisminerSubObject.addObject(CsvWriter);
    AssetSidebar.addObject(isisminerSubObject);
  isisminerObject.addObject(AssetSidebar);
  conf.addObject(isisminerObject);

  conf.write(tempDir.path() + "/gisoverlap_test.conf");

  QVector<QString> args = {"config=" + tempDir.path() + "/gisoverlap_test.conf",
                           "parameters=inputdir:data/isisminer/gisoverlap/@outputdir:"
                                     + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // EN1021761181M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/EN1021761181M.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 18);
  ASSERT_EQ(line.rows(), 9);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN1021761181M,EN1036942546M,0.0018404364458238,0.0045964873685719,2013260,2014071,232.52801191399,34.144540354162,232.44495057379,33.756650893517,19.975873233453,12.669009285673,1.061492188504,1.1557775323499,67.311651853235,67.868344280149,66.276273887369,66.729426757129");
  compareCsvLine(line.getRow(2),"EN1021761181M,EN1036913735M,0.10948052217373,0.25123368395193,2013260,2014071,232.52801191399,34.144540354162,232.1771752747,33.971521573057,19.975873233453,13.04437997592,1.061492188504,12.47925941016,67.311651853235,69.01571187858,66.276273887369,56.538003647904");
  compareCsvLine(line.getRow(3),"EN1021761181M,EN1036942550M,0.33948696195239,0.85808717342441,2013260,2014071,232.52801191399,34.144540354162,232.47789285611,34.019111459508,19.975873233453,12.573888220525,1.061492188504,1.2005388830088,67.311651853235,67.913641869889,66.276273887369,66.729448728797");
  compareCsvLine(line.getRow(4),"EN1021761181M,EN1036913739M,0.17368555517412,0.40319461043998,2013260,2014071,232.52801191399,34.144540354162,232.22005440747,34.234229936448,19.975873233453,12.948350331433,1.061492188504,12.514823960699,67.311651853235,69.049579574544,66.276273887369,56.536261199233");
  compareCsvLine(line.getRow(5),"EN1021761181M,EN1036942554M,0.34162153234608,0.87387942998419,2013260,2014071,232.52801191399,34.144540354162,232.5111761136,34.282349809786,19.975873233453,12.479301137269,1.061492188504,1.2458226380849,67.311651853235,67.959527732031,66.276273887369,66.729468790675");
  compareCsvLine(line.getRow(6),"EN1021761181M,EN1036913743M,0.045073091457237,0.10584840781994,2013260,2014071,232.52801191399,34.144540354162,232.26374005244,34.496838871054,19.975873233453,12.85282457611,1.061492188504,12.54727925943,67.311651853235,69.08345992827,66.276273887369,56.537696956496");
  compareCsvLine(line.getRow(7),"EN1021761181M,EN1036942558M,5.5723000148695e-05,1.44255410502414e-04,2013260,2014071,232.52801191399,34.144540354162,232.54511653405,34.546667910972,19.975873233453,12.385246277081,1.061492188504,1.2903552439122,67.311651853235,68.005843233514,66.276273887369,66.730286246905");
  compareCsvLine(line.getRow(8),"EN1021761181M,EN1021761189M,0.061662276901637,0.062613088394135,2013260,2013260,232.52801191399,34.144540354162,232.57212232368,34.594082884512,19.975873233453,19.770215210919,1.061492188504,1.1506540403403,67.311651853235,67.404957936665,66.276273887369,66.27844012357");

  /////////////////////////////////////////////////////////////////
  // EN1021761189M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1021761189M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 18);
  ASSERT_EQ(line.rows(), 9);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1021761189M,EN1021761181M,0.062613088394135,0.061662276901637,2013260,2013260,232.57212232368,34.594082884512,232.52801191399,34.144540354162,19.770215210919,19.975873233453,1.1506540403403,1.061492188504,67.404957936665,67.311651853235,66.27844012357,66.276273887369");
  compareCsvLine(line.getRow(2), "EN1021761189M,EN1036942554M,0.099644398409847,0.25102304943863,2013260,2014071,232.57212232368,34.594082884512,232.5111761136,34.282349809786,19.770215210919,12.479301137269,1.1506540403403,1.2458226380849,67.404957936665,67.959527732031,66.27844012357,66.729468790675");
  compareCsvLine(line.getRow(3), "EN1021761189M,EN1036913743M,0.17985409460908,0.41595058954094,2013260,2014071,232.57212232368,34.594082884512,232.26374005244,34.496838871054,19.770215210919,12.85282457611,1.1506540403403,12.54727925943,67.404957936665,69.08345992827,66.27844012357,56.537696956496");
  compareCsvLine(line.getRow(4), "EN1021761189M,EN1036942558M,0.39223644923758,1.0,2013260,2014071,232.57212232368,34.594082884512,232.54511653405,34.546667910972,19.770215210919,12.385246277081,1.1506540403403,1.2903552439122,67.404957936665,68.005843233514,66.27844012357,66.730286246905");
  compareCsvLine(line.getRow(5), "EN1021761189M,EN1036913747M,0.17825961289758,0.41704405870308,2013260,2014071,232.57212232368,34.594082884512,232.3069838914,34.760416375565,19.770215210919,12.757750410871,1.1506540403403,12.583845501599,67.404957936665,69.118631749872,66.27844012357,56.536297017165");
  compareCsvLine(line.getRow(6), "EN1021761189M,EN1036942562M,0.23029667852461,0.59419081049821,2013260,2014071,232.57212232368,34.594082884512,232.57910407091,34.811449361952,19.770215210919,12.291737281869,1.1506540403403,1.3367113362227,67.404957936665,68.052908360044,66.27844012357,66.730311688061");
  compareCsvLine(line.getRow(7), "EN1021761189M,EN1021761196M,0.14661687464036,0.1485837898998,2013260,2013260,232.57212232368,34.594082884512,232.61050123036,34.989896178512,19.770215210919,19.591558441031,1.1506540403403,1.2334349244097,67.404957936665,67.488961290993,66.27844012357,66.277837510067");
  compareCsvLine(line.getRow(8), "EN1021761189M,EN1036913751M,0.012184477158529,0.028835489205492,2013260,2014071,232.57212232368,34.594082884512,232.35048265759,35.024725780293,19.770215210919,12.663340839084,1.1506540403403,12.621046696046,67.404957936665,69.154484278709,66.27844012357,56.53493426309");

  /////////////////////////////////////////////////////////////////
  // EN1021761238M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1021761238M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 8);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1021761238M,EN1021761231M,0.091050134253178,0.089878418837092,2013260,2013260,232.85483232838,37.400758642813,232.81171674128,36.99456109644,18.545314633428,18.716523036416,1.7593072870187,1.670348344096,68.022425993194,67.930419034114,66.278859151431,66.27667143939");
  compareCsvLine(line.getRow(2), "EN1021761238M,EN1036913783M,0.15116711616998,0.35747562391835,2013260,2014071,232.85483232838,37.400758642813,232.70968042425,37.160874336858,18.545314633428,11.927721183266,1.7593072870187,12.928442371948,68.022425993194,69.463780537612,66.278859151431,56.536796449239");
  compareCsvLine(line.getRow(3), "EN1021761238M,EN1036942598M,0.27657074024146,0.724098320609,2013260,2014071,232.85483232838,37.400758642813,232.90012204755,37.22313072135,18.545314633428,11.475069621568,1.7593072870187,1.7823793327341,68.022425993194,68.503131344896,66.278859151431,66.731125731603");
  compareCsvLine(line.getRow(4), "EN1021761238M,EN1036913787M,0.40763067267025,0.97493964433015,2013260,2014071,232.85483232838,37.400758642813,232.75614488035,37.430801068496,18.545314633428,11.838125612084,1.7593072870187,12.968023719737,68.022425993194,69.505286807968,66.278859151431,56.538706419214");
  compareCsvLine(line.getRow(5), "EN1021761238M,EN1036942602M,0.34505762309655,0.91407693140774,2013260,2014071,232.85483232838,37.400758642813,232.93766309208,37.494056190768,18.545314633428,11.387159152199,1.7593072870187,1.834415608204,68.022425993194,68.556024720098,66.278859151431,66.731875482359");
  compareCsvLine(line.getRow(6), "EN1021761238M,EN1036913791M,0.10762209593955,0.26030123964735,2013260,2014071,232.85483232838,37.400758642813,232.80182514214,37.701645604188,18.545314633428,11.749557286997,1.7593072870187,13.013754313889,68.022425993194,69.548385984313,66.278859151431,56.536052846333");
  compareCsvLine(line.getRow(7), "EN1021761238M,EN1021761245M,0.080348100994626,0.081387014303031,2013260,2013260,232.85483232838,37.400758642813,232.89732803313,37.808931981601,18.545314633428,18.375606266129,1.7593072870187,1.8544868351596,68.022425993194,68.116985196354,66.278859151431,66.277498316909");

  /////////////////////////////////////////////////////////////////
  // EN1021761290M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1021761290M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1021761290M,EN1021761284M,0.12945104014642,0.12809580269899,2013260,2013260,233.1933517066,40.475966044329,233.15175088706,40.11664057133,17.315199273183,17.453463830469,2.4896150747464,2.4022297786106,68.757715846472,68.668942166113,66.278959487345,66.277812257336");
  compareCsvLine(line.getRow(2), "EN1021761290M,EN1036913828M,0.1624089718692,0.39633406260925,2013260,2014071,233.1933517066,40.475966044329,233.24410210064,40.233659466114,17.315199273183,10.956122626477,2.4896150747464,13.438848000537,68.757715846472,69.975661819658,66.278959487345,56.538172369907");
  compareCsvLine(line.getRow(3), "EN1021761290M,EN1052094598M,0.047922378552444,0.66257900097914,2013260,2014246,233.1933517066,40.475966044329,232.89216381581,40.388431578129,17.315199273183,4.1882143471923,2.4896150747464,36.172578724374,68.757715846472,70.840088829318,66.278959487345,34.668805456926");
  compareCsvLine(line.getRow(4), "EN1021761290M,EN1052152459M,0.024282477349162,0.60300159890788,2013260,2014247,233.1933517066,40.475966044329,232.96027962448,40.573401182751,17.315199273183,3.3649694822407,2.4896150747464,20.231362081474,68.757715846472,69.260416542782,66.278959487345,89.489172701106");
  compareCsvLine(line.getRow(5), "EN1021761290M,EN1052094602M,0.0241281962346,0.34572332646292,2013260,2014246,233.1933517066,40.475966044329,232.99520411868,40.709640806713,17.315199273183,4.1037320954415,2.4896150747464,36.192848508932,68.757715846472,70.860798087726,66.278959487345,34.669258942779");
  compareCsvLine(line.getRow(6), "EN1021761290M,EN1021761296M,0.11910490872416,0.12035110723934,2013260,2013260,233.1933517066,40.475966044329,233.23527735846,40.837238659147,17.315199273183,17.178146624313,2.4896150747464,2.580004872432,68.757715846472,68.848065012877,66.278959487345,66.278460195165");

  /////////////////////////////////////////////////////////////////
  // EN1036913727M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1036913727M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1036913727M,EN1036942538M,0.041116322495792,0.044713739792859,2014071,2014071,232.09083352732,33.449410395126,232.38036751628,33.233325419827,13.238354232817,12.860832904636,12.417383593934,1.0659209738681,68.95132850983,67.779062166713,56.535486838215,66.731542777743");
  compareCsvLine(line.getRow(2), "EN1036913727M,EN1036942542M,0.13094547745014,0.14412157859908,2014071,2014071,232.09083352732,33.449410395126,232.41264255462,33.49422271561,13.238354232817,12.764654059661,12.417383593934,1.1096486973153,68.95132850983,67.8231907161,56.535486838215,66.731565597755");
  compareCsvLine(line.getRow(3), "EN1036913727M,EN1036913731M,0.15633345821687,0.15815521897189,2014071,2014071,232.09083352732,33.449410395126,232.13394667263,33.710392048372,13.238354232817,13.14122632122,12.417383593934,12.448154132674,68.95132850983,68.983210406497,56.535486838215,56.536579183221");

  /////////////////////////////////////////////////////////////////
  // EN1036913759M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1036913759M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1036913759M,EN1036913755M,0.10292431847331,0.10175180430064,2014071,2014071,232.43909522009,35.555037207238,232.39477806809,35.288954213534,12.475989820425,12.569331545352,12.692806872054,12.655214974985,69.2274574368,69.190379776408,56.536121923413,56.536702767677");
  compareCsvLine(line.getRow(2), "EN1036913759M,EN1036942570M,0.11249957521749,0.12275692060386,2014071,2014071,232.43909522009,35.555037207238,232.64821842047,35.342450125509,12.475989820425,12.106351049889,12.692806872054,1.4300253136977,69.2274574368,68.14853194938,56.536121923413,66.731813945252");
  compareCsvLine(line.getRow(3), "EN1036913759M,EN1021761203M,0.51026614180508,0.21639720793304,2014071,2013260,232.43909522009,35.555037207238,232.64978188942,35.386951375898,12.475989820425,19.414103010153,12.692806872054,1.316521939233,69.2274574368,67.574011698386,56.536121923413,66.278559456646");
  compareCsvLine(line.getRow(4), "EN1036913759M,EN1036942574M,0.24802618357446,0.27387822440164,2014071,2014071,232.43909522009,35.555037207238,232.68253238032,35.609105909121,12.475989820425,12.014500679504,12.692806872054,1.4814489198249,69.2274574368,68.197860614499,56.536121923413,66.729336958733");
  compareCsvLine(line.getRow(5), "EN1036913759M,EN1021761210M,0.28078919984918,0.12066401200529,2014071,2013260,232.43909522009,35.555037207238,232.68967684356,35.785794058878,12.475989820425,19.237869860513,12.692806872054,1.4011374002054,69.2274574368,67.660513881622,56.536121923413,66.279347719659");
  compareCsvLine(line.getRow(6), "EN1036913759M,EN1036913763M,0.096545094197906,0.097658083801241,2014071,2014071,232.43909522009,35.555037207238,232.48369735373,35.820901976881,12.475989820425,12.383144444574,12.692806872054,12.729547444305,69.2274574368,69.264956863062,56.536121923413,56.536892545146");

  /////////////////////////////////////////////////////////////////
  // EN1036913791M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1036913791M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1036913791M,EN1021761238M,0.26030123964735,0.10762209593955,2014071,2013260,232.80182514214,37.701645604188,232.85483232838,37.400758642813,11.749557286997,18.545314633428,13.013754313889,1.7593072870187,69.548385984313,68.022425993194,56.536052846333,66.278859151431");
  compareCsvLine(line.getRow(2), "EN1036913791M,EN1036913787M,0.041927068776367,0.041460122907473,2014071,2014071,232.80182514214,37.701645604188,232.75614488035,37.430801068496,11.749557286997,11.838125612084,13.013754313889,12.968023719737,69.548385984313,69.505286807968,56.536052846333,56.538706419214");
  compareCsvLine(line.getRow(3), "EN1036913791M,EN1036942602M,0.15860289741585,0.17371105127614,2014071,2014071,232.80182514214,37.701645604188,232.93766309208,37.494056190768,11.749557286997,11.387159152199,13.013754313889,1.834415608204,69.548385984313,68.556024720098,56.536052846333,66.731875482359");
  compareCsvLine(line.getRow(4), "EN1036913791M,EN1036942606M,0.3585534215049,0.39733770719091,2014071,2014071,232.80182514214,37.701645604188,232.97538316574,37.76637043429,11.749557286997,11.29973521337,13.013754313889,1.8891166052652,69.548385984313,68.609905717125,56.536052846333,66.730505030711");
  compareCsvLine(line.getRow(5), "EN1036913791M,EN1021761245M,0.83747772333887,0.35073408449717,2014071,2013260,232.80182514214,37.701645604188,232.89732803313,37.808931981601,11.749557286997,18.375606266129,13.013754313889,1.8544868351596,69.548385984313,68.116985196354,56.536052846333,66.277498316909");
  compareCsvLine(line.getRow(6), "EN1036913791M,EN1036913795M,0.035231856394076,0.035631339063128,2014071,2014071,232.80182514214,37.701645604188,232.84876256554,37.972399230802,11.749557286997,11.661128293816,13.013754313889,13.054381825194,69.548385984313,69.591243121129,56.536052846333,56.538306452889");

  /////////////////////////////////////////////////////////////////
  // EN1036942534M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1036942534M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1036942534M,EN1036942538M,0.14379665623515,0.14553400482145,2014071,2014071,232.34794321125,32.973118396829,232.38036751628,33.233325419827,12.957542890185,12.860832904636,1.0249278389099,1.0659209738681,67.735873059761,67.779062166713,66.729817340074,66.731542777743");

  /////////////////////////////////////////////////////////////////
  // EN1036942566M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1036942566M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1036942566M,EN1036942562M,0.092441159045687,0.091344596357673,2014071,2014071,232.61332305194,35.07641633233,232.57910407091,34.811449361952,12.198773155023,12.291737281869,1.3837382835867,1.3367113362227,68.100502666401,68.052908360044,66.730657834365,66.730311688061");
  compareCsvLine(line.getRow(2), "EN1036942566M,EN1021761196M,1.0,0.38812060203568,2014071,2013260,232.61332305194,35.07641633233,232.61050123036,34.989896178512,12.198773155023,19.591558441031,1.3837382835867,1.2334349244097,68.100502666401,67.488961290993,66.730657834365,66.277837510067");
  compareCsvLine(line.getRow(3), "EN1036942566M,EN1036913751M,0.24057085244014,0.21804325421648,2014071,2014071,232.61332305194,35.07641633233,232.35048265759,35.024725780293,12.198773155023,12.663340839084,1.3837382835867,12.621046696046,68.100502666401,69.154484278709,66.730657834365,56.53493426309");
  compareCsvLine(line.getRow(4), "EN1036942566M,EN1036913755M,0.11546647775654,0.10586667112637,2014071,2014071,232.61332305194,35.07641633233,232.39477806809,35.288954213534,12.198773155023,12.569331545352,1.3837382835867,12.655214974985,68.100502666401,69.190379776408,66.730657834365,56.536702767677");
  compareCsvLine(line.getRow(5), "EN1036942566M,EN1036942570M,0.08252411468653,0.083513213837328,2014071,2014071,232.61332305194,35.07641633233,232.64821842047,35.342450125509,12.198773155023,12.106351049889,1.3837382835867,1.4300253136977,68.100502666401,68.14853194938,66.730657834365,66.731813945252");
  compareCsvLine(line.getRow(6), "EN1036942566M,EN1021761203M,0.25691967793054,0.10104887666314,2014071,2013260,232.61332305194,35.07641633233,232.64978188942,35.386951375898,12.198773155023,19.414103010153,1.3837382835867,1.316521939233,68.100502666401,67.574011698386,66.730657834365,66.278559456646");

  /////////////////////////////////////////////////////////////////
  // EN1036942598M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1036942598M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1036942598M,EN1036942594M,0.029325568857502,0.028983076276192,2014071,2014071,232.90012204755,37.22313072135,232.86261942936,36.952434253844,11.475069621568,11.563455759973,1.7823793327341,1.7321665565906,68.503131344896,68.450977906316,66.731125731603,66.729639743768");
  compareCsvLine(line.getRow(2), "EN1036942598M,EN1021761231M,0.44705253417341,0.16855517314363,2014071,2013260,232.90012204755,37.22313072135,232.81171674128,36.99456109644,11.475069621568,18.716523036416,1.7823793327341,1.670348344096,68.503131344896,67.930419034114,66.731125731603,66.27667143939");
  compareCsvLine(line.getRow(3), "EN1036942598M,EN1036913783M,0.36646598323784,0.33100264531544,2014071,2014071,232.90012204755,37.22313072135,232.70968042425,37.160874336858,11.475069621568,11.927721183266,1.7823793327341,12.928442371948,68.503131344896,69.463780537612,66.731125731603,56.536796449239");
  compareCsvLine(line.getRow(4), "EN1036942598M,EN1021761238M,0.724098320609,0.27657074024146,2014071,2013260,232.90012204755,37.22313072135,232.85483232838,37.400758642813,11.475069621568,18.545314633428,1.7823793327341,1.7593072870187,68.503131344896,68.022425993194,66.731125731603,66.278859151431");
  compareCsvLine(line.getRow(5), "EN1036942598M,EN1036913787M,0.17045886747787,0.15571812494104,2014071,2014071,232.90012204755,37.22313072135,232.75614488035,37.430801068496,11.475069621568,11.838125612084,1.7823793327341,12.968023719737,68.503131344896,69.505286807968,66.731125731603,56.538706419214");
  compareCsvLine(line.getRow(6), "EN1036942598M,EN1036942602M,0.022733226779083,0.02300175104202,2014071,2014071,232.90012204755,37.22313072135,232.93766309208,37.494056190768,11.475069621568,11.387159152199,1.7823793327341,1.834415608204,68.503131344896,68.556024720098,66.731125731603,66.731875482359");

  /////////////////////////////////////////////////////////////////
  // EN1052152435M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1052152435M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1052152435M,EN1036913803M,0.27134140981115,0.031350533278149,2014247,2014071,232.74422187417,38.573733199403,232.94218295549,38.516723718965,3.8252260358171,11.48656199809,20.704391422767,13.145516866747,68.784299333367,69.680349630327,89.486079833771,56.536266792475");
  compareCsvLine(line.getRow(2), "EN1052152435M,EN1021761259M,1.0,0.047997704011156,2014247,2013260,232.74422187417,38.573733199403,232.98624673467,38.631403712351,3.8252260358171,18.039591121066,20.704391422767,2.0444390943557,68.784299333367,68.309556318414,89.486079833771,66.278265253193");

  /////////////////////////////////////////////////////////////////
  // EN1052152463M.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN1052152463M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),18);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0), "SourceProductIdA,SourceProductIdB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1), "EN1052152463M,EN1021761296M,0.6797099405155,0.026583693612234,2014247,2013260,233.00002788354,40.909124819378,233.23527735846,40.837238659147,3.2915173575723,17.178146624313,20.150638415388,2.580004872432,69.341262947896,68.848065012877,89.489333151681,66.278460195165");
}


/**
   * IsisminerTestGisUnion
   * 
   * Tests GIS union behavior
   * 
   * INPUT: 1) 10 csv files:
   *           gisUnion_data0_0.csv; gisUnion_data0_1.csv; gisUnion_data1_0.csv;
   *           gisUnion_data1_1.csv; gisUnion_data2_0.csv; gisUnion_data2_1.csv;
   *           gisUnion_data3_0.csv; gisUnion_data3_1.csv; gisUnion_data4_0.csv;
   *           gisUnion_data4_1.csv; gisUnion_data5_0.csv; gisUnion_data5_1.csv;
   *        2) 6 configuration (conf) files:
   *           gisUnion_test0.conf; gisUnion_test1.conf; gisUnion_test2.conf;
   *           gisUnion_test3.conf; gisUnion_test4.conf; gisUnion_test5.conf
   * 
   * OUTPUT: 12 csv files:
   *            EN1021761181M.csv; EN1021761238M.csv; EN1021761290M.csv;
   *            EN1036913727M.csv; EN1036913759M.csv; EN1036913791M.csv;
   *            EN1036942534M.csv; EN1036942566M.csv; EN1036942598M.csv;
   *            EN1052094505M.csv; EN1052152435M.csv; EN1052152463M.csv
   */  
TEST(Isisminer, IsisminerTestGisUnion) {
  QTemporaryDir tempDir;

  // create 12 input csv files
  ofstream of;
  of.open(tempDir.path().toStdString() + "/gisUnion_data0_0.csv");
  of << "GisGeometry;\nPOLYGON ((0 1,1 3,2 1,0 1));";
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data0_1.csv");
  of << "GisGeometry;\nLINESTRING(3 2, 3 3);"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data1_0.csv");
  of << "GisGeometry;\nPOLYGON((1 1,1 2,2 2,2 1,1 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data1_1.csv");
  of << "GisGeometry;\nPOLYGON((1 3,1 4,2 4,2 3,1 3));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data2_0.csv");
  of << "GisGeometry;\nPOLYGON((1 1,1 2,2 2,2 1,1 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data2_1.csv");
  of << "GisGeometry;\nPOLYGON((1 1,1 2,2 2,2 1,1 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data3_0.csv");
  of << "GisGeometry;\nPOLYGON((1 1,1 2,2 2,2 1,1 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data3_1.csv");
  of << "GisGeometry;\nPOLYGON((2 1,2 2,3 2,3 1,2 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data4_0.csv");
  of << "GisGeometry;\nPOLYGON((0 1,0 4,3 4,3 1,0 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data4_1.csv");
  of << "GisGeometry;\nPOLYGON((1 2,1 3,4 3,4 2,1 2));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data5_0.csv");
  of << "GisGeometry;\nPOLYGON((1 1,1 2,2 2,2 1,1 1));"; 
  of.close();
  of.open(tempDir.path().toStdString() + "/gisUnion_data5_1.csv");
  of << "GisGeometry;\nPOLYGON((2 2,2 3,3 3,3 2,2 2));"; 
  of.close();

  // create 6 input config files in tempDir
  // gisUnion_test0.conf
  Pvl conf0;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "GisUnionStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadFilterData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data0_0.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", "\";\""));
    CsvReader.addKeyword(PvlKeyword("GisGeometryRef", "GisGeometry"));
    CsvReader.addKeyword(PvlKeyword("GisType", "WKT"));
  isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add 2nd CsvReader strategy
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data0_1.csv\""), Pvl::Replace);
  isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add GisUnion strategy
    PvlObject GisUnion("Strategy");
    GisUnion.addKeyword(PvlKeyword("Name", "Union0"));
    GisUnion.addKeyword(PvlKeyword("Type", "GisUnion"));
    GisUnion.addKeyword(PvlKeyword("RatioRef", "\"CustomRatioKey\"")); // optional parameter
  isisminerObject.addObject(GisUnion);

    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestExclude"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/test0.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(GisGeometry,CustomRatioKey)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", "\";\""));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
  isisminerObject.addObject(CsvWriter);
  
  conf0.addObject(isisminerObject);
  conf0.write(tempDir.path() + "/gisUnion_test0.conf");

  // gisUnion_test1.conf
  Pvl conf1;
  PvlObject isisminerObject1("IsisMiner");
  isisminerObject1.addKeyword(PvlKeyword("Name", "GisUnionStrategyTest"));
  isisminerObject1.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data1_0.csv\""), Pvl::Replace);
  isisminerObject1.addObject(CsvReader);
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data1_1.csv\""), Pvl::Replace);
  isisminerObject1.addObject(CsvReader);
  
  isisminerObject1.addObject(GisUnion);
  
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/test1.csv\""), Pvl::Replace);
  isisminerObject1.addObject(CsvWriter);

  conf1.addObject(isisminerObject1);  
  conf1.write(tempDir.path() + "/gisUnion_test1.conf");

  // gisUnion_test2.conf
  Pvl conf2;
  PvlObject isisminerObject2("IsisMiner");
  isisminerObject2.addKeyword(PvlKeyword("Name", "GisUnionStrategyTest"));
  isisminerObject2.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data2_0.csv\""), Pvl::Replace);
  isisminerObject2.addObject(CsvReader);
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data2_1.csv\""), Pvl::Replace);
  isisminerObject2.addObject(CsvReader);
  
  isisminerObject2.addObject(GisUnion);
  
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/test2.csv\""), Pvl::Replace);
  isisminerObject2.addObject(CsvWriter);

  conf2.addObject(isisminerObject2);  
  conf2.write(tempDir.path() + "/gisUnion_test2.conf");
  
  // gisUnion_test3.conf
  Pvl conf3;
  PvlObject isisminerObject3("IsisMiner");
  isisminerObject3.addKeyword(PvlKeyword("Name", "GisUnionStrategyTest"));
  isisminerObject3.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data3_0.csv\""), Pvl::Replace);
  isisminerObject3.addObject(CsvReader);
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data3_1.csv\""), Pvl::Replace);
  isisminerObject3.addObject(CsvReader);
  
  isisminerObject3.addObject(GisUnion);
  
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/test3.csv\""), Pvl::Replace);
  isisminerObject3.addObject(CsvWriter);

  conf3.addObject(isisminerObject3);  
  conf3.write(tempDir.path() + "/gisUnion_test3.conf");

  // gisUnion_test4.conf
  Pvl conf4;
  PvlObject isisminerObject4("IsisMiner");
  isisminerObject4.addKeyword(PvlKeyword("Name", "GisUnionStrategyTest"));
  isisminerObject4.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data4_0.csv\""), Pvl::Replace);
  isisminerObject4.addObject(CsvReader);
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data4_1.csv\""), Pvl::Replace);
  isisminerObject4.addObject(CsvReader);
  
  isisminerObject4.addObject(GisUnion);
  
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/test4.csv\""), Pvl::Replace);
  isisminerObject4.addObject(CsvWriter);

  conf4.addObject(isisminerObject4);  
  conf4.write(tempDir.path() + "/gisUnion_test4.conf");

  // gisUnion_test5.conf
  Pvl conf5;
  PvlObject isisminerObject5("IsisMiner");
  isisminerObject5.addKeyword(PvlKeyword("Name", "GisUnionStrategyTest"));
  isisminerObject5.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data5_0.csv\""), Pvl::Replace);
  isisminerObject5.addObject(CsvReader);
  
  CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/gisUnion_data5_1.csv\""), Pvl::Replace);
  isisminerObject5.addObject(CsvReader);
  
  isisminerObject5.addObject(GisUnion);
  
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/test5.csv\""), Pvl::Replace);
  isisminerObject5.addObject(CsvWriter);

  conf5.addObject(isisminerObject5);  
  conf5.write(tempDir.path() + "/gisUnion_test5.conf");

  // run isisminer for each example (6 times)
  QVector<QString> args = {"config=" + tempDir.path() + "/gisUnion_test0.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui0(APP_XML, args);

  try {
    isisminer(ui0);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  args = {"config=" + tempDir.path() + "/gisUnion_test1.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui1(APP_XML, args);

  try {
    isisminer(ui1);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  args = {"config=" + tempDir.path() + "/gisUnion_test2.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui2(APP_XML, args);

  try {
    isisminer(ui2);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  args = {"config=" + tempDir.path() + "/gisUnion_test3.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui3(APP_XML, args);

  try {
    isisminer(ui3);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  args = {"config=" + tempDir.path() + "/gisUnion_test4.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui4(APP_XML, args);

  try {
    isisminer(ui4);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  args = {"config=" + tempDir.path() + "/gisUnion_test5.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui5(APP_XML, args);

  try {
    isisminer(ui5);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // test0.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/test0.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"GisGeometry;CustomRatioKey", ';');
  compareCsvLine(line.getRow(1),"POLYGON ((0 1,1 3,2 1,0 1));1.0", ';');
  compareCsvLine(line.getRow(2),"LINESTRING(3 2, 3 3);0.0", ';');

  /////////////////////////////////////////////////////////////////
  // test1.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/test1.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"GisGeometry;CustomRatioKey", ';');
  compareCsvLine(line.getRow(1),"POLYGON((1 1,1 2,2 2,2 1,1 1));1.0", ';');
  compareCsvLine(line.getRow(2),"POLYGON((1 3,1 4,2 4,2 3,1 3));0.0", ';');

  /////////////////////////////////////////////////////////////////
  // test2.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/test2.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"GisGeometry;CustomRatioKey", ';');
  compareCsvLine(line.getRow(1),"POLYGON((1 1,1 2,2 2,2 1,1 1));1.0", ';');
  compareCsvLine(line.getRow(2),"POLYGON((1 1,1 2,2 2,2 1,1 1));1.0", ';');

  /////////////////////////////////////////////////////////////////
  // test3.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/test3.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"GisGeometry;CustomRatioKey", ';');
  compareCsvLine(line.getRow(1),"POLYGON((1 1,1 2,2 2,2 1,1 1));1.0", ';');
  compareCsvLine(line.getRow(2),"POLYGON((2 1,2 2,3 2,3 1,2 1));0.0", ';');

  /////////////////////////////////////////////////////////////////
  // test4.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/test4.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"GisGeometry;CustomRatioKey", ';');
  compareCsvLine(line.getRow(1),"POLYGON((0 1,0 4,3 4,3 1,0 1));1.0", ';');
  compareCsvLine(line.getRow(2),"POLYGON((1 2,1 3,4 3,4 2,1 2));0.66666666666667", ';');

  /////////////////////////////////////////////////////////////////
  // test5.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/test5.csv",
                             false, 0, ';', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"GisGeometry;CustomRatioKey", ';');
  compareCsvLine(line.getRow(1),"POLYGON((1 1,1 2,2 2,2 1,1 1));1.0", ';');
  compareCsvLine(line.getRow(2),"POLYGON((2 2,2 3,3 3,3 2,2 2));0.0", ';');
}


/**
   * IsisminerTestIsNumeric
   * 
   * Tests a variety of input numeric data
   * 
   * INPUT: 1) isnumeric_data.csv
   *        2) isnumeric_test.conf
   * 
   * OUTPUT: 1) isnumeric_test_all.csv
   *         2) isnumeric_test_key_dne.csv (non-existent key)
   *         3) isnumeric_test_none.csv (no data present)
   *         4) isnumeric_test_some.csv
   */  
TEST(Isisminer, IsisminerTestIsNumeric) {
  QTemporaryDir tempDir;

  // create input file isnumeric_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/isnumeric_data.csv");
  of << "Respondent,Age,Gender,Height\n";
  of << "1,30,NoData,60.0\n";
  of << "2,NoData,M,68.2\n";
  of << "3,14,F,58.1\n";
  of << "4,23,F,71.9\n";
  of << "5,51,M,72.0\n";
  of << "6,NoData,M,63.0\n";
  of.close();

  // create input config file isnumeric_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "IsNumericStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadIsNumericData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/isnumeric_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1_%2\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId,StereoSource)"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add IsNumeric strategy
    PvlObject IsNumeric("Strategy");
    IsNumeric.addKeyword(PvlKeyword("Name", "TestSomeNumeric"));
    IsNumeric.addKeyword(PvlKeyword("Type", "IsNumeric"));
    IsNumeric.addKeyword(PvlKeyword("Description", "Keep all numeric ages"));
    IsNumeric.addKeyword(PvlKeyword("Keyword", "Age"));
    isisminerObject.addObject(IsNumeric);

    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestInclude"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/isnumeric_test_some.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Respondent,Age,Gender,Height)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
    isisminerObject.addObject(CsvWriter);
  
    /////////////////////////////////////////////////////////////////
    // add ResourceManager strategy
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for multi-Include test"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    isisminerObject.addObject(ResourceManager);
  
    /////////////////////////////////////////////////////////////////
    // add 2nd IsNumeric strategy
    IsNumeric.addKeyword(PvlKeyword("Name", "TestNoneNumeric"), Pvl::Replace);
    IsNumeric.addKeyword(PvlKeyword("Description", "Keep all numeric genders, which is none."), Pvl::Replace);
    IsNumeric.addKeyword(PvlKeyword("Keyword", "Gender"), Pvl::Replace);
    isisminerObject.addObject(IsNumeric);
  
    /////////////////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/isnumeric_test_none.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);
  
    /////////////////////////////////////////////////////////////////
    // add 2nd ResourceManager strategy
    isisminerObject.addObject(ResourceManager);
  
    /////////////////////////////////////////////////////////////////
    // add 3rd IsNumeric strategy
    IsNumeric.addKeyword(PvlKeyword("Name", "TestAllNumeric"), Pvl::Replace);
    IsNumeric.addKeyword(PvlKeyword("Description", "Keep all numeric heights, which is all of them."), Pvl::Replace);
    IsNumeric.addKeyword(PvlKeyword("Keyword", "Height"), Pvl::Replace);
    isisminerObject.addObject(IsNumeric);

    /////////////////////////////////////////////////////////////////
    // add 3rd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/isnumeric_test_all.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 4th IsNumeric strategy
    IsNumeric.addKeyword(PvlKeyword("Name", "TestKeyDNE"), Pvl::Replace);
    IsNumeric.addKeyword(PvlKeyword("Description", "Try to sort on a key that does not exist"), Pvl::Replace);
    IsNumeric.addKeyword(PvlKeyword("Keyword", "FavoriteColor"), Pvl::Replace);
    isisminerObject.addObject(IsNumeric);

    /////////////////////////////////////////////////////////////////
    // add 4th CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/isnumeric_test_key_dne.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Respondent,Age,Gender,Height,FavoriteColor)"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/isnumeric_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/isnumeric_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // isnumeric_test_none.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/isnumeric_test_none.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 1);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");

  /////////////////////////////////////////////////////////////////
  // isnumeric_test_some.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/isnumeric_test_some.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(2),"3,14,F,58.1");
  compareCsvLine(line.getRow(3),"4,23,F,71.9");
  compareCsvLine(line.getRow(4),"5,51,M,72.0");

  /////////////////////////////////////////////////////////////////
  // isnumeric_test_all.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/isnumeric_test_all.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(2),"2,NoData,M,68.2");
  compareCsvLine(line.getRow(3),"3,14,F,58.1");
  compareCsvLine(line.getRow(4),"4,23,F,71.9");
  compareCsvLine(line.getRow(5),"5,51,M,72.0");
  compareCsvLine(line.getRow(6),"6,NoData,M,63.0");

  /////////////////////////////////////////////////////////////////
  // isnumeric_test_key_dne.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/isnumeric_test_key_dne.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 5);
  ASSERT_EQ(line.rows(), 1);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height,FavoriteColor");  
}


/**
   * IsisminerTestLimit
   * 
   * Tests a variety of input numeric data
   * 
   * INPUT: 1) limit_data.csv
   *        2) limit_test.conf
   * 
   * OUTPUT: 1) limit_test_between.csv
   *         2) limit_test_equal.csv
   *         3) limit_test_greater.csv
   *         4) limit_test_less.csv
   */  
TEST(Isisminer, IsisminerTestLimit) {
  QTemporaryDir tempDir;

  // create input file limit_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/limit_data.csv");
  of << "Respondent,Age,Gender,Height\n";
  of << "1,30,NoData,60.0\n";
  of << "2,62,M,68.2\n";
  of << "3,14,F,58.1\n";
  of << "4,23,F,71.9\n";
  of << "5,51,M,72.0\n";
  of << "6,5,M,63.0\n";
  of.close();

  // create input config file limit_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "LimitStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadLimitData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/limit_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1_%2\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId,StereoSource)"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add TestGreaterThan Limit strategy
    PvlObject Limit("Strategy");
    Limit.addKeyword(PvlKeyword("Name", "TestGreaterThan"));
    Limit.addKeyword(PvlKeyword("Type", "Limit"));
    Limit.addKeyword(PvlKeyword("Equation", "((Age > 21))"));
    isisminerObject.addObject(Limit);

    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestInclude"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/limit_test_greater.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Respondent,Age,Gender,Height)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
    isisminerObject.addObject(CsvWriter);
  
    /////////////////////////////////////////////////////////////////
    // add ResourceManager strategy
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for multi-Include test"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add TestLessThan Limit strategy
    Limit.addKeyword(PvlKeyword("Name", "TestLessThan"), Pvl::Replace);
    Limit.addKeyword(PvlKeyword("Equation", "((Age < 50))"), Pvl::Replace);
    isisminerObject.addObject(Limit);
  
    /////////////////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/limit_test_less.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);
  
    /////////////////////////////////////////////////////////////////
    // add 2nd ResourceManager strategy
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add TestEqualTo Limit strategy
    Limit.addKeyword(PvlKeyword("Name", "TestEqualTo"), Pvl::Replace);
    Limit.addKeyword(PvlKeyword("Equation", "((Age == 30))"), Pvl::Replace);
    isisminerObject.addObject(Limit);

    /////////////////////////////////////////////////////////////////
    // add 3rd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/limit_test_equal.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 3rd ResourceManager strategy
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add TestBetween Limit strategy
    Limit.addKeyword(PvlKeyword("Name", "TestBetween"), Pvl::Replace);
    Limit.addKeyword(PvlKeyword("Equation", "((Age > 10) and (Age < 50))"), Pvl::Replace);
    isisminerObject.addObject(Limit);

  /////////////////////////////////////////////////////////////////
  // add 4th CsvWriter strategy
  CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/limit_test_between.csv\""), Pvl::Replace);
  isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/limit_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/limit_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // limit_test_between.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/limit_test_between.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(2),"3,14,F,58.1");
  compareCsvLine(line.getRow(3),"4,23,F,71.9");

  /////////////////////////////////////////////////////////////////
  // limit_test_equal.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/limit_test_equal.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"1,30,NoData,60.0");

  /////////////////////////////////////////////////////////////////
  // limit_test_greater.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/limit_test_greater.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(2),"2,62,M,68.2");
  compareCsvLine(line.getRow(3),"4,23,F,71.9");
  compareCsvLine(line.getRow(4),"5,51,M,72.0");

  /////////////////////////////////////////////////////////////////
  // limit_test_less.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/limit_test_less.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(2),"3,14,F,58.1");
  compareCsvLine(line.getRow(3),"4,23,F,71.9");
  compareCsvLine(line.getRow(4),"6,5,M,63.0");
}


/**
   * IsisminerTestMainProgramGlobals
   * 
   * Test isisminor application with empty globals.pvl provided.
   * 
   * INPUT: 1) filter_data.csv
   *        2) globals.pvl
   *        3) mainProgram_test.conf
   * 
   * OUTPUT: redirected from std::cout to mainProgram_globals_out.txt
   */  
TEST_F(IsisminerMainProgram, IsisminerTestMainProgramGlobals) {

  // create empty input file globals.pvl (should generate an error)
  ofstream of;
  of.open(tempDir.path().toStdString() + "/globals.pvl");
  of.close();

  // redirect isisminer's std::cout to file
  ofstream out(tempDir.path().toStdString() + "/mainProgram_globals_out.txt");
  streambuf *coutbuf = cout.rdbuf(); //save old buffer
  cout.rdbuf(out.rdbuf());

  cout << "TEST: GLOBALS provided.\n";

  // run isisminer, globals provided (although empty)
  QVector<QString> args = {"config=" + tempDir.path() + "/mainProgram_test.conf",
                           "globals=" + tempDir.path() + "/globals.pvl",
                           "parameters=inputdir:" + tempDir.path()
                          };

  UserInterface ui1(APP_XML, args);

  try {
    isisminer(ui1);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // try to read output text file
  QFile outText(tempDir.path() + "/mainProgram_globals_out.txt");
  try {
    outText.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  catch (IException &e) {
    FAIL() << "Unable to open mainProgram_globals_out.txt file: " << e.what() << endl;
  }

  // close ofstream and reset to standard output again
  out.close();
  cout.rdbuf(coutbuf);

  // confirm contents of mainProgram_globals_out.txt (skipping timestamps and elapsed times)
  QTextStream in (&outText);
  const QString content = in.readAll();

  // verify output
  ASSERT_TRUE(content.contains("TEST: GLOBALS provided."));
  ASSERT_TRUE(content.contains("Creating strategies..."));
  ASSERT_TRUE(content.contains("Finished creating 1 strategies..."));
  ASSERT_TRUE(content.contains("Running CsvReader::ReadFilterData"));
  ASSERT_TRUE(content.contains("Description: Strategy::ReadFilterData is running a CsvReader algorithm."));
  ASSERT_TRUE(content.contains("12 of 0 processed in CsvReader::ReadFilterData"));
  ASSERT_TRUE(content.contains("Session complete in"));
}


/**
   * IsisminerTestMainProgramNoParameters
   * 
   * Test isisminor application with no parameters provided.
   * 
   * INPUT: 1) filter_data.csv
   *        2) mainProgram_test.conf
   * 
   * OUTPUT: redirected from std::cout to mainProgram_noparameters_out.txt
   */  
TEST_F(IsisminerMainProgram, IsisminerTestMainProgramNoParameters) {

  // redirect isisminer's std::cout to file
  ofstream out(tempDir.path().toStdString() + "/mainProgram_noparameters_out.txt");
  streambuf *coutbuf = cout.rdbuf(); //save old buffer
  cout.rdbuf(out.rdbuf());

  cout << "TEST: No PARAMETERS string given.\n";

  // run isisminer, no parameters
  QVector<QString> args = {"config=" + tempDir.path() + "/mainProgram_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // close ofstream and reset std output
  out.close();
  cout.rdbuf(coutbuf);

  // try to read output text file
  QFile outText(tempDir.path() + "/mainProgram_noparameters_out.txt");
  try {
    outText.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  catch (IException &e) {
    FAIL() << "Unable to open mainProgram_run2_out.txt file: " << e.what() << endl;
  }

  // confirm contents of mainProgram_run2_out.txt (skipping timestamps and elapsed times)
  QTextStream in (&outText);
  const QString content = in.readAll();

  // run output
  ASSERT_TRUE(content.contains("TEST: No PARAMETERS string given."));
  ASSERT_TRUE(content.contains("Creating strategies..."));
  ASSERT_TRUE(content.contains("Finished creating 1 strategies..."));
  ASSERT_TRUE(content.contains("Running CsvReader::ReadFilterData"));
  ASSERT_TRUE(content.contains("Description: Strategy::ReadFilterData is running a CsvReader algorithm."));
  ASSERT_TRUE(content.contains("12 of 0 processed in CsvReader::ReadFilterData"));
  ASSERT_TRUE(content.contains("Session complete in"));
}


/**
   * IsisminerTestMainProgramParametersIllformed
   * 
   * Test isisminor application with ill-formed parameters.
   * 
   * INPUT: 1) filter_data.csv
   *        2) mainProgram_test.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: **USER ERROR** Ill-formed PARAMETERS (x) - use form @key:val.
   */
TEST_F(IsisminerMainProgram, IsisminerTestMainProgramParametersIllformed) {

  // run isisminer run, parameters string ill-formed
  QVector<QString> args = {"config=" + tempDir.path() + "/mainProgram_test.conf",
                           "parameters=x"
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Ill-formed PARAMETERS"));
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"**USER ERROR** Ill-formed PARAMETERS (x) - use form @key:val.\"";
  }
}


/**
   * IsisminerTestNumericalSort
   * 
   * Test isisminor numerical sort.
   * 
   * INPUT: 1) numericalsort_data.csv
   *        2) numericalsort_test.conf
   * 
   * OUTPUT: 1) numericalsort_ascending_test.csv
   *         2) numericalsort_default_test.csv
   *         3) numericalsort_descending_test.csv
   */  
TEST(Isisminer, IsisminerTestNumericalSort) {
  QTemporaryDir tempDir;

  // create input file numericalsort_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/numericalsort_data.csv");
  of << "Respondent,Age,Gender,Height\n";
  of << "1,30,NoData,60.0\n";
  of << "2,NoData,M,68.2\n";
  of << "3,14,F,58.1\n";
  of << "4,23,F,71.9\n";
  of << "5,51,M,72.0\n";
  of << "6,NoData,M,63.0\n";
  of.close();

  // create input config file numericalsort_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "NumericalSortTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadNumericalSortData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/numericalsort_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    
    // Specify column name that is unique to use as the Resource name (identity).
    // If this is not present, a name is generated as "RowX" where X is input row
    // starting at 0. We don't need IdentityArgs since Respondent keyword is a
    // unique identifier
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add IsNumeric strategy
    PvlObject IsNumeric("Strategy");
    IsNumeric.addKeyword(PvlKeyword("Type", "IsNumeric"));
    IsNumeric.addKeyword(PvlKeyword("Name", "TestSomeNumeric"));
    IsNumeric.addKeyword(PvlKeyword("Description", "Keep all numeric ages"));
    IsNumeric.addKeyword(PvlKeyword("Keyword", "Age"));
    isisminerObject.addObject(IsNumeric);

    /////////////////////////////////////////////////////////////////
    // add NumericalSort strategy
    PvlObject NumericalSort("Strategy");
    NumericalSort.addKeyword(PvlKeyword("Type", "NumericalSort"));
    NumericalSort.addKeyword(PvlKeyword("Name", "TestDefault"));
    NumericalSort.addKeyword(PvlKeyword("Description", "Default numerical sort"));
    NumericalSort.addKeyword(PvlKeyword("SortKey", "Age")); // default order is ascending
    isisminerObject.addObject(NumericalSort);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestDefault"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/numericalsort_default_test.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Respondent,Age,Gender,Height)"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "NULL"));
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add ResourceManager strategy to reset for next test
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Activate all discarded Resources for multi-Include test"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 2nd IsNumeric strategy
    isisminerObject.addObject(IsNumeric);

    /////////////////////////////////////////////////////////////////
    // add 2nd NumericalSort strategy
    NumericalSort.addKeyword(PvlKeyword("Name", "TestAscendingSort"), Pvl::Replace);
    NumericalSort.addKeyword(PvlKeyword("Description", "Sort ages in ascending order"), Pvl::Replace);
    NumericalSort.addKeyword(PvlKeyword("Order", "Ascending"));
    isisminerObject.addObject(NumericalSort);

    /////////////////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestAscending"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/numericalsort_ascending_test.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add 2nd ResourceManager strategy to reset for next test
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 3rd IsNumeric strategy
    isisminerObject.addObject(IsNumeric);

    /////////////////////////////////////////////////////////////////
    // add 3rd NumericalSort strategy
    NumericalSort.addKeyword(PvlKeyword("Name", "TestDescendingSort"), Pvl::Replace);
    NumericalSort.addKeyword(PvlKeyword("Description", "Sort ages in descending order"), Pvl::Replace);
    NumericalSort.addKeyword(PvlKeyword("Order", "Descending"), Pvl::Replace);
    isisminerObject.addObject(NumericalSort);

    /////////////////////////////////////////////////////////////////
    // add 3rd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestDescending"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/numericalsort_descending_test.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/numericalsort_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/numericalsort_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // numericalsort_default_test.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/numericalsort_default_test.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"3,14,F,58.1");
  compareCsvLine(line.getRow(2),"4,23,F,71.9");
  compareCsvLine(line.getRow(3),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(4),"5,51,M,72.0");

  /////////////////////////////////////////////////////////////////
  // numericalsort_ascending_test.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/numericalsort_ascending_test.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"3,14,F,58.1");
  compareCsvLine(line.getRow(2),"4,23,F,71.9");
  compareCsvLine(line.getRow(3),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(4),"5,51,M,72.0");

  /////////////////////////////////////////////////////////////////
  // numericalsort_descending_test.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/numericalsort_descending_test.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Respondent,Age,Gender,Height");
  compareCsvLine(line.getRow(1),"5,51,M,72.0");
  compareCsvLine(line.getRow(2),"1,30,NoData,60.0");
  compareCsvLine(line.getRow(3),"4,23,F,71.9");
  compareCsvLine(line.getRow(4),"3,14,F,58.1");
}


/**
   * IsisminerTestNumericalSortError
   * 
   * Test isisminor numerical sort with invalid sort order.
   * 
   * INPUT: 1) numericalsort_data.csv
   *        2) numericalsort_error.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: Specified sort Order [Best] is invalid.
             Must be Ascending or Descending.
   */  
TEST(Isisminer, IsisminerTestNumericalSortError) {
  QTemporaryDir tempDir;

  // create input file numericalsort_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/numericalsort_data.csv");
  of << "Respondent,Age,Gender,Height\n";
  of << "1,30,NoData,60.0\n";
  of << "2,NoData,M,68.2\n";
  of << "3,14,F,58.1\n";
  of << "4,23,F,71.9\n";
  of << "5,51,M,72.0\n";
  of << "6,NoData,M,63.0\n";
  of.close();

  // create input config file numericalsort_error.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "NumericalSortErrorTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadNumericalSortData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/numericalsort_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
  
    // Specify column name that is unique to use as the Resource name (identity).
    // If this is not present, a name is generated as "RowX" where X is input row
    // starting at 0. We don't need IdentityArgs since Respondent keyword is a
    // unique identifier
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add IsNumeric strategy
    PvlObject IsNumeric("Strategy");
    IsNumeric.addKeyword(PvlKeyword("Type", "IsNumeric"));
    IsNumeric.addKeyword(PvlKeyword("Name", "TestSomeNumeric"));
    IsNumeric.addKeyword(PvlKeyword("Description", "Keep all numeric ages"));
    IsNumeric.addKeyword(PvlKeyword("Keyword", "Age"));
    isisminerObject.addObject(IsNumeric);

    /////////////////////////////////////////////////////////////////
    // add NumericalSort strategy
    PvlObject NumericalSort("Strategy");
    NumericalSort.addKeyword(PvlKeyword("Type", "NumericalSort"));
    NumericalSort.addKeyword(PvlKeyword("Name", "TestAscendingSort"));
    NumericalSort.addKeyword(PvlKeyword("Description", "Test exception for bad Order value"));
    NumericalSort.addKeyword(PvlKeyword("SortKey", "Age")); // default order is ascending
    NumericalSort.addKeyword(PvlKeyword("Order", "Best")); // default order is ascending
    isisminerObject.addObject(NumericalSort);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/numericalsort_error.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/numericalsort_error.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Specified sort Order [Best] is invalid."));
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Specified sort Order [Best] is invalid.\n"
              "Must be Ascending or Descending.\"";
  }
}


/**
   * IsisminerTestPdsTableCreator
   * 
   * Test isisminor pds table creation behavior.
   * 
   * INPUT: 1) pdstablecreator_data.csv
   *        2) TestGeneralFormat.txt
   *        3) pdstablecreator_test.conf
   * 
   * OUTPUT: 1) TestAppend.txt
   *         2) TestCreate.txt
   */
TEST(Isisminer, IsisminerTestPdsTableCreator) {
  QTemporaryDir tempDir;

  // create input file pdstablecreator_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/pdstablecreator_data.csv");

  of << "\"h10_18kmgrid_00001              \",\"FREE        \",9,0,0.33,-21.57140655,288.39472589,2439.60338711,8.04079431,9.02723992,41.25230949,47.18502115,158.97566783,-396.61289245,715.92546425,-2152.81295915,-896.94580602\n";
  of << "\"h10_18kmgrid_00002              \",\"FREE        \",5,0,0.20,-21.14922828,288.45653560,2439.93628497,11.08541203,25.03339906,45.06864387,20.49010422,481.26783265,-63.71502766,720.41856927,-2158.54452019,-880.32477765\n";
  of << "\"h10_18kmgrid_00003              \",\"FREE        \",6,0,0.30,-20.72275856,288.44293971,2440.30749359,10.77523152,12.01996005,40.40680917,167.01820426,359.85522679,307.49358921,722.06917427,-2165.20089311,-863.49398569\n";
  of << "\"h10_18kmgrid_00004              \",\"FREE        \",4,0,0.26,-20.30373524,288.43750002,2440.45845846,13.13973106,29.08199284,48.16428609,-15.37602363,-225.37896470,458.45846194,723.88631453,-2171.33666372,-846.83126058\n";
  of << "\"h10_18kmgrid_00005              \",\"FREE        \",11,0,0.27,-19.88503279,288.43537855,2440.25461055,9.16352697,9.38095145,35.77893423, 65.44367366,107.14451061,254.61054961,725.68294651,-2176.99441380,-830.01334190\n";
  of << "\"h10_18kmgrid_00006              \",\"FREE        \",4,0,0.43,-19.44884948,288.42285195,2440.44213372,12.33181367,12.56280806,51.38066945,247.34955474,191.97465846,442.13372042,727.23871918,-2183.25255119,-812.58227343\n";
  of << "\"h10_18kmgrid_00007              \",\"FREE        \",12,0,0.35,-19.04967331,288.39493819,2440.85938237,10.03883691,11.33891319,30.31395174,2.17461409,103.68747474,859.38236825,728.06828186,-2189.29974701,-796.66662990\n";
  of << "\"h10_18kmgrid_00008              \",\"FREE        \",14,0,0.32,-18.62418475,288.39355282,2441.00912330,8.56952211,9.77765460,26.21020575,64.74297529,131.72015893,1009.12330037,729.90683721,-2195.00551885,-779.55905566\n";
  of << "\"h10_18kmgrid_00009              \",\"FREE        \",7,0,0.49,-18.17238805,288.40984520,2441.03690435,26.66079608,38.90040525,51.35927076,163.25388383,-344.29970949,1036.90434538,732.45789870,-2200.58711773,-761.30344298\n";
  of << "\"h10_18kmgrid_00010              \",\"FREE        \",9,0,0.33,-17.77521241,288.40472252,2440.45402354,16.36689434,22.21955121,38.10661762,147.07836787,249.52524173,454.02354003,733.73450382,-2205.08045363,-745.03000740\n";
  of << "\"h10_18kmgrid_00011              \",\"FREE        \",13,0,0.36,-17.35298936,288.39875336,2441.06313639,9.48435328,10.04109830,30.31133996,20.64172643,-263.51408679,1063.13638653,735.40122489,-2210.85824686,-728.06599052\n";
  of << "\"h10_18kmgrid_00012              \",\"FREE        \",9,0,0.28,-16.91964567,288.43365994,2440.48372498,15.27997856,18.36879140,43.24433207,123.20972642,-97.45504312,483.72498256,738.29285892,-2215.04466885,-710.25458913\n";
  of << "\"h10_18kmgrid_00013              \",\"FREE        \",5,0,0.14,-16.48762978,288.42214180,2440.59885228,38.88580515,57.94135794,127.35375129,17.20525015,-786.00557788,598.85228379,739.55383768,-2220.31570859,-692.66227926\n";
  of << "\"h10_18kmgrid_00014              \",\"FREE        \",11,0,0.27,-16.07705612,288.39019863,2440.43586172,12.90193336,14.66413981,39.06376806,82.34454221,5.56743235,435.86171840,739.81341258,-2225.23203557,-675.82963651\n";
  of << "\"h10_18kmgrid_00015              \",\"FREE        \",10,0,0.18,-15.65552727,288.42901296,2440.06587653,9.43679582,9.55109852,46.70429590,1.53260031,-243.71165153,65.87653264,742.75983347,-2229.04924119,-658.45940797\n";
  of << "\"h10_18kmgrid_00016              \",\"FREE        \",10,0,0.30,-15.24198423,288.40596912,2440.35014854,10.32055401,10.25897740,42.04205015,69.81515921,-194.03902092,350.14853746,743.43114325,-2234.05935436,-641.55887019\n";
  of << "\"h10_18kmgrid_00017              \",\"FREE        \",6,0,0.25,-14.80237521,288.43839671,2440.18093789,18.47945251,14.71044214,74.15725559,-20.84820688,-455.01684940,180.93788716,746.17864663,-2238.08699309,-623.43167091\n";
  of << "\"h10_18kmgrid_00018              \",\"FREE        \",9,0,0.19,-14.39720539,288.40786616,2439.88231635,11.20169802,11.01092782,40.96183319,111.19185674,-106.66883571,-117.68365295,746.26813417,-2242.33688406,-606.65879035\n";
  of << "\"h10_18kmgrid_00019              \",\"FREE        \",5,0,0.48,-13.95206780,288.45896010,2439.58454770,13.20332924,12.57317791,66.64409749,156.94216911,-61.43493615,-415.45230235,749.64567999,-2245.79940119,-588.20844048\n";
  of << "\"h10_18kmgrid_00020              \",\"FREE        \",4,0,0.30,-13.53734394,288.45979459,2439.57441959,20.20612102,27.44809985,114.50905279,160.43520761,31.46792240,-425.58040908,751.00375201,-2249.75883151,-571.05333658\n";
  of << "\"h10_18kmgrid_00021              \",\"FREE        \",7,0,0.35,-13.12054593,288.40996616,2439.61946885,10.48653529,18.25176273,103.54698834,205.77315832,-239.04797112,-380.53115193,750.35287688,-2254.33467801,-553.79497330\n";
  of << "\"h10_18kmgrid_00022              \",\"FREE        \",7,0,0.36,-12.68297698,288.46016400,2439.10167585,10.41300363,11.20509149,61.41483163,136.35977888,45.01065537,-898.32415343,753.48494386,-2257.14320751,-535.52027532\n";
  of << "\"h10_18kmgrid_00023              \",\"FREE        \",8,0,0.28,-12.28501855,288.40377605,2439.17063206,10.55497324,11.56534939,58.63978465,21.64291983,-293.61616521,-829.36793849,752.44064717,-2261.42241985,-518.99430038\n";
  of << "\"h10_18kmgrid_00024              \",\"FREE        \",9,0,0.27,-11.85579907,288.40176262,2439.10838851,10.54925114,11.86291658,61.86147175,7.80146073,-318.64473262,-891.61149137,753.54816341,-2265.01670288,-501.11299322\n";
  of << "\"h10_18kmgrid_00025              \",\"FREE        \",11,0,0.23,-11.43398597,288.40138715,2439.29498924,9.31754445,14.19519926,22.72013383,-11.21442304,-378.54116386,-705.01076387,754.73520455,-2268.63434612,-483.56283886\n";
  of.close();

  // create input TestGeneralFormat.txt file in tempDir (pvl format)
  Pvl testGeneralFormat;
  PvlObject column("COLUMN");
  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "1"));
  column.addKeyword(PvlKeyword("NAME", "POINT_ID"));
  column.addKeyword(PvlKeyword("DATA_TYPE", "CHARACTER"));
  column.addKeyword(PvlKeyword("UNIT", "NONE"));
  column.addKeyword(PvlKeyword("START_BYTE", "2"));
  column.addKeyword(PvlKeyword("BYTES", "32"));
  column.addKeyword(PvlKeyword("DESCRIPTION", "Unique point identifier for each"
      "jigsaw control point."));
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "2"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "STATUS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "37"), Pvl::Replace);
  column.addKeyword(PvlKeyword("BYTES", "12"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Status of point. It can be FREE,"
      " FIXED or CONSTRAINED."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "3"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "ACCEPTED_MEASURES"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DATA_TYPE", "ASCII_INTEGER"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "51"), Pvl::Replace);
  column.addKeyword(PvlKeyword("BYTES", "0"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Number of accepted (non-rejected)"
      " measures for the control point. This is actually the number of images"
      " that contain the control point point (also referred to as the point"
      " depth)"), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "4"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "REJECTED_MEASURES"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "52"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Number of measures rejected by"
      " jigsaw during the bundle adjustment (if outlier rejection is active)"),
      Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "5"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "RESIDUAL_RMS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DATA_TYPE", "ASCII_REAL"), Pvl::Replace);
  column.addKeyword(PvlKeyword("UNIT", "PIXELS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "53"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Root mean square error of"
      " residuals for a point. Indicates the statistical measure of variation in the"
      " difference of each measure within a control point."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "6"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "ADJUSTED_LATITUDE"), Pvl::Replace);
  column.addKeyword(PvlKeyword("UNIT", "DEGREES"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "54"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Latitude coordinate of the"
      " control point. This is the adjusted location of the control point"
      " after jigsaw bundle adjustment."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "7"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "ADJUSTED_LONGITUDE"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "55"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Longitude coordinate of control"
      " point. This is the adjusted location of the control point after jigsaw"
      " bundle adjustment."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "8"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "ADJUSTED_RADIUS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("UNIT", "KILOMETERS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "56"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Radius of control point. This is"
      " the adjusted radius of the control point after jigsaw bundle adjustment."),
      Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "9"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "SIGMA_LATITUDE"), Pvl::Replace);
  column.addKeyword(PvlKeyword("UNIT", "METERS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "57"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Adjusted uncertainty of Latitude"
      " coordinate of the control point after jigsaw bundle adjustment."),
      Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "10"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "SIGMA_LONGITUDE"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "58"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Adjusted uncertainty of Longitude"
      " coordinate of the control point after jigsaw bundle adjustment."),
      Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "11"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "SIGMA_RADIUS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "59"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Adjusted uncertainty of Radius"
      " coordinate of the control point after jigsaw bundle adjustment."),
      Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "12"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "DELTA_LATITUDE"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "60"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Latitude adjustment. The number"
      " of meters the latitude coordinate has been adjusted by the jigsaw bundle"
      " adjustment."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "13"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "DELTA_LONGITUDE"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "61"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Longitude adjustment. The number"
      " of meters the longitude coordinate has been adjusted by the jigsaw"
      "bundle adjustment."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "14"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "DELTA_RADIUS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "62"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Radius adjustment. The number of"
      " meters the radius has been adjusted by the jigsaw bundle adjustment."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "15"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "X"), Pvl::Replace);
  column.addKeyword(PvlKeyword("UNIT", "KILOMETERS"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "63"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Body-fixed X coordinate of the"
      " vector from the center of the target body to the surface coordinate"
      " location."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "16"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "Y"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "64"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Body-fixed Y coordinate of the"
      " vector from the center of the target body to the surface coordinate"
      "location."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  column.addKeyword(PvlKeyword("COLUMN_NUMBER", "17"), Pvl::Replace);
  column.addKeyword(PvlKeyword("NAME", "Z"), Pvl::Replace);
  column.addKeyword(PvlKeyword("START_BYTE", "65"), Pvl::Replace);
  column.addKeyword(PvlKeyword("DESCRIPTION", "Body-fixed Z coordinate of the"
      " vector from the center of the target body to the surface coordinate"
      " location."), Pvl::Replace);
  testGeneralFormat.addObject(column);

  testGeneralFormat.write(tempDir.path() + "/TestGeneralFormat.txt");

  // create input config file pdstablecreator_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "PdsTableCreatorTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/pdstablecreator_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "False"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "StereoSource"));

    QStringList headerList = {"\"POINT_ID\"","\"STATUS\"","\"ACCEPTED_MEASURES\"",
                              "\"REJECTED_MEASURES\"","\"RESIDUAL_RMS\"",
                              "\"ADJUSTED_LATITUDE\"","\"ADJUSTED_LONGITUDE\"",
                              "\"ADJUSTED_RADIUS\"","\"SIGMA_LATITUDE\"",
                              "\"SIGMA_LONGITUDE\"","\"SIGMA_RADIUS\"",
                              "\"DELTA_LATITUDE\"","\"DELTA_LONGITUDE\"",
                              "\"DELTA_RADIUS\"","\"X\"","\"Y\"","\"Z\""};

    PvlKeyword hw("Header");
    for (auto const &v : headerList) {
      hw.addValue(v);
    }

    CsvReader.addKeyword(hw);
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add PdsTableCreator strategy
    PvlObject PdsTableCreator("Strategy");
    PdsTableCreator.addKeyword(PvlKeyword("Type", "PdsTableCreator"));
    PdsTableCreator.addKeyword(PvlKeyword("Name", "TestAppendTableWriter"));
    PdsTableCreator.addKeyword(PvlKeyword("PdsFormatFile", "\"%1/TestGeneralFormat.txt\""));
    PdsTableCreator.addKeyword(PvlKeyword("PdsFormatFileArgs", "\"inputdir\""));
    PdsTableCreator.addKeyword(PvlKeyword("PdsTableFile", "\"%1/TestAppend.txt\""));
    PdsTableCreator.addKeyword(PvlKeyword("PdsTableFileArgs", "\"outputdir\""));
    PdsTableCreator.addKeyword(PvlKeyword("Mode", "Append"));
    PdsTableCreator.addKeyword(PvlKeyword("Delimiter", ","));
    PdsTableCreator.addKeyword(PvlKeyword("DefaultValue", "NULL"));
    isisminerObject.addObject(PdsTableCreator);

    /////////////////////////////////////////////////////////////////
    // add 2nd PdsTableCreator strategy
    PdsTableCreator.addKeyword(PvlKeyword("Name", "TestCreateTableWriter"), Pvl::Replace);
    PdsTableCreator.addKeyword(PvlKeyword("PdsTableFile", "%1/TestCreate.txt"), Pvl::Replace);
    PdsTableCreator.addKeyword(PvlKeyword("Mode", "Create"), Pvl::Replace);
    isisminerObject.addObject(PdsTableCreator);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/pdstablecreator_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/pdstablecreator_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // TestAppend.txt
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/TestAppend.txt",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 17);
  ASSERT_EQ(line.rows(), 25);

  compareCsvLine(line.getRow(0),"\"\"h10_18kmgrid_00001              \"\",\"\"FREE        \"\",9,0,0.33,-21.57140655,288.39472589,2439.60338711,8.04079431,9.02723992,41.25230949,47.18502115,158.97566783,-396.61289245,715.92546425,-2152.81295915,-896.94580602");
  compareCsvLine(line.getRow(1),"\"\"h10_18kmgrid_00002              \"\",\"\"FREE        \"\",  5,0,  0.20,    -21.14922828,    288.45653560,   2439.93628497,     11.08541203,     25.03339906,     45.06864387,     20.49010422,    481.26783265,    -63.71502766,    720.41856927,  -2158.54452019,   -880.32477765");
  compareCsvLine(line.getRow(2),"\"\"h10_18kmgrid_00003              \"\",\"\"FREE        \"\",  6,0,  0.30,    -20.72275856,    288.44293971,   2440.30749359,     10.77523152,     12.01996005,     40.40680917,    167.01820426,    359.85522679,    307.49358921,    722.06917427,  -2165.20089311,   -863.49398569");
  compareCsvLine(line.getRow(3),"\"\"h10_18kmgrid_00004              \"\",\"\"FREE        \"\",  4,0,  0.26,    -20.30373524,    288.43750002,   2440.45845846,     13.13973106,     29.08199284,     48.16428609,    -15.37602363,   -225.37896470,    458.45846194,    723.88631453,  -2171.33666372,   -846.83126058");
  compareCsvLine(line.getRow(4),"\"\"h10_18kmgrid_00005              \"\",\"\"FREE        \"\", 11,0,  0.27,    -19.88503279,    288.43537855,   2440.25461055,      9.16352697,      9.38095145,     35.77893423,     65.44367366,    107.14451061,    254.61054961,    725.68294651,  -2176.99441380,   -830.01334190");
  compareCsvLine(line.getRow(5),"\"\"h10_18kmgrid_00006              \"\",\"\"FREE        \"\",  4,0,  0.43,    -19.44884948,    288.42285195,   2440.44213372,     12.33181367,     12.56280806,     51.38066945,    247.34955474,    191.97465846,    442.13372042,    727.23871918,  -2183.25255119,   -812.58227343");
  compareCsvLine(line.getRow(6),"\"\"h10_18kmgrid_00007              \"\",\"\"FREE        \"\", 12,0,  0.35,    -19.04967331,    288.39493819,   2440.85938237,     10.03883691,     11.33891319,     30.31395174,      2.17461409,    103.68747474,    859.38236825,    728.06828186,  -2189.29974701,   -796.66662990");
  compareCsvLine(line.getRow(7),"\"\"h10_18kmgrid_00008              \"\",\"\"FREE        \"\", 14,0,  0.32,    -18.62418475,    288.39355282,   2441.00912330,      8.56952211,      9.77765460,     26.21020575,     64.74297529,    131.72015893,   1009.12330037,    729.90683721,  -2195.00551885,   -779.55905566");
  compareCsvLine(line.getRow(8),"\"\"h10_18kmgrid_00009              \"\",\"\"FREE        \"\",  7,0,  0.49,    -18.17238805,    288.40984520,   2441.03690435,     26.66079608,     38.90040525,     51.35927076,    163.25388383,   -344.29970949,   1036.90434538,    732.45789870,  -2200.58711773,   -761.30344298");
  compareCsvLine(line.getRow(9),"\"\"h10_18kmgrid_00010              \"\",\"\"FREE        \"\",  9,0,  0.33,    -17.77521241,    288.40472252,   2440.45402354,     16.36689434,     22.21955121,     38.10661762,    147.07836787,    249.52524173,    454.02354003,    733.73450382,  -2205.08045363,   -745.03000740");
  compareCsvLine(line.getRow(10),"\"\"h10_18kmgrid_00011              \"\",\"\"FREE        \"\", 13,0,  0.36,    -17.35298936,    288.39875336,   2441.06313639,      9.48435328,     10.04109830,     30.31133996,     20.64172643,   -263.51408679,   1063.13638653,    735.40122489,  -2210.85824686,   -728.06599052");
  compareCsvLine(line.getRow(11),"\"\"h10_18kmgrid_00012              \"\",\"\"FREE        \"\",  9,0,  0.28,    -16.91964567,    288.43365994,   2440.48372498,     15.27997856,     18.36879140,     43.24433207,    123.20972642,    -97.45504312,    483.72498256,    738.29285892,  -2215.04466885,   -710.25458913");
  compareCsvLine(line.getRow(12),"\"\"h10_18kmgrid_00013              \"\",\"\"FREE        \"\",  5,0,  0.14,    -16.48762978,    288.42214180,   2440.59885228,     38.88580515,     57.94135794,    127.35375129,     17.20525015,   -786.00557788,    598.85228379,    739.55383768,  -2220.31570859,   -692.66227926");
  compareCsvLine(line.getRow(13),"\"\"h10_18kmgrid_00014              \"\",\"\"FREE        \"\", 11,0,  0.27,    -16.07705612,    288.39019863,   2440.43586172,     12.90193336,     14.66413981,     39.06376806,     82.34454221,      5.56743235,    435.86171840,    739.81341258,  -2225.23203557,   -675.82963651");
  compareCsvLine(line.getRow(14),"\"\"h10_18kmgrid_00015              \"\",\"\"FREE        \"\", 10,0,  0.18,    -15.65552727,    288.42901296,   2440.06587653,      9.43679582,      9.55109852,     46.70429590,      1.53260031,   -243.71165153,     65.87653264,    742.75983347,  -2229.04924119,   -658.45940797");
  compareCsvLine(line.getRow(15),"\"\"h10_18kmgrid_00016              \"\",\"\"FREE        \"\", 10,0,  0.30,    -15.24198423,    288.40596912,   2440.35014854,     10.32055401,     10.25897740,     42.04205015,     69.81515921,   -194.03902092,    350.14853746,    743.43114325,  -2234.05935436,   -641.55887019");
  compareCsvLine(line.getRow(16),"\"\"h10_18kmgrid_00017              \"\",\"\"FREE        \"\",  6,0,  0.25,    -14.80237521,    288.43839671,   2440.18093789,     18.47945251,     14.71044214,     74.15725559,    -20.84820688,   -455.01684940,    180.93788716,    746.17864663,  -2238.08699309,   -623.43167091");
  compareCsvLine(line.getRow(17),"\"\"h10_18kmgrid_00018              \"\",\"\"FREE        \"\",  9,0,  0.19,    -14.39720539,    288.40786616,   2439.88231635,     11.20169802,     11.01092782,     40.96183319,    111.19185674,   -106.66883571,   -117.68365295,    746.26813417,  -2242.33688406,   -606.65879035");
  compareCsvLine(line.getRow(18),"\"\"h10_18kmgrid_00019              \"\",\"\"FREE        \"\",  5,0,  0.48,    -13.95206780,    288.45896010,   2439.58454770,     13.20332924,     12.57317791,     66.64409749,    156.94216911,    -61.43493615,   -415.45230235,    749.64567999,  -2245.79940119,   -588.20844048");
  compareCsvLine(line.getRow(19),"\"\"h10_18kmgrid_00020              \"\",\"\"FREE        \"\",  4,0,  0.30,    -13.53734394,    288.45979459,   2439.57441959,     20.20612102,     27.44809985,    114.50905279,    160.43520761,     31.46792240,   -425.58040908,    751.00375201,  -2249.75883151,   -571.05333658");
  compareCsvLine(line.getRow(20),"\"\"h10_18kmgrid_00021              \"\",\"\"FREE        \"\",  7,0,  0.35,    -13.12054593,    288.40996616,   2439.61946885,     10.48653529,     18.25176273,    103.54698834,    205.77315832,   -239.04797112,   -380.53115193,    750.35287688,  -2254.33467801,   -553.79497330");
  compareCsvLine(line.getRow(21),"\"\"h10_18kmgrid_00022              \"\",\"\"FREE        \"\",  7,0,  0.36,    -12.68297698,    288.46016400,   2439.10167585,     10.41300363,     11.20509149,     61.41483163,    136.35977888,     45.01065537,   -898.32415343,    753.48494386,  -2257.14320751,   -535.52027532");
  compareCsvLine(line.getRow(22),"\"\"h10_18kmgrid_00023              \"\",\"\"FREE        \"\",  8,0,  0.28,    -12.28501855,    288.40377605,   2439.17063206,     10.55497324,     11.56534939,     58.63978465,     21.64291983,   -293.61616521,   -829.36793849,    752.44064717,  -2261.42241985,   -518.99430038");
  compareCsvLine(line.getRow(23),"\"\"h10_18kmgrid_00024              \"\",\"\"FREE        \"\",  9,0,  0.27,    -11.85579907,    288.40176262,   2439.10838851,     10.54925114,     11.86291658,     61.86147175,      7.80146073,   -318.64473262,   -891.61149137,    753.54816341,  -2265.01670288,   -501.11299322");
  compareCsvLine(line.getRow(24),"\"\"h10_18kmgrid_00025              \"\",\"\"FREE        \"\", 11,0,  0.23,    -11.43398597,    288.40138715,   2439.29498924,      9.31754445,     14.19519926,     22.72013383,    -11.21442304,   -378.54116386,   -705.01076387,    754.73520455,  -2268.63434612,   -483.56283886");

  /////////////////////////////////////////////////////////////////
  // TestAppend.txt
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/TestCreate.txt",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 17);
  ASSERT_EQ(line.rows(), 25);

  compareCsvLine(line.getRow(0),"\"\"h10_18kmgrid_00001              \"\",\"\"FREE        \"\",9,0,0.33,-21.57140655,288.39472589,2439.60338711,8.04079431,9.02723992,41.25230949,47.18502115,158.97566783,-396.61289245,715.92546425,-2152.81295915,-896.94580602");
  compareCsvLine(line.getRow(1),"\"\"h10_18kmgrid_00002              \"\",\"\"FREE        \"\",  5,0,  0.20,    -21.14922828,    288.45653560,   2439.93628497,     11.08541203,     25.03339906,     45.06864387,     20.49010422,    481.26783265,    -63.71502766,    720.41856927,  -2158.54452019,   -880.32477765");
  compareCsvLine(line.getRow(2),"\"\"h10_18kmgrid_00003              \"\",\"\"FREE        \"\",  6,0,  0.30,    -20.72275856,    288.44293971,   2440.30749359,     10.77523152,     12.01996005,     40.40680917,    167.01820426,    359.85522679,    307.49358921,    722.06917427,  -2165.20089311,   -863.49398569");
  compareCsvLine(line.getRow(3),"\"\"h10_18kmgrid_00004              \"\",\"\"FREE        \"\",  4,0,  0.26,    -20.30373524,    288.43750002,   2440.45845846,     13.13973106,     29.08199284,     48.16428609,    -15.37602363,   -225.37896470,    458.45846194,    723.88631453,  -2171.33666372,   -846.83126058");
  compareCsvLine(line.getRow(4),"\"\"h10_18kmgrid_00005              \"\",\"\"FREE        \"\", 11,0,  0.27,    -19.88503279,    288.43537855,   2440.25461055,      9.16352697,      9.38095145,     35.77893423,     65.44367366,    107.14451061,    254.61054961,    725.68294651,  -2176.99441380,   -830.01334190");
  compareCsvLine(line.getRow(5),"\"\"h10_18kmgrid_00006              \"\",\"\"FREE        \"\",  4,0,  0.43,    -19.44884948,    288.42285195,   2440.44213372,     12.33181367,     12.56280806,     51.38066945,    247.34955474,    191.97465846,    442.13372042,    727.23871918,  -2183.25255119,   -812.58227343");
  compareCsvLine(line.getRow(6),"\"\"h10_18kmgrid_00007              \"\",\"\"FREE        \"\", 12,0,  0.35,    -19.04967331,    288.39493819,   2440.85938237,     10.03883691,     11.33891319,     30.31395174,      2.17461409,    103.68747474,    859.38236825,    728.06828186,  -2189.29974701,   -796.66662990");
  compareCsvLine(line.getRow(7),"\"\"h10_18kmgrid_00008              \"\",\"\"FREE        \"\", 14,0,  0.32,    -18.62418475,    288.39355282,   2441.00912330,      8.56952211,      9.77765460,     26.21020575,     64.74297529,    131.72015893,   1009.12330037,    729.90683721,  -2195.00551885,   -779.55905566");
  compareCsvLine(line.getRow(8),"\"\"h10_18kmgrid_00009              \"\",\"\"FREE        \"\",  7,0,  0.49,    -18.17238805,    288.40984520,   2441.03690435,     26.66079608,     38.90040525,     51.35927076,    163.25388383,   -344.29970949,   1036.90434538,    732.45789870,  -2200.58711773,   -761.30344298");
  compareCsvLine(line.getRow(9),"\"\"h10_18kmgrid_00010              \"\",\"\"FREE        \"\",  9,0,  0.33,    -17.77521241,    288.40472252,   2440.45402354,     16.36689434,     22.21955121,     38.10661762,    147.07836787,    249.52524173,    454.02354003,    733.73450382,  -2205.08045363,   -745.03000740");
  compareCsvLine(line.getRow(10),"\"\"h10_18kmgrid_00011              \"\",\"\"FREE        \"\", 13,0,  0.36,    -17.35298936,    288.39875336,   2441.06313639,      9.48435328,     10.04109830,     30.31133996,     20.64172643,   -263.51408679,   1063.13638653,    735.40122489,  -2210.85824686,   -728.06599052");
  compareCsvLine(line.getRow(11),"\"\"h10_18kmgrid_00012              \"\",\"\"FREE        \"\",  9,0,  0.28,    -16.91964567,    288.43365994,   2440.48372498,     15.27997856,     18.36879140,     43.24433207,    123.20972642,    -97.45504312,    483.72498256,    738.29285892,  -2215.04466885,   -710.25458913");
  compareCsvLine(line.getRow(12),"\"\"h10_18kmgrid_00013              \"\",\"\"FREE        \"\",  5,0,  0.14,    -16.48762978,    288.42214180,   2440.59885228,     38.88580515,     57.94135794,    127.35375129,     17.20525015,   -786.00557788,    598.85228379,    739.55383768,  -2220.31570859,   -692.66227926");
  compareCsvLine(line.getRow(13),"\"\"h10_18kmgrid_00014              \"\",\"\"FREE        \"\", 11,0,  0.27,    -16.07705612,    288.39019863,   2440.43586172,     12.90193336,     14.66413981,     39.06376806,     82.34454221,      5.56743235,    435.86171840,    739.81341258,  -2225.23203557,   -675.82963651");
  compareCsvLine(line.getRow(14),"\"\"h10_18kmgrid_00015              \"\",\"\"FREE        \"\", 10,0,  0.18,    -15.65552727,    288.42901296,   2440.06587653,      9.43679582,      9.55109852,     46.70429590,      1.53260031,   -243.71165153,     65.87653264,    742.75983347,  -2229.04924119,   -658.45940797");
  compareCsvLine(line.getRow(15),"\"\"h10_18kmgrid_00016              \"\",\"\"FREE        \"\", 10,0,  0.30,    -15.24198423,    288.40596912,   2440.35014854,     10.32055401,     10.25897740,     42.04205015,     69.81515921,   -194.03902092,    350.14853746,    743.43114325,  -2234.05935436,   -641.55887019");
  compareCsvLine(line.getRow(16),"\"\"h10_18kmgrid_00017              \"\",\"\"FREE        \"\",  6,0,  0.25,    -14.80237521,    288.43839671,   2440.18093789,     18.47945251,     14.71044214,     74.15725559,    -20.84820688,   -455.01684940,    180.93788716,    746.17864663,  -2238.08699309,   -623.43167091");
  compareCsvLine(line.getRow(17),"\"\"h10_18kmgrid_00018              \"\",\"\"FREE        \"\",  9,0,  0.19,    -14.39720539,    288.40786616,   2439.88231635,     11.20169802,     11.01092782,     40.96183319,    111.19185674,   -106.66883571,   -117.68365295,    746.26813417,  -2242.33688406,   -606.65879035");
  compareCsvLine(line.getRow(18),"\"\"h10_18kmgrid_00019              \"\",\"\"FREE        \"\",  5,0,  0.48,    -13.95206780,    288.45896010,   2439.58454770,     13.20332924,     12.57317791,     66.64409749,    156.94216911,    -61.43493615,   -415.45230235,    749.64567999,  -2245.79940119,   -588.20844048");
  compareCsvLine(line.getRow(19),"\"\"h10_18kmgrid_00020              \"\",\"\"FREE        \"\",  4,0,  0.30,    -13.53734394,    288.45979459,   2439.57441959,     20.20612102,     27.44809985,    114.50905279,    160.43520761,     31.46792240,   -425.58040908,    751.00375201,  -2249.75883151,   -571.05333658");
  compareCsvLine(line.getRow(20),"\"\"h10_18kmgrid_00021              \"\",\"\"FREE        \"\",  7,0,  0.35,    -13.12054593,    288.40996616,   2439.61946885,     10.48653529,     18.25176273,    103.54698834,    205.77315832,   -239.04797112,   -380.53115193,    750.35287688,  -2254.33467801,   -553.79497330");
  compareCsvLine(line.getRow(21),"\"\"h10_18kmgrid_00022              \"\",\"\"FREE        \"\",  7,0,  0.36,    -12.68297698,    288.46016400,   2439.10167585,     10.41300363,     11.20509149,     61.41483163,    136.35977888,     45.01065537,   -898.32415343,    753.48494386,  -2257.14320751,   -535.52027532");
  compareCsvLine(line.getRow(22),"\"\"h10_18kmgrid_00023              \"\",\"\"FREE        \"\",  8,0,  0.28,    -12.28501855,    288.40377605,   2439.17063206,     10.55497324,     11.56534939,     58.63978465,     21.64291983,   -293.61616521,   -829.36793849,    752.44064717,  -2261.42241985,   -518.99430038");
  compareCsvLine(line.getRow(23),"\"\"h10_18kmgrid_00024              \"\",\"\"FREE        \"\",  9,0,  0.27,    -11.85579907,    288.40176262,   2439.10838851,     10.54925114,     11.86291658,     61.86147175,      7.80146073,   -318.64473262,   -891.61149137,    753.54816341,  -2265.01670288,   -501.11299322");
  compareCsvLine(line.getRow(24),"\"\"h10_18kmgrid_00025              \"\",\"\"FREE        \"\", 11,0,  0.23,    -11.43398597,    288.40138715,   2439.29498924,      9.31754445,     14.19519926,     22.72013383,    -11.21442304,   -378.54116386,   -705.01076387,    754.73520455,  -2268.63434612,   -483.56283886");
}


/**
   * IsisminerTestPdsTableFormat
   * 
   * Test isisminor pds table format behavior.
   * 
   * INPUT: 1) pdsExportRootGenModified.typ
   *        2) pdstableformat_data.csv
   *        3) pdstableformat_test.conf
   * 
   * OUTPUT: 1) TestGeneralFormat.txt
   *         2) TestNoPointIDFormat.txt
   *         3) TestNoStatusBytesFormat.txt
   */
TEST(Isisminer, IsisminerTestPdsTableFormat) {
  QTemporaryDir tempDir;

  // create input file pdstablecreator_data.csv
  ofstream of;
  of.open(tempDir.path().toStdString() + "/pdstableformat_data.csv");

  of << "\"h10_18kmgrid_00001              \",\"FREE        \",  9,0,  0.33,    -21.57140655,    288.39472589,   2439.60338711,      8.04079431,      9.02723992,     41.25230949,     47.18502115,    158.97566783,   -396.61289245,    715.92546425,  -2152.81295915,   -896.94580602\n";
  of << "\"h10_18kmgrid_00002              \",\"FREE        \",  5,0,  0.20,    -21.14922828,    288.45653560,   2439.93628497,     11.08541203,     25.03339906,     45.06864387,     20.49010422,    481.26783265,    -63.71502766,    720.41856927,  -2158.54452019,   -880.32477765\n";
  of << "\"h10_18kmgrid_00003              \",\"FREE        \",  6,0,  0.30,    -20.72275856,    288.44293971,   2440.30749359,     10.77523152,     12.01996005,     40.40680917,    167.01820426,    359.85522679,    307.49358921,    722.06917427,  -2165.20089311,   -863.49398569\n";
  of << "\"h10_18kmgrid_00004              \",\"FREE        \",  4,0,  0.26,    -20.30373524,    288.43750002,   2440.45845846,     13.13973106,     29.08199284,     48.16428609,    -15.37602363,   -225.37896470,    458.45846194,    723.88631453,  -2171.33666372,   -846.83126058\n";
  of << "\"h10_18kmgrid_00005              \",\"FREE        \", 11,0,  0.27,    -19.88503279,    288.43537855,   2440.25461055,      9.16352697,      9.38095145,     35.77893423,     65.44367366,    107.14451061,    254.61054961,    725.68294651,  -2176.99441380,   -830.01334190\n";
  of << "\"h10_18kmgrid_00006              \",\"FREE        \",  4,0,  0.43,    -19.44884948,    288.42285195,   2440.44213372,     12.33181367,     12.56280806,     51.38066945,    247.34955474,    191.97465846,    442.13372042,    727.23871918,  -2183.25255119,   -812.58227343\n";
  of << "\"h10_18kmgrid_00007              \",\"FREE        \", 12,0,  0.35,    -19.04967331,    288.39493819,   2440.85938237,     10.03883691,     11.33891319,     30.31395174,      2.17461409,    103.68747474,    859.38236825,    728.06828186,  -2189.29974701,   -796.66662990\n";
  of << "\"h10_18kmgrid_00008              \",\"FREE        \", 14,0,  0.32,    -18.62418475,    288.39355282,   2441.00912330,      8.56952211,      9.77765460,     26.21020575,     64.74297529,    131.72015893,   1009.12330037,    729.90683721,  -2195.00551885,   -779.55905566\n";
  of << "\"h10_18kmgrid_00009              \",\"FREE        \",  7,0,  0.49,    -18.17238805,    288.40984520,   2441.03690435,     26.66079608,     38.90040525,     51.35927076,    163.25388383,   -344.29970949,   1036.90434538,    732.45789870,  -2200.58711773,   -761.30344298\n";
  of << "\"h10_18kmgrid_00010              \",\"FREE        \",  9,0,  0.33,    -17.77521241,    288.40472252,   2440.45402354,     16.36689434,     22.21955121,     38.10661762,    147.07836787,    249.52524173,    454.02354003,    733.73450382,  -2205.08045363,   -745.03000740\n";
  of << "\"h10_18kmgrid_00011              \",\"FREE        \", 13,0,  0.36,    -17.35298936,    288.39875336,   2441.06313639,      9.48435328,     10.04109830,     30.31133996,     20.64172643,   -263.51408679,   1063.13638653,    735.40122489,  -2210.85824686,   -728.06599052\n";
  of << "\"h10_18kmgrid_00012              \",\"FREE        \",  9,0,  0.28,    -16.91964567,    288.43365994,   2440.48372498,     15.27997856,     18.36879140,     43.24433207,    123.20972642,    -97.45504312,    483.72498256,    738.29285892,  -2215.04466885,   -710.25458913\n";
  of << "\"h10_18kmgrid_00013              \",\"FREE        \",  5,0,  0.14,    -16.48762978,    288.42214180,   2440.59885228,     38.88580515,     57.94135794,    127.35375129,     17.20525015,   -786.00557788,    598.85228379,    739.55383768,  -2220.31570859,   -692.66227926\n";
  of << "\"h10_18kmgrid_00014              \",\"FREE        \", 11,0,  0.27,    -16.07705612,    288.39019863,   2440.43586172,     12.90193336,     14.66413981,     39.06376806,     82.34454221,      5.56743235,    435.86171840,    739.81341258,  -2225.23203557,   -675.82963651\n";
  of << "\"h10_18kmgrid_00015              \",\"FREE        \", 10,0,  0.18,    -15.65552727,    288.42901296,   2440.06587653,      9.43679582,      9.55109852,     46.70429590,      1.53260031,   -243.71165153,     65.87653264,    742.75983347,  -2229.04924119,   -658.45940797\n";
  of << "\"h10_18kmgrid_00016              \",\"FREE        \", 10,0,  0.30,    -15.24198423,    288.40596912,   2440.35014854,     10.32055401,     10.25897740,     42.04205015,     69.81515921,   -194.03902092,    350.14853746,    743.43114325,  -2234.05935436,   -641.55887019\n";
  of << "\"h10_18kmgrid_00017              \",\"FREE        \",  6,0,  0.25,    -14.80237521,    288.43839671,   2440.18093789,     18.47945251,     14.71044214,     74.15725559,    -20.84820688,   -455.01684940,    180.93788716,    746.17864663,  -2238.08699309,   -623.43167091\n";
  of << "\"h10_18kmgrid_00018              \",\"FREE        \",  9,0,  0.19,    -14.39720539,    288.40786616,   2439.88231635,     11.20169802,     11.01092782,     40.96183319,    111.19185674,   -106.66883571,   -117.68365295,    746.26813417,  -2242.33688406,   -606.65879035\n";
  of << "\"h10_18kmgrid_00019              \",\"FREE        \",  5,0,  0.48,    -13.95206780,    288.45896010,   2439.58454770,     13.20332924,     12.57317791,     66.64409749,    156.94216911,    -61.43493615,   -415.45230235,    749.64567999,  -2245.79940119,   -588.20844048\n";
  of << "\"h10_18kmgrid_00020              \",\"FREE        \",  4,0,  0.30,    -13.53734394,    288.45979459,   2439.57441959,     20.20612102,     27.44809985,    114.50905279,    160.43520761,     31.46792240,   -425.58040908,    751.00375201,  -2249.75883151,   -571.05333658\n";
  of << "\"h10_18kmgrid_00021              \",\"FREE        \",  7,0,  0.35,    -13.12054593,    288.40996616,   2439.61946885,     10.48653529,     18.25176273,    103.54698834,    205.77315832,   -239.04797112,   -380.53115193,    750.35287688,  -2254.33467801,   -553.79497330\n";
  of << "\"h10_18kmgrid_00022              \",\"FREE        \",  7,0,  0.36,    -12.68297698,    288.46016400,   2439.10167585,     10.41300363,     11.20509149,     61.41483163,    136.35977888,     45.01065537,   -898.32415343,    753.48494386,  -2257.14320751,   -535.52027532\n";
  of << "\"h10_18kmgrid_00023              \",\"FREE        \",  8,0,  0.28,    -12.28501855,    288.40377605,   2439.17063206,     10.55497324,     11.56534939,     58.63978465,     21.64291983,   -293.61616521,   -829.36793849,    752.44064717,  -2261.42241985,   -518.99430038\n";
  of << "\"h10_18kmgrid_00024              \",\"FREE        \",  9,0,  0.27,    -11.85579907,    288.40176262,   2439.10838851,     10.54925114,     11.86291658,     61.86147175,      7.80146073,   -318.64473262,   -891.61149137,    753.54816341,  -2265.01670288,   -501.11299322\n";
  of << "\"h10_18kmgrid_00025              \",\"FREE        \", 11,0,  0.23,    -11.43398597,    288.40138715,   2439.29498924,      9.31754445,     14.19519926,     22.72013383,    -11.21442304,   -378.54116386,   -705.01076387,    754.73520455,  -2268.63434612,   -483.56283886\n";
  of << "\"h10_18kmgrid_00026              \",\"FREE        \", 12,0,  0.22,    -11.01150092,    288.39887243,   2439.32466332,     10.16152580,     14.11302242,     27.47206892,    -50.20803105,   -524.84921633,   -675.33668451,    755.74973572,  -2272.01680623,   -465.92572609\n";
  of.close();  

  // create input file pdsExportRootGenModified.typ
  of.open(tempDir.path().toStdString() + "/pdsExportRootGenModified.typ");
  of << "PDS_VERSION_ID = Enum\n";
  of << "RECORD_TYPE = Enum\n";
  of << "RECORD_BYTES = Integer\n";
  of << "FILE_RECORDS = Integer\n";
  of << "LABEL_RECORDS = Integer\n";
  of << "INTERCHANGE_FORMAT = Enum\n";
  of << "DATA_SET_ID = String\n";
  of << "PRODUCER_ID = String\n";
  of << "OBSERVATION_ID = String\n";
  of << "PRODUCT_ID = String\n";
  of << "PRODUCT_VERSION_ID = String\n";
  of << "INSTRUMENT_ID = String\n";
  of << "INSTRUMENT_NAME = String\n";
  of << "INSTRUMENT_HOST_ID = String\n";
  of << "TARGET_NAME = String\n";
  of << "MISSION_PHASE_NAME = String\n";
  of << "ORBIT_NUMBER = Integer\n";
  of << "RATIONALE_DESC = String\n";
  of << "SOFTWARE_NAME = String\n";
  of << "ACCEPTED_MEASURES = Integer\n";
  of << "POINT_ID = Character\n";
  of.close();

  // create input config file pdstableformat_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "PdsTableFormatTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));
  
  /////////////////////////////////////////////////////////////////
  // add PdsTableFormat strategy
  PvlObject PdsTableFormat("Strategy");
  PdsTableFormat.addKeyword(PvlKeyword("Name", "PdsTableFormat"));
  PdsTableFormat.addKeyword(PvlKeyword("Type", "PdsTableFormat"));
  PdsTableFormat.addKeyword(PvlKeyword("PdsFormatFile", "\"%1/TestGeneralFormat.txt\""));
  PdsTableFormat.addKeyword(PvlKeyword("PdsFormatFileArgs", "\"outputdir\""));
  PdsTableFormat.addKeyword(PvlKeyword("Delimiter", ","));
  PdsTableFormat.addKeyword(PvlKeyword("IdentityKey", "POINT_ID"));
  PdsTableFormat.addKeyword(PvlKeyword("POINT_ID_BYTES", "32"));
  PdsTableFormat.addKeyword(PvlKeyword("STATUS_BYTES", "12"));

  QStringList stringList1 = {"\"POINT_ID\"","\"STATUS\"","\"ACCEPTED_MEASURES\"",
                             "\"REJECTED_MEASURES\"","\"RESIDUAL_RMS\"",
                             "\"ADJUSTED_LATITUDE\"","\"ADJUSTED_LONGITUDE\"",
                             "\"ADJUSTED_RADIUS\"","\"SIGMA_LATITUDE\"",
                             "\"SIGMA_LONGITUDE\"","\"SIGMA_RADIUS\"",
                             "\"DELTA_LATITUDE\"","\"DELTA_LONGITUDE\"",
                             "\"DELTA_RADIUS\"","\"X\"","\"Y\"","\"Z\""};

  PvlKeyword colList("Column");
  for (auto const &v : stringList1) {
    colList.addValue(v);
  }

  PdsTableFormat.addKeyword(colList);

  QStringList stringList2 = {"\"CHARACTER\"", "\"CHARACTER\"",  "\"ASCII_INTEGER\"",
                             "\"ASCII_INTEGER\"", "\"ASCII_REAL\"", "\"ASCII_REAL\"",
                             "\"ASCII_REAL\"", "\"ASCII_REAL\"", "\"ASCII_REAL\"",
                             "\"ASCII_REAL\"", "\"ASCII_REAL\"", "\"ASCII_REAL\"",
                             "\"ASCII_REAL\"", "\"ASCII_REAL\"", "\"ASCII_REAL\"",
                             "\"ASCII_REAL\"", "\"ASCII_REAL\""};

  PvlKeyword dataType("DataType");
  for (auto const &v : stringList2) {
    dataType.addValue(v);
  }

  PdsTableFormat.addKeyword(dataType);

  QStringList stringList3 = {"\"NONE\"", "\"NONE\"", "\"NONE\"", "\"NONE\"",
                             "\"PIXELS\"", "\"DEGREES\"", "\"DEGREES\"",
                             "\"KILOMETERS\"", "\"METERS\"", "\"METERS\"",
                             "\"METERS\"", "\"METERS\"", "\"METERS\"",
                             "\"METERS\"", "\"KILOMETERS\"", "\"KILOMETERS\"",
                             "\"KILOMETERS\""};

  PvlKeyword unit("Unit");
  for (auto const &v : stringList3) {
    unit.addValue(v);
  }

  PdsTableFormat.addKeyword(unit);

  QStringList stringList4 = {"Unique point identifier for each jigsaw control point.",
      "Status of point. It can be FREE, FIXED or CONSTRAINED.",
      "Number of accepted (non-rejected) measures for the control point."
          " This is actually the number of images that contain the control point"
          " point (also referred to as the point depth).",
      "Number of measures rejected by jigsaw during the bundle"
          " adjustment (if outlier rejection is active)",
      "Root mean square error of residuals for a point. Indicates the"
          " statistical measure of variation in the difference of each measure"
          " within a control point.",
      "Latitude coordinate of the control point. This is the"
          " adjusted location of the control point after jigsaw bundle adjustment.",
      "Longitude coordinate of control point. This is the"
          " adjusted location of the control point after jigsaw bundle adjustment.",
      "Radius of control point. This is the adjusted radius of"
          " the control point after jigsaw bundle adjustment.",
      "Adjusted uncertainty of Latitude coordinate of the control point"
          " after jigsaw bundle adjustment.",
      "Adjusted uncertainty of Longitude coordinate of the control point"
          " after jigsaw bundle adjustment.",
      "Adjusted uncertainty of Radius coordinate of the control point"
          " after jigsaw bundle adjustment.",
      "Latitude adjustment. The number of meters the latitude"
          " coordinate has been adjusted by the jigsaw bundle adjustment.",
      "Longitude adjustment. The number of meters the longitude"
          " coordinate has been adjusted by the jigsaw bundle adjustment.",
      "Radius adjustment. The number of meters the radius has been"
          " adjusted by the jigsaw bundle adjustment.",
      "Body-fixed X coordinate of the vector from the center of the"
          " target body to the surface coordinate location.",
      "Body-fixed Y coordinate of the vector from the center of the"
          " target body to the surface coordinate location.",
      "Body-fixed Z coordinate of the vector from the center of the"
          " target body to the surface coordinate location."};

  PvlKeyword dataDescription("DataDescription");
  for (auto const &v : stringList4) {
    dataDescription.addValue(v);
  }

  PdsTableFormat.addKeyword(dataDescription);
  isisminerObject.addObject(PdsTableFormat);

  /////////////////////////////////////////////////////////////////
  // add 2nd PdsTableFormat strategy (no point id format)
  PdsTableFormat.addKeyword(PvlKeyword("PdsFormatFile", "\"%1/TestNoPointIDFormat.txt\""), Pvl::Replace);
  PdsTableFormat.deleteKeyword("IdentityKey");
  PdsTableFormat.deleteKeyword("POINT_ID_BYTES");
  isisminerObject.addObject(PdsTableFormat);

  /////////////////////////////////////////////////////////////////
  // add 3rd PdsTableFormat strategy (no status bytes format)
  PdsTableFormat.addKeyword(PvlKeyword("PdsFormatFile", "\"%1/TestNoStatusBytesFormat.txt\""), Pvl::Replace);
  PdsTableFormat.addKeyword(PvlKeyword("KeywordFormatFile", "\"%1/pdsExportRootGenModified.typ\""));
  PdsTableFormat.addKeyword(PvlKeyword("KeywordFormatFileArgs", "\"inputdir\""));
  PdsTableFormat.addKeyword(PvlKeyword("POINT_ID_BYTES", "32"));
  PdsTableFormat.deleteKeyword("STATUS_BYTES");
  isisminerObject.addObject(PdsTableFormat);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/pdstableformat_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/pdstableformat_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // read TestGeneralFormat.txt into pvl file
  Pvl testgenformatPvl;
  testgenformatPvl.read(tempDir.path()+ "/TestGeneralFormat.txt");

  // confirm 17 Pvl objects in this file
  EXPECT_EQ(testgenformatPvl.objects(), 17);

  ASSERT_TRUE(testgenformatPvl.hasObject("COLUMN"));
  PvlObject column = testgenformatPvl.findObject("COLUMN");

  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 1);
  EXPECT_EQ(column["NAME"][0].toStdString(), "POINT_ID");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "CHARACTER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 2);
  EXPECT_EQ(int(column["BYTES"]), 32);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Unique point identifier for each jigsaw control point.");
  
  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 2);
  EXPECT_EQ(column["NAME"][0].toStdString(), "STATUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "CHARACTER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 37);
  EXPECT_EQ(int(column["BYTES"]), 12);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Status of point. It can be FREE, FIXED or CONSTRAINED.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 3);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ACCEPTED_MEASURES");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_INTEGER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 51);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Number of accepted (non-rejected) measures for the control"
                   " point. This is actually the number of images that contain"
                   " the control point point (also referred to as the point"
                   " depth).");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 4);
  EXPECT_EQ(column["NAME"][0].toStdString(), "REJECTED_MEASURES");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_INTEGER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 52);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Number of measures rejected by jigsaw during the bundle"
                   " adjustment (if outlier rejection is active)");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 5);
  EXPECT_EQ(column["NAME"][0].toStdString(), "RESIDUAL_RMS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "PIXELS");
  EXPECT_EQ(int(column["START_BYTE"]), 53);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Root mean square error of residuals for a point." 
                   " Indicates the statistical measure of variation in the"
                   " difference of each measure within a control point.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 6);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "DEGREES");
  EXPECT_EQ(int(column["START_BYTE"]), 54);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Latitude coordinate of the control point. This is the"
                   " adjusted location of the control point after jigsaw bundle"
                   " adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 7);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "DEGREES");
  EXPECT_EQ(int(column["START_BYTE"]), 55);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Longitude coordinate of control point. This is the"
                   " adjusted location of the control point after jigsaw bundle"
                   " adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 8);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 56);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Radius of control point. This is the adjusted radius of"
                   " the control point after jigsaw bundle adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 9);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 57);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Latitude coordinate of the control"
                   " point after jigsaw bundle adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 10);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 58);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Longitude coordinate of the"
                   " control point after jigsaw bundle adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 11);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 59);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Radius coordinate of the control"
                   " point after jigsaw bundle adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 12);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 60);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Latitude adjustment. The number of meters the latitude"
                   " coordinate has been adjusted by the jigsaw bundle"
                   " adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 13);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 61);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Longitude adjustment. The number of meters the longitude"
                   " coordinate has been adjusted by the jigsaw bundle"
                   " adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 14);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 62);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Radius adjustment. The number of meters the radius has"
                   " been adjusted by the jigsaw bundle adjustment.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 15);
  EXPECT_EQ(column["NAME"][0].toStdString(), "X");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 63);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed X coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 16);
  EXPECT_EQ(column["NAME"][0].toStdString(), "Y");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 64);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed Y coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  testgenformatPvl.deleteObject("COLUMN");
  column = testgenformatPvl.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 17);
  EXPECT_EQ(column["NAME"][0].toStdString(), "Z");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 65);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed Z coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  // read TestNoPointIDFormat.txt into pvl file
  Pvl noPointIdFormat;
  noPointIdFormat.read(tempDir.path()+ "/TestNoPointIDFormat.txt");

  // confirm 17 Pvl objects in this file
  EXPECT_EQ(noPointIdFormat.objects(), 17);

  ASSERT_TRUE(noPointIdFormat.hasObject("COLUMN"));
  column = noPointIdFormat.findObject("COLUMN");

  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 1);
  EXPECT_EQ(column["NAME"][0].toStdString(), "POINT_ID");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "CHARACTER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 2);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Unique point identifier for each jigsaw control point.");
  
  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 2);
  EXPECT_EQ(column["NAME"][0].toStdString(), "STATUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "CHARACTER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 5);
  EXPECT_EQ(int(column["BYTES"]), 12);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Status of point. It can be FREE, FIXED or CONSTRAINED.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 3);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ACCEPTED_MEASURES");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_INTEGER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 19);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Number of accepted (non-rejected) measures for the control"
                   " point. This is actually the number of images that contain"
                   " the control point point (also referred to as the point"
                   " depth).");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 4);
  EXPECT_EQ(column["NAME"][0].toStdString(), "REJECTED_MEASURES");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_INTEGER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 20);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Number of measures rejected by jigsaw during the bundle"
                   " adjustment (if outlier rejection is active)");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 5);
  EXPECT_EQ(column["NAME"][0].toStdString(), "RESIDUAL_RMS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "PIXELS");
  EXPECT_EQ(int(column["START_BYTE"]), 21);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Root mean square error of residuals for a point." 
                   " Indicates the statistical measure of variation in the"
                   " difference of each measure within a control point.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 6);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "DEGREES");
  EXPECT_EQ(int(column["START_BYTE"]), 22);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Latitude coordinate of the control point. This is the"
                   " adjusted location of the control point after jigsaw bundle"
                   " adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 7);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "DEGREES");
  EXPECT_EQ(int(column["START_BYTE"]), 23);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Longitude coordinate of control point. This is the"
                   " adjusted location of the control point after jigsaw bundle"
                   " adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 8);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 24);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Radius of control point. This is the adjusted radius of"
                   " the control point after jigsaw bundle adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 9);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 25);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Latitude coordinate of the control"
                   " point after jigsaw bundle adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 10);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 26);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Longitude coordinate of the"
                   " control point after jigsaw bundle adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 11);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 27);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Radius coordinate of the control"
                   " point after jigsaw bundle adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 12);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 28);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Latitude adjustment. The number of meters the latitude"
                   " coordinate has been adjusted by the jigsaw bundle"
                   " adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 13);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 29);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Longitude adjustment. The number of meters the longitude"
                   " coordinate has been adjusted by the jigsaw bundle"
                   " adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 14);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 30);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Radius adjustment. The number of meters the radius has"
                   " been adjusted by the jigsaw bundle adjustment.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 15);
  EXPECT_EQ(column["NAME"][0].toStdString(), "X");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 31);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed X coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 16);
  EXPECT_EQ(column["NAME"][0].toStdString(), "Y");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 32);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed Y coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  noPointIdFormat.deleteObject("COLUMN");
  column = noPointIdFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 17);
  EXPECT_EQ(column["NAME"][0].toStdString(), "Z");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 33);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed Z coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  // read TestNoStatusBytesFormat.txt into pvl file
  Pvl noStatusBytesFormat;
  noStatusBytesFormat.read(tempDir.path()+ "/TestNoStatusBytesFormat.txt");

  // confirm 17 Pvl objects in this file
  EXPECT_EQ(noStatusBytesFormat.objects(), 17);

  ASSERT_TRUE(noStatusBytesFormat.hasObject("COLUMN"));
  column = noStatusBytesFormat.findObject("COLUMN");

  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 1);
  EXPECT_EQ(column["NAME"][0].toStdString(), "POINT_ID");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "CHARACTER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 2);
  EXPECT_EQ(int(column["BYTES"]), 32);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Unique point identifier for each jigsaw control point.");
  
  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 2);
  EXPECT_EQ(column["NAME"][0].toStdString(), "STATUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "CHARACTER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 37);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Status of point. It can be FREE, FIXED or CONSTRAINED.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 3);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ACCEPTED_MEASURES");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_INTEGER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 39);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Number of accepted (non-rejected) measures for the control"
                   " point. This is actually the number of images that contain"
                   " the control point point (also referred to as the point"
                   " depth).");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 4);
  EXPECT_EQ(column["NAME"][0].toStdString(), "REJECTED_MEASURES");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_INTEGER");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "NONE");
  EXPECT_EQ(int(column["START_BYTE"]), 40);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Number of measures rejected by jigsaw during the bundle"
                   " adjustment (if outlier rejection is active)");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 5);
  EXPECT_EQ(column["NAME"][0].toStdString(), "RESIDUAL_RMS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "PIXELS");
  EXPECT_EQ(int(column["START_BYTE"]), 41);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Root mean square error of residuals for a point." 
                   " Indicates the statistical measure of variation in the"
                   " difference of each measure within a control point.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 6);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "DEGREES");
  EXPECT_EQ(int(column["START_BYTE"]), 42);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Latitude coordinate of the control point. This is the"
                   " adjusted location of the control point after jigsaw bundle"
                   " adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 7);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "DEGREES");
  EXPECT_EQ(int(column["START_BYTE"]), 43);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Longitude coordinate of control point. This is the"
                   " adjusted location of the control point after jigsaw bundle"
                   " adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 8);
  EXPECT_EQ(column["NAME"][0].toStdString(), "ADJUSTED_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 44);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Radius of control point. This is the adjusted radius of"
                   " the control point after jigsaw bundle adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 9);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 45);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Latitude coordinate of the control"
                   " point after jigsaw bundle adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 10);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 46);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Longitude coordinate of the"
                   " control point after jigsaw bundle adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 11);
  EXPECT_EQ(column["NAME"][0].toStdString(), "SIGMA_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 47);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Adjusted uncertainty of Radius coordinate of the control"
                   " point after jigsaw bundle adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 12);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_LATITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 48);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Latitude adjustment. The number of meters the latitude"
                   " coordinate has been adjusted by the jigsaw bundle"
                   " adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 13);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_LONGITUDE");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 49);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Longitude adjustment. The number of meters the longitude"
                   " coordinate has been adjusted by the jigsaw bundle"
                   " adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 14);
  EXPECT_EQ(column["NAME"][0].toStdString(), "DELTA_RADIUS");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "METERS");
  EXPECT_EQ(int(column["START_BYTE"]), 50);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Radius adjustment. The number of meters the radius has"
                   " been adjusted by the jigsaw bundle adjustment.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 15);
  EXPECT_EQ(column["NAME"][0].toStdString(), "X");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 51);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed X coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 16);
  EXPECT_EQ(column["NAME"][0].toStdString(), "Y");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 52);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed Y coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");

  noStatusBytesFormat.deleteObject("COLUMN");
  column = noStatusBytesFormat.findObject("COLUMN");
  EXPECT_EQ(int(column["COLUMN_NUMBER"]), 17);
  EXPECT_EQ(column["NAME"][0].toStdString(), "Z");
  EXPECT_EQ(column["DATA_TYPE"][0].toStdString(), "ASCII_REAL");
  EXPECT_EQ(column["UNIT"][0].toStdString(), "KILOMETERS");
  EXPECT_EQ(int(column["START_BYTE"]), 53);
  EXPECT_EQ(int(column["BYTES"]), 0);
  EXPECT_EQ(column["DESCRIPTION"][0].toStdString(), "Body-fixed Z coordinate of the vector from the center of"
                   " the target body to the surface coordinate location.");
}


/**
   * IsisminerTestPdsTableReader
   * 
   * Test isisminor pds table reader behavior.
   * 
   * INPUT: 1) index.lbl (pvl format)
   *        2) index.tab
   *        3) pdstablereader_test.conf
   *        4) pvlreader_data.lis
   * 
   * OUTPUT: 1) PdsTableReader_formatted.csv
   *         2) PdsTableReader_highlights.csv
   */
TEST(Isisminer, IsisminerTestPdsTableReader) {
  QTemporaryDir tempDir;

  // create pvlreader_data.lis in tempDir
  ofstream of;
  of.open(tempDir.path().toStdString() + "/pvlreader_data.lis");
  of << "./input/testpvl1.pvl\n";
  of << "./input/testpvl2.pvl\n";
  of << "./input/testpvl3.pvl\n";
  of.close();

  // create input config file pdstablereader_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "PdsTableReaderStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add PdsTableReader strategy
    PvlObject PdsTableReader("Strategy");
    PdsTableReader.addKeyword(PvlKeyword("Name", "PdsTableReaderTestDefault"));
    PdsTableReader.addKeyword(PvlKeyword("Type", "PdsTableReader"));
    PdsTableReader.addKeyword(PvlKeyword("PdslabelFile", "\"%1/index.lbl\""));
    PdsTableReader.addKeyword(PvlKeyword("PdslabelFileArgs", "\"inputdir\""));
    PdsTableReader.addKeyword(PvlKeyword("TableName", "\"INDEX_TABLE\""));
    PdsTableReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    PdsTableReader.addKeyword(PvlKeyword("IdentityArgs", "File_Name"));
    PdsTableReader.addKeyword(PvlKeyword("UseFormattedName", "False"));
    PdsTableReader.addKeyword(PvlKeyword("Description", "Test the default functionality of PdsTableReader"));
    isisminerObject.addObject(PdsTableReader);

    /////////////////////////////////////////////////////////////////
    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest1"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/PdsTableReader_highlights.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Identity, Row, File_Name, FileName, Start_Time, StartTime," 
                                                "Twist_Angle, TwistAngle)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "NULL"));
    isisminerObject.addObject(CsvWriter);

    /////////////////////////////////////////////////////////////////
    // add ResourceManager strategy
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "DoOver"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Clear the list and prepare to read again"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "(ResetDiscard, ToggleDiscard, DeleteDiscard)"));
    isisminerObject.addObject(ResourceManager);

    /////////////////////////////////////////////////////////////////
    // add 2nd PdsTableReader strategy
    PdsTableReader.addKeyword(PvlKeyword("Name", "PdsTableReaderTest2"), Pvl::Replace);
    PdsTableReader.addKeyword(PvlKeyword("IdentityArgs", "Filename"), Pvl::Replace);
    PdsTableReader.addKeyword(PvlKeyword("UseFormattedName", "True"), Pvl::Replace);
    isisminerObject.addObject(PdsTableReader);

    /////////////////////////////////////////////////////////////////
    // add 2nd CsvWriter strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest2"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/PdsTableReader_formatted.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/pdstablereader_test.conf");

  QVector<QString> args = {"config=" + tempDir.path() + "/pdstablereader_test.conf",
                           "parameters=inputdir:data/isisminer/pdstablereader/@outputdir:"
                                     + tempDir.path()
                          };           

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // PdsTableReader_formatted.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/PdsTableReader_formatted.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 8);
  ASSERT_EQ(line.rows(), 11);

  compareCsvLine(line.getRow(0),"Identity,Row,File_Name,FileName,Start_Time,StartTime,Twist_Angle,TwistAngle");
  compareCsvLine(line.getRow(1),"v1787814151_1.qub,0,NULL,v1787814151_1.qub,NULL,2014-08-27T06:03:02.045,NULL,8.528621");
  compareCsvLine(line.getRow(2),"v1787918052_1.qub,1,NULL,v1787918052_1.qub,NULL,2014-08-28T10:54:42.381,NULL,8.294748");
  compareCsvLine(line.getRow(3),"v1787919632_1.qub,2,NULL,v1787919632_1.qub,NULL,2014-08-28T11:21:02.370,NULL,8.305265");
  compareCsvLine(line.getRow(4),"v1787921312_1.qub,3,NULL,v1787921312_1.qub,NULL,2014-08-28T11:49:02.360,NULL,8.315999");
  compareCsvLine(line.getRow(5),"v1787922892_1.qub,4,NULL,v1787922892_1.qub,NULL,2014-08-28T12:15:22.349,NULL,8.326134");
  compareCsvLine(line.getRow(6),"v1787924492_1.qub,5,NULL,v1787924492_1.qub,NULL,2014-08-28T12:42:02.340,NULL,8.333430");
  compareCsvLine(line.getRow(7),"v1787926092_1.qub,6,NULL,v1787926092_1.qub,NULL,2014-08-28T13:08:42.330,NULL,8.340224");
  compareCsvLine(line.getRow(8),"v1787927692_1.qub,7,NULL,v1787927692_1.qub,NULL,2014-08-28T13:35:22.320,NULL,8.352135");
  compareCsvLine(line.getRow(9),"v1787929372_1.qub,8,NULL,v1787929372_1.qub,NULL,2014-08-28T14:03:22.309,NULL,8.363090");

  /////////////////////////////////////////////////////////////////
  // PdsTableReader_highlights.csv
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/PdsTableReader_highlights.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 8);
  ASSERT_EQ(line.rows(), 11);

  compareCsvLine(line.getRow(0),"Identity,Row,File_Name,FileName,Start_Time,StartTime,Twist_Angle,TwistAngle");
  compareCsvLine(line.getRow(1),"v1787814151_1.qub,0,v1787814151_1.qub,NULL,2014-08-27T06:03:02.045,NULL,8.528621,NULL");
  compareCsvLine(line.getRow(2),"v1787918052_1.qub,1,v1787918052_1.qub,NULL,2014-08-28T10:54:42.381,NULL,8.294748,NULL");
  compareCsvLine(line.getRow(3),"v1787919632_1.qub,2,v1787919632_1.qub,NULL,2014-08-28T11:21:02.370,NULL,8.305265,NULL");
  compareCsvLine(line.getRow(4),"v1787921312_1.qub,3,v1787921312_1.qub,NULL,2014-08-28T11:49:02.360,NULL,8.315999,NULL");
  compareCsvLine(line.getRow(5),"v1787922892_1.qub,4,v1787922892_1.qub,NULL,2014-08-28T12:15:22.349,NULL,8.326134,NULL");
  compareCsvLine(line.getRow(6),"v1787924492_1.qub,5,v1787924492_1.qub,NULL,2014-08-28T12:42:02.340,NULL,8.333430,NULL");
  compareCsvLine(line.getRow(7),"v1787926092_1.qub,6,v1787926092_1.qub,NULL,2014-08-28T13:08:42.330,NULL,8.340224,NULL");
  compareCsvLine(line.getRow(8),"v1787927692_1.qub,7,v1787927692_1.qub,NULL,2014-08-28T13:35:22.320,NULL,8.352135,NULL");
  compareCsvLine(line.getRow(9),"v1787929372_1.qub,8,v1787929372_1.qub,NULL,2014-08-28T14:03:22.309,NULL,8.363090,NULL");
  compareCsvLine(line.getRow(10),"v1787930952_1.qub,9,v1787930952_1.qub,NULL,2014-08-28T14:29:42.299,NULL,8.368970,NULL");
}


/**
   * IsisminerTestPvlReader
   * 
   * Test isisminor pvl reader behavior.
   * 
   * INPUT: 1) pvlreader_keylist.lis
   *        2) pvlreader_test.conf
   *        3) testpvl1.pvl
   *        4) testpvl2.pvl
   *        5) testpvl3.pvl
   * 
   * OUTPUT: 1) pvlreader_debug.csv
   *         2) pvlreader_default.csv
   *         3) pvlreader_excludes.csv
   *         4) pvlreader_includes_excludes.csv
   *         5) pvlreader_includes.csv
   *         6) pvlreader_keylistfile.csv
   *         7) pvlreader_no_identity.csv
   */  
TEST(Isisminer, IsisminerTestPvlReader) {
  QTemporaryDir tempDir;

  // create 3 input pvl record files
  ofstream of;
  of.open(tempDir.path().toStdString() + "/testpvl1.pvl");
  of << "Object = Record\n";
  of << "  Number = 0009\n";
  of << "  Group = Info\n";
  of << "    Name = Sarah\n";
  of << "    Age = 29\n";
  of << "    Height = 61\n";
  of << "  EndGroup\n";
  of << "  Group = Extra\n";
  of << "    FavoriteColor = purple\n";
  of << "  EndGroup\n";
  of << "  Object = Internal\n";
  of << "    InternalID = s0009\n";
  of << "    Group = BaseData\n";
  of << "      Data = \"Internal Data for Sarahs Record\"\n";
  of << "    EndGroup\n";
  of << "  EndObject\n";
  of << "EndObject\n";
  of.close();

  of.open(tempDir.path().toStdString() + "/testpvl2.pvl");
  of << "Object = Record\n";
  of << "  Number = 0001\n";
  of << "  Group = Info\n";
  of << "    Name = John\n";
  of << "    Age = 40\n";
  of << "    Height = 68\n";
  of << "  EndGroup\n";
  of << "  Group = Extra\n";
  of << "    FavoriteColor = red\n";
  of << "  EndGroup\n";
  of << "  Object = Internal\n";
  of << "    InternalID = j0001\n";
  of << "    Group = BaseData\n";
  of << "      Data = \"Internal Data for Johns Record\"\n";
  of << "    EndGroup\n";
  of << "  EndObject\n";
  of << "EndObject\n";
  of.close();

  of.open(tempDir.path().toStdString() + "/testpvl3.pvl");
  of << "Object = Record\n";
  of << "  Number = 0002\n";
  of << "  Group = Info\n";
  of << "    Name = ( James, Paul )\n";
  of << "    Age = 18\n";
  of << "    Height = 70\n";
  of << "  EndGroup\n";
  of << "  Group = Extra\n";
  of << "    FavoriteColor = green\n";
  of << "  EndGroup\n";
  of << "  Object = Internal\n";
  of << "    InternalID = j0002\n";
  of << "    Group = BaseData\n";
  of << "      Data = \"Internal Data for James Record\"\n";
  of << "    EndGroup\n";
  of << "  EndObject\n";
  of << "EndObject\n";
  of.close();

  // create list of input pvl files
  of.open(tempDir.path().toStdString() + "/pvlreader_data.lis");
  of << tempDir.path().toStdString() + "/testpvl1.pvl\n";
  of << tempDir.path().toStdString() + "/testpvl2.pvl\n";
  of << tempDir.path().toStdString() + "/testpvl3.pvl\n";
  of.close();

  // create input keylist file
  of.open(tempDir.path().toStdString() + "/pvlreader_keylist.lis");
  of << "Number\nName\nInternalID\n";
  of.close();

  // create input config file pvlreader_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "PvlReaderStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    // add PvlReader strategy
    PvlObject PvlReader("Strategy");
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestDefault"));
    PvlReader.addKeyword(PvlKeyword("Type", "PvlReader"));
    PvlReader.addKeyword(PvlKeyword("FromList", "\"%1/pvlreader_data.lis\""));
    PvlReader.addKeyword(PvlKeyword("FromListArgs", "\"outputdir\""));
    PvlReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    PvlReader.addKeyword(PvlKeyword("IdentityArgs", "Name"));
    PvlReader.addKeyword(PvlKeyword("Description", "Test the default functionality of PvlReader"));
    isisminerObject.addObject(PvlReader);

    // add CsvWriter strategy
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest1"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_default.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Name, Age, Height, FavoriteColor)"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "\"NULL\""));
    isisminerObject.addObject(CsvWriter);

    // add ResourceManager strategy
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Discard all resources from previous test"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"));
    isisminerObject.addObject(ResourceManager);

    // add 2nd ResourceManager strategy
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // add 2nd PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestNoIdentity"), Pvl::Replace);
    PvlReader.deleteKeyword("Identity");
    PvlReader.deleteKeyword("IdentityArgs");
    PvlReader.addKeyword(PvlKeyword("Description", "Test without using Identity keyword"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);

    // add 2nd CsvWriter Strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest2"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_no_identity.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);
  
    // Reset for next test
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // add 3rd PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestDebug"), Pvl::Replace);
    PvlReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    PvlReader.addKeyword(PvlKeyword("IdentityArgs", "\"Height\""));
    PvlReader.addKeyword(PvlKeyword("PvlBaseName", "\"Test\""));
    PvlReader.addKeyword(PvlKeyword("Debug", "True"));
    PvlReader.addKeyword(PvlKeyword("Description", "Test Debug keyword"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);

    // add 3rd CsvWriter Strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest3"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_debug.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);
 
    // Reset for next test
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    
    // add 4th PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestExcludes"), Pvl::Replace);
    PvlReader.deleteKeyword("Identity");
    PvlReader.deleteKeyword("IdentityArgs");
    PvlReader.deleteKeyword("PvlBaseName");
    PvlReader.deleteKeyword("Debug");
    PvlReader.addKeyword(PvlKeyword("Excludes", "(Internal,Extra,Age)"));
    PvlReader.addKeyword(PvlKeyword("Description", "Test Excludes keyword - exclude Object, Group, and Keyword"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);

    // add 4th CsvWriter Strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest4"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_excludes.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Number, Name, Age, Height, FavoriteColor, InternalID, Data)"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    // Reset for next test
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // add 5th PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestIncludes"), Pvl::Replace);
    PvlReader.addKeyword(PvlKeyword("Includes", "(Root, Record, Internal)"));
    PvlReader.deleteKeyword("Excludes");
    PvlReader.addKeyword(PvlKeyword("Description", "Test Includes keyword - include the Root Object, Record Object, Internal Object"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);
    
    // add 5th CsvWriter Strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest5"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_includes.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Number, Name, Age, Height, FavoriteColor, InternalID, Data)"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);
        
    // Reset for next test
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    
    // add 6th PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestIncludesExcludes"), Pvl::Replace);
    PvlReader.addKeyword(PvlKeyword("Includes", "(Root, Record)"), Pvl::Replace);
    PvlReader.addKeyword(PvlKeyword("Excludes", "Extra"));
    PvlReader.addKeyword(PvlKeyword("Description", "Test Includes and Excludes together keyword -" 
                                                    "Include the Root Object (and its Groups / Keywords),"
                                                    "Include the Record Object (and its Groups / Keywords),"
                                                    "Exclude the Extra Group"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);

    // add 6th CsvWriter Strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest6"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_includes_excludes.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    // Reset for next test
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // add 7th PvlReader strategy
    PvlReader.addKeyword(PvlKeyword("Name", "PvlReaderTestKeyListFile"), Pvl::Replace);
    PvlReader.addKeyword(PvlKeyword("KeyListFile", "%1/pvlreader_keylist.lis"));
    PvlReader.addKeyword(PvlKeyword("KeyListFileArgs", "\"inputdir\""));
    PvlReader.deleteKeyword("Includes");
    PvlReader.deleteKeyword("Excludes");
    PvlReader.addKeyword(PvlKeyword("Description", "Test KeyListFile keyword - include only Number, Name, and InternalID"), Pvl::Replace);
    isisminerObject.addObject(PvlReader);
   
    // add 7th CsvWriter Strategy
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteCsvTest7"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/pvlreader_keylistfile.csv\""), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("Keywords", "(Number, Name, Age, Height, FavoriteColor, InternalID, Data)"), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/pvlreader_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/pvlreader_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/pvlreader_debug.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Name,Age,Height,FavoriteColor");
  compareCsvLine(line.getRow(1),"Sarah,29,61,purple");
  compareCsvLine(line.getRow(2),"John,40,68,red");
  compareCsvLine(line.getRow(3),"James,18,70,green");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/pvlreader_default.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Name,Age,Height,FavoriteColor");
  compareCsvLine(line.getRow(1),"Sarah,29,61,purple");
  compareCsvLine(line.getRow(2),"John,40,68,red");
  compareCsvLine(line.getRow(3),"James,18,70,green");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/pvlreader_excludes.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 7);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Number,Name,Age,Height,FavoriteColor,InternalID,Data");
  compareCsvLine(line.getRow(1),"0009,Sarah,29,61,NULL,NULL,NULL");
  compareCsvLine(line.getRow(2),"0001,John,40,68,NULL,NULL,NULL");
  compareCsvLine(line.getRow(3),"0002,James,18,70,NULL,NULL,NULL");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/pvlreader_includes_excludes.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 7);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Number,Name,Age,Height,FavoriteColor,InternalID,Data");
  compareCsvLine(line.getRow(1),"0009,Sarah,29,61,NULL,NULL,NULL");
  compareCsvLine(line.getRow(2),"0001,John,40,68,NULL,NULL,NULL");
  compareCsvLine(line.getRow(3),"0002,James,18,70,NULL,NULL,NULL");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/pvlreader_includes.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 7);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Number,Name,Age,Height,FavoriteColor,InternalID,Data");
  compareCsvLine(line.getRow(1),"0009,NULL,NULL,NULL,NULL,s0009,NULL");
  compareCsvLine(line.getRow(2),"0001,NULL,NULL,NULL,NULL,j0001,NULL");
  compareCsvLine(line.getRow(3),"0002,NULL,NULL,NULL,NULL,j0002,NULL");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/pvlreader_keylistfile.csv",
                   false, 0, ',', false, true);
  
  ASSERT_EQ(line.columns(), 7);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Number,Name,Age,Height,FavoriteColor,InternalID,Data");
  compareCsvLine(line.getRow(1),"0009,Sarah,NULL,NULL,NULL,s0009,NULL");
  compareCsvLine(line.getRow(2),"0001,John,NULL,NULL,NULL,j0001,NULL");
  compareCsvLine(line.getRow(3),"0002,James,NULL,NULL,NULL,j0002,NULL");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/pvlreader_no_identity.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"Name,Age,Height,FavoriteColor");
  compareCsvLine(line.getRow(1),"Sarah,29,61,purple");
  compareCsvLine(line.getRow(2),"John,40,68,red");
  compareCsvLine(line.getRow(3),"James,18,70,green");
}


/**
   * IsisminerTestResourceManager
   * 
   * Tests default behavior.
   * 
   * INPUT: 1) resourceManager_data.csv
   *        2) isisTruth.pvl
   *        3) pvls.lis
   *        4) resourceManager_test.conf
   * 
   * OUTPUT: 1) resourceManager_test_resetDiscard.csv
   *         2) resourceManager_test_toggleDiscard.csv
   *         3) resourceManager_test_deleteAsset.csv
   *         4) resourceManager_test_deleteDiscard.csv
   *         5) resourceManager_test_hasAsset.csv
   */  
TEST_F(IsisminerResourceManager, FunctionalTestIsisminerResourceManager) {

  // create input isisTruth.pvl file
  Pvl isisTruth;
  PvlObject cubePvl("IsisCube");
  cubePvl.addKeyword(PvlKeyword("SpacecraftName", "MARS GLOBAL SURVEYOR"));
  cubePvl.addKeyword(PvlKeyword("InstrumentId", "MOC-WA"));
  cubePvl.addKeyword(PvlKeyword("TargetName", "Mars"));
  cubePvl.addKeyword(PvlKeyword("StartTime", "1997-10-20T10:58:37.46"));
  cubePvl.addKeyword(PvlKeyword("StopTime", "1997-10-20T11:03:44.66"));
  cubePvl.addKeyword(PvlKeyword("CrosstrackSumming", "4"));
  cubePvl.addKeyword(PvlKeyword("DowntrackSumming", "4"));
  cubePvl.addKeyword(PvlKeyword("FocalPlaneTemperature", "213.1"));
  cubePvl.addKeyword(PvlKeyword("GainModeId", "1A"));
  cubePvl.addKeyword(PvlKeyword("LineExposureDuration", "100.000000", "milliseconds"));
  cubePvl.addKeyword(PvlKeyword("MissionPhaseName", "AB-1"));
  cubePvl.addKeyword(PvlKeyword("OffsetModeId", "5"));
  cubePvl.addKeyword(PvlKeyword("SpacecraftClockCountv", "561812335:32"));
  cubePvl.addKeyword(PvlKeyword("RationaleDesc", "OLYMPUS MONS SPECIAL RED WIDE ANGLE"));
  cubePvl.addKeyword(PvlKeyword("FirstLineSample", "673"));
  isisTruth.addObject(cubePvl);
  isisTruth.write(tempDir.path() + "/isisTruth.pvl");

  ofstream of;
  of.open(tempDir.path().toStdString() + "/pvls.lis");
  of << tempDir.path() + "/isisTruth.pvl\n" + tempDir.path() + "/isisTruth.pvl";
  of.close();

  // create input config file resourceManager_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "ResourceManagerStrategyTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    // Add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadResourceManagerData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "%1_%2"));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId, StereoSource)"));
    isisminerObject.addObject(CsvReader);

    // Add Filter strategy (discards anything that isn't SourceA)
    PvlObject Filter("Strategy");
    Filter.addKeyword(PvlKeyword("Name", "TestInclude"));
    Filter.addKeyword(PvlKeyword("Type", "Filter"));
    Filter.addKeyword(PvlKeyword("Description", "Keep all SourceA stereo sets"));
    Filter.addKeyword(PvlKeyword("Keyword", "SourceSet"));
    Filter.addKeyword(PvlKeyword("Include", "\"SourceA\""));
    isisminerObject.addObject(Filter);

    // Add ResourceManager stategy
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Re-activate all discarded Resources."));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    isisminerObject.addObject(ResourceManager);

    // add CsvWriter stategy. Write all input fields. Since ResetDiscard was
    // run, this should be identical to the input
    PvlObject CsvWriter("Strategy");
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestResetDiscard"));
    CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_test_resetDiscard.csv\""));
    CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter.addKeyword(PvlKeyword("DefaultValue", "NULL"));

    QStringList keywordList = {"YearDoy","File","StereoSource","SourceProductId",
                              "SourceSet","CenterLongitude","CenterLatitude",
                              "ObservationType","EmissionAngle","PhaseAngle",
                              "SubSolarGroundAzimuth"};

    PvlKeyword kw("Keywords");
    for (auto const &v : keywordList) {
      kw.addValue(v);
    }

    CsvWriter.addKeyword(kw);
    isisminerObject.addObject(CsvWriter);

    // Add 2nd Filter strategy (identical to 1st above)
    isisminerObject.addObject(Filter);

    // Add ResourceManager strategy. swap activated and deactivated Resources with ToggleDiscard
    ResourceManager.addKeyword(PvlKeyword("Name", "SwapActivated"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "Switch activated and deactivated Resources."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "ToggleDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add 2nd CsvWriter strategy. Write all input fields. Since ToggleDiscard was run after
    // SourceA was selected, all non-SourceA Resources should appear.
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestToggleDiscard"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_test_toggleDiscard.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    // Add AssetSidebar strategy. Adds the "TestB" asset to all SourceB Resources
    // (Since we didn't reset Resources from the last test)
    PvlObject AssetSidebar("Strategy");
    AssetSidebar.addKeyword(PvlKeyword("Name", "AddAssetsB"));
    AssetSidebar.addKeyword(PvlKeyword("Type", "AssetSidebar"));
    AssetSidebar.addKeyword(PvlKeyword("Asset", "\"TestB\""));
    AssetSidebar.addKeyword(PvlKeyword("Operation", "create"));
    AssetSidebar.addKeyword(PvlKeyword("Description", "Add TestB asset to Resource."));

      // Add IsisMiner subobject
      PvlObject isisminerSubObject("IsisMiner");
      isisminerSubObject.addKeyword(PvlKeyword("Name", "AddAssetsTestB"));

        // Add PvlReader subobject
        PvlObject PvlReader("Strategy");
        PvlReader.addKeyword(PvlKeyword("Name", "TestReadingPvls"));
        PvlReader.addKeyword(PvlKeyword("Type", "PvlReader"));
        PvlReader.addKeyword(PvlKeyword("FromList", "\"%1/pvls.lis\""));
        PvlReader.addKeyword(PvlKeyword("FromListArgs", "inputdir"));
        PvlReader.addKeyword(PvlKeyword("Identity", "PvlTestB"));
        PvlReader.addKeyword(PvlKeyword("IdentityArgs", "TargetName"));
        PvlReader.addKeyword(PvlKeyword("Includes", "IsisCube"));  
        isisminerSubObject.addObject(PvlReader);
    AssetSidebar.addObject(isisminerSubObject);    
    isisminerObject.addObject(AssetSidebar);

    // Add ResourceManager strategy. reset everything with ResetDiscard
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "Re-activate all discarded Resources."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add ResourceManager strategy
    ResourceManager.addKeyword(PvlKeyword("Name", "HasTestAssetB"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "De-activate all resources without the TestB asset."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "HasAsset::TestB"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add 3rd CsvWriter strategy. Since only SourceB Resources have the TestB
    // asset, only these Resources appear.
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestHasAsset"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_test_hasAsset.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

    // Add ResourceManager strategy.
    ResourceManager.addKeyword(PvlKeyword("Name", "DeleteTestAssetA"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "Remove the TestA asset from all Resources."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "DeleteAsset::TestA"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add ResourceManager strategy.
    ResourceManager.addKeyword(PvlKeyword("Name", "HasTestAssetA"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "De-activate all resources without the TestA asset."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "HasAsset::TestA"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add 4th CsvWriter strategy.
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestDeleteAsset"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_test_deleteAsset.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);
  
    // Add ResourceManager strategy.
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "Re-activate all discarded Resources."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add 3rd Filter strategy, discards anything that isn't SourceA
    Filter.addKeyword(PvlKeyword("Name", "TestExclude"), Pvl::Replace);
    isisminerObject.addObject(Filter);

    // Add ResourceManager strategy.
    ResourceManager.addKeyword(PvlKeyword("Name", "DeleteSourceA"), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Description", "Delete discarded Resources."), Pvl::Replace);
    ResourceManager.addKeyword(PvlKeyword("Operations", "(DeleteDiscard, ResetDiscard)"), Pvl::Replace);
    isisminerObject.addObject(ResourceManager);

    // Add 5th CsvWriter strategy.
    // Write all input fields. Since DeleteDiscard was run after SourceA was
    // selected, only SourceA Resources should appear.
    CsvWriter.addKeyword(PvlKeyword("Name", "WriteTestDeleteDiscard"), Pvl::Replace);
    CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_test_deleteDiscard.csv\""), Pvl::Replace);
    isisminerObject.addObject(CsvWriter);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/resourceManager_test.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/resourceManager_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/resourceManager_test_resetDiscard.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 11);
  ASSERT_EQ(line.rows(), 13);

  compareCsvLine(line.getRow(0),"YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationType,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth");
  compareCsvLine(line.getRow(1),"2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(2),"2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.0458720107,-35.3972876604,Albedo,28.9490701558,64.4678332012,354.1080763404");
  compareCsvLine(line.getRow(3),"2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(4),"2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(5),"2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521");
  compareCsvLine(line.getRow(6),"2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(7),"2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521");
  compareCsvLine(line.getRow(8),"2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.1875329109,-37.5388662682,Albedo,36.9166983086,77.264668712,334.8882754124");
  compareCsvLine(line.getRow(9),"2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(10),"2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(11),"2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(12),"2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/resourceManager_test_toggleDiscard.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 11);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationType,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth");
  compareCsvLine(line.getRow(1),"2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521");
  compareCsvLine(line.getRow(2),"2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521");
  compareCsvLine(line.getRow(3),"2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(4),"2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/resourceManager_test_deleteAsset.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 11);
  ASSERT_EQ(line.rows(), 1);

  compareCsvLine(line.getRow(0),"YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationType,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/resourceManager_test_deleteDiscard.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(),11);
  ASSERT_EQ(line.rows(), 9);

  compareCsvLine(line.getRow(0),"YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationType,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth");
  compareCsvLine(line.getRow(1),"2012223,EN0253077000M.lev1.cub,EN0253105125M,EN0253077000M,SourceA,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(2),"2012224,EN0253133950M.lev1.cub,EN0253105125M,EN0253133950M,SourceA,10.0458720107,-35.3972876604,Albedo,28.9490701558,64.4678332012,354.1080763404");
  compareCsvLine(line.getRow(3),"2012241,EN0254615176M.lev1.cub,EN0253105125M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(4),"2012223,EN0253077000M.lev1.cub,EN0253133950M,EN0253077000M,SourceA,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(5),"2012241,EN0254615176M.lev1.cub,EN0253133950M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(6),"2012245,EN0254962699M.lev1.cub,EN0253077000M,EN0254962699M,SourceA,16.1875329109,-37.5388662682,Albedo,36.9166983086,77.264668712,334.8882754124");
  compareCsvLine(line.getRow(7),"2012241,EN0254615176M.lev1.cub,EN0253077000M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");
  compareCsvLine(line.getRow(8),"2012241,EN0254615176M.lev1.cub,EN0254962699M,EN0254615176M,SourceA,11.1779652078,-37.1091880701,Monochrome,50.9441707494,89.1594453885,342.9787987839");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/resourceManager_test_hasAsset.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 11);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"YearDoy,File,StereoSource,SourceProductId,SourceSet,CenterLongitude,CenterLatitude,ObservationType,EmissionAngle,PhaseAngle,SubSolarGroundAzimuth");
  compareCsvLine(line.getRow(1),"2012223,EN0253105125M.lev1.cub,EN0253133950M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521");
  compareCsvLine(line.getRow(2),"2012223,EN0253105125M.lev1.cub,EN0253077000M,EN0253105125M,SourceB,11.2303707867,-35.1137564262,Albedo,29.2521121856,64.5519306603,352.8811201521");
  compareCsvLine(line.getRow(3),"2012223,EN0253077000M.lev1.cub,EN0254962699M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
  compareCsvLine(line.getRow(4),"2012223,EN0253077000M.lev1.cub,EN0254615176M,EN0253077000M,SourceB,12.8493256106,-37.3678779418,Albedo,32.1118509543,69.7664968719,351.4272246214");
}


/**
   * IsisminerTestResourceManagerDebug
   * 
   * Tests behavior with Debug = True
   * 
   * INPUT: 1) resourceManager_data.csv
   *        2) resourceManager_debug.conf
   * 
   * OUTPUT: isisminer std output redirected to resourceManager_debug.txt file
   */  
TEST_F(IsisminerResourceManager, FunctionalTestIsisminerResourceManagerDebug) {

  // create input config file resourceManager_debug.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "ReourceManagerDebugTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    // Add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadResourceManagerData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "%1_%2"));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId, StereoSource)"));
    isisminerObject.addObject(CsvReader);

    // Add ResourceManager strategy, reset everything with ResetDiscard
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Debug", "True"));
    ResourceManager.addKeyword(PvlKeyword("Name", "ResetActive"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Re-activate all discarded Resources."));
    ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
    isisminerObject.addObject(ResourceManager);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/resourceManager_debug.conf");

  // redirect isisminer's std::cout to file
  ofstream out(tempDir.path().toStdString() + "/resourceManager_debug.txt");
  streambuf *coutbuf = cout.rdbuf(); //save old buffer
  cout.rdbuf(out.rdbuf());

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/resourceManager_debug.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // close ofstream and reset to standard output again
  out.close();
  cout.rdbuf(coutbuf);

  // try to read debugTextFile
  QFile debugTextFile(tempDir.path() + "/resourceManager_debug.txt");
  try {
    debugTextFile.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  catch (IException &e) {
    FAIL() << "Unable to open sidebar_debug.txt file: " << e.what() << endl;
  }

  // confirm contents of sidebar_debug.txt (skipping timestamps and elapsed times)
  QTextStream in (&debugTextFile);
  const QString content = in.readAll();

  ASSERT_TRUE(content.contains("Creating strategies..."));
  ASSERT_TRUE(content.contains("Finished creating 2 strategies..."));
  ASSERT_TRUE(content.contains("Running CsvReader::ReadResourceManagerData"));
  ASSERT_TRUE(content.contains("Description: Strategy::ReadResourceManagerData is running a CsvReader algorithm."));
  ASSERT_TRUE(content.contains("12 of 0 processed in CsvReader::ReadResourceManagerData"));
  ASSERT_TRUE(content.contains("Running ResourceManager::ResetActive"));
  ASSERT_TRUE(content.contains("Description: Re-activate all discarded Resources."));
  ASSERT_TRUE(content.contains("Running  1 ResourceManager operators..."));
  ASSERT_TRUE(content.contains("Running ResetDiscard:0 resources affected"));
  ASSERT_TRUE(content.contains("0 of 12 processed in ResourceManager::ResetActive"));
  ASSERT_TRUE(content.contains("Session complete in"));
}


/**
   * IsisminerTestResourceManagerIllFormedException
   * 
   * Tests exceptions thrown due to ill-formed Operations keyword
   * 
   * INPUT: 1) resourceManager_data.csv
   *        2) resourceManager_exception_illformed.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: 1) **USER ERROR** Invalid operations requested in ResourceManager.
   *         2) **USER ERROR** ResourceManager::Operator [Unknown] not recognized.  Valid are ResetDiscard, ToggleDiscard, DeleteDiscard and DeleteAsset::AssetName.
   *         3) **USER ERROR** ResourceManager::Operator [ResetDiscard::C::D] is ill-formed. 
   */  
TEST_F(IsisminerResourceManager, FunctionalTestIsisminerResourceManagerIllFormedException) {

  // create input config file resourceManager_exception_illformed.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "ResourceManagerIllFormedTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    // Add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadResourceManagerData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "%1_%2"));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId, StereoSource)"));
    isisminerObject.addObject(CsvReader);

    // Add ResourceManager strategy, reset everything with ResetDiscard
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "IllFormedOperators"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "Ill-formed empty valued operator"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "(ResetDiscard::C::D, Unknown)"));
    isisminerObject.addObject(ResourceManager);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/resourceManager_exception_illformed.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/resourceManager_exception_illformed.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Invalid operations requested in ResourceManager"));
    EXPECT_TRUE(e.toString().toLatin1().contains("ResourceManager::Operator [Unknown] not recognized.  Valid are ResetDiscard, ToggleDiscard, DeleteDiscard and DeleteAsset::AssetName"));
    EXPECT_TRUE(e.toString().toLatin1().contains("ResourceManager::Operator [ResetDiscard::C::D] is ill-formed"));
  }
  catch(...) {
    FAIL() << "Expected an IException with message:"
              " \"**USER ERROR** Invalid operations requested in ResourceManager.\n"
              " **USER ERROR** ResourceManager::Operator [Unknown] not recognized.  Valid are ResetDiscard, ToggleDiscard, DeleteDiscard and DeleteAsset::AssetName.\n"
              " **USER ERROR** ResourceManager::Operator [ResetDiscard::C::D] is ill-formed.\"";
  }
}


/**
   * IsisminerTestResourceManagerNoAssetException
   * 
   * Tests exceptions thrown when Operations "DeleteAsset" and "HasAsset" do
   * not have associated names.
   * 
   * INPUT: 1) resourceManager_data.csv
   *        2) resourceManager_exception_notargetasset.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: 1) **USER ERROR** Invalid operations requested in ResourceManager
   *         2) **USER ERROR** ResourceManager HasAsset requires an asset name.  Operation [HasAsset] is ill-formed.
   *         3) **USER ERROR** ResourceManager DeleteAsset requires an asset name.  Operation [DeleteAsset] is ill-formed.
   */  
TEST_F(IsisminerResourceManager, FunctionalTestIsisminerResourceManagerNoAssetException) {

  // create input config file resourceManager_exception_notargetasset.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "ReourceManagerNoTargetAssetTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    // Add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadResourceManagerData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/resourceManager_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "False"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    CsvReader.addKeyword(PvlKeyword("Identity", "%1_%2"));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "(SourceProductId, StereoSource)"));
    isisminerObject.addObject(CsvReader);

    // Add CsvReader strategy, reset everything with ResetDiscard
    PvlObject ResourceManager("Strategy");
    ResourceManager.addKeyword(PvlKeyword("Name", "NoTargetAsset"));
    ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
    ResourceManager.addKeyword(PvlKeyword("Description", "DeleteAsset and HasAsset must specify an asset"));
    ResourceManager.addKeyword(PvlKeyword("Operations", "(DeleteAsset, HasAsset)"));
    isisminerObject.addObject(ResourceManager);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/resourceManager_exception_notargetasset.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/resourceManager_exception_notargetasset.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("**USER ERROR** Invalid operations requested in ResourceManager"));
    EXPECT_TRUE(e.toString().toLatin1().contains("**USER ERROR** ResourceManager HasAsset requires an asset name.  Operation [HasAsset] is ill-formed."));
    EXPECT_TRUE(e.toString().toLatin1().contains("**USER ERROR** ResourceManager DeleteAsset requires an asset name.  Operation [DeleteAsset] is ill-formed."));    
  }
  catch(...) {
    FAIL() << "Expected an IException with message:"
              " \"**USER ERROR** Invalid operations requested in ResourceManager.\n"
              " **USER ERROR** ResourceManager HasAsset requires an asset name.  Operation [HasAsset] is ill-formed.\n"
              " **USER ERROR** ResourceManager DeleteAsset requires an asset name.  Operation [DeleteAsset] is ill-formed.\"";
  }
}


/**
   * IsisminerTestSidebar
   * 
   * Tests normal behavior
   * 
   * INPUT: 1) sidebar_data.csv
   *        2) sidebar_data2.csv
   *        3) calculate_area.conf
   *        4) sidebar_test.conf
   * 
   * OUTPUT: 1) sidebar_test.csv
   *         2) sidebar_savedelete_false.csv
   *         3) sidebar_savepoint_false.csv
   * 
   */  
TEST(Isisminer, IsisminerTestSidebar) {
  QTemporaryDir tempDir;

  // create input csv files
  ofstream of;
  of.open(tempDir.path().toStdString() + "/sidebar_data.csv");
  of << "Circle,Radius\n1,4\n2,1.5\n3,1\n4,0\n";
  of << "# This resource should not be discarded, as savepoints are used in the .conf file.\n";
  of << "5,NULL\n";
  of.close();

  of.open(tempDir.path().toStdString() + "/sidebar_data2.csv");
  of << "Circle, Radius\n6,14\n7,18\n";
  of.close();

  // create input config file calculate_area.conf in tempDir
  Pvl conf;
  PvlObject isisminerCalcArea("IsisMiner");
  PvlObject Calculator("Strategy");
  Calculator.addKeyword(PvlKeyword("Name", "CalculateArea"));
  Calculator.addKeyword(PvlKeyword("Type", "Calculator"));
  Calculator.addKeyword(PvlKeyword("Equation", "pi*Radius^2"));
  Calculator.addKeyword(PvlKeyword("Result", "Area"));
  PvlGroup initializers("Initializers");
  initializers.addKeyword(PvlKeyword("Area", "NULL"));
  Calculator.addGroup(initializers);
  isisminerCalcArea.addObject(Calculator);
  conf.addObject(isisminerCalcArea);
  conf.write(tempDir.path() + "/calculate_area.conf");

  // create input config file sidebar_test.conf in tempDir
  Pvl conf1;
  PvlObject isisminerSidebarTest("IsisMiner");
  isisminerSidebarTest.addKeyword(PvlKeyword("Name", "SidebarStrategyTest"));
  isisminerSidebarTest.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    // Add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadSidebarData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/sidebar_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "True"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    isisminerSidebarTest.addObject(CsvReader);

    // Add Sidebar1 object
    PvlObject Sidebar1("Strategy");
    Sidebar1.addKeyword(PvlKeyword("Name", "TestIsisMinerObject"));
    Sidebar1.addKeyword(PvlKeyword("Type", "Sidebar"));
    Sidebar1.addKeyword(PvlKeyword("SavePoint", "True"));
    Sidebar1.addKeyword(PvlKeyword("SaveDelete", "True"));
  
      // Add isisminer subobject
      PvlObject isisminerSubObject("IsisMiner");
      Calculator.addKeyword(PvlKeyword("Name", "CalculateCircumference"), Pvl::Replace);
      Calculator.addKeyword(PvlKeyword("Equation", "2*pi*Radius"), Pvl::Replace);
      Calculator.addKeyword(PvlKeyword("Result", "Circumference"));
      Calculator.deleteGroup("Initializers");
      initializers.deleteKeyword("Area");
      initializers.addKeyword(PvlKeyword("Circumference", "NULL"));
      Calculator.addGroup(initializers);
      isisminerSubObject.addObject(Calculator);
      Sidebar1.addObject(isisminerSubObject);
      isisminerSidebarTest.addObject(Sidebar1);

      // Add Sidebar2 object
      PvlObject Sidebar2("Strategy");
      Sidebar2.addKeyword(PvlKeyword("Name", "TestConfigurationFile"));
      Sidebar2.addKeyword(PvlKeyword("Type", "Sidebar"));
      Sidebar2.addKeyword(PvlKeyword("SavePoint", "True"));
      Sidebar2.addKeyword(PvlKeyword("SaveDelete", "True"));
      Sidebar2.addKeyword(PvlKeyword("StrategyConfigFile", "\"%1/calculate_area.conf\""));
      Sidebar2.addKeyword(PvlKeyword("StrategyConfigFileArgs", "\"inputdir\""));
      isisminerSidebarTest.addObject(Sidebar2);

      // Add CsvWriter object
      PvlObject CsvWriter("Strategy");
      CsvWriter.addKeyword(PvlKeyword("Name", "WriteResources"));
      CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
      CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/sidebar_test.csv\""));
      CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
      CsvWriter.addKeyword(PvlKeyword("Mode", "Create"));
      CsvWriter.addKeyword(PvlKeyword("Header", "True"));
      CsvWriter.addKeyword(PvlKeyword("KeyWords", "(Circle, Radius, Circumference, Area)"));
      CsvWriter.addKeyword(PvlKeyword("Delimiter", ","));
      CsvWriter.addKeyword(PvlKeyword("DefaultValue", "NULL"));
      isisminerSidebarTest.addObject(CsvWriter);

      // Add Sidebar3 object
      PvlObject Sidebar3("Strategy");
      Sidebar3.addKeyword(PvlKeyword("Name", "TestSavePointFalse"));
      Sidebar3.addKeyword(PvlKeyword("Type", "Sidebar"));
      Sidebar3.addKeyword(PvlKeyword("Description", "Tests the SavePoint keyword with False"));
      Sidebar3.addKeyword(PvlKeyword("SavePoint", "False"));

      // Add IsisMinerSubobject1
      PvlObject isisminerSubObject1("IsisMiner");
      PvlObject Calculator2("Strategy");
      Calculator2.addKeyword(PvlKeyword("Name", "CalculateVolume"));
      Calculator2.addKeyword(PvlKeyword("Type", "Calculator"));
      Calculator2.addKeyword(PvlKeyword("Equation", "4*Area*Radius/3"));
      Calculator2.addKeyword(PvlKeyword("Result", "Volume"));
      initializers.deleteKeyword("Circumference");
      initializers.addKeyword(PvlKeyword("Volume", "NULL"));
      Calculator2.addGroup(initializers);
      isisminerSubObject1.addObject(Calculator2);
      Sidebar3.addObject(isisminerSubObject1);
      isisminerSidebarTest.addObject(Sidebar3);

      // Add CsvWriter2 object
      PvlObject CsvWriter2("Strategy");
      CsvWriter2.addKeyword(PvlKeyword("Name", "WriteResourcesSavePointFalse"));
      CsvWriter2.addKeyword(PvlKeyword("Type", "CsvWriter"));
      CsvWriter2.addKeyword(PvlKeyword("Description", "Circle 5 should have been discarded when calculating volume"));
      CsvWriter2.addKeyword(PvlKeyword("CsvFile", "\"%1/sidebar_savepoint_false.csv\""));
      CsvWriter2.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
      CsvWriter2.addKeyword(PvlKeyword("Mode", "Create"));
      CsvWriter2.addKeyword(PvlKeyword("Header", "True"));
      CsvWriter2.addKeyword(PvlKeyword("KeyWords", "(Circle, Radius, Circumference, Area, Volume)"));
      CsvWriter2.addKeyword(PvlKeyword("Delimiter", ","));
      CsvWriter2.addKeyword(PvlKeyword("DefaultValue", "NULL"));
      isisminerSidebarTest.addObject(CsvWriter2);

    // Add Sidebar4 object
    PvlObject Sidebar4("Strategy");
    Sidebar4.addKeyword(PvlKeyword("Name", "TestSaveDeleteFalse"));
    Sidebar4.addKeyword(PvlKeyword("Type", "Sidebar"));
    Sidebar4.addKeyword(PvlKeyword("Description", "Read in resources in sidebar and keep them active"));
    Sidebar4.addKeyword(PvlKeyword("SaveDelete", "False"));
  
    // add IsisMiner subobject
    PvlObject isisminerSubObject2("IsisMiner");
  
    PvlObject CsvReader1("Strategy");
    CsvReader1.addKeyword(PvlKeyword("Name", "ReadResourcesFromSidebarConfig"));
    CsvReader1.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader1.addKeyword(PvlKeyword("CsvFile", "\"%1/sidebar_data2.csv\""));
    CsvReader1.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader1.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader1.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader1.addKeyword(PvlKeyword("IgnoreComments", "True"));
    CsvReader1.addKeyword(PvlKeyword("Delimiter", ","));
    isisminerSubObject2.addObject(CsvReader1);
    Sidebar4.addObject(isisminerSubObject2);
    isisminerSidebarTest.addObject(Sidebar4);

    // add CsvWriter3 object
    PvlObject CsvWriter3("Strategy");
    CsvWriter3.addKeyword(PvlKeyword("Name", "WriteResourcesFromSidebarConfig"));
    CsvWriter3.addKeyword(PvlKeyword("Type", "CsvWriter"));
    CsvWriter3.addKeyword(PvlKeyword("Description", "Output will contain active resources circles 6 and 7 from previous sidebar"));
    CsvWriter3.addKeyword(PvlKeyword("CsvFile", "\"%1/sidebar_savedelete_false.csv\""));
    CsvWriter3.addKeyword(PvlKeyword("CsvFileArgs", "\"outputdir\""));
    CsvWriter3.addKeyword(PvlKeyword("Mode", "Create"));
    CsvWriter3.addKeyword(PvlKeyword("Header", "True"));
    CsvWriter3.addKeyword(PvlKeyword("KeyWords", "(Circle, Radius)"));
    CsvWriter3.addKeyword(PvlKeyword("Delimiter", ","));
    CsvWriter3.addKeyword(PvlKeyword("DefaultValue", "NULL"));
    isisminerSidebarTest.addObject(CsvWriter3);

  conf1.addObject(isisminerSidebarTest);
  conf1.write(tempDir.path() + "/sidebar_test.conf");

  ifstream f3(tempDir.path().toStdString() + "/sidebar_test.conf");
  cout << f3.rdbuf() << endl;
  f3.close();

  // isisminer run
  QVector<QString> args = {"config=" + tempDir.path() + "/sidebar_test.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/sidebar_test.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 4);
  ASSERT_EQ(line.rows(), 6);

  compareCsvLine(line.getRow(0),"Circle,Radius,Circumference,Area");
  compareCsvLine(line.getRow(1),"1,4,25.1327,50.2655");
  compareCsvLine(line.getRow(2),"2,1.5,9.42478,7.06858");
  compareCsvLine(line.getRow(3),"3,1,6.28319,3.14159");
  compareCsvLine(line.getRow(4),"4,0,0,0");
  compareCsvLine(line.getRow(5),"5,NULL,NULL,NULL");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/sidebar_savedelete_false.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 2);
  ASSERT_EQ(line.rows(), 7);

  compareCsvLine(line.getRow(0),"Circle,Radius");
  compareCsvLine(line.getRow(1),"1,4");
  compareCsvLine(line.getRow(2),"2,1.5");
  compareCsvLine(line.getRow(3),"3,1");
  compareCsvLine(line.getRow(4),"4,0");
  compareCsvLine(line.getRow(5),"6,14");
  compareCsvLine(line.getRow(6),"7,18");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/sidebar_savepoint_false.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 5);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"Circle,Radius,Circumference,Area,Volume");
  compareCsvLine(line.getRow(1),"1,4,25.1327,50.2655,268.083");
  compareCsvLine(line.getRow(2),"2,1.5,9.42478,7.06858,14.1372");
  compareCsvLine(line.getRow(3),"3,1,6.28319,3.14159,4.18879");
  compareCsvLine(line.getRow(4),"4,0,0,0,0");
}


/**
   * IsisminerTestSidebarException
   * 
   * Tests exception
   * 
   * INPUT: sidebar_exception.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: No IsisMiner strategies found in TestException Sidebar.
    */

TEST(Isisminer, IsisminerTestSidebarException) {
  QTemporaryDir tempDir;

  // create input config file sidebar_exception.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "SidebarStrategyExceptionTest"));

    // Add Sidebar strategy
    PvlObject Sidebar("Strategy");
    Sidebar.addKeyword(PvlKeyword("Name", "TestException"));
    Sidebar.addKeyword(PvlKeyword("Type", "Sidebar"));
    Sidebar.addKeyword(PvlKeyword("Description", "Test Sidebar without an IsisMiner object or StrategyConfigFile"), Pvl::Replace);
    isisminerObject.addObject(Sidebar);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/sidebar_exception.conf");

  QVector<QString> args = {"config=" + tempDir.path() + "/sidebar_exception.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  // isisminer run
  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("No IsisMiner strategies found"));
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"No IsisMiner strategies found in TestException Sidebar.\"";
  }
}


/**
   * IsisminerTestSidebarDebug

   * Tests Debug = True
   * 
   * INPUT: 1) sidebar_data.csv
   *        2) sidebar_debug.conf
   * 
   * OUTPUT: Application output redirected to text file sidebar_debug.txt
   */
TEST(Isisminer, IsisminerTestSidebarDebug) {
  QTemporaryDir tempDir;

  // create input csv files
  ofstream of;
  of.open(tempDir.path().toStdString() + "/sidebar_data.csv");
  of << "Circle,Radius\n1,4\n2,1.5\n3,1\n4,0\n";
  of << "# This resource should not be discarded, as savepoints are used in the .conf file.\n";
  of << "5,NULL\n";
  of.close();  

  // create input config file sidebar_debug.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "SidebarStrategyDebugTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "inputdir"));

    // Add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadSidebarData"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/sidebar_data.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("SkipLines", "0"));
    CsvReader.addKeyword(PvlKeyword("IgnoreComments", "True"));
    CsvReader.addKeyword(PvlKeyword("Delimiter", ","));
    isisminerObject.addObject(CsvReader);

    // add Sidebar strategy
    PvlObject Sidebar("Strategy");
    Sidebar.addKeyword(PvlKeyword("Debug", "True"));
    Sidebar.addKeyword(PvlKeyword("Name", "TestSidebarDebug"));
    Sidebar.addKeyword(PvlKeyword("Type", "Sidebar"));
    Sidebar.addKeyword(PvlKeyword("Description", "Testing Sidebar's debug conditionals"));
    Sidebar.addKeyword(PvlKeyword("SavePoint", "True"));
    Sidebar.addKeyword(PvlKeyword("SaveDelete", "True"));

    // Add isisminer subobject with Calculator subgroup
    PvlObject isisminerSubObject("IsisMiner");
    PvlObject Calculator("Strategy");
    Calculator.addKeyword(PvlKeyword("Name", "CalculateCircumference"));
    Calculator.addKeyword(PvlKeyword("Type", "Calculator"));
    Calculator.addKeyword(PvlKeyword("Equation", "2*pi*Radius"));
    Calculator.addKeyword(PvlKeyword("Result", "Circumference"));
    PvlGroup initializers("Initializers"); 
    initializers.addKeyword(PvlKeyword("Circumference", "NULL"));
    Calculator.addGroup(initializers);
    isisminerSubObject.addObject(Calculator);
    Sidebar.addObject(isisminerSubObject);
    isisminerObject.addObject(Sidebar);
  
  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/sidebar_debug.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/sidebar_debug.conf",
                           "parameters=inputdir:" + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  // redirect isisminer's std::cout to file
  ofstream out(tempDir.path().toStdString() + "/sidebar_debug.txt");
  streambuf *coutbuf = cout.rdbuf(); //save old buf
  cout.rdbuf(out.rdbuf());

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }
  
  // close ofstream and reset to standard output again
  out.close();
  cout.rdbuf(coutbuf);

  // try to read output text file
  QFile debugTextFile(tempDir.path() + "/sidebar_debug.txt");
  try {
    debugTextFile.open(QIODevice::ReadOnly | QIODevice::Text);
  }
  catch (IException &e) {
    FAIL() << "Unable to open sidebar_debug.txt file: " << e.what() << endl;
  }

  // confirm contents of sidebar_debug.txt (skipping timestamps and elapsed times)
  QTextStream in (&debugTextFile);
  const QString content = in.readAll();
  ASSERT_TRUE(content.contains("Creating strategies..."));
  ASSERT_TRUE(content.contains("Loading IsisMiner Objects from input CONFIG file..."));
  ASSERT_TRUE(content.contains("Sidebar::TestSidebarDebug::StrategiesLoaded = 1"));
  ASSERT_TRUE(content.contains("Finished creating 3 strategies..."));
  ASSERT_TRUE(content.contains("Running CsvReader::ReadSidebarData"));
  ASSERT_TRUE(content.contains("Description: Strategy::ReadSidebarData is running a CsvReader algorithm."));
  ASSERT_TRUE(content.contains("5 of 0 processed in CsvReader::ReadSidebarData"));
  ASSERT_TRUE(content.contains("Running Sidebar::TestSidebarDebug"));
  ASSERT_TRUE(content.contains("Description: Testing Sidebar's debug conditionals"));
  ASSERT_TRUE(content.contains("Running SideBar::Calculator::CalculateCircumference"));
  ASSERT_TRUE(content.contains("4 of 5 processed in Calculator::CalculateCircumference"));
  ASSERT_TRUE(content.contains("4 of 5 processed in Sidebar::TestSidebarDebug"));
  ASSERT_TRUE(content.contains("Session complete in"));
}


/**
   * IsisminerTestStereoPair
   * 
   * Tests Stereopair perimeter behavior.
   * 
   * INPUT: 1) degaus_stereo_perimeter.csv, in isis/tests/data/isisminer/stereopair
   *        2) stereopair_test.conf
   * 
   * OUTPUT: EN0211850377M.csv; EN0211937411M.csv; EN0211937488M.csv
   *         EN0211980850M.csv; EN0211980977M.csv; EN0211981012M.csv
   *         EW0211937827G.csv; EW0214460053G.csv; EW0216448475G.csv
   *         EW0227091446G.csv; EW0227176126G.csv; EW0227422989G.csv
   * 
   * NOTE: Original test has 78 output files, I chose 12 to check,
   *        evenly distributed across the original 78.
   */  
TEST(Isisminer, IsisminerTestStereoPair) {
  QTemporaryDir tempDir;

  // create input isisminer config file stereopair_test.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "StereoPairAppTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadDegausPerimeter"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("Description", "Read Degaus data"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/degaus_stereo_perimeter.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "\"SourceProductId\""));
    CsvReader.addKeyword(PvlKeyword("GisGeometryRef", "GisFootprint"));
    CsvReader.addKeyword(PvlKeyword("GisType", "WKB"));
    CsvReader.addKeyword(PvlKeyword("RemoveGisKeywordAfterImport", "false"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add StereoPair strategy
    PvlObject StereoPair("Strategy");
    StereoPair.addKeyword(PvlKeyword("Name", "DegausStereoPairs"));
    StereoPair.addKeyword(PvlKeyword("Type", "StereoPair"));
    StereoPair.addKeyword(PvlKeyword("Description", "Find all stereo pairs in the Degaus Crater region"));
    StereoPair.addKeyword(PvlKeyword("OverlapMerge", "None"));
    StereoPair.addKeyword(PvlKeyword("ShowProgress", "True"));
    StereoPair.addKeyword(PvlKeyword("PixelPrecisionMatch", "0.2"));
    StereoPair.addKeyword(PvlKeyword("Asset", "StereoPairs"));
    StereoPair.addKeyword(PvlKeyword("Equation", "imagestrengthrank * 1.0 + stereostrengthrank * 1.0"));
    StereoPair.addKeyword(PvlKeyword("Result", "StereoPairRank"));

      /////////////////////////////////////////////////////////////////
      // add First level constraints/strengths group
      PvlGroup ImageStrength("ImageStrength");
      ImageStrength.addKeyword(PvlKeyword("Incidence", "(30.0,65.0,50.0)"));
      ImageStrength.addKeyword(PvlKeyword("Emission", "(0.0,55.0,15.0)"));
      ImageStrength.addKeyword(PvlKeyword("Phase", "(15.0,120.00,30.0)"));
      ImageStrength.addKeyword(PvlKeyword("Resolution", "(25.0,5000.0,200.0)"));
      StereoPair.addGroup(ImageStrength);

      /////////////////////////////////////////////////////////////////
      // add Second level constraints/strengths group
      PvlGroup StereoStrength("StereoStrength");
      StereoStrength.addKeyword(PvlKeyword("ResolutionRatio", "(1.0,2.5,1.0)"));
      StereoStrength.addKeyword(PvlKeyword("ParallaxHeightRatio", "(0.1,1.0,0.4)"));
      StereoStrength.addKeyword(PvlKeyword("ShadowTipDistance", "(0.0,2.58,0.0)"));
      StereoStrength.addKeyword(PvlKeyword("OverlapRatio", "(0.1,1.0,0.5)"));
      StereoPair.addGroup(StereoStrength);

      /////////////////////////////////////////////////////////////////
      // add KeywordMap group
      PvlGroup KeywordMap("KeywordMap");
      KeywordMap.addKeyword(PvlKeyword("Resolution", "PixelResolution"));
      KeywordMap.addKeyword(PvlKeyword("Emission", "EmissionAngle"));
      KeywordMap.addKeyword(PvlKeyword("Incidence", "IncidenceAngle"));
      KeywordMap.addKeyword(PvlKeyword("Phase", "PhaseAngle"));
      StereoPair.addGroup(KeywordMap);

      // add IsisMiner sub object
      PvlObject isisminerSubObject1("IsisMiner");
      
      /////////////////////////////////////////////////////////////////
      // add ResourceManager strategy to isisminerSubObject
      PvlObject ResourceManager("Strategy");
      ResourceManager.addKeyword(PvlKeyword("Name", "PairCandidates"));
      ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
      ResourceManager.addKeyword(PvlKeyword("Description", "Choose all resources to determine stereo"));
      ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
      isisminerSubObject1.addObject(ResourceManager);
      StereoPair.addObject(isisminerSubObject1);
  isisminerObject.addObject(StereoPair);

  /////////////////////////////////////////////////////////////////
  // add AssetSidebar strategy
  PvlObject AssetSidebar("Strategy");
  AssetSidebar.addKeyword(PvlKeyword("Name", "WriteStereoFiles"));
  AssetSidebar.addKeyword(PvlKeyword("Type", "AssetSidebar"));
  AssetSidebar.addKeyword(PvlKeyword("Description", "Process all stereo pair assets"));
  AssetSidebar.addKeyword(PvlKeyword("Asset", "StereoPairs"));

  // add IsisMiner sub object
  PvlObject isisminerSubObject2("IsisMiner");

    /////////////////////////////////////////////////////////////////
    // add NumericalSort strategy
    PvlObject NumericalSort("Strategy");
    NumericalSort.addKeyword(PvlKeyword("Name", "SortStereoPairs"));
    NumericalSort.addKeyword(PvlKeyword("Type", "NumericalSort"));
    NumericalSort.addKeyword(PvlKeyword("Description", "Sort stereo pairs by StereoPairRank"));
    NumericalSort.addKeyword(PvlKeyword("SortKey", "StereoPairRank"));
    NumericalSort.addKeyword(PvlKeyword("Order", "Descending"));
    isisminerSubObject2.addObject(NumericalSort);

        // add CsvWriter strategy
        PvlObject CsvWriter("Strategy");
        CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
        CsvWriter.addKeyword(PvlKeyword("Name", "WritePairs"));
        CsvWriter.addKeyword(PvlKeyword("Description", "Write individual files with stereo pair data"));
        CsvWriter.addKeyword(PvlKeyword("SkipEmptyLists", "True"));
        CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/%2.csv\""));
        CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "(outputdir, Identity)"));

        QStringList keywordList = {"SourceProductIdA", "SourceProductIdB", "StereoPairRank",
                          "ImageStrengthRank", "StereoStrengthRank",
                          "ParallaxHeightRatio", "ShadowTipDistance", "ResolutionRatio", 
                          "DeltaSolarAzimuth", "DeltaSpacecraftAzimuth", 
                          "VerticalPrecision", "StereoAngle",
                          "ImageStrengthRankA", "ImageStrengthRankB",
                          "IncidenceRankA", "IncidenceRankB",
                          "EmissionRankA", "EmissionRankB",
                          "PhaseRankA", "PhaseRankB",
                          "OverlapRatioA", "OverlapRatioB",
                          "YearDoyA", "YearDoyB", 
                          "CenterLongitudeA", "CenterLatitudeA",
                          "CenterLongitudeB", "CenterLatitudeB",
                          "PixelResolutionA", "PixelResolutionB", 
                          "EmissionAngleA", "EmissionAngleB",
                          "IncidenceAngleA", "IncidenceAngleB",
                          "PhaseAngleA", "PhaseAngleB"};

        PvlKeyword kw("Keywords");
        for (auto const &v : keywordList) {
          kw.addValue(v);
        }

        CsvWriter.addKeyword(kw);
        isisminerSubObject2.addObject(CsvWriter);

    AssetSidebar.addObject(isisminerSubObject2);
  
  isisminerObject.addObject(AssetSidebar);
  conf.addObject(isisminerObject);

  conf.write(tempDir.path() + "/stereopair_test.conf");

  QVector<QString> args = {"config=" + tempDir.path() + "/stereopair_test.conf",
                           "parameters=inputdir:data/isisminer/stereopair/@outputdir:"
                                     + tempDir.path()
                          };
  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/EN0211850377M.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN0211850377M,EN0211980858M,6.04511,2.97192696,3.07318550,0.25542367,0.23099652,1.08041201,2.70548073,172.96275314,87.11733801,15.56231138,3.03926028,2.90459364,0.40900938,0.56020733,0.88477248,0.72858886,0.76187233,0.63407234,0.22864379,0.26972296,2011111,2011112,240.14716310,29.55869413,239.00116236,29.90665361,121.30918488,112.28048500,10.39089946,4.14355464,61.81981227,58.79585331,51.43148998,62.93348883");
  compareCsvLine(line.getRow(2),"EN0211850377M,EW0227133784G,5.15031,2.93774017,2.21257008,0.23196233,0.11582761,2.26280480,2.65273296,84.32235086,170.64489722,17.83399881,3.03926028,2.83622006,0.40900938,0.36245308,0.88477248,0.85415281,0.76187233,0.63513479,0.18969857,0.00876262,2011111,2011288,240.14716310,29.55869413,237.66803145,34.01279077,121.30918488,274.49900616,10.39089946,9.16611240,61.81981227,62.75093832,51.43148998,62.83786832");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211937411M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN0211937411M,EN0211850425M,6.11511,2.91324098,3.20186609,0.16174609,0.14102046,1.05653549,1.67870138,8.38221975,132.16601868,10.07854993,2.80090722,3.02557475,0.49093306,0.40333054,0.65204332,0.88065846,0.67643678,0.75878229,0.34565267,0.30564807,2011112,2011111,239.45744655,31.09327322,240.48370504,30.81625183,111.17140727,117.45653760,1.08173319,10.22633869,60.18133863,61.93338905,59.12068975,51.70959321");
  compareCsvLine(line.getRow(2),"EN0211937411M,EW0227091446G,5.13292,2.84896522,2.28395679,0.15941462,0.12652563,2.45942496,1.12351098,75.16479380,250.89455002,12.29557194,2.80090722,2.89702322,0.49093306,0.40919079,0.65204332,0.85672502,0.67643678,0.64640276,0.64653832,0.02520226,2011112,2011287,239.45744655,31.09327322,240.56840385,34.18787049,111.17140727,273.41773462,1.08173319,9.26900090,60.18133863,61.81618413,59.12068975,61.82375105");
  compareCsvLine(line.getRow(3),"EN0211937411M,EW0227133784G,4.35965,2.81856364,1.54108837,0.15811653,0.19695314,2.46915113,0.11640173,76.79463937,268.13003515,11.57141549,2.80090722,2.83622006,0.49093306,0.36245308,0.65204332,0.85415281,0.67643678,0.63513479,1.0,0.03876427,2011112,2011288,239.45744655,31.09327322,237.66803145,34.01279077,111.17140727,274.49900616,1.08173319,9.16611240,60.18133863,62.75093832,59.12068975,62.83786832");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211937488M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN0211937488M,EN0211980980M,5.70052,2.79238659,2.90812844,0.10927996,0.12145600,1.02562955,1.34275050,170.78100922,199.77435941,6.01115107,2.79013316,2.79464001,0.41107532,0.48762166,0.71290684,0.69396177,0.68590471,0.63335786,0.77158790,0.81324616,2011112,2011112,238.58356334,33.67370765,238.84908551,33.56874368,105.18221135,102.55380329,3.51627363,2.75847090,61.77849353,60.24756670,58.26857601,62.99779185");
  compareCsvLine(line.getRow(2),"EN0211937488M,EN0211981012M,5.69464,2.78385138,2.91078554,0.10840106,0.11358389,1.05147872,1.95165838,170.18670945,191.20675828,6.27839361,2.79013316,2.77756960,0.41107532,0.47462970,0.71290684,0.69276657,0.68590471,0.63099985,0.23756384,0.26022329,2011112,2011112,238.58356334,33.67370765,238.99038636,34.52754626,105.18221135,100.03265745,3.51627363,2.71066287,61.77849353,60.50740585,58.26857601,63.21001274");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211980850M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN0211980850M,EW0227133784G,4.98726,2.80464434,2.18261987,0.18368022,0.12472243,2.42699615,2.82163895,83.41546724,236.70360169,13.05800585,2.77306862,2.83622006,0.31718607,0.36245308,0.77937542,0.85415281,0.69461080,0.63513479,0.27141281,0.01097468,2011112,2011288,233.58275757,31.29165399,237.66803145,34.01279077,113.10236561,274.49900616,6.17501684,9.16611240,63.65627845,62.75093832,57.48502790,62.83786832");
  compareCsvLine(line.getRow(2),"EN0211980850M,EW0227176126G,4.60429,2.77362424,1.83067055,0.18534848,0.06726772,2.42716581,1.88460117,84.46683415,237.16394944,13.03440574,2.77306862,2.77417986,0.31718607,0.31179369,0.77937542,0.85405346,0.69461080,0.62385731,0.91703014,0.03709076,2011112,2011288,233.58275757,31.29165399,234.73576933,33.97866162,113.10236561,274.51819495,6.17501684,9.16213875,63.65627845,63.76412601,57.48502790,63.85284140");
  compareCsvLine(line.getRow(3),"EN0211980850M,EW0227218472G,4.39397,2.74055526,1.65341925,0.18555976,0.11434706,2.41728967,1.07254416,85.31556030,226.24582935,13.58698671,2.77306862,2.70804191,0.31718607,0.25900289,0.77937542,0.85204463,0.69461080,0.61228630,1.0,0.04071345,2011112,2011289,233.58275757,31.29165399,231.83005815,34.13645061,113.10236561,273.40118013,6.17501684,9.08178520,63.65627845,64.81994211,57.48502790,64.89423285");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211980977M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN0211980977M,EN0211937486M,5.638,2.75291003,2.88508848,0.11020450,0.13361618,1.02747851,1.35342105,138.51835499,200.64421928,6.00598047,2.69477493,2.81104513,0.43267388,0.35392301,0.63370535,0.77567094,0.64866114,0.70112853,0.78090539,0.73453337,2011112,2011112,237.58479307,33.89954131,237.27112110,34.02234270,102.72590404,105.54865922,0.34821403,6.02683787,61.34652231,62.92153966,61.62049731,56.89843215");
  compareCsvLine(line.getRow(2),"EN0211980977M,EN0211937446M,5.42208,2.76013890,2.66194563,0.11129868,0.12978433,1.05835058,2.08338634,137.83750706,195.25870902,6.35427610,2.69477493,2.82550286,0.43267388,0.36330003,0.63370535,0.77728724,0.64866114,0.70393225,0.11615944,0.10433298,2011112,2011112,237.58479307,33.89954131,237.01706205,32.87939564,102.72590404,108.72002099,0.34821403,6.09148965,61.34652231,62.73399929,61.62049731,56.64609734");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211981012M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EN0211981012M,EN0211937488M,5.73996,2.78385138,2.95610443,0.10840106,0.11358389,1.05147872,1.95165838,170.18670945,191.20675828,6.27839361,2.77756960,2.79013316,0.47462970,0.41107532,0.69276657,0.71290684,0.63099985,0.68590471,0.26022329,0.23756384,2011112,2011112,238.99038636,34.52754626,238.58356334,33.67370765,100.03265745,105.18221135,2.71066287,3.51627363,60.50740585,61.77849353,63.21001274,58.26857601");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0211937827G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 6);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EW0211937827G,EW0227134017G,5.9907,2.68114177,3.30956128,0.19074518,0.03259805,1.29044551,0.87593032,43.99252746,316.56008867,9.89932003,2.57592477,2.78635877,0.33276355,0.33963336,0.62939845,0.89801483,0.62964266,0.55163825,0.56770771,0.94568547,2011112,2011288,242.63800548,44.13232562,242.30612536,43.16336292,276.22354284,214.05285230,0.17593827,10.92059326,63.34472885,63.20733264,63.33215970,70.35255711");
  compareCsvLine(line.getRow(2),"EW0211937827G,EW0227176360G,5.84179,2.65463626,3.18715233,0.19006757,0.09378727,1.29192550,1.94112758,45.13522427,294.86603605,10.61164479,2.57592477,2.73334775,0.33276355,0.29679248,0.62939845,0.89701885,0.62964266,0.54241301,0.38400418,0.64057150,2011112,2011288,242.63800548,44.13232562,239.40578374,43.22627070,276.22354284,213.80763955,0.17593827,10.88075400,63.34472885,64.06415035,63.33215970,71.18282877");
  compareCsvLine(line.getRow(3),"EW0211937827G,EW0227049218G,5.43816,2.72335822,2.71480536,0.15346100,0.16083951,1.13774227,3.80218676,5.82588549,541.22768565,5.82816439,2.57592477,2.87079167,0.33276355,0.39040627,0.62939845,0.84738775,0.62964266,0.64191060,0.13993638,0.19389334,2011112,2011287,242.63800548,44.13232562,244.28557385,39.06533759,276.22354284,242.78217291,0.17593827,8.89551024,63.34472885,62.19187456,63.33215970,62.22804581");
  compareCsvLine(line.getRow(4),"EW0211937827G,EW0227091561G,5.39443,2.69415841,2.70026726,0.15245749,0.16880805,1.13873824,4.85025546,6.72056211,791.84979948,3.99086800,2.57592477,2.81239205,0.33276355,0.34337706,0.62939845,0.84597701,0.62964266,0.63190668,0.13537987,0.18789134,2011112,2011287,242.63800548,44.13232562,241.35602061,39.08394157,276.22354284,242.56983077,0.17593827,8.83908048,63.34472885,63.13245863,63.33215970,63.12839824");
  compareCsvLine(line.getRow(5),"EW0211937827G,EW0227218705G,5.342,2.62690796,2.71509315,0.18918352,0.18228439,1.29667947,2.93505782,46.19021950,266.23006754,11.72292734,2.57592477,2.67789114,0.33276355,0.25199747,0.62939845,0.89574085,0.62964266,0.53286609,0.16744656,0.28068045,2011112,2011289,242.63800548,44.13232562,236.51781004,43.39051800,276.22354284,213.02376507,0.17593827,10.82963439,63.34472885,64.96005053,63.33215970,72.04205127");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0214460053G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EW0214460053G,EW0214460026G,6.46791,3.39127733,3.07663628,0.12765626,0.04160876,1.04413084,2.25424499,25.77231783,139.64756720,7.32937555,3.52348383,3.25907084,0.82967728,0.82666456,0.81618604,0.63558617,0.90057671,0.82056711,0.28804525,0.31300787,2011141,2011141,231.99393161,37.41256798,230.57712841,38.35436687,89.81021492,86.01433010,7.64744170,0.42344714,46.59354560,46.53329134,38.94809534,46.14895995");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0216448475G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EW0216448475G,EW0217048515G,4.91352,1.55198977,3.36153448,0.47496549,0.97554515,1.03267099,10.43268704,22.51434093,645.85137282,33.89280713,1.60145239,1.50252716,0.58652583,0.17108451,0.15020150,0.09691715,0.27501299,0.63051478,0.55681203,0.59213249,2011164,2011171,238.35767715,-6.71133408,212.89484149,4.13352441,2169.38215315,2100.74859611,48.99193964,51.12331372,58.26948332,33.42169024,95.24883035,63.25366930");
  compareCsvLine(line.getRow(2),"EW0216448475G,EW0217133530G,4.76686,1.50809563,3.25875997,0.58351076,1.03443210,1.03212696,14.52248727,26.58793922,542.64261504,38.64450385,1.60145239,1.41473888,0.58652583,0.07731474,0.15020150,0.06247440,0.27501299,0.67116971,0.50651404,0.55085569,2011164,2011172,238.35767715,-6.71133408,210.09143125,6.31175711,2169.38215315,2101.85590688,48.99193964,52.50102396,58.26948332,31.54629495,95.24883035,59.59472556");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0227091446G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EW0227091446G,EW0211937665G,6.02955,2.78561205,3.24393946,0.15989830,0.13612743,1.17031028,4.16441075,2.49070149,733.84691853,4.98399408,2.89702322,2.67420089,0.40919079,0.42059730,0.85672502,0.62973500,0.64640276,0.64886516,0.40520585,0.28296941,2011287,2011112,240.56840385,34.18787049,241.40882645,38.51924937,273.41773462,319.98358792,9.26900090,0.18940005,61.81618413,61.58805381,61.82375105,61.60213509");
  compareCsvLine(line.getRow(2),"EW0227091446G,EW0211981114G,5.97074,2.75880380,3.21193634,0.15986329,0.10714100,1.18050161,2.85710933,35.68205668,526.57261371,6.98917632,2.89702322,2.62058439,0.40919079,0.37737797,0.85672502,0.63091750,0.64640276,0.63786601,0.38701305,0.26690951,2011287,2011112,240.56840385,34.18787049,238.32114291,38.16027593,273.41773462,322.77007837,9.26900090,0.23670015,61.81618413,62.45244053,61.82375105,62.59205831");
  compareCsvLine(line.getRow(3),"EW0227091446G,EW0211894196G,5.84203,2.81895988,3.02306603,0.15985270,0.19028449,1.17999916,5.11611237,4.44943807,864.86647591,4.26685668,2.89702322,2.74089655,0.40919079,0.47564676,0.85672502,0.62981055,0.64640276,0.66098771,0.30853232,0.21283308,2011287,2011111,240.56840385,34.18787049,244.43138952,38.20574896,273.41773462,322.63269890,9.26900090,0.19242223,61.81618413,60.48706478,61.82375105,60.51110527");
  compareCsvLine(line.getRow(4),"EW0227091446G,EW0212024579G,5.51566,2.72636367,2.78929746,0.15876605,0.15599899,1.17461932,1.82992518,17.72870444,414.53698683,8.80791442,2.89702322,2.55570412,0.40919079,0.32284540,0.85672502,0.63167849,0.64640276,0.62642226,0.18411582,0.12795954,2011287,2011113,240.56840385,34.18787049,235.23328924,38.33176699,273.41773462,321.16175413,9.26900090,0.26713997,61.81618413,63.54309199,61.82375105,63.62199641");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0227176126G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EW0227176126G,EW0212024579G,5.90752,2.66494199,3.24258111,0.15678863,0.13441824,1.16991062,3.77407221,15.04749196,753.71892187,4.87100936,2.77417986,2.55570412,0.31179369,0.32284540,0.85405346,0.63167849,0.62385731,0.62642226,0.40665362,0.28411835,2011288,2011113,234.73576933,33.97866162,235.23328924,38.33176699,274.51819495,321.16175413,9.16213875,0.26713997,63.76412601,63.54309199,63.85284140,63.62199641");
  compareCsvLine(line.getRow(2),"EW0227176126G,EW0227260700G,5.81737,2.62272986,3.19464383,0.15684691,0.12850077,1.10073532,1.59582118,17.94161843,292.66062870,11.66757095,2.77417986,2.47127987,0.31179369,0.25042158,0.85405346,0.63169433,0.62385731,0.61044976,0.35843118,0.30189779,2011288,2011289,234.73576933,33.97866162,229.63489425,33.24135398,274.51819495,302.17187462,9.16213875,0.26777329,63.76412601,64.99156835,63.85284140,65.05952118");
  compareCsvLine(line.getRow(3),"EW0227176126G,EW0211981114G,5.7197,2.69738212,3.02232080,0.15783793,0.19958795,1.17576934,4.80125636,33.00084420,896.62718164,4.11798999,2.77417986,2.62058439,0.31179369,0.37737797,0.85405346,0.63091750,0.62385731,0.63786601,0.31023174,0.21508877,2011288,2011112,234.73576933,33.97866162,238.32114291,38.16027593,274.51819495,322.77007837,9.16213875,0.23670015,63.76412601,62.45244053,63.85284140,62.59205831");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0227422989G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 36);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB");
  compareCsvLine(line.getRow(1),"EW0227422989G,EW0211624221G,5.42942,1.85240579,3.57701041,0.32657102,0.31169637,1.15529813,7.94691037,16.74029474,648.66605005,34.52065409,2.01510698,1.68970461,0.81257877,0.35887899,0.16962876,0.16638229,0.39351005,0.58752686,0.53813165,0.41441780,2011291,2011108,237.11265368,-6.81678316,268.54563190,1.05205525,1930.93098213,2230.80097160,48.21484936,48.34470836,46.25157559,37.17757998,84.58409498,67.12258233");
  compareCsvLine(line.getRow(2),"EW0227422989G,EW0212189243G,4.42606,2.04196934,2.38408574,0.10438570,0.06191721,1.14883726,2.62769560,5.02073653,6037.37934392,4.20290415,2.01510698,2.06883170,0.81257877,0.86384653,0.16962876,0.19586365,0.39351005,0.42960598,1.0,0.77692584,2011291,2011115,237.11265368,-6.81678316,237.11302579,-4.25682422,1930.93098213,2218.32546666,48.21484936,47.16545379,46.25157559,47.27693068,84.58409498,81.33546115");
}


/**
   * IsisminerTestStereoPair2
   * 
   * Tests Stereopair perimeter behavior using centroid for gis center
   * instead of image center used in previous StereoPair test.
   * 
   * INPUT: 1) degaus_stereo_perimeter.csv, in isis/tests/data/isisminer/stereopair
   *        2) stereopair_test.conf
   * 
   * OUTPUT: EN0211850377M.csv; EN0211937411M.csv; EN0211937488M.csv
   *         EN0211980850M.csv; EN0211980977M.csv; EN0211981012M.csv
   *         EW0211937827G.csv; EW0214460053G.csv; EW0216448475G.csv
   *         EW0227091446G.csv; EW0227176126G.csv; EW0227422989G.csv
   * 
   * NOTE: Original test has 78 output files, I chose 12 to check,
   *        evenly distributed across the original 78.
   */
TEST(Isisminer, IsisminerTestStereoPair2) {
  QTemporaryDir tempDir;

  // create input isisminer config file stereopair_intersect_centroid.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "StereoIntersectTest"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir, outputdir)"));
  
    /////////////////////////////////////////////////////////////////
    // add CsvReader strategy
    PvlObject CsvReader("Strategy");
    CsvReader.addKeyword(PvlKeyword("Name", "ReadDegausPerimeter"));
    CsvReader.addKeyword(PvlKeyword("Type", "CsvReader"));
    CsvReader.addKeyword(PvlKeyword("Description", "Read Degaus data"));
    CsvReader.addKeyword(PvlKeyword("CsvFile", "\"%1/degaus_stereo_perimeter.csv\""));
    CsvReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
    CsvReader.addKeyword(PvlKeyword("HasHeader", "True"));
    CsvReader.addKeyword(PvlKeyword("Identity", "\"%1\""));
    CsvReader.addKeyword(PvlKeyword("IdentityArgs", "\"SourceProductId\""));
    CsvReader.addKeyword(PvlKeyword("GisGeometryRef", "GisFootprint"));
    CsvReader.addKeyword(PvlKeyword("GisType", "WKB"));
    CsvReader.addKeyword(PvlKeyword("RemoveGisKeywordAfterImport", "False"));
    isisminerObject.addObject(CsvReader);

    /////////////////////////////////////////////////////////////////
    // add StereoPair strategy
    PvlObject StereoPair("Strategy");
    StereoPair.addKeyword(PvlKeyword("Name", "DegausStereoPairs"));
    StereoPair.addKeyword(PvlKeyword("Type", "StereoPair"));
    StereoPair.addKeyword(PvlKeyword("Description", "Find all stereo pairs in the Degaus Crater region"));
    StereoPair.addKeyword(PvlKeyword("OverlapMerge", "\"Intersection\""));
    StereoPair.addKeyword(PvlKeyword("PixelPrecisionMatch", "0.2"));
    StereoPair.addKeyword(PvlKeyword("Asset", "StereoPairs"));
    StereoPair.addKeyword(PvlKeyword("Equation", "imagestrengthrank * 1.0 + stereostrengthrank * 1.0"));
    StereoPair.addKeyword(PvlKeyword("Result", "StereoPairRank"));

    /////////////////////////////////////////////////////////////////
    // add First level constraints/strengths group
    PvlGroup ImageStrength("ImageStrength");
    ImageStrength.addKeyword(PvlKeyword("Incidence", "(30.0,65.0,50.0)"));
    ImageStrength.addKeyword(PvlKeyword("Emission", "(0.0,55.0,15.0)"));
    ImageStrength.addKeyword(PvlKeyword("Phase", "(15.0,120.0,30.0)"));
    ImageStrength.addKeyword(PvlKeyword("Resolution", "(25.0,5000.0,200.0)"));
    StereoPair.addGroup(ImageStrength);

    /////////////////////////////////////////////////////////////////
    // add Second level constraints/strengths group
    PvlGroup StereoStrength("StereoStrength");
    StereoStrength.addKeyword(PvlKeyword("ResolutionRatio", "(1.0,2.5,1.0)"));
    StereoStrength.addKeyword(PvlKeyword("ParallaxHeightRatio", "(0.1,1.0,0.4)"));
    StereoStrength.addKeyword(PvlKeyword("ShadowTipDistance", "(0.0,2.58,0.0)"));
    StereoStrength.addKeyword(PvlKeyword("OverlapRatio", "(0.1,1.0,0.5)"));
    StereoPair.addGroup(StereoStrength);

      /////////////////////////////////////////////////////////////////
      // add KeywordMap group
      PvlGroup KeywordMap("KeywordMap");
      KeywordMap.addKeyword(PvlKeyword("Resolution", "PixelResolution"));
      KeywordMap.addKeyword(PvlKeyword("Emission", "EmissionAngle"));
      KeywordMap.addKeyword(PvlKeyword("Incidence", "IncidenceAngle"));
      KeywordMap.addKeyword(PvlKeyword("Phase", "PhaseAngle"));
      KeywordMap.addKeyword(PvlKeyword("CenterLongitude", "GisOverlapCentroidX"));
      KeywordMap.addKeyword(PvlKeyword("CenterLatitude", "GisOverlapCentroidY"));
      StereoPair.addGroup(KeywordMap);

      // add IsisMiner sub object
      PvlObject isisminerSubObject1("IsisMiner");
  
      /////////////////////////////////////////////////////////////////
      // add ResourceManager strategy to isisminerSubObject
      PvlObject ResourceManager("Strategy");
      ResourceManager.addKeyword(PvlKeyword("Name", "PairCandidates"));
      ResourceManager.addKeyword(PvlKeyword("Type", "ResourceManager"));
      ResourceManager.addKeyword(PvlKeyword("Description", "Choose all resources to determine stereo"));
      ResourceManager.addKeyword(PvlKeyword("Operations", "ResetDiscard"));
      isisminerSubObject1.addObject(ResourceManager);
      StereoPair.addObject(isisminerSubObject1);
    isisminerObject.addObject(StereoPair);

      /////////////////////////////////////////////////////////////////
      // add AssetSidebar strategy
      PvlObject AssetSidebar("Strategy");
      AssetSidebar.addKeyword(PvlKeyword("Name", "WriteStereoFiles"));
      AssetSidebar.addKeyword(PvlKeyword("Type", "AssetSidebar"));
      AssetSidebar.addKeyword(PvlKeyword("Description", "Process all stereo pair assets"));
      AssetSidebar.addKeyword(PvlKeyword("Asset", "StereoPairs"));

      // add IsisMiner sub object
      PvlObject isisminerSubObject2("IsisMiner");

      /////////////////////////////////////////////////////////////////
      // add NumericalSort strategy
      PvlObject NumericalSort("Strategy");
      NumericalSort.addKeyword(PvlKeyword("Name", "SortStereoPairs"));
      NumericalSort.addKeyword(PvlKeyword("Type", "NumericalSort"));
      NumericalSort.addKeyword(PvlKeyword("Description", "Sort stereo pairs by StereoPairRank"));
      NumericalSort.addKeyword(PvlKeyword("SortKey", "StereoPairRank"));
      NumericalSort.addKeyword(PvlKeyword("Order", "Descending"));
      isisminerSubObject2.addObject(NumericalSort);

      // add CsvWriter strategy
      PvlObject CsvWriter("Strategy");
      CsvWriter.addKeyword(PvlKeyword("Type", "CsvWriter"));
      CsvWriter.addKeyword(PvlKeyword("Name", "WritePairs"));
      CsvWriter.addKeyword(PvlKeyword("Description", "Write individual files with stereo pair data"));
      CsvWriter.addKeyword(PvlKeyword("SkipEmptyLists", "True"));
      CsvWriter.addKeyword(PvlKeyword("CsvFile", "\"%1/%2.csv\""));
      CsvWriter.addKeyword(PvlKeyword("CsvFileArgs", "(\"outputdir\", \"Identity\")"));
      CsvWriter.addKeyword(PvlKeyword("GisGeometryKey", "GisIntersectionFootprint"));
      CsvWriter.addKeyword(PvlKeyword("GisType", "WKB"));

      QStringList keywordList = {"\"SourceProductIdA\"", "\"SourceProductIdB\"", "\"StereoPairRank\"",
                        "\"ImageStrengthRank\"", "\"StereoStrengthRank\"",
                        "\"ParallaxHeightRatio\"", "\"ShadowTipDistance\"", "\"ResolutionRatio\"", 
                        "\"DeltaSolarAzimuth\"", "\"DeltaSpacecraftAzimuth\"", 
                        "\"VerticalPrecision\"", "\"StereoAngle\"",
                        "\"ImageStrengthRankA\"", "\"ImageStrengthRankB\"",
                        "\"IncidenceRankA\"", "\"IncidenceRankB\"",
                        "\"EmissionRankA\"", "\"EmissionRankB\"",
                        "\"PhaseRankA\"", "\"PhaseRankB\"",
                        "\"OverlapRatioA\"", "\"OverlapRatioB\"",
                        "\"YearDoyA\"", "\"YearDoyB\"", 
                        "\"CenterLongitudeA\"", "\"CenterLatitudeA\"",
                        "\"CenterLongitudeB\"", "\"CenterLatitudeB\"",
                        "\"CenterRadiusA\"", "\"CenterRadiusB\"",
                        "\"SubSpacecraftLongitudeA\"", "\"SubSpacecraftLatitudeA\"", "\"TargetCenterDistanceA\"",
                        "\"SubSpacecraftLongitudeB\"", "\"SubSpacecraftLatitudeB\"", "\"TargetCenterDistanceB\"",
                        "\"GisOverlapCentroidX\"", "\"GisOverlapCentroidY\"",
                        "\"PixelResolutionA\"", "\"PixelResolutionB\"", 
                        "\"EmissionAngleA\"", "\"EmissionAngleB\"",
                        "\"IncidenceAngleA\"", "\"IncidenceAngleB\"",
                        "\"PhaseAngleA\"", "\"PhaseAngleB\"", 
                        "\"GisIntersectionFootprint\""};
      PvlKeyword kw("Keywords");
      for (auto const &v : keywordList) {
        kw.addValue(v);
      }

      CsvWriter.addKeyword(kw);
      isisminerSubObject2.addObject(CsvWriter);
      AssetSidebar.addObject(isisminerSubObject2);
    isisminerObject.addObject(AssetSidebar);
  
  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/stereopair_intersect_centroid.conf");

  // test open and output of conf file contents
  // ifstream f2(tempDir.path().toStdString() + "/stereopair_intersect_centroid.conf");
  // if ( f2.good() ) {
  //   cout << f2.rdbuf() << endl;
  //   f2.close();
  // }
  // else {
  //   cout << "uh oh" << endl;
  // }

  QVector<QString> args = {"config=" + tempDir.path() + "/stereopair_intersect_centroid.conf",
                           "parameters=inputdir:data/isisminer/stereopair/@outputdir:"
                                     + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
  }
  catch(IException &e) {
    FAIL() << e.toString().toStdString().c_str() << endl;
  }

  // Validate output csv files
  // NOTE: Header is the same for all output csv's so 
  //       we're only checking it in the first file

  CSVReader::CSVAxis csvLine;
  
  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  CSVReader line = CSVReader(tempDir.path() + "/EN0211850377M.csv",
                             false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EN0211850377M,EN0211980858M,6.04511,2.97192696,3.07318550,0.25542367,0.23099652,1.08041201,2.70548073,172.96275314,81.02042525,16.67050478,3.03926028,2.90459364,0.40900938,0.56020733,0.88477248,0.72858886,0.76187233,0.63407234,0.22864379,0.26972296,2011111,2011112,240.14716310,29.55869413,239.00116236,29.90665361,2439400,2439400,245.71940302,27.98477964,4799.32881366,236.90549531,30.67537555,4638.91917687,239.54087730,29.75254655,121.30918488,112.28048500,10.39089946,4.14355464,61.81981227,58.79585331,51.43148998,62.93348883,010300000001000000830000007A62DEE668E36D4014C0CA71FA2B3D40AC93489FBAE36D402489927AF7323D40C3F2D45B0CE46D4060B4FE45F4393D4021A1731C5EE46D4016CA59D2F0403D4095BE14E1AFE46D40ABE1EE1DED473D40A169A8A901E56D40FAA60927E94E3D40A9BF1E7653E56D40D45FF6EBE4553D402DDD6746A5E56D4052F1016BE05C3D40F3DD731AF7E56D403FE579A2DB633D4057DD32F248E66D40606FAC90D66A3D4068F694CD9AE66D40EF72E833D1713D401F448AACECE66D40B8877D8ACB783D40AFE1028F3EE76D4095FFBB92C57F3D409CEAEE7490E76D4080EBF44ABF863D40FE7A3E5EE2E76D40FB207AB1B88D3D40BCAFE14A34E86D40233F9EC4B1943D40B8A6C83A86E86D4002B4B482AA9B3D400A7FE32DD8E86D409CC111EAA2A23D40305922242AE96D401A830AF99AA93D404A57751D7CE96D40F5F1F4AD92B03D404D9DCC19CEE96D40EFEA27078AB73D403451181920EA6D403933FB0281BE3D403F9B481B72EA6D40657DC79F77C53D4012A64D20C4EA6D40776EE6DB6DCC3D400B9F172816EB6D40D1A2B2B563D33D4052B6963268EB6D4015B3872B59DA3D402D1FBB3FBAEB6D402139C23B4EE13D401F10754F0CEC6D40EAD4BFE442E83D4023C3B4615EEC6D404131DF2437EF3D40E1756A76B0EC6D40AA0880FA2AF63D40E469868D02ED6D40262A03641EFD3D40BFE4F8A654ED6D40E47DCA5F11043E405930B2C2A6ED6D40060A39EC030B3E40089BA2E0F8ED6D402DF7B207F6113E40CF77BA004BEE6D402F959DB0E7183E40921EEA229DEE6D40B05F5FE5D81F3E4041EC2147EFEE6D40A30260A4C9263E401443526D41EF6D40D45E08ECB92D3E40B38A6B9593EF6D40618EC2BAA9343E406D305EBFE5EF6D4036E9F90E993B3E4069A71AEB37F06D4059091BE787423E40D66891188AF06D4068CF934176493E401CF4B247DCF06D40CC66D31C64503E400ACF6F782EF16D401C4A4A7751573E401186B8AA80F16D4043476A4F3E5E3E4061AC7DDED2F16D40C883A6A32A653E402FDCAF1325F26D40EC807372166C3E4001C6E84755F26D4060F2550025703E40365434C28CF26D40713780038E6F3E409D38B7E376F36D401157A2460F6D3E40609E74F060F46D40DEC5E33A8F6A3E405C3A2AE84AF56D40A45510E10D683E40A4EC95CA34F66D4028C4F5398B653E405BC175971EF76D403BB8634607633E4088F1874E08F86D40B5BE2B0782603E40E3E38AEFF1F86D406047217DFB5D3E40B02D3D7ADBF96D400EA219A9735B3E4084935DEEC4FA6D405AFBEB8BEA583E40160AAB4BAEFB6D408159712660563E4017B7E49197FC6D4047998479D4533E40F3F1C9C080FD6D40A86A028647513E409D441AD869FE6D40994DC94CB94E3E40626C95D752FF6D40AB8EB9CE294C3E40A05AFBBE3B006E40C743B50C99493E409953E8885A006E4060793397EE483E40E7DC951DFEFF6D402C6409F6A9423E4061F7102DA2FF6D403986C0C76C3C3E409BA4B44146FF6D40E18A66122F363E40B66B945BEAFE6D40DE8267D7F02F3E40A4DCC37A8EFE6D404CC43018B2293E40F28F569F32FE6D40C9E630D672233E408E2660C9D6FD6D40D9BFD712331D3E407E49F4F87AFD6D40FC5E96CFF2163E40B1A9262E1FFD6D40E309DF0DB2103E40BCFF0A69C3FC6D407B3825CF700A3E40980BB5A967FC6D401091DD142F043E406C9438F00BFC6D405BE47DE0ECFD3D404B68A93CB0FB6D406E297D33AAF73D40F15B1B8F54FB6D40C379530F67F13D40884AA2E7F8FA6D40200D7A7523EB3D40711552469DFA6D4088356B67DFE43D40EEA33EAB41FA6D40F95AA2E69ADE3D40F9E27B16E6F96D4067F79BF455D83D40F6C41D888AF96D408192D59210D23D40794138002FF96D4058BDCDC2CACB3D400155DF7ED3F86D40410E048684C53D40BC00270478F86D406E1CF9DD3DBF3D403C4A23901CF86D40A47B2ECCF6B83D40413BE822C1F76D40E1B72652AFB23D406FE189BC65F76D40E750657167AC3D400F4E1C5D0AF76D40E7B56E2B1FA63D40CA95B304AFF66D400141C881D69F3D4070D063B353F66D40CA32F8758D993D40A5184169F8F56D40C9AD850944933D40AA8B5F269DF56D40EBB1F83DFA8C3D401349D3EA41F56D40F817DA14B0863D409272B0B6E6F46D40088DB38F65803D409B2B0B8A8BF46D40B98D0FB01A7A3D403599F76430F46D40C9617977CF733D40B0E18947D5F36D402A177DE7836D3D40552CD6317AF36D40867DA70138673D403EA1F0231FF36D406E2186C7EB603D40F368ED1DC4F26D40A047A73A9F5A3D4033ACE01F69F26D4035E8995C52543D40B693DE290EF26D4001AAED2E054E3D40DB47FB3BB3F16D408FDD32B3B7473D4071F04A5658F16D407D78FAEA69413D4060B4E178FDF06D408110D6D71B3B3D4079B9D3A3A2F06D4096D6577BCD343D40222435D747F06D40209212D77E2E3D4017171A13EDEF6D40029C99EC2F283D4020B3965792EF6D409DD980BDE0213D40D216BFA437EF6D40F9B75C4B911B3D40485EA7FADCEE6D40C226C29741153D40DCA2635982EE6D405C9346A4F10E3D40E1FA07C127EE6D40CAE37F72A1083D407134297210EE6D40776AC56BCA073D40BFCBCDEF29ED6D408273ADE9530A3D404DBF400F45EC6D402CB2CC6AD70C3D403C468C1960EB6D4057E123C5590F3D40C08AEE0E7BEA6D408FEADDF7DA113D401EE5A5EF95E96D40CE4F27025B143D40EADAF0BBB0E86D40A52E2EE3D9163D40421E0E74CBE76D405643229A57193D40FE8C3C18E6E66D40E0EB3426D41B3D40ED2FBBA800E66D40172B99864F1E3D400E3AC9251BE56D409EAB83BAC9203D40BC07A68F35E46D40DCC22AC142233D40EE1D91E64FE36D40FE73C699BA253D409994443325E36D40FE3152D22F263D407A62DEE668E36D4014C0CA71FA2B3D40");
  compareCsvLine(line.getRow(2),"EN0211850377M,EW0227133784G,5.15031,2.93774017,2.21257008,0.23196233,0.11582761,2.26280480,2.65273296,84.32235086,117.23508139,25.09313670,3.03926028,2.83622006,0.40900938,0.36245308,0.88477248,0.85415281,0.76187233,0.63513479,0.18969857,0.00876262,2011111,2011288,240.14716310,29.55869413,237.66803145,34.01279077,2439400,2439400,245.71940302,27.98477964,4799.32881366,239.34576275,37.27724952,3962.00410469,239.73042389,30.02652887,121.30918488,274.49900616,10.39089946,9.16611240,61.81981227,62.75093832,51.43148998,62.83786832,010300000001000000460000003451181920EA6D403933FB0281BE3D403F9B481B72EA6D40657DC79F77C53D4012A64D20C4EA6D40776EE6DB6DCC3D400B9F172816EB6D40D1A2B2B563D33D4052B6963268EB6D4015B3872B59DA3D402D1FBB3FBAEB6D402139C23B4EE13D401F10754F0CEC6D40EAD4BFE442E83D4023C3B4615EEC6D404131DF2437EF3D40E1756A76B0EC6D40AA0880FA2AF63D40E469868D02ED6D40262A03641EFD3D40BFE4F8A654ED6D40E47DCA5F11043E405930B2C2A6ED6D40060A39EC030B3E40089BA2E0F8ED6D402DF7B207F6113E40CF77BA004BEE6D402F959DB0E7183E40921EEA229DEE6D40B05F5FE5D81F3E4041EC2147EFEE6D40A30260A4C9263E401443526D41EF6D40D45E08ECB92D3E40B38A6B9593EF6D40618EC2BAA9343E406D305EBFE5EF6D4036E9F90E993B3E4069A71AEB37F06D4059091BE787423E40D66891188AF06D4068CF934176493E401CF4B247DCF06D40CC66D31C64503E400ACF6F782EF16D401C4A4A7751573E401186B8AA80F16D4043476A4F3E5E3E4061AC7DDED2F16D40C883A6A32A653E402FDCAF1325F26D40EC807372166C3E40D0B63F4A77F26D40C61F47BA01733E40F7E41D82C9F26D405AA59879EC793E408A7418C1E1F26D407416B35EDC7A3E4093E8DBBBEBF36D40088541569E783E4017A29F3FF4F46D40EBF8FBCB61763E40BE5BF59EFCF56D409E1D57E623743E40B35B95D904F76D40641706A6E4713E400F0938EF0CF86D40913FBE0BA46F3E40CCEC95DF14F96D40C8223718626D3E40BCB267AA1CFA6D40357F2ACC1E6B3E40932A664F24FB6D40A1425428DA683E40D1484ACE2BFC6D409F88722D94663E40C227CD2633FD6D407E9845DC4C643E407908A8583AFE6D4055E38F3504623E40C153946341FF6D40E201163ABA5F3E401C9B4B4748006E4074B29EEA6E5D3E40AE9987034F016E40C2D6F247225B3E404235029855026E40A271DD52D4583E402E7F75045C036E40CFA42B0C85563E4055B59B4862046E4085AEAC7434543E4009432F6468056E403DE7318DE2513E40BF589CA999056E403F8E419872513E4044E602DAD9046E40E297984071443E40C2C620BEEB036E400334159444343E40D4573EC7FD026E409443BDB112243E407EBFC7F40F026E40651B428FDB133E40C405294622016E405E0742229F033E403712CEBA34006E40960D48605DF33D4081A9225247FF6D4076B0CB3E16E33D40E46A920B5AFE6D40FAAF30B3C9D23D40C7CD88E66CFD6D401ECAC6B277C23D402C1F71E27FFC6D40547AC93220B23D40207FB6FE92FB6D4001B85F28C3A13D4042DEC33AA6FA6D4026B49B8860913D40B71C015F3FFA6D405CD9B3EDDF8B3D40DDE46EB7FAF76D40E4B318E7C1923D4075A42F78BEF56D4075A9E35085993D406C74E60B82F36D40B958E24044A03D407937A67145F16D40C04739BAFEA63D4048FB81A808EF6D40BFD303C0B4AD3D4045F58CAFCBEC6D404E40545566B43D40817FDA858EEA6D404AC6337D13BB3D40C5BB0CE109EA6D402570A9DF9DBC3D403451181920EA6D403933FB0281BE3D40");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211937411M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EN0211937411M,EN0211850425M,6.11511,2.91324098,3.20186609,0.16174609,0.14102046,1.05653549,1.67870138,8.38221975,120.04858736,11.07183001,2.80090722,3.02557475,0.49093306,0.40333054,0.65204332,0.88065846,0.67643678,0.75878229,0.34565267,0.30564807,2011112,2011111,239.45744655,31.09327322,240.48370504,30.81625183,2439400,2439400,240.04590622,31.01225938,4620.02587353,245.91703548,29.22080180,4724.68617522,239.93633873,30.95214451,111.17140727,117.45653760,1.08173319,10.22633869,60.18133863,61.93338905,59.12068975,51.70959321,0103000000010000008B000000190AB547A4F46D40CC6E069984503E4014127EFBBEF36D404D2EE5E9EE523E40260AFE93D9F26D4047AC6C2258553E40596B7311F4F16D4004C4C641C0573E4034DE1C740EF16D4062EA1E47275A3E40EE3939BC28F06D40BB30A2318D5C3E40AB8307EA42EF6D40CE477F00F25E3E40ADEDC6FD5CEE6D40CE82E6B255613E40B4557343C9ED6D40F82522D5DD623E40176D0361FBED6D403CE03FF3E5663E403B38D8224FEE6D40F0C03547A26D3E404E38C5E9A2EE6D40A7302B625E743E40CBD1B9B5F6EE6D40599B74421A7B3E409266A5864AEF6D40A2EE66E6D5813E402556775C9EEF6D40069F574C91883E40D7FD1E37F2EF6D4028AD9C724C8F3E4006B98B1646F06D403DAB8C5707963E405EE1ACFA99F06D4028C27EF9C19C3E4004CF71E3EDF06D40BFB6CA567CA33E40D1D8C9D041F16D4007EFC86D36AA3E409554A4C295F16D406F77D23CF0B03E404697F0B8E9F16D40EA0741C2A9B73E4036F59DB33DF26D4026096FFC62BE3E4050C29BB291F26D40A999B7E91BC53E405452D9B5E5F26D40E1927688D4CB3E400BF945BD39F36D40588E08D78CD23E40780AD1C88DF36D40A8EACAD344D93E401DDB69D8E1F36D408ED01B7DFCDF3E402BC0FFEB35F46D40EE375AD1B3E63E40B80F82038AF46D40D6ECE5CE6AED3E40FF20E01EDEF46D4061941F7421F43E40934C093E32F56D40B1B168BFD7FA3E4090ECEC6086F56D40E3AA23AF8D013F40DA5C7A87DAF56D40D2CDB34143083F405BFBA0B12EF66D400D557D75F80E3F40232850DF82F66D40986CE548AD153F40B7457710D7F66D40B83652BA611C3F403DB905452BF76D40AED02AC815233F40B3EAEA7C7FF76D407D57D770C9293F40224516B8D3F76D409DECC0B27C303F40DE3677F627F86D4094BA518C2F373F40B031FD377CF86D40AAF9F4FBE13D3F4015AB977CD0F86D405EF4160094443F40731C36C424F96D40320C2597454B3F404603C80E79F96D40EEBD8DBFF6513F405EE13C5CCDF96D4055A6C077A7583F40153D84AC21FA6D4071862EBE575F3F4074A18DFF75FA6D401E48499107663F40819E4855CAFA6D404C0284EFB66C3F405DC9A4AD1EFB6D407EFD52D765733F4084BC910873FB6D40F0B72B47147A3F40FC17FF65C7FB6D40FFE9843DC2803F408981DCC51BFC6D405A8AD6B86F873F40E3A4192870FC6D4011D299B71C8E3F40E733A68CC4FC6D4009414938C9943F40CAE671F318FD6D40CCA16039759B3F40477C6C5C6DFD6D40C50D5DB920A23F400AFC47EFB0FD6D40AB9EA21A77A73F40990BB5A359FE6D405F862955B7A53F4071E68EF043FF6D40C52C50EE47A33F4023C811212E006E404C6E6C4BD7A03F40E9EAFB3418016E40A0FD546D659E3F40D9B90B2C02026E40296AE254F29B3F40A7D1FF05EC026E40071DEF027E993F40870197C2D5036E401856577808973F40FE4B9061BF046E40D428F9B591943F40A3E7AAE2A8056E404279B4BC19923F40FB3FA64592066E40C6F86A8DA08F3F4047F6418A7B076E40FC220029268D3F4046E23DB064086E40703A5990AA8A3F4004135AB74D096E4063455DC42D883F40A4CF569F360A6E40700AF5C5AF853F402698F4671F0B6E404C0D0B9630833F403226F410080C6E40468B8B35B0803F40CC6D169AF00C6E40E77764A52E7E3F402D9E1C03D90D6E408E7985E6AB7B3F407322C84BC10E6E40D1E5DFF927793F408629A505E00E6E40C239892480783F4008712756840E6E4057C60D024B723F4006B76C1E290E6E40ADD5C4451D6C3F409F4FC0E9CD0D6E4049387109EF653F40C78335B8720D6E402F867C4EC05F3F4017A7DF89170D6E402495511691593F408B17D25EBC0C6E40E7745C6261533F404B3D2037610C6E406B6B0A34314D3F40778ADD12060C6E4017F1C98C00473F40D87A1DF2AA0B6E40DCAC0A6ECF403F40BC93F3D44F0B6E4074703DD99D3A3F40B06373BBF40A6E406A34D4CF6B343F403D82B0A5990A6E4026144253392E3F40B48FBE933E0A6E40FB49FB6406283F40F034B185E3096E40332B7506D3213F4019239C7B88096E40DD2326399F1B3F40601393752D096E40E7B285FE6A153F40C7C6A973D2086E40E5650C58360F3F40E405F47577086E40F0D4334701093F4097A0857C1C086E408A9E76CDCB023F40DC6D7287C1076E404E6350EC95FC3E407D4BCE9666076E40C9C13DA55FF63E40DD1DADAA0B076E402452BCF928F03E40ACCF22C3B0066E40E7A14AEBF1E93E40B45143E055066E409B2F687BBAE33E408C9A2202FB056E406E6695AB82DD3E4064A6D428A0056E40C699537D4AD73E40B6766D5445056E40EA0025F211D13E400F120185EA046E4083B28C0BD9CA3E40CB83A3BA8F046E4016A00ECB9FC43E40D0DB68F534046E4099912F3266BE3E404F2E6535DA036E40E82075422CB83E408193AC7A7F036E4021B565FDF1B13E40612753C524036E402A7E8864B7AB3E406E096D15CA026E40087065797CA53E40695C0E6B6F026E40343E853D419F3E400C464BC614026E40135771B205993E40C8EE3727BA016E401FDFB3D9C9923E408A81E88D5F016E4052ACD7B48D8C3E40642B71FA04016E405F41684551863E40661BE66CAA006E40FAC8F18C14803E403B825BE54F006E400411018DD7793E40FA91E563F5FF6D40DC8523479A733E40DD7D98E89AFF6D40762DE7BC5C6D3E40F479887340FF6D409EA2DAEF1E673E40F2BAC904E6FE6D4008108DE1E0603E40CD75709C8BFE6D40812B8E93A25A3E409DDF903A31FE6D401C316E0764543E40332D3FDFD6FD6D4026DEBD3E254E3E40F2928F8A7CFD6D405E6C0E3BE6473E407644963C22FD6D40FD8CF1FDA6413E40A42635A8E4FC6D407D4F56FC633D3E4011E82F4994FC6D4039A26C261D3E3E409B2E0BCD98FB6D40ADD3BD0D5F403E40C076532C9DFA6D40FCE03FCD9F423E4005474B67A1F96D40DF2B2F64DF443E40314C357EA5F86D401FEEC9D11D473E4059585471A9F76D40BE3B50155B493E400062EB40ADF66D401A05042E974B3E4026833DEDB0F56D400819291BD24D3E400EAB262F61F56D4001272611864E3E40190AB547A4F46D40CC6E069984503E40");
  compareCsvLine(line.getRow(2),"EN0211937411M,EW0227091446G,5.13292,2.84896522,2.28395679,0.15941462,0.12652563,2.45942496,1.12351098,75.16479380,176.90578750,17.17696086,2.80090722,2.89702322,0.49093306,0.40919079,0.65204332,0.85672502,0.67643678,0.64640276,0.64653832,0.02520226,2011112,2011287,239.45744655,31.09327322,240.56840385,34.18787049,2439400,2439400,240.04590622,31.01225938,4620.02587353,242.38214360,37.43944410,3955.72810017,239.55727323,31.31318889,111.17140727,273.41773462,1.08173319,9.26900090,60.18133863,61.81618413,59.12068975,61.82375105,0103000000010000008E000000CF2CD56EF9D46D40077AD3E7572A3F403096B9D44FD56D40468295819F303F40B6882A43A6D56D407DCB4CBEE6363F40662716BAFCD56D4094676C9C2D3D3F408D926A3953D66D403346681A74433F40F6E715C1A9D66D407239B536BA493F403343065100D76D405FFAC8EFFF4F3F40C8BD29E956D76D40AA2D1A4445563F40776F6E89ADD76D400E6820328A5C3F40706EC23104D86D40033354B8CE623F4097CF13E25AD86D4013112FD512693F40B7A6509AB1D86D405D822B87566F3F40C506675A08D96D400C09C5CC99753F400F0245225FD96D40B12D78A4DC7B3F408BAAD8F1B5D96D40A383C20C1F823F40031210C90CDA6D4052AD220461883F40574AD9A763DA6D4093601889A28E3F40B065228EBADA6D40E16A249AE3943F40C576D97B11DB6D40A1B5C835249B3F401891EC7068DB6D40504A885A64A13F401CC9496DBFDB6D409F56E706A4A73F408B34DF7016DC6D40AD306B39E3AD3F4087EA9A7B6DDC6D40005B9AF021B43F40EB036B8DC4DC6D40AF88FC2A60BA3F40749B3DA61BDD6D4054A11AE79DC03F40FFCD00C672DD6D4003C57E23DBC63F40C7BAA2ECC9DD6D404550B4DE17CD3F409F83111A21DE6D4014E0471754D33F401C4D3B4E78DE6D408955C7CB8FD93F40E33E0E89CFDE6D40D8D9C1FACADF3F40D98378CA26DF6D4000E2C7A205E63F404E4A68127EDF6D40A0326BC23FEC3F404FC4CB60D5DF6D40A4E33E5879F23F4029E7EA28EDDF6D409E50AA384EF33F4074D1A0B8DBE06D40CACBCC4C06F13F4026EAEB04C9E16D40947468F4BFEE3F40794B693DB6E26D409832374978EC3F406DE1D261A3E36D406E09CF4B2FEA3F4070ACE27190E46D408320C8FCE4E73F404AC2526D7DE56D409EC1BC5C99E53F40114FDD536AE66D406E57496C4CE33F400F963C2557E76D40D46B0C2CFEE03F40AAF22AE143E86D406BA6A69CAEDE3F4057D9628730E96D40D7CABABE5DDC3F4083D89E171DEA6D4007B7ED920BDA3F407699999109EB6D406A61E619B8D73F4046E10DF5F5EB6D4028D74D5463D53F40BD91B641E2EC6D405A3ACF420DD33F4041AA4E77CEED6D40EABF17E6B5D03F40BB489195BAEE6D40D5ADD63E5DCE3F4085AA399CA6EF6D400059BD4D03CC3F404B2D038B92F06D4045237F13A8C93F40EE4FA9617EF16D403779D1904BC73F4074B3E71F6AF26D4019D06BC6EDC43F40E51B7AC555F36D408AA307B58EC23F4031711C5241F46D405D73605D2EC03F4018C08AC52CF56D4041C133C0CCBD3F40FD3A811F18F66D40500E41DE69BB3F40E13ABC5F03F76D40E1D849B805B93F402B40F885EEF76D40CE99114FA0B63F4094F3F191D9F86D402CC25DA339B43F4009276683C4F96D40ADB8F5B5D1B13F407FD6115AAFFA6D4005D7A28768AF3F40D928B2159AFB6D4053673019FEAC3F40B77004B684FC6D4067A16B6B92AA3F40622DC63A6FFD6D4011A8237F25A83F40990BB5A359FE6D405F862955B7A53F4071E68EF043FF6D40C52C50EE47A33F4023C811212E006E404C6E6C4BD7A03F40E9EAFB3418016E40A0FD546D659E3F40D9B90B2C02026E40296AE254F29B3F40A7D1FF05EC026E40071DEF027E993F40870197C2D5036E401856577808973F40FE4B9061BF046E40D428F9B591943F40A3E7AAE2A8056E404279B4BC19923F40FB3FA64592066E40C6F86A8DA08F3F4047F6418A7B076E40FC220029268D3F4046E23DB064086E40703A5990AA8A3F4004135AB74D096E4063455DC42D883F40A4CF569F360A6E40700AF5C5AF853F402698F4671F0B6E404C0D0B9630833F403226F410080C6E40468B8B35B0803F40CC6D169AF00C6E40E77764A52E7E3F402D9E1C03D90D6E408E7985E6AB7B3F407322C84BC10E6E40D1E5DFF927793F408629A505E00E6E40C239892480783F4008712756840E6E4057C60D024B723F4006B76C1E290E6E40ADD5C4451D6C3F409F4FC0E9CD0D6E4049387109EF653F40C78335B8720D6E402F867C4EC05F3F4017A7DF89170D6E402495511691593F408B17D25EBC0C6E40E7745C6261533F404B3D2037610C6E406B6B0A34314D3F40778ADD12060C6E4017F1C98C00473F40D87A1DF2AA0B6E40DCAC0A6ECF403F40BC93F3D44F0B6E4074703DD99D3A3F40B06373BBF40A6E406A34D4CF6B343F403D82B0A5990A6E4026144253392E3F40B48FBE933E0A6E40FB49FB6406283F40F034B185E3096E40332B7506D3213F4019239C7B88096E40DD2326399F1B3F40601393752D096E40E7B285FE6A153F40C7C6A973D2086E40E5650C58360F3F40E405F47577086E40F0D4334701093F4097A0857C1C086E408A9E76CDCB023F40DC6D7287C1076E404E6350EC95FC3E407D4BCE9666076E40C9C13DA55FF63E40DD1DADAA0B076E402452BCF928F03E40ACCF22C3B0066E40E7A14AEBF1E93E40B45143E055066E409B2F687BBAE33E408C9A2202FB056E406E6695AB82DD3E4064A6D428A0056E40C699537D4AD73E40B6766D5445056E40EA0025F211D13E400F120185EA046E4083B28C0BD9CA3E40CB83A3BA8F046E4016A00ECB9FC43E40D0DB68F534046E4099912F3266BE3E404F2E6535DA036E40E82075422CB83E408193AC7A7F036E4021B565FDF1B13E40612753C524036E402A7E8864B7AB3E4035D8EDD7F9026E4042AA7F6DC4A83E408CB2496149016E40B9C3E1749AAD3E4021264ED309FF6D400B732E2005B43E4075E8C4F1C9FC6D40EC0518BC6BBA3E404066BFBB89FA6D40E047A24ACEC03E40D7BF4E3049F86D404C82C9CD2CC73E4035C6834E08F66D40BF83824787CD3E40F9F76E15C7F36D4018A7BAB9DDD33E40647E208485F16D4060DA572630DA3E405C2AA89943EF6D4085A5388F7EE03E406371155501ED6D40E63034F6C8E63E409A6A77B5BEEA6D40BC4B1A5D0FED3E40AFCBDCB97BE86D404272B3C551F33E40E3E5536138E66D40AFD3C03190F93E40F4A2EAAAF4E36D401458FCA2CAFF3E401582AE95B0E16D4001A6181B01063F40E894AC206CDF6D400528C19B330C3F405C7CF14A27DD6D40E4119A2662123F40AB658913E2DA6D40D16540BD8C183F403F0780799CD86D4052F94961B31E3F40959DE07B56D66D40167A4514D6243F40D7E2CE56E3D46D40742A398EBC283F40CF2CD56EF9D46D40077AD3E7572A3F40");
  compareCsvLine(line.getRow(3),"EN0211937411M,EW0227133784G,4.35965,2.81856364,1.54108837,0.15811653,0.19695314,2.46915113,0.11640173,76.79463937,190.11558424,16.10716539,2.80090722,2.83622006,0.49093306,0.36245308,0.65204332,0.85415281,0.67643678,0.63513479,1.0,0.03876427,2011112,2011288,239.45744655,31.09327322,237.66803145,34.01279077,2439400,2439400,240.04590622,31.01225938,4620.02587353,239.34576275,37.27724952,3962.00410469,239.45523491,31.09360733,111.17140727,274.49900616,1.08173319,9.16611240,60.18133863,62.75093832,59.12068975,62.83786832,010300000001000000D10000002B711DB6CAFB6D40DECB320C0B3D3E40A4BAE04CE6FA6D40D972F2007E3F3E407E366EC601FA6D40142144E4EF413E4015C602231DF96D40C25545B560443E405D80DB6238F86D4070101573D0463E402DB1358653F76D4038D4D31C3F493E4070D84E8D6EF66D400AABA3B1AC4B3E4073A9647889F56D40DB28A830194E3E40190AB547A4F46D40CC6E069984503E4014127EFBBEF36D404D2EE5E9EE523E40260AFE93D9F26D4047AC6C2258553E40596B7311F4F16D4004C4C641C0573E4034DE1C740EF16D4062EA1E47275A3E40EE3939BC28F06D40BB30A2318D5C3E40AB8307EA42EF6D40CE477F00F25E3E40ADEDC6FD5CEE6D40CE82E6B255613E4080D6B6F776ED6D4019DA0948B8633E403CC816D890EC6D402EEE1CBF19663E409E77269FAAEB6D406D0A55177A683E404FC3254DC4EA6D40DD27E94FD96A3E40FDB254E2DDE96D40EFEF1168376D3E409E76F35EF7E86D4019BF095F946F3E408B6542C310E86D409BA70C34F0713E40AEFD810F2AE76D40FB7358E64A743E40B4E2F24343E66D40A6A92C75A4763E4030DDD5605CE56D407B8BCADFFC783E40C6D96B6675E46D403F1C7525547B3E4054E8F5548EE36D400E217145AA7D3E40133BB52CA7E26D40C423053FFF7F3E40C325EBEDBFE16D405875791153823E40CA1CD998D8E06D40373018BCA5843E4062B4C02DF1DF6D40783A2D3EF7863E40B19FE3AC09DF6D402748069747893E40F6AF831622DE6D406ADDF2C5968B3E40A8D3E26A3ADD6D40AC5044CAE48D3E40951543AA52DC6D40BDCC4DA331903E40069CE6D46ADB6D40CC5264507D923E40DDA70FEB82DA6D4078BCDED0C7943E40BA9300ED9AD96D40D0BD152411973E4018D3FBDAB2D86D4031E7634959993E4066F143B5CAD76D4018A72540A09B3E4034911B7CE2D66D40264CB907E69D3E40456BC52FFAD56D40B2067F9F2AA03E40AB4D84D011D56D40AFEAD8066EA23E40F11A9B5E29D46D4044F12A3DB0A43E402FC94CDA40D36D4080FADA41F1A63E402A61DC4358D26D40F4CE501431A93E4070FD8C9B6FD16D403221F6B36FAB3E4071C9A1E186D06D405E8F3620ADAD3E40A5005E169ECF6D40A2A47F58E9AF3E409EED043AB5CE6D4085DA405C24B23E4025A6E02596CE6D40A24BB783C2B23E408982D949ECCE6D408C6DEFEB17B93E40114FFF0942CF6D40643FE30365BF3E400150F0D397CF6D4027F933DBB1C53E40B0019BA7EDCF6D4066494870FECB3E407BD9ED8443D06D40FD6487C14AD23E400746D76B99D06D40190C59CD96D83E4079AF455CEFD06D403E8F2592E2DE3E40B877275645D16D403DD4550E2EE53E40A4FA6A599BD16D40425B534079EB3E40538EFE65F1D16D40B0438826C4F13E404F83D07B47D26D402C515FBF0EF83E40D324CF9A9DD26D4072F0430959FE3E40FDB8E8C2F3D26D40523CA202A3043F401B810BF449D36D408302E7A9EC0A3F40DCB9252EA0D36D4080C87FFD35113F40909B2571F6D36D405BD0DAFB7E173F40635AF9BC4CD46D409A1D67A3C71D3F409A268F11A3D46D40EE7994F20F243F40CF2CD56EF9D46D40077AD3E7572A3F403096B9D44FD56D40468295819F303F40B6882A43A6D56D407DCB4CBEE6363F40662716BAFCD56D4094676C9C2D3D3F408D926A3953D66D403346681A74433F40F6E715C1A9D66D407239B536BA493F403343065100D76D405FFAC8EFFF4F3F40C8BD29E956D76D40AA2D1A4445563F40776F6E89ADD76D400E6820328A5C3F40706EC23104D86D40033354B8CE623F4097CF13E25AD86D4013112FD512693F40B7A6509AB1D86D405D822B87566F3F40C506675A08D96D400C09C5CC99753F400F0245225FD96D40B12D78A4DC7B3F408BAAD8F1B5D96D40A383C20C1F823F40031210C90CDA6D4052AD220461883F40574AD9A763DA6D4093601889A28E3F40B065228EBADA6D40E16A249AE3943F40C576D97B11DB6D40A1B5C835249B3F401891EC7068DB6D40504A885A64A13F401CC9496DBFDB6D409F56E706A4A73F408B34DF7016DC6D40AD306B39E3AD3F4087EA9A7B6DDC6D40005B9AF021B43F40EB036B8DC4DC6D40AF88FC2A60BA3F40749B3DA61BDD6D4054A11AE79DC03F40FFCD00C672DD6D4003C57E23DBC63F40C7BAA2ECC9DD6D404550B4DE17CD3F409F83111A21DE6D4014E0471754D33F401C4D3B4E78DE6D408955C7CB8FD93F40E33E0E89CFDE6D40D8D9C1FACADF3F40D98378CA26DF6D4000E2C7A205E63F404E4A68127EDF6D40A0326BC23FEC3F404FC4CB60D5DF6D40A4E33E5879F23F4029E7EA28EDDF6D409E50AA384EF33F4074D1A0B8DBE06D40CACBCC4C06F13F4026EAEB04C9E16D40947468F4BFEE3F40794B693DB6E26D409832374978EC3F406DE1D261A3E36D406E09CF4B2FEA3F4070ACE27190E46D408320C8FCE4E73F404AC2526D7DE56D409EC1BC5C99E53F40114FDD536AE66D406E57496C4CE33F400F963C2557E76D40D46B0C2CFEE03F40AAF22AE143E86D406BA6A69CAEDE3F4057D9628730E96D40D7CABABE5DDC3F4083D89E171DEA6D4007B7ED920BDA3F407699999109EB6D406A61E619B8D73F4046E10DF5F5EB6D4028D74D5463D53F40BD91B641E2EC6D405A3ACF420DD33F4041AA4E77CEED6D40EABF17E6B5D03F40BB489195BAEE6D40D5ADD63E5DCE3F4085AA399CA6EF6D400059BD4D03CC3F404B2D038B92F06D4045237F13A8C93F40EE4FA9617EF16D403779D1904BC73F4074B3E71F6AF26D4019D06BC6EDC43F40E51B7AC555F36D408AA307B58EC23F4031711C5241F46D405D73605D2EC03F4018C08AC52CF56D4041C133C0CCBD3F40FD3A811F18F66D40500E41DE69BB3F40E13ABC5F03F76D40E1D849B805B93F402B40F885EEF76D40CE99114FA0B63F4094F3F191D9F86D402CC25DA339B43F4009276683C4F96D40ADB8F5B5D1B13F407FD6115AAFFA6D4005D7A28768AF3F40D928B2159AFB6D4053673019FEAC3F40B77004B684FC6D4067A16B6B92AA3F40622DC63A6FFD6D4011A8237F25A83F40990BB5A359FE6D405F862955B7A53F4071E68EF043FF6D40C52C50EE47A33F4023C811212E006E404C6E6C4BD7A03F40E9EAFB3418016E40A0FD546D659E3F40D9B90B2C02026E40296AE254F29B3F40A7D1FF05EC026E40071DEF027E993F40870197C2D5036E401856577808973F40FE4B9061BF046E40D428F9B591943F40A3E7AAE2A8056E404279B4BC19923F40FB3FA64592066E40C6F86A8DA08F3F4047F6418A7B076E40FC220029268D3F4046E23DB064086E40703A5990AA8A3F4004135AB74D096E4063455DC42D883F40A4CF569F360A6E40700AF5C5AF853F402698F4671F0B6E404C0D0B9630833F403226F410080C6E40468B8B35B0803F40CC6D169AF00C6E40E77764A52E7E3F402D9E1C03D90D6E408E7985E6AB7B3F407322C84BC10E6E40D1E5DFF927793F408629A505E00E6E40C239892480783F4008712756840E6E4057C60D024B723F4006B76C1E290E6E40ADD5C4451D6C3F409F4FC0E9CD0D6E4049387109EF653F40C78335B8720D6E402F867C4EC05F3F4017A7DF89170D6E402495511691593F408B17D25EBC0C6E40E7745C6261533F404B3D2037610C6E406B6B0A34314D3F40778ADD12060C6E4017F1C98C00473F40D87A1DF2AA0B6E40DCAC0A6ECF403F40BC93F3D44F0B6E4074703DD99D3A3F40B06373BBF40A6E406A34D4CF6B343F403D82B0A5990A6E4026144253392E3F40B48FBE933E0A6E40FB49FB6406283F40F034B185E3096E40332B7506D3213F4019239C7B88096E40DD2326399F1B3F40601393752D096E40E7B285FE6A153F40C7C6A973D2086E40E5650C58360F3F40E405F47577086E40F0D4334701093F4097A0857C1C086E408A9E76CDCB023F40DC6D7287C1076E404E6350EC95FC3E407D4BCE9666076E40C9C13DA55FF63E40DD1DADAA0B076E402452BCF928F03E40ACCF22C3B0066E40E7A14AEBF1E93E40B45143E055066E409B2F687BBAE33E408C9A2202FB056E406E6695AB82DD3E4064A6D428A0056E40C699537D4AD73E40B6766D5445056E40EA0025F211D13E400F120185EA046E4083B28C0BD9CA3E40CB83A3BA8F046E4016A00ECB9FC43E40D0DB68F534046E4099912F3266BE3E404F2E6535DA036E40E82075422CB83E408193AC7A7F036E4021B565FDF1B13E40612753C524036E402A7E8864B7AB3E406E096D15CA026E40087065797CA53E40695C0E6B6F026E40343E853D419F3E400C464BC614026E40135771B205993E40C8EE3727BA016E401FDFB3D9C9923E408A81E88D5F016E4052ACD7B48D8C3E40642B71FA04016E405F41684551863E40661BE66CAA006E40FAC8F18C14803E403B825BE54F006E400411018DD7793E40FA91E563F5FF6D40DC8523479A733E40DD7D98E89AFF6D40762DE7BC5C6D3E40F479887340FF6D409EA2DAEF1E673E40F2BAC904E6FE6D4008108DE1E0603E40CD75709C8BFE6D40812B8E93A25A3E409DDF903A31FE6D401C316E0764543E40332D3FDFD6FD6D4026DEBD3E254E3E40F2928F8A7CFD6D405E6C0E3BE6473E407644963C22FD6D40FD8CF1FDA6413E40567467F5C7FC6D40BD63F988673B3E405D201FB8B0FC6D405B6E1550923A3E402B711DB6CAFB6D40DECB320C0B3D3E40");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211937488M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EN0211937488M,EN0211980980M,5.70052,2.79238659,2.90812844,0.10927996,0.12145600,1.02562955,1.34275050,170.78100922,210.03935368,5.71937263,2.79013316,2.79464001,0.41107532,0.48762166,0.71290684,0.69396177,0.68590471,0.63335786,0.77158790,0.81324616,2011112,2011112,238.58356334,33.67370765,238.84908551,33.56874368,2439400,2439400,240.41861335,33.17318666,4500.63711629,237.50495352,34.12378688,4449.88425716,238.72050375,33.61225664,105.18221135,102.55380329,3.51627363,2.75847090,61.77849353,60.24756670,58.26857601,62.99779185,010300000001000000BD000000FC2F48B119DF6D40B70C46A1346E4040896A14763CDE6D404E71BE46666F40409CBBC3195FDD6D4096FFDA6E97704040F5CD909C81DC6D40B8F82819C8714040B085B6FEA3DB6D40554E3645F872404080FF6F40C6DA6D4038A491F2277440400490F861E8D96D40D851CA205775404005C38B630AD96D400A6470CF85764040C65A65452CD86D408D9E14FEB3774040394FC1074ED76D409F7D48ACE17840404FCDDBAA6FD66D4086379ED90E7A40403336F12E91D56D401DBEA8853B7B4040871E3E94B2D46D4058C0FBAF677C4040A84DFFDAD3D36D40CDAB2B58937D4040E8BC7103F5D26D4024AECD7DBE7E40403AAA66C84FD26D4034A8CECD9B7F40403499349D9AD16D4016C77DEA9A8040401E58599FC2D06D4073FA7F69CA814040350DFF8EEACF6D40503EBA5BF98240408E86646C12CF6D4080A5D3C027844040ECAFC8373ACE6D401F1B749855854040F1916AF161CD6D40AF6344E2828640405251899989CC6D402E1EEE9DAF874040F92D6430B1CB6D4030C51BCBDB8840403B823AB6D8CA6D40E9AF7869078A404001C24B2B00CA6D402F13B178328B4040F379D78F27C96D408F0272F85C8C4040A84E1DE44EC86D403D7169E8868D4040C1FB5C2876C76D4015334648B08E40402B53D65C9DC66D4089FDB717D98F4040323CC981C4C56D4091686F5601914040BBB27597EBC46D4099EF1D042992404063C61B9E12C46D4058F2752050934040AC99FB9539C36D40BCB52AAB769440401E61557F60C26D40BA64F0A39C9540407D62695A87C16D401E117C0AC2964040E5F37727AEC06D405EB483DEE6974040F17AC1E6D4BF6D405B30BE1F0B994040EF6B8698FBBE6D402E50E3CD2E9A4040F748073D22BE6D40D3C8ABE8519B40401BA184D448BD6D40EC39D16F749C40408D0F3F5F6FBC6D40712E0E63969D4040E73BD3BF52BC6D40F3DA41D7E29D40405ED1AE1CACBC6D4082E32007C4A040406344A71305BD6D406C07EA6BA1A3404096AE72165EBD6D405936DAAF7EA640400C14FE24B7BD6D405BC232D25BA94040E06F363F10BE6D40333F35D238AC404072B4086569BE6D409C8423AF15AF4040A6CB6196C2BE6D4094B03F68F2B1404022972ED31BBF6D40BE29CCFCCEB4404096F05B1B75BF6D40A0A10B6CABB74040F1A9D66ECEBF6D40F41641B587BA4040AD8D8BCD27C06D40F1D7AFD763BD4040105F673781C06D4087849BD23FC040405FDA56ACDAC06D40B51048A51BC340402DB5462C34C16D40BBC6F94EF7C54040979E23B78DC16D405949F5CED2C84040873FDA4CE7C16D4011967F24AECB4040EE3A57ED40C26D405907DE4E89CE4040102E87989AC26D40DB56564D64D14040BDB0564EF4C26D40979F2E1F3FD440409355B20E4EC36D402160ADC319D7404045AA86D9A7C36D40C87C193AF4D94040D837C0AE01C46D40C041BA81CEDC4040DD824B8E5BC46D404765D799A8DF4040C40B1578B5C46D40C909B98182E24040094F096C0FC56D40FEBFA7385CE5404083C5146A69C56D400A89ECBD35E840409FE42372C3C56D4093D8D0100FEB4040A61E23841DC66D40CE969E30E8ED4040F4E2FE9F77C66D40A622A01CC1F04040459EA3C5D1C66D40AF5320D499F34040ECBAFDF42BC76D40457C6A5672F6404018A1F92D86C76D40836BCAA24AF9404016B78370E0C76D40526F8CB822FC40408E6188BC3AC86D405C56FD96FAFE4040C403F41195C86D4014726A3DD2014140DFFFB270EFC86D409A9821ABA904414016B7B1D849C96D40BD2671DF800741400A8ADC49A4C96D40E601A8D9570A4140F2D81FC4FEC96D40FD9915992E0D4140E803684759CA6D404BEB091D05104140166BA1D3B3CA6D406F80D564DB1241400E6FB8680ECB6D402374C96FB1154140F470990669CB6D402473373D87184140CCD230ADC3CB6D40FEBD71CC5C1B4140A9F76A5C1ECC6D40E52ACB1C321E41400044341479CC6D407727972D07214140D11D79D4D3CC6D4089BA29FEDB234140F7EC259D2ECD6D40E285D78DB0264140541B276E89CD6D4002C8F5DB8429414028156947E4CD6D40D25DDAE7582C41402E49D8283FCE6D405FC4DBB02C2F4140A9264FC656CE6D4064F83A078C2F41407A15EDA235CF6D4055B2C7B8642E4140A2D7805413D06D40D474D83F3E2D41402BADA8F6F0D06D40D235FF24172C414055482189CED16D40949C8368EF2A4140BD6CA70BACD26D402154AE0AC72941403DF0F77D89D36D40A20AC90B9E284140D4BBCFDF66D46D4095701E6C742741407FCCEB3044D56D401B38FA2B4A2641402034097121D66D402314A94B1F254140561AE59FFED66D40AAB778CBF32341406DBD3CBDDBD76D40E4D4B7ABC72241402C73CDC8B8D86D405B1CB6EC9A214140BFA954C295D96D40163CC48E6D20414093E88FA972DA6D40BADE33923F1F414035D13C7E4FDB6D408CAA57F7101E41402C2019402CDC6D40964083BEE11C4140DFADE2EE08DD6D409A3B0BE8B11B4140696F578AE5DD6D40232F4574811A41407A773512C2DE6D407AA687635019414032F73A869EDF6D40A3232AB61E184140F53E26E67AE06D40491E856CEC16414053BFB53157E16D40A502F286B9154140D909A86833E26D407030CB0586144140E8D1BB8A0FE36D40B2F96BE9511341408DEDAF97EBE36D40A4A130321D1241405F56438FC7E46D408C5B76E0E7104140552A3571A3E56D407A499BF4B10F41408AAC443D7FE66D401B7BFE6E7B0E41402E4631F35AE76D407EECFF4F440D41404287BA9236E86D40CC8400980C0C41407427A01B12E96D40FC146247D40A4140F406A28DEDE96D409456875E9B0941403B2F80E8C8EA6D4049EAD3DD61084140E7D3FA2BA4EB6D40B156ACC5270741407D53D2577FEC6D40D3067616ED0541404238C76B5AED6D40E34897D0B1044140F9389A6735EE6D40BD4C77F475034140C2390C4B10EF6D4087227E8239024140D44CDE15EBEF6D4038B9147BFC00414049B3D1C7C5F06D4030DDA4DEBEFF4040ECDDA760A0F16D40A23699AD80FE4040A9B99D4EBBF16D40C97FE16359FE40406BA84D8687F16D40E0E7BDF3B4FC4040A915DCE62BF16D405C199BB9CCF940408092F949D0F06D40F43E3844E4F640401102BAAF74F06D403EA14294FBF340406952311819F06D40631D68AA12F14040457C7383BDEF6D404323578729EE4040CF8294F161EF6D40C3B3BE2B40EB40406773A86206EF6D40DB5E4E9856E840406365C3D6AAEE6D40DA41B6CD6CE54040DF79F94D4FEE6D407F05A7CC82E2404069DB5EC8F3ED6D4012DCD19598DF4040D4BD074698ED6D40917FE829AEDC4040F95D08C73CED6D40BF2F9D89C3D940407701754BE1EC6D4031B0A2B5D8D640406DF661D385EC6D406746ACAEEDD340404293E35E2AEC6D40DEB76D7502D1404068360EEECEEB6D4001489B0A17CE40401846F68073EB6D4045B6E96E2BCB40401030B01718EB6D401C3C0EA33FC84040566950B2BCEA6D40F28ABEA753C54040F96DEB5061EA6D4022CAB07D67C24040CAC095F305EA6D40F7949B257BBF40401FEB639AAAE96D4090F835A08EBC4040947C6A454FE96D40CA7137EEA1B94040BB0ABEF4F3E86D4042EB5710B5B64040F23073A898E86D4016BB4F07C8B3404008909E603DE86D40ECA0D7D3DAB040400BCE541DE2E76D40B5C3A876EDAD4040FC95AADE86E76D409BAF7CF0FFAA40408F97B4A42BE76D40CF530D4212A84040E686876FD0E66D406800156C24A54040521C383F75E66D4029644E6F36A240400714DB131AE66D405A8A744C489F4040E02D85EDBEE56D4096D842045A9C4040162D4BCC63E56D408E0C75976B994040FBD741B008E56D40CE39C7067D964040B5F77D99ADE46D408BC7F5528E9340400158148852E46D405A6EBD7C9F904040DEC6197CF7E36D40F635DB84B08D40405B14A3759CE36D40F3720C6CC18A40404112C57441E36D4086C40E33D2874040DB939479E6E26D402E12A0DAE2844040A06D26848BE26D406E897E63F3814040FD748F9430E26D40839B68CE037F40400B80E4AAD5E16D4013FB1C1C147C404044653AC77AE16D40D4995A4D247940403CFBA5E91FE16D4044A6E0623476404064183C12C5E06D4049896E5D44734040C09211416AE06D40E4E3C33D54704040983F3B760FE06D40CD8CA004646D4040DEC88773F8DF6D40B19AA532006D4040FC2F48B119DF6D40B70C46A1346E4040");
  compareCsvLine(line.getRow(2),"EN0211937488M,EN0211981012M,5.69464,2.78385138,2.91078554,0.10840106,0.11358389,1.05147872,1.95165838,170.18670945,183.49119548,6.54015009,2.79013316,2.77756960,0.41107532,0.47462970,0.71290684,0.69276657,0.68590471,0.63099985,0.23756384,0.26022329,2011112,2011112,238.58356334,33.67370765,238.99038636,34.52754626,2439400,2439400,240.41861335,33.17318666,4500.63711629,237.67949104,35.07673579,4400.48853663,238.78716235,34.11459297,105.18221135,100.03265745,3.51627363,2.71066287,61.77849353,60.50740585,58.26857601,63.21001274,01030000000100000083000000F5A97B9D98C56D4071F5BA839E3D41407B5E8B5A7FC66D40D3031028803C414056E364FF65C76D4051E4AA2E613B414006B4C38B4CC86D404B6CDB97413A41409E6563FF32C96D403A73F26321394140BAA7FF5919CA6D40D4D14193003841406245549BFFCA6D403D611C26DF364140F4251DC3E5CB6D4036FAD51CBD354140024E16D1CBCC6D403474C3779A34414044E0FBC4B1CD6D407EA43A3777334140751E8A9E97CE6D404A5D925B53324140356A7D5D7DCF6D40BE6C22E52E314140FE45920163D06D40009C43D409304140F555858A48D16D403BAE4F29E42E4140D56013F82DD26D40975FA1E4BD2D4140CF50F94913D36D4033649406972C41407234F47FF8D36D400967858F6F2B4140803FC199DDD46D40F508D27F472A4140DCCB1D97C2D56D407DDFD8D71E2941405D5AC777A7D66D40BA73F997F5274140B7937B3B8CD76D40364194C0CB2641404B49F8E170D86D40BEB40A52A12541401576FB6A55D96D40232BBF4C762441407A3F43D639DA6D4016F014B14A23414023F68D231EDB6D40DD3C707F1E224140E2169A5202DC6D400F3736B8F12041407E4B2663E6DC6D4056EFCC5BC41F4140906BF154CADD6D400E609B6A961E41405D7DBA27AEDE6D40086C09E5671D4140A7B640DB91DF6D401ADD7FCB381C4140807D436F75E06D40D562681E091B4140206982E358E16D400D912DDED8194140B642BD373CE26D407FDE3A0BA81841403B06B46B1FE36D4059A3FCA57617414039E3267F02E46D40CC17E0AE44164140A63DD671E5E46D408D52532612154140A8AE8243C8E56D405B47C50CDF1341405F05EDF3AAE66D407CC5A562AB124140B747D6828DE76D40337665287711414029B3FFEF6FE86D4032DB755E4210414087BD2A3B52E96D400B4D49050D0F4140C415196434EA6D4099F9521DD70D4140ACA48C6A16EB6D405EE206A7A00C4140B78D474EF8EB6D40EEDAD9A2690B4140C42F0C0FDAEC6D403B874111320A4140CE259DACBBED6D40015AB4F2F9084140C247BD269DEE6D400893A947C107414022AB2F7D7EEF6D40693D991088064140D3A3B7AF5FF06D40F92DFC4D4E054140F7D7DA9AB7F06D4008BC8385D3044140783C41255AF06D40F48F1C9E1B024140DC3BDA7FFBEF6D4033BAB7805AFF4040F75288E29CEF6D40EAD6B43F99FC4040FC9B614D3EEF6D40739DC8DBD7F9404042307CC0DFEE6D40960FA85516F74040EF27EE3B81EE6D40537708AE54F44040B099CDBF22EE6D40B2649FE592F140407D9A304CC4ED6D40A1AB22FDD0EE40403B3D2DE165ED6D40B56148F50EEC40407C92D97E07ED6D40FADBC6CE4CE9404034A84B25A9EC6D40BCAC548A8AE64040748999D44AEC6D405EA1A828C8E34040123ED98CECEB6D4005C079AA05E140406BCA204E8EEB6D4073457F1043DE4040F980334177EB6D404FFB5B8AE6DD4040A9AF5CD2A3EA6D40AE4BE2B925DF4040940546DDD1E96D40FD98CE0962E04040A3A753CFFFE86D40CF6D73DA9DE140403F9BBFA82DE86D409A45642BD9E240402B17C4695BE76D40954C35FC13E44040D7829B1289E66D4065617B4C4EE54040A17580A3B6E56D409216CC1B88E6404022B6AD1CE4E46D4020B4BD69C1E7404078395E7E11E46D400939E735FAE840408822CDC83EE36D40B95CE07F32EA404044C135FC6BE26D409F9041476AEB4040EE91D31899E16D408B01A48BA1EC4040613CE21EC6E06D403299A14CD8ED404048939D0EF3DF6D4099FFD4890EEF4040669341E81FDF6D40819CD94244F04040D2620AAC4CDE6D40CB984B7779F140403B50345A79DD6D40DDDFC726AEF2404013D2FBF2A5DC6D400221EC50E2F34040E7859D76D2DB6D40C5D056F515F54040812F56E5FEDA6D403C2AA71349F6404030B8623F2BDA6D405F307DAB7BF74040FC2D008557D96D4057AF79BCADF84040DCC26BB683D86D40C53D3E46DFF94040F5CBE2D3AFD76D40FE3D6D4810FB4040C9C0A2DDDBD66D404BDFA9C240FC40406E3AE9D307D66D40221F98B470FD4040C9F2F3B633D56D4061CADC1DA0FE4040B0C300875FD46D407A7E1DFECEFF404037A64D448BD36D408BAA0055FD004140CFB118EFB6D26D40A2902D222B0241407B1BA087E2D16D40C3464C65580341400A35220E0ED16D4017B8051E850441403A6CDD8239D06D40EFA5034CB1054140F34910E664CF6D40E7A8F0EEDC0641407071F93790CE6D40E3317806080841406D9FD778BBCD6D40208B4692320941405BA9E9A8E6CC6D402BD908925C0A4140847C6EC811CC6D40E81B6D05860B4140461DA5D73CCB6D407C2F22ECAE0C41402FA6CCD667CA6D404FCDD745D70D4140334724C692C96D40E68C3E12FF0E4140D644EBA5BDC86D40D8E40751261041405DF76076E8C76D40A22BE6014D114140EAC9C43713C76D408F988C2473124140B63956EA3DC66D408544AFB89813414034D5548E68C56D40DC2A03BEBD1441403C3B002493C46D40202A3E34E2154140391A98ABBDC36D40E804171B061741404B2F5C25E8C26D40826245722918414078458C9112C26D40B7CF81394C194140D43468F03CC16D4082BF85706E1A4140C12436F020C16D40CE0803FCB91A4140A3CDD75E7BC16D4062A5B8F4851D41403B3E5E66D5C16D402E4DD23D4E204140B2ADDB792FC26D4013928B661623414035A53C9989C26D4008FD296EDE254140CCA46DC4E3C26D409557F353A62841409F235BFB3DC36D4022AE2D176E2B41402B90F13D98C36D4039521FB7352E414093501D8CF2C36D40C1DC0E33FD304140DEC2CAE54CC46D404330438AC4334140353DE64AA7C46D40297B03BC8B364140210E5CBB01C56D40ED3997C752394140DC7C18375CC56D405C3946AC193C41403FDD15398FC56D40B4CF0C24AA3D4140F5A97B9D98C56D4071F5BA839E3D4140");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211980850M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EN0211980850M,EW0227133784G,4.98726,2.80464434,2.18261987,0.18368022,0.12472243,2.42699615,2.82163895,83.41546724,169.16033887,17.98047493,2.77306862,2.83622006,0.31718607,0.36245308,0.77937542,0.85415281,0.69461080,0.63513479,0.27141281,0.01097468,2011112,2011288,233.58275757,31.29165399,237.66803145,34.01279077,2439400,2439400,236.86940318,30.45894607,4651.34307233,239.34576275,37.27724952,3962.00410469,233.76324902,31.76103529,113.10236561,274.49900616,6.17501684,9.16611240,63.65627845,62.75093832,57.48502790,62.83786832,010300000001000000680000008FC1CF3FEA1D6D40D0BD6E4685D03F400679096A371E6D40959A96B9FDD63F40FFF9AB98841E6D40DDE902B875DD3F400BD6A7CBD11E6D40C3C92F40EDE33F40D4A1ED021F1F6D404F899A5064EA3F4049F56D3E6C1F6D40C3ACC1E7DAF03F40CF6B197EB91F6D408BF1240451F73F407AA4E0C106206D40805245A4C6FD3F403442B40954206D40EC8552E31D024040F1EB8455A1206D4099CFE33458054040E14C43A5EE206D40CAEB1846920840409E14E0F83B216D40A4E63416CC0B404057F74B5089216D40E5747BA4050F404011AE77ABD6216D40CEF530F03E12404073846070ED216D401858BB0FAF1240408E6214BAE6226D4098C5AD8FAA114040891069ACDE236D40515EBDB1A6104040EB66A784D6246D40CB17D82AA20F404048AE8742CE256D40D79148FB9C0E4040B647C2E5C5266D40C77E5A23970D4040C1AD0F6EBD276D40C1A25AA3900C4040647528DBB4286D40FCD2967B890B4040F54EC52CAC296D40FCF45DAC810A40401F079F62A32A6D40CAFDFF3579094040D3876E7C9A2B6D402FF1CD187008404039D9EC79912C6D40C1E0195566074040A222D35A882D6D4029EB36EB5B0640407FABDA1E7F2E6D40233B79DB5005404044DCBCC5752F6D40AB06362645044040693F334F6C306D40F38DC3CB380340404F82F7BA62316D40831A79CC2B0240403076C30859326D4033FEAE281E0140400A1151384F336D401D92BEE00F0040409B6E5A4945346D405D6B04EA01FE3F403AD1993B3B356D40EF9AAACBE2FB3F40CBA2C90E31366D4052842867C2F93F40B275A4C226376D40E6FE38BDA0F73F40A905E5561C386D40BEE598CE7DF53F40BF3846CB11396D405D15079C59F33F402E20831F073A6D404169442634F13F4050F95653FC3A6D4087B9136E0DEF3F40782E7D66F13B6D4079D83974E5EC3F40DF57B158E63C6D4007907D39BCEA3F40893CAF29DB3D6D40309FA7BE91E83F4020D332D9CF3E6D4069B7820466E63F40DD42F866C43F6D400B7ADB0B39E43F4064E4BBD2B8406D40907580D50AE23F40A8423A1CAD416D40DE224262DBDF3F40BB1B3043A1426D4068E2F2B2AADD3F40C1615A4795436D408CF966C878DB3F40B83B762889446D407D8F74A345D93F405C0641E67C456D407FAAF34411D73F40F954788070466D40E52CBEADDBD43F404CF2D9F663476D400ED2AFDEA4D23F404CE1234957486D40642BA6D86CD03F400D5E14774A496D40409D809C33CE3F4080DE69803D4A6D40D05B202BF9CB3F405C13E364304B6D40DF676885BDC93F40D9E83E24234C6D40BD8B3DAC80C73F408B873CBE154D6D40DE5786A042C53F4023559B32084E6D40B41F2B6303C33F4050F51A81FA4E6D403DF615F5C2C03F406F4A7BA9EC4F6D40C6AA325781BE3F405D767CABDE506D4062C56E8A3EBC3F4047DBDE86D0516D409B83B98FFAB93F40621C633BC2526D40F2D40368B5B73F40172A6BCEE2526D40D9EE6B8E14B73F4043F4C9E190526D4039E8DB1BA9B03F407307C75D3F526D4021C4905845AA3F40429E1BDAED516D40996A2719E1A33F40308ED8569C516D4020B5125F7C9D3F40A3B80ED44A516D40B3BDC62B17973F40B50ACF51F9506D40E5DAB880B1903F40FD7C2AD0A7506D401D9C5F5F4B8A3F406B13324F56506D409AC532C9E4833F40FEDCF6CE04506D40824CABBF7D7D3F40AFF3894FB34F6D40DD52434416773F40217CFCD0614F6D4097237658AE703F407CA55F53104F6D407F2EC0FD456A3F4038A9C4D6BE4E6D4006049F35DD633F40CFF8FF7A714E6D405D308012C75D3F4066086B9A774D6D409E61377C1D603F40B7AC3612244B6D4077CDA2B4AA653F40751F2916D0486D40FC261B07346B3F409C8D40A57B466D40E6738074B9703F402F657ABE26446D4079BAABFD3A763F40AF51D360D1416D4005026FA3B87B3F40A338478B7B3F6D402A53956632813F401036D13C253D6D40EAB7E247A8863F40F4986B74CE3A6D40B83B14481A8C3F40B0DF0F3177386D4020EBDF6788913F4077B4B6711F366D4077D3F4A7F2963F40AFE95735C7336D403802FB08599C3F404D76EA7A6E316D405984938BBBA13F4038726441152F6D40576558301AA73F408312BB87BB2C6D400AAEDCF774AC3F40D7A5E24C612A6D408763ACE2CBB13F409790CE8F06286D408A854CF11EB73F403549714FAB256D40EF0C3B246EBC3F405A54BC8A4F236D40D5E9EE7BB9C13F401A41A040F3206D40B401D8F800C73F401BA50C70961E6D40292D5F9B44CC3F400A392670CC1D6D40A29FE30405CE3F408FC1CF3FEA1D6D40D0BD6E4685D03F40");
  compareCsvLine(line.getRow(2),"EN0211980850M,EW0227176126G,4.60429,2.77362424,1.83067055,0.18534848,0.06726772,2.42716581,1.88460117,84.46683415,179.32352404,17.02306207,2.77306862,2.77417986,0.31718607,0.31179369,0.77937542,0.85405346,0.69461080,0.62385731,0.91703014,0.03709076,2011112,2011288,233.58275757,31.29165399,234.73576933,33.97866162,2439400,2439400,236.86940318,30.45894607,4651.34307233,236.33684517,37.26678462,3962.11042265,233.60369041,31.34485682,113.10236561,274.51819495,6.17501684,9.16213875,63.65627845,63.76412601,57.48502790,63.85284140,010300000001000000AA00000039DAC37A4F146D40B613E163AA003F4020BEF0F79B146D40723E25362E073F4049F8757BE8146D40D8F036C6B10D3F407220440535156D40A984751235143F4008CB4B9581156D40A7EE4019B81A3F4051897D2BCE156D40BCC3F9D83A213F40ADE9C9C71A166D400C3E0150BD273F40BD77216A67166D40E841B97C3F2E3F40A2BC7412B4166D40F962845DC1343F402C3FB4C000176D4028E9C5F0423B3F401284D0744D176D40A1D5E134C4413F401D0EBA2E9A176D40D1E73C2845483F40695E61EEE6176D404BA23CC9C54E3F408DF4B6B333186D40A54F471646553F40D94EAB7E80186D408707C40DC65B3F4089EA2E4FCD186D4053B31AAE45623F40EE4332251A196D401B13B4F5C4683F40B0D6A50067196D4068C2F9E2436F3F40FC1D7AE1B3196D40FD3C5674C2753F40B2949FC7001A6D40A6E334A8407C3F40A2B506B34D1A6D40D500027DBE823F40BEFB9FA39A1A6D4076CD2AF13B893F4045E25B99E71A6D407D751D03B98F3F4006E52A94341B6D40A11C49B135963F407E80FD93811B6D40DDE21DFAB19C3F402332C498CE1B6D4015E90CDC2DA33F4083786FA21B1C6D40A4558855A9A93F4081D3EFB0681C6D40CD58036524B03F4089C435C4B51C6D403831F2089FB63F40B9CE31DC021D6D407930CA3F19BD3F402277D4F84F1D6D405BBF010893C33F40EB440E1A9D1D6D40556210600CCA3F408FC1CF3FEA1D6D40D0BD6E4685D03F400679096A371E6D40959A96B9FDD63F40FFF9AB98841E6D40DDE902B875DD3F400BD6A7CBD11E6D40C3C92F40EDE33F40D4A1ED021F1F6D404F899A5064EA3F4049F56D3E6C1F6D40C3ACC1E7DAF03F40CF6B197EB91F6D408BF1240451F73F407AA4E0C106206D40805245A4C6FD3F403442B40954206D40EC8552E31D024040F1EB8455A1206D4099CFE33458054040E14C43A5EE206D40CAEB1846920840409E14E0F83B216D40A4E63416CC0B404057F74B5089216D40E5747BA4050F404011AE77ABD6216D40CEF530F03E12404073846070ED216D401858BB0FAF1240408E6214BAE6226D4098C5AD8FAA114040891069ACDE236D40515EBDB1A6104040EB66A784D6246D40CB17D82AA20F404048AE8742CE256D40D79148FB9C0E4040B647C2E5C5266D40C77E5A23970D4040C1AD0F6EBD276D40C1A25AA3900C4040647528DBB4286D40FCD2967B890B4040F54EC52CAC296D40FCF45DAC810A40401F079F62A32A6D40CAFDFF3579094040D3876E7C9A2B6D402FF1CD187008404039D9EC79912C6D40C1E0195566074040A222D35A882D6D4029EB36EB5B0640407FABDA1E7F2E6D40233B79DB5005404044DCBCC5752F6D40AB06362645044040693F334F6C306D40F38DC3CB380340404F82F7BA62316D40831A79CC2B0240403076C30859326D4033FEAE281E0140400A1151384F336D401D92BEE00F0040409B6E5A4945346D405D6B04EA01FE3F403AD1993B3B356D40EF9AAACBE2FB3F40CBA2C90E31366D4052842867C2F93F40B275A4C226376D40E6FE38BDA0F73F40A905E5561C386D40BEE598CE7DF53F40BF3846CB11396D405D15079C59F33F402E20831F073A6D404169442634F13F4050F95653FC3A6D4087B9136E0DEF3F40782E7D66F13B6D4079D83974E5EC3F40DF57B158E63C6D4007907D39BCEA3F40893CAF29DB3D6D40309FA7BE91E83F4020D332D9CF3E6D4069B7820466E63F40DD42F866C43F6D400B7ADB0B39E43F4064E4BBD2B8406D40907580D50AE23F40A8423A1CAD416D40DE224262DBDF3F40BB1B3043A1426D4068E2F2B2AADD3F40C1615A4795436D408CF966C878DB3F40B83B762889446D407D8F74A345D93F405C0641E67C456D407FAAF34411D73F40F954788070466D40E52CBEADDBD43F404CF2D9F663476D400ED2AFDEA4D23F404CE1234957486D40642BA6D86CD03F400D5E14774A496D40409D809C33CE3F4080DE69803D4A6D40D05B202BF9CB3F405C13E364304B6D40DF676885BDC93F40D9E83E24234C6D40BD8B3DAC80C73F408B873CBE154D6D40DE5786A042C53F4023559B32084E6D40B41F2B6303C33F4050F51A81FA4E6D403DF615F5C2C03F406F4A7BA9EC4F6D40C6AA325781BE3F405D767CABDE506D4062C56E8A3EBC3F4047DBDE86D0516D409B83B98FFAB93F40621C633BC2526D40F2D40368B5B73F40172A6BCEE2526D40D9EE6B8E14B73F4043F4C9E190526D4039E8DB1BA9B03F407307C75D3F526D4021C4905845AA3F40429E1BDAED516D40996A2719E1A33F40308ED8569C516D4020B5125F7C9D3F40A3B80ED44A516D40B3BDC62B17973F40B50ACF51F9506D40E5DAB880B1903F40FD7C2AD0A7506D401D9C5F5F4B8A3F406B13324F56506D409AC532C9E4833F40FEDCF6CE04506D40824CABBF7D7D3F40AFF3894FB34F6D40DD52434416773F40217CFCD0614F6D4097237658AE703F407CA55F53104F6D407F2EC0FD456A3F4038A9C4D6BE4E6D4006049F35DD633F40E6CA3C5B6D4E6D4048519101745D3F40FB57D9E01B4E6D40BDDB16630A573F4099A7AB67CA4D6D401F7DB05BA0503F405E1AC5EF784D6D402C1FE0EC354A3F402B1A3779274D6D404CB72818CB433F40EC191304D64C6D404A420EDF5F3D3F4065956A90844C6D4019C01543F4363F40F4104F1E334C6D40512FC54588303F406719D2ADE14B6D40E488A3E81B2A3F40B743053F904B6D4097BB382DAF233F40D52CFAD13E4B6D409FA70D15421D3F406D79C266ED4A6D401C1AACA1D4163F40BED56FFD9B4A6D4093C89ED466103F4047F513964A4A6D40734C71AFF8093F40A392C030F9496D404C1EB0338A033F40486F87CDA7496D407691E8621BFD3E404E537A6C56496D4043CFA83EACF63E40300DAB0D05496D4069D27FC83CF03E4099712BB1B3486D403F62FD01CDE93E40235B0D5762486D401F0EB2EC5CE33E402AAA62FF10486D4097282F8AECDC3E407C443DAABF476D409FC206DC7BD63E402A15AF576E476D40ECA6CBE30AD03E404E0CCA071D476D40FE5411A399C93E40CF1EA0BACB466D4068FC6B1B28C33E401B4643707A466D40D877704EB6BC3E40F87FC52829466D405248B43D44B63E4040CE38E4D7456D403090CDEAD1AF3E40A836AFA286456D404F0E53575FA93E407FC23A6435456D400719DC84ECA23E40777EED28E4446D4042990075799C3E40677AD9F092446D408205592906963E4009C910BC41446D40DB5C7EA3928F3E40C07FA58AF0436D40F9210AE51E893E40E62FC20FBD436D400C7A394E07853E4096D39CB84E426D40103E6E5BB0883E40BB64651E05406D409E23115B858E3E409D031A36BB3D6D406DAB3A3C56943E40545DCCFE703B6D40A45BB200239A3E4058D68D7726396D405A3C38AAEB9F3E4090876F9FDB366D4005DE843AB0A53E40393B827590346D40B95F49B370AB3E40ED69D6F844326D40E5742F162DB13E408C377C28F92F6D40576BD964E5B63E403B708303AD2D6D40C830E2A099BC3E404A85FB88602B6D403158DDCB49C23E40338AF3B713296D40311F57E7F5C73E4081317A8FC6266D400473D4F49DCD3E40BCC99D0E79246D4073F5D2F541D33E405C3A6C342B226D408D01C9EBE1D83E40A700F3FFDC1F6D4032B025D87DDE3E40A12C3F708E1D6D4071DC50BC15E43E40EF5D5D843F1B6D40A627AB99A9E93E40AFC0593BF0186D409DFD8D7139EF3E40670A4094A0166D4054984B45C5F43E40D8761B8E50146D40B4032F164DFA3E400EA51D410C146D40C62D8CDCEFFA3E4039DAC37A4F146D40B613E163AA003F40");
  compareCsvLine(line.getRow(3),"EN0211980850M,EW0227218472G,4.39397,2.74055526,1.65341925,0.18555976,0.11434706,2.41728967,1.07254416,85.31556030,163.13218050,18.53062419,2.77306862,2.70804191,0.31718607,0.25900289,0.77937542,0.85204463,0.69461080,0.61228630,1.0,0.04071345,2011112,2011289,233.58275757,31.29165399,231.83005815,34.13645061,2439400,2439400,236.86940318,30.45894607,4651.34307233,233.35512703,37.40672711,3956.10521980,233.57878030,31.29238546,113.10236561,273.40118013,6.17501684,9.08178520,63.65627845,64.81994211,57.48502790,64.89423285,010300000001000000D1000000F8ECB363A6416D4038243861A4703E40DCF08EC5B8406D4007A017A0D5723E404E42B503CB3F6D402C05E0CE05753E408874641EDD3E6D4008CBB0EC34773E408D55DA15EF3D6D40F9EAAAF862793E4060ED54EA003D6D40B5E3F0F18F7B3E40407D129C123C6D4052BCA6D7BB7D3E40E87E512B243B6D408707F2A8E67F3E40C4A35098353A6D40C8E6F96410823E402CD44EE346396D404B0DE70A39843E409B2E8B0C58386D4015C3E39960863E40DD06451469376D4018E81B1187883E4046E5BBFA79366D4012F7BC6FAC8A3E40F0852FC08A356D407D08F6B4D08C3E40D7D7DF649B346D4085D5F7DFF38E3E4019FC0CE9AB336D40DFBAF4EF15913E401B45F74CBC326D409CBB20E436933E40BB35DF90CC316D40E983B1BB56953E407A8005B5DC306D40E46BDE7575973E40A906ABB9EC2F6D40477AE01193993E408FD7109FFC2E6D401667F28EAF9B3E40892F78650C2E6D40319E50ECCA9D3E404977220D1C2D6D400B423929E59F3E40E34251962B2C6D40292EEC44FEA13E40045146013B2B6D4098F9AA3E16A43E40058A434E4A2A6D407FF9B8152DA63E401FFF8A7D59296D406B435BC942A83E4081E95E8F68286D40DAAFD85857AA3E4074A9018477276D407DDC79C36AAC3E4081C5B55B86266D407F2E89087DAE3E4087E9BD1695256D40E0D452278EB03E40DDE55CB5A3246D4098CA241F9EB23E4076AED537B2236D40CAD84EEFACB43E40EE596B9EC0226D40E5982297BAB63E40BC2061E9CE216D40BD76F315C7B83E40365CFA18DD206D409CB2166BD2BA3E40BE857A2DEB1F6D402A63E395DCBC3E40CE352527F91E6D408D77B295E5BE3E401F233E06071E6D4014B9DE69EDC03E40BB2109CB141D6D4051CDC411F4C23E400A22CA75221C6D40C837C38CF9C43E40FF2FC506301B6D40B65B3ADAFDC63E4022723E7E3D1A6D40E77D8CF900C93E40A5287ADC4A196D4033C61DEA02CB3E4082ACBC2158186D40584154AB03CD3E40906E4A4E65176D406CE2973C03CF3E408EF6676272166D407484529D01D13E403FE2595E7F156D40D9EBEFCCFED23E408CE464428C146D40F3C7DDCAFAD43E407DC4CD0E99136D4056B48B96F5D63E40645CD9C3A5126D40393A6B2FEFD83E40C553A1B284126D409616CC0A86D93E4036B22A6AD1126D40F6CBB5A013E03E400E4EB7C61D136D4098C678A598E63E403E10E9296A136D405DEA34701DED3E4047A4B093B6136D40D8D646FFA1F33E4047B1FE0303146D40D7AC0B5126FA3E4039DAC37A4F146D40B613E163AA003F4020BEF0F79B146D40723E25362E073F4049F8757BE8146D40D8F036C6B10D3F407220440535156D40A984751235143F4008CB4B9581156D40A7EE4019B81A3F4051897D2BCE156D40BCC3F9D83A213F40ADE9C9C71A166D400C3E0150BD273F40BD77216A67166D40E841B97C3F2E3F40A2BC7412B4166D40F962845DC1343F402C3FB4C000176D4028E9C5F0423B3F401284D0744D176D40A1D5E134C4413F401D0EBA2E9A176D40D1E73C2845483F40695E61EEE6176D404BA23CC9C54E3F408DF4B6B333186D40A54F471646553F40D94EAB7E80186D408707C40DC65B3F4089EA2E4FCD186D4053B31AAE45623F40EE4332251A196D401B13B4F5C4683F40B0D6A50067196D4068C2F9E2436F3F40FC1D7AE1B3196D40FD3C5674C2753F40B2949FC7001A6D40A6E334A8407C3F40A2B506B34D1A6D40D500027DBE823F40BEFB9FA39A1A6D4076CD2AF13B893F4045E25B99E71A6D407D751D03B98F3F4006E52A94341B6D40A11C49B135963F407E80FD93811B6D40DDE21DFAB19C3F402332C498CE1B6D4015E90CDC2DA33F4083786FA21B1C6D40A4558855A9A93F4081D3EFB0681C6D40CD58036524B03F4089C435C4B51C6D403831F2089FB63F40B9CE31DC021D6D407930CA3F19BD3F402277D4F84F1D6D405BBF010893C33F40EB440E1A9D1D6D40556210600CCA3F408FC1CF3FEA1D6D40D0BD6E4685D03F400679096A371E6D40959A96B9FDD63F40FFF9AB98841E6D40DDE902B875DD3F400BD6A7CBD11E6D40C3C92F40EDE33F40D4A1ED021F1F6D404F899A5064EA3F4049F56D3E6C1F6D40C3ACC1E7DAF03F40CF6B197EB91F6D408BF1240451F73F407AA4E0C106206D40805245A4C6FD3F403442B40954206D40EC8552E31D024040F1EB8455A1206D4099CFE33458054040E14C43A5EE206D40CAEB1846920840409E14E0F83B216D40A4E63416CC0B404057F74B5089216D40E5747BA4050F404011AE77ABD6216D40CEF530F03E12404073846070ED216D401858BB0FAF1240408E6214BAE6226D4098C5AD8FAA114040891069ACDE236D40515EBDB1A6104040EB66A784D6246D40CB17D82AA20F404048AE8742CE256D40D79148FB9C0E4040B647C2E5C5266D40C77E5A23970D4040C1AD0F6EBD276D40C1A25AA3900C4040647528DBB4286D40FCD2967B890B4040F54EC52CAC296D40FCF45DAC810A40401F079F62A32A6D40CAFDFF3579094040D3876E7C9A2B6D402FF1CD187008404039D9EC79912C6D40C1E0195566074040A222D35A882D6D4029EB36EB5B0640407FABDA1E7F2E6D40233B79DB5005404044DCBCC5752F6D40AB06362645044040693F334F6C306D40F38DC3CB380340404F82F7BA62316D40831A79CC2B0240403076C30859326D4033FEAE281E0140400A1151384F336D401D92BEE00F0040409B6E5A4945346D405D6B04EA01FE3F403AD1993B3B356D40EF9AAACBE2FB3F40CBA2C90E31366D4052842867C2F93F40B275A4C226376D40E6FE38BDA0F73F40A905E5561C386D40BEE598CE7DF53F40BF3846CB11396D405D15079C59F33F402E20831F073A6D404169442634F13F4050F95653FC3A6D4087B9136E0DEF3F40782E7D66F13B6D4079D83974E5EC3F40DF57B158E63C6D4007907D39BCEA3F40893CAF29DB3D6D40309FA7BE91E83F4020D332D9CF3E6D4069B7820466E63F40DD42F866C43F6D400B7ADB0B39E43F4064E4BBD2B8406D40907580D50AE23F40A8423A1CAD416D40DE224262DBDF3F40BB1B3043A1426D4068E2F2B2AADD3F40C1615A4795436D408CF966C878DB3F40B83B762889446D407D8F74A345D93F405C0641E67C456D407FAAF34411D73F40F954788070466D40E52CBEADDBD43F404CF2D9F663476D400ED2AFDEA4D23F404CE1234957486D40642BA6D86CD03F400D5E14774A496D40409D809C33CE3F4080DE69803D4A6D40D05B202BF9CB3F405C13E364304B6D40DF676885BDC93F40D9E83E24234C6D40BD8B3DAC80C73F408B873CBE154D6D40DE5786A042C53F4023559B32084E6D40B41F2B6303C33F4050F51A81FA4E6D403DF615F5C2C03F406F4A7BA9EC4F6D40C6AA325781BE3F405D767CABDE506D4062C56E8A3EBC3F4047DBDE86D0516D409B83B98FFAB93F40621C633BC2526D40F2D40368B5B73F40172A6BCEE2526D40D9EE6B8E14B73F4043F4C9E190526D4039E8DB1BA9B03F407307C75D3F526D4021C4905845AA3F40429E1BDAED516D40996A2719E1A33F40308ED8569C516D4020B5125F7C9D3F40A3B80ED44A516D40B3BDC62B17973F40B50ACF51F9506D40E5DAB880B1903F40FD7C2AD0A7506D401D9C5F5F4B8A3F406B13324F56506D409AC532C9E4833F40FEDCF6CE04506D40824CABBF7D7D3F40AFF3894FB34F6D40DD52434416773F40217CFCD0614F6D4097237658AE703F407CA55F53104F6D407F2EC0FD456A3F4038A9C4D6BE4E6D4006049F35DD633F40E6CA3C5B6D4E6D4048519101745D3F40FB57D9E01B4E6D40BDDB16630A573F4099A7AB67CA4D6D401F7DB05BA0503F405E1AC5EF784D6D402C1FE0EC354A3F402B1A3779274D6D404CB72818CB433F40EC191304D64C6D404A420EDF5F3D3F4065956A90844C6D4019C01543F4363F40F4104F1E334C6D40512FC54588303F406719D2ADE14B6D40E488A3E81B2A3F40B743053F904B6D4097BB382DAF233F40D52CFAD13E4B6D409FA70D15421D3F406D79C266ED4A6D401C1AACA1D4163F40BED56FFD9B4A6D4093C89ED466103F4047F513964A4A6D40734C71AFF8093F40A392C030F9496D404C1EB0338A033F40486F87CDA7496D407691E8621BFD3E404E537A6C56496D4043CFA83EACF63E40300DAB0D05496D4069D27FC83CF03E4099712BB1B3486D403F62FD01CDE93E40235B0D5762486D401F0EB2EC5CE33E402AAA62FF10486D4097282F8AECDC3E407C443DAABF476D409FC206DC7BD63E402A15AF576E476D40ECA6CBE30AD03E404E0CCA071D476D40FE5411A399C93E40CF1EA0BACB466D4068FC6B1B28C33E401B4643707A466D40D877704EB6BC3E40F87FC52829466D405248B43D44B63E4040CE38E4D7456D403090CDEAD1AF3E40A836AFA286456D404F0E53575FA93E407FC23A6435456D400719DC84ECA23E40777EED28E4446D4042990075799C3E40677AD9F092446D408205592906963E4009C910BC41446D40DB5C7EA3928F3E40C07FA58AF0436D40F9210AE51E893E405FB6A95C9F436D40065696EFAA823E40DD862F324E436D40B973BDC4367C3E40310D490BFD426D40316A1A66C2753E40F86608E8AB426D40E69748D54D6F3E40D0CFB8A595426D40F9B47DDA6D6E3E40F8ECB363A6416D4038243861A4703E40");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211980977M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EN0211980977M,EN0211937486M,5.638,2.75291003,2.88508848,0.11020450,0.13361618,1.02747851,1.35342105,138.51835499,212.80997855,5.66493533,2.69477493,2.81104513,0.43267388,0.35392301,0.63370535,0.77567094,0.64866114,0.70112853,0.78090539,0.73453337,2011112,2011112,237.58479307,33.89954131,237.27112110,34.02234270,2439400,2439400,237.48900314,34.03556586,4454.51760935,240.40843306,33.11557283,4503.73620832,237.43296305,33.95025399,102.72590404,105.54865922,0.34821403,6.02683787,61.34652231,62.92153966,61.62049731,56.89843215,010300000001000000BA000000B73C883BFB9F6D40A857EC9EECB6404002E15B35209F6D4020927D590CB84040D183651C459E6D4003CB9B862BB94040767AE5F0699D6D400E46F6254ABA404087311CB38E9C6D40EF273D3768BB40402B2C4A63B39B6D40467721BA85BC40401E03B001D89A6D406D1D55AEA2BD4040E4638E8EFC996D4060E78A13BFBE4040F60F260A21996D40928676E9DABF4040DBDBB77445986D40BA91CC2FF6C0404058AE84CE69976D409C8542E610C240408B7FCD178E966D40D5C58E0C2BC340401F58D350B2956D40969D68A244C440405A50D779D6946D405B4088A75DC540405D8F1A93FA936D40A3CAA61B76C6404059417F7BDD936D4079893261C1C640400C93BD9034946D40F9A843F5A7C94040EE25F9418B946D4098C6FDB88ACC404021F601FEE1946D406F85795D6DCF40406392C5C438956D401A15F7E14FD240406A8131968F956D407BE5B64532D5404018423372E6956D40FAA8F98714D84040C04BB8583D966D40ED5600A8F6DA4040610EAE4994966D40DE2D0CA5D8DD4040E9F20145EB966D40F0B55E7EBAE040406C5BA14A42976D4013C339339CE340406DA3795A99976D406577DFC27DE640400F207874F0976D407845922C5FE9404063208A9847986D4098F2946F40EC40409BED9CC69E986D400C992A8B21EF404052CB9DFEF5986D4061AA967E02F24040BFF779404D996D40A4F11C49E3F4404007AC1E8CA4996D409D9501EAC3F74040661C79E1FB996D40141B8960A4FA404078787640539A6D40F966F8AB84FD404083EB03A9AA9A6D40A7C094CB64004140A29C0E1B029B6D4008D4A3BE440341400EAF8396599B6D40CBB36B84240641406442501BB19B6D4092DB321C04094140CD7261A9089C6D4006324085E30B41405E59A440609C6D40170BDBBEC20E41403A0C06E1B79C6D400A2A4BC8A1114140DB9E738A0F9D6D4098C3D8A0801441405722DA3C679D6D400F80CC475F17414094A526F8BE9D6D405F7D6FBC3D1A41408E3546BC169E6D4038510BFE1B1D414092DD25896E9E6D40070BEA0BFA1F41407FA7B25EC69E6D40123656E5D7224140009CD93C1E9F6D4078DB9A89B5254140CFC28723769F6D40368403F892284140EC22AA12CE9F6D401F3BDC2F702B4140E3C22D0A26A06D40DB8E71304D2E41400AA9FF097EA06D40EE9310F929314140B1DB0C12D6A06D408AE6068906344140726142222EA16D409BACA2DFE236414063418D3A86A16D40A49732FCBE3941405383DA5ADEA16D40A8E605DE9A3C41400C30178336A26D400A686C84763F41408D5130B38EA26D40717BB6EE5142414047F312EBE6A26D409513351C2D4541405822AC2A3FA36D401EB8390C08484140CAEDE87197A36D40728716BEE24A4140CC66B6C0EFA36D407C381E31BD4D4140F1A0011748A46D407C1CA4649750414067B2B774A0A46D40B820FC577153414037B4C5D9F8A46D4053D07A0A4B5641407DC2184651A56D40E855757B245941403C1D9AA168A56D40CF58711085594140C4B9CAF849A66D407E69D199665841403D79BC1E2AA76D40611748F048574140133937320AA86D40F69FAAA72A5641404923F732EAA86D40C57642C00B554140FE75B820CAA96D4006115A3AEC534140518437FBA9AA6D40DEE43C16CC5241404BB730C289AB6D40A8683754AB514140BC8E607569AC6D40301297F48950414020A2831449AD6D40E155AAF7674F414082A1569F28AE6D40F4A5C05D454E41405A56961508AF6D409C712A27224D414075A4FF76E7AF6D4027243954FE4B4140D58A4FC3C6B06D4015243FE5D94A41408D2443FAA5B16D4033D28FDAB4494140A3A9971B85B26D40AA887F348F484140F46F0A2764B36D40069A63F3684741400DEC581C43B46D4040509217424641400AB240FB21B56D40AFEB62A11A4541407C767FC300B66D4016A22D91F2434140350FD374DFB66D407E9D4BE7C94241403774F90EBEB76D402EFB16A4A041414084C0B0919CB86D4097CAEAC776404140FD32B7FC7AB96D40280C23534C3F4140392FCB4F59BA6D4039B01C46213E4140603EAB8A37BB6D40CC9535A1F53C4140051016AD15BC6D407189CC64C93B4140F57ACAB6F3BC6D40024441919C3A41401B7E87A7D1BD6D406F69F4266F39414049410C7FAFBE6D4080874726413841401216183D8DBF6D4081149D8F123741409F786AE16AC06D40056E5863E33541407C10C36B48C16D408ED7DDA1B33441406FB1E1DB25C26D403379924B83334140435C863103C36D40515EDC6052324140993F716CE0C36D401C7422E220314140B7B8628CBDC46D404088CCCFEE2F414059541B919AC56D407947432ABC2E41406ECF5B7A77C66D40163CF0F1882D4140F317E54754C76D4090CC3D27552C4140C65E0963C9C76D403B563287B12B4140BC05C184ABC76D407647476AB62A414001D8174B52C76D408605AFFDC7274140A4E5F212F9C66D40E8A7F656D92441400E6765DC9FC66D40BE7FCC76EA2141403EA082A746C66D405F72DF5DFB1E414096E05D74EDC56D4092F7DE0C0C1C41409B820A4394C56D40AD177B841C194140C1EB9B133BC56D40CB6964C52C1641402D8C25E6E1C46D40ED114CD03C13414081DEBABA88C46D4018BFE3A54C10414098676F912FC46D4075A9DD465C0D414053B6566AD6C36D406890ECB36B0A4140536384457DC36D40ABB8C3ED7A074140CF100C2324C36D404DEA16F589044140416A0103CBC26D40CF6E9ACA98014140402478E571C26D40290F036FA7FE40402DFC83CA18C26D40C01106E3B5FB40400CB838B2BFC16D4072385927C4F840403126AA9C66C16D409BBEB23CD2F54040111DEC890DC16D40F356C923E0F24040FA7A127AB4C06D40962954DDEDEF4040DC25316D5BC06D40F3D10A6AFBEC4040070B5C6302C06D40B15CA5CA08EA4040E91EA75CA9BF6D409B45DCFF15E74040CE5C265950BF6D408D75680A23E44040A9C6ED58F7BE6D40494003EB2FE14040C764115C9EBE6D40656266A23CDE40409645A56245BE6D4015FF4B3149DB4040677DBD6CECBD6D40159E6E9855D840401F266E7A93BD6D406E2989D861D540400B5FCB8B3ABD6D4056EB56F26DD240408B4CE9A0E1BC6D40F08B93E679CF4040DC17DCB988BC6D40270FFBB585CC4040CEEEB7D62FBC6D406CD2496191C940408A0391F7D6BB6D407B8A3CE99CC64040488C7B1C7EBB6D402441904EA8C3404013C38B4525BB6D400E530292B3C040407DE5D572CCBA6D40666D50B4BEBD404068346EA473BA6D40A88B38B6C9BA4040BBF368DA1ABA6D4051F57898D4B740401A6ADA14C2B96D409B3BD05BDFB44040B1E0D65369B96D402437FD00EAB14040E4A2729710B96D40AE05BF88F4AE40400CFEC1DFB7B86D40C007D5F3FEAB40403C41D92C5FB86D4064DEFE4209A94040F3BCCC7E06B86D40BB68FC7613A64040DBC2B0D5ADB76D40C0C18D901DA3404085A5993155B76D40D03D739027A040402BB89B92FCB66D4072686D77319D40405F4ECBF8A3B66D40DC013D463B9A40408CBDAC258DB66D404DDD2531D6994040A180EF00ABB56D409E5109EC019B4040F1072066CAB46D40D7E633F02A9C40403282A5A6E9B36D40885CD978539D4040A566BAC208B36D4077AA85857B9E4040116999BA27B26D407B76C515A39F404015797D8E46B16D4023162629CAA040405EC1A13E65B06D40449035BFF0A14040FCA641CB83AF6D40AD9E82D716A34040ADC89834A2AE6D40B2AF9C713CA440400EFEE27AC0AD6D40BDE7138D61A54040EA565C9EDEAC6D40F222792986A640407E1A419FFCAB6D40A2F65D46AAA74040AEC6CD7D1AAB6D40E3B254E3CDA840404C0F3F3A38AA6D401364F0FFF0A940404FDDD1D455A96D4055D4C49B13AB4040134EC34D73A86D400A8D66B635AC404090B250A590A76D404CD86A4F57AD4040978EB7DBADA66D406AC2676678AE4040009835F1CAA56D403C1BF4FA98AF4040EBB508E6E7A46D40AA77A70CB9B04040F1FF6EBA04A46D4000331A9BD8B1404055BDA66E21A36D405670E5A5F7B240403864EE023EA26D40E41BA32C16B44040D19884775AA16D4068ECED2E34B54040932CA8CC76A06D40646461AC51B64040E4F855C100A06D40AA767059E5B64040B73C883BFB9F6D40A857EC9EECB64040");
  compareCsvLine(line.getRow(2),"EN0211980977M,EN0211937446M,5.42208,2.76013890,2.66194563,0.11129868,0.12978433,1.05835058,2.08338634,137.83750706,187.76232703,6.60576990,2.69477493,2.82550286,0.43267388,0.36330003,0.63370535,0.77728724,0.64866114,0.70393225,0.11615944,0.10433298,2011112,2011112,237.58479307,33.89954131,237.01706205,32.87939564,2439400,2439400,237.48900314,34.03556586,4454.51760935,240.21066833,31.98067927,4565.71325635,237.34322240,33.39292779,102.72590404,108.72002099,0.34821403,6.09148965,61.34652231,62.73399929,61.62049731,56.64609734,010300000001000000710000007F517357B7BD6D402428C88A448E4040A1E3837DDFBC6D40C460E8BB768F40404F37518807BC6D40ECCDA76CA8904040067016782FBB6D409AC4999CD991404060E40E4D57BA6D40DA4E524B0A9340405C1D76077FB96D40552D66783A944040ADD587A7A6B86D40DBD86A236A954040EAF87F2DCEB76D40E683F64B99964040EDA29A99F5B66D40291CA0F1C7974040F81E14EC1CB66D40014CFF13F698404009E7282544B56D40F07BACB2239A404012A315456BB46D401ED440CD509B40403728174C92B36D40B93D56637D9C404012786A3AB9B26D4075648774A99D4040E0BF4C10E0B16D40E3B76F00D59E4040C757FBCD06B16D40EA6CAB0600A0404011C2B3732DB06D400C7FD7862AA140405EAAB30154AF6D40DAB1918054A24040D9E438787AAE6D403C9278F37DA34040796D81D7A0AD6D40CB772BDFA6A440402C67CB1FC7AC6D4011864A43CFA54040111B5551EDAB6D40E3AD761FF7A64040A6F75C6C13AB6D409EAE51731EA840400290217139AA6D405D177E3E45A94040FE9AE15F5FA96D4040489F806BAA404072F2DB3885A86D409A73593991AB404050924FFCAAA76D401E9F5168B6AC4040EB977BAAD0A66D4011A52D0DDBAD404016419F43F6A56D406E359427FFAE404053EBF9C71BA56D4004D72CB722B040400213CB3741A46D4094E89FBB45B140409252529366A36D40E5A1963468B24040A161CFDA8BA26D40DA14BB218AB340403414820EB1A16D40792EB882ABB44040D059AA2ED6A06D40F3B73957CCB54040B73C883BFB9F6D40A857EC9EECB6404002E15B35209F6D4020927D590CB84040D183651C459E6D4003CB9B862BB94040767AE5F0699D6D400E46F6254ABA404087311CB38E9C6D40EF273D3768BB40402B2C4A63B39B6D40467721BA85BC40401E03B001D89A6D406D1D55AEA2BD4040E4638E8EFC996D4060E78A13BFBE4040F60F260A21996D40928676E9DABF4040DBDBB77445986D40BA91CC2FF6C0404058AE84CE69976D409C8542E610C240408B7FCD178E966D40D5C58E0C2BC340401F58D350B2956D40969D68A244C440405A50D779D6946D405B4088A75DC540405D8F1A93FA936D40A3CAA61B76C6404059417F7BDD936D4079893261C1C640400C93BD9034946D40F9A843F5A7C94040EE25F9418B946D4098C6FDB88ACC404021F601FEE1946D406F85795D6DCF40406392C5C438956D401A15F7E14FD240406A8131968F956D407BE5B64532D5404080BF0D2AB2956D40B9AED42958D640402420167BF5956D40480CF0C009D64040177410E1E4966D40069D6C26F2D44040971A632AD4976D4007CF2EECD9D3404067AFC856C3986D400B868B12C1D24040B5EDFB65B2996D40F4A9D899A7D1404003B1B757A19A6D40DD256D828DD040401BF6B62B909B6D4038E7A0CC72CF4040EBDBB4E17E9C6D40CEDCCC7857CE40407FA46C796D9D6D40CDF54A873BCD4040E3B599F25B9E6D40D12076F81ECC40400B9BF74C4A9F6D40CD4AAACC01CB4040BC04428838A06D400F5E4404E4C9404075CA34A426A16D402941A29FC5C8404054EB8BA014A26D40D4D5229FA6C74040FC8E037D02A36D40DFF7250387C6404077065839F0A36D40007C0CCC66C5404026CD45D5DDA46D40B32E38FA45C440408B898950CBA56D4003D30B8E24C34040460EE0AAB8A66D405F21EB8702C24040ED5A06E4A5A76D404FC63AE8DFC04040DE9CB9FB92A86D40466160AFBCBF40403230B7F17FA96D404D83C2DD98BE404091A0BCC56CAA6D40BBADC87374BD40400CAA877759AB6D40E850DB714FBC4040043AD60646AC6D40CECA63D829BB4040F46F667332AD6D40AF65CCA703BA40405A9EF6BC1EAE6D40B45680E0DCB84040844B45E30AAF6D4080BCEB82B5B740406E3211E6F6AF6D40C79D7B8F8DB64040924319C5E2B06D40D8E79D0665B54040BFA51C80CEB16D40286DC1E83BB44040EBB6DA16BAB26D40D3E3553612B34040020D1389A5B36D4011E4CBEFE7B14040BE7685D690B46D40BEE69415BDB040406DFCF1FE7BB56D40C04323A891AF4040BEE0180267B66D407930EAA765AE404093A1BADF51B76D402FBE5D1539AD4040C8F897973CB86D4076D8F2F00BAC4040FBDC712927B96D4085431F3BDEAA40405282099511BA6D40959A59F4AFA940403F5B20DAFBBA6D40394E191D81A84040511978F8E5BB6D40A8A2D6B551A74040DDADD2EFCFBC6D4014AE0ABF21A64040DA4AF2BFB9BD6D40E6562F39F1A440408C639968A3BE6D400A52BF24C0A3404049AD8AE98CBF6D402B2136828EA240403620894276C06D40E41010525CA14040DC10194D95C06D4091B8FE690BA14040EB292E253DC06D40AF06212BFF9D404006D18E6EE5BF6D40490B0091F69A4040A9B005B98DBF6D4051D9B5BAED974040C755A50436BF6D4004C1F4A8E494404041598051DEBE6D40A0AC6F5CDB914040C25FA99F86BE6D40931EDAD5D18E404093ACB4425CBE6D4087A2D0195A8D40407F517357B7BD6D402428C88A448E4040");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EN0211981012M.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EN0211981012M,EN0211937488M,5.73996,2.78385138,2.95610443,0.10840106,0.11358389,1.05147872,1.95165838,170.18670945,183.49119548,6.54015009,2.77756960,2.79013316,0.47462970,0.41107532,0.69276657,0.71290684,0.63099985,0.68590471,0.26022329,0.23756384,2011112,2011112,238.99038636,34.52754626,238.58356334,33.67370765,2439400,2439400,237.67949104,35.07673579,4400.48853663,240.41861335,33.17318666,4500.63711629,238.78716235,34.11459297,100.03265745,105.18221135,2.71066287,3.51627363,60.50740585,61.77849353,63.21001274,58.26857601,01030000000100000083000000A9AF5CD2A3EA6D40AE4BE2B925DF4040940546DDD1E96D40FD98CE0962E04040A3A753CFFFE86D40CF6D73DA9DE140403F9BBFA82DE86D409A45642BD9E240402B17C4695BE76D40954C35FC13E44040D7829B1289E66D4065617B4C4EE54040A17580A3B6E56D409216CC1B88E6404022B6AD1CE4E46D4020B4BD69C1E7404078395E7E11E46D400939E735FAE840408822CDC83EE36D40B95CE07F32EA404044C135FC6BE26D409F9041476AEB4040EE91D31899E16D408B01A48BA1EC4040613CE21EC6E06D403299A14CD8ED404048939D0EF3DF6D4099FFD4890EEF4040669341E81FDF6D40819CD94244F04040D2620AAC4CDE6D40CB984B7779F140403B50345A79DD6D40DDDFC726AEF2404013D2FBF2A5DC6D400221EC50E2F34040E7859D76D2DB6D40C5D056F515F54040812F56E5FEDA6D403C2AA71349F6404030B8623F2BDA6D405F307DAB7BF74040FC2D008557D96D4057AF79BCADF84040DCC26BB683D86D40C53D3E46DFF94040F5CBE2D3AFD76D40FE3D6D4810FB4040C9C0A2DDDBD66D404BDFA9C240FC40406E3AE9D307D66D40221F98B470FD4040C9F2F3B633D56D4061CADC1DA0FE4040B0C300875FD46D407A7E1DFECEFF404037A64D448BD36D408BAA0055FD004140CFB118EFB6D26D40A2902D222B0241407B1BA087E2D16D40C3464C65580341400A35220E0ED16D4017B8051E850441403A6CDD8239D06D40EFA5034CB1054140F34910E664CF6D40E7A8F0EEDC0641407071F93790CE6D40E3317806080841406D9FD778BBCD6D40208B4692320941405BA9E9A8E6CC6D402BD908925C0A4140847C6EC811CC6D40E81B6D05860B4140461DA5D73CCB6D407C2F22ECAE0C41402FA6CCD667CA6D404FCDD745D70D4140334724C692C96D40E68C3E12FF0E4140D644EBA5BDC86D40D8E40751261041405DF76076E8C76D40A22BE6014D114140EAC9C43713C76D408F988C2473124140B63956EA3DC66D408544AFB89813414034D5548E68C56D40DC2A03BEBD1441403C3B002493C46D40202A3E34E2154140391A98ABBDC36D40E804171B061741404B2F5C25E8C26D40826245722918414078458C9112C26D40B7CF81394C194140D43468F03CC16D4082BF85706E1A4140C12436F020C16D40CE0803FCB91A4140A3CDD75E7BC16D4062A5B8F4851D41403B3E5E66D5C16D402E4DD23D4E204140B2ADDB792FC26D4013928B661623414035A53C9989C26D4008FD296EDE254140CCA46DC4E3C26D409557F353A62841409F235BFB3DC36D4022AE2D176E2B41402B90F13D98C36D4039521FB7352E414093501D8CF2C36D40C1DC0E33FD304140DEC2CAE54CC46D404330438AC4334140353DE64AA7C46D40297B03BC8B364140210E5CBB01C56D40ED3997C752394140DC7C18375CC56D405C3946AC193C41403FDD15398FC56D40B4CF0C24AA3D4140F5A97B9D98C56D4071F5BA839E3D41407B5E8B5A7FC66D40D3031028803C414056E364FF65C76D4051E4AA2E613B414006B4C38B4CC86D404B6CDB97413A41409E6563FF32C96D403A73F26321394140BAA7FF5919CA6D40D4D14193003841406245549BFFCA6D403D611C26DF364140F4251DC3E5CB6D4036FAD51CBD354140024E16D1CBCC6D403474C3779A34414044E0FBC4B1CD6D407EA43A3777334140751E8A9E97CE6D404A5D925B53324140356A7D5D7DCF6D40BE6C22E52E314140FE45920163D06D40009C43D409304140F555858A48D16D403BAE4F29E42E4140D56013F82DD26D40975FA1E4BD2D4140CF50F94913D36D4033649406972C41407234F47FF8D36D400967858F6F2B4140803FC199DDD46D40F508D27F472A4140DCCB1D97C2D56D407DDFD8D71E2941405D5AC777A7D66D40BA73F997F5274140B7937B3B8CD76D40364194C0CB2641404B49F8E170D86D40BEB40A52A12541401576FB6A55D96D40232BBF4C762441407A3F43D639DA6D4016F014B14A23414023F68D231EDB6D40DD3C707F1E224140E2169A5202DC6D400F3736B8F12041407E4B2663E6DC6D4056EFCC5BC41F4140906BF154CADD6D400E609B6A961E41405D7DBA27AEDE6D40086C09E5671D4140A7B640DB91DF6D401ADD7FCB381C4140807D436F75E06D40D562681E091B4140206982E358E16D400D912DDED8194140B642BD373CE26D407FDE3A0BA81841403B06B46B1FE36D4059A3FCA57617414039E3267F02E46D40CC17E0AE44164140A63DD671E5E46D408D52532612154140A8AE8243C8E56D405B47C50CDF1341405F05EDF3AAE66D407CC5A562AB124140B747D6828DE76D40337665287711414029B3FFEF6FE86D4032DB755E4210414087BD2A3B52E96D400B4D49050D0F4140C415196434EA6D4099F9521DD70D4140ACA48C6A16EB6D405EE206A7A00C4140B78D474EF8EB6D40EEDAD9A2690B4140C42F0C0FDAEC6D403B874111320A4140CE259DACBBED6D40015AB4F2F9084140C247BD269DEE6D400893A947C107414022AB2F7D7EEF6D40693D991088064140D3A3B7AF5FF06D40F92DFC4D4E054140F7D7DA9AB7F06D4008BC8385D3044140783C41255AF06D40F48F1C9E1B024140DC3BDA7FFBEF6D4033BAB7805AFF4040F75288E29CEF6D40EAD6B43F99FC4040FC9B614D3EEF6D40739DC8DBD7F9404042307CC0DFEE6D40960FA85516F74040EF27EE3B81EE6D40537708AE54F44040B099CDBF22EE6D40B2649FE592F140407D9A304CC4ED6D40A1AB22FDD0EE40403B3D2DE165ED6D40B56148F50EEC40407C92D97E07ED6D40FADBC6CE4CE9404034A84B25A9EC6D40BCAC548A8AE64040748999D44AEC6D405EA1A828C8E34040123ED98CECEB6D4005C079AA05E140406BCA204E8EEB6D4073457F1043DE4040F980334177EB6D404FFB5B8AE6DD4040A9AF5CD2A3EA6D40AE4BE2B925DF4040");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0211937827G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 6);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EW0211937827G,EW0227134017G,5.9907,2.68114177,3.30956128,0.19074518,0.03259805,1.29044551,0.87593032,43.99252746,368.97914874,8.51524989,2.57592477,2.78635877,0.33276355,0.33963336,0.62939845,0.89801483,0.62964266,0.55163825,0.56770771,0.94568547,2011112,2011288,242.63800548,44.13232562,242.30612536,43.16336292,2439400,2439400,242.69024983,44.18930803,3983.69951235,241.33740092,46.68650420,3621.56165839,242.40572728,43.23362644,276.22354284,214.05285230,0.17593827,10.92059326,63.34472885,63.20733264,63.33215970,70.35255711,0103000000010000007C010000D2BC2F420C7B6E4091903EC25DE74340CFA86B90AD786E4087DB368E88EB434044093E6E4E766E40D4287129B1EF434032FFA6DAEE736E4029B25D95D7F343409880A6D48E716E40155568D3FBF74340B4553C5B2E6F6E402C99F8E41DFC43401A16686DCD6C6E4017B671CB3D004440DC25290A6C6A6E40AE9932885B044440AEB27E300A686E40C1ED951C770844401AB167DFA7656E40E71DF289900C444098D9E21545636E40295D99D1A7104440C5A5EED2E1606E408CABD9F4BC144440854D89157E5E6E4089DBFCF4CF1844402FC4B0DC195C6E406A9748D3E01C4440B5B56227B5596E407866FE90EF204440D9839CF44F576E4039B25B2FFC24444047435B43EA546E405ECB99AF06294440D3B89B1284526E40D1EEED120F2D44408F565A611D506E406E4A895A153144400C39932EB64D6E40DC01998719354440752442794E4B6E401833469B1B394440B9816240E6486E4019FBB5961B3D4440C05BEF827D466E402E7A097B194144408C5CE33F14446E4060D85D491545444062CA3876AA416E40B649CC020F494440F984E924403F6E405B126AA8064D44409502EF4AD53C6E40AC8A483BFC5044403B4D42E7693A6E40392375BCEF544440CBFFDBF8FD376E409B68F92CE15844402E43B47E91356E404C07DB8DD05C444074CBC27724336E404CCF1BE0BD604440F8D4FEE2B6306E40C4B7B924A96444407B215FBF482E6E4085E2AE5C926844404FF5D90BDA2B6E40789FF188796C44406E1465C76A296E40F56F74AA5E70444094BFF5F0FA266E40FB0926C2417444405FB180878A246E40635BF1D022784440601BFA8919226E40EE8CBDD7017C444045A355F7A71F6E4034056ED7DE7F4440D35F86CE351D6E40A26BE2D0B98344400BD67E0EC31A6E4038ABF6C4928744403DF630B64F186E4040F582B4698B44400B198EC4DB156E4000C45BA03E8F44407AFC863867136E403EDD51891193444002C10B11F2106E40B0543270E29644408EE60B4D7C0E6E40718EC655B19A4440834976EB050C6E403A41D43A7E9E4440C11F39EB8E096E40AB781D2049A24440A9F5414B17076E405D97600612A6444013AB7D0A9F046E40F25858EED8A944404570D82726026E4015D4BBD89DAD4440F4C23DA2ACFF6D404E7C3EC660B144402E6B987832FD6D40DB2390B721B544404778D2A9B7FA6D405CFD5CADE0B84440D63DD5343CF86D40799D4DA89DBC44408D508918C0F56D4078FC06A958C044402683D65343F36D40A6772AB011C4444043E3A3E5C5F06D40CCD255BEC8C7444053B6D7CC47EE6D40613923D47DCB44405B765708C9EB6D40E43F29F230CF4440E6CE079749E96D40F3E4FA18E2D24440BA99CC77C9E66D405192274991D64440C3DB88A948E46D40ED1D3B833EDA4440BEC11E2BC7E16D40CCCABDC7E9DD4440219D6FFB44DF6D40C549341793E14440BAE05B19C2DC6D405BBA1F723AE54440851DC3833EDA6D404BABFDD8DFE844404FFF8339BAD76D40341B484C83EC44407F497C3935D56D40FF7875CC24F04440AAD38882AFD26D405DA4F859C4F344405586851329D06D4012EE40F561F7444082574DEBA1CD6D403618BA9EFDFA44406347BA081ACB6D405F56CC5697FE4440DD5CA56A91C86D40BD4DDC1D2F0245402CA2E60F08C66D400F154BF4C4054540712155F77DC36D40A03476DA5809454027E1C61FF3C06D400FA6B7D0EA0C4540BCE0108867BE6D401AD465D77A1045400215072FDBBB6D404F9AD3EE08144540A0647C134EB96D4095445017951745405C73A413D2B66D40698DF58B061B454064B52A8187B76D40CE571F9C5C1F45408C2C5E3283B86D406EDA1E095E254540FFE5F0087FB96D40192F1FCE5D2B4540F91158057BBA6D4016CBB7ED5B314540E3D6082877BB6D40161C7C6A58374540B252787173BC6D408592FB46533D4540379C1BE26FBD6D40C9ABC1854C43454068C4677A6CBE6D4064FC552944494540C0D7D13A69BF6D40EC393C343A4F454075DFCE2366C06D40ED44F4A82E554540E2E2D33563C16D40B332FA89215B4540BAE8557160C26D40E756C6D9126145405BF8C9D65DC36D402A4DCD9A026745401E1BA5665BC46D40760280CFF06C45408B5D5C2159C56D407ABE4B7ADD724540B6D0640757C66D40D72C9A9DC87845406D8B331955C76D403566D13BB27E45408EAB3D5753C86D4054F953579A8445403F57F8C151C96D40E8F380F2808A45402EBED85950CA6D407FEBB30F66904540D91A541F4FCB6D40230645B149964540C2B3DF124ECC6D400D0389D92B9C4540BCDCF0344DCD6D401C43D18A0CA2454017F8FC854CCE6D4054D16BC7EBA74540E87779064CCF6D402A6BA391C9AD45403DDFDBB64BD06D40C988BFEBA5B345405DC399974BD16D40406504D880B94540FECC28A94BD26D408206B3585ABF454080B9FEEB4BD36D407E45097032C54540205C91604CD46D40ECD5412009CB45402F9F56074DD56D402C4E946BDED045405885C4E04DD66D40F92E3554B2D64540BF2A51ED4ED76D4014EB55DC84DC454046C6722D50D86D40D2EE240656E24540B5AA9FA151D96D4092A7CDD325E8454002484E4A53DA6D40398B7847F4ED4540712CF52755DB6D40651F4B63C1F34540CD050B3B57DC6D40C50068298DF94540A1A2068459DD6D4047EAEE9B57FF454061F35E035CDE6D4022BCFCBC20054640A10B8BB95EDF6D40E482AB8EE80A46404E2302A761E06D40617E1213AF104640CB973BCC64E16D409C28464C7416464035EDAE2968E26D40913C583C381C464091CFD3BF6BE36D40EEBC57E5FA214640EF13228F6FE46D40BBFA5049BC274640AAB9119873E56D400A9C4D6A7C2D464095EB1ADB77E66D405EA2544A3B3346401F01B6587CE76D403A716AEBF8384640977F5B1181E86D407BD4904FB53E4640491B840586E96D40BA06C77870444640BBB8A8358BEA6D4081B709692A4A4640DB6D42A290EB6D408B115322E34F46402383CA4B96EC6D40E6C09AA69A554640DA74BA329CED6D4009F9D5F7505B46403DF48B57A2EE6D40DC7AF71706614640AAE8B8BAA8EF6D40A69AEF08BA664640DA70BB5CAFF06D400246ACCC6C6C46400CE40D3EB6F16D40AC0919651E72464032D32A5FBDF26D404E171FD4CE7746402E0A8DC0C4F36D403F4BA51B7E7D4640FC90AF62CCF46D402832903D2C834640DFAC0D46D4F56D40B20EC23BD98846409CE1226BDCF66D40FFDE1A18858E4640A9F26AD2E4F76D40456278D42F94464061E4617CEDF86D40331EB672D999464030FD8369F6F96D406464ADF4819F4640D4C64D9AFFFA6D40AE57355C29A54640840F3C0F09FC6D407DF122ABCFAA464030EBCBC812FD6D400A0749E374B04640ABB47AC71CFE6D408E4E780619B64640E90EC60B27FF6D4072647F16BCBB464038E62B9631006E4063D02A155EC146406E712A673C016E40620A4504FFC646402433407F47026E40C97F96E59ECC4640F2FAEBDE52036E403F98E5BA3DD24640A1E6AC865E046E40A0BAF685DBD74640756302776A056E40E2518C4878DD4640512F6CB076066E40E8D1660414E34640FF596A3383076E4045BC44BBAEE8464075467D0090086E40FAA4E26E48EE4640FDAB25189D096E402F37FB20E1F346408697E47AAA0A6E40D23947D378F94640D76C3B29B80B6E4036947D870FFF4640D6E7AB23C60C6E40A352533FA5044740CB1DB86AD40D6E40E1AA7BFC390A4740937EE2FEE20E6E40B200A8C0CD0F4740FBD5ADE0F10F6E4042EA878D60154740EA4C9D1001116E408F34C964F21A4740BC6A348F10126E40D0E71748832047408016F75C20136E40BD4B1E39132647403498697A30146E40E3EB8439A22B4740289A10E840156E40D99BF24A30314740302A71A651166E40877B0C6FBD364740F4BA10B662176E404BFB75A7493C47404B25751774186E4019E0D0F5D44147407AA924CB85196E409F47BD5B5F47474081F0A5D1971A6E4056ACD9DAE84C4740B74643A5081B6E40D9448EBFC04E47401C8EBFBF171D6E405E274B08D04B47406FD6DC821D1F6E407C0621D8EA484740728C8F0023216E40AC7A543F044647406D665F3928236E40767D2E3D1C4347407210D42D2D256E406E48F6D0324047405A2D75DE31276E400153F1F9473D4740DF57CA4B36296E404F5063B75B3A4740AC235B763A2B6E40002D8E086E3747406D1EAF5E3E2D6E40F80CB2EC7E344740E3D04D05422F6E401E490D638E314740E9BFBE6A45316E40126DDC6A9C2E47409A6D898F48336E40CC345A03A92B4740535A35744B356E40548ABF2BB4284740D3054A194E376E40468343E3BD25474042F04E7F50396E40815E1B29C6224740579BCBA6523B6E4095817AFCCC1F47405A8B4790543D6E405B76925CD21C474048484A3C563F6E4064E89248D6194740E35E5BAB57416E406EA2A9BFD8164740CE6102DE58436E40CD8B02C1D91347409DEAC6D459456E40C9A5C74BD9104740FC9A30905A476E40EF08215FD70D4740B71DC7105B496E4072E234FAD30A4740E22712575B4B6E405171271CCF074740F07999635B4D6E40B0031BC4C8044740CFE0E4365B4F6E4000F42FF1C001474008377CD15A516E401AA684A2B7FE4640D565E7335A536E40798435D7ACFB46404966AE5E59556E4037FD5C8EA0F846406D42595258576E40267F13C792F546406216700F57596E40D9766F8083F2464083117B96555B6E40874B85B972EF4640817702E8535D6E40145C677160EC464095A18E04525F6E40E5FB25A74CE946409CFFA7EC4F616E40BA6FCF5937E646404019D7A04D636E408EEA6F8820E34640208FA4214B656E40508A113208E04640FA1B996F48676E40A054BC55EEDC4640D4953D8B45696E40813376F2D2D9464028EF1A75426B6E400CF24207B6D646401038BA2D3F6D6E40F638249397D34640799FA4B53B6F6E40348B199577D046404D74630D38716E407542200C56CD4640AA26803534736E40A68B33F732CA46400749842E30756E405B634C550EC746408091F9F82B776E403B926125E8C34640F3DA699527796E404BA96766C0C0464049265F04237B6E4050FE501797BD4640A89B63461E7D6E40F9A70D376CBA4640A48B015C197F6E402C7A8BC43FB746409270C34514816E401B02B6BE11B44640A8EF33040F836E4075827624E2B046405FDADD9709856E4067EFB3F4B0AD4640902F4C0104876E40A7EA522E7EAA4640D21C0A41FE886E4076BF35D049A74640B3FFA257F88A6E40745E3CD913A446400767A245F28C6E409E594448DCA046401E14940BEC8E6E4003E0281CA39D464025FC03AAE5906E409CB9C253689A464066497E21DF926E40FE42E8ED2B974640985C8F72D8946E40FB686DE9ED93464032CEC39DD1966E4050A42345AE904640BE6FA8A3CA986E4022F5D9FF6C8D4640274DCA84C39A6E408FDE5C182A8A464018AEB641BC9C6E400D62768DE58646405517FBDAB49E6E40E8FAED5D9F834640104C2551ADA06E407B998888578046404A4FC3A4A5A26E40939E080C0E7D4640396563D69DA46E408FD62DE7C27946409E1494E695A66E409B74B518767646403628E4D58DA86E40C50D5A9F277346401BB0E2A485AA6E400394D379D76F46402D031F547DAC6E403751D7A6856C464086C028E474AE6E4013E2172532694640DCD08F556CB06E40F93045F3DC6546400368E4A863B26E40C9700C10866246405106B7DE5AB46E409717187A2D5F4640207A98F751B66E4055D90F30D35B464049E119F448B86E4073A298307758464099AACCD43FBA6E406A92547A195546405197429A36BC6E4026F6E20BBA514640B4BC0D452DBE6E407D42E0E3584E46407F85C0D523C06E40790EE600F64A464078B3ED4C1AC26E40A00D8B6191474640FA6028AB10C46E40290A63042B444640860204F106C66E4007DFFEE7C24046405068141FFDC76E401372EC0A593D4640D3BFED35F3C96E40F1ADB66BED39464076952436E9CB6E40F87BE5088036464016D64D20DFCD6E400DBEFDE010334640B6D0FEF4D4CF6E40594881F29F2F46401938CDB4CAD16E4002DBEE3B2D2C46406B244F60C0D36E40AF1BC2BBB8284640E8141BF8B5D56E401D8F73704225464095F1C77CABD76E4089927858CA214640E00CEDEEA0D96E4003554372501E46406325224F96DB6E40B7D042BCD41A46409A67FF9D8BDD6E4020C4E234571746409C6F1DDC80DF6E401EAB8BDAD7134640E44A150A76E16E40F3B7A2AB561046400C7A80286BE36E403DCC89A6D30C46409FF2F83760E56E40C0719FC94E094640ABC0D72905E66E40D14C2FF6E10746400FC50468D7E46E40B33809D15102464077DBAA85AEE36E401DE3A28DD5FC45408892D30B86E26E40B7FFA22058F7454009E0F2F95DE16E40CA235E88D9F1454019357D4F36E06E4077BE26C359EC4540F67CE70B0FDF6E402E144DCFD8E64540B01BA72EE8DD6E40F1391FAB56E14540E9EC31B7C1DC6E409F10E954D3DB45408C42FEA49BDB6E403E40F4CA4ED6454092E382F775DA6E401D33880BC9D04540BC0A37AE50D96E40F910EA1442CB4540586592C82BD86E401EBA5CE5B9C54540FE110D4607D76E405EC2207B30C045405E9F1F26E3D56E401D6C74D4A5BA4540FC0A4368BFD46E4033A393EF19B54540F5BFF00B9CD36E40DDF7B7CA8CAF4540D095A21079D26E4078991864FEA945403CCFD27556D16E405C51EAB96EA44540E618FC3A34D06E40897D5FCADD9E45403188995F12CF6E40510BA8934B9945401B9A26E3F0CD6E40EF71F113B8934540F2311FC5CFCC6E401CAD6649238E45403398FF04AFCB6E40813730328D884540517944A28ECA6E402C0574CCF58245408AE46A9C6EC96E40EF7D55165D7D4540B14AF0F24EC86E40AB77F50DC3774540087D52A52FC76E40923072B1277245400DAC0FB310C66E405949E7FE8A6C45405766A61BF2C46E4058BF6DF4EC6645405E9795DED3C36E4091E61B904D61454058865CFBB5C26E40C16305D0AC5B454012D57A7198C16E4045263BB20A564540C17E70407BC06E40F561CB3467504540DED6BD675EBF6E40F888C155C24A4540FE87E3E641BE6E40804526131C454540AA9262BD25BD6E407373FF6A743F4540384CBCEA09BC6E40031A505BCB394540AF5D726EEEBA6E403A6518E22034454096C20648D3B96E406B9F55FD742E4540D4C7FB76B8B86E409B2A02ABC7284540920AD4FA9DB76E40C57915E918234540127712D383B66E402B0A84B5681D454090473AFF69B56E406E5C3F0EB71745401C03CF7E50B46E40B1ED35F1031245407F7C545137B36E409730535C4F0C454013D14E761EB26E403A867F4D99064540AB6742ED05B16E40FE36A0C2E100454065EFB3B5EDAF6E405E6B97B928FB44409B5E28CFD5AE6E409E2444306EF54440BAF12439BEAD6E406D358224B2EF44401F2A2FF3A6AC6E40623A2A94F4E9444002CDCCFC8FAB6E407F92117D35E4444052E2835579AA6E4087570ADD74DE444098B3DAFC62A96E404156E3B1B2D84440D1CA57F24CA86E40B40668F9EED244405FF1813537A76E40398460B129CD4440DD2EE0C521A66E40838591D762C744400BC8F9A20CA56E408C54BC699AC14440A63D56CCF7A36E4067C69E65D0BB4440554B7D41E3A26E400033F3C804B6444089E6F601CFA16E40BF6C709137B04440523D4B0DBBA06E4016B8C9BC68AA444054B50263A79F6E40EFC2AE4898A44440A0EAA502949E6E40119CCB32C69E444092AEBDEB809D6E4054AAC878F2984440BD06D31D6E9C6E40D0A34A181D934440C52B6F985B9B6E40E484F20E468D444043881B5B499A6E402E875D5A6D874440A6B7616537996E40591825F8928144401485CBB625986E40DFD0DEE5B67B444047EAE24E14976E40A36A1C21D9754440790E322D03966E4077B76BA7F96F444038454351F2946E407E975676186A4440490DA1BAE1936E4081EF628B356444408A0FD668D1926E40079F12E4505E4440CF1D6D5BC1916E407276E37D6A584440BD31F191B1906E40E92C4F5682524440B26BED0BA28F6E402D56CB6A984C44409611EDC8928E6E404158C9B8AC464440C18D7BC8838D6E400361B63DBF404440CE6D240A758C6E40A15BFBF6CF3A44408461738D668B6E40DCE5FCE1DE344440A239F451588A6E4045451BFCEB2E4440C3E632574A896E40485CB242F72844403278BB9C3C886E40129F19B300234440CA1A1A222F876E406608A44A081D4440C217DBE621866E40390EA0060E1744408BD38AEA14856E40389657E411114440ABCCB52C08846E402CEA0FE1130B4440869AE8ACFB826E402AAC09FA130544403FECAF6AEF816E40B1CA802C12FF43407F879865E3806E409574AC750EF943404B472F9DD77F6E40C90CBFD208F34340E21A0111CC7E6E40FD1DE64001ED434072049BC0C07D6E40244E4ABDF7E64340B82FC6AD2C7D6E409D8FD6CD9DE34340D2BC2F420C7B6E4091903EC25DE74340");
  compareCsvLine(line.getRow(2),"EW0211937827G,EW0227176360G,5.84179,2.65463626,3.18715233,0.19006757,0.09378727,1.29192550,1.94112758,45.13522427,267.19694645,11.68167462,2.57592477,2.73334775,0.33276355,0.29679248,0.62939845,0.89701885,0.62964266,0.54241301,0.38400418,0.64057150,2011112,2011288,242.63800548,44.13232562,239.40578374,43.22627070,2439400,2439400,242.69024983,44.18930803,3983.69951235,238.33884703,46.71909681,3620.29077913,240.32138945,43.51269908,276.22354284,213.80763955,0.17593827,10.88075400,63.34472885,64.06415035,63.33215970,71.18282877,0103000000010000002801000074CBC27724336E404CCF1BE0BD604440F8D4FEE2B6306E40C4B7B924A96444407B215FBF482E6E4085E2AE5C926844404FF5D90BDA2B6E40789FF188796C44406E1465C76A296E40F56F74AA5E70444094BFF5F0FA266E40FB0926C2417444405FB180878A246E40635BF1D022784440601BFA8919226E40EE8CBDD7017C444045A355F7A71F6E4034056ED7DE7F4440D35F86CE351D6E40A26BE2D0B98344400BD67E0EC31A6E4038ABF6C4928744403DF630B64F186E4040F582B4698B44400B198EC4DB156E4000C45BA03E8F44407AFC863867136E403EDD51891193444002C10B11F2106E40B0543270E29644408EE60B4D7C0E6E40718EC655B19A4440834976EB050C6E403A41D43A7E9E4440C11F39EB8E096E40AB781D2049A24440A9F5414B17076E405D97600612A6444013AB7D0A9F046E40F25858EED8A944404570D82726026E4015D4BBD89DAD4440F4C23DA2ACFF6D404E7C3EC660B144402E6B987832FD6D40DB2390B721B544404778D2A9B7FA6D405CFD5CADE0B84440D63DD5343CF86D40799D4DA89DBC44408D508918C0F56D4078FC06A958C044402683D65343F36D40A6772AB011C4444043E3A3E5C5F06D40CCD255BEC8C7444053B6D7CC47EE6D40613923D47DCB44405B765708C9EB6D40E43F29F230CF4440E6CE079749E96D40F3E4FA18E2D24440BA99CC77C9E66D405192274991D64440C3DB88A948E46D40ED1D3B833EDA4440BEC11E2BC7E16D40CCCABDC7E9DD4440219D6FFB44DF6D40C549341793E14440BAE05B19C2DC6D405BBA1F723AE54440851DC3833EDA6D404BABFDD8DFE844404FFF8339BAD76D40341B484C83EC44407F497C3935D56D40FF7875CC24F04440AAD38882AFD26D405DA4F859C4F344405586851329D06D4012EE40F561F7444082574DEBA1CD6D403618BA9EFDFA44406347BA081ACB6D405F56CC5697FE4440DD5CA56A91C86D40BD4DDC1D2F0245402CA2E60F08C66D400F154BF4C4054540712155F77DC36D40A03476DA5809454027E1C61FF3C06D400FA6B7D0EA0C4540BCE0108867BE6D401AD465D77A1045400215072FDBBB6D404F9AD3EE08144540A0647C134EB96D4095445017951745408DA44234C0B66D40C08E27511F1B454074942A9031B46D40F7A3A19CA71E454014DB0326A2B16D400B1E03FA2D224540A0029DF411AF6D40BF048D69B22545401575C3FA80AC6D40F6CC7CEB3429454086784337EFA96D40BF570C80B52C4540642BE8A85CA76D4072F1712734304540C0807B4EC9A46D408050E0E1B0334540873CC62635A26D405C9486AF2B374540A7EF8F30A09F6D403B449090A43A45404AF49E6A0A9D6D40BC4D25851B3E4540F169B8D3739A6D407C036A8D904145408D31A06ADC976D409B1B7FA90345454091E9182E44956D4021AE81D97448454003EAE31CAB926D4056338B1DE44B45407D40C13511906D40FA81B175514F4540C33C9FAB328F6D401E488765C8504540B35670E269906D4075AF522B8558454051E37CDF9C916D408BB8EFB423604540251E683CD0926D40B050524DC1674540136E24FA03946D403BCDCEF75D6F45400F15A51938956D40F68CB6B7F97645401134DE9B6C966D40C8035890947E4540F4CEC481A1976D4055C6FE842E8645407AD04ECCD6986D407995F398C78D45403A0E737C0C9A6D40BA697CCF5F954540A74C2993429B6D408B7EDC2BF79C454015436A11799C6D408F5D54B18DA44540C29F2FF8AF9D6D40B3E9216323AC4540E50B7448E79E6D40426A8044B8B34540D42F33031FA06D40C795A8584CBB45400DB7692957A16D40079DD0A2DFC245406C5415BC8FA26D40AE352C2672CA45404FC634BCC8A36D4017A5ECE503D24540CEDAC72A02A56D40DDCA40E594D94540EB73CF083CA66D40712B552725E14540DE8B4D5776A76D4088FA53AFB4E8454058394517B1A86D408425658043F04540D1B3BA49ECA96D40C05DAE9DD1F74540E757B3EF27AB6D40CD22530A5FFF4540B0AB350A64AC6D40A1CC74C9EB0646403663499AA0AD6D40AA9532DE770E4640D064F7A0DDAE6D40DBA4A94B03164640ABCD491F1BB06D409817F5148E1D464048F64B1659B16D40A50B2E3D18254640FE760A8797B26D40E9A86BC7A12C4640942C9372D6B36D404A2BC3B62A344640D83CF5D915B56D4048EC470EB33B46404A1B41BE55B66D40A96C0BD13A434640C88D882096B76D40115E1D02C24A46403FB1DE01D7B86D4081AC8BA4485246407EFE576318BA6D40C88762BBCE594640E94E0A465ABB6D40FA6CAC49546146406FE10CAB9CBC6D40B42F7252D96846404E5F7893DFBD6D407E03BBD85D70464011E1660023BF6D40EE848CDFE17746407EF3F3F266C06D40F1C2EA69657F4640989C3C6CABC16D40DE47D87AE8864640B5605F6DF0C26D40932256156B8E464086477CF735C46D407BEF633CED9546404AE1B40B7CC56D4092E1FFF26E9D4640F34B2CABC2C66D4054CB263CF0A446406D3807D709C86D409A27D41A71AC4640DBEF6B9051C96D4085220292F1B34640F45882D899CA6D4042A2A9A471BB46405DFD73B0E2CB6D40D74FC255F1C24640230F6C192CCD6D40D69F42A870CA4640286E971476CE6D4015DB1F9FEFD14640BBAD24A3C0CF6D404B274E3D6ED94640261A44C60BD16D40BD8FC085ECE0464054BE277F57D26D40B70D697B6AE84640876903CFA3D36D4032913821E8EF464018B50CB7F0D46D4044091F7A65F74640470A7B383ED66D409D6C0B89E2FE46401AA887548CD76D40F7C1EB505F0647404DA96D0CDBD86D407E28ADD4DB0D47404D0A6A612ADA6D402DE03B17581547404CAFBB547ADB6D402B52831BD41C4740546AA3E7CADC6D4013196EE44F2447404EC983A2E4DC6D4069F5E30EE22447401B8AFE25C8DD6D40D75B3925A123474084D9264DCFDF6D403A8FE015C3204740045BDA3AD6E16D40C94FE58EE31D4740FAA49EEFDCE36D40C7CD798F021B4740FA58F96BE3E56D403E57CE1620184740E32470B0E9E76D40655511243C154740F4C388BDEFE96D40F4496FB656124740ECFFC893F5EB6D4079CC12CD6F0F474021B2B633FBED6D40A7872467870C474098C4D79D00F06D409236CB839D0947402333B2D205F26D40E9A12B22B20647407E0CCCD20AF46D40219D6841C50347406D73AB9E0FF66D409A03A3E0D6004740D89FD63614F86D40C1B5F9FEE6FD4640EDDFD39B18FA6D401696899BF5FA4640429929CE1CFC6D4040866DB502F84640F9495ECE20FE6D40FD63BE4B0EF54640D589F89C24006E402A06935D18F24640770B7F3A28026E409A3900EA20EF46406C9D78A72B046E40FEBD18F027EC4640632B6CE42E066E40BF42ED6E2DE946404CBFE0F131086E40C5638C6531E6464088825DD0340A6E403AA602D333E3464010BF6980370C6E403D755AB634E04640A3E08C023A0E6E40951E9C0E34DD4640F2754E573C106E404DCFCDDA31DA4640D131367F3E126E405090F3192ED7464063ECCB7A40146E40EF420FCB28D4464051A4974A42166E40649D20ED21D14640FA7F21EF43186E404B27257F19CE4640ACCEF168451A6E40053618800FCB4640D50991B8461C6E401AE9F2EE03C8464042D687DE471E6E409326ACCAF6C446404D055FDB48206E4035973812E8C146402B969FAF49226E40C6A28AC4D7BE464019B7D25B4A246E40366C92E0C5BB46409EC681E04A266E40CCCD3D65B2B84640D854363E4B286E40315578519DB54640AB247A754B2A6E40883F2BA486B24640162DD7864B2C6E405F753D5C6EAF4640769AD7724B2E6E40AB86937854AC4640C4CF053A4B306E40A9A60FF838A94640ED67ECDC4A326E40B7A791D91BA646401B37165C4A346E401EF7F61BFDA24640F94B0EB849366E40D2981ABEDC9F464012F15FF148386E401E23D5BEBA9C46401DAE9608483A6E4049BAFC1C9799464053493EFE463C6E402D0C65D771964640C6C8E2D2453E6E40BA4BDFEC4A934640B373108744406E40742C3A5C22904640F0D3531B43426E40DADD4124F88C464039B7399041446E40C106C043CC89464093304FE63F466E40A6C07BB99E864640BB99211E3E486E40E89239846F8346407D943E383C4A6E40036EBBA23E804640200C34353A4C6E40A4A6C0130C7D4640D3369015384E6E40BEF005D6D77946401E97E1D935506E40905A45E8A176464045FDB68233526E40974736496A734640C7889F1031546E406F6B8DF730704640CEA92A842E566E40A0C4FCF1F56C4640A322E8DD2B586E4072973337B96946403209681E295A6E408668DEC57A6646407DC83A46265C6E408CF7A69C3A6346402B22F155235E6E40C33934BAF85F4640F92F1C4E20606E408B542A1DB55C464051654D2F1D626E40C0972AC46F594640CB9016FA19646E401D78D3AD28564640BCDD09AF16666E408989C0D8DF524640CBD5B94E13686E4051798A43954F46407762B9D90F6A6E404208C7EC484C4640BDCE9B500C6C6E40CD0409D3FA484640ADC8F4B3086E6E40F744E0F4AA4546400063580405706E4059A0D95059424640C3165B4201726E40E7E97EE5053F4640F9C4916EFD736E40C6E956B1B03B46403BB89189F9756E40FB56E5B25938464076A6F093F5776E4005D1AAE80035464084B2448EF1796E406DD92451A6314640F36D2479ED7B6E403DCDCDEA492E4640ADDA2655E97D6E4061DE1CB4EB2A4640BE6CE322E57F6E40EE0C86AB8B2746400B0CF2E2E0816E4060207ACF292446401E16EB95DC836E40BBA0661EC6204640E55F673CD8856E408CCFB596601D4640803700D7D3876E40EEA0CE36F91946406087AD667B886E4048173131961846400B03930557876E402556A814F7124640F9EA1B6037866E4070AA7D196C0D4640C08DCE2218856E400BFDA5FBDF074640836D204DF9836E40FE9570B9520246403D8B87DEDA826E4038A02A51C4FC45407B657AD6BC816E40F7241FC134F7454012F76F349F806E4009079707A4F14540DDB5DFF7817F6E4024FED82212EC454079914120657E6E400C9229117FE6454000F20DAD487D6E40C515CBD0EAE04540CDB6BD9D2C7C6E40B0A2FD5F55DB45403E35CAF1107B6E409613FFBCBED545407237ADA8F5796E40C0FF0AE626D045400DFBE0C1DA786E40D3B55AD98DCA45400330E03CC0776E40CE362595F3C4454055F72519A6766E40E0309F1758BF4540E3E12D568C756E4037FAFA5EBBB9454033EF73F372746E40C18B68691DB44540358C74F059736E40DD7B15357EAE45401392AC4C41726E400BF92CC0DDA845400245990729716E4076C4D7083CA345400B53B82011706E407C2C3C0D999D4540DBD28797F96E6E4039077ECBF49745409442866BE26D6E40DFACBE414F9245409C86329CCB6C6E401DF21C6EA88C45407BE80B29B56B6E406722B54E008745409C1592119F6A6E4037FAA0E156814540311E455589696E4038A1F724AC7B45400174A5F373686E4067A4CD160076454045E933EC5E676E4020F034B55270454073AF713E4A666E401BCA3CFEA36A45402A56E0E935656E405CCBF1EFF3644540FBC901EE21646E4013DA5D88425F45404653584A0E636E405E2388C58F5945401C9566FEFA616E40101575A5DB534540188CAF09E8606E4055572626264E4540358DB66BD55F6E404FC69A456F484540B844FF23C35E6E40936BCE01B742454003B50D32B15D6E40B377BA58FD3C4540793566959F5C6E40853B5548423745405B718D4D8E5B6E408E2192CE85314540AD66085A7D5A6E402AA761E9C72B454014655CBA6C596E40D055B19608264540B70C0F6E5C586E400EBC6BD4472045401E4DA6744C576E40A76678A0851A45401A64A8CD3C566E406FD9BBF8C1144540ACDC9B782D556E403B8817DBFC0E4540DA8D07751E546E409DCF6945360945409F9972C20F536E409EED8D356E034540CE6B646001526E405BFA5BA9A4FD4440F4B8644EF3506E409BE0A89ED9F744403F7DFB8BE54F6E40305646130DF2444065FBB018D84E6E406BD402053FEC444082BB0DF4CA4D6E406190A9716FE644400C8A9A1DBE4C6E40157302579EE04440B176E094B14B6E40A811D2B2CBDA444039D36859A54A6E404CA5D982F7D444407A32BD6A99496E404A03D7C421CF4440336767C88D486E40C99484764AC944400083F17182476E40944E999571C344402FD5E56677466E40C8A8C81F97BD4440BEE9CEA66C456E406396C212BBB744403388373162446E40B27C336CDDB1444088B2AA0558436E40C72AC429FEAB444012A4B3234E426E40A1D019491DA644406DD0DD8A44416E4070F6D5C73AA044405CE2B43A3B406E409E7396A3569A4440C1BAC432323F6E40C265F5D9709444406B6F9972293E6E4086278968898E4440114ABFF9203D6E405E47E44CA088444039C7C2C7183C6E40397E9584B5824440129530DC103B6E40FEA5270DC97C44406A929536093A6E4005B021E4DA76444088CD7ED601396E40639B0607EB7044401D8379BBFA376E40196B5573F96A4440201D13E5F3366E40321C892606654440C131D952ED356E40B49B181E115F4440AA7819D78B356E4097793EB5D95C444074CBC27724336E404CCF1BE0BD604440");
  compareCsvLine(line.getRow(3),"EW0211937827G,EW0227049218G,5.43816,2.72335822,2.71480536,0.15346100,0.16083951,1.13774227,3.80218676,5.82588549,339.74718093,9.23576685,2.57592477,2.87079167,0.33276355,0.39040627,0.62939845,0.84738775,0.62964266,0.64191060,0.13993638,0.19389334,2011112,2011287,242.63800548,44.13232562,244.28557385,39.06533759,2439400,2439400,242.69024983,44.18930803,3983.69951235,246.25476832,41.86816063,3786.25597121,243.54304018,41.28870306,276.22354284,242.78217291,0.17593827,8.89551024,63.34472885,62.19187456,63.33215970,62.22804581,010300000001000000AE000000DFF1AF10E7BD6E405C49722EE3D044403FF923547ABC6E40ADD64714B9C94440AB8B07470EBB6E40FE673D048EC2444083D132E8A2B96E408E9FB1FB61BB44403E807F3638B86E40566E00F834B44440EBD5C830CEB66E404F0C83F606AD4440BD94EBD564B56E4092F08FF4D7A54440A3FEC524FCB36E4072C97AEFA79E4440EFD0371C94B26E406D7494E476974440F93F22BB2CB16E401EF62AD144904440D5F26700C6AF6E40077289B21189444006FFECEA5FAE6E404D22F885DD81444053E49679FAAC6E40674FBC48A87A444084884CAB95AB6E409B4718F8717344403433F67E31AA6E407F564B913A6C4440C1897DF3CDA86E405FBC9111026544401B8BCD076BA76E407AA52476C85D4440BB8BD2BA08A66E4037213ABC8D56444098317A0BA7A46E40461905E1514F44401770B3F845A36E408D48B5E11448444021846E81E5A16E402D3277BBD640444018F09CA485A06E402B18746B97394440F9773161269F6E404AF2D1EE563244406A1D20B6C79D6E408B64B342152B4440E11B5EA2699C6E40BEB53764D2234440BEE4E1240C9B6E40DCC57A508E1C4440841BA33CAF996E40660495044915444007929AE852986E4078669B7D020E4440A444C227F7966E40FC5C9FB8BA0644407D5615F99B956E4092CAAEB271FF4340CB0D905B41946E406FF9D36827F843401DD02F4EE7926E401C9115D8DBF04340AD1EF3CF8D916E40218C76FD8EE94340B792D9DF34906E407C2DF6D540E24340DED9E37CDC8E6E4017F68F5EF1DA43407FB213A6848D6E40009A3B94A0D3434026E86B5A2D8C6E40A1F5EC734ECC4340E8825E34A08B6E400A54DE1CE4C943404E3D066C3B896E409DAE1EA82ECE4340D21DC647DF866E4042FDB6B766D2434006C818B982846E40DFF9928D9CD643403322FFBE25826E401A903D2BD0DA4340F1F87958C87F6E40C6273D9201DF434049FC89846A7D6E40B4AB13C430E34340D2BC2F420C7B6E4091903EC25DE74340CFA86B90AD786E4087DB368E88EB434044093E6E4E766E40D4287129B1EF434032FFA6DAEE736E4029B25D95D7F343409880A6D48E716E40155568D3FBF74340B4553C5B2E6F6E402C99F8E41DFC43401A16686DCD6C6E4017B671CB3D004440DC25290A6C6A6E40AE9932885B044440AEB27E300A686E40C1ED951C770844401AB167DFA7656E40E71DF289900C444098D9E21545636E40295D99D1A7104440C5A5EED2E1606E408CABD9F4BC144440854D89157E5E6E4089DBFCF4CF1844402FC4B0DC195C6E406A9748D3E01C4440B5B56227B5596E407866FE90EF204440D9839CF44F576E4039B25B2FFC24444047435B43EA546E405ECB99AF06294440D3B89B1284526E40D1EEED120F2D44408F565A611D506E406E4A895A153144400C39932EB64D6E40DC01998719354440752442794E4B6E401833469B1B394440B9816240E6486E4019FBB5961B3D4440C05BEF827D466E402E7A097B194144408C5CE33F14446E4060D85D491545444062CA3876AA416E40B649CC020F494440F984E924403F6E405B126AA8064D44409502EF4AD53C6E40AC8A483BFC5044403B4D42E7693A6E40392375BCEF544440CBFFDBF8FD376E409B68F92CE15844402E43B47E91356E404C07DB8DD05C444074CBC27724336E404CCF1BE0BD604440F8D4FEE2B6306E40C4B7B924A96444407B215FBF482E6E4085E2AE5C926844404FF5D90BDA2B6E40789FF188796C44406E1465C76A296E40F56F74AA5E70444094BFF5F0FA266E40FB0926C2417444405070B8AFEE266E405E4C943F557444401C9F7D081B276E404B99020F7375444065386D5023286E40CFC1B4F2177C444041F5DBC32B296E40C02AE78DBB824440C8425E63342A6E409A544FE35D894440EDB1882F3D2B6E400F929FF5FE8F444052F9EF28462C6E40631187C79E9644402BF728504F2D6E40BEE5B15B3D9D444012B3C8A5582E6E406C10C9B4DAA34440ED5F642A622F6E40FE8972D576AA4440C25D91DE6B306E40544B51C011B144409B3BE5C275316E4092560578ABB744405FB9F5D77F326E4008C02BFF43BE4440A1C9581E8A336E40F0B65E58DBC444409493A49694346E40398E358671CB4440D8746F419F356E4003C5448B06D244405203501FAA366E40550F1E6A9AD84440100FDD30B5376E40785E50252DDF444031A4AD76C0386E4064E967BFBEE54440A10C59F1CB396E401235EE3A4FEC44401DD276A1D73A6E40B51C6A9ADEF24440F3BF9E87E33B6E40E6D95FE06CF94440F3E468A4EF3C6E40B60C510FFAFF444042956DF8FB3D6E40BFC3BC2986064540426C4584083F6E4000841F32110D45406F4E894815406E40D550F32A9B1345403A6BD24522416E40B9B3AF16241A4540F53EBA7C2F426E4001C4C9F7AB204540B294DAED3C436E408F2EB4D0322745402088CD994A446E406F3DDFA3B82D454079872D8158456E404EDFB8733D344540635595A466466E4014AFAC42C13A4540D40AA00475476E4036FB231344414540FF18E9A183486E4013CD85E7C54745403B4B0C7D92496E4049F036C2464E4540EAC8A596A14A6E40E7F999A5C6544540681752EFB04B6E40A14F0F94455B4540F81BAE87C04C6E40E02EF58FC3614540AC1D5760D04D6E40E2B3A79B4068454061C7EA79E04E6E40AEE080B9BC6E4540A62907D5F04F6E400AA4D8EB37754540E0862DE962506E4037C01FE062774540C29EF3D2AB526E4041E157574274454032F9460FEA546E40148271562E71454024D0CCAF27576E4094D42C1B196E45402C5471B564596E40F21E3FA5026B4540EED81F21A15B6E401BCA5BF4EA674540B8D7C2F3DC5D6E400C623408D2644540FDF1432E18606E40E39578E0B7614540ECF38BD152626E402838D67C9C5E4540F1D682DE8C646E40C63EF9DC7F5B45402DC40F56C6666E4037C38B0062584540F3161939FF686E40880236E742554540445F8488376B6E404D5D9E9022524540396436456F6D6E40AE5769FC004F454077261370A66F6E403999392ADE4B454093E2FD09DD716E40DDECAF19BA4845407B13D91313746E40C1406BCA94454540D674868E48766E400BA6083C6E4245406205E77A7D786E40AF50236E463F45404E09DBD9B17A6E402F9754601D3C4540900C42ACE57C6E4051F23312F338454037E5FAF2187F6E40C5FC5683C7354540BBB5E3AE4B816E40D07251B39A32454047EFD9E07D836E40E331B5A16C2F4540FC53BA89AF856E402B38124E3D2C45403FF960AAE0876E401AA4F6B70C294540F649A943118A6E40E5B3EEDEDA254540BE086E56418C6E40F9C484C2A72245402E5289E3708E6E4062534162731F45400D9FD4EB9F906E403BF9AABD3D1C454084C62870CE926E40F26D46D4061945404F005E71FC946E40B78596A5CE154540EBE64BF029976E409E301C3195124540BF79C9ED56996E40017A56765A0F45404B1FAD6A839B6E409187C2741E0C454048A7CC67AF9D6E40A798DB2BE1084540C94CFDE5DA9F6E4046051B9BA20545405FB813E605A26E403E3DF8C1620245403602E46830A46E404BC7E89F21FF44402BB4416F5AA66E40FF3F6034DFFB4440EACBFFF983A86E40D958D07E9BF84440FFBCF009ADAA6E401FD7A87E56F54440EE72E69FD5AC6E40E592573310F244403F53B2BCFDAE6E40D675489CC8EE4440913F256125B16E40157AE5B87FEB4440A3970F8E4CB36E4008A9968835E84440613B414473B56E401F1AC20AEAE44440EC8C898499B76E408DF1CB3E9DE144409D72B74FBFB96E40F85E16244FDE44400A5999A6E4BB6E40249C01BAFFDA44400A35FD8909BE6E408DEBEBFFAED744408E0AF728F8BE6E40F5909D273DD64440DFF1AF10E7BD6E405C49722EE3D04440");
  compareCsvLine(line.getRow(4),"EW0211937827G,EW0227091561G,5.39443,2.69415841,2.70026726,0.15245749,0.16880805,1.13873824,4.85025546,6.72056211,534.16964469,5.90463198,2.57592477,2.81239205,0.33276355,0.34337706,0.62939845,0.84597701,0.62964266,0.63190668,0.13537987,0.18789134,2011112,2011287,242.63800548,44.13232562,241.35602061,39.08394157,2439400,2439400,242.69024983,44.18930803,3983.69951235,243.25336109,41.89086960,3785.18816617,242.08712025,41.28884463,276.22354284,242.56983077,0.17593827,8.83908048,63.34472885,63.13245863,63.33215970,63.12839824,010300000001000000CF000000C1897DF3CDA86E405FBC9111026544401B8BCD076BA76E407AA52476C85D4440BB8BD2BA08A66E4037213ABC8D56444098317A0BA7A46E40461905E1514F44401770B3F845A36E408D48B5E11448444021846E81E5A16E402D3277BBD640444018F09CA485A06E402B18746B97394440F9773161269F6E404AF2D1EE563244406A1D20B6C79D6E408B64B342152B4440E11B5EA2699C6E40BEB53764D2234440BEE4E1240C9B6E40DCC57A508E1C4440841BA33CAF996E40660495044915444007929AE852986E4078669B7D020E4440A444C227F7966E40FC5C9FB8BA0644407D5615F99B956E4092CAAEB271FF4340CB0D905B41946E406FF9D36827F843401DD02F4EE7926E401C9115D8DBF04340AD1EF3CF8D916E40218C76FD8EE94340B792D9DF34906E407C2DF6D540E24340DED9E37CDC8E6E4017F68F5EF1DA43407FB213A6848D6E40009A3B94A0D3434026E86B5A2D8C6E40A1F5EC734ECC4340E8825E34A08B6E400A54DE1CE4C943404E3D066C3B896E409DAE1EA82ECE4340D21DC647DF866E4042FDB6B766D2434006C818B982846E40DFF9928D9CD643403322FFBE25826E401A903D2BD0DA4340F1F87958C87F6E40C6273D9201DF434049FC89846A7D6E40B4AB13C430E34340D2BC2F420C7B6E4091903EC25DE74340CFA86B90AD786E4087DB368E88EB434044093E6E4E766E40D4287129B1EF434032FFA6DAEE736E4029B25D95D7F343409880A6D48E716E40155568D3FBF74340B4553C5B2E6F6E402C99F8E41DFC43401A16686DCD6C6E4017B671CB3D004440DC25290A6C6A6E40AE9932885B044440AEB27E300A686E40C1ED951C770844401AB167DFA7656E40E71DF289900C444098D9E21545636E40295D99D1A7104440C5A5EED2E1606E408CABD9F4BC144440854D89157E5E6E4089DBFCF4CF1844402FC4B0DC195C6E406A9748D3E01C4440B5B56227B5596E407866FE90EF204440D9839CF44F576E4039B25B2FFC24444047435B43EA546E405ECB99AF06294440D3B89B1284526E40D1EEED120F2D44408F565A611D506E406E4A895A153144400C39932EB64D6E40DC01998719354440752442794E4B6E401833469B1B394440B9816240E6486E4019FBB5961B3D4440C05BEF827D466E402E7A097B194144408C5CE33F14446E4060D85D491545444062CA3876AA416E40B649CC020F494440F984E924403F6E405B126AA8064D44409502EF4AD53C6E40AC8A483BFC5044403B4D42E7693A6E40392375BCEF544440CBFFDBF8FD376E409B68F92CE15844402E43B47E91356E404C07DB8DD05C444074CBC27724336E404CCF1BE0BD604440F8D4FEE2B6306E40C4B7B924A96444407B215FBF482E6E4085E2AE5C926844404FF5D90BDA2B6E40789FF188796C44406E1465C76A296E40F56F74AA5E70444094BFF5F0FA266E40FB0926C2417444405FB180878A246E40635BF1D022784440601BFA8919226E40EE8CBDD7017C444045A355F7A71F6E4034056ED7DE7F4440D35F86CE351D6E40A26BE2D0B98344400BD67E0EC31A6E4038ABF6C4928744403DF630B64F186E4040F582B4698B44400B198EC4DB156E4000C45BA03E8F44407AFC863867136E403EDD51891193444002C10B11F2106E40B0543270E29644408EE60B4D7C0E6E40718EC655B19A4440834976EB050C6E403A41D43A7E9E4440C11F39EB8E096E40AB781D2049A24440A9F5414B17076E405D97600612A6444013AB7D0A9F046E40F25858EED8A944404570D82726026E4015D4BBD89DAD4440F4C23DA2ACFF6D404E7C3EC660B144402E6B987832FD6D40DB2390B721B544404778D2A9B7FA6D405CFD5CADE0B84440D63DD5343CF86D40799D4DA89DBC44408D508918C0F56D4078FC06A958C044402683D65343F36D40A6772AB011C4444043E3A3E5C5F06D40CCD255BEC8C7444053B6D7CC47EE6D40613923D47DCB44405B765708C9EB6D40E43F29F230CF4440E6CE079749E96D40F3E4FA18E2D24440BA99CC77C9E66D405192274991D64440C3DB88A948E46D40ED1D3B833EDA4440BEC11E2BC7E16D40CCCABDC7E9DD4440219D6FFB44DF6D40C549341793E14440BAE05B19C2DC6D405BBA1F723AE54440851DC3833EDA6D404BABFDD8DFE84440A6476714CFD96D400E5001F380E9444072B2D8B3D0D96D40916F60B48BE944406DE249A0D0DA6D40DD94B27029F04440B09F73C0D0DB6D4028210D19C6F64440C685E814D1DC6D40394AF8AF61FD4440A8743B9ED1DD6D40B9BEF937FC0345408E92FF5CD2DE6D404EAE94B3950A4540BF4DC851D3DF6D4062D149252E114540685E297DD4E06D401D71978FC51745406DC8B6DFD5E16D402F6FF9F45B1E45403FDD047AD7E26D406E4DE957F1244540B53DA84CD9E36D40A435DEBA852B4540DDDB3558DBE46D4009014D2019324540DBFC429DDDE56D40E13FA88AAB384540BF3A651CE0E66D40EA4060FC3C3F45405A8632D6E2E76D40C618E377CD454540282941CBE5E86D405CA99CFF5C4C45401CC727FCE8E96D4018A9F695EB5245408E607D69ECEA6D4036AA583D795945400C54D913F0EB6D40DF2128F8056045404460D3FBF3EC6D40536FC8C891664540E8A50322F8ED6D40F3E29AB11C6D45408CA90287FCEE6D404AC5FEB4A6734540318E59936AEF6D40A178A8FED775454042A47D75B8F16D405C4E5E33DB7245406F530198FBF36D40F761B24AEA6F45408FB55F223EF66D40BF27DE1FF86C4540B3F8801580F86D40B1CAA1B2046A454008764C72C1FA6D404093BB02106745405AB4A83902FD6D40A2E7E70F1A6445408C6A7B6C42FF6D40174CE1D9226145401982A90B82016E402E6360602A5E454089191718C1036E40E5ED1BA3305B4540E886A792FF056E40ECCBC8A135584540285A3D7C3D086E40B7FB195C395545409F5FBAD57A0A6E40959AC0D13B52454055A2FF9FB70C6E40CAE46B023D4F4540786EEDDBF30E6E408035C9ED3C4C4540B753638A2F116E40D70684933B4945408F2740AC6A136E40CCF145F338464540B6076242A5156E401CAEB60C35434540565CA64DDF176E4030127CDF2F40454070DAE9CE181A6E40EC123A6B293D4540198608C7511C6E407CC392AF213A4540CEB4DD368A1E6E40165526AC18374540A60F441FC2206E40B11693600E344540A5951581F9226E40C77475CC02314540EA9D2B5D30256E40E1F867EFF52D4540F5D95EB466276E404A4903C9E72A4540D85787879C296E409D28DE58D827454062847CD7D12B6E4053758D9EC7244540602D15A5062E6E403B29A499B5214540BB8327F13A306E400759B349A21E4540AF1D89BC6E326E40A5334AAE8D1B4540E3F80E08A2346E40BA01F6C677184540997C8DD4D4366E40EF24429360154540CE7BD82207396E405117B812481245405137C3F3383B6E40996ADF442E0F4540EA5F20486A3D6E4065C73D29130C45406A18C2209B3F6E407CEC56BFF6084540C4F7797ECB416E40F9ADAC06D9054540280B1962FB436E406DF4BEFEB902454008D86FCC2A466E40FDBB0BA799FF4440315E4EBE59486E407B130FFF77FC4440D7198438884A6E406B1B430655F944409B05E03BB64C6E40FC0420BC30F64440959C30C9E34E6E4010111C200BF3444061DC43E110516E40178FAB31E4EF44401647E7843D536E4000DC40F0BBEC444054E5E7B469556E4010614C5B92E944403D48127295576E40B4923C7267E64440748B32BDC0596E4041EF7D343BE3444019571497EB5B6E40BFFD7AA10DE04440CBE18200165E6E40944C9CB8DEDC44408FF248FA3F606E402F704879AED94440D8E2308569626E40B301E4E27CD6444072A004A292646E40819DD1F449D3444077AF8D51BB666E40CEE171AE15D04440462C9594E3686E40266D230FE0CC444068CDE36B0B6B6E40E7DC4216A9C944408AE541D8326D6E40B0CB2AC370C644405F6577DA596F6E40C5CF331537C344409CDD4B7380716E407379B40BFCBF4440CB8086A3A6736E405A5101A6BFBC44404D25EE6BCC756E40C1D66CE381B944402D4749CDF1776E40C87D47C342B64440150A5EC8167A6E40A3ADDF4402B344402A3BF25D3B7C6E40C7BE8167C0AF4440F352CB8E5F7E6E4003F9772A7DAC44404577AE5B83806E409E910A8D38A94440157D60C5A6826E405DA97F8EF2A5444067EAA5CCC9846E40834A1B2EABA2444027F84272EC866E40D6661F6B629F44400D94FBB60E896E406DD5CB44189C44407862939B308B6E40B5505EBACC98444050C0CD20528D6E40307412CB7F954440E0C46D47738F6E4049BA217631924440B443361094916E401E7AC3BAE18E444076CEE97BB4936E4022E52C98908B4440C8B64A8BD4956E40D704910D3E88444020101B3FF4976E4078B8201AEA844440A3B11C98139A6E4078B20ABD94814440FC371197329C6E401C767BF53D7E44404007BA3C519E6E40F3549DC2E57A4440B34CD8896FA06E40576C98238C774440BA002D7F8DA26E40C4A2921731744440A6E8781DABA46E403EA5AF9DD47044408F987C65C8A66E40A0E410B5766D44402975F857E5A86E40ED92D55C176A4440FA5CD6DB92A96E40EA8C9BCD01694440C1897DF3CDA86E405FBC911102654440");
  compareCsvLine(line.getRow(5),"EW0211937827G,EW0227218705G,5.342,2.62690796,2.71509315,0.18918352,0.18228439,1.29667947,2.93505782,46.19021950,185.40092453,16.59270356,2.57592477,2.67789114,0.33276355,0.25199747,0.62939845,0.89574085,0.62964266,0.53286609,0.16744656,0.28068045,2011112,2011289,242.63800548,44.13232562,236.51781004,43.39051800,2439400,2439400,242.69024983,44.18930803,3983.69951235,235.36278068,46.84356281,3616.07897978,238.87846508,43.59323435,276.22354284,213.02376507,0.17593827,10.82963439,63.34472885,64.96005053,63.33215970,72.04205127,010300000001000000C7000000E6CE079749E96D40F3E4FA18E2D24440BA99CC77C9E66D405192274991D64440C3DB88A948E46D40ED1D3B833EDA4440BEC11E2BC7E16D40CCCABDC7E9DD4440219D6FFB44DF6D40C549341793E14440BAE05B19C2DC6D405BBA1F723AE54440851DC3833EDA6D404BABFDD8DFE844404FFF8339BAD76D40341B484C83EC44407F497C3935D56D40FF7875CC24F04440AAD38882AFD26D405DA4F859C4F344405586851329D06D4012EE40F561F7444082574DEBA1CD6D403618BA9EFDFA44406347BA081ACB6D405F56CC5697FE4440DD5CA56A91C86D40BD4DDC1D2F0245402CA2E60F08C66D400F154BF4C4054540712155F77DC36D40A03476DA5809454027E1C61FF3C06D400FA6B7D0EA0C4540BCE0108867BE6D401AD465D77A1045400215072FDBBB6D404F9AD3EE08144540A0647C134EB96D4095445017951745408DA44234C0B66D40C08E27511F1B454074942A9031B46D40F7A3A19CA71E454014DB0326A2B16D400B1E03FA2D224540A0029DF411AF6D40BF048D69B22545401575C3FA80AC6D40F6CC7CEB3429454086784337EFA96D40BF570C80B52C4540642BE8A85CA76D4072F1712734304540C0807B4EC9A46D408050E0E1B0334540873CC62635A26D405C9486AF2B374540A7EF8F30A09F6D403B449090A43A45404AF49E6A0A9D6D40BC4D25851B3E4540F169B8D3739A6D407C036A8D904145408D31A06ADC976D409B1B7FA90345454091E9182E44956D4021AE81D97448454003EAE31CAB926D4056338B1DE44B45407D40C13511906D40FA81B175514F4540C33C9FAB328F6D401E488765C8504540B35670E269906D4075AF522B8558454051E37CDF9C916D408BB8EFB423604540251E683CD0926D40B050524DC1674540136E24FA03946D403BCDCEF75D6F45400F15A51938956D40F68CB6B7F97645401134DE9B6C966D40C8035890947E4540F4CEC481A1976D4055C6FE842E8645407AD04ECCD6986D407995F398C78D45403A0E737C0C9A6D40BA697CCF5F954540A74C2993429B6D408B7EDC2BF79C454015436A11799C6D408F5D54B18DA44540C29F2FF8AF9D6D40B3E9216323AC4540E50B7448E79E6D40426A8044B8B34540D42F33031FA06D40C795A8584CBB45400DB7692957A16D40079DD0A2DFC245406C5415BC8FA26D40AE352C2672CA45404FC634BCC8A36D4017A5ECE503D24540CEDAC72A02A56D40DDCA40E594D94540EB73CF083CA66D40712B552725E14540DE8B4D5776A76D4088FA53AFB4E8454058394517B1A86D408425658043F04540D1B3BA49ECA96D40C05DAE9DD1F74540E757B3EF27AB6D40CD22530A5FFF4540B0AB350A64AC6D40A1CC74C9EB0646403663499AA0AD6D40AA9532DE770E4640D064F7A0DDAE6D40DBA4A94B03164640ABCD491F1BB06D409817F5148E1D464048F64B1659B16D40A50B2E3D18254640FE760A8797B26D40E9A86BC7A12C4640942C9372D6B36D404A2BC3B62A344640D83CF5D915B56D4048EC470EB33B46404A1B41BE55B66D40A96C0BD13A434640C88D882096B76D40115E1D02C24A46403FB1DE01D7B86D4081AC8BA4485246407EFE576318BA6D40C88762BBCE594640E94E0A465ABB6D40FA6CAC49546146406FE10CAB9CBC6D40B42F7252D96846404E5F7893DFBD6D407E03BBD85D70464011E1660023BF6D40EE848CDFE17746407EF3F3F266C06D40F1C2EA69657F4640989C3C6CABC16D40DE47D87AE8864640B5605F6DF0C26D40932256156B8E464086477CF735C46D407BEF633CED9546404AE1B40B7CC56D4092E1FFF26E9D4640F34B2CABC2C66D4054CB263CF0A446406D3807D709C86D409A27D41A71AC4640DBEF6B9051C96D4085220292F1B34640F45882D899CA6D4042A2A9A471BB46405DFD73B0E2CB6D40D74FC255F1C2464044AFEFA3D7CC6D403549B58D84C84640C60B9CBF0DCE6D408245AA72BBC6464000E94BAC12D06D40ED26C6AEBFC346405D5D1D7717D26D406948AF4DC2C0464021F698201CD46D4006D14D4EC3BD4640F07C47A920D66D40EA8C87AFC2BA464018F9B11125D86D402CE93F70C0B74640DCB0615A29DA6D409BEF578FBCB44640D22AE0832DDC6D408342AE0BB7B146401E2FB78E31DE6D405D181FE4AFAE4640DDC8707B35E06D4075378417A7AB46406B47974A39E26D407FF1B4A49CA84640C13FB5FC3CE46D40251F868A90A54640D58D559240E66D407B1BCAC782A24640F155030C44E86D4070BF505B739F46400F064A6A47EA6D402D5DE743629C46404B57B5AD4AEC6D4065BB58804F994640384FD1D64DEE6D4091106D0F3B9646404E412AE650F06D402BFEE9EF2493464053D04CDC53F26D40C78B92200D904640CDEFC5B956F46D40322227A0F38C46406AE5227F59F66D406D86656DD88946406D4AF12C5CF86D40A3D40887BB864640390DBFC35EFA6D40177BC9EB9C834640B3721A4461FC6D40EB345D9A7C804640C81792AE63FE6D40ED0477915A7D4640DEF2B40366006E404A30C7CF367A46406955124468026E402D39FB531177464062ED39706A046E405BD9BD1CEA734640CBC6BB886C066E40A8FCB628C1704640404D288E6E086E406FBB8B76966D4640834D1081700A6E40EB54DE046A6A46400EF70462720C6E408D294ED23B6746409BDD9731740E6E4027B577DD0B644640CFFA5AF075106E402589F424DA604640C3AFE09E77126E4095465BA7A65D4640ACC6BB3D79146E4035983F63715A46407F747FCD7A166E40572C32573A574640875ABF4E7C186E40CBAEC081015446401D880FC27D1A6E409FC275E1C6504640537C04287F1C6E40D6FBD8748A4D46409D273381801E6E400AD96E3A4C4A464088ED30CE81206E40FCBCB8300C4746407BA6930F83226E400BE83456CA43464064A1F14584246E4092715EA98640464084A5E17185266E403F41AD28413D46402FF4FA9386286E40460896D2F9394640924AD5AC872A6E408B3A8AA5B0364640966B377A312B6E404EC90DDC573546401337B542162A6E40199D3006AF2F4640312093A3FF286E400BA7F27D1A2A464096AB046CE9276E4036956EDA842446406F73819BD3266E40E380F119EE1E464026938131BE256E407172C63A561946403DA67D2DA9246E40BB5C363BBD134640E8C6EE8E94236E4068188819230E4640E08C4E5580226E403E5F00D4870846401C0C17806C216E4051C7E168EB02464090D3C20E59206E4042BE6CD64DFD4540EFEBCC00461F6E406284DF1AAFF745406DD6B055331E6E40C12776340FF24540898BEA0C211D6E40437F6A216EEC4540D279F6250F1C6E409F25F4DFCBE64540B08451A0FD1A6E404B74486E28E145402103797BEC196E40647E9ACA83DB454096BEEAB6DB186E408B0B1BF3DDD54540BBF12452CB176E40A692F8E536D045403C47A64CBB166E40A2345FA18ECA454094D8EDA5AB156E4024B77823E5C44540ED2C7B5D9C146E40207F6C6A3ABF4540DF37CE728D136E40788B5F748EB945404B5867E57E126E40796F743FE1B345403157C7B470116E40544DCBC932AE454081666FE062106E407DD0811183A84540F21FE167550F6E401728B314D2A24540DA839E4A480E6E40270178D11F9D454007F829883B0D6E40D980E6456C974540974606202F0C6E40AC3E1270B7914540D89CB611230B6E408B3E0C4E018C45401B8ABE5C170A6E40D3EAE2DD4986454096FEA1000C096E40590EA21D91804540444AE5FC00086E404DCE520BD77A4540BE1B0D51F6066E4014A4FBA41B7545401D7F9EFCEB056E401757A0E85E6F4540DDDC1EFFE1046E407AF641D4A0694540B9F81358D8036E40C2D2DE65E163454092F00307CF026E407077729B205E4540543B750BC6016E407AA4F5725E584540D1A7EE64BD006E40D6475EEA9A524540AA5BF712B5FF6D40C2769FFFD54C454040D21615ADFE6D402467A9B00F4745407DDBD46AA5FD6D40BD6869FB47414540E09AB9139EFC6D4059DEC9DD7E3B454048864D0F97FB6D40E036B255B4354540E664195D90FA6D4063E60661E82F4540284EA6FC89F96D40035FA9FD1A2A4540A0A87DED83F86D40D40978294C244540EA28292F7EF76D409D3F4EE27B1E4540A0D032C178F66D409B410426AA1845403BED24A373F56D4010326FF2D612454006178AD46EF46D40D70C6145020D45400130ED546AF36D40DE9FA81C2C074540DE62D92366F26D407F83117654014540D221DA4062F16D40C512644F7BFB4440A6257BAB5EF06D40B76365A6A0F54440826C48635BEF6D405D3FD778C4EF4440F338CE6758EE6D40D31978C4E6E94440CF1099B855ED6D40410A038707E4444022BC355553EC6D40AEC22FBE26DE44402344313D51EB6D40BB87B26744D844401BF218704FEA6D405A283C8160D244404767135D2CEA6D40A85111E892D14440E6CE079749E96D40F3E4FA18E2D24440");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0214460053G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 2);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EW0214460053G,EW0214460026G,6.46791,3.39127733,3.07663628,0.12765626,0.04160876,1.04413084,2.25424499,25.77231783,92.25753900,11.01734990,3.52348383,3.25907084,0.82967728,0.82666456,0.81618604,0.63558617,0.90057671,0.82056711,0.28804525,0.31300787,2011141,2011141,231.99393161,37.41256798,230.57712841,38.35436687,2439400,2439400,230.90240804,36.44448687,2937.78858285,230.55177059,38.28753774,2920.26211530,231.25679500,37.88869364,89.81021492,86.01433010,7.64744170,0.42344714,46.59354560,46.53329134,38.94809534,46.14895995,0103000000010000000C0100004459121DFBD46C40DD85B73320774240163051F95CD46C403561EFA0EC784240B779C2C5BED36C408F1712FBB87A4240B242548220D36C40F4E24442857C42403B90F42E82D26C40AAF6AC76517E42402A6091CBE3D16C40587F6F981D804240DCA8185845D16C4023A3B1A7E98142402E5978D4A6D06C40DD8198A4B583424068589E4008D06C400F35498F818542402586789C69CF6C4030D0E8674D87424053BAF4E7CACE6C40B6609C2E198942400BC500232CCE6C4042EE88E3E48A4240926E8A4D8DCD6C40B97AD386B08C424041777F67EECC6C406402A1187C8E42407697CD704FCC6C401D7C169947904240797F6269B0CB6C4061D958081392424078D72B5111CB6C4076068D66DE9342406F3F172872CA6C4093EAD7B3A9954240164F12EED2C96C40EF675EF074974240D1950AA333C96C40F95B451C409942409B9AED4694C86C40629FB1370B9B4240FCDBA8D9F4C76C404C06C842D69C4240ECCF295B55C76C406660AD3DA19E4240CBE35DCBB5C66C400B7986286CA04240497C322A16C66C406717780337A2424059F5947776C56C4091FEA6CE01A442401CA272B3D6C46C40B2ED378ACCA54240CBCCB8DD36C46C401FA04F3697A74240ADB654F696C36C4080CD12D361A94240FF9733FDF6C26C40E929A6602CAB4240D87AFDEFC3C26C405AAFAEC2DEAB4240D47BB9B453C36C40E8463C09E6AD4240FBCE3F77E1C36C40AB0320CAE5AF424052FA58576FC46C406076CBB1E5B14240F5851C55FDC46C40E3596DC0E5B34240EE0DA2708BC56C407B8334F6E5B54240564201AA19C66C4018E34F53E6B7424066E75101A8C66C408183EED7E6B94240A6D5AB7636C76C40828A3F84E7BB424005FA260AC5C76C402A397258E8BD4240F055DBBB53C86C40F2EBB554E9BF424080FFE08BE2C86C40FA1A3A79EAC142408821507A71C96C40345A2EC6EBC34240C6FB408700CA6C409E59C23BEDC54240F1E2CBB28FCA6C4077E525DAEEC74240E84009FD1ECB6C406DE688A1F0C94240C9941166AECB6C40D3611B92F2CB42401173FDED3DCC6C40E1790DACF4CD4240C685E594CDCC6C40D76D8FEFF6CF4240868CE25A5DCD6C40419AD15CF9D14240B65C0D40EDCD6C40247904F4FBD34240A5E17E447DCE6C4033A258B5FED542409A1C50680DCF6C4015CBFEA001D842400F259AAB9DCF6C4081C727B704DA4240BC28760E2ED06C408D8904F807DC4240C56BFD90BED06C40D421C6630BDE4240DB4849334FD16C40B9BF9DFA0EE042405C3173F5DFD16C4096B1BCBC12E2424071AD94D770D26C40F56454AA16E442403B5CC7D901D36C40CF6696C31AE64240EFF324FC92D36C40BF63B4081FE84240F441C73E24D46C403328E07923EA4240142BC8A1B5D46C40B6A04B1728EC424093AB412547D56C4019DA28E12CEE42405AD74DC9D8D56C40B401AAD731F0424015DA068E6AD66C40A36501FB36F2424059F78673FCD66C40F374614B3CF44240D18AE8798ED76C40EDBFFCC841F64240550846A120D86C4043F8057447F8424013FCB9E9B2D86C4052F1AF4C4DFA4240BD0A5F5345D96C405EA02D5353FC4240A3F14FDED7D96C40C51CB28759FE4240DE86A78A6ADA6C4043A070EA5F00434073B98058FDDA6C402C879C7B660243407B91F64790DB6C40A950693B6D0443404B30245923DC6C40F19E0A2A7406434099D0248CB6DC6C408737B4477B0843409EC613E149DD6C4080039A94820A434044800C58DDDD6C40B10FF0108A0C43404A852AF170DE6C40FB8CEABC910E43406C7789AC04DF6C4081D0BD98991043408C12458A98DF6C40EC539EA4A1124340D32C798A2CE06C40A6B5C0E0A9144340EBB641ADC0E06C4019B9594DB21643400DBCBAF254E16C40F1469EEABA1843404462005BE9E16C405E6DC3B8C31A434087EA2EE67DE26C404C60FEB7CC1C4340E2B0629412E36C40B07984E8D51E4340AB2CB865A7E36C40BE398B4ADF2043409EF04B5A3CE46C40304748DEE822434012AB3A72D1E46C40836FF1A3F22443401C26A1AD66E56C4044A7BC9BFC264340C1479C0CFCE56C40450AE0C5062943401C12498F91E66C40E2DB9122112B434088A3C43527E76C40538708B21B2D4340CF362C00BDE76C40DC9F7A74262F434053239DEE52E86C401AE11E6A3131434046DD3401E9E86C404E2F2C933C334340BFF510387FE96C408F97D9EF47354340001B4F9315EA6C4022505E805337434093180D13ACEA6C40B7B8F1445F39434078D768B742EB6C40AE5ACB3D6B3B4340605E8080D9EB6C4062E9226B773D4340CAD1716E70EC6C40684230CD833F43403F745B8107ED6C40E16D2B64904143407AA65BB99EED6C40B69E4C309D43434093E7901636EE6C40E832CC31AA4543403AD51999CDEE6C40D1B3E268B7474340DB2B154165EF6C4077D6C8D5C4494340D7C6A10EFDEF6C40CE7BB778D24B4340ADA0DE0195F06C40FEB0E751E04D434035D3EA1A2DF16C40B5AF9261EE4F4340BC97E559C5F16C4073DEF1A7FC5143405347EEBE5DF26C40C4D03E250B544340E85A244AF6F26C40A347B3D919564340816BA7FB8EF36C40B33189C528584340733297D327F46C4093ABFAE8375A4340898913D2C0F46C402E004244475C4340456B3CF759F56C40FFA899D7565E43400EF33143F3F56C40624E3CA366604340565D14B68CF66C40E7C764A776624340E707045026F76C409B1C4EE48664434004722111C0F76C405A83335A97664340AC3C8DF959F86C401A635009A8684340C12A6809F4F86C404153E0F1B86A43404321D3408EF96C40EF1B1F14CA6C43409227EF9F28FA6C4053B64870DB6E43402BE38CD272FA6C40A0A2A5B9D96F4340FF3B1A80B5FA6C4050D3384B216F4340D313B47654FB6C405296335B696D4340C03FBB5FF3FB6C407E661E52B16B4340295C403B92FC6C40A8AAD62FF96943401A01540931FD6C40BDBE39F4406843406AC206CACFFD6C409CF3249F88664340BB2F697D6EFE6C40F18E7530D064434091D48B230DFF6C4024CB08A81763434063387FBCABFF6C4028D7BB055F6143409ADE53484A006D4072D66B49A65F4340B3461AC7E8006D40BFE0F572ED5D434047ECE23887016D4007023782345C43400747BE9D25026D40573A0C777B5A4340E9CABCF5C3026D40AE7D5251C258434024E8EE4062036D40E6B3E610095743403E0B657F00046D4088B8A5B54F554340209D2FB19E046D40B35A6C3F9653434023035FD63C056D40FD5C17AEDC5143401A9F03EFDA056D404C7583012350434068CF2DFB78066D40BC4C8D39694E434008EFEDFA16076D407B7F1156AF4C4340995554EEB4076D40A99CEC56F54A4340775771D552086D403926FB3B3B494340BB4555B0F0086D40CD90190581474340576E107F8E096D40994324B2C6454340161CB3412C0A6D403C98F7420C444340BA964DF8C90A6D40A4DA6FB751424340FA22F0A2670B6D40EF48690F974043409D02AB41050C6D403E13C04ADC3E43407C748ED4A20C6D40A75B5069213D43409FB4AA5B400D6D400036F66A663B43403BFC0FD7DD0D6D40C9A78D4FAB394340B816C655100E6D4046B37C30003943408747F6F37F0D6D402F3058040B37434031BAE520F20C6D408DED95671E354340EF11A460640C6D4016AC8EBB31334340C14F20B3D60B6D407B6C1C0045314340F57D4918490B6D40AC2C1935582F434012B00E90BB0A6D40AFE75E5A6B2D4340CA025F1A2E0A6D408395C76F7E2B4340E79B29B7A0096D40012B2D759129434041AA5D6613096D40BE99696AA4274340A965EA2786086D40E4CF564FB7254340D70EBFFBF8076D4011B8CE23CA23434062EFCAE16B076D404039ABE7DC214340A859FDD9DE066D409D36C69AEF1F4340BFA845E451066D406D8FF93C021E43406F409300C5056D40E31E1FCE141C4340148DD52E38056D400CBC104E271A43409803FC6EAB046D40A439A8BC391843406321F6C01E046D40F865BF194C164340436CB32492036D40CB0A30655E1443406672239A05036D402CEDD39E701243404ACA352179026D405ACD84C682104340A712DAB9EC016D40A0661CDC940E434061F2FF6360016D403D6F74DFA60C43408318971FD4006D40349866D0B80A4340203C8FEC47006D403A8DCCAECA0843404E1CD8CABBFF6C4088F47F7ADC064340188061BA2FFF6C40C86E5A33EE04434067361BBBA3FE6C40E19635D9FF024340F815F5CC17FE6C40E901EB6B1101434054FDDEEF8BFD6C40FB3E54EB22FF4240A8D2C82300FD6C4013D74A5734FD4240DB83A26874FC6C40F14CA8AF45FB42405F065CBEE8FB6C40FA1C46F456F942403757E5245DFB6C400CBDFD2468F74240D77A2E9CD1FA6C40699CA84179F542402C7D272446FA6C409423204A8AF342406F71C0BCBAF96C4022B43D3E9BF142403372E9652FF96C40ACA8DA1DACEF42404AA1921FA4F86C409E54D0E8BCED4240B327ACE918F86C401B04F89ECDEB4240913526C48DF76C40E1FB2A40DEE942401B02F1AE02F76C40187942CCEEE7424090CBFCA977F66C4047B11743FFE5424024D739B5ECF56C401BD283A40FE44240F77098D061F56C40530160F01FE2424001EC08FCD6F46C409C5C852630E0424004A27B374CF46C4065F9CC4640DE42408DF3E082C1F36C40D2E40F5150DC4240CA4729DE36F36C408623274560DA4240940C4549ACF26C4089B1EB2270D8424057B624C421F26C40238236EA7FD6424005C0B84E97F16C40C47FE09A8FD4424006ABF1E80CF16C40CF8BC2349FD242402FFFBF9282F06C40877EB5B7AED04240B34A144CF8EF6C40E9269223BECE42401022DF146EEF6C40884A3178CDCC4240032011EDE3EE6C4068A56BB5DCCA424084E59AD459EE6C40E0E919DBEBC84240A9196DCBCFED6C4075C014E9FAC64240A16978D145ED6C40BAC734DF09C54240A588ADE6BBEC6C40259452BD18C34240ED2FFD0A32EC6C40FBAF468327C142409B1E583EA8EB6C401C9BE93036BF4240B519AF801EEB6C40EDCA13C644BD424014ECF2D194EA6C402EAA9D4253BB4240576614320BEA6C40DB985FA661B94240D15E04A181E96C4005EC31F16FB742408BB1B31EF8E86C40AEEDEC227EB542401D4013AB6EE86C40B2DC683B8CB34240BDF11346E5E76C408EEC7D3A9AB142401AB3A6EF5BE76C4050450420A8AF42406076BCA7D2E66C406703D4EBB5AD42401C33466E49E66C408B37C59DC3AB424039E63443C0E56C408EE6AF35D1A94240F691792637E56C4038096CB3DEA74240C73D0518AEE46C40318CD116ECA542405DF6C81725E46C40CD4FB85FF9A3424089CDB5259CE36C40F227F88D06A242403DDABC4113E36C40F1DB68A113A042406C38CF6B8AE26C406226E299209E42401309DEA301E26C40FBB43B772D9C42401A72DAE978E16C4076284D393A9A4240529EB53DF0E06C406014EEDF4698424062BD609F67E06C4002FFF56A53964240BB03CD0EDFDF6C402D613CDA5F94424092AAEB8B56DF6C4026A6982D6C924240C6EFAD16CEDE6C40732BE26478904240DE1505AF45DE6C40C040F07F848E4240F863E254BDDD6C40B2279A7E908C4240C425370835DD6C40C713B7609C8A42405FABF4C8ACDC6C40342A1E26A888424069490C9724DC6C40B181A6CEB3864240DD586F729CDB6C406D22275ABF8442400F370F5B14DB6C40CC0577C8CA824240A045DD508CDA6C4056166D19D68042406FEACA5304DA6C40882FE04CE17E42408C8FC9637CD96C40B31DA762EC7C42402EA3CA80F4D86C40D09D985AF77A42409F97BFAA6CD86C40605D8B34027942403DE399E1E4D76C4048FA55F00C77424062004B255DD76C409D02CF8D17754240556DC475D5D66C408EF4CC0C227342404E514F81A0D66C40C39C91B681724240FC90B6A701D66C4072F680F43A744240F2677B1A65D56C4093688389ED754240E3B3CA0156D56C4060C8006B177642404459121DFBD46C40DD85B73320774240");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0216448475G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EW0216448475G,EW0217048515G,4.91352,1.55198977,3.36153448,0.47496549,0.97554515,1.03267099,10.43268704,22.51434093,564.16990433,37.56218436,1.60145239,1.50252716,0.58652583,0.17108451,0.15020150,0.09691715,0.27501299,0.63051478,0.55681203,0.59213249,2011164,2011171,238.35767715,-6.71133408,212.89484149,4.13352441,2439400,2439400,269.41911712,-36.88275250,13852.81211698,226.60454805,-36.90494157,13413.11418725,222.46839323,4.84089375,2169.38215315,2100.74859611,48.99193964,51.12331372,58.26948332,33.42169024,95.24883035,63.25366930,0103000000010000000D0100004ED6586625116C40EE314FE3424C3EC0B9FE22132EF96B402BD1E5D154003EC0861F9D36F6E06B408582CBA51EB23DC0A292026A79C86B401A475AC584613DC07D40DCD7B2AF6B40F090F6EF680E3DC0F409A22C9D966B402FFCFCE0A9B83CC0C46F8284327D6B408C3F96E122603CC0C354A4556C636B40457F6F45AB043CC074A9045643496B4026392CCD15A63BC07B30CE5BAF2E6B40AC36D6E72F443BC0A006AD36A7136B4052146CCAC0DE3AC06A441F8020F86A40A9B2B75188753AC0B9590F600FDC6A40FBBA699E3D083AC0D712024266BF6A402EA97F558D9639C0D00EA97515A26A40D5894866172039C02D6079B30A846A404B4A712A6CA438C08F60907A30656A409FD5DDA0082338C0238CFC376D456A40EBB37565519B37C01AE0241FA2246A40B5F3EAD68B0C37C01B972B8DA9026A406F96138AD47536C0A26345B954DF6940CA9E429E11D635C0FAD6FB4868BA69402F33F189DE2B35C0A662C00D97936940D1935C196D7534C0428A838C796A6940B00A339553B033C0E1ABCF887F3E6940949EF7D337D932C08FF60D6DD50E6940DFCDDE4433EB31C0F366BF2E2EDA6840D3F0922C96DE30C06DF9D315449E6840C8EB7576004C2FC08686705B60566840BE0470D59A4B2CC019928F1920FB6740E53AB599276128C0ACBA7F0A9CF76740E830DA6EE32C28C06A31F56163E66740A2AB0483472B27C02E26D72FEAD467401CD6DBF64D2426C0F7C4D2532BC367408ED0AC32A51725C0760A062321B1674040586C08F30424C0562E5252C59E6740CCD17964D3EB22C0019B2ADC108C6740D5FD76B9D6CB21C00643ADE0FB7867400B7F0B1580A420C00FF26B7E7D656740D75EF38A85EA1EC04DE3ECD879616740EE21B29C066D1EC0EE3CB3A85965674019402C2E46C51DC0B447E85F26706740E64C76793EF21BC03050FB74F57A6740CE000538791F1AC0D4F7AC83C7856740569E0462EF4C18C0566AE55B9E906740460AF83F667A16C0F4FB8B16819B674002E586CAC0A614C02148F66159A6674011F2D14AD0D512C0E372BE9D43B167402113B78BE70211C0B9DFD89724BC6740C3C393134A650EC09D04C8EB1CC767403AAC1C895ABF0AC0AEDB880419D267407CE8CAE0AF1A07C013FF1D441ADD6740177881E1FA7603C09F49744821E867407B24E2E65BA8FFBFD9D022B02EF3674075C624527764F8BFA7B8D64743FE6740365C3AE30F22F1BFE554C15C84096840D5FF118B4A92E3BF6BE7C562A21468405C31ED76846FC4BF9108BC9FFA1F68408BF2E8C2862FD33FDE62F52D4A2B6840FE0997910F31E83F2BED8FC28036684023608AA7604DF33F93864291EF416840F7A87EC83C9EFA3FB3BE80BF644D684045FAE21B5AF50040A915A37AE358684014BC0A0F3F9A04408E22B0136D6468401E3762660E3E084074425F7643706840E7BD217C41F50B406184CA99DD7B684087A3DFA5AD940F40F49B33A0C9876840BBA493BE44A411404AD815447A936840BD10588565721340C3CC9DE6849F68407FDDB847B24B1540419E03DF98AB68403A50980B99231740A7E5DF8EB9B768405E256FC588FA18406D1876B1E7C3684089C9D17B87D01A400F33110424D06840DDDB6C049BA51C409FEC338C70DC684039305958F9791E40632B97502AE96840E2637E1B9E2D2040714C66398FF56840130D9B2751162140832F852D6F026940F8B7D2A947062240325A685D5D0F69402D8D75A873F52240B3BF7E635C1C6940683AE354F9E323409BEA2C156D29694086E7D872DBD1244022D3944A90366940B042EDA21CBF25404AFAC031CB436940C293AD7E0CAC2640DD30A9FE925169400235F1C8B3A027404163BB18ED5E69408D704635A68B2840C37F4FDBE76C6940FA660FF9687F2940861EC828F37A6940C58C7BA7FC712A401173E5CB148969408C58B443B8632B40945B44BF4D976940367AEF109E542C40BF1C48019FA569409498E520B0442D402950921B0DB46940671C93922A342E40FDBE31C72CC369408DD2D840792C2F4061D78195C6D16940B40636D9CA0C30402D16BD7221E169405F1F78CA158830403737B67F97F069406BBEA9F4CA0231405EBA9D402D006A40A4708AAB057D314028ED6A4499106A400FCF75633AFC3140E698568E68206A40CE9A1F4827753240FEFF35D31E316A40386B505157F332405833390EF3416A40CADDC08CB670334081AA88D9EA526A404FE2F4C065ED3340D97B499507646A403064213A65693440D68760E64E756A40F2C74130D3E43440CC78C76E90876A40928A88D84E653540FC566F371F996A408969151D04DF354028AD8FDFB9AB6A40606E2637FF5D36402B98DD4171BF6A404310759D5BE2364091A6312A63D26A40630EC717695F3740DB0A79297CE66A407349E680CEE1374048924269CEF96A4040AD02CCFB5C3840776679AE500E6B40FA915F1569DD3840B8D0D19316246B40A772BD3B2963394077A21CE604396B40F5D00BEA3EE1394075E46AF6434F6B407BA76A4C9A643A40C825C14EC1656B40245DB825A6E63A40E176B1C3637B6B40362AD9872C613B40DCF70DB737916B40C6A8C02A43DA3B403B5305E019A66B40004B9D4AC24B3C408D133A830EBA6B4075D6A3B416B63C404126B4047ACF6B4063731B19F0253D40C7F02CEE36E66B40D67320681D9A3D4046DBD68C18FC6B40652866A376073E401A8A6DE332126C40B168E9A982733E40042D914611266C4021CA6CA898D23E407FA308F21D3A6C406CF8CE669D303F40185CBD20CA4F6C408C2A0BC316943F40FFFA7C39A4656C4079B94D1526F63F402B0C6ADA427A6C40AC9FDBB8522840409B3D2DCBFF8D6C406AB8989CB95240400464C71CE7A16C4048516C2F977C4040E57D786BFBB56C400568CEFCEDA540407F9E3BD4CECB6C401DDA6B61DBD14040D9A184413AE06C40033F814204FA4040274996F772F66C4039C0585CB4244140B189839EDC0C6D400789CFCDB34E4140C9AC6A3D7A236D4006A10E0B05784140B940CD959B376D40C80EC58CE99B4140D9B33220E54B6D40B683CFF740BF41406C8DD64B18626D40E04653D9FBE44140DEE729DBB2766D40DC84167721074240EC3D1796428D6D4020351B01932B424023F2F083FFA36D40F7FF3066504F4240BC91B8A2EBBA6D40E99175585A72424093DE40D243CF6D400F8B2E19A1904240A1981CC2BEE36D409306724656AE42402BF2F1DC5CF86D40A8CFD2FB78CB42402046FF621E0D6E408FB1580F08E84240E51D573404226E40465CFB23030443403BB49B4D05396E4075B29083ED214340A30C09892E506E40D318B7101F3F43407A83D0AE80676E4050E1510B975B4340FD4C4FCD1C7A6E4039A7E220A67143405EBDB520E38E6E40EA3E475197894340E6B01D4CB2A16E40BC6AC837A79E434052A81875B3B66E40CB8B9A7F7DB543403213CC84D0CB6E40657C2B12BACB4340502731B30BE16E40E35C804D5EE14340C832499155E76E40D477C0558EE74340E963BCBA45EE6E40343F4D5134E14340F62193544F026F40494110243CCE43403504FD013F166F4092D8E069BCBA4340DEB53BF92C2C6F407DA35D4D8FA44340C4D8C3D217376F40DCAB199E3B994340B93C40A211446F40B1435F42858B43401EAFB3BFB8596F40E2B0E6A707744340DACC690893666F40FF699827B9654340B40554E060736F40173CA61433574340239D129122806F409F5D876A75484340269B24A7E68E6F401A96373F0E374340A42CDAC28C9B6F40CE5EE9FCDA2743403ED43A9131AA6F40003E6A12EB1543406831BC6FCEBA6F40C4C537D71F01434060D0E5EB4DC96F400345747896EE42401928E6D4C5DB6F40AB95419471D64240EA9F69A720EE6F4074E34C4BDABD42404F438B8C2F007040BC1D49D6D0A442406C73FE29E3FA6F4070D7A87E883F4240DC4EB076A1966F40A4EC51872A583E402036FDF5286B6F400655F39EAD703B40DD477240064B6F4053AEA73C2F3539405527B4C5AD306F401671CDE7C853374093365DEAF7196F40B998810736AC3540B9C9100ECB056F402AE00BB9C82D34401DFAD20581F36E4020147EC48CCE3240A8EC935FAEE26E40B68026D300883140FFB9E60F09D36E4038FE7EB09D5530405CF4559A5BC46E407425C5152C682E404D5488EE7DB66E40A6711AFDD5412C40DABDBF2851A96E40FB701E8057342A4094F591E6BC9C6E405E9BB3ED993C2840336A8186AD906E4051172CD71B582640EDFDF7F612856E4082324662CC842440FF6F71E0DF796E40ECE84170F1C02240378FAC0B096F6E400C1F5CEE140B2140DE128FF084646E4035DE390DEEC31E4053FA57614B5A6E40C4450E7107891B403BEDF34955506E4004D6E1C095631840283CEC7D9C466E401F06B608F451154005B51D911B3D6E40AD11C2E1AE521240A2EA84B8CD336E407F90B24CF9C80E40E94B30B1AE2A6E4071C944506E0C0940A27CEEABBA216E40FB2E0522AD6D0340A358AD3CEE186E401A92B5B6B4D5FB3FCDF4C04C46106E401E079AC38F04F13F664C7A0FC0076E4023F2FDA27893D93FB6B398F858FF6D40AC6AEBCCA030D0BF936B3BB40EF76D404AE5C3131DA2ECBFC3620C20DFEE6D40748E2E422A6CF8BF3A206C45C8E66D400A42E37CD02F01C07C3C7254C8DE6D40018A4ADBB61606C09FA49E9FDDD66D40119A32B2B2EB0AC0C8BF1E9806CF6D40EE23B25D9DAF0FC053F68ECA41C76D401CEAC769A03112C01D5E25DC8DBF6D404C5B5590AC8314C04BB63588E9B76D40793E24DC4ACE16C0F296019E53B06D400145F54DCD1119C017ECC9FECAA86D4078CA98B0804E1BC0AB9C189C4EA16D40EBA5390DAD841DC09FB33A76DD996D400404F51296B41FC0A492E49A76926D40A63CB3B93DEF20C0FEB2FA23198B6D40C3F1C79A4C0122C0E7497A36C4836D4063E62700941023C0ADD37D01777C6D407A86B6AE2E1D24C0821A5ABD30756D40EEAF0CFB352725C0D0C7D0AAF06D6D40DED3D6E4C12E26C08DF55512B6666D40582BA42FE93327C07A8C6643805F6D4028157079C13628C04785ED934E586D407D4A254F5F3729C01662B65F20516D403878523FD6352AC0086BEB07F5496D40E4E540EB38322BC0B7669EF2CB426D4050DF9716992C2CC0D4AE5A8AA43B6D40217AB2B507252DC089A3BF3D7E346D40F9DDC8FA941B2EC0019D227F582D6D4030620A6250102FC0DB9437C432266D40AE40615EA40130C099D6BF850C1F6D40FAD9CF1D467A30C0DD193E3FE5176D406260183C14F230C01B7AAF6EBC106D40326AD53E156931C0AACD489491096D40DC5F84684FDF31C096EB373264026D40AD0149BCC85432C04B7A68CC33FB6C400E686B0187C932C0ADEB4BE8FFF36C409D6396C68F3D33C06053A40CC8EC6C40648EDB64E8B033C014CC51C18BE56C4012D68102962334C08427228F4ADE6C405AD2A2959D9534C044AAA2FF03D76C4040CE9AE6030735C0B69AF39CB7CF6C40A20C4F92CD7735C02C6E9DF164C86C40867C4D0CFFE735C0376467880BC16C409EC3C8A09C5736C03E632FECAAB96C40B5437376AAC636C021EDC2A742B26C4020803B902C3537C01C05B945D2AA6C40DE12ECCE26A337C020E34C5059A36C409C2DB1F29C1038C09B523951D79B6C409D78859C927D38C0039D94D14B946C408BF4874F0BEA38C0D1E1AC59B68C6C40DC633C720A5639C076BFE47016856C403B9BB74F93C139C0B531909D6B7D6C404DFCB818A92C3AC04E8BD164B5756C402A41B2E44E973AC09071764AF36D6C40C6A4BEB287013BC044C1D4D024666C40DB5F8A6A566B3BC00A45A778495E6C4093582BDDBDD43BC0E725EAC060566C40F4D2EBC5C03D3CC0B9FEB6266A4E6C406DDC07CB61A63CC02F7C202565466C40721B5E7EA30E3DC056720D35513E6C402F9E145E88763DC07B5113CD2D366C40963232D512DE3DC0AFE34F61FA2D6C400AC82C3C45453EC0A1637762C4276C4045F9580B90923EC04ED6586625116C40EE314FE3424C3EC0");
  compareCsvLine(line.getRow(2),"EW0216448475G,EW0217133530G,4.76686,1.50809563,3.25875997,0.58351076,1.03443210,1.03212696,14.52248727,26.58793922,469.56421562,42.73787364,1.60145239,1.41473888,0.58652583,0.07731474,0.15020150,0.06247440,0.27501299,0.67116971,0.50651404,0.55085569,2011164,2011172,238.35767715,-6.71133408,210.09143125,6.31175711,2439400,2439400,269.41911712,-36.88275250,13852.81211698,220.56984173,-36.76951195,13379.05456322,221.05099868,5.58222625,2169.38215315,2101.85590688,48.99193964,52.50102396,58.26948332,31.54629495,95.24883035,59.59472556,01030000000100000000010000C46F8284327D6B408C3F96E122603CC0C354A4556C636B40457F6F45AB043CC074A9045643496B4026392CCD15A63BC07B30CE5BAF2E6B40AC36D6E72F443BC0A006AD36A7136B4052146CCAC0DE3AC06A441F8020F86A40A9B2B75188753AC0B9590F600FDC6A40FBBA699E3D083AC0D712024266BF6A402EA97F558D9639C0D00EA97515A26A40D5894866172039C02D6079B30A846A404B4A712A6CA438C08F60907A30656A409FD5DDA0082338C0238CFC376D456A40EBB37565519B37C01AE0241FA2246A40B5F3EAD68B0C37C01B972B8DA9026A406F96138AD47536C0A26345B954DF6940CA9E429E11D635C0FAD6FB4868BA69402F33F189DE2B35C0A662C00D97936940D1935C196D7534C0428A838C796A6940B00A339553B033C0E1ABCF887F3E6940949EF7D337D932C08FF60D6DD50E6940DFCDDE4433EB31C0F366BF2E2EDA6840D3F0922C96DE30C06DF9D315449E6840C8EB7576004C2FC08686705B60566840BE0470D59A4B2CC057ED8EC17FF4674009F063305C1828C00BF33D9BEAC56740DB18D0DEFD0226C0C4E3D16282B76740FBD0CCD9E71425C0D462A625E9A76740E026A177B01124C0A4BD34A91B986740C68AA724A70923C0EFAB41F615886740F1E51C5F88FC21C03FC0BAB4D3776740180DF5310AEA20C0751DF11D506767401E54B0A2B6A31FC0187B657308606740DC9FA91B81AB1EC0EE3CB3A85965674019402C2E46C51DC0B447E85F26706740E64C76793EF21BC03050FB74F57A6740CE000538791F1AC0D4F7AC83C7856740569E0462EF4C18C0566AE55B9E906740460AF83F667A16C0F4FB8B16819B674002E586CAC0A614C02148F66159A6674011F2D14AD0D512C0E372BE9D43B167402113B78BE70211C0B9DFD89724BC6740C3C393134A650EC09D04C8EB1CC767403AAC1C895ABF0AC0AEDB880419D267407CE8CAE0AF1A07C013FF1D441ADD6740177881E1FA7603C09F49744821E867407B24E2E65BA8FFBFD9D022B02EF3674075C624527764F8BFA7B8D64743FE6740365C3AE30F22F1BFE554C15C84096840D5FF118B4A92E3BF6BE7C562A21468405C31ED76846FC4BF9108BC9FFA1F68408BF2E8C2862FD33FDE62F52D4A2B6840FE0997910F31E83F2BED8FC28036684023608AA7604DF33F93864291EF416840F7A87EC83C9EFA3FB3BE80BF644D684045FAE21B5AF50040A915A37AE358684014BC0A0F3F9A04408E22B0136D6468401E3762660E3E084074425F7643706840E7BD217C41F50B406184CA99DD7B684087A3DFA5AD940F40F49B33A0C9876840BBA493BE44A411404AD815447A936840BD10588565721340C3CC9DE6849F68407FDDB847B24B1540419E03DF98AB68403A50980B99231740A7E5DF8EB9B768405E256FC588FA18406D1876B1E7C3684089C9D17B87D01A400F33110424D06840DDDB6C049BA51C409FEC338C70DC684039305958F9791E40632B97502AE96840E2637E1B9E2D2040714C66398FF56840130D9B2751162140832F852D6F026940F8B7D2A947062240325A685D5D0F69402D8D75A873F52240B3BF7E635C1C6940683AE354F9E323409BEA2C156D29694086E7D872DBD1244022D3944A90366940B042EDA21CBF25404AFAC031CB436940C293AD7E0CAC2640DD30A9FE925169400235F1C8B3A027404163BB18ED5E69408D704635A68B2840C37F4FDBE76C6940FA660FF9687F2940861EC828F37A6940C58C7BA7FC712A401173E5CB148969408C58B443B8632B40945B44BF4D976940367AEF109E542C40BF1C48019FA569409498E520B0442D402950921B0DB46940671C93922A342E40FDBE31C72CC369408DD2D840792C2F4061D78195C6D16940B40636D9CA0C30402D16BD7221E169405F1F78CA158830403737B67F97F069406BBEA9F4CA0231405EBA9D402D006A40A4708AAB057D314028ED6A4499106A400FCF75633AFC3140E698568E68206A40CE9A1F4827753240FEFF35D31E316A40386B505157F332405833390EF3416A40CADDC08CB670334081AA88D9EA526A404FE2F4C065ED3340D97B499507646A403064213A65693440D68760E64E756A40F2C74130D3E43440CC78C76E90876A40928A88D84E653540FC566F371F996A408969151D04DF354028AD8FDFB9AB6A40606E2637FF5D36402B98DD4171BF6A404310759D5BE2364091A6312A63D26A40630EC717695F3740DB0A79297CE66A407349E680CEE1374048924269CEF96A4040AD02CCFB5C3840776679AE500E6B40FA915F1569DD3840B8D0D19316246B40A772BD3B2963394077A21CE604396B40F5D00BEA3EE1394075E46AF6434F6B407BA76A4C9A643A40C825C14EC1656B40245DB825A6E63A40E176B1C3637B6B40362AD9872C613B40DCF70DB737916B40C6A8C02A43DA3B403B5305E019A66B40004B9D4AC24B3C408D133A830EBA6B4075D6A3B416B63C404126B4047ACF6B4063731B19F0253D40C7F02CEE36E66B40D67320681D9A3D4046DBD68C18FC6B40652866A376073E401A8A6DE332126C40B168E9A982733E40042D914611266C4021CA6CA898D23E407FA308F21D3A6C406CF8CE669D303F40185CBD20CA4F6C408C2A0BC316943F40FFFA7C39A4656C4079B94D1526F63F402B0C6ADA427A6C40AC9FDBB8522840409B3D2DCBFF8D6C406AB8989CB95240400464C71CE7A16C4048516C2F977C4040E57D786BFBB56C400568CEFCEDA540407F9E3BD4CECB6C401DDA6B61DBD14040D9A184413AE06C40033F814204FA4040274996F772F66C4039C0585CB4244140B189839EDC0C6D400789CFCDB34E4140C9AC6A3D7A236D4006A10E0B05784140B940CD959B376D40C80EC58CE99B4140D9B33220E54B6D40B683CFF740BF41406C8DD64B18626D40E04653D9FBE44140DEE729DBB2766D40DC84167721074240EC3D1796428D6D4020351B01932B424023F2F083FFA36D40F7FF3066504F4240BC91B8A2EBBA6D40E99175585A72424093DE40D243CF6D400F8B2E19A1904240A1981CC2BEE36D409306724656AE42402BF2F1DC5CF86D40A8CFD2FB78CB42402046FF621E0D6E408FB1580F08E84240E51D573404226E40465CFB23030443403BB49B4D05396E4075B29083ED214340A30C09892E506E40D318B7101F3F43407A83D0AE80676E4050E1510B975B4340FD4C4FCD1C7A6E4039A7E220A67143405EBDB520E38E6E40EA3E475197894340E2602E1228906E4075302F2E038B434090EEAF4F8A916E407748638E858943407BD882A1F6A06E4033E3288E7D7843400D67135E5AB26E4043C78C24D164434031EEB035A7C36E40D6BD44C0BE504340982A6FE1E3D66E40CE57F0DAD5394340D7BA0CF002EA6E403D5DC0C870224340A281E68B04FD6E40D6E63385900A4340DECCC20BE90F6F402B18DACC35F24240BFF881CCAB246F4098D341A9B9D642400D374E7BFB306F409EE765CF10C6424085C3749A82456F40F0C5B708AFA94240659260EAAF516F405BEC62E37C984240B94F7DA8CF5D6F407DC2CB2319874240CF74E817E2696F40EE2ADDCD837542407223D06BE7756F40568AE5F7BC634240A69486E4CF836F40BAE53E93D54E424072C53706A6916F402D8AE65FAD394240B3346C526A9F6F4041D04F38442442401F9F286906AF6F400ACCBF068D0B4240453FB9918ABE6F40D751CC5984F24140CD1C6D59F7CD6F4062B925462AD94140B07B4B4808C66F40C2CD42715D244140E1E041992A5C6F40077E5E8E97A53C40A1910717EA2B6F40D5FC2F0ACFE0394099EFE7D9D3076F4070D32C6492BB3740344535E711EA6E408432092F50EB35409C06E07952D06E400F24031A05523440EDD9B10363B96E40B402E11505E0324017059B5D8DA46E40D48B1100E78B3140979C29295B916E401E7E74C97D4F3040A41CBC847A7F6E4063B1800BF34C2E409919FB1CB06E6E4006C24999661B2C4018908265CF5E6E40A19A4A3990052A40DCA7F0F2B54F6E40D3AA733BB8072840E677EA8C48416E4023B7864DE81E2640EED7A24071336E4030C43397B848244098676A101E266E40CDD685DC2C832240EF9C5C0740196E4073593EE29BCC20407A04188FCA0C6E40E8B0CF3A3B471E40781A53F2B2006E406E2DE609FC0D1B40B878B2FEEFF46D40E400AC4166EB17400A76E2BC79E96D40515890D4AADD144039B9DC3849DE6D40AF4BE0B833E3114079B6215658D36D4034E47B1735F50D400F4BEBACA1C86D405284062F424508409E2A346E20BE6D400E7E009756B4024022B2034DD0B36D4084301546E980FA3F0676D56BADA96D4024CA0519449FEF3F02D93D4DB49F6D40115EE2FC6046D53FE57B22C7E1956D40E84E8DF89CF0D3BFC58F04F8328C6D40A35A6B999938EEBFCE31F83DA5826D40C68DE90A4E11F9BFDBD0F92E36796D406DCF014FB66E01C010316392E36F6D40FB62FF005E4106C044534E5BAB666D406F67184593010BC00D56BEA38B5D6D40558B75DA38B00FC0730C6EA882546D40F543747B102712C0AD082AC58E4B6D4068016175076E14C0DB9AA071AE426D40BBC74D485CAD16C0980E963EE0396D40AE9C60C863E518C0A97C6ED322316D40CC754C3F6D161BC0FEFA00EC74286D409550ACEAC2401DC0ABF3A956D51F6D404558406CAA641FC0560C94F242176D40280FF79632C120C0D75D30AEBC0E6D40A1748D5D18CD21C010D6D78541066D40A00E5D8823D622C0DA879082D0FD6C40317F9B726FDC23C0FC6FF2B868F56C4056549BF415E024C09AD8274809ED6C40D5D405812FE125C0A40D0659B1E46C4051614B3FD3DF26C0D4873B1D60DC6C40378C9B2317DC27C09B1591CE14D46C40B95DAB0410D628C076DB3BAECECB6C408F1C87AFD1CD29C0A24C3E048DC36C407C29A5F96EC32AC08078D61E4FBB6C40F6DA68D1F9B62BC0313DF85114B36C40CB843E4D83A82CC0341CD2F6DBAA6C4098EF74B91B982DC007945B6BA5A26C40E039F4A4D2852EC00503EC11709A6C40F36FEEECB6712FC08034D9503B926C406D75D163EB2D30C0DBD11C92068A6C40676025E71FA230C03A070043D1816C40BB0B21837F1531C070BECCD39A796C40A204C074108831C039E383B762716C40027636B7D8F931C08E32986328696C40918E9207DE6A32C08C23AD4FEB606C407B821AE825DB32C0B68359F5AA586C40B7DB6CA3B54A33C0366AEDCF66506C400C39684F92B933C0992E3B5C1E486C40321CDFCFC02734C02A176318D13F6C40AFF11BD9459534C0107AA1837E376C40C81739F2250235C01E121F1E262F6C40494E5077656E35C0264EC368C7266C405BA6849B08DA35C0C66508E5611E6C40D4BFE96A134536C09003D114F5156C40C6E04ACC89AF36C0C4573F7A800D6C400338D4826F1937C06B6A8D9703056C404D65A02FC88237C07086E6EE7DFC6B4076332C5397EB37C0669A4102EFF36B409344B24EE05338C0716D3C5356EB6B40F6467065A6BB38C08489F762B3E26B40DB29D7BDEC2239C081BDF2B105DA6B4032A5A762B68939C0CE1BEABF4CD16B408B4AFC4306F039C0365BB30B88C86B403C3D4238DF553AC047811B13B7BF6B40E79121FD43BB3AC07CBEC452D9B66B40A845563837203BC098650446EEAD6B40A2A17A78BB843BC031E8C066F5A46B405ED2C335D3E83BC07FC34F2DEE9B6B403965B1D2804C3CC0638C3E7423936B40C37BC2F08EAC3CC0C46F8284327D6B408C3F96E122603CC0");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0227091446G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 5);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EW0227091446G,EW0211937665G,6.02955,2.78561205,3.24393946,0.15989830,0.13612743,1.17031028,4.16441075,2.49070149,1189.88583226,3.07862505,2.89702322,2.67420089,0.40919079,0.42059730,0.85672502,0.62973500,0.64640276,0.64886516,0.40520585,0.28296941,2011287,2011112,240.56840385,34.18787049,241.40882645,38.51924937,2439400,2439400,242.38214360,37.43944410,3955.72810017,241.44610549,38.59389060,4228.33794452,241.14175884,35.94726340,273.41773462,319.98358792,9.26900090,0.18940005,61.81618413,61.58805381,61.82375105,61.60213509,01030000000100000006010000FC5F84244BCD6D407000D5C1553443406FE18454BACF6D407C719D7A62314340AA9398EA28D26D406FF7F0C76D2E43408756DBE796D46D4069DC7FA9772B4340B6FA674D04D76D409B01F81E802843408645581C71D96D40BDDF04288725434080F4C455DDDB6D407F874FC48C2243401FC1C5FA48DE6D40D3A17EF3901F43405A64710CB4E06D40447036B5931C4340449ADD8B1EE36D402ACD180995194340A4251F7A88E56D40E12BC5EE941643406FD349D8F1E76D40E098D86593134340567E70A75AEA6D40D0B9ED6D901043403F12A5E8C2EC6D408ECD9C068C0D4340C58FF89C2AEF6D4022AC7B2F860A4340970F7BC591F16D40A9C61DE87E074340F6C53B63F8F36D402E27143076044340060649775EF66D407370ED066C0143404445B002C4F86D40B7DD356C60FE4240CF1E7E0629FB6D406142775F53FB4240C056BE838DFD6D40A60939E044F8424081DD7B7BF1FF6D401B3600EE34F5424005D3C0EE54026E4044614F8823F24240238A96DEB7046E4001BBA6AE10EF4240C38B054C1A076E4009098460FCEB4240239A15387C096E4040A6629DE6E8424005B4CDA3DD0B6E400782BB64CFE54240EC1734903E0E6E407F1F05B6B6E242403A474EFE9E106E40C594B3909CDF4240620921EFFE126E40178A38F480DC42400A6FB0635E156E40E43803E063D9424026D5FF5CBD176E40E66A805345D6424019E811DC1B1A6E4017791A4E25D34240C7A6E8E1791C6E40A94A39CF03D04240AD65856FD71E6E40E35342D6E0CC4240EBD1E88534216E40F8949862BCC9424058F4122691236E40D1989C7396C642407D340351ED256E40BD73AC086FC34240A55BB80749286E402FC2232146C04240D997304BA42A6E404AA75BBC1BBD4240DB7E691CFF2C6E4079CBAAD9EFB942402811607C592F6E40EE5A6578C2B64240E7BC106CB3316E401B04DD9793B34240E36077EC0C346E4019F6603763B04240714F8FFE65366E40EEDE3D5631AD4240705153A3BE386E40F7E9BDF3FDA9424025A9BDDB163B6E4001BE280FC9A642402C15C8A86E3D6E40977BC3A792A3424055D36B0BC63F6E4005BBD0BC5AA0424096A3A1041D426E407F8A904D219D4240D7CA619573446E40156C4059E6994240EB15A4BEC9466E40B0531BDFA996424050DC5F811F496E40F8A459DE6B93424027038CDE744B6E40203131562C904240EDFF1ED7C94D6E40C034D545EB8C42406ADB0E6C1E506E40885576ACA88942407C34519E72526E40DF9F428964864240E742DB6EC6546E40958465DB1E83424024DAA1DE19576E4064D607A2D77F4240406C99EE6C596E406EC74FDC8E7C4240940CB69FBF5B6E40B7E66089447942409E72EBF2115E6E40781D5CA8F8754240CCFC2CE963606E407FAC5F38AB72424040B36D83B5626E40622987385C6F4240994AA0C206656E40BA7BEBA70B6C4240C526B7A757676E404ADAA285B9684240B25DA433A8696E400EC8C0D0656542402BBA5967F86B6E403C1156881062424089BEC843486E6E403AC870ABB95E424086A7E2C997706E4089421C39615B4240F66E98FAE6726E408D156130075842408DCEDAD635756E405E134590AB544240A3429A5F84776E407147CB574E514240F20CC795D2796E4042F3F385EF4D42405537517A207C6E40E58ABC198F4A42409296280E6E7E6E4087B11F122D47424019CD3C52BB806E40DF35156EC9434240BC4D7D4708836E40950E922C64404240775ED9EE54856E408956884CFD3C4240321B4049A1876E401649E7CC943942408178A057ED896E403C3E9BAC2A3642406346E91A398C6E40C0A68DEABE32424007330994848E6E403708A585512F42408FCDEEC3CF906E40F6F8C47CE22B4240CD8888AB1A936E40FE1BCECE7128424014BEC44B65956E40CD1C9E7AFF244240ECAF91A5AF976E4028AB0F7F8B214240E58CDDB9F9996E40C076FADA151E424056729689439C6E40D82A338D9E1A4240226FAA158D9E6E40C7698B94251742408B86075FD6A06E406FC8D1EFAA134240F0B29B661FA36E40AAC9D19D2E104240A3E8542D68A56E4096D9539DB00C4240A91821B4B0A76E40D1481DED30094240E72ABDCFA0A96E404649EBF736064240C588BBE2BAA86E404D427E01930042408EAE8C915DA76E40A473D5EFFBF741403AAF02F000A66E40A5F72DE163EF4140AEC3C9FCA4A46E405FDB7BD1CAE641400A1590B649A36E401BC6AFBC30DE41404BB6051CEFA16E4087EAB69E95D54140E59DDC2B95A06E40A6F77A73F9CC41407F9FC8E43B9F6E409509E2365CC44140B0657F45E39D6E40509ACEE4BDBB4140D76BB84C8B9C6E4033721F791EB34140F3F72CF9339B6E407298AFEF7DAA414093149849DD996E4056435644DCA14140D08AB63C87986E406BC8E6723999414053DC46D131976E40828C307795904140683D0906DD956E4082F3FE4CF0874140218FBFD988946E402A5019F0497F41407F592D4B35936E4090D3425CA2764140AFC51759E2916E408D7C3A8DF96D41404898450290906E400707BB7E4F654140A82B7F453E8F6E40F2DA7A2CA45C4140396A8E21ED8D6E4047FB2B92F7534140F0C83E959C8C6E40BFF47BAB494B4140B2415D9F4C8B6E4071CC13749A424140D14DB83EFD896E4030EE97E7E93941408DE01F72AE886E40D11AA80138314140B661653860876E403256DFBD842841402FA85B9012866E401DD5D317D01F4140AAF4D678C5846E40FBEA160B1A17414040ECACF078836E4056F73493620E41403793B4F62C826E402053B5ABA9054140B947C689E1806E40DB3D1A50EFFC4040A6BCBBA8967F6E4079CAE07B33F440405BF46F524C7E6E4022CC802A76EB4040923BBF85027D6E40A6C26C57B7E2404040248741B97B6E40D6C611FEF6D940408380A684707A6E409876D71935D14040915DFD4D28796E40BFE01FA671C84040B3FE6C9CE0776E40BB70479EACBF404041D8D76E99766E40FFD9A4FDE5B64040A98A21C452756E40340389BF1DAE404022F7512FC9746E403DC6C9FC31AB4040F231B55B23726E40E92C804EAAAF404062D3ED3E876F6E40E7096BFC0EB440409845DAD3EA6C6E400B8635F770B84040AF782A194E6A6E4076D80E41D0BC4040628F8E0DB1676E4037A21FDC2CC1404000DAB6AF13656E4068FA89CA86C5404063D153FE75626E40097A690EDEC94040FE1116F8D75F6E409647D3A932CE4040CC56AE9B395D6E409A22D69E84D240408074CDE79A5A6E40C66E7AEFD3D64040695424DBFB576E40113FC29D20DB4040AEEF63745C556E407F60A9AB6ADF4040494A3DB2BC526E40C664251BB2E34040366E61931C506E40BAAC25EEF6E74040906681167C4D6E408872932639EC4040A93A4E3ADB4A6E40BFD351C678F0404040E978FD39486E4024DB3DCFB5F44040A263B25E98456E406B8A2E43F0F84040DE88AB5CF6426E409AE3F42328FD4040EF2015F653406E4069F25B735D014140F5D79F29B13D6E403CD52833900541405E39FCF50D3B6E4037C61A65C009414030ABDA596A386E40D723EB0AEE0D41403069EB53C6356E409E794D26191241402680DEE221336E406388EFB84116414019C963057D306E40974E79C4671A414083E42ABAD72D6E4047108D4A8B1E4140A935E3FF312B6E40FD5EC74CAC224140B8DD3BD58B286E407921BFCCCA26414020B7E338E5256E40309B05CCE62A4140CA5089293E236E40AF73264C002F41405EE9DAA596206E40D9BDA74E17334140816A86ACEE1D6E40DFFE09D52B3741401864393C461B6E402B35C8E03D3B41408807A1539D186E4022DF57734D3F414003236AF1F3156E409A01298E5A434140AF1C41144A136E40522EA6326547414000EED1BA9F106E40398A34626D4B4140ED1EC8E3F40D6E406BD3331E734F414027C1CE8D490B6E403767FE67765341405F6B90B79D086E40D747E940775741408034B75FF1056E401F2244AA755B4140DFAEEC8444036E40F05259A5715F414080E3D92597006E4083EC6D336B6341403E4D2741E9FD6D40AEBBC1556267414009D47CD53AFB6D40D44C8F0D576B414011C881E18BF86D40D5F00B5C496F4140F0DCDC63DCF56D40BEC1674239734140DF24345B2CF36D4074A7CDC126774140D60B2DC67BF06D400F5C63DB117B4140B1526CA3CAED6D4033704990FA7E4140590A96F118EB6D403D4F9BE1E0824140D68E4DAF66E86D403B436FD0C4864140758235DBB3E56D40D478D65DA68A4140D3C8EF7300E36D40FC02DD8A858E4140F5811D784CE06D4090DE89586292414051055FE697DD6D40CDF5DEC73C964140E3DC53BDE2DA6D409223D9D9149A41401AC09AFB2CD86D409736708FEA9D4140EF8ED19F76D56D4075F496E9BDA14140D04C95A8BFD26D407C1C3BE98EA54140931B821408D06D408E6A458F5DA94140723633E24FCD6D409E9999DC29AD4140EAEC421097CA6D40456616D2F3B041409C9D4A9DDDC76D400B919570BBB441402EB1E28723C56D409CE0EBB880B841402595A2CE68C26D40E323E9AB43BC4140ABB62070ADBF6D40EC33584A04C04140627DF26AF1BC6D40B6F5FE94C2C341401846ACBD34BA6D40DC5B9E8C7EC74140955DE16677B76D401D68F23138CB414038FB2365B9B46D40B72CB285EFCE4140BB3B05B7FAB16D40ABCD8F88A4D24140B71B155B3BAF6D40E281383B57D641405F72E24F7BAC6D401B94549E07DA4140FCEBFA93BAA96D40C96387B2B5DD41408604EB25F9A66D40CC656F7861E1414017023E0437A46D40F724A6F00AE541406FEF7D2D74A16D408F42C01BB2E841408FC79B8F4AA16D403E8553FAE8E841407691EA27D1A16D405DA0FE7D44ED4140A7B83D0FC0A26D4076165134FEF441405B611E0FAFA36D408C5F9766B6FC4140E1BC29289EA46D40D3D396186D044240FCFAFC5A8DA56D40CD3E104E220C4240384C35A87CA66D40BAEEBF0AD61342405FE46F106CA76D40D3C35D52881B4240E0FC49945BA86D406A3F9D28392342402ED760344BA96D40D8922D91E82A424028BF51F13AAA6D404CAEB98F96324240790DBACB2AAB6D40704FE827433A4240FF2937C41AAC6D40EF0F5C5DEE4142401C8E66DB0AAD6D40C873B3339849424028C7E511FBAD6D408DF788AE40514240C1785268EBAE6D40771E73D1E75842402C5F4ADFDBAF6D405C8004A08D604240B3516B77CCB06D4073D7CB1D32684240FF445331BDB16D40120E544ED56F42406C4DA00DAEB26D40304C24357777424072A1F00C9FB36D40DB04C0D5177F4240EF9BE22F90B46D407D03A733B78642408FBE147781B56D4011795552558E424013B425E372B66D402D094435F2954240BE52B47464B76D40FAD6E7DF8D9D42409F9E5F2C56B86D40FC91B25528A54240EFCBC60A48B96D40D982129AC1AC42406B4189103ABA6D40E49772B059B44240AD9A463E2CBB6D409C713A9CF0BB424084AA9E941EBC6D401B6FCE6086C34240487D311411BD6D404BBA8F011BCB4240415B9FBD03BE6D402254DC81AED24240FACA8891F6BE6D40A6200FE540DA424098938E90E9BF6D40F1F27F2ED2E142403FBF51BBDCC06D400A99836162E94240659D7312D0C16D40ACE76B81F1F042403CC59596C3C26D40F4C587917FF8424005185A48B7C36D40FB3823950C00434073C36228ABC46D404F6F878F980743400F4452379FC56D4059CCFA83230F43409667CB7593C66D40BAF3C075AD1643405B4F71E487C76D4079D41A68361E4340B572E7837CC86D4033B4465EBE25434057A1D15471C96D401F3A805B452D4340BF05D45766CA6D40147A0063CB344340B12D7F6DD0CA6D407F35F4CB54374340FC5F84244BCD6D407000D5C155344340");
  compareCsvLine(line.getRow(2),"EW0227091446G,EW0211981114G,5.97074,2.75880380,3.21193634,0.15986329,0.10714100,1.18050161,2.85710933,35.68205668,444.85831258,8.25663884,2.89702322,2.62058439,0.40919079,0.37737797,0.85672502,0.63091750,0.64640276,0.63786601,0.38701305,0.26690951,2011287,2011112,240.56840385,34.18787049,238.32114291,38.16027593,2439400,2439400,242.38214360,37.43944410,3955.72810017,238.29478020,38.25876597,4243.90040739,239.64769579,35.75724660,273.41773462,322.77007837,9.26900090,0.23670015,61.81618413,62.45244053,61.82375105,62.59205831,010300000001000000EF000000FC5F84244BCD6D407000D5C1553443406FE18454BACF6D407C719D7A62314340AA9398EA28D26D406FF7F0C76D2E43408756DBE796D46D4069DC7FA9772B4340B6FA674D04D76D409B01F81E802843408645581C71D96D40BDDF04288725434080F4C455DDDB6D407F874FC48C2243401FC1C5FA48DE6D40D3A17EF3901F43405A64710CB4E06D40447036B5931C4340449ADD8B1EE36D402ACD180995194340A4251F7A88E56D40E12BC5EE941643406FD349D8F1E76D40E098D86593134340567E70A75AEA6D40D0B9ED6D901043403F12A5E8C2EC6D408ECD9C068C0D4340C58FF89C2AEF6D4022AC7B2F860A4340970F7BC591F16D40A9C61DE87E074340F6C53B63F8F36D402E27143076044340060649775EF66D407370ED066C0143404445B002C4F86D40B7DD356C60FE4240CF1E7E0629FB6D406142775F53FB4240C056BE838DFD6D40A60939E044F8424081DD7B7BF1FF6D401B3600EE34F5424005D3C0EE54026E4044614F8823F24240238A96DEB7046E4001BBA6AE10EF4240C38B054C1A076E4009098460FCEB4240239A15387C096E4040A6629DE6E8424005B4CDA3DD0B6E400782BB64CFE54240EC1734903E0E6E407F1F05B6B6E242403A474EFE9E106E40C594B3909CDF4240620921EFFE126E40178A38F480DC42400A6FB0635E156E40E43803E063D9424026D5FF5CBD176E40E66A805345D6424019E811DC1B1A6E4017791A4E25D34240C7A6E8E1791C6E40A94A39CF03D04240AD65856FD71E6E40E35342D6E0CC4240EBD1E88534216E40F8949862BCC9424058F4122691236E40D1989C7396C642407D340351ED256E40BD73AC086FC34240A55BB80749286E402FC2232146C04240D997304BA42A6E404AA75BBC1BBD4240DB7E691CFF2C6E4079CBAAD9EFB942402811607C592F6E40EE5A6578C2B64240E7BC106CB3316E401B04DD9793B34240E36077EC0C346E4019F6603763B04240714F8FFE65366E40EEDE3D5631AD4240705153A3BE386E40F7E9BDF3FDA9424025A9BDDB163B6E4001BE280FC9A642402C15C8A86E3D6E40977BC3A792A3424055D36B0BC63F6E4005BBD0BC5AA0424096A3A1041D426E407F8A904D219D4240D7CA619573446E40156C4059E6994240EB15A4BEC9466E40B0531BDFA996424050DC5F811F496E40F8A459DE6B93424027038CDE744B6E40203131562C904240EDFF1ED7C94D6E40C034D545EB8C42406ADB0E6C1E506E40885576ACA88942407C34519E72526E40DF9F428964864240E742DB6EC6546E40958465DB1E83424024DAA1DE19576E4064D607A2D77F4240406C99EE6C596E406EC74FDC8E7C4240940CB69FBF5B6E40B7E66089447942409E72EBF2115E6E40781D5CA8F8754240C5B06EF154606E4003FE6C86C07242405ECF7308795F6E405293CFF7426D42401DF890051D5E6E406549720B8D644240A7170FC7C15C6E40E05B5870D65B4240F394794B675B6E40818D77221F53424085455E910D5A6E409D40C31D674A4240D1654D97B4586E401B692C5EAE4142408E91D95B5C576E40487EA1DFF438424027BC97DD04566E409E6C0E9E3A30424055291F1BAE546E406C875C957F274240AB65091358536E40707A72C1C31E4240523FF2C302526E40403B341E07164240CABE772CAE506E40BFFA82A7490D4240C41F3A4B5A4F6E4058163D598B04424008CADB1E074E6E4025093E2FCCFB4140784A01A6B44C6E40095D5E250CF34140174C51DF624B6E409C9B73374BEA4140359174C9114A6E40033F506189E1414095EC1563C1486E40B6A2C39EC6D84140BB3AE2AA71476E400DF499EB02D041402F5B889F22466E40C6229C433EC74140F929B93FD4446E4066D18FA278BE41400279278A86436E4077453704B2B54140A509887D39426E40AC575164EAAC414039869118ED406E40DD6399BE21A44140B97BFC59A13F6E40E238C70E589B41407D538340563E6E4059088F508D924140EA4CE2CA0B3D6E403256A17FC18941405477D7F7C13B6E4031E8AA97F4804140D0AB22C6783A6E403CB55494267841402387853430396E4084D44371576F4140B963C341E8376E40926C192A87664140B453A1ECA0366E4023A272BAB55D4140F41AE6335A356E40E386E81DE35441403A295A1614346E40060810500F4C41405E94C792CE326E40A8DC794C3A4341408012FAA789316E401974B20E643A41404CF4BE5445306E40E3E341928C3141404E1FE597012F6E40CDD5ABD2B328414057083D70BE2D6E4082756FCBD91F4140D4AD98DC7B2C6E403B5E0778FE1641405092CBDB392B6E401588E9D3210E4140E7B6AA6CF8296E404D3587DA43054140D9950C8EB7286E4054DF4C8764FC4040181DC93E77276E409323A2D583F34040E0A8B97D37266E402EB0E9C0A1EA404069FEB849F8246E4064308144BEE140408C46A3A1B9236E40DB38C15BD9D84040860856847B226E40B633FD01F3CF4040B524B0F03D216E40684C83320BC740406CCF91E500206E40615B9CE821BE4040C18BDC61C41E6E407AD18B1F37B540406B267364881D6E4031A38FD24AAC4040ACB039EC4C1C6E40A533E0FC5CA340403E7B15F8111B6E405D3FB0996D9A40404311ED86D7196E40DDC62CA47C9140405033A8979D186E40E6F87C178A88404060D22F2964176E40981CC2EE957F404074D109D7DE166E40A8FD137E997C40402562B64C2E146E401116FFCAEB804040755B88AA87116E40A72DD2F32A8540406692C0C2E00E6E405E1B855967894040439B0A94390C6E40966D4AFEA08D40401555121D92096E4094EE4DE4D791404063E4835CEA066E4006B1B40D0C964040FFAD0B5142046E405A1C9D7C3D9A4040D15156F999016E40D0F81E336C9E4040AFA51054F1FE6D40487B4B3398A240403AB0E75F48FC6D40F4502D7FC1A64040BEA3881B9FF96D40C7AAC818E8AA40400DD9A085F5F66D409C481B020CAF404080CADD9C4BF46D4052841C3D2DB34040D00EED5FA1F16D40805CBDCB4BB7404016547CCDF6EE6D401A7FE8AF67BB4040C15A39E44BEC6D40D85382EB80BF404094F0D1A2A0E96D406306698097C34040A3EBF307F5E66D405A907470ABC740405A254D1249E46D401DC376BDBCCB404086758BC09CE16D4064513B69CBCF404059AD5C11F0DE6D40C0D88775D7D3404081926E0343DC6D40B9EA1BE4E0D7404030DA6E9595D96D40F215B1B6E7DB40403B240BC6E7D66D40FCEEFAEEEBDF404020F6F09339D46D400319A78EEDE340402AB6CDFD8AD16D404F4E5D97ECE740408FA64E02DCCE6D409568BF0AE9EB404079E020A02CCC6D40226969EAE2EF4040314FF1D57CC96D40C180F137DAF3404043AB6CA2CCC66D409C17E8F4CEF7404092753F041CC46D40C3D4D722C1FB40407CF215FA6AC16D40B5A545C3B0FF4040FF249C82B9BE6D409DC5B0D79D034140D7C97D9C07BC6D405DC4926188074140AB52664655B96D40A28D5F62700B41401CE1007FA2B66D40906F85DB550F41400442F844EFB36D406B216DCE3813414079E8F6963BB16D4002CA793C1917414008E9A67387AE6D4000060927F71A4140CCF4B1D9D2AB6D4011EE728FD21E41409254C1C71DA96D40CB1C0A77AB224140F4E37D3C68A66D408AB41BDF812641407B0C9036B2A36D401865EFC8552A4140BFC09FB4FBA06D402571C735272E4140807754B5449E6D40A4B3E026F6314140BE2655378D9B6D40FEA4729DC2354140D53E4839D5986D401760AF9A8C3941408BA5D3B91C966D402FA7C31F543D41402DB19CB763936D40A4E8D62D194141409B234831AA906D4086430BC6DB444140BB67A5D6248E6D40E4009A6F534841401D6715144A8E6D40779115828F494140B26D1586378F6D403DF896966C5141401769AB0325906D407C2D4FD34759414018EA768D12916D401935723C21614140EB4A172400926D40A0182ED6F8684140D3B12BC8ED926D40AFFAAAA4CE704140C913537ADB936D403D2A0BACA27841401D372C3BC9946D408E356BF07480414008B6550BB7956D4009FDE175458841404C016EEBA4966D40C5C5804014904140C16213DC92976D40E74B5354E1974140F3FFE3DD80986D40C8D45FB5AC9F41409EDC7DF16E996D40F240A76776A7414046DD7E175D9A6D40C81D256F3EAF4140B8C984504B9B6D4029B7CFCF04B741408B4F2D9D399C6D40BF28988DC9BE4140AB0416FE279D6D402B6F6AAC8CC64140CC69DC73169E6D40FD782D304ECE4140ECEC1DFF049F6D407E37C31C0ED64140D0EB77A0F39F6D4050AF0876CCDD414070B68758E2A06D40CD08D63F89E541407691EA27D1A16D405DA0FE7D44ED4140A7B83D0FC0A26D4076165134FEF441405B611E0FAFA36D408C5F9766B6FC4140E1BC29289EA46D40D3D396186D044240FCFAFC5A8DA56D40CD3E104E220C4240384C35A87CA66D40BAEEBF0AD61342405FE46F106CA76D40D3C35D52881B4240E0FC49945BA86D406A3F9D28392342402ED760344BA96D40D8922D91E82A424028BF51F13AAA6D404CAEB98F96324240790DBACB2AAB6D40704FE827433A4240FF2937C41AAC6D40EF0F5C5DEE4142401C8E66DB0AAD6D40C873B3339849424028C7E511FBAD6D408DF788AE40514240C1785268EBAE6D40771E73D1E75842402C5F4ADFDBAF6D405C8004A08D604240B3516B77CCB06D4073D7CB1D32684240FF445331BDB16D40120E544ED56F42406C4DA00DAEB26D40304C24357777424072A1F00C9FB36D40DB04C0D5177F4240EF9BE22F90B46D407D03A733B78642408FBE147781B56D4011795552558E424013B425E372B66D402D094435F2954240BE52B47464B76D40FAD6E7DF8D9D42409F9E5F2C56B86D40FC91B25528A54240EFCBC60A48B96D40D982129AC1AC42406B4189103ABA6D40E49772B059B44240AD9A463E2CBB6D409C713A9CF0BB424084AA9E941EBC6D401B6FCE6086C34240487D311411BD6D404BBA8F011BCB4240415B9FBD03BE6D402254DC81AED24240FACA8891F6BE6D40A6200FE540DA424098938E90E9BF6D40F1F27F2ED2E142403FBF51BBDCC06D400A99836162E94240659D7312D0C16D40ACE76B81F1F042403CC59596C3C26D40F4C587917FF8424005185A48B7C36D40FB3823950C00434073C36228ABC46D404F6F878F980743400F4452379FC56D4059CCFA83230F43409667CB7593C66D40BAF3C075AD1643405B4F71E487C76D4079D41A68361E4340B572E7837CC86D4033B4465EBE25434057A1D15471C96D401F3A805B452D4340BF05D45766CA6D40147A0063CB344340B12D7F6DD0CA6D407F35F4CB54374340FC5F84244BCD6D407000D5C155344340");
  compareCsvLine(line.getRow(3),"EW0227091446G,EW0211894196G,5.84203,2.81895988,3.02306603,0.15985270,0.19028449,1.17999916,5.11611237,4.44943807,902.22714674,4.09078150,2.89702322,2.74089655,0.40919079,0.47564676,0.85672502,0.62981055,0.64640276,0.66098771,0.30853232,0.21283308,2011287,2011111,240.56840385,34.18787049,244.43138952,38.20574896,2439400,2439400,242.38214360,37.43944410,3955.72810017,244.46594015,38.28291550,4243.16132136,241.91233559,36.04825879,273.41773462,322.63269890,9.26900090,0.19242223,61.81618413,60.48706478,61.82375105,60.51110527,010300000001000000F10000005A64710CB4E06D40447036B5931C4340449ADD8B1EE36D402ACD180995194340A4251F7A88E56D40E12BC5EE941643406FD349D8F1E76D40E098D86593134340567E70A75AEA6D40D0B9ED6D901043403F12A5E8C2EC6D408ECD9C068C0D4340C58FF89C2AEF6D4022AC7B2F860A4340970F7BC591F16D40A9C61DE87E074340F6C53B63F8F36D402E27143076044340060649775EF66D407370ED066C0143404445B002C4F86D40B7DD356C60FE4240CF1E7E0629FB6D406142775F53FB4240C056BE838DFD6D40A60939E044F8424081DD7B7BF1FF6D401B3600EE34F5424005D3C0EE54026E4044614F8823F24240238A96DEB7046E4001BBA6AE10EF4240C38B054C1A076E4009098460FCEB4240239A15387C096E4040A6629DE6E8424005B4CDA3DD0B6E400782BB64CFE54240EC1734903E0E6E407F1F05B6B6E242403A474EFE9E106E40C594B3909CDF4240620921EFFE126E40178A38F480DC42400A6FB0635E156E40E43803E063D9424026D5FF5CBD176E40E66A805345D6424019E811DC1B1A6E4017791A4E25D34240C7A6E8E1791C6E40A94A39CF03D04240AD65856FD71E6E40E35342D6E0CC4240EBD1E88534216E40F8949862BCC9424058F4122691236E40D1989C7396C642407D340351ED256E40BD73AC086FC34240A55BB80749286E402FC2232146C04240D997304BA42A6E404AA75BBC1BBD4240DB7E691CFF2C6E4079CBAAD9EFB942402811607C592F6E40EE5A6578C2B64240E7BC106CB3316E401B04DD9793B34240E36077EC0C346E4019F6603763B04240714F8FFE65366E40EEDE3D5631AD4240705153A3BE386E40F7E9BDF3FDA9424025A9BDDB163B6E4001BE280FC9A642402C15C8A86E3D6E40977BC3A792A3424055D36B0BC63F6E4005BBD0BC5AA0424096A3A1041D426E407F8A904D219D4240D7CA619573446E40156C4059E6994240EB15A4BEC9466E40B0531BDFA996424050DC5F811F496E40F8A459DE6B93424027038CDE744B6E40203131562C904240EDFF1ED7C94D6E40C034D545EB8C42406ADB0E6C1E506E40885576ACA88942407C34519E72526E40DF9F428964864240E742DB6EC6546E40958465DB1E83424024DAA1DE19576E4064D607A2D77F4240406C99EE6C596E406EC74FDC8E7C4240940CB69FBF5B6E40B7E66089447942409E72EBF2115E6E40781D5CA8F8754240CCFC2CE963606E407FAC5F38AB72424040B36D83B5626E40622987385C6F4240994AA0C206656E40BA7BEBA70B6C4240C526B7A757676E404ADAA285B9684240B25DA433A8696E400EC8C0D0656542402BBA5967F86B6E403C1156881062424089BEC843486E6E403AC870ABB95E424086A7E2C997706E4089421C39615B4240F66E98FAE6726E408D156130075842408DCEDAD635756E405E134590AB544240A3429A5F84776E407147CB574E514240F20CC795D2796E4042F3F385EF4D42405537517A207C6E40E58ABC198F4A42409296280E6E7E6E4087B11F122D47424019CD3C52BB806E40DF35156EC9434240BC4D7D4708836E40950E922C64404240775ED9EE54856E408956884CFD3C4240321B4049A1876E401649E7CC943942408178A057ED896E403C3E9BAC2A3642406346E91A398C6E40C0A68DEABE32424007330994848E6E403708A585512F42408FCDEEC3CF906E40F6F8C47CE22B4240CD8888AB1A936E40FE1BCECE7128424014BEC44B65956E40CD1C9E7AFF244240ECAF91A5AF976E4028AB0F7F8B214240E58CDDB9F9996E40C076FADA151E424056729689439C6E40D82A338D9E1A4240226FAA158D9E6E40C7698B94251742408B86075FD6A06E406FC8D1EFAA134240F0B29B661FA36E40AAC9D19D2E104240A3E8542D68A56E4096D9539DB00C4240A91821B4B0A76E40D1481DED300942409233EEFBF8A96E409A47F08BAF054240412CAA0541AC6E40FCE08B782C024240C7FA42D288AE6E40B7F5ABB1A7FE4140299FA662D0B06E403E37093621FB41403F24C3B717B36E408A22590499F7414085A286D25EB56E40DCFA4D1B0FF44140F942DFB3A5B76E406BC4967983F04140EB41BB5CECB96E40023FDF1DF6EC414092F234BDAEBA6E40748677B675EB41407BACCF5B9BB96E404735403003E441409B9F6C698CB86E40FE833598ABDC41409B08D4D27DB76E403CE4C6C052D54140979556976FB66E406277E1A6F8CD4140769545B661B56E40642F6F479DC6414097F5F22E54B46E4038C4569F40BF41407D3FB10047B36E4017A97BABE2B741407696D32A3AB26E40C201BE6883B0414057B5ADAC2DB16E409497FAD322A9414022EC938521B06E4089CE0AEAC0A14140C11DDBB415AF6E40289AC4A75D9A4140CBBDD8390AAE6E404072FA09F992414023CEE213FFAC6E40B9477B0D938B4140D4DC4F42F4AB6E400D7912AF2B844140BA0177C4E9AA6E40D3C687EBC27C414051DCAF99DFA96E4014489FBF587541406E9152C1D5A86E40945E1928ED6D414013C9B73ACCA76E40F0AAB221806641402AAC3805C3A66E40A20024A9115F414056E22E20BAA56E40ED5922BBA1574140BD8FF48AB1A46E40A2CB5E5430504140D752E444A9A36E40C7788671BD4841403C42594DA1A26E402886420F4941414074EAAEA399A16E40B00D382AD3394140D44B414792A06E40BB1108BF5B32414041D86C378B9F6E4040704FCAE22A41400F718E73849E6E40C7D5A64868234140D96403FB7D9D6E4056B0A236EC1B41405F6D29CD779C6E402522D3906E14414050AD5EE9719B6E4045F4C353EF0C41402FAE014F6C9A6E400189FC7B6E054140305E71FD66996E404CCEFF05ECFD4040160E0DF461986E40CE2F4CEE67F64040036F34325D976E40FD885B31E2EE4040669047B758966E40F616A3CB5AE740407C9B2E1C98956E40945B8DE2C6E14040A1D5E42EB2936E40E2C59CC9E8E440407B57951C10916E40B537BDED3DE9404074471F9A6D8E6E405C110B8B90ED404023072AA6CA8B6E40359240A3E0F14040C2B15C3F27896E40802712382EF640403B175E6483866E4073742E4B79FA404048B7D413DF836E403A5A3EDEC1FE40407FBC664C3A816E40C3FFE4F20703414070F7B90C957E6E4050D9BF8A4B074140B4D97353EF7B6E40FDAF66A78C0B41400771391F49796E40F3A86B4ACB0F41406262AF6EA2766E40A04C5B750714414007E57940FB736E40928DBC294118414099BD3C9353716E4054CF1069781C414030399B65AB6E6E4000EDD334AD2041406D2838B6026C6E40BC3F7C8EDF24414088DAB58359696E4012A57A770F2941406018B6CCAF666E4001853AF13C2D4140871FDA8F05646E400ED821FD67314140519DC2CB5A616E400E2D919C90354140DBA90F7FAF5E6E40D8AEE3D0B639414016C360A8035C6E40CA296F9BDA3D4140C5C7544657596E40271184FDFB4141407FF28957AA566E4051846DF81A464140BDD49DDAFC536E40DE53718D374A4140C1512DCE4E516E407106D0BD514E41409999D430A04E6E409ADDC48A6952414015242F01F14B6E405EDA85F57E564140B3ABD73D41496E40B6C143FF915A4140942868E590466E40E8202AA9A25E41405ECB79F6DF436E40A7515FF4B062414022F8A46F2E416E402C7E04E2BC6641403E41814F7C3E6E4001A53573C66A41403A62A594C93B6E40DA9C09A9CD6E4140983AA73D16396E401B189284D2724140B1C81B4962366E4050A8DB06D5764140762497B5AD336E407BC1ED30D57A4140387AAC81F8306E4045BDCA03D37E41406D05EEAB422E6E40FCDD6F80CE8241406C0BED328C2B6E407C51D5A7C786414017D63915D5286E40E833EE7ABE8A414096AE63511D266E404492A8FAB28E4140F6D7F8E564236E40EC6CED27A5924140C58986D1AB206E40EAB9A00395964140BEEA9812F21D6E401667A18E829A4140430BBBA7371B6E40345CC9C96D9E4140F9DF768F7C186E40C27CEDB556A24140423C55C8C0156E40D0A9DD533DA64140BACCDD5004136E4096C364A421AA4140AE11972747106E40E2AA48A803AE41407A59064B890D6E408C424A60E3B14140F3BAAFB9CA0A6E408A7025CDC0B54140C20F16720B086E40151F91EF9BB94140ACEEBA724B056E40893D3FC874BD4140D6A51EBA8A026E4035C1DC574BC141400E35C046C9FF6D40F4A5119F1FC54140EF471D1707FD6D40BBEE809EF1C841400930B22944FA6D40E9A5C856C1CC41400ADFF97C80F76D4089DD81C88ED04140CDE06D0FBCF46D4061AF40F459D44140585586DFF6F16D40E13C94DA22D84140E2EAB9EB30EF6D40F0AE067CE9DB4140C4D77D326AEC6D4096351DD9ADDF41405BD445B2A2E96D407F0758F26FE34140E0148469DAE66D40546132C82FE741403C43A95611E46D40FD84225BEDEA4140C578247847E16D40B3B899ABA8EE4140ED3763CC7CDE6D40EC4504BA61F24140F165D151B1DB6D402E78C98618F641406A44D906E5D86D40AC9B4B12CDF94140E36AE3E917D66D40D2FBE75C7FFD41404BC056F949D36D409AE1F6662F0142406C7498337BD06D40BA91CB30DD0442404BF90B97ABCD6D40C44AB4BA8808424073FC1222DBCA6D40FF42FA04320C424044600DD309C86D4033A6E10FD90F4240193559A837C56D404993A9DB7D13424070B252A064C26D40B1198C6820174240063054B990BF6D40C536BEB6C01A4240B74A479D9DBE6D40FD40B87D561C4240EF42F22FCDBF6D400AA21A778E2542401948A18AF8C06D409DC3D45CA22E42409B0F602A24C26D403563A516B5374240810A3E1050C36D409C6A96A9C64042401D534B3D7CC46D402824AD1AD749424053B298B2A8C56D40EF50EA6EE6524240F0A43771D5C66D40FD3E4AABF45B4240FE603A7A02C86D402BDFC4D40165424031DBB3CE2FC96D40F4DA4DF00D6E424035CCB76F5DCA6D40E4A9D4021977424029B65A5E8BCB6D4019A744112380424002EAB19BB9CC6D405B2685202C894240F88CD328E8CD6D4039897935349242400E9ED60617CF6D40D65301553B9B424086FBD23646D06D40AD41F88341A442406768E1B975D16D40115A36C746AD424009921B91A5D26D409A0490234BB64240A5159CBDD5D36D40551DD69D4EBF4240F1857E4006D56D40EE08D63A51C84240C470DF1A37D66D4093C859FF52D14240B264DC4D68D76D40CD0D28F053DA4240CDF693DA99D86D40224E041254E342404DC825C2CBD96D40ACD6AE6953EC4240598CB205FEDA6D4071DFE4FB51F54240D10D5CA630DC6D40B79E60CD4FFE4240203545A563DD6D40275CD9E24C074340110E920397DE6D40D883034149104340BFCD67C2CADF6D4038B990EC441943409D971B474DE06D405ED5B1BE121D43405A64710CB4E06D40447036B5931C4340");
  compareCsvLine(line.getRow(4),"EW0227091446G,EW0212024579G,5.51566,2.72636367,2.78929746,0.15876605,0.15599899,1.17461932,1.82992518,17.72870444,254.36117319,14.17228080,2.89702322,2.55570412,0.40919079,0.32284540,0.85672502,0.63167849,0.64640276,0.62642226,0.18411582,0.12795954,2011287,2011113,240.56840385,34.18787049,235.23328924,38.33176699,2439400,2439400,242.38214360,37.43944410,3955.72810017,235.24861628,38.44438984,4234.90636559,238.14312605,35.89120632,273.41773462,321.16175413,9.26900090,0.26713997,61.81618413,63.54309199,61.82375105,63.62199641,010300000001000000C2000000FC5F84244BCD6D407000D5C1553443406FE18454BACF6D407C719D7A62314340AA9398EA28D26D406FF7F0C76D2E43408756DBE796D46D4069DC7FA9772B4340B6FA674D04D76D409B01F81E802843408645581C71D96D40BDDF04288725434080F4C455DDDB6D407F874FC48C2243401FC1C5FA48DE6D40D3A17EF3901F43405A64710CB4E06D40447036B5931C4340449ADD8B1EE36D402ACD180995194340A4251F7A88E56D40E12BC5EE941643406FD349D8F1E76D40E098D86593134340567E70A75AEA6D40D0B9ED6D901043403F12A5E8C2EC6D408ECD9C068C0D4340C58FF89C2AEF6D4022AC7B2F860A4340970F7BC591F16D40A9C61DE87E074340F6C53B63F8F36D402E27143076044340060649775EF66D407370ED066C0143404445B002C4F86D40B7DD356C60FE4240CF1E7E0629FB6D406142775F53FB4240C056BE838DFD6D40A60939E044F8424081DD7B7BF1FF6D401B3600EE34F5424005D3C0EE54026E4044614F8823F24240238A96DEB7046E4001BBA6AE10EF4240C38B054C1A076E4009098460FCEB4240239A15387C096E4040A6629DE6E84240E9E7B530FE0A6E40BAAA7980F1E642409BA242A2730A6E40FEBEE8F56EE342403B98A46B1C096E4013496E79B7DA42406A235403C6076E400D220C7FFFD1424082FCCD6770066E40EDD3C40247C94240398E91971B056E400C2E99008EC0424021ED2091C7036E40BD378874D4B742407FCF005374026E40C3228F5A1AAF4240ED84B8DB21016E40C13DA9AE5FA6424050EED129D0FF6D40A2E6CF6CA49D4240CC75D93B7FFE6D40DC7CFA90E8944240CB065E102FFD6D40A9531E172C8C42403006F1A5DFFB6D4038A42EFB6E834240834A26FB90FA6D40AB7F1C39B17A42404D14940E43F96D402BC1D6CCF27142408006D3DEF5F76D40BCFF49B233694240ED1E7E6AA9F66D401F8060E573604240D4AE32B05DF56D4089260262B3574240965390AE12F46D4045681424F24E424064EF3864C8F26D404B3D7A27304642400AA2D0CF7EF16D40B91114686D3D4240DCC1FDEF35F06D402DB7BFE1A9344240A0D468C3EDEE6D400D565890E52B42409788BC48A6ED6D40BB5EB66F202342409CADA57E5FEC6D40A67AAF7B5A1A4240482ED36319EB6D40417D16B0931142402D09F6F6D3E96D40EA54BB08CC084240324AC1368FE86D40A8FB6A8103004240E303EA214BE76D40D667EF153AF74140ED4827B707E66D40AD7C0FC26FEE4140962532F5C4E46D40B0FA8E81A4E541405899C5DA82E36D4000702E50D8DC41407A909E6641E26D408A28AB290BD44140C4DD7B9700E16D40241EBF093DCB414041341E6CC0DF6D406BE820EC6DC241400C2148E380DE6D40ABAC83CC9DB941402D05BEFB41DD6D40840D97A6CCB041408E0F46B403DC6D407A1A0776FAA74140E336A80BC6DA6D40693F7C36279F4140CF33AE0089D96D40C9339BE352964140DE7A23924CD86D40D4E904797D8D4140C036D5BE10D76D408D7D56F2A684414069429285D5D56D408F23294BCF7B41405C232BE59AD46D40D717127FF6724140EF0372DC60D36D40498CA2891C6A4140A3AD3A6A27D26D400F976766416141408C835A8DEED06D40ED20EA1065584140B97CA844B6CF6D4044D3AE84874F4140B81EFD8E7ECE6D400B0636BDA84641401578326B47CD6D408EADFBB5C83D4140F21A24D810CC6D400748776AE73441409F17AFD4DACA6D4001CB1BD6042C414042F7B15FA5C96D40A09057F4202341408DB60C7870C86D40A24494C03B1A41407AC0A01C3CC76D4053D13636551141400DE9504C08C66D402D4C9F506D0841402A680106D5C46D405DE2280B84FF404069D49748A2C36D4016C5296199F64040FD1DFB1270C26D40A815F34DADED4040A58913643EC16D4066D1D0CCBFE4404097ABCA3A0DC06D4067BD09D9D0DB404084620B96DCBE6D40F851DF6DE0D24040A5D2C174ACBD6D40E5A58D86EEC94040BF60DBD57CBC6D4093594B1EFBC040403DAD46B84DBB6D40CB81493006B840405A8FF31A1FBA6D406F92B3B70FAF40403610D3FCF0B86D40C448AFAF17A640401266D75CC3B76D40AB955C131E9D4040C167550D42B76D4018BAAE3C1E9A40402FDEEF0C8EB46D4079BB5B1C459E4040BDD7D104E4B16D408176FF7C59A24040A59923BA39AF6D40255F2C1B6BA64040F029962B8FAC6D4089A9FCF879AA404030DDDA57E4A96D40C6E7831886AE40406551A33D39A76D40EF15CF7B8FB24040E168A1DB8DA46D40CAA5E42496B6404029458730E2A16D406E8AC4159ABA4040F641073B369F6D408F4368509BBE40401BF0D3F9899C6D40A5E8C2D699C240409010A06BDD996D40D933C1AA95C640406A8F1E8F30976D40C18C49CE8ECA4040E47E026383946D40D9123C4385CE40406B12FFE5D5916D40EAA7720B79D24040AA99C716288F6D4014FAC0286AD64040A37B0FF4798C6D40C98DF49C58DA4040C1318A7CCB896D4083C7D46944DE4040F942EBAE1C876D4055F522912DE24040E43EE6896D846D403A589A1414E64040B2E7684EE6826D40612706CF4AE840408BA6925F2B836D403C878AB19CEA4040D55F857718846D40A54A3F3491F2404017C57E9305856D401170CBAA83FA40408BCB21B4F2856D4050FEB01974024140951011DADF866D400B056B85620A41409EDCEE05CD876D403FB36DF24E124140FD255D38BA886D40856D2665391A4140C193FD71A7896D40FEE3FBE1212241408B8071B3948A6D4017284E6D082A414057FD59FD818B6D40E1C1760BED31414042D457506F8C6D4057C5C8C0CF3941404C8B0BAD5C8D6D4024E79091B04141401D6715144A8E6D40779115828F494140B26D1586378F6D403DF896966C5141401769AB0325906D407C2D4FD34759414018EA768D12916D401935723C21614140EB4A172400926D40A0182ED6F8684140D3B12BC8ED926D40AFFAAAA4CE704140C913537ADB936D403D2A0BACA27841401D372C3BC9946D408E356BF07480414008B6550BB7956D4009FDE175458841404C016EEBA4966D40C5C5804014904140C16213DC92976D40E74B5354E1974140F3FFE3DD80986D40C8D45FB5AC9F41409EDC7DF16E996D40F240A76776A7414046DD7E175D9A6D40C81D256F3EAF4140B8C984504B9B6D4029B7CFCF04B741408B4F2D9D399C6D40BF28988DC9BE4140AB0416FE279D6D402B6F6AAC8CC64140CC69DC73169E6D40FD782D304ECE4140ECEC1DFF049F6D407E37C31C0ED64140D0EB77A0F39F6D4050AF0876CCDD414070B68758E2A06D40CD08D63F89E541407691EA27D1A16D405DA0FE7D44ED4140A7B83D0FC0A26D4076165134FEF441405B611E0FAFA36D408C5F9766B6FC4140E1BC29289EA46D40D3D396186D044240FCFAFC5A8DA56D40CD3E104E220C4240384C35A87CA66D40BAEEBF0AD61342405FE46F106CA76D40D3C35D52881B4240E0FC49945BA86D406A3F9D28392342402ED760344BA96D40D8922D91E82A424028BF51F13AAA6D404CAEB98F96324240790DBACB2AAB6D40704FE827433A4240FF2937C41AAC6D40EF0F5C5DEE4142401C8E66DB0AAD6D40C873B3339849424028C7E511FBAD6D408DF788AE40514240C1785268EBAE6D40771E73D1E75842402C5F4ADFDBAF6D405C8004A08D604240B3516B77CCB06D4073D7CB1D32684240FF445331BDB16D40120E544ED56F42406C4DA00DAEB26D40304C24357777424072A1F00C9FB36D40DB04C0D5177F4240EF9BE22F90B46D407D03A733B78642408FBE147781B56D4011795552558E424013B425E372B66D402D094435F2954240BE52B47464B76D40FAD6E7DF8D9D42409F9E5F2C56B86D40FC91B25528A54240EFCBC60A48B96D40D982129AC1AC42406B4189103ABA6D40E49772B059B44240AD9A463E2CBB6D409C713A9CF0BB424084AA9E941EBC6D401B6FCE6086C34240487D311411BD6D404BBA8F011BCB4240415B9FBD03BE6D402254DC81AED24240FACA8891F6BE6D40A6200FE540DA424098938E90E9BF6D40F1F27F2ED2E142403FBF51BBDCC06D400A99836162E94240659D7312D0C16D40ACE76B81F1F042403CC59596C3C26D40F4C587917FF8424005185A48B7C36D40FB3823950C00434073C36228ABC46D404F6F878F980743400F4452379FC56D4059CCFA83230F43409667CB7593C66D40BAF3C075AD1643405B4F71E487C76D4079D41A68361E4340B572E7837CC86D4033B4465EBE25434057A1D15471C96D401F3A805B452D4340BF05D45766CA6D40147A0063CB344340B12D7F6DD0CA6D407F35F4CB54374340FC5F84244BCD6D407000D5C155344340");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0227176126G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 4);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EW0227176126G,EW0212024579G,5.90752,2.66494199,3.24258111,0.15678863,0.13441824,1.16991062,3.77407221,15.04749196,1117.10582808,3.29082058,2.77417986,2.55570412,0.31179369,0.32284540,0.85405346,0.63167849,0.62385731,0.62642226,0.40665362,0.28411835,2011288,2011113,234.73576933,33.97866162,235.23328924,38.33176699,2439400,2439400,236.33684517,37.26678462,3962.11042265,235.24861628,38.44438984,4234.90636559,235.16309033,35.75256218,274.51819495,321.16175413,9.16213875,0.26713997,63.76412601,63.54309199,63.85284140,63.62199641,01030000000100000005010000C8E85832510C6D409F2236DBFA124340F70F425AC90E6D40DCAFF59050104340FCA728F240116D40AD2116CAA40D4340AB2E22FBB7136D401A9A5B86F70A43406B2543762E166D4013BF87C548084340E4149F64A4186D401BBB598798054340859048C7191B6D40CF3D8ECBE60243401B3A519F8E1D6D407C7CDF91330043406BC5C9ED02206D40893205DA7EFD424099FBC1B376226D40ECA1B4A3C8FA4240C3BE48F2E9246D407D93A0EE10F84240640D6CAA5C276D40435779BA57F54240D20539DDCE296D40B3C4EC069DF24240A4E9BB8B402C6D40E23AA6D3E0EF4240172100B7B12E6D409BA04E2023ED4240743E106022316D4080648CEC63EA42406301F68792336D40037D0338A3E74240485ABA2F02366D4063685502E1E442408D6D655871386D40912C214B1DE24240EE96FE02E03A6D400157031258DF4240BE6C8C304E3D6D4090FC955691DC42402BC314E2BB3F6D4015B97018C9D9424075AF9C1829426D4040AF2857FFD64240228B28D595446D401888501234D442403CF7BB1802476D40A172784967D1424072DF59E46D496D4059232EFC98CE4240477D0439D94B6D40ABD3FC29C9CB4240345BBD17444E6D4062416DD2F7C84240C9578581AE506D40FCAD05F524C64240CFA85C7718536D40FDDD499150C3424058DE42FA81556D402118BBA67AC04240D1E5360BEB576D409924D834A3BD4240210D37AB535A6D402A4C1D3BCABA4240A50541DBBB5C6D40425704B9EFB742404AE7519C235F6D40F18C04AE13B542407A3366EF8A616D40FCB1921936B242403AD879D5F1636D40AB0721FB56AF42401833884F58666D40B44A1F5276AC424027148C5EBE686D400BB2FA1D94A94240FFC07F03246B6D4096ED1D5EB0A64240AEF75C3F896D6D40E524F111CBA34240A6F11C13EE6F6D40D6F5D938E4A04240B666B87F52726D4029733BD2FB9D4240EC8F2786B6746D40022376DD119B42408B2A62271A776D4067FDE75926984240E77A5F647D796D40A46AEC4639954240554F163EE07B6D40AF41DCA34A92424000037DB5427E6D4069C60D705A8F4240DA8089CBA4806D40E9A7D4AA688C42406C46318106836D409BFE815375894240BC6669D767856D40764A6469808642401B8D26CFC8876D40FC70C7EB898342400F005D69298A6D404ABBF4D99180424018A400A7898C6D4002D43233987D424091FE0489E98E6D403FC5C5F69C7A42407A385D1049916D405EF6EE23A07742404C21FC3DA8936D40CB29EDB9A1744240CD31D41207966D40BF7AFCB7A1714240DE8ED78F65986D40E45A561DA06E42403F0CF8B5C39A6D40F08F31E99C6B4240662F2786219D6D403131C21A98684240473256017F9F6D400CA539B19165424019067628DCA16D40649EC6AB89624240225677FC38A46D40FC199509805F4240868A4A7E95A66D40BC5BCEC9745C4240FFCADFAEF1A86D40FBEB98EB67594240B301278F4DAB6D40A194186E59564240F1DD0F20A9AD6D40475E6E5049534240FCD6896204B06D404A8DB89137504240C72E84575FB26D40C69E1231244D4240C4F4EDFFB9B46D407B45952D0F4A4240A708B65C14B76D40B5665686F8464240241DCB6E6EB96D401217693AE0434240BCBA1B37C8BB6D403D97DD48C6404240784296B621BE6D409F50C1B0AA3D4240B3F028EE7AC06D40EFD11E718D3A4240E6DFC1DED3C26D40C1CBFD886E3742405F0B4F892CC56D40FD0C63F74D3442400952BEEE84C76D40447F50BB2B3142404079FD0FDDC96D404323C5D3072E4240882FFAED34CC6D40000DBD3FE22A4240550FA2898CCE6D40006031FEBA274240E3A1E2E3E3D06D40674B180E92244240EA61A9FD3AD36D400C06656E6721424075BEE3D791D56D407BCA071E3B1E4240A71D7F73E8D76D40C4D2ED1B0D1B42408BDF68D13EDA6D4074540167DD174240DF608EF294DC6D403A7C29FEAB144240DEFDDCD7EADE6D40AE694AE0781142401B15428240E16D40E22A450C440E4240470AABF295E36D40F5B7F7800D0B42400949052AEBE56D4083EE3C3DD5074240D2473E2940E86D40128DEC3F9B0442403DA9077E0FE96D406BC4E7AF7B034240324AC1368FE86D40A8FB6A8103004240E303EA214BE76D40D667EF153AF74140ED4827B707E66D40AD7C0FC26FEE4140962532F5C4E46D40B0FA8E81A4E541405899C5DA82E36D4000702E50D8DC41407A909E6641E26D408A28AB290BD44140C4DD7B9700E16D40241EBF093DCB414041341E6CC0DF6D406BE820EC6DC241400C2148E380DE6D40ABAC83CC9DB941402D05BEFB41DD6D40840D97A6CCB041408E0F46B403DC6D407A1A0776FAA74140E336A80BC6DA6D40693F7C36279F4140CF33AE0089D96D40C9339BE352964140DE7A23924CD86D40D4E904797D8D4140C036D5BE10D76D408D7D56F2A684414069429285D5D56D408F23294BCF7B41405C232BE59AD46D40D717127FF6724140EF0372DC60D36D40498CA2891C6A4140A3AD3A6A27D26D400F976766416141408C835A8DEED06D40ED20EA1065584140B97CA844B6CF6D4044D3AE84874F4140B81EFD8E7ECE6D400B0636BDA84641401578326B47CD6D408EADFBB5C83D4140F21A24D810CC6D400748776AE73441409F17AFD4DACA6D4001CB1BD6042C414042F7B15FA5C96D40A09057F4202341408DB60C7870C86D40A24494C03B1A41407AC0A01C3CC76D4053D13636551141400DE9504C08C66D402D4C9F506D0841402A680106D5C46D405DE2280B84FF404069D49748A2C36D4016C5296199F64040FD1DFB1270C26D40A815F34DADED4040A58913643EC16D4066D1D0CCBFE4404097ABCA3A0DC06D4067BD09D9D0DB404084620B96DCBE6D40F851DF6DE0D24040A5D2C174ACBD6D40E5A58D86EEC94040BF60DBD57CBC6D4093594B1EFBC040403DAD46B84DBB6D40CB81493006B840405A8FF31A1FBA6D406F92B3B70FAF40403610D3FCF0B86D40C448AFAF17A640401266D75CC3B76D40AB955C131E9D4040C167550D42B76D4018BAAE3C1E9A40402FDEEF0C8EB46D4079BB5B1C459E4040BDD7D104E4B16D408176FF7C59A24040A59923BA39AF6D40255F2C1B6BA64040F029962B8FAC6D4089A9FCF879AA404030DDDA57E4A96D40C6E7831886AE40406551A33D39A76D40EF15CF7B8FB24040E168A1DB8DA46D40CAA5E42496B6404029458730E2A16D406E8AC4159ABA4040F641073B369F6D408F4368509BBE40401BF0D3F9899C6D40A5E8C2D699C240409010A06BDD996D40D933C1AA95C640406A8F1E8F30976D40C18C49CE8ECA4040E47E026383946D40D9123C4385CE40406B12FFE5D5916D40EAA7720B79D24040AA99C716288F6D4014FAC0286AD64040A37B0FF4798C6D40C98DF49C58DA4040C1318A7CCB896D4083C7D46944DE4040F942EBAE1C876D4055F522912DE24040E43EE6896D846D403A589A1414E64040E9B82E0CBE816D40482DF0F5F7E94040584378340E7F6D40ADB6D336D9ED4040A06A76015E7C6D407844EED8B7F1404073B0DC71AD796D40363DE3DD93F54040F7865E84FC766D405C2650476DF94040F04BAF374B746D409BACCC1644FD40400144828A99716D40E4ABEA4D18014140D8958A7BE76E6D40563736EEE904414061457B09356C6D4002A135F9B80841400C2F073382696D406D816970850C4140F902E1F6CE666D4002BF4C554F1041403D40BB531B646D40479554A9161441401730484867616D40EA9BF06DDB17414032E139D3B25E6D40B2CD8AA49D1B4140E62242F3FD5B6D40208F874E5D1F41406A8012A748596D4026B5456D1A234140223C5CED92566D406B8B1E02D5264140CE4AD0C4DC536D40A4DA650E8D2A4140D84E1F2C26516D409EEE6993422E41408193F9216F4E6D402F9C7392F531414031080FA5B74B6D400147C60CA6354140A23B0FB4FF486D4028E79F03543941402A57A94D47466D40A00E3978FF3C4140EE198C708E436D4097EEC46BA84041401DD4651BD5406D40A55C71DF4E4441402762E44C1B3E6D40B0D766D4F2474140F927B503613B6D40E88CC84B944B4140290C853EA6386D40695CB446334F4140337300FCEA356D40D1DD42C6CF5241409A3AD33A2F336D40A96487CB6956414025B4A8F972306D40A2049057015A414000A12B37B62D6D40BE95656B965D4140EC2C06F2F82A6D4046B80B08296141405CE9E1283B286D4090D8802EB96441409DC867DA7C256D40C732BEDF46684140F1184005BE226D405DD6B71CD26B4140AC7F12A8FE1F6D407EA95CE65A6F414050F485C13E1D6D40526C963DE17241408EBB40507E1A6D400CBC4923657641406E62E852BD176D40F0155698E679414048B921C8FB146D4022DA959D657D4140CCCE90AE39126D40504EDE33E28041400CEBD804770F6D404FA0FF5B5C844140708A9CC9B30C6D4068E8C416D4874140AC587DFBEF096D40B62BF464498B4140B32B1C992B076D403F5E4E47BC8E4140ADFE18A166046D40DD648FBE2C924140C9EC1212A1016D403A176ECB9A954140302CA8EADAFE6C4069419C6E06994140D008762914FC6C4083A5C6A86F9C414037DF18CD4CF96C4019FD947AD69F414062172CD484F66C4077FAA9E43AA341407A1F4A3DBCF36C40CB49A3E79CA6414093660C07F3F06C4035921984FCA9414073570B3029EE6C409476A0BA59AD41402853DEB65EEB6C405996C68BB4B04140C3AB1B9A93E86C400D8E15F80CB44140F29E58D8C7E56C40D1F7110063B74140D10639A589E36C40C6EC14970DBA41401264B48BEEE36C404EC32782BCBD41408E39AE4FC6E46C400F82719D9AC541408ADCA9279EE56C407C56D03677CD4140E46D371476E66C40758C245252D541402009E7154EE76C404AD949F32BDD4140ABC6482D26E86C40AD6B171E04E541401CBEEC5AFEE86C406DFB5FD6DAEC41406808639FD6E96C4000D9F11FB0F441402AC23BFBAEEA6C4005FD96FE83FC4140CF0D076F87EB6C407017157656044240E21555FB5FEC6C40BB9E2D8A270C4240280FB6A038ED6C40D9DE9D3EF7134240F33ABA5F11EE6C4006081F97C51B424042E9F138EAEE6C40783D669792234240057BED2CC3EF6C40ECA324435E2B424040643D3C9CF06C400B70079E283342404B2E726775F16C40B2F4B7ABF13A4240FE791CAF4EF26C400BB1DB6FB9424240E201CD1328F36C40A05E14EE7F4A4240639C149601F46C4027FFFF2945524240003E8436DBF46C4057EA3827095A42407AFBACF5B4F56C406FDB55E9CB614240040C20D48EF66C40DAFEE9738D69424070CB6ED268F76C4067FF84CA4D71424063BC2AF142F86C40B413B3F00C7942407E8AE5301DF96C40410BFDE9CA8042408F0C3192F7F96C40775BE8B987884240C6469F15D2FA6C40A72CF76343904240DB6CC2BBACFB6C40C666A8EBFD97424042E42C8587FC6C403DBE7754B79F42406146717262FD6C4067C0DDA16FA74240B06222843DFE6C4029E04FD726AF4240FC40D3BA18FF6C40518240F8DCB642408F231717F4FF6C40DE091F0892BE424061898199CF006D4041E4570A46C642405230A642AB016D406B955402F9CD42405A17191387026D40DFC37BF3AAD54240B7806E0B63036D40914431E15BDD424033F43A2C3F046D40C526D6CE0BE542404B4113761B056D40CCBFC8BFBAEC42406A818CE9F7056D40ACB664B768F44240301A3C87D4066D40AE0F03B915FC424096BFB74FB1076D40DF37FAC7C10343403F7695438E086D4083109EE76C0B4340AA956B636B096D4051FA3F1B17134340E5537165CD096D4073D0AE8FAF154340C8E85832510C6D409F2236DBFA124340");
  compareCsvLine(line.getRow(2),"EW0227176126G,EW0227260700G,5.81737,2.62272986,3.19464383,0.15684691,0.12850077,1.10073532,1.59582118,17.94161843,197.03929733,17.05142048,2.77417986,2.47127987,0.31179369,0.25042158,0.85405346,0.63169433,0.62385731,0.61044976,0.35843118,0.30189779,2011288,2011289,234.73576933,33.97866162,229.63489425,33.24135398,2439400,2439400,236.33684517,37.26678462,3962.11042265,229.64219299,33.35074765,4128.72803558,232.40929838,33.69311715,274.51819495,302.17187462,9.16213875,0.26777329,63.76412601,64.99156835,63.85284140,65.05952118,010300000001000000F400000081317A8FC6266D400473D4F49DCD3E40BCC99D0E79246D4073F5D2F541D33E405C3A6C342B226D408D01C9EBE1D83E40A700F3FFDC1F6D4032B025D87DDE3E40A12C3F708E1D6D4071DC50BC15E43E40EF5D5D843F1B6D40A627AB99A9E93E40AFC0593BF0186D409DFD8D7139EF3E40670A4094A0166D4054984B45C5F43E40D8761B8E50146D40B4032F164DFA3E40CFC4F62700126D40F6207CE5D0FF3E401333DC60AF0F6D401EAA6FB450053F40257DD5375E0D6D40F5343F84CC0A3F4016D8EBAB0C0B6D402836195644103F4055EF27BCBA086D40FB03252BB8153F407AE1916768066D40F6D88204281B3F40093D31AD15046D4059D64BE393203F403BFD0C8CC2016D40770692C8FB253F40B1862B036FFF6C40B05E60B55F2B3F403AA492111BFD6C407AC1BAAABF303F40898347B6C6FA6C402E009EA91B363F40E4B14EF071F86C4097DCFFB2733B3F40CD18ACBE1CF66C40600ACFC7C7403F40BFFA6220C7F36C405930F3E817463F40BEEF751471F16C408EE94C17644B3F4009E2E6991AEF6C403EC6B553AC503F40AF0AB7AFC3EC6C40844C009FF0553F4023EEE6546CEA6C4002F9F7F9305B3F40DB58768814E86C402E3F61656D603F40CF5B6449BCE56C40AD89F9E1A5653F401049AF9663E36C40453A7770DA6A3F403AB0546F0AE16C40D2A989110B703F40075B51D2B0DE6C40F827D9C537753F40B149A1BE56DC6C40A8FA068E607A3F4074AF3F33FCD96C40865DAD6A857F3F40F2EE262FA1D76C4018815F5CA6843F409E9650B145D56C40C789A963C3893F40195DB5B8E9D26C40B88E1081DC8E3F408A1D4D448DD06C40719812B5F1933F40F9D30E5330CE6C40649F260003993F409399F0E3D2CB6C40368ABC62109E3F40F4A0E7F574C96C40DC2B3DDD19A33F406632E88716C76C4099410A701FA83F4019A8E598B7C46C40C5707E1B21AD3F40546AD22758C26C405444EDDF1EB23F409EEB9F33F8BF6C40552AA3BD18B73F40DBA43EBB97BD6C402671E5B40EBC3F4071119EBD36BB6C407F44F2C500C13F4053ABAC39D5B86C404DAA00F1EEC53F4009E7572E73B66C40667F4036D9CA3F40BD2F8C9A10B46C400674DA95BFCF3F40C1B1C54D42B36C40E49879EB0AD23F40CBC1709C1CB46C40D2CACBB2EFE23F40C6A208A3F3B46C40E35CEC6F8FF33F401F90349CCAB56C40935BA51015024040C4069688A1B66C40CD18DBE85F0A4040F4E4CD6878B76C40AC6DF545A8124040B06D7C3D4FB86C4016DD482DEE1A4040054C410726B96C40BA8420A4312340406F96BBC6FCB96C40153CBEAF722B404017D2897CD3BA6C40EAB25A55B133404019F64929AABB6C40778F259AED3B4040B86E99CD80BC6C40268C4583274440409220156A57BD6C40E194D8155F4C4040BA6B59FF2DBE6C4000E4F35694544040E82E028E04BF6C40C51EA44BC75C404081CAAA16DBBF6C409571EDF8F7644040AC23EE99B1C06C40B5ABCB63266D40405BA7661888C16C40A95A329152754040454DAE925EC26C4052E50C867C7D4040E39A5E0935C36C408FA63E47A48540405FA6107D0BC46C409A07A3D9C98D404080195DEEE1C46C40069A0D42ED9540408A34DC5DB8C56C4061314A850E9E404026D125CC8EC66C4098FC1CA82DA640402865D13965C76C40E09E42AF4AAE4040700576A73BC86C407E48709F65B64040A668AA1512C96C4013CF537D7EBE404008EA0485E8C96C40ADC5934D95C64040298C1BF6BECA6C409094CF14AACE40409FFB836995CB6C40A8909FD7BCD64040C791D3DF6BCC6C40B612959ACDDE404063579F5942CD6C402B8E3A62DCE6404048077CD718CE6C40C9A71333E9EE40400A11FE59EFCE6C40E94B9D11F4F640408B9BB9E1C5CF6C409AC44D02FDFE4040A487426F9CD06C4054CF940904074140B1722C0373D16C408AB2DB2B090F41401EB90A9E49D26C40E652856D0C174140FE78704020D36C404E48EED20D1F41408A94F0EAF6D36C40A0F26C600D274140AAB41D9ECDD46C40398E511A0B2F4140694B8A5AA4D56C403A48E604073741408196C8207BD66C4085526F24013F4140C3A16AF151D76C409FF72A7DF9464140914902CD28D86C402AAE5113F04E4140533D21B4FFD86C404F2C16EBE4564140D50159A7D6D96C40D47AA508D85E4140C1F33AA7ADDA6C4004082770C9664140FA4958B484DB6C406DBABC25B96E4140FB1742CF5BDC6C404603832DA7764140445089F832DD6C40CDF0908B937E4140A9C6BE300ADE6C404940F8437E864140B2327378E1DE6C40F16FC55A678E4140EE3137D0B8DF6C409DD0FFD34E9641404E4A9B3890E06C403F97A9B3349E41406EEC2FB267E16C402DEEBFFD18A64140EA75853D3FE26C4043063BB6FBAD4140A3332CDB16E36C40CC270EE1DCB541401264B48BEEE36C404EC32782BCBD41408E39AE4FC6E46C400F82719D9AC541408ADCA9279EE56C407C56D03677CD4140E46D371476E66C40758C245252D541402009E7154EE76C404AD949F32BDD4140ABC6482D26E86C40AD6B171E04E541401CBEEC5AFEE86C406DFB5FD6DAEC41406808639FD6E96C4000D9F11FB0F441402AC23BFBAEEA6C4005FD96FE83FC4140CF0D076F87EB6C407017157656044240E21555FB5FEC6C40BB9E2D8A270C4240280FB6A038ED6C40D9DE9D3EF7134240F33ABA5F11EE6C4006081F97C51B424042E9F138EAEE6C40783D669792234240057BED2CC3EF6C40ECA324435E2B424040643D3C9CF06C400B70079E283342404B2E726775F16C40B2F4B7ABF13A4240FE791CAF4EF26C400BB1DB6FB9424240108AEFE2B7F26C40CCB093B07C464240CE29B4DBD9F46C40AF7D32593644424033C237398AF76C408C4937345641424042E9954B3AFA6C40971A3873743E4240EA4C1214EAFC6C40EB5C4115913B4240405DF09399FF6C40F2575C19AC384240EC5073CC48026D40D52A8F7EC5354240AC29DEBEF7046D40B3C8DC43DD324240C1B8736CA6076D40DAF44468F32F42406DA376D6540A6D40E43EC4EA072D4240676729FE020D6D40B6FE53CA1A2A4240565FCEE4B00F6D407E50EA052C2742404DC7A78B5E126D4081107A9C3B24424036C1F7F30B156D40E0D6F28C492142405C59001FB9176D404AF340D6551E4240E18A030E661A6D408F684D77601B4240344443C2121D6D4025E8FD6E69184240986B013DBF1F6D4081CD34BC701542409BE37F7F6B226D407D19D15D76124240A28F008B17256D40826DAE527A0F42405C58C560C3276D40B106A5997C0C4240583010026F2A6D40DEB889317D0942407C1823701A2D6D40A1E92D197C0642409E2440ACC52F6D40078B5F4F790342400680A9B770326D407416E9D2740042400072A1931B356D40378791A26EFD41406B626A41C6376D4015551CBD66FA414052DE46C2703A6D40D06E49215DF74140869C79171B3D6D407534D5CD51F4414036824542C53F6D40987178C144F141408EA7ED436F426D408757E8FA35EE41405C5CB51D19456D404D77D67825EB4140B92CE0D0C2476D40A4BBF03913E84140B1E5B15E6C4A6D40D062E13CFFE44140F7996EC8154D6D4054F84E80E9E141409AA65A0FBF4F6D40914EDC02D2DE4140B8B7BA3468526D404C7828C3B8DB414047CDD33911556D400DC2CEBF9DD84140D23FEB1FBA576D4070AB66F780D541403FC546E8625A6D4051E0836862D24140A8752C940B5D6D40D731B61142CF414028D0E224B45F6D405F8F89F11FCC4140B7BFB09B5C626D405CFF8506FCC841400EA0DDF904656D400A982F4FD6C541408B42B140AD676D40F27706CAAEC241409EA5D84B92686D40AEA765C446C141406A6283B28C676D40DE30650F8FB84140394F569E8B666D40D4F12AB8F8AF41402F292D2F8B656D40B76AD4BE62A74140458CD8638B646D40E1755D1FCD9E414002702B3B8C636D40EF4AC3D5379641403120FBB38D626D409C7004DEA28D4140C2351FCD8F616D408FAE20340E854140A58F718592606D4052FF18D4797C4140DB4BCEDB955F6D404082EFB9E573414081C013CF995E6D40846DA7E1516B41401175225E9E5D6D402B004547BE6241409B1BDD87A35C6D402B74CDE62A5A4140268A284BA95B6D4095F046BC9751414027B4EBA6AF5A6D40A17BB8C30449414001A40F9AB6596D40F8EC29F971404140A2747F23BE586D40E5DFA358DF374140364B2842C6576D4087A52FDE4C2F4140EC50F9F4CE566D402D37D785BA264140CAACE33AD8556D409128A54B281E41409D7DDA12E2546D40349AA42B96154140F3D3D27BEC536D40BB2BE121040D414034ACC374F7526D4046EE662A72044140C0E8A5FC02526D40D2564241E0FB40401D4C74120F516D40A33080624EF3404058732BB51B506D40B98F2D8ABCEA404043D0C9E3284F6D4025C357B42AE24040F3A34F9D364E6D4093470CDD98D940402EF9BEE0444D6D40B0B9580007D14040079F1BAD534C6D409DC84A1A75C8404069236B01634B6D406A28F026E3BF4040D5CDB4DC724A6D408584562251B74040119A013E83496D4045728B08BFAE404000335C2494486D4041639CD52CA6404079EDD08EA5476D40E09796859A9D40402DC36D7CB7466D40BF11871408954040AB4D42ECC9456D402E867A7E758C404062C15FDDDC446D4098507DBFE2834040BFE8D84EF0436D40F5649BD34F7B4040421FC23F04436D403942E0B6BC724040C14C31AF18426D40B4E45665296A4040ACE03D9C2D416D4078B809DB9561404044CD000643406D40BF8B021402594040108394EB583F6D4039814A0C6E50404034EC144C6F3E6D406802EABFD9474040ED679F26863D6D40EFB1E82A453F404014C6527A9D3C6D40D35D4D49B0364040A7424F46B53B6D40C1F11D171B2E40406A81B689CD3A6D404B695F90852540408389AB43E6396D4010C215B1EF1C404036C15273FF386D40EAED4375591440409AE9D11719386D400CC5EBD8C20B4040681A503033376D401DF80DD82B034040CDBDF5BB4D366D407E0454DD28F53F40418CECB968356D4020367C31F9E33F4080885F2984346D404D518EA4C8D23F4070FB7A09A0336D40F062812E97C13F4028706C59BC326D40719C49C764B03F40EDAF6218D9316D405A37D866319F3F4059BE8D45F6306D40F0581B05FD8D3F4062D51EE013306D408BF5FD99C77C3F408D6148E7312F6D40F6B3671D916B3F401AFE3D5A502E6D40A5D03C87595A3F403C7134386F2D6D40C7005ECF20493F4063A861808E2C6D404D55A8EDE6373F4078B4FC31AE2B6D40A81DF5D9AB263F4041C63D4CCE2A6D409BCA198C6F153F40B52A5ECEEE296D40AED0E7FB31043F40644798B70F296D40C78A2C21F3F23E40E496270731286D40391CB1F3B2E13E4046A548BC52276D4027533A6B71D03E40870E66FF22276D40BE20D8C2BACC3E4081317A8FC6266D400473D4F49DCD3E40");
  compareCsvLine(line.getRow(3),"EW0227176126G,EW0211981114G,5.7197,2.69738212,3.02232080,0.15783793,0.19958795,1.17576934,4.80125636,33.00084420,908.04967149,4.06636407,2.77417986,2.62058439,0.31179369,0.37737797,0.85405346,0.63091750,0.62385731,0.63786601,0.31023174,0.21508877,2011288,2011112,234.73576933,33.97866162,238.32114291,38.16027593,2439400,2439400,236.33684517,37.26678462,3962.11042265,238.29478020,38.25876597,4243.90040739,235.96431723,35.90366926,274.51819495,322.77007837,9.16213875,0.23670015,63.76412601,62.45244053,63.85284140,62.59205831,010300000001000000F30000001B3A519F8E1D6D407C7CDF91330043406BC5C9ED02206D40893205DA7EFD424099FBC1B376226D40ECA1B4A3C8FA4240C3BE48F2E9246D407D93A0EE10F84240640D6CAA5C276D40435779BA57F54240D20539DDCE296D40B3C4EC069DF24240A4E9BB8B402C6D40E23AA6D3E0EF4240172100B7B12E6D409BA04E2023ED4240743E106022316D4080648CEC63EA42406301F68792336D40037D0338A3E74240485ABA2F02366D4063685502E1E442408D6D655871386D40912C214B1DE24240EE96FE02E03A6D400157031258DF4240BE6C8C304E3D6D4090FC955691DC42402BC314E2BB3F6D4015B97018C9D9424075AF9C1829426D4040AF2857FFD64240228B28D595446D401888501234D442403CF7BB1802476D40A172784967D1424072DF59E46D496D4059232EFC98CE4240477D0439D94B6D40ABD3FC29C9CB4240345BBD17444E6D4062416DD2F7C84240C9578581AE506D40FCAD05F524C64240CFA85C7718536D40FDDD499150C3424058DE42FA81556D402118BBA67AC04240D1E5360BEB576D409924D834A3BD4240210D37AB535A6D402A4C1D3BCABA4240A50541DBBB5C6D40425704B9EFB742404AE7519C235F6D40F18C04AE13B542407A3366EF8A616D40FCB1921936B242403AD879D5F1636D40AB0721FB56AF42401833884F58666D40B44A1F5276AC424027148C5EBE686D400BB2FA1D94A94240FFC07F03246B6D4096ED1D5EB0A64240AEF75C3F896D6D40E524F111CBA34240A6F11C13EE6F6D40D6F5D938E4A04240B666B87F52726D4029733BD2FB9D4240EC8F2786B6746D40022376DD119B42408B2A62271A776D4067FDE75926984240E77A5F647D796D40A46AEC4639954240554F163EE07B6D40AF41DCA34A92424000037DB5427E6D4069C60D705A8F4240DA8089CBA4806D40E9A7D4AA688C42406C46318106836D409BFE815375894240BC6669D767856D40764A6469808642401B8D26CFC8876D40FC70C7EB898342400F005D69298A6D404ABBF4D99180424018A400A7898C6D4002D43233987D424091FE0489E98E6D403FC5C5F69C7A42407A385D1049916D405EF6EE23A07742404C21FC3DA8936D40CB29EDB9A1744240CD31D41207966D40BF7AFCB7A1714240DE8ED78F65986D40E45A561DA06E42403F0CF8B5C39A6D40F08F31E99C6B4240662F2786219D6D403131C21A98684240473256017F9F6D400CA539B19165424019067628DCA16D40649EC6AB89624240225677FC38A46D40FC199509805F4240868A4A7E95A66D40BC5BCEC9745C4240FFCADFAEF1A86D40FBEB98EB67594240B301278F4DAB6D40A194186E59564240F1DD0F20A9AD6D40475E6E5049534240FCD6896204B06D404A8DB89137504240C72E84575FB26D40C69E1231244D4240C4F4EDFFB9B46D407B45952D0F4A4240A708B65C14B76D40B5665686F8464240241DCB6E6EB96D401217693AE0434240BCBA1B37C8BB6D403D97DD48C6404240784296B621BE6D409F50C1B0AA3D4240B3F028EE7AC06D40EFD11E718D3A4240E6DFC1DED3C26D40C1CBFD886E3742405F0B4F892CC56D40FD0C63F74D3442400952BEEE84C76D40447F50BB2B3142404079FD0FDDC96D404323C5D3072E4240882FFAED34CC6D40000DBD3FE22A4240550FA2898CCE6D40006031FEBA274240E3A1E2E3E3D06D40674B180E92244240EA61A9FD3AD36D400C06656E6721424075BEE3D791D56D407BCA071E3B1E4240A71D7F73E8D76D40C4D2ED1B0D1B42408BDF68D13EDA6D4074540167DD174240DF608EF294DC6D403A7C29FEAB144240DEFDDCD7EADE6D40AE694AE0781142401B15428240E16D40E22A450C440E4240470AABF295E36D40F5B7F7800D0B42400949052AEBE56D4083EE3C3DD5074240D2473E2940E86D40128DEC3F9B044240B08A43F194EA6D40552EDB875F01424033A60283E9EC6D407744DA1322FE4140334269DF3DEF6D403514B8E2E2FA4140BD1C650792F16D40F6AF3FF3A1F74140EB0CE4FBE5F36D40D5F238445FF44140C305D4BD39F66D407A7B68D41AF141401A19234E8DF86D40F6A68FA2D4ED4140877ABFADE0FA6D40888B6CAD8CEA41402F8297DD33FD6D403AF3B9F342E7414046EB045DFBFD6D40722A65D1D8E54140833C7719FDFC6D409B2C4CF43DDE41409E3AEBF202FC6D40FD419BABBED641409DC7182609FB6D408F2FF1303ECF41406E8258B20FFA6D4059C42181BCC74140D4AF039716F96D40A5B5FD9839C04140393874D31DF86D400D945275B5B841404DA5046725F76D4070C0EA1230B14140D21F10512DF66D40DF608D6EA9A94140506DF29035F56D406155FE8421A24140E1ED07263EF46D40A12CFE52989A4140E899AD0F47F36D4085184AD50D934140E7FF404D50F26D40A6E29B08828B4140384220DE59F16D40B7E0A9E9F4834140EC14AAC163F06D40C2E82675667C41408BBB3DF76DEF6D405045C2A7D6744140EC063B7E78EE6D407AA9277E456D41400E53025683ED6D40DD24FFF4B2654140E384F47D8EEC6D405D17ED081F5E4140360873F599EB6D40E42492B6895641407DCDDFBBA5EA6D4000298BFAF24E4140BA479DD0B1E96D40492A71D15A4741405F6A0E33BEE86D40C44DD937C13F414022A796E2CAE76D4018CA542A26384140F5EB99DED7E66D40A7DA70A589304140D9A07C26E5E56D408EB2B6A5EB284140D0A5A3B9F2E46D406B6FAB274C214140C150749700E46D40180CD027AB1941406C6B54BF0EE36D403F53A1A2081241404931AA301DE26D40CED19794640A4140834DDCEA2BE16D4033C927FABE024140DED851ED3AE06D408C21C1CF17FB4040AE5772374ADF6D40AA5BCF116FF34040C7B7A5C859DE6D40E882B9BCC4EB4040754E54A069DD6D40E61EE2CC18E4404067D6E6BD79DC6D401725A73E6BDC4040CCE3989BF9DB6D40DF8348494FD8404030DA6E9595D96D40F215B1B6E7DB40403B240BC6E7D66D40FCEEFAEEEBDF404020F6F09339D46D400319A78EEDE340402AB6CDFD8AD16D404F4E5D97ECE740408FA64E02DCCE6D409568BF0AE9EB404079E020A02CCC6D40226969EAE2EF4040314FF1D57CC96D40C180F137DAF3404043AB6CA2CCC66D409C17E8F4CEF7404092753F041CC46D40C3D4D722C1FB40407CF215FA6AC16D40B5A545C3B0FF4040FF249C82B9BE6D409DC5B0D79D034140D7C97D9C07BC6D405DC4926188074140AB52664655B96D40A28D5F62700B41401CE1007FA2B66D40906F85DB550F41400442F844EFB36D406B216DCE3813414079E8F6963BB16D4002CA793C1917414008E9A67387AE6D4000060927F71A4140CCF4B1D9D2AB6D4011EE728FD21E41409254C1C71DA96D40CB1C0A77AB224140F4E37D3C68A66D408AB41BDF812641407B0C9036B2A36D401865EFC8552A4140BFC09FB4FBA06D402571C735272E4140807754B5449E6D40A4B3E026F6314140BE2655378D9B6D40FEA4729DC2354140D53E4839D5986D401760AF9A8C3941408BA5D3B91C966D402FA7C31F543D41402DB19CB763936D40A4E8D62D194141409B234831AA906D4086430BC6DB44414058257A25F08D6D40018C7DE99B4841408E40D692358B6D40B14F4599594C41401C5CFF777A886D40B6D974D6145041409FB697D3BE856D40BE3619A2CD53414069E140A402836D40DA383AFD8357414080BB9BE845806D402E7BDAE8375B41409A6C489F887D6D407E65F765E95E41401860E6C6CA7A6D409F2F897598624140E43F145E0C786D40A6E48218456641406FEF6F634D756D402866D24FEF694140928696D58D726D400C6F601C976D41406A4C24B3CD6F6D407F96107F3C71414040B2B4FA0C6D6D409952C178DF744140534EE2AA4B6A6D40E6FA4B0A80784140BCD646C289676D40D5CA84341E7C4140241C7B3FC7646D40F9E33AF8B97F41409704172104626D4022503856538341404586B165405F6D406403424FEA86414035A2E00B7C5C6D40E6DD17E47E8A4140F75E3912B7596D4099AD7415118E414054C34F77F1566D40C32F0EE4A0914140F1D0B6392B546D406D1295502E954140E37E005864516D40A7F5B45BB998414056B4BDD09C4E6D40B36C1406429C414009437EA2D44B6D40F8FE5450C89F4140D8E1D0CB0B496D40E728133B4CA341404327434B42466D40B25CE6C6CDA64140D083611F78436D40D20261F44CAA4140883CB746AD406D40847A10C4C9AD41405065CEBFE13D6D40121A7D3644B1414043DB2F89153B6D40EF2E2A4CBCB44140053F63A148386D40C6FD950532B8414006EFEE067B356D404EC23963A5BB4140BE0158B8AC326D400DAF896516BF4140E03F22B4DD2F6D40E0ECF40C85C24140871ED0F80D2D6D407A9AE559F1C5414046B9E2843D2A6D40A3CBC04C5BC941403FCCD9566C276D406788E6E5C2CC41402FAE336D9A246D400DCCB12528D041405F4A6DC6C7216D400584780C8BD341409C1A0261F41E6D40968E8B9AEBD6414012216C3B201C6D4065B936D049DA414023E223544B196D40F4BFC0ADA5DD4140365EA0A975166D40E0496B33FFE04140700B573A9F136D4001E9726156E4414061CFBB04C8106D4064170F38ABE74140ABF84007F00D6D40213572B7FDEA414096385740170B6D400E86C9DF4DEE4140889C6DAE3D086D403B2F3DB19BF141409287F14F63056D405D34F02BE7F44140CAAB4E2388026D40FC74005030F84140AC03EF26ACFF6C408EA9861D77FB4140E162D139B5FE6C40C85B8CCCEEFC4140D96034BCC7FF6C401988FD2A4B06424011B7396BD6006D4024BCF7F3820F42404F0FF255E5016D405280FB98B91842401283547DF4026D4096B3321FEF214240B4C058E203046D4049A8C28B232B42406510F78513056D40363BCCE3563442400159286923066D4036EA6B2C893D42400625E68C33076D40D7EAB96ABA46424089A72AF243086D40A240CAA3EA4F42401BC1F09954096D4057D3ACDC19594240D5043485650A6D40E1846D1A486242404CBDF0B4760B6D4039471462756B424092F1232A880C6D400532A5B8A1744240486ACBE5990D6D4020982023CD7D42409FB6E5E8AB0E6D40EB1C83A6F78642407B317234BE0F6D4080C9C54721904240810671C9D0106D40B221DE0B4A9942403B37E3A8E3116D400339BEF771A242403DA0CAD3F6126D404AC7541099AB424058FE294B0A146D40603D8D5ABFB44240C6F304101E156D4083D94FDBE4BD4240640D602332166D40BCBB819709C7424004C8408646176D4009FA04942DD04240A595AD395B186D406FB4B8D550D94240D0E2AD3E70196D40EB28796173E24240F11B4A96851A6D4050C71F3C95EB4240BCB28B419B1B6D40F244836AB6F442408F237D41B11C6D4045B077F1D6FD4240D7A1D6970A1D6D409E5C44B2C40043401B3A519F8E1D6D407C7CDF9133004340");

  /////////////////////////////////////////////////////////////////
  // file, header, skip, delimiter, keepEmptyParts, ignore comments
  line = CSVReader(tempDir.path() + "/EW0227422989G.csv",
                   false, 0, ',', false, true);

  ASSERT_EQ(line.columns(), 47);
  ASSERT_EQ(line.rows(), 3);

  compareCsvLine(line.getRow(0),"SourceProductIdA,SourceProductIdB,StereoPairRank,ImageStrengthRank,StereoStrengthRank,ParallaxHeightRatio,ShadowTipDistance,ResolutionRatio,DeltaSolarAzimuth,DeltaSpacecraftAzimuth,VerticalPrecision,StereoAngle,ImageStrengthRankA,ImageStrengthRankB,IncidenceRankA,IncidenceRankB,EmissionRankA,EmissionRankB,PhaseRankA,PhaseRankB,OverlapRatioA,OverlapRatioB,YearDoyA,YearDoyB,CenterLongitudeA,CenterLatitudeA,CenterLongitudeB,CenterLatitudeB,CenterRadiusA,CenterRadiusB,SubSpacecraftLongitudeA,SubSpacecraftLatitudeA,TargetCenterDistanceA,SubSpacecraftLongitudeB,SubSpacecraftLatitudeB,TargetCenterDistanceB,GisOverlapCentroidX,GisOverlapCentroidY,PixelResolutionA,PixelResolutionB,EmissionAngleA,EmissionAngleB,IncidenceAngleA,IncidenceAngleB,PhaseAngleA,PhaseAngleB,GisIntersectionFootprint");
  compareCsvLine(line.getRow(1),"EW0227422989G,EW0211624221G,5.42942,1.85240579,3.57701041,0.32657102,0.31169637,1.15529813,7.94691037,16.74029474,547.79785396,39.16148552,2.01510698,1.68970461,0.81257877,0.35887899,0.16962876,0.16638229,0.39351005,0.58752686,0.53813165,0.41441780,2011291,2011108,237.11265368,-6.81678316,268.54563190,1.05205525,2439400,2439400,208.65190459,-37.16343677,12552.81260657,252.54411765,-37.14705142,14210.35024571,254.52184708,5.85704100,1930.93098213,2230.80097160,48.21484936,48.34470836,46.25157559,37.17757998,84.58409498,67.12258233,010300000001000000ED000000E68C4EB616906C409CBE5B08B2623D40D9094EA745B06C40AB4C5ECA20BA3E40282ABC4271D66C40C91B3BF6233A40406CA901AFA40D6D4018AD4B3774A44140FF796391E2446D407C58960A28C04240A50F593C745B6D4079AB5D47EEA44240C1AA9EA8D6716D401736A08B108942402053C21216896D40CB22CED5336B42401C791666259F6D4088D815B4FC4D4240045A5C0815B66D40F2CB8724B12E4240D7BE1E9AD8CB6D409EC8874B1E1042407B469CE57AE26D40FC1C5D6967EF4140D164F8A10AFA6D40A9994D3A57CC41404944E1DD4A106E40475FCA4B45AA41400C1AE31870266E4086F40B276D8741400B488D447C3D6E4047D9ED5F28624140F4C6A38755546E40FB36AD4B2F3C4140E6A7B5DBFE6A6E40CC1836F37F15414014ED41EA78816E4020C2E2FE1BEE4040B1C9B33BC6976E4088F7C19E01C64040422E536E00AF6E40DD8EEF28259B404063C6FBF502C66E4097548A8A916F40409E22238AD0DC6E4053E961694543404056099D226AF36E4009941E814216404070D03E57D1096F40FC2AE75312D13F4062AE867324216F4026BF71C57D6F3F400EA0F2023D386F407E9A0646730C3F4016C0AB501E4F6F400B7621B4EDA73E4064AAE02FAA646F405E36C1C10B473E40F400CFF5197B6F4060F0C2F7D6DF3D40015C4C3940906F402F7733895B7C3D409501223C04A36F40B24518DE47223D406C1DEF7ECAB66F405546962380C13C40AF470CB160CB6F4073B09732B15A3C4065539FAED0DE6F4001E388ACADF73B401441872212F26F40217227FEB4933B406CBEC72394027040A406FD4FBE2E3B401B868DD80A0C7040C48AE346C3C83A40D4CCED05FA14704026410370C5663A402B60D453451E70400DD6A1E70BFF394094149A2D0F2770401D92EB21469B39402169EF24C62F7040AC60195CB336394033CA2EEC6A387040B05F285C52D13840C08BAFC504407040FC0DC2DCD27638407ECF09F924487040E3C14ABBAB1438407747C05895507040E565F06E2FAD3740C5F2809B96587040C06DBB668D49374036B39CB5876070405395D9F047E53640C0859638696870402D1B250D5E8036407BA827E73B7070408006B3F0CB1A3640573AD5611777704008658A539CC035400E9C18C7E67D70402CA13867F9653540FAAD7D47AB847040C27A7780D70A3540A65247CCF68B7040C0D1057C6AA7344055CED9D5A292704010346EEA644B34406144FD15D699704085725423EBE633403D7C5A17FCA07040191024B6F581334065596733ABA67040E45786F0D23033409C6D6A89E4AC70403066D6DF1AD73240399FF8A612B37040C53AD9201C7D3240A7D8EA2BA8B8704007AD87E8032B3240B585E1DFC5BE7040A9B82ECB3FD03140E151ED2D6BC57040DDFBB1C19C6C314088B9F16975CB7040A31EDFCF24113140ECF50BCE06D270403A650525B1AC3040CF040E7B8DD870408C9A9834DC473040E346E70B0ADF70405F4130ED42C52F40BCA1A62B28E47040F4AEC86B1C242F4045D6A751CFE9704078BF0130A7702E40265EB202E0EE704083ED46F28ECE2D40B9CAA8BB78F470407F1F50A0091A2D40310B58CB7CF970400CB0444BFB762C40FA52EBF707FF70409E922CD160C12B405A8C723619057140A10F09F6F2F82A40D59D3433950A714036B6D74548422A4099B1458F961071403C2CB20DA37829407D0891B6C1147140FF018E3EABEB28400B63DEE0E81871401CD85E136B5E2840AA0739390C1D7140D9A9E0B6DFD0274077DD7A4EB72171408E50D7683C302740B28F9B4FD225714080B7D90921A22640CCF7B189742A7140AB6FCB9CCD002640E95AE617122F7140403B8F74215F25405C23F96E353471404A2ADE6301AA244030B07A11C9387140721D7810A8072440EBCD4A01E23D714041C75F69B8512340062D68C77E437140269F27400D882240F1C7B8028C487140669E981354D121406C28222C554B7140CFD93F82146C2140391C547AA54E71401A4FB21833F320400D2DA7E62E54714059DE44DF2528204039BA479C79577140B8E0316F965D1F403D709503C25A71405B1564569C6A1E40E5C57345085E7140BB7B93F754771D40AB24AB6F4C6171405F262F96BF831C403D3A769D8E64714046ABD27AD78F1B406E4CCAB25568714087DD4BA5F4731A40C527A3FE196C7140281F7D44B35719404BFA3DABDB6F71402F76F0FD0B3B1840FFFF0AEA20747140185B68980AF61640FC82E7CD62787140A0DB3AC68EB01540683E6E05277D7140BF7CCDC06C42144094EEA553E781714074F02DA3ABD312407E31AA211C8771409779D51AC23F1140B8959F40AD877140C5ED33E1A6131140051999F9DC8C71407D442BB3F3FD0E4009EC698B6D8D71405A2151FC93A50E40289EFDDB99927140838CDDFACD790B409BA8356D30947140C86C577BC97F0A40EE1E7E4E589971408B8175E09151074057D0E75C769C71407A925CD861630540D786760A9CA1714017E9F43D61300240BF3E5210B1A67140BE037CFF2408FE3FA7E152833FA7714017EF147CD455FD3F47F33E975BAC71409ADA086074ECF63FA444274A73B1714039FB10FE9581F03FB1EC721187B67140FBE22D04BE29E43FC73C512797BB714011FC3600F331CD3FD51188C5A3C071404E4BF4F5E952C6BF01968025ADC571401AC60AFA0A7AE2BFE3E85380B3CA714073940B979863EFBF27F411D9B7CF71401217811BC929F6BF9560A9BABCD4714083E8283F13A8FCBF045150B4B8D97140681B78181C9001C00592FF9840CA7140C79B0A9510D80AC0F733E09DB17E71407FC273245FE31BC0C31EE54E745871405475FCE2579221C0B89B9768BD3A714031928982155D24C096174E9E862171408A98D95D98B526C0E33D23EA320B71406B9A77AE00C428C087902B5FEBF67040EA1E9A12449D2AC02D19DA572FE4704071001FBAF94D2CC073BE35A7AAD27040754623D865DE2DC0E07E00B222C27040AE0E3B9F4E542FC0C8244EBF6CB270403B927F7EF65930C07388DF8A68A370401ADB65E1390031C0EF4AA703FD947040448D206D309E31C0B070913C16877040DFA8B15CD83432C0DF956210A47970405304703500C532C0FCC48D33996C7040D1C22570524F33C0DA98588CEA5F7040821173B45DD433C04A4682B98E5370404448C1CE9A5434C0FE3299B87D47704024D3A31971D034C0C1A472A2B03B704012C1D0D1394835C09E56717721307040BECEB5A342BC35C0006D4FF7CA247040B64261A8CF2C36C0C28E7081A8197040563112F71C9A36C08EE09FFBB50E70403D6DE3E55F0437C0E3ADB4BDEF0370402159A40CC86B37C0C7C70102A5F26F407667D91780D037C051B86BA5B6DD6F409C8F4776AE3238C07AB740110FC96F40644CDDE8759238C06647856FA9B46F406B5AEFFAF5EF38C018AADE5881A06F40B73B61674B4B39C0434D39C7928C6F406CE54C6E90A439C034086E0ADA786F4073D8F81DDDFB39C00A0D8CBE53656F40A2B3569147513AC0A29E7FC3FC516F4020BCD125E4A43AC02CB1DB35D23E6F405D09DCA9C5F63AC0FD588730A8356F401C776BF4741D3BC084EC1938DB326F4021AEB95763EE3AC0413E6D5CD42B6F407B15A1921C773AC09013F711D8246F40B766B6D45DFF39C08170E7CFE51D6F4076CA32FB228739C0FAD8D90EFD166F4066A5D1C2670E39C0D28CA4481D106F4099BA3BC6279538C0BFB027F845096F40DBF6567C5E1B38C017371C9976026F40A3E1783607A137C0C059E2A7AEFB6E40157C791E1D2637C059744FA1EDF46E40F01BA4349BAA36C0580B7B0233EE6E407294834D7C2E36C0A3C98A487EE76E404CB5860FBBB135C0B03A7DF0CEE06E4038DD79F0513435C07B03F37624DA6E40AA03D2323BB634C07957F6577ED36E40B93DC5E2703734C0CD61C00EDCCC6E4057542CD3ECB733C086547C153DC66E407988289AA83733C0E6C807E5A0BF6E40F216888D9DB632C05113B0F406B96E40EE75E3BEC43432C0C424ECB96EB26E403B926CF716B231C08E8812A8D7AB6E40569268B38C2E31C07C010B3041A56E4080C14B1D1EAA30C0623BFBBFAA9E6E4069476E08C32430C065F6EDC213986E400E5D9ED6E53C2FC09AFE73A07B916E4073E5B3B2492E2EC0382E3EBCE18A6E40F5A342F89E1D2DC0B69FAF7545846E40FDD60018D20A2CC05A196727A67D6E40AE862290CEF52AC0659BBE2603776E40F4FFCADA7EDE29C009D43FC35B706E40483BD75ACCC428C072100C46AF696E40D69DCF469FA827C0080F36F1FC626E402377C891DE8926C0AADA0BFF435C6E401376F0D16F6825C02D904EA183556E4089A88224374424C0709C5500BB4E6E404360C60E171D23C0619E1A3AE9476E40A501BA5BF0F221C010A42A610D416E40950FF6F5A1C520C083EE787B263A6E40A220867A112A1FC063C10E8133336E40124B8DAEFEC11CC0EAFB925A332C6E4067F522F5BB521AC0BB46A3DF24256E4048276F66F4DB17C02B7EF7D4061E6E40B45FEB3F4D5D15C0709B46EAD7166E40079D6E4D65D612C006ABE2B7960F6E4009942E3ED44610C0B244FFBB41086E40CD1FB9C4535C0BC0545F9357D7006E40A1A2AB9ADA1706C02B1DC4CA55F96D40236F0CAF37BF00C0151BC230BBF16D40F12D32E89FA2F6BFD4AAFD7A05EA6D40063ACFE6BF33E7BF8CD58F6B32E26D4097A803546E6498BF6CA1AD8E3FDA6D40FEC643354D12E63FC33BF0322AD26D4033204E516BA9F63FAE802E60EFC96D40EE40478957410140B9B091CC8BC16D40300AD956604C0740034C76CFFBB86D401F644099EC770D40A49488513BB06D40A9F8E03B2BE31140BD9C5EB945A76D40493B2306201D154091C192D3159E6D40CCA10706506B1840FD2609B5A5946D40ECD9D1B968CF1B40F0098D95EE8A6D4092C06B91534B1F4010973CA0E8806D40D3A6382CA170214011E42CB68A766D40971EA691DF492340D5AD191ECA6B6D4071789A85E03225400C0681199A606D40139016D0632D274022CAA653EB546D403A65D3F67F3B2940EC749C18AB486D40EC0B9CBFBB5F2B40443E9B37C23B6D4016B3E03B329D2D4054DEE660132E6D40845A783AC7F72F407F6B0CAC781F6D40466D3D243B3A3140C07EDDADBF0F6D4017093968E78C3240BAFD62F3A2FE6C4021A2F0C060F83340AFC58867BEEB6C4093AEEDB405833540283FAACB78D66C4069C68208793637405489225BD2BD6C4041B92F269B223940976B4525E39F6C401A88AB9A4A653B40D993D50075886C40561A9CD16E163D40E68C4EB616906C409CBE5B08B2623D40");
  compareCsvLine(line.getRow(2),"EW0227422989G,EW0212189243G,4.42606,2.04196934,2.38408574,0.10438570,0.06191721,1.14883726,2.62769560,5.02073653,5922.79303545,4.28391649,2.01510698,2.06883170,0.81257877,0.86384653,0.16962876,0.19586365,0.39351005,0.42960598,1.0,0.77692584,2011291,2011115,237.11265368,-6.81678316,237.11302579,-4.25682422,2439400,2439400,208.65190459,-37.16343677,12552.81260657,212.39795511,-37.13504192,14173.30087820,241.73716365,-0.55990397,1930.93098213,2218.32546666,48.21484936,47.16545379,46.25157559,47.27693068,84.58409498,81.33546115,010300000001000000620100006BCFACFBEE306A402005886C9D862D40AD96B97B1C406A4025FB263FAEEB2D402DDA1EE8594F6A40983939C6A5542E40FEDE2CA3A85E6A407FBF04B8A3C12E407EAD3F240A6E6A40ACD55995CA322F401DAD3AFA7F7D6A4081563BF33FA82F40F8A15BCE0B8D6A407D54B767161130406E137567AF9C6A4001F4F6F85E503040C5ABA3AD6CAC6A40871CD42E12923040BC9A96AE45BC6A408621C6E94AD6304067BA88A23CCC6A401345C8B5261D31401F8D0FF253DC6A4015072D24C66631407AB2EE3C8EEC6A402C8AAA354DB33140A9EB2A62EEFC6A4015AF53D8E30232405E4BA889770D6B4063D43C7EB65532409446B52F2D1E6B402A5DF1D2F6AB324005480233132F6B40D50BC597DC053340A476AEE52D406B40A1EAA7B1A6633340AB804B2282516B40473CC6769CC533403FFD0A6615636B40F3AD634F0F2C3440860FBCF1ED746B40B301CBC45C973440C014EAF312876B40FA6C0D24F107354095B56BBF8C996B40717F9DEA4A7E35403BB0371365AC6B40B5E4DB4BFFFA35403232BA7AA7BF6B400E172446C07E36405B30C8D161D36B4080B302FE640A3740C81DD3FDA4E76B407D7E0882F59E3740DFEC61F985FC6B40CEE949D6BB3D3840DB05AA631F126C40429D7C805DE838404AB173EE93286C40BE57576C03A139403D3CEF5712406C40DA51936F9B6A3A40A7B4265ADC586C40DF87C3FA4A493B4050BB9DD153736C407973B26349433C40E68C4EB616906C409CBE5B08B2623D40D9094EA745B06C40AB4C5ECA20BA3E40282ABC4271D66C40C91B3BF6233A40406CA901AFA40D6D4018AD4B3774A44140FF796391E2446D407C58960A28C04240A50F593C745B6D4079AB5D47EEA44240C1AA9EA8D6716D401736A08B108942402053C21216896D40CB22CED5336B42401C791666259F6D4088D815B4FC4D4240045A5C0815B66D40F2CB8724B12E4240D7BE1E9AD8CB6D409EC8874B1E1042407B469CE57AE26D40FC1C5D6967EF4140D164F8A10AFA6D40A9994D3A57CC41404944E1DD4A106E40475FCA4B45AA41400C1AE31870266E4086F40B276D8741400B488D447C3D6E4047D9ED5F28624140F4C6A38755546E40FB36AD4B2F3C4140E6A7B5DBFE6A6E40CC1836F37F15414014ED41EA78816E4020C2E2FE1BEE4040B1C9B33BC6976E4088F7C19E01C64040422E536E00AF6E40DD8EEF28259B404063C6FBF502C66E4097548A8A916F40409E22238AD0DC6E4053E961694543404056099D226AF36E4009941E814216404070D03E57D1096F40FC2AE75312D13F4062AE867324216F4026BF71C57D6F3F400EA0F2023D386F407E9A0646730C3F4016C0AB501E4F6F400B7621B4EDA73E4064AAE02FAA646F405E36C1C10B473E40F400CFF5197B6F4060F0C2F7D6DF3D40015C4C3940906F402F7733895B7C3D409501223C04A36F40B24518DE47223D406C1DEF7ECAB66F405546962380C13C40AF470CB160CB6F4073B09732B15A3C4065539FAED0DE6F4001E388ACADF73B401441872212F26F40217227FEB4933B406CBEC72394027040A406FD4FBE2E3B401B868DD80A0C7040C48AE346C3C83A40D4CCED05FA14704026410370C5663A402B60D453451E70400DD6A1E70BFF394094149A2D0F2770401D92EB21469B39402169EF24C62F7040AC60195CB336394033CA2EEC6A387040B05F285C52D13840C08BAFC504407040FC0DC2DCD27638407ECF09F924487040E3C14ABBAB1438407747C05895507040E565F06E2FAD3740C5F2809B96587040C06DBB668D49374036B39CB5876070405395D9F047E53640C0859638696870402D1B250D5E8036407BA827E73B7070408006B3F0CB1A3640573AD5611777704008658A539CC035400E9C18C7E67D70402CA13867F9653540FAAD7D47AB847040C27A7780D70A3540A65247CCF68B7040C0D1057C6AA7344055CED9D5A292704010346EEA644B34406144FD15D699704085725423EBE633403D7C5A17FCA07040191024B6F581334065596733ABA67040E45786F0D23033409C6D6A89E4AC70403066D6DF1AD73240399FF8A612B37040C53AD9201C7D3240A7D8EA2BA8B8704007AD87E8032B3240B585E1DFC5BE7040A9B82ECB3FD03140E151ED2D6BC57040DDFBB1C19C6C314088B9F16975CB7040A31EDFCF24113140ECF50BCE06D270403A650525B1AC3040CF040E7B8DD870408C9A9834DC473040E346E70B0ADF70405F4130ED42C52F40BCA1A62B28E47040F4AEC86B1C242F4045D6A751CFE9704078BF0130A7702E40265EB202E0EE704083ED46F28ECE2D40B9CAA8BB78F470407F1F50A0091A2D40310B58CB7CF970400CB0444BFB762C40FA52EBF707FF70409E922CD160C12B405A8C723619057140A10F09F6F2F82A40D59D3433950A714036B6D74548422A4099B1458F961071403C2CB20DA37829407D0891B6C1147140FF018E3EABEB28400B63DEE0E81871401CD85E136B5E2840AA0739390C1D7140D9A9E0B6DFD0274077DD7A4EB72171408E50D7683C302740B28F9B4FD225714080B7D90921A22640CCF7B189742A7140AB6FCB9CCD002640E95AE617122F7140403B8F74215F25405C23F96E353471404A2ADE6301AA244030B07A11C9387140721D7810A8072440EBCD4A01E23D714041C75F69B8512340062D68C77E437140269F27400D882240F1C7B8028C487140669E981354D121406C28222C554B7140CFD93F82146C2140391C547AA54E71401A4FB21833F320400D2DA7E62E54714059DE44DF2528204039BA479C79577140B8E0316F965D1F403D709503C25A71405B1564569C6A1E40E5C57345085E7140BB7B93F754771D40AB24AB6F4C6171405F262F96BF831C403D3A769D8E64714046ABD27AD78F1B406E4CCAB25568714087DD4BA5F4731A40C527A3FE196C7140281F7D44B35719404BFA3DABDB6F71402F76F0FD0B3B1840FFFF0AEA20747140185B68980AF61640FC82E7CD62787140A0DB3AC68EB01540683E6E05277D7140BF7CCDC06C42144094EEA553E781714074F02DA3ABD312407E31AA211C8771409779D51AC23F1140B8959F40AD877140C5ED33E1A6131140051999F9DC8C71407D442BB3F3FD0E4009EC698B6D8D71405A2151FC93A50E40289EFDDB99927140838CDDFACD790B409BA8356D30947140C86C577BC97F0A40EE1E7E4E589971408B8175E09151074057D0E75C769C71407A925CD861630540D786760A9CA1714017E9F43D61300240BF3E5210B1A67140BE037CFF2408FE3FA7E152833FA7714017EF147CD455FD3F47F33E975BAC71409ADA086074ECF63FA444274A73B1714039FB10FE9581F03FB1EC721187B67140FBE22D04BE29E43FC73C512797BB714011FC3600F331CD3FD51188C5A3C071404E4BF4F5E952C6BF01968025ADC571401AC60AFA0A7AE2BFE3E85380B3CA714073940B979863EFBF27F411D9B7CF71401217811BC929F6BF9560A9BABCD4714083E8283F13A8FCBF045150B4B8D97140681B78181C9001C00592FF9840CA7140C79B0A9510D80AC0F733E09DB17E71407FC273245FE31BC0C31EE54E745871405475FCE2579221C0B89B9768BD3A714031928982155D24C096174E9E862171408A98D95D98B526C0E33D23EA320B71406B9A77AE00C428C087902B5FEBF67040EA1E9A12449D2AC02D19DA572FE4704071001FBAF94D2CC073BE35A7AAD27040754623D865DE2DC0E07E00B222C27040AE0E3B9F4E542FC0C8244EBF6CB270403B927F7EF65930C07388DF8A68A370401ADB65E1390031C0EF4AA703FD947040448D206D309E31C0B070913C16877040DFA8B15CD83432C0DF956210A47970405304703500C532C0FCC48D33996C7040D1C22570524F33C0DA98588CEA5F7040821173B45DD433C04A4682B98E5370404448C1CE9A5434C0FE3299B87D47704024D3A31971D034C0C1A472A2B03B704012C1D0D1394835C09E56717721307040BECEB5A342BC35C0006D4FF7CA247040B64261A8CF2C36C0C28E7081A8197040563112F71C9A36C08EE09FFBB50E70403D6DE3E55F0437C0E3ADB4BDEF0370402159A40CC86B37C0C7C70102A5F26F407667D91780D037C051B86BA5B6DD6F409C8F4776AE3238C07AB740110FC96F40644CDDE8759238C06647856FA9B46F406B5AEFFAF5EF38C018AADE5881A06F40B73B61674B4B39C0434D39C7928C6F406CE54C6E90A439C034086E0ADA786F4073D8F81DDDFB39C00A0D8CBE53656F40A2B3569147513AC0A29E7FC3FC516F4020BCD125E4A43AC02CB1DB35D23E6F405D09DCA9C5F63AC05B9A9768D12B6F4002A26385FD463BC0DC9D9ADFF7186F40902622DD9B953BC02FFFF54A43066F4053488EB1AFE23BC0C8BCB382B1F36E40509C13FA462E3CC0E17F248340E16E40627919BD6E783CC085DB9A69EECE6E4049FB4A2533C13CC05BDB8471B9BC6E4080E67F949F083DC02141D7F19FAA6E40E51B97B4BE4E3DC034C2BF5AA0986E402FEA86869A933DC006339533B9866E405650DD6F3CD73DC0D9E3FD18E9746E4017BEE146AD193EC0329045BB2E636E40AFCB825DF55A3EC08230DDDC88516E40EB68348B1C9B3EC01BC7FF50F63F6E40DB04DE352ADA3EC0E7EA77FA752E6E404AF2F35925183FC0096282CA061D6E4040B7D39114553FC0B998CABFA70B6E407CE6771CFE903FC059297EE557FA6D40F38695E3E7CB3FC08F03755216E96D40606899C0EB0240C00A0D6C28E2D76D409B8D6222691F40C0EB585093BAC66D409517F19B6E3B40C0394B99C89EB56D405CE14791FE5640C0F02FB0068EA46D4099529D471B7240C083F7639487936D406F072FE7C68C40C052F067C08A826D407C4AF17C03A740C06475DCE096716D4008B91DFCD2C040C0D3A6E052AB606D407A0DA43F37DA40C0B6592C7AC74F6D4097B97F0B32F340C03F83B1C0EA3E6D4029B5F40DC50B41C028764496142E6D40FAA4B5E0F12341C09C5A4A70441D6D40FB47F509BA3B41C0F5576DC9790C6D403EE664FD1E5341C02CF55521B4FB6C408352211D226A41C0754069FCF2EA6C409CE88FBAC48041C07F598BE335DA6C4061CF2C17089741C0F102E6637CC96C40FDA64B65EDAC41C05AE8B20EC6B86C404CB1CBC875C241C0034D097912A86C40D267C057A2D741C068DFAE3B61976C407E5E0F1B74EC41C08472EBF2B1866C408B3D040FEC0042C0E2635F3E04766C40208BDB230B1542C0427ADCC057656C409FED443ED22842C0000D4120AC546C40FA80DD37423C42C04448550501446C4099CBA2DF5B4F42C0D165AA1B56336C40B5D45EFA1F6242C04BB57B11AB226C401AD20D438F7442C0EF519197FF116C409BDA3D6BAA8642C03166246153016C409C00691B729842C037E0C423A6F06B40CC304AF3E6A942C0667C4097F7DF6B4028292D8A09BB42C0FE0C8B7547CF6B40CFD6396FDACB42C0C9E7A77A95BE6B400161BB295ADC42C07D649464E1AD6B408D24633989EC42C0B75733F32A9D6B4099DB871668FC42C02E7839E8718C6B40552A6132F70B43C0CA9C1A07B67B6B4031C43FF7361B43C04DC3F714F76A6B40B759C2C8272A43C04E942824055B6B40F81C8375CE3743C0C2055A91EB556B4095AD9195D20143C019AB39C81D516B402265B74B22CE42C0925D6595624C6B40E7E967A4699A42C0353D5273B9476B4094044BA0A76642C0EF812BE121436B40DF73873DDB3242C0809A9B629B3E6B400F32A67703FF41C0BD2D987F253A6B40DAED73471FCB41C0C0D030C4BF356B4086B4E0A22D9741C0B64760C069316B4073C8DD7C2D6341C0FC29E007232D6B408D9E39C51D2F41C07BC4FE31EB286B407EF97968FDFA40C0131877D9C1246B40BA17B44FCBC640C0F2D34A9CA6206B406FE86260869240C0B32D9E1B991C6B4031373B7C2D5E40C0417B95FB98186B40C4BDFD80BF2940C0447434E3A5146B4002118E9076EA3FC048023F7CBF106B40EF2EBA4E3F813FC09F881B73E50C6B409347F5DDD6173FC0648EB67617096B4088DE2FD63AAE3EC014B6673855056B404E0062C568443EC0DDEFD76B9E016B408753122F5EDA3DC07FD4E8C6F2FD6A407C7DD78B18703DC0E5179D0152FA6A40D68DD34895053DC0950302D6BBF66A408F2029C7D19A3CC0CEE9190030F36A4036DB695BCB2F3CC06B83C73DAEEF6A40BFE6FD4C7FC43BC0D62ABA4E36EC6A400AFC83D5EA583BC0BCE75AF4C7E86A40A69329200BED3AC0343FBAF162E56A40FCBDFA48DD803AC047BD7E0B07E26A406520295C5E143AC0D62CD407B4DE6A40748949558BA739C0D5735BAE69DB6A40AD81871E613A39C0F0091BC827D86A40DF32CE8FDCCC38C05B00701FEED46A400CF2E56DFA5E38C0E891FF7FBCD16A401CAA8569B7F037C0F932A9B692CE6A401953571E108237C0E418799170CB6A401193EE11011337C020319BDF55C86A407990B0B286A336C0A2804E7142C56A4022EAAB569D3336C0E3E3D81736C26A402BB05F3A41C335C0B2287BA530BF6A40ED22707F6E5235C0907A65ED31BC6A4009DD472B21E134C0081BACC339B96A4065F3A325556F34C0415F3CFD47B66A40F470093706FD33C018ECD16F5CB36A40E16F2207308A33C0662AECF176B06A40A4E9001BCE1633C0C0ECC35A97AD6A40272845D3DBA232C05C404182BDAA6A40E68F256A542E32C05562F140E9A76A40334555F132B931C0CBD2FC6F1AA56A401BEBC64F724331C0117F1DE950A26A405F7B483F0DCD30C0EAFB94868C9F6A40D8E3F449FE5530C0A5C82223CD9C6A409282EF8E7FBC2FC0BA93FA99129A6A40965E3EB497CB2EC01979BAC65C976A402A6B6ED738D92DC0FC316185AB946A408CC5535456E52CC0902D44B2FE916A407F34BD07E3EF2BC08189052A568F6A407AFDA447D1F82AC03CE089C9B18C6A40EB6AC6DA12002AC0C0E2ED6D118A6A400EE988EF980529C04EB27BF474876A409BE32F12540928C0CFED9F3ADC846A40D4833C22340B27C04B65DE1D47826A405730EC46280B26C0B464C67BB57F6A404810BCE21E0925C0C085E631277D6A4052D6D685050524C0A6F6BF1D9C7A6A406BB14EDFC8FE22C0E021B91C14786A40E44901AD54F621C07AA00F0C8F756A40042BFFA993EB20C0E45DC9C80C736A40E36A95F6DEBC1FC043D1A42F8D706A400F347935A19D1DC0572C081D106E6A40B192A97F3C791BC0265CEF6C956B6A40FFE16C8C7C4F19C075B2D8FA1C696A40AFDF07232A2017C07609B0A1A6666A40477F88DC0AEB14C0322CB83B32646A40C3B1DBDEE0AF12C05D4672A2BF616A403C4E46906A6E10C09C1483AE4E5F6A40BED86684C44C0CC031839537DF5C6A40B8723AA4FBAE07C07F5B3A14715A6A40776C3E7EDC0203C0C790C41904586A404B02FEC97C8FFCBF45A9211C98556A401FB9B7E4D9F9F2BF4EA9ADED2C536A40C8A8BE5E9B86E2BFBEC9015FC2506A40B255EEA2DEB8A23F4A20BD3E584E6A400FE8A3714526E53FB6354659EE4B6A40603D675BA7B6F43F1951847884496A4045F40FAD5D02FF3FEBFD8E631A476A40FEDB6B153DBC04406F0052DEAF446A40234BB729B10D0A40818423A944426A40D926936CD9760F4003CF4980D83F6A401E1DA1B7917C1240B3116D1B6B3D6A409CE0901D104B15408426F12CFC3A6A408DDBA0ABC4271840F4DA31618B386A4024CEA060A3131B40FBFB9A5D18366A400FC8110ABB0F1E40955D93BFA2336A407EED51CA9C8E2040B7742F1B2A316A40D73EE4A5B81E2240637C9CF9AD2E6A4062FE61A5EFB823402C1B30D72D2C6A40B2424FBC195E2540346C0121A9296A40980F7C442D0F2740A32CE6311F276A4079087C5545CD2840BFFD9F4E8F246A401B7C97EAA9992A407C8EEFA0F8216A4047711683DA752C406D454F7A8C216A40492048CBAA232D406BCFACFBEE306A402005886C9D862D40");
}


/**
   * IsisminerTestStrategyFactoryNoIsisMinerObject
   * 
   * Tests strategy factory config file with no IsisMiner object.
   * 
   * INPUT: error_noIsisMinerObject.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: Strategy config file does not contain IsisMiner strategies object.
   */  
TEST(Isisminer, IsisminerTestStrategyFactoryNoIsisMinerObject) {
  QTemporaryDir tempDir;

  // create input config file error_noIsisMinerObject.conf in tempDir
  Pvl conf;

  /////////////////////////////////////////////////////////////////
  // add CnetReader strategy
  PvlObject CnetReader("Strategy");
  CnetReader.addKeyword(PvlKeyword("Name", "TestWithIdentity"));
  CnetReader.addKeyword(PvlKeyword("Type", "CnetReader"));
  CnetReader.addKeyword(PvlKeyword("CsvFile", "\"%1/Alph_VIS.net\""));
  CnetReader.addKeyword(PvlKeyword("CsvFileArgs", "\"inputdir\""));
  CnetReader.addKeyword(PvlKeyword("Identity", "%1"));
  CnetReader.addKeyword(PvlKeyword("IdentityArgs", "(PointId)"));
  CnetReader.addKeyword(PvlKeyword("Description", "Test the default functionality of CnetReader"));
  
  conf.addObject(CnetReader);

  QString configFilename = tempDir.path() + "/error_noIsisMinerObject.conf";
  conf.write(configFilename);

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/error_noIsisMinerObject.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("does not contain IsisMiner strategies object."));
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Strategy config file\n"
              "does not contain IsisMiner strategies object.\"";
  }
}


/**
   * IsisminerTestStrategyFactoryUnknownStrategy
   * 
   * Tests strategy factory config file with no IsisMiner strategies.
   * 
   * INPUT: error_noIsisMinerObject.conf
   * 
   * OUTPUT: None
   * 
   * THROWS: Could not create a NotAStrategy strategy for type [NotAStrategy].
   */  
TEST(Isisminer, IsisminerTestStrategyFactoryUnknownStrategy) {
  QTemporaryDir tempDir;

  // create input config file error_unknownStrategy.conf in tempDir
  Pvl conf;
  PvlObject isisminerObject("IsisMiner");
  isisminerObject.addKeyword(PvlKeyword("Name", "StrategyFactory"));
  isisminerObject.addKeyword(PvlKeyword("RequiredParameters", "(inputdir,outputdir)"));

    /////////////////////////////////////////////////////////////////
    // add NotAStrategy strategy
    PvlObject NotAStrategy("Strategy");
    NotAStrategy.addKeyword(PvlKeyword("Name", "NotAStrategy"));
    NotAStrategy.addKeyword(PvlKeyword("Type", "NotAStrategy"));
    isisminerObject.addObject(NotAStrategy);

    /////////////////////////////////////////////////////////////////
    // add Unknown strategy
    PvlObject Unknown("Strategy");
    Unknown.addKeyword(PvlKeyword("Name", "UnknownStrategy"));
    Unknown.addKeyword(PvlKeyword("Type", "Unknown"));
    isisminerObject.addObject(Unknown);

    /////////////////////////////////////////////////////////////////
    // add Calculator strategy
    PvlObject Calculator("Strategy");
    Calculator.addKeyword(PvlKeyword("Name", "CalculatorStrategy"));
    Calculator.addKeyword(PvlKeyword("Type", "Calculator"));
    isisminerObject.addObject(Calculator);

    /////////////////////////////////////////////////////////////////
    // add Database strategy
    PvlObject Database("Strategy");
    Database.addKeyword(PvlKeyword("Name", "DatabaseStrategy"));
    Database.addKeyword(PvlKeyword("Type", "Database"));
    isisminerObject.addObject(Database);

    /////////////////////////////////////////////////////////////////
    // add GisBasic strategy
    PvlObject GisBasic("Strategy");
    GisBasic.addKeyword(PvlKeyword("Name", "GisBasicStrategy"));
    GisBasic.addKeyword(PvlKeyword("Type", "GisBasic"));
    isisminerObject.addObject(GisBasic);

    /////////////////////////////////////////////////////////////////
    // add GisIntersect strategy
    PvlObject GisIntersect("Strategy");
    GisIntersect.addKeyword(PvlKeyword("Name", "GisIntersectStrategy"));
    GisIntersect.addKeyword(PvlKeyword("Type", "GisIntersect"));
    isisminerObject.addObject(GisIntersect);

    /////////////////////////////////////////////////////////////////
    // add GisUnion strategy
    PvlObject GisUnion("Strategy");
    GisUnion.addKeyword(PvlKeyword("Name", "GisUnionStrategy"));
    GisUnion.addKeyword(PvlKeyword("Type", "GisUnion"));
    isisminerObject.addObject(GisUnion);

    /////////////////////////////////////////////////////////////////
    // add PdsTableCreator strategy
    PvlObject PdsTableCreator("Strategy");
    PdsTableCreator.addKeyword(PvlKeyword("Name", "PdsTableCreatorStrategy"));
    PdsTableCreator.addKeyword(PvlKeyword("Type", "PdsTableCreator"));
    isisminerObject.addObject(PdsTableCreator);

    /////////////////////////////////////////////////////////////////
    // add Sidebar strategy
    PvlObject Sidebar("Strategy");
    Sidebar.addKeyword(PvlKeyword("Name", "SidebarStrategy"));
    Sidebar.addKeyword(PvlKeyword("Type", "Sidebar"));
    isisminerObject.addObject(Sidebar);

    /////////////////////////////////////////////////////////////////
    // add StereoPair strategy
    PvlObject StereoPair("Strategy");
    StereoPair.addKeyword(PvlKeyword("Name", "StereoPairStrategy"));
    StereoPair.addKeyword(PvlKeyword("Type", "StereoPair"));
    isisminerObject.addObject(StereoPair);

  conf.addObject(isisminerObject);
  conf.write(tempDir.path() + "/error_unknownStrategy.conf");

  // run isisminer
  QVector<QString> args = {"config=" + tempDir.path() + "/error_unknownStrategy.conf",
                           "parameters=inputdir:" + tempDir.path()
                                                  + "@outputdir:"
                                                  + tempDir.path()
                          };

  UserInterface ui(APP_XML, args);

  try {
    isisminer(ui);
    FAIL() << "Expected an exception to be thrown";
  }
  catch(IException &e) {
    EXPECT_TRUE(e.toString().toLatin1().contains("Could not create a NotAStrategy"));
  }
  catch(...) {
    FAIL() << "Expected an IException with message: \"Could not create a \n"
              "NotAStrategy strategy for type [NotAStrategy].\"";
  }
}

