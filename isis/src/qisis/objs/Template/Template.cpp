#include "Template.h"

#include <QFile>

#include "FileName.h"
#include "IException.h"
#include "Project.h"

namespace Isis{
  /**
   * Create a Template from template file's name.
   *
   * @param fileName The name of the template file on disk
   * @param templateType The type of template being imported (Options are maps or registrations)
   * @param importName The name of the TemplateList this Template is imported in
   * @param parent The Qt-relationship parent
   */
  Template::Template(QString fileName, QString templateType, QString importName, QObject *parent) : QObject(parent) {
    m_fileName = fileName;
    m_templateType = templateType;
    m_importName = importName;

  }

  /**
   * Construct this template from XML.
   *
   * @param templateFolder Location of template xml
   * @param xmlReader An XML reader that's up to an <template/> tag.
   * @param parent The Qt-relationship parent
   */
  Template::Template(FileName templateFolder, QXmlStreamReader *xmlReader, QObject *parent) :
      QObject(parent) {
    // readTemplate(xmlReader);
  }


  /**
   * Destroys Template object.
   */
  Template::~Template() {
  }


  /**
   * @brief Get the file name that this Template represents.
   * @return @b QString A string containing the path to the template file associated.
   */
  QString Template::fileName() const {
    return m_fileName;
  }


  /**
   * @brief Get the type of template
   * @return @b QString The name of the directory to find this file under "templates".
   */
  QString Template::templateType() const {
    return m_templateType;
  }


  /**
   * @brief Get the name of the TemplateList this file was imported under.
   * @return @b QString A string containing TemplateList's name.
   */
  QString Template::importName() const {
    return m_importName;
  }


  /**
   * @brief Change the file name for this template to be where it now is with
   * the given project.
   * @param project The project that this file is stored in.
   */
  void Template::updateFileName(Project *project) {
    m_fileName = project->templateRoot() + "/" + m_templateType + "/" + m_importName +"/" + QString::fromStdString(FileName(m_fileName.toStdString()).name());
  }


  /**
   * Delete the template from disk.
   *
   * @throws IException::Io  "Could not remove file."
   */
  void Template::deleteFromDisk() {
    if (!QFile::remove(m_fileName)) {
      throw IException(IException::Io,
                       "Could not remove file ["+m_fileName.toStdString()+"]",
                       _FILEINFO_);
    }
  }


  /**
   * Method to write this Template object's member data to an XML stream.
   *
   * @param stream The stream to which the Template will be saved.
   * @param project The Project to which this Template will be added.
   * @param newProjectRoot The location of the project root directory.
   *
   */
  void Template::save(QXmlStreamWriter &stream, const Project *project, FileName newProjectRoot)
      const {
        stream.writeStartElement("template");
        stream.writeAttribute("fileName", QString::fromStdString(FileName(m_fileName.toStdString()).name()));
        stream.writeAttribute("templateType", m_templateType);
        stream.writeAttribute("importName", m_importName);
        stream.writeEndElement();

  }
}
