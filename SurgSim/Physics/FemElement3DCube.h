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

#ifndef SURGSIM_PHYSICS_FEMELEMENT3DCUBE_H
#define SURGSIM_PHYSICS_FEMELEMENT3DCUBE_H

#include <array>

#include "SurgSim/Physics/FemElement.h"

namespace SurgSim
{

namespace Physics
{

/// Class for Fem Element 3D based on a cube volume discretization
/// \note The stiffness property of the cube is derived from
/// \note http://www.colorado.edu/engineering/CAS/courses.d/AFEM.d/AFEM.Ch11.d/AFEM.Ch11.pdf
/// \note The mass property of the cube is derived from the kinetic energy computed on the cube's volume
/// \note (c.f. internal documentation on cube mass matrix computation for details).
class FemElement3DCube : public FemElement
{
public:
	/// Constructor
	/// \param nodeIds An array of 8 nodes ids defining this cube element in a overall mesh
	/// \param restState The rest state to initialize the cube with
	/// \note The 8 nodes must be valid node ids in the rest state, if they aren't an ASSERT will be raised
	/// \note The 8 nodes must define a cube with positive volume, if they don't an ASSERT will be raised
	/// \note In order to do so (looking at the cube from the exterior, face normal 'n' pointing outward),
	/// \note   the 1st  4 nodeIds (ABCD) should define any face CW            i.e. (AB^AC or AB^AD or AC^AD).n < 0
	/// \note   the last 4 nodeIds (EFGH) should define the opposite face CCW  i.e. (EF^EG or EF^EH or EG^EH).n > 0
	FemElement3DCube(std::array<unsigned int, 8> nodeIds, const DeformableRepresentationState& restState);

	/// Initializes the element once everything has been set
	/// \param state The state to initialize the FemElement with
	/// \note We use the theory of linear elasticity, so this method pre-compute the stiffness and mass matrices
	virtual void initialize(const DeformableRepresentationState& state) override;

	/// Gets the element volume based on the input state
	/// \param state The deformable state to compute the volume with
	virtual double getVolume(const DeformableRepresentationState& state) const override;

	/// Adds the element force (computed for a given state) to a complete system force vector F (assembly)
	/// \param state The state to compute the force with
	/// \param[in,out] F The complete system force vector to add the element force into
	/// \param scale A factor to scale the added force with
	/// \note The element force is of size (getNumDofPerNode() x getNumNodes())
	/// \note This method supposes that the incoming state contains information with the same number of dof
	/// \note per node as getNumDofPerNode()
	virtual void addForce(const DeformableRepresentationState& state, SurgSim::Math::Vector* F,
		double scale = 1.0) override;

	/// Adds the element mass matrix M (computed for a given state) to a complete system mass matrix M (assembly)
	/// \param state The state to compute the mass matrix with
	/// \param[in,out] M The complete system mass matrix to add the element mass-matrix into
	/// \param scale A factor to scale the added mass matrix with
	/// \note The element mass matrix is square of size getNumDofPerNode() x getNumNodes()
	/// \note This method supposes that the incoming state contains information with the same number of
	/// \note dof per node as getNumDofPerNode()
	virtual void addMass(const DeformableRepresentationState& state, SurgSim::Math::Matrix* M,
		double scale = 1.0) override;

	/// Adds the element damping matrix D (= -df/dv) (comuted for a given state)
	/// to a complete system damping matrix D (assembly)
	/// \param state The state to compute the damping matrix with
	/// \param[in,out] D The complete system damping matrix to add the element damping matrix into
	/// \param scale A factor to scale the added damping matrix with
	/// \note The element damping matrix is square of size getNumDofPerNode() x getNumNodes()
	/// \note This method supposes that the incoming state contains information with the same number of
	/// \note dof per node as getNumDofPerNode()
	/// \note FemElement3DCube uses linear elasticity (not visco-elasticity), so it does not give any damping.
	virtual void addDamping(const DeformableRepresentationState& state, SurgSim::Math::Matrix* D,
		double scale = 1.0) override;

