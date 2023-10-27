#include "Isis.h"

#include <vector>
#include <QString>

#include "Process.h"
#include "Pvl.h"
#include "PvlFormat.h"
#include "Histogram.h"
#include "IString.h"


using namespace std;
using namespace Isis;

void IsisMain() {
  // Use a regular Process
  Process p;

  // Open the input cube
  Cube *icube = p.SetInputCube("FROM", OneBand);

  // Get the desired percentage(s)
  UserInterface &ui = Application::GetUserInterface();
  QString sPercentage;
  sPercentage = ui.GetString("PERCENTAGE");
  QStringList tokens = sPercentage.split(",");
  PvlGroup results("Results");
  PvlKeyword kwPercent("Percentage");
  PvlKeyword kwValue("Value");

  for(int i = 0; i < tokens.size(); i++) {
    double percentage = toDouble(tokens[i]);
    // Obtain the Histogram and the value at the percentage
    Histogram *hist = icube->histogram();
    double value = hist->Percent(percentage);
    kwPercent += std::to_string(percentage);
    kwValue += std::to_string(value);
  }
  results += kwPercent;
  results += kwValue;

  // Log the results
  Application::Log(results);
  // Write an output file if requested
  if(ui.WasEntered("TO")) {
    Pvl temp;
    temp.addGroup(results);
    temp.write(ui.GetFileName("TO", "txt").toStdString());
  }
}
