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
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <QFile>

#include <SpiceUsr.h>

#include "CkKernelWriter.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "NaifStatus.h"
#include "Pvl.h"
#include "CkSpiceSegment.h"
#include "TextFile.h"

using namespace std;

namespace Isis {

 CkKernelWriter::CkKernelWriter() {
   init();
 }

 CkKernelWriter::CkKernelWriter(const QString &kfile, const int &csize,
                                const int &cktype) {
   init();
   setCommentSize(csize);
   setType(cktype);
   open(kfile);
 }

 void CkKernelWriter::init() {
   _ckType = DefaultCkType;
   _handle = 0;
   _comSize = DefaultCommentSize;
   _comCharsWritten = 0;
 }

 void CkKernelWriter::setType(const int cktype) {
   if ( (cktype < 1) || (cktype > 3) ) {
     string mess = "Invalid CK kernel type: " + IString(cktype);
     throw IException(IException::Programmer, mess, _FILEINFO_);
   }
 }
 void CkKernelWriter::setCommentSize(const int &csize) {
   if ( _handle == 0 ) { _comSize = csize; }
   return;
 }

 bool CkKernelWriter::addComment(const QString &comment) {
   return (writeComment(comment));
 }

 bool CkKernelWriter::addCommentFile(const QString &comfile) {
   TextFile t(comfile);
   QString comment;
   bool lastWrite(true);
   while ( t.GetLine(comment, false) ) {
     if ( comment.size() == 0 ) comment.push_back('\n');
     lastWrite = writeComment(comment);
   }
   return (lastWrite);
 }

 void CkKernelWriter::open(const QString &kfile,
                           const QString &intCkName) {
   NaifStatus::CheckErrors();
   FileName kf(kfile.toStdString());
   if ( kf.fileExists() ) {
     QString full_kf = QString::fromStdString(kf.expanded());
     QFile::remove(full_kf);
   }
   SpiceInt  myHandle;
   ckopn_c(kf.expanded().c_str(), intCkName.toLatin1().data(), _comSize, &myHandle);
   _handle = myHandle;

   NaifStatus::CheckErrors();
   return;
 }

 void CkKernelWriter::write(const CkSpiceSegment &segment) const {
   switch ( _ckType ) {
     case 1:
       writeCk1(segment);
       break;
     case 2:
       writeCk2(segment);
       break;
     case 3:
       writeCk3(segment);
       break;
     default:
       string mess = "Selected CK type (" + IString(_ckType) +
                     ") invalid - must be 1, 2 or 3";
       throw IException(IException::Programmer, mess, _FILEINFO_);
   }
   return;

 }

 /** Writes a comment to an opened NAIF kernel file */
 bool CkKernelWriter::writeComment(const QString &comment) const {
   if ( _handle == 0 ) {
     string mess = "Comments cannot be written as the file is not open";
     throw IException(IException::Programmer, mess, _FILEINFO_);
   }

   // Trap errors so they are not fatal if the comment section fills up.
   // Calling environments can decide how to handle it.
   try {
     QString commOut;
     NaifStatus::CheckErrors();
     for ( int i = 0 ; i < comment.size() ; i++ ) {
        if ( comment[i] == '\n' ) {
          while ( commOut.size() < 2 ) { commOut.append(" "); }
          dafac_c(_handle, 1, commOut.size(), commOut.toLatin1().data());
          _comCharsWritten += commOut.size();
          NaifStatus::CheckErrors();
          commOut.clear();
        }
        else {
          commOut.push_back(comment[i]);
        }
     }

     // See if there is residual to write
     if ( commOut.size() > 0 ) {
       while ( commOut.size() < 2 ) { commOut.append(" "); }
       dafac_c(_handle, 1, commOut.size(), commOut.toLatin1().data());
       _comCharsWritten += commOut.size();
       NaifStatus::CheckErrors();
     }
   } catch (IException &) {
     return (false);
   }

   return (true);
 }

 /** Close an opened kernel file */
 void CkKernelWriter::close() {
   if ( _handle != 0 ) {
     NaifStatus::CheckErrors();
     ckcls_c(_handle);
     NaifStatus::CheckErrors();
     _handle = 0;
   }
   return;
 }

  void CkKernelWriter::writeCk1(const CkSpiceSegment &segment) const {

    const CkSpiceSegment::SVector &sclks = segment.SCLKTimes();
    const CkSpiceSegment::SMatrix &quats = segment.Quaternions();
    ConstSpiceDouble *avvs(0);
    SpiceBoolean hasAvvs(SPICEFALSE);
    if ( segment.hasAngularVelocities() ) {
      avvs = segment.AngularVelocities()[0];
      hasAvvs = SPICETRUE;
    }
    QString refFrame = segment.ReferenceFrame();
    QString segId = segment.Id();

    int nrecs = segment.size();

    NaifStatus::CheckErrors();
    ckw01_c(_handle, sclks[0], sclks[nrecs-1], segment.InstCode(),
             refFrame.toLatin1().data(), hasAvvs, segId.toLatin1().data(), nrecs, &sclks[0],
             quats[0], avvs);
    NaifStatus::CheckErrors();
    return;
  }

  void CkKernelWriter::writeCk2(const CkSpiceSegment &segment) const {

    if ( !segment.hasAngularVelocities() ) {
      std::string mess = "Type 2 CK kernels require angular velocities";
      throw IException(IException::User, mess, _FILEINFO_);
    }

    const CkSpiceSegment::SVector &sclks = segment.SCLKTimes();
    const CkSpiceSegment::SMatrix &quats = segment.Quaternions();
    const CkSpiceSegment::SMatrix &avvs = segment.AngularVelocities();

    QString refFrame = segment.ReferenceFrame();
    QString segId = segment.Id();

    int nrecs = segment.size();

    CkSpiceSegment::SVector stops(nrecs);
    for ( int i = 0 ; i < nrecs-1 ; i++) {
      stops[i] = sclks[i+1];
    }
    stops[nrecs-1] = sclks[nrecs-1];
    CkSpiceSegment::SVector rates(nrecs, segment.TickRate());
    NaifStatus::CheckErrors();
    ckw02_c(_handle, sclks[0], sclks[nrecs-1], segment.InstCode(),
             refFrame.toLatin1().data(), segId.toLatin1().data(), nrecs, &sclks[0],
             &stops[0], quats[0], avvs[0], &rates[0]);
    NaifStatus::CheckErrors();
    return;
  }


  void CkKernelWriter::writeCk3(const CkSpiceSegment &segment) const {

    const CkSpiceSegment::SVector &sclks = segment.SCLKTimes();
    const CkSpiceSegment::SMatrix &quats = segment.Quaternions();
    ConstSpiceDouble *avvs(0);
    SpiceBoolean hasAvvs(SPICEFALSE);
    if ( segment.hasAngularVelocities() ) {
      avvs = segment.AngularVelocities()[0];
      hasAvvs = SPICETRUE;
    }
    QString refFrame = segment.ReferenceFrame();
    QString segId = segment.Id();

    int nrecs = segment.size();

    segment.FurnshKernelType("FK");
    NaifStatus::CheckErrors();
    ckw03_c(_handle, sclks[0], sclks[nrecs-1], segment.InstCode(),
             refFrame.toLatin1().data(), hasAvvs, segId.toLatin1().data(), nrecs, &sclks[0],
             quats[0], avvs, 1, &sclks[0]);
    segment.UnloadKernelType("FK");

    NaifStatus::CheckErrors();
    return;
  }


};  // namespace Isis
