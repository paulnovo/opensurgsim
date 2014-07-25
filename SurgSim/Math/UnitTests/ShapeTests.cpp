//// This file is a part of the OpenSurgSim project.
//// Copyright 2013, SimQuest Solutions Inc.
////
//// Licensed under the Apache License, Version 2.0 (the "License");
//// you may not use this file except in compliance with the License.
//// You may obtain a copy of the License at
////
////     http://www.apache.org/licenses/LICENSE-2.0
////
//// Unless required by applicable law or agreed to in writing, software
//// distributed under the License is distributed on an "AS IS" BASIS,
//// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//// See the License for the specific language governing permissions and
//// limitations under the License.

#include <gtest/gtest.h>

#include "SurgSim/Framework/ApplicationData.h"
#include "SurgSim/Framework/Runtime.h"
#include "SurgSim/Math/MathConvert.h"
#include "SurgSim/Math/Matrix.h"
#include "SurgSim/Math/Shape.h"
#include "SurgSim/Math/Shapes.h"
#include "SurgSim/Math/Vector.h"

using SurgSim::Math::Vector3d;
using SurgSim::Math::Matrix33d;

using SurgSim::Math::BoxShape;
using SurgSim::Math::CapsuleShape;
using SurgSim::Math::CylinderShape;
using SurgSim::Math::DoubleSidedPlaneShape;
using SurgSim::Math::OctreeShape;
using SurgSim::Math::PlaneShape;
using SurgSim::Math::Shape;
using SurgSim::Math::SphereShape;

namespace {
	const double epsilon = 1e-10;
}

class ShapeTest : public ::testing::Test
{
public:
	void SetUp()
	{
		m_rho = 9000.0;
		m_radius = 0.01;
		m_length = 0.1;
		m_size[0] = 0.1;
		m_size[1] = 0.2;
		m_size[2] = 0.3;
	}

	void TearDown()
	{
	}

	// Mass density
	double m_rho;

	// Radius (sphere/cylinder/capsule)
	double m_radius;

	// Length (cylinder/capsule)
	double m_length;

	// Size (box)
	double m_size[3];
};

TEST_F(ShapeTest, EncodeEmptyShapeTest)
{
	std::shared_ptr<Shape> shape;
	EXPECT_ANY_THROW(YAML::convert<std::shared_ptr<Shape>>::encode(shape));
}

TEST_F(ShapeTest, SphereSerializationTest)
{
	{
		YAML::Node node;
		node["SurgSim::Math::SphereShape"]["Radius"] = m_radius;

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::SphereShape", shape->getClassName());
		EXPECT_NEAR(m_radius, shape->getValue<double>("Radius"), epsilon);
	}

	{
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::SphereShape"));
		shape->setValue("Radius", m_radius);

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);
		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::SphereShape"].IsDefined());
		auto data = node["SurgSim::Math::SphereShape"];
		EXPECT_EQ(1u, data.size());

		std::shared_ptr<SphereShape> sphereShape;
		ASSERT_NO_THROW(sphereShape = std::dynamic_pointer_cast<SphereShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::SphereShape", sphereShape->getClassName());
		EXPECT_NEAR(m_radius, sphereShape->getValue<double>("Radius"), epsilon);
	}
}

TEST_F(ShapeTest, Sphere)
{
	ASSERT_NO_THROW({SphereShape sphere(m_radius);});

	SphereShape sphere(m_radius);
	EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_SPHERE, sphere.getType());
	EXPECT_NEAR(m_radius, sphere.getRadius(), epsilon);

	const double& r = m_radius;
	const double r2 = r * r;
	double expectedVolume = 4.0 / 3.0 * M_PI * (r2 * r);
	double expectedMass = m_rho * expectedVolume;
	double coef = 2.0 / 5.0 * expectedMass * r2;
	Matrix33d expectedInertia;
	expectedInertia << coef, 0.0, 0.0,
					   0.0, coef, 0.0,
					   0.0, 0.0, coef;

	double volume = sphere.getVolume();
	Vector3d center = sphere.getCenter();
	Matrix33d inertia = sphere.getSecondMomentOfVolume() * m_rho;

	EXPECT_NEAR(expectedVolume, volume, epsilon);
	EXPECT_TRUE(center.isZero());
	EXPECT_TRUE(expectedInertia.isApprox(inertia));
}

