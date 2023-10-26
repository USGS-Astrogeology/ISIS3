#ifndef Preference_h
#define Preference_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include "Pvl.h"

namespace Isis {
  class Application;

  /**
   * @brief Reads user preferences from a data file.
   *
   * This class is used to load, set, and obtain user preferences for
   * the operation of Isis. User preferences for Isis are items such
   * as the name of the log file, default data directories, and processing
   * status. This is essentially a specialized version of a Label object. It
   * reads a system-wide preference file and overwrites values in
   * the label with a user preference file. This object does not need
   * to be modified to add new prefences ... simply edit the system
   * wide preference file, add the new group and keywords, update the
   * preference document (user documentation) explaining the new
   * keywords.
   *
   * @ingroup ApplicationInterface
   *
   * @author 2002-03-13 Jeff Anderson
   *
   * @internal
   *   @todo The Preference class needs private methods & variables documented.
   *   The documentation shows methods that don't exist in the code. It also
   *   needs an example.
   *   @history 2003-04-23 Jeff Anderson - reworked the class to use the Label
   *                           object.
   *   @history 2003-05-16 Stuart Sides - Modified schema from astrogeoloy...
   *                           isis.astrogeology
   *   @history 2003-12-03 Stuart Sides - added HasGroup member.
   *   @history 2005-10-03 Elizabeth Miller - changed @ingroup tag
   *   @history 2008-01-09 Steven Lambright - Made Application a friend
   *                           for destruction purposes
   *   @history 2008-07-08 Steven Lambright - Now uses atexit for destruction
   *   @history 2010-05-28 Steven Lambright - More resilient to $HOME/.Isis
   *                           problems
   *   @history 2010-07-19 Jeannie Walldren - Added FileCustomization group to
   *                           TestPreferences file
   *   @history 2012-02-24 Steven Lambright - This class now sets the thread
   *                           limit in the Qt global thread pool.
   *   @history 2013-03-27 Jeannie Backer - Added Near mission to DataDirectory
   *                           group to TestPreferences file. References #1248.
   *   @history 2015-05-27 Andrew Stebenne - Backwards Compatibility Issue:
   *                           Preference now throws an exception when Preference.Load()
   *                           is called on a nonexistent preference file. This may cause issues
   *                           for anyone who has been using a nonexistent preference file.
   */
  class Preference : public Pvl {

    public:
      void Load(const std::string &file);

      /**
       * Tests whether or not a file is a unitTest
       *
       * @return bool True if it is a unitTest, and false if it is not
       */
      inline bool IsUnitTest() {
        return p_unitTest;
      }

      bool outputErrorAsPvl();
      bool reportFileLine();

      static Preference &Preferences(bool unitTest = false);

    private:
      Preference();

      //! Destroys the Preference object
      ~Preference() {};

      Preference(const Preference &p);
      Preference &operator=(const Preference &p);

      static Preference *p_preference;   //!< Pointer to a Preference object
      static bool p_unitTest;            /**< Flag indicating whether the file
                                              is a unitTest or not.*/
      static void Shutdown();
  };
};

#endif
