#include "TableMainWindow.h"

#include <iostream>

#include <QAction>
#include <QDockWidget>
#include <QFileDialog>
#include <QHeaderView>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>


namespace Isis {
  /**
   * Constructs a new TableMainWindow object
   *
   * @param title
   * @param parent
   */
  TableMainWindow::TableMainWindow(QString title, QWidget *parent) : MainWindow(title, parent) {
    p_parent = parent;

    p_title = title;
    p_table = NULL;
    p_visibleColumns = -1;
    p_currentRow = 0;
    p_currentIndex = 0;
    setObjectName(title);
    createTable();
    readSettings(QSize(500, 300));
    readColumnSettings();
  }


  TableMainWindow::~TableMainWindow() {
    writeSettings();
  }


  void TableMainWindow::clear() {
    writeSettings();
    p_table->clear();
    p_listWidget->clear();
    p_table->setRowCount(0);
    p_table->setColumnCount(0);
  }


  QList<QListWidgetItem *> TableMainWindow::itemList() const {
    QList<QListWidgetItem *> result;

    if (p_listWidget)
      result = p_listWidget->findItems(QString("*"), Qt::MatchWrap | Qt::MatchWildcard);

    return result;
  }

  void TableMainWindow::resizeColumn(int columnIndex) {
    QHeaderView* header = p_table->horizontalHeader();

    QString columnName(p_table->model()->headerData(columnIndex, Qt::Horizontal).toString());

    if (columnName.isEmpty())
    {
      return;
    }

    if (header->sectionResizeMode(columnIndex) == QHeaderView:: ResizeToContents) {
      header->setSectionResizeMode(columnIndex, QHeaderView::Interactive);
    }
    else {
      header->setSectionResizeMode(columnIndex, QHeaderView::ResizeToContents);
    }
  }

  /**
   * This creates the table main window.  The table and docking
   * area are created here. It also adds the two default menus to
   * the menu bar. Programmers can add more menus to the menu bar
   * once an instance of this class is established.
   */
  void TableMainWindow::createTable() {
#if defined(__APPLE__)
    setWindowFlags(Qt::Tool);
#endif

#ifndef __APPLE__
    setWindowFlags(Qt::Dialog);
#endif

    statusBar()->setSizeGripEnabled(true);
    // Create the table widget
    p_table = new QTableWidget(this);
    p_table->setAlternatingRowColors(true);
    //
    QHeaderView* columnHeader = p_table->horizontalHeader();
    columnHeader->setSectionResizeMode(QHeaderView::ResizeToContents);
    connect ( columnHeader, SIGNAL( sectionPressed(int) ),
              this, SLOT( resizeColumn(int) ) );

    setCentralWidget(p_table);

    // Create the dock area
    p_dock = new QDockWidget("Columns", this);
    p_dock->setObjectName("dock");
    p_dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    p_dock->setMinimumWidth(190);
    p_listWidget = new QListWidget(p_dock);
    p_dock->setWidget(p_listWidget);
    addDockWidget(Qt::LeftDockWidgetArea, p_dock, Qt::Vertical);
    connect(p_listWidget, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(syncColumns()));

    // Create the file menu
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu("&File");

    p_save = new QAction(this);
    p_save->setText("Save...");
    p_save->setShortcut(Qt::CTRL + Qt::Key_S);
    connect(p_save, SIGNAL(triggered()), this, SLOT(saveTable()));
    p_save->setDisabled(true);

    QAction *saveas = new QAction(this);
    saveas->setText("Save As...");
    connect(saveas, SIGNAL(triggered()), this, SLOT(saveAsTable()));

    QAction *load = new QAction(this);
    load->setText("Load...");
    connect(load, SIGNAL(triggered()), this, SLOT(loadTable()));

    QAction *del = new QAction(this);
    del->setText("Delete Selected Row(s)");
    del->setShortcut(Qt::Key_Delete);
    connect(del, SIGNAL(triggered()), this, SLOT(deleteRows()));

    QAction *clear = new QAction(this);
    clear->setText("Clear table");
    clear->setShortcut(Qt::CTRL + Qt::Key_Delete);
    connect(clear, SIGNAL(triggered()), this, SLOT(clearTable()));

    QAction *close = new QAction(this);
    close->setText("Close");
    connect(close, SIGNAL(triggered()), this, SLOT(hide()));

    fileMenu->addAction(p_save);
    fileMenu->addAction(saveas);
    fileMenu->addAction(load);
    fileMenu->addAction(del);
    fileMenu->addAction(clear);
    fileMenu->addAction(close);

    //2009-01-12
    //If we have the Mainwindow flag set to Qt::Tool so that on Macs the
    //table window always stays on top, then we can not access the
    //menu bar to the table window, so we need to add the file options
    //for the table to the tool bar.
#if defined(__APPLE__)
    QToolBar *toolBar = new QToolBar();
    toolBar->setObjectName("ToolBar");
    toolBar->addAction(p_save);
    toolBar->addAction(saveas);
    toolBar->addAction(load);
    toolBar->addAction(del);
    toolBar->addAction(clear);
    toolBar->addAction(close);
    this->addToolBar(toolBar);
#endif

    // Create the view menu
    QMenu *viewMenu = menuBar->addMenu("&View");
    QAction *cols = new QAction(this);
    cols->setText("Columns");
    connect(cols, SIGNAL(triggered()), p_dock, SLOT(show()));
    viewMenu->addAction(cols);
    this->setMenuBar(menuBar);
    installEventFilter(this);
  }


