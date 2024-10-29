#ifndef CASSINI_H
#define CASSINI_H

#include <vector>

#include <QString>

#include "EndianSwapper.h"
#include "IsisImportBaseProcessFunctor.h"
#include "ProcessByLine.h"
#include "ProcessImport.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "Stretch.h"
#include "Table.h"
#include "TableField.h"
#include "TableRecord.h"

using namespace std;

namespace Isis {

  /**
   * This class is used as a functor to perform post processing on cassini iss
   * images
   *
   * @author 2022-03-31 Adam Paquette
   *
   * @internal
   */
  class CassiniIssFixDnFunctor : public ProcessFunctor {
    public:
      /**
       * Constructs a CassiniIssFunctor
       *
       * @param stats Pointer to a Statistics object to add data to
       * @param percent Sampling percentage of the image, used to calculate a line increment,
       *                when calculating statistics
       */
      CassiniIssFixDnFunctor(PvlKeyword &stretchPairs, QString dataConversionType, int validMax) {
        m_stretch = Stretch();
        for (size_t i = 0; i < (size_t)stretchPairs.size(); i+=2) {
          m_stretch.AddPair(toDouble(stretchPairs[i]),
                             toDouble(stretchPairs[i + 1]));
        }
        m_validMax = validMax;
        m_dataConversionType = dataConversionType;
      }

      virtual ~CassiniIssFixDnFunctor() {};

      void operator()(Buffer &buf) const {
        for(int i = 0; i < buf.size(); i++) {
          // zeros and negatives are valid DN values, according to scientists,
          // but likelyhood of a zero in 16 bit is rare,
          // so assume these are missing pixels and set them to null
          if(buf[i] == 0) {
            buf[i] = Null;
          }
          else if(m_dataConversionType == "Table") {
            buf[i] = m_stretch.Map((int)buf[i]);
          }
          // save max values (4095 for table-converted images and 255 for others) as HRS
          if(buf[i] >= m_validMax) {
            buf[i] = Hrs;
          }
        }
      };

    private:
      Stretch m_stretch; //!< Calculated stretch created from a stretch pair
      QString m_dataConversionType;
      int m_validMax; //!< Valid maximum as defined in the cassini iss image label
  };

  void cassiniIssFixDnPostProcess(QString ioFile, PvlObject postProcessObj) {
    PvlKeyword stretchPairsKeyword = postProcessObj["stretchPairs"];
    QString dataConversionType = postProcessObj["DataConversionType"];
    int validMax = toInt(postProcessObj["ValidMaximum"]);
    CassiniIssFixDnFunctor cassiniIssFixDnFunctor(stretchPairsKeyword, dataConversionType, validMax);

    ProcessByLine postProcess;
    CubeAttributeInput att;
    postProcess.SetInputCube(ioFile, att, ReadWrite);
    if(dataConversionType == "12Bit") {
      postProcess.Progress()->SetText("Setting special pixels and saving as 16bit...");
    }
    else if(dataConversionType == "8LSB") {
      postProcess.Progress()->SetText("Setting special pixels and saving as 16bit...");
    }
    //if ConversionType == Table, Use LUT to create stretch pairs for conversion
    else {
      postProcess.Progress()->SetText("Converting image pixels back to 12-bit and saving as 16bit...");
    }
    postProcess.StartProcess(cassiniIssFixDnFunctor);
    postProcess.EndProcess();
  };

