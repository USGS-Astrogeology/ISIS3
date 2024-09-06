/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "AbstractPointItem.h"

#include <QDateTime>
#include <QMessageBox>
#include <QString>
#include <QVariant>

#include "CnetDisplayProperties.h"
#include "ControlMeasure.h"
#include "ControlNet.h"
#include "ControlPoint.h"
#include "IException.h"
#include "Latitude.h"
#include "Longitude.h"
#include "TableColumn.h"
#include "TableColumnList.h"


namespace Isis {
  QString AbstractPointItem::getColumnName(Column col) {
    CnetDisplayProperties *displayProperties = CnetDisplayProperties::getInstance();
    bool latLonRadiusCoordDisplay = false;
    if (displayProperties->coordinateDisplayType() == CnetDisplayProperties::LatLonRadius) {
      latLonRadiusCoordDisplay = true;
    }

    switch (col) {
      case Id:
        return "Point ID";
      case PointType:
        return "Point Type";
      case ChooserName:
        return "Chooser Name";
      case DateTime:
        return "Date Time";
      case EditLock:
        return "Edit Lock";
      case Ignored:
        return "Ignored";
      case Reference:
        return "Reference";
      case AdjustedSPCoord1:
        if (latLonRadiusCoordDisplay) {
          return "Adjusted SP Lat";
        }
        else {
          return "Adjusted SP X";
        }
      case AdjustedSPCoord2:
        if (latLonRadiusCoordDisplay) {
          return "Adjusted SP Lon";
        }
        else {
          return "Adjusted SP Y";
        }
      case AdjustedSPCoord3:
        if (latLonRadiusCoordDisplay) {
          return "Adjusted SP Radius";
        }
        else {
          return "Adjusted SP Z";
        }
      case AdjustedSPCoord1Sigma:
        if (latLonRadiusCoordDisplay) {
          return "Adjusted SP Lat Sigma";
        }
        else {
          return "Adjusted SP X Sigma";
        }
      case AdjustedSPCoord2Sigma:
        if (latLonRadiusCoordDisplay) {
          return "Adjusted SP Lon Sigma";
        }
        else {
          return "Adjusted SP Y Sigma";
        }
      case AdjustedSPCoord3Sigma:
        if (latLonRadiusCoordDisplay) {
          return "Adjusted SP Radius Sigma";
        }
        else {
          return "Adjusted SP Z Sigma";
        }
      case APrioriSPCoord1:
        if (latLonRadiusCoordDisplay) {
          return "A Priori SP Lat";
        }
        else {
          return "A Priori SP X";
        }
      case APrioriSPCoord2:
        if (latLonRadiusCoordDisplay) {
          return "A Priori SP Lon";
        }
        else {
          return "A Priori SP Y";
        }
      case APrioriSPCoord3:
        if (latLonRadiusCoordDisplay) {
          return "A Priori SP Radius";
        }
        else {
          return "A Priori SP Z";
        }
      case APrioriSPCoord1Sigma:
        if (latLonRadiusCoordDisplay) {
          return "A Priori SP Lat Sigma";
        }
        else {
          return "A Priori SP X Sigma";
        }
      case APrioriSPCoord2Sigma:
        if (latLonRadiusCoordDisplay) {
          return "A Priori SP Lon Sigma";
        }
        else {
          return "A Priori SP Y Sigma";
        }
      case APrioriSPCoord3Sigma:
        if (latLonRadiusCoordDisplay) {
          return "A Priori SP Radius Sigma";
        }
        else {
          return "A Priori SP Z Sigma";
        }
      case APrioriSPSource:
        return "A Priori SP Source";
      case APrioriSPSourceFile:
        return "A Priori SP Source File";
      case APrioriRadiusSource:
        return "A Priori Radius Source";
      case APrioriRadiusSourceFile:
        return "A Priori Radius Source File";
      case JigsawRejected:
        return "Jigsaw Rejected";
    }

    return QString();
  }


