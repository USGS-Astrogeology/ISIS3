/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */#include "UserInterface.h"

#include <sstream>
#include <vector>

#include <QDir>

#include "Application.h"
#include "FileName.h"
#include "Gui.h"
#include "IException.h"
#include "IString.h"
#include "Message.h"
#include "Preference.h"
#include "ProgramLauncher.h"
#include "TextFile.h"

using namespace std;
namespace Isis {
 /**
   * Constructs an UserInterface object.
   *
   * @param xmlfile Name of the Isis application xml file to open.
   *
   * @param argc Number of arguments on the command line.  Must be pass by
   *             reference!!
   *
   * @param argv[] Array of arguments
   */
  UserInterface::UserInterface(const QString &xmlfile, QVector<QString> &args) : IsisAml::IsisAml(xmlfile) {
    p_interactive = false;
    p_info = false;
    p_infoFileName = "";
    p_gui = NULL;
    p_errList = "";
    p_saveFile = "";
    p_abortOnError = true;
    p_parentId = 0;

    // Make sure the user has a .Isis and .Isis/history directory
    try {
      FileName setup = "$HOME/.Isis/history";
      // cannot completely test this if in unit test
      if ( !setup.fileExists() ) {
        setup.dir().mkpath(".");
      }
    }
    catch (IException &) {
    }

    // Parse the user input
    loadCommandLine(args);
  }


  /**
   * Constructs an UserInterface object.
   *
   * @param xmlfile Name of the Isis application xml file to open.
   *
   * @param argc Number of arguments on the command line.  Must be pass by
   *             reference!!
   *
   * @param argv[] Array of arguments
   */
  UserInterface::UserInterface(const QString &xmlfile, int &argc,
                               char *argv[]) : IsisAml::IsisAml(xmlfile) {
    p_interactive = false;
    p_info = false;
    p_infoFileName = "";
    p_gui = NULL;
    p_errList = "";
    p_saveFile = "";
    p_abortOnError = true;
    p_parentId = 0;

    // Make sure the user has a .Isis and .Isis/history directory
    try {
      FileName setup = "$HOME/.Isis/history";
      // cannot completely test this if in unit test
      if ( !setup.fileExists() ) {
        setup.dir().mkpath(".");
      }
    }
    catch (IException &) {
    }

    // Parse the user input
    loadCommandLine(argc, argv);

    // See if we need to create the gui
    // can't unit test - don't want to create a Gui object while unit testing
    if (p_interactive) {
      Gui::checkX11();
      p_gui = Gui::Create(*this, argc, argv);
    }
  }


  //! Destroys the UserInterface object
  UserInterface::~UserInterface() {
    // can't unit test - p_gui will be NULL in unit test
    if (p_gui) {
      delete p_gui;
      p_gui = NULL;
    }
  }

  /**
   * This method returns the filename where the debugging info is
   * stored when the "-info" tag is used.
   *
   * @return @b QString Name of file containing debugging ingo.
   */
  QString UserInterface::GetInfoFileName() {
    return p_infoFileName;
  }


  /**
   * This method returns the flag state of info. This returns if
   * its in debugging mode(the -info tag was specified).
   *
   * @return @b bool Flag state on info.
   */
  bool UserInterface::GetInfoFlag() {
    return p_info;
  }


  /**
   * Clears the gui parameters and sets the batch list information at line i as
   * the new parameters
   *
   * @param i The line number to retrieve parameter information from
   *
   * @throws Isis::IException::User - Invalid command line
   */
  void UserInterface::SetBatchList(int i) {
    //Clear all parameters currently in the gui
    for (int k = 0; k < NumGroups(); k++) {
      for (int j = 0; j < NumParams(k); j++) {
        Clear( ParamName(k, j) );
      }
    }

    //Load the new parameters into the gui
    cout << p_progName << " ";

    for (unsigned int currArgument = 1; currArgument < p_cmdline.size(); currArgument ++) {
      QString paramName;
      vector<QString> paramValue;


      try {
        getNextParameter(currArgument, paramName, paramValue);

        if (paramName[0] == '-')
          continue;

        for (unsigned int value = 0; value < paramValue.size(); value++) {
          IString thisValue = paramValue[value];
          QString token = thisValue.Token("$").ToQt();

          QString newValue;

          while (thisValue != "") {
            newValue += token;
            try {
              int j = toInt( thisValue.substr(0, 1).c_str() ) - 1;
              newValue += p_batchList[i][j];
              thisValue.replace(0, 1, "");
              token = thisValue.Token("$").ToQt();
            }
            catch (IException &e) {
              // Let the variable be parsed by the application
              newValue += "$";
              token = thisValue.Token("$").ToQt();
            }
          }

          if (token != "")
            newValue += token;

          paramValue[value] = newValue;
        }
      }
      // can't test with unit test - command line is already parsed before SetBatchList() is called
      catch (IException &e) {
        throw IException(IException::User, "Invalid command line", _FILEINFO_);
      }

      PutAsString(paramName, paramValue);

      cout << paramName;

      if(paramValue.size() == 1) {
        cout << "=" << paramValue[0] << " ";
      }
      else if (paramValue.size() > 1) {
        cout << "=(";

        for (unsigned int value = 0; value < paramValue.size(); value++) {
          if(value != 0)
            cout << ",";

          cout << paramValue[value] << endl;
        }

        cout << ") ";
      }
    }
    cout << endl;

    // Verify the command line
    VerifyAll();
  }


