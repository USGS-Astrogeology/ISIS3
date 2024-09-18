#include <nlohmann/json.hpp>

#include "FitsToJson.h"

#include "gmock/gmock.h"
#include "IException.h"

using json = nlohmann::json;
using namespace Isis;


TEST(FitsToJson, FitsConversion) {
  FileName fileTemplate = "data/lorri2isis/lor_0034974377_0x630_sci_1_cropped.fit";
  json jsonData;
  try {
    jsonData = fitsToJson(fileTemplate);
  }
  catch (IException &e) {
    FAIL() << "Unable to convert lorri fits label to json " <<  e.toString().c_str() << std::endl;
  }
  // Instrument info
  EXPECT_EQ(jsonData["FitsLabels"][0]["HOSTNAME"]["Value"], "NEW HORIZONS");
  EXPECT_EQ(jsonData["FitsLabels"][0]["INSTRU"]["Value"], "lor");
  EXPECT_EQ(jsonData["FitsLabels"][0]["TARGET"]["Value"], "IO");
  EXPECT_EQ(jsonData["FitsLabels"][0]["SPCUTCAL"]["Value"], "2007-02-28T13:14:19.358");
  EXPECT_EQ(jsonData["FitsLabels"][0]["SPCSCLK"]["Value"], "1/0034974376:48500");
  EXPECT_EQ(jsonData["FitsLabels"][0]["SPCSCDP"]["Value"], "1748718848500.0");
  EXPECT_EQ(jsonData["FitsLabels"][0]["EXPTIME"]["Value"], "0.02");

  // Archive info
  EXPECT_EQ(jsonData["FitsLabels"][0]["HSCOMPR"]["Value"], "LOSSLESS");
  EXPECT_EQ(jsonData["FitsLabels"][0]["OBSCOMPL"]["Value"], "COMPLETE");
  EXPECT_EQ(jsonData["FitsLabels"][0]["REQDESC"]["Value"], "Jupiter shine");

  // Kernels info
  EXPECT_EQ(jsonData["FitsLabels"][0]["FORMAT"]["Value"], "0");


  fileTemplate = "data/leisa2isis/lsb_0034933739_0x53c_sci_1_cropped.fit";
  try {
    jsonData = fitsToJson(fileTemplate);
  }
  catch (IException &e) {
    FAIL() << "Unable to convert fits leisa label to json " <<  e.toString().c_str() << std::endl;
  }

  // Instrument info
  EXPECT_EQ(jsonData["FitsLabels"][0]["HOSTNAME"]["Value"], "NEW HORIZONS");
  EXPECT_EQ(jsonData["FitsLabels"][0]["INSTRU"]["Value"], "lei");
  EXPECT_EQ(jsonData["FitsLabels"][0]["TARGET"]["Value"], "EUROPA");
  EXPECT_EQ(jsonData["FitsLabels"][0]["SPCSCLK0"]["Value"], "1/0034931099:00000");
  EXPECT_EQ(jsonData["FitsLabels"][0]["RALPHEXP"]["Value"], "0.676");

  // Archive info
  EXPECT_EQ(jsonData["FitsLabels"][0]["SPCSCET"]["Value"], "225897372.0736388");
  EXPECT_EQ(jsonData["FitsLabels"][0]["DURMET"]["Value"], "251.0");
  EXPECT_EQ(jsonData["FitsLabels"][0]["DETECTOR"]["Value"], "LEISA");
  EXPECT_EQ(jsonData["FitsLabels"][0]["SCANTYPE"]["Value"], "LEISA");

  // BandBin info
  EXPECT_EQ(jsonData["FitsLabels"][0]["FILTER"]["Value"], "WEDGE");

  // Kernels info
  EXPECT_EQ(jsonData["FitsLabels"][0]["SPCINSID"]["Value"], "-98201");
}
