/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CubeDataThreadTester.h"

#include <iostream>

#include <QMap>
#include <QMutex>
#include <QReadWriteLock>
#include <QPair>
#include <QVector>

#include "CubeDataThread.h"
#include "IString.h"
#include "FileName.h"
#include "Brick.h"
#include "Cube.h"

using namespace std;
using namespace Isis;

namespace Isis {
  /**
   * This initializes a CubeDataThreadTester. The CubeDataThread given is the
   * subject of the tests.
   *
   * @param testObject The CubeDataThread instance to be tested
   */
  CubeDataThreadTester::CubeDataThreadTester(CubeDataThread *testObject) :
    QThread() {
    p_cachedDoneBricks = NULL;
    p_cubeDataThread = testObject;
    p_numTestsDone = 0;
    p_execStarted = false;
    p_notifyDone = true;

    p_cachedDoneBricks = new QVector< QPair<int, const Isis::Brick *> >;

    start();
    moveToThread(this);
  }


  /**
   * This connects this class' signals and slots with CubeDataThread's signals
   * and slots.
   */
  void CubeDataThreadTester::Connect() {
    connect(this,
            SIGNAL(RequestReadCube(int, int, int, int, int, int, void *, double)),
            p_cubeDataThread,
            SLOT(ReadCube(int, int, int, int, int, int, void *, double)));

    connect(this,
            SIGNAL(RequestReadWriteCube(int, int, int, int, int, int, void *)),
            p_cubeDataThread,
            SLOT(ReadWriteCube(int, int, int, int, int, int, void *)));

    connect(this, SIGNAL(NotifyDoneWithData(int, const Isis::Brick *)),
            p_cubeDataThread, SLOT(DoneWithData(int, const Isis::Brick *)));

    connect(p_cubeDataThread, SIGNAL(ReadReady(void *, int, const Isis::Brick *)),
            this, SLOT(ReadBrick(void *, int, const Isis::Brick *)));

    connect(p_cubeDataThread, SIGNAL(ReadWriteReady(void *, int, Isis::Brick *)),
            this, SLOT(ReadWriteBrick(void *, int, Isis::Brick *)));
  }


  /**
   * This cleans up the cube data thread.
   *
   */
  CubeDataThreadTester::~CubeDataThreadTester() {
    if(!p_execStarted) {
      QThread::yieldCurrentThread();
    }

    QThread::exit(0);

    wait();
  }

  //! returns the cube id
  void CubeDataThreadTester::run() {
    p_execStarted = true;
    exec();
  }

  /**
   * This tests a basic read. This performs 1 test.
   *
   * @param cubeId The identifier given by the data thread for the file to test
   */
  void CubeDataThreadTester::ReadCubeTest(int cubeId) {
    cout << "=============== Testing Basic Read ===============" << endl;
    emit RequestReadCube(cubeId, 1, 1, 2, 2, 1, this, 1.0);
  }


  /**
   * This tests two basic reads with no conflicts. This performs 2 tests.
   *
   * @param cubeId1 Cube for first read
   * @param cubeId2 Cube for second read
   */
  void CubeDataThreadTester::ReadCubeTest2(int cubeId1, int cubeId2) {
    cout << "=============== Testing Multiple Non-Conflicting Cube Reads " <<
         "===============" << endl;

    p_notifyDone = false;
    emit RequestReadCube(cubeId1, 1, 1, 3, 2, 1, this, 1.0);
    emit RequestReadCube(cubeId2, 1, 2, 3, 2, 1, this, 1.0);
  }


  /**
   * This tests an overlapping read. This performs 2 tests.
   *
   * @param cubeId The identifier given by the data thread for the file to test
   */
  void CubeDataThreadTester::ReadCubeTest3(int cubeId) {
    cout << "=============== Testing Exact Overlap Cube Reads ==============="
         << endl << endl;

    p_notifyDone = false;
    emit RequestReadCube(cubeId, 1, 2, 2, 2, 1, this, 1.0);
    emit RequestReadCube(cubeId, 1, 2, 2, 2, 1, this, 1.0);
  }