  /**
   * sets the status message in the lower lefthand corner of the
   * window.
   *
   * @param message
   */
  void TableMainWindow::setStatusMessage(std::string message) {
    this->statusBar()->showMessage(message);
  }


  /**
   *
   * Adds a new column to the table when a new curve is added to the plot.  Also
   * adds the item to the dock area.
   *
   * @param setOn
   * @param heading
   * @param menuText
   * @param insertAt
   * @param o
   * @param toolTip
   */
  void TableMainWindow::addToTable(bool setOn, const QString &heading,
                                   const QString &menuText, int insertAt,
                                   Qt::Orientation o, QString toolTip) {
    // Insert the new column
    int startCol = p_table->columnCount();

    for(int i = 0; !heading.section(":", i, i).isEmpty(); i++) {
      QString htext = heading.section(":", i, i);

      int destinationColumn = insertAt;

      if (insertAt >= 0) {
        p_table->insertColumn(insertAt);
      }
      else {
        destinationColumn = startCol + i;
        p_table->insertColumn(startCol + i);
      }
      QTableWidgetItem *header = new QTableWidgetItem(htext);
      if (insertAt >= 0) {

        if (o == Qt::Horizontal) {
          p_table->setHorizontalHeaderItem(insertAt, header);
        }
        else {
          p_table->setVerticalHeaderItem(insertAt, header);
        }
      }
      else {

        if (o == Qt::Horizontal) {
          p_table->setHorizontalHeaderItem(startCol + i, header);
        }
        else {
          p_table->setVerticalHeaderItem(startCol + i, header);
        }
      }

      p_table->setColumnWidth(destinationColumn,
          QFontMetrics(header->font()).horizontalAdvance(header->text()) + 20);
          // Removed: rounding and int?
          // qRound(QFontMetrics(header->font()).horizontalAdvance(header->text()) + 20));
    }

    int endCol = p_table->columnCount() - 1;

    // Insert the column name into the columns dock area
    if (!menuText.isEmpty()) {
      QListWidgetItem *item = new QListWidgetItem();

      item->setText(menuText);
      if (toolTip.isEmpty()) {
        item->setToolTip(heading);
      }
      else {
        item->setToolTip(toolTip);
      }

      if (insertAt >= 0) {
        p_listWidget->insertItem(insertAt, item);
      }
      else {
        p_listWidget->insertItem(endCol, item);
      }

      item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

      readItemSettings(menuText, item, setOn);

      p_startColumn.push_back(startCol);
      p_endColumn.push_back(endCol);

      readItemSettings(heading, item, setOn);
    }
    readColumnSettings();
  }


