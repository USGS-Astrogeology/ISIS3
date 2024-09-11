#include "WarningWidget.h"
#include "IString.h"
#include "FileName.h"

#include <QIcon>
#include <QPixmap>
#include <QGridLayout>
#include <QPushButton>
#include <QFont>

namespace Isis {
  /**
   * Warning widget constructor, initializes and creates the
   * Nowarning, Warning objects and objects associated with them
   *
   * @author sprasad (12/11/2009)
   *
   * @param parent - parent of this object
   */
  WarningWidget::WarningWidget(QStatusBar *pParent): QObject(pParent) {
    mSBar = pParent;

    QString sToolIconDir = QString::fromStdString(FileName("$ISISROOT/appdata/images/icons").expanded());
    QString qsIconFile(sToolIconDir);

    // default Action - No warning
    mNoWarning = new QPushButton(mSBar);
    mNoWarning->setFixedSize(22, 22) ;
    mNoWarning->setFlat(true);
    mNoWarning->setIconSize(QSize(15, 15));
    mNoWarning->setIcon(QPixmap(qsIconFile + "/qview_NoWarning.png"));
    mNoWarning->setToolTip("No Warning");
    mNoWarning->setVisible(true);
    // Add Nowarning object to the layout of the active tool bar of the Tool
    mSBar->insertPermanentWidget(0, mNoWarning);

    mSBar->showMessage("Ready");
    mMsgStr = QString("Ready");

    // Warning is not set
    mbWarningFlag = false;

    // Dialog box displayed when the Warning Icon is clicked
    mDialog = new QDialog(mSBar);
    mDialog->setWindowTitle("Warning");
    mDialog->setSizeGripEnabled(true);
    mWindow = new QWidget(mDialog);
    mWindow->installEventFilter(this);//receive events directed to this object

    mTextEdit = new QTextEdit(mDialog);
    mTextEdit->setReadOnly(true);
    QFont font("Helvetica", 11);
    mTextEdit->setFont(font);

    // OK button for the dialog window
    QPushButton *okButton = new QPushButton("Ok", mDialog);
    okButton->setShortcut(Qt::Key_Enter);
    connect(okButton, SIGNAL(clicked()),  this, SLOT(resetWarning()));  //when clicked close dialog and display Nowarning icon

    QGridLayout *layout = new QGridLayout();
    layout->addWidget(mTextEdit, 0, 0, 1, 3);
    layout->addWidget(okButton, 1, 1, 1, 1);
    mDialog->setLayout(layout);

    // Warning object
    mWarning = new QPushButton(mSBar);
    mWarning->setFixedSize(22, 22) ;
    mWarning->setFlat(false);
    mWarning->setIconSize(QSize(15, 15));
    mWarning->setIcon(QPixmap(qsIconFile + "/qview_Warning.png"));
    mWarning->setToolTip("Warning");
    mWarning->setVisible(false);
    mSBar->insertPermanentWidget(0, mWarning);

    connect(mWarning, SIGNAL(clicked()), mDialog, SLOT(show())); // display the dialog window when this button is clicked
    mDialog->resize(800, 250);
    mTextEdit->setBaseSize(750, 200) ;
  }

  /**
   * Destructor for the Warning widget
   *
   * Delete all the dynamically created class members
   *
   * @author sprasad (12/11/2009)
   */
  WarningWidget::~WarningWidget() {

  }

  /**
   * Set the message for the status bar and the dialog window. Highlight
   * the text within "[..]" in red
   *
   * @author sprasad (12/11/2009)
   *
   * @param pStr - error message
   */
  void WarningWidget::setWarningText(std::string pStr) {
    int findChar1 = pStr.find('[', 0);
    std::string redStr = pStr.substr(0, findChar1 + 1);
    redStr += "<font color=#ff0000>";
    int findChar2 = pStr.find(']', 0);
    redStr += pStr.substr(findChar1 + 1, findChar2 - findChar1 - 1);
    redStr += "</font>";
    redStr += pStr.substr(findChar2, pStr.length() - findChar2);
    mSBar->showMessage(QString(redStr.c_str()));
    mTextEdit->setText(QString(redStr.c_str()));
  }

  /**
   * When the dialog "OK" button is clicked or when the mouse is
   * released on some other area or tool the Warning is reset and
   * Nowarning object is displayed.
   *
   * @author sprasad (12/11/2009)
   */
  void WarningWidget::resetWarning(void) {
    if(mbWarningFlag == true) {
      mWarning->setVisible(false);
      mNoWarning->setVisible(true);
      mDialog->setVisible(false);
      mbWarningFlag = false;
      mSBar->showMessage("Ready");
      mMsgStr = "Ready";
    }
  }

  /**
   * Verify that the right message is displayed in the status bar
   *
   * @author sprasad (3/18/2010)
   */
  void WarningWidget::checkMessage(void) {
    if(mbWarningFlag == true && mMsgStr.length()) {
      if(mSBar->currentMessage() != mMsgStr)
        mSBar->showMessage(mMsgStr);
    }
  }
  /**
   * View Warning icon when there is an exception. Usually called
   * when an exception occurs.
   *
   * @author sprasad (12/11/2009)
   *
   * @param pStr   - Warning message sent from the exception handler
   * @param pExStr - Propagated Exception message
   */
  void WarningWidget::viewWarningWidgetIcon(std:: string &pStr,  const std::string   &pExStr) {
    mbWarningFlag = true;
    mWarning->setVisible(true);
    mNoWarning->setVisible(false);
    std::string sStr = "**PROGRAMMER ERROR** " + pStr;
    mMsgStr = sStr.c_str();
    setWarningText(sStr + "<br>" + pExStr);
  }
}
