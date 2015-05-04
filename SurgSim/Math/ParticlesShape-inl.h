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
//

#ifndef SURGSIM_MATH_PARTICLESSHAPE_INL_H
#define SURGSIM_MATH_PARTICLESSHAPE_INL_H

namespace SurgSim
{
namespace Math
{

template <class VertexData>
ParticlesShape::ParticlesShape(const SurgSim::DataStructures::Vertices<VertexData>& other)
{
	getVertices().reserve(other.getVertices().size());
	for (auto& otherVertex : other.getVertices())
	{
		addVertex(VertexType(otherVertex.position));
	}
	update();
}

}; // namespace Math
}; // namespace SurgSim

#endif
