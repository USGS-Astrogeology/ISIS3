#include "FileTool.h"

#include <cmath>

#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QImage>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPrinter>
#include <QPrintDialog>
#include <QToolBar>

#include "Brick.h"
#include "BrowseDialog.h"
#include "CubeAttribute.h"
#include "Enlarge.h"
#include "FileDialog.h"
#include "Interpolator.h"
#include "MainWindow.h"
#include "MdiCubeViewport.h"
#include "OriginalLabel.h"
#include "Portal.h"
#include "ProcessRubberSheet.h"
#include "ProcessByLine.h"
#include "Pvl.h"
#include "Reduce.h"
#include "SaveAsDialog.h"
#include "SubArea.h"
#include "ViewportMainWindow.h"
#include "Workspace.h"

namespace Isis {
  /**
   * Constructs a FileTool object.
   *
   * @param parent
   * @internal
   *   @history 2008-12-10 Jeannie Walldren - Added "What's this?"
   *            and shortcut to "Save" action
   */
  FileTool::FileTool(QWidget *parent) : Tool(parent) {
    p_parent = parent;
    p_dir.setPath("/thisDirDoesNotExist!");
    p_open = new QAction(parent);
    p_open->setShortcut(Qt::CTRL + Qt::Key_O);
    p_open->setText("&Open...");
    p_open->setIcon(QPixmap(toolIconDir() + "/fileopen.png"));
    p_open->setToolTip("Open cube");
    QString whatsThis =
      "<b>Function:</b> Open an <i>Isis cube</i> in new viewport \
       <p><b>Shortcut:</b>  Ctrl+O\n</p> \
       <p><b>Hint:</b> Use Ctrl or Shift in file dialog to open \
       multiple cubes</p>";
    p_open->setWhatsThis(whatsThis);
    connect(p_open, SIGNAL(triggered()), this, SLOT(open()));

    p_browse = new QAction(parent);
    p_browse->setShortcut(Qt::CTRL + Qt::Key_B);
    p_browse->setText("&Browse...");
    p_browse->setToolTip("Browse cubes");
    whatsThis =
      "<b>Function:</b> Browse a <i>Isis cubes</i> in new viewport \
       <p><b>Shortcut:</b>  Ctrl+B\n</p>";
    p_browse->setWhatsThis(whatsThis);
    connect(p_browse, SIGNAL(triggered()), this, SLOT(browse()));

    p_save = new QAction(parent);
    p_save->setShortcut(Qt::CTRL + Qt::Key_S);
    p_save->setText("&Save");
    p_save->setIcon(QPixmap(toolIconDir() + "/filesave.png"));
    p_save->setToolTip("Save");
    whatsThis =
      "<b>Function:</b> Save changes to the current Cube \
       <p><b>Shortcut:</b> Ctrl+S</p>";
    p_save->setWhatsThis(whatsThis);
    connect(p_save, SIGNAL(triggered()), this, SLOT(save()));
    p_save->setEnabled(false);

    p_saveAs = new QAction(parent);
    p_saveAs->setText("Save &As...");
    p_saveAs->setIcon(QPixmap(toolIconDir() + "/filesaveas.png"));
    p_saveAs->setToolTip("Save As");
    whatsThis =
      "<b>Function:</b> Save the current Cube to the specified location";
    p_saveAs->setWhatsThis(whatsThis);
    connect(p_saveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
    p_saveAs->setEnabled(false);

    p_saveInfo = new QAction(parent);
    p_saveInfo->setText("Save &Info...");
    p_saveInfo->setIcon(QPixmap(toolIconDir() + "/filesaveas.png"));
    p_saveInfo->setToolTip("Save Info");
    whatsThis =
      "<b>Function:</b> Save the current Cube's Whatsthis Info to the specified location";
    p_saveInfo->setWhatsThis(whatsThis);
    connect(p_saveInfo, SIGNAL(triggered()), this, SLOT(saveInfo()));
    p_saveInfo->setEnabled(false);

    p_exportView = new QAction(parent);
    p_exportView->setText("Export View");
    p_exportView->setIcon(QPixmap(toolIconDir() + "/fileexport.png"));
    p_exportView->setToolTip("Export View");
    whatsThis =
      "<b>Function:</b> Save visible contents of the active \
       viewport as a png, jpg, tiff \
       <p><b>Hint:</b>  Your local installation of Qt may not support \
       all formats.  Reinstall Qt if necessary</p>";
    p_exportView->setWhatsThis(whatsThis);
    connect(p_exportView, SIGNAL(triggered()), this, SLOT(exportView()));
    p_exportView->setEnabled(false);

    p_exportToList = new QAction(parent);
    p_exportToList->setText("Export to List");
    p_exportToList->setIcon(QPixmap(toolIconDir() + "/fileexport.png"));
    p_exportToList->setToolTip("Export active cubes to a .lis file");
    whatsThis =
      "<b>Function:</b> Save all open cubes \
       to a .lis file containing their file names";
    p_exportToList->setWhatsThis(whatsThis);
    connect(p_exportToList, SIGNAL(triggered()), this, SLOT(exportToList()));
    p_exportToList->setEnabled(false);

    p_print = new QAction(parent);
    p_print->setText("&Print...");
    p_print->setShortcut(Qt::CTRL + Qt::Key_P);
    p_print->setIcon(QPixmap(toolIconDir() + "/fileprint.png"));
    p_print->setToolTip("Print");
    whatsThis =
      "<b>Function:</b> Print visible contents of the active viewport \
      <p><b>Shortcut:</b> Ctrl+P</b>";
    p_print->setWhatsThis(whatsThis);
    connect(p_print, SIGNAL(triggered()), this, SLOT(print()));
    p_print->setEnabled(false);

    p_closeAll = new QAction(parent);
    p_closeAll->setText("&Close All...");
    p_closeAll->setToolTip("Close All");
    whatsThis =
      "<b>Function:</b> Close all cube viewports.";
    p_closeAll->setWhatsThis(whatsThis);

    p_exit = new QAction(this);
    p_exit->setShortcut(Qt::CTRL + Qt::Key_Q);
    p_exit->setText("E&xit");
    p_exit->setIcon(QPixmap(toolIconDir() + "/fileclose.png"));
    whatsThis =
        "<b>Function:</b>  Quit qview \
        <p><b>Shortcut:</b> Ctrl+Q</p>";
    p_exit->setWhatsThis(whatsThis);
    connect(p_exit, SIGNAL(triggered()), this, SLOT(exit()));

    p_lastViewport = NULL;

    p_saveAsDialog = NULL;
    activate(true);
  }


  /**
   * Adds the file tool's actions to the menu
   *
   * @param menu
   */
  void FileTool::addTo(QMenu *menu) {
    menu->addAction(p_open);
    menu->addAction(p_browse);
    menu->addAction(p_save);
    menu->addAction(p_saveAs);
    menu->addAction(p_saveInfo);
    menu->addAction(p_exportView);
    menu->addAction(p_exportToList);
    menu->addAction(p_print);
    menu->addAction(p_closeAll);
    menu->addAction(p_exit);
  }


  /**
   * Connects the fileSelected signal to the workspace's addCubeViewport slot
   *
   * @param ws
   */
  void FileTool::addTo(Workspace *ws) {
    p_workSpace = ws;
    Tool::addTo(ws);
    connect(this, SIGNAL(fileSelected(QString)),
            ws, SLOT(addCubeViewport(QString)));

    connect(p_closeAll, SIGNAL(triggered()), ws->mdiArea(), SLOT(closeAllSubWindows()));
  }


  /**
   * Adds the file tool's actions to the permanent toolbar
   *
   * @param perm
   */
  void FileTool::addToPermanent(QToolBar *perm) {
    perm->addAction(p_open);
    perm->addAction(p_exportView);
    perm->addAction(p_print);
    perm->addAction(p_exit);
  }


  /**
   * This method allows the user to navigate and open a cube with a file dialog.
   *
   */
  void FileTool::open() {
    //Set up the list of filters that are default with this dialog.
    if (!p_filterList.contains("Isis cubes (*.cub)")) {
      p_filterList.append("Isis cubes (*.cub)");
      p_filterList.append("All files (*)");
    }
    if (!p_dir.exists()) {
      p_dir = QDir::current();
    }

    FileDialog *fileDialog = new FileDialog("Open", p_filterList, p_dir, (QWidget *)parent());
    fileDialog->show();
    connect(fileDialog, SIGNAL(fileSelected(QString)),
            p_workSpace, SLOT(addCubeViewport(QString)));
  }


  /**
   * This method allows the user to navigate and browse cubes with a file dialog .
   *
   */
  void FileTool::browse() {
    //Set up the list of filters that are default with this dialog.
    if (!p_filterList.contains("Isis cubes (*.cub)")) {
      p_filterList.append("Isis cubes (*.cub)");
      p_filterList.append("All files (*)");
    }
    if (!p_dir.exists()) {
      p_dir = QDir::current();
    }
    BrowseDialog *browser = new BrowseDialog("Browse", p_filterList, p_dir, (QWidget *)parent());
    browser->show();
    connect(browser, SIGNAL(fileSelected(QString)),
            p_workSpace, SLOT(addBrowseView(QString)));
  }


  /**
   * This method saves any changes made to the current cube, these
   * changes are finalized! There is no undoing once a save has
   * been made.
   *
   */
  void FileTool::save() {
    //If the current viewport is null (safety check), return from this method
    if (cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to save");
      return;
    }

    emit saveChanges(cubeViewport());
    p_save->setEnabled(false);

    cubeViewport()->cube()->reopen("rw");
  }


  /**
   * SaveAs Action - Displays the FileDialog with the filterlist (*.cub) to select
   * the output cube. This dialog additionally displays radio buttons for choices
   * FullImage, ExportAsIs, ExportFullRes. These choices are located at the bottom
   * of the dialog.
   * FullImage     - copies the entire image into the user specified output file
   * ExportAsIs    - copies the image as displayed in the qview app window
   * ExportFullRes - copies the image as displayed in the qview app window but with
   *                 full resolution
   *
   * @author Sharmila Prasad (4/8/2011)
   */
  void FileTool::saveAs() {
    //If the current viewport is null (safety check), return from this method
    if (cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to save");
      return;
    }
    //Set up the list of filters that are default with this dialog.
    if (!p_filterList.contains("Isis cubes (*.cub)")) {
      p_filterList.append("Isis cubes (*.cub)");
    }
    if (!p_dir.exists()) {
      p_dir = QDir(p_lastDir);
    }
    if (p_saveAsDialog) {
      delete p_saveAsDialog;
      p_saveAsDialog = NULL;
    }

    p_saveAsDialog = new SaveAsDialog("Save As", p_filterList, p_dir, (QWidget *)parent());
    connect(p_saveAsDialog, SIGNAL(fileSelected(QString)), this, SLOT(saveAsCubeByOption(QString)));

    p_saveAsDialog->show();
  }


  /**
   * Save input image as a cube into specified output file as FullImage or
   * ExportAsIs or ExportFullRes option
   *
   * @author Sharmila Prasad (4/8/2011)
   *
   * @param psOutFile - user specified output file
   */
  void FileTool::saveAsCubeByOption(QString psOutFile) {
    //If the current viewport is null (safety check), return from this method
    if (cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error",
                               "No active cube to save");
      return;
    }

    //If the filename is empty, return
    if (psOutFile.isEmpty() || (p_saveAsDialog==NULL)){
      QMessageBox::information((QWidget *)parent(), "Error",
                               "No output file selected");
      return;
    }

    //Check if the output file is already opened
    QVector< MdiCubeViewport *> *vwportList = p_workSpace->cubeViewportList();
    QVector<MdiCubeViewport *>::iterator it;
    for (it = vwportList->begin(); it != vwportList->end(); ++it){
      if (QString((*it)->cube()->fileName()) ==  psOutFile) {
        QMessageBox::information((QWidget *)parent(), "Error",
            "Output File is already open\n\""+ psOutFile + "\"");
        return;
      }
    }

    //If the filename is the same as the current cube's filename, just save it
    if (p_saveAsDialog->getSaveAsType() == SaveAsDialog::FullImage &&
       psOutFile == cubeViewport()->cube()->fileName()) {
      save();
      return;
    }

    //Save the current cube's changes by reopening it, and open an input cube
    //from the current cube's location
    Cube *icube = new Cube();
    icube->open(cubeViewport()->cube()->fileName(), "rw");
    Cube *ocube = NULL;

    // The output cube needs to be created if the scale is 1 since saveAs_FullResolution will be
    //  called, which expects a cube.
    //  NOTE:  This really should be cleaned up and the cube should be created in 1 place.
    if (p_saveAsDialog->getSaveAsType() != SaveAsDialog::ExportAsIs ||
       p_lastViewport->scale() == 1) {
      //Create the output cube
      ocube = new Cube;
    }

    if (p_saveAsDialog->getSaveAsType() == SaveAsDialog::FullImage) {
      copyCubeDetails(psOutFile, icube, ocube, icube->sampleCount(),
                      icube->lineCount(), icube->bandCount());
      saveAsFullImage(icube, ocube);
      ocube->close();
    }
    else {
      // start and end Samples and Lines
      double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
      p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);

      if (p_saveAsDialog->getSaveAsType() == SaveAsDialog::ExportFullRes ||
         p_lastViewport->scale() == 1) {
//      int numSamples = (int)ceil(dEndSample-dStartSample);
//      int numLines = (int)ceil(dEndLine-dStartLine);
        int numSamples = (int)((dEndSample - dStartSample + 1) + 0.5);
        int numLines = (int)((dEndLine - dStartLine + 1) + 0.5);
        copyCubeDetails(psOutFile, icube, ocube, numSamples, numLines, icube->bandCount());
        saveAs_FullResolution(icube, ocube, numSamples, numLines);
        ocube->close();
      }
      else if (p_saveAsDialog->getSaveAsType() == SaveAsDialog::ExportAsIs ) {
        saveAs_AsIs(icube, psOutFile);
      }
    }

