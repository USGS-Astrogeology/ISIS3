#ifndef SplineFill_h
#define SplineFill_h

/** This is free and unencumbered software released into the public domain.

The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */                                                                  

#include <string>
#include <vector>

#include "IString.h"
#include "Module.h"
#include "NumericalApproximation.h"
#include "SpecialPixel.h"
#include "IException.h"

namespace Isis {

  /**
   * @brief Compute a low pass filter from a Module class content
   *
   * @ingroup Utility
   *
   * @author 2007-10-09 Kris Becker
   * @history 2008-11-05 Jeannie Walldren Replaced references to
   *  DataInterp class with NumericalApproximation.
   */
  class SplineFill : public Module {

    public:
      //  Constructors and Destructor
      SplineFill() : Module("SplineFill"), _filled(0) { }

      SplineFill(const Module &c) :
                     Module("SplineFill", c), _filled(0) {
        fill(c.ref());
        _history.add(formHistory());
      }

      SplineFill(const HiVector &v) :
                     Module("SplineFill"), _filled(0) {
        fill(v);
        _history.add(formHistory());
      }
      SplineFill(const HiVector &v, const HiHistory &h) :
                     Module("SplineFill", h), _filled(0) {
        fill(v);
        _history.add(formHistory());
      }

      /** Destructor */
      virtual ~SplineFill() { }

      void Process(const HiVector &v) {
        fill(v);
        _history.clear();
        _history.add(formHistory());
      }

      inline int Filled() const { return (_filled); }

    private:
      int   _filled;         //!< Number values replaced

      QString formHistory() {
        QString cfilled(QString::number(_filled));
        return (QString("SplineFill(Cubic,Filled[" + cfilled + "])"));
      }

      void fill(const HiVector &v) {
        NumericalApproximation spline(NumericalApproximation::CubicNatural);
        for (int i = 0 ; i < v.dim() ; i++) {
          if (!IsSpecial(v[i])) {
            spline.AddData(i, v[i]);
          }
        }

        //  Compute the spline and fill missing data
        HiVector vout(v.dim());
        _filled = 0;
        for (int j = 0 ; j < v.dim() ; j++) {
          if (IsSpecial(v[j])) {
            vout[j] = spline.Evaluate(j,NumericalApproximation::NearestEndpoint);
            _filled++;
          }
          else {
            vout[j] = v[j];
          }
        }

        _data = vout;
        return;
      }
  };

}     // namespace Isis
#endif
