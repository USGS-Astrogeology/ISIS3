#include "Isis.h"

#include <string>
#include <cstdlib>

#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "Pvl.h"
#include "UserInterface.h"
#include "Brick.h"
#include "Statistics.h"

using namespace std;
using namespace Isis;

void cpy(Buffer &in, Buffer &out);
void remrx(Buffer &in, Buffer &out);
static int sdim, ldim;
static bool resvalid;
static QString action;

void IsisMain() {
  // We will be processing by line
  ProcessByLine p;

  // Setup the input and output cubes
  Cube *icube = p.SetInputCube("FROM");
  PvlKeyword &status = icube->group("RESEAUS")["STATUS"];
  UserInterface &ui = Application::GetUserInterface();
  QString in = ui.GetCubeName("FROM");

  // Check reseau status and make sure it is not nominal or removed
  if(((std::string)status == "Nominal")) {
    std::string msg = "Input file [" + in +
                 "] appears to have nominal reseau status. You must run findrx first.";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  if((std::string)status == "Removed") {
    std::string msg = "Input file [" + in +
                 "] appears to already have reseaus removed.";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  status = "Removed";

  p.SetOutputCube("TO");

  // Start the processing
  p.StartProcess(cpy);
  p.EndProcess();

  // Get the user entered dimensions
  sdim = ui.GetInteger("SDIM");
  ldim = ui.GetInteger("LDIM");

  // Get other user entered options
  QString out = ui.GetCubeName("TO");
  resvalid = ui.GetBoolean("RESVALID");
  action = ui.GetString("ACTION");

  // Open the output cube
  Cube cube;
  cube.open(out, "rw");

  PvlGroup &res = cube.label()->findGroup("RESEAUS", Pvl::Traverse);

  // Get reseau line, sample, type, and valid Keywords
  PvlKeyword lines = res.findKeyword("LINE");
  PvlKeyword samps = res.findKeyword("SAMPLE");
  PvlKeyword type = res.findKeyword("TYPE");
  PvlKeyword valid = res.findKeyword("VALID");
  int numres = lines.size();

  Brick brick(sdim, ldim, 1, cube.pixelType());
  for(int res = 0; res < numres; res++) {
    if((resvalid == 0 || std::stoi(valid[res]) == 1) && std::stoi(type[res]) != 0) {
      int baseSamp = (int)(std::stod(samps[res]) + 0.5) - (sdim / 2);
      int baseLine = (int)(std::stod(lines[res]) + 0.5) - (ldim / 2);
      brick.SetBasePosition(baseSamp, baseLine, 1);
      cube.read(brick);
      if(action == "NULL") {
        for(int i = 0; i < brick.size(); i++) brick[i] = Isis::Null;
      }
      else if(action == "BILINEAR") {
        Statistics stats;
        double array[sdim][ldim];
        for(int s = 0; s < sdim; s++) {
          for(int l = 0; l < ldim; l++) {
            int index = l * sdim + s;
            array[s][l] = brick[index];
            // Add perimeter data to stats object for calculations
            if(s == 0 || l == 0 || s == (sdim - 1) || l == (ldim - 1)) {
              stats.AddData(&array[s][l], 1);
            }
          }
        }
        // Get the average and standard deviation of the perimeter of the brick
        double avg = stats.Average();
        double sdev = stats.StandardDeviation();

        // Top Edge Reseau
        if(std::stoi(type[res]) == 2) {
          int l1 = 0;
          int l2 = ldim - 1;
          for(int s = 0; s < sdim; s++) {
            array[s][l1] = array[s][l2];
          }
        }
        // Left Edge Reseau
        else if(std::stoi(type[res]) == 4) {
          int s1 = 0;
          int s2 = sdim - 1;
          for(int l = 0; l < ldim; l++) {
            array[s1][l] = array[s2][l];
          }
        }
        // Right Edge Reseau
        else if(std::stoi(type[res]) == 6) {
          int s1 = 0;
          int s2 = sdim - 1;
          for(int l = 0; l < ldim; l++) {
            array[s2][l] = array[s1][l];
          }
        }
        // Bottom Edge Reseau
        else if(std::stoi(type[res]) == 8) {
          int l1 = 0;
          int l2 = ldim - 1;
          for(int s = 0; s < sdim; s++) {
            array[s][l2] = array[s][l1];
          }
        }
        // Walk top edge & replace data outside of 2devs with the avg
        for(int s = 0; s < sdim; s++) {
          int l = 0;
          double diff = fabs(array[s][l] - avg);
          if(diff > (2 * sdev)) array[s][l] = avg;
        }
        // Walk bottom edge & replace data outside of 2devs with the avg
        for(int s = 0; s < sdim; s++) {
          int l = ldim - 1;
          double diff = fabs(array[s][l] - avg);
          if(diff > (2 * sdev)) array[s][l] = avg;
        }
        // Walk left edge & replace data outside of 2devs with the avg
        for(int l = 0; l < ldim; l++) {
          int s = 0;
          double diff = fabs(array[s][l] - avg);
          if(diff > (2 * sdev)) array[s][l] = avg;
        }
        // Walk right edge & replace data outside of 2devs with the avg
        for(int l = 0; l < ldim; l++) {
          int s = sdim - 1;
          double diff = fabs(array[s][l] - avg);
          if(diff > (2 * sdev)) array[s][l] = avg;
        }
        srand(0);
        double dn, gdn1, gdn2;
        for(int l = 0; l < ldim; l++) {
          int c = l * sdim;  //count
          // Top Edge Reseau
          if(std::stoi(type[res]) == 2 && l < (ldim / 2)) continue;
          // Bottom Edge Reseau
          if(std::stoi(type[res]) == 8 && l > (ldim / 2 + 1)) continue;
          for(int s = 0; s < sdim; s++, c++) {
            // Left Edge Reseau
            if(std::stoi(type[res]) == 4 && s < (sdim / 2)) continue;
            // Right Edge Reseau
            if(std::stoi(type[res]) == 6 && s > (sdim / 2 + 1)) continue;
            double sum = 0.0;
            int gline1 = 0;
            int gline2 = ldim - 1;
            gdn1 = array[s][gline1];
            gdn2 = array[s][gline2];

            // Linear Interpolation to get pixel value
            dn = gdn2 + (l - gline2) * (gdn1 - gdn2) / (gline1 - gline2);
            sum += dn;

            int gsamp1 = 0;
            int gsamp2 = sdim - 1;
            gdn1 = array[gsamp1][l];
            gdn2 = array[gsamp2][l];

            // Linear Interpolation to get pixel value
            dn = gdn2 + (s - gsamp2) * (gdn1 - gdn2) / (gsamp1 - gsamp2);
            sum += dn;
            dn = sum / 2;
            int rdm = rand();
            double drandom = rdm / (double)RAND_MAX;
            double offset = 0.0;
            if(drandom < .333) offset = -1.0;
            if(drandom > .666) offset = 1.0;
            brick[c] = dn + offset;
          }
        }
      }
    }
    cube.write(brick);
  }
  cube.close();

}

// Copy the input cube to the output cube
void cpy(Buffer &in, Buffer &out) {
  for(int i = 0; i < in.size(); i++) {
    out[i] = in[i];
  }
}
