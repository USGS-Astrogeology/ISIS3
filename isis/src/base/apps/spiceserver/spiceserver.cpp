#include <iomanip>
#include <sstream>

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QFile>
#include <QString>
#include <QStringList>

#include "Application.h"
#include "Blob.h"
#include "Camera.h"
#include "CameraFactory.h"
#include "Cube.h"
#include "FileName.h"
#include "IString.h"
#include "Kernel.h"
#include "KernelDb.h"
#include "Longitude.h"
#include "Process.h"
#include "Pvl.h"
#include "PvlToPvlTranslationManager.h"
#include "Table.h"
#include "TextFile.h"
#include "spiceserver.h"

using namespace std;


namespace Isis {
  bool g_ckSmithed = false;
  bool g_ckRecon = false;
  bool g_ckPredicted = false;
  bool g_ckNadir = false;
  bool g_spkSmithed = false;
  bool g_spkRecon = false;
  bool g_spkPredicted = false;
  double g_startPad = 0.0;
  double g_endPad = 0.0;
  QString g_shapeKernelStr;

  bool tryKernels(Cube &cube, UserInterface &ui, Pvl *log, Pvl &labels, Process &p,
                  Kernel lk, Kernel pck,
                  Kernel targetSpk, Kernel ck,
                  Kernel fk, Kernel ik,
                  Kernel sclk, Kernel spk,
                  Kernel iak, Kernel dem,
                  Kernel exk);

  //! Combines all the temp files into one final output file
  void packageKernels(QString toFile);

  //! Read the spiceinit parameters
  void parseParameters(QDomElement parametersElement);

  //! Convert a table into an xml tag
  QString tableToXml(QString tableName, QString file);

