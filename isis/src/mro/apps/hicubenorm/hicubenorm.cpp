/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

// system include files go first
#include <QString>
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>

// Isis specific include files go next
#include "ProcessByTile.h"
#include "ProcessByLine.h"
#include "SpecialPixel.h"
#include "IException.h"
#include "Pvl.h"
#include "Statistics.h"
#include "VecFilter.h"

#include "hicubenorm.h"

using namespace std;

namespace Isis {
    // These global vectors are used to keep track of info for columns
    // of image data.  For example, a 100 sample x 200 line x 2 band cube will
    // have a vectors of 200 columns (100 samples x 2 bands).
    vector<double> stddev;
    vector<int> validpixels;
    vector<double> minimum;
    vector<double> maximum;
    vector<int> band;
    vector<int> element;
    vector<double> median;
    vector<double> average;
    vector<double> normalizer;

    // Size of the cube
    int totalLines;
    int totalSamples;

    enum Mode {SUBTRACT, DIVIDE};

    // function prototypes
    void getStats            (Buffer &in);
    void multiply            (Buffer &in, Buffer &out);
    void subtract            (Buffer &in, Buffer &out);
    void pvlOut              (const QString &pv);
    void tableOut            (const QString &pv);
    void PVLIn               (const Isis::FileName &filename);
    void tableIn             (const Isis::FileName &filename);
    void keepSame            (int &totalBands, int &rowcol, Mode mode);
    void filterStats         (vector<double> &filter, int &filtsize, bool &pause_crop,int &channel);
    void CorrectCubenormStats(int piFilterSize, bool pbPauseCrop, int piChannelNum, QString psMode);

    void hicubenorm(UserInterface &ui) {
        Cube *cube = new Cube(ui.GetCubeName("FROM").toStdString(), "r");
        hicubenorm(cube, ui);
    }

