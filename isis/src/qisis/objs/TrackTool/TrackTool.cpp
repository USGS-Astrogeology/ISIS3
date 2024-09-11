#include "TrackTool.h"

#include <QStatusBar>
#include <QLabel>
#include <QCursor>

#include "Camera.h"
#include "Distance.h"
#include "MdiCubeViewport.h"
#include "Projection.h"
#include "RingPlaneProjection.h"
#include "SpecialPixel.h"
#include "Target.h"
#include "TProjection.h"
#include "ViewportBuffer.h"
#include "WarningWidget.h"


namespace Isis {
  /**
   * TrackTool constructor
   *
   *
   * @param parent
   */
  TrackTool::TrackTool(QStatusBar *parent) : Tool(parent) {
    p_sbar = parent;

    p_sampLabel = new QLabel(p_sbar);
    p_sampLabel->setText("W 999999");
    p_sampLabel->setMinimumSize(p_sampLabel->sizeHint());
    p_sampLabel->setToolTip("Sample Position");
    p_sbar->addPermanentWidget(p_sampLabel);

    p_lineLabel = new QLabel(p_sbar);
    p_lineLabel->setText("W 999999");
    p_lineLabel->setMinimumSize(p_lineLabel->sizeHint());
    p_lineLabel->setToolTip("Line Position");
    p_sbar->addPermanentWidget(p_lineLabel);

    p_latLabel = new QLabel(p_sbar);
    p_latLabel->setText("9.999999E-99");
    p_latLabel->setMinimumSize(p_latLabel->sizeHint());
    p_latLabel->hide();
    p_latLabel->setToolTip("Latitude Position");
    p_sbar->addPermanentWidget(p_latLabel);

    p_lonLabel = new QLabel(p_sbar);
    p_lonLabel->setText("9.999999E-99");
    p_lonLabel->setMinimumSize(p_lonLabel->sizeHint());
    p_lonLabel->hide();
    p_lonLabel->setToolTip("Longitude Position");
    p_sbar->addPermanentWidget(p_lonLabel);

    p_grayLabel = new QLabel(p_sbar);
    p_grayLabel->setText("9.999999E-99");
    p_grayLabel->setMinimumSize(p_grayLabel->sizeHint());
    p_grayLabel->setToolTip("Gray Pixel Value");
    p_sbar->addPermanentWidget(p_grayLabel);

    p_redLabel = new QLabel(p_sbar);
    p_redLabel->setText("W 9.999999E-99");
    p_redLabel->setMinimumSize(p_redLabel->sizeHint());
    p_redLabel->hide();
    p_redLabel->setToolTip("Red Pixel Value");
    p_sbar->addPermanentWidget(p_redLabel);

    p_grnLabel = new QLabel(p_sbar);
    p_grnLabel->setText("W 9.999999E-99");
    p_grnLabel->setMinimumSize(p_grnLabel->sizeHint());
    p_grnLabel->hide();
    p_grnLabel->setToolTip("Green Pixel Value");
    p_sbar->addPermanentWidget(p_grnLabel);

    p_bluLabel = new QLabel(p_sbar);
    p_bluLabel->setText("W 9.999999E-99");
    p_bluLabel->setMinimumSize(p_bluLabel->sizeHint());
    p_bluLabel->hide();
    p_bluLabel->setToolTip("Blue Pixel Value");
    p_sbar->addPermanentWidget(p_bluLabel);

    mWarningWidget = new WarningWidget(p_sbar);
    connect(p_sbar, SIGNAL(messageChanged(const QString &)), mWarningWidget, SLOT(checkMessage()));

    clearLabels();

    activate(true);
  }

  /**
   * Display the Warning icon in case of an exception, sent from
   * the tool where the exception occured
   *
   * @param pStr   - Topmost Exception Message String
   * @param pExStr - Propagated exception message string
   */
  void TrackTool::displayWarning(std::string &pStr,  const std::string   &pExStr) {
    mWarningWidget->viewWarningWidgetIcon(pStr, pExStr);
  }

