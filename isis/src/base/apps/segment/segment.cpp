#include "Application.h"
#include "Cube.h"
#include "ProgramLauncher.h"

#include "segment.h"

using namespace std;

namespace Isis {

  void segment(UserInterface &ui) {
    Cube *incube = new Cube();
    incube->open(ui.GetCubeName("FROM"));
    segment(incube, ui);
  }

  void segment(Cube *cube, UserInterface &ui) {

    //Get user parameters
    FileName inFile = ui.GetCubeName("FROM").toStdString();
    int numberOfLines = ui.GetInteger("NL");
    int lineOverlap   = ui.GetInteger("OVERLAP");

    //Throws exception if user is dumb
    if(lineOverlap >= numberOfLines) {
      throw IException(IException::User, "The Line Overlap (OVERLAP) must be less than the Number of Lines (LN).", _FILEINFO_);
    }

    //Loops through, cropping as desired
    int cropNum = 1;
    int startLine = 1;
    bool hasReachedEndOfCube = false;
    while(startLine <= cube->lineCount()  &&  not hasReachedEndOfCube) {
      //! Sets up the proper paramaters for running the crop program
      QString parameters = "FROM=" + QString::fromStdString(inFile.expanded()) +
                           " TO=" + QString::fromStdString(inFile.path()) + "/" + QString::fromStdString(inFile.baseName()) + ".segment" + toString(cropNum) + ".cub"
                           + " LINE=" + toString(startLine) + " NLINES=";

      if(startLine + numberOfLines > cube->lineCount()) {
        parameters += toString(cube->lineCount() - (startLine - 1));
        hasReachedEndOfCube = true;
      }
      else {
        parameters += toString(numberOfLines);
      }

      if(ui.GetParamPreference() != "") { 
        parameters += " -PREF=" + ui.GetParamPreference();
      }

      ProgramLauncher::RunIsisProgram("crop", parameters);
      //The starting line for next crop
      startLine = 1 + cropNum * (numberOfLines - lineOverlap);
      cropNum++;
    }
  }
}
