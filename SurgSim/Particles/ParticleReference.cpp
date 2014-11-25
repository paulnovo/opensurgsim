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

#include "SurgSim/Particles/ParticleReference.h"

#include "SurgSim/Particles/Particle.h"
#include "SurgSim/Particles/ParticlesState.h"


namespace SurgSim
{
namespace Particles
{

ParticleReference::ParticleReference(std::shared_ptr<ParticlesState> state, size_t index) :
	m_state(state),
	m_index(index)
{
}


void ParticleReference::operator=(const Particle& other)
{
	setPosition(other.getPosition());
	setVelocity(other.getVelocity());
	setLifetime(other.getLifetime());
}

bool ParticleReference::operator==(const ParticleReference& other) const
{
	return m_index == other.m_index && m_state.get() == other.m_state.get();
}

const Eigen::VectorBlock<SurgSim::Math::Vector, 3> ParticleReference::getPosition() const
{
	return m_state->getPositions().segment<3>(3 * m_index);
}

void ParticleReference::setPosition(const Eigen::Ref<const SurgSim::Math::Vector3d>& position)
{
	m_state->getPositions().segment<3>(3 * m_index) = position;
}

const Eigen::VectorBlock<SurgSim::Math::Vector, 3> ParticleReference::getVelocity() const
{
	return m_state->getVelocities().segment<3>(3 * m_index);
}

void ParticleReference::setVelocity(const Eigen::Ref<const SurgSim::Math::Vector3d>& velocity)
{
	m_state->getVelocities().segment<3>(3 * m_index) = velocity;
}

double ParticleReference::getLifetime() const
{
	return m_state->getLifetimes()[m_index];
}

void ParticleReference::setLifetime(double lifetime)
{
	m_state->getLifetimes()[m_index] = lifetime;
}

}; // namespace Particles
}; // namespace SurgSim
