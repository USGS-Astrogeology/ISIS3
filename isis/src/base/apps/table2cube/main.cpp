#include "Isis.h"

#include <cmath>

#include "IString.h"
#include "ProcessByLine.h"
#include "Table.h"

using namespace std;
using namespace Isis;

Table *g_table;
QString g_field;
int g_startRecord;
int g_numRecords;
int g_startElement;
int g_numElements;

void readTable(Isis::Buffer &outcube);

void IsisMain() {
  ProcessByBrick p;
  UserInterface &ui = Application::GetUserInterface();

  // Get Parameters
  g_table = new Table(ui.GetString("TABLENAME"), ui.GetFileName("FROM"));

  g_field = ui.GetString("FIELD");
  g_startRecord = ui.GetInteger("STARTREC");
  g_startElement = ui.GetInteger("STARTELEM");

  if (ui.WasEntered("NUMREC")) {
    g_numRecords = ui.GetInteger("NUMREC");
  }
  else {
    g_numRecords = g_table->Records() - (g_startRecord - 1);
  }

  if (ui.WasEntered("NUMELEM")) {
    g_numElements = ui.GetInteger("NUMELEM");
  }
  else {
    g_numElements = (*g_table)[g_startRecord][g_field.toStdString()].size() - (g_startElement - 1);
  }

  p.SetOutputCube("TO", g_numElements, g_numRecords);
  p.SetBrickSize(g_numElements, g_numRecords, 1);
  p.StartProcess(readTable);
  p.EndProcess();

  delete g_table;
}

void readTable(Isis::Buffer &outcube) {
  for (int record = g_startRecord; record < g_startRecord + g_numRecords; record ++) {
    for (int element = g_startElement; element < g_startElement + g_numElements; element ++) {
      int index = (g_numElements * (record - 1)) + (element - g_startElement);

      if ((*g_table)[record-1][g_field.toStdString()].isReal()) {
        std::vector<float> data = (*g_table)[record-1][g_field.toStdString()];
        outcube[index] = data.at(element - 1);
      }
      else if ((*g_table)[record-1][g_field.toStdString()].isInteger()) {
        std::vector<int> data = (*g_table)[record-1][g_field.toStdString()];
        outcube[index] = data.at(element - 1);
      }
      else if ((*g_table)[record-1][g_field.toStdString()].isDouble()) {
        std::vector<double> data = (*g_table)[record-1][g_field.toStdString()];
        outcube[index] = data.at(element - 1);
      }
    }
  }
}
