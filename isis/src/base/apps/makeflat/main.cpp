#include "Isis.h"

#include <vector>

#include "ProcessByLine.h"
#include "FileList.h"
#include "Histogram.h"
#include "LineManager.h"

using namespace Isis;
using namespace std;

// This method is used for creating the temp file on all camery types
void CreateTemporaryData(Buffer &in);

// This method is used for creating the final flat field on all camera types
void ProcessTemporaryData(Buffer &in);

// This method resets the current line scan camera temp data to NULL
void CreateNullData();

// This returns true if a file is excluded
bool Excluded(int fileNum);

// This returns true if a framelet is excluded and gives the standard deviation
bool Excluded(int fileNum, int frameletNum, double &stdev);

// This method populates the framelet exclusion list given
//   a cube. *Uses currImage.
bool CheckFramelets(QString progress, Cube &theCube);

// This method resets the global variables for another run
void ResetGlobals();

/**
 * This enum holds the supported camera types.
 *
 * The variable cameraType will be one of these values, which
 * will represent the current camera type.
 */
enum CameraType {
  LineScan,
  PushFrame,
  Framing
};

/**
 * The number of samples in the temp file
 * will always equal that of the input cube,
 * but the lines will vary. This variable stores
 * the number of lines in the temp file; it will
 * vary drastically depending on user-entered options
 */
BigInt tempFileLength;

/**
 * This will keep track of which files are excluded. The
 * int is the file number in the input list (0 = first file),
 * and the boolean is whether or not it's excluded (always true).
 * Files that arent excluded will not be added to this map in
 * order to increase efficiency.
 */
map<int, bool> excludedFiles;

/**
 * This will keep track of which framelets are excluded. The
 * pair<int,int> is the key, the first number being the file
 * in the input list (0 = first), the second being the framelet
 * in the file (0 = first). The double is the standard deviation
 * of the framelet. Framelets that arent excluded will not be added
 * to this map in order to increase efficiency.
 */
map<pair<int, int>, double> excludedFramelets;

/**
 * This object will be used to keep track of exclusions found during
 * the creating of the temporary file. The main method will add an object
 * onto this list for every pass, with basic keywords such as filename, and
 * the CreateTemporaryData will add a group for every exclusion found. The main
 * method will trim objects with no exclusion groups.
 */
vector<PvlObject> excludedDetails;

/**
 * This is used for the line scan camera temporary data. Because
 * the line scan temporary data creation is done by frame column,
 * the data in the previous columns of data needs to be stored until
 * a frame is complete. The outputTmpAverages keeps track of the
 * averages of each column.
 */
vector<double> outputTmpAverages;

/**
 * This is closely tied with outputTmpAverages, except it keeps track
 * of valid pixel counts used in the averages.
 */
vector<double> outputTmpCounts;

/**
 * This keeps track of the averages of the input framelets for framing
 * and push frame cameras. For framing cameras, the average is in framelet
 * zero. This is necessary in order to normalize the temporary data (second pass)
 * with the statistics collected on the first pass.
 */
vector< vector<double> > inputFrameletAverages;

/**
 * This keeps the statistics for each frame in a line scan camera. This is collected
 * during the second pass of the data because in a line scan camera, all of the frame
 * DNs are read before being written to the temporary file (which is possible because
 * only one line of data is written per frame).
 */
Statistics inputFrameStats;

/**
 * This is the standard deviation tolerance. For framing cameras, this is the maximum
 * standard deviation of an image before it is excluded. For push frame cameras, this is
 * the maximum standard deviation of a framelet. For framing cameras, this is the maximum
 * standard deviation of an input image. For line scan cameras, this is the maximum
 * standard deviation of a frame.
 */
double maxStdev;

/**
 * This variable is used in two ways. During pass 2, where the temporary file is being
 * created, this is the temporary file. During pass 3, where the final output is being
 * created, this is the final output.
 */
Cube *ocube = NULL;

/**
 * This is a line manager that is always associated with ocube.
 */
