#include "Isis.h"
#include "ProcessImport.h"
#include "UserInterface.h"
#include "SpecialPixel.h"
#include "FileName.h"
#include "Pvl.h"
#include <QMap>

using namespace std;
using namespace Isis;

void IsisMain() {

  UserInterface &ui = Application::GetUserInterface();
  IString from = ui.GetFileName("FROM");
  ifstream fin;

  fin.open(from.c_str(), ios::in | ios::binary);
  if( !fin.is_open() ) {
    string msg = "Cannot open input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  /**
   *  0-rel byte offset   value
   *       0          32-bit integer magic number
   *       4          32-bit integer number of image lines
   *       8          32-bit integer number of bytes per image line
   *      12          32-bit integer number of bits per image elements
   *      16          32-bit integer currently unused
   *      20          32-bit integer number of bytes to start of image data
   *      24          ASCII label up to 1000 characters long
   *                  The label is NUL-terminated
   *
   */

   // ifstream read() needs a char* to read values into, so the union
   // is used to store read values
   union {
     char readChars[4];
     long readLong;
     float readFloat;
   } readBytes;

   // ddd files are LSB
   EndianSwapper swp("MSB");

  // Verify that the file is a ddd by reading in the first 4 bytes and
  // comparing the magic numbers. The magic number for a ddd file is 1659.
  readBytes.readLong = 0;
  fin.seekg(0);
  fin.read(readBytes.readChars, 4);
  if( fin.fail() || fin.eof() ) {
    string msg = "Could not read the magic number in the input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }
  readBytes.readFloat = swp.Float(readBytes.readChars);

  if(readBytes.readLong != 1659) {
    string msg = "Input file [" + from + "] does not appear to be in ddd format";
    throw IException(IException::Io, msg, _FILEINFO_);
  }

  // Read bytes 4-7 to get number of lines
  fin.read(readBytes.readChars, 4);
  if( fin.fail() || fin.eof() ) {
    string msg = "Could not read the number of lines in the input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }
  readBytes.readFloat = swp.Float(readBytes.readChars);
  int nLines = (int) readBytes.readLong;

  // Read bytes 8-11 to get number of bytes
  fin.read(readBytes.readChars, 4);
  if( fin.fail() || fin.eof() ) {
    string msg = "Could not read the number of bytes in the input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }
  readBytes.readFloat = swp.Float(readBytes.readChars);
  int nBytes = (int) readBytes.readLong;

  // Read bytes 12-15 to get the total number of bits out of all the bands
  fin.read(readBytes.readChars, 4);
  if( fin.fail() || fin.eof() ) {
    string msg = "Could not read the number of bits in the input file [" + from + "]";
    throw IException(IException::Io, msg, _FILEINFO_);
  }
  readBytes.readFloat = swp.Float(readBytes.readChars);
  int totalBandBits = readBytes.readLong;

  // Maps the bit type of the file to the number of bytes of that type
  // Taken directly from a given python program that reads in ddd data
  QMap<int, int> dataTypes;
  dataTypes.insert(1450901768, 1);
  dataTypes.insert(1450902032, 2);
  dataTypes.insert(1450902288, 2);
  dataTypes.insert(1450902560, 4);
  dataTypes.insert(1450902816, 4);
  dataTypes.insert(1450903072, 4);
  dataTypes.insert(1450903360, 8);
  dataTypes.insert(8, 1);
  dataTypes.insert(16, 2);
  dataTypes.insert(32, 4);
  dataTypes.insert(48, 2);

  // Read bytes 16-19 to get the bit type
  // Map the bit type to the number of bytes of that data type
  fin.read(readBytes.readChars, 4);
  readBytes.readFloat = swp.Float(readBytes.readChars);
  int bitType = (int) readBytes.readLong;

  int dataTypeBytes;
  int nOffset;
  // Check for new header format. Taken from the python program.
  if ( (bitType & 0xfffff000) == 0x567b0000 ) {
    dataTypeBytes = dataTypes.value(bitType);

    // Read bytes 20-23 to get offset
    // New header format may have different offsets
    fin.read(readBytes.readChars, 4);
    readBytes.readFloat = swp.Float(readBytes.readChars);
    nOffset = (int) readBytes.readLong;
    if (nOffset < 1024) {
      nOffset = 1024;
    }
  }
  else {
    // Old header format does not have a bit type
    // Old header format's offset is always 1024.
     dataTypeBytes = dataTypes.value(totalBandBits);
     nOffset = 1024;
  }

  if (dataTypeBytes == 0) {
    string msg = "The value totalBandBits [" + to_string(totalBandBits) + "] does not map " +
                  "to any byte size in the dataTypes table.";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  fin.close();

  PvlGroup results("FileInfo");
  results += PvlKeyword( "NumberOfLines", std::to_string(nLines) );
  results += PvlKeyword( "NumberOfBytesPerLine", std::to_string(nBytes) );
  results += PvlKeyword( "BitType", std::to_string(bitType) );
  int nSamples = nBytes / (totalBandBits / 8);
  results += PvlKeyword( "NumberOfSamples", std::to_string(nSamples) );
  int nBands = (totalBandBits / 8) / dataTypeBytes;
  results += PvlKeyword( "NumberOfBands", std::to_string(nBands) );
  results += PvlKeyword( "LabelBytes", std::to_string(nOffset) );
  Application::Log(results);

  ProcessImport p;

  int bitsPerBand = totalBandBits / nBands;
  switch(bitsPerBand) {
    case 8:
      p.SetPixelType(Isis::UnsignedByte);
      break;
    case 16:
      p.SetPixelType(Isis::UnsignedWord);
      break;
    case 32:
      p.SetPixelType(Isis::Real);
      break;
    default:
      IString msg = "Unsupported bit per pixel count [" + IString(bitsPerBand) + "] ";
      msg += "from [" + from + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
  }

  // ddd files with more than one band are pixel interleaved
  // Having one band is similar to BIP, but this is here for clarification
  if (nBands > 1) {
    p.SetOrganization(ProcessImport::BIP);
  }

  p.SetDimensions(nSamples, nLines, nBands);
  p.SetFileHeaderBytes(nOffset);
  p.SetByteOrder(Isis::Msb);
  p.SetInputFile( ui.GetFileName("FROM") );
  p.SetOutputCube("TO");

  p.StartProcess();
  p.EndProcess();
}