  void spiceserver(UserInterface &ui, Pvl *log) {

    try {
      Process p;

      g_ckSmithed = false;
      g_ckRecon = false;
      g_ckPredicted = false;
      g_ckNadir = false;
      g_spkSmithed = false;
      g_spkRecon = false;
      g_spkPredicted = false;
      g_shapeKernelStr = "";
      g_startPad = 0.0;
      g_endPad = 0.0;

      // Get the single line of encoded XML from the input file that the client, spiceinit, sent us.
      TextFile inFile( ui.GetFileName("FROM") );
      QString hexCode;

      // GetLine returns false if it was the last line... so we can't check for problems really
      inFile.GetLine(hexCode);

      Pvl label;
      label.clear();
      QString otherVersion;

      if ( !hexCode.isEmpty() ) {
        // Convert HEX to XML
        QString xml( QByteArray::fromHex( QByteArray( hexCode.toLatin1() ) ).constData() );

        // Parse the XML with Qt's XML parser... kindof convoluted, I'm sorry
        QDomDocument document;
        QString error;
        int errorLine, errorCol;
        if ( document.setContent(QString(xml), &error, &errorLine, &errorCol) ) {
          QDomElement rootElement = document.firstChild().toElement();

          for ( QDomNode node = rootElement.firstChild();
                !node .isNull();
                node = node.nextSibling() ) {
            QDomElement element = node.toElement();

            // Store off the other isis version
            if (element.tagName() == "isis_version") {
              QString encoded = element.firstChild().toText().data();
              otherVersion = QByteArray::fromHex( encoded.toLatin1() ).constData();
            }
            else if (element.tagName() == "parameters") {
              // Read the spiceinit parameters
              parseParameters(element);
            }
            else if (element.tagName() == "label") {
              // Get the cube label
              QString encoded = element.firstChild().toText().data();
              stringstream labStream;
              labStream << QString( QByteArray::fromHex( encoded.toLatin1() ).constData() );
              labStream >> label;
            }
          }
        }
        else {
          QString err = "Unable to read XML. The reason given was [";
          err += error;
          err += "] on line [" + toString(errorLine) + "] column [";
          err += toString(errorCol) + "]";
          throw IException(IException::Io, err, _FILEINFO_);
        }
      }
      else {
        QString msg = "Unable to read input file";
        throw IException(IException::User, msg, _FILEINFO_);
      }


      if (ui.GetBoolean("CHECKVERSION") ) {
        QStringList remoteVersion = otherVersion.split(QRegExp("\\s+"))[0].split(QRegExp("\\."));
        if ( remoteVersion[0].toInt() <= 3 && remoteVersion[1].toInt() < 5) {

         QString msg ="The SPICE server only supports Isis versions greater than or equal to 3.5.*.*.";
                 msg += "Your version:   [" + otherVersion + "] is not compatible";
          throw IException(IException::User, msg, _FILEINFO_);
        }

      }

      // This next section looks a lot like spiceinit, its semi-duplicated because
      //   I did not want users to be able to spiceinit a label without cube
      //   data.

      // Set up for getting the mission name
      // Get the directory where the system missions translation table is.
      QString transFile = "$ISISROOT/appdata/translations/MissionName2DataDir.trn";

      // Get the mission translation manager ready
      PvlToPvlTranslationManager missionXlater(label, transFile);

      // Get the mission name so we can search the correct DB's for kernels
      QString mission = missionXlater.Translate("MissionName");

      // Get system base kernels
      unsigned int allowed = 0;
      unsigned int allowedCK = 0;
      unsigned int allowedSPK = 0;
      if (g_ckPredicted)  allowedCK |= Kernel::typeEnum("PREDICTED");
      if (g_ckRecon)      allowedCK |= Kernel::typeEnum("RECONSTRUCTED");
      if (g_ckSmithed)    allowedCK |= Kernel::typeEnum("SMITHED");
      if (g_ckNadir)      allowedCK |= Kernel::typeEnum("NADIR");
      if (g_spkPredicted) allowedSPK |= Kernel::typeEnum("PREDICTED");
      if (g_spkRecon)     allowedSPK |= Kernel::typeEnum("RECONSTRUCTED");
      if (g_spkSmithed)   allowedSPK |= Kernel::typeEnum("SMITHED");

      KernelDb baseKernels(allowed);
      KernelDb ckKernels(allowedCK);
      KernelDb spkKernels(allowedSPK);

      baseKernels.loadSystemDb(mission, label);
      ckKernels.loadSystemDb(mission, label);
      spkKernels.loadSystemDb(mission, label);

      Kernel lk, pck, targetSpk, fk, ik, sclk, spk, iak, dem, exk;
      QList< priority_queue<Kernel> > ck;
      lk        = baseKernels.leapSecond(label);
      pck       = baseKernels.targetAttitudeShape(label);
      targetSpk = baseKernels.targetPosition(label);
      ik        = baseKernels.instrument(label);
      sclk      = baseKernels.spacecraftClock(label);
      iak       = baseKernels.instrumentAddendum(label);
      fk        = ckKernels.frame(label);
      ck        = ckKernels.spacecraftPointing(label);
      spk       = spkKernels.spacecraftPosition(label);

      if (g_ckNadir) {
        // Only add nadir if no spacecraft pointing found
        QStringList nadirCk;
        nadirCk.push_back("Nadir");
        // if a priority queue already exists, add Nadir with low priority of 0
        if (ck.size() > 0) {
          ck[0].push( Kernel( (Kernel::Type)0, nadirCk ) );
        }
        // if no queue exists, create a nadir queue
        else {
          priority_queue<Kernel> nadirQueue;
          nadirQueue.push( Kernel( (Kernel::Type)0, nadirCk ) );
          ck.push_back(nadirQueue);
        }
      }

      // Get shape kernel
      if (g_shapeKernelStr == "system") {
        dem = baseKernels.dem(label);
      }
      else if (g_shapeKernelStr != "ellipsoid") {
        stringstream demPvlKeyStream;
        demPvlKeyStream << "ShapeModel = " + g_shapeKernelStr;
        PvlKeyword key;
        demPvlKeyStream >> key;

        for (int value = 0; value < key.size(); value++) {
          dem.push_back(key[value]);
        }
      }

      bool kernelSuccess = false;

      if (ck.size() == 0 || ck.at(0).size() == 0) {
        throw IException(IException::Unknown,
                         "No Camera Kernel found for the image [" +
                          ui.GetFileName("FROM") + "]",
                         _FILEINFO_);
      }

      FileName inputLabels;

      while (ck.at(0).size() != 0 && !kernelSuccess) {
        // create an empty kernel
        Kernel realCkKernel;
        QStringList ckKernelList;

        /*
         * Add the list of cks from each Kernel object at the top of each
         * priority queue. If multiple priority queues exist, we will not\
         * pop of the top priority from any of the queues except for the
         * first one.  So each time tryKernels() fails, the same files
         * will be loaded with the next priority from the first queue.
         */
        for (int i = ck.size() - 1; i >= 0; i--) {
          if (ck.at(i).size() != 0) {
            Kernel topPriority = ck.at(i).top();
            ckKernelList.append( topPriority.kernels() );
            // set the type to equal the type of the to priority of the first queue
            realCkKernel.setType( topPriority.type() );
          }
        }

        /*
         * pop the top priority ck off only the first queue so that the next
         * iteration will test the next highest priority of the first queue with
         * the top priority of each of the other queues.
         */
        ck[0].pop();

        // Merge SpacecraftPointing and Frame into ck
        for (int i = 0; i < fk.size(); i++) {
          ckKernelList.push_back(fk[i]);
        }

        realCkKernel.setKernels(ckKernelList);

        /*
         * Create a dummy cube from the labels that spiceinit sent. We do this because the camera
         * classes take a cube instead of a pvl as input.
         *
         * This program has read and write access on the spice server in /tmp/spice_web_service.
         */
        inputLabels = FileName::createTempFile( ui.GetCubeName("TEMPFILE") );
        label.write( inputLabels.expanded() );
        Cube cube;
        cube.open(inputLabels.expanded(), "rw");
        kernelSuccess = tryKernels(cube, ui, log, label, p, lk, pck, targetSpk,
                                   realCkKernel, fk, ik, sclk, spk,
                                   iak, dem, exk);
      }

      if (!kernelSuccess) {
        throw IException(IException::Unknown, "Unable to initialize camera model", _FILEINFO_);
      }
      else {
        packageKernels( ui.GetFileName("TO") );
      }
      remove( inputLabels.expanded().toLatin1() ); //clean up
      p.EndProcess();
    }
    catch (...) {
      // We failed at something, delete the temp files...
      QString outFile = ui.GetFileName("TO");
      QFile pointingFile(outFile + ".pointing");
      if ( pointingFile.exists() ) pointingFile.remove();

      QFile positionFile(outFile + ".position");
      if ( positionFile.exists() ) positionFile.remove();

      QFile bodyRotFile(outFile + ".bodyrot");
      if ( bodyRotFile.exists() ) bodyRotFile.remove();

      QFile sunFile(outFile + ".sun");
      if ( sunFile.exists() ) sunFile.remove();

      throw;
    }
  }

