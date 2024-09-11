#include "ShapeReader.h"

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QFuture>
#include <QFutureWatcher>
#include <QInputDialog>
#include <QList>
#include <QProgressBar>
#include <QSettings>
#include <QString>
#include <QtConcurrentMap>
#include <QVariant>

#include "IException.h"
#include "Shape.h"
#include "ShapeDisplayProperties.h"
#include "IString.h"
#include "FileName.h"
#include "ProgressBar.h"
#include "PvlObject.h"

namespace Isis {

  /**
   * MosaicWidget constructor.
   * MosaicWidget is derived from QSplitter, the left side of the
   * splitter is a QTreeWidget and the right side of the splitter
   * is a QGraphicsView.
   *
   *
   * @param parent
   */
  ShapeReader::ShapeReader(QMutex *cameraMutex, bool requireFootprints, QObject *parent) :
      QObject(parent) {
    m_watcher = NULL;

    m_cameraMutex = cameraMutex;
    m_requireFootprints = requireFootprints;

    m_progress = new ProgressBar("Reading Shapes");
    m_watcher = new QFutureWatcher<Shape *>;

    initProgress();

    connect(m_watcher, SIGNAL(resultReadyAt(int)),
            this, SLOT(shapesReady(int)));
    connect(m_watcher, SIGNAL(finished()),
            this, SLOT(mappedFinished()));

    m_safeFileOpen = false;
    m_openFilled = true;
    m_defaultAlpha = 60;

    m_mappedRunning = false;

    readSettings();
  }


  /**
   * Free the allocated memory by this object
   */
  ShapeReader::~ShapeReader() {
    writeSettings();

    if (m_watcher) {
      m_watcher->cancel();
      m_watcher->waitForFinished();
    }

    delete m_watcher;
    m_watcher = NULL;

    m_backlog.clear();

    m_cameraMutex = NULL;

    delete m_progress;
  }


  QList<QAction *> ShapeReader::actions(ShapeDisplayProperties::Property relevantDispProperties) {
    QList<QAction *> results;

    if (!m_safeFileOpenAct) {
      m_safeFileOpenAct = new QAction(tr("&Safe File Open"), this);
      m_safeFileOpenAct->setCheckable(true);
      m_safeFileOpenAct->setChecked(m_safeFileOpen);
      m_safeFileOpenAct->setWhatsThis(tr("This lowers the number of "
          "simulataneously open files drastically in order to stay under the "
          "operating system limit. Only use this if you are having trouble "
          "loading large numbers of shapes."));
      connect(m_safeFileOpenAct, SIGNAL(toggled(bool)),
              this, SLOT(setSafeFileOpen(bool)));
    }

    results.append(m_safeFileOpenAct);

    return results;
  }


  QProgressBar *ShapeReader::progress() {
    return m_progress;
  }


  void ShapeReader::read(PvlObject shapesObject) {
    read(shapesObject.beginObject(), shapesObject.endObject());
  }


  /**
   * Handle opening cubes by filename.
   */
  void ShapeReader::read(QStringList cubeNames) {
    read(cubeNames.begin(), cubeNames.end());
  }


  void ShapeReader::setSafeFileOpen(bool safeFileOpen) {
    m_safeFileOpen = safeFileOpen;
    if (m_safeFileOpen) {
      m_safeFileOpenAct->setChecked(m_safeFileOpen);
    }
  }


  void ShapeReader::initProgress() {
    m_progress->setVisible(false);
    m_progress->setRange(0, 0);
    m_progress->setValue(0);
  }


  void ShapeReader::start() {
    if (!m_backlog.isEmpty() && !m_mappedRunning) {
      m_progress->setVisible(true);

      int maxOpenShapes = m_safeFileOpen? 20 : 400;
      QList<QVariant> culledBacklog = m_backlog.mid(0, maxOpenShapes);
      m_backlog = m_backlog.mid(maxOpenShapes);

      QFuture< Shape * > shapes = QtConcurrent::mapped(
          culledBacklog,
          VariantToShapeFunctor(m_cameraMutex, m_requireFootprints, QThread::currentThread(),
            m_openFilled, m_defaultAlpha));

      m_watcher->setFuture(shapes);
      m_mappedRunning = true;
    }
  }


  void ShapeReader::readSettings() {
    QSettings settings(QString::fromStdString(
        FileName("$HOME/.Isis/" + QApplication::applicationName().toStdString() + "/Shape Reader.config")
          .expanded()),
        QSettings::NativeFormat);

    m_safeFileOpen = settings.value("safeFileOpen", m_safeFileOpen).toBool();
  }


  void ShapeReader::writeSettings() {
    QSettings settings(QString::fromStdString(
        FileName("$HOME/.Isis/" + QApplication::applicationName().toStdString() + "/Shape Reader.config")
          .expanded()),
        QSettings::NativeFormat);

    settings.setValue("safeFileOpen", m_safeFileOpen);
  }


  void ShapeReader::shapesReady(int index) {
    m_progress->setValue(m_progress->value() + 1);
  }


  void ShapeReader::mappedFinished() {
    ShapeList shapes(m_watcher->future().results());

    //  Tracie Sucharski:  Go through list & get rid of NULLs.  This is a temporary fix to get rid
    //   of seg faulting when something goes wrong.  This is not a good solution & needs to be
    //   properly fixed
    foreach (Shape *shape, shapes) {
      if (!shape) {
        shapes.removeAll(shape);
      }
    }

    emit shapesReady(shapes);

    m_mappedRunning = false;
    if (!m_backlog.isEmpty()) {
      start();
    }
    else {
      initProgress();
    }
  }


  /**
   * Create a functor for converting from QVariant to an Shape *
   *
   * This method is always called from the GUI thread.
   */
  ShapeReader::VariantToShapeFunctor::VariantToShapeFunctor(
      QMutex *cameraMutex, bool requireFootprints, QThread *targetThread, bool openFilled,
      int defaultAlpha) {
    m_mutex = cameraMutex;
    m_targetThread = targetThread;
    m_openFilled = openFilled;
    m_defaultAlpha = defaultAlpha;
    m_requireFootprints = requireFootprints;
  }


  /**
   * Read the QString filename and make an Shape
   *   from it. Set the default values. This is what we're doing in another
   *   thread, so make sure the QObject ends up in the correct thread.
   *
   * This method is never called from the GUI thread.
   */
  Shape *ShapeReader::VariantToShapeFunctor::operator()(
      const QVariant &shapeData) {
    Shape *result = NULL;

    try {
      std::string fileName;
      if (shapeData.canConvert<QString>()) {
        fileName = shapeData.value<QString>().toStdString();

        result = new Shape(QString::fromStdString(FileName(fileName).expanded()));
      }
      else if (shapeData.canConvert<PvlObject>()) {
        PvlObject shapeObj = shapeData.value<PvlObject>();
        fileName = (shapeObj["FileName"][0]);
        result = new Shape(QString::fromStdString(FileName(fileName).expanded()));
        result->fromPvl(shapeObj);
      }

      if (m_requireFootprints) {
        try {
          result->initFootprint(m_mutex);
        }
        catch (IException &) {
          throw;
        }
      }

      result->moveToThread(m_targetThread);
    }
    catch(IException &e) {
      e.print();
      result = NULL;
    }

    return result;
  }
}