LineManager *oLineMgr = NULL;

/**
 * This is the number of samples in the input images, temporary file,and final output
 * cubes (one in the same!).
 */
int numOutputSamples;

/**
 * This is the number of lines per frame. For line scans, this is how many lines
 * that are combined into one section for the standard deviation tolerance. For
 * push frame cameras, this is the number of lines per framelet. For framing
 * cameras, this ends up being the height of the image (1 framelet).
 */
int numFrameLines;

/**
 * This keeps track of special pixel counts for pass 3 of line scan cameras.
 * This is the count of total valid input DNs for each column of data that is
 * in the temporary data (sum in the line direction of band 2 of the temporary
 * file). This is used for the final weighted average calculation.
 */
vector<BigInt> numInputDns;

/**
 * This keeps track of which camera type we're processing. See CameraType.
 */
CameraType cameraType;

/**
 * Since in framing and push frame cameras we don't know if the temporary cube has been
 * initialized, and the operations all depend on both the input data and current
 * temporary data, this boolean keeps track of whether or not the temporary cube has been
 * initialized.
 */
bool cubeInitialized;

/**
 * This is the image that is currently being processed. This is necessary in order for
 * the processing methods to determine exclusions and pre-calculated statistics of each
 * image.
 */
int currImage;

/**
 * This is the main method. Makeflat runs in three steps:
 *
 * 1) Calculate statistics
 *   - For all cameras, this checks for one band and matching
 *       sample counts.
 *   - For framing cameras, this checks the standard deviation of
 *       the images and records the averages of each image
 *   - For push frame cameras, this calls CheckFramelets for each
 *       image.
 *
 * 2) Create the temporary file, collect more detailed statistics
 *   - For all cameras, this generates the temporary file and calculates
 *       the final exclusion list
 *   - For framing/push frame cameras, the temporary file is
 *       2 bands, where the first is a sum of DNs from each image/framelet
 *       and the second band is a count of valid DNs that went into each sum
 *
 *  3) Create the final flat field file
 *   - For all cameras, this processes the temporary file to create the final flat
 *       field file.
 */
