#include "isis2ascii.h"

using namespace std;

namespace Isis {
  // Global declarations
  void isis2ascii(Buffer &in);
  ofstream fout;
  /**
   * @brief The SpecialPixelFunctor class
   *
   * This class is used to convert ISIS cubes into ascii format with
   * specified values for the special pixels in the cube.  The main purpose
   * for the class was to reduce global variables.
   *
   * @author 2016-06-15 Adam Paquette
   *
   * @internal
   *   @history 2016-06-15 Adam Paquette - Original Version
   *   @history 2020-06-10 Adam Paquette - Added delimiter variable to use in
   *                                       output file and removed static spacing
   *
   */

  class SpecialPixelFunctor{

  public:
      explicit SpecialPixelFunctor(QString null,
                                   QString hrs,
                                   QString his,
                                   QString lrs,
                                   QString lis,
                                   QString delimiter) :
          null(null), hrs(hrs), his(his), lrs(lrs), lis(lis), delimiter(delimiter) {}

      void operator() (Buffer &in) const {
          for(int i = 0; i < in.size(); i++) {
            if(IsSpecial(in[i])) {
              if(IsNullPixel(in[i])) fout << null.toStdString();
              if(IsHrsPixel(in[i])) fout << hrs.toStdString();
              if(IsHisPixel(in[i])) fout << his.toStdString();
              if(IsLrsPixel(in[i])) fout << lrs.toStdString();
              if(IsLisPixel(in[i])) fout << lis.toStdString();
            }
            else {
              fout << in[i];
            }
            if (i != in.size() - 1) {
              fout << delimiter.toStdString();
            }
          }
          fout << endl;
      }
  private:
      QString null;
      QString hrs;
      QString his;
      QString lrs;
      QString lis;
      QString delimiter;
  };

  void isis2ascii(UserInterface &ui) {
    Cube icube;
    icube.open(ui.GetCubeName("FROM"));

    isis2ascii(&icube, ui);
  }

  void isis2ascii(Cube *icube, UserInterface &ui) {
    // Create a process by line object
    QString null;
    QString hrs;
    QString his;
    QString lrs;
    QString lis;

    ProcessByLine p;

    // Get the size of the cube
    p.SetInputCube(icube);

    //  Open output text file
    QString to = ui.GetFileName("TO", "txt");
    fout.open(to.toLatin1().data());

    QString delimiter = ui.GetString("DELIMITER");
    if (delimiter.isEmpty()) {
      delimiter = " ";
    }

    // Print header if needed
    if(ui.GetBoolean("HEADER")) {
      fout << "Input_Cube" << delimiter.toStdString() << icube->fileName().toStdString() << endl;
      fout << "Samples" << delimiter.toStdString() << icube->sampleCount() << endl;
      fout << "Lines" << delimiter.toStdString() << icube->lineCount() << endl;
      fout << "Bands" << delimiter.toStdString() << icube->bandCount() << endl;
    }

    //Determine special pixel values
    if (ui.GetBoolean("SETPIXELVALUES")) {
      null = ui.GetString("NULLVALUE");
      hrs = ui.GetString("HRSVALUE");
      his = ui.GetString("HISVALUE");
      lrs = ui.GetString("LRSVALUE");
      lis = ui.GetString("LISVALUE");
    }
    else {
      null = "NULL";
      hrs = "HRS";
      his = "HIS";
      lrs = "LRS";
      lis = "LIS";
    }

    SpecialPixelFunctor isis2ascii(null, hrs, his, lrs, lis, delimiter);

    fout << std::setprecision(7);

    // List the cube
    p.ProcessCubeInPlace(isis2ascii, false);
    p.EndProcess();

    fout.close();
  }
}