  bool tryKernels(Cube &cube, UserInterface &ui, Pvl *log, Pvl &lab, Process &p,
                  Kernel lk, Kernel pck,
                  Kernel targetSpk, Kernel ck,
                  Kernel fk, Kernel ik, Kernel sclk,
                  Kernel spk, Kernel iak,
                  Kernel dem, Kernel exk) {

    Pvl origLabels = lab;

    // Add the new kernel files to the existing kernels group
    PvlKeyword lkKeyword("LeapSecond");
    PvlKeyword pckKeyword("TargetAttitudeShape");
    PvlKeyword targetSpkKeyword("TargetPosition");
    PvlKeyword ckKeyword("InstrumentPointing");
    PvlKeyword ikKeyword("Instrument");
    PvlKeyword sclkKeyword("SpacecraftClock");
    PvlKeyword spkKeyword("InstrumentPosition");
    PvlKeyword iakKeyword("InstrumentAddendum");
    PvlKeyword emptyDemKeyword("ShapeModel");
    PvlKeyword demKeyword("ShapeModel");
    PvlKeyword exkKeyword("Extra");

    for (int i = 0; i < lk.size(); i++) {
      lkKeyword.addValue(lk[i]);
    }
    for (int i = 0; i < pck.size(); i++) {
      pckKeyword.addValue(pck[i]);
    }
    for (int i = 0; i < targetSpk.size(); i++) {
      targetSpkKeyword.addValue(targetSpk[i]);
    }
    for (int i = 0; i < ck.size(); i++) {
      ckKeyword.addValue(ck[i]);
    }
    for (int i = 0; i < ik.size(); i++) {
      ikKeyword.addValue(ik[i]);
    }
    for (int i = 0; i < sclk.size(); i++) {
      sclkKeyword.addValue(sclk[i]);
    }
    for (int i = 0; i < spk.size(); i++) {
      spkKeyword.addValue(spk[i]);
    }
    for (int i = 0; i < iak.size(); i++) {
      iakKeyword.addValue(iak[i]);
    }
    for (int i = 0; i < dem.size(); i++) {
      demKeyword.addValue(dem[i]);
    }
    for (int i = 0; i < exk.size(); i++) {
      exkKeyword.addValue(exk[i]);
    }

    PvlGroup originalKernels = lab.findGroup("Kernels", Pvl::Traverse);
    PvlGroup currentKernels = lab.findGroup("Kernels", Pvl::Traverse);
    currentKernels.addKeyword(lkKeyword, Pvl::Replace);
    currentKernels.addKeyword(pckKeyword, Pvl::Replace);
    currentKernels.addKeyword(targetSpkKeyword, Pvl::Replace);
    currentKernels.addKeyword(ckKeyword, Pvl::Replace);
    currentKernels.addKeyword(ikKeyword, Pvl::Replace);
    currentKernels.addKeyword(sclkKeyword, Pvl::Replace);
    currentKernels.addKeyword(spkKeyword, Pvl::Replace);
    currentKernels.addKeyword(iakKeyword, Pvl::Replace);
    currentKernels.addKeyword(emptyDemKeyword, Pvl::Replace);
    currentKernels.addKeyword(demKeyword, Pvl::Replace);

    // report qualities
    PvlKeyword spkQuality("InstrumentPositionQuality");
    spkQuality.addValue( Kernel::typeEnum( spk.type() ) );
    currentKernels.addKeyword(spkQuality, Pvl::Replace);

    PvlKeyword ckQuality("InstrumentPointingQuality");
    ckQuality.addValue( Kernel::typeEnum( ck.type() ) );
    currentKernels.addKeyword(ckQuality, Pvl::Replace);

    if ( !exkKeyword.isNull() )
      currentKernels.addKeyword(exkKeyword, Pvl::Replace);
    else if ( currentKernels.hasKeyword("EXTRA") )
      currentKernels.deleteKeyword("EXTRA");

    // Get rid of old keywords from previously inited cubes
    if ( currentKernels.hasKeyword("SpacecraftPointing") )
      currentKernels.deleteKeyword("SpacecraftPointing");

    if ( currentKernels.hasKeyword("SpacecraftPosition") )
      currentKernels.deleteKeyword("SpacecraftPosition");

    if ( currentKernels.hasKeyword("ElevationModel") )
      currentKernels.deleteKeyword("ElevationModel");

    if ( currentKernels.hasKeyword("Frame") )
      currentKernels.deleteKeyword("Frame");

    if ( currentKernels.hasKeyword("StartPadding") )
      currentKernels.deleteKeyword("StartPadding");

    if ( currentKernels.hasKeyword("EndPadding") )
      currentKernels.deleteKeyword("EndPadding");

    // Add any time padding the user specified to the spice group
    if (g_startPad > DBL_EPSILON)
      currentKernels.addKeyword( PvlKeyword("StartPadding", toString(g_startPad), "seconds") );

    if (g_endPad > DBL_EPSILON)
      currentKernels.addKeyword( PvlKeyword("EndPadding", toString(g_endPad), "seconds") );


    currentKernels.addKeyword(
        PvlKeyword( "CameraVersion", toString( CameraFactory::CameraVersion(cube) ) ), Pvl::Replace);

    // Add the modified Kernels group to the input cube labels
    cube.putGroup(currentKernels);

    // Create the camera so we can get blobs if necessary
    try {
      Camera *cam = NULL;
      try {
        cam = CameraFactory::Create(cube);

        // If success then pretend we had the shape model keyword in there...
        Pvl applicationLog;
        applicationLog += currentKernels;
        applicationLog.write(ui.GetFileName("TO") + ".print");
      }
      catch (IException &e) {
        Pvl errPvl = e.toPvl();

        if (errPvl.groups() > 0)
          currentKernels += PvlKeyword("Error", errPvl.group(errPvl.groups() - 1)["Message"][0]);

        Application::Log(currentKernels);
        throw e;
      }
      Table ckTable = cam->instrumentRotation()->Cache("InstrumentPointing");
      ckTable.Label() += PvlKeyword("Description", "Created by spiceinit");
      ckTable.Label() += PvlKeyword("Kernels");

      for (int i = 0; i < ckKeyword.size(); i++)
        ckTable.Label()["Kernels"].addValue(ckKeyword[i]);

      ckTable.toBlob().Write(ui.GetFileName("TO") + ".pointing");

      Table spkTable = cam->instrumentPosition()->Cache("InstrumentPosition");
      spkTable.Label() += PvlKeyword("Description", "Created by spiceinit");
      spkTable.Label() += PvlKeyword("Kernels");
      for (int i = 0; i < spkKeyword.size(); i++)
        spkTable.Label()["Kernels"].addValue(spkKeyword[i]);

      spkTable.toBlob().Write(ui.GetFileName("TO") + ".position");

      Table bodyTable = cam->bodyRotation()->Cache("BodyRotation");
      bodyTable.Label() += PvlKeyword("Description", "Created by spiceinit");
      bodyTable.Label() += PvlKeyword("Kernels");
      for (int i = 0; i < targetSpkKeyword.size(); i++)
        bodyTable.Label()["Kernels"].addValue(targetSpkKeyword[i]);

      for (int i = 0; i < pckKeyword.size(); i++)
        bodyTable.Label()["Kernels"].addValue(pckKeyword[i]);

      bodyTable.Label() += PvlKeyword( "SolarLongitude", toString( cam->solarLongitude().degrees() ) );
      bodyTable.toBlob().Write(ui.GetFileName("TO") + ".bodyrot");

      Table sunTable = cam->sunPosition()->Cache("SunPosition");
      sunTable.Label() += PvlKeyword("Description", "Created by spiceinit");
      sunTable.Label() += PvlKeyword("Kernels");
      for (int i = 0; i < targetSpkKeyword.size(); i++)
        sunTable.Label()["Kernels"].addValue(targetSpkKeyword[i]);

      sunTable.toBlob().Write(ui.GetFileName("TO") + ".sun");

      //  Save original kernels in keyword before changing to Table
      PvlKeyword origCk = currentKernels["InstrumentPointing"];
      PvlKeyword origSpk = currentKernels["InstrumentPosition"];
      PvlKeyword origTargPos = currentKernels["TargetPosition"];

      currentKernels["InstrumentPointing"] = "Table";
      for (int i = 0; i < origCk.size(); i++)
        currentKernels["InstrumentPointing"].addValue(origCk[i]);

      currentKernels["InstrumentPosition"] = "Table";
      for (int i = 0; i < origSpk.size(); i++)
        currentKernels["InstrumentPosition"].addValue(origSpk[i]);

      currentKernels["TargetPosition"] = "Table";
      for (int i = 0; i < origTargPos.size(); i++)
        currentKernels["TargetPosition"].addValue(origTargPos[i]);

      Pvl kernelsLabels;
      kernelsLabels += currentKernels;
      kernelsLabels += cam->getStoredNaifKeywords();
      kernelsLabels.write(ui.GetFileName("TO") + ".lab");
    }
    catch (IException &) {
      lab = origLabels;
      return false;
    }

    return true;
  }


