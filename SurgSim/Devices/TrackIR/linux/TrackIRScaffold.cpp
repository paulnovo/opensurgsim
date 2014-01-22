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

#include "SurgSim/Devices/TrackIR/TrackIRScaffold.h"

#include <algorithm>
#include <list>
#include <memory>

#include <boost/thread/locks.hpp>
#include <boost/thread/mutex.hpp>

#include <linuxtrack.h>

#include "SurgSim/Devices/TrackIR/TrackIRDevice.h"
#include "SurgSim/Devices/TrackIR/TrackIRThread.h"
#include "SurgSim/Framework/Assert.h"
#include "SurgSim/Framework/Log.h"
#include "SurgSim/Framework/SharedInstance.h"
#include "SurgSim/DataStructures/DataGroup.h"
#include "SurgSim/DataStructures/DataGroupBuilder.h"
#include "SurgSim/Math/Matrix.h"
#include "SurgSim/Math/RigidTransform.h"
#include "SurgSim/Math/Vector.h"

using SurgSim::DataStructures::DataGroupBuilder;
using SurgSim::Math::Matrix33d;
using SurgSim::Math::RigidTransform3d;
using SurgSim::Math::Vector3d;

namespace SurgSim
{
namespace Device
{

struct TrackIRScaffold::DeviceData
{
	/// Constructor
	/// \param device Device to be wrapped
	explicit DeviceData(TrackIRDevice* device) :
		m_deviceObject(device),
		m_thread(),
		m_positionScale(TrackIRDevice::defaultPositionScale()),
		m_orientationScale(TrackIRDevice::defaultOrientationScale())
	{
	}

	/// The corresponding device object.
	SurgSim::Device::TrackIRDevice* const m_deviceObject;
	/// Processing thread.
	std::unique_ptr<SurgSim::Device::TrackIRThread> m_thread;

	/// Scale factor for the position axes; stored locally before the device is initialized.
	double m_positionScale;
	/// Scale factor for the orientation axes; stored locally before the device is initialized.
	double m_orientationScale;

	/// The mutex that protects the externally modifiable parameters.
	boost::mutex m_parametersMutex;

private:
	// Prevent copy construction and copy assignment.  (VS2012 does not support "= delete" yet.)
	DeviceData(const DeviceData&) /*= delete*/;
	DeviceData& operator=(const DeviceData&) /*= delete*/;
};

struct TrackIRScaffold::StateData
{
public:
	/// Initialize the state.
	StateData() : isApiInitialized(false)
	{
	}

	/// True if the API has been initialized (and not finalized).
	bool isApiInitialized;

	/// The list of known devices.
	std::list<std::unique_ptr<TrackIRScaffold::DeviceData>> activeDeviceList;

	/// The mutex that protects the list of known devices.
	boost::mutex mutex;

private:
	// Prevent copy construction and copy assignment.  (VS2012 does not support "= delete" yet.)
	StateData(const StateData&) /*= delete*/;
	StateData& operator=(const StateData&) /*= delete*/;
};


TrackIRScaffold::TrackIRScaffold(std::shared_ptr<SurgSim::Framework::Logger> logger) :
	m_logger(logger),
	m_state(new StateData)
{
	if (!m_logger)
	{
		m_logger = SurgSim::Framework::Logger::getLogger("TrackIR device");
		m_logger->setThreshold(m_defaultLogLevel);
	}
	SURGSIM_LOG_DEBUG(m_logger) << "TrackIR: Shared scaffold created.";
}


TrackIRScaffold::~TrackIRScaffold()
{
	// The following block controls the duration of the mutex being locked.
	{
		boost::lock_guard<boost::mutex> lock(m_state->mutex);

		if (!m_state->activeDeviceList.empty())
		{
			SURGSIM_LOG_SEVERE(m_logger) << "TrackIR: Destroying scaffold while devices are active!?!";
			for (auto it = std::begin(m_state->activeDeviceList);  it != std::end(m_state->activeDeviceList);  ++it)
			{
				if ((*it)->m_thread)
				{
					destroyPerDeviceThread(it->get());
				}
			}
			m_state->activeDeviceList.clear();
		}

		if (m_state->isApiInitialized)
		{
			if (!finalizeSdk())
			{
				SURGSIM_LOG_SEVERE(m_logger) << "Finalizing TrackIR SDK failed.";
			}
		}
	}
	SURGSIM_LOG_DEBUG(m_logger) << "TrackIR: Shared scaffold destroyed.";
}

std::shared_ptr<SurgSim::Framework::Logger> TrackIRScaffold::getLogger() const
{
	return m_logger;
}


bool TrackIRScaffold::registerDevice(TrackIRDevice* device)
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);