  void cassiniIssCreateLinePrefixTable(Cube *cube, vector<char *> prefixData, PvlObject translation) {
    int sumMode = translation["SummingMode"];
    QString compressionType = translation["CompressionType"];
    double flightSoftware = 0.0;

    if (QString(translation["flightSoftwareVersionId"]).toStdString() != std::string("Unknown")) {
      flightSoftware = toDouble(translation["flightSoftwareVersionId"]);
    }

    PvlKeyword stretchPairsKeyword = translation["stretchPairs"];
    QString dataConversionType = translation["DataConversionType"];
    int validMax = toInt(translation["ValidMaximum"]);
    CassiniIssFixDnFunctor cassiniIssFixDnFunctor(stretchPairsKeyword, dataConversionType, validMax);

    TableField overclockPixels("OverclockPixels", TableField::Double, 3);
    //3 columns, first two are overclocked pixels and the third is their average
    TableRecord linePrefixRecord;
    linePrefixRecord += overclockPixels;
    Table linePrefixTable("ISS Prefix Pixels", linePrefixRecord);
    linePrefixTable.SetAssociation(Table::Lines);
    for(int l = 0; l < (int)prefixData.size(); l++) {
      unsigned char *linePrefix = (unsigned char *)(prefixData[l]);
      Buffer pixelBuf(1, 1, 1, SignedWord);

      vector<double> calibrationPixels;
      //Pixel data is MSB, see SIS version 1.1 page 17
      EndianSwapper swapper("MSB");

      vector<double> pixel;
      //12 is start byte for First Overclocked Pixel Sum in Binary Line Prefix, SIS version 1.1 page 94
      pixel.push_back(swapper.ShortInt(& (linePrefix[12])));
      //22 is start byte for Last Overclocked Pixel Sum in Binary Line Prefix, see SIS version 1.1 page 94
      pixel.push_back(swapper.ShortInt(& (linePrefix[22])));

      // section modelled after IDL CISSCAL's OverclockAvg() in cassimg_define.pro
      double overclockAvg = 0.0;
      if(compressionType != "Lossy" && flightSoftware < 1.3) {  //numberOfOverclocks == 1
        // if Bltype CASSINI-ISS or CAS-ISS2, i.e. flight software version < 1.3
        // then there is only one column of valid overclocks in prefix pixels table,
        // the first column contains nulls, so use column 2 as average
        overclockAvg = pixel[1];
      }
      else { //numberOfOverclocks == 2
        // number of columns of valid overclocks in prefix pixels table is 2
        // for CAS-ISS3 or CAS-ISS4, i.e. flight software version 1.3 or 1.4
        // calculate appropriate average (as in cassimg_define.pro, CassImg::OverclockAvg())
        if(sumMode == 1) {
          overclockAvg = ((((double) pixel[0]) / 2 + ((double) pixel[1]) / 6) / 2);
        }
        if(sumMode == 2) {
          overclockAvg = ((((double)pixel[0]) + ((double) pixel[1]) / 3) / 2);
        }
        if(sumMode == 4) {
          overclockAvg = ((((double) pixel[0]) + ((double) pixel[1])) / 2);
        }
        else overclockAvg = 0;
      }
      // End Section

      pixel.push_back(overclockAvg);

      for(int i = 0; i < (int)pixel.size(); i++) {
        pixelBuf[0] = pixel[i];
        //  Do 8 bit to 12 bit conversion for prefix data
        cassiniIssFixDnFunctor(pixelBuf);
        double pix = pixelBuf[0];
        if(pix == NULL8) {
          calibrationPixels.push_back(NULL2);
        }
        else if(pix == LOW_REPR_SAT8) {
          calibrationPixels.push_back(LOW_REPR_SAT2);
        }
        else if(pix == LOW_INSTR_SAT8) {
          calibrationPixels.push_back(LOW_INSTR_SAT2);
        }
        else if(pix == HIGH_INSTR_SAT8) {
          calibrationPixels.push_back(HIGH_INSTR_SAT2);
        }
        else if(pix == HIGH_REPR_SAT8) {
          calibrationPixels.push_back(HIGH_REPR_SAT2);
        }
        else {
          calibrationPixels.push_back(pix);
        }
      }
      linePrefixRecord[0] = calibrationPixels;
      linePrefixTable += linePrefixRecord;
    }
    cube->write(linePrefixTable);
  };