  /**
   * This method hides and shows the columns according to which
   * items the user has selected to be view-able on the left hand
   * side list. (dock area)
   */
  void TableMainWindow::syncColumns() {
    if (this->isHidden()) {
      return;
    }

    p_visibleColumns = 0;
    for(int i = 0; i < p_listWidget->count(); i++) {
      QListWidgetItem *item = p_listWidget->item(i);
      int index = itemList().indexOf(item);

      if (index != -1) {
        for(int col = p_startColumn[index]; col <= p_endColumn[index]; col++) {

          if (item->checkState() == Qt::Checked) {
            p_table->setColumnHidden(col, false);
            p_visibleColumns++;
          }
          else {
            p_table->setColumnHidden(col, true);
          }

        }
      }
    }
  }


  /**
   * Use this method to sync the table with the dock widget list
   * if the table orientation is horizontal.
   *
   */
  void TableMainWindow::syncRows() {
    if (!isHidden()) {
      p_visibleColumns = 0;
      for (int i = 0; i < p_listWidget->count(); i++) {
        QListWidgetItem *item = p_listWidget->item(i);
        int index = itemList().indexOf(item);

        if (index != -1) {

          for (int col = p_startColumn[index];
               col <= p_endColumn[index];
               col++) {

            if (item->checkState() == Qt::Checked) {
              p_table->setRowHidden(col, false);
              p_visibleColumns++;
            }
            else {
              p_table->setRowHidden(col, true);
            }

          }
        }
      }
    }
  }


  /**
   * This method deletes a column from the table
   *
   * @param item
   */
  void TableMainWindow::deleteColumn(int item) {
    if (p_table != NULL) {
      p_table->setColumnCount(p_table->columnCount() - 1);

      bool vis = p_table->isVisible();
      p_table = NULL;
      p_listWidget = NULL;
      close();

      if (vis) showTable();
    }
  }


  /**
   * This method checks to see if the table has been created. If
   * not it calls the createTable method before calling show.
   * 
   * @history 2017-10-06 Adam Goins - showTable() now calls syncColumns() after it calls
   *                        this->show() so that it hides the unselected columns appropriately.
   *                        Fixes #5141.
   *
   * @history 2017-10-06 Adam Goins - showTable() now calls syncColumns() after it calls
   *                        this->show() so that it hides the unselected columns appropriately.
   *                        Fixes #5141.
   *
   */
  void TableMainWindow::showTable() {
    if (p_table == NULL) createTable();
    this->show();
    syncColumns();
  }


  /**
   * This method clears all items from each row and column.
   *
   */
  void TableMainWindow::clearTable() {
    if (p_table->rowCount() == 0) return;
    for(int r = 0; r < p_table->rowCount(); r++) {
      for(int c = 0; c < p_table->columnCount(); c++) {
        p_table->item(r, c)->setText("");
      }
    }

    p_table->scrollToItem(p_table->item(0, 0), QAbstractItemView::PositionAtTop);
    p_currentRow = 0;
    p_currentIndex = 0;
  }


  /**
   * This method is called when the user selects a row or rows
   * uses the delete button or selects the delete row menu item
   * from the file menu.
   *
   */
  void TableMainWindow::deleteRows() {
    QList<QTableWidgetItem *> list = p_table->selectedItems();
    QList<int> selectedRows;

    for(int i = 0; i < list.size(); i++) {
      if (selectedRows.size() == 0) {
        selectedRows.push_back(p_table->row(list[i]));
      }
      else if (!selectedRows.contains(p_table->row(list[i]))) {
        selectedRows.push_back(p_table->row(list[i]));
      }
    }

    std::sort(selectedRows.begin(), selectedRows.end());
    for(int d = selectedRows.size(); d > 0; d--) {
      p_table->removeRow(selectedRows[d-1]);
    }

    // get the number of rows that are filled in
    int filledRows = 0;
    for(int r = 0; r <  p_table->rowCount(); r++) {
      for(int c = 0; c < p_table->columnCount(); c++) {
        if (!p_table->item(r, c) || p_table->item(r, c)->text() != "") {
          filledRows++;
          break;
        }
      }
    }

    p_currentRow = filledRows;
  }


