/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "Kernels.h"

#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>

#include <QVector>

#include <SpiceUsr.h>

#include "FileName.h"
#include "IException.h"
#include "NaifStatus.h"
#include "PvlKeyword.h"
#include "Pvl.h"

using namespace std;

namespace Isis {

  /** Default constructor simple initialization */
  Kernels::Kernels() {
    _kernels.clear();
    _camVersion = -1;
  }


  /**
   * @brief Initialize new Kernels from state of existing one
   *
   *  Initialize new Kernels object from the state of an existing one while
   *  preserving the state of one copied.  Each kernel file state is copied to
   *  this new one.  It ensures that ones that are already opened are set to an
   *  unmanaged state from the source.  It also checks the current open state
   *  and will set ones found to be open as unmanaged as well. All other kernels
   *  files can be managed by this new instance.
   *
   * @author kbecker (3/26/2011)
   *
   * @param kernels Source kernels object
   */
  Kernels::Kernels(const Kernels &kernels) {
    _kernels = kernels._kernels;
    _camVersion = kernels._camVersion;
    UpdateLoadStatus();
    UpdateManagedStatus();
  }

  /**
   * @brief Copy constructor for existing Kernels objecr
   *
   * This copy constructor ensures the kernel list copied from an existing
   * object is set to proper conditions as to minimize interference of both
   * object that existence.
   *
   * Any current kernels that in this object are removed without regard for ther
   * current state.  The list of kernels in the source object are copied and
   * their state is analyzed.  Any opened kernels are set to unmanaged state.
   *
   * @author kbecker (3/26/2011)
   *
   * @param kernels Source list from which the list is copied
   *
   * @return Kernels&  Returned Kernel instance
   */
  Kernels &Kernels::operator=(const Kernels &kernels) {
    if (this != &kernels) {
      Clear();
      _kernels = kernels._kernels;
      _camVersion = kernels._camVersion;
      UpdateLoadStatus();
      UpdateManagedStatus();
    }
    return (*this);
  }

  /**
   * @brief Construct using an ISIS file name
   *
   * @param filename Name of ISIS cube file
   */
  Kernels::Kernels(const QString &filename) {
    Pvl pvl(filename.toStdString());
    Init(pvl);
  }

  /**
   * @brief Construct using an ISIS Cube object
   *
   * @param cube    Cube object of ISIS file
   */
  Kernels::Kernels(Cube &cube) {
    Init(*cube.label());
  }

  /**
   * @brief Construct from an ISIS label
   *
   * @param pvl  ISIS label to get kernel information from
   */
  Kernels::Kernels(Pvl &pvl) {
    Init(pvl);
  }

