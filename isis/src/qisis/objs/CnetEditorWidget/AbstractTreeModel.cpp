/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AbstractTreeModel.h"

#include <algorithm>
#include <iostream>

#include <QFutureWatcher>
#include <QList>
#include <QModelIndex>
#include <QMutex>
#include <QPair>
#include <QStack>
#include <QString>
#include <QtConcurrentFilter>

#include <QtConcurrentMap>

#include <QFlags>
#include <QtGlobal>
#include <QVariant>

#include "BusyLeafItem.h"
#include "TreeView.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "IException.h"

#include "AbstractTreeItem.h"
#include "FilterWidget.h"
#include "RootItem.h"


namespace Isis {
  AbstractTreeModel::AbstractTreeModel(ControlNet *controlNet, TreeView *v,
      QObject *parent) : QObject(parent), m_view(v), m_cNet(controlNet) {

    m_filterWatcher = NULL;
    m_rebuildWatcher = NULL;
    m_busyItem = NULL;
    rootItem = NULL;
    m_expandedState = NULL;
    m_selectedState = NULL;
    m_guisFilterWidget = NULL;
    m_localFilterWidgetCopy = NULL;
    m_mutex = NULL;

    m_busyItem = new BusyLeafItem(NULL);
    rootItem = new RootItem;
    m_expandedState = new QList< QPair< QString, QString > >;
    m_selectedState = new QList< QPair< QString, QString > >;
    m_mutex = new QMutex;

    m_filterWatcher = new QFutureWatcher< QAtomicPointer< AbstractTreeItem > >;
    m_rebuildWatcher = new QFutureWatcher< QAtomicPointer< RootItem > >;

    connect(m_filterWatcher, SIGNAL(finished()), this, SLOT(applyFilterDone()));
    connect(m_rebuildWatcher, SIGNAL(finished()), this, SLOT(rebuildItemsDone()));

    connect(m_filterWatcher, SIGNAL(progressValueChanged(int)),
        this, SIGNAL(filterProgressChanged(int)));
    connect(m_filterWatcher, SIGNAL(progressRangeChanged(int, int)),
        this, SIGNAL(filterProgressRangeChanged(int, int)));
    connect(m_rebuildWatcher, SIGNAL(progressValueChanged(int)),
        this, SIGNAL(rebuildProgressChanged(int)));
    connect(m_rebuildWatcher, SIGNAL(progressRangeChanged(int, int)),
        this, SIGNAL(rebuildProgressRangeChanged(int, int)));

    m_drivable = false;
    m_filterAgain = false;
    m_filterRunning = false;
    m_rebuildRunning = false;
    m_frozen = false;
    m_rebuildPending = false;
  }


  AbstractTreeModel::~AbstractTreeModel() {
    delete m_filterWatcher;
    m_filterWatcher = NULL;

    delete m_rebuildWatcher;
    m_rebuildWatcher = NULL;

    delete m_busyItem;
    m_busyItem = NULL;

    delete rootItem;
    rootItem = NULL;

    delete m_expandedState;
    m_expandedState = NULL;

    delete m_selectedState;
    m_selectedState = NULL;

    delete m_mutex;
    m_mutex = NULL;

    delete m_localFilterWidgetCopy;
    m_localFilterWidgetCopy = NULL;

    m_guisFilterWidget = NULL;
    m_cNet = NULL;
    m_view = NULL;
  }


