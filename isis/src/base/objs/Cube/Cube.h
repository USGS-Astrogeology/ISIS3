#ifndef Cube_h
#define Cube_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <vector>

// This is needed for the QVariant macro
#include <QMetaType>
#include <QList>
#include <QMap>

#include <nlohmann/json.hpp>

#include "Endian.h"
#include "PixelType.h"
#include "PvlKeyword.h"

class QFile;
class QMutex;
class QString;

namespace Isis {
  class Blob;
  class Buffer;
  class Camera;
  class CubeAttributeOutput;
  class CubeCachingAlgorithm;
  class CubeStretch;
  class FileName;
  class ImageIoHandler;
  class Projection;
  class Pvl;
  class PvlGroup;
  class Statistics;
  class Table;
  class Histogram;
  class History;
  class OriginalLabel;
  class OriginalXmlLabel;
  class ImagePolygon;


  /**
   * @brief IO Handler for Isis Cubes.
   *
   * This class is used to open, create, read, and write data from Isis cube
   * files.
   *
   * @ingroup LowLevelCubeIO
   *
   * @author 2003-02-14 Jeff Anderson
   *
   * @internal
   *   @todo There are undocumented methods and private variables in the Cube
   *         class. Also, it needs an example.
   *   @history 2003-03-31 Jeff Anderson - updated unitTest truth file.
   *   @history 2003-05-16 Stuart Sides  - modified schema from
   *                           astrogeology...isis.astrogeology
   *   @history 2003-05-21 Jeff Anderson - added read/write methods to improve
   *                           speed for IsisLine buffer managers. Also added error checks on reads,
   *                           writes, and seeks.
   *   @history 2003-05-30 Jeff Anderson - added PhysicalBand method and updated unitTest.
   *   @history 2003-10-06 Jeff Anderson - added PVL to create method to allow for propagation of
   *                           Pixel specifications.
   *   @history 2003-10-15 Jeff Anderson - fixed bad error check in PhysicalBand method.
   *   @history 2004-01-30 Jeff Anderson - added many Set methods. Major refractor of the class.
   *   @history 2004-02-12 Jeff Anderson - added pruning of the band bin group given the virtual
   *                           band list.
   *   @history 2004-02-17 Jeff Anderson - modified the number of bands in the label if a virtual
   *                           band list was given.
   *   @history 2004-03-01 Jeff Anderson - added ability to read Isis 2.1 cube format.
   *   @history 2004-04-01 Jeff Anderson - fixed bug swapping NULL cache tile for non-native cubes
   *                           in move method.
   *   @history 2005-03-24 Jeff Anderson - Added methods to return a camera or projection associated
   *                           with the cube.
   *   @history 2005-07-18 Elizabeth Ribelin - Fixed bug in method that returns a projection
   *                           associated with the cube
   *   @history 2005-10-03 Elizabeth Miller - Added error check to Write(blob)
   *   @history 2006-02-24 Kris Becker - Made the destructor virtual to properly allow the class to
   *                           be inherited.
   *   @history 2006-04-21 Jacob Danton - Modified the WriteLabel method to use the Pvl's format
   *                           template abilities.
   *   @history 2006-05-17 Elizabeth Miller - Depricated CameraManger to CameraFactory
   *   @history 2006-07-10 Elizabeth Miller - Added max size preference
   *   @history 2006-11-28 Jeff Anderson - Fixed detached blob bug
   *   @history 2007-01-05 Jeff Anderson - Fixed bug when reading/writing outside
   *                           cube
   *   @history 2007-02-07 Tracie Sucharski - Added ReOpen method
   *   @history 2008-05-09 Steven Lambright - Added Statistics, Histogram, PutGroup, GetGroup,
   *                           DeleteGroup, HasGroup conveinience methods. Removed excess references
   *                           to CubeInfo.
   *   @history 2008-05-27 Jeff Anderson - Fixed bug in open method where virtual bands were not
   *                           handled correctly if a BandBin group did not exist
   *   @history 2008-06-09 Christopher Austin - Improved maximum cube size error message.
   *   @history 2008-06-11 Steven Lambright - Changed prototype for SetVirtualBands
   *   @history 2008-06-18 Christopher Austin - Fixed documentation errors
   *   @history 2008-08-11 Steven Lambright - Added another Statistics method which accepts a valid
   *                           range -- also added another Histogram method which accepts a
   *                           bin/valid range (applies both).
   *   @history 2008-08-11 Steven Lambright - Fixed definition of IsisCubeDef, problem pointed out
   *                           by "novas0x2a" (Support Board Member)
   *   @history 2008-12-15 Steven Koechle - Added a method to delete blobs from a cube
   *   @history 2008-12-17 Steven Koechle - BlobDelete method was broken, fixed
   *   @history 2009-06-30 Steven Lambright - Added "HasProjection" for uniform
   *                           projection existance test
   *   @history 2010-03-22 Steven Lambright - Added a mutex for reading and writing,
   *                           which makes these methods thread safe.
   *   @history 2011-03-23 Steven Lambright - Added ClearCache method.
   *   @history 2011-06-01 Jai Rideout and Steven Lambright - Updated API to conform to new Isis
   *                           standards, re-implemented IO handler and implemented new caching
   *                           algorithms that can automatically apply themselves when the behavior
   *                           of the caller changes.
   *   @history 2011-06-27 Steven Lambright - Added addCachingAlgorithm
   *   @history 2011-07-08 Steven Lambright - projection() and camera() will now return NULL if the
   *                           cube is not open, instead of crashing, references #161
   *   @history 2012-02-17 Steven Lambright - Made the read() method const. Added mutex lock calls
   *                           around data file accesses.
   *   @history 2012-04-05 Stuart Sides and Steven Lambright - Added new
   *                          constructor which immediately open the cube.
   *   @history 2012-07-02 Steven Lambright and Stuart Sides - Added copy() and added support for
   *                           external cube label files (.ecub). This was done so that cubes can
   *                           be imported into projects without copying the DN data. Also, multiple
   *                           versions of SPICE will be save-able for one cube (multiple ecubs...
   *                           again, not copying DN data). Fixes #961.
   *   @history 2012-09-06 Steven Lambright - Improved parsing of ^DnFile in the ecub format and
   *                           simplified/optimized writeLabels().
   *   @history 2012-09-17 Steven Lambright - Fixed ASSERT in getRealDataFileName() to be in the
   *                           correct place.
   *   @history 2012-10-26 Steven Lambright and Stuard Sides - Externel cube label files no longer
   *                           allocate disk space for DN data when writing out blobs.
   *   @history 2012-11-06 Steven Lambright and Mathew Eis - Fixed (the lack of) deletion of
   *                           temporary files in the unit test. Fixes #1009.
   *   @history 2012-12-31 Steven Lambright - Removed 'get' prefix from accessors. Fixes #1356.
   *   @history 2014-06-20 Ian Humphrey - Added checks to statistics() and histogram() to throw
   *                           an error if the cube is unopened. Fixes #2085.
   *   @history 2014-10-06 Ian Humphrey - Added case to unittest when chunk dimensions are the same
   *                           as the buffer shape to ensure virtual bands accessed correctly.
   *                           Added cases to test creating bsq and large bsq cubes.
   *                           References #1689.
   *   @history 2015-01-30 Ian Humphrey - Deallocated copied cubes in unittest. References #2082.
   *   @history 2015-06-22 Makayla Shepherd - Using putGroup() a group on the label of a read-only
   *                           cube now throws an error.
   *   @history 2015-09-02 Tyler Wilson - Commented out first call to SetValidRange
   *                           in the call to the
   *                           historgram function. Also commented out a call to
   *                           Histogram::SetBinRange() because this function no longer
   *                           exists in Histogram.  See Ref. #2188.
   *   @history 2017-06-08 Chris Combs - Made "Failed to create" error messages more descriptive.
   *                           Fixes #833.
   *   @history 2017-09-22 Cole Neubauer - Fixed documentation. References #4807
   *   @history 2018-01-04 Tracie Sucharski - Allow relative paths that are not "." in the DnFile
   *                           keyword for ecubs. Changes to ::open to set m_dataFileName for ecubs,
   *                           and changed ::realDataFileName() to return the absolute path if it is
   *                           relative. Changed ::realDataFileLabel to call realDataFileName to
   *                           make sure we get absolute path.  Fixes #5276.
   *   @history 2018-01-18 Summer Stapleton - Updated error message in ::create() to address when
   *                           an IsisPreference file cannot be found. Fixes #5145.
   *   @history 2018-11-16 Jesse Mapel - Made several methods virtual for mocking.
   *   @history 2019-06-15 Kristin Berry - Added latLonRange method to return the valid lat/lon rage of the cube. The values in the mapping group are not sufficiently accurate for some purposes.
   *   @history 2021-02-17 Jesse Mapel - Added hasBlob method to check for any type of BLOB.
   *   @history 2021-10-18 Evin Dunn - Switch to single quotes for 'Environment and Preferences' in Cube::create() exception
   */
  class Cube {
    public:
      Cube();
      Cube(const FileName &fileName, QString access = "r");

