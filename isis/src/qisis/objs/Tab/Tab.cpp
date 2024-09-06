#include "Tab.h"

//#include <iostream>

#include <QWidget>
#include <QStylePainter>

#include "IException.h"
#include "IString.h"


namespace Isis
{

  //! constructs a Tab
  Tab::Tab(QWidget * associatedWidget, QWidget * parent) : QAction(parent)
  {
    this->associatedWidget = associatedWidget;

    position = 0;
    radioGroup = 0;
    selectedStatus = false;

    setCheckable(true);

    connect(this, SIGNAL(triggered()), this, SLOT(handleTriggered()));
  }


  //! destructs a Tab
  Tab::~Tab()
  {
    associatedWidget = NULL;
  }


  //! set the position of the Tab within a TabBar
  void Tab::setPosition(const int & newPosition)
  {
    position = newPosition;
  }


  //! get the position of the Tab within a TabBar
  const int & Tab::getPosition() const
  {
    return position;
  }


  /**
   * Tabs which share a radio group have the property that only only one Tab
   * in the group can be selected at a time.
   *
   * @param newRadioGroup The new radio group to which the Tab should belong
   */
  void Tab::setRadioGroup(const int & newRadioGroup)
  {
    radioGroup = newRadioGroup;
  }


  /**
   * Tabs which share a radio group have the property that only only one Tab
   * in the group can be selected at a time.
   *
   * @returns The radio group which this Tab belongs to
   */
  const int & Tab::getRadioGroup() const
  {
    return radioGroup;
  }


  /**
   * A selected Tab will look visually pressed and have its associatedWidget
   * visible.  A Tab which is not selected will look like a normal button and
   * its associatedWidget will be hidden.
   *
   * @param newStatus True if the Tab should be selected, false otherwise
   */
  void Tab::setSelected(bool newStatus)
  {
    if (associatedWidget == NULL)
    {
      std::string msg = "Tab::setSelected called but can't show or hide the "
          "associatedWidget because it is NULL!";
      throw IException(IException::Programmer, msg,
          _FILEINFO_);
    }
    setChecked(newStatus);
    newStatus ? associatedWidget->show() : associatedWidget->hide();
    selectedStatus = newStatus;
  }


  /**
   * A selected Tab will look visually pressed and have its associatedWidget
   * visible.  A Tab which is not selected will look like a normal button and
   * its associatedWidget will be hidden.
   *
   * @returns True if the Tab is selected, false otherwise
   */
  bool Tab::isSelected()
  {
    return selectedStatus;
  }


  /**
   * This SLOT is executed when the Tab is clicked, and emits its own clicked
   * SIGNAL (which contains its index) to the TabBar.  The TabBar can then
   * use this index to determine which Tab was clicked (TabBars store their Tabs
   * in a QVector).
   */
  void Tab::handleTriggered()
  {
    emit clicked(position);
  }

}
