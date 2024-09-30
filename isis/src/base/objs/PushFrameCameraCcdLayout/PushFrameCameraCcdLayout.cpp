/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <iostream>
#include <sstream>

#include <SpiceUsr.h>
#include <SpiceZfc.h>
#include <SpiceZmc.h>

#include "FileName.h"
#include "IString.h"
#include "PushFrameCameraCcdLayout.h"
#include "NaifStatus.h"

namespace Isis {
  /** 
   * Push frame full CCD layout class
   *  
   */
  PushFrameCameraCcdLayout::PushFrameCameraCcdLayout() {
    m_ccdId = 1;
  }


  /** 
   * Push frame full CCD layout class for specific Id
   * 
   * @param ccId The NAIF ID of the CCD
   */
  PushFrameCameraCcdLayout::PushFrameCameraCcdLayout(const int ccdId) {
    m_ccdId = ccdId;
  }


  /**
   * Destructor
   */
  PushFrameCameraCcdLayout::~PushFrameCameraCcdLayout() {
  }


  /**
   * Add a NAIF kernel to the set of kernels that values will be pulled from.
   * 
   * @param kernel The path to the kernel file to load. The kernel name can
   *               contain a series of ?? characters to indicate version
   *               numbers. In this case the highest version numbered file will
   *               be added.
   * 
   * @return @b bool If the kernel was successfully loaded.
   */
  bool PushFrameCameraCcdLayout::addKernel(const QString &kernel) {
    FileName kern(kernel.toStdString());
    if ( kern.isVersioned()) {
      kern = kern.highestVersion();
    }
    m_kernels.Add(QString::fromStdString(kern.expanded()));
    int nloaded = m_kernels.Load();
    return (nloaded > 0);
  }


  /**
   * Return the number of samples in the CCD. Queries the NAIF keyword
   * INS(CCD NAIF ID)_FILTER_SAMPLES from the loaded kernels.
   * 
   * @return @b int The number of samples in the CCD.
   */
  int PushFrameCameraCcdLayout::ccdSamples() const {
    QString var = "INS" + QString::fromStdString(toString(m_ccdId)) + "_FILTER_SAMPLES";
    return (getSpiceInt(var));
  }


  /**
   * Return the number of lines in the CCD. Queries the NAIF keyword
   * INS(CCD NAIF ID)_FILTER_LINES from the loaded kernels.
   * 
   * @return @b int The number of lines in the CCD.
   */
  int PushFrameCameraCcdLayout::ccdLines() const {
    QString var = "INS" + QString::fromStdString(toString(m_ccdId)) + "_FILTER_LINES";
    return (getSpiceInt(var));
  }


  /**
   * Get the layout information for a framelet.
   * 
   * @param frameId The NAIF ID of the framelet.
   * @param name The name of the framelet. If empty, the loaded kernels will be
   *             queried for the name associated with the NAIF ID.
   * 
   * @return @b FrameletInfo The location and size of the framelet on the CCD.
   */
  PushFrameCameraCcdLayout::FrameletInfo PushFrameCameraCcdLayout::getFrameInfo(
                                            const int &frameId, 
                                             const QString &name) const {
    FrameletInfo finfo(frameId);
    finfo.m_filterName = name;

    QString base = "INS" + QString::fromStdString(toString(frameId));
    try {
      finfo.m_samples = getSpiceInt(base + "_FILTER_SAMPLES");
      finfo.m_lines = getSpiceInt(base + "_FILTER_LINES");
      finfo.m_startLine = getSpiceInt(base + "_FILTER_OFFSET");
    }
    catch (IException &e) {
      std::string msg = "Could not find layout information for framelet ["
                    + toString(frameId) + "].";
      throw IException(e, IException::Io, msg, _FILEINFO_);
    }
    finfo.m_startSample = 1;

    if ( finfo.m_filterName.isEmpty()) {
      try {
        finfo.m_filterName = getSpiceString(base + "_FILTER_NAME");
      } catch (IException &ie) {
        // noop - leave name empty
      }
    }

    return (finfo);
  }


  /**
   * Query the loaded kernels for an integer valued keyword.
   * 
   * @param var The keyword to find.
   * @param index The index of the value to take from the keyword.
   * 
   * @return @b int The keyword value
   * 
   * @see gipool_c
   */
  int PushFrameCameraCcdLayout::getSpiceInt(const QString &var, 
                                          const int index) const {
    SpiceBoolean found = false;
    SpiceInt numValuesRead;
    SpiceInt kernelValue;
    gipool_c(var.toLatin1().data(), (SpiceInt) index, 1, &numValuesRead,
             &kernelValue, &found);

    // Gotta throw an error here if not found
    if (!found) {
      NaifStatus::CheckErrors();
      std::string msg = "Can not find [" + var.toStdString() + "] in text kernels";
      throw IException(IException::Io, msg, _FILEINFO_);
     }

      return ( (int) kernelValue );
  }


  /**
   * Query the loaded kernels for a double valued keyword.
   * 
   * @param var The keyword to find.
   * @param index The index of the value to take from the keyword.
   * 
   * @return @b double The keyword value
   * 
   * @see gdpool_c
   */
  double PushFrameCameraCcdLayout::getSpiceDouble(const QString &var, 
                                               const int index) const {
    SpiceBoolean found = false;
    SpiceInt numValuesRead;
    SpiceDouble kernelValue;
    gdpool_c(var.toLatin1().data(), (SpiceInt) index, 1, &numValuesRead,
             &kernelValue, &found);

    // Gotta throw an error here if not found
    if (!found) {
      NaifStatus::CheckErrors();
      std::string msg = "Can not find [" + var.toStdString() + "] in text kernels";
      throw IException(IException::Io, msg, _FILEINFO_);
     }

      return ( (double) kernelValue );
  }


  /**
   * Query the loaded kernels for a string valued keyword.
   * 
   * @param var The keyword to find.
   * @param index The index of the value to take from the keyword.
   * 
   * @return @b QString The keyword value
   * 
   * @see gcpool_c
   */
  QString PushFrameCameraCcdLayout::getSpiceString(const QString &var, 
                                                   const int index) const {
    SpiceBoolean found = false;
    SpiceInt numValuesRead;
    char kernelValue[512];
    gcpool_c(var.toLatin1().data(), (SpiceInt) index, 1, sizeof(kernelValue),
             &numValuesRead, kernelValue, &found);

    // Gotta throw an error here if not found
    if (!found) {
      NaifStatus::CheckErrors();
      std::string msg = "Can not find [" + var.toStdString() + "] in text kernels";
      throw IException(IException::Io, msg, _FILEINFO_);
     }

      return ( QString(kernelValue) );
  }

};
