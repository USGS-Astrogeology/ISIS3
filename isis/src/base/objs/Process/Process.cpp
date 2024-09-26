/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <sstream>
#include <fstream>

#include <QSet>

#include "SessionLog.h"
#include "Process.h"
#include "FileName.h"
#include "Message.h"
#include "IException.h"
#include "IString.h"
#include "Preference.h"
#include "Application.h"
#include "History.h"
#include "OriginalLabel.h"
#include "LineManager.h"

using namespace std;
namespace Isis {


  //! Constructs a Process Object
  Process::Process() {
    m_ownedCubes = NULL;

    p_progress = new Isis::Progress();
    p_progress->SetText("Working");
    p_propagateLabels = true;
    p_propagateTables = true;
    p_propagatePolygons = true;
    p_propagateHistory = true;
    p_propagateOriginalLabel = true;

    m_ownedCubes = new QSet<Cube *>;
  }

  //! Destroys the Process Object. It will close all opened cubes
  Process::~Process() {
    EndProcess();
    delete p_progress;

    delete m_ownedCubes;
    m_ownedCubes = NULL;
  }


  /**
   * Opens an input cube specified by the programmer and verifies requirements
   * are met.
   *
   * @param fname Programmer specified work file. For example, "myfile.cub".
   *
   * @param att  The cube attributes to use when opening the input cube.
   *
   * @param requirements Requirements to check that the input cube meets.
   *                     See CheckRequirements().
   *
   * @return Cube*
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *Process::SetInputCube(const QString &fname,
                                    const Isis::CubeAttributeInput &att,
                                    int requirements) {
    Isis::Cube *cube = new Isis::Cube;
    if(att.bands().size() != 0) {
      vector<QString> lame = att.bands();
      cube->setVirtualBands(lame);
    }

    try {
      if(requirements & Isis::ReadWrite) {
        cube->open(fname, "rw");
      }
      else {
        // Make sure attributes don't get processed twice
        cube->open(QString::fromStdString(FileName(fname.toStdString()).expanded()));
      }
    }
    catch(IException &e) {
      delete cube;
      throw;
    }

    CheckRequirements(cube, requirements);

    // Everything is good so save the cube on the stack
    AddInputCube(cube);
    return cube;
  }


  /**
   * Set the InputCube vector to an opened Cube which was dynamically allocated.
   * This is used if there already exists a valid opened cube
   *
   * @author Sharmila Prasad (5/7/2011)
   *
   * @param inCube - Pointer to input Cube
   *
   * @param requirements Requirements to check that the input cube meets.
   *                     See CheckRequirements().
   */
  void Process::SetInputCube(Isis::Cube *inCube, const int requirements)
  {
    if(inCube != NULL && inCube->isOpen()) {
      CheckRequirements(inCube, requirements);
      AddInputCube(inCube, false);
    }
    else {
      std::string message = "Input cube does not exist";
      throw IException(IException::User, message, _FILEINFO_);
    }
  }


