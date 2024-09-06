#ifndef ImageOverlapSet_h
#define ImageOverlapSet_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>
#include <string>

#include <QList>
#include <QThread>
#include <QMutex>

#include "geos/geom/MultiPolygon.h"
#include "geos/geom/LinearRing.h"
#include "geos/util/GEOSException.h"

#include "ImageOverlap.h"
#include "IException.h"
#include "PvlGroup.h"

namespace Isis {

  // Forward declarations
  class SerialNumberList;

  /**
   * This class is used to find the overlaps between all the images in a list of
   * serial numbers. The overlaps are created in (Lon,Lat) coordinates of
   * geos::geom::MultiPolygons. Each overlap has an associated list of serial numbers
   * which are contained in that overlap.
   *
   * @ingroup PatternMatching
   *
   * @author 2006-01-20 Stuart Sides
   *
   * @internal
   *   @history 2008-06-18 Christopher Austin - Fixed documentation
   *   @history 2008-08-18 Steven Lambright - Updated to work with geos3.0.0 instead of geos2.
   *                           Mostly namespace changes.
   *   @history 2008-11-24 Steven Lambright - Improved upon error reporting.
   *                           Added the Errors() method.
   *   @history 2008-11-25 Steven Koechle - Moved Despike Methods from ImageOverlapSet to
   *                           PolygonTools.
   *   @history 2008-12-10 Steven Koechle - Moved MakeMultiPolygon Method from ImageOverlapSet to
   *                           PolygonTools.
   *   @history 2008-12-05 Steven Lambright - Checking footprints for validity now, fixed an issue
   *                           with the intersection operator where an invalid but repairable
   *                           polygon is produced, and fixed a memory leak.
   *   @history 2008-12-15 Steven Koechle - Fixed to read new footprint blob naming scheme.
   *   @history 2009-01-06 Steven Koechle - Removed backwards compatibility for old footprint blob
   *                           name. Added a throw if footprints are invalid in an image.
   *   @history 2009-01-07 Steven Lambright & Christopher Austin - Fixed handling of Despike(...)
   *                           throwing errors on empty polygons.
   *   @history 2009-01-13 Steven Lambright - Deletes both overlaps if an intersection fails for an
   *                           unknown reason.
   *   @history 2009-01-28 Steven Lambright - Fixed memory leaks.
   *   @history 2009-03-12 Christopher Austin - Added the MULTIPOLYGON to HandleError() as the
   *                           Keyword "Polygon".
   *   @history 2009-06-01 Christopher Austin - Changed the basic algorithm to improve results.
   *   @history 2009-06-01 Steven Lambright - Multi-threaded this object. Split code into smaller
   *                           methods, now new elements are inserted next instead of appended to
   *                           the end of the overlap list, and added more error-recovery solutions.
   *   @history 2010-09-27 Christopher Austin - Added an error when no new overlaps are calculated.
   *                           (i.e. All overlaps contain only a single Serial Number).
   *   @history 2011-03-29 Steven Lambright - Added some safety around p_lonLatOverlaps to
   *                           (hopefully) get rid of a race condition.
   *   @history 2016-08-26 Kelvin Rodriguez - Added tryLock() just before unlocking mutex in
   *                           FindAllOverlaps() to avoid undefined behavior. This was done to
   *                           prevent compile errors on MAC OS 10.11.
   *   @history 2016-09-14 Marjorie Hahn - Modified FindAllOverlaps()'s check to ensure that at
   *                           least one overlap has been calculated by adding in the "foundOverlap"
   *                           boolean. References #2199.
   *   @history 2016-09-28 Jeannie Backer - Replaced deprecated IStrings with QStrings. Added mutex
   *                          unlock() calls immediately before exception calls to prevent warning
   *                          message by latest Qt library 5.7.
   *   @history 2016-12-09 Marjorie Hahn - Added mutex tryLock() calls before unlocking to prevent
   *                          undefined behavior caused by unlocking an unlocked mutex.
   *   @history 2017-05-23 Ian Humphrey - Added a tryLock() to FindAllOverlaps to prevent a
   *                           segfault from occuring on OSX with certain data. Fixes #4810.
   *
   */
  class ImageOverlapSet : private QThread {
    public:
      ImageOverlapSet(bool continueOnError = false, bool useThread = true);
      virtual ~ImageOverlapSet();

      void FindImageOverlaps(SerialNumberList &boundaries);
      void FindImageOverlaps(std::vector<QString> sns,
                             std::vector<geos::geom::MultiPolygon *> polygons);
      void FindImageOverlaps(SerialNumberList &boundaries, QString outputFile);
      void ReadImageOverlaps(const QString &filename);
      void WriteImageOverlaps(const QString &filename);

      /**
       * Returns the total number of latitude and longitude overlaps
       *
       * @return int The number of lat/lon overlaps
       */
      int Size() {
        return p_lonLatOverlaps.size();
      }

      /**
       * Returns the images which overlap at a given loverlap
       *
       * @param index The index of the overlap
       *
       * @return const ImageOverlap* The polygon and serial numbers which define the
       *         indexed overlap.
       */
      const ImageOverlap *operator[](int index) {
        return p_lonLatOverlaps[index];
      };

      std::vector<ImageOverlap *> operator[](QString serialNumber);

      //! Return the a list of errors encountered
      const std::vector<PvlGroup> &Errors() {
        return p_errorLog;
      }
    protected:
      void FindAllOverlaps(SerialNumberList *snlist = NULL);
      void AddSerialNumbers(ImageOverlap *to, ImageOverlap *from);

      //! This is a list of detailed* errors including all known information
      std::vector<PvlGroup> p_errorLog;

    private:
      //! Find overlaps is all the threaded calculate does
      void run() {
        FindAllOverlaps(p_snlist);
      }

      void DespikeLonLatOverlaps();

      QList<ImageOverlap *> p_lonLatOverlaps; //!< The list of lat/lon overlaps

      ImageOverlap *CreateNewOverlap(QString serialNumber,
                                     geos::geom::MultiPolygon *lonLatPolygon);

      bool SetPolygon(geos::geom::Geometry *poly, int position, ImageOverlap *sncopy = NULL, bool insert = false);
      void HandleError(IException &e, SerialNumberList *snlist, std::string msg = "", int overlap1 = -1, int overlap2 = -1);
      void HandleError(geos::util::GEOSException *exc, SerialNumberList *snlist, std::string msg = "", int overlap1 = -1, int overlap2 = -1);
      void HandleError(SerialNumberList *snlist, std::string msg, int overlap1 = -1, int overlap2 = -1);

      bool p_continueAfterError; //!< If false iExceptions will be thrown from FindImageOverlaps(...)
      bool p_threadedCalculate; //!< True if we want to do calculations in a threaded way
      int p_writtenSoFar; //!< The index of the last overlap that is done writing (number written-1)
      int p_calculatedSoFar; //!< The index of the last overlap that is done calculating (number calculated-1)

      //! This is used for multi-threaded calls to FindAllOverlaps only; this class never gets ownership of this pointer
      SerialNumberList *p_snlist;

      /**
       * This mutex will be used to have blocking on the write method when
       * multi-threading (instead of busy waiting), it is not intended to prevent
       * calculations and writing from happening simultaneously. Every time we have
       * new polygons this is unlocked by FindImageOverlaps(...) and re-locked by
       * WriteImageOverlaps(...).
       */
      QMutex p_calculatePolygonMutex;
      QMutex p_lonLatOverlapsMutex;
  };
};

#endif