    void hicubenorm(Cube *icube, UserInterface &ui) {
        vector<double> filter;
        int rowcol;                    // how many rows or cols per band
        bool normalizeUsingAverage;    // mult/sub using average or median?
        int totalBands;

        // Used for filtering the initial cubenorm averages and median values
        int filtsize;
        bool pause_crop;
        int channel;
        // ERROR CHECK:  The user must specify at least the TO or STATS
        // parameters.
        if(!(ui.WasEntered("TO")) && !(ui.WasEntered("STATS"))) {
            std::string msg = "User must specify a TO and/or STATS file.";
            throw IException(IException::User, msg, _FILEINFO_);
        }

        // We will be processing by tile.
        ProcessByTile p;

        p.SetInputCube(icube);

        // Setup the input cube;
        // Obtain information from the input file
        totalSamples = icube->sampleCount();
        totalLines   = icube->lineCount();
        totalBands   = icube->bandCount();

        channel = icube->group("Instrument")["ChannelNumber"];

        // Cubenorm New Version Flag
        bool bNewVersion = ui.GetBoolean("NEW_VERSION");

        // Setup the tile size for columnar processing
        p.SetTileSize(1, totalLines);
        rowcol = totalSamples;
        // Gather statistics from the configured source
        if(!bNewVersion && ui.GetString("STATSOURCE") == "CUBE") {
            p.StartProcess(getStats);
        }
        else if(ui.GetString("STATSOURCE") == "TABLE") {
            tableIn(ui.GetFileName("FROMSTATS").toStdString());
        }
        else {
            PVLIn(ui.GetFileName("FROMSTATS").toStdString());
        }
        // Check to make sure the first vector has as many elements as the last
        // vector, and that there is a vector element for each row/col
        if(!bNewVersion && band.size() != (unsigned int)(rowcol * totalBands)) {
            std::string message = "You have entered an invalid input file " +
                            ui.GetFileName("FROMSTATS").toStdString();
            throw IException(IException::Io, message, _FILEINFO_);
        }

        // Get the information needed to filter the statistics
        filtsize   = ui.GetInteger("FILTER");
        pause_crop = ui.GetBoolean("PAUSECROP");
        if(bNewVersion) {
            CorrectCubenormStats(filtsize, pause_crop, channel, ui.GetString("HIGHPASS_MODE"));
        }
        else {
            // Filter the column averages
            filter = average;
            filterStats(filter, filtsize, pause_crop, channel);
            average = filter;

            // Filter the column medians
            filter = median;
            filterStats(filter, filtsize, pause_crop, channel);
            median = filter;
        }
        // If a STATS file was specified then create statistics file
        if(ui.WasEntered("STATS")) {
            QString op = ui.GetString("FORMAT");
            if(op == "PVL"){
            pvlOut(ui.GetFileName("STATS"));
            }
            if(op == "TABLE"){
            tableOut(ui.GetFileName("STATS"));
            }
        }
        // Update the statistics vectors before creating the output
        // file. Now get the statistics for each column
        normalizeUsingAverage = ui.GetString("NORMALIZER") == "AVERAGE";

        if(normalizeUsingAverage) {
            normalizer = average;
        }
        else {
            normalizer = median;
        }

        // If an output file was specified then normalize the cube
        if(ui.WasEntered("TO")) {
            // Before creating a normalized cube check to see if there
            // are any column averages less than or equal to zero.
            if(ui.GetString("MODE") == "MULTIPLY") {
            for(unsigned int i = 0; i < band.size(); i++) {
                if(IsValidPixel(normalizer[i]) && normalizer[i] <= 0.0) {
                std::string msg = "Cube file can not be normalized with [MULTIPLY] ";
                msg += "option, some column averages <= 0.0";
                throw IException(IException::User, msg, _FILEINFO_);
                }
            }
            }

            Isis::CubeAttributeOutput atts = ui.GetOutputAttribute("TO");
            FileName outFileName = ui.GetCubeName("TO").toStdString();

            // Setup the output file and apply the coefficients by either
            // subtracting or multipling them
            p.SetOutputCube(QString::fromStdString(outFileName.expanded()), atts, totalSamples, totalLines, totalBands);
            // Should we preserve the average/median of the input image???
            if(ui.GetBoolean("PRESERVE")) {
            if(ui.GetString("MODE") == "SUBTRACT") {
                keepSame(totalBands, rowcol, SUBTRACT);
            }
            else {
                keepSame(totalBands, rowcol, DIVIDE);
            }
            }

            // Process based on the mode
            if(ui.GetString("MODE") == "SUBTRACT") {
                p.StartProcess(subtract);
            }
            else {
                p.StartProcess(multiply);
            }
        }

        // Cleanup
        p.EndProcess();
        stddev.clear();
        validpixels.clear();
        minimum.clear();
        maximum.clear();
        band.clear();
        element.clear();
        median.clear();
        average.clear();
        normalizer.clear();
        filter.clear();
    }

    //**********************************************************
    // DOUSER - Get statistics on a column or row of pixels
    //**********************************************************
    void getStats(Buffer &in) {
    Statistics stats;
    stats.AddData(in.DoubleBuffer(), in.size());

    band.push_back(in.Band());
    element.push_back(in.Sample());

    // Sort the input buffer
    vector<double> pixels;
    for(int i = 0; i < in.size(); i++) {
        if(IsValidPixel(in[i])) pixels.push_back(in[i]);
    }
    sort(pixels.begin(), pixels.end());

    // Now obtain the median value and store in the median vector
    int size = pixels.size();
    if(size != 0) {
        int med = size / 2;
        if(size % 2 == 0) {
            median.push_back((pixels[med-1] + pixels[med]) / 2.0);
        }
        else {
            median.push_back(pixels[med]);
        }
    }
    else {
        median.push_back(Isis::Null);
    }

    // Store the statistics in the appropriate vectors
    average.push_back(stats.Average());
    stddev.push_back(stats.StandardDeviation());
    validpixels.push_back(stats.ValidPixels());
    minimum.push_back(stats.Minimum());
    maximum.push_back(stats.Maximum());
    }

