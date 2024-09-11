#include "StatisticsTool.h"

#include <QAction>
#include <QCheckBox>
#include <QDebug>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QRadioButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QSlider>
#include <QToolButton>

#include "Brick.h"
#include "MdiCubeViewport.h"
#include "ToolPad.h"

namespace Isis {

  /**
   * Constructor, creates and sets up widgets for this tool.
   *
   * @param parent
   */
  StatisticsTool::StatisticsTool(QWidget *parent) : Tool(parent) {
    p_boxSamps = 3;
    p_boxLines = 3;

    p_ulSamp = -1;
    p_ulLine = -1;

    p_set = false;

    p_dialog = new QDialog(parent);
    p_dialog->setWindowTitle("Statistics");

    p_visualBox = new QGroupBox("Visual Display");

    p_visualScroll = new QScrollArea;
    p_visualScroll->setBackgroundRole(QPalette::Dark);
    
    p_visualDisplay = new VisualDisplay(p_visualScroll);
    p_visualDisplay->setObjectName("dnDisplay");

    QCheckBox *checkBox = new QCheckBox("Hide Display");
    connect(checkBox, SIGNAL(toggled(bool)), this, SLOT(hideDisplay(bool)));

    QLabel *boxLabel = new QLabel("Box Size:");
    p_boxLabel = new QLabel;
    QString samps, lines;
    samps.setNum(p_boxSamps);
    lines.setNum(p_boxLines);
    p_boxLabel->setText(samps + "x" + lines);

    QHBoxLayout *boxLabelLayout = new QHBoxLayout;

    boxLabelLayout->addWidget(checkBox);
    boxLabelLayout->addStretch(1);
    boxLabelLayout->addWidget(boxLabel);
    boxLabelLayout->addWidget(p_boxLabel);

    QSlider *slider = new QSlider(Qt::Vertical);
    slider->setRange(2, 18);
    slider->setSliderPosition(10);
    slider->setSingleStep(1);
    slider->setTickInterval(1);
    slider->setTickPosition(QSlider::TicksBelow);
    connect(slider, SIGNAL(valueChanged(int)), p_visualDisplay, SLOT(setBoxSize(int)));
    connect(slider, SIGNAL(valueChanged(int)), this, SLOT(resizeScrollbars()));
    p_visualScroll->setWidget(p_visualDisplay);

    QGroupBox *displayMode = new QGroupBox("Display Mode");
    QRadioButton *displayText = new QRadioButton("Show Text");
    displayText->setToolTip("Display the pixels of a region as text");
    QRadioButton *displayPixels = new QRadioButton("Show Pixel Values");
    displayPixels->setToolTip("Display the pixels of a region");
    QRadioButton *displayDeviation = new QRadioButton("Show Deviation");
    displayDeviation->setToolTip("Display standard deviation over a region,\n where red denotes a larger deviation");

    QHBoxLayout *displayModeLayout = new QHBoxLayout;
    displayModeLayout->addWidget(displayText);
    displayModeLayout->addWidget(displayPixels);
    displayModeLayout->addWidget(displayDeviation);

    displayMode->setLayout(displayModeLayout);

    connect(displayText, SIGNAL(toggled(bool)), p_visualDisplay, SLOT(showText(bool)));
    connect(displayText, SIGNAL(toggled(bool)), slider, SLOT(setDisabled(bool)));
    connect(displayPixels, SIGNAL(toggled(bool)), p_visualDisplay, SLOT(showPixels(bool)));
    connect(displayDeviation, SIGNAL(toggled(bool)), p_visualDisplay, SLOT(showDeviation(bool)));

    displayText->setChecked(true);

    QHBoxLayout *visualHBoxLayout = new QHBoxLayout;
    visualHBoxLayout->addWidget(p_visualScroll);
    visualHBoxLayout->addWidget(slider);

    QVBoxLayout *visualVBoxLayout = new QVBoxLayout;
    visualVBoxLayout->addLayout(visualHBoxLayout);
    visualVBoxLayout->addWidget(displayMode);

    p_visualBox->setLayout(visualVBoxLayout);

    QGroupBox *statsBox = new QGroupBox("Statistics");

    p_minLabel = new QLabel("Minimum: n/a");
    p_minLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    p_minLabel->setLineWidth(1);
    p_minLabel->setMargin(10);
    p_minLabel->setAlignment(Qt::AlignLeft);

    p_maxLabel = new QLabel("Maximum: n/a");
    p_maxLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    p_maxLabel->setLineWidth(1);
    p_maxLabel->setMargin(10);
    p_maxLabel->setAlignment(Qt::AlignLeft);

    p_avgLabel = new QLabel("Average: n/a");
    p_avgLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    p_avgLabel->setLineWidth(1);
    p_avgLabel->setMargin(10);
    p_avgLabel->setAlignment(Qt::AlignLeft);

    p_stdevLabel = new QLabel("Standard Dev: n/a");
    p_stdevLabel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    p_stdevLabel->setLineWidth(1);
    p_stdevLabel->setMargin(10);
    p_stdevLabel->setAlignment(Qt::AlignLeft);

    p_dnLabel = new QLabel("DN: n/a");
    QFont labelFont = p_dnLabel->font();
    labelFont.setPointSize(8);
    p_dnLabel->setFont(labelFont);
    p_dnLabel->setAlignment(Qt::AlignRight);
    connect(p_visualDisplay, SIGNAL(setDn(QString)), p_dnLabel, SLOT(setText(QString)));

    p_sampLabel = new QLabel("Sample: n/a");
    p_sampLabel->setFont(labelFont);
    p_sampLabel->setAlignment(Qt::AlignLeft);
    connect(p_visualDisplay, SIGNAL(setSample(QString)), p_sampLabel, SLOT(setText(QString)));

    p_lineLabel = new QLabel("Line: n/a");
    p_lineLabel->setFont(labelFont);
    p_lineLabel->setAlignment(Qt::AlignCenter);
    connect(p_visualDisplay, SIGNAL(setLine(QString)), p_lineLabel, SLOT(setText(QString)));

    QGridLayout *statsLayout = new QGridLayout;
    statsLayout->addWidget(p_minLabel, 0, 0, 1, 2);
    statsLayout->addWidget(p_maxLabel, 1, 0, 1, 2);
    statsLayout->addWidget(p_avgLabel, 0, 2, 1, 2);
    statsLayout->addWidget(p_stdevLabel, 1, 2, 1, 2);
    statsLayout->addWidget(p_sampLabel, 2, 0);
    statsLayout->addWidget(p_lineLabel, 2, 1, 1, 2);
    statsLayout->addWidget(p_dnLabel, 2, 3);

    statsBox->setLayout(statsLayout);

    QVBoxLayout *dialogLayout = new QVBoxLayout;
    dialogLayout->addLayout(boxLabelLayout);
    dialogLayout->addWidget(p_visualBox);
    dialogLayout->addWidget(statsBox);

    p_dialog->setLayout(dialogLayout);

    checkBox->setChecked(true);
  }

