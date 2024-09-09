#ifndef ProgramLauncher_h
#define ProgramLauncher_h

/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

class QString;

namespace Isis {
  class IException;

  /**
   * @brief Execute External Programs and Commands
   *
   * This class is designed to handle running any other programs or commands.
   *
   * @author 2010-12-03 Steven Lambright
   *
   * @internal
   *   @history 2010-12-03 Steven Lambright - Original version
   *   @history 2011-08-19 Jeannie Backer - Modified unitTest to use
   *                           $temporary variable instead of
   *                           /tmp directory.
   *   @history 2011-08-19 Kelvin Rodriguez - Added truth data for OS X 10.11
   *                           Part of proting to 10.11.
   *   @history 2017-05-19 Christopher Combs - Modified unitTest.cpp: now creates unitTest.cub to
   *                          perform tests on. Allows test to pass when not using the default data
   *                          area. Fixes #4738.
   */
  class ProgramLauncher {
    public:
      static void RunIsisProgram(QString isisProgramName, QString arguments);
      static void RunSystemCommand(QString commandLine);

    private:
      static IException ProcessIsisMessageFromChild(QString code, QString msg);

    private:
      //! Construction is not allowed
      ProgramLauncher();
      //! Destruction is not allowed
      ~ProgramLauncher();

      /**
       * Copy construction is not allowed
       *
       * @param other
       */
      ProgramLauncher(ProgramLauncher &other);

      /**
       * Assignment is not allowed
       *
       * @param other
       * @returns
       */
      ProgramLauncher &operator=(ProgramLauncher &other);
  };
};

#endif