  AbstractPointItem::Column AbstractPointItem::getColumn(QString columnTitle) {
    for (int i = 0; i < COLS; i++) {
      if (columnTitle == getColumnName((Column)i))
        return (Column)i;
    }

    std::string msg = "Column title [" + columnTitle + "] does not match any of "
        "the defined column types";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }


  TableColumnList *AbstractPointItem::createColumns() {
    TableColumnList *columnList = new TableColumnList;

    columnList->append(new TableColumn(getColumnName(Id), false, false));
    columnList->append(
      new TableColumn(getColumnName(PointType), false, false));
    columnList->append(
      new TableColumn(getColumnName(ChooserName), false, false));
    columnList->append(
      new TableColumn(getColumnName(DateTime), true, false));
    columnList->append(
      new TableColumn(getColumnName(EditLock), false, false));
    columnList->append(
      new TableColumn(getColumnName(Ignored), false, true));
    columnList->append(
      new TableColumn(getColumnName(Reference), false, false));
    columnList->append(
      new TableColumn(getColumnName(AdjustedSPCoord1), true, false));
    columnList->append(
      new TableColumn(getColumnName(AdjustedSPCoord2), true, false));
    columnList->append(
      new TableColumn(getColumnName(AdjustedSPCoord3), true, false));
    columnList->append(
      new TableColumn(getColumnName(AdjustedSPCoord1Sigma), true, false));
    columnList->append(
      new TableColumn(getColumnName(AdjustedSPCoord2Sigma), true, false));
    columnList->append(
      new TableColumn(getColumnName(AdjustedSPCoord3Sigma), true, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPCoord1), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPCoord2), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPCoord3), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPCoord1Sigma), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPCoord2Sigma), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPCoord3Sigma), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPSource), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriSPSourceFile), false, false));
    columnList->append(
      new TableColumn(getColumnName(APrioriRadiusSource), false, false));
    columnList->append(new TableColumn(
        getColumnName(APrioriRadiusSourceFile), false, false));
    columnList->append(
      new TableColumn(getColumnName(JigsawRejected), true, false));

    return columnList;
  }


  AbstractPointItem::AbstractPointItem(ControlPoint *cp,
      int avgCharWidth, AbstractTreeItem *parent)
    : AbstractTreeItem(parent) {

    m_point = cp;
    calcDataWidth(avgCharWidth);

    connect(m_point, SIGNAL(destroyed(QObject *)), this, SLOT(sourceDeleted()));
  }


  AbstractPointItem::~AbstractPointItem() {
    m_point = NULL;
  }


  QVariant AbstractPointItem::getData() const {
    return getData(getColumnName(Id));
  }


  /**
   * Resets pertinent point table column headers based on the active point
   * coordinate display type (Lat/Lon/Radius or XYZ).
   *
   */
  void AbstractPointItem::resetColumnHeaders(TableColumnList *columns) {
    TableColumn *col = (*columns)[8];
    col->setTitle(getColumnName(AdjustedSPCoord1));
    col = (*columns)[9];
    col->setTitle(getColumnName(AdjustedSPCoord2));
    col = (*columns)[10];
    col->setTitle(getColumnName(AdjustedSPCoord3));

    col = (*columns)[11];
    col->setTitle(getColumnName(AdjustedSPCoord1Sigma));
    col = (*columns)[12];
    col->setTitle(getColumnName(AdjustedSPCoord2Sigma));
    col = (*columns)[13];
    col->setTitle(getColumnName(AdjustedSPCoord3Sigma));

    col = (*columns)[14];
    col->setTitle(getColumnName(APrioriSPCoord1));
    col = (*columns)[15];
    col->setTitle(getColumnName(APrioriSPCoord2));
    col = (*columns)[16];
    col->setTitle(getColumnName(APrioriSPCoord3));

    col = (*columns)[17];
    col->setTitle(getColumnName(APrioriSPCoord1Sigma));
    col = (*columns)[18];
    col->setTitle(getColumnName(APrioriSPCoord2Sigma));
    col = (*columns)[19];
    col->setTitle(getColumnName(APrioriSPCoord3Sigma));
  }


  QVariant AbstractPointItem::getData(QString columnTitle) const {
    CnetDisplayProperties *displayProperties = CnetDisplayProperties::getInstance();

    if (m_point) {
      bool latLonRadiusCoordDisplay = false;
      if (displayProperties->coordinateDisplayType() == CnetDisplayProperties::LatLonRadius) {
        latLonRadiusCoordDisplay = true;
      }

      Column column = getColumn(columnTitle);

      switch ((Column) column) {
        case Id:
          return QVariant((QString)m_point->GetId());
        case PointType:
          return QVariant((QString)m_point->GetPointTypeString());
        case ChooserName:
          return QVariant((QString)m_point->GetChooserName());
        case DateTime:
          //          return QVariant(QDateTime::fromString(
          //              m_point->GetDateTime(), "yyyy-MM-ddTHH:mm:ss"));
          return QVariant((QString)m_point->GetDateTime());
        case EditLock:
          if (m_point->IsEditLocked())
            return QVariant("Yes");
          else
            return QVariant("No");
          break;
        case Ignored:
          if (m_point->IsIgnored())
            return QVariant("Yes");
          else
            return QVariant("No");
        case Reference:
          if (m_point->GetNumMeasures())
            return QVariant(
                CnetDisplayProperties::getInstance()->getImageName(
                    (QString) m_point->GetRefMeasure()->GetCubeSerialNumber()));
          else
            return QVariant();
        case AdjustedSPCoord1:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetLatitude().degrees());
          }
          else {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetX().meters());
          }
        case AdjustedSPCoord2:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetLongitude().degrees());
          }
          else {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetY().meters());
          }
        case AdjustedSPCoord3:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetLocalRadius().meters());
          }
          else {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetZ().meters());
          }
        case AdjustedSPCoord1Sigma:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetLatSigmaDistance().meters());
          }
          else {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetXSigma().meters());
          }
        case AdjustedSPCoord2Sigma:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetLonSigmaDistance().meters());
          }
          else {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetYSigma().meters());
          }
        case AdjustedSPCoord3Sigma:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetLocalRadiusSigma().meters());
          }
          else {
            return QVariant(m_point->GetAdjustedSurfacePoint().GetZSigma().meters());
          }
        case APrioriSPCoord1:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAprioriSurfacePoint().GetLatitude().degrees());
          }
          else {
            return QVariant(m_point->GetAprioriSurfacePoint().GetX().meters());
          }
        case APrioriSPCoord2:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAprioriSurfacePoint().GetLongitude().degrees());
          }
          else {
            return QVariant(m_point->GetAprioriSurfacePoint().GetY().meters());
          }
        case APrioriSPCoord3:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAprioriSurfacePoint().GetLocalRadius().meters());
          }
          else {
            return QVariant(m_point->GetAprioriSurfacePoint().GetZ().meters());
          }
        case APrioriSPCoord1Sigma:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAprioriSurfacePoint().GetLatSigmaDistance().meters());
          }
          else {
            return QVariant(m_point->GetAprioriSurfacePoint().GetXSigma().meters());
          }
        case APrioriSPCoord2Sigma:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAprioriSurfacePoint().GetLonSigmaDistance().meters());
          }
          else {
            return QVariant(m_point->GetAprioriSurfacePoint().GetYSigma().meters());
          }
        case APrioriSPCoord3Sigma:
          if (latLonRadiusCoordDisplay) {
            return QVariant(m_point->GetAprioriSurfacePoint().GetLocalRadiusSigma().meters());
          }
          else {
            return QVariant(m_point->GetAprioriSurfacePoint().GetZSigma().meters());
          }
        case APrioriSPSource:
          return QVariant((QString)m_point->GetSurfacePointSourceString());
        case APrioriSPSourceFile:
          return QVariant((QString)m_point->GetAprioriSurfacePointSourceFile());
        case APrioriRadiusSource:
          return QVariant((QString)m_point->GetRadiusSourceString());
        case APrioriRadiusSourceFile:
          return QVariant((QString)m_point->GetAprioriRadiusSourceFile());
        case JigsawRejected:
          if (m_point->IsRejected())
            return QVariant("Yes");
          else
            return QVariant("No");
      }
    }

    return QVariant();
  }


  void AbstractPointItem::setData(QString const &columnTitle,
      QString const &newData) {

      CnetDisplayProperties *displayProperties = CnetDisplayProperties::getInstance();
    if (m_point) {
      bool latLonRadiusCoordDisplay = false;
      if (displayProperties->coordinateDisplayType() == CnetDisplayProperties::LatLonRadius) {
        latLonRadiusCoordDisplay = true;
      }

      Column column = getColumn(columnTitle);

      switch ((Column) column) {
        case Id:
          m_point->SetId(newData);
          break;
        case PointType:
          m_point->SetType(m_point->StringToPointType(newData));
          break;
        case ChooserName:
          m_point->SetChooserName(newData);
          break;
        case DateTime:
          m_point->SetDateTime(newData);
          break;
        case EditLock:
          if (newData == "Yes")
            m_point->SetEditLock(true);
          else
            m_point->SetEditLock(false);
          break;
        case Ignored:
          m_point->SetIgnored(newData == "Yes");
          break;
        case Reference:
          m_point->SetRefMeasure(newData);
          break;
        case AdjustedSPCoord1:
          if (latLonRadiusCoordDisplay) {
            m_point->SetAdjustedSurfacePoint(SurfacePoint(
                Latitude(catchNull(newData), Angle::Degrees),
                m_point->GetAdjustedSurfacePoint().GetLongitude(),
                m_point->GetAdjustedSurfacePoint().GetLocalRadius()));
          }
          else {
              m_point->SetAdjustedSurfacePoint(SurfacePoint(
                  Distance(catchNull(newData), Distance::Meters),
                  m_point->GetAdjustedSurfacePoint().GetY(),
                  m_point->GetAdjustedSurfacePoint().GetZ()));
          }
          break;
        case AdjustedSPCoord2:
          if (latLonRadiusCoordDisplay) {
            m_point->SetAdjustedSurfacePoint(SurfacePoint(
                m_point->GetAdjustedSurfacePoint().GetLatitude(),
                Longitude(catchNull(newData), Angle::Degrees),
                m_point->GetAdjustedSurfacePoint().GetLocalRadius()));
          }
          else {
              m_point->SetAdjustedSurfacePoint(SurfacePoint(
                  m_point->GetAdjustedSurfacePoint().GetX(),
                  Distance(catchNull(newData), Distance::Meters),
                  m_point->GetAdjustedSurfacePoint().GetZ()));
          }
          break;
        case AdjustedSPCoord3:
          if (latLonRadiusCoordDisplay) {
            m_point->SetAdjustedSurfacePoint(SurfacePoint(
                m_point->GetAdjustedSurfacePoint().GetLatitude(),
                m_point->GetAdjustedSurfacePoint().GetLongitude(),
                Distance(catchNull(newData), Distance::Meters)));
          }
          else {
              m_point->SetAdjustedSurfacePoint(SurfacePoint(
                  m_point->GetAdjustedSurfacePoint().GetX(),
                  m_point->GetAdjustedSurfacePoint().GetY(),
                  Distance(catchNull(newData), Distance::Meters)));
          }
          break;
        case AdjustedSPCoord1Sigma: {
          std::string msg;
          if (latLonRadiusCoordDisplay) {
            msg = "Cannot set adjusted surface point latitude sigma";
          }
          else {
            msg = "Cannot set adjusted surface point X sigma";
          }
          throw IException(IException::Programmer, msg, _FILEINFO_);
          break;
        }
        case AdjustedSPCoord2Sigma: {
          std::string msg;
          if (latLonRadiusCoordDisplay) {
            msg = "Cannot set adjusted surface point longitude sigma";
          }
          else {
            msg = "Cannot set adjusted surface point Y sigma";
          }
          throw IException(IException::Programmer, msg, _FILEINFO_);
          break;
        }
        case AdjustedSPCoord3Sigma: {
          std::string msg;
          if (latLonRadiusCoordDisplay) {
            msg = "Cannot set adjusted surface point radius sigma";
          }
          else {
            msg = "Cannot set adjusted surface point Z sigma";
          }
          throw IException(IException::Programmer, msg, _FILEINFO_);
          break;
        }
        case APrioriSPCoord1: {
          if (latLonRadiusCoordDisplay) {
            Latitude newLat(catchNull(newData), Angle::Degrees);
            SurfacePoint newSurfacePoint(prepareSurfacePoint(newLat,
                m_point->GetAprioriSurfacePoint()));

            newSurfacePoint.SetSphericalCoordinates(newLat,
                newSurfacePoint.GetLongitude(),
                newSurfacePoint.GetLocalRadius());
            m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          else {
              Distance newX(catchNull(newData), Distance::Meters);
              SurfacePoint newSurfacePoint(prepareSurfacePoint(newX,
                  m_point->GetAprioriSurfacePoint()));

              newSurfacePoint.SetRectangularCoordinates(newX,
                  newSurfacePoint.GetY(),
                  newSurfacePoint.GetZ());
              m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          break;
        }
        case APrioriSPCoord2: {
          if (latLonRadiusCoordDisplay) {
            Longitude newLon(catchNull(newData), Angle::Degrees);
            SurfacePoint newSurfacePoint(prepareSurfacePoint(newLon,
                m_point->GetAprioriSurfacePoint()));

            newSurfacePoint.SetSphericalCoordinates(
              newSurfacePoint.GetLatitude(),
              newLon,
              newSurfacePoint.GetLocalRadius());
            m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          else {
            Distance newY(catchNull(newData), Distance::Meters);
            SurfacePoint newSurfacePoint(prepareSurfacePoint(newY,
                m_point->GetAprioriSurfacePoint()));

            newSurfacePoint.SetRectangularCoordinates(newSurfacePoint.GetX(),
                newY,
                newSurfacePoint.GetZ());
            m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          break;
        }
        case APrioriSPCoord3: {
          if (latLonRadiusCoordDisplay) {
            Distance newRadius(catchNull(newData), Distance::Meters);
            SurfacePoint newSurfacePoint(prepareSurfacePoint(newRadius,
                m_point->GetAprioriSurfacePoint()));

            newSurfacePoint.SetSphericalCoordinates(
              newSurfacePoint.GetLatitude(),
              newSurfacePoint.GetLongitude(),
              newRadius);
            m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          else {
              Distance newZ(catchNull(newData), Distance::Meters);
              SurfacePoint newSurfacePoint(prepareSurfacePoint(newZ,
                  m_point->GetAprioriSurfacePoint()));

              newSurfacePoint.SetRectangularCoordinates(newSurfacePoint.GetX(),
                  newSurfacePoint.GetY(),
                  newZ);
              m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          break;
        }
        case APrioriSPCoord1Sigma: {
          if (latLonRadiusCoordDisplay) {
            Distance newSigma(catchNull(newData), Distance::Meters);
            SurfacePoint newSurfacePoint(prepareSigmas(newSigma,
                m_point->GetAprioriSurfacePoint()));

            newSurfacePoint.SetSphericalSigmasDistance(
                newSigma, newSurfacePoint.GetLonSigmaDistance(),
                newSurfacePoint.GetLocalRadiusSigma());

            m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          else {
              Distance newSigma(catchNull(newData), Distance::Meters);
              SurfacePoint newSurfacePoint(prepareXYZSigmas(newSigma,
                  m_point->GetAprioriSurfacePoint()));

              newSurfacePoint.SetRectangularSigmas(
                  newSigma, newSurfacePoint.GetYSigma(),
                  newSurfacePoint.GetZSigma());

              m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          break;
        }
        case APrioriSPCoord2Sigma: {
          if (latLonRadiusCoordDisplay) {
            Distance newSigma(catchNull(newData), Distance::Meters);
            SurfacePoint newSurfacePoint(prepareSigmas(newSigma,
                m_point->GetAprioriSurfacePoint()));

            newSurfacePoint.SetSphericalSigmasDistance(
              newSurfacePoint.GetLatSigmaDistance(), newSigma,
              newSurfacePoint.GetLocalRadiusSigma());

            m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          else {
              Distance newSigma(catchNull(newData), Distance::Meters);
              SurfacePoint newSurfacePoint(prepareXYZSigmas(newSigma,
                  m_point->GetAprioriSurfacePoint()));

              newSurfacePoint.SetRectangularSigmas(
                newSurfacePoint.GetXSigma(), newSigma,
                newSurfacePoint.GetZSigma());

              m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          break;
        }
        case APrioriSPCoord3Sigma: {
          if (latLonRadiusCoordDisplay) {
              Distance newSigma(catchNull(newData), Distance::Meters);
              SurfacePoint newSurfacePoint(prepareSigmas(newSigma,
                  m_point->GetAprioriSurfacePoint()));

              newSurfacePoint.SetSphericalSigmasDistance(
                newSurfacePoint.GetLatSigmaDistance(),
                newSurfacePoint.GetLonSigmaDistance(),
                newSigma);

              m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          else {
              Distance newSigma(catchNull(newData), Distance::Meters);
              SurfacePoint newSurfacePoint(prepareXYZSigmas(newSigma,
                  m_point->GetAprioriSurfacePoint()));

              newSurfacePoint.SetRectangularSigmas(
                newSurfacePoint.GetXSigma(),
                newSurfacePoint.GetYSigma(),
                newSigma);

              m_point->SetAprioriSurfacePoint(newSurfacePoint);
          }
          break;
        }
        case APrioriSPSource:
          m_point->SetAprioriSurfacePointSource(
            m_point->StringToSurfacePointSource(newData));
          break;
        case APrioriSPSourceFile:
          m_point->SetAprioriSurfacePointSourceFile(newData);
          break;
        case APrioriRadiusSource:
          m_point->SetAprioriRadiusSource(
            m_point->StringToRadiusSource(newData));
          break;
        case APrioriRadiusSourceFile:
          m_point->SetAprioriRadiusSourceFile(newData);
          break;
        case JigsawRejected:
          // jigsaw rejected is not editable!
          break;
      }
    }
  }


  // Returns true if the data at the given column is locked (i.e.
  // edit-locked). If the m_point is edit-locked, all columns except the edit
  // lock column should be uneditable.
  bool AbstractPointItem::isDataEditable(QString columnTitle) const {
    bool locked = true;
    if (m_point->IsEditLocked()) {
      if (getColumn(columnTitle) == EditLock)
        locked = false;
    }
    else {
      locked = false;
    }

    return !locked;
  }


  void AbstractPointItem::deleteSource() {
    if (m_point) {
      if (m_point->IsEditLocked()) {
        std::string msg = "Point [" + getFormattedData() + "] is edit locked and "
            "cannot be deleted";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      else if (m_point->GetNumLockedMeasures() > 0) {
        std::string msg = "Point [" + getFormattedData() + "] has at least one "
            "edit locked measure and cannot be deleted";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      ControlPoint *tempPoint = m_point;
      m_point = NULL;
      tempPoint->Parent()->DeletePoint(tempPoint);
    }
  }


  AbstractTreeItem::InternalPointerType AbstractPointItem::getPointerType() const {
    return AbstractTreeItem::Point;
  }


  void *AbstractPointItem::getPointer() const {
    return m_point;
  }


  bool AbstractPointItem::hasPoint(ControlPoint *p) const {
    return m_point == p;
  }

  void AbstractPointItem::sourceDeleted() {
    m_point = NULL;
  }


  SurfacePoint AbstractPointItem::prepareSigmas(Distance newSigma,
      SurfacePoint surfacePoint) {
    const Distance free(10000, Distance::Meters);
    Distance latSigDist = surfacePoint.GetLatSigmaDistance();
    Distance lonSigDist = surfacePoint.GetLonSigmaDistance();
    Distance radiusSigDist = surfacePoint.GetLocalRadiusSigma();

    if (newSigma.isValid()) {
      if (!latSigDist.isValid())
        latSigDist = free;
      if (!lonSigDist.isValid())
        lonSigDist = free;
      if (!radiusSigDist.isValid())
        radiusSigDist = free;
    }
    else {
      latSigDist = Distance();
      lonSigDist = Distance();
      radiusSigDist = Distance();
    }

    surfacePoint.SetSphericalSigmasDistance(
      latSigDist, lonSigDist, radiusSigDist);
    return surfacePoint;
  }


  SurfacePoint AbstractPointItem::prepareXYZSigmas(Distance newSigma,
      SurfacePoint surfacePoint) {
    const Distance free(10000, Distance::Meters);
    Distance xSigma = surfacePoint.GetXSigma();
    Distance ySigma = surfacePoint.GetYSigma();
    Distance zSigma = surfacePoint.GetZSigma();

    if (newSigma.isValid()) {
      if (!xSigma.isValid())
        xSigma = free;
      if (!ySigma.isValid())
        ySigma = free;
      if (!zSigma.isValid())
        zSigma = free;
    }
    else {
      xSigma = Distance();
      ySigma = Distance();
      zSigma = Distance();
    }

    surfacePoint.SetRectangularSigmas(xSigma, ySigma, zSigma);
    return surfacePoint;
  }


  SurfacePoint AbstractPointItem::prepareSurfacePoint(Latitude newLat,
      SurfacePoint surfacePoint) {
    if (newLat.isValid()) {
      surfacePoint = prepareSurfacePoint(surfacePoint);
    }
    else {
      surfacePoint.SetSphericalCoordinates(Latitude(), Longitude(),
          Distance());
    }

    return surfacePoint;
  }


  SurfacePoint AbstractPointItem::prepareSurfacePoint(Longitude newLon,
      SurfacePoint surfacePoint) {
    if (newLon.isValid()) {
      surfacePoint = prepareSurfacePoint(surfacePoint);
    }
    else {
      surfacePoint.SetSphericalCoordinates(Latitude(), Longitude(),
          Distance());
    }

    return surfacePoint;
  }


  SurfacePoint AbstractPointItem::prepareSurfacePoint(
    Distance newRadius, SurfacePoint surfacePoint) {
    if (newRadius.isValid()) {
      surfacePoint = prepareSurfacePoint(surfacePoint);
    }
    else {
      surfacePoint.SetSphericalCoordinates(Latitude(), Longitude(),
          Distance());
    }

    return surfacePoint;
  }


  SurfacePoint AbstractPointItem::prepareSurfacePoint(
    SurfacePoint surfacePoint) {
    Latitude lat = surfacePoint.GetLatitude();
    Longitude lon = surfacePoint.GetLongitude();
    Distance radius = surfacePoint.GetLocalRadius();

    if (!lat.isValid())
      lat = Latitude(0, Angle::Degrees);
    if (!lon.isValid())
      lon = Longitude(0, Angle::Degrees);
    if (!radius.isValid())
      radius = Distance(10000, Distance::Meters);

    surfacePoint.SetSphericalCoordinates(lat, lon, radius);
    return surfacePoint;
  }
}
