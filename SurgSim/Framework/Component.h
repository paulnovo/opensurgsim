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

#ifndef SURGSIM_FRAMEWORK_COMPONENT_H
#define SURGSIM_FRAMEWORK_COMPONENT_H

#include <string>
#include <memory>

#include <SurgSim/Framework/Log.h>

namespace SurgSim
{
namespace Framework
{

// Forward References
class ComponentManager;
class Runtime;

/// Component is the main interface class to pass information to the system managers each will decide
/// whether to handle a component of a given type or not. Components will get initialized by having
/// doInit(), and doWakeUp() called in succession, all components together will have doInit() called before
/// any component will recieve doWakeUp()
class Component : public std::enable_shared_from_this<Component>
{
public:
	explicit Component(const std::string& name) : m_name(name), m_didInit(false), m_didWakeUp(false)
	{
	}

	virtual ~Component()
	{
	}

	/// Gets the name.
	/// \return	The name.
	std::string getName()
	{
		return m_name;
	};

	bool initialize(std::shared_ptr<Runtime> runtime)
	{
		SURGSIM_ASSERT(! m_didInit) << "Double initialization called on component " << getName();
		SURGSIM_ASSERT(runtime != nullptr) << "Runtime cannot be nullptr";
		m_runtime = runtime;

		m_didInit = true;
		return doInitialize();
	};

	bool wakeUp()
	{
		SURGSIM_ASSERT(! m_didWakeUp) << "Double wakeup called on component " << getName();
		m_didWakeUp = true;
		return doWakeUp();
	};

	std::shared_ptr<Runtime> getRuntime()
	{
		return m_runtime.lock();
	}

protected:

	/// Sets the name.
	/// \param	name	The name.
	void setName(const std::string& name)
	{
		m_name = name;
	}

private:
	std::string m_name;

	std::weak_ptr<Runtime> m_runtime;

	virtual bool doInitialize() = 0;
	virtual bool doWakeUp() = 0;

	bool m_didInit;
	bool m_didWakeUp;
};

}
}
#endif
