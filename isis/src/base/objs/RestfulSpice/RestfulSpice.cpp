
#include <iostream>
#include <Preference.h>
#include "RestfulSpice.h"
#include "restincurl.h"
#include <string>

using json=nlohmann::json;

namespace Isis::RestfulSpice{
  std::map<std::string, std::string> spiceql_mission_map = {
    {"AMICA", "amica"},
    {"CHANDRAYAAN-1_M3", "m3"},
    {"CHANDRAYAAN-1_MRFFR", "mrffr"},
    {"CASSINI_ISS_NAC", "cassini"},
    {"CASSINI_ISS_WAC", "cassini"},
    {"DAWN_FC2_FILTER_1", "fc2"},
    {"DAWN_FC2_FILTER_2", "fc2"},
    {"DAWN_FC2_FILTER_3", "fc2"},
    {"DAWN_FC2_FILTER_4", "fc2"},
    {"DAWN_FC2_FILTER_5", "fc2"},
    {"DAWN_FC2_FILTER_6", "fc2"},
    {"DAWN_FC2_FILTER_7", "fc2"},
    {"DAWN_FC2_FILTER_8", "fc2"},
    {"GLL_SSI_PLATFORM", "galileo"},
    {"HAYABUSA_AMICA", "amica"},
    {"HAYABUSA_NIRS", "nirs"},
    {"HAYABUSA2_ONC-W2", ""},
    {"JUNO_JUNOCAM", "juno"},
    {"JUPITER", "voyager1"},
    {"LRO_LROCNACL", "lroc"},
    {"LRO_LROCNACR", "lroc"},
    {"LRO_LROCWAC_UV", "lroc"},
    {"LRO_LROCWAC_VIS", "lroc"},
    {"LRO_MINIRF", ""},
    {"M10_VIDICON_A", "m10_vidicon_a"},
    {"M10_VIDICON_B", "m10_vidicon_b"},
    {"MARS", "mro"},
    {"MSGR_MDIS_WAC", "mdis"},
    {"MSGR_MDIS_NAC", "mdis"},
    {"MEX_HRSC_SRC", "src"},
    {"MEX_HRSC_IR", "hrsc"},
    {"MGS_MOC_NA", "mgs"},
    {"MGS_MOC_WA_RED", "mgs"},
    {"MGS_MOC_WA_BLUE", "mgs"},
    {"MOON", "apollo15"},
    {"MRO_MARCI_VIS", "marci"},
    {"MRO_MARCI_UV", "marci"},
    {"MRO_CTX", "ctx"},
    {"MRO_HIRISE", "hirise"},
    {"MRO_CRISM_VNIR", "crism"},
    {"NEAR EARTH ASTEROID RENDEZVOUS", ""},
    {"MSL_MASTCAM_RIGHT", ""},
    {"MSL_MASTCAM_LEFT", ""},
    {"NH_LORRI", "lorri"},
    {"NH_RALPH_LEISA", "leisa"},
    {"NH_MVIC", "mvic_tdi"},
    {"ISIS_NH_RALPH_MVIC_METHANE", "mvic_framing"},
    {"THEMIS_IR", "odyssey"},
    {"THEMIS_VIS", "odyssey"},
    {"ORX_OCAMS_MAPCAM", ""},
    {"ORX_OCAMS_POLYCAM", ""},
    {"ORX_OCAMS_SAMCAM", ""},
    {"LISM_MI-VIS1", "kaguya"},
    {"LISM_MI-VIS2", "kaguya"},
    {"LISM_MI-VIS3", "kaguya"},
    {"LISM_MI-VIS4", "kaguya"},
    {"LISM_MI-VIS5", "kaguya"},
    {"LISM_MI-NIR1", "kaguya"},
    {"LISM_MI-NIR2", "kaguya"},
    {"LISM_MI-NIR3", "kaguya"},
    {"LISM_MI-NIR4", "kaguya"},
    {"LISM_TC1_WDF", "kaguya"},
    {"LISM_TC1_WTF", "kaguya"},
    {"LISM_TC1_SDF", "kaguya"},
    {"LISM_TC1_STF", "kaguya"},
    {"LISM_TC1_WDN", "kaguya"},
    {"LISM_TC1_WTN", "kaguya"},
    {"LISM_TC1_SDN", "kaguya"},
    {"LISM_TC1_STN", "kaguya"},
    {"LISM_TC1_WDH", "kaguya"},
    {"LISM_TC1_WTH", "kaguya"},
    {"LISM_TC1_SDH", "kaguya"},
    {"LISM_TC1_STH", "kaguya"},
    {"LISM_TC1_SSH", "kaguya"},
    {"LO1_HIGH_RESOLUTION_CAMERA", ""},
    {"LO2_HIGH_RESOLUTION_CAMERA", ""},
    {"LO3_HIGH_RESOLUTION_CAMERA", ""},
    {"LO4_HIGH_RESOLUTION_CAMERA", ""},
    {"LO5_HIGH_RESOLUTION_CAMERA", ""},
    {"NEPTUNE", "voyager1"}, 
    {"SATURN", "voyager1"},
    {"TGO_CASSIS", "cassis"},
    {"VIKING ORBITER 1", "viking1"},
    {"VIKING ORBITER 2", "viking2"},
    {"VG1_ISSNA", ""},
    {"VG1_ISSWA", ""},
    {"VG2_ISSNA", ""},
    {"VG2_ISSWA", ""},
    {"ULTRAVIOLET/VISIBLE CAMERA", "uvvis"},
    {"Near Infrared Camera", "nir"},
    {"High Resolution Camera", "clementine1"},
    {"Long Wave Infrared Camera", "clementine1"},
    {"Visual and Infrared Spectrometer", "vir"}
  };


