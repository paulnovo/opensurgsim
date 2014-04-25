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

#include "SurgSim/Framework/Assert.h"
#include "SurgSim/Framework/Log.h"

#include "SurgSim/Physics/MassSpringRepresentation.h"

#include "SurgSim/Math/Vector.h"
#include "SurgSim/Math/Matrix.h"
#include "SurgSim/Math/Valid.h"

using SurgSim::Math::Vector;
using SurgSim::Math::DiagonalMatrix;
using SurgSim::Math::Matrix;

namespace SurgSim
{

namespace Physics
{

MassSpringRepresentation::MassSpringRepresentation(const std::string& name) :
	DeformableRepresentation(name)
{
	m_rayleighDamping.massCoefficient = 0.0;
	m_rayleighDamping.stiffnessCoefficient = 0.0;

	// Reminder: m_numDofPerNode is held by DeformableRepresentation
	// but needs to be set by all concrete derived classes
	m_numDofPerNode = 3;
}

MassSpringRepresentation::~MassSpringRepresentation()
{
}

void MassSpringRepresentation::addMass(const std::shared_ptr<Mass> mass)
{
	m_masses.push_back(mass);
}

void MassSpringRepresentation::addSpring(const std::shared_ptr<Spring> spring)
{
	m_springs.push_back(spring);
}

unsigned int MassSpringRepresentation::getNumMasses() const
{
	return m_masses.size();
}

unsigned int MassSpringRepresentation::getNumSprings() const
{
	return m_springs.size();
}

std::shared_ptr<Mass> MassSpringRepresentation::getMass(unsigned int nodeId)
{
	SURGSIM_ASSERT(nodeId < getNumMasses()) << "Invalid node id to request a mass from";
	return m_masses[nodeId];
}

std::shared_ptr<Spring> MassSpringRepresentation::getSpring(unsigned int springId)
{
	SURGSIM_ASSERT(springId < getNumSprings()) << "Invalid spring id";
	return m_springs[springId];
}

double MassSpringRepresentation::getTotalMass() const
{
	double mass = 0.0;
	for (auto it = std::begin(m_masses); it != std::end(m_masses); it++)
	{
		mass += (*it)->getMass();
	}
	return mass;
}

double MassSpringRepresentation::getRayleighDampingStiffness() const
{
	return m_rayleighDamping.stiffnessCoefficient;
}

double MassSpringRepresentation::getRayleighDampingMass() const
{
	return m_rayleighDamping.massCoefficient;
}

void MassSpringRepresentation::setRayleighDampingStiffness(double stiffnessCoef)
{
	m_rayleighDamping.stiffnessCoefficient = stiffnessCoef;
}

void MassSpringRepresentation::setRayleighDampingMass(double massCoef)
{
	m_rayleighDamping.massCoefficient = massCoef;
}

RepresentationType MassSpringRepresentation::getType() const
{
	return REPRESENTATION_TYPE_MASSSPRING;
}

void MassSpringRepresentation::beforeUpdate(double dt)
{
	if (! isActive())
	{
		return;
	}

	SURGSIM_ASSERT(3 * getNumMasses() == getNumDof()) <<
		"Mismatch between the number of masses ("<<getNumMasses()<<") and the number of dof ("<<getNumDof()<<")";
	SURGSIM_ASSERT(getNumMasses()) << "No masses specified yet, call addMass() prior to running the simulation";
	SURGSIM_ASSERT(getNumSprings()) << "No springs specified yet, call addSpring() prior to running the simulation";
	SURGSIM_ASSERT(getNumDof()) <<
		"State has not been initialized yet, call setInitialState() prior to running the simulation";

	// Call the DeformableRepresentation implementation to take care of the OdeSolver setup
	DeformableRepresentation::beforeUpdate(dt);
}

void MassSpringRepresentation::update(double dt)
{
	if (! isActive())
	{
		return;
	}

	SURGSIM_ASSERT(m_odeSolver != nullptr) <<
		"Ode solver has not been set yet. Did you call beforeUpdate() ?";
	SURGSIM_ASSERT(m_initialState != nullptr) <<
		"Initial state has not been set yet. Did you call setInitialState() ?";

	// Solve the ode
	m_odeSolver->solve(dt, *m_currentState, m_newState.get());

	// Back up the current state into the previous state (by swapping)
	m_currentState.swap(m_previousState);
	// Make the new state, the current state (by swapping)
	m_currentState.swap(m_newState);
}

void MassSpringRepresentation::afterUpdate(double dt)
{
	if (! isActive())
	{
		return;
	}

	if ( !isValidState(*m_currentState))
	{
		deactivateAndReset();
		return;
	}

	// Back up the current state into the final state
	*m_finalState = *m_currentState;
}

void MassSpringRepresentation::applyCorrection(double dt, const Eigen::VectorBlock<Vector>& deltaVelocity)
{
	if ( !isActive())
	{
		return;
	}

	m_currentState->getPositions() += deltaVelocity * dt;
	m_currentState->getVelocities() += deltaVelocity;

	if ( !isValidState(*m_currentState))
	{
		deactivateAndReset();
	}
}

Vector& MassSpringRepresentation::computeF(const DeformableRepresentationState& state)
{
	// Make sure the force vector has been properly allocated and zeroed out
	SurgSim::Math::resizeVector(&m_f, state.getNumDof(), true);

	addGravityForce(&m_f, state);
	addRayleighDampingForce(&m_f, state);
	addSpringsForce(&m_f, state);

	// Apply boundary conditions globally
	for (auto boundaryCondition = std::begin(state.getBoundaryConditions());
		boundaryCondition != std::end(state.getBoundaryConditions());
		boundaryCondition++)
	{
		m_f[*boundaryCondition] = 0.0;
	}

	return m_f;
}

const DiagonalMatrix& MassSpringRepresentation::computeM(const DeformableRepresentationState& state)
{
	using SurgSim::Math::Vector3d;
	using SurgSim::Math::setSubVector;

	// Make sure the mass matrix has been properly allocated
	// It does not need to be zeroed out, as it will be directly set
	SurgSim::Math::resizeMatrix(&m_M, state.getNumDof(), state.getNumDof(), false);

	DiagonalMatrix::DiagonalVectorType& diagonal = m_M.diagonal();

	for (unsigned int massId = 0; massId < getNumMasses(); massId++)
	{
		setSubVector(Vector3d::Ones() * getMass(massId)->getMass(), massId, 3, &diagonal);
	}

	// Apply boundary conditions globally
	for (auto boundaryCondition = std::begin(state.getBoundaryConditions());
		boundaryCondition != std::end(state.getBoundaryConditions());
		boundaryCondition++)
	{
		diagonal[*boundaryCondition] = 1e9;
	}

	return m_M;
}

const Matrix& MassSpringRepresentation::computeD(const DeformableRepresentationState& state)
{
	using SurgSim::Math::Vector3d;
	using SurgSim::Math::setSubVector;
	using SurgSim::Math::addSubMatrix;

	const double& rayleighStiffness = m_rayleighDamping.stiffnessCoefficient;
	const double& rayleighMass = m_rayleighDamping.massCoefficient;

	// Make sure the damping matrix has been properly allocated and zeroed out
	SurgSim::Math::resizeMatrix(&m_D, state.getNumDof(), state.getNumDof(), true);

	// D += rayleighMass.M
	if (rayleighMass != 0.0)
	{
		for (unsigned int massId = 0; massId < getNumMasses(); massId++)
		{
			double coef = rayleighMass * getMass(massId)->getMass();
			Matrix::DiagonalReturnType Ddiagonal = m_D.diagonal();
			setSubVector(Vector3d::Ones() * coef, massId, 3, &Ddiagonal);
		}
	}

	// D += rayleighStiffness.K
	if (rayleighStiffness != 0.0)
	{
		for (auto spring = std::begin(m_springs); spring != std::end(m_springs); spring++)
		{
			(*spring)->addStiffness(state, &m_D, rayleighStiffness);
		}
	}

	// D += Springs damping matrix
	for (auto spring = std::begin(m_springs); spring != std::end(m_springs); spring++)
	{
		(*spring)->addDamping(state, &m_D);
	}

	// Apply boundary conditions globally
	for (auto boundaryCondition = std::begin(state.getBoundaryConditions());
		boundaryCondition != std::end(state.getBoundaryConditions());
		boundaryCondition++)
	{
		m_D.block(*boundaryCondition, 0, 1, getNumDof()).setZero();
		m_D.block(0, *boundaryCondition, getNumDof(), 1).setZero();
		m_D(*boundaryCondition, *boundaryCondition) = 1e9;
	}

	return m_D;
}

const Matrix& MassSpringRepresentation::computeK(const DeformableRepresentationState& state)
{
	using SurgSim::Math::addSubMatrix;

	// Make sure the stiffness matrix has been properly allocated and zeroed out
	SurgSim::Math::resizeMatrix(&m_K, state.getNumDof(), state.getNumDof(), true);

	for (auto spring = std::begin(m_springs); spring != std::end(m_springs); spring++)
	{
		(*spring)->addStiffness(state, &m_K);
	}

	// Apply boundary conditions globally
	for (auto boundaryCondition = std::begin(state.getBoundaryConditions());
		boundaryCondition != std::end(state.getBoundaryConditions());
		boundaryCondition++)
	{
		m_K.block(*boundaryCondition, 0, 1, getNumDof()).setZero();
		m_K.block(0, *boundaryCondition, getNumDof(), 1).setZero();
		m_K(*boundaryCondition, *boundaryCondition) = 1e9;
	}

	return m_K;
}

void MassSpringRepresentation::computeFMDK(const DeformableRepresentationState& state,
	Vector** f, DiagonalMatrix** M, Matrix** D, Matrix** K)
{
	using SurgSim::Math::addSubVector;
	using SurgSim::Math::addSubMatrix;

	// Make sure the force vector has been properly allocated and zeroed out
	SurgSim::Math::resizeVector(&m_f, state.getNumDof(), true);

	// Make sure the mass matrix has been properly allocated
	// It does not need to be zeroed out, as it will be directly set
	SurgSim::Math::resizeMatrix(&m_M, state.getNumDof(), state.getNumDof(), false);

	// Make sure the damping matrix has been properly allocated and zeroed out
	SurgSim::Math::resizeMatrix(&m_D, state.getNumDof(), state.getNumDof(), true);

	// Make sure the stiffness matrix has been properly allocated and zeroed out
	SurgSim::Math::resizeMatrix(&m_K, state.getNumDof(), state.getNumDof(), true);

	// Computes the mass matrix m_M
	computeM(state);

	// Computes the stiffness matrix m_K
	// Add the springs damping matrix to m_D
	// Add the springs force to m_f
	for (auto spring = std::begin(m_springs); spring != std::end(m_springs); spring++)
	{
		(*spring)->addFDK(state, &m_f, &m_D, &m_K);
	}

	// Add the Rayleigh damping matrix
	if (m_rayleighDamping.massCoefficient)
	{
		m_D.diagonal() += m_M.diagonal() * m_rayleighDamping.massCoefficient;
	}
	if (m_rayleighDamping.stiffnessCoefficient)
	{
		m_D += m_K * m_rayleighDamping.stiffnessCoefficient;
	}

	// Add the gravity to m_f
	addGravityForce(&m_f, state);

	// Add the Rayleigh damping force to m_f (using the damping matrix)
	addRayleighDampingForce(&m_f, state, true);

	// Apply boundary conditions globally
	for (auto boundaryCondition = std::begin(state.getBoundaryConditions());
		boundaryCondition != std::end(state.getBoundaryConditions());
		boundaryCondition++)
	{
		m_M.diagonal()[*boundaryCondition] = 1e9;

		m_D.block(*boundaryCondition, 0, 1, getNumDof()).setZero();
		m_D.block(0, *boundaryCondition, getNumDof(), 1).setZero();
		m_D(*boundaryCondition, *boundaryCondition) = 1e9;

		m_K.block(*boundaryCondition, 0, 1, getNumDof()).setZero();
		m_K.block(0, *boundaryCondition, getNumDof(), 1).setZero();
		m_K(*boundaryCondition, *boundaryCondition) = 1e9;

		m_f[*boundaryCondition] = 0.0;
	}

	*f = &m_f;
	*M = &m_M;
	*D = &m_D;
	*K = &m_K;
}

void MassSpringRepresentation::addRayleighDampingForce(Vector* force, const DeformableRepresentationState& state,
	bool useGlobalDampingMatrix, bool useGlobalStiffnessMatrix, bool useGlobalMassMatrix, double scale)
{
	using SurgSim::Math::getSubVector;
	using SurgSim::Math::addSubVector;
	using SurgSim::Math::getSubMatrix;

	// Temporary variables for convenience
	double& rayleighMass = m_rayleighDamping.massCoefficient;
	double& rayleighStiffness = m_rayleighDamping.stiffnessCoefficient;
	const Vector& v = state.getVelocities();

	// If we have the damping matrix build (D = rayleighMass.M + rayleighStiffness.K), F = -D.v(t)
	if (useGlobalDampingMatrix && (rayleighStiffness != 0.0 || rayleighMass != 0.0))
	{
		*force -= scale * (m_D * v);
	}
	else // Otherwise we unroll the calculation separately on the mass and stiffness components
	{
		// Rayleigh damping mass: F = - rayleighMass.M.v(t)
		if (rayleighMass != 0.0)
		{
			// If we have the mass matrix, we can compute directly F = -rayleighMass.M.v(t)
			if (useGlobalMassMatrix)
			{
				// M is diagonal, take advantage of it...
				*force -= (scale * rayleighMass) * (m_M.diagonal().cwiseProduct(v));
			}
			else
			{
				for (unsigned int nodeID = 0; nodeID < getNumMasses(); nodeID++)
				{
					double mass = getMass(nodeID)->getMass();
					SurgSim::Math::Vector3d f = - scale * rayleighMass * mass * getSubVector(v, nodeID, 3);
					addSubVector(f, nodeID, 3, force);
				}
			}
		}

		// Rayleigh damping stiffness: F = - rayleighStiffness.K.v(t)
		if (rayleighStiffness != 0.0)
		{
			if (useGlobalStiffnessMatrix)
			{
				*force -= scale * rayleighStiffness * (m_K * v);
			}
			else
			{
				// Otherwise, we loop through each fem element to compute its contribution
				for (auto spring = std::begin(m_springs); spring != std::end(m_springs); ++spring)
				{
					(*spring)->addMatVec(state, 0.0, - scale * rayleighStiffness, v, force);
				}
			}
		}
	}
}

void MassSpringRepresentation::addSpringsForce(Vector *force, const DeformableRepresentationState& state, double scale)
{
	for (auto spring = std::begin(m_springs); spring != std::end(m_springs); spring++)
	{
		(*spring)->addForce(state, force, scale);
	}
}

void MassSpringRepresentation::addGravityForce(Vector *f, const DeformableRepresentationState& state, double scale)
{
	using SurgSim::Math::addSubVector;

	if (isGravityEnabled())
	{
		for (unsigned int massId = 0; massId < getNumMasses(); massId++)
		{
			addSubVector(getGravity() * getMass(massId)->getMass(), massId, 3, f);
		}
	}
}

static void transformVectorByBlockOf3(const SurgSim::Math::RigidTransform3d& transform,
									  Vector* x, bool rotationOnly = false)
{
	unsigned int numNodes = x->size() / 3;
	SURGSIM_ASSERT(static_cast<int>(numNodes * 3) == x->size()) <<
		"Unexpected number of dof in a MassSpring state vector (not a multiple of 3)";

	for (unsigned int nodeId = 0; nodeId < numNodes; nodeId++)
	{
		SurgSim::Math::Vector3d xi = SurgSim::Math::getSubVector(*x, nodeId, 3);
		SurgSim::Math::Vector3d xiTransformed;
		if (rotationOnly)
		{
			xiTransformed = transform.linear() * xi;
		}
		else
		{
			xiTransformed = transform * xi;
		}
		SurgSim::Math::setSubVector(xiTransformed, nodeId, 3, x);
	}
}

void MassSpringRepresentation::transformState(std::shared_ptr<DeformableRepresentationState> state,
	const SurgSim::Math::RigidTransform3d& transform)
{
	transformVectorByBlockOf3(transform, &state->getPositions());
	transformVectorByBlockOf3(transform, &state->getVelocities(), true);
	transformVectorByBlockOf3(transform, &state->getAccelerations(), true);
}

bool MassSpringRepresentation::isValidState(const DeformableRepresentationState &state) const
{
	return SurgSim::Math::isValid(state.getPositions())
		&& SurgSim::Math::isValid(state.getVelocities());
}

void MassSpringRepresentation::deactivateAndReset(void)
{
	SURGSIM_LOG(SurgSim::Framework::Logger::getDefaultLogger(), DEBUG)
		<< getName() << " deactivated and reset:" << std::endl
		<< "position=(" << m_currentState->getPositions() << ")" << std::endl
		<< "velocity=(" << m_currentState->getVelocities() << ")" << std::endl
		<< "acceleration=(" << m_currentState->getAccelerations() << ")" << std::endl;

	resetState();
	setIsActive(false);
}

} // namespace Physics

} // namespace SurgSim