TEST_F(ShapeTest, BoxSerializationTest)
{
	{
		YAML::Node node;
		node["SurgSim::Math::BoxShape"]["SizeX"] = m_size[0];
		node["SurgSim::Math::BoxShape"]["SizeY"] = m_size[1];
		node["SurgSim::Math::BoxShape"]["SizeZ"] = m_size[2];

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::BoxShape", shape->getClassName());
		EXPECT_NEAR(m_size[0], shape->getValue<double>("SizeX"), epsilon);
		EXPECT_NEAR(m_size[1], shape->getValue<double>("SizeY"), epsilon);
		EXPECT_NEAR(m_size[2], shape->getValue<double>("SizeZ"), epsilon);
	}

	{
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::BoxShape"));
		shape->setValue("SizeX", m_size[0]);
		shape->setValue("SizeY", m_size[1]);
		shape->setValue("SizeZ", m_size[2]);

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);
		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::BoxShape"].IsDefined());
		auto data = node["SurgSim::Math::BoxShape"];
		EXPECT_EQ(3u, data.size());

		std::shared_ptr<BoxShape> boxShape;
		ASSERT_NO_THROW(boxShape = std::dynamic_pointer_cast<BoxShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::BoxShape", boxShape->getClassName());
		EXPECT_NEAR(m_size[0], boxShape->getValue<double>("SizeX"), epsilon);
		EXPECT_NEAR(m_size[1], boxShape->getValue<double>("SizeY"), epsilon);
		EXPECT_NEAR(m_size[2], boxShape->getValue<double>("SizeZ"), epsilon);
	}
}

TEST_F(ShapeTest, Box)
{
	ASSERT_NO_THROW({BoxShape box(m_size[0], m_size[1], m_size[2]);});

	Vector3d size(m_size[0], m_size[1], m_size[2]);
	BoxShape box(m_size[0], m_size[1], m_size[2]);
	EXPECT_NEAR(m_size[0], box.getSizeX(), epsilon);
	EXPECT_NEAR(m_size[1], box.getSizeY(), epsilon);
	EXPECT_NEAR(m_size[2], box.getSizeZ(), epsilon);
	EXPECT_TRUE(box.getSize().isApprox(size));
	EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_BOX, box.getType());

	double expectedVolume = m_size[0] * m_size[1] * m_size[2];
	double expectedMass = m_rho * expectedVolume;
	double coef = 1.0 / 12.0 * expectedMass;
	double x2 = m_size[0] * m_size[0];
	double y2 = m_size[1] * m_size[1];
	double z2 = m_size[2] * m_size[2];
	Matrix33d expectedInertia;
	expectedInertia << coef * (y2 + z2), 0.0, 0.0,
					   0.0, coef * (x2 + z2), 0.0,
					   0.0, 0.0, coef * (x2 + y2);

	double volume = box.getVolume();
	Vector3d center = box.getCenter();
	Matrix33d inertia = box.getSecondMomentOfVolume() * m_rho;

	EXPECT_NEAR(expectedVolume, volume, epsilon);
	EXPECT_TRUE(center.isZero());
	EXPECT_TRUE(expectedInertia.isApprox(inertia));
}

