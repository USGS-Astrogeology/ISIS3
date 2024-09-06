/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Control.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QString>
#include <QUuid>
#include <QXmlStreamWriter>

#include "ControlDisplayProperties.h"
#include "ControlNet.h"
#include "FileName.h"
#include "IException.h"
#include "IString.h"
#include "Project.h"
#include "PvlObject.h"

namespace Isis {
  /**
   * Create a Control from control net located on disk.
   *
   * @param cNetFileName The name of the control net on disk
   * @param parent The Qt-relationship parent
   */
  Control::Control(QString cNetFileName, QObject *parent) : QObject(parent) {

    m_fileName = cNetFileName;

    m_controlNet = NULL;
    m_displayProperties = NULL;
    m_project = NULL;
    m_modified = false;

    try {
      openControlNet();
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer, "Error opening control net.", _FILEINFO_);
    }

    m_displayProperties
        = new ControlDisplayProperties(QString::fromStdString(FileName(m_fileName.toStdString()).name()), this);

    m_id = new QUuid(QUuid::createUuid());
  }


  /**
   * Create a Control from control net located on disk associated with a Project.
   *
   * @param cNetFileName The name of the control net on disk
   * @param parent The Qt-relationship parent
   */
  Control::Control(Project *project, QString cNetFileName, QObject *parent) : QObject(parent) {

    m_fileName = cNetFileName;

    m_controlNet = NULL;
    m_displayProperties = NULL;
    m_project = project;
    m_modified = false;

    m_displayProperties
        = new ControlDisplayProperties(QString::fromStdString(FileName(m_fileName.toStdString()).name()), this);

    m_id = new QUuid(QUuid::createUuid());
  }


  /**
   * Create a Control from a control network that has already been created and read from disk.
   *
   * @param controlNet The actual control network
   * @param cnetFileName The name of the control net on disk
   * @param parent The Qt-relationship parent
   */
  Control::Control(ControlNet *controlNet, QString cnetFileName, QObject *parent) :
      QObject(parent) {

    m_fileName = cnetFileName;

    m_controlNet = controlNet;
    m_displayProperties = NULL;
    m_project = NULL;
    m_modified = false;

    m_displayProperties
        = new ControlDisplayProperties(QString::fromStdString(FileName(m_fileName.toStdString()).name()), this);

    m_id = new QUuid(QUuid::createUuid());
  }


  /**
   * Destroys Control object.
   */
  Control::~Control() {
    delete m_controlNet;
    m_controlNet = NULL;

    delete m_id;
    m_id = NULL;

    //  Image is a "Qt" parent of display properties, so the Image QObject
    //    destructor will take care of deleting the display props. See call to
    //    DisplayProperties' constructor.
    m_displayProperties = NULL;

    // TODO: If control net is modified, prompt for save before destroying??

  }


  /**
   * Open and return a pointer to the ControlNet for this Control.
   *
   * @see openControlNet()
   * @return @b ControlNet* Pointer to the ControlNet object associated with
   *         this Control.
   */
  ControlNet *Control::controlNet() {
    if (!m_controlNet) {
      openControlNet();
    }

    return m_controlNet;
  }


  /**
   * Sets the ControlNet from the control net file name provided in the
   * constructor.
   *
   * @throws IException::Programmer  "Error opening control net."
   */
  void Control::openControlNet() {
    if (!m_controlNet) {
      try {
        m_controlNet = new ControlNet(m_fileName);
        if (m_project) {
          m_controlNet->SetMutex(m_project->mutex());
        }
        m_modified = false;

      }
      catch (IException &e) {
        throw IException(e, IException::Programmer, "Error opening control net.", _FILEINFO_);
      }
    }
  }


  /**
   * @description Write control net to disk.  This method is used instead of calling
   * ControlNet::Write directly so that Control knows the modification state of the control net.
   * Note that if there is not a control net opened, there should no be any changes to write.
   *
   * @return @b bool Returns false if there is not a control net open to write
   *
   * @throws IException::Programmer "Cannot write control net to disk"
   */
  bool Control::write() {

    if (!m_controlNet) {
      return false;
    }

    try {
      m_controlNet->Write(fileName());
    }
    catch (IException &e) {
      throw IException(e, IException::Programmer, "Cannot write control net.", _FILEINFO_);
    }

    m_modified = false;
    return true;
  }


  /**
   * Cleans up the ControlNet pointer. This method should be called
   * once there is no more need for this network because the OS will limit
   * how many of these can be open.
   */
  void Control::closeControlNet() {
    if (m_controlNet) {
      delete m_controlNet;
      m_controlNet = NULL;
    }
    m_modified = false;
  }


  /**
   * @description Has this control been modified?
   *
   * @return @b bool Has this control been modified?
   *
   */
  bool Control::isModified() {
    return m_modified;
  }


  /**
   * @description Sets the modification state of this control. This is needed for now since many
   * classes make changes to the control net contained in this object, but the control does not
   * know the state of the control net.
   * TODO:  Change this class to always know the state of the control Net.
   *
   */
  void Control::setModified(bool modified) {

    m_modified = modified;
  }


  /**
   * Access a pointer to the display properties for the control network.
   *
   * @return @b ControlDisplayProperties * A pointer to the display properties.
   */
  ControlDisplayProperties *Control::displayProperties() {
    return m_displayProperties;
  }


  /**
   * Access a const pointer to the display properties for the control network.
   *
   * @return @b ControlDisplayProperties * A pointer to the display properties.
   */
  const ControlDisplayProperties *Control::displayProperties() const {
    return m_displayProperties;
  }


  /**
   * Access the name of the control network file associated with this Control.
   *
   * @return @b QString The file name of the control network.
   */
  QString Control::fileName() const {
    return m_fileName;
  }


  /**
   * Access the unique ID associated with this Control.
   *
   * @return @b QString The Control ID.
   */
  QString Control::id() const {
    return m_id->toString().remove(QRegExp("[{}]"));
  }


  /**
   * Copies the files of the given Project to the given location.
   *
   * @param project A pointer to the Project.
   * @param newProjectRoot The name of the new root directory where the project
   *                       will be copied.
   */
  void Control::copyToNewProjectRoot(const Project *project, FileName newProjectRoot) {

    if (FileName(newProjectRoot).toString() != FileName(project->projectRoot().toStdString()).toString()) {

      QString newNetworkPath =  project->cnetRoot(QString::fromStdString(newProjectRoot.toString())) + "/" +
                  QString::fromStdString(FileName(m_fileName.toStdString()).dir().filename().string()) + "/" + QString::fromStdString(FileName(m_fileName.toStdString()).name());

      // If there is active control & it has been modified, write to disk instead of copying
      //  Leave control net at old location in unmodified state
      if (isModified()) {
        controlNet()->Write(newNetworkPath);
        setModified(false);
      }
      else {
        QString oldNetworkPath = project->cnetRoot(project->projectRoot()) + "/" +
                    QString::fromStdString(FileName(m_fileName.toStdString()).dir().filename().string()) + "/" + QString::fromStdString(FileName(m_fileName.toStdString()).name());
        if (!QFile::copy(oldNetworkPath,newNetworkPath) ) {
          throw IException(IException::Io, "Error saving control net.", _FILEINFO_);
        }
      }
    }
    //   Project "Save" to current location, if active control exists & is modified, write to disk
    //  Note:  It does not look like this code is ever executed.  If project is saved with a
    //         "Save" this method is not called.
    else {
      if (isModified()) {
        write();
        setModified(false);
      }
    }

  }


  /**
   * Delete the control net from disk. The control net will no longer be accessible until you call
   * updateFileName().
   *
   * @throws IException::Io  "Could not remove file."
   */
  void Control::deleteFromDisk() {

    if (!QFile::remove(m_fileName)) {
      throw IException(IException::Io,
                       tr("Could not remove file [%1]").arg(m_fileName),
                       _FILEINFO_);
    }

    // If we're the last thing in the folder, remove the folder too.
    QDir dir;
    dir.rmdir(QString::fromStdString(FileName(m_fileName.toStdString()).path()));
    m_modified = false;
  }


  /**
   * Change the on-disk file name for this control to be where the control ought to be in the given
   * project.
   *
   * @param project The project that this control is stored in
   */
  void Control::updateFileName(Project *project) {
    closeControlNet();

    FileName original(m_fileName.toStdString());
    FileName newName(project->cnetRoot().toStdString() + "/" +
                     original.dir().filename().string() + "/" + original.name());
    m_fileName = QString::fromStdString(newName.expanded());
  }


  /**
   * Method to write this Control object's member data to an XML stream.
   *
   * @param stream The stream to which the Control will be saved.
   * @param project The Project to which this Control will be added.
   * @param newProjectRoot The location of the project root directory.
   *
   */
  void Control::save(QXmlStreamWriter &stream, const Project *project,
                     FileName newProjectRoot) const {
    stream.writeStartElement("controlNet");
    stream.writeAttribute("id", m_id->toString());
    //  Change filename to new path
    stream.writeAttribute("fileName", QString::fromStdString(FileName(m_fileName.toStdString()).name()));

    m_displayProperties->save(stream, project, newProjectRoot);

    stream.writeEndElement();
  }
}
