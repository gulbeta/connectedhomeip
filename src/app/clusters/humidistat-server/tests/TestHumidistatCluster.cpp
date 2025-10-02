/*
 *    Copyright (c) 2025 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <pw_unit_test/framework.h>

#include <app/clusters/humidistat-server/humidistat-server.h>

#include <app/clusters/testing/AttributeTesting.h>
#include <app/server-cluster/AttributeListBuilder.h>
#include <app/server-cluster/DefaultServerCluster.h>
#include <clusters/Humidistat/Attributes.h>
#include <clusters/Humidistat/Metadata.h>

using namespace chip;
using namespace chip::app;
using namespace chip::app::Clusters;
using namespace chip::app::Clusters::Humidistat;
using namespace chip::app::Clusters::Humidistat::Attributes;

namespace {

struct TestHumidistatCluster : public ::testing::Test
{
    static void SetUpTestSuite() { ASSERT_EQ(chip::Platform::MemoryInit(), CHIP_NO_ERROR); }
    static void TearDownTestSuite() { chip::Platform::MemoryShutdown(); }
};

TEST_F(TestHumidistatCluster, CompileTest)
{
    // Cannot instantiate HumidistatCluster (abstract). Just validate constants/types exist.
    EXPECT_NE(Humidistat::Id, static_cast<ClusterId>(0));
    // Optionally ensure an attribute id compiles:
    EXPECT_NE(Attributes::AttributeList::Id, static_cast<AttributeId>(0));
}

} // namespace