  /**
   * Resets the warning status on the status bar to default
   */
  void TrackTool::resetStatusWarning(void) {
    if(mWarningWidget != NULL) {
      mWarningWidget->resetWarning();
    }
  }

  /**
   * Updates the labels anytime the mouse moves.
   *
   *
   * @param p
   */
  void TrackTool::mouseMove(QPoint p) {
    MdiCubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    if(p.x() >= 0 && p.x() < cvp->width() &&
        p.y() >= 0 && p.y() < cvp->height()) {
      updateLabels(p);
    }
  }


  /**
   * Clears the labels if the mouse leaves the application.
   *
   */
  void TrackTool::mouseLeave() {
    clearLabels();
  }


  /**
   * Updates the tracking labels.
   *
   *
   * @param p
   */
  void TrackTool::updateLabels(QPoint p) {
    MdiCubeViewport *cvp = cubeViewport();

    clearLabels();

    if(cvp == NULL) {
      return;
    }

    double sample, line;
    cvp->viewportToCube(p.x(), p.y(), sample, line);
    if((sample < 0.5) || (line < 0.5) ||
        (sample > cvp->cubeSamples() + 0.5) ||
        (line > cvp->cubeLines() + 0.5)) {
      return;
    }

    int isamp = (int)(sample + 0.5);
    QString text;
    text.setNum(isamp);
    text = "S " + text;
    p_sampLabel->setText(text);

    int iline = (int)(line + 0.5);
    text.setNum(iline);
    text = "L " + text;
    p_lineLabel->setText(text);


    // Do we have a projection?
    if(cvp->projection() != NULL) {
      // Set up for projection types
      Projection::ProjectionType projType = cvp->projection()->projectionType();
      p_latLabel->show();
      p_lonLabel->show();

      if(cvp->projection()->SetWorld(sample, line)) {
        if (projType == Projection::Triaxial) {
          TProjection *tproj = (TProjection *) cvp->projection();
          double lat = tproj->Latitude();
          double lon = tproj->Longitude();
          if (cvp->projection()->IsSky()) {
            p_latLabel->setText(QString("DEC %1").arg(lat));
            p_lonLabel->setText(QString("RA %1").arg(lon));
          }
          else {
            p_latLabel->setText(QString("Lat %1").arg(lat));
            p_lonLabel->setText(QString("Lon %1").arg(lon));
          }
        }
        else { // RingPlane TODO write out radius azimuth instead of lat/lon
          RingPlaneProjection *rproj = (RingPlaneProjection *) cvp->projection();
          double rad = rproj->RingRadius();
          double lon = rproj->RingLongitude();
          //??? p_latLabel->setToolTip("Radius Position");
          p_latLabel->setText(QString("Rad %1").arg(rad));
          p_lonLabel->setText(QString("Lon %1").arg(lon));
        }
      }
      else {
        p_latLabel->setText("Lat N/A");
        p_lonLabel->setText("Lon N/A");
      }
    }
    // Do we have a camera model?
    else if(cvp->camera() != NULL) {
      p_latLabel->show();
      p_lonLabel->show();

      if(cvp->camera()->SetImage(sample, line)) {
        if (cvp->camera()->target()->shape()->name() != "Plane") {
          if (cvp->camera()->target()->isSky()) {
            double dec = cvp->camera()->Declination();
            double ra = cvp->camera()->RightAscension();
            p_latLabel->setText(QString("DEC %1").arg(dec));
            p_lonLabel->setText(QString("RA %1").arg(ra));
          }
          else {
            double lat = cvp->camera()->UniversalLatitude();
            double lon = cvp->camera()->UniversalLongitude();
            p_latLabel->setText(QString("Lat %1").arg(lat));
            p_lonLabel->setText(QString("Lon %1").arg(lon));
          }
        }
        else {
          double rad = cvp->camera()->LocalRadius().meters();
          double lon = cvp->camera()->UniversalLongitude();
          //??? p_latLabel->setToolTip("Radius Position");
          p_latLabel->setText(QString("Rad %1").arg(rad));
          p_lonLabel->setText(QString("Lon %1").arg(lon));
        }
      }
      else {
        p_latLabel->setText("Lat N/A");
        p_lonLabel->setText("Lon N/A");
      }
    }

    else {
      p_latLabel->hide();
      p_lonLabel->hide();
    }

    if(cvp->isGray()) {
      p_grayLabel->show();
      p_redLabel->hide();
      p_grnLabel->hide();
      p_bluLabel->hide();

      ViewportBuffer *buf = cvp->grayBuffer();

      QString pixelString = updateColorLabel(p, buf, p_grayLabel);

      p_grayLabel->setText(pixelString);
    }
    else {
      p_grayLabel->hide();
      p_redLabel->show();
      p_grnLabel->show();
      p_bluLabel->show();

      ViewportBuffer *redBuf = cvp->redBuffer();
      QString pixelString = updateColorLabel(p, redBuf, p_redLabel);
      QString rLab = "R ";
      rLab += pixelString;
      p_redLabel->setText(rLab);

      ViewportBuffer *greenBuf = cvp->greenBuffer();
      pixelString = updateColorLabel(p, greenBuf, p_grnLabel);
      QString gLab = "G ";
      gLab += pixelString;
      p_grnLabel->setText(gLab);

      ViewportBuffer *blueBuf = cvp->blueBuffer();
      pixelString = updateColorLabel(p, blueBuf, p_bluLabel);
      QString bLab = "B ";
      bLab += pixelString;
      p_bluLabel->setText(bLab);
    }
  }

  QString TrackTool::updateColorLabel(QPoint p, ViewportBuffer *buf, QLabel *label) {
    if(!buf->working()) {
      const QRect rRect = buf->bufferXYRect();

      if(p.x() >= rRect.left() && p.x() < rRect.right() &&
          p.y() >= rRect.top() && p.y() < rRect.bottom()) {
        const int rBufX = p.x() - rRect.left();
        const int rBufY = p.y() - rRect.top();
        return QString::fromStdString(PixelToString(buf->getLine(rBufY)[rBufX], 12));
      }
    }
    return "BUSY";
  }


  /**
   * Clears the labels.
   *
   */
  void TrackTool::clearLabels() {
    p_sampLabel->setText("S N/A");
    p_lineLabel->setText("L N/A");
    p_latLabel->setText("Lat N/A");
    p_lonLabel->setText("Lon N/A");
    p_grayLabel->setText("N/A");
    p_redLabel->setText("R N/A");
    p_grnLabel->setText("G N/A");
    p_bluLabel->setText("B N/A");
  }


  /**
   * Finds the cursor position.
   *
   */
  void TrackTool::locateCursor() {
    if(cubeViewport() == NULL) return;
    QPoint p = cubeViewport()->viewport()->mapFromGlobal(QCursor::pos());
    if(p.x() < 0) return;
    if(p.y() < 0) return;
    if(p.x() >= cubeViewport()->viewport()->width()) return;
    if(p.y() >= cubeViewport()->viewport()->height()) return;
    updateLabels(p);
  }


  /**
   * Adds the connections to the given viewport.
   *
   *
   * @param cvp
   */
  void TrackTool::addConnections(MdiCubeViewport *cvp) {
    connect(cubeViewport(), SIGNAL(viewportUpdated()),
            this, SLOT(locateCursor()));
  }


  /**
   * Removes the connections from the given viewport.
   *
   *
   * @param cvp
   */
  void TrackTool::removeConnections(MdiCubeViewport *cvp) {
    disconnect(cubeViewport(), SIGNAL(viewportUpdated()),
               this, SLOT(locateCursor()));
  }


  QStatusBar *TrackTool::getStatusBar(void) {
    return p_sbar;
  }
}
