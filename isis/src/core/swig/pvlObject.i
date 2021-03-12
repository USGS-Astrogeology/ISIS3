%module(package="isispvl") PvlObject
%{
    #include "PvlObject.h"
%}

%include "PvlObject.h"

%extend Isis::PvlObject {
  const char* __str__() {
    std::ostringstream out;
    out << *$self;
    std::string str = out.str();
    char * cstr = new char [str.length()+1];
    std::strcpy (cstr, str.c_str());
    return cstr;
  }

  PvlObject(const char* objectName) {
    QString qobjectName(objectName);
    Isis::PvlObject *object = new Isis::PvlObject(qobjectName);
    return object;
  }
}
