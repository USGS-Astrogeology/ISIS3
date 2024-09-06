/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AbstractFilterSelector.h"

#include <algorithm>
#include <iostream>

#include <QComboBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QWriteLocker>

#include "AbstractFilter.h"
#include "FileName.h"
#include "PointIdFilter.h"


using std::nothrow;
using std::swap;


namespace Isis {
  AbstractFilterSelector::AbstractFilterSelector() {
    nullify();
  }


  AbstractFilterSelector::~AbstractFilterSelector() {
    deleteFilter();
  }


  bool AbstractFilterSelector::hasFilter() const {
    return m_filter != NULL;
  }


  bool AbstractFilterSelector::hasFilter(bool (AbstractFilter::*meth)() const) const {
    return m_filter && (m_filter->*meth)();
  }


  QString AbstractFilterSelector::getDescription(QString(AbstractFilter::*meth)() const) const {
    QString description;
    if (m_filter) {
      description = (m_filter->*meth)();
    }

    return description;
  }


  AbstractFilterSelector &AbstractFilterSelector::operator=(const AbstractFilterSelector &other) {
    getSelector()->setCurrentIndex(other.getSelector()->currentIndex());
    if (m_filter && other.m_filter) {
      setFilter(other.m_filter->clone());
    }

    return *this;
  }


  void AbstractFilterSelector::nullify() {
    m_closeButton = NULL;
    m_filter = NULL;
    m_mainLayout = NULL;
    m_selector = NULL;
  }


  void AbstractFilterSelector::createSelector() {
    m_closeButton = new QPushButton;
    m_closeButton->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/editdelete.png").expanded())));
    connect(m_closeButton, SIGNAL(clicked()), this, SLOT(sendClose()));

    m_selector = new QComboBox;
    m_selector->addItem("---- select ----");
    m_selector->insertSeparator(m_selector->count());
    connect(m_selector, SIGNAL(currentIndexChanged(int)),
            this, SLOT(changeFilter(int)));

    m_mainLayout = new QHBoxLayout;
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_closeButton);
    m_mainLayout->addWidget(m_selector);
    m_mainLayout->addStretch();
    m_mainLayout->setAlignment(m_closeButton, Qt::AlignTop);
    m_mainLayout->setAlignment(m_selector, Qt::AlignTop);


    setLayout(m_mainLayout);
  }


  QComboBox *AbstractFilterSelector::getSelector() const {
    return m_selector;
  }


  QHBoxLayout *AbstractFilterSelector::getMainLayout() const {
    return m_mainLayout;
  }


  AbstractFilter *AbstractFilterSelector::getFilter() const {
    return m_filter;
  }


  void AbstractFilterSelector::setFilter(AbstractFilter *someFilter) {
    if (m_filter) {
      delete m_filter;
      m_filter = NULL;
    }

    m_filter = someFilter;
    connect(getFilter(), SIGNAL(filterChanged()),
            this, SIGNAL(filterChanged()));
    getMainLayout()->insertWidget(2, getFilter());
  }


  void AbstractFilterSelector::deleteFilter() {
    if (m_filter) {
      delete m_filter;
      m_filter = NULL;
    }
  }


  void AbstractFilterSelector::sendClose() {
    emit close(this);
  }
}
