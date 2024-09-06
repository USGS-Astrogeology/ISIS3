#ifndef CollectorMap_h
#define CollectorMap_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <cctype>
#include <QString>
#include <map>
#include <algorithm>
#include <functional>
#include <sstream>
#include <iomanip>
#include <exception>
#include "IException.h"
#include "IString.h"
#include <gsl/gsl_math.h>


namespace Isis {

  /**
   * @brief Provides a simple comparison between two values
   *
   * This simple comparison function object is provided with no special frills
   * that does pretty much exactly what std::less does.
   */
  template <typename K> struct SimpleCompare {

    /**
     * Returns true if v1 is less than v2
     *
     * @param v1 Input constant
     * @param v2 Input constant
     *
     * @return bool Returns true if v1 is less than v2
     */
    bool operator()(const K &v1, const K &v2) const {
      return (v1 < v2);
    }

  };

  /**
  * @brief Provides a case insensitive string comparison
  *
  * This string comparison functor object compares two strings ignoring case. Use
  * this policy when your key into the collector map is a string and you want to
  * ignore case when finding elements in the collection.
  */
  template <typename K> struct NoCaseStringCompare {

    /**
     * Compares v1 and v2 as case insensitive strings, and returns true of v1 is
     * less than v2 (as those strings).
     *
     * @param v1 Input constant
     * @param v2 Input constant
     *
     * @return bool Returns true if v1 is less than v2 in string format
     */
    bool operator()(const K &v1, const K &v2) const {
      return (IString::DownCase(v1) < IString::DownCase(v2));
    }

  };

  /**
   * @brief Provides a robust comparison of double/float values
   *
   * This functor compares floating point values using a default epsilon of
   * 1.0e-6. It can be used for doubles or floats, however floats will be promoted
   * to double for the comparison.
   */
  template <typename K> struct RobustFloatCompare {

    /**
     * Compares v1 and v2 as floating point values.
     *
     * @param v1
     * @param v2
     *
     * @return bool
     */
    bool operator()(const K &v1, const K &v2) const {
      return (gsl_fcmp(v1, v2, -1.0E-6) < 0);
    }

  };

  /**
   * @brief Supplies a NOOP default for removal of a CollectorMap entry
   *
   * This simple declaration is basically a NOOP that implements removal
   * of a CollectionMap entry.  It is most useful (and the default behavior)
   * when the storage element of the CollectorMap is anything but a pointer.
   * Pointers that require deletion should use the PointerRemoval policy unless
   * the pointers are owned by another "entity".
   */
  template <typename T> struct NoopRemoval {

    // defines used for cross platform suppression of unused parameter warning. This will prevent
    // distroy() from causing unused parameter warnings in clang. This is done for the purpose
    // of porting ISIS to OSX 10.11
    // #define MON_Internal_UnusedStringify(macro_arg_string_literal) #macro_arg_string_literal
    // #define MONUnusedParameter(macro_arg_parameter) _Pragma(MON_Internal_UnusedStringify(unused(macro_arg_parameter)))

    protected:

      /**
       * Destroys the CollectorMap entry
       *
       * @param element The CollectorMap to be destroyed
       */
      void destroy(T *element) {
        // arbitrary cast type to suppress unused parameter warnings generated
        // by clang when building on mac
        (void)element;

        return;
      }

  };

  /**
   * @brief Supplies a policy for deleting pointers that CollectorMap owns
   *
   * Defines a method to delete pointers when removed from a CollectorMap.
   * This is necessary to prevent memory leaks and defer the deletion to
   * removal from CollectorMap class.
   */
  template <typename T> struct PointerRemoval {
    protected:

      /**
       * Destroys the CollectorMap pointer's CollectorMap
       *
       * @param element The pointer pointing to the CollectorMap to be destroyed
       */
      void destroy(T *element) {
        delete(*element);
        return;
      }

  };

