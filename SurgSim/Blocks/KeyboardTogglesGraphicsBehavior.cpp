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

#include "SurgSim/Blocks/KeyboardTogglesGraphicsBehavior.h"
#include "SurgSim/DataStructures/DataStructuresConvert.h"
#include "SurgSim/DataStructures/DataGroup.h"
#include "SurgSim/Framework/FrameworkConvert.h"
#include "SurgSim/Framework/ObjectFactory.h"
#include "SurgSim/Framework/Log.h"
#include "SurgSim/Graphics/Representation.h"
#include "SurgSim/Input/InputComponent.h"

namespace SurgSim
{
namespace Blocks
{
SURGSIM_REGISTER(SurgSim::Framework::Component, SurgSim::Blocks::KeyboardTogglesGraphicsBehavior,
				 KeyboardTogglesGraphicsBehavior);

KeyboardTogglesGraphicsBehavior::KeyboardTogglesGraphicsBehavior(const std::string& name) :
	SurgSim::Framework::Behavior(name),
	m_keyPressedLastUpdate(false)
{
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(KeyboardTogglesGraphicsBehavior, std::shared_ptr<SurgSim::Framework::Component>,
									  InputComponent, getInputComponent, setInputComponent);
	SURGSIM_ADD_SERIALIZABLE_PROPERTY(KeyboardTogglesGraphicsBehavior, KeyboardRegistryType,
									  KeyboardRegistry, getKeyboardRegistry, setKeyboardRegistry);
}

void KeyboardTogglesGraphicsBehavior::setInputComponent(std::shared_ptr<SurgSim::Framework::Component> inputComponent)
{
	SURGSIM_ASSERT(nullptr != inputComponent) << "'inputComponent' cannot be 'nullptr'";

	m_inputComponent = std::dynamic_pointer_cast<SurgSim::Input::InputComponent>(inputComponent);

	SURGSIM_ASSERT(nullptr != m_inputComponent)	<< "'inputComponent' must derive from SurgSim::Input::InputComponent";
}

std::shared_ptr<SurgSim::Input::InputComponent> KeyboardTogglesGraphicsBehavior::getInputComponent() const
{
	return m_inputComponent;
}

void KeyboardTogglesGraphicsBehavior::registerKey(SurgSim::Device::KeyCode key,
												  std::shared_ptr<SurgSim::Framework::Component> component)
{
	m_registry[static_cast<int>(key)].insert(component);
}

void KeyboardTogglesGraphicsBehavior::update(double dt)
{
	SurgSim::DataStructures::DataGroup dataGroup;
	m_inputComponent->getData(&dataGroup);

	int key;
	if (dataGroup.integers().get("key", &key))
	{
		auto match = m_registry.find(key);
		if (match != m_registry.end() && !m_keyPressedLastUpdate)
		{
			for (auto it = std::begin(match->second); it != std::end(match->second); ++it)
			{
				(*it)->setActive(!(*it)->isActive());
			};
		}
		m_keyPressedLastUpdate = (SurgSim::Device::KeyCode::NONE != key);
	}
}

bool KeyboardTogglesGraphicsBehavior::doInitialize()
{
	return true;
}

bool KeyboardTogglesGraphicsBehavior::doWakeUp()
{
	bool result = true;
	if (nullptr == m_inputComponent)
	{
		SURGSIM_LOG_SEVERE(SurgSim::Framework::Logger::getDefaultLogger()) << __FUNCTION__ <<
			"KeyboardTogglesGraphicsBehavior " << getName() << " does not have an Input Component.";
		result = false;
	}
	return result;
}

void KeyboardTogglesGraphicsBehavior::setKeyboardRegistry(const KeyboardRegistryType& map)
{
	m_registry = map;
}

const KeyboardTogglesGraphicsBehavior::KeyboardRegistryType&
		KeyboardTogglesGraphicsBehavior::getKeyboardRegistry() const
{
	return m_registry;
}

}; // namespace Blocks
}; // namespace SurgSim
