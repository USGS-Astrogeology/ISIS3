#include "ManualStretchType.h"

#include <QVBoxLayout>
#include <QLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>

#include "CubeViewport.h"
#include "HistogramWidget.h"
#include "Statistics.h"
#include "Stretch.h"
#include "CubeStretch.h"

namespace Isis {
  /**
   * This constructs a manual stretch type.
   *
   * @param hist
   * @param stretch
   * @param name
   * @param color
   */
  ManualStretchType::ManualStretchType(const Histogram &hist,
                                       const Stretch &stretch,
                                       const QString &name, const QColor &color)
      : StretchType(hist, stretch, name, color) {
    p_errorMessage = NULL;

    QWidget *buttonContainer = new QWidget();
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    p_errorMessage = new QLabel;

    QPushButton *addButton = new QPushButton("Add Row");
    connect(addButton, SIGNAL(clicked(bool)),
            this, SLOT(addButtonPressed(bool)));
    buttonLayout->addWidget(addButton);

    QPushButton *deleteButton = new QPushButton("Delete Row");
    connect(deleteButton, SIGNAL(clicked(bool)),
            this, SLOT(deleteButtonPressed(bool)));
    buttonLayout->addWidget(deleteButton);

    buttonContainer->setLayout(buttonLayout);

    p_mainLayout->addWidget(buttonContainer, 1, 0);
    p_mainLayout->addWidget(p_errorMessage, 4, 0);

    p_table->setSelectionMode(QAbstractItemView::SingleSelection);
    p_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    p_table->setEditTriggers(QAbstractItemView::DoubleClicked |
                             QAbstractItemView::SelectedClicked |
                             QAbstractItemView::AnyKeyPressed);
    connect(p_table, SIGNAL(cellChanged(int, int)),
            this, SLOT(readTable()));
    disconnect(this, SIGNAL(stretchChanged()), this, SLOT(updateTable()));

    setLayout(p_mainLayout);
    setStretch(stretch);
  }


  /**
   * Destructor
   */
  ManualStretchType::~ManualStretchType() {
  }


  /**
   * Given an arbitrary stretch, this will re-interpret it, as
   * best as possible, into a manual stretch. It is required that
   * a stretch that represents a manual stretch always translate
   * into itself and does not cause a stretchChanged().
   *
   * It is necessary to always update slider positions in this
   * method even if the stretch did not change.
   *
   * Good thing this is a manual stretch so no interpretation is
   * really needed.
   *
   * @param newStretch Stretch to interpret
   */
  void ManualStretchType::setStretch(Stretch newStretch) {
    if(newStretch.Text() != p_stretch->Text()) {
      p_stretch->CopyPairs(newStretch);
      updateTable();
      emit stretchChanged();
    }
  }


  /**
   * This is called when a user clicks "Add / Edit" and is
   * responsible for adding the pair into the correct location or
   * editing the pair at that location (input value).
   */
  void ManualStretchType::addButtonPressed(bool) {
    p_table->insertRow(p_table->rowCount());
  }


  /**
   * This is called when a user clicks "Delete" and is
   * responsible for removing the pair with the given input value.
   */
  void ManualStretchType::deleteButtonPressed(bool) {
    QList<QTableWidgetSelectionRange> selectedRanges =
        p_table->selectedRanges();

    if (selectedRanges.empty()) {
      IString msg = "You must select a row to delete";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    int row = selectedRanges.first().topRow();
    p_table->removeRow(row);
  }

  void ManualStretchType::readTable() {
    *p_stretch = convertTableToStretch();
    emit stretchChanged();
  }


  Stretch ManualStretchType::convertTableToStretch() {
    Stretch stretch(*p_stretch);
    stretch.ClearPairs();

    p_errorMessage->setText("");
    try {
      if (p_table->columnCount() == 2) {
        for (int i = 0; i < p_table->rowCount(); i++) {
          if (p_table->item(i, 0) &&
              p_table->item(i, 1) &&
              !p_table->item(i, 0)->data(Qt::DisplayRole).isNull() &&
              !p_table->item(i, 1)->data(Qt::DisplayRole).isNull()) {
            stretch.AddPair(
                p_table->item(i, 0)->data(Qt::DisplayRole).toString().toDouble(),
                p_table->item(i, 1)->data(Qt::DisplayRole).toString().toDouble());
          }
        }
      }
    }
    catch (IException &e) {
      p_errorMessage->setText(
          "<font color='red'>" + QString::fromStdString(e.toString()) + "</font>");
    }

    return stretch;
  }


/**
 * Gets the current CubeStretch for this ManualStretch. 
 *  
 * @return CubeStretch 
 */
  CubeStretch ManualStretchType::getStretch() {
    CubeStretch cubeStretch(*p_stretch, "Manual");
    return cubeStretch;
  }

}