  /**
   * @brief Policy for deleting arrays that CollectorMap owns
   *
   * Defines a method to delete arrays when removed from a CollectorMap.
   * This is necessary to prevent memory leaks and defer the deletion to
   * removal from CollectorMap class.
   */
  template <typename T> struct ArrayRemoval {
    protected:

      /**
       * Destroys the array of CollectorMaps
       *
       * @param element The array of CollectorMaps to be destroyed
       */
      void destroy(T *element) {
        delete [](*element);
        return;
      }

  };


  /**
   * @brief (Default) Policy for copying map elements
   *
   * Defines a method to copy simple elements from an existing map to a
   * destination map.  This policy just makes a direct copy of the element to the
   * destination.
   *
   * This policy assumes the assignment operator handles the proper copying of
   * each element T in the collection.
   */
  template <typename T> struct DefaultCopy {
    protected:

      /**
       * Returns a copy of the input
       *
       * @param src The map element to be copied
       *
       * @return const T& The copy of the input
       */
      const T &copy(const T &src) const {
        return (src);
      }

  };


  /**
   * @brief Pointer to object policy for copying map elements
   *
   * Defines a copy method to properly handle pointers to objects (assumed) when
   * copying the complete CollectorMap.  This implementation assumes the copy
   * constructor properly handles the creation of a new element from a different
   * one.
   *
   * This policy assumes the assignment operator handles the proper copying of
   * each element T* in the collection.
   *
   * This employs an intersting technique of redirection.  Because the type T is
   * actually T*, the templated allocate() method exists to get down to the T
   * class base level.  Looks strange but it works.
   */
  template <typename T> struct PointerCopy {
    protected:
      /**
       * @brief Allocate new object using copy construtor and new pointer
       *
       * This copy method takes a pointer to a pointer (T is actually a T*) and
       * allocates a new object using the copy constructor.
       *
       * @param src  Pointer to pointer of new class to allocate
       *
       * @return T  Pointer to new object type T
       */
      T copy(const T &src)  const {
        return (allocate(*(src)));
      }

    private:
      /**
       * @brief Allocate new object using copy constructor
       *
       * @param obj Source object to create new one from
       *
       * @return P* Pointer to newly allocated object
       */
      template <typename P>
      P *allocate(const P &obj) const {
        return (new P(obj));
      }
  };



