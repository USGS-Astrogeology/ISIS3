/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "MdiCubeViewport.h"

#include <QIcon>
#include <QPainter>

#include <iostream>
#include <string>

#include "FileName.h"
#include "IString.h"
#include "RubberBandTool.h"
#include "StretchTool.h"
#include "Tool.h"


using namespace std;


namespace Isis {
  MdiCubeViewport::MdiCubeViewport(Cube *cube, CubeDataThread * cdt,
      QWidget *parent) : CubeViewport(cube, cdt, parent) {
    p_linked = false;

    QString unlinkedIcon = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/unlinked.png").expanded());
    static QIcon unlinked(unlinkedIcon);
    parentWidget()->setWindowIcon(unlinked);

  }

  MdiCubeViewport::~MdiCubeViewport() {
  }


  /**
   * Registers the tool given tool.
   *
   * @param tool
   */
  void MdiCubeViewport::registerTool(Tool *tool) {
    p_toolList.push_back(tool);
  }


  /**
   * Change the linked state of the viewport
   *
   *
   * @param b
   */
  void MdiCubeViewport::setLinked(bool b) {
    if(!parentWidget() || !parentWidget()->parentWidget())
      return;

    QString unlinkedIcon = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/unlinked.png").expanded());
    static QIcon unlinked(unlinkedIcon);
    QString linkedIcon = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/linked.png").expanded());
    static QIcon linked(linkedIcon);

    bool notify = false;
    if(b != p_linked)
      notify = true;

    p_linked = b;
    if(p_linked) {
      parentWidget()->parentWidget()->setWindowIcon(linked);
    }
    else {
      parentWidget()->parentWidget()->setWindowIcon(unlinked);
    }

    if(notify)
      emit linkChanging(b);
  }


  /**
   * Repaint the viewport
   *
   * @param e [in]  (QPaintEvent *)  event
   *
   * @internal
   *
   * @history  2007-04-30  Tracie Sucharski - Add the QPainter to the call to
   *                           Tool::paintViewport.
   */
  void MdiCubeViewport::paintEvent(QPaintEvent *e) {
    CubeViewport::paintEvent(e);

    QPainter painter(viewport());
    painter.drawPixmap(0, 0, p_pixmap);
    emit viewportUpdated();

    // Draw anything the tools might need
    for(int i = 0; i < p_toolList.size(); i++) {
      p_toolList[i]->paintViewport(this, &painter);
    }

    painter.end();
  }


  void MdiCubeViewport::viewGray(int band) {
    CubeViewport::viewGray(band);

    for(int i = 0; i < p_toolList.size(); i++)
      p_toolList[i]->updateTool();
  }


  void MdiCubeViewport::viewRGB(int rband, int gband, int bband) {
    CubeViewport::viewRGB(rband, gband, bband);

    for(int i = 0; i < p_toolList.size(); i++)
      p_toolList[i]->updateTool();
  }


  void MdiCubeViewport::restretch(ViewportBuffer *buffer) {
    if(buffer == grayBuffer()) {
      emit requestRestretch(this, (int)StretchTool::Gray);
    }
    else if(buffer == redBuffer()) {
      emit requestRestretch(this, (int)StretchTool::Red);
    }
    else if(buffer == greenBuffer()) {
      emit requestRestretch(this, (int)StretchTool::Green);
    }
    else if(buffer == blueBuffer()) {
      emit requestRestretch(this, (int)StretchTool::Blue);
    }
  }
}
