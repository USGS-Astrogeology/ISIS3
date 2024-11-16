/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "TableViewContent.h"

#include <cmath>
#include <iostream>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QMutex>
#include <QPainter>
#include <QPaintEvent>
#include <QRect>
#include <QScrollBar>
#include <QSize>
#include <QStyle>
#include <QStyleOptionViewItemV4>
#include <QThread>
#include <QVBoxLayout>

#include "ControlMeasure.h"
#include "IException.h"
#include "IString.h"

#include "AbstractTableDelegate.h"
#include "AbstractTableModel.h"
#include "AbstractTreeItem.h"
#include "TableColumn.h"
#include "TableColumnList.h"
#include <ControlPoint.h>


namespace Isis {
  /**
    * Constructor
    *
    * @param someModel The abstract table model
    */
  TableViewContent::TableViewContent(AbstractTableModel *someModel) {
    nullify();

    m_model = someModel;
    connect(m_model, SIGNAL(modelModified()), this, SLOT(refresh()));
    connect(m_model, SIGNAL(filterProgressChanged(int)),
        this, SLOT(updateItemList()));
    connect(this, SIGNAL(modelDataChanged()),
        m_model, SLOT(applyFilter()));
    connect(this,
        SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
        m_model,
        SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)));
    connect(m_model, SIGNAL(treeSelectionChanged(QList< AbstractTreeItem * >)),
        this, SLOT(scrollTo(QList< AbstractTreeItem * >)));

    m_columns = getModel()->getColumns();
    for (int i = 0; i < m_columns->size(); i++) {
      TableColumn *column = (*m_columns)[i];

      connect(column, SIGNAL(visibilityChanged()), this, SLOT(refresh()));
      connect(column, SIGNAL(visibilityChanged()),
          this, SLOT(updateHorizontalScrollBar()));
      connect(column, SIGNAL(widthChanged()), this, SLOT(refresh()));
    }

    m_items = new QList< QPointer<AbstractTreeItem> >;
    m_activeCell = new QPair<AbstractTreeItem *, int>(NULL, -1);
    rowsWithActiveColumnSelected = new QList<AbstractTreeItem *>;
    m_lastShiftSelection = new QList<AbstractTreeItem *>;
    m_lastShiftArrowSelectedCell = new QPair< AbstractTreeItem *, int >;
    m_lastShiftArrowSelectedCell->first = NULL;

    verticalScrollBar()->setSingleStep(1);

    m_rowHeight = QFontMetrics(font()).height() + ITEM_PADDING;

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
        this, SIGNAL(horizontalScrollBarValueChanged(int)));

    setMouseTracking(true);
    updateHorizontalScrollBar();

    createActions();

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
        this, SLOT(showContextMenu(QPoint)));

    m_activeControlNet = false;
  }


  /**
    * Destructor
    */
  TableViewContent::~TableViewContent() {
    delete m_items;
    m_items = NULL;

    delete m_activeCell;
    m_activeCell = NULL;

    delete m_editWidget;
    m_editWidget = NULL;

    delete m_lastShiftSelection;
    m_lastShiftSelection = NULL;

    delete m_applyToSelectionAct;
    m_applyToSelectionAct = NULL;

    delete m_applyToAllAct;
    m_applyToAllAct = NULL;

    delete m_deleteSelectedRowsAct;
    m_deleteSelectedRowsAct = NULL;

    delete m_editControlPointAct;
    m_editControlPointAct = NULL;

    delete m_lastShiftArrowSelectedCell;
    m_lastShiftArrowSelectedCell = NULL;

    m_columns = NULL;
  }


  /**
    * Returns the minimum size hint
    *
    * @return QSize Minimum size hint
    */
  QSize TableViewContent::minimumSizeHint() const {
    return QWidget::minimumSizeHint();
  }


  /**
    * Returns the minimum size hint
    *
    * @see TableViewContent::minimumSizeHint()
    *
    * @return QSize Minimum size hint
    */
  QSize TableViewContent::sizeHint() const {
    return minimumSizeHint();
  }


  /**
    * Returns the model
    *
    * @return AbstractTableModel The model of the table
    */
  AbstractTableModel *TableViewContent::getModel() {
    return m_model;
  }


  //   void TableViewContent::setModel(AbstractTableModel * someModel)
  //   {
  //     if (!someModel)
  //     {
  //       IString msg = "Attempted to set a NULL m_model!";
  //       throw iException::Message(iException::Programmer, msg, _FILEINFO_);
  //     }
  //
  //     if (getModel())
  //     {
  //       disconnect(getModel(), SIGNAL(m_modelModified()), this, SLOT(refresh()));
  //       disconnect(getModel(), SIGNAL(filterProgressChanged(int)),
  //                  this, SLOT(updateItemList()));
  //       disconnect(this,
  //                  SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
  //                  getModel(),
  //                  SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)));
  //       disconnect(getModel(), SIGNAL(selectionChanged(QList<AbstractTreeItem*>)),
  //                  this, SLOT(scrollTo(QList<AbstractTreeItem*>)));
  //     }
  //
  //     m_model = someModel;
  //     m_columns = someModel->getColumns();
  //
  //     connect(m_model, SIGNAL(m_modelModified()), this, SLOT(refresh()));
  //     connect(m_model, SIGNAL(filterProgressChanged(int)),
  //             this, SLOT(updateItemList()));
  //     connect(this, SIGNAL(m_modelDataChanged()),
  //             m_model, SLOT(applyFilter()));
  //     connect(this, SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)),
  //             m_model, SIGNAL(tableSelectionChanged(QList< AbstractTreeItem * >)));
  //     connect(m_model, SIGNAL(treeSelectionChanged(QList<AbstractTreeItem*>)),
  //             this, SLOT(scrollTo(QList<AbstractTreeItem*>)));
  //
  //     refresh();
  //   }


  /**
    * Sets if there is an active control net
    *
    * @param bool The bool if there is an active control net
    */
  void TableViewContent::setActiveControlNet(bool activeNet) {
    m_activeControlNet = activeNet;
  }


  /**
    * Refreshes the table and viewport
    */
  void TableViewContent::refresh() {
    if (m_model) {
      if (!m_model->isFiltering()) {
        int rowCount = m_model->getVisibleRowCount();
        verticalScrollBar()->setRange(0, qMax(rowCount - 1, 0));
      }

      updateItemList();
      m_lastDirectlySelectedRow = NULL;
      m_lastShiftSelection->clear();

      if (m_model->getSelectedItems().size() &&
          rowsWithActiveColumnSelected->size()) {
        m_lastDirectlySelectedRow = NULL;
        clearColumnSelection();
      }

      viewport()->update();
    }
  }


  /**
    * Updates the horizontal scroll bar
    *
    * @param scrollRight True if the horizontal scroll bar has scrolled right
    */
  void TableViewContent::updateHorizontalScrollBar(bool scrollRight) {
    if (m_columns) {
      int range = 0;
      TableColumnList visibleCols = m_columns->getVisibleColumns();
      for (int i = 0; i < visibleCols.size(); i++)
        range += visibleCols[i]->getWidth() - 1;

      // For the border...
      range -= 2;
      horizontalScrollBar()->setRange(0, range - viewport()->width());
      horizontalScrollBar()->setPageStep(viewport()->width());

      if (scrollRight)
        horizontalScrollBar()->setValue(horizontalScrollBar()->maximum());
    }
  }


  /**
    * Scrolls to the selected items
    *
    * @param newlySelectedItems Newly selected items to scroll to
    */
  void TableViewContent::scrollTo(
    QList< AbstractTreeItem * > newlySelectedItems) {
    if (newlySelectedItems.size())
      scrollTo(newlySelectedItems.last());
  }


  /**
    * Scrolls to the selected item
    *
    * @param newlySelectedItem Newly selected item to scroll to
    */
  void TableViewContent::scrollTo(AbstractTreeItem *newlySelectedItem) {
    int row = getModel()->indexOfVisibleItem(newlySelectedItem);

    if (row >= 0) {
      int topRow = verticalScrollBar()->value();

      if (row < topRow) {
        verticalScrollBar()->setValue(row);
      }
      else {
        int wholeVisibleRowCount = viewport()->height() / m_rowHeight;
        int bottomRow = topRow + wholeVisibleRowCount;
        if (row > bottomRow)
          verticalScrollBar()->setValue(row - wholeVisibleRowCount + 1);
      }
    }

    viewport()->update();
  }


  /**
    * Overrides QObject::eventFilter
    *
    * @param target The object that was changed
    * @param event The event that was triggered
    *
    * @return bool True if the event is filtered out
    */
  bool TableViewContent::eventFilter(QObject *target, QEvent *event) {
    return QObject::eventFilter(target, event);
  }


  /**
    * Overrides QWidget::mouseDoubleClickEvent.
    *
    * @param event QMouseEvent
    */
  void TableViewContent::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
      int rowNum = event->pos().y() / m_rowHeight;

      if (m_activeCell->first && cellIsEditable(rowNum, m_activeCell->second)) {
        const AbstractTableDelegate *delegate = m_model->getDelegate();
        TableColumn *col =
          m_columns->getVisibleColumns()[m_activeCell->second];

        delete m_editWidget;
        m_editWidget = NULL;
        m_editWidget = delegate->getWidget(col);
        delegate->readData(m_editWidget, m_activeCell->first, col);
        m_editWidget->setParent(this);
        m_editWidget->setFocus(Qt::OtherFocusReason);
      }

      viewport()->update();
    }
  }


  /**
    * Overrides QWidget::mousePressEvent.
    *
    * @param event QMouseEvent
    */
  void TableViewContent::mousePressEvent(QMouseEvent *event) {

    if (event->buttons() & Qt::LeftButton) {
      // FIXME refactor this file (lol)
      if (!(event->modifiers() & Qt::ShiftModifier))
        updateActiveCell(event->pos());

      int rowNum = event->pos().y() / m_rowHeight;
      int colNum = getColumnFromScreenX(event->pos().x());

      // HACK
      // BUG
      // TODO
      // FIXME
      if (colNum == 0)
        clearActiveCell();

      if (rowNum >= 0 && rowNum < m_items->size() && m_activeCell->first) {
        // The user clicked on a valid item, handle selection of individual
        // cells (not rows).

        // Deselect all rows, as this will now be a cell selection.
        m_model->setGlobalSelection(false);

        if (cellIsEditable(rowNum, m_activeCell->second)) {
          if (event->modifiers() & Qt::ControlModifier) {
            if (rowsWithActiveColumnSelected->indexOf(m_activeCell->first) < 0)
              rowsWithActiveColumnSelected->append(m_activeCell->first);
            else
              rowsWithActiveColumnSelected->removeAll(m_activeCell->first);
            m_lastDirectlySelectedRow = m_activeCell->first;

            m_lastShiftSelection->clear();
          }
          else {
            if (event->modifiers() & Qt::ShiftModifier) {
              updateColumnGroupSelection((*m_items)[rowNum]);
            }
            else {
              // Normal click, no modifiers.
              clearColumnSelection();
              rowsWithActiveColumnSelected->append(m_activeCell->first);
              m_lastDirectlySelectedRow = m_activeCell->first;
              m_lastShiftSelection->clear();
            }
          }
        }
      }
      else {
        // row selections
        if (rowNum >= 0 && rowNum < m_items->size()) {
          int columnNum = getColumnFromScreenX(event->pos().x());

          if (columnNum != -1) {
            TableColumn *column = m_columns->getVisibleColumns()[columnNum];
            if (column->getTitle().isEmpty()) {
              clearColumnSelection();

              AbstractTreeItem *const &item = m_items->at(rowNum);
              QList< AbstractTreeItem * > newlySelectedItems;

              if (event->modifiers() & Qt::ControlModifier) {
                if (item->getPointerType() == AbstractTreeItem::Measure)
                  item->parent()->setSelected(!item->isSelected());

                item->setSelected(!item->isSelected());
                m_lastDirectlySelectedRow = item;
                newlySelectedItems.append(item);
              }
              else {
                if (event->modifiers() & Qt::ShiftModifier) {
                  newlySelectedItems = updateRowGroupSelection(rowNum);
                }
                else {
                  QList<AbstractTreeItem *> selectedItems =
                    m_model->getSelectedItems();

                  foreach (AbstractTreeItem * selectedItem, selectedItems) {
                    if (selectedItem->getPointerType() ==
                        AbstractTreeItem::Measure)
                      selectedItem->parent()->setSelected(false);
                  }

                  m_model->setGlobalSelection(false);

                  if (item->getPointerType() == AbstractTreeItem::Measure)
                    item->parent()->setSelected(true);

                  item->setSelected(true);
                  m_lastDirectlySelectedRow = item;
                  newlySelectedItems.append(item);
                }
              }

              QList< AbstractTreeItem * > tmp = newlySelectedItems;
              newlySelectedItems.clear();
              foreach (AbstractTreeItem * i, tmp) {
                newlySelectedItems.append(i);
                if (i->getPointerType() == AbstractTreeItem::Point) {
                  foreach (AbstractTreeItem * child, i->getChildren()) {
                    child->setSelected(true);
                    newlySelectedItems.append(child);
                  }
                }
              }

              emit tableSelectionChanged(newlySelectedItems);
            }
          }
        }
      }

      delete m_editWidget;
      m_editWidget = NULL;

      viewport()->update();
      emit tableSelectionChanged();
    }
  }


  /**
    * Overrides QWidget::mouseReleaseEvent. Empty function
    *
    * @param event QMouseEvent
    */
  void TableViewContent::mouseReleaseEvent(QMouseEvent *event) {
  }


  /**
    * Overrides QWidget::mouseMoveEvent.
    *
    * @param event QMouseEvent
    */
  void TableViewContent::mouseMoveEvent(QMouseEvent *event) {
    if (!m_editWidget) {
      if (event->buttons() & Qt::LeftButton) {
        int rowNum = event->pos().y() / m_rowHeight;

        // used to make sure that the mouse position is inside the content
        int yPos = event->pos().y();
        if (yPos >= 0 && rowNum < m_items->size() && m_activeCell->first) {
          // The user clicked on a valid item,
          // handle selection of individual cells (not rows).
          if (cellIsEditable(rowNum, m_activeCell->second)) {
            updateColumnGroupSelection((*m_items)[rowNum]);
          }
        }
        else {
          // row selections
          if (yPos >= 0 && rowNum < m_items->size()) {
            // There is no active cell,
            // maybe they clicked on the row number column.
            int columnNum = getColumnFromScreenX(event->pos().x());

            if (columnNum != -1) {
              clearColumnSelection();

              QList< AbstractTreeItem * > tmp =
                updateRowGroupSelection(rowNum);
              QList< AbstractTreeItem * > newlySelectedItems;
              foreach (AbstractTreeItem * i, tmp) {
                newlySelectedItems.append(i);
                if (i->getPointerType() == AbstractTreeItem::Point) {
                  foreach (AbstractTreeItem * child, i->getChildren()) {
                    child->setSelected(true);
                    newlySelectedItems.append(child);
                  }
                }
              }

              emit tableSelectionChanged(newlySelectedItems);
            }
          }
        }

        QScrollBar *vertScroll = verticalScrollBar();

        if (yPos > viewport()->height() &&
            vertScroll->value() < vertScroll->maximum()) {
          // Scroll down to allow for more drag selections.
          vertScroll->setValue(vertScroll->value() + 1);
        }
        else {
          if (yPos < 0 && vertScroll->value() > vertScroll->minimum())
            vertScroll->setValue(vertScroll->value() - 1);
        }

        viewport()->update();
        emit tableSelectionChanged();
      }
    }
  }


  /**
    * Overrides QWidget::leaveEvent
    *
    * @param event QMouseEvent
    */
  void TableViewContent::leaveEvent(QEvent *event) {
    //     viewport()->update();
  }


  /**
    * Overrides QWidget::keyPressEvent
    *
    * @param event QMouseEvent
    */
  void TableViewContent::keyPressEvent(QKeyEvent *event) {
    Qt::Key key = (Qt::Key) event->key();

    // Handle Ctrl-A (selects all rows)
    if (key == Qt::Key_A && event->modifiers() == Qt::ControlModifier) {
      clearActiveCell();
      clearColumnSelection();
      m_model->setGlobalSelection(true);
      viewport()->update();

      emit tableSelectionChanged();
    }

    // Handle esc key (cancel editing)
    else if (key == Qt::Key_Escape) {
      if (m_editWidget) {
        delete m_editWidget;
        m_editWidget = NULL;
        setFocus(Qt::ActiveWindowFocusReason);
        viewport()->update();
      }
    }

    // Handle delete key (delete row(s) if any are selected)
    else if (key == Qt::Key_Delete) {
      if (hasRowSelection())
        deleteSelectedRows();
    }

    // Handle return or enter (stop editing)
    else if (key == Qt::Key_Return || key == Qt::Key_Enter) {
      finishEditing();
      moveActiveCellDown();
    }

    // Handle
    else if (key == Qt::Key_Tab) {
      finishEditing();
      moveActiveCellRight();
    }

    // Handle arrow key navigation
    else if (key == Qt::Key_Up || key == Qt::Key_Down ||
        key == Qt::Key_Left || key == Qt::Key_Right) {
      if (!hasActiveCell()) {
        if (m_items && m_items->size()) {
          m_activeCell->first = (*m_items)[0];
          m_activeCell->second = 1;
        }
      }

      if (hasActiveCell() && !m_editWidget) {

        // Handle up arrow with shift pressed
        if (key == Qt::Key_Up && event->modifiers() == Qt::ShiftModifier) {

          AbstractTreeItem *prevCell = m_lastShiftArrowSelectedCell->first ?
              m_lastShiftArrowSelectedCell->first : m_activeCell->first;

          int prevCellIndex = getModel()->indexOfVisibleItem(prevCell);

          if (prevCellIndex > 0) {
            QList< AbstractTreeItem * > itemList =
              getModel()->getItems(prevCellIndex - 1, prevCellIndex);

            if (itemList.size()) {
              AbstractTreeItem *curItem = itemList[0];

              if (rowsWithActiveColumnSelected->contains(curItem) ||
                  curItem == m_activeCell->first)
                rowsWithActiveColumnSelected->removeAll(prevCell);
              else
                rowsWithActiveColumnSelected->append(curItem);

              if (curItem == m_activeCell->first)
                m_lastShiftArrowSelectedCell->first = NULL;
              else
                m_lastShiftArrowSelectedCell->first = curItem;
              m_lastShiftArrowSelectedCell->second = m_activeCell->second;

              // scroll if needed
              int m_itemsPrevIndex = m_items->indexOf(prevCell);
              int m_itemsCurIndex = m_items->indexOf(curItem);
              if (m_itemsCurIndex == -1) {
                if (m_itemsPrevIndex == 0) { // then need to scroll up!
                  verticalScrollBar()->setValue(qMax(0, prevCellIndex - 1));
                }
                else {
                  //int m_itemsLastIndex = m_items->size() - 1;
                  //verticalScrollBar()->setValue(qMin(prevCell + 1, ));
                }
              }

              viewport()->update();
            }
          }
        }

        // Handle down arrow with shift pressed
        else if (key == Qt::Key_Down && event->modifiers() == Qt::ShiftModifier) {
          AbstractTreeItem *prevCell = m_lastShiftArrowSelectedCell->first ?
              m_lastShiftArrowSelectedCell->first : m_activeCell->first;

          int prevCellIndex = getModel()->indexOfVisibleItem(prevCell);

          if (prevCellIndex >= 0 &&
              prevCellIndex < getModel()->getVisibleRowCount() - 1) {
            QList< AbstractTreeItem * > itemList =
              getModel()->getItems(prevCellIndex + 1, prevCellIndex + 2);

            if (itemList.size()) {
              AbstractTreeItem *curItem = itemList[0];

              if (rowsWithActiveColumnSelected->contains(curItem) ||
                  curItem == m_activeCell->first)
                rowsWithActiveColumnSelected->removeAll(prevCell);
              else
                rowsWithActiveColumnSelected->append(curItem);

              if (curItem == m_activeCell->first)
                m_lastShiftArrowSelectedCell->first = NULL;
              else
                m_lastShiftArrowSelectedCell->first = curItem;
              m_lastShiftArrowSelectedCell->second = m_activeCell->second;
              viewport()->update();

              // scroll if needed
              int m_itemsPrevIndex = m_items->indexOf(prevCell);
              int m_itemsCurIndex = m_items->indexOf(curItem);
              if (m_itemsCurIndex == -1) {
                if (m_itemsPrevIndex == m_items->size() - 1) {
                  int visibleItemCount = getModel()->getVisibleRowCount();
                  verticalScrollBar()->setValue(qMin(visibleItemCount,
                      getModel()->indexOfVisibleItem(m_items->at(1))));
                }
                else {
                  //int m_itemsLastIndex = m_items->size() - 1;
                  //verticalScrollBar()->setValue(qMin(prevCell + 1, ));
                }
              }
            }
          }
        }

        // Handle up arrow
        else if (key == Qt::Key_Up) {
          moveActiveCellUp();
        }

        // Handle down arrow
        else if (key == Qt::Key_Down) {
          moveActiveCellDown();
        }

        // Handle left arrow
        else if (key == Qt::Key_Left) {
          moveActiveCellLeft();
        }

        // Handle right arrow
        else if (key == Qt::Key_Right) {
          moveActiveCellRight();
        }
      }
    }

    // start editing the active cell
    else {
      // event->text() will be empty if a modifier was pressed.
      if (hasActiveCell() && !event->text().isEmpty()) {
        if (!m_items->contains(m_activeCell->first))
          scrollTo(m_activeCell->first);

        if (m_items->contains(m_activeCell->first) &&
            cellIsEditable(m_items->indexOf(m_activeCell->first),
                m_activeCell->second)) {
          AbstractTableDelegate const *delegate = m_model->getDelegate();
          TableColumn *col =
            m_columns->getVisibleColumns()[m_activeCell->second];

          delete m_editWidget;
          m_editWidget = NULL;
          m_editWidget = delegate->getWidget(col);
          delegate->readData(m_editWidget, m_activeCell->first, col, event->text());
          m_editWidget->setParent(this);
          m_editWidget->setFocus(Qt::OtherFocusReason);
        }

        viewport()->update();
      }
    }
  }


  /**
    * Saves the data from the cell the user was modifying
    */
  void TableViewContent::finishEditing() {
    if (m_editWidget) {
      TableColumn *col =
        m_columns->getVisibleColumns()[m_activeCell->second];
      getModel()->getDelegate()->saveData(
        m_editWidget, m_activeCell->first, col);
      delete m_editWidget;
      m_editWidget = NULL;

      cellDataChanged(col);
      setFocus(Qt::ActiveWindowFocusReason);
    }
  }


  /**
    * Shifts the active cell up
    */
  void TableViewContent::moveActiveCellUp() {
    int activeIndex = m_items->indexOf(m_activeCell->first);
    if (activeIndex != -1) {
      if (activeIndex == 0) {
        int row = qMax(0, getModel()->indexOfVisibleItem(
            m_activeCell->first) - 1);

        verticalScrollBar()->setValue(row);
      }

      m_activeCell->first = (*m_items)[qMax(0, activeIndex - 1)];
      clearColumnSelection();
      viewport()->update();
    }
  }


  /**
    * Changes the viewport when the active cell is moved.
    */
  void TableViewContent::moveActiveCellDown() {
    int activeIndex = m_items->indexOf(m_activeCell->first);
    if (activeIndex != -1) {
      if (activeIndex == m_items->size() - 1) {
        int row = qMin(getModel()->getVisibleRowCount() - 1,
            getModel()->indexOfVisibleItem(m_items->at(0)));

        verticalScrollBar()->setValue(row + 1);
        activeIndex = m_items->indexOf(m_activeCell->first);
      }

      m_activeCell->first = (*m_items)[qMin(activeIndex + 1, m_items->size() - 1)];
      clearColumnSelection();
      viewport()->update();
    }
  }


  /**
    * Changes the viewport when the active cell is moved.
    */
  void TableViewContent::moveActiveCellLeft() {
    m_activeCell->second = qMax(1, m_activeCell->second - 1);
    int leftMostVisibleCol = getColumnFromScreenX(0);
    if (leftMostVisibleCol == m_activeCell->second) {
      horizontalScrollBar()->setValue(horizontalScrollBar()->value() -
          m_columns->getVisibleColumns()[m_activeCell->second]->getWidth());
    }

    clearColumnSelection();
    viewport()->update();
  }


  /**
    * Changes the viewport when the active cell is moved.
    */
  void TableViewContent::moveActiveCellRight() {
    m_activeCell->second = qMin(m_columns->getVisibleColumns().size() - 1,
        m_activeCell->second + 1);
    int rightMostVisibleCol = getColumnFromScreenX(viewport()->width());
    if (rightMostVisibleCol == m_activeCell->second) {
      horizontalScrollBar()->setValue(horizontalScrollBar()->value() +
          m_columns->getVisibleColumns()[m_activeCell->second]->getWidth());
    }

    clearColumnSelection();
    viewport()->update();
  }


  /**
    * Paints the table when there is a paint event. Overrides QWidget::paintEvent
    *
    * @param event The paint event
    */
  void TableViewContent::paintEvent(QPaintEvent *event) {
    if (m_model && m_columns) {
      //       int startRow = verticalScrollBar()->value();
      int rowCount = (int) ceil(viewport()->height() / (double) m_rowHeight);

      QPainter painter(viewport());
      painter.setRenderHint(QPainter::Antialiasing, false);
      //      painter.setRenderHints(QPainter::NonCosmeticDefaultPen, true);

      bool editWidgetVisible = false;
      for (int i = 0; i < rowCount; i++) {
        // define the top left corner of the row and also how big the row is
        QPoint relativeTopLeft(0, i * m_rowHeight);
        QPoint scrollBarPos(horizontalScrollBar()->value(),
            verticalScrollBar()->value());
        QPoint absoluteTopLeft(relativeTopLeft + scrollBarPos);
        QSize rowSize(viewport()->width(), (int) m_rowHeight);

        // Fill in the background with the background color
        painter.fillRect(QRect(relativeTopLeft, rowSize), palette().base());

        if (i < m_items->size()) {






          // ********************************************************
          // HACK: FIXME: ask tree m_items if background needs to be
          // darkened, instead figuring that out here.  Also, change
          // composition mode so that ref measure rows look different
          // when they are highlighted as well
          /*
          if (m_items->at(i)->getPointerType() == AbstractTreeItem::Measure)
          {
            ControlMeasure * cm = (ControlMeasure *) m_items->at(i)->getPointer();
            if (cm->Parent()->GetRefMeasure() == cm)
            {
              QPoint selectionTopLeft(-absoluteTopLeft.x(), relativeTopLeft.y());
              QSize selectionSize(m_columns->getVisibleWidth(), (int) m_rowHeight);

              QRect selectionRect(selectionTopLeft, selectionSize);
              QColor color = palette().base().color();
              painter.fillRect(selectionRect, color.darker(110));
            }
          }
          */
          //*********************************************************












          if (m_items->at(i)->isSelected()) {
            QPoint selectionTopLeft(-absoluteTopLeft.x(), relativeTopLeft.y());
            QSize selectionSize(m_columns->getVisibleWidth(), (int) m_rowHeight);

            QRect selectionRect(selectionTopLeft, selectionSize);
            painter.fillRect(selectionRect, palette().highlight().color());
          }

          paintRow(&painter, i, absoluteTopLeft, relativeTopLeft);
        }
      }

      for (int i = 0; i < rowCount; i++) {
        if (i < m_items->size()) {
          QPoint relativeTopLeft(0, i * m_rowHeight);
          if (m_editWidget && m_activeCell->first == m_items->at(i)) {
            QPair<int, int> xRange(
              m_columns->getVisibleXRange(m_activeCell->second));

            m_editWidget->move(
              QPoint(xRange.first - horizontalScrollBar()->value() - 1,
                  relativeTopLeft.y() + 1));
            m_editWidget->resize(xRange.second - xRange.first, m_rowHeight + 1);
            m_editWidget->setVisible(true);
            editWidgetVisible = true;
          }
          else {
            if (m_activeCell->first == m_items->at(i)) {
              QPair<int, int> activeXArea =
                m_columns->getVisibleXRange(m_activeCell->second);

              QRect activeArea(activeXArea.first, relativeTopLeft.y(),
                  activeXArea.second - activeXArea.first, m_rowHeight);

              activeArea.moveLeft(
                activeArea.left() - horizontalScrollBar()->value());
              activeArea.adjust(-1, -1, -2, -1);
              QPen pen(Qt::black);
              pen.setWidth(3);
              painter.setPen(pen);
              painter.drawRect(activeArea);
            }
          }
        }
      }

      if (m_editWidget && !editWidgetVisible)
        m_editWidget->setVisible(false);
    }
    else {
      QWidget::paintEvent(event);
    }
  }


  /**
    * Updates the table when it is resized.
    *
    * @param event Resize event
    */
  void TableViewContent::resizeEvent(QResizeEvent *event) {
    QAbstractScrollArea::resizeEvent(event);
    updateHorizontalScrollBar();
    updateItemList();
  }


  /**
    * Updates the item list when the user scrolls
    *
    * @param dx X scroll
    * @param dy Y scroll
    */
  void TableViewContent::scrollContentsBy(int dx, int dy) {
    QAbstractScrollArea::scrollContentsBy(dx, dy);
    updateItemList();
  }


  /**
    * Clears all member variables
    */
  void TableViewContent::nullify() {
    m_parentView = NULL;
    m_model = NULL;
    m_items = NULL;
    m_activeCell = NULL;
    m_lastShiftArrowSelectedCell = NULL;
    m_editWidget = NULL;
    m_lastDirectlySelectedRow = NULL;
    m_lastShiftSelection = NULL;
    m_columns = NULL;
    rowsWithActiveColumnSelected = NULL;
    m_applyToSelectionAct = NULL;
    m_applyToAllAct = NULL;
    m_deleteSelectedRowsAct = NULL;
    m_editControlPointAct = NULL;
  }


  /**
    * Rebuilds the models when the data is changed
    *
    * @param col The table column that changed
    */
  void TableViewContent::cellDataChanged(const TableColumn *col) {
    if (col->hasNetworkStructureEffect())
      emit rebuildModels(QList< AbstractTreeItem * >());

    emit modelDataChanged();
  }


  /**
    * Clears the active cell
    */
  void TableViewContent::clearActiveCell() {
    m_activeCell->first = NULL;
    m_activeCell->second = -1;
  }


  /**
    * Clears the selected column
    */
  void TableViewContent::clearColumnSelection() {
    m_lastShiftArrowSelectedCell->first = NULL;
    rowsWithActiveColumnSelected->clear();
  }


  /**
    * Copies the selected cells
    *
    * @param allCells Determines if all of the visible rows should be copied
    */
  void TableViewContent::copyCellSelection(bool allCells) {
    if (hasActiveCell()) {
      TableColumn *col = m_columns->getVisibleColumns()[m_activeCell->second];

      QString colTitle = col->getTitle();

      // Grab the active cell's data and copy it to the selected cells that are
      // in the same column as the active cell.
      QString cellData = m_activeCell->first->getFormattedData(colTitle);

      QList< AbstractTreeItem * > selection = allCells ? m_model->getItems(
          0, m_model->getVisibleRowCount()) : *rowsWithActiveColumnSelected;

      bool needsDialog = true;
      bool done = false;
      for (int i = 0; !done && i < selection.size(); i++) {
        AbstractTreeItem *row = selection[i];
        bool changeData = true;

        QString warningText =
          m_model->getWarningMessage(row, col, cellData);
        if (needsDialog && warningText.size()) {
          QMessageBox::StandardButton status = QMessageBox::warning(
              this, "Change cells?", warningText, QMessageBox::Yes |
              QMessageBox::No | QMessageBox::YesToAll | QMessageBox::NoToAll);

          switch (status) {
            case QMessageBox::YesToAll:
              needsDialog = false;
              break;
            case QMessageBox::NoToAll:
              done = true;
            case QMessageBox::No:
              changeData = false;
            default:
              ;
          }
        }

        if (changeData)
          row->setData(colTitle, cellData);
      }

      viewport()->update();
      cellDataChanged(col);
    }
  }


  /**
    * Builds the menus
    */
  void TableViewContent::createActions() {
    m_applyToSelectionAct = new QAction(tr("Copy to selected cells"), this);
    m_applyToSelectionAct->setStatusTip(tr("Copy the contents of this cell to the"
        "selected cells"));
    connect(m_applyToSelectionAct, SIGNAL(triggered()),
        this, SLOT(copySelection()));

    m_applyToAllAct = new QAction(tr("Copy to all cells"), this);
    m_applyToAllAct->setStatusTip(tr("Copy the contents of this cell to all"
        "cells in the current column"));
    connect(m_applyToAllAct, SIGNAL(triggered()),
        this, SLOT(copyAll()));

    m_deleteSelectedRowsAct = new QAction(tr("Delete selected rows"), this);
    m_deleteSelectedRowsAct->setStatusTip(
      tr("Delete the currently selected rows"));
    connect(m_deleteSelectedRowsAct, SIGNAL(triggered()),
        this, SLOT(deleteSelectedRows()));

    m_editControlPointAct = new QAction(tr("Edit selected control point"), this);
    m_editControlPointAct->setStatusTip(
      tr("Edit the selected control point or the parent control point of control measure"));
    connect(m_editControlPointAct, SIGNAL(triggered()),
        this, SLOT(editControlPoint()));
  }


  /**
    * Calculates the visible range of a column and returns the index of the column
    *
    * @param screenX X value of the screen size
    *
    * @return int Column that fits the screen, or if there aren't any columns that fit -1
    */
  int TableViewContent::getColumnFromScreenX(int screenX) const {

    for (int i = 0; i < m_columns->getVisibleColumns().size(); i++) {
      QPair<int, int> cellXRange(m_columns->getVisibleXRange(i));
      int deltaX = -horizontalScrollBar()->value();

      if (cellXRange.first + deltaX < screenX &&
          cellXRange.second  + deltaX > screenX) {
        return i;
      }
    }

    return -1;
  }


  /**
    * Calculates the visible range of a row and returns the index of the column
    *
    * @param screenY Y value of the screen size
    *
    * @return int Row that fits the screen, or if there aren't any columns that fit -1
    */
  int TableViewContent::getRowFromScreenY(int screenY) const {
    int calculatedRowNum = screenY / m_rowHeight;

    if (calculatedRowNum >= 0 && calculatedRowNum < m_items->size() &&
        screenY >= 0 && screenY <= viewport()->height())
      return calculatedRowNum;

    return -1;
  }


  /**
    * Checks if there is an active cell
    *
    * @return bool True if there is an active cell
    */
  bool TableViewContent::hasActiveCell() const {
    return (m_activeCell->first && m_activeCell->second >= 0);
  }


  /**
    * Checks if there is a row selected
    *
    * @return bool True if there is a row selected
    */
  bool TableViewContent::hasRowSelection() const {
    return (m_model->getSelectedItems().size());
  }


  /**
    * Checks if the mouse is in the selected cells
    *
    * @param mousePos Mouse position
    *
    * @return bool True if the mouse is in the selected cells
    */
  bool TableViewContent::mouseInCellSelection(QPoint mousePos) const {
    int colNum = getColumnFromScreenX(mousePos.x());
    AbstractTreeItem *row = m_items->at(getRowFromScreenY(mousePos.y()));

    return (rowsWithActiveColumnSelected->contains(row) &&
        m_activeCell->second == colNum);
  }


  /**
    * Checks if the mouse is in the selected row
    *
    * @param mousePos Mouse position
    *
    * @return bool True if the mouse is in the selected row
    */
  bool TableViewContent::mouseInRowSelection(QPoint mousePos) const {
    AbstractTreeItem *row = m_items->at(getRowFromScreenY(mousePos.y()));

    return (m_model->getSelectedItems().contains(row));
  }


  /**
    * Checks if the row number is valid
    *
    * @param rowNum Row to check
    *
    * @return bool True if the row is greater or equal to 0 and less than m_items->size()
    */
  bool TableViewContent::rowIsValid(int rowNum) const {
    bool valid = false;

    if (rowNum >= 0 && rowNum < m_items->size())
      valid = true;

    return valid;
  }


  /**
    * Checks if the column number is valid
    *
    * @param colNum Column to check
    *
    * @return bool True if the column is greater or equal to 0 and less than m_items->size()
    */
  bool TableViewContent::columnIsValid(int colNum) const {
    bool valid = false;

    if (colNum >= 0 && colNum < m_columns->getVisibleColumns().size())
      valid = true;

    return valid;
  }


  /**
    * Checks if the cell is editable
    *
    * @param rowNum The cell's row
    * @param colNum The cell's column
    *
    * @return bool True if the cell is selectable and editable
    */
  bool TableViewContent::cellIsEditable(int rowNum, int colNum) const {

    bool editable = false;

    QString colName = m_columns->getVisibleColumns()[colNum]->getTitle();
    if (m_items->at(rowNum)->isSelectable() &&
        m_items->at(rowNum)->isDataEditable(colName) &&
        !m_columns->getVisibleColumns()[colNum]->isReadOnly())
      editable = true;

    return editable;
  }


  /**
    * Checks if the column has a non empty title
    *
    * @param colNum The column to check
    *
    * @return bool True if the column has a non-empty title
    */
  bool TableViewContent::isDataColumn(int colNum) const {
    //TODO: Figure out what this does and if we can do it in a less confusing and roundabout way
    return m_columns->getVisibleColumns()[colNum]->getTitle().size();
  }


  /**
    * Repaints the row
    *
    * @param painter The QPainter
    * @param rowNum The row to repaint
    * @param absolutePosition The row position
    * @param relativePosition The row position in the visible area
    */
  void TableViewContent::paintRow(QPainter *painter, int rowNum,
      QPoint absolutePosition, QPoint relativePosition) {

    QPoint point(-absolutePosition.x(), relativePosition.y());

    AbstractTreeItem *item = (*m_items)[rowNum];

    // should always be true, but prevents segfault in case of bug
    if (item) {
      QPen originalPen = painter->pen();

      QPoint textPoint(point.x() + ITEM_INDENTATION,
          point.y() + ITEM_PADDING / 2);

      // finally draw the text
      int textHeight = m_rowHeight - ITEM_PADDING;

      QFontMetrics metrics(font());

      QPen gridPen(Qt::gray);

      TableColumnList visibleCols = m_columns->getVisibleColumns();
      for (int i = 0; i < visibleCols.size(); i++) {
        // draw text
        QPair<int, int> cellXRange(visibleCols.getVisibleXRange(i));
        QRect cellRect(cellXRange.first, point.y(),
            cellXRange.second - cellXRange.first, m_rowHeight);
        cellRect.moveLeft(cellRect.left() - horizontalScrollBar()->value() - 1);

        QString columnTitle = visibleCols[i]->getTitle();
        QRect textRect(textPoint, QSize(cellRect.right() - textPoint.x(),
            textHeight));
        QString text;
        bool textCentered = false;
        if (!columnTitle.isEmpty()) {
          text = item->getFormattedData(columnTitle);

          if (rowsWithActiveColumnSelected->indexOf(item) != -1 &&
              m_activeCell->second == i) {
            // This cell is selected, so render it as such.
            if (m_activeCell->first != item) {
              painter->fillRect(cellRect,
                  QBrush(palette().highlight().color()));
              painter->setPen(palette().highlightedText().color());
            }
            else {
              painter->setPen(palette().text().color());
            }
          }
          else {
            if (item->isSelected()) {
              painter->setPen(palette().highlightedText().color());
            }
            else {
              // If the current column is not editable (i.e. read-only), or it
              // is locked (think edit-locked control point or measure), then
              // it should be grayed out.
              //
              // NOTE: The following two lines provide for graying out of edit
              // locked data. This is commented out because this functionality
              // is not fully working yet.
              //if (!cellIsEditable(rowNum, i) ||
              //    item->isDataLocked(columnTitle))
              if (!cellIsEditable(rowNum, i)) {
                painter->setPen(palette().color(QPalette::Disabled,
                    QPalette::Text));
              }
              else {
                painter->setPen(palette().text().color());
              }
            }
          }
        }
        else {
          // Draw the row number.
          text = QString::number(rowNum + verticalScrollBar()->value() + 1);
          textCentered = true;

          // We need to paint the row number column.
          int x = cellRect.center().x();
          QLinearGradient gradient(x, cellRect.top(), x, cellRect.bottom());

          bool selected = item->isSelected();
          QColor color = selected ? palette().highlight().color() :
              palette().button().color();

          int adjustment = 110;
          gradient.setColorAt(0, color.lighter(adjustment));
          gradient.setColorAt(1, color.darker(adjustment));
          painter->fillRect(cellRect, gradient);
          if (selected)
            painter->setPen(palette().highlightedText().color());
          else
            painter->setPen(palette().text().color());
        }

        int flags = Qt::TextDontClip;
        if (textCentered)
          flags |= Qt::AlignCenter;

        // TODO this shouldn't be here... an item should be able to tell
        // whether or not it should be bolded.
        QFont normalFont = painter->font();

        if (item->getPointerType() == AbstractTreeItem::Measure) {
          ControlMeasure *cm = (ControlMeasure *) item->getPointer();
          if (cm && cm->Parent() && cm->Parent()->GetRefMeasure() == cm) {
            QFont boldFont(normalFont);
            boldFont.setBold(true);
            painter->setFont(boldFont);
          }
        }

        painter->drawText(textRect, flags,
            metrics.elidedText(text, Qt::ElideRight,
                textRect.width() - ITEM_INDENTATION));
        painter->setFont(normalFont);

        textPoint.setX(cellRect.right() + ITEM_INDENTATION);
        painter->setPen(originalPen);

        painter->setPen(gridPen);
        painter->drawLine(QPoint(cellRect.right(), point.y()),
            QPoint(cellRect.right(), point.y() + m_rowHeight));
        painter->setPen(originalPen);
      }

      int left = -horizontalScrollBar()->value() - 1;
      int right = m_columns->getVisibleWidth();

      gridPen.setWidth(2);
      painter->setPen(gridPen);
      painter->drawLine(QPoint(left, point.y() + m_rowHeight),
          QPoint(right, point.y() + m_rowHeight));
      painter->setPen(originalPen);
    }
  }


  /**
    * Updates which cell is active
    *
    * @param screenPos The position of the active cell
    */
  void TableViewContent::updateActiveCell(QPoint screenPos) {
    if (m_editWidget && m_activeCell->first && m_activeCell->second >= 0) {
      try {
        const TableColumn *col =
          m_columns->getVisibleColumns()[m_activeCell->second];
        m_model->getDelegate()->saveData(m_editWidget, m_activeCell->first,
            col);

        cellDataChanged(col);
      }
      catch (IException &e) {
        QMessageBox::critical(this, "Failed to Set Data", e.what());
      }
    }

    int rowNum = getRowFromScreenY(screenPos.y());
    int oldActiveColumn = m_activeCell->second;

    clearActiveCell();

    if (rowNum >= 0) {
      AbstractTreeItem *item = (*m_items)[rowNum];

      TableColumnList visibleCols = m_columns->getVisibleColumns();
      for (int i = 0; i < visibleCols.size(); i++) {
        QPair<int, int> cellXRange(m_columns->getVisibleXRange(i));
        QRect cellRect(cellXRange.first, m_rowHeight * rowNum,
            cellXRange.second - cellXRange.first, m_rowHeight);

        cellRect.moveLeft(cellRect.left() - horizontalScrollBar()->value());

        if (cellRect.contains(screenPos) &&
            (oldActiveColumn != -1 || !visibleCols[i]->getTitle().isEmpty())) {
          m_activeCell->first = item;
          m_activeCell->second = i;
        }
      }
    }

    if (oldActiveColumn != m_activeCell->second) {
      clearColumnSelection();
      m_lastDirectlySelectedRow = NULL;
    }

    clearColumnSelection();
  }


  /**
    * Updates which column is selected
    *
    * @param item The new selected group
    */
  void TableViewContent::updateColumnGroupSelection(AbstractTreeItem *item) {
    // delete current row selection
    foreach (AbstractTreeItem * row, *m_lastShiftSelection) {
      if (rowsWithActiveColumnSelected->indexOf(row) != -1)
        rowsWithActiveColumnSelected->removeOne(row);
    }


    if (m_lastDirectlySelectedRow) {
      *m_lastShiftSelection = m_model->getItems(
          m_lastDirectlySelectedRow, item);
    }
    else {
      m_lastShiftSelection->clear();
    }

    foreach (AbstractTreeItem * row, *m_lastShiftSelection) {
      if (rowsWithActiveColumnSelected->indexOf(row) == -1)
        rowsWithActiveColumnSelected->append(row);
    }
  }


  /**
    * Updates which row is selected
    *
    * @param lastRow The index of the last row
    *
    * @return QList< AbstractTreeItem * > The newly selected row
    */
  QList< AbstractTreeItem * > TableViewContent::updateRowGroupSelection(int lastRow) {
    foreach (AbstractTreeItem * row, *m_lastShiftSelection) {
      if (row->getPointerType() == AbstractTreeItem::Point)
        foreach (AbstractTreeItem * child, row->getChildren())
        child->setSelected(false);

      if (row->getPointerType() == AbstractTreeItem::Measure)
        row->parent()->setSelected(false);

      row->setSelected(false);
    }

    if (m_lastDirectlySelectedRow) {
      *m_lastShiftSelection = m_model->getItems(
          m_lastDirectlySelectedRow, m_items->at(lastRow));
    }
    else {
      m_lastShiftSelection->clear();
    }

    QList< AbstractTreeItem * > newlySelectedItems;
    foreach (AbstractTreeItem * row, *m_lastShiftSelection) {
      row->setSelected(true);

      if (row->getPointerType() == AbstractTreeItem::Measure)
        row->parent()->setSelected(true);

      newlySelectedItems.append(row);
    }

    return newlySelectedItems;
  }


  /**
    * Copies selected cells
    *
    * @see copyCellSelection
    */
  void TableViewContent::copySelection() {
    copyCellSelection(false);
  }


  /**
    * Copies all of the cells
    *
    * @see copyCellSelection
    */
  void TableViewContent::copyAll() {
    copyCellSelection(true);
  }


  /**
    * Deletes the selected rows
    */
  void TableViewContent::deleteSelectedRows() {
    // Prompt the user for confirmation before deletion.
    QMessageBox::StandardButton status = QMessageBox::warning(
        this, "Delete row(s)?", "Delete selected row(s)?",
        QMessageBox::Yes | QMessageBox::No);

    if (status == QMessageBox::Yes) {
      // TODO should we store off the selected rows for efficiency?
      QList<AbstractTreeItem *> selectedRows = m_model->getSelectedItems();

      emit rebuildModels(selectedRows);
      emit modelDataChanged();

      m_lastShiftSelection->clear();
    }
  }


  /**
    * Retrieves the control point from the selected cells for editing
    */
  void TableViewContent::editControlPoint() {

    ControlPoint *cp;
    QString serialNumber;
    AbstractTreeItem *item;

    //  A single cell is chosen
    if (m_model->getSelectedItems().count() == 0) {
      item = m_activeCell->first;
    }
    // A row is chosen
    else {
      item = m_lastDirectlySelectedRow;
    }

    //  Item chosen from the Point table view
    if (item->getPointerType() == AbstractTreeItem::Point) {
      cp = (ControlPoint *) (item->getPointer());
    }
    //  Item chosen from the Measure table view
    else {
      cp = (ControlPoint *) (item->parent()->getPointer());
      serialNumber = item->getData("Image ID").toString();
    }

//    qDebug()<<"activeCell cpid = "<<cp->GetId()<<"  sn = "<<serialNumber;

    emit editControlPoint(cp, serialNumber);
  }


  /**
    * Updates the item list
    */
  void TableViewContent::updateItemList() {

    if (m_model) {
      int startRow = verticalScrollBar()->value();
      int rowCount = (int) ceil(viewport()->height() / (double) m_rowHeight);
      m_items->clear();
      foreach (AbstractTreeItem * item,
          m_model->getItems(startRow, startRow + rowCount)) {
        m_items->append(item);
      }

      viewport()->update();
    }
  }


  /**
    * Populates the context menus based on where the user clicked
    *
    * @param mouseLocation Location of the mouse
    */
  void TableViewContent::showContextMenu(QPoint mouseLocation) {
    QMenu contextMenu(this);

    // Only add this action if a single row is selected or a cell is selected
    // TODO: 2017-05-17 TLS
    // Always allow editing of point.  Should we check for editLock??
    QList<AbstractTreeItem *> selectedRows = m_model->getSelectedItems();


    // If there is a row selection, show a context menu if the user clicked
    // anywhere on any of the selected row(s).

    if (QApplication::applicationName() != "cneteditor") {
      if (m_activeControlNet) {
        m_editControlPointAct->setEnabled(true);
        m_applyToSelectionAct->setEnabled(true);
        m_applyToAllAct->setEnabled(true);
      }
      else {
        m_editControlPointAct->setEnabled(false);
        m_applyToSelectionAct->setEnabled(false);
        m_applyToAllAct->setEnabled(false);
      }

      // We want to be able to delete rows in a nonactive control.
      m_deleteSelectedRowsAct->setEnabled(true);

      if (hasActiveCell() && selectedRows.count() <= 1) {
        contextMenu.addAction(m_editControlPointAct);
      }
      if (hasRowSelection() && mouseInRowSelection(mouseLocation)) {
        contextMenu.addAction(m_deleteSelectedRowsAct);
      }

      // Only show the context menu for cells if the user right-clicked on the
      // active cell.
      if (hasActiveCell() && mouseInCellSelection(mouseLocation)) {
        if (rowsWithActiveColumnSelected->size() > 1) {
          contextMenu.addAction(m_applyToSelectionAct);
        }

        contextMenu.addAction(m_applyToAllAct);
      }
    }
    else {
      if (hasRowSelection() && mouseInRowSelection(mouseLocation)) {
        contextMenu.addAction(m_deleteSelectedRowsAct);
      }

      // Only show the context menu for cells if the user right-clicked on the
      // active cell.
      if (hasActiveCell() && mouseInCellSelection(mouseLocation)) {
        if (rowsWithActiveColumnSelected->size() > 1) {
          contextMenu.addAction(m_applyToSelectionAct);
        }

        contextMenu.addAction(m_applyToAllAct);
      }
    }
    contextMenu.exec(mapToGlobal(mouseLocation));
  }
}
