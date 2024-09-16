/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Isis.h"

#include <QString>
#include  <QColor>
#include <QMenuBar>

#include "ControlNet.h"
#include "ControlMeasure.h"
#include "FileList.h"
#include "Process.h"
#include "Histogram.h"
#include "ImageHistogram.h"
#include "UserInterface.h"
#include "Progress.h"
#include "LineManager.h"
#include "QHistogram.h"
#include "HistogramPlotWindow.h"
#include "HistogramItem.h"
#include "IString.h"
#include "CubePlotCurve.h"

using namespace std;
using namespace Isis;

QColor curveColor(int i);

void IsisMain() {
  Process p;
  Progress progress;

  UserInterface &ui = Application::GetUserInterface();

  //prepare the list of net files
  FileList fList;  //empty FileList
  if (ui.WasEntered("CLIST"))
    fList.read(ui.GetFileName("CLIST").toStdString());
  if (ui.WasEntered("CNET"))
    fList << ui.GetFileName("CNET").toStdString();

  HistogramPlotWindow *plot=NULL;

  //setup plot tile and axis labels (if any)
  if(ui.IsInteractive()) {
    // Set the title for the dialog
    QString title;
    if(ui.WasEntered("TITLE")) {
      title = ui.GetString("TITLE");
    }
    else {
      title = "Control Net Histograms";
    }

    // Create the QHistogram, set the title & load the Isis::Histogram into it

    plot = new HistogramPlotWindow(title.toLatin1().data(), ui.TheGui());

    // Set the xaxis title if they entered one
    if(ui.WasEntered("XAXIS")) {
      QString xaxis(ui.GetString("XAXIS"));
      plot->setAxisLabel(QwtPlot::xBottom, xaxis.toLatin1().data());
    }

    // Set the yLeft axis title if they entered one
    if(ui.WasEntered("FREQAXIS")) {
      QString yaxis(ui.GetString("FREQAXIS"));
      plot->setAxisLabel(QwtPlot::yLeft, yaxis);
    }
    else {
      QString yaxis = "Frequencey";
      plot->setAxisLabel(QwtPlot::yLeft, yaxis);
    }

    QString yaxis = "";
    plot->setAxisLabel(QwtPlot::yRight, yaxis);
  }

  //open text report file (if any)
  ofstream fout;
  if(!ui.IsInteractive() || ui.WasEntered("TO")) {
    // Write the results

    if(!ui.WasEntered("TO")) {
      std::string msg = "The [TO] parameter must be entered";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    QString outfile = ui.GetFileName("TO");
    fout.open(outfile.toLatin1().data());
  }

  //loop throught the control nets writing reports and drawing histograms as needed
  for (int i=0;i<fList.size();i++) {
    ControlNet net(QString::fromStdString(fList[i].toString()),&progress);
    Histogram *hist;
    // Setup the histogram
    try {
      hist = new Histogram(net, &ControlMeasure::GetResidualMagnitude, ui.GetDouble("BIN_WIDTH"));
    }
    catch (IException &e) {
      std::string msg = "The following error was thrown while building a histogram from netfile [" +
                    fList[i].expanded() + "]: " + e.toString() + "\n";
      if (ui.IsInteractive())  //if in gui mode print the error message to the terminal
        Application::GuiLog(QString::fromStdString(msg));
      if (ui.WasEntered("TO")) //add the msg to the output file if there is one
        fout << msg << endl << endl << endl;

      Application::Log(e.toPvl().findGroup("Error"));
      Application::Log(IException(IException::User,msg, _FILEINFO_).toPvl().findGroup("Error"));

      continue; //skip to the next next net file
    }


    //Tabular Histogram Data
    if(!ui.IsInteractive() || ui.WasEntered("TO")) {
      fout << "Network:        " << fList[i].toString() << endl;
      fout << "Average:        " << hist->Average() << endl;
      fout << "Std Deviation:  " << hist->StandardDeviation() << endl;
      fout << "Variance:       " << hist->Variance() << endl;
      fout << "Median:         " << hist->Median() << endl;
      fout << "Mode:           " << hist->Mode() << endl;
      fout << "Skew:           " << hist->Skew() << endl;
      fout << "Minimum:        " << hist->Minimum() << endl;
      fout << "Maximum:        " << hist->Maximum() << endl;
      fout << "Total Measures: " << hist->TotalPixels() << endl;

      //  Write histogram in tabular format
      fout << endl;
      fout << "ResidualMagnitudeMin,ResidualMagnitudeMax,MeasuresInBin,CumulativeMeasures,Percent,CumulativePercent" << endl;

      Isis::BigInt total = 0;
      double cumpct = 0.0;
      double low;
      double high;

      for(int j = 0; j < hist->Bins(); j++) {
        if(hist->BinCount(j) > 0) {
          total += hist->BinCount(j);
          double pct = (double)hist->BinCount(j) / hist->ValidPixels() * 100.;
          cumpct += pct;

          hist->BinRange(j, low, high);

          fout << low << ",";
          fout << high << ",";
          fout << hist->BinCount(j) << ",";
          fout << total << ",";
          fout << pct << ",";
          fout << cumpct << endl;
        }
      }
      fout << endl;
      fout << endl;
      fout << endl;
      fout << endl;
    }

    // If we are in gui mode, add a plot
    if(ui.IsInteractive()) {
      //Transfer data from histogram to the plotcurve
      QVector<QPointF> binCountData;
      double low;
      double high;
      for(int j = 0; j < hist->Bins(); j++) {
        if(hist->BinCount(j) > 0) {
          hist->BinRange(j, low, high);
          binCountData.append(QPointF(low, hist->BinCount(j)));
        }
      }

      CubePlotCurve *histCurve = new CubePlotCurve(CubePlotCurve::CubeDN,
                                                CubePlotCurve::Percentage);
      histCurve->setColor(curveColor(i));
      QString baseName = QString::fromStdString(FileName(fList[i]).baseName());
      histCurve->setTitle(baseName);


      QPen *pen = new QPen(curveColor(i));
      pen->setWidth(2);
      histCurve->setYAxis(QwtPlot::yLeft);
      histCurve->setPen(*pen);
      histCurve->setMarkerSymbol(QwtSymbol::NoSymbol);

      histCurve->setData(new QwtPointSeriesData(binCountData));

      plot->add(histCurve);
    }
    delete hist;
  }
  if (ui.IsInteractive()) plot->showWindow();
  if (!ui.IsInteractive() || ui.WasEntered("TO")) fout.close();
  p.EndProcess();
}

QColor curveColor(int i) {
  int  j = i%16;

  switch (j) {
  case 0:
    return QColor("red");
  case 6:
    return QColor("DeepPink");
  case 5:
    return QColor("orange");
  case 3:
    return QColor("black");
  case 4:
    return QColor("Indigo");
  case 2:
    return QColor("green");
  case 1:
    return QColor("Blue");
  case 7:
    return QColor("SaddleBrown");
  case 8:
    return QColor("darkred");
  case 9:
    return QColor("palevioletred");
  case 10:
    return QColor("coral");
  case 11:
    return QColor("darkkhaki");
  case 12:
    return QColor("orchid");
  case 13:
    return QColor("lime");
  case 14:
    return QColor("cyan");
  case 15:
    return QColor("goldenrod");
  }

  return QColor("olivedrab");
}