  /**
   * @brief Return count of missing kernel files
   *
   * @return int Number of files that do not exist
   */
  int Kernels::Missing() const {
    int nMissing(0);
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      if (!_kernels[i].exists) nMissing++;
    }
    return (nMissing);
  }

  /**
   * @brief Determine Spice kernels in an ISIS label
   *
   * This method reads keywords in the Kernels group in the ISIS label hierarchy
   * to find all the pertinent kernel files.  They are found in the Kernels
   * Group.
   *
   * The order listed here is the same as the order specified in the ISIS Spice
   * class at the time this class was written.  When loading and unloading, the
   * order is the same as is read in here.
   *
   * @param pvl Assumed to be a valid ISIS cube label containing the Kernels
   *            group
   */
  void Kernels::Init(Pvl &pvl) {
    UnLoad();
    _kernels.clear();
    addKernels(findKernels(pvl, "TargetPosition"));
    addKernels(findKernels(pvl, "InstrumentPosition"));
    addKernels(findKernels(pvl, "InstrumentPointing"));
    addKernels(findKernels(pvl, "Frame"));
    addKernels(findKernels(pvl, "TargetAttitudeShape"));
    addKernels(findKernels(pvl, "Instrument"));
    addKernels(findKernels(pvl, "InstrumentAddendum"));
    addKernels(findKernels(pvl, "LeapSecond"));
    addKernels(findKernels(pvl, "SpacecraftClock"));
    addKernels(findKernels(pvl, "ShapeModel"));
    addKernels(findKernels(pvl, "Extra"));
    _camVersion = getCameraVersion(pvl);
    return;
  }

  /**
   * @brief Add a new kernel to the list
   *
   * This method is provided to add individual NAIF kernels directly.  Files
   * names must contain a path to the file.  ISIS "shortcuts" as defined in the
   * DataDirectory group in the IsisPreferences files (e.g.,
   * "$base/kernels/lsk/naif0009.tls") are supported as well.
   *
   * Note that if the file already exists, it is not added.  This situation is
   * indicated with a return value of false.
   *
   * Files added in this way are set to manageable status.
   *
   * @param kfile Name of kernel file to add with valid directory path
   *
   * @return bool True if the files was added to the list, false if it already
   *              exists
   */
  bool Kernels::Add(const QString &kfile) {
    if (!findByName(kfile)) {
      _kernels.push_back(examine(kfile, true));
      return (true);
    }
    return (false);
  }

  /**
   * @brief Remove all kernel files from internal list
   *
   * This method will clear out the kernel file list without any regard for its
   * current state.  This does not unload any kernels that are unloaded.  One
   * must Unload() prior to calling this method in order to gracefully close
   * out the existing state of the kernel pool.
   *
   */
  void Kernels::Clear() {
    _kernels.clear();
  }

  /**
   * @brief Determine which NAIF kernels are currently loaded in the pool
   *
   * This method queries the NAIF KEEPER system/pool for all loaded kernels and
   * adds them to the list in this object.  It only knows about kernels that are
   * loaded by the NAIF furnsh_c routine.  If no kernels are loaded there will
   * be no kernels discovered by this routine.
   *
   * Upon entry into this method, the current list is discarded without
   * unloading them.  Hence, any kernels in this list are discard, whether they
   * are loaded and managed by this class or not.  If they are loaded according
   * to the internal state as determined by previous activity in this class, and
   * no additional NAIF activity has occured that unloads them, they should be
   * found again by this method.
   *
   * Note that ALL kernels discovered by this routine are marked as unmanaged,
   * meaning you cannot unload the kernels without exerting management upon
   * them via the Manage() method.  See the Manage() documentation for more
   * details and potential issues with this feature.
   *
   * This method should not be used to update the load nature of an established
   * kernel list.  See the UpdateLoadStatus() method for this feature.
   *
   * Here is a short coding example showing use of this method.  It assumes
   * there are some kernels already loaded (although it is entirely possible
   * that no kernels are loaded).  It will take over management of these kernels
   * as well:
   *
   * @code
   *   Kernels myKernels;
   *   myKernels.Discover();
   *   myKernels.Manage();
   * @endcode
   *
   * @return int Number of kernels discovered
   */
  int Kernels::Discover() {
    _kernels.clear();
    SpiceInt count;
    NaifStatus::CheckErrors();
    ktotal_c("ALL", &count);
    int nfound(0);
    for (int i = 0 ; i < count ; i++) {
      SpiceChar file[128];
      SpiceChar ktype[32];
      SpiceChar source[128];
      SpiceInt  handle;
      SpiceBoolean found;
      kdata_c(i, "ALL", sizeof(file), sizeof(ktype), sizeof(source),
              file, ktype,source, &handle, &found);
      if (found == SPICETRUE) {
        _kernels.push_back(examine(file, false));
        nfound++;
      }
    }
    NaifStatus::CheckErrors();
    return (nfound);
  }


  /**
   * @brief Set each kernels management status to managed
   *
   * This method will change every kernels's management state to managed so that
   * full manipulation of every kernel can be performed by this object.
   *
   * If a kernel is managed, it will be allowed to UnLoad at will.  This is
   * typically the only restriction of an unmanaged kernel.
   *
   */
  void Kernels::Manage() {
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      _kernels[i].managed = true;
    }
    return;
  }


  /**
   * @brief Set each kernels' management state to unmanaged
   *
   * This method changes the management state to unmanaged, thus restricting the
   * ability of this method to unload loaded kernels.
   *
   * When a kernel is marked as unmanaged, it will persist in its loaded state
   * even after the destruction of this object (there is really no way to track
   * how kernels get loaded reliably).
   *
   */
  void Kernels::UnManage() {
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      _kernels[i].managed = false;
    }
    return;
  }

  /**
   * @brief Determine if all kernels are managed by this object
   *
   * This method will return true if all kernels listed in this object are
   * marked as "managed".  If any one of the kernels is found to have an
   * unmanaged state, false will be returned by this routine.
   *
   * Management/unmanagement is an all-or-nothing scheme.
   *
   *
   * @return bool True if all kernels are marked manageable, false if even one
   *              is not.
   */
  bool Kernels::IsManaged() const {
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      if (!_kernels[i].managed) return (false);;
    }
    return (true);
  }


  /**
   * @brief Initialize the NAIF kernel keeper pool
   *
   * This method will thoroughly clear out the NAIF kernel keeper pool.  This is
   * done using the NAIF k_clear() utility.  As the documentation states, it
   * operates by side effects unloading all kernels and reinitializing the
   * keeper pool.
   *
   * This method has no regard at to whether any of the NAIF kernels are managed
   * or not.  It does update all kernels load status to unloaded.
   *
   * NOTE:  This method should be used with caution as it can have disasterous
   * side effects for ISIS processing.  Specifically, using this method
   * arbitrarily will likely cause fatal exceptions particularly for Spice class
   * instantiations.
   *
   */
  void Kernels::InitializeNaifKernelPool() {
    NaifStatus::CheckErrors();
    kclear_c();
    NaifStatus::CheckErrors();
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      _kernels[i].loaded = false;
    }
    return;
  }


  /**
   * @brief Load all kernels with one more types
   *
   * This method will find all kernels with a specific type and loads them into
   * the NAIF KEEPER system/pool using the NAIF furnsh_c utility.  If they are
   * already loaded, they are not loaded again.  Status is set to show they are
   * loaded.
   *
   * This object cannot monitor the kernel pool unless explicit action is taken.
   * See the CheckLoadStatus() method.  It will update the load status of
   * kernels in the current list.
   *
   * NOTE:  If individual kernel are not managed by this routine they are not
   * loaded.  Users must invoke the Manage() method to enable full kernel
   * manipulation.
   *
   * @param ktypes One or more kernel types specified in a comma separated list
   *
   * @return int Number of kernels loaded
   */
  int Kernels::Load(const QString &ktypes) {
    //  If no types specified, return them all
    int nLoaded(0);
    if (ktypes.isEmpty()) {
      for (unsigned int k = 0 ; k < _kernels.size() ; k++) {
        if (Load(_kernels[k])) { nLoaded++; }
      }
    }
    else {
      // Find types and return requested types
      QStringList tlist = getTypes(ktypes);
      for (int t = 0 ; t < tlist.size() ; t++) {
        for (unsigned int k = 0; k < _kernels.size() ; k++) {
          if (_kernels[k].ktype == tlist[t]) {
            if (Load(_kernels[k])) { nLoaded++; }
          }
        }
      }
    }
    return (nLoaded);
  }


  /**
   * @brief Load all kernels in list
   *
   * This method will iterate though all NAIF-type kernels and load them into
   * the NAIF KEEPER system/pool using the NAIF furnsh_c utility.  If they are
   * already loaded, they are not loaded again.  Status is set to show they are
   * loaded.
   *
   * This object cannot monitor the kernel pool unless explicit action is taken.
   * See the CheckLoadStatus() method.  It will update the load status of
   * kernels in the current list.
   *
   * NOTE:  If individual kernel are not managed by this routine they are not
   * loaded.  Users must invoke the Manage() method to enable full kernel
   * manipulation.
   *
   * @return int Number of kernels loaded
   */
  int Kernels::Load() {
    //  If not types specified, return them all
    int nLoaded(0);
    for (unsigned int k = 0 ; k < _kernels.size() ; k++) {
      if (Load(_kernels[k])) { nLoaded++; }
    }
    return (nLoaded);
  }


  /**
   * @brief Unloads all kernels from the NAIF pool
   *
   * This method will unload all kernels from the kernel pool.  It will unload
   * them one at a time, which is known to be inefficient.  You could consider
   * using the InitializeNaifKernelPool() method for a more thorough and
   * efficient cleansing.
   *
   * @return int Number of kernels unloaded
   */
  int Kernels::UnLoad() {
    int nUnloaded(0);
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      if (UnLoad(_kernels[i])) nUnloaded++;
    }
    return (nUnloaded);
  }

  /**
   * @brief Unloads all kernels of a specific type
   *
   * This method will parse a string containing acronyms of NAIF kernels types
   * and close only those found with the associated type.  Types in the context
   * of this class are loosely managed.  They are typically types returned by
   * the NAIF utilities, kinfo_c and kdata_c.
   *
   * Only those files that match will be unloaded.  There load states will be
   * altered to reflect its load status.
   *
   * Only files that are tagged as manageable but this object are closed.
   * Unmanaged kernels are essentially ignored.
   *
   *
   * @param ktypes Comma delimited string specifying the type to unload.  This
   *               string is of the form "LSK,FK,SPK".
   *
   * @return int Number of kernels unloaded
   */
  int Kernels::UnLoad(const QString &ktypes) {
    //  If not types specified, return them all
    int nUnloaded(0);
    if (ktypes.isEmpty()) {
      nUnloaded = UnLoad();
    }
    else {
      // Find types and return requested types
      QStringList tlist = getTypes(ktypes);
      for (int t = 0 ; t < tlist.size() ; t++) {
        for (unsigned int k = 0; k < _kernels.size() ; k++) {
          if (_kernels[k].ktype == tlist[t]) {
            if (UnLoad(_kernels[k])) nUnloaded++;
          }
        }
      }
    }
    return (nUnloaded);
  }

  /**
   * @brief Determine the load status of all kernels known to this object
   *
   * This method will reevaluate the load status of each NAIF kernel contained
   * within this object.  This method can be useful when there are more than one
   * of these Kernels objects instantiated.
   *
   * It will check to determine if each NAIF kernel is loaded using the NAIF
   * kinfo_c utility.  It will change loaded status to unloaded and unloaded
   * status to loaded from results of the query.  It does not consider the
   * managed state of any kernel.
   *
   * One can always choose to use the Load() and UnLoad() utilities for
   * explicit handling of the kernel (type) states.
   *
   * @return int Number of kernels whose states where changed/affected by this
   *             method
   */
  int Kernels::UpdateLoadStatus() {
    int nchanged(0);
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      // Use NAIF to determine if it is loaded
      if (IsNaifType(_kernels[i].ktype)) {
        SpiceChar ktype[32];
        SpiceChar source[128];
        SpiceInt  handle;
        SpiceBoolean found;

        NaifStatus::CheckErrors();
        kinfo_c(_kernels[i].fullpath.toLatin1().data(), sizeof(ktype), sizeof(source),
                 ktype,source, &handle, &found);
        NaifStatus::CheckErrors();

        if (found == SPICETRUE) {
          if (!_kernels[i].loaded) nchanged++;
          _kernels[i].loaded = true;
        }
        else {
          if (_kernels[i].loaded)  nchanged++;
          _kernels[i].loaded = false;
        }
      }
    }

    return (nchanged);
  }

  /**
   * @brief Update the managed state of the kernel file list
   *
   * This method changes the managed state of individual files that are opened.
   * If a file is determined to be open it sets the managed state such that this
   * object will not manage it.  In other words, it ensures it will not close
   * the file when asked.
   *
   * This method is intended to be used when copying the contents of another
   * Kernels object and it will not allow closing of existing kernel files that
   * are found to be open.  It is useful to call the UpdateLoadStatus() method
   * first to determine load states of files.
   *
   * And files that it finds to be unloaded, it will set it manageable.
   *
   * @author kbecker (3/26/2011)
   *
   * @return int
   */
  int Kernels::UpdateManagedStatus() {
    int nchanged(0);
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      if (_kernels[i].loaded) {
        _kernels[i].managed = false;
        nchanged++;
      }
      else {
        _kernels[i].managed = true;
      }
    }
    return (nchanged);
  }

  /**
   * @brief Merge the contents of another Kernels object
   *
   * This method mergest the contents of another Kernels object into this one.
   * Kernels that exist in both instances of the object are handled special.
   * The status of this object's version of the file is updated to reflect
   * conditions of the other objects kernels.
   *
   * When the file does not exist in this object, the kernel file structure is
   * copied from the other Kernels object, its managed status is set to false
   * (since it pre-exists in another object that must relinquish its management
   * of it) and is added to this objects kernel list.
   *
   * When the file does exist in this object, its load status is checked in the
   * other Kernels object.  If it is loaded according to the other object, it is
   * marked as loaded in this object and its managed state is set to unmanaged
   * (since it contained in another object, no assumptions can be made as to
   * whom and how it came about to be loaded.
   *
   * The following code fragment demonstrates how to ensure only one Kernels
   * object retains complete control of the merged set of kernels:
   *
   * @code
   *
   * Kernels master, other;
   *
   * //  ...
   *
   *
   * //  Both objects are assumed to contain kenels and some state.
   * master.Merge(other);
   * other.UnManage();
   * master.Manage();
   * master.UpdateLoadStatus();
   * @endcode
   *
   * The UpdateLoadStatus() ensures the state of the managed kernel list in
   * master is completely up to date.
   *
   * @param other Other Kenels object containing a separate list of NAIF
   *              kernels that will be merged with the contents of this object.
   *
   * @return int Number of kernels added to this object.  It does not include
   *             the count of kernels that are common to both and may have had
   *             its status modified.
   */
  int Kernels::Merge(const Kernels &other) {
    int nAdded(0);
    for (unsigned int i = 0 ; i < other._kernels.size() ; i++) {
      KernelFile *kernel = findByName(other._kernels[i].fullpath);
      if (kernel == 0) {
        KernelFile kfile = other._kernels[i];
        kfile.managed = false;
        _kernels.push_back(kfile);
        nAdded++;
      }
      else {
        if (other._kernels[i].loaded) {
          kernel->loaded = true;
          kernel->managed = false;
        }
      }
    }
    return (nAdded);
  }


  /**
   * @brief Return list of types in kernel list
   *
   * This method determines what types of kernels are currently in the internal
   * list and returns them in alphabetical order.  An empty list is returned if
   * there are no kernels.  The list may contain "UNKNOWN" which is the default
   * type assigned when the type of the kernel file cannot be determined.  These
   * kernels are excluded from any loading/unloading.
   *
   * The list of known types are:
   *
   * CK
   * SPK
   * DAF  (synonymous for SPKs as well so load both)
   * PCK
   * EK
   * META
   * IK
   * FK
   * SCLK
   * IAK  (ISIS specific)
   * DSK
   *
   * Kernel types are determined by inspecting the first 8 characters of a
   * kernel file and extracting the contents there.  The actual type is the
   * string value after the last '/' character.  This is typically the value
   * that is also returned by the NAIF kinfo_c utility.
   *
   * @return std::vector<QString> Alphabetical list of kernel types
   */
  QStringList Kernels::getKernelTypes() const {
    TypeList kmap = categorizeByType();
    QStringList types;
    for (int i = 0 ; i < kmap.size() ; i++) {
      types.append(kmap.key(i));
    }
    return (types);
  }


  /**
   * @brief Provide a list of all the kernels found
   *
   * This method will return all the kernel file references as found in the ISIS
   * label.
   *
   * @param ktypes Optional list of kernel types to return in the list.  This
   *               can be any type found in the internal list as returned by
   *               getKernelTypes().  If empty, returns all kernels.
   *
   * @return std::vector<QString> A vector of filenames of SPICE kernels
   */
  QStringList Kernels::getKernelList(const QString &ktypes) const {

    //  If not types specified, return them all
    QStringList flist;
    if (ktypes.isEmpty()) {
      for (unsigned int k = 0; k < _kernels.size() ; k++) {
        flist.push_back(_kernels[k].pathname);
      }
    }
    else {
      // Find types and return requested types
      QStringList tlist = getTypes(ktypes);
      for (int t = 0 ; t < tlist.size() ; t++) {
        for (unsigned int k = 0; k < _kernels.size() ; k++) {
          if (_kernels[k].ktype == tlist[t]) {
            flist.push_back(_kernels[k].pathname);
          }
        }
      }
    }
    return (flist);
  }


  /**
   * @brief Returns list of kernel currently loaded according to status
   *
   * This method returns a list of kernel filenames that are loaded according to
   * internal status.  If the load status is in question, used
   * UpdateLoadStatus() prior to calling this method.
   *
   * Users can restrict the list to loaded kernels of a specific type.  For
   * example, to get a list of all CKs and SPKs loaded, the following code can
   * be used:
   *
   * @code
   *   mykernels.getLoadedList("CK,SPK");
   * @endcode
   *
   *
   * @param ktypes Optional string containing comma separated list of types to
   *               return if loaded.  If empty, returns all types of kernels
   *               that are deemed loaded.
   *
   * @return std::vector<QString> List of kernel filenames that are
   *         currently loaded.
   *
   */
  QStringList Kernels::getLoadedList(const QString &ktypes)
                                                 const {
    QStringList flist;
    if (ktypes.isEmpty()) {
      for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
        if (_kernels[i].loaded) flist.push_back(_kernels[i].pathname);
      }
    }
    else {
      QStringList tlist = getTypes(ktypes);
      for (int t = 0 ; t < tlist.size() ; t++ ) {
        for (unsigned int k = 0; k < _kernels.size() ; k++) {
          if (_kernels[k].ktype == tlist[t]) {
            flist.push_back(_kernels[k].pathname);
          }
        }
      }
    }
    return (flist);
  }


  /**
   * @brief Returns list of kernels that were not found to exist
   *
   * This method can be used to identify which kernels do not exist when they
   * were added to this object.  If the Missing() method returns a number > 0,
   * then this routine returns a list of files missing.
   *
   *
   * @return std::vector<QString>  List of missing kernel files
   */
  QStringList Kernels::getMissingList() const {
    QStringList flist;
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      if (!_kernels[i].exists) flist.push_back(_kernels[i].pathname);
    }
    return (flist);
  }

  /**
   * @brief Load a kernel file
   *
   * The internal file structure contains information related to a kernel that
   * is common to the ISIS system.  Its status is checked and only if the
   * kernels is not loaded will it be loaded via the NAIF furnsh_c utility.  Its
   * load status is updated as well as its manage status (it becomes managed).
   *
   * @param kfile Structure containing the file to load
   *
   * @return bool True if the file is loaded successfully, false if not.  It may
   *         not be loaded for various reasons: it is not a NAIF type kernel or
   *         it failed to load due to errors or non-existance.
   */
  bool Kernels::Load(Kernels::KernelFile &kfile) {
    if (IsNaifType(kfile.ktype)) {
      if (!kfile.loaded) {
        NaifStatus::CheckErrors();
        try {
          furnsh_c(kfile.fullpath.toLatin1().data());
          NaifStatus::CheckErrors();
          kfile.loaded = true;
          kfile.managed = true;
        }
        catch (IException &ie) {
          return (false);
        }
      }
    }
    return (kfile.loaded);
  }

  /**
   * @brief Unloads a kernel from the NAIF kernel pool
   *
   * This method will unload a NAIF kernel from the kernel pool.  If the
   * internal load state does not indicate it is loaded, there will be no
   * activity performed.
   *
   * The file must also be tagged as manageable by this object.  Certain
   * conditions will affect the manageability of a NAIF kernel in this object.
   * See the class documentation for details.  In the situation of the file
   * being tagged as unmanageable, the load state never changes.  Use
   * UpdateLoadStatus() to determine its true load state.
   *
   * There is good chance that a NAIF kernel may no longer be loaded in the
   * pool.  This is particularly a real possibility as a side effect of this
   * class.  Errors are, therefore, ignored when encountered attempting to
   * unload a kernel file.  The state is then set to a unloaded condition.
   *
   * @param kfile
   *
   * @return bool
   */
  bool Kernels::UnLoad(KernelFile &kfile) {
    //  If its loaded assume its a loaded NAIF kernel and unload it
    bool wasLoaded(false);
    if (kfile.loaded) {
      if (kfile.managed) {
         NaifStatus::CheckErrors();
         try {
           unload_c(kfile.fullpath.toLatin1().data());
           NaifStatus::CheckErrors();
         }
         catch (IException &) {
           // Errors are trapped and ignored.  It may be unloaded by other source
         }
         kfile.loaded = false;
         wasLoaded = true;
      }
    }
    return (wasLoaded);
  }

  /**
   * @brief Get a vector of comma separated types from a string
   *
   * This method parses a string assumed to contain NAIF kernel file types.  It
   * will return a vector of strings that contain each type found in the string
   * parameters, ktypes.
   *
   * Validity of the types of values found here are not verified but can still
   * be used effectively in other manipulation activities in this object.
   *
   * Here is now the strings are used:
   *
   * @code
   * std::vector<QString> klist = myKernels.getTypes("LSK,FK,SPK"); for
   * (unsigned int i = 0 ; i < klist.size(I) ; i++) {
   *    cout << i << ": " << klist[i] << endl;
   * }
   * @endcode
   *
   * This will result in out of the following:
   * @code
   * 0: LSK
   * 1: FK
   * 2: SPK
   * @endcode
   *
   *
   * @param ktypes Sgtring containing comma separated list of kernel file types
   *
   * @return std::vector<QString> Vector containing values between commas.
   */
  QStringList Kernels::getTypes(const QString &ktypes) const {
   // Find types and return requested types
    QStringList tlist = ktypes.split(",");
    for (int k = 0 ; k < tlist.size() ; k++) {
      tlist[k] = QString::fromStdString(IString(tlist[k].toStdString()).Trim(" \r\n\t\v\b\f").UpCase());
    }
    return (tlist);
  }

  /**
   * @brief Add a list of kernel files to internal storage
   *
   * Specialized inserter of images into exist container.
   *
   * @param klist List to add to existing collection
   */
  void Kernels::addKernels(const KernelList &klist) {
    copy(klist.begin(), klist.end(), back_inserter(_kernels));
    return;
  }


  /**
   * @brief Retrieve contents of keyword
   *
   * This method retrieves the contents of a keyword.   It is specifically
   * designed to handle Kernel group keywords.  As such, any value of "Table"
   * found in the keyword is not added to the kernel list.
   *
   *
   * @param pvl    ISIS label containing the Kernels group.
   * @param kname  Name of keyword to extract
   * @param manage Default management state to assign to kernels.  The default
   *               is to manage the kernels that are found. See examine() for
   *               full disclosure of how this is set.
   *
   * @return Kernels::KernelList List of scrutinized kernel file names
   */
  Kernels::KernelList Kernels::findKernels(Pvl &pvl,
                                           const QString &kname,
                                           const bool &manage) {
    KernelList klist;
    // Get the kernel group and load main kernels
    PvlGroup &kernels = pvl.findGroup("Kernels",Pvl::Traverse);
    // Check for the keyword
    if (kernels.hasKeyword(kname.toStdString())) {
      PvlKeyword &kkey = kernels[kname.toStdString()];
      for (int i = 0 ; i < kkey.size() ; i++) {
        if (!kkey.isNull(i)) {
          if (QString::fromStdString(kkey[i]).toLower() != "table") {
            klist.push_back((examine(QString::fromStdString(kkey[i]), manage)));
          }
        }
      }
    }

    return (klist);
  }


  /**
   * @brief Search kernel file list of a particular pattern
   *
   * This method will compare the file, kfile, to each kernel contained in the
   * internal list and return a pointer to the internal file entry structure for
   * it if it exists.
   *
   * There are three possible forms of the filename that are maintained by this
   * routine.  The kernel name as extracted from the Kernels group in the label
   * of an ISIS file.  These filenames will typically have an ISIS "environment"
   * variable included in the full file name, such as
   * "$base/kernels/lsk/naif0009.tls".  When a new file is added, the complete
   * path is stripped off and just the base filename itself is stored, such as
   * "naif0009.tls".  The third form is the fully expanded version of the file,
   * which may look like "/usgs/cpkgs/isis3/data/base/kernels/lsk/naif0009.tls".
   * All three forms of the filename are checked and only one has to match.
   *
   * @param kfile Name of file to find.  It can be any one of the three stored
   *              forms.
   *
   * @return Kernels::KernelFile* Pointer to structure containing associated
   *              with the name if it exists in the list, otherwise a 0 pointer
   *              is returned indicating it does not exist in the list
   */
  Kernels::KernelFile *Kernels::findByName(const QString &kfile)  {
    KernelList::iterator klist;
    for (klist = _kernels.begin() ; klist != _kernels.end() ; ++klist) {
      if (klist->pathname == kfile) { return (&(*klist)); }
      if (klist->name     == kfile) { return (&(*klist)); }
      if (klist->fullpath == kfile) { return (&(*klist)); }
    }
    return (0);
  }


  /**
   * @brief Categorizes the kernel list by type
   *
   * This method creates essentially a kernel type map with a pointer list to
   * each KernelFile structure of that type.
   *
   * @return Kernels::TypeList Kernel type list map of kernel file pointers
   */
  Kernels::TypeList Kernels::categorizeByType() const {
    TypeList ktypes;
    for (unsigned int i = 0 ; i < _kernels.size() ; i++) {
      KernelFile *kfile = const_cast<KernelFile *> (&_kernels[i]);
      if (ktypes.exists(_kernels[i].ktype)) {
        ktypes.get(_kernels[i].ktype).push_back(kfile);
      }
      else {
        ktypes.add(_kernels[i].ktype, KernelFileList(1, kfile));
      }
    }
    return (ktypes);
  }


  /**
   * @brief Determine if the type is a NAIF supported kernel type
   *
   *
   * @param ktype String containing the kernel type
   *
   * @return bool True if it is a NAIF kernel type
   */
  bool Kernels::IsNaifType(const QString &ktype) const {
    if (ktype.toUpper() == "UNKNOWN") return (false);
    if (ktype.toUpper() == "DEM") return (false);
    return (true);
  }
  /**
   * @brief Determine type of NAIF kernel file
   *
   *
   * This method will determine the type of kernel contained in the file
   * specified by the kfile parameter.
   *
   * The file specified by the kfile parameter is assumed to conform to NAIF
   * kernel file conventions (i.e., binary kernels are created using the NAIF
   * toolkit, text kernels conform to NAIF standards).  There are, however, two
   * exceptions that must be considered.  ISIS DEMs are cubes and do not follow
   * the NAIF convention for obvious reasons.  ISIS IAK kernels also do not
   * typically follow NAIF identification standards.  These two cases are
   * handled special.
   *
   * To determine a NAIF standard conforming file type, the first eight
   * characters of the file given will be inspected to determine the NAIF kernel
   * type.  If this fails to produce a known type, then it is assumed to be an
   * ISIS DEM or IAK kernel.
   *
   * For valid NAIF kernels, the NAIF routine kinfo_c is used to acquire
   * additional information such as if it is loaded.
   *
   * For files where the type cannot be determined, the type is set to
   * "UNKNOWN".  ISIS DEMs are set to the type "DEM".  ISIS IAKs are set to
   * "IAK".  Other types are set as follows:
   *
   * CK
   * SPK
   * DAF  (may be SPKs)
   * PCK
   * EK
   * META
   * IK
   * FK
   * SCLK
   *
   *
   * @param kfile Name of kernel file to inspect
   * @param manage Default state to assign to kernel.  Note that this only
   *               retains effect if the kernel is not loaded.  If it is loaded,
   *               its state is set to unmanaged.  You must explicitly exert
   *               management upon kernels that are already loaded.  Optional
   *               argument so the default is true.
   *
   * @return Kernels::KernelFile An internal Kernels file structure describing
   *         the file.
   */
  Kernels::KernelFile Kernels::examine(const QString &kfile,
                                       const bool &manage) const {

    FileName kernfile(kfile.toStdString());
    KernelFile kf;
    kf.pathname = kfile;
    kf.name = QString::fromStdString(kernfile.name());
    kf.fullpath = QString::fromStdString(kernfile.expanded());
    kf.exists = kernfile.fileExists();
    kf.ktype = "UNKNOWN";
    kf.loaded = false;     // Assumes its not loaded
    kf.managed = manage;

    // Determine type and load info
    if (kf.exists) {
      kf.ktype = resolveType(kf.fullpath);

      // Use NAIF to determine if it is loaded
      if (IsNaifType(kf.ktype)) {
        SpiceChar ktype[32];
        SpiceChar source[128];
        SpiceInt  handle;
        SpiceBoolean found;

        NaifStatus::CheckErrors();
        kinfo_c(kf.fullpath.toLatin1().data(), sizeof(ktype), sizeof(source), ktype,
                source, &handle, &found);
        NaifStatus::CheckErrors();

        if (found == SPICETRUE) {
          kf.loaded = true;
          kf.managed = false;
          kf.ktype = QString::fromStdString(IString(ktype).UpCase());
        }
      }
    }

    return (kf);
  }

  /**
   * @brief Determines type of NAIF/ISIS kernel we're dealing with
   *
   * This method will open the file and look at the first 8 characters.  Valid
   * NAIF binary and text kernels typically have its type defined in the first 8
   * characters.  The expected format of these characters is "DAF/CK".  This
   * method retrieves this string, trims off formatting characters and spaces
   * and returns the string after the "/" character.  This value is returned as
   * the kernel type.
   *
   * It also checks for ISIS DEMs and a special case of ISIS IAKs, which does
   * not follow the NAIF convention.  ISIS IAKs don't typically contain the NAIF
   * format identifier but do contain the ".ti" file extention.
   *
   * UNKNOWN and DAF types are further checked by the filename.  This is a last
   * resort to properly tag kernel types that are ambiguous.  DAFs are common
   * for some SPKs.  UNKNOWNs are common for many kernels that do not follow the
   * NAIF standard first 8 character identifier model.
   *
   * @param kfile Name of potential NAIF kernel to determine type for
   *
   * @return QString Value of the identifier found.  If undetermined,
   *         "UNKNOWN" is returned.
   */
  QString Kernels::resolveType(const QString &kfile) const {
    FileName kernFile(kfile.toStdString());
    QString kpath = QString::fromStdString(kernFile.expanded());
    ifstream ifile(kpath.toLatin1().data(), ios::in | ios::binary);
    QString ktype("UNKNOWN");
    if (ifile) {
      char ibuf[10];
      ifile.read(ibuf, 8);
      ibuf[8] = '\0';
      for (int i = 0 ; i < 8 ; i++)
        if (ibuf[i] == '\n') ibuf[i] = '\0';

      // See if the file is a known NAIF type.  Assume it has been
      // extracted from a NAIF compliant kernel
      QString istr = QString::fromStdString(IString(ibuf).Trim(" \n\r\f\t\v\b"));
      if (istr.contains("/")) {
        ktype = istr.split("/").last();
      }

      // If type is not resolved, check file extensions and special ISIS types
      if ((ktype == "UNKNOWN") || (ktype == "DAF")) {
        ktype = resolveTypeByExt(kfile, ktype);
      }

    }
    return (ktype);
  }

  /**
   * @brief Check kernel type by file extension and special ISIS kernels
   *
   * This method is a fallback attempt to determine the type of an expected NAIF
   * kernel.  This method assumes the proper way (inspecting first 8 characters
   * of the kernel file) did not succeed in making this determination.
   *
   * There are some times that are expected to fail this test.  ISIS DEMs are
   * cube files and will not be determined using the NAIF 8 character approach.
   * As such, all files that end in .cub are assumed to be DEMs and are tagged
   * as such.
   *
   * There are also some special ISIS IK addendum files that exist in the ISIS
   * system.  These files are used to augment an instruments' IK kernel and
   * potentially override some of the original IK contents.  This file type is
   * determined by checking for a ".ti" extension and then further checking the
   * base filename for the substring "Addendum".  This is the only way to ensure
   * it is an ISIS IAK file.  It must pass both tests or it is simply tagged as
   * an IK (due to the .ti extension).
   *
   * The remainder of the tests are strictly testing the file extension. Here is
   * how the associations are made to the files based upon their file
   * extensions:
   *
   * .cub         = DEM - ISIS cubes are DEMs
   * .ti          = IK - unless "Addendum" is in basename, then it is an IAK
   * .tf          = FK - frames kernel
   * .tsc         = SCLK - spacecraft clock kernel
   * .tsl         = LSK - leap seconds kernel
   * .tpc         = PCK - planetary ephemeris kernel
   * .bc          = CK - C-kernel
   * .bsp         = SPK - spacecraft position kernel
   * .bes         = EK - event kernels
   * .bds         = DSK - NAIF DSK shape model kernel
   * .meta        = META - NAIF meta kernels file
   *
   * If none of these file extensions or condition are found, then the value of
   * the parameter iktype is returned as the default type.
   *
   * @param kfile  File to determine type for
   * @param iktype Default type to be used in none are determined from this
   *               methiod
   *
   * @return QString Type of kernel found from file extensions
   */
  QString Kernels::resolveTypeByExt(const QString &kfile,
                                    const QString &iktype) const {

    QString ktype(iktype);  // Set default condition

    //  Deciminate file parts
    FileName kf(kfile.toStdString());
    string ext = IString(kf.extension()).DownCase();

    //  Check extensions for types
    if (ext == "cub") {
      ktype = "DEM";
    }
    else if (ext == "ti") {
      //  Assume its an instrument kernel but check for ISIS IAK file
      ktype = "IK";
      string base = IString(kf.baseName()).DownCase();
      string::size_type idx = base.find("addendum");
      if (idx != string::npos) {   // This is an ISIS IK addendum (IAK)
        ktype = "IAK";
      }
    }
    else if (ext == "tsc") {
      ktype = "SCLK";
    }
    else if (ext == "tf") {
      ktype = "FK";
    }
    else if (ext == "tls") {
      ktype = "LSK";
    }
    else if (ext == "tpc") {
      ktype = "PCK";
    }
    else if (ext == "bc") {
      ktype = "CK";
    }
    else if (ext == "bsp") {
      ktype = "SPK";
    }
    else if (ext == "bes") {
      ktype = "EK";
    }
    else if (ext == "bds") {
      ktype = "DSK";
    }
    else if (ext == "meta") {
      ktype = "META";
    }
    return (ktype);
  }



  /**
   * @brief Determine the ISIS camera model version number
   *
   * This method looks for the "CameraVersion" keyword in the Kernels group of
   * an ISIS label.  If found, it returns this number, otherwise it returns 0
   * (this is indicative of an older file).
   *
   *
   * @param pvl ISIS label
   *
   * @return int The version number found in the label.  If it cannot be
   *         determined, 0 is returned.
   */
  int Kernels::getCameraVersion(Pvl &pvl) const {
    PvlGroup &kernels = pvl.findGroup("Kernels",Pvl::Traverse);
    int cv(0);
    // Check for the keyword
    if (kernels.hasKeyword("CameraVersion")) {
      PvlKeyword &kkey = kernels["CameraVersion"];
      cv = IString(kkey[0]).ToInteger();
    }
    return (cv);
  }

}  // namespace Isis


