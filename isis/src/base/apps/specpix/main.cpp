#include "Isis.h"

#include "IException.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"

using namespace std;
using namespace Isis;

enum SpecPix {
  NULLP,
  LRS,
  HRS,
  LIS,
  HIS,
  NONE
};

//  Define class for checking the input ranges to make sure
//  there is no overlap.  If there is any overlap, exit with
//  user error.
struct spRange {
  double min;
  double max;
  SpecPix specPix;
};


void specpix(Buffer &in, Buffer &out);
bool descending(const spRange &r1, const spRange &r2);
void addRange(QString minName, QString maxName, SpecPix pixel);






vector <spRange> rangeList;
int numRange;
//tjw:  int -> BigInt
BigInt nnull, nlis, nlrs, nhis, nhrs;




void IsisMain() {
  // We will be processing by line
  ProcessByLine p;


  

  
  nnull = nlis = nlrs = nhis = nhrs = 0;

  // Setup the input and output cubes
  p.SetInputCube("FROM");
  p.SetOutputCube("TO");

  // Read range values from user
  addRange("NULLMIN", "NULLMAX", NULLP);
  addRange("LRSMIN", "LRSMAX", LRS);
  addRange("HRSMIN", "HRSMAX", HRS);
  addRange("LISMIN", "LISMAX", LIS);
  addRange("HISMIN", "HISMAX", HIS);

  //  If more than one range was entered,
  //  make sure there is no overlap in ranges between differing special
  //  pixel values.  First sort on the min value in descending order.
  //  Then compare each min to the max in the next set of ranges.  If
  //  the min is less than the next max, there is overlap between those
  //  two sets of ranges.
  numRange = rangeList.size();
  if (numRange > 1) {
    vector <spRange> sortList(numRange);
    copy(rangeList.begin(), rangeList.end(), sortList.begin());
    sort(sortList.begin(), sortList.end(), descending);
    for (int i = 0; i < numRange - 1; i++) {
      if (sortList[i].min < sortList[i+1].max) {
        //  We have overlap
        string message = "Check the ranges entered for overlap between differing  ";
        message += "special pixels.  ";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }
    //  Copy sorted list back to original vector.
    copy(sortList.begin(), sortList.end(), rangeList.begin());
  }

  // Start the processing
  p.StartProcess(specpix);
  p.EndProcess();

  // Erase vector, otherwise if specpix is re-run it adds new
  // ranges to the end of the last run.
  rangeList.clear();

  //  Print out number of values changed
  PvlGroup results("Results");
  results.addComment("The number and type of pixels created");
  results += PvlKeyword("Null", Isis::toString(nnull));
  results += PvlKeyword("Lrs", Isis::toString(nlrs));
  results += PvlKeyword("Lis", Isis::toString(nlis));
  results += PvlKeyword("Hrs", Isis::toString(nhrs));
  results += PvlKeyword("His", Isis::toString(nhis));
  //tjw:  int total -> BigInt total
  BigInt total = nnull + nlrs + nhrs + nlis + nhis;
  results += PvlKeyword("Total", Isis::toString(total));

  Application::Log(results);

}


//  Line processing routine
void specpix(Buffer &in, Buffer &out) {

  for (int i = 0; i < in.size(); i++) {
    out[i] = in[i];
    for (int rng = 0; rng < numRange; rng++) {
      switch(rangeList[rng].specPix) {
        case NULLP:
          if (in[i] >= rangeList[rng].min && in[i] <= rangeList[rng].max) {
            out[i] = NULL8;
            nnull++;
   
          }
          break;

        case LRS:
          if (in[i] >= rangeList[rng].min && in[i] <= rangeList[rng].max) {
            out[i] = LOW_REPR_SAT8;
            nlrs++;
          }
          break;

        case HRS:
          if (in[i] >= rangeList[rng].min && in[i] <= rangeList[rng].max) {
            out[i] = HIGH_REPR_SAT8;
            nhrs++;
          }
          break;

        case LIS:
          if (in[i] >= rangeList[rng].min && in[i] <= rangeList[rng].max) {
            out[i] = LOW_INSTR_SAT8;
            nlis++;
          }
          break;

        case HIS:
          if (in[i] >= rangeList[rng].min && in[i] <= rangeList[rng].max) {
            out[i] = HIGH_INSTR_SAT8;
            nhis++;
          }
          break;

        case NONE:
          break;
      }
    }
  }
}


bool descending(const spRange &r1, const spRange &r2) {
  return r1.min > r2.min;
}


void addRange(QString minName, QString maxName, SpecPix pixel) {
  UserInterface &ui = Application::GetUserInterface();

  if (ui.WasEntered(minName) && ui.WasEntered(maxName)) {
    spRange temp;
    temp.min = ui.GetDouble(minName);
    temp.max = ui.GetDouble(maxName);
    temp.specPix = pixel;

    rangeList.push_back(temp);
  }
}

