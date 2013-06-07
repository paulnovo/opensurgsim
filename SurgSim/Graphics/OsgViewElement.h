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

#ifndef SURGSIM_GRAPHICS_OSGVIEWELEMENT_H
#define SURGSIM_GRAPHICS_OSGVIEWELEMENT_H

#include <SurgSim/Graphics/ViewElement.h>

namespace SurgSim
{

namespace Graphics
{

/// OSG-based implementation of graphics view element.
///
/// A Graphics::OsgViewElement creates and wraps a Graphics::OsgView so that it can be added to the Scene.
///
/// A Scene needs at least one Graphics::View component for any visualization of Graphics:Representation objects to be shown.
class OsgViewElement : public Graphics::ViewElement
{
public:
	/// Constructor
	/// \param	name	Name of the scene element
	explicit OsgViewElement(const std::string& name);

	/// Destructor
	virtual ~OsgViewElement();

	/// Sets the view component that provides the visualization of the graphics representations
	/// Only allows OsgView components, any other will not be set and it will return false.
	/// \return	True if it succeeds, false if it fails
	virtual bool setView(std::shared_ptr<View> view);
};

};  // namespace Graphics

};  // namespace SurgSim

#endif  // SURGSIM_GRAPHICS_OSGVIEWELEMENT_H
