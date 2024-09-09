#ifndef OriginalLabel_h
#define OriginalLabel_h

/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Blob.h"
#include "Pvl.h"
#include <QString>

namespace Isis {
  /**
   * @brief Read and store original labels.
   *
   * This class provides a means to read and store the labels from the original
   * source.  The labels can be retreived in a Pvl object.
   *
   * @ingroup LowLevelCubeIO
   *
   * @author 2006-02-23 Robert Wallace and Brian Lipkowitz
   *
   * @internal
   *   @history 2006-06-13 Brendan George - Modified to throw error when called
   *                           file does not contain an Original Label
   *   @history 2006-12-11 Kris Becker - Fixed bug in WriteInit method using a
   *                           temporary string to reference a char pointer
   *                           to its contents.  The string remain after the
   *                           reference or its toast.
   *   @history 2008-06-18 Steven Koechle - Fixed Documentation Errors
   *
   */
  class OriginalLabel {
    public:
      OriginalLabel();
      OriginalLabel(const QString &file);
      OriginalLabel(Isis::Blob &blob);
      OriginalLabel(Pvl pvl);
      ~OriginalLabel();

      // Return the original labels
      Pvl ReturnLabels() const;
      Isis::Blob toBlob();

    protected:
      // prepare data for writing
      void WriteInit();

    private:
      // labels of original source
      Pvl m_originalLabel;
      void fromBlob(Isis::Blob blob);
  };
};

#endif
