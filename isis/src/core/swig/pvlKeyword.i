%module(package="isiscore") PvlKeyword
%{
    #include "PvlKeyword.h"
%}

%include "PvlKeyword.h"

%extend Isis::PvlKeyword {
  const char* __str__() {
    std::ostringstream out;
    out << *$self;
    std::string str = out.str();
    char * cstr = new char [str.length()+1];
    std::strcpy (cstr, str.c_str());
    return cstr;
  }

  PvlKeyword(const char* key, const char* val) {
            Isis::PvlKeyword *kw = new Isis::PvlKeyword(key, val);
            return kw;
  }
}