  /**
   * Attaches this tool to the toolpad
   *
   * @param toolpad
   *
   * @return QAction*
   */
  QAction *StatisticsTool::toolPadAction(ToolPad *toolpad) {
    QAction *action = new QAction("Statistics", toolpad);
    action->setIcon(QPixmap(toolIconDir() + "/statistics.png"));
    action->setToolTip("Statistics");
    QObject::connect(action, SIGNAL(triggered()), p_dialog, SLOT(show()));

    QString text  = "";

    action->setWhatsThis(text);
    return action;
  }

  /**
   * Attaches this tool to the toolbar
   *
   * @param parent
   *
   * @return QWidget*
   */
  QWidget *StatisticsTool::createToolBarWidget(QStackedWidget *parent) {
    QWidget *hbox = new QWidget(parent);

    QIntValidator *ival = new QIntValidator(hbox);
    ival->setRange(1, 100);

    QLabel *sampleLabel = new QLabel("Box Samples:");
    p_sampsEdit = new QLineEdit(hbox);
    p_sampsEdit->setValidator(ival);
    p_sampsEdit->setMaximumWidth(50);

    QString samps;
    samps.setNum(p_boxSamps);

    p_sampsEdit->setText(samps);
    connect(p_sampsEdit, SIGNAL(editingFinished()), this, SLOT(changeBoxSamples()));

    QLabel *lineLabel = new QLabel("Box Lines:");
    p_linesEdit = new QLineEdit(hbox);
    p_linesEdit->setValidator(ival);
    p_linesEdit->setMaximumWidth(50);

    QString lines;
    lines.setNum(p_boxLines);

    p_linesEdit->setText(lines);
    connect(p_linesEdit, SIGNAL(editingFinished()), this, SLOT(changeBoxLines()));

    QToolButton *showButton = new QToolButton();
    showButton->setText("Show");
    showButton->setToolTip("");
    QString text = "";
    showButton->setWhatsThis(text);

    connect(showButton, SIGNAL(clicked()), p_dialog, SLOT(show()));

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->addWidget(sampleLabel);
    layout->addWidget(p_sampsEdit);
    layout->addWidget(lineLabel);
    layout->addWidget(p_linesEdit);
    layout->addWidget(showButton);
    layout->addStretch(1);
    hbox->setLayout(layout);
    return hbox;
  }