TEST_F(ShapeTest, CylinderSerializationTest)
{
	{
		YAML::Node node;
		node["SurgSim::Math::CylinderShape"]["Length"] = m_length;
		node["SurgSim::Math::CylinderShape"]["Radius"] = m_length;

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::CylinderShape", shape->getClassName());
		EXPECT_NEAR(m_length, shape->getValue<double>("Length"), epsilon);
		EXPECT_NEAR(m_length, shape->getValue<double>("Radius"), epsilon);
	}

	{
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::CylinderShape"));
		shape->setValue("Length", m_length);
		shape->setValue("Radius", m_radius);

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);

		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::CylinderShape"].IsDefined());
		auto data = node["SurgSim::Math::CylinderShape"];
		EXPECT_EQ(2u, data.size());

		std::shared_ptr<CylinderShape> cylinderShape;
		ASSERT_NO_THROW(cylinderShape = std::dynamic_pointer_cast<CylinderShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::CylinderShape", cylinderShape->getClassName());
		EXPECT_NEAR(m_length, cylinderShape->getValue<double>("Length"), epsilon);
		EXPECT_NEAR(m_radius, cylinderShape->getValue<double>("Radius"), epsilon);
	}
}

TEST_F(ShapeTest, Cylinder)
{
	ASSERT_NO_THROW({CylinderShape cyliner(m_length, m_radius);});

	CylinderShape cylinder(m_length, m_radius);
	EXPECT_NEAR(m_length, cylinder.getLength(), epsilon);
	EXPECT_NEAR(m_radius, cylinder.getRadius(), epsilon);
	EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_CYLINDER, cylinder.getType());

	double expectedVolume = M_PI * m_radius * m_radius * m_length;
	double expectedMass = m_rho * expectedVolume;

	double r1sq = m_radius * m_radius;
	double l2 = m_length * m_length;
	double coefDir = 1.0 /  2.0 * expectedMass * (r1sq);
	double coef    = 1.0 / 12.0 * expectedMass * (3.0 * (r1sq) + l2);
	Matrix33d expectedInertia;
	expectedInertia << coef, 0.0, 0.0,
					   0.0, coefDir, 0.0,
					   0.0, 0.0, coef;

	double volume = cylinder.getVolume();
	Vector3d center = cylinder.getCenter();
	Matrix33d inertia = cylinder.getSecondMomentOfVolume() * m_rho;

	EXPECT_NEAR(expectedVolume, volume, epsilon);
	EXPECT_TRUE(center.isZero());
	EXPECT_TRUE(expectedInertia.isApprox(inertia));
}

TEST_F(ShapeTest, CapsuleSerializationTest)
{
	{
		YAML::Node node;
		node["SurgSim::Math::CapsuleShape"]["Length"] = m_length;
		node["SurgSim::Math::CapsuleShape"]["Radius"] = m_length;

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::CapsuleShape", shape->getClassName());
		EXPECT_NEAR(m_length, shape->getValue<double>("Length"), epsilon);
		EXPECT_NEAR(m_length, shape->getValue<double>("Radius"), epsilon);
	}

	{
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::CapsuleShape"));
		shape->setValue("Length", m_length);
		shape->setValue("Radius", m_radius);

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);

		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::CapsuleShape"].IsDefined());
		auto data = node["SurgSim::Math::CapsuleShape"];
		EXPECT_EQ(2u, data.size());

		std::shared_ptr<CapsuleShape> capsuleShape;
		ASSERT_NO_THROW(capsuleShape = std::dynamic_pointer_cast<CapsuleShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::CapsuleShape", capsuleShape->getClassName());
		EXPECT_NEAR(m_length, capsuleShape->getValue<double>("Length"), epsilon);
		EXPECT_NEAR(m_radius, capsuleShape->getValue<double>("Radius"), epsilon);
	}
}

