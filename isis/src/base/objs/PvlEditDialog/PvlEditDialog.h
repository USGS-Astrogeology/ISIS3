#ifndef PvlEditDialog_h
#define PvlEditDialog_h

#include <QDialog>
#include <QString>

#include "Pvl.h"

class QLabel;
class QLineEdit;
class QListWidget;
class QPushButton;
class QTextEdit;

#include <vector>

namespace Isis {
  /**
   * PvlEditDialog creates a QDialog window in which a QTextEdit
   * box displays the contents of a pvl file.  This file may be
   * viewed or edited and saved as a new pvl file.
   *
   *  @ingroup ApplicationInterface
   *
   * @author 2008-12-10 Jeannie Walldren
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Original version
   *            written to view and edit the template file in the
   *            qnet application.
   *   @history 2008-12-10 Jeannie Walldren - Changed namespace
   *            from Qisis to Isis
   *   @history 2008-12-10 Jeannie Walldren - Cleaned up code and
   *            fixed bugs arising from moving this class from
   *            Qisis and to Isis. Added moc commands to Makefile.
   *   @history 2008-12-15 Jeannie Walldren - Added a verification
   *            that the edited file is in Pvl format.  Replaced
   *            error throws with QMessage warning boxes.
   */
  class PvlEditDialog : public QDialog {
      Q_OBJECT

    public:
      PvlEditDialog(Pvl &pvl, QWidget *parent = 0);

    private:
      QTextEdit *p_textEdit;
      QPushButton *p_saveButton;

    private slots:
      void enableSaveButton();
      void saveTextEdit();
  };
};

#endif