  /**
   * Called when a mouse button is released
   *
   * @param p
   * @param s
   */
  void StatisticsTool::mouseButtonRelease(QPoint p, Qt::MouseButton s) {
    if(s == Qt::LeftButton) {
      getStatistics(p);
    }
  }

  /**
   * Hide/Show the visual display
   *
   * @param hide
   */
  void StatisticsTool::hideDisplay(bool hide) {
    if(hide) {
      p_visualBox->hide();
      p_sampLabel->hide();
      p_lineLabel->hide();
      p_dnLabel->hide();

      p_dialog->setMinimumSize(350, 165);
      p_dialog->resize(350, 165);
    }
    else {
      p_visualBox->show();
      p_sampLabel->show();
      p_lineLabel->show();
      p_dnLabel->show();

      p_dialog->setMinimumSize(565, 765);
      p_dialog->resize(565, 765);
    }
  }

  /**
   * Retrieve the statistics based on the box size
   * and point on the cube.
   *
   * @param p
   */
  void StatisticsTool::getStatistics(QPoint p) {
    MdiCubeViewport *cvp = cubeViewport();
    if(cvp == NULL) return;

    double sample, line;
    cvp->viewportToCube(p.x(), p.y(), sample, line);

    // If we are outside of the cube, do nothing
    if((sample < 0.5) || (line < 0.5) ||
        (sample > cvp->cubeSamples() + 0.5) || (line > cvp->cubeLines() + 0.5)) {
      return;
    }

    int isamp = (int)(sample + 0.5);
    int iline = (int)(line + 0.5);

    Statistics stats;
    Brick *brick = new Brick(1, 1, 1, cvp->cube()->pixelType());


    QVector<QVector<double> > pixelData(p_boxLines, QVector<double>(p_boxSamps, Null));

    double lineDiff = p_boxLines / 2.0;
    double sampDiff = p_boxSamps / 2.0;

    p_ulSamp = isamp - (int)floor(sampDiff);
    p_ulLine = iline - (int)floor(lineDiff);

    int x, y;

    y = p_ulLine;

    for(int i = 0; i < p_boxLines; i++) {
      x = p_ulSamp;
      if(y < 1 || y > cvp->cubeLines()) {
        y++;
        continue;
      }
      for(int j = 0; j < p_boxSamps; j++) {
        if(x < 1 || x > cvp->cubeSamples()) {
          x++;
          continue;
        }
        brick->SetBasePosition(x, y, cvp->grayBand());
        cvp->cube()->read(*brick);
        stats.AddData(brick->at(0));
        pixelData[i][j] = brick->at(0);

        x++;
      }
      y++;
    }

    p_visualDisplay->setPixelData(pixelData, p_ulSamp, p_ulLine);

    if (stats.ValidPixels()) {
      p_minLabel->setText(QString("Minimum: %1").arg(stats.Minimum()));
      p_maxLabel->setText(QString("Maximum: %1").arg(stats.Maximum()));
      p_avgLabel->setText(QString("Average: %1").arg(stats.Average()));
      p_stdevLabel->setText(QString("Standard Dev: %1").arg(stats.StandardDeviation(), 0, 'f', 6));
    }
    else {
      p_minLabel->setText(QString("Minimum: n/a"));
      p_maxLabel->setText(QString("Maximum: n/a"));
      p_avgLabel->setText(QString("Average: n/a"));
      p_stdevLabel->setText(QString("Standard Dev: n/a"));
    }

    p_set = true;

    resizeScrollbars();
  }

