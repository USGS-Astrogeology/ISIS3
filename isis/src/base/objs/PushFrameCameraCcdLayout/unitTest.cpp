/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <QDebug>

#include "IException.h"
#include "Preference.h"

#include "PushFrameCameraCcdLayout.h"

using namespace std;
using namespace Isis;

int main(void) {
  Preference::Preferences(true);

  qDebug() << "Unit Test for PushFrameCameraCcdLayout::FrameletInfo...";
  qDebug() << "";
  try {
    qDebug() << "Create a default FrameletInfo object";
    PushFrameCameraCcdLayout::FrameletInfo defaultInfo;
    qDebug() << "framelet ID:" << defaultInfo.m_frameId;
    qDebug() << "framelet name:" << defaultInfo.m_filterName;
    qDebug() << "framelet start sample:" << defaultInfo.m_startSample;
    qDebug() << "framelet start line:" << defaultInfo.m_startLine;
    qDebug() << "framelet samples:" << defaultInfo.m_samples;
    qDebug() << "framelet lines:" << defaultInfo.m_lines;
    qDebug() << "";

    qDebug() << "Create a FrameletInfo object for a specific ID";
    PushFrameCameraCcdLayout::FrameletInfo numberedInfo(42);
    qDebug() << "framelet ID:" << numberedInfo.m_frameId;
    qDebug() << "framelet name:" << numberedInfo.m_filterName;
    qDebug() << "framelet start sample:" << numberedInfo.m_startSample;
    qDebug() << "framelet start line:" << numberedInfo.m_startLine;
    qDebug() << "framelet samples:" << numberedInfo.m_samples;
    qDebug() << "framelet lines:" << numberedInfo.m_lines;
    qDebug() << "";
  }
  catch(IException &e) {
    e.print();
  }

  qDebug() << "Unit Test for PushFrameCameraCcdLayout...";
  qDebug() << "";
  try {
    qDebug() << "Create a default PushFrameCameraCcdLayout";
    PushFrameCameraCcdLayout defaultLayout;
    qDebug() << "Try adding a kernel that does not exist";
    bool loaded = defaultLayout.addKernel("not_a_kernel_file");
    qDebug() << "Kernel file loaded?" << loaded;
    qDebug() << "";

    qDebug() << "Create the JunoCam layout";
    PushFrameCameraCcdLayout junoLayout(-61500);
    qDebug() << "Load the JunoCam kernels";
    if (!junoLayout.addKernel("$juno/kernels/ik/juno_junocam_v??.ti")) {
      std::string msg = "Failed to load the JunoCam Instrument Kernel.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    if (!junoLayout.addKernel("$juno/kernels/iak/junoAddendum???.ti")) {
      std::string msg = "Failed to load the JunoCam Instrument Addendum.";
      throw IException(IException::Io, msg, _FILEINFO_);
    }
    qDebug() << "JunoCam CCD samples:" << junoLayout.ccdSamples();
    qDebug() << "JunoCam CCD lines:" << junoLayout.ccdLines();
    qDebug() << "";
    qDebug() << "Get the METHANE filter layout";
    PushFrameCameraCcdLayout::FrameletInfo methane_info = junoLayout.getFrameInfo(-61504);
    qDebug() << "METHANE filter ID:" << methane_info.m_frameId;
    qDebug() << "METHANE filter name:" << methane_info.m_filterName;
    qDebug() << "METHANE filter start sample:" << methane_info.m_startSample;
    qDebug() << "METHANE filter start line:" << methane_info.m_startLine;
    qDebug() << "METHANE filter samples:" << methane_info.m_samples;
    qDebug() << "METHANE filter lines:" << methane_info.m_lines;
    qDebug() << "";
    qDebug() << "Get the METHANE filter layout but give it a different name";
    PushFrameCameraCcdLayout::FrameletInfo named_info = junoLayout.getFrameInfo(-61504, "methane");
    qDebug() << "METHANE filter ID:" << named_info.m_frameId;
    qDebug() << "METHANE filter name:" << named_info.m_filterName;
    qDebug() << "METHANE filter start sample:" << named_info.m_startSample;
    qDebug() << "METHANE filter start line:" << named_info.m_startLine;
    qDebug() << "METHANE filter samples:" << named_info.m_samples;
    qDebug() << "METHANE filter lines:" << named_info.m_lines;
    qDebug() << "";
    qDebug() << "Attempt to get the layout for a filter that doesn't exist";
    try {
      junoLayout.getFrameInfo(-61509);
    }
    catch(IException &e) {
      e.print();
    }
  }
  catch(IException &e) {
    e.print();
  }
}