    //********************************************************
    // Create PVL output of statistics
    //*******************************************************
    void pvlOut(const QString &StatFile) {
    PvlGroup results("Results");
    for(unsigned int i = 0; i < band.size(); i++) {
        results += PvlKeyword("Band", std::to_string(band[i]));
        results += PvlKeyword("RowCol", std::to_string(element[i]));
        results += PvlKeyword("ValidPixels", std::to_string(validpixels[i]));
        if(validpixels[i] > 0) {
        results += PvlKeyword("Mean", std::to_string(average[i]));
        results += PvlKeyword("Median", std::to_string(median[i]));
        results += PvlKeyword("Std", std::to_string(stddev[i]));
        results += PvlKeyword("Minimum", std::to_string(minimum[i]));
        results += PvlKeyword("Maximum", std::to_string(maximum[i]));
        }
        else {
        results += PvlKeyword("Mean", std::to_string(0.0));
        results += PvlKeyword("Median", std::to_string(0.0));
        results += PvlKeyword("Std", std::to_string(0.0));
        results += PvlKeyword("Minimum", std::to_string(0.0));
        results += PvlKeyword("Maximum", std::to_string(0.0));
        }
    }

    Pvl t;
    t.addGroup(results);
    t.write(StatFile.toStdString());
    }

    //********************************************************
    // Create Tabular output of statistics
    //*******************************************************
    void tableOut(const QString &StatFile) {
    // Open output file
    // TODO check status and throw error
    ofstream out;
    out.open(StatFile.toLatin1().data(), std::ios::out);

    // Output a header
    out << std::setw(8)  << "Band";
    out << std::setw(8)  << "RowCol";
    out << std::setw(15) << "ValidPoints";
    out << std::setw(15) << "Average";
    out << std::setw(15) << "Median";
    out << std::setw(15) << "StdDev";
    out << std::setw(15) << "Minimum";
    out << std::setw(15) << "Maximum";
    out << endl;

    // Print out the table results
    for(unsigned int i = 0; i < band.size(); i++) {
        out << std::setw(8)  << band[i];
        out << std::setw(8)  << element[i];
        out << std::setw(15) << validpixels[i];
        if(validpixels[i] > 0) {
        out << std::setw(15) << average[i];
        out << std::setw(15) << median[i];
        //Make sure the table's SD is 0 for RowCols with 1 or less valid pixels
        if(validpixels[i] > 1) {
            out << std::setw(15) << stddev[i];
        }
        else {
            out << std::setw(15) << 0;
        }
        out << std::setw(15) << minimum[i];
        out << std::setw(15) << maximum[i];
        }
        else {
        out << std::setw(15) << 0;
        out << std::setw(15) << 0;
        out << std::setw(15) << 0;
        out << std::setw(15) << 0;
        out << std::setw(15) << 0;
        }
        out << endl;
    }
    out.close();
    }

    //********************************************************
    // Gather statistics from a PVL input file
    //*******************************************************
    void PVLIn(const Isis::FileName &filename) {
    Pvl pvlFileIn;
    pvlFileIn.read(filename.expanded());
    PvlGroup results = pvlFileIn.findGroup("Results");
    PvlObject::PvlKeywordIterator itr = results.begin();

    while(itr != results.end()) {
        band.push_back(std::stoi((*itr)[0]));
        itr++;
        element.push_back(std::stoi((*itr)[0]));
        itr++;
        validpixels.push_back(std::stoi((*itr)[0]));
        itr++;
        average.push_back(std::stod((*itr)[0]));
        itr++;
        median.push_back(std::stod((*itr)[0]));
        itr++;
        stddev.push_back(std::stod((*itr)[0]));
        itr++;
        minimum.push_back(std::stod((*itr)[0]));
        itr++;
        maximum.push_back(std::stod((*itr)[0]));
        itr++;
    }
    }