  /**
   * Change the box sample size.
   *
   */
  void StatisticsTool::changeBoxSamples() {
    QString samps = p_sampsEdit->text();
    if(samps != "" && samps.toInt() != p_boxSamps && samps.toInt() > 0) {
      p_boxSamps = samps.toInt();
      QString lines;
      lines.setNum(p_boxLines);
      p_boxLabel->setText(samps + "x" + lines);

      p_visualDisplay->setSamples(p_boxSamps);

      p_set = false;

      resizeScrollbars();
    }
  }

  /**
   * Change the box line size.
   *
   */
  void StatisticsTool::changeBoxLines() {
    QString lines = p_linesEdit->text();
    if(lines != "" && lines.toInt() != p_boxLines && lines.toInt() > 0) {
      p_boxLines = lines.toInt();
      QString samps;
      samps.setNum(p_boxSamps);
      p_boxLabel->setText(samps + "x" + lines);

      p_visualDisplay->setLines(p_boxLines);

      p_set = false;

      resizeScrollbars();
    }
  }

  /**
   * Resize the scroll bars and center the point clicked.
   *
   */
  void StatisticsTool::resizeScrollbars() {
    QScrollBar *hbar = p_visualScroll->horizontalScrollBar();
    QScrollBar *vbar = p_visualScroll->verticalScrollBar();
    hbar->setSliderPosition((hbar->maximum() + hbar->minimum()) / 2);
    vbar->setSliderPosition((vbar->maximum() + vbar->minimum()) / 2);
  }

  /**
   * Constructor for visual display.
   *
   * @param parent
   */
  VisualDisplay::VisualDisplay(QWidget *parent) : QWidget(parent),
    p_boxSamps(3),
    p_boxLines(3),
    p_boxWidth(20),
    p_boxHeight(20),
    p_oldWidth(20),
    p_oldHeight(20),
    p_ulSamp(-1),
    p_ulLine(-1),
    p_set(false),
    p_showText(true),
    p_showPixels(false),
    p_showDeviation(false) {

    p_stretch.SetNull(0.0);
    p_stretch.SetLis(0.0);
    p_stretch.SetLrs(0.0);
    p_stretch.SetHis(255.0);
    p_stretch.SetHrs(255.0);
    p_stretch.SetMinimum(0.0);
    p_stretch.SetMaximum(255.0);
    p_pixelData = QVector<QVector<double> >(p_boxLines, QVector<double>(p_boxSamps, Null));
    paintPixmap();
    setMouseTracking(true);
    setBackgroundRole(QPalette::Dark);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
  }

  /**
   * Size hint for this widget
   *
   *
   * @return QSize
   */
  QSize VisualDisplay::sizeHint() const {
    return QSize(460, 460);
  }

  /**
   * Set box sample size
   *
   * @param samps
   */
  void VisualDisplay::setSamples(int samps) {
    p_boxSamps = samps;
    p_pixelData = QVector<QVector<double> >(p_boxLines, QVector<double>(p_boxSamps, 0));
    p_stats.Reset();

    p_set = false;
    updateSize();
  }

  /**
   * Set box line size
   *
   * @param lines
   */
  void VisualDisplay::setLines(int lines) {
    p_boxLines = lines;
    p_pixelData = QVector<QVector<double> >(p_boxLines, QVector<double>(p_boxSamps, 0));
    p_stats.Reset();

    p_set = false;
    updateSize();
  }