	/// Adds the element stiffness matrix K (= -df/dx) (computed for a given state)
	/// to a complete system stiffness matrix K (assembly)
	/// \param state The state to compute the stiffness matrix with
	/// \param[in,out] K The complete system stiffness matrix to add the element stiffness matrix into
	/// \param scale A factor to scale the added stiffness matrix with
	/// \note The element stiffness matrix is square of size getNumDofPerNode() x getNumNodes()
	/// \note This method supposes that the incoming state contains information with the same number of
	/// \note dof per node as getNumDofPerNode()
	virtual void addStiffness(const DeformableRepresentationState& state, SurgSim::Math::Matrix* K,
		double scale = 1.0) override;

	/// Adds the element force vector, mass, stiffness and damping matrices (computed for a given state)
	/// into a complete system data structure F, M, D, K (assembly)
	/// \param state The state to compute everything with
	/// \param[in,out] F The complete system force vector to add the element force into
	/// \param[in,out] M The complete system mass matrix to add the element mass matrix into
	/// \param[in,out] D The complete system damping matrix to add the element damping matrix into
	/// \param[in,out] K The complete system stiffness matrix to add the element stiffness matrix into
	/// \note This method supposes that the incoming state contains information with the same number of dof
	/// \note per node as getNumDofPerNode()
	virtual void addFMDK(const DeformableRepresentationState& state,
		SurgSim::Math::Vector* F,
		SurgSim::Math::Matrix* M,
		SurgSim::Math::Matrix* D,
		SurgSim::Math::Matrix* K) override;

	/// Adds the element matrix-vector contribution F += (alphaM.M + alphaD.D + alphaK.K).x (computed for a given state)
	/// into a complete system data structure F (assembly)
	/// \param state The state to compute everything with
	/// \param alphaM The scaling factor for the mass contribution
	/// \param alphaD The scaling factor for the damping contribution
	/// \param alphaK The scaling factor for the stiffness contribution
	/// \param x A complete system vector to use as the vector in the matrix-vector multiplication
	/// \param[in,out] F The complete system force vector to add the element matrix-vector contribution into
	/// \note This method supposes that the incoming state contains information with the same number of dof
	/// \note per node as getNumDofPerNode()
	virtual void addMatVec(const DeformableRepresentationState& state,
		double alphaM, double alphaD, double alphaK,
		const SurgSim::Math::Vector& x, SurgSim::Math::Vector* F) override;

protected:
	/// Computes the cube stiffness matrix along with the strain and stress matrices
	/// \param state The deformable state to compute the stiffness matrix from
	/// \param[out] strain, stress, k The strain, stress and stiffness matrices to store the result into
	void computeStiffness(const DeformableRepresentationState& state,
		Eigen::Matrix<double, 6, 24, Eigen::DontAlign>* strain,
		Eigen::Matrix<double, 6, 24, Eigen::DontAlign>* stress,
		Eigen::Matrix<double, 24, 24, Eigen::DontAlign>* k);

	/// Computes the cube mass matrix
	/// \param state The deformable state to compute the mass matrix from
	/// \param[out] m The mass matrix to store the result into
	void computeMass(const DeformableRepresentationState& state,
		Eigen::Matrix<double, 24, 24, Eigen::DontAlign>* m);

	/// Adds the element force (computed for a given state) to a complete system force vector F (assembly)
	/// This method relies on a given stiffness matrix and does not evaluate it from the state
	/// \param state The state to compute the force with
	/// \param k The given element stiffness matrix
	/// \param[in,out] F The complete system force vector to add the element force into
	/// \param scale A factor to scale the added force with
	/// \note The element force is of size (getNumDofPerNode() x getNumNodes())
	/// \note This method supposes that the incoming state contains information with the same number of dof
	/// \note per node as getNumDofPerNode()
	void addForce(const DeformableRepresentationState& state, const Eigen::Matrix<double, 24, 24>& k,
		SurgSim::Math::Vector* F, double scale = 1.0);

