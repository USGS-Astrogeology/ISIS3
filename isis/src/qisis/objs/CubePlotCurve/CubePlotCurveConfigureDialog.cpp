/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CubePlotCurveConfigureDialog.h"

#include <iostream>

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

#include <qwt_text.h>

#include "CubePlotCurve.h"
#include "IException.h"
#include "PlotWindow.h"

namespace Isis {
  /**
   * This instantiates a configuration dialog associated with the given cube
   *   plot curve.
   *
   * @param curve The plot curve to be configured.
   * @param parent The parent widget/widget who owns this dialog.
   */
  CubePlotCurveConfigureDialog::CubePlotCurveConfigureDialog(
      CubePlotCurve *curve, QWidget *parent) : QDialog(parent) {
    m_plotCurve = curve;
    m_parent = parent;
    // initially set selected curve index to 0 (first curve)
    m_selectedCurve = 0;

    // we can grab the CubePlotCurve QList from the parent widget (PlotWindow)
    if (parent) {
      connect( parent, SIGNAL( plotChanged() ),
               this, SLOT( updateCurvesList() ) );
      m_plotCurvesList = qobject_cast<PlotWindow *>(parent)->plotCurves();
    }
    else {
      m_plotCurvesList.append(curve);
    }

    QGridLayout *optionsLayout = new QGridLayout;
    int row = 0;

    // only create a combo box if instantiating this dialog with the configure tool button
    if (parent) {
      QLabel *curvesLabel = new QLabel("Curves: ");
      m_curvesCombo = new QComboBox();
      connect( m_curvesCombo, SIGNAL( currentIndexChanged(int) ),
               this, SLOT( updateComboIndex(int) ) );
      optionsLayout->addWidget(curvesLabel, row, 0);
      optionsLayout->addWidget(m_curvesCombo, row, 1);
      row++;
    }

    QLabel *nameLabel = new QLabel("Curve Name: ");
    m_nameEdit = new QLineEdit( m_plotCurve->title().text() );
    optionsLayout->addWidget(nameLabel, row, 0);
    optionsLayout->addWidget(m_nameEdit, row, 1);
    row++;

    QLabel *colorLabel = new QLabel("Color: ");
    m_colorButton = new QPushButton;
    m_colorButton->setFixedWidth(25);
    connect( m_colorButton, SIGNAL( clicked() ),
             this, SLOT( askUserForColor() ) );
    optionsLayout->addWidget(colorLabel, row, 0);
    optionsLayout->addWidget(m_colorButton, row, 1);
    row++;

    QLabel *styleLabel = new QLabel("Style:");
    m_styleCombo = new QComboBox;
    m_styleCombo->addItem("No Line", static_cast<int>(Qt::NoPen));
    m_styleCombo->addItem("Solid Line", static_cast<int>(Qt::SolidLine));
    m_styleCombo->addItem("Dash Line", static_cast<int>(Qt::DashLine));
    m_styleCombo->addItem("Dot Line", static_cast<int>(Qt::DotLine));
    m_styleCombo->addItem("Dash Dot Line", static_cast<int>(Qt::DashDotLine));
    m_styleCombo->addItem("Dash Dot Dot Line", static_cast<int>(Qt::DashDotDotLine));
    optionsLayout->addWidget(styleLabel, row, 0);
    optionsLayout->addWidget(m_styleCombo, row, 1);
    row++;

    QLabel *sizeLabel = new QLabel("Size:");
    m_sizeCombo = new QComboBox;
    m_sizeCombo->addItem("1", 1);
    m_sizeCombo->addItem("2", 2);
    m_sizeCombo->addItem("3", 3);
    m_sizeCombo->addItem("4", 4);
    optionsLayout->addWidget(sizeLabel, row, 0);
    optionsLayout->addWidget(m_sizeCombo, row, 1);
    row++;

    QLabel *symbolLabel = new QLabel("Symbol:");
    m_symbolCombo = new QComboBox;
    m_symbolCombo->addItem("None", QwtSymbol::NoSymbol);

    m_symbolCombo->addItem("Diamond", QwtSymbol::Diamond);
    m_symbolCombo->addItem("Rectangle", QwtSymbol::Rect);
    m_symbolCombo->addItem("Triangle", QwtSymbol::Triangle);
    m_symbolCombo->insertSeparator( m_symbolCombo->count() );

    m_symbolCombo->addItem("Down Facing Triangle", QwtSymbol::UTriangle);
    m_symbolCombo->addItem("Up Facing Triangle", QwtSymbol::DTriangle);
    m_symbolCombo->addItem("Left Facing Triangle", QwtSymbol::RTriangle);
    m_symbolCombo->addItem("Right Facing Triangle", QwtSymbol::LTriangle);
    m_symbolCombo->insertSeparator( m_symbolCombo->count() );

    m_symbolCombo->addItem("Diagonal Cross (X)", QwtSymbol::XCross);
    m_symbolCombo->addItem("Eight-Pointed Star", QwtSymbol::Star1);
    m_symbolCombo->addItem("Ellipse", QwtSymbol::Ellipse);
    m_symbolCombo->addItem("Hexagon", QwtSymbol::Hexagon);
    m_symbolCombo->addItem("Horizontal Line", QwtSymbol::HLine);
    m_symbolCombo->addItem("Plus Sign (+)", QwtSymbol::Cross);
    m_symbolCombo->addItem("Six-Pointed Star", QwtSymbol::Star2);
    m_symbolCombo->addItem("Vertical Line", QwtSymbol::VLine);
    optionsLayout->addWidget(symbolLabel, row, 0);
    optionsLayout->addWidget(m_symbolCombo, row, 1);
    row++;

    QHBoxLayout *applyButtonsLayout = new QHBoxLayout;
    applyButtonsLayout->addStretch();

    QPushButton *okay = new QPushButton("&Ok");
    okay->setIcon( QIcon::fromTheme("dialog-ok") );
    connect( okay, SIGNAL( clicked() ),
             this, SLOT( applySettingsToCurve() ) );
    connect( okay, SIGNAL( clicked() ),
             this, SLOT( close() ) );
    applyButtonsLayout->addWidget(okay);

    QPushButton *apply = new QPushButton("&Apply");
    apply->setIcon( QIcon::fromTheme("dialog-ok-apply") );
    connect( apply, SIGNAL( clicked() ),
             this, SLOT( applySettingsToCurve() ) );
    applyButtonsLayout->addWidget(apply);

    QPushButton *cancel = new QPushButton("&Cancel");
    cancel->setIcon( QIcon::fromTheme("dialog-cancel") );
    connect( cancel, SIGNAL( clicked() ),
             this, SLOT( close() ) );
    applyButtonsLayout->addWidget(cancel);

    QWidget *optionsHolder = new QWidget;
    optionsHolder->setLayout(optionsLayout);

    QWidget *applyButtonsHolder = new QWidget;
    applyButtonsHolder->setLayout(applyButtonsLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(optionsHolder);
    mainLayout->addWidget(applyButtonsHolder);

    setLayout(mainLayout);

    readSettingsFromCurve();
  }


  /**
   * This destroys the configuration dialog, which happens when the user closes
   *   it or clicks ok or cancel.
   */
  CubePlotCurveConfigureDialog::~CubePlotCurveConfigureDialog() {
    m_colorButton = NULL;
    m_plotCurve = NULL;
  }


  /**
   * This takes the configuration settings and applies them to the plot curve.
   *   This happens when the user clicks 'apply' or 'ok.' Any settings that fail
   *   to be applied correctly will be reverted in the GUI.
   */
  void CubePlotCurveConfigureDialog::applySettingsToCurve() {
    if ( m_plotCurve->title() != m_nameEdit->text() ) {
      m_plotCurve->enableAutoRenaming(false);
      m_plotCurve->setTitle( m_nameEdit->text() );
    }

    QPalette colorPalette( m_colorButton->palette() );

    QPen curvePen;
    curvePen.setColor( colorPalette.color(QPalette::Button) );

    int penWidth = m_sizeCombo->itemData( m_sizeCombo->currentIndex() ).toInt();
    curvePen.setWidth(penWidth);

    Qt::PenStyle penStyle = (Qt::PenStyle)m_styleCombo->itemData(
        m_styleCombo->currentIndex() ).toInt();
    curvePen.setStyle(penStyle);

    m_plotCurve->setPen(curvePen);
    m_plotCurve->setColor( colorPalette.color(QPalette::Button) );


    QwtSymbol::Style symbolStyle = (QwtSymbol::Style)m_symbolCombo->itemData(
        m_symbolCombo->currentIndex() ).toInt();
    m_plotCurve->setMarkerSymbol(symbolStyle);

    m_plotCurve->show();
    m_plotCurve->plot()->replot();

    // When user hits Apply, the current curve will still be selected
    // can't use m_curvesCombo->currentIndex() - CubePlotCurve instantiates this config dialog
    // without a combo box
    readSettingsFromCurve();
  }


  /**
   * This takes the current data inside of the plot curve and populates this
   *   configuration dialog's widgets with the appropriate data/settings.
   */
  void CubePlotCurveConfigureDialog::readSettingsFromCurve() {
    // ensure that the m_selectedCurve index is in bounds
    if (m_selectedCurve > m_plotCurvesList.size() - 1 || m_selectedCurve < 0) {
      throw IException(IException::Programmer, "Curves combobox index out of bounds", _FILEINFO_);
    }

    m_plotCurve = m_plotCurvesList[m_selectedCurve];

    setWindowTitle( "Configure " + m_plotCurve->title().text() );

    // see if the curves combo box exists in the dialog (right-clicking a curve will not create
    // a combo box in the dialog)
    if (m_curvesCombo) {
      m_curvesCombo->blockSignals(true);
      m_curvesCombo->clear();
      // add items to combo box
      foreach(CubePlotCurve *curve, m_plotCurvesList) {
        m_curvesCombo->addItem( curve->title().text() );
      }
      m_curvesCombo->setCurrentIndex(m_selectedCurve);
//       m_curvesCombo->setItemText( m_curvesCombo->currentIndex(), m_plotCurve->title().text() );
      m_curvesCombo->blockSignals(false);
    }

    m_nameEdit->setText( m_plotCurve->title().text() );

    QPalette colorPalette;
    colorPalette.setColor( QPalette::Button, m_plotCurve->pen().color() );
    m_colorButton->setPalette(colorPalette);

    if ( m_sizeCombo->count() ) {
      m_sizeCombo->setCurrentIndex(0);
      for (int i = 0; i < m_sizeCombo->count(); i++) {
        if ( m_sizeCombo->itemData(i) == m_plotCurve->pen().width() )
          m_sizeCombo->setCurrentIndex(i);
      }
    }

    if ( m_styleCombo->count() ) {
      m_styleCombo->setCurrentIndex(0);
      for (int i = 0; i < m_styleCombo->count(); i++) {
        if ( m_styleCombo->itemData(i) == static_cast<int>(m_plotCurve->pen().style()) )
          m_styleCombo->setCurrentIndex(i);
      }
    }

    if ( m_symbolCombo->count() ) {
      m_symbolCombo->setCurrentIndex(0);
      for (int i = 0; i < m_symbolCombo->count(); i++) {
          if ( m_symbolCombo->itemData(i).toInt() == m_plotCurve->markerSymbol()->style() ) {
            m_symbolCombo->setCurrentIndex(i);
          }
      }
    }
  }


  void CubePlotCurveConfigureDialog::updateComboIndex(int selected) {
    m_selectedCurve = selected;
    readSettingsFromCurve();
  }


  void CubePlotCurveConfigureDialog::updateCurvesList() {
    QList<CubePlotCurve*> newPlotCurveList = qobject_cast<PlotWindow*>(m_parent)->plotCurves();
    // if we deleted a plot curve, the new list will be smaller in size, reset m_selectedCurve
    if ( newPlotCurveList.size() < m_plotCurvesList.size() ) {
      m_selectedCurve = 0;
    }
    m_plotCurvesList.clear();
    m_plotCurvesList = newPlotCurveList;
    // don't read settings if there are 0 curves in the plot window
    if (m_plotCurvesList.size() != 0) {
      readSettingsFromCurve();
    }
    // close the configure dialog if the last curve was deleted
    else {
      close();
    }
  }

  /**
   * This prompts the user to select a new color for the curve/curve markers.
   */
  void CubePlotCurveConfigureDialog::askUserForColor() {
    QPalette colorPalette( m_colorButton->palette() );

    QColor newColor = QColorDialog::getColor(
        colorPalette.color(QPalette::Button), this);

    if( newColor.isValid() ) {
      colorPalette.setColor(QPalette::Button, newColor);
      m_colorButton->setPalette(colorPalette);
    }
  }
}
