#include "PlotWindow.h"

#include <algorithm>
#include <iostream>

#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_spectrogram.h>
#include <qwt_symbol.h>
#include <qwt_scale_engine.h>
#include <qwt_text.h>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QFileDialog>
#include <QGridLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QProgressDialog>
#include <QPushButton>
#include <QSet>
#include <QTableWidget>
#include <QToolBar>
#include <QPageSize>

#include "Cube.h"
#include "CubePlotCurve.h"
#include "CubePlotCurveConfigureDialog.h" //
#include "CubeViewport.h"
#include "FileName.h"
#include "Interpolator.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "PlotWindowBestFitDialog.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "QHistogram.h"
#include "Stretch.h"
#include "TableMainWindow.h"


using namespace std;

namespace Isis {
  /**
   * This constructs a plot window. The plot window graphs any
   * curve sent to it via the addPlotCurve() method.
   *
   * @param title The window title and default plot title
   * @param xAxisUnits The x-bottom axis data's units, which must match any
   *                   curves' units that are added to this window.
   * @param yAxisUnits The y-left axis data's units, which must match any
   *                   curves' units that are added to this window.
   * @param parent The Qt parent widget
   * @param optionsToProvide A bit-flag containing information on which options
   *                         to provide to the users.
   */
  PlotWindow::PlotWindow(QString title, PlotCurve::Units xAxisUnits,
                         PlotCurve::Units yAxisUnits, QWidget *parent,
                         MenuOptions optionsToProvide) :
        MainWindow(title, parent) {

    m_toolBar = NULL;
    m_menubar = NULL;
    m_tableWindow = NULL;
    m_pasteAct = NULL;
    m_allowUserToAddCurves = true;
    m_autoscaleAxes = true;

    setObjectName("Plot Window: " + title);

    m_parent = parent;
    m_xAxisUnits = xAxisUnits;
    m_yAxisUnits = yAxisUnits;

    if (!m_parent) {
      IString msg = "PlotWindow cannot be instantiated with a NULL parent";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    installEventFilter(this);
    setAcceptDrops(true);

    createWidgets(optionsToProvide);
    setWindowTitle(title);

    setPlotBackground(Qt::black);

    connect(QGuiApplication::clipboard(), SIGNAL(changed(QClipboard::Mode)),
            this, SLOT(onClipboardChanged()));
    connect(this, SIGNAL(plotChanged()),
            this, SLOT(scheduleFillTable()));
    connect(this, SIGNAL(requestFillTable()),
            this, SLOT(fillTable()), Qt::QueuedConnection);

    QMap<PlotCurve::Units, QString> unitLabels;
    unitLabels.insert(PlotCurve::Band, "Band");
    unitLabels.insert(PlotCurve::Percentage, "Percentage");
    unitLabels.insert(PlotCurve::PixelNumber, "Pixel Number");
    unitLabels.insert(PlotCurve::CubeDN, "Pixel Value");
    unitLabels.insert(PlotCurve::Elevation, "Elevation");
    unitLabels.insert(PlotCurve::Meters, "Meters");
    unitLabels.insert(PlotCurve::Kilometers, "Kilometers");
    unitLabels.insert(PlotCurve::Wavelength, "Wavelength");

    plot()->setAxisTitle(QwtPlot::xBottom, unitLabels[xAxisUnits]);
    plot()->setAxisTitle(QwtPlot::yLeft,   unitLabels[yAxisUnits]);
    setPlotTitle(title);

    onClipboardChanged();

    readSettings();

#ifdef __APPLE__
    setWindowFlags(Qt::Tool);
#else
    setWindowFlags(Qt::Dialog);
#endif
  }



  PlotWindow::~PlotWindow() {
    foreach (QwtPlotCurve *curve, plotCurves()) {
      delete curve;
    }
  }


  /**
   * This method is called by the constructor to create the
   *   plot, legend. zoomer, and main window.
   *
   * @param optionsToProvide This is a bit-flag containing information on which
   *                         menu options to give the user/put in the GUI.
   */
  void PlotWindow::createWidgets(MenuOptions optionsToProvide) {
    /*Create plot*/
    m_plot = new QwtPlot();
    m_plot->installEventFilter(this);
    m_plot->setAxisMaxMinor(QwtPlot::yLeft, 5);
    m_plot->setAxisMaxMajor(QwtPlot::xBottom, 30);
    m_plot->setAxisMaxMinor(QwtPlot::xBottom, 5);
    m_plot->setAxisLabelRotation(QwtPlot::xBottom, 45);
    m_plot->setAxisLabelAlignment(QwtPlot::xBottom, Qt::AlignRight);

    /*Plot Legend*/
    m_legend = new QwtLegend();
    m_legend->setDefaultItemMode(QwtLegendData::Clickable);
    m_legend->setWhatsThis("Right Click on a legend item to display the context "
                         "menu.");
    m_plot->insertLegend(m_legend, QwtPlot::RightLegend, 1.0);
    m_legend->installEventFilter(this);

    /*Plot Grid*/
    m_grid = new QwtPlotGrid;
    m_grid->enableXMin(true);
    m_grid->setMajorPen(QPen(Qt::white, 1, Qt::DotLine));
    m_grid->setMinorPen(QPen(Qt::gray, 1, Qt::DotLine));
    m_grid->attach(m_plot);
    m_grid->setVisible(false);

    /*Plot Zoomer*/
    m_zoomer = new QwtPlotZoomer(m_plot->canvas());
    m_zoomer->setRubberBandPen(QPen(Qt::lightGray));
    m_zoomer->setTrackerPen(QPen(Qt::lightGray));

    setCentralWidget(m_plot);
    setupDefaultMenu(optionsToProvide);
  }


  /**
   * Shows the plot window, and raises it to the front of any
   * overlapping sibling widgets.
   */
  void PlotWindow::showWindow() {
    raise();
    show();
  }


  /**
   * This is provided to allow children to react to tool updates. This is useful
   *   for example for band markers in the spectral plots.
   *
   * @param activeViewport The currently selected viewport
   */
  void PlotWindow::update(MdiCubeViewport *activeViewport) {
  }


  /**
   * Sets the plots given axis title to the given string.
   *
   *
   * @param axisId
   * @param title
   */
  void PlotWindow::setAxisLabel(int axisId, QString title) {
    m_plot->setAxisTitle(axisId, title);
  }


  /**
   * Sets the plot title to the given string. This does not update the window
   *   title.
   *
   * @param pt The plot title to use
   */
  void PlotWindow::setPlotTitle(QString pt) {
    m_plot->setTitle(pt);
  }


  /**
   * Allow or disallow users from manually putting curves into this plot window
   *   through either copy-and-paste or drag-and-drop.
   *
   * @param userHasControl True if users can add curves manually, false
   *                       otherwise
   */
  void PlotWindow::setUserCanAddCurves(bool userHasControl) {
    m_allowUserToAddCurves = userHasControl;
  }

  /**
   * Returns the plot title.
   *
   *
   * @return QwtText
   */
  QString PlotWindow::plotTitle() const {
    return m_plot->title().text();
  }


  /**
   * Ask if a user action can add this curve to this window in general. This
   *   verifies that the user is allowed to add curves to this window but not
   *   that a particular curve is compatible with this window.
   *
   * @return True if the user should be allowed to paste/drop curves in general
   *         into this window
   */
  bool PlotWindow::userCanAddCurves() const {
    return m_allowUserToAddCurves;
  }


  /**
   * This is the data-type of the curves' x data in this plot window. All of the
   *   cube plot curves must have the same units for x axis data or the display
   *   will not make sense.
   *
   * @return X-Axis Curve Data Units
   */
  PlotCurve::Units PlotWindow::xAxisUnits() const {
    return m_xAxisUnits;
  }


  /**
   * This is the data-type of the curves' y data in this plot window. All of the
   *   cube plot curves must have the same units for y-left axis data (y-right
   *   sometimes has different units) or the display will not make sense.
   *
   * @return Left Y-Axis Data Units
   */
  PlotCurve::Units PlotWindow::yAxisUnits() const {
    return m_yAxisUnits;
  }


  /**
   * Sets the plot background color to the given color.
   *
   *
   * @param c
   */
  void PlotWindow::setPlotBackground(QColor c) {
    m_plot->setCanvasBackground(c);
  }


  /**
   * This method tests whethere or not a CubePlotCurve can be successfully
   *   added to this window. Plot curves with mismatched X/Y data from the
   *   plot's x/y axis types can not be added to the window. This does not
   *   test whether or not a user is allowed to add the curve manually, just if
   *   the curve can be added programatically.
   *
   * @param curveToTest The plot curve to test for compatibility with this
   *                    window
   * @return True if the curve is compatible with this window, false otherwise
   */
  bool PlotWindow::canAdd(CubePlotCurve *curveToTest) const {
    return (curveToTest->xUnits() == m_xAxisUnits &&
            curveToTest->yUnits() == m_yAxisUnits);
  }


  /**
   * Returns the plot's background color.
   *
   *
   * @return QColor
   */
  QColor PlotWindow::plotBackgroundColor() const {
    return m_plot->canvasBackground().color();
  }


  /**
   * Get a comprehensive list of the plot curves inside of this plot window,
   *   excluding plot curves that are in the process of being removed.
   *
   * @return The plot curves contained inside of this plot window
   */
  QList<CubePlotCurve *> PlotWindow::plotCurves() {
    QList<CubePlotCurve *> foundCurves;

    const QwtPlotItemList &plotItems = m_plot->itemList();

    for (int itemIndex = 0; itemIndex < plotItems.size(); itemIndex++) {
      QwtPlotItem *item = plotItems[itemIndex];

      if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
        CubePlotCurve *curve = dynamic_cast<CubePlotCurve *>(item);

        if (curve && curve->color().alpha() != 0)
          foundCurves.append(curve);
      }
    }

    return foundCurves;
  }


