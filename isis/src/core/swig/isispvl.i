%module isispvl
%{
    #include "Pvl.h"
%}

%include "Pvl.h"
%nodefaultdtor Isis::Pvl;

%extend Isis::Pvl {
  const char* __str__() {
    std::ostringstream out;
    out << *$self;
    std::string str = out.str();
    char * cstr = new char [str.length()+1];
    std::strcpy (cstr, str.c_str());
    return cstr;
  }

  Pvl(const char* file) {
    QString qfile(file);
    Isis::Pvl *pvl = new Isis::Pvl(qfile);
    return pvl;
  }

  void readString(const char *pvlString) {
    std::istringstream in(pvlString);
    in >> *$self;
  }

  void read(const char* file) {
    QString qfile(file);
    $self->read(qfile);
  }

  void write(const char* file) {
    QString qfile(file);
    $self->write(qfile);
  }
}
