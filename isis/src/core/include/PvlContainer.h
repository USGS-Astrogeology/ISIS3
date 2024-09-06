#ifndef PvlContainer_h
#define PvlContainer_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "PvlKeyword.h"

template<typename T> class QList;

namespace Isis {
  /**
   * @brief Contains more than one keyword-value pair.
   *
   * This is the container for PvlKeywords. It holds information about more than
   * one set of PvlKeywords.
   *
   * @ingroup Parsing
   *
   * @author 2002-10-11 Jeff Anderson
   *
   * @internal
   *  @history 2005-04-04 Leah Dahmer wrote class documentation.
   *  @history 2006-04-21 Jacob Danton Added format templates abilities.
   *  @history 2006-05-17 Jacob Danton Added DeleteKeyword by index method
   *  @history 2006-09-11 Stuart Sides Added formatting ability
   *  @history 2008-07-02 Steven Lambright Added const functionality
   *  @history 2008-07-10 Steven Lambright PvlContainer is no longer a PvlKeyword,
   *           but rather has a set of pvl keywords
   *  @history 2008-09-30 Christopher Austin Replaced all std::endl in the <<
   *           operator with PvlFormat.FormatEOL()
   *  @history 2008-10-30 Steven Lambright Moved Find methods' implementations to
   *           the cpp file from the header file, added <algorithm> include,
   *           problem pointed out by "novas0x2a" (Support Forum Member)
   *  @history 2009-06-01 Kris Becker - Added a new AddKeyword method that
   *           provides insert capabilities at iterator positions.
   *  @history 2010-01-06 Christopher Austin - Added CleanDuplicateKeywords()
   *  @history 2010-04-13 Eric Hyer - Added Copy constructor
   *                                - Added Assignment operator
   *  @history 2010-09-27 Sharmila Prasad - Validate all the Keywords in a Container and
   *                                        verify the 'Repeat' Option also
   *  @history 2010-10-18 Sharmila Prasad - Added more options for the keyword validation
   *  @history 2013-03-11 Steven Lambright and Mathew Eis - Brought method names and member variable
   *                          names up to the current Isis 3 coding standards. Fixes #1533.
   *  @history 2015-05-15 J Bonn - fixed usage of iterator that had been deleted.
   */
  class PvlContainer {
    public:
      PvlContainer() = default;
      PvlContainer(const std::string &type);
      PvlContainer(const std::string &type, const std::string &name);
      PvlContainer(const PvlContainer &other);

      //! Set the name of the container.
      void setName(const std::string &name) {
        m_name.setValue(name);
      };
      /**
       * Returns the container name.
       * @return The container name.
       */
      inline std::string name() const {
        return (std::string) m_name;
      };
      /**
       * Returns whether the given string is equal to the container name or not.
       * @param match The string to compare to the name.
       * @return True if the name and string are the same, false if they are
       * not.
       */
      bool isNamed(const std::string &match) const {
        return PvlKeyword::stringEqual(match, (std::string)m_name);
      }
      /**
       * Returns the container type.
       * @return The container type.
       */
      inline std::string type() const {
        return m_name.name();
      };
      /**
       * Returns the number of keywords contained in the PvlContainer.
       * @return The number of keywords.
       */
      inline int keywords() const {
        return m_keywords.size();
      };

      //! Clears PvlKeywords
      void clear() {
        m_keywords.clear();
      };
      //! Contains both modes: Append or Replace.
      enum InsertMode { Append, Replace };
      /**
       * Add a keyword to the container.
       * @param keyword The PvlKeyword object to append.
       * @param mode Using the InsertMode value of Append.
       */
      void addKeyword(const PvlKeyword &keyword,
                      const InsertMode mode = Append);


      /**
       * When you use the += operator with a PvlKeyword, it will call the
       * addKeyword() method.
       * @param keyword The PvlKeyword to be added.
       */
      void operator+= (const PvlKeyword &keyword) {
        addKeyword(keyword);
      };

      PvlKeyword &findKeyword(const std::string &name);
      /**
       * When you use the [] operator with a (string) name, it will call the
       * findKeyword() method.
       * @param name The name of the keyword to find.
       */
      PvlKeyword &operator[](const std::string &name) {
        return findKeyword(name);
      };
      PvlKeyword &operator[](const int index);

      /**
       * When you use the [] operator with a (char) name, it will call the
       * findKeyword() method.
       * @param name The name of the keyword to find.
       */
      PvlKeyword &operator[](const char *name) {
        return operator[](std::string(name));
      };

      const PvlKeyword &findKeyword(const std::string &name) const;
      /**
       * When you use the [] operator with a (string) name, it will call the
       * findKeyword() method.
       * @param name The name of the keyword to find.
       */

