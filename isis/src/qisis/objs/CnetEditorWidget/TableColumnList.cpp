/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "TableColumnList.h"

#include <iostream>

#include <QList>
#include <QPair>
#include <QString>

#include "IException.h"
#include "TableColumn.h"


using std::cerr;
using std::swap;


namespace Isis {
  TableColumnList::TableColumnList() {
    nullify();

    m_cols = new QList< TableColumn * >;
    m_sortingOrder = new QList< TableColumn * >;
  }


  TableColumnList::TableColumnList(TableColumnList const &other) {
    nullify();

    m_cols = new QList< TableColumn * >(*other.m_cols);
    m_sortingOrder = new QList< TableColumn * >(*other.m_sortingOrder);
  }


  TableColumnList::~TableColumnList() {
    delete m_cols;
    m_cols = NULL;

    delete m_sortingOrder;
    m_sortingOrder = NULL;
  }


  TableColumn *&TableColumnList::operator[](int index) {
    checkIndexRange(index);

    return (*m_cols)[index];
  }


  TableColumn *&TableColumnList::operator[](QString title) {
    for (int i = 0; i < m_cols->size(); i++)
      if (m_cols->at(i)->getTitle() == title)
        return (*m_cols)[i];

    std::string msg = "There is no column with a title of [";
    msg += title;
    msg += "] inside this column list";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  void TableColumnList::append(TableColumn *newCol) {
    if (!newCol) {
      std::string msg = "Attempted to add NULL column to the columnlist";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    m_cols->append(newCol);
    m_sortingOrder->append(newCol);
    connect(newCol, SIGNAL(sortOutDated()), this, SIGNAL(sortOutDated()));
  }


  void TableColumnList::prepend(TableColumn *newCol) {
    m_cols->prepend(newCol);
    m_sortingOrder->append(newCol);
  }


  int TableColumnList::indexOf(TableColumn const *someCol) const {
    int index = -1;
    for (int i = 0; index < 0 && i < m_cols->size(); i++)
      if ((*m_cols)[i] == someCol)
        index = i;

    return index;
  }


  bool TableColumnList::contains(TableColumn const *someCol) const {
    return indexOf(someCol) != -1;
  }


  bool TableColumnList::contains(QString columnTitle) const {
    bool foundTitle = false;
    for (int i = 0; i < m_cols->size() && !foundTitle; i++)
      foundTitle = (m_cols->at(i)->getTitle() == columnTitle);
    return foundTitle;
  }


  void TableColumnList::lower(TableColumn *col, bool emitSortOutDated) {
    int oldIndex = m_sortingOrder->indexOf(col);
    checkIndexRange(oldIndex);

    // if not already lowest priority
    if (oldIndex < m_sortingOrder->size() - 1) {
      m_sortingOrder->removeAt(oldIndex);
      m_sortingOrder->insert(oldIndex + 1, col);
    }

    if (emitSortOutDated)
      emit sortOutDated();
  }


  //void TableColumnList::lower(int visibleColumnIndex, bool emitSortOutDated) {
  //  lower(indexOf(getVisibleColumns()[visibleColumnIndex]), emitSortOutDated);
  //}


  void TableColumnList::raise(TableColumn *col, bool emitSortOutDated) {
    int oldIndex = m_sortingOrder->indexOf(col);
    checkIndexRange(oldIndex);

    // if not already highest priority
    if (oldIndex > 0) {
      m_sortingOrder->removeAt(oldIndex);
      m_sortingOrder->insert(oldIndex - 1, col);
    }

    if (emitSortOutDated)
      emit sortOutDated();
  }


  //void TableColumnList::raise(int visibleColumnIndex, bool emitSortOutDated) {
  //  raise(indexOf(getVisibleColumns()[visibleColumnIndex]), emitSortOutDated);
  //}


  void TableColumnList::raiseToTop(TableColumn *col) {
    while (m_sortingOrder->at(0) != col)
      raise(col, false);

    emit sortOutDated();
  }


  //void TableColumnList::raiseToTop(int visibleColumnIndex) {
  //  raiseToTop(indexOf(getVisibleColumns()[visibleColumnIndex]));
  //}


  int TableColumnList::size() const {
    return m_cols->size();
  }


  TableColumnList &TableColumnList::operator=(
    TableColumnList other) {
    swap(*m_cols, *other.m_cols);
    swap(*m_sortingOrder, *other.m_sortingOrder);

    return *this;
  }


  /**
  * @returns minX, maxX in a pair
  */
  QPair<int, int> TableColumnList::getVisibleXRange(int visibleColumn) {
    int minX = 0;
    int maxX = 0;

    TableColumnList visibleCols = getVisibleColumns();

    if (visibleColumn < visibleCols.size() && visibleColumn >= 0) {
      int indent = 0;

      for (int i = 0; i < visibleColumn; i++)
        indent += visibleCols[i]->getWidth() - 1;

      minX = indent;
      maxX = minX + visibleCols[visibleColumn]->getWidth() - 1;
    }

    return QPair<int, int>(minX, maxX);
  }


  TableColumnList TableColumnList::getVisibleColumns() {
    TableColumnList visibleColumns;

    //     cerr << "TableColumnList::getVisibleColumns() this: " << this << "\n";
    //     cerr << "TableColumnList::getVisibleColumns() m_cols size: " << m_cols->size() << "\n";

    for (int i = 0; i < size(); i++)
      if (m_cols->at(i)->isVisible())
        visibleColumns.append(m_cols->at(i));

    //     cerr << "TableColumnList::getVisibleColumns() visibleColumns: " << visibleColumns.size() << "\n";

    // fix sorting order
    *visibleColumns.m_sortingOrder = *m_sortingOrder;
    for (int i = m_sortingOrder->size() - 1; i >= 0; i--)
      if (!visibleColumns.contains((*visibleColumns.m_sortingOrder)[i]))
        visibleColumns.m_sortingOrder->removeAt(i);

    //     cerr << "TableColumnList::getVisibleColumns() visibleColumns: " << visibleColumns.size() << "\n";

    return visibleColumns;
  }


  int TableColumnList::getVisibleWidth() const {
    int width = 0;

    for (int i = 0; i < size(); i++)
      if (m_cols->at(i)->isVisible())
        width += m_cols->at(i)->getWidth() - 1;
    // For the border...
    width -= 2;

    return width;
  }


  QList< TableColumn * > TableColumnList::getSortingOrder() {

    QList< TableColumn * > validSortingOrder;
    for (int i = 0; i < m_sortingOrder->size(); i++)
      if (m_sortingOrder->at(i)->getTitle().size())
        validSortingOrder.append(m_sortingOrder->at(i));

    return validSortingOrder;
  }


  QStringList TableColumnList::getSortingOrderAsStrings() const {
    QStringList m_sortingOrderStrings;
    for (int i = 0; i < m_sortingOrder->size(); i++)
      if (m_sortingOrder->at(i)->getTitle().size())
        m_sortingOrderStrings.append(m_sortingOrder->at(i)->getTitle());

    return m_sortingOrderStrings;
  }


  void TableColumnList::setSortingOrder(QStringList newOrder) {
    for (int i = newOrder.size() - 1; i >= 0; i--)
      if (contains(newOrder[i]))
        raiseToTop(operator[](newOrder[i]));
  }


  void TableColumnList::checkIndexRange(int index) {

    if (index < 0 || index >= m_cols->size()) {
      std::string msg = "index [";
      msg += index;
      msg += "] is out of range.  Size of list is: ";
      msg += m_cols->size();
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }
  }


  void TableColumnList::nullify() {
    m_cols = NULL;
    m_sortingOrder = NULL;
  }
}
