/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "PointTableDelegate.h"

#include <iostream>

#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QString>
#include <QWidget>

#include "ControlMeasure.h"
#include "ControlPoint.h"
#include "IException.h"

#include "AbstractPointItem.h"
#include "AbstractTreeItem.h"
#include "PointTableModel.h"
#include "TableColumn.h"

namespace Isis {
  PointTableDelegate::PointTableDelegate() {
  }


  PointTableDelegate::~PointTableDelegate() {
  }


  QWidget *PointTableDelegate::getWidget(TableColumn const *col) const {
    AbstractPointItem::Column column =
      AbstractPointItem::getColumn(col->getTitle());

    switch (column) {
        // handle combo box cases
      case AbstractPointItem::PointType:
      case AbstractPointItem::EditLock:
      case AbstractPointItem::Ignored:
      case AbstractPointItem::Reference:
      case AbstractPointItem::APrioriSPSource:
      case AbstractPointItem::APrioriRadiusSource: {
          QComboBox *combo = new QComboBox;

          switch (column) {
            case AbstractPointItem::PointType:
              for (int i = 0; i < ControlPoint::PointTypeCount; i++) {
                combo->insertItem(i, ControlPoint::PointTypeToString(
                    (ControlPoint::PointType) i));
              }
              break;
            case AbstractPointItem::EditLock:
              combo->insertItem(0, "Yes");
              combo->insertItem(1, "No");
              break;
            case AbstractPointItem::Ignored:
              combo->insertItem(0, "Yes");
              combo->insertItem(1, "No");
              break;
            case AbstractPointItem::Reference:
              // Reference depends on possibilities, not static, so will be
              // populated later in readData(...).
              break;
            case AbstractPointItem::APrioriSPSource:
              combo->insertItem(0, "None");
              combo->insertItem(1, "User");
              combo->insertItem(2, "AverageOfMeasures");
              combo->insertItem(3, "Reference");
              combo->insertItem(4, "Basemap");
              combo->insertItem(5, "BundleSolution");
              break;
            case AbstractPointItem::APrioriRadiusSource:
              combo->insertItem(0, "None");
              combo->insertItem(1, "User");
              combo->insertItem(2, "AverageOfMeasures");
              combo->insertItem(3, "Ellipsoid");
              combo->insertItem(4, "DEM");
              combo->insertItem(5, "BundleSolution");
              break;
            default:
              break;
          }

          return combo;
        }

        // handle line edit cases
      default:
        return new QLineEdit();
    }

    std::string msg = "Could not create delegate widget for column ["
        + col->getTitle() + "]";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  void PointTableDelegate::readData(QWidget *widget,
      AbstractTreeItem *row, TableColumn const *col) const {
    QString columnTitle = col->getTitle();
    AbstractPointItem::Column column =
      AbstractPointItem::getColumn(columnTitle);

    QString data = row->getFormattedData(columnTitle);

    ControlPoint *point = (ControlPoint *)row->getPointer();

    switch (column) {
      case AbstractPointItem::Reference: {
          QComboBox *combo = static_cast< QComboBox * >(widget);

          for (int i = 0; i < point->GetNumMeasures(); i++)
            combo->insertItem(i, point->GetMeasure(i)->GetCubeSerialNumber());
          combo->setCurrentIndex(point->IndexOfRefMeasure());
        }
        break;

      case AbstractPointItem::PointType:
      case AbstractPointItem::EditLock:
      case AbstractPointItem::Ignored:
      case AbstractPointItem::APrioriSPSource:
      case AbstractPointItem::APrioriRadiusSource: {
          QComboBox *combo = static_cast< QComboBox * >(widget);

          switch (column) {
            case AbstractPointItem::PointType:
              combo->setCurrentIndex(
                (int) point->StringToPointType(data));
              break;
            case AbstractPointItem::EditLock:
              combo->setCurrentIndex(point->IsEditLocked() ? 0 : 1);
              break;
            case AbstractPointItem::Ignored:
              combo->setCurrentIndex(point->IsIgnored() ? 0 : 1);
              break;
            case AbstractPointItem::APrioriSPSource:
              combo->setCurrentIndex(
                (int) point->StringToSurfacePointSource(data));
              break;
            case AbstractPointItem::APrioriRadiusSource:
              combo->setCurrentIndex(
                (int) point->StringToRadiusSource(data));
              break;
            default:
              break;
          }
        }
        break;
      default: {
          QLineEdit *lineEdit = static_cast< QLineEdit * >(widget);
          lineEdit->setText(data);
        }
    }
  }


  void PointTableDelegate::readData(QWidget *widget,
      AbstractTreeItem *row, TableColumn const *col,
      QString newData) const {
    QString columnTitle = col->getTitle();
    AbstractPointItem::Column column =
      AbstractPointItem::getColumn(columnTitle);

    QString data = row->getFormattedData(columnTitle);

    ControlPoint *point = (ControlPoint *)row->getPointer();

    switch (column) {
      case AbstractPointItem::Reference: {
          QComboBox *combo = static_cast< QComboBox * >(widget);

          for (int i = 0; i < point->GetNumMeasures(); i++)
            combo->insertItem(i, point->GetMeasure(i)->GetCubeSerialNumber());

          combo->setCurrentIndex(point->IndexOfRefMeasure());

          for (int i = combo->count() - 1; i >= 0; --i)
            if (combo->itemText(i).toLower().startsWith(newData.toLower()))
              combo->setCurrentIndex(i);
        }
        break;

      case AbstractPointItem::PointType:
      case AbstractPointItem::EditLock:
      case AbstractPointItem::Ignored:
      case AbstractPointItem::APrioriSPSource:
      case AbstractPointItem::APrioriRadiusSource: {
          QComboBox *combo = static_cast< QComboBox * >(widget);

          switch (column) {
            case AbstractPointItem::PointType:
              combo->setCurrentIndex(
                (int) point->StringToPointType(data));
              break;
            case AbstractPointItem::EditLock:
              combo->setCurrentIndex(point->IsEditLocked() ? 0 : 1);
              break;
            case AbstractPointItem::Ignored:
              combo->setCurrentIndex(point->IsIgnored() ? 0 : 1);
              break;
            case AbstractPointItem::APrioriSPSource:
              combo->setCurrentIndex(
                (int) point->StringToSurfacePointSource(data));
              break;
            case AbstractPointItem::APrioriRadiusSource:
              combo->setCurrentIndex(
                (int) point->StringToRadiusSource(data));
              break;
            default:
              break;
          }

          for (int i = combo->count() - 1; i >= 0; --i)
            if (combo->itemText(i).toLower().startsWith(newData.toLower()))
              combo->setCurrentIndex(i);
        }
        break;
      default: {
          QLineEdit *lineEdit = static_cast< QLineEdit * >(widget);
          lineEdit->setText(newData);
        }
    }
  }


  void PointTableDelegate::saveData(QWidget *widget,
      AbstractTreeItem *row, const TableColumn *col) const {
    AbstractPointItem::Column column =
      AbstractPointItem::getColumn(col->getTitle());

    QString newData;

    switch (column) {
      case AbstractPointItem::PointType:
      case AbstractPointItem::EditLock:
      case AbstractPointItem::Ignored:
      case AbstractPointItem::Reference:
      case AbstractPointItem::APrioriSPSource:
      case AbstractPointItem::APrioriRadiusSource: {
          QComboBox *combo = static_cast< QComboBox * >(widget);
          newData = combo->currentText();
          break;
        }
      default: {
          QLineEdit *lineEdit = static_cast< QLineEdit * >(widget);
          newData = lineEdit->text();
        }
    }

    QString warningText = PointTableModel::getPointWarningMessage(
        row, col, newData);

    bool changeData = true;

    if (!warningText.isEmpty()) {
      QMessageBox::StandardButton status = QMessageBox::warning(
          NULL, "Change cell?", warningText, QMessageBox::Yes |
          QMessageBox::No);

      changeData = (status == QMessageBox::Yes);
    }

    if (changeData)
      row->setData(col->getTitle(), newData);
  }
}
