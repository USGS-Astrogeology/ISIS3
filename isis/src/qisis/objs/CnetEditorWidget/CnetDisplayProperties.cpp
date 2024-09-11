/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "CnetDisplayProperties.h"

#include <iostream>

#include <QAtomicInt>
#include <QFile>
#include <QFileInfo>
#include <QFuture>
#include <QFutureWatcher>
#include <QMap>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QtConcurrentRun>

#include "ControlNet.h"
#include "Cube.h"
#include "IException.h"
#include "IString.h"
#include "SerialNumber.h"


namespace Isis {
  CnetDisplayProperties *CnetDisplayProperties::m_instance = NULL;


  CnetDisplayProperties *CnetDisplayProperties::getInstance() {
    return m_instance ? m_instance : m_instance = new CnetDisplayProperties;
  }


  CnetDisplayProperties::CnetDisplayProperties() {
    nullify();

    useFileNames = true;
    m_curComposing = false;
    m_showFullPath = false;
    m_coordinateDisplayType = LatLonRadius;

    m_readWriteLock = new QReadWriteLock;

    m_serialNumberToFileNameMap = new QMap< QString, QString >;

    m_composedCount = new QAtomicInt;
    m_interruptFlag = new QAtomicInt;
    m_interruptFlag->fetchAndStoreRelaxed(0);

    m_composeStatusPoller = new QTimer;
    connect(m_composeStatusPoller, SIGNAL(timeout()),
        this, SLOT(composeStatusUpdated()));

    m_composeWatcher = new QFutureWatcher< QMap< QString, QString > >;
    connect(m_composeWatcher, SIGNAL(finished()),
        this, SLOT(serialNumbersComposed()));
  }


//   CnetDisplayProperties::CnetDisplayProperties(
//       CnetDisplayProperties const & other)
//   {
//     nullify();
//
//     useFileNames = other.useFileNames;
//     m_serialNumberToFileNameMap = new QMap< QString, QString >(
//         *other.m_serialNumberToFileNameMap);
//
//     m_composeStatusPoller = new QTimer;
//
//     m_composeWatcher = new QFutureWatcher< QMap< QString, QString > >;
//     connect(m_composeWatcher, SIGNAL(finished()),
//             this, SLOT(serialNumbersComposed()));
//   }


  CnetDisplayProperties::~CnetDisplayProperties() {
    delete m_serialNumberToFileNameMap;
    m_serialNumberToFileNameMap = NULL;

    delete m_composeWatcher;
    m_composeWatcher = NULL;

    delete m_composedCount;
    m_composedCount = NULL;

    delete m_interruptFlag;
    m_interruptFlag = NULL;
  }


  bool CnetDisplayProperties::currentlyComposing() const {
    return m_curComposing;
  }


  /**
   * TODO comment me
   *
   * If a cube list is currently being loaded, this method will return an empty
   * list. If there is no cube list, it will also return an empty list.
   *
   * This method is thread safe.
   */
  QList<QString> CnetDisplayProperties::getCubeList(ControlNet *cnet) const {
    QList<QString> results;

    if (!currentlyComposing()) {
      foreach (QString serialNumber, cnet->GetCubeSerials()) {
        QString possibleFileName = getImageName(serialNumber, true);

        if (possibleFileName != serialNumber)
          results.append(possibleFileName);
      }
    }

    return results;
  }


  QString CnetDisplayProperties::getFileName(QString fileName,
      bool forceFullPaths) const {
    QString result;

    if (forceFullPaths || getShowsFullPaths())
      result = fileName;
    else
      result = QFileInfo(fileName).fileName();

    return result;
  }


  /**
   * @param cubeSerialNumber Cube serial number as a QString
   *
   * @returns The file name associated with the given cube serial number, or
   *          the given cube serial number if a file name can not be found in
   *          the current cube list.
   *
   * @see setCubeList()
   *
   * This method is thread safe!
   */
  QString CnetDisplayProperties::getImageName(QString cubeSerialNumber,
      bool forceFullPaths) const {

    QString imageName = cubeSerialNumber;

    m_readWriteLock->lockForRead();

    if (m_serialNumberToFileNameMap && useFileNames &&
        m_serialNumberToFileNameMap->contains(cubeSerialNumber)) {
      QString value = m_serialNumberToFileNameMap->value(cubeSerialNumber);
      m_readWriteLock->unlock();

      if (value.toLower() != "unknown")
        imageName = getFileName(value, forceFullPaths);
    }
    else {
      m_readWriteLock->unlock();
    }

    return imageName;
  }