  /**
   * Get a comprehensive const list of the plot curves inside of this plot
   *   window, excluding plot curves that are in the process of being removed.
   *
   * @return The const plot curves contained inside of this plot window
   */
  QList<const CubePlotCurve *> PlotWindow::plotCurves() const {
    QList<const CubePlotCurve *> foundCurves;

    const QwtPlotItemList &plotItems = m_plot->itemList();

    for (int itemIndex = 0; itemIndex < plotItems.size(); itemIndex++) {
      const QwtPlotItem *item = plotItems[itemIndex];

      if (item->rtti() == QwtPlotItem::Rtti_PlotCurve) {
        const CubePlotCurve *curve = dynamic_cast<const CubePlotCurve *>(item);

        if (curve)
          foundCurves.append(curve);
      }
    }

    return foundCurves;
  }


  /**
   * Get a comprehensive list of the scatter plots (spectrograms) inside of this
   *   plot window.
   *
   * @return The spectrograms (scatter plots) contained inside of this plot
   *         window
   */
  QList<QwtPlotSpectrogram *> PlotWindow::plotSpectrograms() {
    QList<QwtPlotSpectrogram *> foundSpectrograms;

    const QwtPlotItemList &plotItems = m_plot->itemList();

    for (int itemIndex = 0; itemIndex < plotItems.size(); itemIndex++) {
      QwtPlotItem *item = plotItems[itemIndex];

      if (item->rtti() == QwtPlotItem::Rtti_PlotSpectrogram) {
        QwtPlotSpectrogram *spectrogram =
            dynamic_cast<QwtPlotSpectrogram *>(item);

        if (spectrogram)
          foundSpectrograms.append(spectrogram);
      }
    }

    return foundSpectrograms;
  }


  /**
   * Get a comprehensive const list of the scatter plots (spectrograms) inside
   *   of this plot window.
   *
   * @return The const spectrograms (scatter plots) contained inside of this
   *         plot window
   */
  QList<const QwtPlotSpectrogram *> PlotWindow::plotSpectrograms() const {
    QList<const QwtPlotSpectrogram *> foundSpectrograms;

    const QwtPlotItemList &plotItems = m_plot->itemList();

    for (int itemIndex = 0; itemIndex < plotItems.size(); itemIndex++) {
      const QwtPlotItem *item = plotItems[itemIndex];

      if (item->rtti() == QwtPlotItem::Rtti_PlotSpectrogram) {
        const QwtPlotSpectrogram *spectrogram =
            dynamic_cast<const QwtPlotSpectrogram *>(item);

        if (spectrogram)
          foundSpectrograms.append(spectrogram);
      }
    }

    return foundSpectrograms;
  }


  /**
   * This method adds the curves to the plot.
   *
   *
   * @param pc
   */
  void PlotWindow::add(CubePlotCurve *pc) {
    if (!canAdd(pc)) {
      QMessageBox::warning(NULL, "Failed to add plot curve",
          "Can not add plot curves with x/y units that do not match the plot's "
          "x/y units");
    }
    else {
      QString curveTitle = pc->title().text();

      bool titleAccepted = false;
      int titleTryCount = 0;
      while (!titleAccepted) {
        if (titleTryCount > 0) {
          curveTitle = pc->title().text() + " (" +
              QString::number(titleTryCount + 1) + ")";
        }

        titleTryCount++;
        titleAccepted = true;

        const QwtPlotItemList &plotItems = m_plot->itemList();

        for (int itemIndex = 0; itemIndex < plotItems.size(); itemIndex ++) {
          QwtPlotItem *item = plotItems[itemIndex];

          if (item->title().text() == curveTitle)
            titleAccepted = false;
        }
      }

      pc->setTitle(curveTitle);
      pc->attach(m_plot);
      pc->attachMarkers();
      fillTable();

      updateVisibility(pc);

      connect(pc, SIGNAL(needsRepaint()),
              this, SIGNAL(plotChanged()));
      connect(pc, SIGNAL(destroyed(QObject *)),
              this, SLOT(resetScale()));

      // Get the legend widget for the recently attached plotcurve and give to the plotcurve
      QWidget *legendWidget = m_legend->legendWidget( plot()->itemToInfo(pc) );
      pc->updateLegendItemWidget(legendWidget);

      replot();
    }
  }


  /**
   * This method completely clears the plot of all plot items.
   * i.e. curves and markers, which also deletes the legend also
   * calls the necessary method to delete the table stuff
   */
  void PlotWindow::clearPlot() {
    clearPlotCurves();

    /*Table Stuff if table is open*/
    if (m_tableWindow != NULL && m_tableWindow->isVisible()) {
      m_tableWindow->table()->setColumnCount(1);
      m_tableWindow->table()->setRowCount(0);
//       deleteFromTable();
    }
  }


  /**
   * This method creates a CubePlotCurveConfigureDialog object. When there are no curves in this
   * PlotWindow, the dialog will not be created.
   */
  void PlotWindow::configurePlotCurves() {
    // make sure that there are CubePlotCurves to configure
    QList<CubePlotCurve *> curves = plotCurves();
    // can't configure 0 curves - menu item is deactivated
    if (curves.size() < 1) {
      return;
    }
    CubePlotCurve *curve = curves.first();
    CubePlotCurveConfigureDialog *configDialog = new CubePlotCurveConfigureDialog(curve, this);
    configDialog->exec();

    emit plotChanged();
  }


  /**
   * This method prompts the user to select the best fit line criterea. The
   *   PlotWindowBestFitDialog will create the best fit line automatically
   *   when the user asks for one, so we don't need to worry about doing
   *   any more than showing a dialog here.
   */
  void PlotWindow::createBestFitLine() {
    PlotWindowBestFitDialog *dialog = new PlotWindowBestFitDialog(this, plot());
    dialog->show();
  }


