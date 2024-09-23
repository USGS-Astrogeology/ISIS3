#include "Isis.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Histogram.h"
#include "UserInterface.h"
#include "Pvl.h"
#include "Cube.h"
#include "Process.h"

using namespace std;
using namespace Isis;

void populate(Buffer &in, Buffer &out);

void IsisMain(){
  UserInterface &ui = Application::GetUserInterface();


  //We will process by line
  ProcessByLine p;
  Cube* cubeptr = p.SetInputCube("FROM");


  // Histogram* histptr = (cubeptr -> Histogram());
  double max = ui.GetDouble("MAXVAL");
  double min = ui.GetDouble("MINVAL");

  /*
  A histogram is made from the input cube, as the default min of the
  bit2bit output is at .5% of the data range, and the default max is at 99.5%
  */
  Histogram* histptr = cubeptr -> histogram();

  double maxper = histptr -> Percent(ui.GetDouble("MAXPER"));
  double minper = histptr -> Percent(ui.GetDouble("MINPER"));
  double validMin = Isis::ValidMinimum;
  double validMax = Isis::ValidMaximum;



  // Set properties MIN,MAX, and PixelType for output cube
  CubeAttributeOutput outputProperties;
  if(ui.GetString("CLIP") == "PERCENT"){
    outputProperties.setMaximum(maxper);
    outputProperties.setMinimum(minper);
    validMax = maxper;
    validMin = minper;
  }
  else{
    outputProperties.setMaximum(max);
    outputProperties.setMinimum(min);
    validMax = max;
    validMin = min;
  }
  if(ui.GetString("BITTYPE")=="8BIT"){
    outputProperties.setPixelType(UnsignedByte);
  }
  else if(ui.GetString("BITTYPE")=="16BIT"){
    outputProperties.setPixelType(SignedWord);
  }
  else {
    outputProperties.setPixelType(Real);
  }


  if(ui.GetBoolean("STATS")) { //! Run extended statistics
    // The TO parameter is a filename not a cubename, so we can ignore the command line attributes
    Cube* ocubeptr = p.SetOutputCube (ui.GetCubeName("TO"),outputProperties,
                     cubeptr->sampleCount(),cubeptr->lineCount(),
                     cubeptr->bandCount());

    p.StartProcess(populate);

    Histogram* ohistptr =  (ocubeptr -> histogram(1,validMin,validMax));
    int iLrs = histptr -> LrsPixels();
    int iHrs = histptr -> HrsPixels();
    int iNull = histptr -> NullPixels();
    int oLrs = ohistptr -> LrsPixels();
    int oHrs = ohistptr -> HrsPixels();
    int oNull = ohistptr -> NullPixels();
    double invalid_pi = (( (histptr -> TotalPixels()) - (histptr -> ValidPixels()))*100.0) / ((histptr -> TotalPixels())*1.0);
    double invalid_po = (( (ohistptr -> TotalPixels()) - (ohistptr -> ValidPixels()))*100.0) / ((ohistptr -> TotalPixels())*1.0);

    p.EndProcess();

    //!Write bit2bit summary to the screen
    cout << "\n\nIN:\n";
    cout << "              LRS:\t\t" << iLrs << endl;
    cout << "              HRS:\t\t" << iHrs << endl;
    cout << "             NULL:\t\t" << iNull << endl;
    cout << "  Invalid Pixel %:\t\t" << invalid_pi << endl;
    cout << "\nOUT:\n\n";
    cout << "       Data Range:\t\t";
    cout << validMin << " < x < " << validMax << endl;
    cout << "              LRS:\t\t" << oLrs << endl;
    cout << "              HRS:\t\t" << oHrs << endl;
    cout << "             NULL:\t\t" << oNull << endl;
    cout << "  Invalid Pixel %:\t\t" << invalid_po << endl<< endl;

    //!Write bit2bit summary to print.prt logfile
    PvlGroup results("bit2bit_Results");
    results += PvlKeyword ("INPUT_LRS",toString(iLrs));
    results += PvlKeyword ("INPUT_HRS",toString(iHrs));
    results += PvlKeyword ("INPUT_NULL",toString(iNull));
    results += PvlKeyword ("INPUT_INVALID_PERCENT",toString(invalid_pi));
    results += PvlKeyword ("OUTPUT_MIN",toString(validMin));
    results += PvlKeyword ("OUTPUT_MAX",toString(validMax));
    results += PvlKeyword ("OUTPUT_LRS",toString(oLrs));
    results += PvlKeyword ("OUTPUT_HRS",toString(oHrs));
    results += PvlKeyword ("OUTPUT_NULL",toString(oNull));
    results += PvlKeyword ("OUTPUT_INVALID_PERCENT",toString(invalid_po));
    Application::Log(results);

    delete histptr;
    delete ohistptr;
  }
  else{ //! run minimal statistics (runs faster)
    int iLrs = histptr -> LrsPixels();
    int iHrs = histptr -> HrsPixels();
    int iNull = histptr -> NullPixels();
    double invalid_pi = (( (histptr -> TotalPixels()) - (histptr -> ValidPixels()))*100.0) / ((histptr -> TotalPixels())*1.0);

    p.EndProcess();

    //!Write bit2bit summary to the screen
    cout << "\n\nIN:\n";
    cout << "              LRS:\t\t" << iLrs << endl;
    cout << "              HRS:\t\t" << iHrs << endl;
    cout << "             NULL:\t\t" << iNull << endl;
    cout << "  Invalid Pixel %:\t\t" << invalid_pi << endl;
    cout << "\nOUT:\n\n";
    cout << "       Data Range:\t\t";
    cout << validMin << " < x < " << validMax << endl;


    //!Write bit2bit summary to print.prt logfile
    PvlGroup results("bit2bit_Results");
    results += PvlKeyword ("INPUT_LRS",toString(iLrs));
    results += PvlKeyword ("INPUT_HRS",toString(iHrs));
    results += PvlKeyword ("INPUT_NULL",toString(iNull));
    results += PvlKeyword ("INPUT_INVALID_PERCENT",toString(invalid_pi));
    results += PvlKeyword ("OUTPUT_MIN",toString(validMin));
    results += PvlKeyword ("OUTPUT_MAX",toString(validMax));
    Application::Log(results);

    delete histptr;
  }
}

// Line processing routine
void populate(Buffer &in, Buffer &out){
  for (int i=0; i<in.size(); i++) {
    out[i]=in[i];
  }
}
