#include "Isis.h"

#include <sstream>
#include <string>
#include <float.h>
#include <fstream>

#include "ProcessByLine.h"

#include "Statistics.h"
#include "Pvl.h"
#include "IException.h"
#include "WriteTabular.h"
#include "IString.h"
#include "Pixel.h"

using namespace Isis;

void compare(std::vector<Buffer *> &in, std::vector<Buffer *> &out);

void diffTable(std::ofstream &target, int precision);

// This is only used for the difference table
struct Difference {
  int lineNum;
  int sampNum;
  double cube1Val;
  double cube2Val;
};

double tolerance;
bool filesEqual = true;
bool firstDifferenceFound = false; // Set to true when first DN value difference is found
int sample, line, band, spCount, diffCount, colWidth;
Statistics stats;
bool doTable;
unsigned int sigFigAccuracy = DBL_DIG; // DBL_DIG is maximum accuracy for a double
std::vector<Difference> diffset;
int sigFigLine = 0;
int sigFigSample = 0;
int sigFigBand = 0;

int gMaxDiffLine = 0, gMaxDiffSample = 0, gMaxDiffBand = 0;
double gMaxDiff;

void IsisMain() {
  // Set up the two input cubes
  ProcessByLine p;
  p.SetInputCube("FROM");
  p.SetInputCube("FROM2", SizeMatch);

  // Read tolerance value
  UserInterface &ui = Application::GetUserInterface();
  if(ui.WasEntered("TOLERANCE")) {
    tolerance = ui.GetDouble("TOLERANCE");
  }
  else {
    tolerance = DBL_EPSILON;
  }

  // See if we should output the difference table
  if(ui.GetBoolean("OUTPUTDIFFS")) {
    doTable = true;
    diffCount = ui.GetInteger("COUNT");
    if(!ui.WasEntered("TO")) {
      std::string message = "A target file is required for difference output";
      throw IException(IException::User, message, _FILEINFO_);
    }

  }

  // Compare the cubes
  filesEqual = true;
  firstDifferenceFound = false;
  spCount = 0;
  stats.Reset();
  colWidth = 0;
  gMaxDiff = tolerance;
  p.StartProcess(compare);

  // Write to log indicating if two files are filesEqual.
  PvlGroup results("Results");
  if(filesEqual) {
    results += PvlKeyword("Compare", "Identical");
  }
  else {
    results += PvlKeyword("Compare", "Different");
    results += PvlKeyword("Sample", std::to_string(sample));
    results += PvlKeyword("Line", std::to_string(line));
    results += PvlKeyword("Band", std::to_string(band));
    if(stats.TotalPixels() < 1) {
      results += PvlKeyword("AverageDifference", "0");
      results += PvlKeyword("StandardDeviation", "0");
      results += PvlKeyword("Variance", "0");
      results += PvlKeyword("MinimumDifference", "0");
      results += PvlKeyword("MaximumDifference", "0");
    }
    else {
      results += PvlKeyword("AverageDifference", std::to_string(stats.Average()));
      results += PvlKeyword("StandardDeviation", std::to_string(stats.StandardDeviation()));
      results += PvlKeyword("Variance", std::to_string(stats.Variance()));
      results += PvlKeyword("MinimumDifference", std::to_string(stats.Minimum()));
      results += PvlKeyword("MaximumDifference", std::to_string(stats.Maximum()));
      results += PvlKeyword("MaxDifferenceSample", std::to_string(gMaxDiffSample));
      results += PvlKeyword("MaxDifferenceLine", std::to_string(gMaxDiffLine));
      results += PvlKeyword("MaxDifferenceBand", std::to_string(gMaxDiffBand));
    }
    results += PvlKeyword("ValidPixelDifferences", std::to_string(stats.TotalPixels()));
    results += PvlKeyword("SpecialPixelDifferences", std::to_string(spCount));
    results += PvlKeyword("SigFigAccuracy", std::to_string(sigFigAccuracy));
    results += PvlKeyword("SigFigMaxDifferenceSample", std::to_string(sigFigSample));
    results += PvlKeyword("SigFigMaxDifferenceLine", std::to_string(sigFigLine));
    results += PvlKeyword("SigFigMaxDifferenceBand", std::to_string(sigFigBand));
  }
  Application::Log(results);

  // Output a file if the user request it
  if(ui.WasEntered("TO")) {
    Pvl lab;
    lab.addGroup(results);
    lab.write(ui.GetFileName("TO", "txt").toStdString());
  }
  if(doTable) {
    std::string filename = FileName(ui.GetFileName("TO", "txt").toStdString()).expanded();
    std::ofstream ofile(filename.c_str(), std::ios_base::app);
    diffTable(ofile, ui.GetInteger("PRECISION"));
  }

  p.EndProcess();
  filesEqual = true;
}

