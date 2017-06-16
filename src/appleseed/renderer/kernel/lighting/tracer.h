
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2017 Francois Beaune, The appleseedhq Organization
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

#ifndef APPLESEED_RENDERER_KERNEL_LIGHTING_TRACER_H
#define APPLESEED_RENDERER_KERNEL_LIGHTING_TRACER_H

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/kernel/intersection/intersector.h"
#include "renderer/kernel/shading/shadingpoint.h"
#include "renderer/kernel/shading/shadingray.h"
#include "renderer/modeling/scene/visibilityflags.h"

// appleseed.foundation headers.
#include "foundation/core/concepts/noncopyable.h"
#include "foundation/math/vector.h"

// Standard headers.
#include <cstddef>

// Forward declarations.
namespace renderer  { class Material;}
namespace renderer  { class OSLShaderGroupExec; }
namespace renderer  { class Scene; }
namespace renderer  { class ShadingContext; }
namespace renderer  { class TextureCache; }

namespace renderer
{

//
// The Tracer class wraps the Intersector class and allows to compute
// visibility from a given point along a given direction, as well as
// point-to-point visibility. It automatically takes into account alpha
// transparency.
//

class Tracer
  : public foundation::NonCopyable
{
  public:
    // Constructors.
    Tracer(
        const Scene&                    scene,
        const Intersector&              intersector,
        TextureCache&                   texture_cache,
        OSLShaderGroupExec&             shadergroup_exec,
        const float                     transparency_threshold = 0.001f,
        const size_t                    max_iterations = 1000,
        const bool                      print_details = true);

    // Compute the transmission in a given direction. Returns the intersection
    // with the closest fully opaque occluder and the transmission factor up
    // to (but excluding) this occluder, or a miss if there is no fully opaque
    // occluder in this direction.
    const ShadingPoint& trace(
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     direction,
        const ShadingRay::Time&         ray_time,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay::DepthType     ray_depth,
        float&                          transmission);
    const ShadingPoint& trace(
        const ShadingPoint&             origin,
        const foundation::Vector3d&     direction,
        const VisibilityFlags::Type     ray_flags,
        float&                          transmission);
    // These methods require higher computational effort,
    // but take participating media into account while computing transmission.
    const ShadingPoint& trace(
        const ShadingContext&           shading_context,
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     direction,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay&               parent_ray,
        Spectrum&                       transmission);
    const ShadingPoint& trace(
        const ShadingContext&           shading_context,
        const ShadingPoint&             origin,
        const foundation::Vector3d&     direction,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay&               parent_ray,
        Spectrum&                       transmission);

    // Compute the transmission in a given direction. This variant may take
    // advantage of the fact that the intersection with the closest occluder
    // is not required to deliver higher performances.
    float trace(
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     direction,
        const ShadingRay::Time&         ray_time,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay::DepthType     ray_depth);
    float trace(
        const ShadingPoint&             origin,
        const foundation::Vector3d&     direction,
        const VisibilityFlags::Type     ray_flags);

    // Compute the transmission between two points. Returns the intersection
    // with the closest fully opaque occluder and the transmission factor up
    // to (but excluding) this occluder, or a miss if there is no fully opaque
    // occluder in the segment [origin, target).
    const ShadingPoint& trace_between(
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     target,
        const ShadingRay::Time&         ray_time,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay::DepthType     ray_depth,
        float&                          transmission);
    const ShadingPoint& trace_between(
        const ShadingPoint&             origin,
        const foundation::Vector3d&     target,
        const VisibilityFlags::Type     ray_flags,
        float&                          transmission);

    // Compute the transmission between two points. This variant may take
    // advantage of the fact that the intersection with the closest occluder
    // is not required to deliver higher performances.
    float trace_between(
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     target,
        const ShadingRay::Time&         ray_time,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay::DepthType     ray_depth);
    float trace_between(
        const ShadingPoint&             origin,
        const foundation::Vector3d&     target,
        const VisibilityFlags::Type     ray_flags);
    // These methods require higher computational effort,
    // but take participating media into account while computing transmission.
    const ShadingPoint& trace_between(
        const ShadingContext&           shading_context,
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     target,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay&               parent_ray,
        Spectrum&                       transmission);
    const ShadingPoint& trace_between(
        const ShadingContext&           shading_context,
        const ShadingPoint&             origin,
        const foundation::Vector3d&     target,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay&               parent_ray,
        Spectrum&                       transmission);

  private:
    const Intersector&                  m_intersector;
    TextureCache&                       m_texture_cache;
    OSLShaderGroupExec&                 m_shadergroup_exec;
    const bool                          m_assume_no_alpha_mapping;
    const float                         m_transmission_threshold;
    const size_t                        m_max_iterations;
    ShadingPoint                        m_shading_points[2];

    const ShadingPoint& do_trace(
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     direction,
        const ShadingRay::Time&         ray_time,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay::DepthType     ray_depth,
        float&                          transmission,
        const ShadingPoint*             parent_shading_point);

    const ShadingPoint& do_trace_between(
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     target,
        const ShadingRay::Time&         ray_time,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay::DepthType     ray_depth,
        float&                          transmission,
        const ShadingPoint*             parent_shading_point);

    const ShadingPoint& do_trace(
        const ShadingContext&           shading_context,
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     direction,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay&               parent_ray,
        Spectrum&                       transmission,
        const ShadingPoint*             parent_shading_point);

    const ShadingPoint& do_trace_between(
        const ShadingContext&           shading_context,
        const foundation::Vector3d&     origin,
        const foundation::Vector3d&     target,
        const VisibilityFlags::Type     ray_flags,
        const ShadingRay&               parent_ray,
        Spectrum&                       transmission,
        const ShadingPoint*             parent_shading_point);

    void evaluate_alpha(
        const Material&                 material,
        const ShadingPoint&             shading_point,
        Alpha&                          alpha) const;
};


//
// Tracer class implementation.
//

inline const ShadingPoint& Tracer::trace(
    const foundation::Vector3d&         origin,
    const foundation::Vector3d&         direction,
    const ShadingRay::Time&             ray_time,
    const VisibilityFlags::Type         ray_flags,
    const ShadingRay::DepthType         ray_depth,
    float&                              transmission)
{
    return
        do_trace(
            origin,
            direction,
            ray_time,
            ray_flags,
            ray_depth,
            transmission,
            0);
}

inline const ShadingPoint& Tracer::trace(
    const ShadingPoint&                 origin,
    const foundation::Vector3d&         direction,
    const VisibilityFlags::Type         ray_flags,
    float&                              transmission)
{
    return
        do_trace(
            origin.get_biased_point(direction),
            direction,
            origin.get_time(),
            ray_flags,
            origin.get_ray().m_depth + 1,
            transmission,
            &origin);
}

inline float Tracer::trace(
    const foundation::Vector3d&         origin,
    const foundation::Vector3d&         direction,
    const ShadingRay::Time&             ray_time,
    const VisibilityFlags::Type         ray_flags,
    const ShadingRay::DepthType         ray_depth)
{
    if (m_assume_no_alpha_mapping)
    {
        assert(foundation::is_normalized(direction));

        const ShadingRay ray(
            origin,
            direction,
            ray_time,
            ray_flags,
            ray_depth);

        return m_intersector.trace_probe(ray) ? 0.0f : 1.0f;
    }
    else
    {
        float transmission;
        const ShadingPoint& shading_point =
            trace(
                origin,
                direction,
                ray_time,
                ray_flags,
                ray_depth,
                transmission);

        return shading_point.hit() ? 0.0f : transmission;
    }
}

inline float Tracer::trace(
    const ShadingPoint&                 origin,
    const foundation::Vector3d&         direction,
    const VisibilityFlags::Type         ray_flags)
{
    if (m_assume_no_alpha_mapping)
    {
        assert(foundation::is_normalized(direction));

        const ShadingRay ray(
            origin.get_biased_point(direction),
            direction,
            origin.get_time(),
            ray_flags,
            origin.get_ray().m_depth + 1);

        return m_intersector.trace_probe(ray, &origin) ? 0.0f : 1.0f;
    }
    else
    {
        float transmission;
        const ShadingPoint& shading_point =
            trace(
                origin,
                direction,
                ray_flags,
                transmission);

        return shading_point.hit() ? 0.0f : transmission;
    }
}

inline const ShadingPoint& Tracer::trace(
    const ShadingContext&           shading_context,
    const foundation::Vector3d&     origin,
    const foundation::Vector3d&     direction,
    const VisibilityFlags::Type     ray_flags,
    const ShadingRay&               parent_ray,
    Spectrum&                       transmission)
{
    return
        do_trace(
        shading_context,
        origin,
        direction,
        ray_flags,
        parent_ray,
        transmission,
        nullptr);
}

inline const ShadingPoint& Tracer::trace(
    const ShadingContext&           shading_context,
    const ShadingPoint&             origin,
    const foundation::Vector3d&     direction,
    const VisibilityFlags::Type     ray_flags,
    const ShadingRay&               parent_ray,
    Spectrum&                       transmission)
{
    return
        do_trace(
            shading_context,
            origin.get_biased_point(direction),
            direction,
            ray_flags,
            parent_ray,
            transmission,
            &origin);
}

inline const ShadingPoint& Tracer::trace_between(
    const foundation::Vector3d&         origin,
    const foundation::Vector3d&         target,
    const ShadingRay::Time&             ray_time,
    const VisibilityFlags::Type         ray_flags,
    const ShadingRay::DepthType         ray_depth,
    float&                              transmission)
{
    return
        do_trace_between(
            origin,
            target,
            ray_time,
            ray_flags,
            ray_depth,
            transmission,
            0);
}

inline const ShadingPoint& Tracer::trace_between(
    const ShadingPoint&                 origin,
    const foundation::Vector3d&         target,
    const VisibilityFlags::Type         ray_flags,
    float&                              transmission)
{
    return
        do_trace_between(
            origin.get_biased_point(target - origin.get_point()),
            target,
            origin.get_time(),
            ray_flags,
            origin.get_ray().m_depth + 1,
            transmission,
            &origin);
}

inline float Tracer::trace_between(
    const foundation::Vector3d&         origin,
    const foundation::Vector3d&         target,
    const ShadingRay::Time&             ray_time,
    const VisibilityFlags::Type         ray_flags,
    const ShadingRay::DepthType         ray_depth)
{
    if (m_assume_no_alpha_mapping)
    {
        const foundation::Vector3d direction = target - origin;
        const double dist = foundation::norm(direction);

        const ShadingRay ray(
            origin,
            direction / dist,
            0.0,                        // ray tmin
            dist * (1.0 - 1.0e-6),      // ray tmax
            ray_time,
            ray_flags,
            ray_depth);

        return m_intersector.trace_probe(ray) ? 0.0f : 1.0f;
    }
    else
    {
        float transmission;
        const ShadingPoint& shading_point =
            trace_between(
                origin,
                target,
                ray_time,
                ray_flags,
                ray_depth,
                transmission);

        return shading_point.hit() ? 0.0f : transmission;
    }
}

inline float Tracer::trace_between(
    const ShadingPoint&                 origin,
    const foundation::Vector3d&         target,
    const VisibilityFlags::Type         ray_flags)
{
    if (m_assume_no_alpha_mapping)
    {
        const foundation::Vector3d direction = target - origin.get_point();
        const double dist = foundation::norm(direction);

        const ShadingRay ray(
            origin.get_biased_point(direction),
            direction / dist,
            0.0,                        // ray tmin
            dist * (1.0 - 1.0e-6),      // ray tmax
            origin.get_time(),
            ray_flags,
            origin.get_ray().m_depth + 1);

        return m_intersector.trace_probe(ray, &origin) ? 0.0f : 1.0f;
    }
    else
    {
        float transmission;
        const ShadingPoint& shading_point =
            trace_between(
                origin,
                target,
                ray_flags,
                transmission);

        return shading_point.hit() ? 0.0f : transmission;
    }
}

inline const ShadingPoint& Tracer::trace_between(
    const ShadingContext&           shading_context,
    const foundation::Vector3d&     origin,
    const foundation::Vector3d&     target,
    const VisibilityFlags::Type     ray_flags,
    const ShadingRay&               parent_ray,
    Spectrum&                       transmission)
{
    return
        do_trace_between(
            shading_context,
            origin,
            target,
            ray_flags,
            parent_ray,
            transmission,
            nullptr);
}

inline const ShadingPoint& Tracer::trace_between(
    const ShadingContext&           shading_context,
    const ShadingPoint&             origin,
    const foundation::Vector3d&     target,
    const VisibilityFlags::Type     ray_flags,
    const ShadingRay&               parent_ray,
    Spectrum&                       transmission)
{
    return
        do_trace_between(
            shading_context,
            origin.get_biased_point(target - origin.get_point()),
            target,
            ray_flags,
            parent_ray,
            transmission,
            &origin);
}

}       // namespace renderer

#endif  // !APPLESEED_RENDERER_KERNEL_LIGHTING_TRACER_H
