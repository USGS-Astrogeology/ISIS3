// system include files go first
#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

// Isis specific include files go next
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "Pvl.h"
#include "TextFile.h"
#include "Statistics.h"

#include "bandnorm.h"

using namespace std;

namespace Isis {

  static vector<int> band;
  static vector<double> average;
  static vector<double> normalizer;
  
  // function prototypes
  static void getStats(Buffer &in);
  static void normalize(Buffer &in, Buffer &out);
  static void Tokenize(const QString &str,
                vector<QString> & tokens,
                const QString &delimiters = " ");
  
  void bandnorm(UserInterface &ui) {
    Cube icube(ui.GetCubeName("FROM").toStdString(), "r");
    bandnorm(&icube, ui);
  }
  
  
  void bandnorm(Cube *icube, UserInterface &ui) {
    // We will be processing by line
    ProcessByLine p;
  
    // Now get the statistics for each band or the entire cube
    QString avg = ui.GetString("AVERAGE");
    p.SetInputCube(icube);
    p.StartProcess(getStats);
    if(avg == "BAND") {
      int b = 0;
      Statistics stats;
      for(int i = 0; i < (int)average.size(); i++) {
        if(b == band[i]) {
          stats.AddData(&average[i], (unsigned int)1);
        }
        else {
          normalizer.push_back(stats.Average());
          b++;
          stats.Reset();
        }
      }
      normalizer.push_back(stats.Average());
    }
    else if(avg == "PENCIL") {
      TextFile pencil;
      pencil.Open(ui.GetFileName("SPECTRUM"));
      std::cout << pencil.LineCount() << " " << icube->bandCount() << std::endl;
      if(pencil.LineCount() - 1 < icube->bandCount()) {
        std::string msg = "The spectral pencil file [" + ui.GetAsString("SPECTRUM").toStdString() +
                      "] does not contain enough data for all bands.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      QString st;
      int column = -1;
      vector<QString> tokens;
      pencil.GetLine(st);     //Takes care of title line
      Tokenize(st, tokens, ", \"-+");
      if(ui.GetAsString("METHOD") == "number") {
        column = ui.GetInteger("NUMBER");
      }
      else {
        for(unsigned i = 0; i < tokens.size(); i++) {
          if(tokens[i] == ui.GetString("NAME")) {
            column = i;
        break;
          }
        }
      }
      if(column < 0  || (unsigned)column > tokens.size()) {
        std::string msg = "The column specified in file [" + ui.GetFileName("SPECTRUM").toStdString()
                      + "] was not found.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      // Add the correct column of data to normalizer
      for(int i = 0; i < icube->bandCount(); i++) {
        tokens.clear();
        pencil.GetLine(st);
        Tokenize(st, tokens, ", \"");
        std::cout << "col: " << column << std::endl;
        std::cout << tokens[column].toDouble() << std::endl;
        normalizer.push_back(tokens[column].toDouble());
      }
    }
    else {  // avg == "CUBE"
      Statistics stats;
      for(int i = 0; i < (int)average.size(); i++) {
        stats.AddData(&average[i], (unsigned int)1);
      }
      for(int b = 0; b < icube->bandCount(); b++) {
        normalizer.push_back(stats.Average());
      }
    }
  
    // Setup the output file and apply the correction
    p.SetOutputCube(ui.GetCubeName("TO"), ui.GetOutputAttribute("TO"), icube->sampleCount(), icube->lineCount(), icube->bandCount());
    p.StartProcess(normalize);
  
    // Cleanup
    p.EndProcess();
    normalizer.clear();
    band.clear();
    average.clear();
  }
  
  //**********************************************************
  // Get statistics on a band or entire cube
  //**********************************************************
  void getStats(Buffer &in) {
    Statistics stats;
    stats.AddData(in.DoubleBuffer(), in.size());
    average.push_back(stats.Average());
    band.push_back(in.Band() - 1);
  }
  
  // Apply coefficients
  void normalize(Buffer &in, Buffer &out) {
    int index = in.Band() - 1;
    double coeff = normalizer[index];
  
    // Now loop and apply the coefficents
    for(int i = 0; i < in.size(); i++) {
      if(IsSpecial(in[i])) {
        out[i] = in[i];
      }
      else {
        out[i] = Null;
        if(coeff != 0.0 && IsValidPixel(coeff)) {
          out[i] = in[i] / coeff;
        }
      }
    }
  }
  
  // Tokenizer
  void Tokenize(const QString &strQStr,
                vector<QString> & tokens,
                const QString &delimitersQStr) {
    IString str = strQStr.toStdString();
    IString delimiters = delimitersQStr.toStdString();
  
    //Skip delimiters at the beginning
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);
  
    while(string::npos != pos  ||  string::npos != lastPos) {
      // Found a token, add it to the vector
      tokens.push_back(str.substr(lastPos, pos - lastPos).c_str());
      // Skip delimiters
      lastPos = str.find_first_not_of(delimiters, pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
    }
  }
}