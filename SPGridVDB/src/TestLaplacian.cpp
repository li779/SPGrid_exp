#include <gtest/gtest.h>

#include <openvdb/openvdb.h>
#include <openvdb/tools/LevelSetSphere.h>
#include <openvdb/tools/LevelSetUtil.h>
#include <openvdb/tools/GridOperators.h>
#include <openvdb/util/CpuTimer.h>

#include "SPGridAllocator.h"

namespace {

// The fixture for testing class.
class TestLaplacian : public ::testing::Test {
 protected:

  TestLaplacian() {}

  ~TestLaplacian() override { }

  void SetUp() override {
    openvdb::initialize();
    const float radius = 250.0f, voxelSize = 1.0f, width = 3.0f;
    const openvdb::Vec3f center(0.0f);
    mSphere = openvdb::tools::createLevelSetSphere<openvdb::FloatGrid>(radius, center, voxelSize, width); 
    openvdb::tools::sdfToFogVolume(*mSphere);
    mSphere->tree().voxelizeActiveTiles();
  }

  void TearDown() override {
  }
  openvdb::FloatGrid::Ptr mSphere;
};

TEST_F(TestLaplacian, OpenVDB) 
{
  //mSphere->print(std::cerr, 3);
  const auto voxelCount = mSphere->activeVoxelCount();

  allocateWithSPGrid<openvdb::FloatGrid::TreeType>(mSphere->tree());

  openvdb::util::CpuTimer timer("OpenVDB");
  auto grid = openvdb::tools::laplacian( *mSphere );
  timer.stop();

  //grid->print(std::cerr, 3);

  EXPECT_EQ( voxelCount,  grid->activeVoxelCount());
}

}  // namespace

int main(int argc, char **argv) 
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}