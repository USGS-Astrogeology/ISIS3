#include "Isis.h"

#include <QFile>
#include <QQueue>
#include <QSet>
#include <QVector>

#include "Buffer.h"
#include "Chip.h"
#include "Cube.h"
#include "FileList.h"
#include "FileName.h"
#include "OverlapStatistics.h"
#include "ProcessByLine.h"
#include "UserInterface.h"
#include "IException.h"
#include "IString.h"


using namespace Isis;


int r1;
int r2;
int c1;
int c2;

/**
 * @author ????-??-?? Unknown
 *
 * @internal
 */
class Node {
  public:
    Node(int sample, int line, int samples, int lines, int score) {
      m_sample = sample;
      m_line = line;
      m_samples = samples;
      m_lines = lines;
      m_score = score;
    }

    int createIndex(int sample, int line) {
      return line * m_samples + sample;
    }

    bool hasNeighbor(int sample, int line) {
      return inBounds(sample, line);
    }

    bool inBounds(int sample, int line) {
      return sample >= c1 && sample <= c2 && line >= r1 && line <= r2;
    }

    Node create(int sample, int line, int stop) {
      return Node(sample, line, m_samples, m_lines, getNextScore(stop));
    }

    void evaluate(int sample, int line,
        QQueue<Node> &nodes, QVector<int> &ol, int stop) {

      if (hasNeighbor(sample, line)) {
        int index = createIndex(sample, line);
        if (ol[index] == 0) {
          ol[index] = getNextScore(stop);
          nodes.enqueue(create(sample, line, stop));
        }
      }
    }

    void evaluateUp(QQueue<Node> &nodes, QVector<int> &ol, int stop) {
      evaluate(m_sample, m_line - 1, nodes, ol, stop);
    }

    void evaluateDown(QQueue<Node> &nodes, QVector<int> &ol, int stop) {
      evaluate(m_sample, m_line + 1, nodes, ol, stop);
    }

    void evaluateLeft(QQueue<Node> &nodes, QVector<int> &ol, int stop) {
      evaluate(m_sample - 1, m_line, nodes, ol, stop);
    }

    void evaluateRight(QQueue<Node> &nodes, QVector<int> &ol, int stop) {
      evaluate(m_sample + 1, m_line, nodes, ol, stop);
    }

    int getNextScore(int stop) {
      return (m_score < stop) ? m_score + 1 : stop;
    }

    ~Node() {}

  private:
    int m_sample;
    int m_line;
    int m_samples;
    int m_lines;
    int m_score;
};


void blend(Buffer &in);
bool inIntersection(int sample, int line);
bool validValue(int oSample, int oLine);
Chip * createRamp(Chip *pic1, Chip *pic2, int stop);
void processNodes(QQueue<Node> &nodes, QVector<int> &ol,
    int &maxScore, int stop);

void readOutputs(QString outName, const FileList &inputs, FileList &outputs);
void generateOutputs(const FileList &inputs, FileList &outputs);


Chip *blendRamp;
Chip *i1;
Chip *i2;
int s1;
int s2;
int l1;
int l2;
int line;


