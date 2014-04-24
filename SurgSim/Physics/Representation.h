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

#ifndef SURGSIM_PHYSICS_REPRESENTATION_H
#define SURGSIM_PHYSICS_REPRESENTATION_H

#include <string>

#include "SurgSim/Math/Vector.h"
#include "SurgSim/Math/MlcpSolution.h"

#include "SurgSim/Framework/Representation.h"

#include "SurgSim/Collision/Location.h"

namespace SurgSim
{

namespace Collision
{
class Representation;
}

namespace Physics
{

class Localization;

enum RepresentationType
{
	REPRESENTATION_TYPE_INVALID = -1,
	REPRESENTATION_TYPE_FIXED = 0,
	REPRESENTATION_TYPE_RIGID,
	REPRESENTATION_TYPE_VTC_RIGID,
	REPRESENTATION_TYPE_MASSSPRING,
	REPRESENTATION_TYPE_FEM1D,
	REPRESENTATION_TYPE_FEM2D,
	REPRESENTATION_TYPE_FEM3D,
	REPRESENTATION_TYPE_COUNT
};

/// The Representation class defines the base class for all physics objects
class Representation : public SurgSim::Framework::Representation
{
public:
	/// Constructor
	/// \param name The representation's name
	explicit Representation(const std::string& name);

	/// Destructor
	virtual ~Representation();

	/// Query the representation type
	/// \return the RepresentationType for this representation
	virtual RepresentationType getType() const = 0;

	/// Reset the representation to its initial/default state
	virtual void resetState();

	/// Reset the representation parameters to their initial/default values
	virtual void resetParameters();

	/// Query the object number of degrees of freedom
	/// \return The number of degrees of freedom
	inline unsigned int getNumDof() const
	{
		return m_numDof;
	}

	/// Set active flag for this Representation
	/// \param isActive True if the Representation is being activated, False otherwise
	virtual void setIsActive(bool isActive);

	/// Query if this object is active in the scene.
	/// \return true if active, false if not.
	inline bool isActive() const
	{
		return m_isActive;
	}

	/// Set the gravity enable flag
	/// \param isGravityEnabled True if gravity enabled, false if not.
	virtual void setIsGravityEnabled(bool isGravityEnabled);

	/// Get the gravity enable flag
	/// \return true if gravity enabled, false if not.
	bool isGravityEnabled() const;

	/// Set whether this Representation is controling the pose of the SceneElement
	/// that it is part of.
	/// \param isDrivingElement true if this Representation is driving the pose of the SceneElement
	void setIsDrivingElement(bool isDrivingElement);

	/// Query if this Representation is controling the pose of the SceneElement
	/// that it is part of.
	/// \return true if this Representation is controlling the pose of the SceneElement
	bool isDrivingElement();

	/// Preprocessing done before the update call
	/// This needs to be called from the outside usually from a Computation
	/// \param dt The time step (in seconds)
	virtual void beforeUpdate(double dt);

	/// Update the representation state to the current time step
	/// \param dt The time step (in seconds)
	virtual void update(double dt);

	/// Postprocessing done after the update call
	/// This needs to be called from the outside usually from a Computation
	/// \param dt The time step (in seconds)
	virtual void afterUpdate(double dt);

	virtual std::shared_ptr<Localization> createLocalization(const SurgSim::Collision::Location& location);

	/// Update the Representation's current position and velocity using a time interval, dt, and change in velocity,
	/// deltaVelocity.
	///
	/// This function typically is called in the physics pipeline (PhysicsManager::doUpdate) after solving the equations
	/// that enforce constraints when collisions occur.  Specifically it is called in the PushResults::doUpdate step.
	/// \param dt The time step
	/// \param deltaVelocity The block of a vector containing the correction to be applied to the velocity
	virtual void applyCorrection(double dt, const Eigen::VectorBlock<SurgSim::Math::Vector>& deltaVelocity);

	/// \return the collision representation for this physics representation.
	std::shared_ptr<SurgSim::Collision::Representation> getCollisionRepresentation() const;

	/// Set the collision representation for this physics representation, when the collision object
	/// is involved in a collision, the collision should be resolved inside the dynamics calculation.
	/// \param representation The appropriate collision representation for this object.
	virtual void setCollisionRepresentation(std::shared_ptr<SurgSim::Collision::Representation> representation);

protected:
	/// Set the number of degrees of freedom
	/// \param numDof The number of degrees of freedom
	/// \note protected so that nobody can change the number of DOF
	/// \note except daughter classes
	void setNumDof(unsigned int numDof);

	/// Get the gravity used by this Representation
	/// \return The gravity vector
	const SurgSim::Math::Vector3d& getGravity() const;

	/// This entity's collision representation, these are usually very specific to the physics representation
	std::shared_ptr<SurgSim::Collision::Representation> m_collisionRepresentation;

private:
	/// NO copy constructor
	Representation(const Representation& a);

	/// NO assignment operator
	Representation& operator =(const Representation& a);

	/// Gravity vector
	const SurgSim::Math::Vector3d m_gravity;

	/// Number of degrees of freedom for this representation
	unsigned int m_numDof;

	/// Gravity enabled flag
	bool m_isGravityEnabled;

	/// Is this representation active or not ?
	bool m_isActive;

	/// Is this representation driving the SceneElement
	bool m_isDrivingElement;

};

};  // namespace Physics

};  // namespace SurgSim

#endif  // SURGSIM_PHYSICS_REPRESENTATION_H
