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

#include "SurgSim/Devices/MultiAxis/Win32HidDeviceHandle.h"

#undef  _WIN32_WINNT
#define _WIN32_WINNT 0x0501   // request Windows XP-compatible SDK APIs
#undef  WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN   // do not automatically include WinSock 1 and some other header files
#include <windows.h>

#include <setupapi.h>
extern "C" {  // sigh...
#include <hidsdi.h>
}

#include <stdint.h>

#include "SurgSim/Devices/MultiAxis/FileHandle.h"
#include "SurgSim/Devices/MultiAxis/GetSystemError.h"
#include <SurgSim/Framework/Log.h>

using SurgSim::Device::Internal::getSystemErrorCode;
using SurgSim::Device::Internal::getSystemErrorText;


namespace SurgSim
{
namespace Device
{

// Usage page and usage IDs for interface devices; see e.g. http://www.usb.org/developers/devclass_docs/Hut1_12v2.pdf
enum UsagePageConstants
{
	DEV_USAGE_PAGE_GENERIC_DESKTOP		= 0x01		// Generic Desktop usage page
};
enum UsageConstants
{
	// Usages for the DEV_USAGE_PAGE_GENERIC_DESKTOP usage page:
	DEV_USAGE_ID_MOUSE						= 0x02,	// Mouse usage ID
	DEV_USAGE_ID_JOYSTICK					= 0x04,	// Joystick usage ID
	DEV_USAGE_ID_GAME_PAD					= 0x05,	// Game Pad usage ID
	DEV_USAGE_ID_KEYBOARD					= 0x06,	// Keyboard usage ID
	DEV_USAGE_ID_KEYPAD						= 0x07,	// Keypad usage ID
	DEV_USAGE_ID_MULTI_AXIS_CONTROLLER		= 0x08	// Multi-axis Controller usage ID
};


struct Win32HidDeviceHandle::State
{
public:
	explicit State(std::shared_ptr<SurgSim::Framework::Logger>&& logger_) :
		logger(std::move(logger_)),
		handle()
	{
	}

