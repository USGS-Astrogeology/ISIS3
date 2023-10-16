#define USE_GUI_QAPP

#include "Isis.h"

#include "Camera.h"
#include "Cube.h"
#include "IException.h"
#include "Process.h"
#include "ProgramLauncher.h"
#include "Projection.h"
#include "ProjectionFactory.h"
#include "SpecialPixel.h"
#include "UniversalGroundMap.h"

#include <cmath>
#include <QFile>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QVector>

using namespace std;
using namespace Isis;

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();

  // Get the camera information if this is not a mosaic. Otherwise, get the
  // projection information
  Process p;
  Cube icube;
  icube.open(ui.GetCubeName("FROM") );

  // Determine where in image to get resolution from and get it
  int numSamps = icube.sampleCount();
  int numLines = icube.lineCount();
  int numBands = icube.bandCount();
  double sampForResolution, lineForResolution;
  if (ui.GetString("PIXRES").toUpper() == "USER") {
    sampForResolution = ui.GetDouble("SAMPLE");
    lineForResolution = ui.GetDouble("LINE");
  }
  else {
    sampForResolution = (numSamps / 2.0) + .5;
    lineForResolution = (numLines / 2.0) + .5;
  }

  UniversalGroundMap groundMap(icube);
  groundMap.SetImage(sampForResolution, lineForResolution);
  double resolution = groundMap.Resolution();

  // Determine the unit of measure used to create the scale and the limit for the
  // right and left sides of the scale
  QString units = ui.GetString("UNITS").toUpper();
  double rightLimit = ui.GetInteger("RIGHTLIMIT");
  double leftLimit = ui.GetInteger("LEFTLIMIT");

  // Determine how many segments will be in right and left sides of the scale
  int rightSegs = ui.GetInteger("RIGHTSEGS");
  int leftSegs = 0;
  if (leftLimit > 0) {
    if (ui.WasEntered("LEFTSEGS") ) {
      leftSegs = ui.GetInteger("LEFTSEGS");
    }
    else {
      std::string msg = "Number of segments was not specified for left side of scale - "
                    "must be at least 1";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  // Determine the units of measurement for the scale bar - the resolution provided
  // by ISIS is in meters/pixel.
  double scaleUnit;
  if (units == "KILOMETER") {
    scaleUnit = 1000.0;
  }
  else {
    scaleUnit = 1.0;
  }

  // Determine where the bar scale will be placed in the image.
  // cornerLine,cornerSample keep track of lower left corner of bar scale
  // placeLine,placeSample keep track of upper left corner of bar scale
  bool padImage = false;
  QString padLocation;
  int cornerLine = 0;
  int cornerSample = 0;
  int placeLine = 0;
  int placeSample = 0;
  if (ui.GetBoolean("PADIMAGE") ) {
    padImage = true;
    padLocation = ui.GetString("PLACEMENT").toUpper();
  }
  else {
    if (!ui.WasEntered("CORNERLINE") || !ui.WasEntered("CORNERSAMPLE") ) {
      std::string msg = "The upper left placement of the scale bar must be specified ";
      msg = msg + "if you are not padding the image.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
    placeLine = ui.GetInteger("CORNERLINE");
    placeSample = ui.GetInteger("CORNERSAMPLE");
  }

  // Determine width/height of the scale based on the resolution that the bar
  // scale will be printed at
  int barHeight = ui.GetInteger("BARHEIGHT");
  if (barHeight < 5) {
    std::string msg = "Requested BARHEIGHT is too small to produce a bar scale";
    throw IException(IException::User, msg, _FILEINFO_);
  }
  int barWidth = ( (rightLimit + leftLimit) * scaleUnit) / resolution + .5;

  // Determine how many pixels are needed to make up each component of the bar
  // scale. The amount of pixels available are determined by height. Each
  // component takes up a specified percentage of the available pixels. Extra
  // pixels need to be divided between the horizontal measurement line and the
  // space around it.
  //
  // The following measurements for the line weights making up the bar scale
  // were taken from the http://pubs.usgs.gov/of/1999/of99-430/of99-430_sec35.pdf
  // document of USGS standards:
  //
  //                 --  |--------------------|--------------------| <--  8.3%
  //               33.3% |                    |                    |  "
  //           16.6% --> |====================|                    | 83.4%
  //               33.3% |                    |                    |  "
  //                 --  |--------------------|--------------------| <--  8.3%
  //                                          ^
  //                                          |
  //                                         8.3%
  //
  // Each exterior horizontal line takes up 8.3% of the total height of the bar
  // scale. All vertical lines have the same weight as the exterior horizontal lines.
  // Interior horizontal lines take up 16.6% of the total height of the bar
  // scale. The remaining 66.6% of the total height of the bar scale is evenly
  // divided among the space between the interior horizontal line and the exterior
  // horizontal lines.
  //
  int bndLine = .083 * barHeight;
  if (bndLine < 1) bndLine = 1;
  int midLine = .166 * barHeight;
  if (midLine < 1) midLine = 1;
  int spacing = .333 * barHeight;
  if (spacing < 1) spacing = 1;
  int topSpace = spacing;
  int botSpace = spacing;
  int total = 2 * bndLine + midLine + 2 * spacing;
  if (spacing > (2 * midLine) ) {
    while (spacing > (2 * midLine) && total < barHeight) {
      midLine = midLine + 1;
      total = total + 1;
    }
    if (total < barHeight) {
      while (total < barHeight) {
        topSpace = topSpace + 1;
        total = total + 1;
        if (total < barHeight) {
          botSpace = botSpace + 1;
          total = total + 1;
        }
        if (total < barHeight && topSpace > (2 * midLine) ) {
          midLine = midLine + 1;
          total = total + 1;
        }
      }
    }
  }

  // Get user's preferences for background color and text size and
  // location
  QString backGround = ui.GetString("BACKGROUND").toUpper();
  int textHt = ui.GetInteger("TEXTHEIGHT");
  QString textLoc = ui.GetString("TEXTLOC").toUpper();

  // Try to force text to have no anti aliasing. This may not be
  // possible, so a check is made later on to determine if the data
  // needs to be stretched to force it into a non anti aliased state.
  QFont font;
  font.setPixelSize(textHt);
  font.setStyleStrategy(QFont::NoAntialias);
  QFontMetrics metric(font);
  int fontHeight = metric.height();

  // There are 8 pixels between the bar scale and the edge of the
  // image, 4 pixels between the bar scale and the text, and 4 pixels
  // between the text and the edge of the image. This accounts for the
  // 16 extra pixels used in the totalHeight calculation.
  int totalHeight = barHeight + fontHeight + 16;

  // If there is no left side to the scale bar, then "0"
  // will be the text character that occurs at the left side
  // of the scale bar - set cornerSample in slightly to make
  // room for the digit "0" plus some space between the left
  // edge and the "0".
  // A starting totalWidth is calculated, but will be updated
  // to account for text on left and right sides of bar scale.
  cornerSample = (textHt + 1) / 2 + 10;
  int totalWidth = barWidth + cornerSample;
  int imgSamps = 0;

  if (padImage) {
    imgSamps = numSamps;
  }

  if (padImage) {
    if (padLocation == "CENTER") {
      cornerSample = numSamps / 2 - barWidth / 2;
    }
  }

  // Center line of text area is calculated to help in placing
  // the text display area rectangles - allow for 4 pixels above
  // and below the text
  int textCtrLine = (fontHeight + 8) / 2;
  if (textLoc == "BELOW") {
    textCtrLine = textCtrLine + barHeight + 8;
  }

  // Define rectangles to contain text at left, center, and right locations in bar scale
  QRect ctrDisplayRect;
  QRect leftDisplayRect;
  QRect rightDisplayRect;
  QString lblStr;
  int lblStrWidth;
  if (leftLimit > 0) {
    lblStr.setNum(leftLimit);
    lblStrWidth = metric.horizontalAdvance(lblStr);
    totalWidth = totalWidth + lblStrWidth / 2;
    cornerSample = cornerSample + lblStrWidth / 2;
    leftDisplayRect.setRect( ( cornerSample - lblStrWidth / 2), (textCtrLine - fontHeight / 2 - 2),
                            (lblStrWidth + 10), (fontHeight + 8) );
  }
  lblStrWidth = metric.horizontalAdvance("0");
  ctrDisplayRect.setRect( (cornerSample + leftLimit * scaleUnit / resolution + .5 - lblStrWidth / 2),
                         (textCtrLine - fontHeight / 2 - 2), (lblStrWidth + 10), (fontHeight + 8) );

  lblStr.setNum(rightLimit);
  QString unitStr;
  if (units == "KILOMETER") {
    unitStr = "KILOMETER";
    if (rightLimit > 1) {
      unitStr = "KILOMETERS";
    }
  }
  else {
    unitStr = "METER";
    if (rightLimit > 1) {
      unitStr = "METERS";
    }
  }
  lblStr = lblStr + " " + unitStr;
  lblStrWidth = metric.horizontalAdvance(lblStr);
  totalWidth = totalWidth + lblStrWidth + (textHt + 1) / 2 + 10;
  rightDisplayRect.setRect( (barWidth + cornerSample - lblStrWidth / 2),
                           (textCtrLine - fontHeight / 2 - 2),
                           (lblStrWidth + 30), (fontHeight + 8) );

  // Make sure text does not overlap
  if (leftLimit > 0) {
    if (leftDisplayRect.right() > ctrDisplayRect.left() ) {
      int diff = leftDisplayRect.right() - ctrDisplayRect.left();
      leftDisplayRect.setRect( (leftDisplayRect.left() - diff), leftDisplayRect.top(),
                              leftDisplayRect.width(), leftDisplayRect.height() );
    }
  }
  if (ctrDisplayRect.right() > rightDisplayRect.left() ) {
    int diff = ctrDisplayRect.right() - rightDisplayRect.left();
    rightDisplayRect.setRect( (rightDisplayRect.left() + diff), rightDisplayRect.top(),
                             rightDisplayRect.width(), rightDisplayRect.height());
  }

  // Define total width of bar scale including the text. Add 6 pixels of padding on
  // each side.
  if (leftLimit > 0) {
    totalWidth = rightDisplayRect.right() - leftDisplayRect.left() + 12;
  }
  else {
    totalWidth = rightDisplayRect.right() - ctrDisplayRect.left() + 12;
  }

  if (padImage) {
    if (padLocation == "RIGHT") {
      cornerSample = numSamps - 10 - totalWidth;
    }
  }

  if (textLoc == "ABOVE") {
    cornerLine = totalHeight - 8;
  }
  else {
    cornerLine = barHeight + 8;
  }

  // Set up the image that will contain the bar scale.
  QImage barScaleQImage;
  if (totalWidth > imgSamps) {
    barScaleQImage = QImage(totalWidth, totalHeight, QImage::Format_RGB32);
  }
  else {
    barScaleQImage = QImage(imgSamps, totalHeight, QImage::Format_RGB32);
  }
  if (backGround == "WHITE") {
    barScaleQImage.fill(qRgb(255, 255, 255) );
  }
  else {
    barScaleQImage.fill(qRgb(0, 0, 0) );
  }

  QPainter painter(&barScaleQImage);

  painter.setRenderHint(QPainter::Antialiasing, false);

  QPen pen;
  if (backGround == "WHITE") {
    pen.setColor(qRgb(0, 0, 0) );
  }
  else {
    pen.setColor(qRgb(255, 255, 255) );
  }
  pen.setStyle(Qt::SolidLine);
  pen.setWidth(1);
  painter.setPen(pen);

  // Need to keep track of location of vertical division
  // lines so horizontal lines can be drawn without
  // overshooting them
  QVector<int> vertLine;
  vertLine.push_back(cornerSample);

  // Draw outline of the bar scale
  QPoint pt1(cornerSample, cornerLine);
  QPoint pt2( (barWidth + cornerSample), cornerLine);
  for (int i = 0; i < bndLine; i++) {
    pt1.setY(cornerLine - i);
    pt2.setY(cornerLine - i);
    painter.drawLine(pt1, pt2);
  }
  pt1.setY(cornerLine);
  pt2.setY(cornerLine - barHeight + 1);
  for (int i = 0; i < bndLine; i++) {
    pt1.setX(cornerSample + i);
    pt2.setX(cornerSample + i);
    painter.drawLine(pt1, pt2);
  }
  pt1.setX(barWidth + cornerSample);
  pt2.setX(cornerSample);
  for (int i = 0; i < bndLine; i++) {
    pt1.setY(cornerLine - barHeight + 1 + i);
    pt2.setY(cornerLine - barHeight + 1 + i);
    painter.drawLine(pt1, pt2);
  }
  pt1.setY(cornerLine - barHeight + 1);
  pt2.setY(cornerLine);
  for (int i = 0; i < bndLine; i++) {
    pt1.setX(barWidth + cornerSample + i);
    pt2.setX(barWidth + cornerSample +i);
    painter.drawLine(pt1, pt2);
  }
  // Draw center line of barscale (only for left limit)
  if (leftLimit > 0.0) {
    vertLine.insert(1, (cornerSample + leftLimit * scaleUnit / resolution + .5) );
    pt1.setY(cornerLine - barHeight + 1);
    pt2.setY(cornerLine);
    for (int i = 0; i < bndLine; i++) {
      pt1.setX(cornerSample + leftLimit * scaleUnit / resolution + .5 + i);
      pt2.setX(cornerSample + leftLimit * scaleUnit / resolution + .5 + i);
      painter.drawLine(pt1, pt2);
    }
  }
  // Draw segment marks in left and right sides of bar scale
  if (leftLimit > 0.0 && leftSegs > 0) {
    double ticSpace = (leftLimit * scaleUnit / resolution) / leftSegs;
    pt1.setY(cornerLine - barHeight + 1);
    pt2.setY(cornerLine);
    for (int i = 1; i < leftSegs; i++) {
      vertLine.insert(i, (cornerSample + ticSpace * i) );
      for (int j = 0; j < bndLine; j++) {
        pt1.setX(cornerSample + ticSpace * i + j);
        pt2.setX(cornerSample + ticSpace * i + j);
        painter.drawLine(pt1, pt2);
      }
    }
  }
  if (rightSegs > 0) {
    double ticSpace = (rightLimit * scaleUnit / resolution) / rightSegs;
    pt1.setY(cornerLine - barHeight + 1);
    pt2.setY(cornerLine);
    for (int i = 1; i < rightSegs; i++) {
      vertLine.insert(vertLine.size(),
                      (cornerSample + leftLimit * scaleUnit / resolution + ticSpace * i) );
      for (int j = 0; j < bndLine; j++) {
        pt1.setX(cornerSample + leftLimit * scaleUnit / resolution + ticSpace * i + j);
        pt2.setX(cornerSample + leftLimit * scaleUnit / resolution + ticSpace * i + j);
        painter.drawLine(pt1, pt2);
      }
    }
  }

  vertLine.push_back(cornerSample + barWidth);
  // Draw horizontal measurement lines
  int vertLineIdx = 0;
  for (int i = 0; i < leftSegs; i+=2) {
    pt1.setX(vertLine[i]);
    pt2.setX(vertLine[i + 1]);
    vertLineIdx = i + 2;
    for (int j = 0; j < midLine; j++) {
      pt1.setY(cornerLine - bndLine - botSpace - j);
      pt2.setY(cornerLine - bndLine - botSpace - j);
      painter.drawLine(pt1, pt2);
    }
  }
  int startTic = 0;
  if ( ( (leftSegs % 2)* 2) != leftSegs && (leftSegs % 2) != 0) {
    startTic = 1;
  }
  for (int i = startTic; i < rightSegs; i+=2) {
    pt1.setX(vertLine[vertLineIdx]);
    pt2.setX(vertLine[vertLineIdx + 1]);
    vertLineIdx = vertLineIdx + 2;
    for (int j = 0; j < midLine; j++) {
      pt1.setY(cornerLine - bndLine - botSpace - j);
      pt2.setY(cornerLine - bndLine - botSpace - j);
      painter.drawLine(pt1, pt2);
    }
  }

  // Write text that goes with the bar scale
  font.setStyleStrategy(QFont::NoAntialias);
  painter.setFont(font);
  painter.setLayoutDirection(Qt::LeftToRight);
  if (leftLimit > 0) {
    lblStr.setNum(leftLimit);
    lblStrWidth = metric.horizontalAdvance(lblStr);
    painter.drawText(leftDisplayRect, lblStr);
  }
  lblStr.setNum(0);
  lblStrWidth = metric.horizontalAdvance(lblStr);
  painter.drawText(ctrDisplayRect, lblStr);
  lblStr.setNum(rightLimit);
  lblStr = lblStr + " " + unitStr;
  painter.drawText(rightDisplayRect, lblStr);

  QString inFile = ui.GetCubeName("FROM");
  QString outFile = ui.GetFileName("TO");
  FileName tmpBarFile = FileName::createTempFile("barscale.tif");
  QString scaleTif = tmpBarFile.expanded();
  tmpBarFile = FileName::createTempFile("barscale.cub");
  QString scaleCub = tmpBarFile.expanded();
  barScaleQImage.save(scaleTif, "TIFF", 100);

  // Convert bar scale to ISIS cube
  QString parameters = "FROM=" + scaleTif + " TO=" + scaleCub + " MODE=GRAYSCALE";
  ProgramLauncher::RunIsisProgram("std2isis", parameters);

  // When this program is run from the command line, it cannot alter the default
  // font used by the Qt Gui interface. As a result, it is possible to get a font
  // that is antialiased. The following code stretches the antialiased font so that
  // it is no longer antialiased.
  Cube tmpCube;
  tmpCube.open(scaleCub, "r");
  Statistics *stats = tmpCube.statistics(1);
  tmpBarFile = FileName::createTempFile("barscalestr.cub");
  QString scaleStrCub = tmpBarFile.expanded();
  if (stats->ValidPixels() > 0) {
    parameters = "FROM=" + scaleCub + " TO=" + scaleStrCub + " NULLMIN=0 NULLMAX=130" +
                 " HRSMIN=131 HRSMAX=255";
  }
  else {
    parameters = "FROM=" + scaleCub + " TO=" + scaleStrCub + " NULLMIN=0 NULLMAX=0" +
                 " HRSMIN=255 HRSMAX=255";
  }
  delete stats;
  ProgramLauncher::RunIsisProgram("specpix", parameters);

  // Need to mosaic the bar scale on top of the image if requested
  if (!padImage) {
    // Paste bar scale onto image
    for (int i = 1; i <= numBands; i++) {
      if (i == 1) {
        parameters = "FROM=" + inFile + " MOSAIC=" + outFile + " PRIORITY=ONTOP OUTSAMPLE=1" +
                     " OUTLINE=1" + " OUTBAND=1" + " MATCHBANDBIN=NO CREATE=YES" +
                     " NSAMPLES=" + toString(numSamps) + " NLINES=" + toString(numLines) +
                     " NBANDS=" + toString(numBands);
        ProgramLauncher::RunIsisProgram("handmos", parameters);
      }
      parameters = "FROM=" + scaleStrCub + " MOSAIC=" + outFile + " PRIORITY=ONTOP OUTSAMPLE=" +
                   toString(placeSample) + " OUTLINE=" + toString(placeLine) + " OUTBAND=" +
                   toString(i) + " MATCHBANDBIN=NO NULL=YES HIGHSATURATION=YES";
      ProgramLauncher::RunIsisProgram("handmos", parameters);
    }
  }

  // Need to pad the image with bar scale if requested
  if (padImage) {
    parameters = "FROM=" + inFile + " TO=" + outFile + " BOTTOM=" + toString(totalHeight);
    ProgramLauncher::RunIsisProgram("pad", parameters);
    for (int i = 1; i <= numBands; i++) {
      parameters = "FROM=" + scaleStrCub + " MOSAIC=" + outFile + " PRIORITY=ONTOP OUTSAMPLE=1" +
                   " OUTLINE=" + toString(numLines+1) + " OUTBAND=" + toString(i) +
                   " MATCHBANDBIN=NO " + " NULL=YES HIGHSATURATION=YES";
      ProgramLauncher::RunIsisProgram("handmos", parameters);
    }
  }

  // Remove the temporary files
  QFile::remove(scaleTif);
  QFile::remove(scaleCub);
  QFile::remove(scaleStrCub);
}
