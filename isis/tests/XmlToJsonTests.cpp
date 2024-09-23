#include <nlohmann/json.hpp>

#include "XmlToJson.h"

#include "gmock/gmock.h"

using json = nlohmann::json;
using namespace Isis;

// XML: <tag>value</tag>
// JSON: {tag: value}
TEST(XmlToJson, XmlNoAttributeWithTextValue) {
  QString xmlInput = R"(
    <TagWithoutAttribute>textValue</TagWithoutAttribute>
    )";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  EXPECT_EQ(result["TagWithoutAttribute"], "textValue");
}

// XML: <tag attributeName="attributeValue">textValue</tag>
// JSON: {tag: {attrib_attributeName: "attributeValue, "_text":textValue } }
TEST(XmlToJson, XmlAttributeWithTextValue) {
  QString xmlInput = R"(<Tag>
    <TagWithAttribute attribute="attributeValue">textValue</TagWithAttribute>
    <Tag>)";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  EXPECT_EQ(result["Tag"]["TagWithAttribute"]["attrib_attribute"], "attributeValue");
  EXPECT_EQ(result["Tag"]["TagWithAttribute"]["_text"], "textValue");
}

// XML: <tag attributeName="attributeValue" />
//  JSON: {tag: {attrib_attributeName: "attributeValue"} }
TEST(XmlToJson, XmlAttributeButNoText) {
  QString xmlInput = R"(<Tag>
    <TagWithAttribute attribute="attributeValue" />>
    <Tag>)";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  EXPECT_EQ(result["Tag"]["TagWithAttribute"]["attrib_attribute"], "attributeValue");
  EXPECT_EQ(result["Tag"]["TagWithAttribute"]["_text"], nullptr);
}

//  XML: <tag />
//  JSON: tag: null
TEST(XmlToJson, XmlNoTextValueNoAttribute) {
  QString xmlInput = R"(<Tag>
    <TagWithoutAnythingElse />
    <Tag>)";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  EXPECT_EQ(result["Tag"]["TagWithoutAnythingElse"], nullptr);
}

// XML:  <tag><a>value</a><b>otherValue</b></tag>
// JSON: {tag: {a:value, b:otherValue}}
TEST(XmlToJson, XmlNestedTags) {
 QString xmlInput = R"(<OuterTag>
      <TagLevel1>
        <TagLevel2A>TagLevel2AValue</TagLevel2A>
        <TagLevel2B>TagLevel2BValue</TagLevel2B>
        <TagLevel2C> 
          <TagLevel3>
            <TagLevel4>DeepValue</TagLevel4>
          </TagLevel3>
       </TagLevel2C>
     </TagLevel1>
  </OuterTag>)";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);
  
  EXPECT_EQ(result["OuterTag"]["TagLevel1"]["TagLevel2A"], "TagLevel2AValue");
  EXPECT_EQ(result["OuterTag"]["TagLevel1"]["TagLevel2B"], "TagLevel2BValue");
  EXPECT_EQ(result["OuterTag"]["TagLevel1"]["TagLevel2C"]["TagLevel3"]["TagLevel4"], "DeepValue");
}

// XML: <tag><a>value</a><a>otherValue</a></tag>
// JSON: {tag: {a: [value, otherValue]}}
TEST(XmlToJson, TestRepeatedTagNoChildren){
  QString xmlInput = R"(<Tag>
     <A>A1</A>
     <A>A2</A>
     <A attribute="value"/>
     <A otherAttribute="otherValue">textValue</A>
     <A>
       <B>b1</B>
       <B>b2</B>
       <C>c1</C>
     </A>
     <A>A3</A>
     <A /></Tag>)";
   
  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  EXPECT_EQ(result["Tag"]["A"][0], "A1");
  EXPECT_EQ(result["Tag"]["A"][1], "A2");
  EXPECT_EQ(result["Tag"]["A"][2]["attrib_attribute"], "value");
  EXPECT_EQ(result["Tag"]["A"][3]["attrib_otherAttribute"], "otherValue");
  EXPECT_EQ(result["Tag"]["A"][3]["_text"], "textValue");
  EXPECT_EQ(result["Tag"]["A"][4]["B"][0], "b1");
  EXPECT_EQ(result["Tag"]["A"][4]["B"][1], "b2");
  EXPECT_EQ(result["Tag"]["A"][4]["C"], "c1");
  EXPECT_EQ(result["Tag"]["A"][5], "A3");
  EXPECT_EQ(result["Tag"]["A"][6], nullptr);
}

// XML: <tag><a><b>value</b></a><a><c>otherValue</c></a></tag>
// JSON: {tag: { a: [{b: value}, {c: otherValue}]} }
TEST(XmlToJson, TestRepeatedTagWithChildren){
  QString xmlInput = R"(<Tag>
      <a><b>value</b></a>
      <a><c>otherValue</c></a>
      <a><justTag /> </a>
      </Tag>)";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  EXPECT_EQ(result["Tag"]["a"][0]["b"], "value");
  EXPECT_EQ(result["Tag"]["a"][1]["c"], "otherValue");
  EXPECT_EQ(result["Tag"]["a"][2]["justTag"], nullptr);
}