  /**
   * This method also clears the plot of all plot items, but does
   * not call the table delete stuff This method is called from
   * plotTool each time the changePlot() method is called.
   */
  void PlotWindow::clearPlotCurves() {
    const QwtPlotItemList &plotItems = m_plot->itemList();

    for (int itemIndex = plotItems.size()- 1; itemIndex >= 0; itemIndex --) {
      QwtPlotItem *item = plotItems[itemIndex];

      if (item->rtti() == QwtPlotItem::Rtti_PlotCurve ||
          item->rtti() == QwtPlotItem::Rtti_PlotHistogram) {
        delete item;
      }
    }

    replot();
  }


  /**
   * Enables the plot mouse tracker.
   *
   */
  void PlotWindow::trackerEnabled() {
    if (m_zoomer->trackerMode() == QwtPicker::ActiveOnly) {
      m_zoomer->setTrackerMode(QwtPicker::AlwaysOn);
    }
    else {
      m_zoomer->setTrackerMode(QwtPicker::ActiveOnly);
    }
  }


  /**
   * Provides printing support of the plot image.
   */
  void PlotWindow::printPlot() {
    QPixmap pixmap;
    /* Initialize a printer*/
    static QPrinter *printer = NULL;
    if (printer == NULL) printer = new QPrinter;
    QPageSize pageSize(QPageSize::Letter);
    printer->setPageSize(pageSize);
    printer->setColorMode(QPrinter::Color);

    QPrintDialog printDialog(printer, (QWidget *)parent());

    if (printDialog.exec() == QDialog::Accepted) {
      /* Get display widget as a pixmap and convert to an image*/
      pixmap = m_plot->grab();
      QImage img = pixmap.toImage();
      /* C++ Gui Programming with Qt, page 201*/
      QPainter painter(printer);
      QRect rect = painter.viewport();
      QSize size = img.size();
      size.scale(rect.size(), Qt::KeepAspectRatio);
      painter.setViewport(rect.x(), rect.y(),
                          size.width(), size.height());
      painter.setWindow(img.rect());
      painter.drawImage(0, 0, img);
    }

  }


  /**
   * This method allows the user to save the plot as a png,
   * jpg, or tif image file.
   */
  void PlotWindow::savePlot() {
    QPixmap pixmap;
    QString output =
    QFileDialog::getSaveFileName((QWidget *)parent(),
                                 "Choose output file",
                                 "./",
                                 QString("Images (*.png *.jpg *.tif)"));
    if (output.isEmpty()) return;
    //Make sure the filename is valid
    if (!output.isEmpty()) {
      if (!output.endsWith(".png") && !output.endsWith(".jpg") && !output.endsWith(".tif")) {
        output = output + ".png";
      }
    }

    QString format = QFileInfo(output).suffix();
    pixmap = m_plot->grab();

    std::string formatString = format.toStdString();
    if (!pixmap.save(output, formatString.c_str())) {
      QMessageBox::information((QWidget *)parent(), "Error", "Unable to save " + output);
      return;
    }
  }


  /**
   * This method toggles the plot background color between
   * black and white.
   */
  void PlotWindow::switchBackground() {
    QPen *pen = new QPen(Qt::white);

    if (m_plot->canvasBackground() == Qt::white) {
      m_plot->setCanvasBackground(Qt::black);
      m_grid->setMajorPen(QPen(Qt::white, 1, Qt::DotLine));
    }
    else {
      m_plot->setCanvasBackground(Qt::white);
      pen->setColor(Qt::black);
      m_grid->setMajorPen(QPen(Qt::black, 1, Qt::DotLine));
    }

    m_zoomer->setRubberBandPen(*pen);
    m_zoomer->setTrackerPen(*pen);
    pen->setWidth(2);
    /*Replot with the new background and pen colors*/
    m_plot->replot();
  }


  /**
   * Sets plot scale back to the defaults.
   *
   */
  void PlotWindow::resetScale() {
    m_zoomer->zoom(0);

    if (m_autoscaleAxes) {
      if (m_xAxisUnits != PlotCurve::Band) {
        m_plot->setAxisAutoScale(QwtPlot::xBottom);
      }
      else {
        QPair<double, double> calculatedXRange = findDataRange(
            QwtPlot::xBottom);
        m_plot->setAxisScale(QwtPlot::xBottom, calculatedXRange.first,
                             calculatedXRange.second);
      }

      if (m_yAxisUnits != PlotCurve::Band) {
        m_plot->setAxisAutoScale(QwtPlot::yLeft);
      }
      else {
        QPair<double, double> calculatedYRange = findDataRange(
            QwtPlot::yLeft);
        m_plot->setAxisScale(QwtPlot::yLeft, calculatedYRange.first,
                             calculatedYRange.second);
      }
    }

    m_zoomer->setZoomBase();
    m_plot->replot();
  }


  /**
   * This method sets the scale for the axis according to the
   * user specified numbers.
   */
  void PlotWindow::setUserValues() {
    if (m_xLogCheckBox->isChecked()) {
      m_plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLogScaleEngine);
      m_plotXLogScale = true;
    }
    else {
      m_plot->setAxisScaleEngine(QwtPlot::xBottom, new QwtLinearScaleEngine);
      m_plotXLogScale = false;
    }