  /**
   * This method clears the text of the given row.
   *
   * @param row
   */
  void TableMainWindow::clearRow(int row) {
    if (!this->isVisible()) return;

    for(int c = 0; c < p_table->columnCount(); c++) {
      p_table->item(row, c)->setText("");
    }
  }

  /**
   * This method will select a file, set it as the current file and save the
   * table.
   *
   */
  void TableMainWindow::saveAsTable() {
    QString fn = QFileDialog::getSaveFileName((QWidget *)parent(),
                 "Choose filename to save under",
                 ".",
                 "Text Files (*.txt)");
    QString filename;

    //Make sure the filename is valid
    if (!fn.isEmpty()) {
      if (!fn.endsWith(".txt")) {
        filename = fn + ".txt";
      }
      else {
        filename = fn;
      }
    }
    //The user cancelled, or the filename is empty
    else {
      return;
    }

    p_currentFile.setFileName(filename);

    p_save->setEnabled(true);
    saveTable();
  }

  /**
   * This method allows the user to save the data from the
   * table to the current file.
   */
  void TableMainWindow::saveTable() {
    if (p_currentFile.fileName().isEmpty()) return;

    //if (p_currentFile.exists()) p_currentFile.remove();
    bool success = p_currentFile.open(QIODevice::WriteOnly);
    if (!success) {
      QMessageBox::critical((QWidget *)parent(),
                            "Error", "Cannot open file, please check permissions");
      p_currentFile.setFileName("");
      p_save->setDisabled(true);
      return;
    }

    QString currentText;
    QTextStream t(&p_currentFile);
    QString line = "";
    bool first = true;

    //Write each column's header to the first line in CSV format
    for(int i = 0; i < p_table->columnCount(); i++) {
      if (!p_table->isColumnHidden(i)) {
        QTableWidgetItem *header = p_table->horizontalHeaderItem(i);
        QString temp = header->text();
        temp.replace("\n", " ");
        if (first) {
          line = line + "\"" + temp + "\"";
          first = false;
        }
        else {
          line = line + ",\"" + temp + "\"";
        }
      }
    }
    //Add the headers to the file
    t << line << Qt::endl;

    //Add each row to the file
    for(int i = 0; i < p_table->rowCount(); i++) {
      bool first = true;
      line = "";

      //Add each column to the line
      for(int j = 0; j < p_table->columnCount(); j++) {

        if (!p_table->isColumnHidden(j)) {
          if (p_table->item(i, j) == 0) break;
          currentText = p_table->item(i, j)->text();

          if (first) {
            line = line + currentText;
            first = false;
          }
          else {
            line = line + "," + currentText;
          }
        }
      }
      //If the line is not empty, add it to the file
      if (line.split(",", Qt::SkipEmptyParts).count() != 0)
        t << line << Qt::endl;
    }
    p_currentFile.close();
    this->setWindowTitle(p_title + " : " + p_currentFile.fileName());
  }


  /**
   * This method reads the 'checked' settings on the items listed
   * in the dock area which determine which table columns are
   * visible.  This method is only called if p_trackItems is set
   * to true.
   *
   * @param heading
   * @param item
   */
  void TableMainWindow::readItemSettings(QString heading,
      QListWidgetItem *item, bool defaultChecked) {
    QSettings settings(settingsFileName(), QSettings::NativeFormat);

    QString itemTitle = "item-" + item->text();
    Qt::CheckState defaultState = defaultChecked? Qt::Checked : Qt::Unchecked;
    Qt::CheckState state = (Qt::CheckState)
        settings.value(itemTitle, defaultState).toInt();
    item->setCheckState(state);
  }

