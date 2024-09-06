/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlToJSON.h"

#include <nlohmann/json.hpp>

#include "Pvl.h"
#include "PvlKeyword.h"
#include "IException.h"

using json = nlohmann::json;
using namespace std;

namespace Isis {

  /**
   * Convert the contents of a PvlKeyword to a JSON object.
   * All values from the keyword will be stored in "Value", all units will be
   * stored in "Units", and all comments will be stored in "Comment".
   *
   * How a keyword with only a value is converted
   *
   * PvlKeyword:
   * <pre>
   * ExposureDuration = 10
   * </pre>
   *
   * JSON:
   * <pre>
   * {"Value":"10"}
   * </pre>
   *
   * How single values, units, and comments are converted
   *
   * PvlKeyword:
   * <pre>
   * # The exposure duration of the image
   * ExposureDuration = 10 <ms>
   * </pre>
   *
   * JSON:
   * <pre>
   * {"Comment":"# The exposure duration of the image",
   *  "Units":"ms",
   *  "Value":"10"}
   * </pre>
   *
   * How multiple values, units, and comments are converted
   *
   * PvlKeyword:
   * <pre>
   * # First comment
   * # Second comment
   * TestKey2 = ("This keyword has multiple comments" <first unit>,
   *             "It also has multiple values",
   *             "It even has values with and without units" <third unit>)
   * </pre>
   *
   * JSON:
   * <pre>
   * {"Comment":["# First comment",
   *             "# Second comment"],
   *  "Units":["first unit",
   *           "",
   *           "third unit"],
   *  "Value":["This keyword has multiple comments",
   *           "It also has multiple values",
   *           "It even has values with and without units"]}
   * </pre>
   *
   * @param keyword The keyword to convert
   * @return @b json The contents of the keyword as a JSON object
   */
  json pvlKeywordToJSON(PvlKeyword &keyword) {
    json jsonKeyword;

    // Convert values
    if (keyword.size() == 1) {
      jsonKeyword["Value"] = keyword[0];
    }
    else if (keyword.size() > 1) {
      json valueList;
      for (int i = 0; i < keyword.size(); i++) {
        valueList.push_back(keyword[i]);
      }
      jsonKeyword["Value"] = valueList;
    }

    // Optionally convert units
    if (keyword.size() == 1 && !QString::fromStdString(keyword.unit(0)).isEmpty()) {
      jsonKeyword["Units"] = keyword.unit(0);
    }
    else if (keyword.size() > 1 && !QString::fromStdString(keyword.unit(0)).isEmpty()) {
      json valueList;
      for (int i = 0; i < keyword.size(); i++) {
        valueList.push_back(keyword.unit(i));
      }
      jsonKeyword["Units"] = valueList;
    }

    // Optionally convert comments
    if (keyword.comments() == 1) {
      jsonKeyword["Comment"] = keyword.comment(0);
    }
    else if (keyword.comments() > 1) {
      json commentList;
      for (int i = 0; i < keyword.comments(); i++) {
        commentList.push_back(keyword.comment(i));
      }
      jsonKeyword["Comment"] = commentList;
    }
    return jsonKeyword;
  }


  /**
   * Convert the contents of a PvlContainer to a JSON object.
   * Any comments in the container will be stored in "Comment".
   * Comments associated with keywords will be stored inside their json object.
   * If a keyword is repeated in the container, then the instances will be
   * packed into an array in the order that they occur.
   *
   * This function is used by the PvlGroup, PvlObject, and Pvl conversion
   * functions; see their documentation for examples.
   *
   * @param container The container to convert
   * @return @b json The contents of the container as a JSON object
   */
  json pvlContainerToJSON(PvlContainer &container) {
    json jsonContainer;

    // Convert keywords
    PvlContainer::PvlKeywordIterator keywordIt;
    for (keywordIt = container.begin(); keywordIt != container.end(); keywordIt++) {
      string keywordName = QString::fromStdString(keywordIt->name()).replace("^", "ptr").replace(":", "_").toStdString();
      // Handle repeated keywords by packing them into an array
      if ( jsonContainer.contains(keywordName) ) {
        if (!jsonContainer[keywordName].is_array()) {
          json repeatedArray;
          repeatedArray.push_back(jsonContainer[keywordName]);
          jsonContainer[keywordName] = repeatedArray;
        }
        jsonContainer[keywordName].push_back(pvlKeywordToJSON(*keywordIt));
      }
      else {
          jsonContainer[keywordName] = pvlKeywordToJSON(*keywordIt);
      }
    }

    // Optionally convert comments
    if (container.comments() == 1) {
      jsonContainer["Comment"] = container.comment(0);
    }
    else if (container.comments() > 1) {
      json commentList;
      for (int i = 0; i < container.comments(); i++) {
        commentList.push_back(container.comment(i));
      }
      jsonContainer["Comment"] = commentList;
    }
    return jsonContainer;

  }


  /**
   * Convert the contents of a PvlGroup to a JSON object.
   * Any comments in the group will be stored in "Comment".
   * Comments associated with keywords will be stored inside their json object.
   *
   * A simple example group
   *
   * PvlGroup:
   * <pre>
   * Group = TestGroup
   *   TestKey1 = A
   *   TestKey2 = 1
   * End_Group
   * </pre>
   *
   * JSON:
   * <pre>
   * {"TestKey1":{"Value":"A"},
   *  "TestKey2":{"Value":"1"}}
   * </pre>
   *
   * If a keyword is repeated in the group, then the instances will be packed
   * into an array in the order that they occur.
   *
   * An example group with repeated keywords
   *
   * PvlGroup:
   * <pre>
   * Group = TestGroup
   *   TestKey1 = A
   *   TestKey2 = 1
   *   TestKey2 = 2
   * End_Group
   * </pre>
   *
   * JSON:
   * <pre>
   * {"TestKey1":{"Value":"A"},
   *  "TestKey2":[{"Value":"1"},
   *              {"Value":"2"}]}
   * </pre>
   *
   * @param group The group to convert
   * @return @b json The contents of the group as a JSON object
   */
  json pvlGroupToJSON(PvlGroup &group) {
    // PvlGroups are just PvlContainers with extra input/output options
    // so we can just use the PvlContainer conversion directly.
    return pvlContainerToJSON(group);;
  }