  // If a negative end is passed in, grabs all items from start to the end of
  // the tree. No busy leaf items will be inserted.
  QList< AbstractTreeItem * > AbstractTreeModel::getItems(int start, int end,
      InterestingItemsFlag flags, bool ignoreExpansion) {
    QList< AbstractTreeItem * > foundItems;
    int rowCount = end - start;
    const AbstractTreeItem *lastVisibleFilteredItem =
      rootItem->getLastVisibleFilteredItem();

    bool grabToEnd = (start >= 0 && end < 0);

    if (lastVisibleFilteredItem && (rowCount > 0 || grabToEnd) &&
        rootItem->childCount()) {
      int row = 0;
      AbstractTreeItem *currentItem = rootItem->getFirstVisibleChild();

      if (currentItem && !itemIsInteresting(currentItem, flags)) {
        currentItem = nextItem(currentItem, flags, ignoreExpansion);
      }

      bool listStillValid = true;

      while (row < start && listStillValid && currentItem) {
        row++;
        listStillValid = (currentItem != lastVisibleFilteredItem ||
            currentItem == currentItem->parent()->getLastVisibleChild());

        if (listStillValid)
          currentItem = nextItem(currentItem, flags, ignoreExpansion);
      }

      while ((row < end || grabToEnd) && listStillValid && currentItem) {
        foundItems.append(currentItem);
        listStillValid = (currentItem != lastVisibleFilteredItem ||
            currentItem == currentItem->parent()->getLastVisibleChild());
        row++;

        if (listStillValid)
          currentItem = nextItem(currentItem, flags, ignoreExpansion);
      }

      // Fill in the rest with busy items if needed. If we are grabbing all
      // items to the end of the visible tree, we do not want any busy items
      // added to our found items list.
      while (!grabToEnd && isFiltering() && foundItems.size() < rowCount) {
        foundItems.append(m_busyItem);
      }
    }

    return foundItems;
  }


