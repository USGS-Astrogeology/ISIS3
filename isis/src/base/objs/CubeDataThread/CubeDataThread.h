#ifndef CubeDataThread_h
#define CubeDataThread_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <QThread>

template<typename T> class QList;

template<typename A, typename B> struct QPair;
template<typename A, typename B> class QMap;

class QReadWriteLock;
class QMutex;

namespace Isis {
  class Cube;
  class FileName;
  class Brick;
  class UniversalGroundMap;

  /**
   * @brief Encapsulation of Cube I/O with Change Notifications
   *
   * The main purpose of this class is to create cube change notifications,
   * ideal for GUI's with changing cubes. This class is designed to encapsulate
   * Cube I/O into a separate thread and serializes each cube I/O. This class
   * also speeds up I/O by reusing bricks instead of always reading from the
   * disk.
   *
   * This is not a full concurrency control/transaction handler. Consistent
   * states are not guaranteed, though a consistent state for any given brick
   * is, and results from reads do not guaranteed serial equivalence. Deadlocks
   * are possible, if two processes want to R/W to the same sections of data,
   * and it is up to the users of this class to avoid such conditions.
   *
   * @author 2010-01-15 Steven Lambright
   *
   * @internal
   *   @history 2010-04-12 Eric Hyer - Added check for valid cube ID's for
   *                           slots ReadCube and ReadWriteCube.
   *   @history 2010-07-29 Eric Hyer - AcquireLock now flushes events for this
   *                           thread instead of the main eventloop's thread.
   *   @history 2010-08-12 Steven Lambright - Fixed memory leak and simplified
   *                           acquiring write bricks
   *   @history 2010-08-23 Eric Hyer - Added the FindCubeId method
   *   @history 2010-08-24 Eric Hyer - Added the RemoveCube method
   *   @history 2012-02-27 Jai Rideout and Steven Lambright - Made
   *                           BricksInMemory() thread-safe. Fixes #733.
   *   @history 2016-06-21 Kris Becker - Properly forward declare QPair as struct not class
   *
   *   @todo Add state recording/reverting functionality
   *
   */
  class CubeDataThread : public QThread {
      Q_OBJECT

    public:
      CubeDataThread();
      virtual ~CubeDataThread();

      int AddCube(const FileName &fileName,
                  bool mustOpenReadWrite = false);
      int AddCube(Cube *cube);

      void RemoveCube(int cubeId);

      void AddChangeListener();
      void RemoveChangeListener();

      int BricksInMemory();

      UniversalGroundMap *GetUniversalGroundMap(int cubeId) const;

      const Cube *GetCube(int cubeId) const;
      int FindCubeId(const Cube *) const;

    public slots:
      void ReadCube(int cubeId, int startSample, int startLine,
                    int endSample, int endLine, int band, void *caller, double scale);
      void ReadWriteCube(int cubeId, int startSample, int startLine,
                         int endSample, int endLine, int band, void *caller);

      void DoneWithData(int, const Isis::Brick *);

    signals:

      /**
       * This signal will be emitted when ReadCube has finished processing.
       *
       * When done with the data, given you are the requester, you call the
       * DoneWithData slot (via a signal).
       *
       * @param requester Pointer to the calling class (must ignore all
       *   ReadReady signals where this != requester)
       * @param cubeId Cube ID of the cube the data is from
       * @param data The data in the cube
       */
      void ReadReady(void *requester, int cubeId, const Isis::Brick *data);

      /**
       * This signal will be emitted when ReadWriteCube has finished processing.
       *
       * When done with the data, given you are the requester, you call the
       * DoneWithData slot (via a signal).
       *
       * @param requester Pointer to the calling class (must ignore all
       *   ReadReady signals where this != requester)
       * @param cubeId Cube ID of the cube the data is from
       * @param data The data in the cube, also where you should write changes
       */
      void ReadWriteReady(void *requester, int cubeId, Isis::Brick *data);

      /**
       * DO NOT CONNECT TO THIS SIGNAL WITHOUT CALLING AddChangeListener().
       *
       * When a write occurs, and change listeners exist, this signal is emitted
       *   with the new data.
       *
       * When done with the data, given you are the requester, you call the
       *   DoneWithData slot (via a signal).
       *
       * @param cubeId Cube ID of the change
       * @param data Area written to the cube
       */
      void BrickChanged(int cubeId, const Isis::Brick *data);

    private:
      /**
       * Assigning CubeDataThreads to eachother is bad, so this has been
       * intentionally not implemented!
       *
       * @param cdt The CubeDataThread that would be used to copy construct
       *            this CubeDataThread if this method were implemented (which
       *            its not)
       */
      CubeDataThread(const CubeDataThread &cdt);

      /**
       * Assigning CubeDataThreads to eachother is bad, so this has been
       * intentionally not implemented!
       *
       * @param rhs The right hand side CubeDataThread in the assignment
       *            operation.
       *
       * @returns would return this if it was implemented :)
       */
      const CubeDataThread &operator=(CubeDataThread rhs);

      int OverlapIndex(const Brick *initial, int cubeId,
                       int instanceNum, bool &exact);

      void GetCubeData(int cubeId, int ss, int sl, int es, int el, int band,
                       void *caller, bool sharedLock, double scale=1);

      void AcquireLock(QReadWriteLock *lockObject, bool readLock);

      bool FreeBrick(int brickIndex);

      /**
       * This is a list of the opened cubes. Since opening cubes is allowed in
       * other threads (via AddCube(...)) and is accessed with many threads, all
       * operations on this map must be serialized (non-simultaneous). The
       * p_managedCubesMutex enables this.  The bool indicates ownership.
       *
       */
      QMap< int, QPair< bool, Cube * > > * p_managedCubes;

      //! This locks the member variable p_managedCubes and p_managedData itself
      QMutex *p_threadSafeMutex;

      /**
       * This is a list of bricks in memory and their locks. The following
       * assumptions are vital or at least important for understanding this data
       * structure:
       *   1) No two bricks have the exact same area.
       *   2) Deletions may only happen in the destructor or in FreeBrick(int)
       *   3) A brick with no locks on it is available for deletion later, but
       *        may not be deleted if p_currentLocksWaiting != 0 (something is
       *        attempting a lock somewhere else).
       *   4) Bricks may overlap, but locks only pertain to exact matches.
       *   5) If you want to make an exclusive (R/W) lock on a brick, you must
       *        first make all overlapping bricks available for deletion (and/or
       *        delete)
       *   6) If you wish to make a shared (R) lock on a brick, no overlapping
       *        bricks can be write locked.
       *   7) All operations happen in this thread in order to prevent checks
       *        happening for access executing simultaneously, at least more
       *        than AcquireLock() allows.
       *   8) New bricks must be appended to the end of this list.
       *   8) Searches for conflicts must start at the beginning of this list
       *        and proceed to the end.
       *   10) No NULL or invalid pointers may exist in this list.
       */
      QList< QPair<QReadWriteLock *, Brick *> > * p_managedData;

      //! This is the associated cube ID with each brick
      QList< int > * p_managedDataSources;

      //! This is the number of shaded locks to put on a brick when changes made
      int p_numChangeListeners;

      //! This is the unique id counter for cubes
      unsigned int p_currentId;

      //! This is set to help the shutdown process when deleted
      bool p_stopping;

      /**
       * Number of locks being attempted that re-entered the event loop. As long
       * as this isn't zero, no bricks should be removed from p_managedData.
       */
      unsigned int p_currentLocksWaiting;
  };

};


#endif
