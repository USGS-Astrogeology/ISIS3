#include <gtest/gtest.h>
#include <QString>

#include "Area3D.h"
#include "Displacement.h"
#include "Distance.h"
#include "IException.h"
#include "TestUtilities.h"


TEST(Area3D, DefaultConstructor) {
  Isis::Area3D area;
  EXPECT_FALSE(area.isValid());
}

TEST(Area3D, DisplacementConstructor) {
  Isis::Displacement disp(10, Isis::Displacement::Units::Meters);
  Isis::Area3D area(disp, disp, disp, disp, disp, disp);
  EXPECT_TRUE(area.isValid());
  EXPECT_TRUE(area.getStartX() == disp);
  EXPECT_TRUE(area.getStartY() == disp);
  EXPECT_TRUE(area.getStartZ() == disp);
  EXPECT_TRUE(area.getEndX() == disp);
  EXPECT_TRUE(area.getEndY() == disp);
  EXPECT_TRUE(area.getEndZ() == disp);
  EXPECT_DOUBLE_EQ(area.getHeight().meters(), 0);
  EXPECT_DOUBLE_EQ(area.getWidth().meters(), 0);
  EXPECT_DOUBLE_EQ(area.getDepth().meters(), 0);
}

TEST(Area3D, DistanceConstructor) {
  Isis::Displacement disp(10, Isis::Displacement::Meters);
  Isis::Distance distance(15, Isis::Distance::Meters);
  Isis::Area3D area(disp, disp, disp, distance, distance, distance);
  EXPECT_TRUE(area.isValid());
  EXPECT_TRUE(area.getStartX() == disp);
  EXPECT_TRUE(area.getStartY() == disp);
  EXPECT_TRUE(area.getStartZ() == disp);
  EXPECT_DOUBLE_EQ(area.getEndX().meters(), 25);
  EXPECT_DOUBLE_EQ(area.getEndY().meters(), 25);
  EXPECT_DOUBLE_EQ(area.getEndZ().meters(), 25);
  EXPECT_TRUE(area.getHeight() == distance);
  EXPECT_TRUE(area.getWidth() == distance);
  EXPECT_TRUE(area.getDepth() == distance);
}

TEST(Area3D, CopyConstructor) {
  Isis::Displacement disp(10, Isis::Displacement::Meters);
  Isis::Distance distance(15, Isis::Distance::Meters);
  Isis::Area3D area1(disp, disp, disp, distance, distance, distance);
  Isis::Area3D area2(area1);
  EXPECT_TRUE(area2.isValid());
  EXPECT_TRUE(area2.getStartX() == area1.getStartX());
  EXPECT_TRUE(area2.getStartY() == area1.getStartY());
  EXPECT_TRUE(area2.getStartZ() == area1.getStartZ());
  EXPECT_TRUE(area2.getEndX() == area1.getEndX());
  EXPECT_TRUE(area2.getEndY() == area1.getEndY());
  EXPECT_TRUE(area2.getEndZ() == area1.getEndZ());
  EXPECT_TRUE(area2.getHeight() == area1.getHeight());
  EXPECT_TRUE(area2.getWidth() == area1.getWidth());
  EXPECT_TRUE(area2.getDepth() == area1.getDepth());
}

TEST(Area3D, InvalidInputConstructor) {
  Isis::Displacement d1(0, Isis::Displacement::Meters);
  Isis::Distance d2(0, Isis::Distance::Meters);
  Isis::Area3D area1(Isis::Displacement(), d1, d1, d1, d1, d1);
  Isis::Area3D area2(d1, d1, d1, d2, Isis::Distance(), d2);
  EXPECT_FALSE(area1.isValid());
  EXPECT_FALSE(area2.isValid());
}

