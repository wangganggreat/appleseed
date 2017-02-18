
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

// Interface header.
#include "bsdfmix.h"

// appleseed.renderer headers.
#include "renderer/global/globallogger.h"
#include "renderer/global/globaltypes.h"
#include "renderer/kernel/shading/shadingcontext.h"
#include "renderer/modeling/bsdf/bsdf.h"
#include "renderer/modeling/bsdf/bsdfwrapper.h"
#include "renderer/modeling/scene/assembly.h"

// appleseed.foundation headers.
#include "foundation/math/basis.h"
#include "foundation/math/vector.h"
#include "foundation/platform/types.h"
#include "foundation/utility/api/apistring.h"
#include "foundation/utility/api/specializedapiarrays.h"
#include "foundation/utility/arena.h"
#include "foundation/utility/containers/dictionary.h"

// Standard headers.
#include <cassert>
#include <cstddef>
#include <string>

// Forward declarations.
namespace foundation    { class IAbortSwitch; }
namespace renderer      { class ShadingPoint; }

using namespace foundation;
using namespace std;

namespace renderer
{

namespace
{
    //
    // A mix of two BSDFs, each with its own weight.
    //

    const char* Model = "bsdf_mix";

    class BSDFMixImpl
      : public BSDF
    {
      public:
        BSDFMixImpl(
            const char*             name,
            const ParamArray&       params)
          : BSDF(name, Reflective, ScatteringMode::All, params)
        {
            m_inputs.declare("weight0", InputFormatFloat);
            m_inputs.declare("weight1", InputFormatFloat);
        }

        virtual void release() APPLESEED_OVERRIDE
        {
            delete this;
        }

        virtual const char* get_model() const APPLESEED_OVERRIDE
        {
            return Model;
        }

        virtual bool on_frame_begin(
            const Project&          project,
            const BaseGroup*        parent,
            OnFrameBeginRecorder&   recorder,
            IAbortSwitch*           abort_switch) APPLESEED_OVERRIDE
        {
            if (!BSDF::on_frame_begin(project, parent, recorder, abort_switch))
                return false;

            const Assembly* assembly = static_cast<const Assembly*>(parent);

            m_bsdf[0] = retrieve_bsdf(*assembly, "bsdf0");
            m_bsdf[1] = retrieve_bsdf(*assembly, "bsdf1");

            if (m_bsdf[0] == 0 || m_bsdf[1] == 0)
                return false;

            return true;
        }

        virtual void* evaluate_inputs(
            const ShadingContext&   shading_context,
            const ShadingPoint&     shading_point) const APPLESEED_OVERRIDE
        {
            assert(m_bsdf[0] && m_bsdf[1]);

            Values* values = shading_context.get_arena().allocate<Values>();

            values->m_inputs =
                static_cast<Values::Inputs*>(
                    BSDF::evaluate_inputs(shading_context, shading_point));

            values->m_child_inputs[0] = m_bsdf[0]->evaluate_inputs(shading_context, shading_point);
            values->m_child_inputs[1] = m_bsdf[1]->evaluate_inputs(shading_context, shading_point);

            return values;
        }

        virtual void sample(
            SamplingContext&        sampling_context,
            const void*             data,
            const bool              adjoint,
            const bool              cosine_mult,
            BSDFSample&             sample) const APPLESEED_OVERRIDE
        {
            assert(m_bsdf[0] && m_bsdf[1]);

            const Values* values = static_cast<const Values*>(data);

            // Retrieve blending weights.
            const float w[2] = { values->m_inputs->m_weight[0], values->m_inputs->m_weight[1] };

            // Handle absorption.
            const float total_weight = w[0] + w[1];
            if (total_weight == 0.0f)
                return;

            // Choose which of the two BSDFs to sample.
            sampling_context.split_in_place(1, 1);
            const float s = sampling_context.next2<float>();
            const size_t bsdf_index = s * total_weight < w[0] ? 0 : 1;

            // Sample the chosen BSDF.
            m_bsdf[bsdf_index]->sample(
                sampling_context,
                values->m_child_inputs[bsdf_index],
                adjoint,
                false,                      // do not multiply by |cos(incoming, normal)|
                sample);
        }

        virtual float evaluate(
            const void*             data,
            const bool              adjoint,
            const bool              cosine_mult,
            const Vector3f&         geometric_normal,
            const Basis3f&          shading_basis,
            const Vector3f&         outgoing,
            const Vector3f&         incoming,
            const int               modes,
            Spectrum&               value) const APPLESEED_OVERRIDE
        {
            assert(m_bsdf[0] && m_bsdf[1]);

            const Values* values = static_cast<const Values*>(data);

            // Retrieve blending weights.
            float w0 = values->m_inputs->m_weight[0];
            float w1 = values->m_inputs->m_weight[1];
            const float total_weight = w0 + w1;

            // Handle absorption.
            if (total_weight == 0.0f)
                return 0.0f;

            // Normalize the blending weights.
            const float rcp_total_weight = 1.0f / total_weight;
            w0 *= rcp_total_weight;
            w1 *= rcp_total_weight;

            // Evaluate the first BSDF.
            Spectrum bsdf0_value;
            const float bsdf0_prob =
                w0 > 0.0f
                    ? m_bsdf[0]->evaluate(
                          values->m_child_inputs[0],
                          adjoint,
                          false,                // do not multiply by |cos(incoming, normal)|
                          geometric_normal,
                          shading_basis,
                          outgoing,
                          incoming,
                          modes,
                          bsdf0_value)
                    : 0.0f;

            // Evaluate the second BSDF.
            Spectrum bsdf1_value;
            const float bsdf1_prob =
                w1 > 0.0f
                    ? m_bsdf[1]->evaluate(
                          values->m_child_inputs[1],
                          adjoint,
                          false,                // do not multiply by |cos(incoming, normal)|
                          geometric_normal,
                          shading_basis,
                          outgoing,
                          incoming,
                          modes,
                          bsdf1_value)
                    : 0.0f;

            // Blend BSDF values.
            value.set(0.0f);
            if (bsdf0_prob > 0.0f) madd(value, bsdf0_value, w0);
            if (bsdf1_prob > 0.0f) madd(value, bsdf1_value, w1);

            // Blend PDF values.
            return bsdf0_prob * w0 + bsdf1_prob * w1;
        }

        virtual float evaluate_pdf(
            const void*             data,
            const Vector3f&         geometric_normal,
            const Basis3f&          shading_basis,
            const Vector3f&         outgoing,
            const Vector3f&         incoming,
            const int               modes) const APPLESEED_OVERRIDE
        {
            assert(m_bsdf[0] && m_bsdf[1]);

            const Values* values = static_cast<const Values*>(data);

            // Retrieve blending weights.
            const float w0 = values->m_inputs->m_weight[0];
            const float w1 = values->m_inputs->m_weight[1];
            const float total_weight = w0 + w1;

            // Handle absorption.
            if (total_weight == 0.0f)
                return 0.0f;

            // Evaluate the PDF of the first BSDF.
            const float bsdf0_prob =
                w0 > 0.0f
                    ? m_bsdf[0]->evaluate_pdf(
                          values->m_child_inputs[0],
                          geometric_normal,
                          shading_basis,
                          outgoing,
                          incoming,
                          modes)
                    : 0.0f;

            // Evaluate the PDF of the second BSDF.
            const float bsdf1_prob =
                w1 > 0.0f
                    ? m_bsdf[1]->evaluate_pdf(
                          values->m_child_inputs[1],
                          geometric_normal,
                          shading_basis,
                          outgoing,
                          incoming,
                          modes)
                    : 0.0f;

            // Blend PDF values.
            return (bsdf0_prob * w0 + bsdf1_prob * w1) / total_weight;
        }

      private:
        struct Values
        {
            APPLESEED_DECLARE_INPUT_VALUES(Inputs)
            {
                float       m_weight[2];
            };

            const Inputs*   m_inputs;
            const void*     m_child_inputs[2];
        };

        const BSDF* m_bsdf[2];
        size_t      m_bsdf_data_offset[2];

        const BSDF* retrieve_bsdf(const Assembly& assembly, const char* param_name) const
        {
            const string bsdf_name = m_params.get_required<string>(param_name, "");
            if (bsdf_name.empty())
            {
                RENDERER_LOG_ERROR("while preparing bsdf \"%s\": no bsdf bound to \"%s\".", get_path().c_str(), param_name);
                return 0;
            }

            const BSDF* bsdf = assembly.bsdfs().get_by_name(bsdf_name.c_str());
            if (bsdf == 0)
                RENDERER_LOG_ERROR("while preparing bsdf \"%s\": cannot find bsdf \"%s\".", get_path().c_str(), bsdf_name.c_str());

            return bsdf;
        }
    };