TEST_F(ShapeTest, Capsule)
{
	ASSERT_NO_THROW({CapsuleShape capsule(m_length, m_radius);});

	CapsuleShape capsule(m_length, m_radius);
	EXPECT_NEAR(m_length, capsule.getLength(), epsilon);
	EXPECT_NEAR(m_radius, capsule.getRadius(), epsilon);
	EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_CAPSULE, capsule.getType());

	double r2 = m_radius * m_radius;
	double r3 = r2 * m_radius;
	double l2 = m_length * m_length;

	double volumeCylinder = M_PI * r2 * m_length;
	double massCylinder = m_rho * volumeCylinder;
	double volumeSphere = 4.0 / 3.0 * M_PI * r3;
	double massSphere = m_rho * volumeSphere;
	double expectedVolume = volumeCylinder + volumeSphere;
	double coefDir = 2.0 /  5.0 * massSphere * r2;
	double coef    = coefDir;
	coefDir += 1.0 / 2.0 * massCylinder * r2;
	coef += massSphere  * (1.0 / 4.0 * l2 + 3.0 / 8.0 * m_radius * m_length);
	coef += 1.0 / 12.0 * massCylinder * (3 * r2 + l2);
	Matrix33d expectedInertia;
	expectedInertia << coef, 0.0, 0.0,
					   0.0, coefDir, 0.0,
					   0.0, 0.0, coef;

	double volume = capsule.getVolume();
	Vector3d center = capsule.getCenter();
	Matrix33d inertia = capsule.getSecondMomentOfVolume() * m_rho;

	EXPECT_NEAR(expectedVolume, volume, epsilon);
	EXPECT_TRUE(center.isZero());
	EXPECT_TRUE(expectedInertia.isApprox(inertia));
}

TEST_F(ShapeTest, DoubleSidedPlaneShapeSerializationTest)
{
	{
		YAML::Node node, empty;
		node["SurgSim::Math::DoubleSidedPlaneShape"] = empty;

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::DoubleSidedPlaneShape", shape->getClassName());
	}

	{
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::DoubleSidedPlaneShape"));

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);

		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::DoubleSidedPlaneShape"].IsDefined());
		auto data = node["SurgSim::Math::DoubleSidedPlaneShape"];
		EXPECT_EQ(0u, data.size()); // DoubleSidedPlaneShape has no serialized property .

		std::shared_ptr<DoubleSidedPlaneShape> doubleSidedPlaneShape;
		ASSERT_NO_THROW(doubleSidedPlaneShape =
			std::dynamic_pointer_cast<DoubleSidedPlaneShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::DoubleSidedPlaneShape", doubleSidedPlaneShape->getClassName());
	}
}

TEST_F(ShapeTest, DoubleSidedPlaneShape)
{
	EXPECT_NO_THROW(DoubleSidedPlaneShape doubleSidedPlaneShape);
	DoubleSidedPlaneShape doubleSidedPlaneShape;

	EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_DOUBLESIDEDPLANE, doubleSidedPlaneShape.getType());
	EXPECT_NEAR(0.0, doubleSidedPlaneShape.getVolume(), epsilon);
	EXPECT_TRUE(doubleSidedPlaneShape.getCenter().isZero());
	EXPECT_TRUE(doubleSidedPlaneShape.getSecondMomentOfVolume().isApprox(Matrix33d::Zero()));
	EXPECT_NEAR(0.0, doubleSidedPlaneShape.getD(), epsilon);
	EXPECT_TRUE(doubleSidedPlaneShape.getNormal().isApprox(Vector3d(0.0, 1.0, 0.0)));
}


TEST_F(ShapeTest, OctreeSerializationTest)
{
	const std::string fileName = "OctreeShapeData/staple.vox";

	{
		YAML::Node node;
		node["SurgSim::Math::OctreeShape"]["FileName"] = fileName;

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::OctreeShape", shape->getClassName());
		EXPECT_EQ(fileName, shape->getValue<std::string>("FileName"));
	}

	{
		SurgSim::Framework::Runtime runtime("config.txt");
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::OctreeShape"));
		shape->setValue("FileName", fileName);

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);
		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::OctreeShape"].IsDefined());
		auto data = node["SurgSim::Math::OctreeShape"];
		EXPECT_EQ(1u, data.size());

		std::shared_ptr<OctreeShape> newOctreeShape;
		ASSERT_NO_THROW(newOctreeShape = std::dynamic_pointer_cast<OctreeShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::OctreeShape", newOctreeShape->getClassName());
		EXPECT_EQ(fileName, newOctreeShape->getFileName());
	}
}

