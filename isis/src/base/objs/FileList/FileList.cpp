/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "FileList.h"
#include "IException.h"
#include "Message.h"
#include "FileName.h"
#include "IString.h"
#include <iostream>
#include <fstream>

using namespace std;
namespace Isis {

  /**
   * Constructs an empty FileList.
   *
   */
  FileList::FileList() {
  }


  /**
   * Constructs a FileList from a FileName.
   *
   * @param listFile A FileName obj
   */
  FileList::FileList(FileName listFile) {
    read(listFile);
  }

  /**
   * Constructs a FileList from an istream.
   *
   * @param in the istream to read from
   */
  FileList::FileList(std::istream &in) {
    read(in);
  }


  /**
   * Opens and loads the list of files from a file.
   *
   * @param listFile Name of the file to open that contains the list of files.
   *
   * @throws Isis::iException::Io - Cannot open file
   */
  void FileList::read(FileName listFile) {
    // Open the file
    ifstream istm;
    istm.open(listFile.toString().c_str(), std::ios::in);
    if(!istm) {
      std::string message = Isis::Message::FileOpen(listFile.toString());
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // Internalize
    try {
      read(istm);

      // Close the file
      istm.close();
    }
    catch (IException &e) {
      istm.close();
      std::string msg = "File [" + listFile.toString() + "] contains no data";
      throw IException(IException::User, msg, _FILEINFO_);
    }

  }

  /**
   * Loads list of files from a stream.
   * This takes in a stream and loads a file list from it. The lines in the stream
   * are considered separate entries, and comments are ignored. comments are
   * considered to be any line starting with a '#' or '//', and anything after any
   * whitespace following the first text on the line.
   *
   * @param in An input stream containing a list of files.
   *
   */
  void FileList::read(std::istream &in) {
    // Read each file and put it in the vector
    char buf[65536];
    Isis::IString s;
    bool bHasQuotes = false;

    bool isComment = false;
    do {
      in.getline(buf, 65536);

      s = buf;
      string::size_type loc = s.find("\"", 0);

      if (loc != string::npos) {
        bHasQuotes = true;
      }

      if (bHasQuotes) {
        s = s.TrimHead("\"");
        s = s.TrimTail("\"");
      }

      s = s.TrimHead(" \n\r\t\v");

      isComment = false;
      if(strlen(buf) == 0) {
        continue;
      }
      for (int index = 0; index < (int)strlen(buf); index++) {
        if (buf[index] == '#' || (buf[index] == '/' && buf[index+1] == '/')) {
          isComment = true;
          break;
        }
        else if(buf[index] == ' ') {
          continue;
        }
        else {
          isComment = false;
          break;
        }
      }
      if (isComment) {
        continue;
      }
      else {

        if(bHasQuotes) {
          s = s.Token(" \n\r\t\v");
        }
        else {
          s = s.Token(" \n\r\t\v,");
        }

        this->push_back(s);
      }

    } while(!in.eof());
    if (this->size() == 0) {
      string msg = "Input Stream Empty";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * Writes a list of files to a file.
   *
   * @param outputFileList The name of the file to create. The method will overwrite any
   * existing files.
   *
   * @throws Isis::iException::Io File could not be created.
   */
  void FileList::write(FileName outputFileList) {
    // Open the file
    ofstream ostm;
    ostm.open(outputFileList.toString().c_str(), std::ios::out);
    if (!ostm) {
      std::string message = Message::FileOpen(outputFileList.toString());
      throw IException(IException::Io, message, _FILEINFO_);
    }

    // Internalize
    write(ostm);

    // Close the file
    ostm.close();
  }


  /**
   * Writes a list of files to a stream.
   *
   * @param out The list will be written to this output stream.
   */
  void FileList::write(std::ostream &out) {
    for (int i = 0; i < this->size(); i++) {
      out << (*this)[i].toString() << endl;
    }
  }
} // end isis namespace
