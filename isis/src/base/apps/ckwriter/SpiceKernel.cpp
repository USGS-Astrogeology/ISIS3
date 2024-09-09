/**
 * @file
 * $Revision$
 * $Date$
 * $Id$
 *
 *   Unless noted otherwise, the portions of Isis written by the USGS are
 *   public domain. See individual third-party library and package descriptions
 *   for intellectual property information, user agreements, and related
 *   information.
 *
 *   Although Isis has been used by the USGS, no warranty, expressed or
 *   implied, is made by the USGS as to the accuracy and functioning of such
 *   software and related material nor shall the fact of distribution
 *   constitute any such warranty, and no responsibility is assumed by the
 *   USGS in connection therewith.
 *
 *   For additional information, launch
 *   $ISISROOT/doc//documents/Disclaimers/Disclaimers.html
 *   in a browser or see the Privacy &amp; Disclaimers page on the Isis website,
 *   http://isis.astrogeology.usgs.gov, and the USGS privacy and disclaimers on
 *   http://www.usgs.gov/privacy.html.
 */
#include <algorithm>
#include <functional>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

#include <QString>
#include <QStringList>

#include <SpiceUsr.h>

#include "Application.h"
#include "CkKernelWriter.h"
#include "Cube.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "SpiceKernel.h"
#include "TextFile.h"

using namespace std;

namespace Isis {

 SpiceKernel::SpiceKernel() {
   init();
 }

void SpiceKernel::add(const QString &cfile) {
 Cube cube;
 cube.open(cfile);
 add(cube);
 return;
}

void SpiceKernel::add(Cube &cube) {
  m_segments.push_back(CkSpiceSegment(cube));
  return;
}
const CkSpiceSegment &SpiceKernel::operator[](const int i) const {
 return (m_segments[i]);
}


bool CheckSegment(const CkSpiceSegment *s1, const CkSpiceSegment *s2) {
  return (s1->startTime() < s2->startTime());
}

QString SpiceKernel::getSummary(const QString &commfile) const {
  vector<const CkSpiceSegment *> seglist;
  for ( int i = 0 ; i < size() ; i++) {
    seglist.push_back(&m_segments[i]);
  }

  // Sorts the Segment pointers
  stable_sort(seglist.begin(), seglist.end(), CheckSegment);

  QString comment = getCkComment(commfile);

  // Collect comments from each segment
  for ( unsigned int i = 0 ; i < seglist.size() ; i++ ) {
    comment += seglist[i]->getComment();
  }
  return (comment);
}

bool SpiceKernel::validate() const {

 QVector<const CkSpiceSegment *> seglist;
  for ( int i = 0 ; i < size() ; i++) {
    seglist.push_back(&m_segments[i]);
  }

  // Sorts the Segment pointers
  stable_sort(seglist.begin(), seglist.end(), CheckSegment);

  // Now check for overlapping times.  In general, any segment that overlaps
  // another is suspect and will be flagged as an error.  An IException is
  // created and thrown when all images have been evaluated and overlaps are
  // found to exist.
  QStringList errors;
  for (int seg = 1 ; seg < seglist.size() ; seg++) {
    if ( (seglist[seg-1]->endTime() > seglist[seg]->startTime()) &&
         (seglist[seg-1]->InstCode() == seglist[seg]->InstCode()) ) {
      errors  << "CKSegment " + seglist[seg]->Id() + " overlaps CKSegment "
                  + seglist[seg-1]->Id();
    }
  }

  //  Now check for problems
  if ( 0 < errors.size() ) {
     QString mess = "Time overlap conflicts are present in segment (image) list. "
                    "This will likely create erroneous pointing in one or more "
                    "images.  You should create a seperate kernel for conflicting "
                    "images that overlap another.  Images with time overlap "
                    "conflicts are:   \n"
                    + errors.join("; ");
     throw IException(IException::User, mess.toStdString(), _FILEINFO_);
  }

  return (true);
}