  /**
   * This method adds the line specified in the BatchList that the error occured
   * on.  The BatchList line is added exactly as it is seen, so the BatchList
   * command can be run on the errorlist file created.
   *
   * @param i The line of the batchlist to write to the error file
   *
   * @throws Isis::IException::User - Unable to create error list - Disk may be full or
   *                                  directory permissions not writeable
   */
  void UserInterface::SetErrorList(int i) {
    if (p_errList != "") {
      std::ofstream os;
      QString fileName( FileName(p_errList).expanded() );
      os.open(fileName.toLatin1().data(), std::ios::app);

      // did not unit test since it is assumed ofstream will be instantiated correctly
      if ( !os.good() ) {
        QString msg = "Unable to create error list [" + p_errList
                      + "] Disk may be full or directory permissions not writeable";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      for (int j = 0; j < (int) p_batchList[i].size(); j++) {
        os << p_batchList[i][j] << " ";
      }

      os << endl;
      os.close();
    }
  }


  /**
   * Saves the user parameter information in the history of the program for later
   * use
   */
  void UserInterface::SaveHistory() {

    // If history recording is off, return
    Preference &p = Preference::Preferences();
    PvlGroup &grp = p.findGroup("UserInterface", Isis::Pvl::Traverse);
    if (grp["HistoryRecording"][0] == "Off")
      return;

    // Get the current history file
    Isis::FileName histFile(grp["HistoryPath"][0] + "/" + ProgramName() + ".par");

    // If a save file is specified, override the default file path
    if (p_saveFile != "")
      histFile = p_saveFile;

    // Get the current command line
    Isis::Pvl cmdLine;
    CommandLine(cmdLine);

    Isis::Pvl hist;

    // If the history file's Pvl is corrupted, then
    // leave hist empty such that the history gets
    // overwriten with the new entry.
    try {
      if ( histFile.fileExists() ) {
        hist.read( histFile.expanded() );
      }
    }
    catch (IException &) {
    }

    // Add it
    hist.addGroup( cmdLine.findGroup("UserParameters") );

    // See if we have exceeded history length
    while( hist.groups() > toInt(grp["HistoryLength"][0]) ) {
      hist.deleteGroup("UserParameters");
    }

    // Write it
    try {
      hist.write( histFile.expanded() );
    }
    catch (IException &) {
    }
  }


  /**
   * Loads the user entered batchlist file into a private variable for later use
   *
   * @param file The batchlist file to load
   *
   *  @history 2010-03-26 Sharmila Prasad - Remove the restriction of the number of
   *           columns in the batchlist file to 10.
   * @throws Isis::IException::User - The batchlist file could not be opened
   * @throws Isis::IException::User - The number of columns in the batchlist file must be
   *                                  consistent
   * @throws Isis::IException::User - The batchlist does not contain any data
   */
  void UserInterface::loadBatchList(const QString file) {
    // Read in the batch list
    TextFile temp;
    try {
      temp.Open(file);
    }
    catch (IException &e) {
      QString msg = "The batchlist file [" + file + "] could not be opened";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    p_batchList.resize( temp.LineCount() );

    for (int i = 0; i < temp.LineCount(); i++) {
      QString t;
      temp.GetLine(t);

      // Convert tabs to spaces but leave tabs inside quotes alone
      t = IString(t).Replace("\t", " ", true).ToQt();

      t = IString(t).Compress().ToQt().trimmed();
      // Allow " ," " , " or ", " as a valid single seperator
      t = IString(t).Replace(" ,", ",", true).ToQt();
      t = IString(t).Replace(", ", ",", true).ToQt();
      // Convert all spaces to "," the use "," as delimiter
      t = IString(t).Replace(" ", ",", true).ToQt();
      int j = 0;

      QStringList tokens = t.split(",");

      foreach (QString token, tokens) {
        // removes quotes from tokens. NOTE: also removes escaped quotes.
        token = token.remove( QRegExp("[\"']") );
        p_batchList[i].push_back(token);
        j++ ;
      }

      p_batchList[i].resize(j);
      // Every row in the batchlist must have the same number of columns
      if (i == 0)
        continue;
      if ( p_batchList[i - 1].size() != p_batchList[i].size() ) {
        QString msg = "The number of columns must be constant in batchlist";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    // The batchlist cannot be empty
    if (p_batchList.size() < 1) {
      QString msg = "The list file [" + file + "] does not contain any data";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }



/**
   * This is used to load the command line into p_cmdline and the Aml object
   * using information contained in argc and argv.
   *
   * @param args QVector of arguments
   *
   * @throws Isis::IException::User - Invalid value for reserve parameter
   * @throws Isis::IException::User - Invalid command line
   * @throws Isis::IException::User - -BATCHLIST cannot be used with -GUI, -SAVE,
   *                                  -RESTORE, or -LAST
   * @throws Isis::IException::User - -ERRLIST and -ONERROR=continue cannot be used without
   *                                  -BATCHLIST
   */
  void UserInterface::loadCommandLine(QVector<QString> &args, bool ignoreAppName) {
    char **c_args;

    if (ignoreAppName) {
      args.prepend("someapp");
    }

    c_args = (char**)malloc(sizeof(char*)*args.size());

    for (int i = 0; i < args.size(); i++) {
      c_args[i] = (char*)malloc(sizeof(char)*args[i].size()+1);
      strcpy(c_args[i], args[i].toLatin1().data());
    }

    loadCommandLine(args.size(), c_args);
  }


  /**
   * This is used to load the command line into p_cmdline and the Aml object
   * using information contained in argc and argv.
   *
   * @param argc Number of arguments on the command line
   *
   * @param argv[] Array of arguments
   *
   * @throws Isis::IException::User - Invalid value for reserve parameter
   * @throws Isis::IException::User - Invalid command line
   * @throws Isis::IException::User - -BATCHLIST cannot be used with -GUI, -SAVE,
   *                                  -RESTORE, or -LAST
   * @throws Isis::IException::User - -ERRLIST and -ONERROR=continue cannot be used without
   *                                  -BATCHLIST
   */
  void UserInterface::loadCommandLine(int argc, char *argv[]) {
    // The program will be interactive if it has no arguments or
    // if it has the name unitTest
    p_progName = argv[0];
    Isis::FileName file(p_progName);
    // cannot completely test in a unit test since unitTest will always evaluate to true
    if ( (argc == 1) && (file.name() != "unitTest") ) {
      p_interactive = true;
    }

    p_cmdline.clear();
    for (int i = 0; i < argc; i++) {
      p_cmdline.push_back(argv[i]);
    }

    // Check for special tokens (reserved parameters) (those beginning with a dash)
    vector<QString> options;
    options.push_back("-GUI");
    options.push_back("-NOGUI");
    options.push_back("-BATCHLIST");
    options.push_back("-LAST");
    options.push_back("-RESTORE");
    options.push_back("-WEBHELP");
    options.push_back("-HELP");
    options.push_back("-ERRLIST");
    options.push_back("-ONERROR");
    options.push_back("-SAVE");
    options.push_back("-INFO");
    options.push_back("-PREFERENCE");
    options.push_back("-LOG");
    options.push_back("-VERBOSE");
    options.push_back("-PID");

    bool usedDashLast = false;
    bool usedDashRestore = false; //< for throwing -batchlist exceptions at end of function

    // pre-process command line for -HELP first
    preProcess("-HELP", options);
    // pre-process command line for -WEBHELP
    preProcess("-WEBHELP", options);
    // now, parse command line to evaluate -LAST
    preProcess("-LAST", options);

    for (unsigned int currArgument = 1; currArgument < (unsigned)argc; currArgument++) {
      QString paramName;
      vector<QString> paramValue;

      getNextParameter(currArgument, paramName, paramValue);

      // we now have a name,value pair
      if (paramName[0] == '-') {
        paramName = paramName.toUpper();

        // where if(paramname == -last ) to continue } was originally

        if (paramValue.size() > 1) {
          QString msg = "Invalid value for reserve parameter ["
                       + paramName + "]";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        // resolve the reserved parameter (e.g. set -h to -HELP)
        paramName = resolveParameter(paramName, options);
        


        // Prevent double handling of -LAST to prevent conflicts
        // Keep track of using -LAST to prevent conflicts with -BATCHLIST
        if (paramName == "-LAST") {
          usedDashLast = true;
          continue;
        }


        // Keep track of using -RESTORE to prevent conflicts with -BATCHLIST
        if (paramName == "-RESTORE") {
          usedDashRestore = true;
        }


        QString realValue = "";

        if ( paramValue.size() ) {
          realValue = paramValue[0];
        }

        evaluateOption(paramName, realValue);

        continue;
      }

      try {
        Clear(paramName);
        PutAsString(paramName, paramValue);
      }
      catch (IException &e) {
        throw IException(e, IException::User, "Invalid command line", _FILEINFO_);
      }

    }
    if(usedDashLast) {
      Pvl temp;
      CommandLine(temp);
      cout << BuildNewCommandLineFromPvl(temp) << endl;
    }

    // Can't use the batchlist with the gui, save, last or restore option
    if ( BatchListSize() != 0 && (p_interactive || usedDashLast || p_saveFile != ""
                                  || usedDashRestore) ) {
      QString msg = "-BATCHLIST cannot be used with -GUI, -SAVE, -RESTORE, ";
      msg += "or -LAST";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    // Must use batchlist if using errorlist or onerror=continue
    if ( (BatchListSize() == 0) && (!p_abortOnError || p_errList != "") ) {
      QString msg = "-ERRLIST and -ONERROR=continue cannot be used without ";
      msg += " the -BATCHLIST option";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  QString UserInterface::BuildNewCommandLineFromPvl(Pvl temp){
    PvlGroup group = temp.group(0);
    int numKeywords = group.keywords();
    QString returnVal = p_progName + " ";

    for(int i = 0; i < numKeywords; i++){
      PvlKeyword key = group[i];
      returnVal += key.name();
      returnVal += "=";
      returnVal += QString(key);
      returnVal += " ";
    }
    return returnVal;
  }
  /**
   * Loads the previous history for the program
   *
   * @param file FileName to get the history entry from
   *
   * @throws Isis::IException::User - The file does not contain any parameters
   *                                  to restore
   * @throws Isis::IException::User - The file does not contain a valid parameter
   *                                  history file
   * @throws Isis::IException::User - Parameter history file does not exist
   */
  void UserInterface::loadHistory(const QString file) {
    Isis::FileName hist(file);
    if ( hist.fileExists() ) {
      try {
        Isis::Pvl lab( hist.expanded() );

        int g = lab.groups() - 1;
        if (g >= 0 && lab.group(g).isNamed("UserParameters") ) {
          Isis::PvlGroup &up = lab.group(g);
          QString commandline(p_progName + " ");
          for (int k = 0; k < up.keywords(); k++) {
            QString keyword = up[k].name();

            vector<QString> values;

            for (int i = 0; i < up[k].size(); i++) {
              values.push_back(up[k][i]);
            }

            const IsisParameterData *paramData = ReturnParam(keyword);

            bool matchesDefault = false;
            if (values.size() == 1 && paramData->internalDefault == values[0])
              matchesDefault = true;

            if (!matchesDefault) {
              matchesDefault =
                  (values.size() == paramData->defaultValues.size());

              for (int i = 0; matchesDefault && i < (int)values.size(); i++) {
                matchesDefault = matchesDefault &&
                                 values[i] == paramData->defaultValues[i];
              }
            }

            if (!matchesDefault) {
              PutAsString(keyword, values);
              commandline += keyword + "=";
              foreach(QString val, values) {
                commandline += val + " ";
              }
            }
          }

          return;
        }

        for (int o = lab.objects() - 1; o >= 0; o--) {
          if ( lab.object(o).isNamed( ProgramName() ) ) {
            Isis::PvlObject &obj = lab.object(o);
            for (int g = obj.groups() - 1; g >= 0; g--) {
              Isis::PvlGroup &up = obj.group(g);
              if ( up.isNamed("UserParameters") ) {
                for (int k = 0; k < up.keywords(); k++) {
                  QString keyword = up[k].name();
                  QString value = up[k][0];
                  PutAsString(keyword, value);
                }
              }
              return;
            }
          }
        }

        /*QString msg = "[" + hist.expanded() +
         "] does not contain any parameters to restore";
         throw Isis::iException::Message( Isis::iException::User, msg, _FILEINFO_ );*/
      }
      catch (...) {
        QString msg = "The history file [" + file + "] is for a different application or corrupt, "\
                      "please fix or delete this file";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else {
      QString msg = "The history file [" + file + "] does not exist";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }


  /**
   * This interprets the "-" options for reserved parameters
   *
   * @param name "-OPTIONNAME" (name of the reserved parameter)
   * @param value Value of the option, if supplied (-name=value)
   *
   * @throws Isis::IException::Programmer - evaluating -WEBHELP throws an exception when
   *                                        unit testing to avoid exiting from the unit test
   * @throws Isis::IException::Programmer - evaluating -HELP throws an exception when
   *                                        unit testing to avoid exiting from the unit test
   * @throws Isis::IException::User - -ERRLIST expects a file name
   * @throws Isis::IException::User - -ONERROR only accpets CONTINUE and ABORT as valid values
   * @throws Isis::IException::Unknown - -GUI and -PID are incompatible arguments
   *
   * @internal
   * @history 2021-06-05 Kris Becker - Fixed path to ISIS docs
   */
  void UserInterface::evaluateOption(const QString name,
                                     const QString value) {
    // check to see if the program is a unitTest
    bool unitTest = false;
      if (FileName(p_progName).name() == "unitTest") {
        unitTest = true;
      }
    Preference &p = Preference::Preferences();

    if (name == "-GUI") {
      p_interactive = true;
    }
    else if (name == "-NOGUI") {
      p_interactive = false;
    }
    else if (name == "-BATCHLIST") {
      loadBatchList(value);
    }
    else if (name == "-LAST") {
      QString histFile;
      // need to handle for unit test since -LAST is preprocessed
      if (unitTest) {
        histFile = "./" + FileName(p_progName).name() + ".par";
      }
      else {
        PvlGroup &grp = p.findGroup("UserInterface", Isis::Pvl::Traverse);
        histFile = grp["HistoryPath"][0] + "/" + FileName(p_progName).name() + ".par";
      }

      loadHistory(histFile);
    }
    else if(name == "-RESTORE") {
      loadHistory(value);
    }
    else if(name == "-WEBHELP") {
      Isis::PvlGroup &pref = Isis::Preference::Preferences().findGroup("UserInterface");
      QString command = pref["GuiHelpBrowser"];
      command += " $ISISROOT/docs/Application/presentation/Tabbed/";
      command += FileName(p_progName).name() + "/" + FileName(p_progName).name() + ".html";
      // cannot test else in unit test - don't want to open webhelp
      if (unitTest) {
        throw IException(IException::Programmer,
                         "Evaluating -WEBHELP should only throw this exception during a unitTest",
                         _FILEINFO_);
      }
      else {
        ProgramLauncher::RunSystemCommand(command);
        exit(0);
      }

    }
    else if (name == "-INFO") {
      p_info = true;

      // check for filename and set value
      if (value.size() != 0) {
        p_infoFileName = value;
      }
    }
    else if (name == "-HELP") {
      if (value.size() == 0) {
        Pvl params;
        params.setTerminator("");
        for (int k = 0; k < NumGroups(); k++) {
          for (int j = 0; j < NumParams(k); j++) {
            if (ParamListSize(k, j) == 0) {
              params += PvlKeyword( ParamName(k, j), ParamDefault(k, j) );
            }
            else {
              PvlKeyword key( ParamName(k, j) );
              QString def = ParamDefault(k, j);
              for (int l = 0; l < ParamListSize(k, j); l++) {
                if (ParamListValue(k, j, l) == def)
                  key.addValue("*" + def);
                else
                  key.addValue( ParamListValue(k, j, l) );
              }
              params += key;
            }
          }
        }
        cout << params;
      }
      else {
        Pvl param;
        param.setTerminator("");
        QString key = value;
        for (int k = 0; k < NumGroups(); k++) {
          for (int j = 0; j < NumParams(k); j++) {
            if (ParamName(k, j) == key) {
              param += PvlKeyword("ParameterName", key);
              param += PvlKeyword( "Brief", ParamBrief(k, j) );
              param += PvlKeyword( "Type", ParamType(k, j) );
              if (PixelType(k, j) != "") {
                param += PvlKeyword( "PixelType", PixelType(k, j) );
              }
              if (ParamInternalDefault(k, j) != "") {
                param += PvlKeyword( "InternalDefault", ParamInternalDefault(k, j) );
              }
              else {
                param += PvlKeyword( "Default", ParamDefault(k, j) );
              }
              if (ParamMinimum(k, j) != "") {
                if (ParamMinimumInclusive(k, j).toUpper() == "YES") {
                  param += PvlKeyword( "GreaterThanOrEqual",
                                       ParamMinimum(k, j) );
                }
                else {
                  param += PvlKeyword( "GreaterThan",
                                       ParamMinimum(k, j) );
                }
              }
              if (ParamMaximum(k, j) != "") {
                if (ParamMaximumInclusive(k, j).toUpper() == "YES") {
                  param += PvlKeyword( "LessThanOrEqual",
                                       ParamMaximum(k, j) );
                }
                else {
                  param += PvlKeyword( "LessThan",
                                       ParamMaximum(k, j) );
                }
              }
              if (ParamLessThanSize(k, j) > 0) {
                PvlKeyword key("LessThan");
                for(int l = 0; l < ParamLessThanSize(k, j); l++) {
                  key.addValue( ParamLessThan(k, j, l) );
                }
                param += key;
              }
              if (ParamLessThanOrEqualSize(k, j) > 0) {
                PvlKeyword key("LessThanOrEqual");
                for (int l = 0; l < ParamLessThanOrEqualSize(k, j); l++) {
                  key.addValue( ParamLessThanOrEqual(k, j, l) );
                }
                param += key;
              }
              if (ParamNotEqualSize(k, j) > 0) {
                PvlKeyword key("NotEqual");
                for (int l = 0; l < ParamNotEqualSize(k, j); l++) {
                  key.addValue( ParamNotEqual(k, j, l) );
                }
                param += key;
              }
              if (ParamGreaterThanSize(k, j) > 0) {
                PvlKeyword key("GreaterThan");
                for (int l = 0; l < ParamGreaterThanSize(k, j); l++) {
                  key.addValue( ParamGreaterThan(k, j, l) );
                }
                param += key;
              }
              if (ParamGreaterThanOrEqualSize(k, j) > 0) {
                PvlKeyword key("GreaterThanOrEqual");
                for(int l = 0; l < ParamGreaterThanOrEqualSize(k, j); l++) {
                  key.addValue( ParamGreaterThanOrEqual(k, j, l) );
                }
                param += key;
              }
              if (ParamIncludeSize(k, j) > 0) {
                PvlKeyword key("Inclusions");
                for (int l = 0; l < ParamIncludeSize(k, j); l++) {
                  key.addValue( ParamInclude(k, j, l) );
                }
                param += key;
              }
              if (ParamExcludeSize(k, j) > 0) {
                PvlKeyword key("Exclusions");
                for (int l = 0; l < ParamExcludeSize(k, j); l++) {
                  key.addValue( ParamExclude(k, j, l) );
                }
                param += key;
              }
              if (ParamOdd(k, j) != "") {
                param += PvlKeyword( "Odd", ParamOdd(k, j) );
              }
              if (ParamListSize(k, j) != 0) {
                for (int l = 0; l < ParamListSize(k, j); l++) {
                  PvlGroup grp( ParamListValue(k, j, l) );
                  grp += PvlKeyword( "Brief", ParamListBrief(k, j, l) );
                  if (ParamListIncludeSize(k, j, l) != 0) {
                    PvlKeyword include("Inclusions");
                    for (int m = 0; m < ParamListIncludeSize(k, j, l); m++) {
                      include.addValue( ParamListInclude(k, j, l, m) );
                    }
                    grp += include;
                  }
                  if (ParamListExcludeSize(k, j, l) != 0) {
                    PvlKeyword exclude("Exclusions");
                    for (int m = 0; m < ParamListExcludeSize(k, j, l); m++) {
                      exclude.addValue( ParamListExclude(k, j, l, m) );
                    }
                    grp += exclude;
                  }
                  param.addGroup(grp);
                }
              }
              cout << param;
            }
          }
        }
      }
      // we must throw an exception for unitTest to handle to continue testing
      if (unitTest) {
        throw IException(IException::Programmer,
                         "Evaluating -HELP should only throw this exception during a unitTest",
                         _FILEINFO_);
      }
      // all other apps shall exit when -HELP is present
      else {
        exit(0);
      }
    }
    else if (name == "-PID") {
      p_parentId = toInt(value);
    }
    else if (name == "-ERRLIST") {
      p_errList = value;

      if (value == "") {
        QString msg = "-ERRLIST expects a file name";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      if ( FileName(p_errList).fileExists() ) {
        QFile::remove(p_errList);
      }
    }
    else if (name == "-ONERROR") {
      if (value.toUpper() == "CONTINUE") {
        p_abortOnError = false;
      }

      else if (value.toUpper() == "ABORT") {
        p_abortOnError = true;
      }

      else {
        QString msg = "[" + value
                      + "] is an invalid value for -ONERROR, options are ABORT or CONTINUE";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    else if (name == "-SAVE") {
      if (value.size() == 0) {
        p_saveFile = ProgramName() + ".par";
      }
      else {
        p_saveFile = value;
      }
    }
    else if (name == "-PREFERENCE") {
      p.Load(value);
      p_preference = value;
    }
    else if (name == "-LOG") {
      if( value.isEmpty() ) {
        p.findGroup("SessionLog")["FileOutput"].setValue("On");
      }
      else {
        p.findGroup("SessionLog")["FileOutput"].setValue("On");
        p.findGroup("SessionLog")["FileName"].setValue(value);
      }
    }
    // this only evaluates to true in unit test since this is last else if
    else if (name == "-VERBOSE") {
      p.findGroup("SessionLog")["TerminalOutput"].setValue("On");
    }

    // Can't have a parent id and the gui
    if (p_parentId > 0 && p_interactive) {
      QString msg = "-GUI and -PID are incompatible arguments";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }
  }



  /**
   * This gets the next parameter in the list of arguments. curPos will be changed
   * to be the end of the current argument (still needs incremented to get the
   * next argument).
   *
   * @param curPos End of previous argument
   * @param name Resulting parameter name
   * @param value Resulting array of parameter values (usually just 1 element)
   *
   * @throws Isis::IException::User - parameters cannot start with "="
   */
  void UserInterface::getNextParameter(unsigned int &curPos,
                                       QString &name,
                                       std::vector<QString> &value) {
    QString paramName = p_cmdline[curPos];
    QString paramValue = "";

    // we need to split name and value, they can either be in 1, 2 or 3 arguments,
    //   try to see if "=" is end of argument to distinguish. Some options have no
    //   "=" and they are value-less (-gui for example).
    if ( !paramName.contains("=") ) {
      // This looks value-less, but lets make sure
      //   the next argument is not an equals sign by
      //   itself
      if (curPos < p_cmdline.size() - 2) {
        if (QString(p_cmdline[curPos + 1]).compare("=") == 0) {
          paramValue = p_cmdline[curPos + 2];

          // increment extra to skip 2 elements next time around
          curPos += 2;
        }
      }
    }
    // = is end of parameter, next item must be value
    else if ( paramName.endsWith("=") ) {
      paramName = paramName.mid(0, paramName.size() - 1);

      if (curPos + 1 < p_cmdline.size() ) {
        paramValue = p_cmdline[curPos + 1];
      }

      // increment extra to skip next element next time around
      curPos++ ;
    }
    // we found "=" in the middle
    else if (paramName.indexOf("=") > 0) {
      QString parameterLiteral = p_cmdline[curPos];
      paramName = parameterLiteral.mid( 0, parameterLiteral.indexOf("=") );
      paramValue = parameterLiteral.mid(parameterLiteral.indexOf("=") + 1);
    }
    // We found "=" at the beginning - did we find "appname param =value" ?
    else {
      // parameters can not start with "="
      QString msg = "Unknown parameter [" + QString(p_cmdline[curPos])
                   + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    name = paramName;
    value.clear();

    // read arrays out of paramValue
    paramValue = paramValue.trimmed();

    if (paramValue.length() > 0 && paramValue[0] != '(') {
      // We dont have an array... if they escaped
      //  an open paren, undo their escape

      // escape: \( result: (
      if (paramValue.length() > 1 && paramValue.mid(0, 2) =="\\(") {
        paramValue = paramValue.mid(1);
      }
      // escape: \\( result: \(
      else if (paramValue.length() > 2 && paramValue.mid(0, 3) == "\\\\(") {
        paramValue = paramValue.mid(1);
      }

      value.push_back(paramValue);
    }
    else if ( paramValue.length() ) {
      // We have an array...
      value = readArray(paramValue);
    }
  }


  /**
   * This parses the command line and looks for the specified reserved parameter
   * name passed. Resolves and evaluates the passed reserved parameter.
   * This method ignores invalid parameters ( @see resolveParameter() ).
   *
   * Example: preProcess("-HELP", options) will try to resolve any reserved parameters
   * and will evaluate if one resolves to -HELP.
   *
   * @param fullReservedName the full name of reserved parameter being looked for
   * @param reservedParams the list of reserved parameters for resolving parameter name
   *
   */
  void UserInterface::preProcess(QString fullReservedName,
                                 std::vector<QString> &reservedParams) {
    for (unsigned int currArgument = 1; currArgument < (unsigned)p_cmdline.size();
         currArgument++) {

      QString paramName = p_cmdline[currArgument];
      QString trueParamValue = "";
      vector<QString> paramValue;

      // reserved parameters start with -
      if (paramName[0] == '-') {

        // grab the current argument
        getNextParameter(currArgument, paramName, paramValue);
        paramName = paramName.toUpper();

        // grab the argument's value
        if ( paramValue.size() ) {
          trueParamValue = paramValue[0].toUpper();
        }

        // resolve the reserved parameter token
        paramName = resolveParameter(paramName, reservedParams, false);

        // evaluate the resolved parameter if it matches fullReservedName
        if (fullReservedName == paramName) {
          evaluateOption(paramName, trueParamValue);
        }
      }
    }
  }


  /**
   * This interprets an array value from the command line.
   *
   * @param arrayString Parameter value containing an array of
   *   format (a,b,c)
   *
   * @return std::vector<QString> Values in the array QString
   *
   * @throws Isis::IException::User - arrays not starting with '(' are invalid
   * @throws Isis::IException::User - arrays ending in a backslash are invalid
   * @throws Isis::IException::User - invalid array format
   * @throws Isis::IException::User - invalid array format
   */
  std::vector<QString> UserInterface::readArray(QString arrayString) {
    std::vector<QString> values;

    bool inDoubleQuotes = false;
    bool inSingleQuotes = false;
    bool arrayClosed = false;
    bool nextElementStarted = false;
    QString currElement = "";

    for (int strPos = 0; strPos < arrayString.size(); strPos++) {
      if (strPos == 0) {
        if (arrayString[strPos] != '(') {
          QString msg = "Invalid array format [" + arrayString + "]";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        continue;
      }

      // take literally anything that is escaped and not quoted
      if ( arrayString[strPos] == '\\' && strPos + 1 < (int)arrayString.size() ) {
        currElement += arrayString[strPos+1];
        strPos++;
        continue;
      }
      // ends in a backslash??
      else if (arrayString[strPos] == '\\') {
        QString msg = "Invalid array format [" + arrayString + "]";
        throw IException(IException::User, msg, _FILEINFO_);
      }

      // not in quoted part of QString
      if (!inDoubleQuotes && !inSingleQuotes) {
        if (arrayClosed) {
          QString msg = "Invalid array format [" + arrayString + "]";
          throw IException(IException::User, msg, _FILEINFO_);
        }

        nextElementStarted = (nextElementStarted || arrayString[strPos] != ' ');

        if (!nextElementStarted) {
          continue;
        }

        if (arrayString[strPos] == '"') {
          inDoubleQuotes = true;
        }
        else if (arrayString[strPos] == '\'') {
          inSingleQuotes = true;
        }
        else if (arrayString[strPos] == ',') {
          values.push_back(currElement);
          currElement = "";
          nextElementStarted = false;
        }
        else if (arrayString[strPos] == ')') {
          values.push_back(currElement);
          currElement = "";
          arrayClosed = true;
          nextElementStarted = false;
        }
        else if (nextElementStarted && arrayString[strPos] == ' ') {
          // Make sure there's something before the next ',' or ')'
          bool onlyWhite = true;

          for( int pos = strPos;
               onlyWhite && arrayString[pos] != ',' && arrayString[pos] != ')' &&
               pos < arrayString.size(); pos++) {
            onlyWhite &= (arrayString[pos] == ' ');
          }

          if (!onlyWhite) {
            currElement += arrayString[strPos];
          }
        }
        else if (nextElementStarted) {
          currElement += arrayString[strPos];
        }
      }
      else if (inSingleQuotes) {
        if(arrayString[strPos] == '\'') {
          inSingleQuotes = false;
        }
        else {
          currElement += arrayString[strPos];
        }
      }
      // in double quotes
      else {
        if (arrayString[strPos] == '"') {
          inDoubleQuotes = false;
        }
        else {
          currElement += arrayString[strPos];
        }
      }
    }

    if (!arrayClosed || currElement != "") {
      QString msg = "Invalid array format [" + arrayString + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }

    return values;
  }


  /**
   * This resolves a reserved parameter token on the command line to its fullname.
   * Matches with the list of reserved parameters (options).
   * Resolution necessary for evaluateOption().
   *
   * Example: an -h token on the command line will resolve to -HELP
   *
   * @param unresolvedParam the parameter name that needs to be resolved
   * @param reservedParams the list of reserved parameters for resolving parameter name
   * @param handleNoMatches boolean value defaulted to true for handling invalid reserved
   *                        parameters
   *
   * @return @b QString the resolved parameter name
   *
   * @throws Isis::IException::User - unresolved reserved parameter is ambigious
   * @throws Isis::IException::User - reserved parameter cannot be matched (invalid)
   *
   */
  QString UserInterface::resolveParameter(QString &unresolvedParam,
                                          std::vector<QString> &reservedParams,
                                          bool handleNoMatches) {
    // index of the reserved parameter in options that matches the cmdline parameter
    int matchOption = -1;
    // determine if the reserved parameter on cmdline is shortened (e.g. -h for -HELP)
    for (int option = 0; option < (int)reservedParams.size(); option++) {
      // If our option starts with the parameter name so far, this is it
      if ( reservedParams[option].startsWith(unresolvedParam) ) {
        if (matchOption >= 0) {
          QString msg = "Ambiguous Reserve Parameter ["
          + unresolvedParam + "]. Please clarify.";
          throw IException(IException::User, msg, _FILEINFO_);
        }
        // set match to current iteration in loop
        matchOption = option;
      }
    }
    // handle matches by default
    if (handleNoMatches) {
      // handle no matches
      if (matchOption < 0) {
        QString msg = "Invalid Reserve Parameter Option ["
        + unresolvedParam + "]. Choices are ";

        QString msgOptions;
        for (int option = 0; option < (int)reservedParams.size(); option++) {
          // Make sure not to show -PID as an option
          if (reservedParams[option].compare("-PID") == 0) {
            continue;
          }

          msgOptions += reservedParams[option];
          msgOptions += ",";

          // this condition will never evaluate to FALSE -
          // this condition is only reachable when reservedParams.size() == 0 (empty) -
          // if this is the case, this for loop will never be entered
//           if ( !reservedParams.empty() ) {
//             msgOptions += ",";
//           }
        }

        // remove the terminating ',' from msgOptions
        msgOptions.chop(1);
        msg += " [" + msgOptions + "]";

        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
    if (matchOption < 0) {
      return "";
    }
    else {
      return reservedParams[matchOption];
    }
  }
} // end namespace isis