      virtual ~Cube();

      /**
       * These are the possible storage formats of ISIS cubes. There is an
       *   internal IO handler for each one of these.
       */
      enum Format {
        /**
         * Cubes are stored in band-sequential format, that is the order of the
         *   pixels in the file (on disk) is:
         *   <pre>
         *     S1,L1,B1
         *     S2,L1,B1
         *     and so on until ...
         *     SN,L1,B1
         *     S1,L2,B1
         *     and so on until ...
         *     S1,LN,B1
         *     S1,L1,B2
         *     S2,L1,B2
         *     and so on until ...
         *     S1,L1,BN
         *   </pre>
         *
         * The idea is the cubes are stored left-to-right, top-to-bottom, then
         *   front-to-back.
         */
        Bsq,
        /**
         * Cubes are stored in tile format, that is the order of the
         *   pixels in the file (on disk) is BSQ within a given sub-area
         *   defined by the Tile IO handler. Typically these tiles are around
         *   1MB for efficiency.
         *
         * The idea is the cubes are stored left-to-right, top-to-bottom inside
         *   the tiles (which have 1 band). The tiles are themselves in BSQ
         *   order also. Please note that this can cause cubes to be larger
         *   on disk due to the tile size not being evenly divisible into the
         *   cube size.
         *
         * Cube:
         * <pre>
         * ------------------------------
         * |Tile *Tile *Tile *Tile *Tile|*
         * |  0  *  1  *  2  *  3  *  4 |*
         * |     *     *     *     *    |*
         * |****************************|*
         * |Tile *Tile *Tile *Tile *Tile|*
         * |  5  *  6  *  7  *  8  *  9 |*
         * |     *     *     *     *    |*
         * |****************************|*
         * |Tile *Tile *Tile *Tile *Tile|*
         * ------------------------------*
         * * 10  * 11  * 12  * 13  * 14  *
         * *******************************
         * </pre>
         *
         * The symbol '*' denotes tile boundaries.
         * The symbols '-' and '|' denote cube boundaries.
         */
        Tile,
        GTiff
      };