      const PvlKeyword &operator[](const std::string &name) const {
        return findKeyword(name);
      };
      const PvlKeyword &operator[](const int index) const;

      /**
       * When you use the [] operator with a (char) name, it will call the
       * findKeyword() method.
       * @param name The name of the keyword to find.
       */
      PvlKeyword operator[](const char *name) const {
        return operator[](std::string(name));
      };

      bool hasKeyword(const std::string &name) const;
      //! The keyword iterator.
      typedef std::vector<PvlKeyword>::iterator PvlKeywordIterator;

      //! The const keyword iterator
      typedef std::vector<PvlKeyword>::const_iterator ConstPvlKeywordIterator;


      PvlKeywordIterator findKeyword(const std::string &name,
                                     PvlKeywordIterator beg,
                                     PvlKeywordIterator end);

      ConstPvlKeywordIterator findKeyword(const std::string &name,
                                          ConstPvlKeywordIterator beg,
                                          ConstPvlKeywordIterator end) const;

      PvlKeywordIterator addKeyword(const PvlKeyword &keyword,
                                    PvlKeywordIterator pos);

      /**
       * Return the beginning iterator.
       * @return The beginning iterator.
       */
      PvlKeywordIterator begin() {
        return m_keywords.begin();
      };

      /**
       * Return the const beginning iterator.
       * @return The const beginning iterator.
       */
      ConstPvlKeywordIterator begin() const {
        return m_keywords.begin();
      };

      /**
       * Return the ending iterator.
       * @return The ending iterator.
       */
      PvlKeywordIterator end() {
        return m_keywords.end();
      };

      /**
       * Return the const ending iterator.
       * @return The const ending iterator.
       */
      ConstPvlKeywordIterator end() const {
        return m_keywords.end();
      };

      void deleteKeyword(const std::string &name);
      void deleteKeyword(const int index);

      bool cleanDuplicateKeywords();

      /**
       * When you use the -= operator with a (string) name, it will call the
       * deleteKeyword() method.
       * @param name The name of the keyword to remove.
       */
      void operator-= (const std::string &name) {
        deleteKeyword(name);
      };
      /**
       * When you use the -= operator with a PvlKeyword object, it will call the
       * deleteKeyword() method.
       * @param key The PvlKeyword object to remove.
       */
      void operator-= (const PvlKeyword &key) {
        deleteKeyword(key.name());
      };
      /**
       * Returns the filename used to initialise the Pvl object. If the object
       * was not initialized using a file, this string is empty.
       * @return The filename.
       */
      std::string fileName() const {
        return m_filename;
      };

      void setFormatTemplate(PvlContainer &ref) {
        m_formatTemplate = &ref;
      };

      bool hasFormatTemplate() {
        return m_formatTemplate != NULL;
      };

      PvlContainer *formatTemplate() {
        return m_formatTemplate;
      };

      PvlFormat *format() {
        return m_name.format();
      }
      void setFormat(PvlFormat *format) {
        m_name.setFormat(format);
      }

      int indent() {
        return m_name.indent();
      }
      void setIndent(int indent) {
        m_name.setIndent(indent);
      }

      inline int comments() const {
        return m_name.comments();
      };
      std::string comment(const int index) const {
        return m_name.comment(index);
      }

      void addComment(const std::string &comment) {
        m_name.addComment(comment);
      }

      PvlKeyword &nameKeyword() {
        return m_name;
      }
      const PvlKeyword &nameKeyword() const {
        return m_name;
      }

      const PvlContainer &operator=(const PvlContainer &other);

    protected:
      std::string m_filename;                   /**<This contains the filename
                                                    used to initialize
                                                    the pvl object. If the
                                                    object was not
                                                    initialized using a filename
                                                    the string is empty.*/
      PvlKeyword m_name;                   //!< This is the name keyword
      std::vector<PvlKeyword> m_keywords; /**<This is the vector of
                                                    PvlKeywords the container is
                                                    holding. */

      void init();

      /**
       * Sets the filename to the specified string.
       *
       * @param filename The new filename to use.
       */
      void setFileName(const std::string &filename) {
        m_filename = filename;
      }

      PvlContainer *m_formatTemplate;

      //! Validate All the Keywords in a Container comparing with the Template
      void validateAllKeywords(PvlContainer &pPvlCont);

      //! Validate the Repeat Option for a Keyword
      void validateRepeatOption(PvlKeyword & pPvlTmplKwrd, PvlContainer & pPvlCont);
  };

  std::ostream &operator<<(std::ostream &os, PvlContainer &container);
};

#endif