  /**
   * Set box size in pixels
   *
   * @param size
   */
  void VisualDisplay::setBoxSize(int size) {
    p_boxWidth = 2 * size;
    p_boxHeight = 2 * size;
    p_oldWidth = p_boxWidth;
    p_oldHeight = p_boxHeight;
    updateSize();
  }

  /**
   * Update the size of the box
   *
   */
  void VisualDisplay::updateSize() {
    if(p_boxSamps > this->sizeHint().width() / p_boxWidth) {
      resize(this->sizeHint().width() + (p_boxWidth * (p_boxSamps - this->sizeHint().width() / p_boxWidth)), this->size().height());
    }
    else {
      resize(this->sizeHint().width(), this->size().height());
    }

    if(p_boxLines > this->sizeHint().height() / p_boxHeight) {
      resize(this->size().width(), this->sizeHint().height() + (p_boxHeight * (p_boxLines - this->sizeHint().height() / p_boxHeight)));
    }
    else {
      resize(this->size().width(), this->sizeHint().height());
    }

    paintPixmap();
    update();
  }

  /**
   * Show/Hide text
   *
   * @param b
   */
  void VisualDisplay::showText(bool b) {
    p_showText = b;
    if(b) {
      p_oldWidth = p_boxWidth;
      p_oldHeight = p_boxHeight;
      p_boxWidth = 100;
      p_boxHeight = 20;
      updateSize();
    }
  }


  /**
   * Show/Hide pixels
   *
   * @param b
   */
  void VisualDisplay::showPixels(bool b) {
    p_showPixels = b;
    if(b) {
      if(p_boxWidth != p_oldWidth || p_boxHeight != p_oldHeight) {
        p_boxWidth = p_oldWidth;
        p_boxHeight = p_oldHeight;
      }
      updateSize();
    }
  }

  /**
   * Show/Hide deviation
   *
   * @param b
   */
  void VisualDisplay::showDeviation(bool b) {
    p_showDeviation = b;
    if(b) {
      if(p_boxWidth != p_oldWidth || p_boxHeight != p_oldHeight) {
        p_boxWidth = p_oldWidth;
        p_boxHeight = p_oldHeight;
      }
      updateSize();
    }
  }

  /**
   * Set pixel data and upper left sample/line
   *
   * @param data
   * @param samp
   * @param line
   */
  void VisualDisplay::setPixelData(QVector<QVector<double> > data, int samp, int line) {
    p_pixelData = data;
    p_ulSamp = samp;
    p_ulLine = line;

    p_stats.Reset();

    for(int i = 0; i < data.size(); i++) {
      for(int j = 0; j < data[i].size(); j++) {
        if(p_ulSamp + j < 0 || p_ulLine + i < 0) continue;
        p_stats.AddData(data[i][j]);
      }
    }

    if(fabs(p_stats.BestMinimum()) < DBL_MAX && fabs(p_stats.BestMaximum()) < DBL_MAX) {
      Histogram hist(p_stats.BestMinimum(), p_stats.BestMaximum());
      for(int i = 0; i < data.size(); i++) {
        hist.AddData(data[i].data(), data[i].size());
      }

      p_stretch.ClearPairs();
      if(hist.Percent(0.5) != hist.Percent(99.5)) {
        p_stretch.AddPair(hist.Percent(0.5), 0.0);
        p_stretch.AddPair(hist.Percent(99.5), 255.0);
      }
      else {
        p_stretch.AddPair(-DBL_MAX, 0.0);
        p_stretch.AddPair(DBL_MAX, 255.0);
      }
    }
    else {
      p_stretch.ClearPairs();
      p_stretch.AddPair(-DBL_MAX, 0.0);
      p_stretch.AddPair(DBL_MAX, 255.0);
    }

    p_set = true;
    paintPixmap();
  }

