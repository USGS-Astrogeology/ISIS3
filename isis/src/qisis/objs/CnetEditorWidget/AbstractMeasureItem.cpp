/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AbstractMeasureItem.h"

#include <QMessageBox>
#include <QString>
#include <QVariant>

#include "CnetDisplayProperties.h"
#include "ControlMeasure.h"
#include "ControlMeasureLogData.h"
#include "ControlPoint.h"
#include "IException.h"


#include "TableColumn.h"
#include "TableColumnList.h"


namespace Isis {
  QString AbstractMeasureItem::getColumnName(Column col) {
    switch (col) {
      case PointId:
        return "Point ID";
      case ImageId:
        return "Image ID";
      case Sample:
        return "Sample";
      case Line:
        return "Line";
      case EditLock:
        return "Edit Locked";
      case Ignored:
        return "Ignored";
      case Reference:
        return "Reference";
      case Type:
        return "Measure Type";
      case Obsolete_Eccentricity:
        return "Obsolete_Eccentricity";
      case GoodnessOfFit:
        return "Goodness of Fit";
      case MinPixelZScore:
        return "Minimum Pixel Z-Score";
      case MaxPixelZScore:
        return "Maximum Pixel Z-Score";
      case SampleShift:
        return "Sample Shift";
      case LineShift:
        return "Line Shift";
      case SampleSigma:
        return "Sample Sigma";
      case LineSigma:
        return "Line Sigma";
      case APrioriSample:
        return "A Priori Sample";
      case APrioriLine:
        return "A Priori Line";
      case Diameter:
        return "Diameter";
      case JigsawRejected:
        return "Rejected by Jigsaw";
      case ResidualSample:
        return "Residual Sample";
      case ResidualLine:
        return "Residual Line";
      case ResidualMagnitude:
        return "Residual Magnitude";
    }

    return QString();
  }


