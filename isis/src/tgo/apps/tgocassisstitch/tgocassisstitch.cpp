/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "tgocassisstitch.h"

#include <QList>
#include <QMap>
#include <QString>

#include "AlphaCube.h"
#include "Blob.h"
#include "CubeAttribute.h"
#include "Cube.h"
#include "FileName.h"
#include "History.h"
#include "IException.h"
#include "LineManager.h"
#include "ObservationNumberList.h"
#include "ProcessByLine.h"
#include "Pvl.h"

using namespace std;

namespace Isis {
  static QMap<QString, FileName> sortFramelets(FileName frameletListFile);
  static void stitchFrame(QList<FileName> frameletList, FileName frameFileName);

  /**
   * Functor for stitching framelets into a full frame
   *
   * @author 2017-09-09 Jesse Mapel
   * @internal
   *   @history 2017-09-09 Original Version.
   */
  class StitchFunctor {
  public:
    StitchFunctor(Cube *inputCube, Cube *outputCube);
    ~StitchFunctor() {};
    AlphaCube alphaCube() const;
    Cube *outputCube() const;
    void operator()(Isis::Buffer &in) const;

  private:
    AlphaCube m_alphaCube;
    Cube *m_outputCube;
  };

  void tgocassisstitch(UserInterface &ui) {

    QMultiMap<QString, FileName> frameMap;

    try {
      // Open up the list of framelet files
      FileName frameletListFile = ui.GetFileName("FROMLIST");

      // Sort the framelets into frames based on start time
      frameMap = sortFramelets(frameletListFile);
    }
    catch (IException &e) {
      std::string msg = "Failed reading and sorting framelets into frames.";
      throw IException(e, IException::Unknown, msg, _FILEINFO_);
    }

    // read the optional cubename
    FileName frameletCubeFlag(ui.GetCubeName("CUBENAME"));
    QString frameletCubeName = frameletCubeFlag.expanded();

    // Stitch together the individual frames
    FileName outputPrefix(ui.GetCubeName("OUTPUTPREFIX"));
    FileName outputSuffix(ui.GetCubeName("OUTPUTSUFFIX"));
    QString outputPrefBaseName = outputPrefix.expanded();
    QString outputSuffBaseName = outputSuffix.expanded();
    QStringList frameKeys = frameMap.uniqueKeys();
    Progress stitchProgress;
    stitchProgress.SetText("Stitching Frames");
    stitchProgress.SetMaximumSteps(frameKeys.size());
    stitchProgress.CheckStatus();
    
    foreach(QString frameKey, frameKeys) {
      try {
        QString frameIdentifier = frameKey.split("/").last();
        FileName frameFileName;
        if (frameletCubeName != "") {
          frameFileName = FileName(outputPrefBaseName + "-" +
                               frameletCubeName +
                               outputSuffBaseName + ".cub");
        } else {
          frameFileName = FileName(outputPrefBaseName + "-" +
                                 frameIdentifier +
                                 outputSuffBaseName + ".cub");
        }
        stitchFrame( frameMap.values(frameKey), frameFileName );
        stitchProgress.CheckStatus();
      }
      catch (IException &e) {
        QString msg = "Failed stitch frame for observation ["
                      + frameKey + "].";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }
    }

    return;
  }


  /**
   * Go through a list of framelet cube files and sort them into frames based on
   * their timing.
   *
   * @param frameletList The file containing the list of framelet cubes
   *
   * @return @b QMap<QString,FileName> A multi-valued mapping from observation
   *                                   number for a frame to the framelet cube
   *                                   files in that frame.
   */
  QMap<QString, FileName> sortFramelets(FileName frameletListFile) {
    QMultiMap<QString, FileName> frameMap;

    ObservationNumberList frameletList(frameletListFile.expanded(), false);

    for (int i = 0; i < frameletList.size(); i++) {
      frameMap.insert( frameletList.observationNumber(i),
                            frameletList.fileName(i) );
    }

    return std::move(frameMap);
  }


