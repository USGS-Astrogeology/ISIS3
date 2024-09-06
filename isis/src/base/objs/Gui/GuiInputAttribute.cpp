/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <string>

#include <QDebug>
#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include "CubeAttribute.h"
#include "FileName.h"
#include "GuiInputAttribute.h"

namespace Isis {
  //! Convenience access to dialog
  int GuiInputAttribute::GetAttributes(const QString &defaultAttribute,
                                       QString &newAttribute,
                                       const QString &title,
                                       QWidget *parent) {
    // Construct dialog if necessary
    static GuiInputAttribute *p_dialog = 0;
    if(p_dialog == 0) {
      p_dialog = new GuiInputAttribute(parent);
    }
    p_dialog->setWindowTitle(title);

    // Load default attributes and then get the new ones
    p_dialog->SetAttributes(defaultAttribute);
    if(p_dialog->exec() == QDialog::Accepted) {
      newAttribute = p_dialog->GetAttributes();
      return 1;
    }
    newAttribute = defaultAttribute;
    return 0;
  }

  //! Constuctor
  GuiInputAttribute::GuiInputAttribute(QWidget *parent) : QDialog(parent) {
    // Create the two radio buttons
    QRadioButton *allBands = new QRadioButton("&All Bands");
    allBands->setToolTip("Select all bands from the input cube");

    QRadioButton *listBands = new QRadioButton("&Band List");
    listBands->setToolTip("Select any bands from the input cube");

    // Create the button group for the radio buttons
    p_buttonGroup = new QButtonGroup();
    p_buttonGroup->addButton(allBands);
    p_buttonGroup->addButton(listBands);
    p_buttonGroup->setExclusive(true);

    // Create the text field for list toggle button
    p_lineEdit = new QLineEdit();
    connect(allBands, SIGNAL(toggled(bool)), p_lineEdit, SLOT(setDisabled(bool)));
    allBands->setChecked(true);

    // Put the buttons and text field in a gridlayout
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(allBands, 0, 0);
    gridLayout->addWidget(listBands, 1, 0);
    gridLayout->addWidget(p_lineEdit, 1, 1);

    // Create the action buttons
    QPushButton *okButton = new QPushButton("Ok");
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

#if 0
    // Add them to a button group
    QButtonGroup *actionGroup = new QButtonGroup();
    actionGroup->addButton(okButton);
    actionGroup->addButton(cancelButton);
#endif

    // Put the buttons in a horizontal orientation
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(okButton);
    actionLayout->addWidget(cancelButton);

    // Put the grid layout and action layout on the dialog
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addLayout(gridLayout);
    dialogLayout->addLayout(actionLayout);
  }


  // Destructor
  GuiInputAttribute::~GuiInputAttribute() {}


  // Return the attributes in the dialog
  QString GuiInputAttribute::GetAttributes() {
    QString attStr("");
    if(p_lineEdit->isEnabled()) {
      attStr = p_lineEdit->text().simplified();
      attStr = attStr.remove(QRegExp("^[+]*"));
      if (attStr.length() > 0) {
        if (attStr.left(1) != "+") attStr.prepend('+');
      }
    }
    return attStr;
  }


  // Set the attributes in the dialog
  void GuiInputAttribute::SetAttributes(const QString &value) {
    Isis::CubeAttributeInput att(value.toStdString());
    std::vector<QString> bands = att.bands();
    if(bands.size() == 0) {
      p_buttonGroup->buttons()[0]->setChecked(true);
      p_lineEdit->setText("");
    }
    else {
      p_buttonGroup->buttons()[1]->setChecked(true);
      p_lineEdit->setText(att.toString());
    }
  }
}

