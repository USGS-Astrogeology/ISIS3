#include <iostream>
#include <QTemporaryFile>


#include "Cube.h"
#include "CubeAttribute.h"
#include "IException.h"
#include "PixelType.h"
#include "Pvl.h"
#include "PvlGroup.h"
#include "PvlKeyword.h"
#include "TestUtilities.h"
#include "FileName.h"
#include "Camera.h"
#include "Fixtures.h"

using namespace Isis;

TEST_F(DemCube, CameraObliqueResolution) {

    Camera *c = NULL;
    c = testCube->camera();

    c->SetImage(1, 1);
    EXPECT_NEAR(c->ObliqueDetectorResolution(), 19.2435, 1e-4);
    EXPECT_NEAR(c->ObliqueDetectorResolution(false), 19.2435, 1e-4);

    c->SetImage(1, 1055);
    EXPECT_NEAR(c->ObliqueDetectorResolution(), 19.7497, 1e-4);
    EXPECT_NEAR(c->ObliqueDetectorResolution(false), 19.7497, 1e-4);

    c->SetImage(1203, 1055);
    EXPECT_NEAR(c->ObliqueDetectorResolution(), 19.8820, 1e-4);
    EXPECT_NEAR(c->ObliqueDetectorResolution(false), 19.8820, 1e-4);

    c->SetImage(1203, 1);
    EXPECT_NEAR(c->ObliqueDetectorResolution(), 19.3449, 1e-4);
    EXPECT_NEAR(c->ObliqueDetectorResolution(false), 19.3449, 1e-4);
}
