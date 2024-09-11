/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "LatLonGridTool.h"

#include <QAction>
#include <QPixmap>
#include <QStackedWidget>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QPainter>

#include "MdiCubeViewport.h"
#include "ToolPad.h"
#include "Camera.h"


namespace Isis {
  /**
   * Constructs an LatLonGridTool object.
   *
   * @param parent Parent widget
   */
  LatLonGridTool::LatLonGridTool(QWidget *parent) : Tool(parent) {
  }

  /**
   * Adds the LatLonGridTool to the tool pad.
   *
   * @param pad input - The tool pad that LatLonGridTool is to be added to
   *
   * @return QAction*
   */
  QAction *LatLonGridTool::toolPadAction(ToolPad *pad) {
    QAction *action = new QAction(pad);
    action->setIcon(QPixmap(toolIconDir() + "/grid.png"));
    action->setToolTip("Lat Lon Grid Tool (G)");
    action->setShortcut(Qt::Key_G);

    QString text  =
      "<b>Function:</b>  View lat lon grid \
      <p><b>Shortcut:</b> G</p> ";
    action->setWhatsThis(text);

    return action;
  }

  /**
   * Creates the toolbar containing the lat-lon grid tool widgets
   *
   * @param active  input  The widget that will contain the lat-lon grid tool
   *                       specific widgets
   *
   * @return QWidget*
   */
  QWidget *LatLonGridTool::createToolBarWidget(QStackedWidget *active) {
    QWidget *container = new QWidget(active);
    container->setObjectName("LatLonGridToolActiveToolBarWidget");

    m_gridCheckBox = new QCheckBox;
    m_gridCheckBox->setText("Show Grid");

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(m_gridCheckBox);
    layout->addStretch(1);
    container->setLayout(layout);

    m_container = container;
    return container;
  }

  /**
   * Draws grid onto cube viewport
   * This is overiding the parents paintViewport member.
   *
   * @param vp Pointer to Viewport to be painted
   * @param painter
   */
  void LatLonGridTool::paintViewport(MdiCubeViewport *mvp, QPainter *painter) {
    int x1, x2, y1, y2;
    double lat, lon;

    QFont font;
    QBrush brush(Qt::gray);
    QPen pen(brush, 1);

    // Only draws if "Show Grid" checkbox is checked
    if (m_gridCheckBox->isChecked()) {
      painter->setPen(pen);
      font.setPixelSize(8);
      painter->setFont(font);

      // Draws Longitude Lines
      for (int i = mvp->cubeSamples(); i > 0; i -= mvp->cubeSamples() / 12) {
        if (mvp->camera() != NULL) {
            mvp->camera()->SetImage(i, 0);
            lon = mvp->camera()->UniversalLongitude();

            lon = ceil(lon * 100.0) / 100.0;

            mvp->cubeToViewport(i, 0, x1, y1);
            mvp->cubeToViewport(0, mvp->cubeLines(), x2, y2);
            painter->drawLine(x1, y1, x1, y2);

            painter->drawText(x1, y2 + 10, QString::number(lon));
        }
      }

      // Draws Latitude Lines
      for (int i = mvp->cubeLines(); i > 0; i -= mvp->cubeLines() / 12) {
        if (mvp->camera() != NULL) {
            mvp->camera()->SetImage(0, i);
            lat = mvp->camera()->UniversalLatitude();

            lat = ceil(lat * 100.0) / 100.0;

            mvp->cubeToViewport(0, i, x1, y1);
            mvp->cubeToViewport(mvp->cubeSamples(), 0, x2, y2);
            painter->drawLine(x1, y1, x2, y1);

            painter->drawText(x2 + 5, y1, QString::number(lat));
        }
      }
    }
    // remove grid by updating viewport to original cubeViewport
    else {
      mvp = cubeViewport();
    }
  }

  /**
   * Enables/Disable grid option tool based on camera model
   */
  void LatLonGridTool::updateTool() {
    MdiCubeViewport *vp = cubeViewport();

    if (vp != NULL) {
      if (vp->camera() == NULL) {
        m_gridCheckBox->setEnabled(false);
      }
      else {
          m_gridCheckBox->setEnabled(true);
      }
    }
  }
}