void IsisMain() {
  blendRamp = NULL;
  i1 = NULL;
  i2 = NULL;

  // Get the user interface
  UserInterface &ui = Application::GetUserInterface();

  FileList inputs(ui.GetFileName("FROMLIST"));
  if (inputs.size() < 2) {
    std::string msg = "FROMLIST must have at least two images to blend";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  FileList outputs;
  ui.WasEntered("TOLIST") ?
      readOutputs(ui.GetFileName("TOLIST"), inputs, outputs) :
      generateOutputs(inputs, outputs);

  for (int i = 0; i < inputs.size(); i++) {
    FileName inputFileName=inputs[i];
    FileName outputFileName=outputs[i];
    QString input(inputFileName.expanded());
    QString output(outputFileName.expanded());

    QFile::remove(output);
    if (!QFile::copy(input, output)) {
      std::string msg = "Cannot create output cube [" + output + "]";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }

  QSet<QString> overlapped;
  for (int j = 1; j < outputs.size(); j++) {
    Cube from2;
    from2.open(outputs[j].toString());

    for (int i = 0; i < j; i++) {
      Cube from1;
      from1.open(outputs[i].toString());

      OverlapStatistics oStats(from1, from2);

      if (oStats.HasOverlap()) {
        overlapped.insert(inputs[j].toString());
        overlapped.insert(inputs[i].toString());

        i1 = new Chip(oStats.Samples() + 2, oStats.Lines() + 2);
        int from1CenterSample =
            (oStats.StartSampleX() + oStats.EndSampleX()) / 2;
        int from1CenterLine =
            (oStats.StartLineX() + oStats.EndLineX()) / 2;
        i1->TackCube(from1CenterSample, from1CenterLine);
        i1->Load(from1);

        i2 = new Chip(oStats.Samples() + 2, oStats.Lines() + 2);
        int from2CenterSample =
            (oStats.StartSampleY() + oStats.EndSampleY()) / 2;
        int from2CenterLine =
            (oStats.StartLineY() + oStats.EndLineY()) / 2;
        i2->TackCube(from2CenterSample, from2CenterLine);
        i2->Load(from2);

        // compute the linear regression fit of the mvstats data
        int stop = ui.GetInteger("STOP");
        blendRamp = createRamp(i1, i2, stop);

        // Apply the correction
        s1 = oStats.StartSampleX() - 1;
        s2 = oStats.EndSampleX() + 1;
        l1 = oStats.StartLineX() - 1;
        l2 = oStats.EndLineX() + 1;

        line = 1;

        // We will be processing by line
        ProcessByLine p;
        CubeAttributeInput att;

        QString cubeName = outputs[i].toString();
        p.SetInputCube(cubeName, att, ReadWrite);

        p.StartProcess(blend);
        p.EndProcess();
        p.ClearInputCubes();

        s1 = oStats.StartSampleY() - 1;
        s2 = oStats.EndSampleY() + 1;
        l1 = oStats.StartLineY() - 1;
        l2 = oStats.EndLineY() + 1;

        line = 1;

        cubeName = outputs[j].toString();
        p.SetInputCube(cubeName, att, ReadWrite);

        p.StartProcess(blend);
        p.EndProcess();
        p.ClearInputCubes();

        delete blendRamp;
        blendRamp = NULL;

        delete i1;
        i1 = NULL;

        delete i2;
        i2 = NULL;
      }
    }
  }

  // Make sure cube projection overlaps at least one other cube
  if (ui.GetBoolean("ERROR")) {
    for (int i = 1; i < inputs.size(); i++) {
      if (!overlapped.contains(inputs[i].toString())) {
        std::string msg = "Input Cube [" + inputs[i].toString() +
            "] does not overlap another cube";
        throw IException(IException::User, msg, _FILEINFO_);
      }
    }
  }
}


void blend(Buffer &in) {
  for (int i = 0; i < in.size(); i++) {
    int sample = i + 1;
    int oSample = sample - s1 + 1;
    int oLine = line - l1 + 1;

    if (inIntersection(oSample, oLine) && validValue(oSample, oLine)) {
      double blendValue = blendRamp->GetValue(oSample, oLine);
      in[i] =
          i2->GetValue(oSample, oLine) * blendValue +
          i1->GetValue(oSample, oLine) * (1.0 - blendValue);
    }
    else {
      in[i] = in[i];
    }
  }

  line++;
}


bool inIntersection(int sample, int line) {
  int samples = blendRamp->Samples();
  int lines = blendRamp->Lines();
  return sample >= 1 && sample <= samples && line >= 1 && line <= lines;
}


bool validValue(int oSample, int oLine) {
  return blendRamp->GetValue(oSample, oLine) >= 0.0;
}


Chip * createRamp(Chip *pic1, Chip *pic2, int stop) {
  // x and y dimensions of the original pictures
  int x = pic1->Samples();
  int y = pic1->Lines();

  if (x != pic2->Samples() || y != pic2->Lines()) {
    std::string msg = "The two pictures need to be of the exact same dimensions";
    throw IException(IException::Programmer, msg, _FILEINFO_);
  }

  // Create the two overlap arrays
  QVector<int> ol1(x * y, -1);
  QVector<int> ol2(x * y, -1);

  // Lines and columns bounding data
  r1 = y - 1;
  r2 = 0;
  c1 = x - 1;
  c2 = 0;

  QQueue<Node> nodes1;
  QQueue<Node> nodes2;

  // Extract profiles of images
  int sum = 0;
  for (int i = 0; i < x; i++) {
    for (int j = 0; j < y; j++) {
      int t1 = j * x + i;

      double tempval = pic1->GetValue(i + 1, j + 1);
      if (IsValidPixel(tempval)) ol1[t1] = 0;

      tempval = pic2->GetValue(i + 1, j + 1);
      if (IsValidPixel(tempval)) ol2[t1] = 0;

      // What does this do?
      if (ol1[t1] == 0) sum += ol2[t1];

      // Find left and right limits of overlapping area
      if (ol1[t1] != -1 && ol2[t1] != -1) {
        if (j < r1) r1 = j;
        if (j > r2) r2 = j;
        if (i < c1) c1 = i;
        if (i > c2) c2 = i;
      }

      if (ol1[t1] == -1) nodes1.enqueue(Node(i, j, x, y, 0));
      if (ol2[t1] == -1) nodes2.enqueue(Node(i, j, x, y, 0));
    }
  }

  // ol1 and ol2 were being left zero (sum==0) causing the ramp to fail setting
  // the outer edges of both ol1 and ol2 to -1 fixed the problem.
  if (sum == 0) {
    for (int i = 0; i < x; i++) {
      ol1[x * y - (i + 1)] = -1;
      ol1[i] = -1;
    }
    for (int j = 1; j < y; j++) {
      ol1[x * j] = -1;
      ol1[x * j - 1] = -1;
    }
  }

  // Loop through the overlap arrays filling in the appropriate value.  On the
  // first iteration we search for any pixels with a neighbor of -1 and we set
  // that pixel to 1.  On all other iterations we look for neighbors with values
  // num-1. If "stop" number is specified, we stop searching for distances and
  // set all values to value of "stop".
  int maxScore = 0;
  processNodes(nodes1, ol1, maxScore, stop);
  if (sum != 0) processNodes(nodes2, ol2, maxScore, stop);

  // This is just to be consistent with the old, iterative way of creating the
  // ramp (without the queue).  Not sure if it's correct.
  if (maxScore < stop) maxScore++;

  // Loop through last time and create ramp using the special cases
  Chip *ramp = new Chip(x, y);
  ramp->SetAllValues(-1.0);
  for (int i = 0; i < x; i++) {
    for (int j = 0; j < y; j++) {
      int t1 = j * x + i;
      if (ol1[t1] != -1 && ol2[t1] != -1) {
        double value = (sum != 0) ? ((double) ol2[t1]) / (ol2[t1] + ol1[t1]) :
            1.0 - ol1[t1] / (2.0 * maxScore);
        ramp->SetValue(i + 1, j + 1, value);
      }
    }
  }

  return ramp;
}


void processNodes(QQueue<Node> &nodes, QVector<int> &ol,
    int &maxScore, int stop) {

  while (!nodes.empty()) {
    Node node = nodes.dequeue();

    int newScore = node.getNextScore(stop);
    if (newScore > maxScore) maxScore = newScore;

    node.evaluateUp(nodes, ol, stop);
    node.evaluateDown(nodes, ol, stop);
    node.evaluateLeft(nodes, ol, stop);
    node.evaluateRight(nodes, ol, stop);
  }
}


void readOutputs(QString outName, const FileList &inputs, FileList &outputs) {
  outputs.read(outName);

  // Make sure each file in the tolist matches a file in the fromlist
  if (outputs.size() != inputs.size()) {
    std::string msg = "There must be exactly one output image in the TOLIST for "
        "each input image in the FROMLIST";
    throw IException(IException::User, msg, _FILEINFO_);
  }

  // Make sure that every output file has a different filename from its
  // corresponding input file
  for (int i = 0; i < outputs.size(); i++) {
    if (outputs[i].toString().compare(inputs[i].toString()) == 0) {
      std::string msg = "The to list file [" + outputs[i].toString() +
          "] has the same name as its corresponding from list file.";
      throw IException(IException::User, msg, _FILEINFO_);
    }
  }
}


void generateOutputs(const FileList &inputs, FileList &outputs) {
  for (int i = 0; i < inputs.size(); i++) {
    FileName file=inputs[i];
    QString filename = file.path() + "/" + file.baseName() +
      ".blend." + file.extension();
    outputs.push_back(filename);
  }
}

