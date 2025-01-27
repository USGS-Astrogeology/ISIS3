/**                                                                       
 * @file                                                                  
 * $Revision: 6187 $
 * $Date: 2015-05-11 17:31:51 -0700 (Mon, 11 May 2015) $
 * $Id: RunCommandStrategy.cpp 6187 2015-05-12 00:31:51Z kbecker@GS.DOI.NET $
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
 */ 
#include "RunCommandStrategy.h"

// boost library
#include <boost/foreach.hpp>

// other ISIS
#include "IException.h"
#include "ProgramLauncher.h"
#include "PvlFlatMap.h"
#include "PvlObject.h"
#include "Resource.h"

using namespace std;

namespace Isis {

  /** 
   *  Default constructor.
   */
  RunCommandStrategy::RunCommandStrategy() : Strategy("RunCommand", 
                                                      "RunCommand"),
                                             m_preCommands(), m_commands(), 
                                             m_postCommands(), m_argKeys(),
                                             m_skipAllNoData(true), 
                                             m_onPreCommandErrorContinue(false),
                                             m_onPostCommandErrorContinue(false) {
  }
  

  /**
   * @brief Constructor loads from a Strategy object RunCommand definition
   *  
   * This constructor loads and retains processing parameters from the RunCommand 
   * Strategy object definition as (typically) read from the configuration file. 
   *  
   * @author 2015-05-14 Kris Becker
   * 
   * @param definition RunCommand Strategy PVL object definition
   * @param globals   List of global keywords to use in argument substitutions
   */
  RunCommandStrategy::RunCommandStrategy(const PvlObject &definition, 
                                         const ResourceList &globals) : 
                                         Strategy(definition, globals),
                                         m_preCommands(), m_commands(), 
                                         m_postCommands(), m_argKeys(),
                                         m_skipAllNoData(true), 
                                         m_onPreCommandErrorContinue(false),
                                         m_onPostCommandErrorContinue(false)  {

    // Get command argument keyword replacement values
    if ( definition.hasKeyword("CommandArgs") ) {
      PvlFlatMap args;
      args.add(definition["CommandArgs"]);
      m_argKeys = args.allValues("CommandArgs");
    }

    // Flatten RunCommand Strategy object definition
    PvlObject config( getDefinition() );
    if ( config.hasKeyword("Command") ) {
      m_commands.push_back(Command("Command", QString::fromStdString(config["Command"][0])));
    }

    // Read and store all PRE commands
    if ( config.hasGroup("PreCommands") ) {
      PvlGroup &commands = config.findGroup("PreCommands");
      PvlContainer::ConstPvlKeywordIterator key = commands.begin();
      while ( key != commands.end() ) {
         m_preCommands.push_back(Command(QString::fromStdString(key->name()), QString::fromStdString((*key)[0])));
         ++key;
      }
    }

    // Get commands applied to each Resource
    if ( config.hasGroup("Commands") ) {
      PvlGroup &commands = config.findGroup("Commands");
      PvlContainer::ConstPvlKeywordIterator key = commands.begin();
      while ( key != commands.end() ) {
         m_commands.push_back(Command(QString::fromStdString(key->name()), QString::fromStdString((*key)[0])));
         ++key;
      }
    }

    // Retrieve and store all POST commands
    if ( config.hasGroup("PostCommands") ) {
      PvlGroup &commands = config.findGroup("PostCommands");
      PvlContainer::ConstPvlKeywordIterator key = commands.begin();
      while ( key != commands.end() ) {
         m_postCommands.push_back(Command(QString::fromStdString(key->name()), QString::fromStdString((*key)[0])));
         ++key;
      }
    }


    // Determine command execution when Resource list is empty of good data
    // and action taken when pre and post commands fail
    QStringList excludes;
    excludes << "Commands" << "PreCommands" << "PostCommands";
    PvlFlatMap skeys( getDefinition(), PvlConstraints::withExcludes(excludes));
    m_skipAllNoData   = toBool(skeys.get("SkipCommandsIfNoData", "true").toStdString());
    m_onPreCommandErrorContinue = toBool(skeys.get("OnPreCommandErrorContinue", "false").toStdString());
    m_onPostCommandErrorContinue = toBool(skeys.get("OnPostCommandErrorContinue", "false").toStdString());

    return; 
  }
  