  /**
   * Opens an input cube specified by the user and verifies requirements are met.
   *
   * @return Cube*
   *
   * @param parameter User parameter to obtain file to open. Typically, the value
   *                  is "FROM". For example, the user can specify on the command
   *                  line FROM=myfile.cub and this method will attempt to open
   *                  the cube "myfile.cub" if the parameter was set to "FROM".
   *
   *
   * @param requirements Requirements to check that the input cube meets.
   *                     See CheckRequirements().
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *Process::SetInputCube(const QString &parameter,
                                    const int requirements) {
    QString fname = Application::GetUserInterface().GetCubeName(parameter);
    Isis::CubeAttributeInput &att = Application::GetUserInterface().GetInputAttribute(parameter);
    return SetInputCube(QString::fromStdString(FileName(fname.toStdString()).expanded()), att, requirements);
  }


  /**
   * Allocates a user-specified output cube whose size matches the first input
   * cube.
   *
   * @return Cube*
   *
   * @param parameter User specified output file. For example, "TO" is a popular
   *                  user parameter. If the user specified TO=output.cub, then
   *                  this routine would allocate the file output.cub with size
   *                  specified by the first opened input cube. The output pixel
   *                  type will be propagated from the first loaded input cube or
   *                  will use the value in the application XML file for
   *                  pixelType.
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *Process::SetOutputCube(const QString &parameter) {
    // Make sure we have an input cube to get a default size from
    if(InputCubes.size() == 0) {
      std::string message = "No input images have been selected ... therefore";
      message += "the output image size can not be determined";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    int nl = InputCubes[0]->lineCount();
    int ns = InputCubes[0]->sampleCount();
    int nb = InputCubes[0]->bandCount();
    return SetOutputCube(parameter, ns, nl, nb);
  }


  /**
   * Allocates a user-specified output cube whose size matches the first input
   * cube.
   *
   * @return Cube*
   *
   * @param parameter User specified output file. For example, "TO" is a popular
   *                  user parameter. If the user specified TO=output.cub, then
   *                  this routine would allocate the file output.cub with size
   *                  specified by the first opened input cube. The output pixel
   *                  type will be propagated from the first loaded input cube or
   *                  will use the value in the application XML file for
   *                  pixelType.
   *
   * @param ui A user interface used to get the attributes needed for SetOutputCube.
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *Process::SetOutputCubeStretch(const QString &parameter, UserInterface *ui) {
    // Make sure we have an input cube to get a default size from
    if(InputCubes.size() == 0) {
      std::string message = "No input images have been selected ... therefore";
      message += "the output image size can not be determined";
      throw IException(IException::Programmer, message, _FILEINFO_);
    }

    int nl = InputCubes[0]->lineCount();
    int ns = InputCubes[0]->sampleCount();
    int nb = InputCubes[0]->bandCount();
    return SetOutputCubeStretch(parameter, ns, nl, nb, ui);
  }


  /**
   * Allocates a user specified output cube whose size is specified by the
   * programmer.
   *
   * @return Cube*
   *
   * @param parameter User specified output file. For example, "TO" is a popular
   *                  user parameter. If the user specified TO=output.cub, then
   *                  this routine would allocate the file output.cub with size
   *                  specified by the first opened input cube. The output pixel
   *                  type will be propagated from the first loaded input cube or
   *                  will use the value in the application XML file for
   *                  pixelType.
   *
   * @param ns Number of samples to allocate
   *
   * @param nl Number of lines to allocate
   *
   * @param nb Number of bands to allocate
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *Process::SetOutputCube(const QString &parameter, const int ns,
                                     const int nl, const int nb) {
    // Make sure we have good dimensions
    if((ns <= 0) || (nl <= 0) || (nb <= 0)) {
      ostringstream message;
      message << "Invalid cube size specifications [ns=" << ns << ",nl=" << nl
              << ",nb=" << nb << "]";
      throw IException(IException::Programmer, message.str().c_str(), _FILEINFO_);
    }
    QString fname;
    Isis::CubeAttributeOutput atts;
    fname = Application::GetUserInterface().GetCubeName(parameter);
    atts = Application::GetUserInterface().GetOutputAttribute(parameter);
    return SetOutputCube(fname, atts, ns, nl, nb);
}


/**
 * Allocates a user specified output cube whose size is specified by the
 * programmer.
 *
 * @return Cube*
 *
 * @param parameter User specified output file. For example, "TO" is a popular
 *                  user parameter. If the user specified TO=output.cub, then
 *                  this routine would allocate the file output.cub with size
 *                  specified by the first opened input cube. The output pixel
 *                  type will be propagated from the first loaded input cube or
 *                  will use the value in the application XML file for
 *                  pixelType.
 *
 * @param ns Number of samples to allocate
 *
 * @param nl Number of lines to allocate
 *
 * @param nb Number of bands to allocate
 *
 * @param ui A user interface used to get the attributes needed. If null, the
 *           user interface will be obtained from the application.
 *
 * @throws Isis::iException::Message
 */
Isis::Cube *Process::SetOutputCubeStretch(const QString &parameter, const int ns,
                                   const int nl, const int nb, UserInterface *ui) {
  // Make sure we have good dimensions
  if((ns <= 0) || (nl <= 0) || (nb <= 0)) {
    ostringstream message;
    message << "Invalid cube size specifications [ns=" << ns << ",nl=" << nl
            << ",nb=" << nb << "]";
    throw IException(IException::Programmer, message.str().c_str(), _FILEINFO_);
  }
  QString fname;
  Isis::CubeAttributeOutput atts;
  if(ui==nullptr){
    fname = Application::GetUserInterface().GetCubeName(parameter);
    atts = Application::GetUserInterface().GetOutputAttribute(parameter);
  }
  else{
    fname = ui->GetCubeName(parameter);
    atts = ui->GetOutputAttribute(parameter);
  }
  return SetOutputCube(fname, atts, ns, nl, nb);
}


