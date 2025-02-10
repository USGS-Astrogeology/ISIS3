/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "BufferManager.h"

#include <iostream>
#include <string>

#include "IException.h"

using namespace std;

namespace Isis {
  BufferManager::BufferManager() {
  }


  /**
   * Constructs a BufferManager object.
   *
   * @param maxsamps Maximum samples to map
   *
   * @param maxlines Maximum lines to map
   *
   * @param maxbands Maximum bands to map
   *
   * @param bufsamps Number of samples in the shape buffer
   *
   * @param buflines Number of lines in the shape buffer
   *
   * @param bufbands Number of bands in the shape buffer
   *
   * @param type Type of pixel in raw buffer
   *
   * @param reverse Modifies the order of progression this
   *             buffer takes through the cube.  By default,
   *             progresses samples first, then lines, then bands.
   *             If reverse = true, then the buffer progresses
   *             bands first, then lines, then samples.
   */
  BufferManager::BufferManager(int maxsamps, int maxlines,
                               int maxbands, int bufsamps,
                               int buflines, int bufbands,
                               Isis::PixelType type, bool reverse, 
                               double scale) :
                                   Isis::Buffer(bufsamps, buflines, bufbands, type, scale),
                                   p_maxSamps(maxsamps), p_maxLines(maxlines),
                                   p_maxBands(maxbands) {
    SetIncrements(bufsamps, buflines, bufbands);
    p_reverse = reverse;
  }


  /**
   * Constructs a BufferManager object.
   *
   * @param &other Reference to a BufferManager to copy
   *
   */
  BufferManager::BufferManager(const BufferManager &other) :
      Buffer(other),
      p_maxSamps(other.p_maxSamps), p_maxLines(other.p_maxLines),
      p_maxBands(other.p_maxBands), p_sinc(other.p_sinc), p_linc(other.p_linc),
      p_binc(other.p_binc), p_soff(other.p_soff), p_loff(other.p_loff),
      p_boff(other.p_boff), p_currentSample(other.p_currentSample),
      p_currentLine(other.p_currentLine), p_currentBand(other.p_currentBand),
      p_nmaps(other.p_nmaps), p_currentMap(other.p_currentMap),
      p_reverse(other.p_reverse) {
  }


  /**
   * Swaps the values of this BufferManager with that of another
   *
   * @param &other Reference to a BufferManager to swap with
   *
   */
  void BufferManager::swap(BufferManager &other) {
    std::swap(p_maxSamps, other.p_maxSamps);
    std::swap(p_maxLines, other.p_maxLines);
    std::swap(p_maxBands, other.p_maxBands);
    std::swap(p_sinc, other.p_sinc);
    std::swap(p_linc, other.p_linc);
    std::swap(p_binc, other.p_binc);
    std::swap(p_soff, other.p_soff);
    std::swap(p_loff, other.p_loff);
    std::swap(p_boff, other.p_boff);
    std::swap(p_currentSample, other.p_currentSample);
    std::swap(p_currentLine, other.p_currentLine);
    std::swap(p_currentBand, other.p_currentBand);
    std::swap(p_nmaps, other.p_nmaps);
    std::swap(p_currentMap, other.p_currentMap);
    std::swap(p_reverse, other.p_reverse);
  }


  /**
   * Creates a new BufferManager with the same values as another
   *
   * @param &rhs Reference to a BufferManager to swap with
   * 
   * @return BufferManager
   */
  BufferManager &BufferManager::operator=(const BufferManager &rhs) {
    BufferManager copy(rhs);
    swap(copy);
    return *this;
  }


  /**
   * Sets how the shape is incremented through the cube. By default (if this
   * method is not invoked) shapes are moved sequentially through the cube with
   * no overlap. For example, assume a 3 sample by 3 line tile buffer. It would
   * first move across the image from left-to-right starting at sample 1, line 1,
   * and band 1. Upon an increment it would be positioned at sample 4, line 1,
   * and band 1. Each successive increment would cause the sample position to
   * increase by 3 until it exceeds the number of samples in the cube. At that
   * point the shape buffer would be positioned at sample 1, line 4, and band 1
   * (effectively moving to the next row of tiles). The shape would then continue
   * moving across the cube until it reaches the edge again. Then the next row of
   * tiles would be accessed until the shape reached the bottom of the cube. This
   * default management can be overridden using this method. For example, by
   * setting the increments to (6,3,1) we effectively skip every other tile. By
   * setting them to (6,6,1) we skip every other tile and every other row of
   * tiles.  By setting them to (1,1,1) we essentially have NSxNLxNB positions
   * in the cube and the 3x3 tile is managed such that the top left corner
   * of the tile is moved over by 1 sample until it reaches the end of the
   * line then down 1 line and so on until the end of the cube is reached.
   *
   * @param sinc Sample increment
   *
   * @param linc Line increment
   *
   * @param binc Band increment
   */
  void BufferManager::SetIncrements(const int sinc, const int linc,
                                    const int binc) {
    p_sinc = sinc;
    p_linc = linc;
    p_binc = binc;

    p_soff = 0;
    p_loff = 0;
    p_boff = 0;

    p_currentSample = p_currentLine = p_currentBand = 1;
    p_currentMap = 0;

    p_nmaps = ((BigInt)((p_maxSamps - 1) / p_sinc + 1) *
               (BigInt)((p_maxLines - 1) / p_linc + 1) *
               (BigInt)((p_maxBands - 1) / p_binc + 1));
  }