// This tests that all the sub-pieces tested above work together in a single XMl document
TEST(XmlToJson, TestXMLEverythingTogether) {
  QString xmlInput = R"(<TagLevel0>
  <TagLevel1A>
    <TagLevel2A>TagLevel2AValue</TagLevel2A>
    <TagLevel2B>TagLevel2BValue</TagLevel2B>
    <TagLevel2Extra attr="justAnAttribute" />
    <TagLevel2ExtraExtra />
    <TagLevel2C>
      <TagLevel3>
        <TagLevel4A>TagLevel4AValue</TagLevel4A>
        <TagLevel4B>TagLevel4BValue</TagLevel4B>
        <TagLevel4C> <TagLevel4D> <TagLevel4E>DeepValue</TagLevel4E></TagLevel4D></TagLevel4C>
     </TagLevel3>
  </TagLevel2C>
  <TagLevel2D attributeTag2D="Attribute value">TagLevel2DValue</TagLevel2D>
  </TagLevel1A>
  <TagLevel1B>
      <First>
       <A>A1</A>
       <A>A2</A>
       <A attribute="value"/>
       <A attr="val">zoom</A>
       <A>
         <B>b1</B>
         <B>b2</B>
         <C>notlist</C>
       </A>
       <A>A3</A>
       <A />
       <ten>10</ten>
       <ten>TEN</ten>
       <oddball>notrepeated</oddball>
      </First>
      <First>
        <tweleve>12</tweleve>
        <thirteen>13</thirteen>
      </First>
      <First>
        <fourteen>14</fourteen>
        <fifteen>15</fifteen>
      </First>
      <Second>
          <A attributeA="A" attributeB="B" attributeC="C">ElementValue</A>
      </Second>
      <Third>
        <Greek attr="alphabet" otherattr="firstLetter" >
          <Alpha attr1="attr1" attr2="attr2">AlphaValue</Alpha>
          <Beta attrbeta1="beta1" attrbeta2="beta2">BetaValue</Beta>
          <AnotherLevel>
            <Gamma attrgamma="gamma">GammaValue</Gamma>
          </AnotherLevel>
        </Greek>
      </Third>
  </TagLevel1B>
  </TagLevel0>)";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  // Test deeply nested value retrieval
  EXPECT_EQ(result["TagLevel0"]["TagLevel1A"]["TagLevel2C"]["TagLevel3"]["TagLevel4C"]["TagLevel4D"]["TagLevel4E"], "DeepValue");

  // Test attributes (uncomplicated)
  EXPECT_EQ(result["TagLevel0"]["TagLevel1A"]["TagLevel2D"]["attrib_attributeTag2D"], "Attribute value");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1A"]["TagLevel2D"]["_text"], "TagLevel2DValue");

  // Test no-text value cases <tag /> and <tag attributeName="attributeValue" /> 
  EXPECT_EQ(result["TagLevel0"]["TagLevel1A"]["TagLevel2ExtraExtra"], nullptr);
  EXPECT_EQ(result["TagLevel0"]["TagLevel1A"]["TagLevel2Extra"]["attrib_attr"], "justAnAttribute");

  // Test list creation for repeated tags at the same level
  
  // Case A: <a><b>bcontents</b></a> <a><c>cContents</c></a> JSON: a: [ {b: bcontents}, {c: cContents} ]
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][1]["tweleve"], "12");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][1]["thirteen"], "13");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][2]["fourteen"], "14");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][2]["fifteen"], "15");

  // Case B: <z><a>aContents1</a><a>aContents2</a></a> JSON: z: {a: [aContents1, aContents2] }
  // including lots of possible combinations
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][0], "A1");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][1], "A2");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][2]["attrib_attribute"], "value");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][3]["attrib_attr"], "val");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][3]["_text"], "zoom");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][5], "A3");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][6], nullptr);

  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["oddball"], "notrepeated");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][4]["B"][0], "b1");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][4]["B"][1], "b2");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["A"][4]["C"], "notlist");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["ten"][0], "10");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["First"][0]["ten"][1], "TEN");

  // Test many attributes at one level
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Second"]["A"]["attrib_attributeA"], "A");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Second"]["A"]["attrib_attributeB"], "B");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Second"]["A"]["attrib_attributeC"], "C");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Second"]["A"]["_text"], "ElementValue");

  // Test multiple attributes at multiple levels
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["attrib_otherattr"], "firstLetter");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["attrib_attr"], "alphabet");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["Alpha"]["attrib_attr2"], "attr2");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["Alpha"]["attrib_attr1"], "attr1");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["Alpha"]["_text"], "AlphaValue");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["Beta"]["_text"], "BetaValue");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["Beta"]["attrib_attrbeta2"], "beta2");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["AnotherLevel"]["Gamma"]["attrib_attrgamma"], "gamma");
  EXPECT_EQ(result["TagLevel0"]["TagLevel1B"]["Third"]["Greek"]["AnotherLevel"]["Gamma"]["_text"], "GammaValue");
}

// This tests that all the sub-pieces tested above work together in a single XMl document
TEST(XmlToJson, TestXMLWithNamespace) {
  QString xmlInput = R"(
  <Discipline_Area>
    <disp:Display_Settings>
      <Local_Internal_Reference>
        <local_identifier_reference>eis000xxx_2032116t234928_0000c35f-wac-pushb-img_raw</local_identifier_reference>
        <local_reference_type>display_settings_to_array</local_reference_type>
      </Local_Internal_Reference>
      <disp:Display_Direction>
        <disp:horizontal_display_axis>Sample</disp:horizontal_display_axis>
        <disp:horizontal_display_direction>Left to Right</disp:horizontal_display_direction>
        <disp:vertical_display_axis>Line</disp:vertical_display_axis>
        <disp:vertical_display_direction>Top to Bottom</disp:vertical_display_direction>
      </disp:Display_Direction>
    </disp:Display_Settings>
    <img:Exposure>
      <img:exposure_duration unit="ms">27.54</img:exposure_duration>
    </img:Exposure>
  </Discipline_Area>
  )";

  QDomDocument xmlDocument("TestDocument");
  xmlDocument.setContent(xmlInput);
  json result = xmlToJson(xmlDocument);

  // Test deeply nested value retrieval
  std::cout << result.dump(2) << std::endl;
}