void IsisMain() {
  // Initialize variables
  ResetGlobals();

  UserInterface &ui = Application::GetUserInterface();
  maxStdev = ui.GetDouble("STDEVTOL");

  if(ui.GetString("IMAGETYPE") == "FRAMING") {
    cameraType = Framing;

    // framing cameras need to figure this out automatically
    //   during step 1
    numFrameLines = -1;
  }
  else if(ui.GetString("IMAGETYPE") == "LINESCAN") {
    cameraType = LineScan;
    numFrameLines = ui.GetInteger("NUMLINES");
  }
  else {
    cameraType = PushFrame;
    numFrameLines = ui.GetInteger("FRAMELETHEIGHT");
  }

  FileList inList(FileName(ui.GetFileName("FROMLIST")));
  Progress progress;

  tempFileLength = 0;
  numOutputSamples = 0;

  /**
   * Line scan progress is based on the input list, whereas
   * the other cameras take much longer and are based on the
   * images themselves. Prepare the progress if we're doing
   * line scan.
   */
  if(cameraType == LineScan) {
    progress.SetText("Calculating Number of Image Lines");
    progress.SetMaximumSteps(inList.size());
    progress.CheckStatus();
  }

  /**
   *  For a push frame camera, the temp file is one framelet.
   *   Technically this is the same for the framing, but we
   *   don't know the height of a framelet yet.
   */
  if(cameraType == PushFrame) {
    tempFileLength = numFrameLines;
  }

  /**
   * Start pass 1, use global currImage so that methods called
   *   know the image we're processing.
   */
  for(currImage = 0; currImage < inList.size(); currImage++) {
    /**
     * Read the current cube into memory
     */
    Cube tmp;
    tmp.open(inList[currImage].toString());

    /**
     * If we haven't determined how many samples the output
     *   should have, we can do so now
     */
    if(numOutputSamples == 0 && tmp.bandCount() == 1) {
      numOutputSamples = tmp.sampleCount();
    }

    /**
     * Try and validate the image, quick tests first!
     *
     * (imageValid &= means imageValid = imageValid && ...)
     */
    bool imageValid = true;

    // Only single band images are acceptable
    imageValid &= (tmp.bandCount() == 1);

    // Sample sizes must always match
    imageValid &= (numOutputSamples == tmp.sampleCount());

    // For push frame cameras, there must be valid all framelets
    if(cameraType == PushFrame) {
      imageValid &= (tmp.lineCount() % numFrameLines == 0);
    }

    // For framing cameras, we need to figure out the size...
    //    setTempFileLength is used to revert if the file
    //    is decided to be invalid
    bool setTempFileLength = false;
    if(cameraType == Framing) {
      if(tempFileLength == 0 && imageValid) {
        tempFileLength = tmp.lineCount();
        numFrameLines = tempFileLength;
        setTempFileLength = true;
      }

      imageValid &= (tempFileLength == tmp.lineCount());
    }

    // Statistics are necessary at this point for push frame and framing cameras
    //   because the framing camera standard deviation tolerance is based on
    //   entire images, and push frame framelet exclusion stats can not be collected
    //   during pass 2 cleanly
    if((cameraType == Framing || cameraType == PushFrame) && imageValid) {
      QString prog = "Calculating Standard Deviation " + toString((int)currImage + 1) + "/";
      prog += toString((int)inList.size()) + " (" + inList[currImage].name() + ")";

      if(cameraType == Framing) {
        Statistics *stats = tmp.statistics(1, prog);
        imageValid &= !IsSpecial(stats->StandardDeviation());
        imageValid &= !IsSpecial(stats->Average());
        imageValid &= stats->StandardDeviation() <= maxStdev;

        vector<double> fileStats;
        fileStats.push_back(stats->Average());
        inputFrameletAverages.push_back(fileStats);

        delete stats;
      }
      else if(cameraType == PushFrame) {
        imageValid &= CheckFramelets(prog, tmp);
      }

      if(setTempFileLength && !imageValid) {
        tempFileLength = 0;
      }
    }

    // The line scan camera needs to actually count the number of lines in each image to know
    //   how many total frames there are before beginning pass 2.
    if(imageValid && (cameraType == LineScan)) {
      int lines = (tmp.lineCount() / numFrameLines);

      // partial frame?
      if(tmp.lineCount() % numFrameLines != 0) {
        lines ++;
      }

      tempFileLength += lines;
    }
    else if(!imageValid) {
      excludedFiles.insert(pair<int, bool>(currImage, true));
    }

    tmp.close();

    if(cameraType == LineScan) {
      progress.CheckStatus();
    }
  }

  /**
   * If the number of output samples could not be determined, we never
   *   found a legitimate cube.
   */
  if(numOutputSamples <= 0) {
    std::string msg = "No valid input cubes were found";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  /**
   * If theres no temp file length, which is based off of valid data in
   *   the input cubes, then we havent found any valid data.
   */
  if(tempFileLength <= 0) {
    std::string msg = "No valid input data was found";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  /**
   * ocube is now the temporary file (for pass 2).
   */
  ocube = new Cube();
  ocube->setDimensions(numOutputSamples, tempFileLength, 2);
  PvlGroup &prefs = Preference::Preferences().findGroup("DataDirectory", Pvl::Traverse);
  QString outTmpName = (QString)prefs["Temporary"][0] + "/";
  outTmpName += FileName(ui.GetCubeName("TO")).baseName() + ".tmp.cub";
  ocube->create(outTmpName);
  oLineMgr = new LineManager(*ocube);
  oLineMgr->SetLine(1);

  ProcessByBrick p;

  if(cameraType == LineScan) {
    outputTmpAverages.resize(numOutputSamples);
    outputTmpCounts.resize(numOutputSamples);
    numInputDns.resize(numOutputSamples);
  }

  cubeInitialized = false;
  for(currImage = 0; currImage < inList.size(); currImage++) {
    if(Excluded(currImage)) {
      continue;
    }

    PvlObject currFile("Exclusions");
    currFile += PvlKeyword("FileName", inList[currImage].toString());
    currFile += PvlKeyword("Tolerance", toString(maxStdev));

    if(cameraType == LineScan) {
      currFile += PvlKeyword("FrameLines", toString(numFrameLines));
    }
    else if(cameraType == PushFrame) {
      currFile += PvlKeyword("FrameletLines", toString(numFrameLines));
    }

    excludedDetails.push_back(currFile);

    CubeAttributeInput inAtt;

    // This needs to be set constantly because ClearInputCubes
    //   seems to be removing the input brick size.
    if(cameraType == LineScan) {
      p.SetBrickSize(1, numFrameLines, 1);
    }
    else if(cameraType == Framing || cameraType == PushFrame) {
      p.SetBrickSize(numOutputSamples, 1, 1);
    }

    p.SetInputCube(inList[currImage].toString(), inAtt);
    QString progText = "Calculating Averages " + toString((int)currImage + 1);
    progText += "/" + toString((int)inList.size());
    progText += " (" + inList[currImage].name() + ")";
    p.Progress()->SetText(progText);

    p.StartProcess(CreateTemporaryData);
    p.EndProcess();
    p.ClearInputCubes();

    if(excludedDetails[excludedDetails.size()-1].groups() == 0) {
      excludedDetails.resize(excludedDetails.size() - 1);
    }
  }

  /**
   * Pass 2 completed. The processing methods were responsible for writing
   * the entire temporary cube.
   */
  if(oLineMgr) {
    delete oLineMgr;
    oLineMgr = NULL;
  }

  if(ocube) {
    ocube->close();
    delete ocube;
  }

  /**
   * ocube is now the final output
   */
  ocube = new Cube();

  if(cameraType == LineScan) {
    ocube->setDimensions(numOutputSamples, 1, 1);
  }
  else if(cameraType == Framing || cameraType == PushFrame) {
    ocube->setDimensions(numOutputSamples, tempFileLength, 1);
  }

  ocube->create(FileName(ui.GetCubeName("TO")).expanded());
  oLineMgr = new LineManager(*ocube);
  oLineMgr->SetLine(1);

  // We now have the necessary temp file, let's go ahead and combine it into
  //   the final output!
  p.SetInputBrickSize(numOutputSamples, 1, 2);
  p.SetOutputBrickSize(numOutputSamples, 1, 1);

  cubeInitialized = false;
  CubeAttributeInput inAtt;
  p.Progress()->SetText("Calculating Final Flat Field");
  p.SetInputCube(outTmpName, inAtt);
  p.StartProcess(ProcessTemporaryData);
  p.EndProcess();

  if(cameraType == LineScan) {
    ocube->write(*oLineMgr);
  }

  if(oLineMgr) {
    delete oLineMgr;
    oLineMgr = NULL;
  }

  if(ocube) {
    ocube->close();
    delete ocube;
    ocube = NULL;
  }

  /**
   * Build a list of excluded files
   */
  PvlGroup excludedFiles("ExcludedFiles");
  for(currImage = 0; currImage < inList.size(); currImage++) {
    if(Excluded(currImage)) {
      excludedFiles += PvlKeyword("File", inList[currImage].original());
    }
  }

  // log the results
  Application::Log(excludedFiles);

  if(ui.WasEntered("EXCLUDE")) {
    Pvl excludeFile;

    // Find excluded files
    excludeFile.addGroup(excludedFiles);

    for(unsigned int i = 0; i < excludedDetails.size(); i++) {
      excludeFile.addObject(excludedDetails[i]);
    }

    excludeFile.write(FileName(ui.GetFileName("EXCLUDE")).expanded());
  }

  remove(outTmpName.toLatin1().data());

  // Clean up settings
  ResetGlobals();
}

/**
 * This method initializes all of the global variables
 */
void ResetGlobals() {
  tempFileLength = 0;
  numOutputSamples = 0;
  numFrameLines = 0;
  currImage = 0;
  maxStdev = 0.0;
  excludedFiles.clear();
  excludedDetails.clear();
  outputTmpAverages.clear();
  outputTmpCounts.clear();
  inputFrameletAverages.clear();
  inputFrameStats.Reset();
  numInputDns.clear();
  excludedFramelets.clear();

  cubeInitialized = false;
  cameraType = LineScan;

  if(ocube) {
    delete ocube;
    ocube = NULL;
  }

  if(oLineMgr) {
    delete oLineMgr;
    oLineMgr = NULL;
  }
}

/**
 * This method performs pass1 on one image. It analyzes each framelet's
 * statistics and populates the necessary global variable.
 *
 * @param progress Progress message
 * @param theCube Current cube that needs processing
 *
 * @return bool True if the file contains a valid framelet
 */
bool CheckFramelets(QString progress, Cube &theCube) {
  bool foundValidFramelet = false;
  LineManager mgr(theCube);
  Progress prog;
  prog.SetText(progress);
  prog.SetMaximumSteps(theCube.lineCount());
  prog.CheckStatus();

  vector<double> frameletAvgs;
  // We need to store off the framelet information, because if no good
  //   framelets were found then no data should be added to the
  //   global variable for framelets, just files.
  vector< pair<int, double> > excludedFrameletsTmp;
  Statistics frameletStats;

  for(int line = 1; line <= theCube.lineCount(); line++) {
    if((line - 1) % numFrameLines == 0) {
      frameletStats.Reset();
    }

    mgr.SetLine(line);
    theCube.read(mgr);
    frameletStats.AddData(mgr.DoubleBuffer(), mgr.size());

    if((line - 1) % numFrameLines == numFrameLines - 1) {
      if(IsSpecial(frameletStats.StandardDeviation()) ||
          frameletStats.StandardDeviation() > maxStdev) {
        excludedFrameletsTmp.push_back(
          pair<int, double>((line - 1) / numFrameLines, frameletStats.StandardDeviation())
        );
      }
      else {
        foundValidFramelet = true;
      }

      frameletAvgs.push_back(frameletStats.Average());
    }

    prog.CheckStatus();
  }

  inputFrameletAverages.push_back(frameletAvgs);

  if(foundValidFramelet) {
    for(unsigned int i = 0; i < excludedFrameletsTmp.size(); i++) {
      excludedFramelets.insert(pair< pair<int, int>, double>(
                                 pair<int, int>(currImage, excludedFrameletsTmp[i].first),
                                 excludedFrameletsTmp[i].second
                               )
                              );
    }

  }

  return foundValidFramelet;
}

/**
 * This method is the pass 2 processing routine. A ProcessByBrick
 * will call this method for sets of data (depending on the camera
 * type) and this method is responsible for writing the entire output
 * temporary cube.
 *
 * @param in Input raw image data, not including excluded files
 */
void CreateTemporaryData(Buffer &in) {
  /**
   * Line scan cameras process by frame columns.
   */
  if(cameraType == LineScan) {
    // The statistics of every column of data need to be known
    //   before we can write to the temp file. Gather stats for this
    //   column.
    Statistics inputColStats;

    for(int i = 0; i < in.size(); i++) {
      inputColStats.AddData(in[i]);

      // We'll also need the stats for the entire frame in order to
      //   normalize and in order to decide whether or not we want
      //   to toss out the frame
      inputFrameStats.AddData(in[i]);
    }

    // Store off the column stats
    outputTmpAverages[in.Sample()-1] = inputColStats.Average();
    outputTmpCounts[in.Sample()-1] = inputColStats.ValidPixels();

    // Test if this is the last column and we've got all of our stats
    if(in.Sample() == numOutputSamples) {
      // Decide if we want this data
      if(IsSpecial(inputFrameStats.StandardDeviation()) ||
          inputFrameStats.StandardDeviation() > maxStdev) {
        // We don't want this data...
        //   CreateNullData is a helper method for this case that
        //   nulls out the stats
        CreateNullData();

        // Record the exclusion
        PvlGroup currExclusion("ExcludedLines");
        currExclusion += PvlKeyword("FrameStartLine", toString(in.Line()));
        currExclusion += PvlKeyword("ValidPixels", toString(inputFrameStats.ValidPixels()));

        if(!IsSpecial(inputFrameStats.StandardDeviation()))
          currExclusion += PvlKeyword("StandardDeviation", toString(inputFrameStats.StandardDeviation()));
        else
          currExclusion += PvlKeyword("StandardDeviation", "N/A");

        excludedDetails[excludedDetails.size()-1].addGroup(currExclusion);
      }

      // Let's write our data... CreateNullData took care of nulls for us
      // Band 1 is our normalized average
      oLineMgr->SetLine(oLineMgr->Line(), 1);
      for(int i = 0; i < (int)outputTmpAverages.size(); i++) {
        if(!IsSpecial(outputTmpAverages[i])) {
          (*oLineMgr)[i] = outputTmpAverages[i] / inputFrameStats.Average();
        }
        else {
          (*oLineMgr)[i] = Isis::Null;
        }
      }

      ocube->write(*oLineMgr);
      oLineMgr->SetLine(oLineMgr->Line(), 2);

      // band 2 is our valid dn counts
      for(int i = 0; i < (int)outputTmpCounts.size(); i++) {
        (*oLineMgr)[i] = outputTmpCounts[i];
        numInputDns[i] += (int)(outputTmpCounts[i] + 0.5);
      }

      ocube->write(*oLineMgr);
      (*oLineMgr) ++;

      inputFrameStats.Reset();
    }
  }
  else if(cameraType == Framing || cameraType == PushFrame) {
    // Framing cameras and push frames are treated identically;
    //   the framelet size for a framelet in the framing camera
    //   is the entire image!
    int framelet = (in.Line() - 1) / numFrameLines;
    double stdev;
    bool excluded = Excluded(currImage, framelet, stdev);

    if(excluded && ((in.Line() - 1) % numFrameLines == 0)) {
      PvlGroup currExclusion("ExcludedFramelet");
      currExclusion += PvlKeyword("FrameletStartLine", toString(in.Line()));
      currExclusion += PvlKeyword("FrameletNumber", toString((in.Line() - 1) / numFrameLines));

      if(!IsSpecial(stdev)) {
        currExclusion += PvlKeyword("StandardDeviation",
                                    toString(stdev));
      }
      else {
        currExclusion += PvlKeyword("StandardDeviation",
                                    "N/A");
      }

      excludedDetails[excludedDetails.size()-1].addGroup(currExclusion);
    }

    // Since this is a line by line iterative process, we need to get the current
    //   data in the temp file
    oLineMgr->SetLine(((in.Line() - 1) % numFrameLines) + 1, 1);

    if(!excluded || !cubeInitialized) {
      ocube->read(*oLineMgr);
    }

    if(!cubeInitialized) {
      for(int i = 0; i < oLineMgr->size(); i++) {
        (*oLineMgr)[i] = Isis::Null;
      }
    }

    vector<bool> isValidData;

    if(!excluded || !cubeInitialized) {
      isValidData.resize(in.size());

      for(int samp = 0; samp < in.size(); samp++) {
        if(IsSpecial((*oLineMgr)[samp]) && !IsSpecial(in[samp])) {
          (*oLineMgr)[samp] = 0.0;
        }

        if(!IsSpecial(in[samp])) {
          isValidData[samp] = true;
          (*oLineMgr)[samp] += in[samp] / inputFrameletAverages[currImage][framelet];
        }
        else {
          isValidData[samp] = false;
        }
      }
    }

    if(!excluded || !cubeInitialized) {
      ocube->write(*oLineMgr);
    }

    oLineMgr->SetLine(oLineMgr->Line(), 2);

    if(!excluded || !cubeInitialized) {
      ocube->read(*oLineMgr);
    }

    if(!cubeInitialized) {
      for(int i = 0; i < oLineMgr->size(); i++) {
        (*oLineMgr)[i] = Isis::Null;
      }

      if(ocube->lineCount() == oLineMgr->Line())
        cubeInitialized = true;
    }

    if(!excluded || !cubeInitialized) {
      for(int i = 0; i < (int)isValidData.size(); i++) {
        if(IsSpecial((*oLineMgr)[i])) {
          (*oLineMgr)[i] = 0.0;
        }

        if(isValidData[i]) {
          (*oLineMgr)[i] ++;
        }
      }
    }

    if(!excluded || !cubeInitialized) {
      ocube->write(*oLineMgr);
    }
  }
}

/**
 * This method is the pass 3 processing routine. This compresses
 * the temporary file into the final flat file. ocube is the final
 * out file, the Buffer argument is the temp file.
 *
 * @param in Line of data in the temporary file generated in pass 2
 */
void ProcessTemporaryData(Buffer &in) {
  if(!cubeInitialized) {
    for(int i = 0; i < (*oLineMgr).size(); i++) {
      (*oLineMgr)[i] = Isis::Null;
    }
  }

  if(cameraType == LineScan) {
    cubeInitialized = true;

    for(int i = 0; i < (*oLineMgr).size(); i++) {
      int avgIndex = in.Index(i + 1, in.Line(), 1);
      int validIndex = in.Index(i + 1, in.Line(), 2);

      if(!IsSpecial(in[avgIndex]) && !IsSpecial(in[validIndex])) {
        if(IsSpecial((*oLineMgr)[i])) {
          (*oLineMgr)[i] = 0.0;
        }

        (*oLineMgr)[i] += in[avgIndex] * (int)(in[validIndex] + 0.5) / (double)numInputDns[i];
      }
    }
  }
  else if(cameraType == Framing || cameraType == PushFrame) {
    oLineMgr->SetLine(((in.Line() - 1) % numFrameLines) + 1);

    for(int i = 0; i < (*oLineMgr).size(); i++) {
      int sumIndex = in.Index(i + 1, in.Line(), 1);
      int validIndex = in.Index(i + 1, in.Line(), 2);

      if(!IsSpecial(in[sumIndex]) && !IsSpecial(in[validIndex])) {
        (*oLineMgr)[i] = in[sumIndex] / in[validIndex];
      }
    }

    ocube->write(*oLineMgr);

    if(ocube->lineCount() == oLineMgr->Line())
      cubeInitialized = true;
  }
}

/**
 * This is a helper method for line scan cameras. It removes the
 * statistics from the global variables that contain the current
 * frame's data.
 */
void CreateNullData() {
  for(int i = 0; i < (int)outputTmpAverages.size(); i++) {
    outputTmpAverages[i] = Isis::Null;
    outputTmpCounts[i] = 0;
  }
}

/**
 * This method tests if a file was excluded.
 *
 * @param fileNum Zero-indexed file from the FROMLIST (often just currFile)
 *
 * @return bool True if file was excluded
 */
bool Excluded(int fileNum) {
  map<int, bool>::iterator it = excludedFiles.find(fileNum);

  if(it == excludedFiles.end()) {
    return false;
  }
  else {
    return it->second;
  }
}

/**
 * This method tests if a framelet in a file was excluded.
 *
 * @param fileNum Zero-indexed file from the FROMLIST (often just currFile)
 * @param frameletNum Framelet to test in the current file
 * @param stdev This will be set to the standard deviation of the framelet
 *                if the framelet was excluded
 *
 * @return bool True if the framelet was excluded
 */
bool Excluded(int fileNum, int frameletNum, double &stdev) {
  if(Excluded(fileNum)) return true;

  map< pair<int, int>, double>::iterator it = excludedFramelets.find(pair<int, int>(fileNum, frameletNum));

  if(it == excludedFramelets.end()) {
    return false;
  }
  else {
    stdev = it->second;
    return true;
  }
}