    //********************************************************
    // Gather statistics from a table input file
    //*******************************************************
    void tableIn(const Isis::FileName &filename) {
    ifstream in;
    QString expanded(QString::fromStdString(filename.expanded()));
    in.open(expanded.toLatin1().data(), std::ios::in);


    if(!in) {
        std::string message = "Error opening " + filename.expanded();
        throw IException(IException::Io, message, _FILEINFO_);
    }

    //skip the header (106 bytes)
    in.seekg(106);


    //read it
    IString inString;
    while(in >> inString) {
        band.push_back(inString);
        in >> inString;
        element.push_back(inString);
        in >> inString;
        validpixels.push_back(inString);
        in >> inString;
        average.push_back(inString);
        in >> inString;
        median.push_back(inString);
        in >> inString;
        stddev.push_back(inString);
        in >> inString;
        minimum.push_back(inString);
        in >> inString;
        maximum.push_back(inString);
        //Make sure Standard Deviation is not < 0 when reading in from a table
        vector<double>::iterator p;
        p = stddev.end() - 1;
        if(*p < 0) {
        *p = 0;
        }
    }
    in.close();
    }

    //********************************************************
    // Compute coefficients such that when we subtract/divide
    // using the coefficient the average or median of the
    // output image stays the same
    void keepSame(int &totalBands, int &rowcol, Mode mode) {
    // Loop for each band
    for(int iband = 1; iband <= totalBands; iband++) {
        double sumAverage = 0.0;
        double sumValidPixels = 0;
        for(int i = 0; i < rowcol; i++) {
        int index = (iband - 1) * rowcol + i;
        if(IsValidPixel(normalizer[index])) {
            sumAverage += normalizer[index] * validpixels[index];
            sumValidPixels += validpixels[index];
        }
        }

        // Neither sumValidPixels nor totalAverage will be zero
        // because of a test done earlier in IsisMain
        double totalAverage = sumAverage / sumValidPixels;
        for(int i = 0; i < rowcol; i++) {
        int index = (iband - 1) * rowcol + i;
        if(IsValidPixel(normalizer[index])) {
            if(mode == SUBTRACT) {
            normalizer[index] = normalizer[index] - totalAverage;
            }
            else {
            normalizer[index] = normalizer[index] / totalAverage;
            }
        }
        }
    }
    }

    // Apply coefficients multiplicatively
    void multiply(Buffer &in, Buffer &out) {
    // Compute the index into the normalizer array
    // We have to tweak the index based on the shape of the buffer
    // either a column or line
    int index;
    if(in.SampleDimension() == 1) {
        index = (in.Band() - 1) * totalSamples;   // Get to the proper band
        index += in.Sample() - 1;                 // Get to the proper column
    }
    else {
        index = (in.Band() - 1) * totalLines;     // Get to the proper band
        index += in.Line() - 1;                   // Get to the proper row
    }
    double coeff = normalizer[index];

    // Now loop and apply the coefficents
    for(int i = 0; i < in.size(); i++) {
        if(IsSpecial(in[i])) {
        out[i] = in[i];
        }
        else {
        out[i] = Null;
        if(coeff != 0.0 && IsValidPixel(coeff)) {
            out[i] = in[i] / coeff;
        }
        }
    }
    }

    // Apply coefficients subtractively
    void subtract(Buffer &in, Buffer &out) {
    // Compute the index into the normalizer array
    // We have to tweak the index based on the shape of the buffer
    // either a column or line
    int index;
    if(in.SampleDimension() == 1) {
        index = (in.Band() - 1) * totalSamples;   // Get to the proper band
        index += in.Sample() - 1;                 // Get to the proper column
    }
    else {
        index = (in.Band() - 1) * totalLines;     // Get to the proper band
        index += in.Line() - 1;                   // Get to the proper row
    }
    double coeff = normalizer[index];

    // Now loop and apply the coefficents
    for(int i = 0; i < in.size(); i++) {
        if(IsSpecial(in[i])) {
        out[i] = in[i];
        }
        else {
        out[i] = Null;
        if(IsValidPixel(coeff)) {
            out[i] = in[i] - coeff;
        }
        }
    }
    }