  /**
   *  @brief Collector/container for arbitrary items
   *
   *  Used to contain types with iterators of const and non-const conditions.
   *  This is a multimap that contains arbitrary keys with arbitrary elements.  It
   *  is intended to be used for pointers and copyable objects.  They should be
   *  rather efficient in the copy out operation so large objects may not be
   *  suitable or classes that do not have a good copy operator.  During testing
   *  it was noted that an object is copied up to four times and destroyed three
   *  times upon an add() operation.
   *
   *  This class is implemented using policies.  The ComparePolicy is used to test
   *  key elements such as strings and double values.  The NoCaseStringCompare
   *  policy is provided that expedites case insensitive string key comparisons.
   *  The RobustFloatCompare implements the comparison of double or float key
   *  types.  Direct comparisons of floats can be problematic due to round off and
   *  storage manifestations of these values in conputers.   The default policy,
   *  SimpleCompare, does a simple parameter to key equality test.
   *
   *  The RemovalPolicy is provided when a map value is removed from the list.
   *  This allows pointers and arrays to be stored in the map as well.  To store
   *  pointers, use PointerRemoval and for arrays there is the ArrayRemoval
   *  policy.  The default is the NoopRemoval policy which simply lets the
   *  destructor handle removals.
   *
   *  The CopyPolicy is necessary to properly handle the copying of elements.
   *  This is especially important for pointers and arrays.  In order to minimize
   *  difficult passing strategies, map elements are passed by address and the
   *  return type is the element type.  DefaultCopy simply copies the elements as
   *  is relying on the element T assigment operator to do the right thing. For
   *  pointers to objects, the PointerCopy allocates the object using the copy
   *  constructor.  One could provide a similar operator assuming a clone()
   *  method existed for the type T element. The ArrayCopy policy is left to the
   *  user to provide their own as it cannot support arrays of varying length.
   *  (One should use std::vector instead!) Users can supply their own CopyPolicy
   *  that need only expose a copy(cont T *src) method.
   *
   *  Here are some examples that demonstrate how this policy-based template class
   *  can be used:
   *
   *  @code
   * // Create a unique string key list that stores double floating point values.
   * // Use the default removal and copy policies but allow testing for character
   * // keys without regard to case via the NoCaseStringCompare.
   *  #include "CollectorMap.h"
   *
   *  CollectorMap<QString, double, NoCaseStringCompare > dmap;
   *  cout << "\nSize of double map = " << dmap.size() << endl;
   *  dmap.add("one", 1.0);
   *  dmap.add("two", 2.0);
   *  cout << "Size of double map = " << dmap.size() << endl;
   *
   *  cout << "One = " << dmap.get("one") << endl;
   *  cout << "Two = " << dmap.get("Two") << endl;
   *
   *  const double &one = dmap.get("one");
   *  cout << "\nTest Const one = " << one << endl;
   *
   *  dmap.remove("one");
   *  @endcode
   *
   *  Using this class internal to classes is perhaps where it may be applied more
   *  frequently.  The example below shows how to declare an integer key using
   *  pointers to classes:
   *
   *  @code
   * #include "CollectorMap.h"
   *
   *  class ClassTest {
   *   public:
   *    ClassTest(int n = 0) : _n(n) {  }
   *    ~ClassTest() {  }
   *    int Ident() const { return (_n); }
   *   private:
   *    int _n;
   *  };
   *
   *
   * //  Typedefs are sometimes convenient in these cases
   *  typedef CollectorMap<int, ClassTest *, SimpleCompare,
   *                       PointerRemoval, PointerCopy> PointerMap;
   *
   *  PointerMap ctest2;
   *  ctest2.add(4,new ClassTest(4));
   *  ctest2.add(5,new ClassTest(5));
   *  ctest2.add(6,new ClassTest(6));
   *  ctest2.add(7,new ClassTest(7));
   *
   *  cout << "Remove ClassTest 6\n";
   *  ctest2.remove(6);
   *
   * //  Creates a copy of ctest2 using the PointerCopy policy
   *  PointerMap map2(ctest2);
   *
   *  cout << "Find element 7: "  << map2.find(7)->Ident() << endl;
   *
   *  @endcode
   *
   *  And, finally, an example of how to use duplicate keys:
   *
   *  @code
   *  #include "CollectorMap.h"
   *
   *  typedef CollectorMap<int,QString> IntStr;
   *  IntStr dupstr(IntStr::DuplicateKeys);
   *  dupstr.add(1,"One");
   *  dupstr.add(1, "One #2");
   *  dupstr.add(1,"One #3");
   *  dupstr.add(2,"Two");
   *  dupstr.add(2,"Two #2");
   *  dupstr.add(3,"Three");
   *
   *  cout << "Size of Dup object: " << dupstr.size() << endl;
   *  cout << "Number Ones:   " << dupstr.count(1) << endl;
   *  cout << "Number Twos:   " << dupstr.count(2) << endl;
   *  cout << "Number Threes: " << dupstr.count(3) << endl;
   *  cout << "Number Fours:  " << dupstr.count(4) << endl;
   *
   *  IntStr::CollectorConstIter isIter;
   *  int j = 0;
   *  for (isIter = dupstr.begin() ; isIter != dupstr.end() ; ++isIter, j++) {
   *       cout << "IntStr[" << j << "] = {" << isIter->first << ", "
   *            << isIter->second << "}, Index: " << dupstr.index(isIter->first)
   *            << endl;
   *       cout << "Nth Test Ident       = " << dupstr.getNth(j) << endl;
   *  }
   *  @endcode
   *
   *  The output of the above example is:
   *  @code
   * Size of Dup object: 6
   * Number Ones:   3
   * Number Twos:   2
   * Number Threes: 1
   * Number Fours:  0
   * IntStr[0] = {1, One}, Index: 0
   * Nth Test Ident       = One
   * IntStr[1] = {1, One #2}, Index: 0
   * Nth Test Ident       = One #2
   * IntStr[2] = * {1, One #3}, Index: 0
   * Nth Test Ident       = One #3
   * IntStr[3] * = {2, Two}, Index: 3
   * Nth Test Ident       = Two
   * IntStr[4] = * {2, Two #2}, Index: 3
   * Nth Test Ident       = Two #2
   * IntStr[5] = * {3, Three}, Index: 5
   * Nth Test Ident = Three
   *  @endcode
   *
   *  @ingroup Utility
   *
   *  @author 2006-06-21 Kris Becker
   *
   *  @internal
   *    @history 2006-07-03 Kris Becker Added the ability to stored duplicate keys
   *             if needed (using a multimap instead of a map).  Initial default
   *             behavior of unique keys is retained.  See KeyPolicy.
   *    @history 2006-07-28 Kris Becker Fixed a bug in the NoCaseStringCompare
   *             implementation.  Prior to this fix, it would not function
   *             properly at all for case-insenstive keys.
   *    @history 2006-08-30 Kris Becker Fixed bug in copy constructors that
   *             attempted to use a virtual method in the object being created
   *             when it *must* use the method from the one it is being created
   *             from. (g++ 4.1 on Suse 10.1 didn't like this bug at all!)
   *    @history 2008-06-18 Christopher Austin Fixed Documentation
   *    @history 2017-08-30 Summer Stapleton - Updated documentation. References #4807.
   * 
   */
  template < typename K, typename T,
           template <class> class ComparePolicy = SimpleCompare,
           template <class> class RemovalPolicy = NoopRemoval,
           template <class> class CopyPolicy = DefaultCopy
           >
  class CollectorMap : public RemovalPolicy<T>, public CopyPolicy<T> {
    public:
      typedef T                                      CollectorType; //!< Data type
      /** A multimap attacking a key to a CollectorType and a ComparePolicy<CollectorType>*/
      typedef std::multimap<K, CollectorType, ComparePolicy<K> > CollectorList;
      //! CollectorList iterator type declaration
      typedef typename CollectorList::iterator       CollectorIter;
      //! CollectorList constant iterator type declaration
      typedef typename CollectorList::const_iterator CollectorConstIter;

