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

#include <memory>

#include <SurgSim/Framework/Runtime.h>
#include <SurgSim/Framework/Scene.h>
#include <SurgSim/Graphics/OsgManager.h>
#include <SurgSim/Graphics/OsgPointCloudRepresentation.h>
#include <SurgSim/Graphics/OsgBoxRepresentation.h>
#include <SurgSim/Graphics/OsgViewElement.h>
#include <SurgSim/DataStructures/Mesh.h>

#include <SurgSim/Math/Quaternion.h>
#include <SurgSim/Math/Vector.h>
#include <SurgSim/Math/RigidTransform.h>

#include <SurgSim/Testing/MathUtilities.h>

using SurgSim::Math::Vector3d;
using SurgSim::Math::Quaterniond;
using SurgSim::Math::makeRigidTransform;
using SurgSim::Math::makeRotationQuaternion;


namespace SurgSim 
{
namespace Graphics
{

struct OsgPointCloudRepresentationRenderTests : public ::testing::Test
{
	virtual void SetUp()
	{
		runtime = std::make_shared<SurgSim::Framework::Runtime>();
		graphicsManager = std::make_shared<SurgSim::Graphics::OsgManager>();

		runtime->addManager(graphicsManager);

		scene = std::make_shared<SurgSim::Framework::Scene>();
		runtime->setScene(scene);
	}

	virtual void TearDown()
	{
		runtime->stop();
	}

	std::shared_ptr<SurgSim::Framework::Runtime> runtime;
	std::shared_ptr<SurgSim::Graphics::OsgManager> graphicsManager;
	std::shared_ptr<SurgSim::Framework::Scene> scene;

};

TEST_F(OsgPointCloudRepresentationRenderTests, StaticRotate)
{
	std::shared_ptr<OsgViewElement> viewElement = std::make_shared<OsgViewElement>("view element");
	scene->addSceneElement(viewElement);

	typedef SurgSim::DataStructures::Mesh<void> CloudMesh;

	std::shared_ptr<CloudMesh> mesh = std::make_shared<CloudMesh>();
	mesh->addVertex(CloudMesh::Vertex(Vector3d( 0.01,-0.01, 0.01)));
	mesh->addVertex(CloudMesh::Vertex(Vector3d(-0.01,-0.01, 0.01)));
	mesh->addVertex(CloudMesh::Vertex(Vector3d(-0.01,-0.01,-0.01)));
	mesh->addVertex(CloudMesh::Vertex(Vector3d( 0.01,-0.01,-0.01)));

	mesh->addVertex(CloudMesh::Vertex(Vector3d( 0.01, 0.01, 0.01)));
	mesh->addVertex(CloudMesh::Vertex(Vector3d(-0.01, 0.01, 0.01)));
	mesh->addVertex(CloudMesh::Vertex(Vector3d(-0.01, 0.01,-0.01)));
	mesh->addVertex(CloudMesh::Vertex(Vector3d( 0.01, 0.01,-0.01)));

	std::shared_ptr<PointCloudRepresentation<void>> cloud = 
		std::make_shared<OsgPointCloudRepresentation<void>>("cloud representation");

	std::shared_ptr<BoxRepresentation> boxRepresentation1 =
		std::make_shared<OsgBoxRepresentation>("box representation 1");

	cloud->setMesh(mesh);
	cloud->setInitialPose(makeRigidTransform(Quaterniond::Identity(), Vector3d(0.0,0.0,-0.2)));

	viewElement->addComponent(cloud);
	//viewElement->addComponent(boxRepresentation1);

	/// Run the thread
	runtime->start();
	EXPECT_TRUE(graphicsManager->isInitialized());
	EXPECT_TRUE(viewElement->isInitialized());
	boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

	int numSteps = 100;

	Vector3d startAngles(0.0,0.0,0.0);
	Vector3d endAngles(M_PI_4, M_PI_2, M_PI_2);
	Vector3d startPosition (-0.1, 0.0, -0.2);
	Vector3d endPosition(0.1, 0.0, -0.2);

	for (int i = 0; i < numSteps; ++i)
	{
		/// Calculate t in [0.0, 1.0]
		double t = static_cast<double>(i) / numSteps;
		cloud->setPose(SurgSim::Testing::lerpPoseFromAngles(t, startAngles, endAngles, startPosition, endPosition));
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000 / numSteps));
	}

	runtime->stop();
}

}; // namespace Graphics
}; // namespace SurgSim
