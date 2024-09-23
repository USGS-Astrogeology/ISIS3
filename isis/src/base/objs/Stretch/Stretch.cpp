/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>

#include <QDebug>

#include "Stretch.h"
#include "Histogram.h"
#include "IString.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "Pvl.h"

using namespace std;
namespace Isis {

  /**
   * Constructs a Stretch object with default mapping of special pixel values to
   * themselves.
   */
  Stretch::Stretch() {
    p_null = Isis::NULL8;
    p_lis = Isis::LOW_INSTR_SAT8;
    p_lrs = Isis::LOW_REPR_SAT8;
    p_his = Isis::HIGH_INSTR_SAT8;
    p_hrs = Isis::HIGH_REPR_SAT8;
    p_minimum = p_lrs;
    p_maximum = p_hrs;
    p_pairs = 0;
  }


  /**
   * Adds a stretch pair to the list of pairs. Note that all input pairs must be
   * in ascending order.
   *
   * @param input Input value to map
   *
   * @param output Output value when the input is mapped
   *
   * @throws Isis::IException::Programmer - input pairs must be in ascending
   *                                        order
   */
  void Stretch::AddPair(const double input, const double output) {
    if(p_pairs > 0) {
      if(input <= p_input[p_pairs-1]) {
        string msg = "Input pairs must be in ascending order";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    p_input.push_back(input);
    p_output.push_back(output);
    p_pairs++;
  }

  /**
   * Maps an input value to an output value based on the stretch pairs and/or
   * special pixel mappings.
   *
   * @param value Value to map
   *
   * @return double The mapped output value is returned by this method
   */
  double Stretch::Map(const double value) const {
    // Check special pixels first
    if(!Isis::IsValidPixel(value)) {
      if(Isis::IsNullPixel(value)) return p_null;
      if(Isis::IsHisPixel(value)) return p_his;
      if(Isis::IsHrsPixel(value)) return p_hrs;
      if(Isis::IsLisPixel(value)) return p_lis;
      return p_lrs;
    }

    // Check to see if we have any pairs
    if(p_input.size() == 0) return value;

    // Check to see if outside the minimum and maximum next
    if(value < p_input[0]) {
      if(!Isis::IsValidPixel(p_minimum)) {
        if(Isis::IsNullPixel(p_minimum)) return p_null;
        if(Isis::IsHisPixel(p_minimum)) return p_his;
        if(Isis::IsHrsPixel(p_minimum)) return p_hrs;
        if(Isis::IsLisPixel(p_minimum)) return p_lis;
        return p_lrs;
      }
      return p_minimum;
    }

    if(value > p_input[p_pairs-1]) {
      if(!Isis::IsValidPixel(p_maximum)) {
        if(Isis::IsNullPixel(p_maximum)) return p_null;
        if(Isis::IsHisPixel(p_maximum)) return p_his;
        if(Isis::IsHrsPixel(p_maximum)) return p_hrs;
        if(Isis::IsLisPixel(p_maximum)) return p_lis;
        return p_lrs;
      }
      return p_maximum;
    }

    // Check the end points
    if(value == p_input[0]) return p_output[0];
    if(value == p_input[p_pairs-1]) return p_output[p_pairs-1];

    // Ok find the surrounding pairs with a binary search
    int start = 0;
    int end = p_pairs - 1;
    while(start != end) {
      int middle = (start + end) / 2;

      if(middle == start) {
        end = middle;
      }
      else if(value < p_input[middle]) {
        end = middle;
      }
      else {
        start = middle;
      }
    }

    end = start + 1;

    // Apply the stretch
    double slope = (p_output[end] - p_output[start]) / (p_input[end] - p_input[start]);
    return slope * (value - p_input[end]) + p_output[end];
  }

  /**
  * Given a string containing stretch pairs for example "0:0 50:0 100:255 255:255"
  * evaluate the first pair and return a pair of doubles where first is the first
  * input and second is the first output
  * @param pairs A string containing stretch pairs for example
  *              "0:0 50:0 100:255 255:255"
  *
  * @throws Isis::IException::User - invalid stretch pair
  *
  * @return std::pair of doubles where first is the first input and
  *         second is the first output
  */
  std::pair<double, double> Stretch::NextPair(QString &pairs) {
    std::pair<double, double> io;
    io.first = Null;
    io.second = Null;

    pairs = pairs.simplified().trimmed();

    if (pairs.contains(":")) {
      QStringList pairList = pairs.split(" ");

      QString firstPair = pairList.takeFirst();

      QStringList firstPairValues = firstPair.split(":");

      if (firstPairValues.count() == 2) {
        io.first = firstPairValues.first().toDouble();
        io.second = firstPairValues.last().toDouble();

        pairs = pairList.join(" ");
      }
    }

    return io;
  }

  /**
   * Parses a string of the form "i1:o1 i2:o2...iN:oN" where each i:o
   * represents an input:output pair. Therefore, the user can enter a string in
   * this form and this method will parse the string and load the stretch pairs
   * into the object via AddPairs.
   *
   * @param pairs A string containing stretch pairs for example
   *              "0:0 50:0 100:255 255:255"
   *
   * @throws Isis::IException::User - invalid stretch pair
   */
  void Stretch::Parse(const QString &pairs) {
    // Zero out the stretch arrays
    p_input.clear();
    p_output.clear();
    p_pairs = 0;

    std::pair<double, double> pear;

    QString p = pairs.simplified().trimmed();
    p.replace(QRegExp("[\\s]*:"), ":");
    p.replace(QRegExp(":[\\s]*"), ":");
    QStringList pairList = p.split(" ", Qt::SkipEmptyParts);

    try {
      foreach(QString singlePair, pairList) {
        pear = Stretch::NextPair(singlePair);
        Stretch::AddPair(pear.first, pear.second);
      }
    }
    catch(IException &e) {
      throw IException(e, IException::User, "Invalid stretch pairs [" + pairs.toStdString() + "]", _FILEINFO_);
    }
  }

  /**
   * Parses a string of the form "i1:o1 i2:o2...iN:oN" where each i:o
   * represents an input:output pair where the input is a percentage.  Using
   * the Histogram an appropriate dn value will be calculated for each input
   * percentage. Therefore, the user can enter a string in this form and this
   * method will parse the string and load the stretch pairs into the object
   * via AddPairs.
   *
   * @param pairs A string containing stretch pairs for example
   *              "0:0 50:0 100:255"
   *
   * @throws Isis::IException::User - invalid stretch pair
   */
  void Stretch::Parse(const QString &pairs, const Isis::Histogram *hist) {
    // Zero out the stretch arrays
    p_input.clear();
    p_output.clear();
    p_pairs = 0;

    QString p(pairs);
    std::pair<double, double> pear;

    // need to save the input dn values in order to
    // to detect collisions
    std::vector<double> converted;

    try {
      while(p.size() > 0) {
        pear = Stretch::NextPair(p);
        pear.first = hist->Percent(pear.first);

        // test for collision!
        // if collision occurs then ignore this pair and move on
        // to next pair
        bool collision = false;
        unsigned int k = 0;
        while(!collision && k < converted.size()) {
          if(pear.first == converted[k]) {
            collision = true;
          }
          else {
            k++;
          }
        }
        if(!collision) {
          Stretch::AddPair(pear.first, pear.second);
          converted.push_back(pear.first);
        }
      }
    }

    catch(IException &e) {
      throw IException(e, IException::User, "Invalid stretch pairs [" +
                       pairs.toStdString() + "]", _FILEINFO_);
    }
  }


  /**
   * Converts stretch pair to a string
   *
   * @return string The stretch pair as a string
   */
  QString Stretch::Text() const {

    if(p_pairs < 0) return "";

    QString p("");
    for(int i = 0; i < p_pairs; i++) {
      p += QString::number(p_input[i]) + ":" + QString::number(p_output[i]) + " ";
    }
    return p.trimmed();
  }

  /**
   * Returns the value of the input side of the stretch pair at the specified
   * index. If the index number is out of bounds, then the method returns -1
   *
   * @param index The index number to retrieve the input stretch pair value from
   *
   * @return double The input side of the stretch pair at the specified index
   */
  double Stretch::Input(int index) const {
    if(index >= p_pairs || index < 0) {
      return -1;
    }
    return p_input[index];
  }

  /**
   * Returns the value of the output side of the stretch pair at the specified
   * index. If the index number is out of bounds, then the method returns -1.
   *
   * @param index The index number to retieve the output stretch pair value from
   *
   * @return double The output side of the stretch pair at the specified index
   */
  double Stretch::Output(int index) const {
    if(index >= p_pairs || index < 0) {
      return -1;
    }
    return p_output[index];
  }

  /**
   * Loads the stretch pairs from the pvl file into the Stretch
   * object.  The file should look similar to this:
   * @code
   * Group = Pairs
   *   Input = (0,100,255)
   *   Output = (255,100,0)
   * EndGroup
   * @endcode
   *
   * @param file - The input file containing the stretch pairs
   * @param grpName - The group name to get the input and output
   *                keywords from
   */
  void Stretch::Load(QString &file, QString &grpName) {
    Pvl pvl(file.toStdString());
    Load(pvl, grpName);
  }

  /**
   * Loads the stretch pairs from the pvl file into the Stretch
   * object.  The pvl should look similar to this:
   * @code
   * Group = Pairs
   *   Input = (0,100,255)
   *   Output = (255,100,0)
   * EndGroup
   * @endcode
   *
   * @param pvl - The pvl containing the stretch pairs
   * @param grpName - The group name to get the input and output
   *                keywords from
   */
  void Stretch::Load(Isis::Pvl &pvl, QString &grpName) {
    PvlGroup grp = pvl.findGroup(grpName.toStdString(), Isis::PvlObject::Traverse);
    PvlKeyword inputs = grp.findKeyword("Input");
    PvlKeyword outputs = grp.findKeyword("Output");

    if(inputs.size() != outputs.size()) {
      std::string msg = "Invalid Pvl file: The number of Input values must equal the number of Output values";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    for(int i = 0; i < inputs.size(); i++) {
      AddPair(Isis::toDouble(inputs[i]), Isis::toDouble(outputs[i]));
    }
  }


  /**
   * Saves the stretch pairs in the Stretch object into the given
   * pvl file
   *
   * @param file - The file that the stretch pairs will be written
   *             to
   * @param grpName - The name of the group to create and put the
   *                stretch pairs into.  The group will contain
   *                two keywords, Input, and Output.
   */
  void Stretch::Save(QString &file, QString &grpName) {
    Pvl p;
    Save(p, grpName);
    p.write(file.toStdString());
  }

  void Stretch::Save(Isis::Pvl &pvl, QString &grpName) {
    PvlGroup *grp = new PvlGroup(grpName.toStdString());
    PvlKeyword inputs("Input");
    PvlKeyword outputs("Output");
    for(int i = 0; i < Pairs(); i++) {
      inputs.addValue(Isis::toString(Input(i)));
      outputs.addValue(Isis::toString(Output(i)));
    }
    grp->addKeyword(inputs);
    grp->addKeyword(outputs);
    pvl.addGroup(*grp);
  }

  /**
   * Copies the stretch pairs from another Stretch object, but maintains special
   * pixel values
   *
   * @param other - The Stretch to copy pairs from
   */
  void Stretch::CopyPairs(const Stretch &other) {
    this->p_pairs = other.p_pairs;
    this->p_input = other.p_input;
    this->p_output = other.p_output;
  }
} // end namespace isis
