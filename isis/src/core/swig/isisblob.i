%module(package="isiscore") Blob
%{
    #include "Blob.h"
%}

%include "Blob.h"
%nodefaultdtor Isis::Blob;

// %extend Isis::Blob {

//   Blob(const char* name, const char* type) {
//     QString qname(name);
//     QString qtype(type);
//     Isis::Blob *blob = new Isis::Blob(qname, qtype);
//     return blob;
//   }

//   Blob(const char* name, const char* type, const char* file) {
//     QString qname(name);
//     QString qtype(type);
//     QString qfile(file);
//     Isis::Blob *blob = new Isis::Blob(qname, qtype, file);
//     return blob;
//   }

//   void Write(const char *file) {
//     QString qfile(file);

//     $self->Write(file);
//   }

//   char * getStringBuffer() {
//     int bufferSize = $self->Size();
//     char * buffer = $self->getBuffer();
//     std::ostringstream os;
//     for (int i = 0; i < bufferSize; i++) os << buffer[i];
//     os << '\0';

//     std::string str = os.str();
//     char * cstr = new char [str.length()+1];
//     std::strcpy (cstr, str.c_str());
//     return cstr;
//   }

// }
