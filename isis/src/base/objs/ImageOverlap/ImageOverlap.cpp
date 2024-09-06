/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <sstream>
#include <QStringList>

#include "geos/io/WKBReader.h"
#include "geos/io/WKBWriter.h"

#include "PolygonTools.h"
#include "IException.h"
#include "FileName.h"
#include "ImageOverlap.h"
#include "IString.h"

namespace Isis {

  /**
   * Construct an empty ImageOverlap object
   *
   */
  ImageOverlap::ImageOverlap() {
    Init();
  }


  /**
   * Construct an ImageOverlap object and initialize it with the arguments
   *
   * @param serialNumber The initial serial number assiciated with the polygon.
   *
   * @param polygon The polygon that defines the overlap area.
   *
   */
  ImageOverlap::ImageOverlap(QString serialNumber,
                             geos::geom::MultiPolygon &polygon) {
    Init();
    SetPolygon(polygon);
    Add(serialNumber);
  }


  /**
   * Construct an ImageOverlap object and initialize it from the istream
   *
   * @param inputStream A stream containing a representation of the image overlap
   *
   */
  ImageOverlap::ImageOverlap(std::istream &inputStream) {
    geos::io::WKBReader geosReader;

    std::string fileData;
    getline(inputStream, fileData);

    QString serialNums = fileData.c_str();
    foreach (QString serialNum, serialNums.split(",")) {
      Add(serialNum);
    }

    // now get the multipolygon on the next line
    getline(inputStream, fileData);

    std::stringstream multiPolygon;
    multiPolygon << fileData;
    multiPolygon.seekg(0, std::ios::beg);

    p_polygon = PolygonTools::MakeMultiPolygon(geosReader.readHEX(multiPolygon).release());
  }


  /**
   * Destroy this ImageOverlap object
   *
   */
  ImageOverlap::~ImageOverlap() {
    delete p_polygon;
  };


  /**
   * Initialize this object to a known state.
   *
   */
  void ImageOverlap::Init() {
    p_serialNumbers.clear();
    p_polygon = NULL;
  }


  /**
   * This method will replace the existing polygon that defines the overlap with
   * a new one.
   *
   * @param polygon The new polygon.
   *
   */
  void ImageOverlap::SetPolygon(const geos::geom::MultiPolygon &polygon) {
    if(p_polygon != NULL) {
      delete p_polygon;
      p_polygon = NULL;
    }

    p_polygon = PolygonTools::CopyMultiPolygon(polygon);
  }


  /**
   * This method will replace the existing polygon that defines the overlap with
   * a new one.
   *
   * @param polygon The new polygon.
   *
   */
  void ImageOverlap::SetPolygon(const geos::geom::MultiPolygon *polygon) {
    if(p_polygon != NULL) {
      delete p_polygon;
      p_polygon = NULL;
    }

    p_polygon = PolygonTools::CopyMultiPolygon(polygon);
  }


  void ImageOverlap::Write(std::ostream &outputStream) {

    geos::io::WKBWriter geosWriter;

    QString serialNums;

    for(unsigned int sn = 0; sn < p_serialNumbers.size(); sn++) {
      if(sn != 0) {
        serialNums += ",";
      }

      serialNums += p_serialNumbers[sn];
    }

    serialNums += "\n";

    outputStream << serialNums.toStdString();

    geosWriter.writeHEX(*p_polygon, outputStream);
  }


  /**
   * This method will add a new serial number to the list of serial numbers
   * alread associated with the overlap.
   *
   * @param sn The serial number to be added to the list.
   *
   */
  void ImageOverlap::Add(QString &sn) {
    for(unsigned int s = 0; s < p_serialNumbers.size(); ++s) {
      if(sn == p_serialNumbers[s]) {
        std::string msg = "Duplicate SN added to [" +
            p_polygon->toString() + "]";
        throw IException(IException::Programmer, msg, _FILEINFO_);
      }
    }

    p_serialNumbers.push_back(sn);
    return;
  }


  /**
   * This method will return the area of the polygon. This member does not
   * assume any particular units of measure for the verticies of the polygon.
   * It is provided as a convience.
   *
   */
  double ImageOverlap::Area() {
    return p_polygon->getArea();
  }


  /**
   * This method will return true if any serial number from this ImageOverlap is
   * also in the other ImageOverlap
   */
  bool ImageOverlap::HasAnySameSerialNumber(ImageOverlap &other) const {
    for(int thisSn = 0; thisSn < this->Size(); ++thisSn) {
      for(int otherSn = 0; otherSn < other.Size(); ++otherSn) {
        if(p_serialNumbers[thisSn] == other.p_serialNumbers[otherSn]) {
          return true;
        }
      }
    }
    return false;
  }


  /**
   * This method will return true if input serial number exists in the
   * ImageOverlap.
   *
   * @param[in] sn    (QString &)  Serial Number to search for
   *
   * @return bool  Returns true if the serial number exists in the
   *               ImageOverlap.
   */
  bool ImageOverlap::HasSerialNumber(QString &sn) const {
    for(int thisSn = 0; thisSn < Size(); ++thisSn) {
      if(p_serialNumbers[thisSn] == sn) {
        return true;
      }
    }
    return false;
  }
}
