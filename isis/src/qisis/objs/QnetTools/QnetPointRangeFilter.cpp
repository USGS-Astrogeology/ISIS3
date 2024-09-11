#include "QnetPointRangeFilter.h"

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>

#include "ControlNet.h"
#include "ControlPoint.h"
#include "Latitude.h"
#include "Longitude.h"
#include "SerialNumberList.h"
#include "SurfacePoint.h"

using namespace std;

namespace Isis {
  /**
   * Contructor for the Point Range filter.  It creates the
   * Range filter window found in the navtool
   *
   * @param parent The parent widget for the point Range
   *               filter
   * @internal
   *   @history 2010-06-03 Jeannie Walldren - Initialized pointers
   *                          to null.
   *
   */
  QnetPointRangeFilter::QnetPointRangeFilter(QnetNavTool *navTool, QWidget *parent) :
      QnetFilter(navTool, parent) {
    m_minlat = NULL;
    m_maxlat = NULL;
    m_minlon = NULL;
    m_maxlon = NULL;

    // Create the components for the filter window
    m_minlat = new QLineEdit;
    m_maxlat = new QLineEdit;
    m_minlon = new QLineEdit;
    m_maxlon = new QLineEdit;

    // Disable all range line edits since the lat/lon range is not selected
    //  m_minlat->setEnabled(false);
    //  m_maxlat->setEnabled(false);
    //  m_minlon->setEnabled(false);
    //  m_maxlon->setEnabled(false);

    // Create labels for the latitude range values
    QLabel *minlatLabel = new QLabel("Minimum Latitude");
    QLabel *maxlatLabel = new QLabel("Maximum Latitude");
    QLabel *minlonLabel = new QLabel("Minimum Longitude");
    QLabel *maxlonLabel = new QLabel("Maximum Longitude");
    QLabel *pad = new QLabel();

    // Create the layout and add the components to it
    QGridLayout *gridLayout = new QGridLayout();
    gridLayout->addWidget(minlatLabel, 0, 0, 1, 1);
    gridLayout->addWidget(m_minlat, 0, 1, 1, 1);
    gridLayout->addWidget(maxlatLabel, 1, 0, 1, 1);
    gridLayout->addWidget(m_maxlat, 1, 1, 1, 1);
    gridLayout->addWidget(minlonLabel, 2, 0, 1, 1);
    gridLayout->addWidget(m_minlon, 2, 1, 1, 1);
    gridLayout->addWidget(maxlonLabel, 3, 0, 1, 1);
    gridLayout->addWidget(m_maxlon, 3, 1, 1, 1);
    gridLayout->addWidget(pad, 4, 0);
    gridLayout->setRowStretch(4, 50);
    this->setLayout(gridLayout);
  }

  /**
   * Filters a list of points for points that are of the selected
   * Range or in the given range. The filtered list will appear in
   * the navtools point list display.
   * @internal
   *   @history 2009-01-08 Jeannie Walldren - Modified to remove
   *                          new filter points from the existing
   *                          filtered list. Previously, a new
   *                          filtered list was created from the
   *                          entire control net each time.
   *   @history 2010-06-03 Jeannie Walldren - Removed "std::"
   *                          since "using namespace std"
   *
   */
  void QnetPointRangeFilter::filter() {
    // Make sure there is a control net loaded
    if (controlNet() == NULL) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "No points to filter");
      return;
    }

    // Make sure all the values we need have been entered by the user
    if ((m_minlat->text() == "") || (m_maxlat->text() == "") ||
        (m_minlon->text() == "") || (m_maxlon->text() == "")) {
      QMessageBox::information((QWidget *)parent(),
          "Error", "All lat/lon range values must be entered");
      return;
    }
    else {
      // Get the user entered values for the range
      double minlat = m_minlat->text().toDouble();
      double maxlat = m_maxlat->text().toDouble();
      double minlon = m_minlon->text().toDouble();
      double maxlon = m_maxlon->text().toDouble();

      // Make sure the lat values are in order
      if (minlat > maxlat) {
        QString msg = "The minimum latitude value must be less than the maximum latitude value";
        QMessageBox::information((QWidget *)parent(), "Error", msg);
        return;
      }
      // Make sure the lon values are in order
      else if (minlon > maxlon) {
        QString msg = "The minimum longitude value must be less than the maximum longitude value";
        QMessageBox::information((QWidget *)parent(), "Error", msg);
        return;
      }

      // Loop through each value of the filtered points list
      // checking to see if each point falls within the rangee
      // Loop in reverse order since removal list of elements affects index number
      for (int i = filteredPoints().size() - 1; i >= 0; i--) {
        // Get the current control point
        ControlPoint &cp = *(*controlNet())[filteredPoints()[i]];

        Latitude lat = cp.GetBestSurfacePoint().GetLatitude();
        Longitude lon = cp.GetBestSurfacePoint().GetLongitude();
        if (lat.inRange(Latitude(minlat,Angle::Degrees),Latitude(maxlat,Angle::Degrees)) &&
            lon.inRange(Longitude(minlon,Angle::Degrees),Longitude(maxlon,Angle::Degrees))) {
          continue;
        }
        else {
          filteredPoints().removeAt(i);
        }

      }
    }

    // Tell the navtool that a list has been filtered and it needs to update
    emit filteredListModified();
    return;
  }
}
