#ifndef BufferManager_h
#define BufferManager_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Constants.h"
#include "PixelType.h"
#include "Buffer.h"

namespace Isis {
  /**
   * @brief Manages a Buffer over a cube.
   *
   * This class is used to manage a Buffer over a cube. Recall a Buffer is
   * simply a 3-d shape (or subset) of a cube. For example, a line, tile, or
   * spectra are some possible shapes. A buffer manager will walk the shape
   * over the entire cube to ensure every pixel is accessed. When constructing
   * a BufferManager, arguments for the cube size and buffer size are required.
   * For example, construction for a (100 sample, 200 line, 2 band) cube
   * accessed by a line shape would require the shape buffer to have 100
   * samples, 1 line, and 1 band. The manager would then access the lines
   * sequentially in the first band and then proceed to the second band. A 100
   * sample, 1 line, and 2 band shape buffer would access each line but both
   * bands simultaneously. Typically, a BufferManager is not instantiated
   * directly but is used in a derived class such a Line or Tile.
   *
   * If you would like to see BufferManager being used in implementation,
   * see the LineManager, BoxcarManager, or TileManager class.
   *
   * @ingroup LowLevelCubeIO
   *
   * @author 2003-02-01 Jeff Anderson
   *
   * @internal
   *   @history 2003-05-16 Stuart Sides - modified schema from
   *                                      astrogeology...isis.astrogeology
   *   @history 2003-06-02 Jeff Anderson - Modified setpos method to allow for
   *                                       speedy reverse direction management.
   *   @history 2005-05-23 Jeff Anderson - Modified to support 2GB+ files
   *   @history 2007-12-04 Christopher Austin - Added option to
   *            constructor to change the order of the progression
   *            through the cube
   *   @history 2008-06-18 Christopher Austin - Fixed documenation errors
   *   @history 2012-02-24 Steven Lambright - Optimized setpos() and made it
   *                           public.
   *   @history 2017-08-30 Summer Stapleton - Updated documentation. References #4807.
   */
  class BufferManager : public Isis::Buffer {

    public:
      //  Constructors and Destructors
      BufferManager();
      BufferManager(int maxsamps, int maxlines, int maxbands,
                    int bufsamps, int buflines, int bufbands,
                    Isis::PixelType type, bool reverse = false,
                    double scale = 1.0);
      BufferManager(const BufferManager &other);

      //! Destroys the BufferManager object
      ~BufferManager() {};

      // Traversal Methods

      /**
       * Moves the shape buffer to the next position. Returns true if the next
       * position is valid.
       *
       * @return bool
       */
      bool operator++(int) {
        return (next());
      }


      /**
       * Moves the shape buffer by a certain amount. Returns a pointer to the BufferManager
       * it was called on.
       * 
       * @param i How far to shift the shape buffer
       * 
       * @return BufferManager
       */
      BufferManager &operator+=(int i) {
        setpos(p_currentMap + i);
        return *this;
      }

      /**
       * Moves the shape buffer to the first position
       *
       * @return bool
       */
      bool begin() {
        return (setpos(0));
      }

      /**
       * Moves the shape buffer to the next position. Returns true if the next
       * position is valid.
       *
       * @return bool
       */
      bool next() {
        return (setpos(p_currentMap + 1));
      }

      /**
       * Returns true if the shape buffer has accessed the end of the cube.
       *
       * @return bool
       */
      bool end() const {
        return (p_currentMap >= p_nmaps);
      }

      bool setpos(BigInt map);

      void swap(BufferManager &other);

      BufferManager &operator=(const BufferManager &rhs);

    protected:

      //  Methods visable to deriving classes

      /**
       * Returns the number of samples in the cube
       *
       * @return int
       */
      int MaxSamples() const {
        return (p_maxSamps);
      }

      /**
       * Returns the number of lines in the cube
       *
       * @return int
       */
      int MaxLines() const {
        return (p_maxLines);
      }

      /**
       * Returns the number of bands in the cube
       *
       * @return int
       */
      int MaxBands() const {
        return (p_maxBands);
      }

      /**
       * Returns the maximum number of positions the shape buffer needs to cover
       * the entire image (see setpos method for more info).
       *
       * @return int
       */
      BigInt MaxMaps() const {
        return (p_nmaps);
      }

      void SetIncrements(const int sinc, const int linc, const int binc);
      void SetOffsets(const int soff, const int loff, const int boff);

    private:
      int p_maxSamps;  //!<  Maximum samples to map
      int p_maxLines;  //!<  Maximum lines to map
      int p_maxBands;  //!<  Maximum bands to map

      int p_sinc;            //!<  Sample increment
      int p_linc;            //!<  Line increment
      int p_binc;            //!<  Band increment

      int p_soff;            //!<  Sample offset
      int p_loff;            //!<  Line offset
      int p_boff;            //!<  Band offset

      int p_currentSample;   //!<  Current sample
      int p_currentLine;     //!<  Current line
      int p_currentBand;     //!<  Current band

      BigInt p_nmaps;           //!<  Total number of objects to map
      BigInt p_currentMap;      //!<  Current buffer map position

      /**
      * If true the axies are processed in Band, Line, Sample order
      * (e.g., BIL). If left false, the axies are  processed in the
      * Sample, Line, Band order (e.g., BSQ, BIP).
      */
      bool p_reverse;
  };
};

#endif

