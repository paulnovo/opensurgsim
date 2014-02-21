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

#ifndef SURGSIM_FRAMEWORK_FRAMEWORKCONVERT_H
#define SURGSIM_FRAMEWORK_FRAMEWORKCONVERT_H

#include <memory>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace SurgSim
{
namespace Framework
{
class Component;
}
}

namespace YAML
{
/// Specialization of YAML::convert for std::shared_ptr<Component>, use this for to read in a component
/// written by the convert<SurgSim::Framework::Component> converter, or a reference to a
/// component written by this converter.
/// This specialization, is slightly asymmetric, on encode it will only encode a components
/// name, id and className. When decoding this conversion will check whether a component with
/// the same id has already been encountered. If no a new instance will be created and stored
/// in a local Registry. If yes, the entry from the registry will be returned, this makes sure
/// that all references to the same id will use the correct, copy of the smart pointer.
/// Additionally this class contains a class factory that can be used to generate the class from
/// its name.
template <>
struct convert<std::shared_ptr<SurgSim::Framework::Component> >
{
	static Node encode(const std::shared_ptr<SurgSim::Framework::Component> rhs);
	static bool decode(const Node& node, std::shared_ptr<SurgSim::Framework::Component>& rhs);

private:

	typedef std::unordered_map<std::string, std::shared_ptr<SurgSim::Framework::Component>> RegistryType;

	/// \return The static registry for shared instances
	static RegistryType& getRegistry();
};

/// Override of the convert structure for an Component, use this form to write out a full version
/// of the component information, to decode a component use the other converter. This converter
/// intentionally does not have a decode function.
template<>
struct convert<SurgSim::Framework::Component>
{
	static Node encode(const SurgSim::Framework::Component& rhs);
};

};

#endif // SURGSIM_FRAMEWORK_FRAMEWORKCONVERT_H