  /**
   * This method reads the columns in the table and sets their size
   * to the appropriate size, or the size to auto based on what they were
   * stored as.
   *
   */
  void TableMainWindow::readColumnSettings() {

    QHeaderView *header = p_table->horizontalHeader();
    QSettings settings(settingsFileName(), QSettings::NativeFormat);

    for (int columnIndex = 0; columnIndex < p_table->model()->columnCount(); columnIndex++) {

       QString headerName = p_table->model()->headerData(columnIndex, Qt::Horizontal).toString();
       QString settingName = "column-" + headerName;
       QString value = settings.value(settingName, "auto").toString();

       if (value == "auto" || value == "0") {
          header->setSectionResizeMode(columnIndex, QHeaderView::ResizeToContents);
       }
       else {
         int width = value.toInt();

         header->setSectionResizeMode(columnIndex, QHeaderView::Interactive);
         p_table->setColumnWidth(columnIndex, width);
       }
    }
  }


  /**
   * This overriden method is called when the Tablemainwindow
   * is closed or hidden to write the size and location settings
   * (and dock widget settings) to a config file in the user's
   * home directory.
   *
   *  @internal
   *    @history 2017-11-13 Adam Goins - Added the behavior of columns sizes to be written
   *                            to the settings file so that they can be persistent through
   *                            new instances of qview.
   *
   */
  void TableMainWindow::writeSettings() const {
    if (p_listWidget) {
      QSettings settings(settingsFileName(), QSettings::NativeFormat);

      foreach (QListWidgetItem *item, itemList()) {
        QString itemTitle = "item-" + item->text();
        settings.setValue(itemTitle, item->checkState());
      }

      QHeaderView *header = p_table->horizontalHeader();
      for(int columnIndex = 0; columnIndex < p_table->model()->columnCount(); columnIndex++)
      {
         QString headerName = p_table->model()->headerData(columnIndex, Qt::Horizontal).toString();
         QString settingName = "column-" + headerName;

         if (header->sectionResizeMode(columnIndex) == QHeaderView::ResizeToContents)
         {
           settings.setValue(settingName, "auto");
         }
         else
         {
           settings.setValue( settingName, header->sectionSize(columnIndex) );
         }
      }
    }
  }


  /**
   * This method loads a text file into the table.
   *
   */
  void TableMainWindow::loadTable() {
    QString fn = QFileDialog::getOpenFileName((QWidget *)parent(),
                 "Select file to load",
                 ".",
                 "Text Files (*.txt)");

    //If the user cancelled or the filename is empty return
    if (fn.isEmpty()) return;

    p_currentFile.setFileName(fn);
    p_save->setEnabled(true);

    bool success = p_currentFile.open(QIODevice::ReadOnly);
    if (!success) {
      QMessageBox::critical((QWidget *)parent(),
                            "Error", "Cannot open file, please check permissions");
      p_currentFile.setFileName("");
      p_save->setDisabled(true);
      return;
    }

    clearTable();

    QList<int> column = QList<int>();

    //Set all headers to be hidden
    for(int i = 0; i < p_listWidget->count() ; i++) {
      p_listWidget->item(i)->setCheckState(Qt::Unchecked);
    }

    // Strip headers off the table into the temp string
    QString temp = p_currentFile.readLine();
    temp.remove("Positive ");
    temp.remove("\"");
    temp.remove("\n");
    QStringList list = temp.split(",");

    // Loop through checking header names and setting relevant columns visible
    for(int i = 0; i < list.count(); i++) {
      for(int j = 0; j < itemList().size(); j++) {
        QListWidgetItem *item = itemList()[j];

        //Special cases
        if (item->text() == "Ground Range" && (list[i] == "Start Latitude" || list[i] == "Start Longitude" ||
            list[i] == "End Latitude" || list[i] == "End Longitude")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Pixel Range" && (list[i] == "Start Sample" || list[i] == "Start Line" ||
            list[i] == "End Sample" || list[i] == "End Line")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Pixel Range" && (list[i] == "Sample" || list[i] == "Line")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Sample:Line" && (list[i] == "Sample" || list[i] == "Line")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Planetocentric Lat" && list[i] == "Planetocentric Latitude") {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Planetographic Lat" && list[i] == "Planetographic Latitude") {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Projected X:Projected Y" && (list[i] == "Projected X" || list[i] == "Projected Y")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Radius" && list[i] == "Local Radius") {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "XYZ" && (list[i] == "Point X" || list[i] == "Point Y" || list[i] == "Point Z")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Ra:Dec" && (list[i] == "Right Ascension" || list[i] == "Declination")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Spacecraft Position" && (list[i] == "Spacecraft X" || list[i] == "Spacecraft Y" || list[i] == "Spacecraft Z")) {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Ephemeris Time" && list[i] == "Ephemeris Time") {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Local Solar Time" && list[i] == "Local Solar Time") {
          item->setCheckState(Qt::Checked);
          break;
        }
        if (item->text() == "Segments Sum" && list[i] == "Segments Sum km") {
          item->setCheckState(Qt::Checked);
          break;
        }
        //End special cases


        if (item->text() == list[i]) {
          item->setCheckState(Qt::Checked);
          break;
        }
      }

      // Loop through column headers to try to find a match
      bool match = false;
      for(int cols = 0; cols < p_table->columnCount(); cols++) {
        QString header = p_table->horizontalHeaderItem(cols)->text();

        header.remove("Positive");
        header.remove("\n");
        header.remove(" ");

        list[i].remove(" ");

        if (header == list[i]) {
          column.push_back(cols);
          match = true;
          break;

        }
      }
      if (!match) column.push_back(-1);
    }

