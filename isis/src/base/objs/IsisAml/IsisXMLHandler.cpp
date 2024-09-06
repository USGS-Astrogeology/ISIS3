/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/util/XMLException.hpp>

#include <sstream>
#include <QString>

#include "IException.h"

#include "IString.h"

#include "IsisXMLHandler.h"
#include "IsisXMLChTrans.h"

using namespace std;

namespace XERCES = XERCES_CPP_NAMESPACE;


// Constructors

IsisXMLHandler::IsisXMLHandler() {
  value = NULL;
  outputEndTag = 0;
}


IsisXMLHandler::IsisXMLHandler(char *PencodingName,
                               bool &PexpandNamespaces,
                               XERCES::SAX2XMLReader* &Pparser,
                               QString *Pvalue) {

  encodingName = PencodingName;
  expandNamespaces = PexpandNamespaces;
  parser = Pparser;
  value = Pvalue;

  outputEndTag = 0;

  prevDocHandler = parser->getContentHandler();
  prevErrorHandler = parser->getErrorHandler();

  parser->setContentHandler(this);
  parser->setErrorHandler(this);
}


IsisXMLHandler::IsisXMLHandler(char *PencodingName,
                               bool &PexpandNamespaces,
                               XERCES::SAX2XMLReader* &Pparser) {

  value = NULL;
  outputEndTag = 0;

  encodingName = PencodingName;
  expandNamespaces = PexpandNamespaces;
  parser = Pparser;

  prevDocHandler = parser->getContentHandler();
  prevErrorHandler = parser->getErrorHandler();

  parser->setContentHandler(this);
  parser->setErrorHandler(this);
}


IsisXMLHandler::~IsisXMLHandler() {}


//  IsisXMLHandler: Overrides the SAX ErrorHandler
void IsisXMLHandler::error(const XERCES::SAXParseException &e) {
  ostringstream os;
  os << "Error in application XML file line: " << e.getLineNumber()
     << " char: " << e.getColumnNumber() << ". "
     << XERCES::XMLString::transcode(e.getMessage());
  throw Isis::IException(Isis::IException::Programmer, os.str(), _FILEINFO_);
}

void IsisXMLHandler::fatalError(const XERCES::SAXParseException &e) {
  ostringstream os;
  os << "Error in application XML file line: " << e.getLineNumber()
     << " char: " << e.getColumnNumber() << ". "
     << XERCES::XMLString::transcode(e.getMessage());
  throw Isis::IException(Isis::IException::Programmer, os.str(), _FILEINFO_);
}

void IsisXMLHandler::warning(const XERCES::SAXParseException &e) {
  ostringstream os;
  os << "Error in application XML file line: " << e.getLineNumber()
     << " char: " << e.getColumnNumber() << ". "
     << XERCES::XMLString::transcode(e.getMessage());
  throw Isis::IException(Isis::IException::Programmer, os.str(), _FILEINFO_);
}


//  IsisXMLHandler: Overrides of the SAX DocumentHandler interface
void IsisXMLHandler::characters(const XMLCh *const chars,
                                const XMLSize_t length) {

  if(value != NULL) {
    QString str;
    str = XERCES::XMLString::transcode(chars);
    str = str.trimmed();
    *value += str;
  }
}


void IsisXMLHandler::endDocument() {
}


void IsisXMLHandler::endElement(const XMLCh *const uri,
                                const XMLCh *const localname,
                                const XMLCh *const qname) {

  if(outputEndTag > 0) {
    QString str;
    str = XERCES::XMLString::transcode(localname);
    *value += "</" + str + ">";
    outputEndTag--;
  }
  else {
    parser->setContentHandler(prevDocHandler);
    parser->setErrorHandler(prevErrorHandler);
  }
}


void IsisXMLHandler::ignorableWhitespace(const XMLCh *const chars,
    const unsigned int length) {
}


void IsisXMLHandler::processingInstruction(const XMLCh *const target,
    const XMLCh *const data) {
}


void IsisXMLHandler::startDocument() {
}


void IsisXMLHandler::startElement(const XMLCh *const uri,
                                  const XMLCh *const localname,
                                  const XMLCh *const qname,
                                  const XERCES::Attributes &attributes) {

  if(value != NULL) {
    QString str;
    str = XERCES::XMLString::transcode(localname);
    // Note: need to build the attributes into the string too
    *value += "<" + str + ">";
    outputEndTag++;
  }
}
