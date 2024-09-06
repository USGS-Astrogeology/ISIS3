/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "FilterWidget.h"

#include <iostream>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPair>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <QVBoxLayout>

#include "IException.h"
#include "IString.h"
#include "FileName.h"

#include "FilterGroup.h"


using std::swap;
using std::nothrow;


namespace Isis {
  FilterWidget::FilterWidget(QString type) {
    nullify();

    m_filterType = new QString(type);

    init();
    addGroup();
    updateDescription();
  }


  FilterWidget::FilterWidget(const FilterWidget &other) {
    nullify();

    m_filterType = new QString(*other.m_filterType);

    init();

    foreach (FilterGroup * group, *other.m_filterGroups) {
      addGroup(new FilterGroup(*group));
    }

    m_buttonGroup->button(other.m_buttonGroup->checkedId())->click();

    updateDescription();
  }


  FilterWidget::~FilterWidget() {
    if (m_buttonGroup) {
      delete m_buttonGroup;
      m_buttonGroup = NULL;
    }

    if (m_filterGroups) {
      delete m_filterGroups;
      m_filterGroups = NULL;
    }

    if (m_filterType) {
      delete m_filterType;
      m_filterType = NULL;
    }
  }


  bool FilterWidget::evaluate(const QPair<QString, ControlNet *> *imageAndNet) const {
    return evaluate(imageAndNet, &AbstractFilter::canFilterImages);
  }


  bool FilterWidget::evaluate(const ControlPoint *point) const {
    return evaluate(point, &AbstractFilter::canFilterPoints);
  }


  bool FilterWidget::evaluate(const ControlMeasure *measure) const {
    return evaluate(measure, &AbstractFilter::canFilterMeasures);
  }


  bool FilterWidget::hasFilter(bool (AbstractFilter::*meth)() const) const {
    bool found = false;

    for (int i = 0; !found && i < m_filterGroups->size(); i++)
      found = m_filterGroups->at(i)->hasFilter(meth);

    return found;
  }


  FilterWidget &FilterWidget::operator=(FilterWidget other) {

    // create temporary list of new groups
    QList< FilterGroup * > newGroups;
    foreach (FilterGroup * group, *other.m_filterGroups) {
      FilterGroup *newGroup = new(nothrow) FilterGroup(*other.m_filterType);
      if (newGroup) {
        *newGroup = *group;
        newGroups.append(newGroup);
      }
    }

    // if all is ok, and it is safe to assign
    if (newGroups.size() == other.m_filterGroups->size()) {
      foreach (FilterGroup * group, *m_filterGroups) {
        deleteGroup(group);
      }

      foreach (FilterGroup * newGroup, newGroups) {
        addGroup(newGroup);
      }

      swap(*m_filterType, *other.m_filterType);
      m_buttonGroup->button(other.m_buttonGroup->checkedId())->click();
    }
    else {
      // clean up any temp groups
      foreach (FilterGroup * newGroup, newGroups) {
        if (newGroup) {
          delete newGroup;
          newGroup = NULL;
        }
      }

      IString msg = "Assignment of FilterWidget failed";
      throw IException(IException::Programmer, msg, _FILEINFO_);
    }

    return *this;
  }


  void FilterWidget::nullify() {
    m_addGroupButton = NULL;
    m_buttonGroup = NULL;
    m_imageDescription = NULL;
    m_pointDescription = NULL;
    m_measureDescription = NULL;
    m_mainLayout = NULL;
    m_filterGroups = NULL;
    m_filterType = NULL;
  }


