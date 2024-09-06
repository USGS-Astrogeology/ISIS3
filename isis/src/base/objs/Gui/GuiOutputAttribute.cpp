/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <iostream>
#include <string>

#include <QRadioButton>
#include <QPushButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QDoubleValidator>

#include "CubeAttribute.h"
#include "FileName.h"
#include "GuiOutputAttribute.h"

namespace Isis {
  //! Convenience access to dialog
  int GuiOutputAttribute::GetAttributes(const QString &defaultAttribute,
                                        QString &newAttribute,
                                        const QString &title,
                                        bool allowProp,
                                        QWidget *parent) {
    // Construct dialog if necessary
    static GuiOutputAttribute *p_dialog = 0;
    if(p_dialog == 0) {
      p_dialog = new GuiOutputAttribute(parent);
    }
    p_dialog->setWindowTitle(title);
    p_dialog->SetPropagation(allowProp);

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
  GuiOutputAttribute::GuiOutputAttribute(QWidget *parent) : QDialog(parent) {
    // Create the pixel type group
    p_propagate = new QRadioButton("&Propagate");
    p_propagate->setToolTip("Propagate pixel type from input cube");
    p_unsignedByte = new QRadioButton("&Unsigned Byte");
    p_unsignedByte->setToolTip("Unsigned 8-bit pixels");
    p_signedWord = new QRadioButton("&Signed Word");
    p_signedWord->setToolTip("Signed 16-bit pixels");
    p_unsignedWord = new QRadioButton("Unsigned Word");
    p_unsignedWord->setToolTip("Unsigned 16-bit pixels");
    p_signedInteger = new QRadioButton("&Signed Integer");
    p_signedInteger->setToolTip("Signed 32-bit integer");
    p_unsignedInteger = new QRadioButton("Unsigned Integer");
    p_unsignedInteger->setToolTip("Unsigned 32-bit integer");
    p_real = new QRadioButton("&Real");
    p_real->setToolTip("Floating point 32-bit pixels");

    QButtonGroup *buttonGroup = new QButtonGroup();
    buttonGroup->addButton(p_propagate);
    buttonGroup->addButton(p_unsignedByte);
    buttonGroup->addButton(p_signedWord);
    buttonGroup->addButton(p_unsignedWord);
    buttonGroup->addButton(p_unsignedInteger);
    buttonGroup->addButton(p_signedInteger);
    buttonGroup->addButton(p_real);
    buttonGroup->setExclusive(true);

    p_minEdit = new QLineEdit();
    p_maxEdit = new QLineEdit();
    QLabel *minLabel = new QLabel("Minimum");
    QLabel *maxLabel = new QLabel("Maximum");
    connect(p_propagate, SIGNAL(toggled(bool)), p_minEdit, SLOT(setDisabled(bool)));
    connect(p_propagate, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setDisabled(bool)));
    connect(p_unsignedByte, SIGNAL(toggled(bool)), p_minEdit, SLOT(setEnabled(bool)));
    connect(p_unsignedByte, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setEnabled(bool)));
    connect(p_signedWord, SIGNAL(toggled(bool)), p_minEdit, SLOT(setEnabled(bool)));
    connect(p_signedWord, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setEnabled(bool)));
    connect(p_unsignedWord, SIGNAL(toggled(bool)), p_minEdit, SLOT(setEnabled(bool)));
    connect(p_unsignedWord, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setEnabled(bool)));
    connect(p_unsignedInteger, SIGNAL(toggled(bool)), p_minEdit, SLOT(setEnabled(bool)));
    connect(p_unsignedInteger, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setEnabled(bool)));
    connect(p_signedInteger, SIGNAL(toggled(bool)), p_minEdit, SLOT(setEnabled(bool)));
    connect(p_signedInteger, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setEnabled(bool)));
    connect(p_real, SIGNAL(toggled(bool)), p_minEdit, SLOT(setDisabled(bool)));
    connect(p_real, SIGNAL(toggled(bool)), p_maxEdit, SLOT(setDisabled(bool)));
    p_minEdit->setValidator(new QDoubleValidator(p_minEdit));
    p_maxEdit->setValidator(new QDoubleValidator(p_maxEdit));

    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(p_propagate, 0, 0);
    gridLayout->addWidget(p_unsignedByte, 1, 0);
    gridLayout->addWidget(p_signedWord, 2, 0);
    gridLayout->addWidget(p_unsignedWord, 3, 0);
    gridLayout->addWidget(p_signedInteger, 4, 0);
    gridLayout->addWidget(p_unsignedInteger, 5, 0);
    gridLayout->addWidget(p_real, 6, 0);
    gridLayout->addWidget(minLabel, 0, 1);
    gridLayout->addWidget(p_minEdit, 1, 1);
    gridLayout->addWidget(maxLabel, 2, 1);
    gridLayout->addWidget(p_maxEdit, 3, 1);

    QGroupBox *pixelTypeBox = new QGroupBox("Pixel Type");
    pixelTypeBox->setLayout(gridLayout);

    // Create detached/attached stuff
    p_attached = new QRadioButton("&Attached");
    p_attached->setToolTip("Save labels and image data in one file");
    p_detached = new QRadioButton("&Detached");
    p_detached->setToolTip("Save labels and image data in separate files");
    p_attached->setChecked(true);

    buttonGroup = new QButtonGroup();
    buttonGroup->addButton(p_attached);
    buttonGroup->addButton(p_detached);
    buttonGroup->setExclusive(true);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(p_attached);
    layout->addWidget(p_detached);

    QGroupBox *labelFormatBox = new QGroupBox("Label Format");
    labelFormatBox->setLayout(layout);

    // Create cube format stuff
    p_tiled = new QRadioButton("&Tiled");
    p_tiled->setToolTip("Save image data in tiled format");
    p_bsq = new QRadioButton("&BSQ");
    p_bsq->setToolTip("Save image data in band sequential format");

    buttonGroup = new QButtonGroup();
    buttonGroup->addButton(p_tiled);
    buttonGroup->addButton(p_bsq);
    buttonGroup->setExclusive(true);

    layout = new QVBoxLayout();
    layout->addWidget(p_tiled);
    layout->addWidget(p_bsq);

    QGroupBox *cubeFormatBox = new QGroupBox("Cube Format");
    cubeFormatBox->setLayout(layout);

    // Create cube format stuff
    p_lsb = new QRadioButton("&LSB");
    p_lsb->setToolTip("Save image data in little endian format");
    p_msb = new QRadioButton("&MSB");
    p_msb->setToolTip("Save image data in big endian format");

    buttonGroup = new QButtonGroup();
    buttonGroup->addButton(p_lsb);
    buttonGroup->addButton(p_msb);
    buttonGroup->setExclusive(true);

    layout = new QVBoxLayout();
    layout->addWidget(p_lsb);
    layout->addWidget(p_msb);

    QGroupBox *byteOrderBox = new QGroupBox("Byte Order");
    byteOrderBox->setLayout(layout);

    // Create the action buttons
    QPushButton *okButton = new QPushButton("Ok");
    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));

    QPushButton *cancelButton = new QPushButton("Cancel");
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));

    // Put the buttons in a horizontal orientation
    QHBoxLayout *actionLayout = new QHBoxLayout();
    actionLayout->addWidget(okButton);
    actionLayout->addWidget(cancelButton);

    // Put the grid layout and action layout on the dialog
    QVBoxLayout *dialogLayout = new QVBoxLayout(this);
    dialogLayout->addWidget(pixelTypeBox);
    dialogLayout->addWidget(labelFormatBox);
    dialogLayout->addWidget(cubeFormatBox);
    dialogLayout->addWidget(byteOrderBox);
    dialogLayout->addLayout(actionLayout);
  }


  // Destructor
  GuiOutputAttribute::~GuiOutputAttribute() {}


  // Return the attributes in the dialog
  QString GuiOutputAttribute::GetAttributes() {
    QString att;
    if(p_lsb->isChecked()) att += "+Lsb";
    if(p_msb->isChecked()) att += "+Msb";

    if(p_tiled->isChecked()) att += "+Tile";
    if(p_bsq->isChecked()) att += "+BandSequential";

    if(p_attached->isChecked()) att += "+Attached";
    if(p_detached->isChecked()) att += "+Detached";

    if(p_real->isChecked()) att += "+Real";
    if(p_unsignedByte->isChecked()) att += "+UnsignedByte";
    if(p_signedWord->isChecked()) att += "+SignedWord";
    if(p_unsignedWord->isChecked()) att += "+UnsignedWord";
    if(p_signedInteger->isChecked()) att += "+SignedInteger";
    if(p_unsignedInteger->isChecked()) att += "+UnsignedInteger";

    if(p_unsignedByte->isChecked() || p_signedWord->isChecked() || p_unsignedWord->isChecked()
        || p_signedInteger->isChecked() || p_unsignedInteger->isChecked()) {
      if((p_minEdit->text() != "") && (p_maxEdit->text() != "")) {
        att += "+";
        att += p_minEdit->text();
        att += ":";
        att += p_maxEdit->text();
      }
    }

    Isis::CubeAttributeOutput catt(att.toStdString());
    QString s = catt.toString();
    return s;
  }

  // Set the attributes in the dialog
  void GuiOutputAttribute::SetAttributes(const QString &value) {
    Isis::CubeAttributeOutput att(value.toStdString());
    if(att.fileFormat() == Cube::Tile) {
      p_tiled->setChecked(true);
    }
    else {
      p_bsq->setChecked(true);
    }

    if(att.byteOrder() == Isis::Lsb) {
      p_lsb->setChecked(true);
    }
    else {
      p_msb->setChecked(true);
    }

    if(att.labelAttachment() == AttachedLabel) {
      p_attached->setChecked(true);
    }
    else {
      p_detached->setChecked(true);
    }

    if(att.propagatePixelType()) {
      p_propagate->setChecked(true);
    }
    else if(att.pixelType() == Isis::UnsignedByte) {
      p_unsignedByte->setChecked(true);
    }
    else if(att.pixelType() == Isis::SignedWord) {
      p_signedWord->setChecked(true);
    }
    else if(att.pixelType() == Isis::UnsignedWord) {
      p_unsignedWord->setChecked(true);
    }
    else if(att.pixelType() == Isis::SignedInteger) {
      p_signedInteger->setChecked(true);
    }
    else if(att.pixelType() == Isis::UnsignedInteger) {
      p_unsignedInteger->setChecked(true);
    }
    else {
      p_real->setChecked(true);
    }

    if(!att.propagateMinimumMaximum()) {
      p_minEdit->setText(QString::number(att.minimum()));
      p_maxEdit->setText(QString::number(att.maximum()));
    }
  }

  //! Do we allow propagation
  void GuiOutputAttribute::SetPropagation(bool enabled) {
    p_propagationEnabled = enabled;
    p_propagate->setEnabled(enabled);
  }
}
