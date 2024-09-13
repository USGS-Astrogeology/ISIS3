/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include <string>
#include <iostream>
#include <iomanip>
#include "CSVReader.h"
#include "Preference.h"

using namespace std;
using namespace Isis;

CSVReader::CSVTable summarize(const CSVReader &csv) {
  cout << "Number of lines read: " << csv.size() << endl;
  cout << "Number of table rows: " << csv.rows() << endl;
  cout << "Number lines skipped: " << csv.getSkip() << endl;
  cout << "Got a header:         " << csv.haveHeader() << endl;

  CSVReader::CSVAxis header = csv.getHeader();
  cout << "Size of header:       " << header.dim() << endl;
  if(header.dim() > 0) {
    cout << "  First header value: " << header[0].toStdString() << endl;
  }

  CSVReader::CSVTable table = csv.getTable();
  CSVReader::CSVColumnSummary summary = csv.getColumnSummary(table);
  cout << "Number of columns:     " <<  csv.columns(table) << endl;
  cout << "Number distinct columns: " << summary.size() << endl;
  for(int ncols = 0 ; ncols < summary.size() ; ncols++) {
    cout << "--> " << summary.getNth(ncols) << " rows have "
         << summary.key(ncols) << " columns." << endl;
  }

  cout << "Table integrity OK: " << csv.isTableValid(table) << endl;
  return (table);
}

int main(int argc, char *argv[]) {
  Isis::Preference::Preferences(true);

  try {
    cout << "\n\nProcessing comma table...\n";
    QString csvfile("comma.csv");
    CSVReader csv(csvfile, true, 2);
    CSVReader::CSVTable table = summarize(csv);

    //  Convert column 0/1 to double
    CSVReader::CSVAxis scol = csv.getColumn("0/1");
    CSVReader::CSVDblVector dcol = csv.convert<double>(scol);
    cout.setf(ios::fixed);
    cout << "Size of column 0/1: " << scol.dim() << endl;
    for(int i = 0 ; i < dcol.dim() ; i++) {
      cout << "IString: " << setw(10)<< scol[i].toStdString() 
           << " \tDouble: " << setprecision(4) << dcol[i] << endl;
    }

    cout << "\n\nProcessing spaces table...\n";
    csv.setSkipEmptyParts();
    csv.setHeader(false);
    csv.setDelimiter(' ');
    csv.setSkip(2);
    csv.read("spaces.csv");
    table = summarize(csv);


    cout << "\n\nCheck empty table conditions...\n";
    csv.clear();
    summarize(csv);

    //  Lets test the stream read operation
    cout << "\n\nProcessing spaces table using streams...\n";
    ifstream isCsv("spaces.csv");
    isCsv >> csv;
    table = summarize(csv);
    isCsv.close();

  }
  catch(IException &ie) {
    ie.print();
  }

  return (0);
}
