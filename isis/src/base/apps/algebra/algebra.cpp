/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "algebra.h"

#include "Cube.h"
#include "FileName.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"

namespace Isis {

  /*
   * Algebra
   *
   * This program performs simple algebra on either one or two
   * cubes. The two cubes may be added, subtracted, multiplied or divided.
   * 
   * The following equations are used:
   *     UNARY:      out = (A * from1) + C
   *     ADD:        out = ((from1 - D) * A) + ((from2 - E) * B) + C
   *     SUBTRACT:   out = ((from1 - D) * A) - ((from2 - E) * B) + C
   *     MULTIPLY:   out = ((from1 - D) * A) * ((from2 - E) * B) + C
   *     DIVIDE:     out = ((from1 - D) * A) / ((from2 - E) * B) + C   
   *
   * The FROM2 cube must have either one band or the same number of bands
   * as the FROM cube. If the FROM2 cube has one band, then the algebraic
   * formula will be applied to all bands in FROM using that single band
   * in FROM2. If FROM2 is a multi-band cube, the algebra will be performed
   * between corresponding bands from FROM and FROM2.
   * 
   * @param ui UserInterface object containing parameters
   */
   void algebra(UserInterface &ui) {
    Cube* icube1 = new Cube();
    icube1->open(ui.GetCubeName("FROM"));

    Cube* icube2 = nullptr;
     if(ui.WasEntered("FROM2")) {
       icube2 = new Cube();
       icube2->open(ui.GetCubeName("FROM2"));
     }

    algebra(icube1, ui, icube2);
  }


  /*
   * Algebra
   *
   * This program performs simple algebra on either one or two
   * cubes. The two cubes may be added, subtracted, multiplied or divided.
   * 
   * The following equations are used:
   *     UNARY:      out = (A * from1) + C
   *     ADD:        out = ((from1 - D) * A) + ((from2 - E) * B) + C
   *     SUBTRACT:   out = ((from1 - D) * A) - ((from2 - E) * B) + C
   *     MULTIPLY:   out = ((from1 - D) * A) * ((from2 - E) * B) + C
   *     DIVIDE:     out = ((from1 - D) * A) / ((from2 - E) * B) + C   
   *
   * The FROM2 cube must have either one band or the same number of bands
   * as the FROM cube. If the FROM2 cube has one band, then the algebraic
   * formula will be applied to all bands in FROM using that single band
   * in FROM2. If FROM2 is a multi-band cube, the algebra will be performed
   * between corresponding bands from FROM and FROM2.
   *
   * @param icube1 Cube* input cube1
   * @param ui UserInterface object containing parameters
   * @param icube2 Cube* input cube2; optional second input cube
   */
  void algebra(Cube* icube1, UserInterface &ui, Cube* icube2) {

    // Processing by line
    ProcessByLine p;

    // Set input cubes and attributes into ProcessByLine p
    CubeAttributeInput inatts1 = ui.GetInputAttribute("FROM");
    CubeAttributeInput inatts2;
    p.SetInputCube(icube1->fileName(), inatts1);
    if(icube2 != nullptr) {
      inatts2 = ui.GetInputAttribute("FROM2");
      p.SetInputCube(icube2->fileName(), inatts2);
    }

    // Set output cube and attributes into ProcessByLine p
    QString outCubeFname = ui.GetCubeName("TO");
    CubeAttributeOutput &outatts = ui.GetOutputAttribute("TO");
    p.SetOutputCube(outCubeFname, outatts);

    // Get the coefficients
    double Isisa = ui.GetDouble("A");
    double Isisb = ui.GetDouble("B");
    double Isisc = ui.GetDouble("C");
    double Isisd = ui.GetDouble("D");
    double Isise = ui.GetDouble("E");

    QString op = ui.GetString("OPERATOR");
    
    //*****************************************
    // Lambda functions to perform operations *
    //*****************************************

    // operatorProcess for add, subtract, multiply, divide
    auto operatorProcess = [&](std::vector<Buffer *> &in, std::vector<Buffer *> &out)->void {
      Buffer &inp1 = *in[0];
      Buffer &inp2 = *in[1];
      Buffer &outp = *out[0];

      // Loop for each pixel in the line
      // Special pixel propagation:
      //   1) special pixels in inp1 propagate to output cube unchanged
      //   2) if inp1 is not special and inp2 is, the output pixel is set to Null
      for(int i = 0; i < inp1.size(); i++) {
        if(IsSpecial(inp1[i])) {
          outp[i] = inp1[i];
        }
        else if(IsSpecial(inp2[i])) {
          outp[i] = NULL8;
        }
        else {
          double operand1 = (inp1[i] - Isisd) * Isisa;
          double operand2 = (inp2[i] - Isise) * Isisb;
          if(op == "ADD") {
            outp[i] = (operand1 + operand2) + Isisc;
          }
          if(op == "SUBTRACT") {
            outp[i] = (operand1 - operand2) + Isisc;
          }
          if(op == "MULTIPLY") {
            outp[i] = (operand1 * operand2) + Isisc;
          }
          if(op == "DIVIDE") {
            if((inp2[i] - Isise) * Isisb  == 0.0) {
              outp[i] = NULL8;
            }
            else {
              outp[i] = (operand1 / operand2) + Isisc;
            }
          }
        }
      }
    };

    // Unary process
    auto unaryProcess = [&](Buffer &in, Buffer &out)->void {
      // Loop for each pixel in the line.
      // Special pixels propagate directly to output
      for(int i = 0; i < in.size(); i++) {
        if(IsSpecial(in[i])) {
          out[i] = in[i];
        }
        else {
          out[i] = in[i] * Isisa + Isisc;
        }
      }
    };

    //*****************************************
    // End Lambda functions                   *
    //*****************************************

    // Start processing based on the operator
    if(op == "UNARY") {
      p.ProcessCube(unaryProcess);
    }
    else {
      p.ProcessCubes(operatorProcess); // add, subtract, multiply, divide
    }

    p.EndProcess();
  }
}

