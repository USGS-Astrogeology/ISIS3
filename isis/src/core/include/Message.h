#ifndef Message_h
#define Message_h

/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/
#include <vector>

namespace Isis {
  /* Documentation for this namespace is in:
  *    isis/src/docsys/Object/build/isisDoxyDefs.doxydef
  * @internal
  *   @history 2016-08-28 Kelvin Rodriguez - Moved 'using namespace' to be properly defined
  *                           after the includes for every cpp file in the message folder.
  *                           Part of porting to OS X 10.11.
  */
  namespace Message {
    /**
     * This error should be used when an Isis object or application is checking
     * array bounds and the legal range has been violated.
     *
     * @param index - Array index which is out of bounds
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string ArraySubscriptNotInRange(int index);

    /**
     * This error should be used when a supplied keyword for an Isis application
     * parameter does not contain enough characters to distinguish it from
     * another keyword.
     *
     * @param key - The name of the keyword which is ambiguous
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordAmbiguous(const std::string key);

    /**
     * This error should be used when a supplied keyword for an Isis application
     * parameter does not match any of the parameters for that application.
     *
     * @param key - The name of the keyword which is unrecognized
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordUnrecognized(const std::string key);

    /**
     * This error should be used when a supplied keyword for an Isis application
     * parameter is the same as one previously supplied.
     *
     * @param key - The name of the keyword which has been duplicated
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordDuplicated(const std::string key);

    /**
     * This error should be used when a supplied keyword is being used as an
     * array (i.e., with a subscript) but is only a scaler entity.
     *
     * @param key - The name of the keyword which is not an array
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordNotArray(const std::string key);

    /**
     * This error should be used when a supplied keyword is not defined in the
     * application.
     *
     * @param key - The name of the keyword which was not found
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordNotFound(const std::string key);

    /**
     * This error has been deprecated and should not be used in new code.
     *
     * @param block - The name of the invalid keyword block
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordBlockInvalid(const std::string block);

    /**
     * This error has been deprecated and should not be used in new code.
     *
     * @param block - The name of the block with the missing start tag
     * @param found - The std::string found in the place of the missing block start
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordBlockStartMissing(const std::string block, const std::string found);

    /**
     * This error has been deprecated and should not be used in new code.
     *
     * @param block - The name of the block with the missing end tag
     * @param found - The std::string found in the place of the missing block end
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordBlockEndMissing(const std::string block, const std::string found);

    /**
     * This error should be used when a supplied keyword does not appear in
     * the list (e.g., an Isis cube label).
     *
     * @param key - The name of the keyword which is bad
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordValueBad(const std::string key);

    /**
     * This error should be used when the value of a supplied keyword is
     * incorrect.
     *
     * @param key - The keyword name which is bad
     * @param value - The value of the keyword which is bad
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordValueBad(const std::string key, const std::string value);

    /**
     * This error should be used when the value of a supplied keyword was
     * expected but not present.
     *
     * @param key - The keyword name which is bad
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordValueExpected(const std::string key);

    /**
     * This error should be used when the value of a supplied keyword is
     * not within a specific range.
     *
     * @param key - The keyword name which has a bad value
     * @param value - The value of the keyword
     * @param range - The minimum and maximum allowed values for the keyword
     *                formatted and inserted into a std::string
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordValueNotInRange(const std::string key, const std::string value,
                                  const std::string range);

    /**
     * This error should be used when the value of a supplied keyword is
     * not one of a specific list of possible values.
     *
     * @param key - The keyword name which has a bad value
     * @param value - The value of the keyword
     * @param list - A vector of all the legal values for the keyword
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string KeywordValueNotInList(const std::string key, const std::string value,
                                  const std::vector<std::string> &list);

    /**
     * This error should be used when a delimiter is missing.
     *
     * @param delimiter - The delimiter that is missing
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string MissingDelimiter(const char delimiter);

    /**
     * This error should be used when a delimiter is missing.
     *
     * @param delimiter - The delimiter that is missing
     * @param near - The value the missing delimiter is near
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string MissingDelimiter(const char delimiter, const std::string near);

    /**
     * This error should be used when a file could not be opened.
     *
     * @param filename - The name of the file
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string FileOpen(const std::string &filename);

    /**
     * This error should be used when a file could not be created.
     *
     * @param filename - The name of the file
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string FileCreate(const std::string &filename);

    /**
     * This error should be used when an error accrues during a read of a file.
     *
     * @param filename - The name of the file
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string FileRead(const std::string &filename);

    /**
     * This error should be used when an error accrues during a write to a file.
     *
     * @param filename - The name of the file
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string FileWrite(const std::string &filename);

    /**
     * This error should be used when an error accrues during a memory
     * allocation such as "new".
     *
     * @return std::string - A standardized Isis error message with the parameter
     *                  inserted
     */
    std::string MemoryAllocationFailed();
  }
}

#endif
