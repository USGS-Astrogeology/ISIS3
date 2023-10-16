#include "hist.h"

#include <QDockWidget>
#include <QMenuBar>
#include <QString>

#include "CubePlotCurve.h"
#include "Histogram.h"
#include "ImageHistogram.h"
#include "HistogramItem.h"
#include "HistogramPlotWindow.h"
#include "LineManager.h"
#include "Process.h"
#include "Progress.h"
#include "QHistogram.h"
#include "UserInterface.h"


using namespace std;

namespace Isis {

  void hist(UserInterface &ui) {
    Cube cube;
    CubeAttributeInput inAtt = ui.GetInputAttribute("FROM");
    if (inAtt.bands().size() != 0) {
      cube.setVirtualBands(inAtt.bands());
    }
    cube.open(ui.GetCubeName("FROM"));
    hist(&cube, ui);
  }

  void hist(Cube *icube, UserInterface &ui) {
    Process p;

    if (!ui.WasEntered("TO") && !ui.IsInteractive()) {
      std::string msg = "The [TO] parameter must be entered";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    Histogram *hist;
    if (ui.WasEntered("MINIMUM") && ui.WasEntered("MAXIMUM")) {
      int nbins = 0;

      if (ui.WasEntered("NBINS")){
        nbins = ui.GetInteger("NBINS");
      }
      else {
        // Calculate bins based on data type.
        // Bin calculations are based on logic in Histogram::InitializeFromCube
        if (icube->pixelType() == UnsignedByte) {
          nbins = 256;
        }
        else if (icube->pixelType() == SignedWord ||
                 icube->pixelType() == UnsignedWord ||
                 icube->pixelType() == UnsignedInteger ||
                 icube->pixelType() == SignedInteger ||
                 icube->pixelType() == Real) {
          nbins = 65536;
        }
        else {
          IString msg = "Unsupported pixel type";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
      hist = new Histogram(ui.GetDouble("MINIMUM"), ui.GetDouble("MAXIMUM"), nbins);
    }
    else {

      if (ui.WasEntered("NBINS")){
        hist = new ImageHistogram(*icube, 1, p.Progress(), 1, 1, Null, Null, ui.GetInteger("NBINS"));
      }
      else {
        hist = new ImageHistogram(*icube, 1, p.Progress());
      }
    }

    // Setup the histogram

    // Loop and accumulate histogram
    p.Progress()->SetText("Gathering Histogram");
    p.Progress()->SetMaximumSteps(icube->lineCount());
    p.Progress()->CheckStatus();
    LineManager line(*icube);

    for (int i = 1; i <= icube->lineCount(); i++) {
      line.SetLine(i);
      icube->read(line);
      hist->AddData(line.DoubleBuffer(), line.size());
      p.Progress()->CheckStatus();
    }

    if (!ui.IsInteractive() || ui.WasEntered("TO") ) {
      // Write the results
      QString outfile = ui.GetFileName("TO");
      ofstream fout;
      fout.open(outfile.toLatin1().data());

      fout << "Cube:              " << icube->fileName() << endl;
      fout << "Band:              " << icube->bandCount() << endl;

      if(hist->ValidPixels() != 0) {
        fout << "Average:           " << hist->Average() << endl;
        fout << "Std Deviation:     " << hist->StandardDeviation() << endl;
        fout << "Variance:          " << hist->Variance() << endl;
        fout << "Median:            " << hist->Median() << endl;
        fout << "Mode:              " << hist->Mode() << endl;
        fout << "Skew:              " << hist->Skew() << endl;
        fout << "Minimum:           " << hist->Minimum() << endl;
        fout << "Maximum:           " << hist->Maximum() << endl;
      }
      else {
        fout << "Average:           " << "N/A" << endl;
        fout << "Std Deviation:     " << "N/A" << endl;
        fout << "Variance:          " << "N/A" << endl;
        fout << "Median:            " << "N/A" << endl;
        fout << "Mode:              " << "N/A" << endl;
        fout << "Skew:              " << "N/A" << endl;
        fout << "Minimum:           " << "N/A" << endl;
        fout << "Maximum:           " << "N/A" << endl;
      }

      fout << endl;
      fout << "Total Pixels:      " << hist->TotalPixels() << endl;
      fout << "Valid Pixels:      " << hist->ValidPixels() << endl;
      fout << "Pixels Below Min:  " << hist->UnderRangePixels() << endl;
      fout << "Pixels Above Max:  " << hist->OverRangePixels() << endl;
      fout << "Null Pixels:       " << hist->NullPixels() << endl;
      fout << "Lis Pixels:        " << hist->LisPixels() << endl;
      fout << "Lrs Pixels:        " << hist->LrsPixels() << endl;
      fout << "His Pixels:        " << hist->HisPixels() << endl;
      fout << "Hrs Pixels:        " << hist->HrsPixels() << endl;

      //  Write histogram in tabular format
      fout << endl;
      fout << endl;
      fout << "MinInclusive,MaxExclusive,Pixels,CumulativePixels,Percent,CumulativePercent" << endl;

      Isis::BigInt total = 0;
      double cumpct = 0.0;
      double low;
      double high;

      for (int i = 0; i < hist->Bins(); i++) {
        if (hist->BinCount(i) > 0) {
          total += hist->BinCount(i);
          double pct = (double)hist->BinCount(i) / hist->ValidPixels() * 100.;
          cumpct += pct;

          hist->BinRange(i, low, high);

          fout << low << ",";
          fout << high << ",";
          fout << hist->BinCount(i) << ",";
          fout << total << ",";
          fout << pct << ",";
          fout << cumpct << endl;
        }
      }
      fout.close();
    }
    // If we are in gui mode, create a histogram plot
    if (ui.IsInteractive()) {
      // Set the title for the dialog
      QString title;
      if (ui.WasEntered("TITLE") ) {
        title = ui.GetString("TITLE");
      }
      else {
        title = "Histogram Plot for " + FileName(ui.GetAsString("FROM")).name();
      }

      // Create the QHistogram, set the title & load the Isis::Histogram into it

      HistogramPlotWindow *plot = new HistogramPlotWindow(title.toLatin1().data(),
                                                          ui.TheGui());

      // Set the xaxis title if they entered one
      if (ui.WasEntered("XAXIS") ) {
        QString xaxis(ui.GetString("XAXIS"));
        plot->setAxisLabel(QwtPlot::xBottom, xaxis.toLatin1().data());
      }

      // Set the yLeft axis title if they entered one
      if (ui.WasEntered("FREQAXIS") ) {
        QString yaxis(ui.GetString("FREQAXIS"));
        plot->setAxisLabel(QwtPlot::yRight, yaxis.toLatin1().data());
      }

      // Set the yRight axis title if they entered one
      if (ui.WasEntered("PERCENTAXIS") ) {
        QString y2axis(ui.GetString("PERCENTAXIS") );
        plot->setAxisLabel(QwtPlot::yLeft, y2axis.toLatin1().data());
      }

      //Transfer data from histogram to the plotcurve
      QVector<QPointF> binCountData;
      QVector<QPointF> cumPctData;
      double cumpct = 0.0;
      double low;
      double high;
      for (int i = 0; i < hist->Bins(); i++) {
        if (hist->BinCount(i) > 0) {
          hist->BinRange(i, low, high);
          binCountData.append(QPointF(low, hist->BinCount(i) ) );

          double pct = (double)hist->BinCount(i) / hist->ValidPixels() * 100.0;
          cumpct += pct;
          cumPctData.append(QPointF(low, cumpct) );
        }
      }

      HistogramItem *histCurve = new HistogramItem();
      histCurve->setColor(Qt::darkCyan);
      histCurve->setTitle("Frequency");

      CubePlotCurve *cdfCurve = new CubePlotCurve(CubePlotCurve::CubeDN,
                                                  CubePlotCurve::Percentage);
      cdfCurve->setStyle(QwtPlotCurve::Lines);
      cdfCurve->setTitle("Percentage");

      QPen *pen = new QPen(Qt::red);
      pen->setWidth(2);
      histCurve->setYAxis(QwtPlot::yRight);
      cdfCurve->setYAxis(QwtPlot::yLeft);
      cdfCurve->setPen(*pen);

      QVector<QwtIntervalSample> intervals(binCountData.size() );
      for (int y = 0; y < binCountData.size(); y++) {

        intervals[y].interval = QwtInterval(binCountData[y].x(),
                                            binCountData[y].x() + hist->BinSize());

        intervals[y].value = binCountData[y].y();
      }

      QPen percentagePen(Qt::red);
      percentagePen.setWidth(2);
      cdfCurve->setColor(Qt::red);
      cdfCurve->setMarkerSymbol(QwtSymbol::NoSymbol);

      histCurve->setData(QwtIntervalSeriesData(intervals));
      cdfCurve->setData(new QwtPointSeriesData(cumPctData));

      plot->add(histCurve);
      plot->add(cdfCurve);

      QLabel *label;
      if(hist->ValidPixels() != 0) {
        label = new QLabel("  Average = " + QString::number(hist->Average()) + '\n' +
               "\n  Minimum = " + QString::number(hist->Minimum()) + '\n' +
               "\n  Maximum = " + QString::number(hist->Maximum()) + '\n' +
               "\n  Stand. Dev.= " + QString::number(hist->StandardDeviation()) + '\n' +
               "\n  Variance = " + QString::number(hist->Variance()) + '\n' +
               "\n  Median = " + QString::number(hist->Median()) + '\n' +
               "\n  Mode = " + QString::number(hist->Mode()) + '\n' +
               "\n  Skew = " + QString::number(hist->Skew()), plot);
      }
      else {
        label = new QLabel("  Average = N/A"
                           "\n\n  Minimum = N/A"
                           "\n\n  Maximum = N/A"
                           "\n\n  Stand. Dev.= N/A"
                           "\n\n  Variance = N/A"
                           "\n\n  Median = N/A"
                           "\n\n  Mode = N/A"
                           "\n\n  Skew = N/A" , plot);
      }

      plot->getDockWidget()->setWidget(label);

      plot->showWindow();
    }
    delete hist;
    p.EndProcess();
  }
}
