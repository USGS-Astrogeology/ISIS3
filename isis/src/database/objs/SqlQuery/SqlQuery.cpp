/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include <string>
#include <vector>
#include <iostream>
#include "DbProfile.h"
#include "Database.h"
#include "IString.h"
#include "SqlQuery.h"
#include "SqlRecord.h"
#include <QString>

using namespace std;

namespace Isis {

  /**
   * @brief Default constructor
   *
   * This constructor prepares a query using the default database as established
   * through the Database class.  It will also ensure that on any error, an
   * exception is thrown, in line with the Isis way of doing things.
   */
  SqlQuery::SqlQuery() : QSqlQuery(Database()), _throwIfFailed(true) { }

  /**
   * @brief Construtor using a specified database
   *
   * This constructor should be used for preparing for queries using a specified
   * database.  This would be used when using a database other than the default.
   *
   * Turns exception throwing on.
   *
   * @param db Database to use for subsequent database queries
   */
  SqlQuery::SqlQuery(Database &db) : QSqlQuery(db), _throwIfFailed(true) { }


  /**
   * @brief Construct a query object and execute the provided query string
   *
   * This constructor will take a query string and an optional database
   * specification and execute the query all after the initial construction.  If
   * the caller does not provide a Database, the default one is used and a
   * connection is automatically attempted.
   *
   * It can be used to execute an initial query and is perhaps the most powerful
   * example of the Isis database design.  It could be used as the starting point
   * for any database access and an initial query in one line of code.  For
   * example, below is a line of code that executes a line of code showing all the
   * tables in a PostgreSQL database using the default database:
   * @code
   *   SqlQuery pgtables("select tblname from pg_tables");
   * @endcode
   *
   * This will use the Database() default constructor since one is not explicitly
   * provide.  That constructor attempts a connection using the default database
   * access profile as read from your IsisPreferences file.  Its default behaviour
   * is to initiate the connection to the database.  If that suceeds, the query is
   * issued.  The resulting \a pgtables SqlQuery object can now be contiually used
   * for other queries until it goes out of scope.
   *
   *
   * @param query IString containing a valid SQL query to execute
   * @param db    An optional database object to issue the query to.  This
   *              database now becomes the one used in all subsequent queries
   *              using this SqlQuery object.
   */
  SqlQuery::SqlQuery(const std::string &query, Database db) : QSqlQuery(db),
    _throwIfFailed(true) {
//  Execute with error detector
    exec(query);
  }

  /**
   * @brief Constructor using an existing SqlQuery object
   *
   * This constructor creates a new SqlQuery object from an existing one,
   * inheriting all its characteristics - including a Database if one is
   * associated to the object.
   *
   * @param other A SqlQuery object used to create this one from
   */
  SqlQuery::SqlQuery(const SqlQuery &other) : QSqlQuery(other),
    _throwIfFailed(other._throwIfFailed) { }


  /**
   * @brief Execute an SQL query provided in the query string
   *
   * This method executes the given query in the string.  This method assumes this
   * query object has a valid and open Database connection associated with it.  It
   * will also check the result for valid completion and toss an iException if the
   * caller has established this course of action when it fails.
   *
   * Results are ready for processing on completion.
   *
   * @see SqlRecord.
   *
   * @param query An SQL query string to issue to a database
   *
   * @return bool  True if successful, false if it fails.  This will only return
   *         valse if setNoThrowOnFailure() has been issued, otherwise an
   *         iException is thrown if it fails.
   */
  bool SqlQuery::exec(const std::string &query) {
    bool ok = this->QSqlQuery::exec(QString::fromStdString((query)));
    if((!ok) && isThrowing()) {
      string mess = "Query \'" + query + "\' failed to execute";
      tossQueryError(mess, _FILEINFO_);
    }
    return (ok);
  }

  /**
   * @brief Returns the executed query string
   *
   * This method returns the last executed query string as it was issued to the
   * database.  Note that some database systems do not support this option
   * directly.  This routine will attempt to return the last executed query first
   * from the Qt QSqlQuery class.  If this is empty/undefined, then the last
   * current query will be returned.
   *
   * @return std::string The last executed query string.  If undetermined or
   *         undefined, an empty string is returned.
   */
  std::string SqlQuery::getQuery() const {
    std::string lastq = lastQuery().toStdString();
    if(lastq.empty()) lastq = executedQuery().toStdString();
    return (lastq);
  }

