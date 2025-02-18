#pragma once

#include <functional>
#include <map>
#include <QString>
#include <spiceql.h>
#include <typeinfo>
#include <typeindex>
#include <vector>

#include <nlohmann/json.hpp>

using json=nlohmann::json;

namespace Isis::RestfulSpice {
  extern std::map<std::string, std::string> spiceql_mission_map; 

  std::vector<std::vector<double>> getTargetStates(std::vector<double> ets, std::string target, std::string observer, std::string frame, std::string abscorr, std::string mission, std::string ckQuality, std::string spkQuality, std::vector<std::string> kernel_list={});
  std::vector<std::vector<double>> getTargetOrientations(std::vector<double> ets, int toFrame, int refFrame, std::string mission, std::string ckQuality);
  double strSclkToEt(int frameCode, std::string sclk, std::string mission);
  double doubleSclkToEt(int frameCode, double sclk, std::string mission);
  double utcToEt(std::string utc);
  std::string etToUtc(double et, std::string format, double precision);
  std::string doubleEtToSclk(int frameCode, double et, std::string mission);
  int translateNameToCode(std::string frame, std::string mission);
  std::string translateCodeToName(int code, std::string mission);
  std::vector<int> getFrameInfo(int frame, std::string mission) ;
  json getTargetFrameInfo(int targetId, std::string mission);
  json findMissionKeywords(std::string key, std::string mission);
  json findTargetKeywords(std::string key, std::string mission);
  std::vector<std::vector<int>> frameTrace(double et, int initialFrame, std::string mission, std::string ckQuality);
  std::vector<double> extractExactCkTimes(double observStart, double observEnd, int targetFrame, std::string mission, std::string ckQuality);

  json spiceAPIQuery(std::string functionName, json args, std::string method="GET");
  std::string url_encode(const std::string &value);
}