      /**
       * @brief Enumerated selection of key behaviour
       *
       * Using this enumeration during construction allows the user of this class
       * to specify if the keys used to identify elements are unique or can be
       * duplicated.
       */
      enum KeyPolicy { UniqueKeys,     //!<  Constrain keys to be unique
                       DuplicateKeys   //!<  Allow duplication of keys
                     };

      /** Constructor */
      CollectorMap() : _keyPolicy(UniqueKeys) { }

      /**
       * @brief Allows the user to choose if keys can be duplicated
       *
       * This constructor is provided to the user that wants to explicity define how
       * the keys, namely insertions are managed.  The default is unique keys in the
       * noop constructor...this one allows instantiation of either policy.
       *
       * @param keyPolicy   Can be UniqueKeys or DuplicateKeys
       */
      CollectorMap(const KeyPolicy &keyPolicy) : _keyPolicy(keyPolicy) { }

      /**  Destructor handles removal of the elements within the collection
       *
       * This must take into account the removal strategy and apply to any
       * remaining elements.
       */
      virtual ~CollectorMap() {
        selfDestruct();
      }

      /**
       * @brief Copy constructor invokes the copy policy as provided by the users
       *
       * This copy constructor will transfer the map of an incoming CollectorMap to
       * a newly created one.  This process employs the user selectable CopyPolicy.
       * It invokes the copy() method exposed in the copy policy.
       *
       * @param cmap The CollectorMap to be copied
       */
      CollectorMap(const CollectorMap &cmap) {
        _keyPolicy = cmap._keyPolicy;
        CollectorConstIter cItr;
        for(cItr = cmap._list.begin() ; cItr != cmap._list.end() ; cItr++) {
          _list.insert(std::make_pair(cItr->first, cmap.copy(cItr->second)));
        }
      }

