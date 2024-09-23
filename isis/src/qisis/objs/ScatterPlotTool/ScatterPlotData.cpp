#include "ScatterPlotData.h"

#include <algorithm>

#include <QtCore>

#include <qwt_interval.h>
#include <qwt_text.h>

#include "Brick.h"
#include "SpecialPixel.h"

namespace Isis {
  /**
   * ScatterPlotDataConstructor
   *
   * @param xCube The x-axis cube
   * @param xCubeBand The x-axis cube's band to get DN values from
   * @param xBinCount The resolution of the x-axis
   * @param yCube The y-axis cube
   * @param yCubeBand The y-axis cube's band to get DN values from
   * @param yBinCount The resolution of the y-axis
   * @param sampleRange The sample range to gather the histogram from, this is
   *                    the same for the x cube and y cube.
   * @param lineRange The line range to gather the histogram from, this is
   *                  the same for the x cube and y cube.
   */
  ScatterPlotData::ScatterPlotData(
      Cube *xCube, int xCubeBand, int xBinCount,
      Cube *yCube, int yCubeBand, int yBinCount,
      QwtInterval sampleRange, QwtInterval lineRange) : QwtMatrixRasterData(),
      m_xDnToBinStretch(new Stretch), m_yDnToBinStretch(new Stretch),
      m_counts(
        new QVector< QVector<int> >(yBinCount, QVector<int>(xBinCount))),
      m_alarmedBins(new QMap<int, bool>) {
    int startLine = qRound(lineRange.minValue());
    int endLine = qRound(lineRange.maxValue());

    ImageHistogram *xCubeHist = new ImageHistogram(*xCube, xCubeBand, NULL,
        sampleRange.minValue(), lineRange.minValue(),
        sampleRange.maxValue(), lineRange.maxValue(),
        xBinCount, true);
    m_xCubeMin = xCubeHist->Minimum();
    m_xCubeMax = xCubeHist->Maximum();


    ImageHistogram *yCubeHist = new ImageHistogram(*yCube, yCubeBand, NULL,
        sampleRange.minValue(), lineRange.minValue(),
        sampleRange.maxValue(), lineRange.maxValue(),
        yBinCount, true);
    m_yCubeMin = yCubeHist->Minimum();
    m_yCubeMax = yCubeHist->Maximum();


    m_xDnToBinStretch->AddPair(m_xCubeMin, 0);
    m_xDnToBinStretch->AddPair(m_xCubeMax, xBinCount - 1);

    m_yDnToBinStretch->AddPair(m_yCubeMin, 0);
    m_yDnToBinStretch->AddPair(m_yCubeMax, yBinCount - 1);

    m_maxCount = 0;

    Brick brick1((int)(sampleRange.maxValue() - sampleRange.minValue() + 1),
                 1, 1, xCube->pixelType());
    Brick brick2((int)(sampleRange.maxValue() - sampleRange.minValue() + 1),
                 1, 1, yCube->pixelType());

    for (int line = startLine; line <= endLine; line++) {
      brick1.SetBasePosition(qRound(sampleRange.minValue()), line, xCubeBand);
      xCube->read(brick1);

      brick2.SetBasePosition(qRound(sampleRange.minValue()), line, yCubeBand);
      yCube->read(brick2);

      for (int i = 0; i < brick1.size(); i++) {
        double xDn = brick1[i];
        double yDn = brick2[i];

        if (!IsSpecial(xDn) && !IsSpecial(yDn)) {
          double x = m_xDnToBinStretch->Map(xDn);
          double y = m_yDnToBinStretch->Map(yDn);

          if (!IsSpecial(x) && !IsSpecial(y)) {
            int roundedX = qRound(x);
            int roundedY = qRound(y);

            if (roundedX >= 0 && roundedX < xBinCount &&
                roundedY >= 0 && roundedY < yBinCount) {
              int value = (*m_counts)[roundedY][roundedX] + 1;
              (*m_counts)[roundedY][roundedX] = value;

              m_maxCount = qMax(m_maxCount, value);
            }
          }
        }
      }
    }

    setInterval(Qt::XAxis, QwtInterval(m_xCubeMin, m_xCubeMax));
    setInterval(Qt::YAxis, QwtInterval(m_yCubeMin, m_yCubeMax));
    setInterval(Qt::ZAxis, QwtInterval(0, m_maxCount));
  }