    if (m_yLogCheckBox->isChecked()) {
      m_plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLogScaleEngine);
      m_plotYLogScale = true;
    }
    else {
      m_plot->setAxisScaleEngine(QwtPlot::yLeft, new QwtLinearScaleEngine);
      m_plotYLogScale = false;
    }

    m_autoscaleAxes = m_autoScaleCheckBox->isChecked();

    if (!m_autoscaleAxes) {
      double xMin = m_xMinEdit->text().toDouble();
      double xMax = m_xMaxEdit->text().toDouble();
  //  QwtScaleDiv xAxisScale =
  //      m_plot->axisScaleEngine(QwtPlot::xBottom)->divideScale(xMin, xMax,
  //                                                             25, 100);
  //  m_plot->setAxisScaleDiv(QwtPlot::xBottom, xAxisScale);
      m_plot->setAxisScale(QwtPlot::xBottom, xMin, xMax);

      double yMin = m_yMinEdit->text().toDouble();
      double yMax = m_yMaxEdit->text().toDouble();
  //  QwtScaleDiv yAxisScale =
  //      m_plot->axisScaleEngine(QwtPlot::yLeft)->divideScale(yMin, yMax,
  //                                                             25, 100);
  //  m_plot->setAxisScaleDiv(QwtPlot::yLeft, yAxisScale);
      m_plot->setAxisScale(QwtPlot::yLeft, yMin, yMax);

      m_zoomer->setZoomBase();
    }

    replot();
  }


  /**
   * Resets the x/y min/max to the defaults.
   *
   */
  void PlotWindow::setDefaultRange() {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Set Display Range");

    QGridLayout *dialogLayout = new QGridLayout;

    int row = 0;

    QLabel *autoLabel = new QLabel("Auto-Scale: ");
    dialogLayout->addWidget(autoLabel, row, 0);

    m_autoScaleCheckBox = new QCheckBox("Scale X/Y Axes Automatically");
    m_autoScaleCheckBox->setChecked(m_autoscaleAxes);
    connect(m_autoScaleCheckBox, SIGNAL(stateChanged(int)),
            this, SLOT(autoScaleCheckboxToggled()));
    dialogLayout->addWidget(m_autoScaleCheckBox, row, 1);
    row++;

    QLabel *xLabel = new QLabel("<h3>X-Axis</h3>");
    dialogLayout->addWidget(xLabel, row, 0, 1, 2);
    row++;

    QLabel *xMinLabel = new QLabel("Minimum: ");
    dialogLayout->addWidget(xMinLabel, row, 0);

    double xMin = plot()->axisScaleDiv(QwtPlot::xBottom).lowerBound();
    m_xMinEdit = new QLineEdit(QString::number(xMin));
    dialogLayout->addWidget(m_xMinEdit, row, 1);
    row++;

    QLabel *xMaxLabel = new QLabel("Maximum: ");
    dialogLayout->addWidget(xMaxLabel, row, 0);

    double xMax = plot()->axisScaleDiv(QwtPlot::xBottom).upperBound();
    m_xMaxEdit = new QLineEdit(QString::number(xMax));
    dialogLayout->addWidget(m_xMaxEdit, row, 1);
    row++;

    QLabel *xLogLabel = new QLabel("Logarithmic Scale");
    dialogLayout->addWidget(xLogLabel, row, 0);

    m_xLogCheckBox = new QCheckBox;
    m_xLogCheckBox->setChecked(m_plotXLogScale);
//    m_xLogCheckBox->setChecked(
//        m_plot->axisScaleEngine(QwtPlot::xBottom)->transformation()->type() ==
//        QwtScaleTransformation::Log10);
    dialogLayout->addWidget(m_xLogCheckBox, row, 1);
    row++;

    QLabel *yLabel = new QLabel("<h3>Y-Axis</h3>");
    dialogLayout->addWidget(yLabel, row, 0, 1, 2);
    row++;

    QLabel *yMinLabel = new QLabel("Minimum: ");
    dialogLayout->addWidget(yMinLabel, row, 0);

    double yMin = plot()->axisScaleDiv(QwtPlot::yLeft).lowerBound();
    m_yMinEdit = new QLineEdit(QString::number(yMin));
    dialogLayout->addWidget(m_yMinEdit, row, 1);
    row++;

    QLabel *yMaxLabel = new QLabel("Maximum: ");
    dialogLayout->addWidget(yMaxLabel, row, 0);

    double yMax = plot()->axisScaleDiv(QwtPlot::yLeft).upperBound();
    m_yMaxEdit = new QLineEdit(QString::number(yMax));
    dialogLayout->addWidget(m_yMaxEdit, row, 1);
    row++;

    QLabel *yLogLabel = new QLabel("Logarithmic Scale");
    dialogLayout->addWidget(yLogLabel, row, 0);

    m_yLogCheckBox = new QCheckBox;
    m_yLogCheckBox->setChecked(m_plotYLogScale);
//    m_yLogCheckBox->setChecked(
//        m_plot->axisScaleEngine(QwtPlot::yLeft)->transformation()->type() ==
//        QwtScaleTransformation::Log10);
    dialogLayout->addWidget(m_yLogCheckBox, row, 1);
    row++;

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();

    QPushButton *okButton = new QPushButton("&Ok");
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(okButton, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog, SIGNAL(accepted()), this, SLOT(setUserValues()));
    okButton->setShortcut(Qt::Key_Enter);
    buttonsLayout->addWidget(okButton);

    QPushButton *cancelButton = new QPushButton("&Cancel");
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()), dialog, SLOT(reject()));
    buttonsLayout->addWidget(cancelButton);

    QWidget *buttonsWrapper = new QWidget;
    buttonsWrapper->setLayout(buttonsLayout);
    dialogLayout->addWidget(buttonsWrapper, row, 0, 1, 2);
    row++;

    autoScaleCheckboxToggled();

    dialog->setLayout(dialogLayout);
    dialog->show();
  }


  /**
   * This method creates the dialog box which allows the user
   * to relabel the plot window
   */
  void PlotWindow::changePlotLabels() {
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Name Plot Labels");

    QGridLayout *dialogLayout = new QGridLayout;

    int row = 0;
    QLabel *plotLabel = new QLabel("Plot Title: ");
    dialogLayout->addWidget(plotLabel, row, 0);

    m_plotTitleText = new QLineEdit(plot()->title().text());
    dialogLayout->addWidget(m_plotTitleText, row, 1);
    row++;

    QLabel *xAxisLabel = new QLabel("X-Axis Label: ");
    dialogLayout->addWidget(xAxisLabel, row, 0);

    m_xAxisText = new QLineEdit(m_plot->axisTitle(QwtPlot::xBottom).text());
    dialogLayout->addWidget(m_xAxisText, row, 1);
    row++;

    QLabel *yAxisLabel = new QLabel("Y-Axis Label: ");
    dialogLayout->addWidget(yAxisLabel, row, 0);

    m_yAxisText = new QLineEdit(m_plot->axisTitle(QwtPlot::yLeft).text());
    dialogLayout->addWidget(m_yAxisText, row, 1);
    row++;

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();

    QPushButton *okButton = new QPushButton("&Ok");
    okButton->setIcon(QIcon::fromTheme("dialog-ok"));
    connect(okButton, SIGNAL(clicked()), dialog, SLOT(accept()));
    connect(dialog, SIGNAL(accepted()), this, SLOT(setLabels()));
    okButton->setShortcut(Qt::Key_Enter);
    buttonsLayout->addWidget(okButton);

    QPushButton *cancelButton = new QPushButton("&Cancel");
    cancelButton->setIcon(QIcon::fromTheme("dialog-cancel"));
    connect(cancelButton, SIGNAL(clicked()), dialog, SLOT(reject()));
    buttonsLayout->addWidget(cancelButton);

    QWidget *buttonsWrapper = new QWidget;
    buttonsWrapper->setLayout(buttonsLayout);
    dialogLayout->addWidget(buttonsWrapper, row, 0, 1, 2);
    row++;

    dialog->setLayout(dialogLayout);
    dialog->show();
  }


  /**
   * Makes the user specified changes to the plot labels.
   */
  void PlotWindow::setLabels() {
    m_plot->setTitle(m_plotTitleText->text());
    m_plot->setAxisTitle(QwtPlot::xBottom, m_xAxisText->text());
    m_plot->setAxisTitle(QwtPlot::yLeft, m_yAxisText->text());
    /*Replot with new labels.*/
    m_plot->replot();
  }


  /**
   * This method hides/shows the grid on the plotWindow and
   * changes the text for the action
   */
  void PlotWindow::showHideGrid() {
    m_grid->setVisible(!m_grid->isVisible());

    if (m_grid->isVisible()) {
      m_showHideGrid->setText("Hide Grid");
    }
    else {
      m_showHideGrid->setText("Show Grid");
    }
    m_plot->replot();
  }


  /**
   *Shows/Hides all the markers(symbols)
   */
  void PlotWindow::showHideAllMarkers() {
    void (QwtPlotItem::*method)();
    if (m_showHideAllMarkers->text() == "Hide All Symbols") {
      method = &QwtPlotItem::hide;

      m_showHideAllMarkers->setText("Show All Symbols");

    }
    else {
      method = &QwtPlotItem::show;

      m_showHideAllMarkers->setText("Hide All Symbols");
    }

    for (int i = 0; i < m_plot->itemList().size(); i ++) {
      QwtPlotItem *plotItem = m_plot->itemList()[i];
      if (plotItem->rtti() == QwtPlotItem::Rtti_PlotMarker)
        (plotItem->*method)();
    }
    /*Replot with all symbols hidden*/
    m_plot->replot();
  }


  /**
   * This method shows or hides all of the curves in the
   * plotWindow
   */
  void PlotWindow::showHideAllCurves() {
    void (QwtPlotItem::*method)();
    if (m_showHideAllCurves->text() == "Hide All Curves") {
      method = &QwtPlotItem::hide;

      m_showHideAllCurves->setText("Show All Curves");
      m_showHideAllCurves->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_showCurves.png").expanded())));

    }
    else {
      method = &QwtPlotItem::show;

      m_showHideAllCurves->setText("Hide All Curves");
      m_showHideAllCurves->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_hideCurves.png").expanded())));
    }

    for (int i = 0; i < m_plot->itemList().size(); i ++) {
      QwtPlotItem *plotItem = m_plot->itemList()[i];
      if (plotItem->rtti() == QwtPlotItem::Rtti_PlotCurve)
        (plotItem->*method)();
    }
    /*Replot with all curves hidden*/
    m_plot->replot();
  }


  /**
   * This method creates and shows the help dialog box for the
   * plot window.  this is called from the Help-->Basic Help menu.
   */
  void PlotWindow::showHelp() {
    QDialog *d = new QDialog(m_plot);
    d->setWindowTitle("Basic Help");

    QLabel *zoomLabel = new QLabel("<U>Zoom Options:</U>");
    QLabel *zoomIn = new
                     QLabel("  <b>Left click</b> on the mouse, drag, and release to select an area to zoom in on");
    QLabel *zoomOut = new
                      QLabel("  <b>Middle click</b> on the mouse to zoom out one level");
    QLabel *zoomReset = new
                        QLabel("  <b>Right click</b> on the mouse and select <I>Reset  Scale</I> to clear the zoom and return to the original plot");

    QLabel *curveConfigLabel = new QLabel("<br><U>Curve Configuration:</U>");
    QLabel *configDirections = new
                               QLabel("  <b>To configure the curve properties</b>  Right click on the legend and select <I>Configure</I> from <br>  the menu"
                                      " or click on the configure icon in the tool bar.");
    QLabel *config = new QLabel();
    config->setPixmap(QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_configure.png").expanded())));

    QLabel *tableLabel = new QLabel("<br><U>Table Options:</U>");
    QLabel *tableDirections = new
                              QLabel("  <b>To view the table</b> Click on the File menu and select <I>Show Table</I> or click on the table icon in the <br>   tool bar.");
    QLabel *table = new QLabel();
    table->setPixmap(QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_table.png").expanded())));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(zoomLabel);
    layout->addWidget(zoomIn);
    layout->addWidget(zoomOut);
    layout->addWidget(zoomReset);
    layout->addWidget(curveConfigLabel);
    layout->addWidget(config);
    layout->addWidget(configDirections);
    layout->addWidget(tableLabel);
    layout->addWidget(table);
    layout->addWidget(tableDirections);

    d->setLayout(layout);
    d->show();
  }


  /**
   * The user can add menu items from parent classes, but there are
   *   some menu items that are common between many types of plot windows.
   *
   * @param optionsToProvide A bit-flag containing information on which options
   *                         to provide to the users.
   */
  void PlotWindow::setupDefaultMenu(MenuOptions optionsToProvide) {
    QList<QMenu *> menu;
    QList<QAction *> actions;

    QMenu *fileMenu = new QMenu("&File");
    QMenu *editMenu = new QMenu("&Edit");
    QMenu *optionsMenu = new QMenu("&Options");
    QMenu *helpMenu = new QMenu("&Help");

    if ((optionsToProvide & SaveMenuOption) == SaveMenuOption) {
      QAction *save = new QAction(m_plot);
      save->setText("&Save Plot As");
      save->setIcon(QIcon::fromTheme("document-save-as"));
      QString text =
          "<b>Function:</b>  Save the plot as a png, jpg, or tif file.";
      save->setWhatsThis(text);
      connect(save, SIGNAL(triggered()), this, SLOT(savePlot()));
      fileMenu->addAction(save);
      actions.push_back(save);
    }

    if ((optionsToProvide & PrintMenuOption) == PrintMenuOption) {
      QAction *prt = new QAction(m_plot);
      prt->setText("&Print Plot");
      prt->setIcon(QIcon::fromTheme("document-print"));
      QString text =
          "<b>Function:</b>  Sends the plot image to the printer";
      prt->setWhatsThis(text);
      connect(prt, SIGNAL(triggered()), this, SLOT(printPlot()));
      fileMenu->addAction(prt);
      actions.push_back(prt);
    }

    if ((optionsToProvide & ShowTableMenuOption) == ShowTableMenuOption) {
      QAction *table = new QAction(m_plot);
      table->setText("Show Table");
      table->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_table.png").expanded())));
      QString text =
          "<b>Function:</b>  Activates the table which displays the data of the "
                            "current plot";
      table->setWhatsThis(text);
      connect(table, SIGNAL(triggered()), this, SLOT(showTable()));
      fileMenu->addAction(table);
      actions.push_back(table);
    }

    QAction *close = new QAction(QIcon::fromTheme("document-close"), "&Close",
                                 m_plot);
    connect(close, SIGNAL(triggered()), this, SLOT(close()));
    fileMenu->addAction(close);

    if ((optionsToProvide & TrackMenuOption) == TrackMenuOption) {
      QAction *track = new QAction(m_plot);
      track->setText("Show Mouse &Tracking");
      track->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/goto.png").expanded())));
      track->setCheckable(true);
      QString text =
          "<b>Function:</b>  Displays the x,y coordinates as the cursor moves "
          "around on the plot.";
      track->setWhatsThis(text);
      connect(track, SIGNAL(triggered()), this, SLOT(trackerEnabled()));
      optionsMenu->addAction(track);
    }

    if ((optionsToProvide & BackgroundSwitchMenuOption) ==
        BackgroundSwitchMenuOption) {
      QAction *backgrdSwitch = new QAction(m_plot);
      backgrdSwitch->setText("White/Black &Background");
      backgrdSwitch->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_switchBackgrd.png").expanded())));
      QString text =
          "<b>Function:</b>  Switch the background color between black and "
                            "white.";
      backgrdSwitch->setWhatsThis(text);
      connect(backgrdSwitch, SIGNAL(triggered()),
              this, SLOT(switchBackground()));
      optionsMenu->addAction(backgrdSwitch);
      actions.push_back(backgrdSwitch);
    }

    if ((optionsToProvide & ShowHideGridMenuOption) == ShowHideGridMenuOption) {
      m_showHideGrid = new QAction(m_plot);
      m_showHideGrid->setText("Show Grid");
      m_showHideGrid->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_grid.png").expanded())));
      QString text =
          "<b>Function:</b>  Display grid lines on the plot.";
      m_showHideGrid->setWhatsThis(text);
      connect(m_showHideGrid, SIGNAL(triggered()), this, SLOT(showHideGrid()));
      optionsMenu->addAction(m_showHideGrid);
      actions.push_back(m_showHideGrid);
    }

    if ((optionsToProvide & RenameLabelsMenuOption) == RenameLabelsMenuOption) {
      QAction *changeLabels = new QAction(m_plot);
      changeLabels->setText("Rename Plot &Labels");
      changeLabels->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_renameLabels.png").expanded())));
      QString text =
          "<b>Function:</b>  Edit the plot title, x and y axis labels.";
      changeLabels->setWhatsThis(text);
      connect(changeLabels, SIGNAL(triggered()),
              this, SLOT(changePlotLabels()));
      optionsMenu->addAction(changeLabels);
      actions.push_back(changeLabels);
    }

    if ((optionsToProvide & SetDisplayRangeMenuOption) ==
        SetDisplayRangeMenuOption) {
      QAction *changeScale = new QAction(m_plot);
      changeScale->setText("Set &Display Range");
      changeScale->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_setScale.png").expanded())));
      QString text =
          "<b>Function:</b>  Adjust the scale for the x and y axis on the "
          "plot.";
      changeScale->setWhatsThis(text);
      connect(changeScale, SIGNAL(triggered()), this, SLOT(setDefaultRange()));
      optionsMenu->addAction(changeScale);
      actions.push_back(changeScale);
    }

    if ((optionsToProvide & ShowHideCurvesMenuOption) ==
        ShowHideCurvesMenuOption) {
      m_showHideAllCurves = new QAction(m_plot);
      m_showHideAllCurves->setText("Hide All Curves");
      m_showHideAllCurves->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_showCurves.png").expanded())));
      QString text =
          "<b>Function:</b>  Displays or hides all the curves currently "
                            "displayed on the plot.";
      m_showHideAllCurves->setWhatsThis(text);
      connect(m_showHideAllCurves, SIGNAL(triggered()),
              this, SLOT(showHideAllCurves()));
      optionsMenu->addAction(m_showHideAllCurves);
      actions.push_back(m_showHideAllCurves);
    }

    if ((optionsToProvide & ShowHideMarkersMenuOption) ==
        ShowHideMarkersMenuOption) {
      m_showHideAllMarkers = new QAction(m_plot);
      m_showHideAllMarkers->setText("Hide All Symbols");
      m_showHideAllMarkers->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_markers.png").expanded())));
      QString text = "<b>Function:</b>  Displays or hides a symbol for each "
                     "data point plotted on a plot.";
      m_showHideAllMarkers->setWhatsThis(text);
      connect(m_showHideAllMarkers, SIGNAL(triggered()),
              this, SLOT(showHideAllMarkers()));
      optionsMenu->addAction(m_showHideAllMarkers);
      actions.push_back(m_showHideAllMarkers);
    }

    if ((optionsToProvide & ResetScaleMenuOption) == ResetScaleMenuOption) {
      QAction *resetScaleButton = new QAction(m_plot);
      resetScaleButton->setText("Reset Scale");
      resetScaleButton->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_resetscale.png").expanded())));
      QString text =
          "<b>Function:</b>  Reset the plot's scale.";
      resetScaleButton->setWhatsThis(text);
      connect(resetScaleButton, SIGNAL(triggered()), this, SLOT(resetScale()));
      actions.push_back(resetScaleButton);
    }

    if ((optionsToProvide & ClearPlotMenuOption) == ClearPlotMenuOption) {
      QAction *clear = new QAction(m_plot);
      clear->setText("Clear Plot");
      clear->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_clear.png").expanded())));
      QString text =
          "<b>Function:</b>  Removes all the curves from the plot.";
      clear->setWhatsThis(text);
      connect(clear, SIGNAL(triggered()), this, SLOT(clearPlot()));
      actions.push_back(clear);
    }

    if ((optionsToProvide & LineFitMenuOption) == LineFitMenuOption) {
      QAction *lineFit = new QAction(m_plot);
      lineFit->setText("Create Best Fit Line");
      lineFit->setIcon(
          QPixmap(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/linefit.png").expanded())));
      QString text = "<b>Function:</b>  Calculates a best fit line from an "
                     "existing curve.";
      lineFit->setWhatsThis(text);
      connect(lineFit, SIGNAL(triggered()), this, SLOT( createBestFitLine() ) );
      optionsMenu->addAction(lineFit);
      actions.push_back(lineFit);
    }

    if ((optionsToProvide & ConfigurePlotMenuOption) == ConfigurePlotMenuOption) {
      QAction *configurePlot = new QAction(m_plot);
      configurePlot->setText("Configure Plot");
      configurePlot->setIcon(
          QPixmap( QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/plot_configure.png").expanded() )) );
      QString text = "<b>Function:</b> Change the name, color, style, and vertex symbol of the "
                     "curves.";
      configurePlot->setWhatsThis(text);
      connect( configurePlot, SIGNAL( triggered() ),
               this, SLOT( configurePlotCurves() ) );
      optionsMenu->addAction(configurePlot);
      actions.push_back(configurePlot);
    }

    QAction *basicHelp = new QAction(m_plot);
    basicHelp->setText("Basic Help");
    QString text = "<b>Function:</b> Provides a basic overview on using components "
                   "of the qview plot window";
    basicHelp->setWhatsThis(text);
    connect( basicHelp, SIGNAL( triggered() ),
             this, SLOT( showHelp() ) );
    helpMenu->addAction(basicHelp);

    /*setup menus*/
    m_pasteAct = new QAction(QIcon::fromTheme("edit-paste"),
                             "&Paste Curve", m_plot);
    m_pasteAct->setEnabled(false);
    m_pasteAct->setShortcut(Qt::CTRL | Qt::Key_V);
    connect(m_pasteAct, SIGNAL(triggered()),
            this, SLOT(pasteCurve()));
    editMenu->addAction(m_pasteAct);

    menu.push_back(fileMenu);
    menu.push_back(editMenu);

    if (optionsMenu->actions().size()) {
      menu.push_back(optionsMenu);
    }
    else {
      delete optionsMenu;
      optionsMenu = NULL;
    }

   if (helpMenu->actions().size()) {
      menu.push_back(helpMenu);
    }
    else {
      delete helpMenu;
      helpMenu = NULL;
    }

    setMenus(menu, actions);
  }


  /**
   * Ask if a user action can add this curve to this window. This verifies that
   *   the user is allowed to add curves to this window and that
   *   programmatically the curve can be added (i.e. things such as the X/Y
   *   data units match).
   *
   * @param curve Mime-data containing a plot curve to be added
   *
   * @return True if the user should be allowed to paste/drop the curve into
   *         this window
   */
  bool PlotWindow::userCanAddCurve(const QMimeData *curve) {
    bool userCanAdd = false;

    if (m_allowUserToAddCurves &&
        curve->hasFormat("application/isis3-plot-curve")) {

      CubePlotCurve * testCurve = new CubePlotCurve(
          curve->data("application/isis3-plot-curve"));

      userCanAdd = canAdd(testCurve);
    }

    return userCanAdd;
  }


  /**
   * This method sets the visibility states in the curve (and it's symbols) to
   *   match with this window's current visibility settings. Every means of
   *   adding a curve to the window needs to call this with the curve that is
   *   being added, otherwise visibility states will not be consistent.
   *
   * @param curve Curve (with symbols) to set the visibility states on
   */
  void PlotWindow::updateVisibility(PlotCurve *curve) {
    if (m_showHideAllCurves) {
      if (m_showHideAllCurves->text() == "Hide All Curves") {
        curve->show();
      }
      else {
        curve->hide();
      }

      if (m_showHideAllMarkers->text() == "Hide All Symbols") {
        curve->setMarkerVisible(true);
      }
      else {
        curve->setMarkerVisible(false);
      }
    }

    emit plotChanged();
  }


  /**
   * Sets up the menus added from a parent object.
   *
   *
   * @param menu
   * @param actions
   */
  void PlotWindow::setMenus(QList<QMenu *> menu, QList<QAction *> actions) {
    if (m_toolBar == NULL) {
      m_toolBar = new QToolBar(this);
      m_toolBar->setObjectName("PlotWindow");
      m_toolBar->setAllowedAreas(Qt::LeftToolBarArea | Qt::RightToolBarArea | Qt::TopToolBarArea);
      addToolBar(Qt::TopToolBarArea, m_toolBar);
    }
    else {
      m_toolBar->clear();
    }

    m_menubar = menuBar();
    m_menubar->clear();

    for (int i = 0; i < menu.size(); i++) {
      m_menubar->addMenu(menu[i]);
    }

    for (int i = 0; i < actions.size(); i++) {
      m_toolBar->addAction(actions[i]);
    }

  }


  /**
   * Get this window's plot's zoomer.
   *
   * @return A QwtPlotZoomer which is associated with this PlotWindow's QwtPlot
   */
  QwtPlotZoomer *PlotWindow::zoomer() {
    return m_zoomer;
  }


  /**
   * Fills in the table with the data from the current curves
   *   in the plotWindow once all current actions/activations are done. This is provided for
   *   performance reasons - re-plotting 4 curves only requires one fillTable at the end, instead
   *   of after each curve change.
   */
  void PlotWindow::scheduleFillTable() {
    if (!m_scheduledFillTable) {
      m_scheduledFillTable = true;
      emit requestFillTable();
    }
  }


  /**
   * Fills in the table with the data from the current curves
   * in the plotWindow immediately.
   */
  void PlotWindow::fillTable() {
    m_scheduledFillTable = false;

    if (m_tableWindow == NULL) return;
    m_tableWindow->listWidget()->clear();
    m_tableWindow->table()->clear();
    m_tableWindow->table()->setRowCount(0);
    m_tableWindow->table()->setColumnCount(0);

    m_tableWindow->addToTable(true,
                              m_plot->axisTitle(QwtPlot::xBottom).text(),
                              m_plot->axisTitle(QwtPlot::xBottom).text());

    QList<CubePlotCurve *> curves = plotCurves();
    foreach (CubePlotCurve *curve, curves) {
      m_tableWindow->addToTable(true,
                                curve->title().text(),
                                curve->title().text());
    }

    // We really need all of the x-values associated with the curves,
    //   but qwt doesn't seem to want to give this to us. It'll give us the
    //   axis scale, but that isn't quite what we want (especially when zooming)
    //   So let's find the list of x-points ourselves.
    //
    // This is what I tried and it did NOT work:
    // QwtScaleDiv *xAxisScaleDiv = m_plot->axisScaleDiv(QwtPlot::xBottom);
    // QList<double> xAxisPoints = xAxisScaleDiv->ticks(QwtScaleDiv::MajorTick);
    //
    // We're going to keep xAxisPoints in standard text sort order until we're done populating it,
    //   then we'll re-sort numerically. That enables us to effectively use binary searches and
    //   insertion sort-like capabilities for speed.
    QList<QString> xAxisPoints;

    QProgressDialog progress(tr("Re-calculating Table"), tr(""), 0, 1000, this);
    double percentPerCurve = 0.5 * 1.0 / curves.count();

    for (int curveIndex = 0; curveIndex < curves.count(); curveIndex++) {
      progress.setValue(qRound(curveIndex * percentPerCurve * 1000.0));

      CubePlotCurve *curve = curves[curveIndex];

      double percentPerDataIndex = (1.0 / curve->data()->size()) * percentPerCurve;

      // Loop backwards because our insertion sort will have a much better
      //   chance of success on it's first try this way.
      for (int dataIndex = (int)curve->data()->size() - 1;
            dataIndex >= 0;
            dataIndex--) {
        double xValue = curve->data()->sample(dataIndex).x();
        QString xValueString = toString(xValue);

        int inverseDataIndex = (curve->data()->size() - 1) - dataIndex;
        progress.setValue(
            qRound( ((curveIndex * percentPerCurve) +
                     (inverseDataIndex * percentPerDataIndex)) * 1000.0));


        QList<QString>::const_iterator foundPos =
            std::lower_bound(xAxisPoints.begin(), xAxisPoints.end(), xValueString);

        if (foundPos == xAxisPoints.end()) {
          bool inserted = false;

          for (int searchIndex = 0;
             searchIndex < xAxisPoints.size() && !inserted;
             searchIndex++) {
            if (xAxisPoints[searchIndex] > xValueString) {
              inserted = true;
              xAxisPoints.insert(searchIndex, xValueString);
            }
          }

          if (!inserted)
            xAxisPoints.append(xValueString);
        }
      }
    }

    sort(xAxisPoints.begin(), xAxisPoints.end(), &numericStringLessThan);

    m_tableWindow->table()->setRowCount(xAxisPoints.size());

    QList<int> lastSuccessfulSamples;

    for (int i = 0; i < curves.count(); i++) {
      lastSuccessfulSamples.append(-1);
    }

    double progressPerRow = 0.5 * 1.0 / m_tableWindow->table()->rowCount();

    for (int row = 0; row < m_tableWindow->table()->rowCount(); row++) {
      progress.setValue(500 + qRound(row * progressPerRow * 1000.0));

      QString xValueString = xAxisPoints[row];
      double xValue = toDouble(xValueString);

      QTableWidgetItem *xAxisItem = new QTableWidgetItem(xValueString);
      m_tableWindow->table()->setItem(row, 0, xAxisItem);

      if (row == m_tableWindow->table()->rowCount() - 1) {
        m_tableWindow->table()->resizeColumnToContents(0);
      }

      // Now search for the x-axis points in the curves to fill in data
      for (int col = 1; col < m_tableWindow->table()->columnCount(); col++) {
        CubePlotCurve *curve = curves[col - 1];

        double y = Null;
        bool tooFar = false;

        for (int dataIndex = lastSuccessfulSamples[col - 1] + 1;
             dataIndex < (int)curve->data()->size() && y == Null && !tooFar;
             dataIndex++) {

          if (toString(curve->data()->sample(dataIndex).x()) == xValueString) {
            // Try to compensate for decreasing x values by not performing this optimization
            if (dataIndex > 0 &&
                curve->data()->sample(dataIndex - 1).x() < curve->data()->sample(dataIndex).x()) {
              lastSuccessfulSamples[col - 1] = dataIndex;
            }
            y = curve->data()->sample(dataIndex).y();
          }
          // Try to compensate for decreasing X values in the too far computation
          else if (dataIndex > 0 &&
              curve->data()->sample(dataIndex - 1).x() < curve->data()->sample(dataIndex).x() &&
              curve->data()->sample(dataIndex).x() > xValue) {
            tooFar = true;
          }
        }

        QTableWidgetItem *item = NULL;

        if (IsSpecial(y))
          item = new QTableWidgetItem(QString("N/A"));
        else
          item = new QTableWidgetItem(toString(y));

        m_tableWindow->table()->setItem(row, col, item);

        if (row == m_tableWindow->table()->rowCount() - 1) {
          m_tableWindow->table()->resizeColumnToContents(col);
        }
      }
    }
  }


  /**
   * This method is called from the showTable action on the tool
   * bar There are some checks done to make sure there are data to
   * fill the table
   */
  void PlotWindow::showTable() {
    if (plotCurves().size()) {
      if (m_tableWindow == NULL) {
        //m_tableWindow = new TableMainWindow("Plot Table", this);
        m_tableWindow = new TableMainWindow("Plot Table", m_parent);
        m_tableWindow->setTrackListItems(false);
      }

      fillTable();
      m_tableWindow->show();
      m_tableWindow->syncColumns();
    }
  }


  /**
   * This method filters the events of the objects it is connected
   * to.  In this case, the eventFilter has been installed on the m_plot and
   * m_legend.
   * @param o
   * @param e
   *
   * @return bool
   */
  bool PlotWindow::eventFilter(QObject *o, QEvent *e) {
    bool blockWidgetFromEvent = false;

    switch (e->type()) {
      case QEvent::MouseButtonPress:
        if (o == this &&
            childAt(((QMouseEvent *)e)->pos()) != plot()->canvas()) {
          mousePressEvent(o, (QMouseEvent *)e);
          blockWidgetFromEvent = true;
        }
        break;

      default:
        break;
    }

    bool stopHandlingEvent = false;
    if (!blockWidgetFromEvent && o == this) {
      stopHandlingEvent = MainWindow::eventFilter(o, e);

      if (e->type() == QEvent::Close && !stopHandlingEvent) {
        emit closed();
      }
    }

    return stopHandlingEvent || blockWidgetFromEvent;
  }


  /**
   * This is a helper method for the eventFilter() method. When a mouse press
   *   event is seen, this handles it. For example, if the user right clicks
   *   and the clipboard contains compatible data then this will give a paste
   *   option.
   *
   * @param object The object which was pressed
   * @param event The mouse event which contains button information
   */
  void PlotWindow::mousePressEvent(QObject *object, QMouseEvent *event) {
    if (qobject_cast<QWidget *>(object) &&
        event->button() == Qt::RightButton &&
        userCanAddCurve(QApplication::clipboard()->mimeData())) {
      QMenu contextMenu;

      QAction *pasteAct = new QAction(QIcon::fromTheme("edit-paste"), "Paste",
                                      this);
      contextMenu.addAction(pasteAct);

      QAction *chosenAct = contextMenu.exec(
          qobject_cast<QWidget *>(object)->mapToGlobal(event->pos()));

      if (chosenAct == pasteAct) {
        pasteCurve();
      }
    }
  }


  /**
   * Get the plot encapsulated by this PlotWindow.
   *
   * @return The QwtPlot that this window encapsulates.
   */
  QwtPlot *PlotWindow::plot() {
    return m_plot;
  }


  /**
   * This turns off scaling the x/y axes automatically. Use this if you have
   *   a very specific axis range you want to use, but keep in mind that
   *   users (potentially) have an option to re-enable axis auto scaling if
   *   they want to.
   */
  void PlotWindow::disableAxisAutoScale() {
    m_autoscaleAxes = false;
  }


  /**
   * This is a helper method for the set scale configuration dialog. This
   *   enables or disables options inside of the dialog when a checkbox is
   *   clicked in the dialog. This does not change the state of the plot or
   *   plot zoomer.
   */
  void PlotWindow::autoScaleCheckboxToggled() {
    m_xMinEdit->setEnabled(!m_autoScaleCheckBox->isChecked());
    m_xMaxEdit->setEnabled(!m_autoScaleCheckBox->isChecked());
    m_yMinEdit->setEnabled(!m_autoScaleCheckBox->isChecked());
    m_yMaxEdit->setEnabled(!m_autoScaleCheckBox->isChecked());
  }


  /**
   * This slot will be called when the system clipboard is changed.
   */
  void PlotWindow::onClipboardChanged() {
    m_pasteAct->setEnabled(
        userCanAddCurve(QApplication::clipboard()->mimeData()));
  }


  /**
   * When the user pastes a curve try to put it into this plot window. This
   *   shouldn't be called when the curve on the system clipboard isn't
   *   compatible.
   */
  void PlotWindow::pasteCurve() {
    if (m_allowUserToAddCurves) {
      QClipboard *globalClipboard = QApplication::clipboard();
      const QMimeData *globalData = globalClipboard->mimeData();

      if (globalData->hasFormat("application/isis3-plot-curve")) {
        CubePlotCurve * newCurve = new CubePlotCurve(
                globalData->data("application/isis3-plot-curve"));
        // add curve to plot
        add(newCurve);
        emit plotChanged();
      }
    }
  }


  /**
   * This calculates the data range of the specified axis (works with xBottom
   *   and yLeft only). This is used to provide unit context (band #'s shouldn't
   *   ever pad) and to fix the lack of an axis when only one value exists (if
   *   there is only one x-value, this will always pad with +/- 0.5).
   *
   * @param axisId This must be yLeft or xBottom
   * @return The double range of the data contained by the given axis.
   */
  QPair<double, double> PlotWindow::findDataRange(int axisId) const {
    QList<const CubePlotCurve *> curves = plotCurves();

    bool foundDataValue = false;
    QPair<double, double> rangeMinMax;

    foreach(const CubePlotCurve *curve, curves) {
      for (int dataIndex = 0; dataIndex < (int)curve->dataSize(); dataIndex++) {
        if (axisId == QwtPlot::xBottom) {
          if (!foundDataValue) {
            rangeMinMax.first = curve->sample(dataIndex).x();
            rangeMinMax.second = curve->sample(dataIndex).x();
            foundDataValue = true;
          }
          else {
            rangeMinMax.first = qMin(rangeMinMax.first, curve->sample(dataIndex).x());
            rangeMinMax.second = qMax(rangeMinMax.second, curve->sample(dataIndex).x());
          }
        }
        else if (axisId == QwtPlot::yLeft) {
          if (!foundDataValue) {
            rangeMinMax.first = curve->sample(dataIndex).y();
            rangeMinMax.second = curve->sample(dataIndex).y();
            foundDataValue = true;
          }
          else {
            rangeMinMax.first = qMin(rangeMinMax.first, curve->sample(dataIndex).y());
            rangeMinMax.second = qMax(rangeMinMax.second, curve->sample(dataIndex).y());
          }
        }
      }
    }

    if (!foundDataValue) {
      rangeMinMax.first = 1;
      rangeMinMax.second = 10;
    }
    else if(rangeMinMax.first == rangeMinMax.second) {
      rangeMinMax.first -= 0.5;
      rangeMinMax.second += 0.5;
    }

    return rangeMinMax;
  }


  bool PlotWindow::numericStringLessThan(QString left, QString right) {
    bool result = false;

    try {
      result = toDouble(left) < toDouble(right);
    }
    catch (IException &) {
    }

    return result;
  }


  /**
   * Paint plot curve information onto the viewport.
   *
   * @param vp The cube viewport to paint onto
   * @param painter The painter to use for painting
   */
  void PlotWindow::paint(MdiCubeViewport *vp, QPainter *painter) {
    foreach (CubePlotCurve *curve, plotCurves()) {
      curve->paint(vp, painter);
    }
  }


  /**
   * Reset the scale of the plot, replot it and emit plot changed.
   */
  void PlotWindow::replot() {
    resetScale();
    emit plotChanged();
  }


  /**
   * This is the typical suffix for plot windows, it's here in case we want to
   *   update all plot windows to have a different ending than just 'Plot' on
   *   them (for example, 'Plot Window').
   *
   * @return A string to be appended to your window title
   */
  QString PlotWindow::defaultWindowTitle() {
    return "Plot";
  }


  /**
   * When a user drags data into our plot window, we need to indicate whether or
   *   not this data is compatible with this window. This checks
   *   userCanAddCurve() on the drag & drop data and allows or disallowed the
   *   event accordingly.
   *
   * @param event The drag event to test
   */
  void PlotWindow::dragEnterEvent(QDragEnterEvent *event) {
    QObject *source = event->source();

    if (source != m_legend->contentsWidget() &&
        userCanAddCurve(event->mimeData())) {
      event->acceptProposedAction();
    }
  }


  /**
   * This is called when a user drops data into our window. The dragEnterEvent()
   *   must have said that this curve is compatible with this window. This
   *   provides all of the available options to the user (if any) set by the
   *   creator of the drag & drop event (i.e. whether or not we can move or
   *   just copy). If there are options, we create a context menu - otherwise
   *   the default action is taken.
   *
   * @param event The drop event containing the curve to put into the current
   *              window.
   */
  void PlotWindow::dropEvent(QDropEvent *event) {
    if (m_allowUserToAddCurves &&
        event->mimeData()->hasFormat("application/isis3-plot-curve")) {
      Qt::DropActions possibleActions = event->possibleActions();
      Qt::DropAction actionToTake = event->proposedAction();

      QFont boldFont;
      boldFont.setBold(true);
      QMenu dropActionsMenu;

      QAction *copyAct = new QAction("&Copy Here", this);
      if (possibleActions.testFlag(Qt::CopyAction)) {
        dropActionsMenu.addAction(copyAct);

        if (actionToTake == Qt::CopyAction)
          copyAct->setFont(boldFont);
      }

      QAction *moveAct = new QAction("&Move Here", this);
      if (possibleActions.testFlag(Qt::MoveAction)) {
        dropActionsMenu.addAction(moveAct);

        if (actionToTake == Qt::MoveAction)
          moveAct->setFont(boldFont);
      }

      if (dropActionsMenu.actions().size() > 1) {
        dropActionsMenu.addSeparator();

        QAction *cancelAct = new QAction("&Cancel", this);
        dropActionsMenu.addAction(cancelAct);

        QAction *chosenAct = dropActionsMenu.exec(mapToGlobal(event->pos()));

        if (chosenAct == copyAct) {
          actionToTake = Qt::CopyAction;
        }
        else if (chosenAct == moveAct) {
          actionToTake = Qt::MoveAction;
        }
        else {
          actionToTake = Qt::IgnoreAction;
        }
      }

      if (actionToTake != Qt::IgnoreAction) {
        CubePlotCurve * newCurve = new CubePlotCurve(
                event->mimeData()->data("application/isis3-plot-curve"));
        // add curve to plot
        add(newCurve);
        emit plotChanged();

        event->setDropAction(actionToTake);
        event->accept();
      }
    }
  }

}