  /**
   * Sets the offset of the buffer. By default (if this method is not invoked)
   * the offsets are (0,0,0). Offsets are applied when computing the top-left
   * corner of the shape buffer. When used in conjunction with the SetIncrements
   * method, this allows for centering shape buffers around a pixel when passing
   * in negative offsets. For example,with a 3x3x1 shape and offsets of (-1,-1,0)
   * and increments of (1,1,1) would cause the manager to walk a 3x3 buffer
   * through the entire image. Setting the manager position the beginning causes
   * the 3x3 window to be positioned such that sample 1, line 1 of the cube would
   * be at the center of the window and increment would cause sample 2, line 1 to
   * be at the center of the window. Successive increments will move the window in
   * the sample direction until the end of line is reached at which time the
   * buffer would be centered on sample 1, line 2.
   *
   * @param soff Sample offset
   *
   * @param loff Line offset
   *
   * @param boff Band offset
   */
  void BufferManager::SetOffsets(const int soff, const int loff,
                                 const int boff) {
    p_soff = soff;
    p_loff = loff;
    p_boff = boff;
  }


  /**
   * Sets the position of the shape in the cube.  This shape fits
   * into the cube a specific number of times.
   *
   * When p_reverse is left false: (default)
   * For example, a line shape on a 100 sample, 200 line, and 2
   * band cube would have 200*2 or 400 different positions as
   * there are 400 total lines in the cube. Performing setpos(0)
   * would position the shape at sample 1, line 1, and band 1.
   * While setpos(200) would position the shape at sample 1, line
   * 1, and band 2. Finally, setpos(399) would position the shape
   * at sample 1, line 400, and band 2. Setpos returns true if it
   * was sucessfully in setting the position,  and false if the
   * shape is at the end of the cube (beyond index 399 in the case
   * of our example).
   *
   * When p_reverse is set to true:
   * Following the above cube with 100 samples, 200 lines, and 2 bands, performing the
   * setpos(0) would still position the shape at sample 1, line
   * 1, and band 1.  However, setpos(1) would position the shape
   * at sample 1, line 1, band 2, while setpos(200) would position
   * the spame at sample 1, line 100, band 1.  Setpos returns true
   * of it was sucessfully in setting the position, and false if
   * the shape is at the end of the cube(beyond final index).
   *
   * @param map Shape buffer position value
   *
   * @return bool True or False depending on whether the shape is at the end of
   *              the cube or not.
   *
   * @throws Isis::iException::Programmer - Invalid value for map argument
   */
  bool BufferManager::setpos(BigInt map) {
    if(map >= 0) {
      p_currentMap = map;

      if(!p_reverse) {
        int sampDimension = (p_maxSamps / p_sinc);
        if (p_maxSamps % p_sinc)
          sampDimension++;

        p_currentSample = (map % sampDimension) * p_sinc + 1;
        map /= sampDimension;

        int lineDimension = (p_maxLines / p_linc);
        if (p_maxLines % p_linc)
          lineDimension++;

        p_currentLine = (map % lineDimension) * p_linc + 1;
        map /= lineDimension;

        p_currentBand = map * p_binc + 1;
      }
      else {
        int bandDimension = (p_maxBands / p_binc);
        if (p_maxBands % p_binc)
          bandDimension++;

        p_currentBand = (map % bandDimension) * p_binc + 1;
        map /= bandDimension;

        int lineDimension = (p_maxLines / p_linc);
        if (p_maxLines % p_linc)
          lineDimension++;

        p_currentLine = (map % lineDimension) * p_linc + 1;
        map /= lineDimension;

        p_currentSample = map * p_sinc + 1;
      }

      SetBasePosition(p_currentSample + p_soff,
                      p_currentLine + p_loff,
                      p_currentBand + p_boff);
    }
    else {
      string message = "Invalid value for argument [map]";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    return !end();
  }
} // end namespace isis
