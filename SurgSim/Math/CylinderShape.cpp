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

#include <SurgSim/Math/CylinderShape.h>

namespace SurgSim
{
namespace Math
{

CylinderShape::CylinderShape(double length, double radius)
{
	m_length = length;
	m_radius = radius;
}

int CylinderShape::getType()
{
	return SHAPE_TYPE_CYLINDER;
}

double CylinderShape::getLength() const
{
	return m_length;
}

double CylinderShape::getRadius() const
{
	return m_radius;
}

double CylinderShape::getVolume() const
{
	return M_PI * m_radius * m_radius * m_length;
}

SurgSim::Math::Vector3d CylinderShape::getCenter() const
{
	return Vector3d(0.0, 0.0, 0.0);
}

SurgSim::Math::Matrix33d CylinderShape::getSecondMomentMatrix() const
{
	const double volume = getVolume();
	const double coef    = 1.0 / 12.0 * volume;
	const double coefDir = 1.0 /  2.0 * volume;
	const double squareL = m_length * m_length;
	const double squareRadius = m_radius * m_radius;

	Matrix33d secondMoment;
	secondMoment.setZero();
	secondMoment.diagonal().setConstant(coef * (3.0 * squareRadius + squareL));
	secondMoment(1, 1) = coefDir * (squareRadius);

	return secondMoment;
}

YAML::Node SurgSim::Math::CylinderShape::encode()
{
	YAML::Node node;
	node = SurgSim::Math::Shape::encode();
	node["Radius"] = getRadius();
	node["Length"] = getLength();
	return node;
}

bool SurgSim::Math::CylinderShape::decode(const YAML::Node& node)
{
	bool isSuccess = SurgSim::Math::Shape::decode(node);
	if (! isSuccess)
	{
		return false;
	}

	m_radius = node["Radius"].as<double>();
	m_length = node["Length"].as<double>();
	return true;
}

}; // namespace Math
}; // namespace SurgSim
