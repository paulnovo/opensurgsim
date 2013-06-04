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

#include <SurgSim/Physics/RigidActorCollisionRepresentation.h>

namespace SurgSim
{
namespace Physics
{

RigidActorCollisionRepresentation::RigidActorCollisionRepresentation(std::shared_ptr<RigidActor> actor) : m_actor(actor)
{

}

RigidActorCollisionRepresentation::~RigidActorCollisionRepresentation()
{

}

int RigidActorCollisionRepresentation::getShapeType() const
{
	return m_actor->getCurrentParameters().getShapeUsedForMassInertia()->getType();
}

const std::shared_ptr<RigidShape> RigidActorCollisionRepresentation::getShape() const
{
	return m_actor->getCurrentParameters().getShapeUsedForMassInertia();
}

const SurgSim::Math::RigidTransform3d& RigidActorCollisionRepresentation::getLocalToWorldTransform() const
{
	return m_actor->getPose();
}

}; // Physics
}; // SurgSim




