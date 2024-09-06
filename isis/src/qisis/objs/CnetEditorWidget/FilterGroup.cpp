/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FilterGroup.h"

#include <QButtonGroup>
#include <QGroupBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QRadioButton>
#include <QString>
#include <QVBoxLayout>

#include "IException.h"
#include "IString.h"
#include "FileName.h"

#include "AbstractFilterSelector.h"
#include "ImageImageFilterSelector.h"
#include "PointMeasureFilterSelector.h"
#include "ImagePointFilterSelector.h"


using std::nothrow;
using std::swap;

namespace Isis {
  FilterGroup::FilterGroup(QString type) {
    nullify();

    m_filterType = new QString(type);

    init();
    addSelector();
  }


  FilterGroup::FilterGroup(const FilterGroup &other) {
    nullify();

    m_filterType = new QString(*other.m_filterType);

    init();

    foreach (AbstractFilterSelector * selector, *other.m_selectors) {
      AbstractFilterSelector *newSelector = NULL;
      if (*m_filterType == "Points and Measures") {
        newSelector = new PointMeasureFilterSelector(
          *((PointMeasureFilterSelector *) selector));
      }
      else {
        if (*m_filterType == "Images and Points") {
          newSelector = new ImagePointFilterSelector(
            *((ImagePointFilterSelector *) selector));
        }
        else {
          if (*m_filterType == "Connections") {
            newSelector = new ImageImageFilterSelector(
              *((ImageImageFilterSelector *) selector));
          }
        }
      }

      addSelector(newSelector);
    }

    m_buttonGroup->button(other.m_buttonGroup->checkedId())->click();
  }


  FilterGroup::~FilterGroup() {
    if (m_buttonGroup) {
      delete m_buttonGroup;
      m_buttonGroup = NULL;
    }

    if (m_selectors) {
      delete m_selectors;
      m_selectors = NULL;
    }

    if (m_filterType) {
      delete m_filterType;
      m_filterType = NULL;
    }
  }


  bool FilterGroup::hasFilter(bool (AbstractFilter::*meth)() const) const {
    bool found = false;

    for (int i = 0; !found && i < m_selectors->size(); i++)
      if (meth)
        found = m_selectors->at(i)->hasFilter(meth);
      else
        found = m_selectors->at(i)->hasFilter();

    return found;
  }


  void FilterGroup::nullify() {
    m_buttonGroup = NULL;
    m_closeButton = NULL;
    m_groupBoxLayout = NULL;
    m_selectors = NULL;
  }


