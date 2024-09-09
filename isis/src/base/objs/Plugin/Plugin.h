#ifndef Plugin_h
#define Plugin_h
/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */

#include "Pvl.h"
#include <QString>

namespace Isis {
  /**
   * @brief Loads plugins from a shared library
   *
   * This class is used to handle dynamic loading of module/classes.  It  is
   * rarely used directly but instead is inherited for a particular purpose such
   * as managing class specific map projections or camera models.
   * The class is derived from a PVL which aides in the selection of the
   * shared library and plugin routine to load.  For example, assume the file
   * my.plugin contained:
   *  @code
   *    OBJECT=SINUSOIDAL
   *      LIBRARY=libisis3.so
   *      ROUTINE=SinusoidalPlugin
   *    END_OBJECT
   *    OBJECT=SIMPLECYLINDRICAL
   *      LIBRARY=libisis3.so
   *      ROUTINE=SimpleCylindricalPlugin
   *    END_OBJECT
   *  @endcode
   * The desired routine can be selected in code as follows:
   * @code
   * Plugin p;
   * p.Read("my.plugin");
   * string proj;
   * cin >> proj;  // Enter either SINUSOIDAL or SIMPLECYLINDRICAL
   * p.Find(proj);
   * void *ptr = p.GetPlugin();
   * @endcode
   * Obtaining plugins can be difficult to understand.  It is suggested you
   * look at ProjectionFactory to get a better understanding of how they are used.
   *
   * @see ProjectionFactory
   * @see CameraFactory
   *
   * @ingroup System
   *
   * @author 2004-02-07 Jeff Anderson
   *
   * @internal
   *   @history 2005-02-15 Jeff Anderson refactored to use Qt Qlibrary class.
   *   @history 2007-07-19 Steven Lambright Fixed memory leak
   */
  class Plugin : public Isis::Pvl {
    public:
      Plugin();

      //! Destroys the Plugin object.
      virtual ~Plugin() {};

      QFunctionPointer GetPlugin(const QString &group);
  };
};

#endif