    typedef BSDFWrapper<BSDFMixImpl> BSDFMix;
}


//
// BSDFMixFactory class implementation.
//

const char* BSDFMixFactory::get_model() const
{
    return Model;
}

Dictionary BSDFMixFactory::get_model_metadata() const
{
    return
        Dictionary()
            .insert("name", Model)
            .insert("label", "BSDF Mix");
}

DictionaryArray BSDFMixFactory::get_input_metadata() const
{
    DictionaryArray metadata;

    metadata.push_back(
        Dictionary()
            .insert("name", "bsdf0")
            .insert("label", "BSDF 1")
            .insert("type", "entity")
            .insert("entity_types",
                Dictionary().insert("bsdf", "BSDF"))
            .insert("use", "required"));

    metadata.push_back(
        Dictionary()
            .insert("name", "weight0")
            .insert("label", "Weight 1")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary().insert("texture_instance", "Textures"))
            .insert("use", "required")
            .insert("default", "0.5"));

    metadata.push_back(
        Dictionary()
            .insert("name", "bsdf1")
            .insert("label", "BSDF 2")
            .insert("type", "entity")
            .insert("entity_types",
                Dictionary().insert("bsdf", "BSDF"))
            .insert("use", "required"));

    metadata.push_back(
        Dictionary()
            .insert("name", "weight1")
            .insert("label", "Weight 2")
            .insert("type", "colormap")
            .insert("entity_types",
                Dictionary().insert("texture_instance", "Textures"))
            .insert("use", "required")
            .insert("default", "0.5"));

    return metadata;
}

auto_release_ptr<BSDF> BSDFMixFactory::create(
    const char*         name,
    const ParamArray&   params) const
{
    return auto_release_ptr<BSDF>(new BSDFMix(name, params));
}

auto_release_ptr<BSDF> BSDFMixFactory::static_create(
    const char*         name,
    const ParamArray&   params)
{
    return auto_release_ptr<BSDF>(new BSDFMix(name, params));
}

}   // namespace renderer
