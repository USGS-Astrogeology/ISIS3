/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "GuiListParameter.h"
#include "GuiComboBoxParameter.h"
#include "GuiCubeParameter.h"
#include "GuiFilenameParameter.h"
#include "GuiStringParameter.h"
#include "GuiIntegerParameter.h"
#include "GuiDoubleParameter.h"
#include "GuiBooleanParameter.h"
#include "GuiParameterFactory.h"
#include "UserInterface.h"
#include "Application.h"

namespace Isis {
  GuiParameter *GuiParameterFactory::Create(QGridLayout *grid,
      Isis::UserInterface &ui,
      int group, int param) {
    QString paramType = ui.ParamType(group, param);

    GuiParameter *p = NULL;
    if(ui.ParamListSize(group, param) > 0) {
      if (paramType == "combo") {
        p = new GuiComboBoxParameter(grid, ui, group, param);
      } else {
        p = new GuiListParameter(grid, ui, group, param);
      }
    }
    else if(paramType == "cube") {
      p = new GuiCubeParameter(grid, ui, group, param);
    }
    else if(paramType == "filename") {
      p = new GuiFileNameParameter(grid, ui, group, param);
    }
    else if(paramType == "string") {
      p = new GuiStringParameter(grid, ui, group, param);
    }
    else if(paramType == "integer") {
      p = new GuiIntegerParameter(grid, ui, group, param);
    }
    else if(paramType == "double") {
      p = new GuiDoubleParameter(grid, ui, group, param);
    }
    else if(paramType == "boolean") {
      p = new GuiBooleanParameter(grid, ui, group, param);
    }
    else {
      std::string msg = "Invalid parameter type in XML [" + paramType.toStdString() + "]";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return p;
  }
}