    // Perform lowpass and highpass filters on statistics
    void filterStats(vector<double> &filter, int &filtsize, bool &pause_crop,
                    int &channel) {
    int fsize = (int)filter.size();
    const int left_cut = 4;
    const int right_cut = 4;
    const int ch_pause_cnt = 3;
    const int ch_pause[2][ch_pause_cnt] = {{252, 515, 778}, {247, 510, 773}};
    const int ch_width[2][ch_pause_cnt] = {{11, 11, 11}, {11, 11, 11}};
    const QString ch_direc[2] = {"RIGHT", "LEFT"};
    const int iterations = 10;
    vector<double> filtin;
    vector<double> filtout;
    vector<double> filtorig;
    VecFilter vfilter;

    filtorig = filter;

    // To avoid filter ringing, cut out those areas in the data that
    // are especially problematic such as the left and right edges and
    // at the pause points
    for(int i = 0; i <= left_cut - 1; i++) {
        filter[i] = 0.0;
    }
    for(int i = fsize - 1; i >= fsize - right_cut; i--) {
        filter[i] = 0.0;
    }

    // Zero out the pause point pixels if requested and the input
    // image file has a bin mode of 1
    if(pause_crop && fsize == 1024) {
        for(int i = 0; i < ch_pause_cnt; i++) {
        int i1;
        int i2;
        if(ch_direc[channel] == "LEFT") {
            i1 = ch_pause[channel][i] - ch_width[channel][i];
            i2 = ch_pause[channel][i] - 1;
        }
        else {
            i1 = ch_pause[channel][i] - 1;
            i2 = ch_pause[channel][i] + ch_width[channel][i] - 2;
        }
        if(i1 < 0) i1 = 0;
        if(i2 > fsize - 1) i2 = fsize - 1;
        for(int j = i1; j <= i2; j++) {
            filter[j] = 0.0;
        }
        }
    }

    // Here is the boxfilter - the outer most loop is for the number
    // of filter iterations
    filtin = filter;

    for(int pass = 1; pass <= 3; pass++) {
        for(int it = 1; it <= iterations; it++) {
        filtout = vfilter.LowPass(filter, filtsize);
        filter = filtout;
        }

        // Zero out any columns that are different from the average by more
        // than a specified percent
        if(pass < 3) {
        double frac = .25;
        if(pass == 2) frac = .125;
        for(int k = 0; k < fsize; k++) {
            if(filter[k] != 0.0 && filtorig[k] != 0.0) {
            if(fabs(filtorig[k] - filter[k]) / filter[k] > frac) {
                filtin[k] = 0.0;
            }
            }
        }
        filter = filtin;
        }
    }

    // Perform the highpass by differencing the original from the lowpass
    filter = vfilter.HighPass(filtorig, filtout);

    filtin.clear();
    filtout.clear();
    filtorig.clear();
    }