  void cassiniIssFixLabel(Cube *cube, PvlObject translation, ProcessImport *importer) {
    Pvl *outputLabel = cube->label();
    //Adjust Table-encoded values from 8 bit back to 12 bit.
    PvlKeyword stretchPairs = translation["stretchPairs"];
    Stretch stretch;
    for (size_t i = 0; i < (size_t)stretchPairs.size(); i+=2) {
      stretch.AddPair(toDouble(stretchPairs[i]),
                         toDouble(stretchPairs[i + 1]));
    }
    PvlGroup &inst = outputLabel->findGroup("Instrument", Pvl::Traverse);
    QString dataConversionType = translation["DataConversionType"];
    if(dataConversionType != "Table") {   //Conversion Type is 12Bit or 8LSB, only save off overclocked pixels
      if(dataConversionType == "12Bit") {
        importer->Progress()->SetText("Image was 12 bit. No conversion needed. \nSaving line prefix data...");
      }
      else { //if (dataConversionType == "8LSB") {
        importer->Progress()->SetText("Image was truncated to 8 least significant bits. No conversion needed. \nSaving line prefix data...");
      }
    }
    else {
      double biasStripMean = inst.findKeyword("BiasStripMean");
      inst.findKeyword("BiasStripMean").setValue(toString(stretch.Map(biasStripMean)));
      inst.findKeyword("BiasStripMean").addComment("BiasStripMean value converted back to 12 bit.");
      importer->Progress()->SetText("Image was converted using 12-to-8 bit table. \nConverting prefix pixels back to 12 bit and saving line prefix data...");
    }

    unsigned char *header = (unsigned char *) importer->FileHeader();
    int vicarLabelBytes = translation.findKeyword("VicarLabelBytes");
    int roo = *(header + 50 + vicarLabelBytes) / 32 % 2; //**** THIS MAY NEED TO BE CHANGED,
    // SEE BOTTOM OF THIS FILE FOR IN DEPTH COMMENTS ON READOUTORDER
    inst.addKeyword(PvlKeyword("ReadoutOrder", toString(roo)));
  };
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File header and readout order comments...
// OUR FILE HEADER INCLUDES TWO SECTIONS:
//        -The first is the VICAR label (SIS page 52).  The number of bytes included here is calculated in the IsisMain()
//        -The second is the Binary Label Header, or Binary Telemetry Header(SIS page 52).  This contains 60 bytes (SIS page 84) of significant data.
// The READOUT ORDER of an image is the order in which the cameras were read.  This is needed for radiometric calibration (CISSCAL).
// The possible values are :
//        0 : Narrow-angle camera was read out first
//        1 : Wide-angle camera was read out first
// IDL CISSCAL FILE CASSIMG_SUBTRACTDARK.PRO LINE 333:
//        roo = bh[50]/32 MOD 2 ;Readout order is the 2nd bit of the 51st byte
// According to SIS page 92 (Field=Software, Valid Values), the readout order is index 2 (the THIRD bit) of the byte.
// Normally, we would assume that this was the third bit from the right, but there is some confusion on this matter.
// SIS page 17 says bits and bytes are both "big endian" for pixel data, but doesn't mention whether this includes the binary telemetry table data,
// Reading the first 3 bytes of the binary header and comparing with bit values described in SIS Table 7.3.2,
// if the bytes are read as most significant bit first (left-to-right), each value matches up except summation mode.
// In this case, SIS says they shoud be sum1:01, sum2:10, sum4:11.  Actual values are sum1:00, sum2:01, sum4:10.
// The IDL code also appears to be written as though bits are read in this manner, accessing the third bit from the left (32 ~ 00100000).
// Since we haven't found a difinitive answer to this, we are mimicking the IDL code to determine the read out order.
// We have not found an image with roo = 1 as of yet to test this.
// If it is found to be the case that bits are read from left to right in this header, it may be more clear in the
// future to rewrite the line using a logical bitwise &-operator: roo = *(header+50+vicarLabelBytes) & (00100000);
// SOURCES :
//        Cassini ISS Tour VICAR Image Data File and Detatched PDS Label SIS, Tour Version 1.1 December 1, 2004
//        IDL cisscal application files: cassimg_subtractdark.pro and linetime.pro
// -Jeannie Walldren 08/06/2008
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif
