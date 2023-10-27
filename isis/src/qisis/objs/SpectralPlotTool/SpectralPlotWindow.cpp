#include "SpectralPlotWindow.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QString>

#include "Cube.h"
#include "MdiCubeViewport.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlObject.h"


using namespace std;

namespace Isis {
  /**
   * This constructs a spectral plot window. The spectral plot window graphs a
   *   spectral curve sent to it via the addPlotCurve() method.
   *
   * @param xUnits The units to use for the x-axis: usually wavelength/band #
   * @param parent The Qt-parent relationship parent for this window
   */
  SpectralPlotWindow::SpectralPlotWindow(PlotCurve::Units xUnits,
      QWidget *parent) : PlotWindow("Spectral Plot", xUnits,
                                    PlotCurve::CubeDN, parent) {
    nullify();

    m_showHideBandMarkers = new QAction("Show Band Markers", this);
    m_showHideBandMarkers->setCheckable(true);
    m_showHideBandMarkers->setChecked(true);
    connect(m_showHideBandMarkers, SIGNAL(toggled(bool)),
            this, SLOT(setBandMarkersVisible(bool)));

    m_grayBandLine = createMarker(Qt::white);
    m_redBandLine = createMarker(Qt::red);
    m_greenBandLine = createMarker(Qt::green);
    m_blueBandLine = createMarker(Qt::blue);

    setBandMarkersVisible(m_showHideBandMarkers->isChecked());

    foreach (QAction *menuAction, menuBar()->actions()) {
      if (menuAction->text() == "&Options") {
        QMenu *optsMenu = qobject_cast<QMenu *>(menuAction->parentWidget());

        optsMenu->addAction(m_showHideBandMarkers);
      }
    }
  }


  SpectralPlotWindow::~SpectralPlotWindow() {
    nullify();
  }


  /**
   * This initializes the class member data to NULL.
   */
  void SpectralPlotWindow::nullify() {
    m_cvp = NULL;
    m_grayBandLine = NULL;
    m_redBandLine = NULL;
    m_greenBandLine = NULL;
    m_blueBandLine = NULL;
    m_showHideBandMarkers = NULL;
  }


  /**
   * This is a helper method to create new band markers with the same line
   *   style and a custom color.
   *
   * @param color The color of the band marker
   * @return The requested plot marker; ownership is passed to the caller
   */
  QwtPlotMarker *SpectralPlotWindow::createMarker(QColor color) {
    QPen markerPen(color);
    markerPen.setWidth(1);

    QwtPlotMarker *newMarker = new QwtPlotMarker;
    newMarker->setLineStyle(QwtPlotMarker::LineStyle(2));
    newMarker->setLinePen(markerPen);
    newMarker->attach(plot());
    newMarker->setVisible(false);

    return newMarker;
  }


  /**
   * This method actually draws in the vertical band line(s) on
   * the plot area.
   *
   */
  void SpectralPlotWindow::drawBandMarkers() {
    if (m_cvp) {
      int redBand = 0, greenBand = 0, blueBand = 0, grayBand = 0;

      Cube *cube = m_cvp->cube();
      Pvl &pvl = *cube->label();
      PvlKeyword wavelengths;

      if (pvl.findObject("IsisCube").hasGroup("BandBin")) {
        PvlGroup &bandBin = pvl.findObject("IsisCube").findGroup("BandBin");
        if (bandBin.hasKeyword("Center")) {
          wavelengths = bandBin.findKeyword("Center");
        }
      }

      if (m_cvp->isColor()) {
        redBand = m_cvp->redBand();
        greenBand = m_cvp->greenBand();
        blueBand = m_cvp->blueBand();
      }
      else {
        grayBand = m_cvp->grayBand();
      }

      /*This is were we need to set the x value to the band number.*/
      if (grayBand > 0) {
        if (xAxisUnits() == PlotCurve::Wavelength) {
          m_grayBandLine->setXValue(std::stod(wavelengths[grayBand-1]));
        }
        else {
          m_grayBandLine->setXValue(grayBand);
        }

        if (m_markersVisible)
          m_grayBandLine->show();
      }
      else {
        m_grayBandLine->hide();
      }

      if (redBand > 0) {
        if (xAxisUnits() == PlotCurve::Wavelength) {
          m_redBandLine->setXValue(std::stod(wavelengths[redBand-1]));
        }
        else {
          m_redBandLine->setXValue(redBand);
        }

        if (m_markersVisible)
          m_redBandLine->show();
      }
      else {
        m_redBandLine->hide();
      }
      if (greenBand > 0) {
        if (xAxisUnits() == PlotCurve::Wavelength) {
          m_greenBandLine->setXValue(std::stod(wavelengths[greenBand-1]));
        }
        else {
          m_greenBandLine->setXValue(greenBand);
        }

        if (m_markersVisible)
          m_greenBandLine->show();
      }
      else {
        m_greenBandLine->hide();
      }

      if (blueBand > 0) {
        if (xAxisUnits() == PlotCurve::Wavelength) {
          m_blueBandLine->setXValue(std::stod(wavelengths[blueBand-1]));
        }
        else {
          m_blueBandLine->setXValue(blueBand);
        }

        if (m_markersVisible)
          m_blueBandLine->show();
      }
      else {
        m_blueBandLine->hide();
      }

      plot()->replot();
    }
  }


  /**
   * This class needs to know which viewport the user is looking
   * at so it can appropriately draw in the band lines.
   *
   * @param cvp
   */
  void SpectralPlotWindow::setViewport(MdiCubeViewport *cvp) {
    m_cvp = cvp;

  }


  /**
   *
   *
   * @param visible
   */
  void SpectralPlotWindow::setBandMarkersVisible(bool visible) {
    m_markersVisible = visible;

    m_blueBandLine->setVisible(m_markersVisible);
    m_redBandLine->setVisible(m_markersVisible);
    m_greenBandLine->setVisible(m_markersVisible);
    m_grayBandLine->setVisible(m_markersVisible);

    replot();
  }


  /**
   * This window can show markers for the currently visible bands. This will
   *   synchronize the markers with the given viewport.
   *
   * @param activeViewport The viewport to synchronize this plot window with
   */
  void SpectralPlotWindow::update(MdiCubeViewport *activeViewport) {
    setViewport(activeViewport);
    drawBandMarkers();
  }


  /**
   *
   *
   *
   * @return bool
   */
  bool SpectralPlotWindow::bandMarkersVisible() const {
    return m_markersVisible;
  }
}

