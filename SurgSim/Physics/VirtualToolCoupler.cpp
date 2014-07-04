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

#include <Eigen/Eigenvalues>

#include "SurgSim/DataStructures/DataGroupBuilder.h"
#include "SurgSim/DataStructures/DataStructuresConvert.h"
#include "SurgSim/Framework/FrameworkConvert.h"
#include "SurgSim/Framework/LogMacros.h"
#include "SurgSim/Input/InputComponent.h"
#include "SurgSim/Input/OutputComponent.h"
#include "SurgSim/Math/MathConvert.h"
#include "SurgSim/Math/Matrix.h"
#include "SurgSim/Math/Quaternion.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Math/Vector.h"
#include "SurgSim/Physics/RigidRepresentation.h"
#include "SurgSim/Physics/VirtualToolCoupler.h"

using SurgSim::Math::Vector3d;
using SurgSim::Math::Matrix33d;
using SurgSim::Math::Matrix66d;
using SurgSim::Math::RigidTransform3d;
using SurgSim::Math::Quaterniond;

namespace SurgSim
{

namespace Physics
{
SURGSIM_REGISTER(SurgSim::Framework::Component, SurgSim::Physics::VirtualToolCoupler, VirtualToolCoupler);

VirtualToolCoupler::VirtualToolCoupler(const std::string& name) :
	SurgSim::Framework::Behavior(name),
	m_poseName(SurgSim::DataStructures::Names::POSE),
	m_linearStiffness(std::numeric_limits<double>::quiet_NaN()),
	m_linearDamping(std::numeric_limits<double>::quiet_NaN()),
	m_angularStiffness(std::numeric_limits<double>::quiet_NaN()),
	m_angularDamping(std::numeric_limits<double>::quiet_NaN()),
	m_localAttachmentPoint(Vector3d(std::numeric_limits<double>::quiet_NaN(), std::numeric_limits<double>::quiet_NaN(),
		std::numeric_limits<double>::quiet_NaN()))
{
	SurgSim::DataStructures::DataGroupBuilder builder;
	builder.addVector(SurgSim::DataStructures::Names::FORCE);
	builder.addVector(SurgSim::DataStructures::Names::TORQUE);
	builder.addMatrix(SurgSim::DataStructures::Names::SPRING_JACOBIAN);
	builder.addPose(SurgSim::DataStructures::Names::INPUT_POSE);
	builder.addMatrix(SurgSim::DataStructures::Names::DAMPER_JACOBIAN);
	builder.addVector(SurgSim::DataStructures::Names::INPUT_LINEAR_VELOCITY);
	builder.addVector(SurgSim::DataStructures::Names::INPUT_ANGULAR_VELOCITY);
	m_outputData = builder.createData();

	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, SurgSim::DataStructures::OptionalValue<double>,
		LinearStiffness, getOptionalLinearStiffness, setOptionalLinearStiffness);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, SurgSim::DataStructures::OptionalValue<double>,
		LinearDamping, getOptionalLinearDamping, setOptionalLinearDamping);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, SurgSim::DataStructures::OptionalValue<double>,
		AngularStiffness, getOptionalAngularStiffness, setOptionalAngularStiffness);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, SurgSim::DataStructures::OptionalValue<double>,
		AngularDamping, getOptionalAngularDamping, setOptionalAngularDamping);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler,
			SurgSim::DataStructures::OptionalValue<SurgSim::Math::Vector3d>, AttachmentPoint,
			getOptionalAttachmentPoint, setOptionalAttachmentPoint);

	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, std::shared_ptr<SurgSim::Framework::Component>,
		Input, getInput, setInput);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, std::shared_ptr<SurgSim::Framework::Component>,
		Output, getOutput, setOutput);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(VirtualToolCoupler, std::shared_ptr<SurgSim::Framework::Component>,
		Representation, getRepresentation, setRepresentation);
}

VirtualToolCoupler::~VirtualToolCoupler()
{
}

const std::shared_ptr<SurgSim::Framework::Component> VirtualToolCoupler::getInput()
{
	return m_input;
}

void VirtualToolCoupler::setInput(const std::shared_ptr<SurgSim::Framework::Component> input)
{
	m_input = std::dynamic_pointer_cast<SurgSim::Input::InputComponent>(input);
}