  /**
   * Combine several framelet cubes into a single frame cube. The labels from the
   * first framelet will be propegated to the frame cube.
   *
   * @param frameletList A list of the framelet cubes to stitch together
   * @param frameFileName The file name of the output frame cube
   *
   * @internal
   *   @history 2018-02-15 Adam Goins - Modified stitchFrame to store the Archive
   *                           group for ingested framelets. Fixes #5333.
   */
  void stitchFrame(QList<FileName> frameletList, FileName frameFileName) {
    // Create the frame cube based on the first framelet cube
    Cube firstFrameletCube( frameletList.first() );
    AlphaCube firstAlphaCube(firstFrameletCube);
    Cube frameCube;
    frameCube.setDimensions( firstAlphaCube.AlphaSamples(), firstAlphaCube.AlphaLines(), 1 );
    frameCube.setPixelType( firstFrameletCube.pixelType() );
    frameCube.setByteOrder( firstFrameletCube.byteOrder() );
    frameCube.setBaseMultiplier( firstFrameletCube.base(), firstFrameletCube.multiplier() );
    frameCube.create( frameFileName.expanded() );

    // Setup the label for the new cube
    PvlGroup kernGroup = firstFrameletCube.group("Kernels");
    PvlGroup instGroup = firstFrameletCube.group("Instrument");
    PvlGroup bandBinGroup("BandBin");
    if ( instGroup.hasKeyword("Filter") ) {
      instGroup["Filter"].setValue("FULLCCD");
    }

    bandBinGroup += PvlKeyword("FilterName", "FULLCCD");

    // Setup Stitch group keywords
    PvlGroup stitchGroup("Stitch");
    stitchGroup += PvlKeyword("OriginalFilters");
    stitchGroup += PvlKeyword("FilterCenters");
    stitchGroup += PvlKeyword("FilterWidths");
    stitchGroup += PvlKeyword("FilterIkCodes");
    stitchGroup += PvlKeyword("FilterStartSamples");
    stitchGroup += PvlKeyword("FilterSamples");
    stitchGroup += PvlKeyword("FilterStartLines");
    stitchGroup += PvlKeyword("FilterLines");
    stitchGroup += PvlKeyword("FilterFileNames");

    // Propagate tables and spice
    Pvl &firstFrameletLabel = *firstFrameletCube.label();
    Pvl &frameCubeLabel = *frameCube.label();
    for(int i = 0; i < firstFrameletLabel.objects(); i++) {
      if(firstFrameletLabel.object(i).isNamed("Table")) {
        Isis::Blob table((QString)firstFrameletLabel.object(i)["Name"], firstFrameletLabel.object(i).name());
        firstFrameletCube.read(table);
        frameCube.write(table);
      }
    }
    if (firstFrameletLabel.hasObject("NaifKeywords")) {
      frameCubeLabel.addObject(firstFrameletLabel.findObject("NaifKeywords"));
    }

    // Close the first framelet cube because we are done with it for now
    firstFrameletCube.close();

    // Process each framelet cube
    foreach(FileName frameletFile, frameletList) {
      // Write the cube DNs to the frame cube
      ProcessByLine frameletProcess;
      frameletProcess.Progress()->DisableAutomaticDisplay();
      CubeAttributeInput inputAtts(frameletFile);
      Cube *frameletCube = frameletProcess.SetInputCube(frameletFile.expanded(), inputAtts);
      // Check for summing in the framelet cube
      // Eventually summing can be handled, but right now we don't know enough, so error
      PvlGroup frameletInst = frameletCube->group("Instrument");
      if ((int)frameletInst["SummingMode"] != 0) {
        QString msg = "Summing mode [" + (QString)frameletInst["SummingMode"]
                      + "] for framelet [" + frameletFile.expanded()
                      + "] is not supported.";
        throw IException(IException::User, msg, _FILEINFO_);
      }
      StitchFunctor stitchFunctor(frameletCube, &frameCube);
      frameletProcess.ProcessCubeInPlace(stitchFunctor);

      // Propagate framelet information and history to the frame cube
      PvlGroup frameletBandBin = frameletCube->group("BandBin");
      stitchGroup["OriginalFilters"] += frameletBandBin["FilterName"];
      stitchGroup["FilterCenters"]   += frameletBandBin["Center"];
      stitchGroup["FilterWidths"]    += frameletBandBin["Width"];
      stitchGroup["FilterIkCodes"]   += frameletBandBin["NaifIkCode"];

      PvlGroup archiveGroup = frameletCube->group("Archive");
      archiveGroup.setName("Archive" + QString(frameletBandBin["FilterName"]));
      frameCube.putGroup(archiveGroup);

      AlphaCube frameletAlphaCube(*frameletCube);
      stitchGroup["FilterStartSamples"] += toString(frameletAlphaCube.AlphaSample(0.0));
      stitchGroup["FilterSamples"]      += toString(frameletAlphaCube.BetaSamples());
      stitchGroup["FilterStartLines"]   += toString(frameletAlphaCube.AlphaLine(0.0));
      stitchGroup["FilterLines"]        += toString(frameletAlphaCube.BetaLines());

      PvlGroup frameletArchGroup = frameletCube->group("Archive");
      stitchGroup["FilterFileNames"]  += frameletArchGroup["FileName"];


      Pvl &frameletLabel = *frameletCube->label();
      for(int i = 0; i < frameletLabel.objects(); i++) {
        if( frameletLabel.object(i).isNamed("History") ) {
          Blob historyBlob((QString) frameletLabel.object(i)["Name"], "History" );
          frameletCube->read(historyBlob);
          frameCube.write(historyBlob);
        }
      }
    }

    // Finalize the frame cube label
    frameCube.putGroup(instGroup);
    frameCube.putGroup(kernGroup);
    frameCube.putGroup(bandBinGroup);
    frameCube.putGroup(stitchGroup);
  }


  /**
   * Construct a stitch functor for input and output cubes.
   *
   * @param inputCube A pointer to the input cube. The alpha cube will be
   *                  extracted from this cube.
   * @param outputCube The output stitched cube that will be written to.
   */
  StitchFunctor::StitchFunctor(Cube *inputCube, Cube *outputCube) :
    m_alphaCube(*inputCube), m_outputCube(outputCube) { }


  /**
   * Return the alpha cube for mapping the input framelet cube into the output
   * frame cube.
   *
   * @return @b AlphaCube The framelet cube's alpha cube.
   */
  AlphaCube StitchFunctor::alphaCube() const {
    return m_alphaCube;
  }


  /**
   * Return a pointer to the output frame cube.
   *
   * @return @b Cube* The output frame cube.
   */
  Cube *StitchFunctor::outputCube() const {
    return m_outputCube;
  }


  /**
   * Process method that maps a line from the input framelet cube to the output
   * frame cube.
   *
   * @param in A line of data from the input cube.
   */
  void StitchFunctor::operator()(Isis::Buffer &in) const {
    // Setup the line manager to write to the frame cube
    LineManager outputManager( *outputCube() );
    outputManager.SetLine( alphaCube().AlphaLine(in.Line()) );
    outputCube()->read(outputManager);

    // Copy the data into the line manager
    for (int i = 0; i < in.size(); i++) {
      outputManager[alphaCube().AlphaSample(i)] = in[i];
    }

    // Write the data out to the frame cube
    outputCube()->write(outputManager);
  }
}


