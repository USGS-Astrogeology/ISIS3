#ifndef PvlGroup_h
#define PvlGroup_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlContainer.h"

namespace Isis {
  /**
   * @brief Contains multiple PvlContainers
   *
   * Contains multiple PvlContainers so that keyword-value pairs can be organized
   * in specific groups. For example, a PvlGroup would be used to group
   * all data from a particular mission together. A PvlGroup object will also
   * organize (indent) objects on output.
   *
   * @ingroup Parsing
   *
   * @author 2002-10-11 Jeff Anderson
   *
   * @internal
   *  @history 2005-04-04 Leah Dahmer wrote class documentation.
   *  @history 2006-04-21 Jacob Danton Added format templates abilities.
   *  @history 2006-09-11 Stuart Sides Added formatting ability
   *  @history 2008-07-02 Steven Lambright Added const functionality and fixed +=
   *  @history 2008-07-02 Steven Lambright Updated to compensate for PvlKeyword no
   *           longer being a parent
   *  @history 2008-09-30 Christopher Austin Replaced all std::endl in the <<
   *           operator with PvlFormat.FormatEOL()
   *  @history 2010-04-13 Eric Hyer - Added copy constructor
   *                                  Added assignment operator
   *  @history 2010-09-27 Sharmila Prasad - Added API to Validate a PVLGroup
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   *
   *  @todo 2005-04-04 Needs coded example.
   */
  class PvlGroup : public Isis::PvlContainer {
    public:
      PvlGroup();
      PvlGroup(const QString &name);
      PvlGroup(const PvlGroup &other);

      //! Validate a Group comparing with the Template Group
      void validateGroup(PvlGroup & pPvlGrp);

      friend std::istream &operator>>(std::istream &is, PvlGroup &result);
      friend std::ostream &operator<<(std::ostream &os, PvlGroup &group);
      /**
       * Whenever the '==' operator is used on a PvlGroup object, it will call
       * the stringEqual() method. This returns a boolean value.
       * @param group The PvlGroup object to compare.
       * @return True if the other PvlGroup has the same name as this one, false
       * if not.
       */
      bool operator==(const PvlGroup &group) const {
        return PvlKeyword::stringEqual(group.name(), this->name());
      };

      const PvlGroup &operator=(const PvlGroup &other);
  };
}
#endif