  QList< AbstractTreeItem * > AbstractTreeModel::getItems(
    AbstractTreeItem *item1, AbstractTreeItem *item2,
    InterestingItemsFlag flags, bool ignoreExpansion) {
    QList< AbstractTreeItem * > foundItems;

    if (rootItem->childCount()) {
      AbstractTreeItem *start = NULL;

      AbstractTreeItem *curItem = rootItem->getFirstVisibleChild();

      while (!start && curItem) {
        if (curItem == item1)
          start = item1;
        else if (curItem == item2)
          start = item2;

        if (!start)
          curItem = nextItem(curItem, flags, ignoreExpansion);
      }

      if (!start) {
        std::string msg = "The first item passed to getItems(AbstractTreeItem*, "
            "AbstractTreeItem*) is not visible in this model's tree";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      AbstractTreeItem *end = item2;

      // Sometimes we need to build the list forwards and sometimes backwards.
      // This is accomplished by using either append or prepend.  We abstract
      // away which of these we should use (why should we care) by using the
      // variable "someKindaPend" to store the appropriate method.
      void (QList<AbstractTreeItem *>::*someKindaPend)(
        AbstractTreeItem * const &);

      someKindaPend = &QList<AbstractTreeItem *>::append;
      if (start == item2) {
        end = item1;
        someKindaPend = &QList<AbstractTreeItem *>::prepend;
      }

      while (curItem && curItem != end) {
        (foundItems.*someKindaPend)(curItem);
        curItem = nextItem(curItem, flags, ignoreExpansion);
      }

      if (!curItem) {
        std::string msg = "The second item passed to getItems(AbstractTreeItem*, "
            "AbstractTreeItem*) is not visible in this model's tree";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }

      (foundItems.*someKindaPend)(end);
    }

    return foundItems;
  }


  QList< AbstractTreeItem * > AbstractTreeModel::getSelectedItems(
    InterestingItemsFlag flags, bool ignoreExpansion) {
    QList< AbstractTreeItem * > selectedItems;

    if (!isFiltering()) {
      AbstractTreeItem *currentItem = rootItem->getFirstVisibleChild();

      if (currentItem && !itemIsInteresting(currentItem, flags))
        currentItem = nextItem(currentItem, flags, ignoreExpansion);

      while (currentItem) {
        if (currentItem->isSelected())
          selectedItems.append(currentItem);

        currentItem = nextItem(currentItem, flags, ignoreExpansion);
      }
    }

    return selectedItems;
  }


  QMutex *AbstractTreeModel::getMutex() const {
    return m_mutex;
  }


  int AbstractTreeModel::getItemCount(InterestingItemsFlag flags) const {
    return getItemCount(rootItem, flags);
  }


  int AbstractTreeModel::getTopLevelItemCount() const {
    return rootItem->childCount();
  }

  int AbstractTreeModel::getVisibleItemCount(InterestingItemsFlag flags,
      bool ignoreExpansion) const {
    AbstractTreeItem *currentItem = rootItem->getFirstVisibleChild();
    int count = -1;

    if (!isFiltering()) {
      count = 0;

      while (currentItem) {
        if (itemIsInteresting(currentItem, flags)) {
          count++;
        }

        currentItem = nextItem(currentItem, flags, ignoreExpansion);
      }
    }

    return count;
  }


  int AbstractTreeModel::getVisibleTopLevelItemCount() const {
    AbstractTreeItem *currentItem = rootItem->getFirstVisibleChild();
    int count = -1;

    if (!isFiltering()) {
      count = 0;

      while (currentItem) {
        count++;
        currentItem = currentItem->getNextVisiblePeer();
      }
    }

    return count;
  }


  int AbstractTreeModel::indexOfVisibleItem(AbstractTreeItem const *item,
      InterestingItemsFlag flags, bool ignoreExpansion) const {
    AbstractTreeItem *currentItem = rootItem->getFirstVisibleChild();
    int index = -1;

    if (!isFiltering()) {
      while (currentItem && currentItem != item) {
        if (itemIsInteresting(currentItem, flags))
          index++;

        currentItem = nextItem(currentItem, flags, ignoreExpansion);
      }

      index++;

      if (!currentItem)
        index = -1;
    }

    return index;
  }


  void AbstractTreeModel::setFrozen(bool newFrozenState) {
    m_frozen = newFrozenState;
    if (!m_frozen) {
      if (m_rebuildPending) {
        rebuildItems();
        m_rebuildPending = false;
      }
      else {
        applyFilter();
      }
    }
  }


  bool AbstractTreeModel::isFrozen() const {
    return m_frozen;
  }


  void AbstractTreeModel::queueRebuild() {
    m_rebuildPending = true;
  }


  bool AbstractTreeModel::isFiltering() const {
    return m_filterRunning;
  }


  bool AbstractTreeModel::isRebuilding() const {
    return m_rebuildRunning;
  }


  void AbstractTreeModel::setFilter(FilterWidget *fw) {
    m_guisFilterWidget = fw;
    if (fw) {
      connect(m_guisFilterWidget, SIGNAL(filterChanged()),
          this, SLOT(applyFilter()));
      applyFilter();
    }
  }


  void AbstractTreeModel::clear() {

    delete rootItem;
    rootItem = NULL;
    rootItem = new RootItem;
  }


  ControlNet *AbstractTreeModel::getControlNetwork() const {
    return m_cNet;
  }


  QFutureWatcher< QAtomicPointer< RootItem > > *
  AbstractTreeModel::getRebuildWatcher() const {
    return m_rebuildWatcher;
  }


  RootItem *AbstractTreeModel::getRootItem() const {
    return rootItem;
  }


  TreeView *AbstractTreeModel::getView() const {
    return m_view;
  }


  void AbstractTreeModel::stopWorking() {
    m_filterWatcher->cancel();
    m_filterWatcher->waitForFinished();
    m_rebuildWatcher->cancel();
    m_rebuildWatcher->waitForFinished();
  }


  //! indentation is in pixels
  QSize AbstractTreeModel::getVisibleSize(int indentation) const {
    QSize size;

    if (!isFiltering()) {
      int visibleRowCount = 0;
      int maxWidth = 0;

      if (rootItem && rootItem->getFirstVisibleChild()) {
        AbstractTreeItem *current = rootItem->getFirstVisibleChild();

        while (current != NULL) {
          int depth = current->getDepth();

          visibleRowCount++;
          maxWidth = qMax(maxWidth,
              current->getDataWidth() + indentation * depth);
          current = nextItem(current, AllItems, false);
        }
      }

      size = QSize(maxWidth, visibleRowCount);
    }

    return size;
  }


  void AbstractTreeModel::applyFilter() {
    // If m_filterAgain is true, then this method will be recalled later
    // with m_filterAgain = false.
    if (!m_frozen && !m_filterAgain && m_guisFilterWidget &&
        m_rebuildWatcher->isFinished()) {
      emit cancelSort();
      QFuture< QAtomicPointer< AbstractTreeItem> > futureRoot;

      if (m_filterRunning) {
        m_filterAgain = true;
        futureRoot = m_filterWatcher->future();
        futureRoot.cancel();
      }
      else {
        // filterCounts are unknown and invalid and this fact is shared to
        // users of this class by emitting invalid (negative) information.
        emit filterCountsChanged(-1, getTopLevelItemCount());

        // update our local copy of the gui widget
        if (m_localFilterWidgetCopy) {
          delete m_localFilterWidgetCopy;
          m_localFilterWidgetCopy = NULL;
        }

        m_localFilterWidgetCopy = new FilterWidget(*m_guisFilterWidget);

        // using the local copy (NOT the GUI's FilterWidget!!!) apply then
        // the filter using qtconcurrent's filteredReduced.  ApplyFilterDone()
        // will get called when the filtering is finished.
        m_filterRunning = true;
        rootItem->setLastVisibleFilteredItem(NULL);
        futureRoot = QtConcurrent::filteredReduced(rootItem->getChildren(),
            FilterFunctor(m_localFilterWidgetCopy),
            &FilterFunctor::updateTopLevelLinks,
            QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);

        m_filterWatcher->setFuture(futureRoot);
      }
    }
  }


  void AbstractTreeModel::setGlobalSelection(bool selected,
      InterestingItemsFlag flags) {
    selectItems(rootItem, selected, flags);
  }


  void AbstractTreeModel::selectItems(
    AbstractTreeItem *item, bool selected, InterestingItemsFlag flags) {
    if (item && itemIsInteresting(item, flags)) {
      item->setSelected(selected);
    }

    if (item->childCount()) {
      foreach (AbstractTreeItem * childItem, item->getChildren()) {
        selectItems(childItem, selected, flags);
      }
    }
  }


  bool AbstractTreeModel::itemIsInteresting(AbstractTreeItem *item,
      InterestingItemsFlag flags) {
    AbstractTreeItem::InternalPointerType pointerType =
      item->getPointerType();

    if ((pointerType == AbstractTreeItem::Point && flags.testFlag(PointItems)) ||
        (pointerType == AbstractTreeItem::Measure && flags.testFlag(MeasureItems)) ||
        (pointerType == AbstractTreeItem::ImageAndNet && flags.testFlag(ImageItems))) {
      return true;
    }
    else {
      return false;
    }
  }


  int AbstractTreeModel::getItemCount(AbstractTreeItem *item,
      InterestingItemsFlag flags) const {
    int count = 0;

    if (item && itemIsInteresting(item, flags)) {
      count++;
    }

    if (item->childCount()) {
      foreach (AbstractTreeItem * childItem, item->getChildren()) {
        count += getItemCount(childItem, flags);
      }
    }

    return count;
  }


  AbstractTreeItem *AbstractTreeModel::nextItem(AbstractTreeItem *current,
      InterestingItemsFlag flags, bool ignoreExpansion) const {
    if (current) {
      do {
        if ((ignoreExpansion || current->isExpanded()) &&
            current->getFirstVisibleChild())
          current = current->getFirstVisibleChild();
        else if (current->getNextVisiblePeer())
          current = current->getNextVisiblePeer();
        else if (current->parent())
          current = current->parent()->getNextVisiblePeer();
        else
          current = NULL;
      }
      while (current && !itemIsInteresting(current, flags));
    }

    return current;
  }


  void AbstractTreeModel::applyFilterDone() {
    m_filterRunning = false;

    if (m_filterAgain) {
      m_filterAgain = false;
      applyFilter();
    }
    else {
      emit modelModified();
      emit filterCountsChanged(getVisibleTopLevelItemCount(),
          getTopLevelItemCount());
    }
  }


  void AbstractTreeModel::rebuildItemsDone() {
    clear();

    QAtomicPointer< RootItem > newRootPtr = m_rebuildWatcher->future();
    RootItem *newRoot = newRootPtr.loadAcquire();

    if (newRoot && newRoot->childCount()) {
      delete rootItem;
      rootItem = NULL;
      rootItem = newRoot;
    }
    // Not safe - if newRoot == NULL, the condition fails - delete (NULL) is undefined
    /*else {
      delete newRoot;
      newRoot = NULL;
    }*/

    applyFilter();

    setRebuilding(false);
    emit modelModified();
  }


  AbstractTreeModel::FilterFunctor::FilterFunctor(
    FilterWidget *fw) : m_filter(fw) {
  }


  AbstractTreeModel::FilterFunctor::FilterFunctor(FilterFunctor const &other) {
    m_filter = other.m_filter;
  }


  AbstractTreeModel::FilterFunctor::~FilterFunctor() {
  }


  bool AbstractTreeModel::FilterFunctor::operator()(
    AbstractTreeItem *const &item) const {
    filterWorker(item);
    return true;
  }


  AbstractTreeModel::FilterFunctor &
  AbstractTreeModel::FilterFunctor::operator=(FilterFunctor const &other) {
    if (this != &other)
      m_filter = other.m_filter;

    return *this;
  }


  void AbstractTreeModel::FilterFunctor::filterWorker(
    AbstractTreeItem *item) const {
    switch (item->getPointerType()) {

      case AbstractTreeItem::Point:
        item->setVisible((!m_filter || m_filter->evaluate(
            (ControlPoint *) item->getPointer())) ? true : false);
        break;

      case AbstractTreeItem::Measure:
        item->setVisible((!m_filter || m_filter->evaluate(
            (ControlMeasure *) item->getPointer())) ? true : false);
        break;

      case AbstractTreeItem::ImageAndNet:
        item->setVisible((!m_filter || m_filter->evaluate(
            (QPair<QString, ControlNet *> *) item->getPointer())) ? true : false);
        break;

      case AbstractTreeItem::None:
        item->setVisible(true);
        break;
    }

    // Destroy peer link because it will need to be recreated later.
    if (item->getFirstVisibleChild())
      item->setFirstVisibleChild(NULL);

    if (item->getLastVisibleChild())
      item->setLastVisibleChild(NULL);

    item->setNextVisiblePeer(NULL);

    // Update each tree item's visible flag based on whether or not it is
    // accepted by the filter.
    if (item->childCount()) {
      for (int i = 0; i < item->childCount(); i++) {
        AbstractTreeItem *child = item->childAt(i);
        filterWorker(child);

        if (child->isVisible()) {
          if (!item->getFirstVisibleChild()) {
            item->setFirstVisibleChild(child);
            item->setLastVisibleChild(child);
          }
          else {
            item->getLastVisibleChild()->setNextVisiblePeer(child);
            item->setLastVisibleChild(child);
          }
        }
      }
    }
  }


  void AbstractTreeModel::FilterFunctor::updateTopLevelLinks(
    QAtomicPointer< AbstractTreeItem > & root,
    AbstractTreeItem *const &item) {
    // We will update the root if it is NULL
    if ( root.testAndSetOrdered(NULL, item->parent()) ) {
      AbstractTreeItem *loadedRoot = root.loadAcquire();
      loadedRoot->setFirstVisibleChild(NULL);
      loadedRoot->setLastVisibleChild(NULL);
      loadedRoot->setLastVisibleFilteredItem(NULL);
    }

    // Let's get that root ptr again
    AbstractTreeItem *loadedRoot = root.loadAcquire();
    if (item->isVisible()) {
      if (!loadedRoot->getFirstVisibleChild()) {
        loadedRoot->setFirstVisibleChild(item);
        loadedRoot->setLastVisibleChild(item);
      }
      else {
        loadedRoot->getLastVisibleChild()->setNextVisiblePeer(item);
        loadedRoot->setLastVisibleChild(item);
      }

      loadedRoot->setLastVisibleFilteredItem(item);
    }
  }
}