    emit(fileSelected(psOutFile));

    //Disable the save action
    p_save->setEnabled(false);

    p_lastDir = psOutFile;
  }


  /**
   * For AsIs option, save the enlarged input image visible in the viewport window
   * using the Enlarge functionality
   *
   * @author Sharmila Prasad (4/26/2011)
   *
   * @param icube - Input Cube
   * @param psOutFile - Output Cube
   *
   */
  void FileTool::saveAsEnlargedCube(Cube *icube, const QString & psOutFile) {
    double dScale = p_lastViewport->scale();

    // start and end Samples and Lines
    double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
    p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);

    double ins = dEndSample - dStartSample + 1;
    double inl = dEndLine - dStartLine + 1;

    double ons = (int)(ins * dScale + 0.5);
    double onl = (int)(inl * dScale + 0.5);

    try {
      ProcessRubberSheet p;
      p.SetInputCube (icube);
      Cube *ocube = p.SetOutputCube(psOutFile, CubeAttributeOutput(" "),
                                    ons, onl, icube->bandCount());

      Interpolator *interp = new Interpolator(Interpolator::NearestNeighborType);

      Enlarge *imgEnlarge  = new Enlarge(icube, dScale, dScale);
      imgEnlarge->SetInputArea((int)dStartSample, (int)dEndSample, (int)dStartLine, (int)dEndLine);

      p.StartProcess(*imgEnlarge, *interp);
      imgEnlarge->UpdateOutputLabel(ocube);
      p.EndProcess();

      delete imgEnlarge;
      delete interp;

    } catch(IException &e) {
        // Stacks error message from Cube's create method
        throw IException(e,
            IException::Io,
            QObject::tr("The cube could not be saved, unable to create the cube"),
            _FILEINFO_);
    }
  }


  /**
   * For AsIs option, save the reduced input image visible in the viewport window
   * using the Reduce functionality
   *
   * @author Sharmila Prasad (4/26/2011)
   *
   * @param icube - Input Cube
   * @param psOutFile - Output filename
   */
  void FileTool::saveAsReducedCube(Cube *icube, const QString & psOutFile) {

    double dScale = p_lastViewport->scale();
    // start and end Samples and Lines
    double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
    p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);

    double ins = dEndSample - dStartSample + 1;
    double inl = dEndLine - dStartLine + 1;

    double ons = (int)(ins * dScale + 0.5);
    double onl = (int)(inl * dScale + 0.5);

    CubeAttributeInput cai(icube->fileName().toStdString());
    std::vector<QString> bands = cai.bands();
    int inb = bands.size();

    if (inb == 0) {
      inb = cubeViewport()->cube()->bandCount();
      for(int i = 1; i <= inb; i++) {
        bands.push_back(toString(i));
      }
    }

    try {
      ProcessByLine p;
      p.SetInputCube (icube);
      Cube *ocube = NULL;
      ocube = p.SetOutputCube(psOutFile, CubeAttributeOutput(""), ons, onl, inb);
      // Our processing routine only needs 1
      // the original set was for info about the cube only
      p.ClearInputCubes();

      Cube *tempcube=new Cube;
      tempcube->open(cubeViewport()->cube()->fileName(), "r");
      Nearest *near = new Nearest(tempcube, ins/ons, inl/onl);
      near->setInputBoundary((int)dStartSample, (int)dEndSample, (int)dStartLine, (int)dEndLine);

      p.ProcessCubeInPlace(*near, false);
      near->UpdateOutputLabel(ocube);
      p.EndProcess();

      delete near;
      near=NULL;
    }
    catch(IException &e) {
      // If there is a problem, catch it and close the cube so it isn't open next time around
      icube->close();
      // Stacks error message from Cube's create method
      throw IException(e,
          IException::Io,
          QObject::tr("The cube could not be saved, unable to create the cube"),
          _FILEINFO_);
    }
  }


  /**
   * AsIs option, save the input image visible in the viewport window Enlarged/Reduced
   *
   * @author Sharmila Prasad (4/26/2011)
   *
   * @param icube - Input Cube
   * @param psOutFile - Output Cube
   */
  void FileTool::saveAs_AsIs(Cube *icube, const QString & psOutFile) {
    double dScale = p_lastViewport->scale();
    // Enlarge the cube area
    if (dScale > 1) {
      saveAsEnlargedCube(icube, psOutFile);
    }
    // Reduce the cube area
    else {
      saveAsReducedCube(icube, psOutFile);
    }
  }


  /**
   * Copy input image details into the output given output images's dimension.
   * Info like instrument, history are transferred to output image
   *
   * @param icube        - input image
   * @param ocube        - output image
   * @param psOutFile      - output cube attributes
   * @param piNumSamples - out samples
   * @param piNumLines   - out lines
   * @param piNumBands   - out bands
   *
   * @history 2011-05-11 Sharmila Prasad - Isolated from original SaveAs function so that
   *                                           it can be used by different SaveAs options
   */
  void FileTool::copyCubeDetails(const QString & psOutFile, Cube *icube,
      Cube *ocube, int piNumSamples, int piNumLines, int piNumBands) {
    //Create the default output attribute with the output filename
    CubeAttributeOutput outAtt(psOutFile.toStdString());

    //Propagate all labels, tables, blobs, etc from the input to output cube
    try {
      ocube->setDimensions(piNumSamples, piNumLines, piNumBands);
      ocube->setByteOrder(outAtt.byteOrder());
      ocube->setFormat(outAtt.fileFormat());
      ocube->setLabelsAttached(outAtt.labelAttachment() == AttachedLabel);

      if (outAtt.propagatePixelType()) {
        ocube->setPixelType(icube->pixelType());
      }
      else {
        ocube->setPixelType(outAtt.pixelType());
      }

      if (outAtt.propagateMinimumMaximum()) {
        if (ocube->pixelType() == Real) {
          ocube->setBaseMultiplier(0.0, 1.0);
        }
        else if (ocube->pixelType() >= icube->pixelType()) {
          double base = icube->base();
          double mult = icube->multiplier();
          ocube->setBaseMultiplier(base, mult);
        }
        else if ((ocube->pixelType() != Real) &&
                (ocube->pixelType() != UnsignedByte) &&
                (ocube->pixelType() != SignedWord) &&
                (ocube->pixelType() != UnsignedWord) &&
                (ocube->pixelType() != Isis::UnsignedInteger) &&
                (ocube->pixelType() != Isis::SignedInteger)) {
          std::string msg = "Looks like your refactoring to add different pixel types";
          msg += " you'll need to make changes here";
          throw IException(IException::Programmer, msg, _FILEINFO_);
        }
        else {
          std::string msg = "You've chosen to reduce your output PixelType for [" +
                            psOutFile + "] you must specify the output pixel range too";
          throw IException(IException::User, msg, _FILEINFO_);
        }
      }
      else {
        // Not propagating so either the user entered or the programmer did
        ocube->setMinMax(outAtt.minimum(), outAtt.maximum());
      }

      int needLabBytes = icube->labelSize(true) + (1024 * 6);
      if (needLabBytes > ocube->labelSize()) {
        ocube->setLabelSize(needLabBytes);
      }

      // Allocate the cube
      ocube->create(psOutFile);

      // Transfer labels from the first input cube
      PvlObject &incube = icube->label()->findObject("IsisCube");
      PvlObject &outcube = ocube->label()->findObject("IsisCube");
      for(int i = 0; i < incube.groups(); i++) {
        outcube.addGroup(incube.group(i));
      }

      // Transfer tables from the first input cube
      Pvl &inlab = *icube->label();
      for(int i = 0; i < inlab.objects(); i++) {
        if (inlab.object(i).isNamed("Table")) {
          Blob t(QString::fromStdString(inlab.object(i)["Name"]), inlab.object(i).name());
          icube->read(t);
          ocube->write(t);
        }
      }

      // Transfer blobs from the first input cube
      inlab = *icube->label();
      for(int i = 0; i < inlab.objects(); i++) {
        if (inlab.object(i).isNamed("Polygon")) {
          Blob t(QString::fromStdString(inlab.object(i)["Name"]), inlab.object(i).name());
          icube->read(t);
          ocube->write(t);
        }
      }

      // Transfer tables from the first input cube
      inlab = *icube->label();
      for(int i = 0; i < inlab.objects(); i++) {
        if (inlab.object(i).isNamed("OriginalLabel")) {
          OriginalLabel ol = icube->readOriginalLabel();
          ocube->write(ol);
        }
      }
    }
    catch(IException &) {
      delete ocube;
      throw;
    }
  }


  /**
   * This method essentially creates a new cube, copies the
   * current cube (and any changes made to it) to the new cube,
   * reverses all changes NOT saved to the current cube and closes
   * it. Finally it sets the cubeviewport's cube to the new saved
   * cube.
   *
   * @param icube  - input image
   * @param ocube - output image
   */
  void FileTool::saveAsFullImage(Cube *icube, Cube *ocube) {
    //Start the copy process line by line
    Brick ibrick(*icube, icube->sampleCount(), 1, 1);
    Brick obrick(*ocube, ocube->sampleCount(), 1, 1);

    int numBricks;
    if (ibrick.Bricks() > obrick.Bricks()) {
      numBricks = ibrick.Bricks();
    }
    else {
      numBricks = obrick.Bricks();
    }

    // Loop and let the app programmer work with the bricks
    ibrick.begin();
    obrick.begin();
    for(int i = 0; i < numBricks; i++) {
      icube->read(ibrick);
      //Copy the contents to the output cube
      copy(ibrick, obrick);
      ocube->write(obrick);
      ibrick++;
      obrick++;
    }
  }


  /**
   * Full Resolution option, save the input image visible in the viewport window
   * Enlarged/Reduced in full resolution
   *
   * @author Sharmila Prasad (4/26/2011)
   *
   * @param pInCube     - input image
   * @param pOutCube    - output image
   * @param pNumSamples - out samples
   * @param pNumLines   - out lines
   */
  void FileTool::saveAs_FullResolution(Cube *pInCube,
      Cube *pOutCube, int pNumSamples, int pNumLines) {
    // start and end Samples and Lines
    double dStartSample=0, dEndSample=0, dStartLine=0, dEndLine=0;
    p_lastViewport->getCubeArea(dStartSample, dEndSample, dStartLine, dEndLine);
    int iNumBands   = pInCube->bandCount();

    PvlGroup results("Results");
    results += PvlKeyword("InputLines",      std::to_string(pInCube->lineCount()));
    results += PvlKeyword("InputSamples",    std::to_string(pInCube->sampleCount()));
    results += PvlKeyword("StartingLine",    std::to_string(dStartLine));
    results += PvlKeyword("StartingSample",  std::to_string(dStartSample));
    results += PvlKeyword("EndingLine",      std::to_string(dEndLine));
    results += PvlKeyword("EndingSample",    std::to_string(dEndSample));
    results += PvlKeyword("LineIncrement",   std::to_string(1));
    results += PvlKeyword("SampleIncrement", std::to_string(1));
    results += PvlKeyword("OutputLines",     std::to_string(pNumLines));
    results += PvlKeyword("OutputSamples",   std::to_string(pNumSamples));
    SubArea subArea;
    subArea.SetSubArea(pInCube->lineCount(), pInCube->sampleCount(), dStartLine, dStartSample,
                       dEndLine, dEndSample, 1.0, 1.0);
    subArea.UpdateLabel(pInCube, pOutCube, results);

    Portal iPortal (pNumSamples, 1, pInCube->pixelType());
    Portal oPortal (pNumSamples, 1, pOutCube->pixelType());

    for(int iBand=1; iBand<=iNumBands; iBand++) {
      int ol=1;
      for(int iLine=(int)dStartLine; iLine<=(int)dEndLine; iLine++) {
        iPortal.SetPosition(dStartSample, iLine, iBand);
        pInCube->read(iPortal);

        oPortal.SetPosition(1, ol++, iBand);
        pOutCube->read(oPortal);

        oPortal.Copy(iPortal);
        pOutCube->write(oPortal);
      }
    }
  }


  /**
   * Saves the whatsthis info of the cubeviewport to
   * user specified output file
   *
   * @author Sharmila Prasad (4/6/2011)
   */
  void FileTool::saveInfo(void)
  {
    if (cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to save info");
      return;
    }

    //Get the new cube's filename
    QString output =
      QFileDialog::getSaveFileName((QWidget *)parent(),
                                   "Choose output file",
                                   p_lastDir,
                                   QString("PVL Files (*.pvl)"));

    //If the filename is empty, return
    if (output.isEmpty()) {
      return;
    }
    else if (!output.endsWith(".pvl")) {
      output += ".pvl";
    }

    Pvl whatsThisPvl;
    cubeViewport()->getAllWhatsThisInfo(whatsThisPvl);
    whatsThisPvl.write(output.toStdString());
  }


  /**
   * This method copies from the input buffer to the output buffer
   *
   * @param in
   * @param out
   */
  void FileTool::copy(Buffer &in, Buffer &out) {
    out.Copy(in);
  }


  /**
   * This slot emits a signal to discard all changes to the
   * current viewport
   *
   */
  void FileTool::discard() {
    emit discardChanges(cubeViewport());
  }


  /**
   * This method allows the user to export the current view as an image file.
   *
   */
  void FileTool::exportView() {
    if (cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to export");
      return;
    }

    QString output =
      QFileDialog::getSaveFileName((QWidget *)parent(),
                                   QString("Choose output file"),
                                   p_lastDir,
                                   QString("PNG (*.png);;JPG (*.jpg);;TIF (*.tif)"));
    if (output.isEmpty()) return;

    p_lastDir = output;

    QString format = QFileInfo(output).suffix();

    if (format.isEmpty()) {
      if (output.endsWith('.')) {
        output.append(QString("png"));
      }
      else {
        output.append(QString(".png"));
      }
    }
    else if (format.compare("png", Qt::CaseInsensitive) &&
            format.compare("jpg", Qt::CaseInsensitive) &&
            format.compare("tif", Qt::CaseInsensitive)) {

      QMessageBox::information((QWidget *)parent(), "Error", format + " is an invalid extension.");
      return;
    }

    QPixmap pm = cubeViewport()->viewport()->grab();

    //if (!cubeViewport()->pixmap().save(output,format)) {

    if (!pm.save(output)) {
      QMessageBox::information((QWidget *)parent(), "Error", "Unable to save " + output);
      return;
    }
  }


  /**
   * @brief FileTool::exportToList
   *
   * This method exports the file names of all active cubes into a .lis file by looping through
   * The ViewportMainWindow and grabbing the file names from the active child CubeViewports.
   *
   * @author Adam Goins
   */
  void FileTool::exportToList() {
      if (cubeViewport() == NULL) {
        QMessageBox::information((QWidget *)parent(), "Error", "No active cubes to export");
        return;
      }

      // The ViewportMainWindow is the parent container to the FileTool.
      // We need to grab that object so that we can loop through it's children
      // To find the active cube viewports.
      ViewportMainWindow* window = dynamic_cast<ViewportMainWindow*>(parent());

      if (window == NULL) {
        QMessageBox::critical((QWidget *)parent(), "Error", "There was an error reading the viewport window.");
        return;
      }

      QList<CubeViewport*> openCubes = window->findChildren<CubeViewport*>();
      QList<QString> cubeFilePaths;

      for (int i = 0; i < openCubes.size(); i++) {

          CubeViewport* cubeViewport = openCubes.value(i);

//          QString cubeFileName(cubeViewport->cube()->fileName());
          QFileInfo cubeFileName(cubeViewport->cube()->fileName());

          cubeFilePaths.append(cubeFileName.absoluteFilePath());
      }

      QString fileName = QFileDialog::getSaveFileName((QWidget *) parent(),
                                                      "Export to cube list",
                                                      ".",
                                                      "Cube List (*.lis)");

      if (!fileName.contains(".lis")) {
          fileName.append(".lis");
      }

      QFile outputFile(fileName);
      outputFile.open(QIODevice::WriteOnly | QIODevice::Text);

      QTextStream out(&outputFile);

      // Write each cube filename onto it's own line inside of that file.
      for (int i = 0; i < cubeFilePaths.size(); i++){
          out << cubeFilePaths.value(i) << "\n";
      }
      outputFile.close();
  }


  /**
   * This method allows the user to print the current viewport.
   *
   */
  void FileTool::print() {
    // Is there anything to print
    if (cubeViewport() == NULL) {
      QMessageBox::information((QWidget *)parent(), "Error", "No active cube to print");
      return;
    }

    // Initialize a printer
    static QPrinter *printer = NULL;
    if (printer == NULL) printer = new QPrinter;
    QPageSize pageSize(QPageSize::Letter);
    printer->setPageSize(pageSize);
    printer->setColorMode(QPrinter::GrayScale);
    if (cubeViewport()->isColor()) printer->setColorMode(QPrinter::Color);

    QPrintDialog printDialog(printer, (QWidget *)parent());
    if (printDialog.exec() == QDialog::Accepted) {
      // Get display widget as a pixmap and convert to an image
      QPixmap pixmap = cubeViewport()->viewport()->grab();
      QImage img = pixmap.toImage();

      // C++ Gui Programmign with Qt, page 201
      QPainter painter(printer);
      QRect rect = painter.viewport();
      QSize size = img.size();
      size.scale(rect.size(), Qt::KeepAspectRatio);
      painter.setViewport(rect.x(), rect.y(),
                          size.width(), size.height());
      painter.setWindow(img.rect());
      painter.drawImage(0, 0, img);
    }
  }


  /**
   * Try to close all open cubes and save/discard if necessary.
   */
  bool FileTool::closeAll() {
    //  Close all cubes
    // We must create a temporary list.  If not the actual
    // list size gets modified when a close occurs and not all
    // windows were being closed.
    MdiCubeViewport *d;
    QVector< MdiCubeViewport * > tempList(*cubeViewportList());
    for(int i = 0; i < (int)tempList.size(); i++) {
      d = tempList.at(i);
      //Set the current viewport to the one being closed
      setCubeViewport(d);

      if (!d->parentWidget()->close()) {
        return false;
      }
    }
    return true;
  }


  /**
   * Exit the program, this slot called when the exit is chosen from the File menu
   *
   * @internal
   *   @history  2007-02-13  Tracie Sucharski,  Close all cubes before exiting
   *   @history  2012-05-24 Steven Lambright - Just close the main window. This should handle
   *                            everything automatically.
   */
  void FileTool::exit() {
    QWidget *mainWindow = qobject_cast<QWidget *>(p_parent);

    if (mainWindow)
      mainWindow->close();
  }

  /**
   * This slot enables or disables save and save as.
   *
   * @param enable
   */
  void FileTool::enableSave(bool enable) {
    p_save->setEnabled(enable);
  }

  /**
   * This method is called when the tool is updated.
   *
   */
  void FileTool::updateTool() {
    if (cubeViewport() == NULL) {
      if (p_lastViewport != NULL) {
        p_lastViewport = NULL;
      }
      p_print->setEnabled(false);
      p_save->setEnabled(false);
      p_exportView->setEnabled(false);
      p_exportToList->setEnabled(false);
      p_saveAs->setEnabled(false);
      p_saveInfo->setEnabled(false);
    }
    else {
      if (p_lastViewport == NULL) {
        //Set the last viewport to the current viewport and connect signals to save and discard
        p_lastViewport = cubeViewport();
        connect(p_lastViewport, SIGNAL(saveChanges(CubeViewport *)), this, SLOT(save()));
      }
      else {
        if (p_lastViewport != cubeViewport()) {
          //If the viewport has changes made to it enable the save action
          if (cubeViewport()->parentWidget()->windowTitle().endsWith("*")) {
            p_save->setEnabled(true);
          }
          //Else disable it
          else {
            p_save->setEnabled(false);
          }
          //disconnect signals from the old viewport and connect them to the new viewport
          disconnect(p_lastViewport, SIGNAL(saveChanges(CubeViewport *)), this, SLOT(save()));
          disconnect(p_lastViewport, SIGNAL(discardChanges(CubeViewport *)), this, SLOT(discard()));
          p_lastViewport = cubeViewport();
          connect(p_lastViewport, SIGNAL(saveChanges(CubeViewport *)), this, SLOT(save()));
          connect(p_lastViewport, SIGNAL(discardChanges(CubeViewport *)), this, SLOT(discard()));
        }
      }
      p_print->setEnabled(true);
      p_exportView->setEnabled(true);
      p_exportToList->setEnabled(true);
      p_saveAs->setEnabled(true);
      p_saveInfo->setEnabled(true);
    }
  }
}
