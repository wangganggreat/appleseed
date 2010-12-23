
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010 Francois Beaune
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#ifndef APPLESEED_STUDIO_MAINWINDOW_PROJECT_ENTITYNAMES_H
#define APPLESEED_STUDIO_MAINWINDOW_PROJECT_ENTITYNAMES_H

// Forward declarations.
namespace renderer  { class BSDF; }
namespace renderer  { class Camera; }
namespace renderer  { class EDF; }
namespace renderer  { class EnvironmentEDF; }
namespace renderer  { class EnvironmentShader; }
namespace renderer  { class Light; }
namespace renderer  { class Material; }
namespace renderer  { class SurfaceShader; }

namespace appleseed {
namespace studio {

template <typename T> const char* get_entity_name();

template <> inline const char* get_entity_name<renderer::BSDF>()                { return "BSDF"; }
template <> inline const char* get_entity_name<renderer::Camera>()              { return "Camera"; }
template <> inline const char* get_entity_name<renderer::EDF>()                 { return "EDF"; }
template <> inline const char* get_entity_name<renderer::EnvironmentEDF>()      { return "Environment EDF"; }
template <> inline const char* get_entity_name<renderer::EnvironmentShader>()   { return "Environment Shader"; }
template <> inline const char* get_entity_name<renderer::Light>()               { return "Light"; }
template <> inline const char* get_entity_name<renderer::Material>()            { return "Material"; }
template <> inline const char* get_entity_name<renderer::SurfaceShader>()       { return "Surface Shader"; }

}       // namespace studio
}       // namespace appleseed

#endif  // !APPLESEED_STUDIO_MAINWINDOW_PROJECT_ENTITYNAMES_H