  AbstractMeasureItem::Column AbstractMeasureItem::getColumn(
    QString columnTitle) {
    for (int i = 0; i < COLS; i++) {
      if (columnTitle == getColumnName((Column) i))
        return (Column) i;
    }

    std::string msg = "Column title [" + columnTitle + "] does not match any of "
        "the defined column types";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  TableColumnList *AbstractMeasureItem::createColumns() {
    TableColumnList *columnList = new TableColumnList;

    columnList->append(new TableColumn(getColumnName(PointId), true, false));
    columnList->append(new TableColumn(getColumnName(ImageId), true,
        true));
    columnList->append(new TableColumn(getColumnName(Sample), true, false));
    columnList->append(new TableColumn(getColumnName(Line), true, false));
    columnList->append(new TableColumn(getColumnName(EditLock), false,
        false));
    columnList->append(new TableColumn(getColumnName(Ignored), false, true));
    columnList->append(new TableColumn(getColumnName(Reference), true, true));
    columnList->append(new TableColumn(getColumnName(Type), false, false));
    columnList->append(new TableColumn(getColumnName(Obsolete_Eccentricity), true,
        false));
    columnList->append(new TableColumn(getColumnName(GoodnessOfFit), true,
        false));
    columnList->append(new TableColumn(getColumnName(MinPixelZScore), true,
        false));
    columnList->append(new TableColumn(getColumnName(MaxPixelZScore), true,
        false));
    columnList->append(new TableColumn(getColumnName(SampleShift), true,
        false));
    columnList->append(new TableColumn(getColumnName(LineShift), true,
        false));
    columnList->append(new TableColumn(getColumnName(SampleSigma), false,
        false));
    columnList->append(new TableColumn(getColumnName(LineSigma), false,
        false));
    columnList->append(new TableColumn(getColumnName(APrioriSample), true,
        false));
    columnList->append(new TableColumn(getColumnName(APrioriLine), true,
        false));
    columnList->append(new TableColumn(getColumnName(Diameter), false,
        false));
    columnList->append(new TableColumn(getColumnName(JigsawRejected), true,
        false));
    columnList->append(new TableColumn(getColumnName(ResidualSample), true,
        false));
    columnList->append(new TableColumn(getColumnName(ResidualLine), true,
        false));
    columnList->append(new TableColumn(getColumnName(ResidualMagnitude),
        true, false));

    return columnList;
  }

  AbstractMeasureItem::AbstractMeasureItem(ControlMeasure *cm,
      int avgCharWidth, AbstractTreeItem *parent)
    : AbstractTreeItem(parent) {

    m_measure = cm;
    calcDataWidth(avgCharWidth);

    connect(m_measure, SIGNAL(destroyed(QObject *)), this, SLOT(sourceDeleted()));
  }


  AbstractMeasureItem::~AbstractMeasureItem() {
    m_measure = NULL;
  }


  QVariant AbstractMeasureItem::getData() const {
    return getData(getColumnName(ImageId));
  }


  QVariant AbstractMeasureItem::getData(QString columnTitle) const {
    if (m_measure) {
      Column column = getColumn(columnTitle);

      switch ((Column) column) {
        case PointId:
          return QVariant((QString) m_measure->Parent()->GetId());
        case ImageId:
          return QVariant(CnetDisplayProperties::getInstance()->getImageName(
              (QString) m_measure->GetCubeSerialNumber()));
        case Sample:
          return QVariant(m_measure->GetSample());
        case Line:
          return QVariant(m_measure->GetLine());
        case EditLock:
          if (m_measure->IsEditLocked())
            return QVariant("Yes");
          else
            return QVariant("No");
        case Ignored:
          if (m_measure->IsIgnored())
            return QVariant("Yes");
          else
            return QVariant("No");
        case Reference:
          if (m_measure->Parent()->GetRefMeasure() == m_measure)
            return QVariant("Yes");
          else
            return QVariant("No");
        case Type:
          return QVariant(
              (QString)m_measure->MeasureTypeToString(m_measure->GetType()));
        case Obsolete_Eccentricity:
          return QVariant(
              m_measure->GetLogData(
                  ControlMeasureLogData::Obsolete_Eccentricity).GetNumericalValue());
        case GoodnessOfFit:
          return QVariant(
              m_measure->GetLogData(
                  ControlMeasureLogData::GoodnessOfFit).GetNumericalValue());
        case MinPixelZScore:
          return QVariant(
              m_measure->GetLogData(ControlMeasureLogData::MinimumPixelZScore).
              GetNumericalValue());
        case MaxPixelZScore:
          return QVariant(
              m_measure->GetLogData(ControlMeasureLogData::MaximumPixelZScore).
              GetNumericalValue());
        case SampleShift:
          return QVariant(m_measure->GetSampleShift());
        case LineShift:
          return QVariant(m_measure->GetLineShift());
        case SampleSigma:
          return QVariant(m_measure->GetSampleSigma());
        case LineSigma:
          return QVariant(m_measure->GetLineSigma());
        case APrioriSample:
          return QVariant(m_measure->GetAprioriSample());
        case APrioriLine:
          return QVariant(m_measure->GetAprioriLine());
        case Diameter:
          return QVariant(m_measure->GetDiameter());
        case JigsawRejected:
          if (m_measure->IsRejected())
            return QVariant("Yes");
          else
            return QVariant("No");
        case ResidualSample:
          return QVariant(m_measure->GetSampleResidual());
        case ResidualLine:
          return QVariant(m_measure->GetLineResidual());
        case ResidualMagnitude:
          return QVariant(
              m_measure->GetResidualMagnitude());
      }
    }

    return QVariant();
  }


  void AbstractMeasureItem::setData(QString const &columnTitle,
      QString const &newData) {
    if (m_measure) {
      Column column = getColumn(columnTitle);

      switch ((Column) column) {
        case PointId:
          // PointId is not editable in the measure table
          break;
        case ImageId:
          m_measure->SetCubeSerialNumber(
            CnetDisplayProperties::getInstance()->getSerialNumber(newData));
          break;
        case Sample:
          m_measure->SetCoordinate(catchNull(newData),
              m_measure->GetLine());
          break;
        case Line:
          m_measure->SetCoordinate(m_measure->GetSample(),
              catchNull(newData));
          break;
        case EditLock:
          if (newData == "Yes")
            m_measure->SetEditLock(true);
          else
            m_measure->SetEditLock(false);
          break;
        case Ignored:
          if (newData == "Yes")
            m_measure->SetIgnored(true);
          else if (newData == "No")
            m_measure->SetIgnored(false);
          break;
        case Reference:
          // A measure's reference status should never be editable. It should
          // only be changed through the point.
          break;
        case Type:
          m_measure->SetType(m_measure->StringToMeasureType(
              CnetDisplayProperties::getInstance()->getSerialNumber(
                  newData)));
          break;
        case Obsolete_Eccentricity:
          setLogData(m_measure, ControlMeasureLogData::Obsolete_Eccentricity, newData);
          break;
        case GoodnessOfFit:
          setLogData(m_measure, ControlMeasureLogData::GoodnessOfFit, newData);
          break;
        case MinPixelZScore:
          setLogData(m_measure, ControlMeasureLogData::MinimumPixelZScore,
              newData);
          break;
        case MaxPixelZScore:
          setLogData(m_measure, ControlMeasureLogData::MaximumPixelZScore,
              newData);
          break;
        case SampleShift:
          // This is not editable anymore.
          break;
        case LineShift:
          // This is not editable anymore.
          break;
        case SampleSigma:
          m_measure->SetSampleSigma(catchNull(newData));
          break;
        case LineSigma:
          m_measure->SetLineSigma(catchNull(newData));
          break;
        case APrioriSample:
          m_measure->SetAprioriSample(catchNull(newData));
          break;
        case APrioriLine:
          m_measure->SetAprioriLine(catchNull(newData));
          break;
        case Diameter:
          m_measure->SetDiameter(catchNull(newData));
          break;
        case JigsawRejected:
          // jigsaw rejected is not editable!
          break;
        case ResidualSample:
          m_measure->SetResidual(
            catchNull(newData), m_measure->GetLineResidual());
          break;
        case ResidualLine:
          m_measure->SetResidual(
            m_measure->GetSampleResidual(), catchNull(newData));
          break;
        case ResidualMagnitude:
          // residual magnitude is not editable!
          break;
      }
    }
  }


  // Returns true if the data at the given column is locked (i.e. is
  // edit-locked). If the m_measure is edit-locked, all columns except the edit
  // lock column should be uneditable. If the m_measure's parent point is
  // edit-locked, none of the columns should be editable as it should only be
  // unlocked from the parent point.
  bool AbstractMeasureItem::isDataEditable(QString columnTitle) const {
    bool parentLocked = !m_measure->Parent() ||
        m_measure->Parent()->IsEditLocked();
    bool locked = m_measure->IsEditLocked() || parentLocked;

    if (getColumn(columnTitle) == EditLock && !parentLocked) {
      locked = false;
    }

    return !locked;
  }


  void AbstractMeasureItem::deleteSource() {
    if (m_measure) {
      if (m_measure->Parent()->IsEditLocked()) {
        std::string msg = "Measures in point [" +
            getFormattedData(getColumnName(PointId)) +
            "] cannot be deleted because point is edit locked";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      else if (m_measure->IsEditLocked()) {
        std::string msg = "Measure [" + getFormattedData() + "] in point [" +
            getFormattedData(getColumnName(PointId)) +
            "] cannot be deleted because m_measure is edit locked";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      //       else if (m_measure->Parent()->GetRefMeasure() == m_measure) {
      //         std::string msg = "Measure [" + getData() + "] in point [" +
      //             getData(getColumnName(PointId)) + "] cannot be deleted because "
      //             "it is the reference";
      //         throw iException::Message(iException::User, msg, _FILEINFO_);
      //       }

      ControlMeasure *tempMeasure = m_measure;
      m_measure = NULL;
      tempMeasure->Parent()->Delete(tempMeasure);
    }
  }


  AbstractTreeItem::InternalPointerType AbstractMeasureItem::getPointerType()
  const {
    return AbstractTreeItem::Measure;
  }


  void *AbstractMeasureItem::getPointer() const {
    return m_measure;
  }


  bool AbstractMeasureItem::hasMeasure(ControlMeasure *m) const {
    return m_measure == m;
  }


  void AbstractMeasureItem::sourceDeleted() {
    m_measure = NULL;
  }


  void AbstractMeasureItem::setLogData(ControlMeasure *m_measure,
      int m_measureLogDataEnum, const QString &value) {

    QString newDataStr = value.toLower();
    ControlMeasureLogData::NumericLogDataType type =
      (ControlMeasureLogData::NumericLogDataType) m_measureLogDataEnum;

    if (newDataStr == "null") {
      m_measure->DeleteLogData(type);
    }
    else {
      m_measure->SetLogData(ControlMeasureLogData(type,
          value.toDouble()));
    }
  }
}