	/// Helper method to evaluate strain-stress and stiffness integral terms with a discrete sum using
	/// a Gauss quadrature rule
	/// \param state The state to compute the evaluation with
	/// \param epsilon, eta, mu The 3D parametric coordinates to evaluate the data at (within \f$[-1 +1]\f$)
	/// \param weightEpsilon, weightEta, weightMu The weight to apply to this evaluation point in the Gauss quadrature
	/// \param[out] strain, stress, k The matrices in which to add the evaluations
	void addStrainStressStiffnessAtPoint(const DeformableRepresentationState& state,
		double epsilon, double eta, double mu,
		double weightEpsilon, double weightEta, double weightMu,
		Eigen::Matrix<double, 6, 24, Eigen::DontAlign>* strain,
		Eigen::Matrix<double, 6, 24, Eigen::DontAlign>* stress,
		Eigen::Matrix<double, 24, 24, Eigen::DontAlign>* k);

	/// Helper method to evaluate mass integral terms with a discrete sum using a Gauss quadrature rule
	/// \param state The state to compute the evaluation with
	/// \param epsilon, eta, mu The 3D parametric coordinates to evaluate the data at (within \f$[-1 +1]\f$)
	/// \param weightEpsilon, weightEta, weightMu The weight to apply to this evaluation point in the Gauss quadrature
	/// \param[out] m The matrix in which to add the evaluations
	void addMassMatrixAtPoint(const DeformableRepresentationState& state,
		double epsilon, double eta, double mu,
		double weightEpsilon, double weightEta, double weightMu,
		Eigen::Matrix<double, 24, 24, Eigen::DontAlign>* m);

	/// Helper method to evaluate matrix J = d(x,y,z)/d(epsilon,eta,mu) at a given 3D parametric location
	/// J expresses the 3D space coordinate frames variation w.r.t. parametric coordinates
	/// \param state The state to compute the evaluation with
	/// \param epsilon, eta, mu The 3D parametric coordinates to evaluate the data at (within \f$[-1 +1]\f$)
	/// \param[out] J, Jinv, detJ The J matrix with its inverse and determinant evaluated at (epsilon, eta, mu)
	void evaluateJ(const DeformableRepresentationState& state, double epsilon, double eta, double mu,
		SurgSim::Math::Matrix33d *J,
		SurgSim::Math::Matrix33d *Jinv,
		double *detJ) const;

	/// Helper method to evaluate the strain-displacement matrix at a given 3D parametric location
	/// c.f. http://www.colorado.edu/engineering/CAS/courses.d/AFEM.d/AFEM.Ch11.d/AFEM.Ch11.pdf for more details
	/// \param epsilon, eta, mu The 3D parametric coordinates to evaluate the data at (within \f$[-1 +1]\f$)
	/// \param Jinv The inverse of matrix J (3D global coords to 3D parametric coords)
	/// \param[out] B The strain-displacement matrix
	void evaluateStrainDisplacement(double epsilon, double eta, double mu, const SurgSim::Math::Matrix33d& Jinv,
		Eigen::Matrix<double, 6, 24, Eigen::DontAlign> *B) const;

	/// Cube rest volume
	double m_restVolume;

	///@{
	/// Shape functions parameters
	/// \f$N_i(\epsilon, \eta, \mu) = (1\pm\epsilon)(1\pm\eta)(1\pm\mu)/8
	///   = (1+\epsilon.sign(\epsilon_i))(1+\eta.sign(\eta_i))(1+\mu.sign(\mu_i))/8
	///   \textbf{ with } (\epsilon, \eta, \mu) \in [-1 +1]^3\f$
	///
	/// We choose to only store the sign of epsilon, eta and mu for each shape functions.
	/// \sa N
	std::array<double, 8> m_shapeFunctionsEpsilonSign;
	std::array<double, 8> m_shapeFunctionsEtaSign;
	std::array<double, 8> m_shapeFunctionsMuSign;
	///@}