      /**
       * @brief Input cube label type tracker
       *
       * This enumeration and its functions are for the label
       * type of an input cube. The enum defines the type of labels (i.e.,
       * Both the label and cube are in the same file and the label is in a
       * separate file from the cube.
       */
      enum LabelAttachment {
        AttachedLabel,  //!< The input label is embedded in the image file
        DetachedLabel,  //!< The input label is in a separate data file from the image
        /**
         * The label is pointing to an external DN file - the label is also external to the data.
         *
         * This format implies that the output is a cube that contains everything except DN data
         *   (more similar to attached than detached).
         */
        ExternalLabel,
        GdalLabel
      };

      void fromIsd(const FileName &fileName, Pvl &label, nlohmann::json &isd, QString access);
      void fromIsd(const FileName &fileName, FileName &labelFile, FileName &isdFile, QString access);

      void fromLabel(const FileName &fileName, Pvl &label, QString access);

      bool isOpen() const;
      bool isProjected() const;
      bool isReadOnly() const;
      bool isReadWrite() const;
      LabelAttachment labelsAttached() const;

      void attachSpiceFromIsd(nlohmann::json Isd);
      void attachLineScanTableFromIsd(nlohmann::json isd);

      void close(bool remove = false);
      Cube *copy(FileName newFile, const CubeAttributeOutput &newFileAttributes);
      void create(const QString &cfile);
      void create(const QString &cfile, const CubeAttributeOutput &att);
      void createTiff(QString &dataFileName);
      void open(const QString &cfile, QString access = "r");
      void reopen(QString access = "r");

