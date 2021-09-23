#ifndef noproj_h
#define noproj_h

#include "Cube.h"
#include "UserInterface.h"

namespace Isis{
  extern void noproj(Cube *icube, Cube *mcube, UserInterface &ui);
  extern void noproj(UserInterface &ui);
}

#endif