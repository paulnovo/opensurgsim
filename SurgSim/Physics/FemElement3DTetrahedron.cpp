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

#include "SurgSim/Framework/Log.h"

#include "SurgSim/Physics/FemElement3DTetrahedron.h"
#include "SurgSim/Physics/DeformableRepresentationState.h"

#include "SurgSim/Math/Geometry.h"

using SurgSim::Math::getSubVector;
using SurgSim::Math::getSubMatrix;
using SurgSim::Math::addSubVector;
using SurgSim::Math::addSubMatrix;

using SurgSim::Math::Vector;
using SurgSim::Math::Vector3d;

namespace
{

/// Computes the determinant of 3 vectors
/// \param a, b, c The 3 vectors to compute the determinant from
/// \return |a b c|, The determinant of the 3 vectors a, b and c
double det(const Vector3d& a, const Vector3d& b, const Vector3d& c)
{
	return a[0]*b[1]*c[2] + a[2]*b[0]*c[1] + a[1]*b[2]*c[0] - a[2]*b[1]*c[0] - a[1]*b[0]*c[2] - a[0]*b[2]*c[1];
}

};

namespace SurgSim
{

namespace Physics
{

FemElement3DTetrahedron::FemElement3DTetrahedron(std::array<unsigned int, 4> nodeIds)
{
	setNumDofPerNode(3); // 3 dof per node (x, y, z)

	m_nodeIds.assign(std::begin(nodeIds), std::end(nodeIds));
}

void FemElement3DTetrahedron::initialize(const DeformableRepresentationState& state)
{
	// Test the validity of the physical parameters
	FemElement::initialize(state);

	for (auto nodeId = m_nodeIds.cbegin(); nodeId != m_nodeIds.cend(); nodeId++)
	{
		SURGSIM_ASSERT(*nodeId >= 0 && *nodeId < state.getNumNodes())
			<< "Invalid nodeId " << *nodeId << " expected in range [0.." << state.getNumNodes() - 1 << "]";
	}

	// Compute the fem tetrahedron shape functions Ni(x,y,z) = 1/6V ( ai + x.bi + y.ci + z.di )
	computeShapeFunctions(state);

	// Store the rest state for this tetrahedron in m_x0
	getSubVector(state.getPositions(), m_nodeIds, 3, &m_x0);

	// Verify the Counter clock-wise condition
	auto A = getSubVector(m_x0, 0, 3);
	auto B = getSubVector(m_x0, 1, 3);
	auto C = getSubVector(m_x0, 2, 3);
	auto D = getSubVector(m_x0, 3, 3);
	SurgSim::Math::Vector3d AB = B - A;
	SurgSim::Math::Vector3d AC = C - A;
	SurgSim::Math::Vector3d AD = D - A;
	SURGSIM_LOG_IF(AB.cross(AC).dot(AD) < 0, SurgSim::Framework::Logger::getDefaultLogger(), WARNING)
		<< "Tetrahedron ill-defined (ABC defined counter clock viewed from D) with node ids[" << m_nodeIds[0] << ", "
		<< m_nodeIds[1] << ", " << m_nodeIds[2] << ", " << m_nodeIds[3] << "]";

	// Pre-compute the mass and stiffness matrix
	computeMass(state, &m_M);
	computeStiffness(state, &m_K);
}

void FemElement3DTetrahedron::addForce(const DeformableRepresentationState& state,
									   const Eigen::Matrix<double, 12, 12>& k, SurgSim::Math::Vector* F, double scale)
{
	Eigen::Matrix<double, 12, 1> x, f;

	// K.U = Fext
	// K.(x - x0) = Fext
	// 0 = Fext + Fint     with Fint = -K.(x - x0)
	getSubVector(state.getPositions(), m_nodeIds, 3, &x);
	f = (- scale) * k * (x - m_x0);
	addSubVector(f, m_nodeIds, 3, F);
}

void FemElement3DTetrahedron::addForce(const DeformableRepresentationState& state, SurgSim::Math::Vector* F,
									   double scale)
{
	addForce(state, m_K, F, scale);
}

void FemElement3DTetrahedron::computeMass(const DeformableRepresentationState& state,
										  Eigen::Matrix<double, 12, 12, Eigen::DontAlign>* M)
{
	// From Przemieniecki book
	// -> section 11 "Inertia properties of structural elements
	//  -> subsection 8 "Solid Tetrahedron"
	//
	// On each axis (x, y, z), the mass matrix is
	// m = rho * volume / 20 * (2 1 1 1)
	//                         (1 2 1 1)
	//                         (1 1 2 1)
	//                         (1 1 1 2)
	//
	//         x1 y1 z1 x2 y2 z2 x3 y3 z3 x4 y4 z4
	//     x1 ( 2        1        1        1       )
	//     y1 (    2        1        1        1    )
	//     z1 (       2        1        1        1 )
	//     x2 ( 1        2        1        1       )
	//     y2 (    1        2        1        1    )
	// M = z2 (       1        2        1        1 ) * rho * volume / 20
	//     x3 ( 1        1        2        1       )
	//     y3 (    1        1        2        1    )
	//     z3 (       1        1        2        1 )
	//     x4 ( 1        1        1        2       )
	//     y4 (    1        1        1        2    )
	//     z4 (       1        1        1        2 )
	double coef = getVolume(state) * m_rho / 20.0;
	for (unsigned int rowNodeId = 0; rowNodeId < 4; rowNodeId++)
	{
		for (unsigned int colNodeId = 0; colNodeId < 4; colNodeId++)
		{
			auto Mii = getSubMatrix(*M, rowNodeId, colNodeId, 3, 3);

			if (rowNodeId == colNodeId)
			{
				Mii = SurgSim::Math::Matrix33d::Identity() * (2.0 * coef);
			}
			else
			{
				Mii = SurgSim::Math::Matrix33d::Identity() * coef;
			}
		}
	}
}


void FemElement3DTetrahedron::addMass(const DeformableRepresentationState& state, SurgSim::Math::Matrix* M,
									  double scale)
{
	addSubMatrix(m_M * scale, m_nodeIds, 3, M);
}

void FemElement3DTetrahedron::addDamping(const DeformableRepresentationState& state, SurgSim::Math::Matrix* D,
										 double scale)
{
}

void FemElement3DTetrahedron::computeStiffness(const DeformableRepresentationState& state,
											   Eigen::Matrix<double, 12, 12, Eigen::DontAlign>* k)
{
	m_Em.setZero();
	m_strain.setZero();

	// Compute the strain matrix
	double coef = 1.0 / (6.0 * m_restVolume);
	for(int i = 0; i < 4; i++)
	{
		m_strain(0, 3 * i    ) = coef * m_bi[i];
		m_strain(1, 3 * i + 1) = coef * m_ci[i];
		m_strain(2, 3 * i + 2) = coef * m_di[i];
		m_strain(3, 3 * i    ) = coef * m_ci[i];
		m_strain(3, 3 * i + 1) = coef * m_bi[i];
		m_strain(4 ,3 * i + 1) = coef * m_di[i];
		m_strain(4, 3 * i + 2) = coef * m_ci[i];
		m_strain(5 ,3 * i    ) = coef * m_di[i];
		m_strain(5, 3 * i + 2) = coef * m_bi[i];
	}

	// Compute the elasticity material matrix
	// which is commonly based on the Lame coefficients (1st = lambda, 2nd = mu = shear modulus):
	double lambda = m_E * m_nu / ((1.0 + m_nu) * (1.0 - 2.0 * m_nu));
	double mu = m_E / (2.0 * (1 + m_nu));
	m_Em(0, 0) = m_Em(1, 1) = m_Em(2, 2) = 2.0 * mu + lambda;
	m_Em(0, 1) = m_Em(0, 2) = m_Em(1, 0) = m_Em(1, 2) = m_Em(2, 0) = m_Em(2, 1) = lambda;
	m_Em(3, 3) = m_Em(4, 4) = m_Em(5, 5) = mu;

	// Compute the stress matrix
	m_stress = m_Em * m_strain;

	// Compute the stiffness matrix
	*k = m_restVolume * m_strain.transpose() * m_stress;

	// Ke is symmetric but numerical computation might introduce epsilon, we force the symmetry here
	*k = ((*k) + (*k).transpose()) * 0.5;
}

void FemElement3DTetrahedron::addStiffness(const DeformableRepresentationState& state, SurgSim::Math::Matrix* K,
										   double scale)
{
	addSubMatrix(m_K * scale, getNodeIds(), 3, K);
}

void FemElement3DTetrahedron::addFMDK(const DeformableRepresentationState& state,
	SurgSim::Math::Vector* F,
	SurgSim::Math::Matrix* M,
	SurgSim::Math::Matrix* D,
	SurgSim::Math::Matrix* K)
{
	// Assemble the mass matrix
	addMass(state, M);

	// No damping matrix as we are using linear elasticity (not visco-elasticity)

	// Assemble the stiffness matrix
	addStiffness(state, K);

	// Assemble the force vector
	addForce(state, F);
}

void FemElement3DTetrahedron::addMatVec(const DeformableRepresentationState& state,
										double alphaM, double alphaD, double alphaK,
										const SurgSim::Math::Vector& x, SurgSim::Math::Vector* F)
{
	using SurgSim::Math::addSubVector;
	using SurgSim::Math::getSubVector;

	if (alphaM == 0.0 && alphaK == 0.0)
	{
		return;
	}

	Eigen::Matrix<double, 12, 1, Eigen::DontAlign> xLoc, resLoc;
	getSubVector(x, m_nodeIds, 3, &xLoc);

	// Adds the mass contribution
	if (alphaM)
	{
		resLoc = alphaM * (m_M * xLoc);
		addSubVector(resLoc, m_nodeIds, 3, F);
	}

	// Adds the damping contribution (No damping)

	// Adds the stiffness contribution
	if (alphaK)
	{
		resLoc = alphaK * (m_K * xLoc);
		addSubVector(resLoc, m_nodeIds, 3, F);
	}
}

double FemElement3DTetrahedron::getVolume(const DeformableRepresentationState& state) const
{
	/// Computes the tetrahedron volume 1/6 * | 1 p0x p0y p0z |
	///                                       | 1 p1x p1y p1z |
	///                                       | 1 p2x p2y p2z |
	///                                       | 1 p3x p3y p3z |
	const Vector& x = state.getPositions();
	auto p0 = getSubVector(x, m_nodeIds[0], 3);
	auto p1 = getSubVector(x, m_nodeIds[1], 3);
	auto p2 = getSubVector(x, m_nodeIds[2], 3);
	auto p3 = getSubVector(x, m_nodeIds[3], 3);

	// fabs is necessary if we don't pay attention to the indexing !
	// If the tetrahedron verify ABC counter clock wise viewed from D, this determinant is always positive = 6V
	// i.e. dot( cross(AB, AC), AD ) > 0
	// Otherwise, it can happen that this determinant is negative = -6V !!
	return (det(p1, p2, p3) - det(p0, p2, p3) + det(p0, p1, p3) - det(p0, p1, p2)) / 6.0;
}

void FemElement3DTetrahedron::computeShapeFunctions(const DeformableRepresentationState& restState)
{
	// The tetrahedron nodes 3D position {a,b,c,d}
	Vector3d a = getSubVector(restState.getPositions(), m_nodeIds[0], 3);
	Vector3d b = getSubVector(restState.getPositions(), m_nodeIds[1], 3);
	Vector3d c = getSubVector(restState.getPositions(), m_nodeIds[2], 3);
	Vector3d d = getSubVector(restState.getPositions(), m_nodeIds[3], 3);

	m_restVolume = getVolume(restState);

	// See http://www.colorado.edu/engineering/CAS/courses.d/AFEM.d/AFEM.Ch09.d/AFEM.Ch09.pdf for more details.
	// Relationship between the notations in this source code and the document mentioned above:
	// a(x1 y1 z1)   b(x2 y2 z2)   c(x3 y3 z3)   d(x4 y4 z4)

	// Shape functions link the 3D space (x, y, z) to the natural parametrization (sigmai) of the shape.
	// (i.e. sigmai are the barycentric coordinates for the respective 4 nodes of the tetrahedon)
	// (1)   ( 1  1  1  1) (sigma1)
	// (x) = (x1 x2 x3 x4) (sigma2)
	// (y)   (y1 y2 y3 y4) (sigma3)
	// (z)   (z1 z2 z3 z4) (sigma4)
	//
	// The shape functions Ni(x, y, z) are given by the inverse relationship:
	// (sigma1)   ( 1  1  1  1)^-1 (1)        (m_a[0] m_b[0] m_c[0] m_d[0]) (1)       | 1  1  1  1|
	// (sigma2) = (x1 x2 x3 x4)    (x) = 1/6V (m_a[1] m_b[1] m_c[1] m_d[1]) (x) where |x1 x2 x3 x4| = 6V
	// (sigma3)   (y1 y2 y3 y4)    (y)        (m_a[2] m_b[2] m_c[2] m_d[2]) (y)       |y1 y2 y3 y4|
	// (sigma4)   (z1 z2 z3 z4)    (z)        (m_a[3] m_b[3] m_c[3] m_d[3]) (z)       |z1 z2 z3 z4|

	// Computes the shape functions parameters m_ai (noted 6V0i in the document mentioned above, eq 9.12)
	// m_ai[0] = 6V01 = 6V(origin,b,c,d) = x2(y3z4 - y4z3) + x3(y4z2 - y2z4) + x4(y2z3 - y3z2) =  |b c d|
	// m_ai[1] = 6V02 = 6V(origin,c,d,a) = x1(y4z3 - y3z4) + x3(y1z4 - y4z1) + x4(y3z1 - y1z3) = -|a c d|
	// m_ai[2] = 6V03 = 6V(origin,d,a,b) = x1(y2z4 - y4z2) + x2(y4z1 - y1z4) + x4(y1z2 - y2z1) =  |a b d|
	// m_ai[3] = 6V04 = 6V(origin,a,b,c) = x1(y3z2 - y2z3) + x2(y1z3 - y3z1) + x3(y2z1 - y1z2) = -|a b c|
	m_ai[0] =  det(b, c, d);
	m_ai[1] = -det(a, c, d);
	m_ai[2] =  det(a, b, d);
	m_ai[3] = -det(a, b, c);

	// Computes the shape function parameters m_bi (noted ai in the document mentioned above, eq 9.11)
	// m_bi[0] = y42z32 - y32z42 = (y4-y2)(z3-z2) - (y3-y2)(z4-z2) = |1 y2 z2| = |1 by bz|
	//                                                              -|1 y3 z3|  -|1 cy cz|
	//                                                               |1 y4 z4|   |1 dy dz|
	//
	// m_bi[1] = y31z43 - y34z13 = (y3-y1)(z4-z3) - (y3-y4)(z1-z3) = |1 y1 z1| = |1 ay az|
	//                                                               |1 y3 z3|   |1 cy cz|
	//                                                               |1 y4 z4|   |1 dy dz|
	//
	// m_bi[2] = y24z14 - y14z24 = (y2-y4)(z1-z4) - (y1-y4)(z2-z4) = |1 y1 z1| = |1 ay az|
	//                                                              -|1 y2 z2|  -|1 by bz|
	//                                                               |1 y4 z4|   |1 dy dz|
	//
	// m_bi[3] = y13z21 - y12z31 = (y1-y3)(z2-z1) - (y1-y2)(z3-z1) = |1 y1 z1| = |1 ay az|
	//                                                               |1 y2 z2|   |1 by bz|
	//                                                               |1 y3 z3|   |1 cy cz|
	{
		Vector3d atilde(1, a[1], a[2]);
		Vector3d btilde(1, b[1], b[2]);
		Vector3d ctilde(1, c[1], c[2]);
		Vector3d dtilde(1, d[1], d[2]);
		m_bi[0] = -det(btilde, ctilde, dtilde);
		m_bi[1] =  det(atilde, ctilde, dtilde);
		m_bi[2] = -det(atilde, btilde, dtilde);
		m_bi[3] =  det(atilde, btilde, ctilde);
	}

	// Computes the shape function parameters m_ci (noted bi in the document mentioned above, eq 9.11)
	// m_ci[0] = x32z42 - x42z32 = (x3-x2)(z4-z2) - (x4-x2)(z3-z2) = |1 x2 z2| = |1 bx bz|
	//                                                               |1 x3 z3|   |1 cx cz|
	//                                                               |1 x4 z4|   |1 dx dz|
	//
	// m_ci[1] = x43z31 - x13z34 = (x4-x3)(z3-z1) - (x1-x3)(z3-z4) = |1 x1 z1| = |1 ax az|
	//                                                              -|1 x3 z3|  -|1 cx cz|
	//                                                               |1 x4 z4|   |1 dx dz|
	//
	// m_ci[2] = x14z24 - x24z14 = (x1-x4)(z2-z4) - (x2-x4)(z1-z4) = |1 x1 z1| = |1 ax az|
	//                                                               |1 x2 z2|   |1 bx bz|
	//                                                               |1 x4 z4|   |1 dx dz|
	//
	// m_ci[3] = x21z13 - x31z12 = (x2-x1)(z1-z3) - (x3-x1)(z1-z2) = |1 x1 z1| = |1 ax az|
	//                                                              -|1 x2 z2|  -|1 bx bz|
	//                                                               |1 x3 z3|   |1 cx cz|
	{
		Vector3d atilde(1, a[0], a[2]);
		Vector3d btilde(1, b[0], b[2]);
		Vector3d ctilde(1, c[0], c[2]);
		Vector3d dtilde(1, d[0], d[2]);
		m_ci[0] =  det(btilde, ctilde, dtilde);
		m_ci[1] = -det(atilde, ctilde, dtilde);
		m_ci[2] =  det(atilde, btilde, dtilde);
		m_ci[3] = -det(atilde, btilde, ctilde);
	}

	// Computes the shape function parameters m_di (noted ci in the document mentioned above, eq 9.11)
	// m_di[0] = x42y32 - x32y42 = (x4-x2)(y3-y2) - (x3-x2)(y4-y2) =  |1 x2 y2| =  |1 bx by|
	//                                                               -|1 x3 y3|   -|1 cx cy|
	//                                                                |1 x4 y4|    |1 dx dy|
	//
	// m_di[1] = x31y43 - x34y13 = (x3-x1)(y4-y3) - (x3-x4)(y1-y3) =  |1 x1 y1| =  |1 ax ay|
	//                                                                |1 x3 y3|    |1 cx cy|
	//                                                                |1 x4 y4|    |1 dx dy|
	//
	// m_di[2] = x24y14 - x14y24 = (x2-x4)(y1-y4) - (x1-x4)(y2-y4) =  |1 x1 y1| =  |1 ax ay|
	//                                                               -|1 x2 y2|   -|1 bx by|
	//                                                                |1 x4 y4|    |1 dx dy|
	//
	// m_di[3] = x13y21 - x12y31 = (x1-x3)(y2-y1) - (x1-x2)(y3-y1) =  |1 x1 y1| =  |1 ax ay|
	//                                                                |1 x2 y2|    |1 bx by|
	//                                                                |1 x3 y3|    |1 cx cy|
	{
		Vector3d atilde(1, a[0], a[1]);
		Vector3d btilde(1, b[0], b[1]);
		Vector3d ctilde(1, c[0], c[1]);
		Vector3d dtilde(1, d[0], d[1]);
		m_di[0] = -det(btilde, ctilde, dtilde);
		m_di[1] =  det(atilde, ctilde, dtilde);
		m_di[2] = -det(atilde, btilde, dtilde);
		m_di[3] =  det(atilde, btilde, ctilde);
	}
}

bool FemElement3DTetrahedron::isValidCoordinate(const SurgSim::Math::Vector& naturalCoordinate) const
{
	return (std::abs(naturalCoordinate.sum() - 1.0) < SurgSim::Math::Geometry::ScalarEpsilon)
		&& (naturalCoordinate.size() == 4);
}

SurgSim::Math::Vector FemElement3DTetrahedron::computeCartesianCoordinate(
	const DeformableRepresentationState& state,
	const SurgSim::Math::Vector& naturalCoordinate) const
{
	SURGSIM_ASSERT(isValidCoordinate(naturalCoordinate))
		<< "naturalCoordinate must be normalized and length 4.";

	const Vector& x = state.getPositions();
	Vector3d p0 = getSubVector(x, m_nodeIds[0], 3);
	Vector3d p1 = getSubVector(x, m_nodeIds[1], 3);
	Vector3d p2 = getSubVector(x, m_nodeIds[2], 3);
	Vector3d p3 = getSubVector(x, m_nodeIds[3], 3);

	return naturalCoordinate(0) * p0
		 + naturalCoordinate(1) * p1
		 + naturalCoordinate(2) * p2
		 + naturalCoordinate(3) * p3;
}

} // namespace Physics

} // namespace SurgSim