void compare(std::vector<Buffer *> &in, std::vector<Buffer *> &out) {
  Buffer &input1 = *in[0];
  Buffer &input2 = *in[1];
  int inputSize = input1.size();
  double MaxDiffTemp;

  for(int index = 0; index < inputSize; index ++) {
    bool pixelDifferent = false;
    bool pixelSpecial = false;

    // First check if there is a special pixel in either cube
    if(Pixel::IsSpecial(input1[index]) || Pixel::IsSpecial(input2[index])) {
      pixelSpecial = true;

      // We have special pixels, if they are both special compare them.
      if(Pixel::IsSpecial(input1[index]) && Pixel::IsSpecial(input2[index])) {
        if(input1[index] != input2[index]) {
          spCount ++;
          pixelDifferent = true;
        }
      }
      // At least one is special, but not both, so they must be different
      else {
        spCount ++;
        pixelDifferent = true;
      }
    }
    // We don't have any special pixels, run against tolerance
    else {
      MaxDiffTemp = abs(input1[index] - input2[index]);
      if(MaxDiffTemp > tolerance) {
        // This pixel is different.
        pixelDifferent = true;

        // Add the difference in dn to the stats object
        stats.AddData(MaxDiffTemp);

        // Store line, sample and band of max difference
        if(MaxDiffTemp > gMaxDiff) {
          gMaxDiff = MaxDiffTemp;
          gMaxDiffLine   = input1.Line(index);
          gMaxDiffSample = input1.Sample(index);
          gMaxDiffBand   = input1.Band(index);
        }
      }
    }

    // If pixels different & neither are special, calculate the significant figure difference
    if(pixelDifferent && !pixelSpecial) {
      unsigned int accuracy = 0;

      // Check positive/negative and ensure both positive
      if((input1[index] < 0.0 && input2[index] > 0.0) ||
          (input1[index] > 0.0 && input2[index] < 0.0)) {
        accuracy = 1;
      }
      else {
        double in1 = abs(input1[index]);
        double in2 = abs(input2[index]);
        int in1log = (int)floor(log10(in1));
        int in2log = (int)floor(log10(in2));

        // Check for zeros
        if(input1[index] == 0.0 || input2[index] == 0.0) {
          accuracy = 0;
        }
        // Check for different decimal places
        else if(in1log != in2log) {
          accuracy = 0;
        }
        else {
          // int values of log of original - log of difference = # sig fig accuracy
          // *The difference can not equal zero because pixelDifferent flag is set to true
          accuracy = in1log - (int)floor(log10(abs(in1 - in2)));
        }
      }

      if(accuracy < sigFigAccuracy) {
        sigFigSample = input1.Sample(index);
        sigFigLine = input1.Line(index);
        sigFigBand = input1.Band(index);
        sigFigAccuracy = accuracy;
      }
    }

    if(pixelDifferent) {
      filesEqual = false;

      if(!firstDifferenceFound) {
        firstDifferenceFound = true;
        sample = input1.Sample(index);
        line = input1.Line(index);
        band = input1.Band(index);
      }
    }

    // If the user indicated that we should make the table, add this entry
    if(pixelDifferent && doTable) {
      if((int)diffset.size() < diffCount) {
        Difference currDiff;
        currDiff.lineNum = input1.Line(index);
        currDiff.sampNum = input1.Sample(index);
        currDiff.cube1Val = input1[index];
        currDiff.cube2Val = input2[index];
        diffset.push_back(currDiff);

        // Check the character lengths of the values
        int val1Length = IString((int)currDiff.cube1Val).length();
        int val2Length = IString((int)currDiff.cube2Val).length();
        if(val1Length < colWidth) {
          colWidth = val1Length;
        }
        if(val2Length < colWidth) {
          colWidth = val2Length;
        }
      }
    }
  }
}

//Function to prepare the table to append to the
void diffTable(std::ofstream &target, int precision) {
  std::vector<int> temp;

  //Make a list of all samples present
  for(unsigned int i = 0; i < diffset.size(); i++) {
    temp.push_back(diffset[i].sampNum);
  }

  //Sort the list
  sort(temp.begin(), temp.end());

  //Remove duplicates
  std::vector<int> samps;
  samps.push_back(temp[0]);
  for(unsigned int i = 1; i < temp.size(); i++) {
    if(temp[i] != samps.back()) {
      samps.push_back(temp[i]);
    }
  }
  std::vector<Column> cols;
  //Add the first Column
  Column first("Line#", 7, Column::Integer);
  cols.push_back(first);

  for(unsigned int i = 0; i < samps.size(); i++) {
    Column currCol;
    //Prepare and add the first file's column
    currCol.SetName(QString::fromStdString("File1_" + std::to_string(samps[i])));
    if((int)(colWidth + precision + 1) < (int)currCol.Name().length()) {
      currCol.SetWidth(currCol.Name().length() + 1);
    }
    else currCol.SetWidth(colWidth + precision + 1);

    currCol.SetType(Column::Pixel);
    currCol.SetAlignment(Column::Decimal);
    currCol.SetPrecision(precision);
    cols.push_back(currCol);

    //Prepare and add the second file's column
    currCol.SetName(QString::fromStdString("File2_" + std::to_string(samps[i])));
    cols.push_back(currCol);
  }

  WriteTabular diffs(target, cols);

  //Go through the entire list of differences and make the table
  for(unsigned int i = 0; i < diffset.size(); i++) {
    diffs.Write(diffset[i].lineNum);
    for(unsigned int j = 0; j < samps.size(); j++) {
      if(diffset[i].sampNum == samps[j]) {
        diffs.Write(diffset[i].cube1Val);
        diffs.Write(diffset[i].cube2Val);
      }
      else {
        diffs.Write();
        diffs.Write();
      }
    }
  }
}