TEST_F(ShapeTest, OctreeShape)
{
	OctreeShape::NodeType::AxisAlignedBoundingBox boundingBox(Vector3d::Zero(), m_size);
	std::shared_ptr<OctreeShape::NodeType> node = std::make_shared<OctreeShape::NodeType>(boundingBox);

	{
		ASSERT_NO_THROW({OctreeShape octree;});
		ASSERT_NO_THROW({OctreeShape octree(*node);});
	}

	{
		OctreeShape octree;
		EXPECT_EQ(nullptr, octree.getRootNode());
		octree.setRootNode(node);
		EXPECT_EQ(node, octree.getRootNode());
	}

	{
		SurgSim::Framework::Runtime runtime("config.txt");
		const std::string fileName = "OctreeShapeData/staple.vox";
		OctreeShape octree;
		EXPECT_NO_THROW(octree.setRootNode(node));
		EXPECT_NO_THROW(octree.load(fileName));

		EXPECT_EQ(octree.getClassName(), "SurgSim::Math::OctreeShape");
		EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_OCTREE, octree.getType());
		EXPECT_THROW(octree.getVolume(), SurgSim::Framework::AssertionFailure);
		EXPECT_TRUE(octree.getCenter().isApprox(Vector3d::Zero(), epsilon));
		EXPECT_THROW(octree.getSecondMomentOfVolume(), SurgSim::Framework::AssertionFailure);
		EXPECT_EQ(fileName, octree.getFileName());
	}
}


TEST_F(ShapeTest, PlaneShapeSerializationTest)
{
	{
		YAML::Node node, empty;
		node["SurgSim::Math::PlaneShape"] = empty;

		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = node.as<std::shared_ptr<Shape>>());

		EXPECT_EQ("SurgSim::Math::PlaneShape", shape->getClassName());
	}

	{
		std::shared_ptr<Shape> shape;
		ASSERT_NO_THROW(shape = Shape::getFactory().create("SurgSim::Math::PlaneShape"));

		YAML::Node node;
		ASSERT_NO_THROW(node = shape);

		EXPECT_TRUE(node.IsMap());
		EXPECT_EQ(1u, node.size());

		ASSERT_TRUE(node["SurgSim::Math::PlaneShape"].IsDefined());
		auto data = node["SurgSim::Math::PlaneShape"];
		EXPECT_EQ(0u, data.size()); //PlaneShape has no serialized property.

		std::shared_ptr<PlaneShape> planeShape;
		ASSERT_NO_THROW(planeShape = std::dynamic_pointer_cast<PlaneShape>(node.as<std::shared_ptr<Shape>>()));
		EXPECT_EQ("SurgSim::Math::PlaneShape", planeShape->getClassName());
	}
}

TEST_F(ShapeTest, PlaneShape)
{
	EXPECT_NO_THROW(PlaneShape planeShape);
	PlaneShape planeShape;

	EXPECT_EQ(SurgSim::Math::SHAPE_TYPE_PLANE, planeShape.getType());
	EXPECT_NEAR(0.0, planeShape.getVolume(), epsilon);
	EXPECT_TRUE(planeShape.getCenter().isZero());
	EXPECT_TRUE(planeShape.getSecondMomentOfVolume().isApprox(Matrix33d::Zero()));
	EXPECT_NEAR(0.0, planeShape.getD(), epsilon);
	EXPECT_TRUE(planeShape.getNormal().isApprox(Vector3d(0.0, 1.0, 0.0)));
}