 void SpiceKernel::write(const QString &kname, const QString &comfile,
                         const int cktype) const {
   vector<const CkSpiceSegment *> seglist;
   int comChars(0);
   for ( int i = 0 ; i < size() ; i++) {
     seglist.push_back(&m_segments[i]);
     comChars += m_segments[i].getComment().size();
   }

   stable_sort(seglist.begin(), seglist.end(), CheckSegment);

   QString comment = getCkComment(comfile);
   comChars += comment.size();

   //  Create the output file.
   try {
     NaifStatus::CheckErrors();
     CkKernelWriter ckwriter(kname, comChars+512, cktype);
     ckwriter.addComment(comment);
     NaifStatus::CheckErrors();

   // Write sorted segments
     for ( unsigned int i = 0 ; i < seglist.size() ; i++ ) {
       try {
         ckwriter.write(*seglist[i]);
         comment = seglist[i]->getComment();
         ckwriter.addComment(comment);
         NaifStatus::CheckErrors();
       } catch ( IException &ie ) {
         ostringstream mess;
         mess << "Failed to write segment, ID = " << seglist[i]->Id().toStdString();
         throw IException(ie, IException::Programmer, mess.str(), _FILEINFO_);
       }
     }
   }
   catch ( IException &ie ) {
     ostringstream mess;
     mess << "Could not create output CK kernel file: " << kname.toStdString();
     throw IException(ie, IException::User, mess.str(), _FILEINFO_);
   }
   return;
 }

 void SpiceKernel::init() {
   m_segments.clear();
   return;
 }

 QString SpiceKernel::getCkComment(const QString &comFile) const {
   ostringstream comment;

   // Write generic comment
   comment << "\
****************************************************************************\n\
  USGS ISIS (ckwriter) Generated CK Kernel\n\
  Created By:   " << Application::UserName().toStdString() << "\n\
  Date Created: " << Application::DateTime().toStdString() << "\n\
****************************************************************************\n\
\n\
Orientation Data in the File\n\
-----------------------------------------------------------------------\n\
\n\
      This file contains orientation and potentially derived angular\n\
      rates (where possible/specified).\n\
\n\
\n\
Status\n\
-----------------------------------------------------------------------\n\
\n\
      This kernel was generated for the purpose of storing C-Smithed\n\
      pointing updates generated through ISIS processing techniques\n\
      (control nets, jitter analysis, etc...).  These CK kernels\n\
      are intended to mimick CKs provided by individual mission\n\
      (NAV teams).\n\
\n\
Pedigree\n\
-----------------------------------------------------------------------\n\
\n\
      This file was generated by an automated process.  The ISIS\n\
      application ckwriter was used to read CK kernel data\n\
      contained within an ISIS cube file.  It then writes it as an\n\
      individual segment in the CK.  Hence, a list of files can be\n\
      written to a single CK kernel.  However, mixing the instruments\n\
      contained in a single CK kernel is discouraged.\n\
\n\
      Individual segments coming from files will have a single record\n\
      written for the center of the exposure (time) for framing \n\
      instruments or a record/image line for line scan instruments.\n\
\n\
      Creating type 3 CK kernels must contain at least 3 records for\n\
      framing instruments to avoid roundoff error for the center of the\n\
      exposure time of an image.  Framing instruments should pad time\n\
      using the spiceinit application options.\n\
\n\
\n\
Angular Rates\n\
-----------------------------------------------------------------------\n\
\n\
      This kernel may or may not contain angular velocity vectors. Efforts\n\
      are made to preserve and provide angular velocities where they\n\
      originally existed.\n\
\n\
\n\
Usage Note\n\
-----------------------------------------------------------------------\n\
\n\
      To make use of this file in a typical SPICE based application,\n\
      you must supply a leapseconds kernel, a mission spacecraft clock\n\
      kernel, and the instrument/spacecraft frame kernel.  These files\n\
      provide the supporting ancillary data to properly query this\n\
      C-kernel for attitude content.  They should be the same kernels that\n\
      were originally used to initialize the image.\n\
\n\
      Segments in this file are actually individual ISIS files where the\n\
      internally cached SPICE data is extracted and transformed into the\n\
      appropriate content to satisfy NAIF\'s SPICE kernel storage\n\
      requirements.  The contents of this kernel are summarized below.\n\
\n\
User Comments\n\
-----------------------------------------------------------------------\n\
\n";

// Now write any user comments provided
   if ( !comFile.isEmpty() ) {

     // Write user comment header
     TextFile txt(comFile);
     QString cline;
     while ( txt.GetLineNoFilter(cline )) {
       comment << cline.toStdString() << "\n";
     }
   }
   else {
     // None provided
     comment << "\
      NONE\n";
   }

   //  Finish comments for segement data
   comment << "\
\n\
Segment (by file) Summary\n\
-----------------------------------------------------------------------\n\
\n\
      The follow sections describe each segment in this CK kernel.  Each\n\
      segment is a file in the input list.  When running ISIS spiceinit,\n\
      the kernels listed for each file should be supplied to ensure proper\n\
      geometry can be reproduced accurately.\n\
\n";

   return (QString(comment.str().c_str()));
}

};  // namespace Isis
