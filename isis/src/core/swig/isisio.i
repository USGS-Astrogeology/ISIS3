%module isisio

%{
    #include <QString>
    #include <array>
    #include <vector> 
    #include <string>
    #include <sstream>
%}

%include "std_vector.i"
%include "std_string.i"
%include "std_array.i"
%include "std_map.i"
%include "carrays.i"
%include "std_pair.i"
%include "exception.i"

#include <nlohmann/json.hpp>
#include <QVector>
#include <QString>

%typemap(in) nlohmann::json {
  if (PyDict_Check($input) || PyList_Check($input)) {
    PyObject* module = PyImport_ImportModule("json");
    PyObject* jsonDumps = PyUnicode_FromString("dumps");
    PyObject* pythonJsonString = PyObject_CallMethodObjArgs(module, jsonDumps, $input, NULL);
    $1 = nlohmann::json::parse(PyUnicode_AsUTF8(pythonJsonString));
  }
  else {
    PyErr_SetString(PyExc_TypeError, "not a json serializable type");
    SWIG_fail;
  }
}

%typemap(typecheck, precedence=SWIG_TYPECHECK_MAP) nlohmann::json {
  $1 = PyDict_Check($input) ? 1 : 0;
}

%typemap(out) nlohmann::json {
  PyObject* module = PyImport_ImportModule("json");
  PyObject* jsonLoads = PyUnicode_FromString("loads");
  std::string jsonString = ($1).dump();
  PyObject* pythonJsonString = PyUnicode_DecodeUTF8(jsonString.c_str(), jsonString.size(), NULL);
  $result = PyObject_CallMethodObjArgs(module, jsonLoads, pythonJsonString, NULL);
}

%typemap(in) QVector<QString> & {
  int i;
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError,"Expected a List");
    SWIG_fail;
  }

  int size = PySequence_Length($input);
  $1 = new QVector<QString>(size);
  
  for (i = 0; i < size; i++) {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyUnicode_Check(o)) {
      QString s = QString::fromUtf8(PyUnicode_AsUTF8(o));
      (*$1)[i] = s; 
    } else {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be string");      
      SWIG_fail;
    }
  }
}



%typemap(out) QVector<QString> & {
  $result = PyList_New($1_dim0);
  for (i = 0; i < $1_dim0; i++) {
    PyObject *o = PyString_AsString($1->at(i));
    PyList_SetItem($result,i,o);
  }
}


%typemap(in) QString const & {
  if (!PyUnicode_Check($input)) {
    PyErr_SetString(PyExc_ValueError,"Expected a String");
    SWIG_fail;
  }
  $1 = new QString(QString::fromUtf8(PyUnicode_AsUTF8($input))); 
}


%typemap(out) QString const & {
  $result = Py_BuildValue("s#", $1.toStdString().c_str(), $1.size());
}


%typemap(in) QString {
  if (!PyUnicode_Check($input)) {
    std::cout << "TANGERINE" << std::endl;
    PyErr_SetString(PyExc_ValueError,"Expected a String");
    SWIG_fail;
  }
  $1 = QString::fromUtf8(PyUnicode_AsUTF8($input)); 
}

%typemap(typecheck,precedence=SWIG_TYPECHECK_STRING) QString {
    $1 = !PyUnicode_Check($input);
    std::cout << PyUnicode_AsUTF8($input) << std::endl;
}

%typemap(out) QString = QString const &;

%typemap(in) QVector<QString> {
  int i;
  if (!PySequence_Check($input)) {
    PyErr_SetString(PyExc_ValueError,"Expected a List");
    SWIG_fail;
  }

  int size = PySequence_Length($input);
  $1 = new QVector<QString>(size);
  
  for (i = 0; i < size; i++) {
    PyObject *o = PySequence_GetItem($input,i);
    if (PyUnicode_Check(o)) {
      QString s = QString::fromUtf8(PyUnicode_AsUTF8(o));
      (*$1)[i] = s; 
    } else {
      PyErr_SetString(PyExc_ValueError,"Sequence elements must be string");      
      SWIG_fail;
    }
  }
}


%typemap(out) QVector<QString> = QVector<QString> &;

namespace std {
  %template(IntVector) vector<int>;
  %template(DoubleVector) vector<double>;
  %template(VectorDoubleVector) vector< vector<double> >;
  %template(StringVector) vector<string>;
  %template(ConstCharVector) vector<const char*>;
  %template(PairDoubleVector) vector<pair<double, double>>;
  %template(DoubleArray6) array<double, 6>;
}

%exception {
  try {
    $action
  } catch (std::exception const& e) {
    SWIG_exception(SWIG_RuntimeError, (std::string("std::exception: ") + e.what()).c_str());
  } catch (...) {
    SWIG_exception(SWIG_UnknownError, "Unknown error");
  }
}

%include "pvlKeyword.i"
%include "PvlContainer.h"
%include "pvlGroup.i"
%include "pvlObject.i"
%nodefaultdtor Isis::PvlObject;
// %include "UserInterface.i"
%include "isisblob.i"
%include "isispvl.i"