      void read(Blob &blob,
                const std::vector<PvlKeyword> keywords = std::vector<PvlKeyword>()) const;
      void read(Buffer &rbuf) const;
      OriginalLabel readOriginalLabel(const QString &name="IsisCube") const;
      CubeStretch readCubeStretch(QString name="CubeStretch",
                                  const std::vector<PvlKeyword> keywords = std::vector<PvlKeyword>()) const;
      OriginalXmlLabel readOriginalXmlLabel() const;
      History readHistory(const QString &name = "IsisCube") const;
      ImagePolygon readFootprint() const;
      Table readTable(const QString &name);
      void write(Blob &blob, bool overwrite=true);
      void write(const Table &table);
      void write(const CubeStretch &cubeStretch);
      void write(OriginalLabel &lab);
      void write(const OriginalXmlLabel &lab);
      void write(History &history, const QString &name = "IsisCube");
      void write(const ImagePolygon &polygon);
      void write(Buffer &wbuf);

      void setBaseMultiplier(double base, double mult);
      void setMinMax(double min, double max);
      void setByteOrder(ByteOrder byteOrder);
      void setDimensions(int ns, int nl, int nb);
      void setExternalDnData(FileName cubeFileWithDnData);
      void setFormat(Format format);
      void setLabelsAttached(LabelAttachment attached);
      // void setLabelsExternal(bool external);
      void setLabelSize(int labelBytes);
      void setPixelType(PixelType pixelType);
      void setVirtualBands(const QList<QString> &vbands);
      void setVirtualBands(const std::vector<QString> &vbands);

      void relocateDnData(FileName dnDataFile);
//       static void relocateDnData(FileName externalLabelFile, FileName dnDataFile);

      virtual int bandCount() const;
      double base() const;
      ByteOrder byteOrder() const;
      Camera *camera();
      FileName externalCubeFileName() const;
      virtual QString fileName() const;
      Format format() const;
      virtual Histogram *histogram(const int &band = 1,
                                        QString msg = "Gathering histogram");
      virtual Histogram *histogram(const int &band, const double &validMin,
                                        const double &validMax,
                                        QString msg = "Gathering histogram");
      Pvl *label() const;
      int labelSize(bool actual = false) const;
      int lineCount() const;
      double multiplier() const;
      PixelType pixelType() const;
      virtual int physicalBand(const int &virtualBand) const;
      Projection *projection();
      int sampleCount() const;
      Statistics *statistics(const int &band = 1,
                             QString msg = "Gathering statistics");
      Statistics *statistics(const int &band, const double &validMin,
                             const double &validMax,
                             QString msg = "Gathering statistics");

      void addCachingAlgorithm(CubeCachingAlgorithm *);
      void clearIoCache();
      bool deleteBlob(QString BlobName, QString BlobType);
      void deleteGroup(const QString &group);
      PvlGroup &group(const QString &group) const;
      bool hasGroup(const QString &group) const;
      bool hasTable(const QString &name);
      bool hasBlob(const QString &name, const QString &type);
      void putGroup(const PvlGroup &group);
      void latLonRange(double &minLatitude, double &maxLatitude, double &minLongitude,
                       double &maxLongitude);
      FileName realDataFileName() const;

