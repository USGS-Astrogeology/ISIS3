%module(package="isispvl") PvlGroup
%{
    #include "PvlGroup.h"
%}

%include "PvlGroup.h"

%extend Isis::PvlGroup {
  const char* __str__() {
    std::ostringstream out;
    out << *$self;
    std::string str = out.str();
    char * cstr = new char [str.length()+1];
    std::strcpy (cstr, str.c_str());
    return cstr;
  }

  PvlGroup(const char* groupName) {
    QString qgroupName(groupName);
    Isis::PvlGroup *group = new Isis::PvlGroup(qgroupName);
    return group;
  }
}