  /**
   * Allocates a output cube whose name and size is specified by the programmer.
   *
   * @return Cube*
   *
   * @param fname Name of the output cube to allocate
   *
   * @param att The cube attributes to use when creating the output cube.
   *
   * @param ns Number of samples to allocate
   *
   * @param nl Number of lines to allocate
   *
   * @param nb Number of bands to allocate
   *
   * @throws Isis::iException::Message
   */
  Isis::Cube *Process::SetOutputCube(const QString &fname,
                                     const Isis::CubeAttributeOutput &att,
                                     const int ns, const int nl,
                                     const int nb) {
    // Make sure we have good dimensions
    if ((ns <= 0) || (nl <= 0) || (nb <= 0)) {
      ostringstream message;
      message << "Invalid cube size specifications [ns=" << ns << ",nl=" << nl
              << ",nb=" << nb << "]";
      throw IException(IException::Programmer, message.str().c_str(), _FILEINFO_);
    }

    // Setup the cube
    Isis::Cube *cube = new Isis::Cube;
    try {
      cube->setDimensions(ns, nl, nb);
      cube->setByteOrder(att.byteOrder());
      cube->setFormat(att.fileFormat());
      cube->setLabelsAttached(att.labelAttachment() == AttachedLabel);
      if(att.propagatePixelType()) {
        if(InputCubes.size() > 0) {
          cube->setPixelType(InputCubes[0]->pixelType());
        }
        else {
          std::string msg = "You told me to propagate PixelType from input to output";
          msg += " cube but there are no input cubes loaded";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
      }
      else {
        cube->setPixelType(att.pixelType());
      }

      if(att.propagateMinimumMaximum()) {
        if(cube->pixelType() == Isis::Real) {
          cube->setBaseMultiplier(0.0, 1.0);
        }
        else if(InputCubes.size() == 0) {
          std::string msg = "You told me to propagate base/multiplier from input to output";
          msg += " cube but there are no input cubes loaded";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        else if(cube->pixelType() >= InputCubes[0]->pixelType()) {
          double base = InputCubes[0]->base();
          double mult = InputCubes[0]->multiplier();
          cube->setBaseMultiplier(base, mult);
        }
        else if((cube->pixelType() != Isis::Real) &&
                (cube->pixelType() != Isis::UnsignedByte) &&
                (cube->pixelType() != Isis::UnsignedWord) &&
                (cube->pixelType() != Isis::SignedWord) &&
                (cube->pixelType() != Isis::UnsignedInteger) &&
                (cube->pixelType() != Isis::SignedInteger)) {
          std::string msg = "Looks like your refactoring to add different pixel types";
          msg += " you'll need to make changes here";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        else {
          std::string msg = "You've chosen to reduce your output PixelType for [" +
                       fname.toStdString() + "] you must specify the output pixel range too";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
      else {
        // Not propagating so either the user entered or the programmer did
        cube->setMinMax(att.minimum(), att.maximum());
      }

      if(InputCubes.size() > 0) {
        int needLabBytes = InputCubes[0]->labelSize(true) + (1024 * 6);
        if(needLabBytes > cube->labelSize()) {
          cube->setLabelSize(needLabBytes);
        }
      }

      // Allocate the cube
      cube->create(fname);

      // Transfer labels from the first input cube
      if((p_propagateLabels) && (InputCubes.size() > 0)) {
        Isis::PvlObject &incube =
            InputCubes[0]->label()->findObject("IsisCube");
        Isis::PvlObject &outcube = cube->label()->findObject("IsisCube");
        for(int i = 0; i < incube.groups(); i++) {
          outcube.addGroup(incube.group(i));
        }

        if (InputCubes[0]->label()->hasObject("NaifKeywords")) {
          cube->label()->addObject(
              InputCubes[0]->label()->findObject("NaifKeywords"));
        }
      }

      // Transfer tables from the first input cube
      if((p_propagateTables) && (InputCubes.size() > 0)) {
        Isis::Pvl &inlab = *InputCubes[0]->label();
        for(int i = 0; i < inlab.objects(); i++) {
          if(inlab.object(i).isNamed("Table")) {
            Isis::Blob t(inlab.object(i)["Name"], inlab.object(i).name());
            InputCubes[0]->read(t);
            cube->write(t);
          }
        }
      }

      // Transfer blobs from the first input cube
      if((p_propagatePolygons) && (InputCubes.size() > 0)) {
        Isis::Pvl &inlab = *InputCubes[0]->label();
        for(int i = 0; i < inlab.objects(); i++) {
          if(inlab.object(i).isNamed("Polygon")) {
            Isis::Blob t(inlab.object(i)["Name"], inlab.object(i).name());
            InputCubes[0]->read(t);
            cube->write(t);
          }
        }
      }

      // Transfer tables from the first input cube
      if((p_propagateOriginalLabel) && (InputCubes.size() > 0)) {
        Isis::Pvl &inlab = *InputCubes[0]->label();
        for(int i = 0; i < inlab.objects(); i++) {
          if(inlab.object(i).isNamed("OriginalLabel")) {
            Isis::OriginalLabel ol = InputCubes[0]->readOriginalLabel(QString::fromStdString(inlab.object(i)["Name"]));
            cube->write(ol);
          }
        }
      }

      // Transfer history from the first input cube
      WriteHistory(*cube);
    }
    catch (IException &e) {
      delete cube;
      throw;
    }
    // Everything is fine so save the cube on the stack
    AddOutputCube(cube);
    return cube;
  }


  /**
   * End the processing sequence and cleans up by closing cubes, freeing memory,
   * etc.
   *
   * @deprecated Please use Finalize()
   */
  void Process::EndProcess() {
    Process::Finalize();
  }


  /**
   * Cleans up by closing cubes and freeing memory for owned cubes.  Clears the
   * lists for all cubes.
   */
  void Process::Finalize() {
    ClearCubes();
  }


  void Process::AddInputCube(Cube *cube, bool owned) {
    InputCubes.push_back(cube);
    if (owned) m_ownedCubes->insert(cube);
  }


  void Process::AddOutputCube(Cube *cube, bool owned) {
    OutputCubes.push_back(cube);
    if (owned) m_ownedCubes->insert(cube);
  }


  /**
   * Checks to make sure the input cube meets the inputted requirements.
   *
   * @param cube Cube to check
   *
   * @param requirements Use to specify requirements for the input file. The
   *                     following requirments are checked against
   *                     1) ic_base::SizeMatch checks to make sure the input cube
   *                     has the same bands, samples, and lines as the first
   *                     input cube open using this method,
   *                     2) ic_base::SpatialMatch checks to make sure the input
   *                     cube has the same samples and lines as the first input
   *                     cube, 3) ic_base::Georeferenced checks to make sure
   *                     latitude/longitudes can be obtained from the input cube,
   *                     4) ic_base::FullyGeoreferenced checks to make sure
   *                     latitude/longitude, phase/incidence/emission, and other
   *                     geometric parameters can be obtained from the input
   *                     cube, 5) ic_base::OneBand checks to make sure the input
   *                     cube has exactly one band. Note, that these requirements
   *                     can be logically or'ed. For example,
   *                     ic_base::SpatialMatch | ic_base::georeferenced.
   *                     Defaults to 0
   *
   * @throws Isis::iException::Message
   */
  void Process::CheckRequirements(const Cube *cube, int requirements) {
    // Test for same size or one in all dimensions
    if(requirements & Isis::AllMatchOrOne) {
      if(InputCubes.size() > 0) {
        if(cube->lineCount() != 1) {
          if(cube->lineCount() != InputCubes[0]->lineCount()) {
            std::string message = "The number of lines in the secondary input cubes must match";
            message += " the primary input cube or be exactly one";
            throw IException(IException::User, message, _FILEINFO_);
          }
        }

        if(cube->sampleCount() != 1) {
          if(cube->sampleCount() != InputCubes[0]->sampleCount()) {
            std::string message = "The number of samples in the secondary input cubes must match";
            message += " the primary input cube or be exactly one";
            throw IException(IException::User, message, _FILEINFO_);
          }
        }
        if(cube->bandCount() != 1) {
          if(cube->bandCount() != InputCubes[0]->bandCount()) {
            std::string message = "The number of bands in the secondary input cubes must match";
            message += " the primary input cube or be exactly one";
            throw IException(IException::User, message, _FILEINFO_);
          }
        }

        // Do not do a spatial match if this flag was set
        requirements = requirements & !Isis::SpatialMatch;
      }
    }

    // Test for size match if requested
    if(requirements & Isis::SizeMatch) {
      if(InputCubes.size() > 0) {
        if(cube->lineCount() != InputCubes[0]->lineCount()) {
          std::string message = "The number of lines in the input cubes must match";
          throw IException(IException::User, message, _FILEINFO_);
        }
        if(cube->sampleCount() != InputCubes[0]->sampleCount()) {
          std::string message = "The number of samples in the input cubes must match";
          throw IException(IException::User, message, _FILEINFO_);
        }
        if(cube->bandCount() != InputCubes[0]->bandCount()) {
          std::string message = "The number of bands in the input cubes must match";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    }

    // Test for spatial match if requested
    if(requirements & Isis::SpatialMatch) {
      if(InputCubes.size() > 0) {
        if(cube->lineCount() != InputCubes[0]->lineCount()) {
          std::string message = "The number of lines in the input cubes must match";
          throw IException(IException::User, message, _FILEINFO_);
        }
        if(cube->sampleCount() != InputCubes[0]->sampleCount()) {
          std::string message = "The number of samples in the input cubes must match";
          throw IException(IException::User, message, _FILEINFO_);
        }
      }
    }

    // Test for one band
    if(requirements & Isis::OneBand) {
      if(cube->bandCount() != 1) {
        std::string message = "Input cube [" + cube->fileName().toStdString() + "] must have one band";
        throw IException(IException::User, message, _FILEINFO_);
      }
    }

    // Test for same bands or one band
    if(requirements & Isis::BandMatchOrOne) {
      if(cube->bandCount() != 1) {
        if(InputCubes.size() > 0) {
          if(cube->bandCount() != InputCubes[0]->bandCount()) {
            std::string message = "The number of bands in the secondary input cubes must match";
            message += " the primary input cube or be exactly one";
            throw IException(IException::User, message, _FILEINFO_);
          }
        }
      }
    }
  }


  /**
   * Close owned cubes from the list and clear the list
   */
  void Process::ClearCubes() {
    // Close the cubes
    ClearInputCubes();
    ClearOutputCubes();
    m_ownedCubes->clear();
  }


  /**
   * Close owned input cubes from the list and clear the list
   */
  void Process::ClearInputCubes() {
    // Close the cubes
    for (unsigned int i = 0; i < InputCubes.size(); i++) {
      if (m_ownedCubes->contains(InputCubes[i])) {
        InputCubes[i]->close();
        delete InputCubes[i];
      }
    }
    InputCubes.clear();
  }


  /**
   * Close owned output cubes from the list and clear the list
   */
  void Process::ClearOutputCubes() {
    // Close the cubes
    for (unsigned int i = 0; i < OutputCubes.size(); i++) {
      if (m_ownedCubes->contains(OutputCubes[i])) {
        OutputCubes[i]->close();
        delete OutputCubes[i];
      }
    }
    OutputCubes.clear();
  }


  /**
   * This method allows the programmer to turn on/off the propagation of labels
   * from the 1st input cube to any of the output cubes.  By default, propagation
   * occurs automatically in the Process class when a call to either of the
   * SetOutputCube methods is invoked. If the program * requires no propagation
   * then utilize this method. This method can be invoked between successive
   * calls of SetOutputCube so that some cube will have labels propagated while
   * others will not.
   *
   * @param prop Flag indicating if labels are be propagated to output cubes.
   */
  void Process::PropagateLabels(const bool prop) {
    p_propagateLabels = prop;
  }


  /**
   * This method allows the programmer to propagate labels from a specific
   * secondary cube.
   *
   * @param cube IString containing the name of the cube containing the labels
   *             to propagate.
   */
  void Process::PropagateLabels(const QString &cube) {
    // Open the Pvl file
    Isis::Pvl inLabels(cube.toStdString());

    // Loop for each output cube
    for(int i = 0; i < (int)OutputCubes.size(); i++) {
      Isis::PvlObject &inCubeLabels = inLabels.findObject("IsisCube");

      Isis::Pvl &outLabels(*OutputCubes[i]->label());
      Isis::PvlObject &outCubeLabels = outLabels.findObject("IsisCube");

      for(int g = 0; g < inCubeLabels.groups(); g++) {
        outCubeLabels.addGroup(inCubeLabels.group(g));
      }

      if (inLabels.hasObject("NaifKeywords")) {
        outLabels.addObject(inLabels.findObject("NaifKeywords"));
      }
    }
  }


  /**
   * This method allows the programmer to propagate input tables to the output
   * cube (default is true)
   *
   * @param prop Flag indicating if input tables are to be propagated to output
   *             cubes.
   */
  void Process::PropagateTables(const bool prop) {
    p_propagateTables = prop;
  }


  /**
   * Propagate the tables from the cube with the given filename to the output
   * cube.  This is done at the time this method is called, not during normal
   * processing. The names of the tables to propagate can be provided through the second paramter,
   * by specifing a list of table names. Not providing any list (or providing an empty list) will
   * propagate all tables.
   *
   * @param fromName QString of the name of the cube containing the tables to propagate.
   * @param tableNames List of QStrings of the names of the tables to propagate; default is empty,
   *                   which indicates that all tables will be propagated.
   *
   * @internal
   *   @history 2016-11-30 Ian Humphrey - Added tableNames parameter so that only specified tables
   *                           will be propagated when calling this method. Note that a default of
   *                           an empty QList is used to propagate all tables. References #4433.
   */
  void Process::PropagateTables(const QString &fromName, const QList<QString> &tableNames) {
    Cube *fromCube = new Isis::Cube;
    fromCube->open(fromName);
    const Pvl *fromLabels = fromCube->label();

    for (unsigned int i = 0; i < OutputCubes.size(); i++) {
      for (int j = 0; j < fromLabels->objects(); j++) {
        const PvlObject &object = fromLabels->object(j);

        if (object.isNamed("Table")) {
          if (tableNames.isEmpty() || tableNames.contains(QString::fromStdString(object["Name"]))) {
            Blob table(object["Name"], object.name());
            fromCube->read(table);
            OutputCubes[i]->write(table);
          }
        }
      }
    }
    fromCube->close();
    delete fromCube;
  }


  /**
   * This method allows the programmer to propagate input blobs to the output
   * cube (default is true)
   *
   * @param prop Flag indicating if input blobs are to be propagated to output
   *             cubes.
   */
  void Process::PropagatePolygons(const bool prop) {
    p_propagatePolygons = prop;
  }


  /**
   * This method allows the programmer to propagate history to the output cube
   * (default is true)
   *
   * @param prop Flag indicating if history is to be propagated to output cubes.
   */
  void Process::PropagateHistory(const bool prop) {
    p_propagateHistory = prop;
  }


  /**
  * This method allows the programmer to propagate original labels
  * to the output cube (default is true)
  *
  * @param prop Flag indicating if original labels is to be propagated
  * to output cubes.
  */
  void Process::PropagateOriginalLabel(const bool prop) {
    p_propagateOriginalLabel = prop;
  }


  /**
   * This method reads the mission specific data directory from the user
   * preference file, makes sure that mission is available in the Isis
   * installation, and the attaches the provided input file to the directory in
   * order to create a full file specification.
   *
   * @param mission Name of the mission data directory
   *
   * @param file Name of the file to attach to the end of the directory
   *
   * @param highestVersion If set to true the method will return the highest
   *                       version number of the given file. Therefore, file must
   *                       contain question marks such as "file???.dat". See the
   *                       FileName class for more information on versioned
   *                       files. Defaults to false.
   */
  QString Process::MissionData(const QString &mission, const QString &file,
                              bool highestVersion) {
    Isis::PvlGroup &dataDir = Isis::Preference::Preferences().findGroup("DataDirectory");
    std::string dir = dataDir[mission.toStdString()];

    // See if the data directory is installed
    Isis::FileName installed(dir);
    if(!installed.fileExists()) {
      std::string message = "Data directory for mission [" + mission.toStdString() + "] " +
                       "is not installed at your site";
      throw IException(IException::Io, message, _FILEINFO_);
    }

    Isis::FileName expanded(dir + "/" + file.toStdString());
    // Highest version checks if the file exists, if we
    // do not ask for the highest version, then we need to check
    // if the file exists
    if (highestVersion) {
      expanded = expanded.highestVersion();
    }
    else {
      if (!expanded.fileExists()) {
        std::string message = "The file [" + expanded.original() + "] " +
                          "cannot be found under [" + mission.toStdString() + "]";
        throw IException(IException::Io, message, _FILEINFO_);
      }
    }
    return QString::fromStdString(expanded.expanded());
  }


  /**
   * Writes out the History blob to the cube
   */
  void Process::WriteHistory(Cube &cube) {
    if(p_propagateHistory) {
      bool addedHist = false;
      if(InputCubes.size() > 0) {
        Isis::Pvl & inlab = *InputCubes[0]->label();
        for(int i = 0; i < inlab.objects(); i++) {
          if(inlab.object(i).isNamed("History") && Isis::iApp != NULL) {
            QString histBlobName = QString::fromStdString(inlab.object(i)["Name"]);
            History h = InputCubes[0]->readHistory(histBlobName);
            h.AddEntry();
            cube.write(h, histBlobName);
            addedHist = true;
          }
        }
      }

      if(!addedHist && Isis::iApp != NULL) {
        Isis::History h = cube.readHistory();
        h.AddEntry();

        cube.write(h);
      }
    }
  }


  /**
   * Calculates and stores off statistics on every band of every
   * cube added to this process via the SetInputCube method.
   *
   * The newly calculated statistics are stored in two ways: as a
   * vector where each entry is a single Statistics object for
   * every band of a particular input cube, and as a vector where
   * each entry is a vector of Statistics objects, for each band
   * separately, of a particular input cube.
   */
  void Process::CalculateStatistics() {
    for(unsigned cubeNum = 0; cubeNum < InputCubes.size(); cubeNum++) {
      Cube *cube = InputCubes[cubeNum];

      // Construct a line buffer manager and a statistics object
      Isis::LineManager line(*cube);
      Isis::Statistics *cubeStats = new Isis::Statistics();

      int bandStart = 1;
      int bandStop = cube->bandCount();
      int maxSteps = cube->lineCount() * cube->bandCount();

      QString cubeNumStr = QString::fromStdString(toString(cubeNum + 1));
      QString totalCubes = QString::fromStdString(toString((int)InputCubes.size()));
      QString msg = "Calculating statistics for cube " + cubeNumStr + " of " + totalCubes;

      Isis::Progress progress;
      progress.SetText(msg);
      progress.SetMaximumSteps(maxSteps);
      progress.CheckStatus();

      // Loop and get the statistics for a good minimum/maximum
      vector<Statistics *> allBandStats;
      for(int useBand = bandStart; useBand <= bandStop; useBand++) {
        Isis::Statistics *bandStats = new Isis::Statistics();

        for(int i = 1; i <= cube->lineCount(); i++) {
          line.SetLine(i, useBand);
          cube->read(line);
          bandStats->AddData(line.DoubleBuffer(), line.size());
          cubeStats->AddData(line.DoubleBuffer(), line.size());
          progress.CheckStatus();
        }

        allBandStats.push_back(bandStats);
      }

      p_bandStats.push_back(allBandStats);
      p_cubeStats.push_back(cubeStats);
    }
  }

} // end namespace isis
