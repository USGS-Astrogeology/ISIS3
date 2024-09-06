#ifndef TableMainWindow_h
#define TableMainWindow_h

#include <QMainWindow>
#include <QMenu>

#include <QListWidgetItem>
#include <QPointer>

#include "MainWindow.h"

class QTableWidget;

namespace Isis {
  /**
  * @brief a subclass of the qisis mainwindow, tablemainwindow
  *        handles all of the table tasks.
  *
  * @ingroup Visualization Tools
  *
  * @author ????-??-?? Stacy Alley
  *
  * @internal
  *  @history 2008-06-12 Noah Hilt - Changed the save/load
  *                          functions to work with blank entries.
  *  @history 2008-06-25 Noah Hilt - Fixed the delete rows method to search the
  *                          entire row for values, rather than just the first column.
  *  @history 2008-10-17 Noah Hilt - Added an optional tooltip parameter to the
  *                          addToTable method. This tooltip will be displayed over the checkbox
  *                          that is to be added to the dock area.
  *  @history 2008-11-25 Noah Hilt - Added an action to save the table to a file
  *                          that has been loaded or set with the save as action. This way the
  *                          user does not have to reselect the file to save the table to.
  *  @history 2012-06-18 Steven Lambright - Gave the mac toolbar an object name for
  *                          saving and restoring state. Fixes #851."
  *  @history 2013-02-21 Steven Lambright - Fixed a seg fault on destruction. This happened because
  *                          the item list was being saved off independently; the items would be
  *                          destroyed in the table widget but our copies of the pointers were not
  *                          yet cleared. This has been fixed by adding the method itemList()
  *                          instead of storing p_itemList. References #710.
  *   @history 2016-08-28 Kelvin Rodriguez - writeSettings now const to match parent and eliminate
  *                          hidden virtual overload warnings in clang. Part of porting to
  *                          OS X 10.11
  *   @history 2017-10-06 Adam Goins - showTable() now calls syncColumns() after it calls
  *                          this->show() so that it hides the unselected columns appropriately.
  *                          Fixes #5141.
  *   @history 2017-11-13 Adam Goins - Afforded TableMainWindow the ability to toggle between auto
  *                          resizing columns or setting a specific column size for each column.
  *                          modified createTable() to set the resize property of the columns to
  *                          resize automatically based on the content inside of the column.
  *                          Added resizeColumn() slot and readColumnSettings().
  *                          modified writeSettings() to write updated settings on destroy.
  *                          Fixes #5142.
  *   @history 2018-04-20 Adam Goins - Added the ctrl+del keyboard shortcut to the TableMainWindow's
  *                          "Clear Table" menu option. Fixes #4912.
  */
  class TableMainWindow : public MainWindow {
      Q_OBJECT
    public:
      TableMainWindow(QString title, QWidget *parent = 0);
      ~TableMainWindow();

      void clear();

      /**
       * Returns the table
       *
       * @return QTableWidget*
       */
      QTableWidget *table() const {
        return p_table;
      };


      QList<QListWidgetItem *> itemList() const;


      /**
       *
       * Returns the list widget
       *
       * @return QListWidget*
       */
      QListWidget *listWidget() const {
        return p_listWidget;
      };

      /**
       *
       * Returns the selected rows
       *
       * @return int
       */
      int selectedRows() const {
        return  p_selectedRows;
      };

      /**
       *
       * Returns the current index
       *
       * @return int
       */
      int currentIndex() const {
        return  p_currentIndex;
      };

      /**
       *
       * Returns the current row
       *
       * @return int
       */
      int currentRow() const {
        return  p_currentRow;
      };

      void addToTable(bool setOn, const QString &heading,
                      const QString &menuText = "", int insertAt = -1,
                      Qt::Orientation o = Qt::Horizontal, QString toolTip = "");
      void deleteColumn(int item);
      void setStatusMessage(std::string message);
      void closeEvent(QCloseEvent *event);
      void hideEvent(QHideEvent *event);

    public slots:
      void showTable();
      void syncColumns();
      void syncRows();
      void saveAsTable();
      void saveTable();
      void clearTable();
      void deleteRows();
      void clearRow(int row);
      void setCurrentRow(int row);
      void setCurrentIndex(int currentIndex);
      void setTrackListItems(bool track = false);
      bool trackListItems();
      void loadTable();
      void writeSettings() const;
      void resizeColumn(int columnIndex);

    signals:
      /**
       * Signal emitted when a file has loaded
       *
       */
      void fileLoaded();

    protected:
      bool eventFilter(QObject *o, QEvent *e);
      void createTable();
      void readItemSettings(QString heading, QListWidgetItem *item,
                            bool defaultChecked);

      void readColumnSettings();

    private:
      std::string p_appName; //!< The application name
      QWidget *p_parent; //!< The parent widget
      QString p_title; //!< The title string
      QDockWidget *p_dock; //!< The dock widget

      QAction *p_save; //!< Action to save the table to the current file
      QFile p_currentFile; //!< The current file

      QTableWidget *p_table; //!< The table
      QPointer<QListWidget> p_listWidget; //!< List widget
      int p_selectedRows; //!< Number of selected rows
      int p_currentIndex; //!< Current index
      int p_currentRow; //!< Current row

      int p_visibleColumns; //!< Number of visible columns
      QList<int>p_startColumn; //!< List of start columns
      QList<int>p_endColumn; //!< List of end columns
      bool p_trackItems; //!< Boolean to track items

  };
};

#endif
