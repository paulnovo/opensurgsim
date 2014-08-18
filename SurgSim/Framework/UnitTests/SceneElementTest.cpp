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

#include <gtest/gtest.h>

#include "SurgSim/Framework/PoseComponent.h"
#include "SurgSim/Framework/SceneElement.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Math/Quaternion.h"
#include "SurgSim/Math/Vector.h"

#include "MockObjects.h"  //NOLINT

using SurgSim::Framework::Component;
using SurgSim::Framework::PoseComponent;
using SurgSim::Framework::SceneElement;

TEST(SceneElementTest, Constructor)
{
	ASSERT_NO_THROW(MockSceneElement element);
}

TEST(SceneElementTest, Pose)
{
	using SurgSim::Math::makeRigidTransform;
	using SurgSim::Math::Quaterniond;
	using SurgSim::Math::RigidTransform3d;
	using SurgSim::Math::Vector3d;

	MockSceneElement element;
	EXPECT_TRUE(element.getPose().isApprox(RigidTransform3d::Identity()));

	RigidTransform3d pose(makeRigidTransform(Quaterniond(0.0, 1.0, 0.0, 0.0), Vector3d(1.0, 2.0, 3.0)));
	element.setPose(pose);
	EXPECT_TRUE(element.getPose().isApprox(pose));
	EXPECT_TRUE(element.getPoseComponent()->getPose().isApprox(pose));
}

TEST(SceneElementTest, UpdateFunctions)
{
	MockSceneElement element;

	element.update(1.0);
	EXPECT_TRUE(element.didUpdate);

	element.lateUpdate(1.0);
	EXPECT_TRUE(element.didLateUpdate);

	element.fixedRateUpdate(1.0);
	EXPECT_TRUE(element.didFixedUpdate);
}

TEST(SceneElementTest, AddAndTestComponents)
{
	std::shared_ptr<MockSceneElement> element = std::make_shared<MockSceneElement>();
	std::shared_ptr<MockComponent> component = std::make_shared<MockComponent>("TestComponent");

	EXPECT_TRUE(element->addComponent(component));

	// SceneElement should be set after add
	EXPECT_EQ(component->getSceneElement(), element);

	// Scene in Component will not be set until initialization.
	EXPECT_NE(component->getScene(), element->getScene() );
}

TEST(SceneElementTest, AddAndAccessComponents)
{
	std::shared_ptr<MockSceneElement> element(new MockSceneElement());

	std::shared_ptr<MockComponent> component1(new MockComponent("TestComponent1"));
	std::shared_ptr<MockComponent> component2(new MockComponent("TestComponent2"));

	EXPECT_TRUE(element->addComponent(component1));
	EXPECT_TRUE(element->addComponent(component2));

	// Should not be able to add two with the same name
	EXPECT_FALSE(element->addComponent(component1));

	// Should not be able to add nullptr component
	EXPECT_ANY_THROW(element->addComponent(nullptr));

	std::shared_ptr<Component> fetched(element->getComponent("TestComponent1"));
	ASSERT_NE(nullptr, fetched);
	EXPECT_EQ("TestComponent1", fetched->getName());

	fetched = element->getComponent("Random");
	EXPECT_EQ(nullptr, fetched);
}

TEST(SceneElementTest, RemoveComponents)
{
	std::shared_ptr<MockSceneElement> element(new MockSceneElement());

	std::shared_ptr<MockComponent> component1(new MockComponent("TestComponent1"));
	std::shared_ptr<MockComponent> component2(new MockComponent("TestComponent2"));

	EXPECT_TRUE(element->addComponent(component1));
	EXPECT_TRUE(element->addComponent(component2));

	EXPECT_TRUE(element->removeComponent("TestComponent2"));
	EXPECT_EQ(nullptr, element->getComponent("TestComponent2"));

	// Add back should work
	EXPECT_TRUE(element->addComponent(component2));

	EXPECT_TRUE(element->removeComponent(component1));
	EXPECT_EQ(nullptr, element->getComponent("TestComponent1"));
}

TEST(SceneElementTest, GetComponentsTest)
{
	std::shared_ptr<MockSceneElement> element(new MockSceneElement());

	std::shared_ptr<MockComponent> component1(new MockComponent("TestComponent1"));
	std::shared_ptr<MockComponent> component2(new MockComponent("TestComponent2"));

	element->addComponent(component1);
	EXPECT_EQ(2u, element->getComponents().size());

	element->addComponent(component2);
	EXPECT_EQ(3u, element->getComponents().size());

	std::vector<std::shared_ptr<Component>> components = element->getComponents();

	EXPECT_NE(components.end(), std::find(components.cbegin(), components.cend(), component1));
	EXPECT_NE(components.end(), std::find(components.cbegin(), components.cend(), component2));

	element->removeComponent(component1);
	components = element->getComponents();
	EXPECT_EQ(2u, components.size());
}

