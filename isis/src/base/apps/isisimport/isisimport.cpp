#include <iostream>
#include <fstream>

#include <inja/inja.hpp>
#include <nlohmann/json.hpp>
#include <QString>
#include <QtMath>

#include "CubeAttribute.h"
#include "FileName.h"
#include "importUtils.h"
#include "iTime.h"
#include "OriginalLabel.h"
#include "OriginalXmlLabel.h"
#include "PvlToJSON.h"
#include "ProcessImport.h"
#include "TextFile.h"
#include "XmlToJson.h"

#include "isisimport.h"

using namespace std;
using namespace inja;
using json = nlohmann::json;

namespace Isis {

  void isisimport(UserInterface &ui, Pvl *log) {
    FileName fileTemplate = ("$ISISROOT/appdata/import/fileTemplate.tpl");
    json jsonData;
    bool isPDS4 = false;
    FileName inputFileName = ui.GetCubeName("FROM");

    if (inputFileName.extension().toUpper() == "IMQ"){
      QString msg = "Input image may be compressed. Please run image through vdcomp to uncompress"
                    "or verify image has correct file extension.";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    try {
      // try to convert xml file to json
      jsonData = xmlToJson(inputFileName.toString());
      isPDS4 = true;
    }
    catch(...) {
      try {
        // try to convert pvl to json
        jsonData = pvlToJSON(inputFileName.toString());
      }
      catch(...) {
        QString msg = "Unable to process import image. Please confirm image is in PDS3 or PDS4 format";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    Environment env;

    // Dump the JSON to the debugging file if requested
    // This needs to be above all uses of the JSON by the template engine
    if (ui.WasEntered("DATA")) {
      std::ofstream jsonDataFile(FileName(ui.GetFileName("DATA")).expanded().toStdString());
      jsonDataFile << jsonData.dump(4);
      jsonDataFile.close();
    }

    // Find associated template
    FileName inputTemplate;
    if (ui.WasEntered("TEMPLATE")) {
      inputTemplate = ui.GetFileName("TEMPLATE");
    }
    else {
        std::string templateFile;
      try {
        templateFile = env.render_file(fileTemplate.expanded().toStdString(), jsonData);
        inputTemplate = FileName(QString::fromStdString(templateFile));
      }
      catch(const std::exception& e) {
        QString msg = "Cannot locate a template named [" + QString::fromStdString(templateFile) + "] for input label [";
        msg += FileName(ui.GetCubeName("FROM")).expanded();
        msg += "]. You can explicitly provide a template file using the [TEMPLATE] parameter. ";
        msg += e.what();
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }

    // Template engine call back functions
    /**
     * Renders YearDoy using StartTime to format to YYYYDOY
     */
    env.add_callback("YearDoy", 1, [](Arguments& args) {
      std::string startTime = args.at(0)->get<string>();
      std::string yearString = startTime.substr(0, 4);

      int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

      int year = stoi(startTime.substr(0, 4));
      int month = stoi(startTime.substr(5, 7));
      int day = stoi(startTime.substr(8, 10));
      int doy = day;
      if (month > 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) {
        doy++;
      }
      // Add the days in the previous months
      while (--month > 0) {
        doy = doy + daysInMonth[month-1];
      }
      return yearString.append(to_string(doy));
    });

    env.add_callback("capitalize", 1, [](Arguments& args) {
      std::string str = args.at(0)->get<string>();
      std::transform(str.begin(), str.end(),str.begin(), ::tolower);
      str[0] = toupper(str[0]);
      return str;
    });

    env.add_callback("CassiniIssBandInfo", 3, [](Arguments& args) {
      std::string instrumentID = args.at(0)->get<string>();
      std::string filter1 = args.at(1)->get<string>();
      std::string filter2 = args.at(2)->get<string>();
      QString filter = QString(filter1.c_str()) + "/" + QString(filter2.c_str());
      QString dir = "$ISISROOT/appdata/translations";
      QString cameraAngleDefs;
      if(instrumentID.at(3) == 'N') {
        cameraAngleDefs = dir + "/CassiniIssNarrowAngle.def";
      }
      else if(instrumentID.at(3) == 'W') {
        cameraAngleDefs = dir + "/CassiniIssWideAngle.def";
      }

      double center = 0;
      double width = 0;

      TextFile cameraAngle(cameraAngleDefs);
      int numLines = cameraAngle.LineCount();
      for(int i = 0; i < numLines; i++) {
        QString line;
        cameraAngle.GetLine(line, true);

        QStringList tokens = line.simplified().split(" ");
        if(tokens.count() > 2 && tokens.first() == filter) {
          center = toDouble(tokens[1]);
          width = toDouble(tokens[2]);
          break;
        }
      }
      vector<double> bandInfo = {center, width};
      return bandInfo;
    });

    env.add_callback("CassiniIssStretchPairs", 0, [](Arguments& args) {
      PvlGroup &dataDir = Preference::Preferences().findGroup("DataDirectory");
      QString missionDir = (QString) dataDir["Cassini"];
      FileName *lutFile = new FileName(missionDir + "/calibration/lut/lut.tab");
      TextFile *stretchPairs = new TextFile(lutFile->expanded());

      vector<double> vectorStretchPairs = {};

      bool begindataFound = false;
      QString line;
      bool goodLine = stretchPairs->GetLine(line);

      // Search for tag "\begindata" if it was not already found by recursively using this method
      while (!begindataFound) {

        if (!goodLine) {
          // Might want to detail this probelm a little more
          return std::string("()");
        }

        if(!line.contains("\\begindata")) {
          goodLine = stretchPairs->GetLine(line);
        }
        else {
          begindataFound = true;
        }
      }

      // Create the stretch pairs
      double temp1 = 0;
      for(int i = 0; i < stretchPairs->LineCount(); i++) {
        stretchPairs->GetLine(line);  //assigns value to line
        line = line.simplified();

        for (QString value: line.split(QRegExp("[\\s,]"), Qt::SkipEmptyParts)) {
          vectorStretchPairs.push_back(temp1);
          vectorStretchPairs.push_back(toDouble(value));
          temp1++;
        }
      }

      return vectorToString(vectorStretchPairs).toStdString();
    });

    env.add_callback("splitOnChar", 2, [](Arguments& args){
      std::string text = args.at(0)->get<string>();

      string delimiter = args.at(1)->get<string>();
      vector<string> words{};

      size_t pos;
      while ((pos = text.find(delimiter)) != string::npos) {
        words.push_back(text.substr(0, pos));
        words.push_back(text.substr(pos + 1));
        text.erase(0, pos + delimiter.length());
    }
      return words;
    });

    /**
     * Converts UniqueId To ObservationId
     * Logic from convertUniqueIdToObservationId in tgocassis2isis app
     */
    env.add_callback("UniqueIdtoObservId", 2, [](Arguments& args) {
      std::string uniqueId = args.at(0)->get<string>();
      std::string target = args.at(1)->get<string>();
      std::string observationId = "";

      long long operationPeriod = (stoll(uniqueId) & 1879048192);
      operationPeriod /= pow(2,28);

      // previously pulled from TgoCassisOperationPeriod.trn
      if ((int)operationPeriod == 0) observationId = "CRUS";
      else if ((int)operationPeriod == 1) observationId = "MY34";
      else if ((int)operationPeriod == 2) observationId = "MY35";
      else if ((int)operationPeriod == 3) observationId = "MY36";
      else if ((int)operationPeriod == 4) observationId = "MY37";
      else if ((int)operationPeriod == 5) observationId = "MY38";
      else if ((int)operationPeriod == 5) observationId = "MY38";
      else if ((int)operationPeriod == 6) observationId = "TBD";
      else if ((int)operationPeriod == 7) observationId = "TEST";
      else observationId = "UNK";

      long long orbitNumber = (stoll(uniqueId) & 268433408);
      orbitNumber /= pow(2,11);
      observationId += "_";
      std::string orbitString = to_string(orbitNumber);
      orbitString.insert(orbitString.begin(), 6 - orbitString.length(), '0');
      observationId += orbitString;

      int orbitPhase = (stoll(uniqueId) & 2044);
      transform(target.begin(), target.end(), target.begin(), ::tolower);
      if (target.compare("mars") == 0) {
        orbitPhase /= pow(2,2);
      }
      else {
        orbitPhase = 900;
      }
      observationId += "_";
      observationId += to_string(orbitPhase);

      int imageType = (stoll(uniqueId) & 3);
      observationId += "_";
      observationId += to_string(imageType);

      return observationId;
    });

   /**
    * Removes 'Z' that is added to StartTime when image has been reingested
    */
    env.add_callback("RemoveStartTimeZ", 1, [](Arguments& args) {
      std::string startTime = args.at(0)->get<string>();

      if(startTime.back() == 'Z') {
        startTime.pop_back();
      }

      return startTime;
    });


    /**
     * Add ImageNumber to Archive Group based on StartTime and ProductId.
     *
     *   Last digit of the year (eg, 1997 => 7), followed by the
     *   Day of the year (Julian day) followed by the
     *   Last five digits of the ProductId
     */
     env.add_callback("SetImageNumber", 2, [](Arguments& args) {
       std::string yearDoy = args.at(0)->get<string>();
       std::string productId = args.at(1)->get<string>();

       // grab the last digit of the year
       std::string imageNumber = yearDoy.substr(3, 1);
       // grab the DOY
       imageNumber += yearDoy.substr(4, 3);
       // grab the last 5 digits of productId
       imageNumber += productId.substr(4);

       return imageNumber;
     });


     /**
      * Add SubFrame keyword to Instrument Group based on substring of ImageNumber.
      */
     env.add_callback("SetSubFrame", 1, [](Arguments& args) {
       std::string imageNumber = args.at(0)->get<string>();

       // grab the last digit of the year
       std::string subFrame = imageNumber.substr(5);

       return subFrame;
     });


     /**
      * Add ImageKeyId to Archive Group based on StartTime and ProductId
      */
      env.add_callback("SetImageKeyId", 2, [](Arguments& args) {
        std::string clockCount = args.at(0)->get<string>();
        std::string productId = args.at(1)->get<string>();

        std::string imageKeyId = clockCount.substr(0, 5) + productId.substr(4);

        return imageKeyId;
      });


    /**
     * Remove units from keyword value if exists at the end of the string.
     */
    env.add_callback("RemoveUnits", 1, [](Arguments& args){

      std::string stringToRemove = args.at(0)->get<string>();

      while(isalpha(stringToRemove.back())){
        stringToRemove.pop_back();
      }

      return stringToRemove;
    });


    /**
     * Returns character at specified index in String.
     */
    env.add_callback("CharAt", 2, [](Arguments& args){
      std::string inputString = args.at(0)->get<string>();
      int index = args.at(1)->get<int>();

      return inputString.substr(index, 1);
    });
     // end of inja callbacks


    ProcessImport importer;
    if (inputFileName.removeExtension().addExtension("dat").fileExists()){
      importer.SetInputFile(inputFileName.removeExtension().addExtension("dat").expanded());
    }
    else if (inputFileName.removeExtension().addExtension("img").fileExists()) {
      importer.SetInputFile(inputFileName.removeExtension().addExtension("img").expanded());
    }
    else if (inputFileName.removeExtension().addExtension("QUB").fileExists()) {
      importer.SetInputFile(inputFileName.removeExtension().addExtension("QUB").expanded());
    }
    else {
      importer.SetInputFile(inputFileName.expanded());
    }

    // Use inja to get number of lines, samples, and bands from the input label
    std::string result;
    try {
      result = env.render_file(inputTemplate.expanded().toStdString(), jsonData);
    }
    catch(const std::exception& e) {
      QString msg = "Unable to create a cube label from [";
      msg += inputTemplate.expanded() + "]. ";
      msg += e.what();
      if (ui.WasEntered("TEMPLATE")) {
        throw IException(IException::User, msg, _FILEINFO_);
      }
      else {
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    // Turn this into a Pvl label
    Pvl newLabel;
    newLabel.fromString(result);

    PvlObject translation = newLabel.findObject("Translation");
    if (translation.hasKeyword("Failure")) {
      throw IException(IException::Io, QString(translation.findKeyword("Failure")), _FILEINFO_);
    }

    // Set everything needed by ProcessImport
    PvlGroup dimensions = newLabel.findObject("IsisCube").findObject("Core").findGroup("Dimensions");
    int ns = toInt(dimensions["Samples"]);
    int nl = toInt(dimensions["Lines"]);
    int nb = toInt(dimensions["Bands"]);
    importer.SetDimensions(ns, nl, nb);

    PvlGroup pixels = newLabel.findObject("IsisCube").findObject("Core").findGroup("Pixels");
    QString pixelType = pixels["Type"];
    QString byteOrder = pixels["ByteOrder"];
    double base = pixels["Base"];
    double multiplier = pixels["Multiplier"];
    importer.SetPixelType(PixelTypeEnumeration(pixelType));
    importer.SetByteOrder(ByteOrderEnumeration(byteOrder));
    importer.SetBase(base);
    importer.SetMultiplier(multiplier);

    // Update TargetName if Target parameter entered
    if (ui.WasEntered("TARGET")) {
      PvlGroup &inst = newLabel.findGroup("Instrument",Pvl::Traverse);
      inst["TargetName"] = ui.GetString("TARGET");
    }

    // Check translation for potential PDS3 offset
    if (translation.hasKeyword("DataFilePointer")) {
      PvlKeyword dataFilePointer = translation["DataFilePointer"];

      int offset = 0;
      int recSize = 1;
      QString units = "BYTES";

      if (dataFilePointer.size() == 1) {
        try {
          offset = toInt(dataFilePointer) - 1;
          units = dataFilePointer.unit();
        }
        catch(IException &e) {
          // Failed to parse to an int, means we have a file name
          // No offset given, so we use 1, offsets are 1 based
          offset = 0;
          units = "BYTES";
        }
      }
      else if (dataFilePointer.size() == 2) {
        offset = toInt(dataFilePointer[1]) - 1;
        units = dataFilePointer.unit(1);
      }
      else {
        QString msg = "Improperly formatted data file pointer keyword ^IMAGE or "
                     "^QUBE, in [" + inputFileName.toString() + "], must contain filename "
                     " or offset or both";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }

      // Now, to handle the offset
      units = units.trimmed();
      if (units == "BYTES" || units == "B") {
        recSize = 1;
      }
      else {
        if (translation.hasKeyword("DataFileRecordBytes")) {
          recSize = toInt(translation["DataFileRecordBytes"]);
        }
      }
      importer.SetFileHeaderBytes(offset * recSize);
      importer.SaveFileHeader();
    }
    // Assume PDS4
    else {
      importer.SetFileHeaderBytes(0);
    }

    // Checks that are unique to mgsmoc
    if (translation.hasKeyword("compressed") && translation.hasKeyword("projected")) {
      if (toBool(translation["compressed"])) {
        QString msg = "[" + inputFileName.name() + "] may be compressed. Please run image through mocuncompress to uncompress.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      if (toBool(translation["projected"])) {
        QString msg = "[" + inputFileName.name() + "] appears to be an rdr file.";
        msg += " Use pds2isis.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    // Processing unique to mroctx
    if (translation.hasKeyword("DataPrefixBytes")) {
      importer.SetDataPrefixBytes(translation["DataPrefixBytes"]);
      if (toInt(translation["DataPrefixBytes"]) > 0) {
        importer.SaveDataPrefix();
      }
    }

    if (translation.hasKeyword("DataSuffixBytes")) {
      importer.SetDataSuffixBytes(translation["DataSuffixBytes"]);
      if (toInt(translation["DataSuffixBytes"]) > 0) {
        importer.SaveDataSuffix();
      }
    }

    if (translation.hasKeyword("CoreAxisNames")) {
      QString originalAxisOrder = QString(translation["CoreAxisNames"]);
      if (originalAxisOrder == "SAMPLELINEBAND") {
        importer.SetOrganization(ProcessImport::BSQ);
      }
      else if (originalAxisOrder == "BANDSAMPLELINE") {
        importer.SetOrganization(ProcessImport::BIP);
      }
      else if (originalAxisOrder == "SAMPLEBANDLINE") {
        importer.SetOrganization(ProcessImport::BIL);
      }
      else {
        stringstream pdsOrgStream;
        pdsOrgStream << originalAxisOrder;

        QString msg = "Unsupported axis order [" + QString(originalAxisOrder) + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }
    QString str;
    // Set any special pixel values
    double pdsNull = Isis::NULL8;
    if (translation.hasKeyword("CoreNull")) {
      str = QString(translation["CoreNull"]);
      if(str != "NULL") {
        pdsNull = toDouble(str);
      }
    }

    double pdsLrs = Isis::Lrs;
    if (translation.hasKeyword("CoreLRS")) {
      str = QString(translation["CoreLRS"]);
      if(str != "NULL") {
        pdsLrs = toDouble(str);
      }
    }

    double pdsLis = Isis::Lis;
    if (translation.hasKeyword("CoreLIS")) {
      str = QString(translation["CoreLIS"]);
      if(str != "NULL") {
        pdsLis = toDouble(str);
      }
    }

    double pdsHrs = Isis::Hrs;
    if (translation.hasKeyword("CoreHRS")) {
      str = QString(translation["CoreHRS"]);
      if(str != "NULL") {
        pdsHrs = toDouble(str);
      }
    }

    double pdsHis = Isis::His;
    if (translation.hasKeyword("CoreHIS")) {
      str = QString(translation["CoreHIS"]);
      if(str != "NULL") {
        pdsHis = toDouble(str);
      }
    }
    importer.SetSpecialValues(pdsNull, pdsLrs, pdsLis, pdsHrs, pdsHis);

    QString cubeAtts = "";
    if (translation.hasKeyword("CubeAtts")) {
      cubeAtts = QString(translation["CubeAtts"]);
    }
    CubeAttributeOutput outputAtts = CubeAttributeOutput(cubeAtts);
    Cube *outputCube = importer.SetOutputCube(ui.GetCubeName("TO"), outputAtts);

    if (isPDS4) {
      OriginalXmlLabel xmlLabel;
      xmlLabel.readFromXmlFile(inputFileName);
      outputCube->write(xmlLabel);
    }
    // Assume PDS3
    else {
      Pvl pdsLab(inputFileName.expanded());
      OriginalLabel pds3Label(pdsLab);
      outputCube->write(pds3Label);
    }
    importer.StartProcess();

    // Write the updated label
    Isis::PvlObject &newCubeLabel = newLabel.findObject("IsisCube");
    Isis::Pvl &outLabel(*outputCube->label());
    Isis::PvlObject &outCubeLabel = outLabel.findObject("IsisCube");

    for(int g = 0; g < newCubeLabel.groups(); g++) {
      outCubeLabel.addGroup(newCubeLabel.group(g));
    }

    if (translation.hasObject("AncillaryProcess")) {
      for(int i = 0; i < translation.objects(); ++i) {
        PvlObject &object = translation.object(i);
        QString objectName = object.name();
        if (objectName == "AncillaryProcess") {
          applyAncillaryProcess(outputCube, QString(object["ProcessFunction"]), translation, &importer);
        }
      }
    }

    importer.EndProcess();

    if (translation.hasObject("PostProcess")) {
      // Expand for potentially more than one prefix process
      // See AncillaryProcess section above
      runProcess(ui.GetCubeName("TO"), translation.findObject("PostProcess"));
    }

    return;
  }
}