	/// Shape functions \f$N_i(\epsilon, \eta, \mu) = (1\pm\epsilon)(1\pm\eta)(1\pm\mu)/8\f$
	///
	/*! \f$
	 *  \begin{array}{r | r r r}
	 *  i & sign(\epsilon) & sign(\eta) & sign(\mu) \\
	 *   \hline
	 *   0 & -1 & -1 & -1 \\
	 *      1 & +1 & -1 & -1 \\
	 *      2 & +1 & +1 & -1 \\
	 *      3 & -1 & +1 & -1 \\
	 *      4 & -1 & -1 & +1 \\
	 *      5 & +1 & -1 & +1 \\
	 *      6 & +1 & +1 & +1 \\
	 *      7 & -1 & +1 & +1
	 *  \end{array}
	 * \f$
	 */
	/// \param i The node id (w.r.t. local element) to evaluate at
	/// \param epsilon, eta, mu The 3D parametric coordinates each within \f$[-1 +1]\f$
	/// \return Ni(epsilon, eta, mu)
	/// \note A check is performed on the nodeId i but not on the 3D parametric coordinates range
	/// \sa m_shapeFunctionsEpsilonSign, m_shapeFunctionsEtaSign, m_shapeFunctionsMuSign
	/// \sa dNdepsilon, dNdeta, dNdmu
	double N(size_t i, double epsilon, double eta, double mu) const;

	/// Shape functions derivative \f$dN_i/d\epsilon(\epsilon, \eta, \mu) = \pm(1\pm\eta)(1\pm\mu)/8\f$
	/// \param i The node id (w.r.t. local element) to evaluate at
	/// \param epsilon, eta, mu The 3D parametric coordinates each within \f$[-1 +1]\f$
	/// \return dNi/depsilon(epsilon, eta, mu)
	/// \note A check is performed on the nodeId i but not on the 3D parametric coordinates range
	/// \sa N
	/// \sa m_shapeFunctionsEpsilonSign, m_shapeFunctionsEtaSign, m_shapeFunctionsMuSign
	double dNdepsilon(size_t i, double epsilon, double eta, double mu) const;

	/// Shape functions derivative \f$dN_i/d\eta(\epsilon, \eta, \mu) = \pm(1\pm\epsilon)(1\pm\mu)/8\f$
	/// \param i The node id (w.r.t. local element) to evaluate at
	/// \param epsilon, eta, mu The 3D parametric coordinates each within \f$[-1 +1]\f$
	/// \return dNi/depsilon(epsilon, eta, mu)
	/// \note A check is performed on the nodeId i but not on the 3D parametric coordinates range
	/// \sa N
	/// \sa m_shapeFunctionsEpsilonSign, m_shapeFunctionsEtaSign, m_shapeFunctionsMuSign
	double dNdeta(size_t i, double epsilon, double eta, double mu) const;

	/// Shape functions derivative \f$dN_i/d\mu(\epsilon, \eta, \mu) = \pm(1\pm\epsilon)(1\pm\eta)/8\f$
	/// \param i The node id (w.r.t. local element) to evaluate at
	/// \param epsilon, eta, mu The 3D parametric coordinates each within \f$[-1 +1]\f$
	/// \return dNi/depsilon(epsilon, eta, mu)
	/// \note A check is performed on the nodeId i but not on the 3D parametric coordinates range
	/// \sa N
	/// \sa m_shapeFunctionsEpsilonSign, m_shapeFunctionsEtaSign, m_shapeFunctionsMuSign
	double dNdmu(size_t i, double epsilon, double eta, double mu) const;

	/// The cube rest state (nodes ordered by m_nodeIds)
	Eigen::Matrix<double, 24, 1, Eigen::DontAlign> m_x0;

	/// Strain matrix (usually noted \f$\epsilon\f$)
	Eigen::Matrix<double, 6, 24, Eigen::DontAlign> m_strain;
	/// Stress matrix (usually noted \f$\sigma\f$)
	Eigen::Matrix<double, 6, 24, Eigen::DontAlign> m_stress;
	/// Constitutive material matrix (Hooke's law in this case) defines the relationship between stress and strain
	Eigen::Matrix<double, 6, 6, Eigen::DontAlign> m_constitutiveMaterial;

	/// %Mass matrix (usually noted \f$M\f$)
	Eigen::Matrix<double, 24, 24, Eigen::DontAlign> m_mass;
	/// Stiffness matrix (usually noted \f$K\f$)
	Eigen::Matrix<double, 24, 24, Eigen::DontAlign> m_stiffness;
};

} // namespace Physics

} // namespace SurgSim

#endif // SURGSIM_PHYSICS_FEMELEMENT3DCUBE_H