TEST(Area3D, EndPointCalculations) {
  Isis::Displacement x(10, Isis::Displacement::Meters);
  Isis::Displacement y(-15, Isis::Displacement::Meters);
  Isis::Displacement z(20, Isis::Displacement::Meters);
  Isis::Distance width(10, Isis::Distance::Meters);
  Isis::Distance height(5, Isis::Distance::Meters);
  Isis::Distance depth(5, Isis::Distance::Meters);
  Isis::Area3D area(x, y, z, width, height, depth);
  EXPECT_DOUBLE_EQ(area.getEndX().meters(), 20);
  EXPECT_DOUBLE_EQ(area.getEndY().meters(), -10);
  EXPECT_DOUBLE_EQ(area.getEndZ().meters(), 25);
}

TEST(Area3D, DimensionCalculations) {
  Isis::Displacement x0(-10, Isis::Displacement::Meters);
  Isis::Displacement y0(0, Isis::Displacement::Meters);
  Isis::Displacement z0(-1, Isis::Displacement::Meters);
  Isis::Displacement x1(50, Isis::Displacement::Meters);
  Isis::Displacement y1(25, Isis::Displacement::Meters);
  Isis::Displacement z1(99, Isis::Displacement::Meters);
  Isis::Area3D area(x0, y0, z0, x1, y1, z1);
  EXPECT_DOUBLE_EQ(area.getWidth().meters(), 60);
  EXPECT_DOUBLE_EQ(area.getHeight().meters(), 25);
  EXPECT_DOUBLE_EQ(area.getDepth().meters(), 100);
}

TEST(Area3D, Intersect) {
  Isis::Displacement start1(0, Isis::Displacement::Meters);
  Isis::Distance dim1(1, Isis::Distance::Meters);
  Isis::Displacement start2(0, Isis::Displacement::Meters);
  Isis::Distance dim2(0.5, Isis::Distance::Meters);
  Isis::Area3D area1(start1, start1, start1, dim1, dim1, dim1);
  Isis::Area3D area2(start2, start2, start2, dim2, dim2, dim2);
  EXPECT_TRUE(area1.intersect(area2) == area2);

  dim2.setMeters(1.5);
  area2.setWidth(dim2);
  area2.setHeight(dim2);
  area2.setDepth(dim2);
  EXPECT_TRUE(area1.intersect(area2) == area1);

  start2.setMeters(0.5);
  area2.setStartX(start2);
  area2.setStartY(start2);
  area2.setStartZ(start2);
  dim1.setMeters(0.5);
  Isis::Area3D area3(start2, start2, start2, dim1, dim1, dim1);
  EXPECT_TRUE(area1.intersect(area2) == area3);
}

TEST(Area3D, NoOverlapIntersect) {
  Isis::Displacement start1(0, Isis::Displacement::Meters);
  Isis::Displacement end1(1, Isis::Displacement::Meters);
  Isis::Displacement start2(2, Isis::Displacement::Meters);
  Isis::Displacement end2(3, Isis::Displacement::Meters);
  Isis::Area3D area1(start1, start1, start1, end1, end1, end1);
  Isis::Area3D area2(start2, start2, start2, end2, end2, end2);
  Isis::Area3D area3(area1.intersect(area2));
  EXPECT_FALSE(area3.isValid());
}