  /**
   * This is an optimized copy constructor. Copies are not zero-time, but they
   *   are pretty quick.
   *
   * @param other The ScatterPlotData to copy.
   */
  ScatterPlotData::ScatterPlotData(const ScatterPlotData &other) {
    m_xDnToBinStretch.reset(new Stretch(*other.m_xDnToBinStretch));
    m_yDnToBinStretch.reset(new Stretch(*other.m_yDnToBinStretch));
    m_counts.reset(new QVector< QVector<int> >(*other.m_counts));
    m_alarmedBins.reset(new QMap< int, bool>(*other.m_alarmedBins));
    m_maxCount = other.m_maxCount;
    m_xCubeMin = other.m_xCubeMin;
    m_xCubeMax = other.m_xCubeMax;
    m_yCubeMin = other.m_yCubeMin;
    m_yCubeMax = other.m_yCubeMax;
  }


  /**
   * Destructor
   *
   */
  ScatterPlotData::~ScatterPlotData() {
  }


  /**
   * Returns a copy of the ScatterPlotData object.
   *
   * @return A new instance this this class
   */
  QwtRasterData *ScatterPlotData::copy() const {
    return new ScatterPlotData(*this);
  }


  /**
   * This gets called every time the scatter plot is re-drawn. This returns the
   * counts for each DN (x), DN (y), or if the bin is alarmed this returns the
   *   max count.
   *
   * @param x The X-Dn value
   * @param y The Y-Dn value
   * @return The bin's colorizable-value
   */
  double ScatterPlotData::value(double x, double y) const {
    double value = 0;

    QPair<int, int> indices = binXYIndices(x, y);
    int index = binIndex(indices.first, indices.second);

    if (index != -1 && (*m_alarmedBins)[index])
      value = m_maxCount;
    else
      value = binCount(indices.first, indices.second);

    return value;
  }


  /**
   * Return the min DN value for the x-axis cube's data range.
   *
   * @return x axis cube data min DN value
   */
  double ScatterPlotData::xCubeMin() const {
    return m_xCubeMin;
  }


  /**
   * Return the max DN value for the y-axis cube's data range.
   *
   * @return x axis cube data max DN value
   */
  double ScatterPlotData::xCubeMax() const {
    return m_xCubeMax;
  }


  /**
   * Return the min DN value for the y-axis cube's data range.
   *
   * @return y axis cube data min DN value
   */
  double ScatterPlotData::yCubeMin() const {
    return m_yCubeMin;
  }


  /**
   * Return the max DN value for the y-axis cube's data range.
   *
   * @return x axis cube data max DN value
   */
  double ScatterPlotData::yCubeMax() const {
    return m_yCubeMax;
  }


  /**
   * This is part of the copy-and-swap paradigm. Swap member data with other.
   *
   * @param other The class to trade member data with
   */
  void ScatterPlotData::swap(ScatterPlotData &other) {
    m_xDnToBinStretch.swap(other.m_xDnToBinStretch);
    m_yDnToBinStretch.swap(other.m_yDnToBinStretch);
    m_counts.swap(other.m_counts);
    m_alarmedBins.swap(other.m_alarmedBins);
    std::swap(m_maxCount, other.m_maxCount);
    std::swap(m_xCubeMin, other.m_xCubeMin);
    std::swap(m_xCubeMax, other.m_xCubeMax);
    std::swap(m_yCubeMin, other.m_yCubeMin);
    std::swap(m_yCubeMax, other.m_yCubeMax);
  }


