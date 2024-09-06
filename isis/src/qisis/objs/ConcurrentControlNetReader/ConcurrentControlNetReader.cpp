/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "ConcurrentControlNetReader.h"

#include <algorithm>
#include <iostream>

#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QString>
#include <QtConcurrentRun>
#include <QtConcurrentMap>
#include <QtCore>
#include <QTimer>

#include "Control.h"
#include "ControlNet.h"
#include "FileName.h"
#include "IString.h"
#include "Progress.h"
#include "IException.h"

using std::cerr;
using std::cout;
using std::swap;

namespace Isis {

  /**
   * Allocates memory at construction instead of as needed
   */
  ConcurrentControlNetReader::ConcurrentControlNetReader() {
    nullify();

    m_mappedRunning = false;

    m_progressBar = new ProgressBar("Reading Control Nets");
    m_watcher = new QFutureWatcher<Control *>;

    initProgress();

    connect(m_watcher, SIGNAL(finished()), this, SLOT(mappedFinished()));
  }


  /**
   * This destructor will cancel all running threads and block.
   */
  ConcurrentControlNetReader::~ConcurrentControlNetReader() {

    if (m_watcher)
    {
      m_watcher->cancel();
      m_watcher->waitForFinished();
    }

    delete m_watcher;
    m_watcher = NULL;

    delete m_progressBar;
  }


  ProgressBar *ConcurrentControlNetReader::progressBar() {
    return m_progressBar;
  }


  /**
   * @param filename The filename of the network to read
   */
  void ConcurrentControlNetReader::read(QString filename) {

    m_backlog.append(filename);

    if (!m_progressBar.isNull()) {
      m_progressBar->setRange(0, m_progressBar->maximum() + 1);
    }

    start();
  }


  /**
   * @param filenames The filenames of the networks to read
   */
  void ConcurrentControlNetReader::read(QStringList filenames) {

    m_backlog.append(filenames);

    if (!m_progressBar.isNull()) {
      m_progressBar->setRange(0, m_progressBar->maximum() + filenames.size());
    }

    start();
  }


  /**
   * Initializes members to NULL
   */
  void ConcurrentControlNetReader::nullify() {
    m_watcher = NULL;
  }


  void ConcurrentControlNetReader::initProgress() {
    if (m_progressBar) {
      m_progressBar->setVisible(false);
      m_progressBar->setRange(0, 100);
      m_progressBar->setValue(0);
    }
  }


  void ConcurrentControlNetReader::start() {

    if (!m_backlog.isEmpty() && !m_mappedRunning) {

      QList< QPair<FileName, Progress *> > functorInput;
      foreach (QString backlogFileName, m_backlog) {
        Progress *progress = new Progress;
        progress->DisableAutomaticDisplay();
        m_progress.append(progress);

        functorInput.append(qMakePair(FileName(backlogFileName.toStdString()), progress));
      }

      QFuture<Control *> networks = QtConcurrent::mapped(functorInput,
                                    FileNameToControlFunctor(QThread::currentThread()));

      try {
        networks.result();
      }
      catch (IException &e) {
        throw e;
      }

      if (!m_progressBar.isNull()) {
        m_progressBar->setVisible(true);
      }

      delete m_progressUpdateTimer;

      m_watcher->setFuture(networks);
      m_mappedRunning = true;
      m_backlog.clear();

      m_progressUpdateTimer = new QTimer;
      connect(m_progressUpdateTimer, SIGNAL(timeout()),
              this, SLOT(updateProgressValue()));
      m_progressUpdateTimer->start(100);
    }
  }


  void ConcurrentControlNetReader::updateProgressValue() {
    if (!m_mappedRunning) {
      foreach (Progress *progress, m_progress) {
        delete progress;
      }

      m_progress.clear();
    }

    int progressMin = 0;
    int progressMax = (m_progress.count() + m_backlog.count()) * 1000;
    int progressCurrent = 0;

    foreach (Progress *progress, m_progress) {
      if (progress->MaximumSteps()) {
        if (progress->CurrentStep() < progress->MaximumSteps()) {
          double progressPercent = progress->CurrentStep() / (double)progress->MaximumSteps();

          progressCurrent += qFloor(progressPercent * 1000);
        }
        else {
          progressCurrent += 1000;
        }
      }
    }

    if (m_progressBar) {
      if (progressMax > 0) {
        m_progressBar->setRange(progressMin, progressMax);
        m_progressBar->setValue(progressCurrent);
      }
      else {
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(100);
      }
    }
  }


  void ConcurrentControlNetReader::mappedFinished() {
    m_mappedRunning = false;

    delete m_progressUpdateTimer;
    updateProgressValue();

    QList<Control *> networks(m_watcher->future().results());
    emit networksReady(networks);

    if (!m_backlog.isEmpty()) {
      start();
    }
    else {
      initProgress();
    }
  }


  ConcurrentControlNetReader::FileNameToControlFunctor::FileNameToControlFunctor(
      QThread *targetThread) {
    m_targetThread = targetThread;
  }


  ConcurrentControlNetReader::FileNameToControlFunctor::FileNameToControlFunctor(
      const FileNameToControlFunctor & other) {
    m_targetThread = other.m_targetThread;
  }


  ConcurrentControlNetReader::FileNameToControlFunctor::~FileNameToControlFunctor() {
    m_targetThread = NULL;
  }


  Control * ConcurrentControlNetReader::FileNameToControlFunctor::operator()(
      const QPair<FileName, Progress *> &fileNameAndProgress) const {

    QString fileNameString = QString::fromStdString(fileNameAndProgress.first.expanded());
    Progress *progress = fileNameAndProgress.second;

    ControlNet *newCnet = new ControlNet(fileNameString, progress);
    Control *result = new Control(newCnet, fileNameString);

    newCnet->setParent(result);
    result->moveToThread(m_targetThread);

    return result;
  }


  ConcurrentControlNetReader::FileNameToControlFunctor &
      ConcurrentControlNetReader::FileNameToControlFunctor::operator=(
        const FileNameToControlFunctor &rhs) {
    m_targetThread = rhs.m_targetThread;
    return *this;
  }
}
