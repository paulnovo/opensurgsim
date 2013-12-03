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

#ifndef SURGSIM_MATH_DOUBLESIDEDPLANESHAPE_H
#define SURGSIM_MATH_DOUBLESIDEDPLANESHAPE_H

#include <SurgSim/Math/Shape.h>

namespace SurgSim
{

namespace Math
{

/// DoubleSidedPlaneShape: The XZ plane (d = 0) with normal pointing along
/// positive Y axis.
class DoubleSidedPlaneShape: public Shape
{
public:

	/// Constructor: No members to initialize.
	DoubleSidedPlaneShape();

	/// \return the type of the shape
	virtual int getType() override;

	/// Get the volume of the shape
	/// \return The volume of the shape (in m-3)
	virtual double getVolume() const override;

	/// Get the volumetric center of the shape
	/// \return The center of the shape
	virtual Vector3d getCenter() const override;

	/// Get the second central moment of the shape, commonly used
	/// to calculate the moment of inertia matrix
	/// \return The 3x3 symmetric second moment matrix
	virtual Matrix33d getSecondMomentMatrix() const override;

	/// Gets the d of the plane equation.
	/// \return	The value of d (always 0).
	double getD() const;

	/// Gets the normal of the plane equation.
	/// \return	The value of the normal (always Y axis).
	Vector3d getNormal() const;

	/// Serialize declarations of the double sided plane
	OSS_SERIALIZE(SurgSim::Math::DoubleSidedPlaneShape);

};

}; // Math

}; // SurgSim

#endif // SURGSIM_MATH_DOUBLESIDEDPLANESHAPE_H