    // Read data into table
    QString str = p_currentFile.readLine();

    while(str.count() != 0) {
      // Do we need more rows?
      if (p_currentRow + 1 > p_table->rowCount()) {
        p_table->insertRow(p_currentRow);
        for(int c = 0; c < p_table->columnCount(); c++) {
          QTableWidgetItem *item = new QTableWidgetItem("");
          p_table->setItem(p_currentRow, c, item);
          if (c == 0) p_table->scrollToItem(item);
        }
      }

      str.remove("\n");

      QStringList list = str.split(",");

      for(int i = 0; i < column.size(); i++) {
        QTableWidgetItem *newItem = new QTableWidgetItem(list[i]);
        if (column[i] != -1) {
          p_table->setItem(p_currentRow, column[i], newItem);
        }
      }
      p_currentRow++;
      p_currentIndex++;


      str = p_currentFile.readLine();
    }

    p_currentFile.close();
    this->setWindowTitle(p_title + " : " + fn);
    emit fileLoaded();
  }


  /**
   * This event filter is installed in the constructor.
   * @param o
   * @param e
   *
   * @return bool
   */
  bool TableMainWindow::eventFilter(QObject *o, QEvent *e) {
    switch(e->type()) {
      case QEvent::Close: {
          this->writeSettings();
        }

      default: {
          return false;
        }
    }
  }


  /**
   *
   *
   *
   * @param event
   */
  void TableMainWindow::closeEvent(QCloseEvent *event) {
    MainWindow::closeEvent(event);
    writeSettings();
    event->accept();
  }


  /**
   *
   *
   *
   * @param event
   */
  void TableMainWindow::hideEvent(QHideEvent *event) {
//     this->writeSettings();
  }


  /**
   * If this property is true, the class will keep track of the
   * checked/unchecked items in the dock area which determines
   * which columns are visible in the table.
   *
   * @param track
   */
  void TableMainWindow::setTrackListItems(bool track) {
    p_trackItems = track;
  }


  /**
   * Returns whether or not we should track items
   *
   *
   * @return bool
   */
  bool TableMainWindow::trackListItems() {
    return p_trackItems;
  }


  /**
   * Sets the current row to row
   *
   * @param row
   */
  void TableMainWindow::setCurrentRow(int row) {
    p_currentRow = row;
  }


  /**
   * Sets the current index to currentIndex
   *
   * @param currentIndex
   */
  void TableMainWindow::setCurrentIndex(int currentIndex) {
    p_currentIndex = currentIndex;
  }

}