  /**
   * This tests a basic write. This performs 2 tests.
   *
   * @param cubeId The identifier given by the data thread for the file to test
   */
  void CubeDataThreadTester::WriteCubeTest(int cubeId) {
    cout << "=============== Testing Basic R/W ===============" << endl << endl;
    emit RequestReadWriteCube(cubeId, 1, 1, 2, 2, 1, this);
    emit RequestReadCube(cubeId, 1, 1, 2, 2, 1, this, 1.0);
  }


  /**
   * This tests two non-conflicting writes. This performs 2 tests.
   *
   * @param cubeId1 Cube for first write
   * @param cubeId2 Cube for second write
   */
  void CubeDataThreadTester::WriteCubeTest2(int cubeId1, int cubeId2) {
    cout << "=============== Testing Multiple Non-Conflicting Cube R/W " <<
         "===============" << endl << endl;

    p_notifyDone = false;
    emit RequestReadWriteCube(cubeId1, 1, 1, 3, 1, 1, this);
    emit RequestReadWriteCube(cubeId2, 1, 1, 3, 1, 1, this);
  }


  /**
   * This tests two conflicting* writes. This causes a deadlock!
   *
   * This performs a test, deadlocks, and once the deadlock is broken (via
   *   WriteCubeTest3BreakDeadlock), finishes another test.
   *
   * @param cubeId The identifier given by the data thread for the file to test
   */
  void CubeDataThreadTester::WriteCubeTest3(int cubeId) {
    cout << "=============== Testing Conflicting Cube R/W ==============="
         << endl;

    p_notifyDone = false;
    emit RequestReadWriteCube(cubeId, 1, 1, 3, 1, 1, this);
    emit RequestReadWriteCube(cubeId, 1, 1, 3, 1, 1, this);
  }


  /**
   * This test breaks the deadlock caused by the third write test
   *
   */
  void CubeDataThreadTester::WriteCubeTest3BreakDeadlock() {
    cout << "  Breaking Deadlock From Test 3" << endl;
    while(!p_cachedDoneBricks->size()) {
      msleep(100);
    }

    if(p_cachedDoneBricks->size()) {
      cout << "  Notify done with first brick" << endl;
      emit NotifyDoneWithData((*p_cachedDoneBricks)[0].first,
                              (*p_cachedDoneBricks)[0].second);
      p_cachedDoneBricks->clear();
    }
  }


  /**
   * This test tests this automatic change notifications. This performs 2 tests.
   *
   * @param cubeId The identifier given by the data thread for the file to test
   */
  void CubeDataThreadTester::NotifyChangeTest(int cubeId) {
    cout << "=============== Testing Change Notification ==============="
         << endl;

    connect(p_cubeDataThread,
            SIGNAL(BrickChanged(int, const Isis::Brick *)),
            this,
            SLOT(BrickChanged(int, const Isis::Brick *))
           );

    p_cubeDataThread->AddChangeListener();
    emit RequestReadWriteCube(cubeId, 5, 1, 5, 1, 1, this);
  }