  QString CnetDisplayProperties::getSerialNumber(QString imageId) {
    QString sn = imageId;

    if (m_serialNumberToFileNameMap && useFileNames) {
      m_readWriteLock->lockForRead();
      QMapIterator< QString, QString > i(*m_serialNumberToFileNameMap);

      bool found = false;
      while (!found && i.hasNext()) {
        i.next();
        if (i.value() == imageId) {
          found = true;
          sn = i.key();
        }
      }

      m_readWriteLock->unlock();
    }

    return sn;
  }


  bool CnetDisplayProperties::getShowsFullPaths() const {
    return m_showFullPath;
  }


  /**
   * Returns coordinate display type (0 = LatLonRadius, 1 = XYZ).
   *
   * @return enum CnetDisplayProperties::coordinateDisplayType.
   *
   */
  enum CnetDisplayProperties::coordinateDisplayType CnetDisplayProperties::coordinateDisplayType() {
      return Isis::CnetDisplayProperties::m_coordinateDisplayType;
  }


  void CnetDisplayProperties::setCubeList(QString fileName) {
    QFile imageListFile(fileName);

    if (!imageListFile.exists()) {
      IString msg = "The file [";
      msg += fileName.toStdString();
      msg += "] does not exist.\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    if (!imageListFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
      IString msg = "The file [";
      msg += fileName.toStdString();
      msg += "] failed to open.\n";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    QStringList imageFileNames;
    QTextStream ts(&imageListFile);

    while (!ts.atEnd())
      imageFileNames << ts.readLine();

    imageListFile.close();

    m_curComposing = true;

    QFuture< QMap< QString, QString > > future = QtConcurrent::run(
        this, &CnetDisplayProperties::composeSerialNumbers, imageFileNames);

    m_composeWatcher->setFuture(future);
  }


  void CnetDisplayProperties::setFileNameUsage(bool preferFileNames) {
    useFileNames = preferFileNames;
  }


//   CnetDisplayProperties & CnetDisplayProperties::operator=(
//       CnetDisplayProperties const & other)
//   {
//     if (this != &other)
//     {
//       m_composeWatcher->waitForFinished();
//       useFileNames = other.useFileNames;
//     }
//
//     return *this;
//   }


  void CnetDisplayProperties::setShowsFullPaths(bool newState) {
    m_showFullPath = newState;
  }


  /**
   * Sets the coordinate display type (0 = LatLonRadius, 1 = XYZ).
   *
   * @param coordDisplay coordinate display type.
   *
   * @return void.
   *
   */
  void CnetDisplayProperties::setCoordinateDisplayType(enum coordinateDisplayType coordDisplay) {
      m_coordinateDisplayType = coordDisplay;
  }


  QMap< QString, QString > CnetDisplayProperties::composeSerialNumbers(
    QStringList fileNames) {
    emit composeProgressRangeChanged(0, fileNames.size() - 1);
    m_composedCount->fetchAndStoreRelaxed(0);

    QMap< QString, QString > newMap;

    for (int i = 0; *m_interruptFlag == 0 && i < fileNames.size(); i++) {
      QString fileName = fileNames[i];

      Cube cube;
      cube.open(fileName);

      newMap.insert(SerialNumber::Compose(fileName), fileNames[i]);

      m_composedCount->fetchAndAddRelaxed(1);
    }

    return newMap;
  }


  void CnetDisplayProperties::nullify() {
    m_serialNumberToFileNameMap = NULL;
    m_composeWatcher = NULL;

    m_composedCount = NULL;
    m_interruptFlag = NULL;

    m_readWriteLock = NULL;
  }


  void CnetDisplayProperties::composeStatusUpdated() {
    emit composeProgressChanged(*m_composedCount);
  }


  void CnetDisplayProperties::serialNumbersComposed() {
    if (*m_interruptFlag) {
      m_interruptFlag->fetchAndStoreRelaxed(0);
    }
    else {
      m_readWriteLock->lockForWrite();
      *m_serialNumberToFileNameMap = m_composeWatcher->result();

      m_readWriteLock->unlock();
    }

    m_curComposing = false;
    m_composeStatusPoller->stop();
    emit composeProgressRangeChanged(0, 0);
    emit composeProgressChanged(0);
    emit compositionFinished();
  }
}