  std::vector<std::vector<double>> getTargetStates(std::vector<double> ets, std::string target, std::string observer, std::string frame, std::string abcorr, std::string mission, std::string ckQuality, std::string spkQuality){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      // @TODO validity checks
      json args = json::object({
          {"target", target},
          {"observer", observer},
          {"frame", frame},
          {"abcorr", abcorr},
          {"ets", ets},
          {"mission", mission},
          {"ckQuality", ckQuality},
          {"spkQuality", spkQuality}
          });
      // @TODO check that json exists / contains what we're looking for
      json out = spiceAPIQuery("getTargetStates", args);
      return out["body"]["return"].get<std::vector<std::vector<double>>>();
    }else{
      return SpiceQL::getTargetStates(ets, target, observer, frame, abcorr, mission, ckQuality, spkQuality, true);
    }
  }

  std::vector<std::vector<double>> getTargetOrientations(std::vector<double> ets, int toFrame, int refFrame, std::string mission, std::string ckQuality) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"ets", ets},
        {"toFrame", toFrame},
        {"refFrame", refFrame},
        {"mission", mission},
        {"ckQuality", ckQuality}
      });
      json out = spiceAPIQuery("getTargetOrientations", args);
      return out["body"]["return"].get<std::vector<std::vector<double>>>();
    }else{
      return SpiceQL::getTargetOrientations(ets, toFrame, refFrame, mission, ckQuality, true);
    }
  }

  double strSclkToEt(int frameCode, std::string sclk, std::string mission) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"frameCode", frameCode},
        {"sclk", sclk},
        {"mission", mission}
      });
      json out = spiceAPIQuery("strSclkToEt", args);
      return out["body"]["return"].get<double>();
    }else{
      return SpiceQL::strSclkToEt(frameCode, sclk, mission, true);
    }
  }

  double doubleSclkToEt(int frameCode, double sclk, std::string mission){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"frameCode", frameCode},
        {"sclk", sclk},
        {"mission", mission}
      });
      json out = spiceAPIQuery("doubleSclkToEt", args);
      return out["body"]["return"].get<double>();
   }else{
      return SpiceQL::doubleSclkToEt(frameCode, sclk, mission, true);
    }
  }

  double utcToEt(std::string utc){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"utc", utc}
      });
      json out = spiceAPIQuery("utcToEt", args);
      return out["body"]["return"].get<double>();

    }else{
      return SpiceQL::utcToEt(utc, true);
    }
  }


  std::string etToUtc(double et, std::string format, double precision){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    // TODO Add etToUtc to web api
    if (false){
      json args = json::object({
        {"et", et},
        {"format", format},
        {"precision", precision}
      });
      json out = spiceAPIQuery("etToUtc", args);
      return out["body"]["return"].get<std::string>();
    }else{
      return SpiceQL::etToUtc(et, format, precision, true);
    }
  }

  std::string doubleEtToSclk(int frameCode, double et, std::string mission) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"frameCode", frameCode},
        {"et", et},
        {"mission", mission}
      });
      json out = spiceAPIQuery("doubleEtToSclk", args);
      return out["body"]["return"].get<std::string>();
    }else{
      return SpiceQL::doubleEtToSclk(frameCode, et, mission, true);
    }

  }

  int translateNameToCode(std::string frame, std::string mission){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"frame", frame},
        {"mission", mission}
      });
      json out = spiceAPIQuery("translateNameToCode", args);
      return out["body"]["return"].get<int>();
    }else{
      return SpiceQL::translateNameToCode(frame, mission, true);
    }
  }

  std::string translateCodeToName(int code, std::string mission){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"code", code},
        {"mission", mission}
      });
      json out = spiceAPIQuery("translateCodeToame", args);
      return out["body"]["return"].get<std::string>();
    }else{
      return SpiceQL::translateCodeToName(code, mission, true);
    }
  }

  std::vector<int> getFrameInfo(int frame, std::string mission) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"frame", frame},
        {"mission", mission}
      });
      json out = spiceAPIQuery("getFrameInfo", args);
      return out["body"]["return"].get<std::vector<int>>();

    }else{
      return SpiceQL::getFrameInfo(frame, mission, true);
    }
  }

  json getTargetFrameInfo(int targetId, std::string mission) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"targetId", targetId},
        {"mission", mission}
      });
      json out = spiceAPIQuery("getTargetFrameInfo", args);
      return out["body"]["return"];
    }else{
      return SpiceQL::getTargetFrameInfo(targetId, mission, true);
    }
  }

  json findMissionKeywords(std::string key, std::string mission){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"key", key},
        {"mission", mission}
      });
      json out = spiceAPIQuery("findMissionKeywords", args);
      return out["body"]["return"];
    }else{
      return SpiceQL::findMissionKeywords(key, mission, true);
    }
  }

  json findTargetKeywords(std::string key, std::string mission){
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"key", key},
        {"mission", mission}
      });
      json out = spiceAPIQuery("findTargetKeywords", args);
      return out["body"]["return"];
    }else{
      return SpiceQL::findTargetKeywords(key, mission, true);
    }
  }

  std::vector<std::vector<int>> frameTrace(double et, int initialFrame, std::string mission, std::string ckQuality) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"et", et},
        {"initialFrame", initialFrame},
        {"mission", mission},
        {"ckQuality", ckQuality}
      });
      json out = spiceAPIQuery("frameTrace", args);
      return out["body"]["return"].get<std::vector<std::vector<int>>>();
    }else{
      return SpiceQL::frameTrace(et, initialFrame, mission, ckQuality, true);
    }
  }

  std::vector<double> extractExactCkTimes(double observStart, double observEnd, int targetFrame, std::string mission, std::string ckQuality) {
    bool useWeb = QString(Preference::Preferences().findGroup("WebSpice")["UseWebSpice"]).toUpper() == "TRUE";
    if (useWeb){
      json args = json::object({
        {"observStart", observStart},
        {"observEnd", observEnd},
        {"targetFrame",targetFrame},
        {"mission", mission},
        {"ckQuality", ckQuality}
      });
      json out = spiceAPIQuery("extractExactCkTimes", args);
      return out["body"]["return"].get<std::vector<double>>();
    }else{
      return SpiceQL::extractExactCkTimes(observStart, observEnd, targetFrame, mission, ckQuality, true);
    }
  }

  json spiceAPIQuery(std::string functionName, json args, std::string method){
    restincurl::Client client;
    // std::string queryString = "https://spiceql-slot1.prod-asc.chs.usgs.gov/" + functionName;
    std::string queryString = "127.0.0.1:8080/" + functionName;

    json j;

    if (method == "GET"){
      std::cout << "[RestfulSpice] spiceAPIQuery GET" << std::endl;
      queryString += "?";
      for (auto x : args.items()) {
          queryString+= x.key();
          queryString+= "=";
          queryString+= x.value().dump();
          queryString+= "&";
      }
      std::string encodedString = url_encode(queryString);
      std::cout << "[RestfulSpice] spiceAPIQuery encodedString = " << encodedString << std::endl;
      client.Build()->Get(encodedString).Option(CURLOPT_FOLLOWLOCATION, 1L).AcceptJson().WithCompletion([&](const restincurl::Result& result) {
        std::cout << "[RestfulSpice] spiceAPIQuery GET result = " << result.body << std::endl;
        j = json::parse(result.body);
      }).ExecuteSynchronous();
    }else{
      std::cout << "[RestfulSpice] spiceAPIQuery POST" << std::endl;
      client.Build()->Post(queryString).Option(CURLOPT_FOLLOWLOCATION, 1L).AcceptJson().WithJson(args.dump()).WithCompletion([&](const restincurl::Result& result) {
        std::cout << "[RestfulSpice] spiceAPIQuery POST result = " << result.body << std::endl;
        j = json::parse(result.body);
      }).ExecuteSynchronous();
    }
    client.CloseWhenFinished();
    client.WaitForFinish();

    // @TODO throw exception if no json or invalid json is returned
    return j;
  }

  std::string url_encode(const std::string &value) {
      std::ostringstream escaped;
      escaped.fill('0');
      escaped << std::hex;

      for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
          std::string::value_type c = (*i);

          // Keep alphanumeric and other accepted characters intact
          if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '&' || c == '/' || c == '?' || c == '=' || c == ':') {
              escaped << c;
              continue;
          }

          if (c == '"'){
            continue;
          }

          // Any other characters are percent-encoded
          escaped << std::uppercase;
          escaped << '%' << std::setw(2) << int((unsigned char) c);
          escaped << std::nouppercase;
      }

      return escaped.str();
  }

}