  QString tableToXml(QString tableName, QString file) {
    QString xml;
    xml += "    <" + tableName + ">\n";

    QFile tableFile(file);
    if ( !tableFile.open(QIODevice::ReadOnly) ) {
      QString msg = "Unable to read temporary file [" + file + "]";
      throw IException(IException::Io, msg, _FILEINFO_);
    }

    QByteArray data = tableFile.readAll();
    xml += QString( data.toHex().constData() ) + "\n";
    tableFile.close();
    // we should now be completely done with this temp file
    tableFile.remove();

    xml += "    </" + tableName + ">\n";
    return xml;
  }


  void parseParameters(QDomElement parametersElement) {
    for ( QDomNode node = parametersElement.firstChild();
          !node .isNull();
          node = node.nextSibling() ) {
      QDomElement element = node.toElement();

      if (element.tagName() == "cksmithed") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_ckSmithed = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "ckrecon") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_ckRecon = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "ckpredicted") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_ckPredicted = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "cknadir") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_ckNadir = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "spksmithed") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_spkSmithed = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "spkrecon") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_spkRecon = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "spkpredicted") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_spkPredicted = (attribute.value().toLower() == "yes");
      }
      else if (element.tagName() == "shape") {
        QDomNode node = element.attributes().namedItem("value");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_shapeKernelStr = attribute.value();
      }
      else if (element.tagName() == "startpad") {
        QDomNode node = element.attributes().namedItem("time");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_startPad = attribute.value().toDouble();
      }
      else if (element.tagName() == "endpad") {
        QDomNode node = element.attributes().namedItem("time");
        QDomAttr attribute = *( (QDomAttr *)&node );
        g_endPad = attribute.value().toDouble();
      }
    }
  }


  void packageKernels(QString toFile) {
    QString xml;
    xml += "<spice_data>\n";

    xml += "  <application_log>\n";

    QString logFile(toFile + ".print");
    Pvl logMessage(logFile);
    QFile::remove(logFile);
    stringstream logStream;
    logStream << logMessage;
    xml += QString( QByteArray( logStream.str().c_str() ).toHex().constData() ) + "\n";
    xml += "  </application_log>\n";

    xml += "  <kernels_label>\n";

    QString kernLabelsFile(toFile + ".lab");
    Pvl kernLabels(kernLabelsFile);
    QFile::remove(kernLabelsFile);
    stringstream labelStream;
    labelStream << kernLabels;

    xml += QString( QByteArray( labelStream.str().c_str() ).toHex().constData() ) + "\n";

    xml += "  </kernels_label>\n";

    xml += "  <tables>\n";
    xml += tableToXml("instrument_pointing", toFile + ".pointing");
    xml += tableToXml("instrument_position", toFile + ".position");
    xml += tableToXml("body_rotation", toFile + ".bodyrot");
    xml += tableToXml("sun_position", toFile + ".sun");

    xml += "  </tables>\n";
    xml += "</spice_data>\n";
    QString encodedXml( QByteArray( xml.toLatin1() ).toHex().constData() );

    QFile finalOutput(toFile);
    finalOutput.open(QIODevice::WriteOnly);
    finalOutput.write( encodedXml.toLatin1() );
    finalOutput.close();
  }
};