  /**
   * @brief Returns the number of fields (columns) from query
   *
   * The method returns the number of fields or columns returned by the last
   * issued query string.  Note that if the query has not been issued, it will
   * return 0 or an undefined value (-1?).
   *
   * \b NOTE this is not valid until \a after the first next() is issued.
   *
   * @return int Number of fields/columns in last query issued
   */
  int SqlQuery::nFields() const {
    return (record().count());
  }

  /**
   * @brief Returns the column name of the resulting query at the given index
   *
   * This method returns the name of the column heading as a result of the query
   * at the given index.
   *
   * \b NOTE this is not valid until \a after the first next() is issued.
   *
   * @param index Zero-based starting index of column name ot retreive
   *
   * @return std::string Name of column at given index
   */
  std::string SqlQuery::fieldName(int index) const {
    return (record().fieldName(index).toStdString());
  }

  /**
   * @brief Returns index of column for given name
   *
   * This method returns the index of the given column name.
   *
   * \b NOTE this is not valid until \a after the first next() is issued.
   *
   * @param name Name of column to get index for
   *
   * @return int Zero-based index of named column
   */
  int SqlQuery::fieldIndex(const std::string &name) const {
    return(record().indexOf(QString::fromStdString(name)));
  }

  /**
   * @brief Returns the names of all fields in the resulting query
   *
   * After a query has been issued, this method will return the names of all
   * fields/columns in the resulting query.
   *
   * \b NOTE this is not valid until \a after the first next() is issued.
   *
   * @see SqlRecord::getFieldName().
   *
   * @return std::vector<std::string>  Vector of strings of all fields/columns in
   *         a query.
   */
  std::vector<std::string> SqlQuery::fieldNameList() const {
    std::vector<std::string> fields;
    QSqlRecord rec = record();
    for(int i = 0 ; i < rec.count() ; i++) {
      fields.push_back(rec.fieldName(i).toStdString());
    }
    return (fields);
  }

  /**
   * @brief Returns the types of each field/column in a resutling query
   *
   * After a query has been issued, this method will return the types of all
   * fields/columns.  These types are defined by the SqlRecord::getType() method.
   *
   * @return std::vector<std::string> IString vector of all field types.
   */
  std::vector<std::string> SqlQuery::fieldTypeList() const {
    std::vector<std::string> types;
    SqlRecord rec = getRecord();
    for(int i = 0 ; i < rec.count() ; i++) {
      types.push_back(rec.getType(i).toLatin1().data());
    }
    return (types);
  }

  /**
   * @brief Returns the count of rows resulting from the query
   *
   * This returns the number of rows returned/accessable as a result of the issued
   * query.  Its value is governed by the Qt
   * <a href="http://doc.trolltech.com/4.1/qsqlquery.html#size">
   * QsqlQuery::size()</a> method.  Namely, a -1 can be returned under many
   * different conditions of the queray and the database (driver) support.  Check
   * the documentation for details.
   *
   * @return int Number of rows returned by the query.  -1 is typically returned
   *         under conditions where the value is not available or undefined.
   */
  int SqlQuery::nRows() const {
    return (size());
  }

  /**
   * @brief Returns a SqlRecord for the current query row
   *
   * While traversing through the resulting query row set, this method returns a
   * lower level interface to individual rows.  The returned object is provided by
   * the SqlRecord class.
   *
   * \b NOTE this is not valid until \a after the first next() is issued.
   * @see SqlRecord.
   *
   * @return SqlRecord A single resulting row of a query after an next() is
   *         issued.
   */
  SqlRecord SqlQuery::getRecord() const {
    return (SqlRecord(*this));
  }

  /**
   * @brief Issues an IException from various sources of error states in this
   *        class.
   *
   * This method is provided to issue a consistant error message format from this
   * class.  The user of this class can decide at runtime to issue
   * Isis::IException exceptions when error condition or query failures are
   * detected or handle the errors themselves.  All exceptions go through this
   * method for deployment to simplify the process.
   *
   * Note callers of this method within this class provide the context of the
   * error, such as name and line of code, to preserve accuracy of the error
   * context.
   *
   * @see setThrowOnFailure(), setNoThrowOnFailure().
   *
   * @param message  Context of the error to report
   * @param f Routine name issuing the error
   * @param l Line number of the offending error
   */
  void SqlQuery::tossQueryError(const std::string &message, const char *f, int l) const {
    string errmess = message + " - QueryError = " +
                     lastError().text().toStdString();
    throw IException(IException::User, errmess, f, l);
  }

}