	if (!m_state->isApiInitialized)
	{
		if (!initializeSdk())
		{
			SURGSIM_LOG_SEVERE(m_logger) << "Failed to initialize TrackIR SDK in TrackIRScaffold::registerDevice(). "
										 << "Continuing without the TrackIR device.";
		}
	}

	// Only proceed when initializationSdk() is successful.
	if (m_state->isApiInitialized)
	{
		// Make sure the object is unique.
		auto sameObject = std::find_if(m_state->activeDeviceList.cbegin(), m_state->activeDeviceList.cend(),
			[device](const std::unique_ptr<DeviceData>& info) { return info->m_deviceObject == device; });
		SURGSIM_ASSERT(sameObject == m_state->activeDeviceList.end()) << "TrackIR: Tried to register a device" <<
			" which is already registered!";

		// Make sure the name is unique.
		const std::string name = device->getName();
		auto sameName = std::find_if(m_state->activeDeviceList.cbegin(), m_state->activeDeviceList.cend(),
			[&name](const std::unique_ptr<DeviceData>& info) { return info->m_deviceObject->getName() == name; });
		SURGSIM_ASSERT(sameName == m_state->activeDeviceList.end()) << "TrackIR: Tried to register a device" <<
			" when the same name is already present!";

		// This assertion overlaps with above two assertions somehow.
		SURGSIM_ASSERT(m_state->activeDeviceList.size() < 1) << "There is already a TrackIR camera exists."
			<< " TrackIRScaffold only supports one TrackIR camera right now."
			<< " Behaviors of multiple cameras are undefined.\n";
		std::unique_ptr<DeviceData> info(new DeviceData(device));
		createPerDeviceThread(info.get());
		SURGSIM_ASSERT(info->m_thread) << "Failed to create a per-device thread for TrackIR device: " <<
				info->m_deviceObject->getName() << ", with ID number " << cameraID << ".";

		m_state->activeDeviceList.emplace_back(std::move(info));
	}

	return m_state->isApiInitialized;
}


bool TrackIRScaffold::unregisterDevice(const TrackIRDevice* const device)
{
	bool found = false;
	{
		boost::lock_guard<boost::mutex> lock(m_state->mutex);
		auto matching = std::find_if(m_state->activeDeviceList.begin(), m_state->activeDeviceList.end(),
			[device](const std::unique_ptr<DeviceData>& info) { return info->m_deviceObject == device; });

		if (matching != m_state->activeDeviceList.end())
		{
			if ((*matching)->m_thread)
			{
				destroyPerDeviceThread(matching->get());
			}
			m_state->activeDeviceList.erase(matching);
			// the iterator is now invalid but that's OK
			found = true;
		}
	}

	if (!found)
	{
		SURGSIM_LOG_WARNING(m_logger) << "TrackIR: Attempted to release a non-registered device.";
	}
	return found;
}

void TrackIRScaffold::setPositionScale(const TrackIRDevice* device, double scale)
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);
	auto matching = std::find_if(m_state->activeDeviceList.begin(), m_state->activeDeviceList.end(),
		[device](const std::unique_ptr<DeviceData>& info) { return info->m_deviceObject == device; });

	if (matching != m_state->activeDeviceList.end())
	{
		boost::lock_guard<boost::mutex> lock((*matching)->m_parametersMutex);
		(*matching)->positionScale = scale;
	}
}

void TrackIRScaffold::setOrientationScale(const TrackIRDevice* device, double scale)
{
	boost::lock_guard<boost::mutex> lock(m_state->mutex);
	auto matching = std::find_if(m_state->activeDeviceList.begin(), m_state->activeDeviceList.end(),
		[device](const std::unique_ptr<DeviceData>& info) { return info->m_deviceObject == device; });

	if (matching != m_state->activeDeviceList.end())
	{
		boost::lock_guard<boost::mutex> lock((*matching)->m_parametersMutex);
		(*matching)->orientationScale = scale;
	}
}

bool TrackIRScaffold::runInputFrame(TrackIRScaffold::DeviceData* info)
{
	if (!updateDevice(info))
	{
		return false;
	}
	info->m_deviceObject->pushInput();
	return true;
}

