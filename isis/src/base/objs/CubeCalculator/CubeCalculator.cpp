/** This is free and unencumbered software released into the public domain.
The authors of ISIS do not claim copyright on the contents of this file.
For more details about the LICENSE terms and the AUTHORS, you will
find files of those names at the top level of this repository. **/

/* SPDX-License-Identifier: CC0-1.0 */
#include "CubeCalculator.h"

#include <QVector>

#include "Angle.h"
#include "Camera.h"
#include "Distance.h"
#include "IString.h"
#include "Statistics.h"

using namespace std;

namespace Isis {

  //! Constructs a CubeCalculator.
  CubeCalculator::CubeCalculator() {
    m_calculations    = NULL;
    m_methods         = NULL;
    m_dataDefinitions = NULL;
    m_cubeStats       = NULL;
    m_cubeCameras     = NULL;
    m_cameraBuffers   = NULL;

    m_calculations    = new QVector<Calculations>();
    m_methods         = new QVector<void (Calculator:: *)(void)>();
    m_dataDefinitions = new QVector<DataValue>();
    m_cubeStats       = new QVector<Statistics *>();
    m_cubeCameras     = new QVector<Camera *>();
    m_cameraBuffers   = new QVector<CameraBuffers *>();

    m_outputSamples = 0;
  }

  
  //! Destroys the CubeCalculator object.
  CubeCalculator::~CubeCalculator() {
    Clear(); // free dynamic memory in container members

    delete m_calculations;
    delete m_methods;
    delete m_dataDefinitions;
    delete m_cubeStats;
    delete m_cubeCameras;
    delete m_cameraBuffers;
    
    m_calculations = NULL;
    m_methods = NULL;
    m_dataDefinitions = NULL;
    m_cubeStats = NULL;
    m_cubeCameras = NULL;
    m_cameraBuffers = NULL;
  }
  
  
  /**
   * Frees dynamic memory in container members.
   */
  void CubeCalculator::Clear() {
    Calculator::Clear();

    if (m_calculations) {
      m_calculations->clear();
    }

    if (m_methods) {
      m_methods->clear();
    }

    if (m_dataDefinitions) {
      m_dataDefinitions->clear();
    }

    // m_cubeStats contains pointers to dynamic memory - need to free
    if (m_cubeStats) {
      for (int i = 0; i < m_cubeStats->size(); i++) {
        delete (*m_cubeStats)[i];
        (*m_cubeStats)[i] = NULL;
      }
      m_cubeStats->clear();
    }

    if (m_cubeCameras) {
      m_cubeCameras->clear();
    }

    // m_cameraBuffers contains pointers to dynamic memory - need to free
    if (m_cameraBuffers) {
      for (int i = 0; i < m_cameraBuffers->size(); i++) {
        delete (*m_cameraBuffers)[i];
        (*m_cameraBuffers)[i] = NULL;
      }
      m_cameraBuffers->clear();
    }
  }

  
  /**
   * This method will execute the calculations built up when PrepareCalculations was called.
   *
   * @param cubeData The input cubes' data
   * @param curLine The current line in the output cube
   * @param curBand The current band in the output cube
   *
   * @return std::vector<double> The results of the calculations (with Isis Special Pixels)
   *
   * @throws IException::Unknown "Too many operands in the equation."
   */
  QVector<double> CubeCalculator::runCalculations(QVector<Buffer *> &cubeData,
                                                  int curLine, 
                                                  int curBand) {
    // For now we'll only process a single line in this method for our results. In order
    //    to do more powerful indexing, passing a list of cubes and the output cube will
    //    be necessary.
    int methodIndex = 0;
    int dataIndex = 0;

    for (int currentCalculation = 0; currentCalculation < m_calculations->size();
        currentCalculation++) {
      if ((*m_calculations)[currentCalculation] == CallNextMethod) {
        void (Calculator::*aMethod)() = (*m_methods)[methodIndex];
        (this->*aMethod)();
        methodIndex ++;
      }
      else {
        DataValue &data = (*m_dataDefinitions)[dataIndex];
        if (data.type() == DataValue::Constant) {
          Push(data.constant());
        }
        else if (data.type() == DataValue::Band) {
          Push(curBand);
        }
        else if (data.type() == DataValue::Line) {
          Push(curLine);
        }
        else if (data.type() == DataValue::Sample) {
          QVector<double> samples;
          samples.resize(m_outputSamples);

          for (int i = 0; i < m_outputSamples; i++) {
            samples[i] = i + 1;
          }

          Push(samples);
        }
        else if (data.type() == DataValue::CubeData) {
          Push(*cubeData[data.cubeIndex()]);
        }
        else if (data.type() == DataValue::InaData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->inaBuffer(curLine,
                                                                 m_outputSamples,
                                                                 curBand)));
        }
        else if (data.type() == DataValue::EmaData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->emaBuffer(curLine,
                                                                 m_outputSamples,
                                                                 curBand)));
        }
        else if (data.type() == DataValue::PhaData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->phaBuffer(curLine,
                                                                 m_outputSamples,
                                                                 curBand)));
        }
        else if (data.type() == DataValue::InalData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->inalBuffer(curLine,
                                                                  m_outputSamples,
                                                                  curBand)));
        }
        else if (data.type() == DataValue::EmalData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->emalBuffer(curLine,
                                                                  m_outputSamples,
                                                                  curBand)));
        }
        else if (data.type() == DataValue::PhalData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->phalBuffer(curLine,
                                                                  m_outputSamples,
                                                                  curBand)));
        }
        else if (data.type() == DataValue::LatData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->latBuffer(curLine,
                                                                 m_outputSamples,
                                                                 curBand)));
        }
        else if (data.type() == DataValue::LonData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->lonBuffer(curLine,
                                                                 m_outputSamples,
                                                                 curBand)));
        }
        else if (data.type() == DataValue::ResData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->resBuffer(curLine,
                                                                 m_outputSamples,
                                                                 curBand)));
        }
        else if (data.type() == DataValue::RadiusData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->radiusBuffer(curLine,
                                                                    m_outputSamples,
                                                                    curBand)));
        }
        else if (data.type() == DataValue::InacData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->inacBuffer(curLine,
                                                                  m_outputSamples,
                                                                  curBand)));
        }
        else if (data.type() == DataValue::EmacData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->emacBuffer(curLine,
                                                                  m_outputSamples,
                                                                  curBand)));
        }
        else if (data.type() == DataValue::PhacData) {
          Push(*((*m_cameraBuffers)[data.cubeIndex()]->phacBuffer(curLine,
                                                                  m_outputSamples,
                                                                  curBand)));
        }
        else {
        }

        dataIndex ++;
      }
    }

    if (StackSize() != 1) {
      string msg = "Too many operands in the equation.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    return Pop(true);
  }


  /**
   * This method builds a list of actions to perform based on the postfix expression.
   *   Error checking is done using the inCubeInfos, and the outCubeInfo is necessary
   *   to tell the dimensions of the output cube. Call this method before calling
   *   RunCalculations(). This method will also erase all calculator history before building
   *   up a new set of calculations to run.
   *
   * @param equation The equation in postfix notation
   * @param inCubes The input cubes
   * @param outCube The output cube
   *
   * @throws IException::Unknown "Invalid file number"
   * @throws IException::Unknown "Unidentified operator"
   */
  void CubeCalculator::prepareCalculations(QString equation,
                                           QVector<Cube *> &inCubes,
                                           Cube *outCube) {
    Clear();

    m_outputSamples = outCube->sampleCount();

    IString eq = equation.toStdString();
    while (eq != "") {
      IString token = eq.Token(" ");

      // Step through every part of the postfix equation and set up the appropriate
      // action list based on the current token. Attempting to order if-else conditions
      // in terms of what would probably be encountered more often.

      // Scalars
      if (isdigit(token[0]) || token[0] == '.') {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::Constant,
                                               token.ToDouble()));
      }
      // File, e.g. F1 = first file in list. Must come after any functions starting with 'f' that
      //   is not a cube.
      else if (token[0] == 'f') {
        IString tok(token.substr(1));
        int file = tok.ToInteger() - 1;
        if (file < 0 || file >= (int)inCubes.size()) {
          QString msg = "Invalid file number [" + QString::fromStdString(tok) + "]";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::CubeData, file));
      }
      else if (token == "band") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::Band));
      }
      else if (token == "line") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::Line));
      }
      else if (token == "sample") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::Sample));
      }
      // Addition
      else if (token == "+") {
        addMethodCall(&Calculator::Add);
      }

      // Subtraction
      else if (token == "-") {
        addMethodCall(&Calculator::Subtract);
      }

      // Multiplication
      else if (token == "*") {
        addMethodCall(&Calculator::Multiply);
      }

      // Division
      else if (token == "/") {
        addMethodCall(&Calculator::Divide);
      }

      // Modulus
      else if (token == "%") {
        addMethodCall(&Calculator::Modulus);
      }

      // Exponent
      else if (token == "^") {
        addMethodCall(&Calculator::Exponent);
      }

      // Negative
      else if (token == "--") {
        addMethodCall(&Calculator::Negative);
      }

      // Negative
      else if (token == "neg") {
        addMethodCall(&Calculator::Negative);
      }

      // Left shift
      else if (token == "<<") {
        addMethodCall(&Calculator::LeftShift);
      }

      // Right shift
      else if (token == ">>") {
        addMethodCall(&Calculator::RightShift);
      }

      // Maximum In The Line
      else if (token == "linemax") {
        addMethodCall(&Calculator::MaximumLine);
      }

      // Maximum Pixel on a per-pixel basis
      else if (token == "max") {
        addMethodCall(&Calculator::MaximumPixel);
      }

      // Minimum In The Line
      else if (token == "linemin") {
        addMethodCall(&Calculator::MinimumLine);
      }

      // Minimum Pixel on a per-pixel basis
      else if (token == "min") {
        addMethodCall(&Calculator::MinimumPixel);
      }

      // Absolute value
      else if (token == "abs") {
        addMethodCall(&Calculator::AbsoluteValue);
      }

      // Square root
      else if (token == "sqrt") {
        addMethodCall(&Calculator::SquareRoot);
      }

      // Natural Log
      else if (token == "log" || token == "ln") {
        addMethodCall(&Calculator::Log);
      }

      // Log base 10
      else if (token == "log10") {
        addMethodCall(&Calculator::Log10);
      }

      // Pi
      else if (token == "pi") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, PI)
        );
      }

      // e
      else if (token == "e") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, E)
        );
      }

      else if (token == "rads") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, PI / 180.0)
        );

        addMethodCall(&Calculator::Multiply);
      }

      else if (token == "degs") {
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, 180.0 / PI)
        );
        addMethodCall(&Calculator::Multiply);
      }

      // Sine
      else if (token == "sin") {
        addMethodCall(&Calculator::Sine);
      }

      // Cosine
      else if (token == "cos") {
        addMethodCall(&Calculator::Cosine);
      }

      // Tangent
      else if (token == "tan") {
        addMethodCall(&Calculator::Tangent);
      }

      // Secant
      else if (token == "sec") {
        addMethodCall(&Calculator::Secant);
      }

      // Cosecant
      else if (token == "csc") {
        addMethodCall(&Calculator::Cosecant);
      }

      // Cotangent
      else if (token == "cot") {
        addMethodCall(&Calculator::Cotangent);
      }

      // Arcsin
      else if (token == "asin") {
        addMethodCall(&Calculator::Arcsine);
      }

      // Arccos
      else if (token == "acos") {
        addMethodCall(&Calculator::Arccosine);
      }

      // Arctan
      else if (token == "atan") {
        addMethodCall(&Calculator::Arctangent);
      }

      // Arctan2
      else if (token == "atan2") {
        addMethodCall(&Calculator::Arctangent2);
      }

      // SineH
      else if (token == "sinh") {
        addMethodCall(&Calculator::SineH);
      }

      // CosH
      else if (token == "cosh") {
        addMethodCall(&Calculator::CosineH);
      }

      // TanH
      else if (token == "tanh") {
        addMethodCall(&Calculator::TangentH);
      }

      // Less than
      else if (token == "<") {
        addMethodCall(&Calculator::LessThan);
      }

      // Greater than
      else if (token == ">") {
        addMethodCall(&Calculator::GreaterThan);
      }

      // Less than or equal
      else if (token == "<=") {
        addMethodCall(&Calculator::LessThanOrEqual);
      }

      // Greater than or equal
      else if (token == ">=") {
        addMethodCall(&Calculator::GreaterThanOrEqual);
      }

      // Equal
      else if (token == "==") {
        addMethodCall(&Calculator::Equal);
      }

      // Not equal
      else if (token == "!=") {
        addMethodCall(&Calculator::NotEqual);
      }

      // Maximum in a cube
      else if (token == "cubemax") {
        int cubeIndex = lastPushToCubeStats(inCubes);

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, (*m_cubeStats)[cubeIndex]->Maximum())
        );
        //TODO: Test for NULL Maximum
      }

      // Maximum in a cube
      else if (token == "cubemin") {
        int cubeIndex = lastPushToCubeStats(inCubes);

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, (*m_cubeStats)[cubeIndex]->Minimum())
        );
        //TODO: Test for NULL Minimum
      }

      // Average of a cube
      else if (token == "cubeavg") {
        int cubeIndex = lastPushToCubeStats(inCubes);

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, (*m_cubeStats)[cubeIndex]->Average())
        );
        //TODO: Test for NULL Average
      }

      // Standard deviation of a cube
      else if (token == "cubestd") {
        int cubeIndex = lastPushToCubeStats(inCubes);

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(
          DataValue(DataValue::Constant, (*m_cubeStats)[cubeIndex]->StandardDeviation())
        );
        //TODO: Test for NULL standard deviation
      }

      // Center incidence
      else if (token == "inac") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableInacBuffer();
        
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::InacData, cubeIndex));
      }

      // Center emission
      else if (token == "emac") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableEmacBuffer();
        
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::EmacData, cubeIndex));
      }

      // Center phase
      else if (token == "phac") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enablePhacBuffer();
        
        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::PhacData, cubeIndex));
      }

      // Incidence on the ellipsoid
      else if (token == "ina") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableInaBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::InaData, cubeIndex));
      }

      // Emission on the ellipsoid
      else if (token == "ema") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableEmaBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::EmaData, cubeIndex));
      }

      // Phase on the ellipsoid
      else if (token == "pha") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enablePhaBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::PhaData, cubeIndex));
      }

      // Incidence on the DTM
      else if (token == "inal") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableInalBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::InalData, cubeIndex));
      }

      // Emission on the DTM
      else if (token == "emal") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableEmalBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::EmalData, cubeIndex));
      }

      // Phase on the ellipsoid
      else if (token == "phal") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enablePhalBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::PhalData, cubeIndex));
      }

      // Latitude
      else if (token == "lat") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableLatBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::LatData, cubeIndex));
      }

      // Longitude
      else if (token == "lon") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableLonBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::LonData, cubeIndex));
      }

      // Pixel resolution
      else if (token == "res") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableResBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::ResData, cubeIndex));
      }

      // Local Radius
      else if (token == "radius") {
        int cubeIndex = lastPushToCubeCameras(inCubes);
        (*m_cameraBuffers)[cubeIndex]->enableRadiusBuffer();

        m_calculations->push_back(PushNextData);
        m_dataDefinitions->push_back(DataValue(DataValue::RadiusData, cubeIndex));
      }

      // Ignore empty token
      else if (token == "") {
      }

      else {
        string msg = "Unidentified operator [";
        msg += token + "]";
        throw IException(IException::Unknown, msg, _FILEINFO_);
      }
    } // while loop
  }


  /**
   * Creates statistics internally for the last cube data pushed to the data definitions.
   *
   * This method is used for setting up internal statistics for the input cubes. This allows 
   * access to the statistics for each of the input cube when adding statistics related values
   * to the calculator.
   *
   * @throws IException::Unknown "This function must not contain constants, only input cubes may
   *                              be specified"
   *
   * @return @b int Returns the index of the last pushed cube data.
   */
  int CubeCalculator::lastPushToCubeStats(QVector<Cube *> &inCubes) {
    if (!m_calculations->size()) {
      string msg = "Not sure which file to get statistics from";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if ((*m_calculations)[m_calculations->size() - 1] != PushNextData) {
      string msg = "This function must not contain calculations,";
      msg += " only input cubes may be specified.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    m_calculations->pop_back();

    // This must have data if calculations had data that equaled push data
    DataValue lastData = (*m_dataDefinitions)[m_dataDefinitions->size() - 1];

    if (lastData.type() != DataValue::CubeData) {
      string msg = "This function must not contain constants,";
      msg += " only input cubes may be specified.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    int cubeStatsIndex = lastData.cubeIndex();
    m_dataDefinitions->pop_back();

    // Member variables are now cleaned up, we need to verify the stats exists

    // Make sure room exists in the vector
    while (m_cubeStats->size() < cubeStatsIndex + 1) {
      m_cubeStats->push_back(NULL);
    }

    // Now we can for sure put the stats object in the right place... put it
    //   there
    if ((*m_cubeStats)[cubeStatsIndex] == NULL) {
      (*m_cubeStats)[cubeStatsIndex] = inCubes[cubeStatsIndex]->statistics();
    }

    return cubeStatsIndex;
  }


  /**
   * Creates internal camera for the last pushed cube data.
   *
   * This method is used to set up cameras for the input cubes.
   *
   * @throws IException::Unknown "Not sure which files to get cameras from"
   * @throws IException::Unknown "This function must not contain calcuations, only input cubes may
   *                              be specified."
   * @throws IException::Unknown "This function must not contain constants, only input cubes may
   *                              be specified."
   * @throws IException::Unknown "This function requires a camera and the input cube does not have
   *                              one. You may need to run spiceinit"
   * @throws IException::Unknown "This function requires a camera and the input cube does not have
   *                              one. You may need to run spiceinit"
   *
   * @return @b int Returns the index of the last pushed cube data.
   */
  int CubeCalculator::lastPushToCubeCameras(QVector<Cube *> &inCubes) {
    if (!m_calculations->size()) {
      string msg = "Not sure which file to get cameras from";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    if ((*m_calculations)[m_calculations->size() - 1] != PushNextData) {
      string msg = "This function must not contain calculations,";
      msg += " only input cubes may be specified.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    m_calculations->pop_back();

    // This must have data if calculations had data that equaled push data
    DataValue lastData = (*m_dataDefinitions)[m_dataDefinitions->size() - 1];

    if (lastData.type() != DataValue::CubeData) {
      string msg = "This function must not contain constants,";
      msg += " only input cubes may be specified.";
      throw IException(IException::Unknown, msg, _FILEINFO_);
    }

    int cubeIndex = lastData.cubeIndex();
    m_dataDefinitions->pop_back();

    // Member variables are now cleaned up, we need to verify the camera exists

    // Make sure room exists in the vector
    while (m_cubeCameras->size() < cubeIndex + 1) {
      m_cubeCameras->push_back(NULL);
    }

    while (m_cameraBuffers->size() < cubeIndex + 1) {
      m_cameraBuffers->push_back(NULL);
    }

    // Now we can for sure put the camera object in the right place... put it
    //   there
    if ((*m_cubeCameras)[cubeIndex] == NULL) {
      Camera *cam;
      try {
        cam = inCubes[cubeIndex]->camera();
        if (cam == NULL) {
          string msg = "This function requires a camera and the input cube does";
          msg += " not have one. You may need to run spiceinit";
          throw IException(IException::Unknown, msg, _FILEINFO_);
        }
      }
      catch (IException &e) {
        string msg = "This function requires a camera and the input cube does";
        msg += " not have one. You may need to run spiceinit";
        throw IException(e, IException::Unknown, msg, _FILEINFO_);
      }

      (*m_cubeCameras)[cubeIndex] = cam;
      (*m_cameraBuffers)[cubeIndex] = new CameraBuffers(cam);
    }

    return cubeIndex;
  }


  /**
   * This is a conveinience method for PrepareCalculations(...).
   * This will cause RunCalculations(...) to execute this method in order.
   *
   * @param method The method to call, i.e. &Calculator::Multiply
   */
  void CubeCalculator::addMethodCall(void (Calculator::*method)(void)) {
    m_calculations->push_back(CallNextMethod);
    m_methods->push_back(method);
  }


  /**
   * Constructs a default DataValue. 
   *
   * This method constructs a default DataValue with its type, cube index, and constant value
   * set to -1, -1, and 0.0, respectively.
   */
  DataValue::DataValue() {
    m_type = (DataValueType) - 1;
    m_cubeIndex = -1;
    m_constantValue = 0.0;
  }


  /**
   * Constructs a DataValue with a given type.
   *
   * This method constructs a DataValue with the passed type. The DataValue's constant value
   * is set to 0.0 and its cube index is set to -1.
   *
   * @param type The DataValueType for the DataValue.
   */
  DataValue::DataValue(DataValueType type) {
    m_type = type;
    m_constantValue = 0.0;
    m_cubeIndex = -1;
  }


  /**
   * Constructs a DataValue with a given type and associated cube index.
   *
   * This method constructs a DataValue with the passed type and a cube index corresponding to 
   * the data's associated cube. The DataValue's constant value is set to 0.0.
   *
   * @param type The DataValueType for the DataValue.
   * @param cubeIndex Index of the associated cube the data corresponds with.
   */
  DataValue::DataValue(DataValueType type, int cubeIndex) {
    m_type = type;
    m_constantValue = 0.0;
    m_cubeIndex = cubeIndex;
  }


  /**
   * Constructs a DataValue with a given type and constant value.
   *
   * This method constructs a DataValue with the passed type and passed constant value.
   * The DataValue's cube index is set to -1. If the type passed is DataValue::Constant,
   * then its constant value is set to the passed value.
   *
   * @param type The DataValueType for the DataValue.
   * @param value The constant value for the DataValue.
   */
  DataValue::DataValue(DataValueType type, double value) {
    m_type = type;
    m_cubeIndex = -1;

    if (type == Constant) {
      m_constantValue = value;
    }
  }


  /**
   * Accesses the type of the DataValue.
   *
   * @return @b DataValue::DataValueType Returns the DataValue's type.
   */
  DataValue::DataValueType DataValue::type() {
    return m_type;
  }


  /**
   * Accesses the cube index of the DataValue.
   *
   * @return @b int Returns the DataValue's cube index.
   */
  int DataValue::cubeIndex() {
    return m_cubeIndex;
  }


  /**
   * Accesses the constant value of the DataValue.
   *
   * @return @b double Returns the DataValue's constant value.
   */
  double DataValue::constant() {
    return m_constantValue;
  }


  /**
   * Constructs a CameraBuffers object.
   *
   * @param camera Pointer to the Camera the CameraBuffers will use.
   */
  CameraBuffers::CameraBuffers(Camera *camera) {
    m_camera = camera;
    m_phaBuffer  = NULL;
    m_inaBuffer  = NULL;
    m_emaBuffer  = NULL;
    m_phalBuffer = NULL;
    m_inalBuffer = NULL;
    m_emalBuffer = NULL;
    m_phacBuffer = NULL;
    m_inacBuffer = NULL;
    m_emacBuffer = NULL;
    m_resBuffer  = NULL;
    m_latBuffer  = NULL;
    m_lonBuffer  = NULL;
    m_radiusBuffer = NULL;

    m_lastLine = -1;
  }


  /**
   * Destroys the CameraBuffers.
   */
  CameraBuffers::~CameraBuffers() {
    delete m_phaBuffer;
    delete m_inaBuffer;
    delete m_emaBuffer;
    delete m_phalBuffer;
    delete m_inalBuffer;
    delete m_emalBuffer;
    delete m_phacBuffer;
    delete m_inacBuffer;
    delete m_emacBuffer;
    delete m_resBuffer;
    delete m_latBuffer;
    delete m_lonBuffer;
    delete m_radiusBuffer;

    m_phaBuffer  = NULL;
    m_inaBuffer  = NULL;
    m_emaBuffer  = NULL;
    m_phalBuffer = NULL;
    m_inalBuffer = NULL;
    m_emalBuffer = NULL;
    m_phacBuffer = NULL;
    m_inacBuffer = NULL;
    m_emacBuffer = NULL;
    m_resBuffer  = NULL;
    m_latBuffer  = NULL;
    m_lonBuffer  = NULL;
    m_radiusBuffer = NULL;
  }


  //! Enables the phase angle buffer for use.
  void CameraBuffers::enablePhaBuffer() {
    if (!m_phaBuffer) m_phaBuffer = new QVector<double>;
  }


  //! Enables the incidence angle buffer for use.
  void CameraBuffers::enableInaBuffer() {
    if (!m_inaBuffer) m_inaBuffer = new QVector<double>;
  }


  //! Enables the emission angle buffer for use.
  void CameraBuffers::enableEmaBuffer() {
    if (!m_emaBuffer) m_emaBuffer = new QVector<double>;
  }


  //! Enables the latitude buffer for use.
  void CameraBuffers::enableLatBuffer() {
    if (!m_latBuffer) m_latBuffer = new QVector<double>;
  }


  //! Enables the longitude buffer for use.
  void CameraBuffers::enableLonBuffer() {
    if (!m_lonBuffer) m_lonBuffer = new QVector<double>;
  }


  //! Enables the resolution buffer for use.
  void CameraBuffers::enableResBuffer() {
    if (!m_resBuffer) m_resBuffer = new QVector<double>;
  }


  //! Enables the radius buffer for use.
  void CameraBuffers::enableRadiusBuffer() {
    if (!m_radiusBuffer) m_radiusBuffer = new QVector<double>;
  }


  //! Enables the local phase angle buffer for use.
  void CameraBuffers::enablePhalBuffer() {
    if (!m_phalBuffer) m_phalBuffer = new QVector<double>;
  }


  //! Enables the local incidence angle buffer for use.
  void CameraBuffers::enableInalBuffer() {
    if (!m_inalBuffer) m_inalBuffer = new QVector<double>;
  }


  //! Enables the local emission angle buffer for use.
  void CameraBuffers::enableEmalBuffer() {
    if (!m_emalBuffer) m_emalBuffer = new QVector<double>;
  }


  //! Enables the center phase angle buffer for use.
  void CameraBuffers::enablePhacBuffer() {
    if (!m_phacBuffer) m_phacBuffer = new QVector<double>;
  }


  //! Enables the center incidence angle buffer for use.
  void CameraBuffers::enableInacBuffer() {
    if (!m_inacBuffer) m_inacBuffer = new QVector<double>;
  }


  //! Enables the center emission angle buffer for use.
  void CameraBuffers::enableEmacBuffer() {
    if (!m_emacBuffer) m_emacBuffer = new QVector<double>;
  }


  QVector<double> *CameraBuffers::phaBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_phaBuffer;
  }


  QVector<double> *CameraBuffers::inaBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_inaBuffer;
  }


  QVector<double> *CameraBuffers::emaBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_emaBuffer;
  }


  QVector<double> *CameraBuffers::latBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_latBuffer;
  }


  QVector<double> *CameraBuffers::lonBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_lonBuffer;
  }


  QVector<double> *CameraBuffers::resBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_resBuffer;
  }


  QVector<double> *CameraBuffers::radiusBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine,ns,currentBand);
    return m_radiusBuffer;
  }


  QVector<double> *CameraBuffers::phalBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_phalBuffer;
  }


  QVector<double> *CameraBuffers::inalBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_inalBuffer;
  }


  QVector<double> *CameraBuffers::emalBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_emalBuffer;
  }


  QVector<double> *CameraBuffers::phacBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_phacBuffer;
  }


  QVector<double> *CameraBuffers::inacBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_inacBuffer;
  }


  QVector<double> *CameraBuffers::emacBuffer(int currentLine, int ns, int currentBand) {
    loadBuffers(currentLine, ns, currentBand);
    return m_emacBuffer;
  }


  void CameraBuffers::loadBuffers(int currentLine, int ns, int currentBand) {
    if (currentLine != m_lastLine) {
      m_lastLine = currentLine;

      // Resize buffers if necessary
      if (m_phaBuffer) m_phaBuffer->resize(ns);
      if (m_inaBuffer) m_inaBuffer->resize(ns);
      if (m_emaBuffer) m_emaBuffer->resize(ns);
      if (m_latBuffer) m_latBuffer->resize(ns);
      if (m_lonBuffer) m_lonBuffer->resize(ns);
      if (m_resBuffer) m_resBuffer->resize(ns);
      if (m_radiusBuffer) m_radiusBuffer->resize(ns);
      if (m_phalBuffer) m_phalBuffer->resize(ns);
      if (m_inalBuffer) m_inalBuffer->resize(ns);
      if (m_emalBuffer) m_emalBuffer->resize(ns);

      // Center angle buffers will only ever have one item, the center angle value
      if (m_phacBuffer) m_phacBuffer->resize(1);
      if (m_inacBuffer) m_inacBuffer->resize(1);
      if (m_emacBuffer) m_emacBuffer->resize(1);

      m_camera->SetBand(currentBand);

      if (m_phacBuffer || m_inacBuffer || m_emacBuffer) {
          QString tokenName; // used for exception
          double centerLine = m_camera->Lines() / 2.0 + 0.5;
          double centerSamp = m_camera->Samples() / 2.0 + 0.5;

          if (m_camera->SetImage(centerSamp, centerLine)) {
            if (m_phacBuffer) {
              tokenName = "phac";
              (*m_phacBuffer)[0] = m_camera->PhaseAngle();
            }
            if (m_inacBuffer) {
              tokenName = "inac";
              (*m_inacBuffer)[0] = m_camera->IncidenceAngle();
            }
            if (m_emacBuffer) {
              tokenName = "emac";
              (*m_emacBuffer)[0] = m_camera->EmissionAngle();
            }
          }
          else {
            QString msg = "Unable to compute illumination angles at image center for operator ["
                          + tokenName + "].";
            throw IException(IException::Unknown, msg, _FILEINFO_);
          }
      }

      else {
        for (int i = 0; i < ns; i++) {
        
          if (m_camera->SetImage(i + 1, currentLine)) {
            if (m_phaBuffer) (*m_phaBuffer)[i] = m_camera->PhaseAngle();
            if (m_inaBuffer) (*m_inaBuffer)[i] = m_camera->IncidenceAngle();
            if (m_emaBuffer) (*m_emaBuffer)[i] = m_camera->EmissionAngle();
            if (m_latBuffer) (*m_latBuffer)[i] = m_camera->UniversalLatitude();
            if (m_lonBuffer) (*m_lonBuffer)[i] = m_camera->UniversalLongitude();
            if (m_resBuffer) (*m_resBuffer)[i] = m_camera->PixelResolution();
            if (m_radiusBuffer) (*m_radiusBuffer)[i] = m_camera->LocalRadius().meters();
            if (m_phalBuffer || m_inalBuffer || m_emalBuffer) {
              Angle phal, inal, emal;
              bool okay;
              m_camera->LocalPhotometricAngles(phal, inal, emal, okay);
              if (okay) {
                if (m_phalBuffer) (*m_phalBuffer)[i] = phal.degrees();
                if (m_inalBuffer) (*m_inalBuffer)[i] = inal.degrees();
                if (m_emalBuffer) (*m_emalBuffer)[i] = emal.degrees();
              }
              else {
                if (m_phalBuffer) (*m_phalBuffer)[i] = NAN;
                if (m_inalBuffer) (*m_inalBuffer)[i] = NAN;
                if (m_emalBuffer) (*m_emalBuffer)[i] = NAN;
              }
            }
          }
          else {
            if (m_phaBuffer) (*m_phaBuffer)[i] = NAN;
            if (m_inaBuffer) (*m_inaBuffer)[i] = NAN;
            if (m_emaBuffer) (*m_emaBuffer)[i] = NAN;
            if (m_latBuffer) (*m_latBuffer)[i] = NAN;
            if (m_lonBuffer) (*m_lonBuffer)[i] = NAN;
            if (m_resBuffer) (*m_resBuffer)[i] = NAN;
            if (m_radiusBuffer) (*m_radiusBuffer)[i] = NAN;
            if (m_phalBuffer) (*m_phalBuffer)[i] = NAN;
            if (m_inalBuffer) (*m_inalBuffer)[i] = NAN;
            if (m_emalBuffer) (*m_emalBuffer)[i] = NAN;
          }
        }
      }
    }
  }

} // End of namespace Isis