TEST(Area3D, Setters) {
  Isis::Area3D area1;
  Isis::Displacement start(0, Isis::Displacement::Meters);
  Isis::Displacement end(10, Isis::Displacement::Meters);
  Isis::Displacement move(3, Isis::Displacement::Meters);
  Isis::Distance dim(5, Isis::Distance::Meters);

  area1.setStartX(start);
  area1.setStartY(start);
  area1.setStartZ(start);
  area1.setEndX(end);
  area1.setEndY(end);
  area1.setEndZ(end);
  EXPECT_DOUBLE_EQ(area1.getEndX().meters(), 10);
  EXPECT_DOUBLE_EQ(area1.getEndY().meters(), 10);
  EXPECT_DOUBLE_EQ(area1.getEndZ().meters(), 10);
  EXPECT_DOUBLE_EQ(area1.getStartX().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);

  area1.setXDimension(start, dim);
  area1.setYDimension(start, dim);
  area1.setZDimension(start, dim);
  EXPECT_DOUBLE_EQ(area1.getEndX().meters(), 5);
  EXPECT_DOUBLE_EQ(area1.getEndY().meters(), 5);
  EXPECT_DOUBLE_EQ(area1.getEndZ().meters(), 5);
  EXPECT_DOUBLE_EQ(area1.getStartX().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);

  area1.moveEndX(move);
  area1.moveEndY(move);
  area1.moveEndZ(move);
  EXPECT_DOUBLE_EQ(area1.getEndX().meters(), 3);
  EXPECT_DOUBLE_EQ(area1.getEndY().meters(), 3);
  EXPECT_DOUBLE_EQ(area1.getEndZ().meters(), 3);
  EXPECT_DOUBLE_EQ(area1.getStartX().meters(), -2);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), -2);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), -2);

  move.setMeters(0);
  area1.moveStartX(move);
  area1.moveStartY(move);
  area1.moveStartZ(move);
  EXPECT_DOUBLE_EQ(area1.getEndX().meters(), 5);
  EXPECT_DOUBLE_EQ(area1.getEndY().meters(), 5);
  EXPECT_DOUBLE_EQ(area1.getEndZ().meters(), 5);
  EXPECT_DOUBLE_EQ(area1.getStartX().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);

  dim.setMeters(10);
  area1.setDepth(dim);
  area1.setHeight(dim);
  area1.setWidth(dim);
  EXPECT_DOUBLE_EQ(area1.getEndX().meters(), 10);
  EXPECT_DOUBLE_EQ(area1.getEndY().meters(), 10);
  EXPECT_DOUBLE_EQ(area1.getEndZ().meters(), 10);
  EXPECT_DOUBLE_EQ(area1.getStartX().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);
  EXPECT_DOUBLE_EQ(area1.getStartY().meters(), 0);
 }

TEST(Area3D, Operators) {
  Isis::Displacement start(5, Isis::Displacement::Meters);
  Isis::Displacement end(10, Isis::Displacement::Meters);
  Isis::Distance dim(5, Isis::Distance::Meters);
  Isis::Area3D area1(start, start, start, end, end, end);
  Isis::Area3D area2(start, start, start, dim, dim, dim);
  EXPECT_TRUE(area1 == area2);
  EXPECT_FALSE(area1 != area2);

  start.setMeters(1);
  area1.setStartX(start);
  EXPECT_FALSE(area1 == area2);
  EXPECT_TRUE(area1 != area2);

  Isis::Area3D area3 = area2;
  EXPECT_DOUBLE_EQ(area3.getStartX().meters(), 5);
  EXPECT_DOUBLE_EQ(area3.getStartY().meters(), 5);
  EXPECT_DOUBLE_EQ(area3.getStartZ().meters(), 5);
  EXPECT_DOUBLE_EQ(area3.getEndX().meters(), 10);
  EXPECT_DOUBLE_EQ(area3.getEndY().meters(), 10);
  EXPECT_DOUBLE_EQ(area3.getEndZ().meters(), 10);
}

TEST(Area3D, InvertedXError) {
  std::string message = "Cannot have a 3D area with inverted X";
  Isis::Displacement d1(-1, Isis::Displacement::Meters);
  Isis::Displacement d2(1, Isis::Displacement::Meters);
  try {
    Isis::Area3D area(d2, d1, d1, d1, d2, d2);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

TEST(Area3D, InvertedYError) {
  std::string message = "Cannot have a 3D area with inverted Y";
  Isis::Displacement d1(-1, Isis::Displacement::Meters);
  Isis::Displacement d2(1, Isis::Displacement::Meters);
  try {
    Isis::Area3D area(d1, d2, d1, d2, d1, d2);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}

TEST(Area3D, InvertedZError) {
  std::string message = "Cannot have a 3D area with inverted Z";
  Isis::Displacement d1(-1, Isis::Displacement::Meters);
  Isis::Displacement d2(1, Isis::Displacement::Meters);
  try {
    Isis::Area3D area(d1, d1, d2, d2, d2, d1);
  }
  catch(Isis::IException &e) {
    EXPECT_PRED_FORMAT2(Isis::AssertIExceptionMessage, e, message);
  }
  catch(...) {
    FAIL() << "Expected an IException with message \""
    << message.toStdString() <<"\"";
  }
}