const std::shared_ptr<SurgSim::Framework::Component> VirtualToolCoupler::getOutput()
{
	return m_output;
}

void VirtualToolCoupler::setOutput(const std::shared_ptr<SurgSim::Framework::Component> output)
{
	m_output = std::dynamic_pointer_cast<SurgSim::Input::OutputComponent>(output);
}

const std::shared_ptr<SurgSim::Framework::Component> VirtualToolCoupler::getRepresentation()
{
	return m_rigid;
}

void VirtualToolCoupler::setRepresentation(const std::shared_ptr<SurgSim::Framework::Component> rigid)
{
	m_rigid = std::dynamic_pointer_cast<SurgSim::Physics::RigidRepresentation>(rigid);
}

const std::string& VirtualToolCoupler::getPoseName()
{
	return m_poseName;
}

void VirtualToolCoupler::setPoseName(const std::string& poseName)
{
	m_poseName = poseName;
}

void VirtualToolCoupler::update(double dt)
{
	SurgSim::DataStructures::DataGroup inputData;
	m_input->getData(&inputData);
	RigidTransform3d inputPose;
	if (inputData.poses().get(m_poseName, &inputPose))
	{
		// TODO(ryanbeasley): If the RigidRepresentation is not colliding, we should turn off the VTC forces and set the
		// RigidRepresentation's state to the input state.
		Vector3d inputLinearVelocity, inputAngularVelocity;
		inputLinearVelocity.setZero();
		inputData.vectors().get(SurgSim::DataStructures::Names::LINEAR_VELOCITY, &inputLinearVelocity);
		inputAngularVelocity.setZero();
		inputData.vectors().get(SurgSim::DataStructures::Names::ANGULAR_VELOCITY, &inputAngularVelocity);

		RigidRepresentationState objectState(m_rigid->getCurrentState());
		RigidTransform3d objectPose(objectState.getPose());
		Vector3d objectPosition = objectPose * m_rigid->getCurrentParameters().getMassCenter();
		Vector3d attachmentPoint = objectPose * m_localAttachmentPoint;
		Vector3d leverArm = attachmentPoint - objectPosition;
		Vector3d attachmentPointVelocity = objectState.getLinearVelocity();
		attachmentPointVelocity += objectState.getAngularVelocity().cross(leverArm);

		Vector3d force = m_linearStiffness * (inputPose.translation() - attachmentPoint);
		force += m_linearDamping * (inputLinearVelocity - attachmentPointVelocity);

		Vector3d rotationVector;
		SurgSim::Math::computeRotationVector(inputPose, objectPose, &rotationVector);
		Vector3d torque = m_angularStiffness * rotationVector;
		torque += leverArm.cross(force);
		torque += m_angularDamping * (inputAngularVelocity - objectState.getAngularVelocity());

		const Matrix33d identity3x3 = Matrix33d::Identity();
		const Matrix33d linearStiffnessMatrix = m_linearStiffness * identity3x3;
		const Matrix33d linearDampingMatrix = m_linearDamping * identity3x3;
		const Matrix33d angularStiffnessMatrix = m_angularStiffness * identity3x3;
		const Matrix33d angularDampingMatrix = m_angularDamping * identity3x3;
		m_rigid->addExternalForce(force, linearStiffnessMatrix, linearDampingMatrix);
		m_rigid->addExternalTorque(torque, angularStiffnessMatrix, angularDampingMatrix);

		if (m_output != nullptr)
		{
			m_outputData.vectors().set(SurgSim::DataStructures::Names::FORCE, -force);
			m_outputData.vectors().set(SurgSim::DataStructures::Names::TORQUE, -torque);
			m_outputData.vectors().set(SurgSim::DataStructures::Names::INPUT_LINEAR_VELOCITY, inputLinearVelocity);
			m_outputData.vectors().set(SurgSim::DataStructures::Names::INPUT_ANGULAR_VELOCITY, inputAngularVelocity);

			m_outputData.poses().set(SurgSim::DataStructures::Names::INPUT_POSE, inputPose);

			const Matrix33d skewLeverArm = SurgSim::Math::makeSkewSymmetricMatrix(leverArm);

			Matrix66d springJacobian;
			springJacobian << -linearStiffnessMatrix, Matrix33d::Zero(),
							  -m_linearStiffness * skewLeverArm, -angularStiffnessMatrix;
			m_outputData.matrices().set(SurgSim::DataStructures::Names::SPRING_JACOBIAN, springJacobian);

			Matrix66d damperJacobian;
			damperJacobian << -linearDampingMatrix, Matrix33d::Zero(),
							  m_linearDamping * skewLeverArm, -angularDampingMatrix;
			m_outputData.matrices().set(SurgSim::DataStructures::Names::DAMPER_JACOBIAN, damperJacobian);

			m_output->setData(m_outputData);
		}
	}
}

