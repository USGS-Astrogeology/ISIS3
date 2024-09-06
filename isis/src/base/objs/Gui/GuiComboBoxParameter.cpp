/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QVBoxLayout>
#include <QComboBox>
#include <QList>

#include "UserInterface.h"

#include "GuiParameter.h"
#include "GuiComboBoxParameter.h"

namespace Isis {

  GuiComboBoxParameter::GuiComboBoxParameter(QGridLayout *grid, UserInterface &ui,
                                     int group, int param) :
    GuiParameter(grid, ui, group, param) {

    // Reset the default alignment of the label
    p_label->setAlignment(Qt::AlignRight | Qt::AlignTop);

    // Create a vertical box layout for the combo box and add it to
    // the grid layout
    QVBoxLayout *lo = new QVBoxLayout;
    grid->addLayout(lo, param, 2);

    // Create a combo box 
    p_combo = new QComboBox();

    // Create a menu item in the combo box for each list item and add it
    // to the layout
    for(int item = 0; item < ui.ParamListSize(group, param); item++) {
      QString btext = ui.ParamListBrief(group, param, item);
      btext += " (";
      btext += ui.ParamListValue(group, param, item);
      btext += ")";
      p_combo->insertItem(item, btext);
    }
    lo->addWidget(p_combo);
    connect(p_combo, SIGNAL(activated(int)),
            this, SIGNAL(ValueChanged()));

    p_type = ComboWidget;
    RememberWidget(p_combo);
  }


  GuiComboBoxParameter::~GuiComboBoxParameter() {
    delete p_combo;
  }


  void GuiComboBoxParameter::Set(QString newValue) {
    IString value = newValue.toStdString();
    value.UpCase();

    int foundAtButton = -1;
    for(int i = 0; i < p_ui->ParamListSize(p_group, p_param); i++) {
      IString option = p_ui->ParamListValue(p_group, p_param, i).toStdString();
      option.UpCase();
      //if(option.compare(0, value.size(), value) == 0) foundAtButton = i;
      if(option == value) foundAtButton = i;
    }

    if(foundAtButton != -1) {
      p_combo->setCurrentIndex(foundAtButton);
    }

    emit ValueChanged();
  }


  QString GuiComboBoxParameter::Value() {
    return p_ui->ParamListValue(p_group, p_param,
                                p_combo->currentIndex());
  }

  std::vector<QString> GuiComboBoxParameter::Exclusions() {
    std::vector<QString> list;

    int index = p_combo->currentIndex();

    for(int i = 0; i < p_ui->ParamListExcludeSize(p_group, p_param, index); i++) {
      QString s = p_ui->ParamListExclude(p_group, p_param, index, i);
      list.push_back(s);
    }

    return list;
  }

  void GuiComboBoxParameter::setOption (int option) {
    std::cout << "Combo box option: " << option << std::endl;
  }
}
