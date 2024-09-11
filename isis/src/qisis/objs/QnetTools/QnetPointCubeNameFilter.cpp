#include "QnetPointCubeNameFilter.h"

#include <QApplication>
#include <QGridLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>

#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "FileName.h"
#include "QnetNavTool.h"
#include "SerialNumberList.h"

using namespace std;

namespace Isis {
  /**
   * Contructor for the Point Cube Name filter.  It creates the
   * Cube Name filter window found in the navtool
   *
   * @param parent The parent widget for the point type
   *               filter
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Initialized pointer
   *                          to null
   *
   */
  QnetPointCubeNameFilter::QnetPointCubeNameFilter(QnetNavTool *navTool, QWidget *parent)
      : QnetFilter(navTool, parent) {
    p_listBox = NULL;

    QLabel *label = new QLabel("Select cube(s) from the list:");
    p_listBox = new QListWidget;
    p_listBox->setSelectionMode(QAbstractItemView::ExtendedSelection);
    //connect (p_listBox,SIGNAL(itemDoubleClicked(QListWidgetItem *)),
    //         this,SLOT(editPoint(QListWidgetItem *)));

    // Create the layout and add the components to it
    QGridLayout *gridLayout = new QGridLayout;
    gridLayout->addWidget(label, 0, 0);
    gridLayout->addWidget(p_listBox, 1, 0);
    this->setLayout(gridLayout);
    //if (g_serialNumberList != NULL) createCubeList();
    return;
  }

  /**
   * @brief Method overwrites parent method.
   * This method keeps all points that contain at least one
   * measure whose CubeName is within the range specified by the
   * user.
   *
   * @internal
   *   @history 2009-01-26 Jeannie Walldren - Original Version
   */
  void QnetPointCubeNameFilter::filter() {
    // Make sure there is a control net loaded
    if (controlNet() == NULL) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No points to filter");
      return;
    }

    if (serialNumberList() == NULL) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No cubes to filter");
      return;
    }

    // load???
    int index = p_listBox->currentRow();
    if (index < 0) {
      QApplication::restoreOverrideCursor();
      QMessageBox::information((QWidget *)parent(),
          "Error", "No file selected to filter");
      return;
    }

    QList<QListWidgetItem *> selected = p_listBox->selectedItems();

    for (int i = 0; i < selected.size(); i++) {
      int index = p_listBox->row(selected[i]);
      QString selectedCubeSerNum = serialNumberList()->serialNumber(index);

      // Loop through each value of the filtered points list
      // checking the types of each control measure for each
      // of the control points.  If no measures match, we remove
      // it from the filtered list
      // Loop in reverse order since removal list of elements affects index number
      for (int i = filteredPoints().size() - 1; i >= 0; i--) {
        ControlPoint &cp = *(*controlNet())[filteredPoints()[i]];
        int numMeasNotMatching = 0;
        for (int j = 0; j < cp.GetNumMeasures(); j++) {
          // if the point contains a measure that matches a checked type,
          // keep this point in the list and go on to the next point
          if (cp[j]->GetCubeSerialNumber() == selectedCubeSerNum) {
            break;
          }
          // if this measure doesn't match any of the checked values, increment
          else
            numMeasNotMatching++;
        }
        // if no measures match the checked values,
        // remove this point from the filter list
        if (cp.GetNumMeasures() == numMeasNotMatching) {
          filteredPoints().removeAt(i);
        }
      }
    }
    // Tell the navtool that a list has been filtered and it needs to update
    emit filteredListModified();
    return;
  }

  /**
   * Fills the list box with the cube name list
   *
   * @internal
   *   @history  2009-01-26 Jeannie Walldren - Original version
   */
  void QnetPointCubeNameFilter::createCubeList() {
    // Clear the old list and update with the entire list
    p_listBox->setCurrentRow(-1);
    p_listBox->clear();

    SerialNumberList *snList = serialNumberList();
    for (int i = 0; i < snList->size(); i++) {
      FileName filename = FileName(snList->fileName(i).toStdString());
      QString tempFileName = QString::fromStdString(filename.name());
      p_listBox->insertItem(i, tempFileName);
    }
  }
}





