/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

/*
$Id: hifringe.cpp,v 1.6 2008/05/14 21:07:24 slambright Exp $

Copyright (C) 2005, 2006  Arizona Board of Regents on behalf of the
Planetary Image Research Laboratory, Lunar and Planetary Laboratory at
the University of Arizona.

This library is free software; you can redistribute it and/or modify it
under the terms of the GNU Lesser General Public License, version 2.1,
as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation,
Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
*/

#include "Isis.h"

#include <string>

#include <QPair>
#include <QVector>

#include "Process.h"
#include "Statistics.h"
#include "LineManager.h"
#include "FileName.h"
#include "IString.h"

using namespace std;
using namespace Isis;


void pvlOut(Statistics stats1, Statistics stats2, QString name, int start,
            int end, PvlObject *one, PvlObject *two);

void IsisMain() {
  UserInterface &ui = Application::GetUserInterface();
  Isis::FileName fromFile = ui.GetCubeName("FROM");

  Isis::Cube inputCube;
  inputCube.open(fromFile.expanded());

  //Check to make sure we got the cube properly
  if(!inputCube.isOpen()) {
    std::string msg = "Could not open FROM cube" + fromFile.expanded();
    throw IException(IException::User, msg, _FILEINFO_);
  }
  Process p;
  Cube *icube = p.SetInputCube("FROM");

  int totalSamples = icube->sampleCount();
  int totalLines   = icube->lineCount();

  Isis::LineManager lineManager(inputCube);
  lineManager.begin();

  int leftFringe, rightFringe;
  int binningMode = icube->group("Instrument")["Summing"];

  //determine the edges between which no statistics should be gathered
  leftFringe = 48 / binningMode;
  rightFringe = totalSamples - leftFringe;


  int numSections = ui.GetInteger("SECTIONS");
  if(numSections > 9) {
    std::string msg = "You may have no more than 9 sections per side";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  int sectionLength = 0;
  if(!ui.WasEntered("LINESIZE")) { //User didn't enter number of lines
    if(numSections == 0) {
      sectionLength = 0;
    }
    else {
      sectionLength = totalLines / numSections;
    }
  }
  else {
    sectionLength = ui.GetInteger("LINESIZE");
    if((sectionLength * numSections > totalLines) || sectionLength < 1) {
      sectionLength = totalLines / numSections;
    }
  }

  QVector<QPair<Statistics,Statistics> > sections(numSections, qMakePair(Statistics(), Statistics()));
  Statistics leftTotal, rightTotal;
  QVector<int> sectionStarts(numSections, 0);
  sectionStarts[0] = 0;
  for(int i = 1 ; i < numSections - 1 ; i ++) {
    sectionStarts[i] = (totalLines / numSections) * i;
  }
  if(numSections > 0) {
    sectionStarts[numSections -1] = totalLines - sectionLength;
  }

  int currentSection = 0;
  Buffer leftFringeBuf(leftFringe, 1, 1, lineManager.PixelType());
  Buffer rightFringeBuf(leftFringe, 1, 1, lineManager.PixelType());

  //Walk down the cube
  for(int lineCount = 0 ; lineCount < totalLines ; lineCount++) {
    inputCube.read(lineManager);
    //Read the edges into the fringe buffers
    for(int i = 0 ; i < leftFringe ; i++) {
      leftFringeBuf[i] = lineManager[i];
    }
    for(int i = rightFringe ; i < totalSamples ; i++) {
      rightFringeBuf[i - rightFringe] = lineManager[i];
    }

    //No matter what, add the fringe buffers to the totals for that side
    leftTotal.AddData(leftFringeBuf.DoubleBuffer(), leftFringeBuf.size());
    rightTotal.AddData(rightFringeBuf.DoubleBuffer(), rightFringeBuf.size());

    if(numSections == 0) {
      continue;
    }
    //Index is not too large for this fringe section
    if(lineCount < sectionStarts[currentSection] + sectionLength) {
      //Index is not too small for this fringe section
      if(lineCount >= sectionStarts[currentSection]) {
        sections[currentSection].first.AddData(leftFringeBuf.DoubleBuffer(),
                                               leftFringeBuf.size());
        sections[currentSection].second.AddData(rightFringeBuf.DoubleBuffer(),
                                                rightFringeBuf.size());
      }
    }
    else {
      currentSection++;
      //Since sections may butt up against each other, it is possible that
      // we have to add this data to the next section.
      if(lineCount >= sectionStarts[currentSection]) {
        sections[currentSection].first.AddData(leftFringeBuf.DoubleBuffer(),
                                               leftFringeBuf.size());
        sections[currentSection].second.AddData(rightFringeBuf.DoubleBuffer(),
                                                rightFringeBuf.size());
      }
    }
  }

  // Write the results to the output file if the user specified one
  PvlObject leftSide("LeftSide"), rightSide("RightSide");
  for(int i = 0 ; i < numSections ; i++) {
    QString sectionName = "Section" + toString(i + 1);
    pvlOut(sections[i].first, //Stats to add to the left Object
           sections[i].second, //Stats to add to the right Object
           sectionName,    //Name for the new groups
           sectionStarts[i], //start line
           sectionStarts[i] + sectionLength, //end line
           &leftSide,      //Object to add left group to
           &rightSide      //Object to add right group to
          );
  }
  pvlOut(leftTotal,   //Stats to add to the left Object
         rightTotal,  //Stats to add to the right Object
         "Total",     //Name for the new groups
         0,           //start line
         totalLines,  //end line
         &leftSide,   //Object to add left group to
         &rightSide   //Object to add right group to
        );
  Pvl outputPvl;
  PvlGroup sourceInfo("SourceInfo");

  sourceInfo += PvlKeyword("From", fromFile.expanded());
  sourceInfo += icube->group("Archive")["ProductId"];
  outputPvl.addGroup(sourceInfo);
  if(numSections > 0) {
    outputPvl.addObject(leftSide);
    outputPvl.addObject(rightSide);
  }
  else {
    PvlGroup leftGroup = leftSide.findGroup("Total");
    PvlGroup rightGroup = rightSide.findGroup("Total");
    leftGroup.setName("LeftSide");
    rightGroup.setName("RightSide");
    outputPvl.addGroup(leftGroup);
    outputPvl.addGroup(rightGroup);
  }
  outputPvl.write(ui.GetFileName("TO").toStdString());
}

void pvlOut(Statistics stats1, Statistics stats2, QString name, int start,
            int end, PvlObject *one, PvlObject *two) {
  PvlGroup left(name.toStdString());
  left += PvlKeyword("StartLine", std::to_string(start + 1));
  left += PvlKeyword("EndLine", std::to_string(end));
  left += PvlKeyword("TotalPixels", std::to_string(stats1.TotalPixels()));
  left += PvlKeyword("ValidPixels", std::to_string(stats1.ValidPixels()));
  if(stats1.ValidPixels() > 0) {
    left += PvlKeyword("Mean", std::to_string(stats1.Average()));
    left += PvlKeyword("StandardDeviation", std::to_string(stats1.StandardDeviation()));
    left += PvlKeyword("Minimum", std::to_string(stats1.Minimum()));
    left += PvlKeyword("Maximum", std::to_string(stats1.Maximum()));
  }
  one->addGroup(left);

  PvlGroup right(name.toStdString());
  right += PvlKeyword("StartLine", std::to_string(start + 1));
  right += PvlKeyword("EndLine", std::to_string(end));
  right += PvlKeyword("TotalPixels", std::to_string(stats2.TotalPixels()));
  right += PvlKeyword("ValidPixels", std::to_string(stats2.ValidPixels()));
  if(stats2.ValidPixels() > 0) {
    right += PvlKeyword("Mean", std::to_string(stats2.Average()));
    right += PvlKeyword("StandardDeviation", std::to_string(stats2.StandardDeviation()));
    right += PvlKeyword("Minimum", std::to_string(stats2.Minimum()));
    right += PvlKeyword("Maximum", std::to_string(stats2.Maximum()));
  }
  two->addGroup(right);
}
