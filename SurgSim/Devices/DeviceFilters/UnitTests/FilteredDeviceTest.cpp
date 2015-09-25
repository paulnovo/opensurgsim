// This file is a part of the OpenSurgSim project.
// Copyright 2013, SimQuest Solutions Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/// \file
/// Tests for the FilteredDevice class.

#include <memory>
#include <string>
#include <gtest/gtest.h>
#include "SurgSim/DataStructures/DataGroup.h"
#include "SurgSim/DataStructures/DataGroupBuilder.h"
#include "SurgSim/Devices/Devices.h"
#include "SurgSim/Framework/Runtime.h"
#include "SurgSim/Testing/MockInputOutput.h"

using SurgSim::DataStructures::DataGroup;
using SurgSim::DataStructures::DataGroupBuilder;
using SurgSim::Devices::FilteredDevice;
using SurgSim::Testing::MockInputOutput;

TEST(FilteredDeviceTest, ByHand)
{
	auto filteredDevice = std::make_shared<FilteredDevice>("device");
	ASSERT_FALSE(filteredDevice->initialize());

	ASSERT_ANY_THROW(filteredDevice->setDevice(nullptr));
	ASSERT_NO_THROW(filteredDevice->setDevice(std::make_shared<SurgSim::Devices::IdentityPoseDevice>("identity")));

	filteredDevice->addFilter(std::make_shared<SurgSim::Devices::PoseTransform>("filter1"));
	filteredDevice->addFilter(std::make_shared<SurgSim::Devices::PoseTransform>("filter2"));
	EXPECT_TRUE(filteredDevice->initialize());
	EXPECT_ANY_THROW(filteredDevice->initialize());

	auto inputOutput = std::make_shared<MockInputOutput>();
	EXPECT_TRUE(filteredDevice->addInputConsumer(inputOutput));
	EXPECT_TRUE(filteredDevice->removeInputConsumer(inputOutput));

	EXPECT_TRUE(filteredDevice->setOutputProducer(inputOutput));
	EXPECT_TRUE(filteredDevice->hasOutputProducer());
	EXPECT_TRUE(filteredDevice->removeOutputProducer(inputOutput));
	EXPECT_FALSE(filteredDevice->hasOutputProducer());
}

TEST(FilteredDeviceTest, Serialization)
{
	auto runtime = std::make_shared<SurgSim::Framework::Runtime>("config.txt");
	std::shared_ptr<SurgSim::Input::DeviceInterface> device;
	ASSERT_NO_THROW(device = SurgSim::Devices::loadDevice("FilteredDevice.yaml"));
	ASSERT_NE(nullptr, device);
	auto typedDevice = std::dynamic_pointer_cast<FilteredDevice>(device);
	ASSERT_NE(nullptr, typedDevice);

	auto input = std::make_shared<MockInputOutput>();
	ASSERT_TRUE(device->addInputConsumer(input));
	SurgSim::Math::RigidTransform3d pose;
	ASSERT_TRUE(input->m_lastReceivedInput.poses().get(SurgSim::DataStructures::Names::POSE, &pose));

	Eigen::AngleAxisd angleAxis;
	angleAxis.angle() = 12.3;
	angleAxis.axis() = SurgSim::Math::Vector3d(0.5, 0.5, 0.0);
	SurgSim::Math::Vector3d translation = SurgSim::Math::Vector3d(7.8, 8.9, 9.0);
	SurgSim::Math::RigidTransform3d expectedTransform =
		SurgSim::Math::makeRigidTransform(SurgSim::Math::Quaterniond(angleAxis), translation);
	EXPECT_TRUE(pose.isApprox(expectedTransform));

	ASSERT_NO_THROW(device = SurgSim::Devices::loadDevice("BadFilteredDevice.yaml"));
	EXPECT_EQ(nullptr, device);
}