      /**
       * @brief Assignment operator for the CollectorMap class object
       *
       * This object assignment operator is provided to properly handle the copying
       * of CollectorMap elements to a new instantiation.  This implements the
       * CopyPolicy for each element in the @b cmap object to the current one.  This
       * is a two step operation:  first destroy any elements that exist in the
       * destination object (using the RemovalPolicy) and then copy all elements
       * from the @b cmap object to the current one using the copy() method exposed
       * in the CopyPolicy.
       *
       * @param cmap The CollectorMap to be copied
       * 
       * @returns Pointer to this CollectorMap
       */
      CollectorMap &operator=(const CollectorMap &cmap) {
        if(&cmap != this) {
          selfDestruct();
          _keyPolicy = cmap._keyPolicy;
          CollectorConstIter cItr;
          for(cItr = cmap._list.begin() ; cItr != cmap._list.end() ; cItr++) {
            _list.insert(std::make_pair(cItr->first, cmap.copy(cItr->second)));
          }
        }
        return (*this);
      }

      /**
       * Returns the size of the collection
       *
       * @return int Number of elements in collection
       */
      int size() const {
        return (_list.size());
      }

      /**
       * @brief Returns the number of keys found in the list
       *
       * For unique keys, this will always be 1.  If duplicate keys are allowed,
       * this will return the number of keys in the container.
       *
       * @param key  Key to return count for
       *
       * @return int Number keys in container
       */
      int count(const K &key) const {
        return (_list.count(key));
      }

      /**
       * Adds the element to the list.
       *
       * If the element exists and the key policy is restricted to uniqueness, it is
       * replaced after the removal strategy is applied.  If it doesn't exist, it is
       * inserted into the list.  For duplicate keys, it is simply inserted.
       *
       * @param key  Key in the associative map for the value
       * @param value Value to be associated with the key
       */
      void add(const K &key, const T &value) {
        if(_keyPolicy == UniqueKeys) remove(key);
        _list.insert(std::make_pair(key, value));
        return;
      }

      /**
       * Checks the existance of a particular key in the list
       * @param key Key to search for in the list
       * @return bool True if the key exists, false otherwise
       */
      bool exists(const K &key) const {
        CollectorConstIter cItr = _list.find(key);
        return (cItr != _list.end());
      }

      /**
       * @brief Returns the value associated with the name provided
       *
       * If the specifed name and value does not exist in the list, an out_of_range
       * exception is thrown.  Use @b exists to predetermine of the value is in the
       * list.
       *
       * @param key Key to fetch the value for
       * @return T Value associated with name
       * @throws IException if the value is not found
       */
      T &get(const K &key) {
        CollectorIter cItr = _list.find(key);
        if(cItr == _list.end()) {
          std::string mess = "Requested value does not exist!";
          throw IException(IException::Programmer, mess, _FILEINFO_);
        }
        return (cItr->second);
      }

      /**
       * @brief Const version returning the value associated with the given name
       *
       * @param key Key to fetch the value for
       * 
       * @returns The value associated with the given name
       */
      const T &get(const K &key) const {
        CollectorConstIter cItr = _list.find(key);
        if(cItr == _list.end()) {
          std::string mess = "Requested value does not exist!";
          throw IException(IException::Programmer, mess, _FILEINFO_);
        }
        return (cItr->second);
      }

      /**
       * @brief Returns the index of the first occuring element in the list
       *
       * This returns the index such that the getNth() methods would retrieve the
       * element with key.  For duplicate keys, it is garaunteed to return the first
       * element.  It will return -1 if the element is not in the list.
       *
       * @param key Key to fetch the value for
       *
       * @return int Zero-based index of (first) element with key.  If it doesn't
       *         exist, -1 is returned.
       */
      int index(const K &key) const {
        CollectorConstIter cItr = _list.lower_bound(key);
        if(cItr == _list.end()) {
          return (-1);
        }
        else {
          return (std::distance(_list.begin(), cItr));
        }
      }

