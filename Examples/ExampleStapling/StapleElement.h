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

#ifndef EXAMPLES_EXAMPLESTAPLING_STAPLEELEMENT_H
#define EXAMPLES_EXAMPLESTAPLING_STAPLEELEMENT_H

#include <string>

#include "SurgSim/Framework/SceneElement.h"
#include "SurgSim/Math/RigidTransform.h"

namespace SurgSim
{
namespace Physics
{
class RigidRepresentation;
}
}

class StapleElement : public SurgSim::Framework::SceneElement
{
public:
	/// Constructor
	/// \param name Name of the staple element.
	explicit StapleElement(const std::string& name);

	/// Set initial pose of the staple
	/// \param pose	The initial pose to set.
	void setPose(const SurgSim::Math::RigidTransform3d& pose);

	/// Return the staple physics representation.
	/// \return The shared poiter to the physics representation.
	const std::shared_ptr<SurgSim::Physics::RigidRepresentation> getPhysicsRepresentation();

	/// Specify if the collision representation should be created or not.
	/// \param flag Flag to specify if the collision representation should be created or not.
	void setHasCollisionRepresentation(bool flag);

	/// Destructor
	~StapleElement();

protected:
	/// Initialize this scene element
	/// \return True on success, otherwise false.
	virtual bool doInitialize() override;

private:
	/// The pose of this scene element
	SurgSim::Math::RigidTransform3d m_pose;

	/// The shared pointer to the physics representation.
	std::shared_ptr<SurgSim::Physics::RigidRepresentation> m_physicsRepresentation;

	/// Flag to specify if the stapleElement needs to be created with a collision representation.
	bool m_hasCollisionRepresentation;
};

#endif //EXAMPLES_EXAMPLESTAPLING_STAPLEELEMENT_H