bool VirtualToolCoupler::doInitialize()
{
	return true;
}

bool VirtualToolCoupler::doWakeUp()
{
	if (m_input == nullptr)
	{
		SURGSIM_LOG_SEVERE(SurgSim::Framework::Logger::getDefaultLogger()) << "VirtualToolCoupler named " <<
			getName() << " does not have an Input Component.";
		return false;
	}
	if (m_rigid == nullptr)
	{
		SURGSIM_LOG_SEVERE(SurgSim::Framework::Logger::getDefaultLogger()) << "VirtualToolCoupler named " <<
			getName() << " does not have a Representation.";
		return false;
	}

	// Provide sensible defaults based on the rigid representation.
	// If one or both of the stiffness and damping are not provided, they are
	// calculated to provide a critically damped system (dampingRatio-1.0).
	// For a mass-spring system, the damping ratio is defined as:
	//
	//     dampingRatio = (damping) / (2 * sqrt(mass * stiffness))
	//
	double dampingRatio = 1.0;
	double mass = m_rigid->getCurrentParameters().getMass();
	if (!m_optionalLinearDamping.hasValue())
	{
		if (m_optionalLinearStiffness.hasValue())
		{
			m_linearStiffness = m_optionalLinearStiffness.getValue();
		}
		else
		{
			m_linearStiffness = mass * 800.0;
		}
		m_linearDamping = 2.0 * dampingRatio * sqrt(mass * m_linearStiffness);
	}
	else
	{
		m_linearDamping = m_optionalLinearDamping.getValue();
		if (m_optionalLinearStiffness.hasValue())
		{
			m_linearStiffness = m_optionalLinearStiffness.getValue();
		}
		else
		{
			m_linearStiffness = pow(m_linearDamping / dampingRatio, 2) / (4.0 * mass);
		}
	}

	const Matrix33d& inertia = m_rigid->getCurrentParameters().getLocalInertia();
	double maxInertia = inertia.eigenvalues().real().maxCoeff();
	if (!m_optionalAngularDamping.hasValue())
	{
		if (m_optionalAngularStiffness.hasValue())
		{
			m_angularStiffness = m_optionalAngularStiffness.getValue();
		}
		else
		{
			m_angularStiffness = maxInertia * 1000.0;
		}
		m_angularDamping = 2.0 * dampingRatio * sqrt(maxInertia * m_angularStiffness);
	}
	else
	{
		m_angularDamping = m_optionalAngularDamping.getValue();
		if (m_optionalAngularStiffness.hasValue())
		{
			m_angularStiffness = m_optionalAngularStiffness.getValue();
		}
		else
		{
			m_angularStiffness = pow(m_angularDamping / dampingRatio, 2) / (4.0 * maxInertia);
		}
	}

	if (!m_optionalAttachmentPoint.hasValue())
	{
		m_localAttachmentPoint = m_rigid->getCurrentParameters().getMassCenter();
	}
	else
	{
		m_localAttachmentPoint = m_optionalAttachmentPoint.getValue();
	}

	return true;
}

int VirtualToolCoupler::getTargetManagerType() const
{
	return SurgSim::Framework::MANAGER_TYPE_PHYSICS;
}

void VirtualToolCoupler::overrideLinearStiffness(double linearStiffness)
{
	SURGSIM_ASSERT(!isInitialized()) << "Cannot override vtc parameter after it has initialized";

	m_optionalLinearStiffness.setValue(linearStiffness);
	m_linearStiffness = linearStiffness;
}

double VirtualToolCoupler::getLinearStiffness()
{
	SURGSIM_ASSERT(isAwake() || m_optionalLinearStiffness.hasValue())
		<< "Vtc parameter has not been initialized";

	return m_linearStiffness;
}