  void FilterWidget::init() {
    m_filterGroups = new QList< FilterGroup * >;

    QString whatsThis = "<html>Filters are organized into groups (bounded by a box)."
        "  Filters within a group will be combined using either AND or OR "
        "logic.  Furthermore, multiple groups are supported, and the logic "
        "used to combine the various groups is also configurable.<br/><br/>"
        "For example, let A, B, and C be filters.  By creating two "
        "groups, one with A and B and the other with C, it is possible to "
        "build the expression \"(A and B) or C\".<br/><br/>"
        "Each group has a green plus (+) button, which adds a new filter to "
        "the group.  There is also a green plus (+) button outside any group "
        "for adding a new group.</html>";


    QString title = "Filter " + *m_filterType;
    QLabel *titleLabel = new QLabel(title);
    titleLabel->setFont(QFont("SansSerif", 15, QFont::DemiBold));

    QHBoxLayout *titleLayout = new QHBoxLayout;
    titleLayout->addStretch();
    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();

    QLabel *logicTypeLabel = new QLabel("Combine groups using logic type: ");
    QFont logicTypeLabelFont("SansSerif", 12);
    logicTypeLabel->setFont(logicTypeLabelFont);

    QFont logicTypeFont("SansSerif", 12, QFont::Bold);
    QRadioButton *andButton = new QRadioButton("AND");
    andButton->setFont(logicTypeFont);
    QRadioButton *orButton = new QRadioButton("OR");
    orButton->setFont(logicTypeFont);
    m_buttonGroup = new QButtonGroup;
    m_buttonGroup->addButton(andButton, 0);
    m_buttonGroup->addButton(orButton, 1);
    connect(m_buttonGroup, SIGNAL(buttonClicked(int)), this,
        SLOT(changeGroupCombinationLogic(int)));

    // FIXME: this should be controlled by QSettings
    orButton->click();

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(logicTypeLabel);
    buttonLayout->addWidget(andButton);
    buttonLayout->addWidget(orButton);
    buttonLayout->addStretch();
    m_logicWidget = new QWidget;
    m_logicWidget->setLayout(buttonLayout);

    m_addGroupButton = new QPushButton;
    m_addGroupButton->setIcon(QIcon(QString::fromStdString(FileName("$ISISROOT/appdata/images/icons/add.png").expanded())));
    QString addGroupTooltip = "Add new filter group";
    m_addGroupButton->setToolTip(addGroupTooltip);
    m_addGroupButton->setStatusTip(addGroupTooltip);
    m_addGroupButton->setWhatsThis(whatsThis);
    connect(m_addGroupButton, SIGNAL(clicked()), this, SLOT(addGroup()));
    QHBoxLayout *addGroupLayout = new QHBoxLayout;
    addGroupLayout->addWidget(m_addGroupButton);
    addGroupLayout->addStretch();

    QLabel *titleDummy = new QLabel;
    titleDummy->setFont(QFont("SansSerif", 6)); // FIXME

    m_imageDescription = new QLabel;
    m_imageDescription->setWordWrap(true);
    m_imageDescription->setFont(QFont("SansSerif", 10)); // FIXME

    m_pointDescription = new QLabel;
    m_pointDescription->setWordWrap(true);
    m_pointDescription->setFont(QFont("SansSerif", 10)); // FIXME

    m_measureDescription = new QLabel;
    m_measureDescription->setWordWrap(true);
    m_measureDescription->setFont(QFont("SansSerif", 10)); // FIXME

    QVBoxLayout *descriptionLayout = new QVBoxLayout;
    descriptionLayout->addWidget(titleDummy);
    descriptionLayout->addWidget(m_imageDescription);
    descriptionLayout->addWidget(m_pointDescription);
    descriptionLayout->addWidget(m_measureDescription);

    connect(this, SIGNAL(filterChanged()),
        this, SLOT(updateDescription()));

    m_mainLayout = new QVBoxLayout;
    m_mainLayout->addLayout(titleLayout);
    m_mainLayout->addLayout(descriptionLayout);
    m_mainLayout->addWidget(m_logicWidget);
    m_mainLayout->addLayout(addGroupLayout);
    m_mainLayout->addStretch();

    setLayout(m_mainLayout);
    setWhatsThis(whatsThis);
  }