  /**
   * This is called when a brick is read.
   *
   * @param requester Pointer to requesting class
   * @param cubeId Cube identifier
   * @param data Brick of data read
   */
  void CubeDataThreadTester::ReadBrick(void *requester, int cubeId,
                                       const Isis::Brick *data) {
    cout << "  CubeDataThreadTester::ReadBrick" << endl;

    cout << "    Requester is me? " << ((this == requester) ? "Yes" : "No")
         << endl;

    if(this != requester) return;

    cout << "    Data:" << endl;

    for(int i = 0; i < data->size(); i++) {
      if(i == 0) cout << "      ";
      if(i % 6 == 6 - 1 && i != data->size() - 1) {
        cout << data->at(i) << endl << "      ";
      }
      else if(i == data->size() - 1) {
        cout << data->at(i) << endl;
      }
      else {
        cout << data->at(i) << "\t";
      }
    }

    cout << endl;
    if(p_notifyDone) {
      cout << "  Notify done with this brick" << endl;
      emit NotifyDoneWithData(cubeId, data);

      if(p_cachedDoneBricks->size()) {
        cout << "  Notify done with first brick" << endl;
        emit NotifyDoneWithData((*p_cachedDoneBricks)[0].first,
                                (*p_cachedDoneBricks)[0].second);
        p_cachedDoneBricks->clear();
      }
    }
    else {
      QPair<int, const Brick *> cache;
      cache.first = cubeId;
      cache.second = data;

      p_cachedDoneBricks->push_back(cache);
    }

    p_notifyDone = true;
    p_numTestsDone ++;
  }

  /**
   * This is called when a brick is given for R/W.
   *
   * @param requester Pointer to requesting class
   * @param cubeId Cube identifier
   * @param data Brick of data read
   */
  void CubeDataThreadTester::ReadWriteBrick(void *requester, int cubeId,
      Isis::Brick *data) {
    cout << "  CubeDataThreadTester::ReadWriteBrick" << endl;

    // This was a nice idea, but has race conditions that are difficult
    //   at best to resolve.

    //cout << "    Managed Bricks in Memory = " <<
    //  p_cubeDataThread->BricksInMemory() << endl;

    cout << "    Changing Brick : Index 0 Becoming 5" << endl;
    cout << endl;

    cout << "    Old Data: " << endl;

    for(int i = 0; i < data->size(); i++) {
      if(i == 0) cout << "      ";
      if(i % 6 == 6 - 1 && i != data->size() - 1) {
        cout << data->at(i) << endl << "      ";
      }
      else if(i == data->size() - 1) {
        cout << data->at(i) << endl;
      }
      else {
        cout << data->at(i) << "\t";
      }
    }

    (*data)[0] = 5;

    cout << "    New Data: " << endl;

    for(int i = 0; i < data->size(); i++) {
      if(i == 0) cout << "      ";
      if(i % 6 == 6 - 1 && i != data->size() - 1) {
        cout << data->at(i) << endl << "      ";
      }
      else if(i == data->size() - 1) {
        cout << data->at(i) << endl;
      }
      else {
        cout << data->at(i) << "\t";
      }
    }

    cout << endl;

    if(p_notifyDone) {
      cout << "  Notify done with this brick" << endl;
      emit NotifyDoneWithData(cubeId, data);

      if(p_cachedDoneBricks->size()) {
        cout << "  Notify done with first brick" << endl;
        emit NotifyDoneWithData((*p_cachedDoneBricks)[0].first,
                                (*p_cachedDoneBricks)[0].second);
        p_cachedDoneBricks->clear();
      }
    }
    else {
      QPair<int, const Brick *> cache;
      cache.first = cubeId;
      cache.second = data;

      p_cachedDoneBricks->push_back(cache);
    }

    p_notifyDone = true;
    p_numTestsDone ++;
  }


  /**
   * This is called when a brick is written.
   *
   * @param cubeId Cube identifier
   * @param data Brick of data read
   */
  void CubeDataThreadTester::BrickChanged(int cubeId, const Isis::Brick *data) {
    cout << "  CubeDataThreadTester::BrickChanged" << endl;
    cout << "    Data:" << endl;

    for(int i = 0; i < data->size(); i++) {
      if(i == 0) cout << "      ";
      if(i % 6 == 6 - 1 && i != data->size() - 1) {
        cout << data->at(i) << endl << "       ";
      }
      else if(i == data->size() - 1) {
        cout << data->at(i) << endl;
      }
      else {
        cout << data->at(i) << "\t";
      }
    }

    p_numTestsDone ++;
    emit NotifyDoneWithData(cubeId, data);
  }
};