  /**
   * Get the center X/Y Dn values for the bin at index.
   *
   * @param index The bin to get the center X/Y DN Values for
   *
   * @return The center DN value of the given bin
   */
  QPair<double, double> ScatterPlotData::binXY(int index) const {
    QPair<int, int> indices = binXYIndices(index);
    int xIndex = indices.first;
    int yIndex = indices.second;

    if (xIndex != -1 && yIndex != -1) {
      int xSize = 0;
      int ySize = m_counts->size();

      // Assume square 2D structurs
      if (ySize > 0)
        xSize = (*m_counts)[0].size();

      double percentAcrossXRange = ((double)xIndex / (double)xSize);
      double xDnValue = m_xCubeMin +
          percentAcrossXRange * (m_xCubeMax - m_xCubeMin);

      double percentAcrossYRange = ((double)yIndex / (double)ySize);
      double yDnValue = m_yCubeMin +
          percentAcrossYRange * (m_yCubeMax - m_yCubeMin);

      return QPair<double, double>(xDnValue, yDnValue);
    }


    std::string msg = "Bin at index [" + toString(index) + "] not found. "
                  "There are [" + toString(numberOfBins()) + "] bins";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  /**
   * Get the count (number of values) which fall into the bin at index.
   *
   * @param binIndex The bin we're getting the data from
   * @return The total number of cube DNs which fall into the given bin
   */
  int ScatterPlotData::binCount(int binIndex) const {
    QPair<int, int> indices = binXYIndices(binIndex);
    return binCount(indices.first, indices.second);
  }


  /**
   * Get the total number of bins (bin count in x * bin count in y).
   *
   * @return The total number of bins
   */
  int ScatterPlotData::numberOfBins() const {
    int xSize = 0;
    int ySize = m_counts->size();

    // Assume square 2D structurs
    if (ySize > 0)
      xSize = (*m_counts)[0].size();

    return xSize * ySize;
  }


  /**
   * Get a list of all of the x-bin center values for this scatter plot.
   *
   * @return A set of discrete x values, useful for things like fitting a line
   */
  QVector<double> ScatterPlotData::discreteXValues() const {
    QVector<double> xValues;

    int ySize = m_counts->size();
    if (ySize) {
      int xSize = (*m_counts)[0].size();
      xValues.resize(xSize);

      for (int xIndex = 0; xIndex < xSize; xIndex++) {
        double percentAcrossXRange = ((double)xIndex / (double)xSize);
        xValues[xIndex] = m_xCubeMin +
            percentAcrossXRange * (m_xCubeMax - m_xCubeMin);
      }
    }

    return xValues;
  }


  /**
   * Alarm the bin (highlight it) at the given x/y DN value. This is for
   *   viewport->plot alarming.
   *
   * @param x The x-dn value of the bin to alarm
   * @param y The y-dn value of the bin to alarm
   */
  void ScatterPlotData::alarm(double x, double y) {
    int binToAlarm = binIndex(x, y);
    if (binToAlarm != -1)
      (*m_alarmedBins)[binToAlarm] = true;
  }


  /**
   * Forget all alarmed bins (viewport->plot).
   */
  void ScatterPlotData::clearAlarms() {
    m_alarmedBins->clear();
  }


  /**
   * This is a hint given to qwt for how to render a pixel in the spectrogram.
   *
   * @param area This is ignored
   * @return The size of a pixel in the scatter plot
   */
  QRectF ScatterPlotData::pixelHint(const QRectF &area) const {
    QRectF hint;

    if (m_xDnToBinStretch->Pairs() > 1 && m_yDnToBinStretch->Pairs() > 1) {
      hint = QRectF(
          QPointF(m_xCubeMin, m_yCubeMin),
          QSizeF(m_xDnToBinStretch->Input(1) - m_xDnToBinStretch->Input(0),
                 m_yDnToBinStretch->Input(1) - m_yDnToBinStretch->Input(1)));
    }

    return hint;
  }


  /**
   * Take the data from other and copy it into this. This uses the copy-and-swap
   *   paradigm for exception safety.
   *
   * @param other The scatter plot data which needs copied to this
   * @return This returns *this.
   */
  ScatterPlotData &ScatterPlotData::operator=(const ScatterPlotData &other) {
    ScatterPlotData tmp(other);

    swap(tmp);

    return *this;
  }


  /**
   * Get the count (number of values) which fall into the bin at xIndex, yIndex.
   *
   * @param xIndex The x index of the bin we're getting our data from
   * @param yIndex The y index of the bin we're getting our data from
   * @return The total number of cube DNs which fall into the given bin
   */
  int ScatterPlotData::binCount(int xIndex, int yIndex) const {
    int count = 0;

    if (yIndex >= 0 && yIndex < m_counts->size()) {
      if (xIndex >= 0 && xIndex < (*m_counts)[yIndex].size()) {
        count = (*m_counts)[yIndex][xIndex];
      }
    }

    return count;
  }


  /**
   * Get the single-index position given an x/y index position. That is, get a
   *   1D (flat) index from a 2D (x/y based) index.
   *
   * @param xIndex The x index of the bin we're translating into a 1D index
   * @param yIndex The y index of the bin we're translating into a 1D index
   * @return The 1D (flat) index position
   */
  int ScatterPlotData::binIndex(int xIndex, int yIndex) const {
    int xSize = 0;
    int ySize = m_counts->size();

    // Assume square 2D structurs
    if (ySize > 0)
      xSize = (*m_counts)[0].size();

    int index = -1;
    if ((xIndex >= 0 && xIndex < xSize) && (yIndex >= 0 && yIndex < ySize))
      index = xSize * yIndex + xIndex;

    return index;
  }


  /**
   * Get the single-index position given an x/y dn value.
   *
   * @param x The x-dn value of the bin
   * @param y The y-dn value of the bin
   * @return The 1D (flat) index position
   */
  int ScatterPlotData::binIndex(double x, double y) const {
    QPair<int, int> indices = binXYIndices(x, y);
    return binIndex(indices.first, indices.second);
  }


  /**
   * Get the 2D index index position given a 1D (flat) index position.
   *
   * @param binIndex The 1D (flat) index of the bin
   * @return The x/y 2D index position
   */
  QPair<int, int> ScatterPlotData::binXYIndices(int binIndex) const {
    int xSize = 0;
    int ySize = m_counts->size();

    // Assume square 2D structurs
    if (ySize > 0)
      xSize = (*m_counts)[0].size();

    int yIndex = (binIndex / xSize);
    binIndex -= yIndex * xSize;

    int xIndex = binIndex;

    if (xIndex < 0 || yIndex < 0 || xIndex >= xSize || yIndex >= ySize) {
      std::string msg = "Bin at index [" + toString(binIndex) + "] not found. "
                    "There are [" + toString(numberOfBins()) + "] bins";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return QPair<int, int>(xIndex, yIndex);
  }


  /**
   * Get the 2D (x/y) index position given an x/y dn value.
   *
   * @param x The x-dn value of the bin
   * @param y The y-dn value of the bin
   * @return The 2D (x/y) bin index position
   */
  QPair<int, int> ScatterPlotData::binXYIndices(double x, double y) const {
    QPair<int, int> indices(-1, -1);

    if (m_counts->size() && m_counts->at(0).size()) {
      double xBinPosition = m_xDnToBinStretch->Map(x);
      double yBinPosition = m_yDnToBinStretch->Map(y);

      if (!IsSpecial(xBinPosition) && !IsSpecial(yBinPosition)) {
        int discreteXBin = qRound(xBinPosition);
        int discreteYBin = qRound(yBinPosition);

        if (discreteXBin >= 0 && discreteXBin < m_counts->at(0).size() &&
            discreteYBin >= 0 && discreteYBin < m_counts->size()) {
          indices = QPair<int, int>(discreteXBin, discreteYBin);
        }
      }
    }

    return indices;
  }
}