      /**
       * @brief Returns the nth value in the collection
       *
       * If the specifed value does not exist in the list, an out_of_range exception
       * is thrown.  Use @b size() to predetermine if the range is valid.
       *
       * @param nth Return the Nth value in the list
       *
       * @return T Value associated with name
       */
      T &getNth(int nth) {
        CollectorIter cItr;
        int i;
        for(cItr = _list.begin(), i = 0 ; cItr != _list.end() ; ++cItr, i++) {
          if(i == nth) break;
        }

        if(cItr == _list.end()) {
          std::ostringstream mess;
          mess << "Requested index (" << nth << ") out of range" << std::endl;
          throw IException(IException::Programmer, mess.str(), _FILEINFO_);
        }
        return (cItr->second);
      }

      /**
        * @brief Returns the nth value in the collection
        *
        * If the specifed value does not exist in the list, an out_of_range exception
        * is thrown.  Use @b size() to predetermine if the range is valid.
        *
        * @param nth Return the Nth value in the list
        *
        * @return T Value associated with name
        */
      const T &getNth(int nth) const {
        CollectorConstIter cItr;
        int i;
        for(cItr = _list.begin(), i = 0 ; cItr != _list.end() ; ++cItr, i++) {
          if(i == nth) break;
        }
        if(cItr == _list.end()) {
          std::ostringstream mess;
          mess << "Requested index (" << nth << ") out of range" << std::endl;
          throw IException(IException::Programmer, mess.str(), _FILEINFO_);
        }
        return (cItr->second);
      }

      /**
        * @brief Returns the nth key in the collection
        *
        * If the specifed key does not exist in the list, an out_of_range exception
        * is thrown.  Use @b size() to predetermine if the range is valid.
        *
        * @param nth Return the Nth key in the list
        *
        * @return K Key associated with name
        */
      const K &key(int nth) const {
        CollectorConstIter cItr;
        int i;
        for(cItr = _list.begin(), i = 0 ; cItr != _list.end() ; ++cItr, i++) {
          if(i == nth) break;
        }
        if(cItr == _list.end()) {
          std::ostringstream mess;
          mess << "Requested key index (" << nth << ") out of range" << std::endl;
          throw IException(IException::Programmer, mess.str(), _FILEINFO_);
        }
        return (cItr->first);
      }

      /**
       * Removes and entry from the list
       *
       * @param key Name of key/value pair to remove from the list
       *
       * @return int Number of elements erased
       */
      int remove(const K &key) {
        CollectorIter Itr1 = _list.lower_bound(key);
        if(Itr1 == _list.end()) return (0);

        CollectorIter Itr2 = _list.upper_bound(key);
        while(Itr1 != Itr2) {
          this->destroy(&Itr1->second);
          ++Itr1;
        }
        return (_list.erase(key));
      }

      /**
       * Const iterator into list
       *
       * @return CollectorConstIter Returns a const iterator to the list
       */
      CollectorConstIter begin() const {
        return _list.begin();
      }

      /**
       * Const iterator to end of list
       *
       * @return CollectorConstIter  Returns the const end of the list
       */
      CollectorConstIter end() const {
        return _list.end();
      }

      /**
       * Returns the start of the list for iterating purposes
       *
       * @return CollectorIter Returns an iterator on the collection
       */
      CollectorIter begin() {
        return _list.begin();
      }

      /**
       * Returns the end of the list
       *
       * @return CollectorIter Returns the end of the list for determining the end
       *         of the iteration loop
       */
      CollectorIter end() {
        return _list.end();
      }

    private:
      KeyPolicy      _keyPolicy;  //!<  Unique or duplicate key constraint
      CollectorList  _list;       //!< The list

      /**
       * @brief Thourough destruction of list
       *
       * This method iterates through each element in the list applying the
       * RemovalPolicy to each value in the map.  It then clears the internal list
       * for subsequent reuse if needed.
       */
      void selfDestruct() {
        CollectorIter itr;
        for(itr = _list.begin() ; itr != _list.end() ; itr++) {
          this->destroy(&itr->second);
        }
        _list.clear();
      }


  };

};
#endif