    /**
    * Corrects the average and median Cubenorm statistics by using combination of
    * low and high pass filters
    *
    * @author Sharmila Prasad (1/27/2011)
    *
    * @param piFilterSize - Box car filter size
    * @param pbPauseCrop  - Flag whether to exclude column averages at pause points
    * @param piChannelNum - Input image Channel number
    * @param psMode       - Highpass Mode (Divide/Subtract)
    */
    void CorrectCubenormStats(int piFilterSize, bool pbPauseCrop, int piChannelNum, QString psMode)
    {
    const int MARKER       = -999999;
    const int iChannelSize = 3;
    const int iIterations  = 50;
    const int iChannelPause[2][iChannelSize] = {{252, 515, 778}, {247, 510, 773}};
    const int iChannelWidth[2][iChannelSize] = {{17, 17, 17}, {17, 17, 17}};
    const QString sChannelDirection[2]        = {"RIGHT", "LEFT"};

    int iLeftCut   = 6;
    int iRightCut  = 6;
    int iMaxPoints = 0;
    int iStatsSize = (int)validpixels.size();

    VecFilter vFilter;
    vector<double> dInFilter, dOrigFilter, dTempFilter, dFilter;

    for (int i = 0; i < iStatsSize; i++) {
        if (validpixels[i] > iMaxPoints) {
        iMaxPoints = validpixels[i];
        }
        if(validpixels[i] == 1) {
        stddev[i] = 0;
        }
    }

    for(int iIndex = 0; iIndex < 2; iIndex++) {
        if(!iIndex) {
        dOrigFilter = average;
        }
        else {
        dOrigFilter=median;
        }
        dTempFilter = dOrigFilter;
        dInFilter   = dOrigFilter;
        // To avoid filter ringing, cut out those areas in the data that
        // are especially problematic such as the left and right edges and
        // at the pause points
        if (iStatsSize == 512) {
        if (piChannelNum == 0) {
            iLeftCut = 40;
        }
        else {
            iRightCut = 40;
        }
        }
        if (iStatsSize == 256) {
        if (piChannelNum == 0) {
            iLeftCut = 50;
        }
        else {
            iRightCut = 50;
        }
        }
        // zero out left edge
        for(int i = 0; i < iLeftCut; i++) {
        dInFilter[i] = 0.0;
        }

        // zero out right edge
        int iMax = iStatsSize - iRightCut;
        for(int i = iStatsSize - 1; i >= iMax; i--) {
        dInFilter[i] = 0.0;
        }

        // Zero out the pause point pixels if requested and the input
        // image file has a bin mode of 1 (samples=1024)
        if(pbPauseCrop && iStatsSize == 1024) {
        for(int i = 0; i < (iChannelSize-1); i++) {
            int i1=0;
            int i2=0;
            if(sChannelDirection[piChannelNum] == "LEFT") {
            i1 = iChannelPause[piChannelNum][i] - iChannelWidth[piChannelNum][i];
            i2 = iChannelPause[piChannelNum][i] - 1;
            }
            else {
            i1 = iChannelPause[piChannelNum][i] - 1;
            i2 = iChannelPause[piChannelNum][i] + iChannelWidth[piChannelNum][i] - 2;
            }
            if(i1 < 0)
            i1 = 0;
            if(i2 > iStatsSize - 1)
            i2 = iStatsSize - 1;
            for(int j = i1; j <= i2; j++) {
            dInFilter[j] = 0.0;
            }
        }
        }

        // Here is the boxfilter - the outer most loop is for the number
        // of filter iterations
        for(int iPass = 1; iPass <= 3; iPass++) {
        for(int iIt = 1; iIt <= iIterations; iIt++) {
            dFilter = vFilter.LowPass(dInFilter, piFilterSize);
            dInFilter = dFilter;
        }

        // Zero out any columns that are different from the average by more
        // than a specific percent
        if(iPass < 3) {
            double dFraction = 0.25;
            if(iPass >= 2)
            dFraction = 0.125;
            for(int k = 0; k < iStatsSize; k++) {
            if(dInFilter[k] != 0.0 && dOrigFilter[k] != 0.0) {
                if(fabs(dOrigFilter[k] - dInFilter[k]) / dInFilter[k] > dFraction) {
                dTempFilter[k] = 0.0;
                }
            }
            }
            dInFilter = dTempFilter;
        }
        }

        // Perform the highpass by differencing the original from the lowpass
        dFilter = vFilter.HighPass(dOrigFilter, dFilter, validpixels, iMaxPoints, psMode);

        // -999999 is a flag set by highpass filter to indicate a column had a missing pixels
        // due to a problem with furrows or noise
        double dValue;
        if (piChannelNum == 0) {
        dValue = dFilter[iStatsSize-1];
        for (int i=iStatsSize-1; i>=0; i--) {
            if (dFilter[i] == MARKER) {
            if (dValue != MARKER) {
                dFilter[i] = dValue;
            }
            else {
                if (psMode=="SUBTRACT") {
                dFilter[i] = 0.0;
                }
                else {
                dFilter[i] = 1.0;
                }
            }
            }
            else {
            dValue = dFilter[i];
            }
        }
        }
        else {
        dValue = dFilter[0];
        for (int i=0; i<iStatsSize; i++) {
            if (dFilter[i] ==  MARKER) {
            if (dValue != MARKER) {
                dFilter[i] = dValue;
            }
            else {
                if (psMode=="SUBTRACT") {
                dFilter[i] = 0.0;
                }
                else {
                dFilter[i] = 1.0;
                }
            }
            }
            else {
            dValue = dFilter[i];
            }
        }
        }

        if(!iIndex) {
        average = dFilter;
        }
        else {
        median = dFilter;
        }
        dInFilter.clear();
        dOrigFilter.clear();
        dTempFilter.clear();
        dFilter.clear();
    }
    }
}
