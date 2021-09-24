#ifndef HistogramPlotWindow_h
#define HistogramPlotWindow_h

#include <qwt_text.h>

#include "PlotWindow.h"

class QDockWidget;

namespace Isis {
  class HistogramItem;

  /**
   * @author ????-??-?? Unknown
   *
   * @internal
   */
  class HistogramPlotWindow : public PlotWindow {
      Q_OBJECT

    public:
      HistogramPlotWindow(QString title, QWidget *parent);
      using PlotWindow::add;
      void add(HistogramItem *);

//       void setViewport(CubeViewport *cvp);
      int getNumItems() {
        return p_histItems.size();
      }

      HistogramItem *getHistItem(int index) {
        return p_histItems[index];
      }

      QDockWidget *getDockWidget() {
        return p_dock;
      }

    public slots:


    private:
//       CubeViewport  *p_cvp; //!< The current viewport
      QList<HistogramItem *> p_histItems;
      QDockWidget *p_dock;
  };
};

#endif