  void FilterWidget::updateDescription() {
    updateDescription(m_imageDescription, &AbstractFilter::canFilterImages,
        &AbstractFilter::getImageDescription, "images");
    updateDescription(m_pointDescription, &AbstractFilter::canFilterPoints,
        &AbstractFilter::getPointDescription, "points");
    updateDescription(m_measureDescription, &AbstractFilter::canFilterMeasures,
        &AbstractFilter::getMeasureDescription, "measures");
  }


  void FilterWidget::updateDescription(QLabel *label,
      bool (AbstractFilter::*hasFilterMeth)() const,
      QString(AbstractFilter::*descriptionMeth)() const,
      QString title) {

    if (label) {
      label->clear();

      QList< FilterGroup * > groups;
      for (int i = 0; i < m_filterGroups->size(); i++)
        if (m_filterGroups->at(i)->hasFilter(hasFilterMeth))
          groups.append(m_filterGroups->at(i));

      const int GROUP_SIZE = groups.size();

      if (GROUP_SIZE) {
        QString black = "<font color=black>";
        QString blue = "<font color=darkBlue>";
        QString red = "<font color=darkRed>";
        QString end = "</font>";

        QString text = "Showing " + red + title + end + black + " which " + end;

        QString groupLogic;
        if (m_andGroupsTogether)
          groupLogic += " AND ";
        else
          groupLogic += " OR ";

        QString leftParen = black + "<b>(</b>" + end;
        QString rightParen = black + "<b>)</b>" + end;

        for (int i = 0; i < GROUP_SIZE - 1; i++) {
          if (GROUP_SIZE > 1)
            text += leftParen;
          text += blue + groups[i]->getDescription(
              hasFilterMeth, descriptionMeth) + end;
          if (GROUP_SIZE > 1)
            text += rightParen + black + "<b>" + groupLogic + "</b>" + end;
        }

        if (GROUP_SIZE > 1)
          text += leftParen;
        text += blue + groups[GROUP_SIZE - 1]->getDescription(
            hasFilterMeth, descriptionMeth) + end;
        if (GROUP_SIZE > 1)
          text += rightParen;

        text += black + "." + end;

        label->setText(text);
      }
    }
  }


  void FilterWidget::maybeScroll(FilterGroup *group) {

    if (m_filterGroups && m_filterGroups->size() &&
        m_filterGroups->at(m_filterGroups->size() - 1) == group)
      emit scrollToBottom();
  }


  void FilterWidget::addGroup() {
    FilterGroup *newGroup = new FilterGroup(*m_filterType);
    addGroup(newGroup);
  }


  void FilterWidget::addGroup(FilterGroup *newGroup) {
    connect(newGroup, SIGNAL(close(FilterGroup *)),
        this, SLOT(deleteGroup(FilterGroup *)));
    connect(newGroup, SIGNAL(filterChanged()), this, SIGNAL(filterChanged()));
    connect(newGroup, SIGNAL(sizeChanged(FilterGroup *)),
        this, SLOT(maybeScroll(FilterGroup *)));
    m_mainLayout->insertWidget(m_mainLayout->count() - 2, newGroup);
    m_filterGroups->append(newGroup);
    m_filterGroups->size() > 1 ? m_logicWidget->show() : m_logicWidget->hide();

    emit scrollToBottom();
    emit filterChanged();
  }


  void FilterWidget::deleteGroup(FilterGroup *filterGroup) {
    m_mainLayout->removeWidget(filterGroup);
    delete filterGroup;
    m_filterGroups->removeOne(filterGroup);
    m_filterGroups->size() > 1 ? m_logicWidget->show() : m_logicWidget->hide();
    emit filterChanged();
  }


  void FilterWidget::changeGroupCombinationLogic(int button) {
    m_andGroupsTogether = button == 0;
    emit filterChanged();
  }
}