    private:
      void applyVirtualBandsToLabel();
      void cleanUp(bool remove);

      void construct();
      QFile *dataFile() const;
      GDALDataset *gdalDataset() const;

      void initialize();
      void initCoreFromLabel(const Pvl &label);
      void initCoreFromGdal(const QString &labelFile);
      void initLabelState(const Pvl &label);
      void initLabelFromFile(FileName labelFileName, bool readWrite);
      void openCheck();
      Pvl realDataFileLabel() const;
      void reformatOldIsisLabel(const QString &oldCube);
      void writeLabels();

    private:
      /**
       * This is the file that contains the labels always; if labels are
       *   attached then this contains the file data also. The method
       *   dataFile() will always give you the appropriate QFile member for
       *   reading cube data; this should always be used for the labels.
       *
       * If isOpen() is true, then this is allocated.
       */
      QFile *m_labelFile;

      /**
       * This is only sometimes allocated. This is used for when IsOpen is true
       *   and labels are detached so the QFile for the labels does not give
       *   us cube data.
       */
      QFile *m_dataFile;

      /**
       * TODO: Write description
       */
      GDALDataset *m_geodataSet = nullptr;

      /**
       * This does the heavy lifting for cube DN IO and is always allocated
       *   when isOpen() is true.
       */
      ImageIoHandler *m_ioHandler;

      /**
       * The byte order of the opened cube; if there is no open cube then
       *   this is the byte order that will be used when a new cube is created.
       *   Defaults to the OS's byte order.
       */
      ByteOrder m_byteOrder;

      /**
       * If isOpen() then this is the IO format that the cube uses. If there is
       *   no file opened then this is the IO format that will be used if a
       *   cube is created (using create(...)). This defaults to Tile.
       */
      Format m_format;

      /**
       * This is the pixel type on disk. If a cube is open, then this will be
       *   the opened cube's pixel type. Otherwise, if a cube is created with
       *   create(...) then this pixel type will be used. The default is Real.
       */
      PixelType m_pixelType;

      //! Basic thread-safety mutex; this class is not optimized for threads.
      QMutex *m_mutex;

      //! Camera allocated from the camera() method.
      Camera *m_camera;

      //! Projection allocated from the projection() method.
      Projection *m_projection;

      //! The full filename of the label file (.lbl or .cub)
      FileName *m_labelFileName;

      //! The full filename of the data file (.cub)
      FileName *m_dataFileName;

      /**
       * If open was called with an Isis 2 cube, then this will be
       *   the name of the imported ISIS cube. m_labelFileName and
       *   m_dataFileName will store the Isis 2 cube's information.
       */
      FileName *m_tempCube;

      //! Label pvl format template file (describes how to format labels)
      FileName *m_formatTemplateFile;

      //! True if labels are attached
      LabelAttachment m_attached;

      //! True if labels are external
      bool m_external;

      /**
       * True (most common case) when the cube DN data is inside the file we're writing to. False
       *   means we're referencing another cube's internal DN data for reading, and writing buffers
       *   is disallowed.
       */
      bool m_storesDnData;

      //! The label if IsOpen(), otherwise NULL
      Pvl *m_label;

      //! The maximum allowed size of the label; the allocated space.
      int m_labelBytes;

      //! The sample count of the open cube or the cube that will be created
      int m_samples;

      //! The line count of the open cube or the cube that will be created
      int m_lines;

      //! The band count of the open cube or the cube that will be created
      int m_bands;

      /**
       * The base of the open cube or the cube that will be created; does not
       *   apply if m_pixelType is Real.
       */
      double m_base;

      /**
       * The multiplier of the open cube or the cube that will be created; does
       *   not apply if m_pixelType is Real.
       */
      double m_multiplier;

      //! If allocated, converts from physical on-disk band # to virtual band #
      QList<int> *m_virtualBandList;
  };
}

//! This allows Cube *'s to be stored in a QVariant.
Q_DECLARE_METATYPE(Isis::Cube *);

#endif
