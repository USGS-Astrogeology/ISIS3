#ifndef KernelWriter_h
#define KernelWriter_h
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
 *   $Id: KernelWriter.h 6715 2016-04-28 17:58:43Z tsucharski@GS.DOI.NET $
 */
#include <string>

#include <SpiceUsr.h>

#include "Commentor.h"
#include "IException.h"
#include "NaifStatus.h"

namespace Isis {

/**
 * @brief Support writting of kernels to NAIF SPICE files
 *
 * This class is designed as the base class to support the fundamentals of
 * opening/creating a NAIF kernel file for writing of NAIF kernel segments
 * (typically CK and SPK types).
 *
 * As such, this object provides the ability to create general files, specify
 * comment section size, get the I/O handle associated with the output file,
 * write comments to the kernel and then close the file when you are done.
 *
 * The thing you may see as missing is the explicit write method.  Due to the
 * structure of the NAIF writing procedure, each kernel type has its own routine
 * that has different arguments.  This makes it "messy" to try and support
 * writing of all CK/SPK types directly in this class.  The template class K
 * must provide some internal virtual methods.  These methods are:
 * @code
 *   int k_open(const QString &fname, const int commnt_size);
 *   void k_write(int _handle, const K kernels);
 *   void k_close(int handle);
 * @endcode
 *
 * The class K may also provide a WriteComment(const int handle, const
 * QString &comment) method that will write comments to the output kernel
 * file if the provided one is not adequate.
 *
 * @author 2010-11-10 Kris Becker
 * @internal
 * @history 2010-12-09 Kris Becker Added more documentation
 */
template <class K>
class KernelWriter {
  public:
    typedef typename K::SegmentType SegmentType;

    /** Default constructor */
    KernelWriter() :_handle(0) { }
    /** Destructor  */
    virtual ~KernelWriter() { }

    /** Open a kernel file using virtual method provided in K */
    void open(const QString &kfile, const int &commnt_size = 5120) {
      _handle = k_open(kfile, commnt_size);

    }

    /** Write header with comments provided   */
    void header(const QString &comment) {
      WriteComment(_handle, comment);
    }

    /** Write a set of kernel segments from teh Kernels segment container */
    void write(const K &kernels)  {
      k_write(_handle, kernels);
    }

    /**
     * @brief Write a set of kernels to file and comment file
     *
     * This method is the typical one called to write a complete kernel file to
     * the named file.  It will write each kernel from list provided in the K
     * list of kernels.
     *
     * @param K Kernel container with segments to write
     * @param QString Name of file to write kernel to
     * @param QString Name
     */
    void write(const K &kernels, const QString &kfile,
               const QString &comfile = "") {
      QString comments = getComment(kernels, comfile);
      open(kfile, comments.size() + 512);
      header(comments);  // Writes header
      write(kernels);
      close();
      return;
    }


    /** Generic close method simply calls specified method */
    void close() {
      k_close(_handle);
    }

    /** Accumulate comment from K object and individed set */
    QString getComment(const K &kernels, const QString &comfile) {
      Commentor<SegmentType> commentor;
      commentor.setCommentHeader(k_header(comfile));
      kernels.Accept(commentor);
      return (commentor.comments());
    }

  protected:
    /** These virtual methods must be provided by the K class */
    virtual int k_open(const QString &kfile, const int &comsize = 512) = 0;
    virtual void k_write(const SpiceInt &handle, const K &kernels) = 0;
    virtual void k_close(SpiceInt &handle) = 0;
    virtual QString k_header(const QString &comfile = "") const = 0;

  private:
    SpiceInt    _handle;    ///< SPICE file handle


    /**
     * @brief Write comments to output NAIF SPICE kernel.
     *
     * @return bool Returns success if so.
     */
    virtual bool WriteComment(SpiceInt handle, const QString &comment)
                              const {
      if ( handle == 0 ) {
        std::string mess = "Comments cannot be written as the file is not open";
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
             dafac_c(handle, 1, commOut.size(), commOut.toLatin1().data());
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
          dafac_c(handle, 1, commOut.size(), commOut.toLatin1().data());
          NaifStatus::CheckErrors();
        }
      }
      catch (IException &) {
        return (false);
      }

      return (true);
    }

};

};     // namespace Isis
#endif