  void FilterGroup::init() {
    m_selectors = new QList< AbstractFilterSelector * >;

    QLabel *descriptionLabel = new QLabel("Combine filters using logic "
        "type: ");
    QFont descriptionFont("SansSerif", 11);
    descriptionLabel->setFont(descriptionFont);
    QFont logicTypeFont("SansSerif", 12, QFont::Bold);
    QRadioButton *andButton = new QRadioButton("and");
    andButton->setFont(logicTypeFont);
    QRadioButton *orButton = new QRadioButton("or");
    orButton->setFont(logicTypeFont);
    m_buttonGroup = new QButtonGroup;
    m_buttonGroup->addButton(andButton, 0);
    m_buttonGroup->addButton(orButton, 1);
    connect(m_buttonGroup, SIGNAL(buttonClicked(int)), this,
        SLOT(changeFilterCombinationLogic(int)));

    // FIXME: this should be controlled by QSettings
    andButton->click();

    QHBoxLayout *logicLayout = new QHBoxLayout;
    QMargins margins = logicLayout->contentsMargins();
    margins.setBottom(4);
    logicLayout->setContentsMargins(margins);
    logicLayout->addStretch();
    logicLayout->addWidget(descriptionLabel);
    logicLayout->addWidget(andButton);
    logicLayout->addWidget(orButton);
    logicLayout->addStretch();
    m_logicWidget = new QWidget;
    m_logicWidget->setLayout(logicLayout);

    m_newSelectorButton = new QPushButton;
    m_newSelectorButton->setIcon(QIcon(
        QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/add.png").expanded())));
    QString newSelectorTooltip = "Add new filter";
    m_newSelectorButton->setToolTip(newSelectorTooltip);
    m_newSelectorButton->setStatusTip(newSelectorTooltip);

    connect(m_newSelectorButton, SIGNAL(clicked()), this, SLOT(addSelector()));
    QHBoxLayout *newSelectorLayout = new QHBoxLayout;
    newSelectorLayout->addWidget(m_newSelectorButton);
    newSelectorLayout->addStretch();

    m_groupBoxLayout = new QVBoxLayout;
    m_groupBoxLayout->addWidget(m_logicWidget);
    m_groupBoxLayout->addLayout(newSelectorLayout);
    QGroupBox *groupBox = new QGroupBox;
    groupBox->setLayout(m_groupBoxLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    margins = mainLayout->contentsMargins();
    margins.setTop(2);
    margins.setBottom(2);
    mainLayout->setContentsMargins(margins);
    mainLayout->addWidget(groupBox);

    setLayout(mainLayout);
  }


  void FilterGroup::addSelector() {
    AbstractFilterSelector *filterSelector = NULL;
    if (*m_filterType == "Points and Measures")
      filterSelector = new PointMeasureFilterSelector;
    else if (*m_filterType == "Images and Points")
      filterSelector = new ImagePointFilterSelector;
    else if (*m_filterType == "Connections")
      filterSelector = new ImageImageFilterSelector;

    addSelector(filterSelector);
  }


  void FilterGroup::addSelector(AbstractFilterSelector *newSelector) {

    if (newSelector) {
      connect(newSelector, SIGNAL(close(AbstractFilterSelector *)),
          this, SLOT(deleteSelector(AbstractFilterSelector *)));
      connect(newSelector, SIGNAL(filterChanged()),
          this, SIGNAL(filterChanged()));
      connect(newSelector, SIGNAL(sizeChanged()),
          this, SLOT(sendSizeChanged()));
      m_groupBoxLayout->insertWidget(m_groupBoxLayout->count() - 1, newSelector);
      m_selectors->append(newSelector);
      m_selectors->size() > 1 ? m_logicWidget->show() : m_logicWidget->hide();
    }

    sendSizeChanged();
  }


  void FilterGroup::deleteSelector(AbstractFilterSelector *filterSelector) {
    m_groupBoxLayout->removeWidget(filterSelector);
    delete filterSelector;
    m_selectors->removeOne(filterSelector);

    int m_selectorsSize = m_selectors->size();
    m_selectorsSize > 1 ? m_logicWidget->show() : m_logicWidget->hide();
    if (!m_selectorsSize)
      emit close(this);
    else
      emit filterChanged();
  }


  void FilterGroup::sendClose() {
    emit close(this);
  }


  void FilterGroup::sendSizeChanged() {
    emit sizeChanged(this);
  }


  void FilterGroup::changeFilterCombinationLogic(int button) {
    m_andFiltersTogether = button == 0;
    emit filterChanged();
  }


  bool FilterGroup::filtersAreAndedTogether() const {
    return m_andFiltersTogether;
  }


  FilterGroup &FilterGroup::operator=(FilterGroup other) {

    // create temporary list of new selectors
    QList< AbstractFilterSelector * > newSelectors;
    foreach (AbstractFilterSelector * selector, *other.m_selectors) {
      AbstractFilterSelector *newSelector = NULL;
      if (*m_filterType == "Points and Measures") {
        newSelector = new(nothrow) PointMeasureFilterSelector(
          *((PointMeasureFilterSelector *) selector));
      }
      else {
        if (*m_filterType == "Images and Points") {
          newSelector = new(nothrow) ImagePointFilterSelector(
            *((ImagePointFilterSelector *) selector));
        }
        else {
          if (*m_filterType == "Connections") {
            newSelector = new(nothrow) ImageImageFilterSelector(
              *((ImageImageFilterSelector *) selector));
          }
        }
      }

      if (newSelector) {
        *newSelector = *selector;
        newSelectors.append(newSelector);
      }
    }

    // if all is ok, and it is safe to assign
    if (newSelectors.size() == other.m_selectors->size()) {
      foreach (AbstractFilterSelector * selector, *m_selectors) {
        deleteSelector(selector);
      }

      foreach (AbstractFilterSelector * newSelector, newSelectors) {
        addSelector(newSelector);
      }

      swap(*m_filterType, *other.m_filterType);
      m_buttonGroup->button(other.m_buttonGroup->checkedId())->click();
    }
    else {
      // clean up any temp groups
      foreach (AbstractFilterSelector * newSelector, newSelectors) {
        if (newSelector) {
          delete newSelector;
          newSelector = NULL;
        }
      }

      IString msg = "Assignment of FilterGroup failed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *this;
  }


  QString FilterGroup::getDescription(
    bool (AbstractFilter::*hasFilterMeth)() const,
    QString(AbstractFilter::*descriptionMeth)() const) const {
    QString description;

    QList< AbstractFilterSelector * > selectorsWithFilters;
    for (int i = 0; i < m_selectors->size(); i++)
      if (m_selectors->at(i)->hasFilter(hasFilterMeth))
        selectorsWithFilters.append(m_selectors->at(i));

    int numFilters = selectorsWithFilters.size();

    if (numFilters) {
      QString logic = "<b> ";
      if (m_andFiltersTogether)
        logic += "and";
      else
        logic += "or";
      logic += " </b>";

      for (int i = 0; i < numFilters - 1; i++) {
        description += selectorsWithFilters[i]->getDescription(descriptionMeth)
            + logic;
      }

      description += selectorsWithFilters[numFilters - 1]->getDescription(
          descriptionMeth);
    }

    return description;
  }
}