  /**
   * Convert the contents of a PvlObject to a JSON object.
   * Any comments in the base object will be stored in "Comment".
   * Comments associated with keywords, groups, or nested objects will be
   * stored inside their associated JSON object.
   *
   * An example demonstrating how nested objects and groups are converted
   *
   * PvlObject:
   * <pre>
   * Object = TestObject2
  *    TestKey3 = "hello world"
  *
  *    Object = TestObject1
  *      TestKey1 = A
  *      TestKey2 = 1
  *    End_Object
  *
  *    Group = TestGroup
  *      TestKey3 = "hello world"
  *    End_Group
  *  End_Object
   * </pre>
   *
   * JSON:
   * <pre>
   * {"TestGroup":{"TestKey3":{"Value":"hello world"}},
   *  "TestKey3":{"Value":"hello world"},
   *  "TestObject1":{"TestKey1":{"Value":"A"},
   *                 "TestKey2":{"Value":"1"}}}
   * </pre>
   *
   * If there are keywords, groups, and/or nested objects with the same name at
   * the same level in the object, then they will be stored in an array starting
   * with the keywords, followed by the groups, and then finally the objects.
   * Within each subset, the repeated elements will be ordered the same as they
   * occur in the object.
   *
   * An example with repeated element names at the same level
   *
   * PvlObject:
   * <pre>
   * Object = TestObject2
   *   TestKey3  = "hello world"
   *   TestGroup = Q
   *
   *   Object = TestObject1
   *     TestKey1 = A
   *     TestKey2 = 1
   *   End_Object
   *
   *   Object = TestGroup
   *     TestKey2 = 1
   *   End_Object
   *
   *   Group = TestGroup
   *     TestKey3 = "hello world"
   *   End_Group
   * End_Object
   * </pre>
   *
   * JSON:
   * <pre>
   * {"TestGroup":[{"Value":"Q"},
   *               {"TestKey3":{"Value":"hello world"}},
   *               {"TestKey2":{"Value":"1"}}],
   *  "TestKey3":{"Value":"hello world"},
   *  "TestObject1":{"TestKey1":{"Value":"A"},
   *                 "TestKey2":{"Value":"1"}}}
   * </pre>
   *
   * @param object The object to convert
   * @return @b json The contents of the object as a JSON object
   */
  json pvlObjectToJSON(PvlObject &object) {
    // Convert keywords and comments
    json jsonObject = pvlContainerToJSON(object);

    // Convert groups
    PvlObject::PvlGroupIterator groupIt;
    for (groupIt = object.beginGroup(); groupIt != object.endGroup(); groupIt++) {
      // Handle repeated elements by packing them into an array
      if ( jsonObject.contains(groupIt->name()) ) {
        if (!jsonObject[groupIt->name()].is_array()) {
          json repeatedArray;
          repeatedArray.push_back(jsonObject[groupIt->name()]);
          jsonObject[groupIt->name()] = repeatedArray;
        }
        jsonObject[groupIt->name()].push_back(pvlGroupToJSON(*groupIt));
      }
      else {
        jsonObject[groupIt->name()] = pvlGroupToJSON(*groupIt);
      }
    }

    // Convert nested objects
    PvlObject::PvlObjectIterator objectIt;
    for (objectIt = object.beginObject(); objectIt != object.endObject(); objectIt++) {
      // Handle repeated elements by packing them into an array
      if ( jsonObject.contains(objectIt->name()) ) {
        if (!jsonObject[objectIt->name()].is_array()) {
          json repeatedArray;
          repeatedArray.push_back(jsonObject[objectIt->name()]);
          jsonObject[objectIt->name()] = repeatedArray;
        }
        jsonObject[objectIt->name()].push_back(pvlObjectToJSON(*objectIt));
      }
      else {
        jsonObject[objectIt->name()] = pvlObjectToJSON(*objectIt);
      }
    }
    return jsonObject;
  }


  /**
   * Convert the contents of a Pvl to a JSON object.
   * Any comments in the root of the Pvl will be stored in "Comment".
   * Comments associated with keywords, groups, or objects will be stored
   * inside their associated JSON object. If there are keywords, groups,
   * and/or objects with the same name at the same level in the Pvl, then
   * they will be stored in an array starting with the keywords, followed by the
   * groups, and then finally the objects. Within each subset, the repeated
   * elements will be ordered the same as they occur in the object.
   *
   * @param pvl The Pvl to convert
   * @return @b json The contents of the Pvl as a JSON object
   */
  json pvlToJSON(Pvl &pvl) {
    return pvlObjectToJSON(pvl);
  }

  /**
   * Converts an PVL file to a json object.
   *
   *
   * @param pvlFile Path to an PVL file.
   *
   * @return json The pvl file converted to a json object.
   */
  json pvlToJSON(QString pvlFile) {
    Pvl pvl;

    try {
      pvl.read(pvlFile.toStdString());
    }
    catch (IException &e){
      std::string msg = QString("Failed to open file for PVL Input: [%1]").arg(pvlFile);
      throw IException(e, IException::User, msg, _FILEINFO_);
    }

    return pvlToJSON(pvl);
  }
}