  /**
   * Paint the pixmap
   *
   */
  void VisualDisplay::paintPixmap() {
    p_pixmap = QPixmap(p_boxSamps * p_boxWidth, p_boxLines * p_boxHeight);
    p_pixmap.fill();
    QPainter p(&p_pixmap);
    QRect rect(0, 0, p_boxWidth, p_boxHeight);

    int midX = p_pixmap.width() / 2 - ((p_boxWidth / 2) * (p_boxSamps % 2));
    int midY = p_pixmap.height() / 2 - ((p_boxHeight / 2) * (p_boxLines % 2));

    int x, y;
    y = 0;

    for(int i = 0; i < p_boxLines; i++) {
      x = 0;
      for(int j = 0; j < p_boxSamps; j++) {
        double dn = p_pixelData[i][j];
        QColor c;
        if(p_showPixels || p_showDeviation) {
          if(p_showDeviation) {
            if(!IsSpecial(dn) && p_stats.TotalPixels() > 0 && p_stats.StandardDeviation() != 0) {
              double diff;

              if(dn < p_stats.Average()) {
                diff = p_stats.Average() - dn;
                diff /= p_stats.Average() - p_stats.Minimum();
              }
              else {
                diff = dn - p_stats.Average();
                diff /= p_stats.Maximum() - p_stats.Average();
              }

              int i = (int)(diff * 255.0);
              c = QColor(i, 255 - i, 0);
            }
            else {
              c = QColor(0, 0, 0);
            }
          }
          else {
            double visualValue = p_stretch.Map(dn);

            c = QColor(visualValue, visualValue, visualValue);
          }
        }
        p.save();
        p.translate(x, y);

        if(p_showText) {
          p.drawRect(rect);

          if (!IsSpecial(dn))
            p.drawText(rect, Qt::AlignCenter, QString::number(dn));
          else
            p.drawText(rect, Qt::AlignCenter, QString::fromStdString(PixelToString(dn)));
        }
        else {
          p.fillRect(rect, c);
        }

        p.restore();
        x += p_boxWidth;
      }
      y += p_boxHeight;
    }

    p.setPen(QPen(Qt::red, 1));
    p.save();
    p.translate(midX, midY);
    p.drawRect(rect);
    p.restore();
    update();
  }

  /**
   * Paint pixmap to the widget
   *
   * @param event
   */
  void VisualDisplay::paintEvent(QPaintEvent *event) {

    QPainter painter(this);

    int midX = width() / 2 - (p_boxWidth * (int)floor(p_boxSamps / 2) + (p_boxWidth / 2));
    int midY = height() / 2 - (p_boxHeight * (int)floor(p_boxLines / 2) + (p_boxHeight / 2));

    painter.drawPixmap(midX, midY, p_pixmap);
  }

  /**
   * Called when the mouse moves over this widget
   *
   * @param event
   */
  void VisualDisplay::mouseMoveEvent(QMouseEvent *event) {
    double startX = width() / 2 - (p_boxWidth * (int)floor(p_boxSamps / 2) + (p_boxWidth / 2));
    double startY = height() / 2 - (p_boxHeight * (int)floor(p_boxLines / 2) + (p_boxHeight / 2));

    int x = (int)ceil((event->x() - startX) / p_boxWidth);
    int y = (int)ceil((event->y() - startY) / p_boxHeight);

    if(!p_set || x < 1 || y < 1 || x > p_boxSamps || y > p_boxLines) {
      emit setSample("Sample: n/a");
      emit setLine("Line: n/a");
      emit setDn("DN: n/a");
    }
    else {
      emit setSample(QString("Sample: %1").arg(p_ulSamp + x - 1));
      emit setLine(QString("Line: %1").arg(p_ulLine + y - 1));
      double dn = p_pixelData[y-1][x-1];
      if(IsSpecial(dn))
        emit setDn(QString("DN: %1").arg(QString::fromStdString(PixelToString(dn))));
      else
        emit setDn(QString("DN: %1").arg(dn));
    }
  }

  /**
   * Mouse left widget, update labels
   *
   * @param event
   */
  void VisualDisplay::leaveEvent(QEvent *event) {
    emit setSample("Sample: n/a");
    emit setLine("Line: n/a");
    emit setDn("DN: n/a");
  }
}
