#include "ProfileDialog.h"

#include <QComboBox>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStackedWidget>


ProfileDialog::ProfileDialog(QWidget *parent) : QDialog(parent) {

  setupUi(this);

  m_startCreated = false;
  m_endCreated = false;

  connect(createStartButton, SIGNAL(clicked()),
          this, SLOT(createStartSelected()));
  connect(createEndButton, SIGNAL(clicked()),
          this, SLOT(createEndSelected()));
  connect(helpButton, SIGNAL(clicked()), this, SLOT(help()));

}



void ProfileDialog::createStartSelected() {
  createStartButton->setEnabled(false);
  m_startCreated = true;
  if (m_startCreated && m_endCreated) {
    profileButton->setEnabled(true);
  }
  emit createStart();

}



void ProfileDialog::createEndSelected() {
  createEndButton->setEnabled(false);
  m_endCreated = true;
  if (m_startCreated && m_endCreated) {
    profileButton->setEnabled(true);
  }
  emit createEnd();

}



void ProfileDialog::help() {
  QString message = "You must create and refine the end points of the profile "
    "line before the elevation profile can be calculated.\n\n  A line  is "
    "computed between the end points on both the left and right cubes, then "
    "sub-pixel registration is computed along these two lines to find the "
    "same pixel on both cubes.  The instument pointing at these pixels is then "
    "used to compute the elevation.";
  QMessageBox::information(this, "Elevation Profile", message);
  
}

