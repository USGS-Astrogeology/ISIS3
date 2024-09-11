#include "ImageReader.h"

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
#include "Image.h"
#include "ImageDisplayProperties.h"
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
  ImageReader::ImageReader(QMutex *cameraMutex, bool requireFootprints, QObject *parent) :
      QObject(parent) {
    m_watcher = NULL;

    m_cameraMutex = cameraMutex;
    m_requireFootprints = requireFootprints;

    m_progress = new ProgressBar("Reading Images");
    m_watcher = new QFutureWatcher<Image *>;

    initProgress();

    connect(m_watcher, SIGNAL(resultReadyAt(int)),
            this, SLOT(imageReady(int)));
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
  ImageReader::~ImageReader() {
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


  QList<QAction *> ImageReader::actions(ImageDisplayProperties::Property relevantDispProperties) {
    QList<QAction *> results;

    if (!m_safeFileOpenAct) {
      m_safeFileOpenAct = new QAction(tr("&Safe File Open"), this);
      m_safeFileOpenAct->setCheckable(true);
      m_safeFileOpenAct->setChecked(m_safeFileOpen);
      m_safeFileOpenAct->setWhatsThis(tr("This lowers the number of "
          "simulataneously open files drastically in order to stay under the "
          "operating system limit. Only use this if you are having trouble "
          "loading large numbers of images."));
      connect(m_safeFileOpenAct, SIGNAL(toggled(bool)),
              this, SLOT(setSafeFileOpen(bool)));
    }

    results.append(m_safeFileOpenAct);

    if ((relevantDispProperties & ImageDisplayProperties::ShowFill) == ImageDisplayProperties::ShowFill) {
      if (!m_openFilledAct) {
        m_openFilledAct = new QAction(tr("Default Images &Filled"), this);
        m_openFilledAct->setCheckable(true);
        m_openFilledAct->setChecked(m_openFilled);
        m_openFilledAct->setWhatsThis(tr("When this is enabled, images will be overlayed with "
            "a color."));
        connect(m_openFilledAct, SIGNAL(toggled(bool)),
                this, SLOT(setOpenFilled(bool)));
      }

      results.append(m_openFilledAct);
    }

    if ((relevantDispProperties & ImageDisplayProperties::Color) == ImageDisplayProperties::Color) {
      if (!m_askAlphaAct) {
        m_askAlphaAct = new QAction(tr("Set Default &Transparency"), this);
        connect(m_askAlphaAct, SIGNAL(triggered(bool)),
                this, SLOT(askDefaultAlpha()));
      }

      results.append(m_askAlphaAct);
    }

    return results;
  }


  QProgressBar *ImageReader::progress() {
    return m_progress;
  }


  void ImageReader::askDefaultAlpha() {
    bool ok = false;
    int alpha = QInputDialog::getInt(NULL, tr("Transparency Value"),
        tr("Set the default transparency value\n"
           "Values are 0 (invisible) to 255 (solid)"),
        m_defaultAlpha, 0, 255, 1, &ok);

    if(ok) {
      m_defaultAlpha = alpha;
    }
  }


  void ImageReader::read(PvlObject imagesObject) {
    read(imagesObject.beginObject(), imagesObject.endObject());
  }


  /**
   * Handle opening cubes by filename.
   */
  void ImageReader::read(QStringList cubeNames) {
    read(cubeNames.begin(), cubeNames.end());
  }


  void ImageReader::setOpenFilled(bool openFilled) {
    m_openFilled = openFilled;
    if (m_openFilledAct) {
      m_openFilledAct->setChecked(m_openFilled);
    }
  }


  void ImageReader::setSafeFileOpen(bool safeFileOpen) {
    m_safeFileOpen = safeFileOpen;
    if (m_safeFileOpen) {
      m_safeFileOpenAct->setChecked(m_safeFileOpen);
    }
  }


  void ImageReader::initProgress() {
    m_progress->setVisible(false);
    m_progress->setRange(0, 0);
    m_progress->setValue(0);
  }


  void ImageReader::start() {
    if (!m_backlog.isEmpty() && !m_mappedRunning) {
      m_progress->setVisible(true);

      int maxOpenImages = m_safeFileOpen? 20 : 400;
      QList<QVariant> culledBacklog = m_backlog.mid(0, maxOpenImages);
      m_backlog = m_backlog.mid(maxOpenImages);

      QFuture< Image * > images = QtConcurrent::mapped(
          culledBacklog,
          VariantToImageFunctor(m_cameraMutex, m_requireFootprints, QThread::currentThread(),
            m_openFilled, m_defaultAlpha));

      m_watcher->setFuture(images);
      m_mappedRunning = true;
    }
  }


  void ImageReader::readSettings() {
    QSettings settings(QString::fromStdString(FileName("$HOME/.Isis/" + QApplication::applicationName().toStdString() + "/Image Reader.config")
          .expanded()),
        QSettings::NativeFormat);

    m_safeFileOpen = settings.value("safeFileOpen", m_safeFileOpen).toBool();
    m_openFilled = settings.value("openFilled", m_openFilled).toBool();
    m_defaultAlpha = settings.value("defaultAlpha", m_defaultAlpha).toInt();
  }


  void ImageReader::writeSettings() {
    QSettings settings(
        QString::fromStdString(FileName("$HOME/.Isis/" + QApplication::applicationName().toStdString() + "/Image Reader.config")
          .expanded()),
        QSettings::NativeFormat);

    settings.setValue("safeFileOpen", m_safeFileOpen);
    settings.setValue("openFilled", m_openFilled);
    settings.setValue("defaultAlpha", m_defaultAlpha);
  }


  void ImageReader::imageReady(int index) {
    m_progress->setValue(m_progress->value() + 1);
  }


  void ImageReader::mappedFinished() {
    ImageList images(m_watcher->future().results());

    //  Tracie Sucharski:  Go through list & get rid of NULLs.  This is a temporary fix to get rid
    //   of seg faulting when something goes wrong.  This is not a good solution & needs to be
    //   properly fixed
    foreach (Image *image, images) {
      if (!image) {
        images.removeAll(image);
      }
    }

    emit imagesReady(images);

    m_mappedRunning = false;
    if (!m_backlog.isEmpty()) {
      start();
    }
    else {
      initProgress();
    }
  }


  /**
   * Create a functor for converting from QVariant to an Image *
   *
   * This method is always called from the GUI thread.
   */
  ImageReader::VariantToImageFunctor::VariantToImageFunctor(
      QMutex *cameraMutex, bool requireFootprints, QThread *targetThread, bool openFilled,
      int defaultAlpha) {
    m_mutex = cameraMutex;
    m_targetThread = targetThread;
    m_openFilled = openFilled;
    m_defaultAlpha = defaultAlpha;
    m_requireFootprints = requireFootprints;
  }


  /**
   * Read the QString filename and make an Image
   *   from it. Set the default values. This is what we're doing in another
   *   thread, so make sure the QObject ends up in the correct thread.
   *
   * This method is never called from the GUI thread.
   */
  Image *ImageReader::VariantToImageFunctor::operator()(
      const QVariant &imageData) {
    Image *result = NULL;

    try {
      std::string fileName;
      if (imageData.canConvert<QString>()) {
        fileName = imageData.value<QString>().toStdString();

        result = new Image(QString::fromStdString(FileName(fileName).expanded()));
        ImageDisplayProperties *prop = result->displayProperties();
        prop->setShowFill(m_openFilled);

        QColor newColor = prop->getValue(ImageDisplayProperties::Color).value<QColor>();
        newColor.setAlpha(m_defaultAlpha);

        prop->setColor(newColor);
      }
      else if (imageData.canConvert<PvlObject>()) {
        PvlObject imageObj = imageData.value<PvlObject>();
        fileName = imageObj["FileName"][0];
        result = new Image(QString::fromStdString(FileName(fileName).expanded()));
        result->fromPvl(imageObj);
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
