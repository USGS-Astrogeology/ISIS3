/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include "Blob.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"

using namespace Isis;
using namespace std;

class MyBlob : public Isis::Blob {
  public:
    MyBlob(const std::string &name) : Isis::Blob(name, "Blob") {};
    MyBlob(const std::string &name, const std::string &file) : Isis::Blob(name, "Blob", file) {};
    void MyBuf(char *buf, int size) {
      p_buffer = new char[size];
      p_nbytes = size;
      memcpy(p_buffer, buf, size);
    };
    int StartByte() {
      return p_startByte;
    };
    void GetBuf(char *buf) {
      memcpy(buf, p_buffer, p_nbytes);
    }
};

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);
  try {
    cout << "Testing Blob(name, type) constructor and Write(filename) method..." << endl;
    MyBlob b("UnitTest");
    char buf[] = {"ABCD"};
    b.MyBuf(buf, 4);
    b.Write("junk");
    cout << "Name = " << b.Name() << endl;
    cout << "Number of Bytes = " << b.Size() << endl;
    cout << "StartByte = " << b.StartByte() << endl;
    cout << "Type = " << b.Type() << endl;
    cout << endl;

    cout << "Testing Blob(name, type, file) constructor..." << endl;
    MyBlob c("UNITtest", "junk");
    char buf2[5];
    c.GetBuf(buf2);
    buf2[4] = 0;
    cout << "Name = " << c.Name() << endl;
    cout << "Number of Bytes = " << c.Size() << endl;
    cout << "StartByte = " << c.StartByte() << endl;
    cout << "Type = " << c.Type() << endl;
    cout << buf2 << endl;
    cout << endl;

    // Test writing into existing space
    cout << "Testing writing into existing space..." << endl;
    cout << "Testing Write(pvl, stream)..." << endl;
    Isis::Pvl pvl("junk");
    fstream strm;
    strm.open("junk", std::ios::binary | std::ios::out);
    c.MyBuf(buf, 3);
    c.Write(pvl, strm);
    strm.seekp(0, std::ios::beg);
    strm << pvl;
    cout << "Name = " << c.Name() << endl;
    cout << "Number of Bytes = " << c.Size() << endl;
    cout << "StartByte = " << c.StartByte() << endl;
    cout << "Type = " << c.Type() << endl;
    cout << endl;
    strm.close();

    // Test writing over existing space at the end of file
    cout << "Testing writing over existing space at the end of file..." << endl;
    cout << "Testing Write(pvl, stream)..." << endl;
    Isis::Pvl pvl2("junk");
    fstream strm2;
    strm2.open("junk", std::ios::binary | std::ios::out);
    c.MyBuf(buf, 4);
    c.Write(pvl2, strm2);
    strm2.seekp(0, std::ios::beg);
    strm2 << pvl2;
    cout << "Name = " << c.Name() << endl;
    cout << "Number of Bytes = " << c.Size() << endl;
    cout << "StartByte = " << c.StartByte() << endl;
    cout << "Type = " << c.Type() << endl;
    cout << endl;
    strm2.close();

    remove("junk");

  }
  catch(Isis::IException &e) {
    e.print();
  }
}