  /** 
   *  Destructor.
   */
  RunCommandStrategy::~RunCommandStrategy() { 
  }
  

/** 
 * @brief Process a list of Resources 
 *  
 * Executes PRE and POST commands while between them, execute any commands for a 
 * each Resource using the single Resorce apply() method. 
 *  
 * @author 2015-05-26 Kris Becker
 * 
 * @param resources List of Resources to process 
 * @param globals   Global parameter pool
 * 
 * @return int  Number of resources commands are applied to
 */
  int RunCommandStrategy::apply(ResourceList &resources, 
                                const ResourceList &globals) {

    // Check execution disposition
    int n = ( isApplyToDiscarded() ) ? resources.size() : countActive(resources);
    if ( (0 == n) && (m_skipAllNoData) ) {
      return (0);
    }

    int total = 0;

    // Run all pre-command commands
    BOOST_FOREACH ( Command command, m_preCommands ) {
      QString cmd = processArgs(command.m_command, m_argKeys, globals);
      cmd = scanAndReplace(cmd, "&quot;", "\"");
      cmd = scanAndReplace(cmd, "&apos;", "\'");

      if ( isDebug() ) { 
        cout << "Running " << command.m_name.toStdString() << " PRE command: " << cmd.toStdString() << "\n"; 
      }
      int status = system(cmd.toLatin1().data());
      if ( 0 != status) {
        if ( !m_onPreCommandErrorContinue ) {
          std::string mess = command.m_name.toStdString() + " RunCommand::PreCommand failed - " + 
                         cmd.toStdString() + " - you are terminated!";
          throw IException(IException::User, mess, _FILEINFO_);
        }
      }
      total++;
    }

    // Now apply the commands to each resource if there are any commands 
    // specified for each resource
    if ( m_commands.size() > 0 ) {
       total += applyToResources(resources, globals);
    }
     
    // Run all post-command commands
    BOOST_FOREACH ( Command command, m_postCommands ) {
      QString cmd = processArgs(command.m_command, m_argKeys, globals);
      cmd = scanAndReplace(cmd, "&quot;", "\"");
      cmd = scanAndReplace(cmd, "&apos;", "\'");

      if ( isDebug() ) { 
        cout << "Running " << command.m_name.toStdString() << " POST command: " << cmd.toStdString() << "\n"; 
      }

      // Check status and disposition
      int status = system(cmd.toLatin1().data());
      if ( 0 != status) {
        if ( !m_onPostCommandErrorContinue ) {
          std::string mess = command.m_name.toStdString() + " RunCommand::PostCommand failed - " + 
                         cmd.toStdString() + " - you are terminated!";
          throw IException(IException::User, mess, _FILEINFO_);
        }
      }
      total++;
    }

    return (total);
  }

  /**
   * @brief Excecute commands to Shell after keyword argument translation
   *  
   * 
   * @author 2015-05-14 Kris Becker
   * 
   * @param resource Resource containing keywords
   * @param globals  List of global keywords to use in argument substitutions
   * 
   * @return int Returns 1 if all is well, 0 otherwise
   */
  int RunCommandStrategy::apply(SharedResource &resource, 
                                const ResourceList &globals) { 

    BOOST_FOREACH ( Command command, m_commands ) {
      QString cmd = processArgs(command.m_command, m_argKeys, 
                                getGlobals(resource, globals));
      cmd = scanAndReplace(cmd, "&quot;", "\"");
      cmd = scanAndReplace(cmd, "&apos;", "\'");

      if ( isDebug() ) { 
        cout << "Running " << command.m_name.toStdString() << " command: " << cmd.toStdString() << "\n"; 
      }
      int status = system(cmd.toLatin1().data());

      // If command failed, deactivate the resource
      if (status != 0 ) {
        if ( isDebug() ) { 
          cout << "Command " << command.m_name.toStdString() 
               << " failed with status = " << status << "\n"; 
        }
        resource->discard();
        return (0);
      }
      else {
      if ( isDebug() ) { 
        cout << "Command " << command.m_name.toStdString() << " succeeded\n"; 
      }
      }
    }
    return (1); 
  }
  
}  //namespace Isis


/**
 * This is the function that is called in order to instantiate a RunCommand
 * plugin that can be derived directy from the version being used in the system
 * now.  See the $(INPUT)/runcommand_test.conf file for how it is used.
 *
 * For any StrateyPlugin the user must add a special group in the Strategy
 * object definition to provide the information regarding finding and loading
 * of the ISIS standard rumtime plugin. See the
 * StrategyFactory::loadStrategyPlugin() for details.
 *
 * @code
 *     Group = Plugin
 *       StrategyPluginPath = ("../../plugin/src/RunCommandStrategy",
 *                             "plugin/src/RunCommandStrategy")
 *       Library = RunCommandStrategy
 *       Routine = RunCommandStrategyPlugin
 *     EndGroup
 * @endcode
 *
 * @param definition  Strategy object definition as read from the CONFIG
 *                    parameter.
 *
 * @return globals    Chain/pool of global keyword parameters 
 */
extern "C" Isis::Strategy *RunCommandStrategyPlugin(const Isis::PvlObject &definition, 
                                                    const Isis::ResourceList &globals) { 
  return new Isis::RunCommandStrategy(definition, globals);
}

