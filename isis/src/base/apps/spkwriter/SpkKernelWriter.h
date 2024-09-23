#ifndef SpkKernelWriter_h
#define SpkKernelWriter_h
/**
 * @file
 * $Revision: 6715 $
 * $Date: 2016-04-28 10:58:43 -0700 (Thu, 28 Apr 2016) $
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
 *
 *   $Id: SpkKernelWriter.h 6715 2016-04-28 17:58:43Z tsucharski@GS.DOI.NET $
 */
#include <string>
#include "SpiceKernel.h"
#include "KernelWriter.h"
#include "SpkSpiceSegment.h"
#include "SpkSegment.h"
#include "FileName.h"
namespace Isis {

typedef SpiceKernel<SpkSegment> SpkKernel;

/**
 * @brief Support writing of SPK kernels to NAIF SPICE files
 *
 * This class is designed to support the fundamentals of opening/creating a NAIF
 * SPK kernel file for writing of SPK segments.  There are currently up to 5
 * different types of SPK kernels that NAIF supports in its toolkit.  We are
 * mainly concerned with creation of type 9 and 13 SPK kernels.
 *
 * As such, this object provides the ability to create general SPK files,
 * specify comment section size, get the I/O handle associated with the output
 * file, write comments to the kernel and then close the file when you are
 * done.
 *
 * The thing you may see as missing is the write method.  Due to the structure
 * of the NAIF SPK writing procedure, each SPK type has its own routine that
 * has different arguments.  This makes it "messy" to try and support writing
 * of all SPK types directly in this class.  It makes more sense, to me anyway,
 * that the environment that contains the data to write can better provide the
 * data required to properly write the SPK segment.  See the implementation of
 * SpkSegment which provides this procedure.  It will have a method that looks
 * like SpkSegment::write(SpkKernelWriter &writer).  This class exposes the
 * handle need to write the segment and the addComment() method to add details
 * in the kernel file regarding the segment being written.
 *
 * @author 2011-05-02 Kris Becker
 *
 * @internal
 * @history 2013-12-19 Kris Becker Add user comments to header section rather
 *                                 than replace generic comments.
 */

class SpkKernelWriter : public KernelWriter<SpkKernel> {
  public:
    SpkKernelWriter() { setType(DefaultSpkType); }
    SpkKernelWriter(const int ktype) { setType(ktype); }
    virtual ~SpkKernelWriter() { }

    /** Return the SPK kernel type to write */
    int Type() const {  return (_spkType); }

    /**
     * @brief Set the kernel type to write
     *
     * Validation on the type will be performed.
     *
     * @param spktype Type of NAIF SPK kernel to write
     */
    void setType(const int spktype) {
      if ( (spktype != 9) && (spktype != 13) ) {
        std::string mess = "SPK kernel type " + Isis::toString(spktype) +
                           " not valid/supported -  must 9 or 13";
        throw IException(IException::User, mess, _FILEINFO_);
      }
      _spkType = spktype;
    }


  protected:
    int k_open(const QString &kfile, const int &comsize = 512) {
      FileName kf(kfile.toStdString());
      if ( kf.fileExists() ) {
        QString full_kf = QString::fromStdString(kf.expanded());
        std::remove(full_kf.toLatin1().data());
      }
      SpiceInt  myHandle;

      NaifStatus::CheckErrors();
      spkopn_c(kf.expanded().c_str(), "USGS_SPK_FILE", comsize, &myHandle);
      NaifStatus::CheckErrors();
      return (myHandle);
    }

    QString k_header(const QString &comfile = "") const;

    void k_write(const SpiceInt &handle, const SpkKernel &kernels) {
      if ( _spkType == 9 ) {
        kernels.Accept(WriteSpk9<SpkSegment>(handle));
      }
      else if ( _spkType == 13 ) {
        kernels.Accept(WriteSpk13<SpkSegment>(handle));
      }
      else {
        setType(_spkType);
      }
      return;
    }

    void  k_close(SpiceInt &handle) {
      if ( handle != 0 ) {
        NaifStatus::CheckErrors();
        spkcls_c(handle);
        NaifStatus::CheckErrors();
      }
      handle = 0;
    }

  private:
    enum { DefaultSpkType = 13 };
    int          _spkType;

    template <class K>
    struct WriteSpk9 {
      typedef typename K::SVector SVector;
      typedef typename K::SMatrix SMatrix;
      WriteSpk9(SpiceInt handle) : _handle(handle) {  }
      virtual ~WriteSpk9() { }
      void operator()(const K &segment) const {
        SpiceInt body   = segment.BodyCode();
        SpiceInt center = segment.CenterCode();
        QString   frame  = segment.ReferenceFrame();
        QString   segId  = segment.Id();

        const SVector &epochs = segment.Epochs();
        const SMatrix &states = segment.States();
        int degree = segment.Degree();

        int nrecs = segment.size();
        segment.LoadKernelType("FK");
        NaifStatus::CheckErrors();

        spkw09_c(_handle, body, center, frame.toLatin1().data(), epochs[0], epochs[nrecs-1],
                 segId.toLatin1().data(), degree, nrecs, states[0], &epochs[0]);

        NaifStatus::CheckErrors();
        segment.UnloadKernelType("FK");
        return;
      }
      private:
        SpiceInt _handle;
    };

    template <class K>
    struct WriteSpk13 {
      typedef typename K::SVector SVector;
      typedef typename K::SMatrix SMatrix;

      WriteSpk13(SpiceInt handle) : _handle(handle) { }
      virtual ~WriteSpk13() { }
      void operator()(const K &segment) const {
        // Collect frames
        SpiceInt body   = segment.BodyCode();
        SpiceInt center = segment.CenterCode();
        QString   frame  = segment.ReferenceFrame();
        QString   segId  = segment.Id();

        // Collect data
        const SVector &epochs = segment.Epochs();
        const SMatrix &states = segment.States();
        int degree = segment.Degree();
        int nrecs = segment.size();

        // Ensure the FK is loaded
        segment.LoadKernelType("FK");
        NaifStatus::CheckErrors();
        spkw13_c(_handle, body, center, frame.toLatin1().data(), epochs[0], epochs[nrecs-1],
                 segId.toLatin1().data(), degree, nrecs, states[0], &epochs[0]);
        NaifStatus::CheckErrors();
        segment.UnloadKernelType("FK");
        return;
      }

      private:
        SpiceInt _handle;
    };

};

}     // namespace Isis
#endif
