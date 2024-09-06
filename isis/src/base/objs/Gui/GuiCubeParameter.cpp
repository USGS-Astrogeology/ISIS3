/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <sstream>

#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMenu>
#include <QString>
#include <QTextEdit>
#include <QToolButton>


#include "Application.h"
#include "Cube.h"
#include "FileName.h"
#include "GuiCubeParameter.h"
#include "GuiInputAttribute.h"
#include "GuiOutputAttribute.h"
#include "IException.h"
#include "ProgramLauncher.h"
#include "Pvl.h"
#include "UserInterface.h"


namespace Isis {

  /**
   * @brief Constructs GuiCubeParameter object
   *
   * @param grid Pointer to QGridLayout
   * @param ui User interface object
   * @param group Index of group
   * @param param Index of parameter
   */
  GuiCubeParameter::GuiCubeParameter(QGridLayout *grid, UserInterface &ui,
                                     int group, int param) :
    GuiFileNameParameter(grid, ui, group, param) {
    p_menu = new QMenu();

    QAction *fileAction = new QAction(this);
    fileAction->setText("Select Cube");
    connect(fileAction, SIGNAL(triggered(bool)), this, SLOT(SelectFile()));
    p_menu->addAction(fileAction);

    QAction *attAction = new QAction(this);
    attAction->setText("Change Attributes ...");
    connect(attAction, SIGNAL(triggered(bool)), this, SLOT(SelectAttribute()));
    p_menu->addAction(attAction);

    QAction *viewAction = new QAction(this);
    viewAction->setText("View cube");
    connect(viewAction, SIGNAL(triggered(bool)), this, SLOT(ViewCube()));
    p_menu->addAction(viewAction);

    QAction *labAction = new QAction(this);
    labAction->setText("View labels");
    connect(labAction, SIGNAL(triggered(bool)), this, SLOT(ViewLabel()));
    p_menu->addAction(labAction);

    p_fileButton->setMenu(p_menu);
    p_fileButton->setPopupMode(QToolButton::MenuButtonPopup);
    QString optButtonWhatsThisText = "<p><b>Function:</b> \
            Opens a file chooser window to select a file from</p> <p>\
            <b>Hint: </b> Click the arrow for more cube parameter options</p>";
    p_fileButton->setWhatsThis(optButtonWhatsThisText);

    p_type = CubeWidget;
  }


  /**
   *  Destructor of GuiCubeParameter object.
   */
  GuiCubeParameter::~GuiCubeParameter() {
    delete p_menu;
  }


  /**
   * Select cube attributes.
   */
  void GuiCubeParameter::SelectAttribute() {
    if(p_ui->ParamFileMode(p_group, p_param) == "input") {
      Isis::CubeAttributeInput att(FileName(p_lineEdit->text().toStdString()));
      QString curAtt = att.toString();
      QString newAtt;
      int status = GuiInputAttribute::GetAttributes(curAtt, newAtt,
                   p_ui->ParamName(p_group, p_param),
                   p_fileButton);
      if((status == 1) && (curAtt != newAtt)) {
        Isis::FileName f(p_lineEdit->text().toStdString());
        p_lineEdit->setText(QString::fromStdString(f.expanded()) + newAtt);
      }
    }
    else {
      Isis::CubeAttributeOutput att("+" + p_ui->PixelType(p_group, p_param).toStdString());
      bool allowProp = att.propagatePixelType();
      att.addAttributes(FileName(p_lineEdit->text().toStdString()));

      QString curAtt = att.toString();
      QString newAtt;
      int status = GuiOutputAttribute::GetAttributes(curAtt, newAtt,
                   p_ui->ParamName(p_group, p_param),
                   allowProp,
                   p_fileButton);
      if((status == 1) && (curAtt != newAtt)) {
        Isis::FileName f(p_lineEdit->text().toStdString());
        p_lineEdit->setText(QString::fromStdString(f.expanded()) + newAtt);
      }
    }

    return;
  }

  /**
   * Opens cube in qview.
   * @throws Isis::Exception::User "You must enter a cube name to open"
   */
  void GuiCubeParameter::ViewCube() {
    try {
      // Make sure the user entered a value
      if(IsModified()) {
        QString cubeName = Value();

        // Check to make sure the cube can be opened
        Isis::Cube temp;
        temp.open(cubeName);
        temp.close();

        // Open the cube in Qview
        QString command = "$ISISROOT/bin/qview " + cubeName + " &";
        ProgramLauncher::RunSystemCommand(command);
      }
      // Throw an error if no cube name was entered
      else {
        std::string msg = "You must enter a cube name to open";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    catch(IException &e) {
      Isis::iApp->GuiReportError(e);
    }
  }

  /**
   * Displays cube label in the GUI log.
   * @throws Isis::Exception::User "You must enter a cube name to open"
   */
  void GuiCubeParameter::ViewLabel() {
    try {
      // Make sure the user entered a value
      if(IsModified()) {
        QString cubeName = Value();

        // Check to make sure the cube can be opened
        Isis::Cube temp;
        temp.open(cubeName);

        // Get the label and write it out to the log
        Isis::Pvl *label = temp.label();
        Isis::Application::GuiLog(*label);

        // Close the cube
        temp.close();

      }
      else {
        std::string msg = "You must enter a cube name to open";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    catch(IException &e) {
      Isis::iApp->GuiReportError(e);
    }

  }
}

