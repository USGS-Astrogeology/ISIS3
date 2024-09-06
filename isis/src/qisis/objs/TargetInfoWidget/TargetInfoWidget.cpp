#include "TargetInfoWidget.h"
#include "ui_TargetInfoWidget.h"
#include <QFont>
#include <QFontInfo>
#include <QPixmap>

#include "Directory.h"
#include "SpiceRotation.h"
#include "TargetBody.h"
#include "TargetBodyDisplayProperties.h"

namespace Isis {

  /**
   * Constructor.  Sets up the widget based on the target.
   * 
   * @param target The target whose information will be displayed
   * @param directory Currently unused
   * @param parent The parent widget
   */
  TargetInfoWidget::TargetInfoWidget(TargetBody* target, Directory *directory,
                                     QWidget *parent) : m_ui(new Ui::TargetInfoWidget) {
    m_ui->setupUi(this);

    m_target = target;

    QString name = m_target->displayProperties()->displayName();


    // Ken TODO - set up map between target display names and icon/image names
    QPixmap image;
    if (name.compare("MOON") == 0) {
      image.load(FileName("$ISISROOT/appdata/images/targets/nasa_moon_large.png").expanded());
      setWindowIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/weather-clear-night.png")
                                   .expanded()));
    }
    else if (name.compare("Enceladus") == 0) {
      image.load(FileName("$ISISROOT/appdata/images/targets/nasa_enceladus_saturn.png").expanded());
      setWindowIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/nasa_enceladus.png").expanded()));
    }
    else if (name.compare("Europa") == 0) {
      image.load(FileName("$ISISROOT/appdata/images/targets/nasa_europa_large.png").expanded());
      setWindowIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/nasa_europa.png").expanded()));
    }
    else if (name.compare("Mars") == 0) {
      image.load(FileName("$ISISROOT/appdata/images/targets/nasa_mars_large.png").expanded());
      setWindowIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/nasa_mars.png").expanded()));
    }
    else if (name.compare("Titan") == 0) {
      image.load(FileName("$ISISROOT/appdata/images/targets/nasa_titan_large.png").expanded());
      setWindowIcon(QIcon(FileName("$ISISROOT/appdata/images/icons/nasa_titan.png").expanded()));
    }

    m_ui->bodySystemlabel->setText(tr("System: %1").arg(m_target->naifPlanetSystemName()));

    setMinimumWidth(m_ui->tabWidget->minimumWidth()+20);

    m_ui->targetImage->setPixmap(image);

    setFrameStyle(QFrame::Panel | QFrame::Sunken);
    setLineWidth(2);

    m_ui->tabWidget->setCurrentIndex(0);

    if (target->frameType() == Isis::SpiceRotation::BPC) {
      m_ui->poleRightAscensionLabel->hide();
      m_ui->poleDeclinationLabel->hide();
      m_ui->polePMOffsetLabel->hide();
    }
    else {
      m_ui->poleRightAscensionLabel->setText(formatPoleRaString());
      m_ui->poleDeclinationLabel->setText(formatPoleDecString());
      m_ui->polePMOffsetLabel->setText(formatPmString());
    }

    m_ui->aRadiiLabel->setText(tr("%1").arg(m_target->radiusA().kilometers()));
    m_ui->bRadiiLabel->setText(tr("%1").arg(m_target->radiusB().kilometers()));
    m_ui->cRadiiLabel->setText(tr("%1").arg(m_target->radiusC().kilometers()));
    m_ui->meanRadiiLabel->setText(tr("%1").arg(m_target->meanRadius().kilometers()));
  }


  /**
   * Destructor
   */
  TargetInfoWidget::~TargetInfoWidget() {
    delete m_ui;
  }


  /**
   * Make the poleRightAscensionLabel text using information from the target.
   * 
   * @return @b QString The poleRightAscensionLabel text
   */
  QString TargetInfoWidget::formatPoleRaString() {

    QString poleRaString = "";

    if (m_target->frameType() != Isis::SpiceRotation::BPC &&
        m_target->frameType() != Isis::SpiceRotation::UNKNOWN ) {

      std::vector<Angle> poleRaCoefs = m_target->poleRaCoefs();
      std::vector<double> poleRaNutPrecCoefs = m_target->poleRaNutPrecCoefs();

      const QChar degChar(0260);
      QString coefLetter = m_target->naifPlanetSystemName().at(0);

      if (poleRaCoefs[1].degrees() < 0.0 ) {
        poleRaString.append(tr("%1%3 - %2T").arg(poleRaCoefs[0].degrees()).arg(-poleRaCoefs[1]
          .degrees()).arg(degChar));
      }
      else {
        poleRaString.append(tr("%1%3 + %2T").arg(poleRaCoefs[0].degrees()).arg(poleRaCoefs[1]
          .degrees()).arg(degChar));
      }

      QString tmp;
      int nCoefs = poleRaNutPrecCoefs.size();;
      for (int i = 0; i < nCoefs; i++) {
        if (poleRaNutPrecCoefs[i] < 0.0 ) {
          tmp.append(tr(" - %1%2%3").arg(-poleRaNutPrecCoefs[i]).arg("sin %1").arg(coefLetter)
                                  .arg(i+1));
        }
        else if (poleRaNutPrecCoefs[i] > 0.0 ) {
          tmp.append(tr(" + %1%2%3").arg(poleRaNutPrecCoefs[i]).arg("sin %1").arg(coefLetter)
                                  .arg(i+1));
        }
      }

      poleRaString.append(tmp);

      return poleRaString;

      }

    else {
         errorMsg();
         return poleRaString;
    }


  }


  /**
   * Make the poleDeclinationLabel text using information from the target.
   * 
   * @return @b QString The poleDeclinationLabel text
   */
  QString TargetInfoWidget::formatPoleDecString() {

    QString poleDecString = "";

    if (m_target->frameType() != Isis::SpiceRotation::BPC &&
        m_target->frameType() != Isis::SpiceRotation::UNKNOWN ) {

      std::vector<Angle> poleDecCoefs = m_target->poleDecCoefs();
      std::vector<double> poleDecNutPrecCoefs = m_target->poleDecNutPrecCoefs();

      const QChar degChar(0260);

      QString coefLetter = m_target->naifPlanetSystemName().at(0);

      if (poleDecCoefs[1].degrees() < 0.0 ) {
        poleDecString.append(tr("%1%3 - %2T").arg(poleDecCoefs[0].degrees()).arg(-poleDecCoefs[1]
          .degrees()).arg(degChar));
      }
      else {
        poleDecString.append(tr("%1%3 + %2T").arg(poleDecCoefs[0].degrees()).arg(poleDecCoefs[1]
          .degrees()).arg(degChar));
      }

      QString tmp;
      int nCoefs = poleDecNutPrecCoefs.size();

      for (int i = 0; i < nCoefs; i++) {
        if (poleDecNutPrecCoefs[i] < 0.0 ) {
          tmp.append(tr(" - %1%2%3").arg(-poleDecNutPrecCoefs[i]).arg("cos %1").arg(coefLetter)
                                  .arg(i+1));
        }
        else if (poleDecNutPrecCoefs[i] > 0.0 ) {
          tmp.append(tr(" + %1%2%3").arg(poleDecNutPrecCoefs[i]).arg("cos %1").arg(coefLetter)
                                  .arg(i+1));
        }
      }

      poleDecString.append(tmp);
      return poleDecString;

    }//end if

    else {
          errorMsg();
          return poleDecString;
    }

  }


  /**
   * Make the polePMOffsetLabel text using information from the target.
   * 
   * @return @b QString The polePMOffsetLabel text
   */
  QString TargetInfoWidget::formatPmString() {

    QString pmString = "";

    if (m_target->frameType() != Isis::SpiceRotation::BPC &&
        m_target->frameType() != Isis::SpiceRotation::UNKNOWN )
    {

      std::vector<Angle> pmCoefs = m_target->pmCoefs();
      std::vector<double> pmNutPrecCoefs = m_target->pmNutPrecCoefs();

      const QChar degChar(0260);

      QString coefLetter = m_target->naifPlanetSystemName().at(0);

      if (pmCoefs[1].degrees() < 0.0 ) {
        pmString.append(tr("%1%3 - %2d").arg(pmCoefs[0].degrees()).arg(-pmCoefs[1].degrees())
          .arg(degChar));
      }
      else if (pmCoefs[1].degrees() > 0.0 ) {
        pmString.append(tr("%1%3 + %2d").arg(pmCoefs[0].degrees()).arg(pmCoefs[1].degrees())
          .arg(degChar));
      }

      if (pmCoefs[2].degrees() < 0.0 ) {
        pmString.append(tr(" - %2d^2").arg(-pmCoefs[2].degrees()));
      }
      else if (pmCoefs[2].degrees() > 0.0 ) {
        pmString.append(tr(" + %2d^2").arg(pmCoefs[2].degrees()));
      }

      QString tmp;
      int nCoefs = pmNutPrecCoefs.size();

      for (int i = 0; i < nCoefs; i++) {
        if (pmNutPrecCoefs[i] < 0.0 ) {
          tmp.append(tr(" - %1%2%3").arg(-pmNutPrecCoefs[i]).arg("sin %1").arg(coefLetter).arg(i+1));
        }
        else if (pmNutPrecCoefs[i] > 0.0 ) {
          tmp.append(tr(" + %1%2%3").arg(pmNutPrecCoefs[i]).arg("sin %1").arg(coefLetter).arg(i+1));
        }
      } //end-for

      pmString.append(tmp);
      return pmString;

    }//end outer-if
    else {
         errorMsg();
         return pmString;
    } //end outer else

  }



  /**
   * Displays an error message on the Prime Meridian/Pole Position tabs of the TargetInfoWidget
   * in the event that the target body parameters could not be retrieved from the cube.
   *
   */
  void TargetInfoWidget::errorMsg() {

    QFont font;
    font.setPointSize(9);
    font.setBold(true);
    font.setWeight(75);
    m_ui->label->setFont(font);
    m_ui->label_6->setFont(font);

    std::string msg1="";
    std::string msg2="";

    m_ui->label->setFont(font);
    m_ui->label_6->setFont(font);
    m_ui->label_2->clear();

    if (m_target->displayProperties()->displayName() == "MOON") {
      msg1 = "Target body parameters cannot be solved for the Moon.";
    }
    else {
              msg2 = "Target body information\n"
                       "is not on the cube labels.\n"
                       "This has no impact on most\n"
                       "operations.  However, to view\n"
                       "or bundle adjust the target body\n"
                       "parameters you will need to rerun\n"
                       "spiceinit.";

              m_ui->label->setText(
                    QApplication::translate("TargetInfoWidget",
                                            msg2.toLatin1().data(), 0));
              m_ui->label_6->setText(
                    QApplication::translate("TargetInfoWidget",
                                            msg2.toLatin1().data(), 0));
    } //end inner-else

  }
}