	/// The logger to use.
	std::shared_ptr<SurgSim::Framework::Logger> logger;
	/// The underlying device file handle.
	FileHandle handle;
};

Win32HidDeviceHandle::Win32HidDeviceHandle(std::shared_ptr<SurgSim::Framework::Logger>&& logger) :
	m_state(new Win32HidDeviceHandle::State(std::move(logger)))
{
}

Win32HidDeviceHandle::~Win32HidDeviceHandle()
{
}

std::vector<std::string> Win32HidDeviceHandle::enumerate(SurgSim::Framework::Logger* logger)
{
	std::vector<std::string> results;

	// Prepare to iterate over the attached HID devices
	GUID hidGuid;
	HidD_GetHidGuid(&hidGuid);
	HDEVINFO hidDeviceInfo = SetupDiGetClassDevs(&hidGuid, NULL, NULL,
		DIGCF_DEVICEINTERFACE | DIGCF_PRESENT | DIGCF_PROFILE);
	if (hidDeviceInfo == INVALID_HANDLE_VALUE)
	{
		DWORD error = GetLastError();
		SURGSIM_LOG_CRITICAL(logger) << "Win32HidDeviceHandle::enumerate: Failed to query HID devices;" <<
			" SetupDiGetClassDevs() failed with error " << error << ", " << getSystemErrorText(error);
		return results;
	}

	// Loop through the device list, looking for the devices we want
	for (int hidEnumerationIndex = 0; true; ++hidEnumerationIndex)
	{
		// Get the next interface in the list.
		SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
		deviceInterfaceData.cbSize = sizeof(deviceInterfaceData);
		if (! SetupDiEnumDeviceInterfaces(hidDeviceInfo, NULL, &hidGuid, hidEnumerationIndex, &deviceInterfaceData))
		{
			DWORD error = GetLastError();
			if (error == ERROR_NO_MORE_ITEMS)
			{
				break;
			}
			else
			{
				SURGSIM_LOG_CRITICAL(logger) << "Win32HidDeviceHandle::enumerate: Failed to query HID devices;" <<
					" SetupDiEnumDeviceInterfaces() failed with error " << error << ", " << getSystemErrorText(error);
				return results;
			}
		}

		// Find out the required size.
		DWORD deviceInterfaceDetailSize = 0;
		if (! SetupDiGetDeviceInterfaceDetail(hidDeviceInfo, &deviceInterfaceData, NULL, 0,
											  &deviceInterfaceDetailSize, NULL))
		{
			DWORD error = GetLastError();
			if (error != ERROR_INSUFFICIENT_BUFFER)
			{
				SURGSIM_LOG_INFO(logger) << "Win32HidDeviceHandle::enumerate: Failed to get the device detail size," <<
					" device will be ignored; error " << error << ", " << getSystemErrorText(error);
				continue;
			}
		}

		// Get the device detail (which actually just means the path).
		SP_DEVICE_INTERFACE_DETAIL_DATA* deviceInterfaceDetail =
			static_cast<SP_DEVICE_INTERFACE_DETAIL_DATA*>(malloc(deviceInterfaceDetailSize));
		deviceInterfaceDetail->cbSize = sizeof(*deviceInterfaceDetail);
		if (! SetupDiGetDeviceInterfaceDetail(hidDeviceInfo, &deviceInterfaceData,
			deviceInterfaceDetail, deviceInterfaceDetailSize, NULL, NULL))
		{
			DWORD error = GetLastError();
			SURGSIM_LOG_INFO(logger) << "Win32HidDeviceHandle::enumerate: Failed to get the HID device detail," <<
				" device will be ignored; error " << error << ", " << getSystemErrorText(error);
			free(deviceInterfaceDetail);
			continue;
		}

		std::string devicePath(deviceInterfaceDetail->DevicePath);
		free(deviceInterfaceDetail);

		FileHandle handle;
		if (! handle.openForReadingAndMaybeWriting(devicePath))
		{
			DWORD error = GetLastError();
			SURGSIM_LOG_INFO(logger) << "Win32HidDeviceHandle::enumerate: Could not open device " << devicePath <<
				": error " << error << ", " << getSystemErrorText(error);
			continue;
		}

		results.push_back(devicePath);
	}

	return results;
}

std::unique_ptr<Win32HidDeviceHandle> Win32HidDeviceHandle::open(
	const std::string& path, std::shared_ptr<SurgSim::Framework::Logger> logger)
{
	std::unique_ptr<Win32HidDeviceHandle> object(new Win32HidDeviceHandle(std::move(logger)));
	if (! object->m_state->handle.openForReadingAndMaybeWriting(path))
	{
		object.reset();  // could not open the device handle; destroy the object again
	}
	return object;
}

bool Win32HidDeviceHandle::canRead() const
{
	return m_state->handle.canRead();
}

bool Win32HidDeviceHandle::canWrite() const
{
	return m_state->handle.canWrite();
}

bool Win32HidDeviceHandle::hasDataToRead() const
{
	return m_state->handle.hasDataToRead();
}

bool Win32HidDeviceHandle::readBytes(void* dataBuffer, size_t bytesToRead, size_t* bytesActuallyRead)
{
	return m_state->handle.readBytes(dataBuffer, bytesToRead, bytesActuallyRead);
}

// XXX HORRIBLE HACK!!!
void* Win32HidDeviceHandle::get() const
{
	return m_state->handle.get();
}

bool Win32HidDeviceHandle::getCapabilities(HIDP_CAPS* capabilities) const
{
	PHIDP_PREPARSED_DATA preParsedData = 0;
	if (HidD_GetPreparsedData(m_state->handle.get(), &preParsedData) != TRUE)
	{
		DWORD error = GetLastError();
		SURGSIM_LOG_INFO(m_state->logger) << "Win32HidDeviceHandle: Could not get preparsed data: error " << error <<
			", " << getSystemErrorText(error);
		return false;
	}

	if (HidP_GetCaps(preParsedData, capabilities) != HIDP_STATUS_SUCCESS)
	{
		DWORD error = GetLastError();
		SURGSIM_LOG_INFO(m_state->logger) << "Win32HidDeviceHandle: Could not get capabilities: error " << error <<
			", " << getSystemErrorText(error);
		HidD_FreePreparsedData(preParsedData);
		return false;
	}

	HidD_FreePreparsedData(preParsedData);  // don't need the pre-parsed data any more
	return true;
}

bool Win32HidDeviceHandle::hasTranslationAndRotationAxes() const
{
	HIDP_CAPS capabilities;
	if (! getCapabilities(&capabilities))
	{
		// message already shown
		return false;
	}

	if ((capabilities.UsagePage != DEV_USAGE_PAGE_GENERIC_DESKTOP) ||
		(capabilities.Usage != DEV_USAGE_ID_MULTI_AXIS_CONTROLLER))
	{
		SURGSIM_LOG_DEBUG(m_state->logger) << "Win32HidDeviceHandle: device is not a multi-axis controller.";
		return false;
	}

	int numExtraAxes = static_cast<int>(capabilities.NumberInputValueCaps) - 6;
	if (numExtraAxes < 0)
	{
		SURGSIM_LOG_DEBUG(m_state->logger) << "Win32HidDeviceHandle: device does not have 6 input axes.";
		return false;
	}
	else if (numExtraAxes > 0)
	{
		SURGSIM_LOG_INFO(m_state->logger) << "Win32HidDeviceHandle: device has more than 6 axes;" <<
			" ignoring " << numExtraAxes << " additional axes.";
	}

	return true;
}

static inline int16_t signedShortData(unsigned char byte0, unsigned char byte1)
{
	return static_cast<int16_t>(static_cast<uint16_t>(byte0) | (static_cast<uint16_t>(byte1) << 8));
}

bool Win32HidDeviceHandle::updateStates(AxisStates* axisStates, ButtonStates* buttonStates, bool* updated)
{
	// We can't keep reading while data is available, because we don't know how to tell when data is available.
	// Both WaitForSingleObject() and WaitForMultipleObjects() always claim data is available for 3DConnexion device
	// file handles.  So we just do it once, blocking until we have data.
	//
	// We also can't unblock the read once we initiate it (closing the handle has no effect).

	{
		unsigned char deviceBuffer[7*128];
		size_t numRead;
		if (! m_state->handle.readBytes(&deviceBuffer, sizeof(deviceBuffer), &numRead))
		{
			int64_t error = getSystemErrorCode();
			SURGSIM_LOG_WARNING(m_state->logger) << "Win32HidDeviceHandle: read failed with error " << error << ", " <<
				getSystemErrorText(error);
		}
		else if ((numRead >= 7) && (deviceBuffer[0] == 0x01))       // Translation
		{
			(*axisStates)[0] = signedShortData(deviceBuffer[1],  deviceBuffer[2]);
			(*axisStates)[1] = signedShortData(deviceBuffer[3],  deviceBuffer[4]);
			(*axisStates)[2] = signedShortData(deviceBuffer[5],  deviceBuffer[6]);
			*updated = true;

			if ((numRead >= 14) && (deviceBuffer[7] == 0x02))  // translation data may have rotation appended to it
			{
				(*axisStates)[3] = signedShortData(deviceBuffer[8],  deviceBuffer[9]);
				(*axisStates)[4] = signedShortData(deviceBuffer[10],  deviceBuffer[11]);
				(*axisStates)[5] = signedShortData(deviceBuffer[12],  deviceBuffer[13]);
			}
		}
		else if ((numRead >= 7) && (deviceBuffer[0] == 0x02))  // Rotation
		{
			(*axisStates)[3] = signedShortData(deviceBuffer[1],  deviceBuffer[2]);
			(*axisStates)[4] = signedShortData(deviceBuffer[3],  deviceBuffer[4]);
			(*axisStates)[5] = signedShortData(deviceBuffer[5],  deviceBuffer[6]);
			*updated = true;

			if ((numRead >= 14) && (deviceBuffer[7] == 0x01))  // rotation data may have translation appended to it
			{
				(*axisStates)[0] = signedShortData(deviceBuffer[8],  deviceBuffer[9]);
				(*axisStates)[1] = signedShortData(deviceBuffer[10],  deviceBuffer[11]);
				(*axisStates)[2] = signedShortData(deviceBuffer[12],  deviceBuffer[13]);
			}
		}
		else if ((numRead >= 2) && (deviceBuffer[0] == 0x03))  // Buttons
		{
			size_t currentByte = 1;  // Byte 0 specifies the packet type; data starts at byte 1
			unsigned char currentBit = 0x01;
			for (size_t i = 0;  i < (*buttonStates).size();  ++i)
			{
				(*buttonStates)[i] = ((deviceBuffer[currentByte] & currentBit) != 0);
				if (currentBit < 0x80)
				{
					currentBit = currentBit << 1;
				}
				else
				{
					currentBit = 0x01;
					++currentByte;
					if (currentByte >= numRead)  // out of data?
					{
						break;
					}
				}
			}
			*updated = true;
		}
	}

	return true;
}

};  // namespace Device
};  // namespace SurgSim