bool TrackIRScaffold::updateDevice(TrackIRScaffold::DeviceData* info)
{
	SurgSim::DataStructures::DataGroup& inputData = info->m_deviceObject->getInputData();

	boost::lock_guard<boost::mutex> lock(info->m_parametersMutex);

	float x = 0.0, y = 0.0, z = 0.0, yaw = 0.0, pitch = 0.0, roll = 0.0;
	unsigned counter = 0; // Current camera frame number

	// Assuming left hand coordinate with Y-axis points up.
	// pitch: rotation around X-axis
	// yaw: rotation around Y-axis
	// roll: rotation around Z-axis (Min: -45; Max: +45)
	ltr_get_pose(&yaw, &pitch, &roll, &x, &y, &z, &counter); // Positions are reported in millimeters.
	// Dec-22-2013-HW Currently, the output of Z-axis value from ltr_get_pose() is not consistent
	// Contacted the developer, waiting for response.
	Vector3d position(static_cast<double>(x) / 1000.0,
					  static_cast<double>(y) / 1000.0,
					  static_cast<double>(z) / 1000.0); // Convert millimeter to meter
	Vector3d rotation(pitch, yaw, roll); // In Degrees

	// Scale Position
	position *= info->positionScale;
	// Scale Orientation
	rotation *= info->orientationScale;

	// Convert to a pose.
	Matrix33d orientation;
	double angle = rotation.norm();
	if (angle < 1e-9)
	{
		orientation.setIdentity();
	}
	else
	{
		orientation = SurgSim::Math::makeRotationMatrix(angle, Vector3d(rotation / angle));
	}

	RigidTransform3d pose;
	pose.makeAffine();
	pose.linear() = orientation;
	pose.translation() = position;

	inputData.poses().set("pose", pose);

	return true;
}

bool TrackIRScaffold::initializeSdk()
{
	SURGSIM_ASSERT(!m_state->isApiInitialized) << "TrackIR API already initialized.";

	//Initialize the tracking using Default profile
	ltr_init(NULL);

	//Wait for TrackIR initialization
	ltr_state_type state;
	int timeout = 100; // Wait for 10 seconds before quit
	while(timeout > 0)
	{
		state = ltr_get_tracking_state();
		if(state != RUNNING)
		{
			usleep(100000); //sleep 0.1s
		}
		else
		{
			m_state->isApiInitialized = true;
			break;
		}
		--timeout;
	};

	return m_state->isApiInitialized;
}

bool TrackIRScaffold::finalizeSdk()
{
	SURGSIM_ASSERT(m_state->isApiInitialized) << "TrackIR API already finalized.";

	ltr_shutdown();
	ltr_state_type state;
	state = ltr_get_tracking_state();
	if (state == STOPPED)
	{
		m_state->isApiInitialized = false;
	}
	return !m_state->isApiInitialized;
}

bool TrackIRScaffold::createPerDeviceThread(DeviceData* deviceData)
{
	SURGSIM_ASSERT(!deviceData->m_thread) << "Device " << deviceData->m_deviceObject->getName()
										  << " already has a thread.";

	std::unique_ptr<TrackIRThread> thread(new TrackIRThread(this, deviceData));
	thread->start();
	deviceData->m_thread = std::move(thread);

	return true;
}

bool TrackIRScaffold::destroyPerDeviceThread(DeviceData* deviceData)
{
	SURGSIM_ASSERT(deviceData->m_thread) << "No thread attached to device " << deviceData->m_deviceObject->getName();

	std::unique_ptr<TrackIRThread> thread = std::move(deviceData->m_thread);
	thread->stop();
	thread.reset();

	return true;
}

bool TrackIRScaffold::startCamera(DeviceData* info)
{
	return true;
}

bool TrackIRScaffold::stopCamera(DeviceData* info)
{
	return true;
}

SurgSim::DataStructures::DataGroup TrackIRScaffold::buildDeviceInputData()
{
	DataGroupBuilder builder;
	builder.addPose("pose");
	return builder.createData();
}

std::shared_ptr<TrackIRScaffold> TrackIRScaffold::getOrCreateSharedInstance()
{
	static SurgSim::Framework::SharedInstance<TrackIRScaffold> sharedInstance;
	return sharedInstance.get();
}

void TrackIRScaffold::setDefaultLogLevel(SurgSim::Framework::LogLevel logLevel)
{
	m_defaultLogLevel = logLevel;
}

SurgSim::Framework::LogLevel TrackIRScaffold::m_defaultLogLevel = SurgSim::Framework::LOG_LEVEL_INFO;

};  // namespace Device
};  // namespace SurgSim