void VirtualToolCoupler::overrideLinearDamping(double linearDamping)
{
	SURGSIM_ASSERT(!isInitialized()) << "Cannot override vtc parameter after it has initialized";

	m_optionalLinearDamping.setValue(linearDamping);
	m_linearDamping = linearDamping;
}

double VirtualToolCoupler::getLinearDamping()
{
	SURGSIM_ASSERT(isAwake() || m_optionalLinearDamping.hasValue())
		<< "Vtc parameter has not been initialized";

	return m_linearDamping;
}

void VirtualToolCoupler::overrideAngularStiffness(double angularStiffness)
{
	SURGSIM_ASSERT(!isInitialized()) << "Cannot override vtc parameter after it has initialized";

	m_optionalAngularStiffness.setValue(angularStiffness);
	m_angularStiffness = angularStiffness;
}

double VirtualToolCoupler::getAngularStiffness()
{
	SURGSIM_ASSERT(isAwake() || m_optionalAngularStiffness.hasValue())
		<< "Vtc parameter has not been initialized";

	return m_angularStiffness;
}

void VirtualToolCoupler::overrideAngularDamping(double angularDamping)
{
	SURGSIM_ASSERT(!isInitialized()) << "Cannot override vtc parameter after it has initialized";

	m_optionalAngularDamping.setValue(angularDamping);
	m_angularDamping = angularDamping;
}

double VirtualToolCoupler::getAngularDamping()
{
	SURGSIM_ASSERT(isAwake() || m_optionalAngularDamping.hasValue())
		<< "Vtc parameter has not been initialized";

	return m_angularDamping;
}

void VirtualToolCoupler::overrideAttachmentPoint(const SurgSim::Math::Vector3d& attachment)
{
	SURGSIM_ASSERT(!isInitialized()) << "Cannot override vtc parameter after it has initialized";

	m_optionalAttachmentPoint.setValue(attachment);
	m_localAttachmentPoint = attachment;
}

const SurgSim::Math::Vector3d& VirtualToolCoupler::getAttachmentPoint()
{
	SURGSIM_ASSERT(isAwake() || m_optionalAttachmentPoint.hasValue())
		<< "Vtc parameter has not been initialized";
	return m_localAttachmentPoint;
}

void VirtualToolCoupler::setOptionalLinearStiffness(
	const SurgSim::DataStructures::OptionalValue<double>& linearStiffness)
{
	m_optionalLinearStiffness = linearStiffness;
}

const SurgSim::DataStructures::OptionalValue<double>& VirtualToolCoupler::getOptionalLinearStiffness() const
{
	return m_optionalLinearStiffness;
}

void VirtualToolCoupler::setOptionalLinearDamping(const SurgSim::DataStructures::OptionalValue<double>& linearDamping)
{
	m_optionalLinearDamping = linearDamping;
}

const SurgSim::DataStructures::OptionalValue<double>& VirtualToolCoupler::getOptionalLinearDamping() const
{
	return m_optionalLinearDamping;
}

void VirtualToolCoupler::setOptionalAngularStiffness(
	const SurgSim::DataStructures::OptionalValue<double>& angularStiffness)
{
	m_optionalAngularStiffness = angularStiffness;
}

const SurgSim::DataStructures::OptionalValue<double>& VirtualToolCoupler::getOptionalAngularStiffness() const
{
	return m_optionalAngularStiffness;
}

void VirtualToolCoupler::setOptionalAngularDamping(
	const SurgSim::DataStructures::OptionalValue<double>& angularDamping)
{
	m_optionalAngularDamping = angularDamping;
}

const SurgSim::DataStructures::OptionalValue<double>& VirtualToolCoupler::getOptionalAngularDamping() const
{
	return m_optionalAngularDamping;
}

void VirtualToolCoupler::setOptionalAttachmentPoint(
	const SurgSim::DataStructures::OptionalValue<SurgSim::Math::Vector3d>& attachmentPoint)
{
	m_optionalAttachmentPoint = attachmentPoint;
}

const SurgSim::DataStructures::OptionalValue<SurgSim::Math::Vector3d>&
VirtualToolCoupler::getOptionalAttachmentPoint() const
{
	return m_optionalAttachmentPoint;
}


}; /// Physics
}; /// SurgSim
