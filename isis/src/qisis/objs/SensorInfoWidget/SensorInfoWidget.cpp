#include "SensorInfoWidget.h"
#include "ui_SensorInfoWidget.h"

#include <QPixmap>

#include "Camera.h"
#include "CameraDistortionMap.h"
#include "Directory.h"
#include "GuiCamera.h"
#include "GuiCameraDisplayProperties.h"

namespace Isis {

  /**
   * Constructor
   * 
   * @param camera The camera whose information is being displayed
   * @param directory unused
   * @param parent The parent widget
   */
  SensorInfoWidget::SensorInfoWidget(GuiCamera* camera, Directory *directory,
                                     QWidget *parent) : m_ui(new Ui::SensorInfoWidget) {
    m_ui->setupUi(this);

    m_camera = camera;

    // danger here
//    Camera *isisCamera = camera->camera();
    // danger here

    QString displayName = camera->displayProperties()->displayName();

//    CameraDistortionMap *distortionMap = isisCamera->DistortionMap();

    QPixmap image;
    if (displayName.contains("ISSNA")) {
      // TODO Find legal image for this!
      image.load(":cassini-iss-nac");
    }
    else if (displayName.contains("Metric")) {
      image.load(QString::fromStdString(FileName("$ISISROOT/images/icons/nasa_apollo_metric_camera.png")
                          .expanded()));
    }
    else if (displayName.contains("HiRISE")) {
      // TODO Find legal image for this!
      image.load(":hirise-camera");
    }

    m_ui->spacecraftlabel->setText(tr("Spacecraft: %1")
                                   .arg(m_camera->spacecraftNameLong()));

    setMinimumWidth(m_ui->tabWidget->minimumWidth()+20);

    m_ui->targetImage->setPixmap(image);

    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setLineWidth(2);

    m_ui->tabWidget->setCurrentIndex(0);

//    m_ui->poleRightAscensionLabel->setText(formatPoleRaString());
//    m_ui->poleDeclinationLabel->setText(formatPoleDecString());
//    m_ui->polePMOffsetLabel->setText(formatPmString());

//    m_ui->aRadiiLabel->setText(tr("%1").arg(m_target->radiusA().kilometers()));
//    m_ui->bRadiiLabel->setText(tr("%1").arg(m_target->radiusB().kilometers()));
//    m_ui->cRadiiLabel->setText(tr("%1").arg(m_target->radiusC().kilometers()));
//    m_ui->meanRadiiLabel->setText(tr("%1").arg(m_target->meanRadius().kilometers()));
  }


  /**
   * Destructor
   */
  SensorInfoWidget::~SensorInfoWidget() {
    delete m_ui;
  }


//  QString SensorInfoWidget::formatPoleRaString() {
//    std::vector<double> poleRaCoefs = m_target->poleRaCoefs();
//    std::vector<double> poleRaNutPrecCoefs = m_target->poleRaNutPrecCoefs();

//    const QChar degChar(0260);
//    QString poleRaString = "";
//    QString coefLetter = m_target->naifPlanetSystemName().at(0);

//    if (poleRaCoefs[1] < 0.0 )
//      poleRaString.append(tr("%1%3 - %2T").arg(poleRaCoefs[0]).arg(-poleRaCoefs[1]).arg(degChar));
//    else
//      poleRaString.append(tr("%1%3 + %2T").arg(poleRaCoefs[0]).arg(poleRaCoefs[1]).arg(degChar));

//    QString tmp;
//    int nCoefs = poleRaNutPrecCoefs.size();;
//    for (int i = 0; i < nCoefs; i++) {
//      if (poleRaNutPrecCoefs[i] < 0.0 ) {
//        tmp.append(tr(" - %1%2%3").arg(-poleRaNutPrecCoefs[i]).arg("sin %1").arg(coefLetter)
//                                  .arg(i+1));
//      }
//      else if (poleRaNutPrecCoefs[i] > 0.0 ) {
//        tmp.append(tr(" + %1%2%3").arg(poleRaNutPrecCoefs[i]).arg("sin %1").arg(coefLetter)
//                                  .arg(i+1));
//      }
//    }

//    poleRaString.append(tmp);

//    return poleRaString;
//  }


//  QString SensorInfoWidget::formatPoleDecString() {
//    std::vector<double> poleDecCoefs = m_target->poleDecCoefs();
//    std::vector<double> poleDecNutPrecCoefs = m_target->poleDecNutPrecCoefs();

//    const QChar degChar(0260);
//    QString poleDecString = "";
//    QString coefLetter = m_target->naifPlanetSystemName().at(0);

//    if (poleDecCoefs[1] < 0.0 )
//      poleDecString.append(tr("%1%3 - %2T").arg(poleDecCoefs[0]).arg(-poleDecCoefs[1])
//                                           .arg(degChar));
//    else
//      poleDecString.append(tr("%1%3 + %2T").arg(poleDecCoefs[0]).arg(poleDecCoefs[1]).arg(degChar));

//    QString tmp;
//    int nCoefs = poleDecNutPrecCoefs.size();;
//    for (int i = 0; i < nCoefs; i++) {
//      if (poleDecNutPrecCoefs[i] < 0.0 ) {
//        tmp.append(tr(" - %1%2%3").arg(-poleDecNutPrecCoefs[i]).arg("cos %1").arg(coefLetter)
//                                  .arg(i+1));
//      }
//      else if (poleDecNutPrecCoefs[i] > 0.0 ) {
//        tmp.append(tr(" + %1%2%3").arg(poleDecNutPrecCoefs[i]).arg("cos %1").arg(coefLetter)
//                                  .arg(i+1));
//      }
//    }

//    poleDecString.append(tmp);

//    return poleDecString;
//  }


//  QString SensorInfoWidget::formatPmString() {
//    std::vector<double> pmCoefs = m_target->pmCoefs();
//    std::vector<double> pmNutPrecCoefs = m_target->pmNutPrecCoefs();

//    const QChar degChar(0260);
//    QString pmString = "";
//    QString coefLetter = m_target->naifPlanetSystemName().at(0);

//    if (pmCoefs[1] < 0.0 )
//      pmString.append(tr("%1%3 - %2d").arg(pmCoefs[0]).arg(-pmCoefs[1]).arg(degChar));
//    else if (pmCoefs[1] > 0.0 )
//      pmString.append(tr("%1%3 + %2d").arg(pmCoefs[0]).arg(pmCoefs[1]).arg(degChar));

//    if (pmCoefs[2] < 0.0 )
//      pmString.append(tr(" - %2d^2").arg(-pmCoefs[2]));
//    else if (pmCoefs[2] > 0.0 )
//      pmString.append(tr(" + %2d^2").arg(pmCoefs[2]));


//    QString tmp;
//    for (int i = 0; i < pmNutPrecCoefs.size(); i++) {
//      if (pmNutPrecCoefs[i] < 0.0 ) {
//        tmp.append(tr(" - %1%2%3").arg(-pmNutPrecCoefs[i]).arg("sin %1").arg(coefLetter).arg(i+1));
//      }
//      else if (pmNutPrecCoefs[i] > 0.0 ) {
//        tmp.append(tr(" + %1%2%3").arg(pmNutPrecCoefs[i]).arg("sin %1").arg(coefLetter).arg(i+1));
//      }
//    }

//    pmString.append(tmp);

//    return pmString;
//  }
}
