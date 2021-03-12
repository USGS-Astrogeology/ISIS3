%module isispvl
%{
    #include <string>
    #include <sstream>
    #include "Pvl.h"
%}

%include std_string.i
%include std_vector.i
%include exception.i

%include "pvlKeyword.i"
%include "PvlContainer.h"
%include "pvlGroup.i"

%include "pvlObject.i"
%nodefaultdtor Isis::PvlObject;

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

  void read(const char* file) {
    QString qfile(file);
    $self->read(qfile);
  }

  void write(const char* file) {
    QString qfile(file);
    $self->write(qfile);
  }
}

class QString
{
public:
    static QString fromStdString(const std::string &s);
    std::string toStdString() const;
    QString(const char* str);
    ~QString();

    int size() const;
    int count() const;
    int length() const;
    bool isEmpty() const;
};

%exception{
    try {
        $action
    } catch (std::exception const& e) {
        SWIG_exception(SWIG_RuntimeError, (std::string("std::exception: ") + e.what()).c_str());
    } catch (...) {
        SWIG_exception(SWIG_UnknownError, "Unknown error");
    }
}