TEST(SceneElementTest, GetTypedComponentsTests)
{
	std::shared_ptr<SceneElement> element(new MockSceneElement());
	std::shared_ptr<MockBehavior> behavior(new MockBehavior("MockBehavior"));
	std::shared_ptr<MockComponent> component1(new MockComponent("Test Component1"));
	std::shared_ptr<MockComponent> component2(new MockComponent("Test Component2"));

	element->addComponent(behavior);
	element->addComponent(component1);
	element->addComponent(component2);

	EXPECT_EQ(1u, element->getComponents<MockBehavior>().size());
	EXPECT_EQ(2u, element->getComponents<MockComponent>().size());

	element->removeComponent(component1);
	EXPECT_EQ(1u, element->getComponents<MockComponent>().size());

	element->removeComponent(component2);
	EXPECT_EQ(0u, element->getComponents<MockComponent>().size());
}

TEST(SceneElementTest, InitComponentTest)
{
	std::shared_ptr<MockSceneElement> element(new MockSceneElement());
	std::shared_ptr<MockComponent> component1(new MockComponent("TestComponent1"));
	std::shared_ptr<MockComponent> component2(new MockComponent("TestComponent2"));

	element->addComponent(component1);
	element->addComponent(component2);

	element->initialize();

	EXPECT_TRUE(element->didInit);
}

TEST(SceneElementTest, DoubleInitTest)
{
	std::shared_ptr<MockSceneElement> element(new MockSceneElement());

	EXPECT_FALSE(element->didInit);

	element->initialize();
	EXPECT_TRUE(element->didInit);

	ASSERT_ANY_THROW(element->initialize());
}

TEST(SceneElement, SetActiveTest)
{
	auto element = std::make_shared<MockSceneElement>();
	auto poseComponent = element->getPoseComponent();
	auto mockComponent0 = std::make_shared<MockComponent>("MockComponent0");
	auto mockComponent1 = std::make_shared<MockComponent>("MockComponent1");
	element->addComponent(mockComponent0);
	element->addComponent(mockComponent1);

	mockComponent1->setActive(false);

	EXPECT_TRUE(element->isActive());
	EXPECT_TRUE(poseComponent->isActive());
	EXPECT_TRUE(mockComponent0->isActive());
	EXPECT_FALSE(mockComponent1->isActive());

	// Before initialization, setting SceneElement to inactive will not affect its PoseComponent.
	// But all other Components will be affected.
	element->setActive(false);
	EXPECT_FALSE(element->isActive());
	EXPECT_TRUE(poseComponent->isActive());
	EXPECT_FALSE(mockComponent0->isActive());
	EXPECT_FALSE(mockComponent1->isActive());

	// After initialization, SceneElement's activity (active/inactive) will affect all its
	// component (including the PoseComponent).
	element->initialize();
	EXPECT_FALSE(element->isActive());
	EXPECT_FALSE(poseComponent->isActive());
	EXPECT_FALSE(mockComponent0->isActive());
	EXPECT_FALSE(mockComponent1->isActive());

	// After initialization, set SceneElement back to active, its Component will be active (if they were active).
	element->setActive(true);
	EXPECT_TRUE(element->isActive());
	EXPECT_TRUE(poseComponent->isActive());
	EXPECT_TRUE(mockComponent0->isActive());
	EXPECT_FALSE(mockComponent1->isActive());

	// After initialization, setting SceneElement to inactive will affect all its components.
	element->setActive(false);
	EXPECT_FALSE(element->isActive());
	EXPECT_FALSE(poseComponent->isActive());
	EXPECT_FALSE(mockComponent0->isActive());
	EXPECT_FALSE(mockComponent1->isActive());

	auto mockComponent2 = std::make_shared<MockComponent>("MockComponent2");
	EXPECT_TRUE(mockComponent2->isActive());

	// An active component added to an inactive SceneElement, the Component will be inactive.
	element->addComponent(mockComponent2);
	EXPECT_FALSE(element->isActive());
	EXPECT_FALSE(mockComponent2->isActive());

	// Set SceneElement back to active, the Component will be active (it was active).
	element->setActive(true);
	EXPECT_TRUE(mockComponent2->isActive());

	// A component can be set to inactive separately.
	mockComponent2->setActive(false);
	EXPECT_FALSE(mockComponent2->isActive());
	EXPECT_TRUE(mockComponent0->isActive());
}