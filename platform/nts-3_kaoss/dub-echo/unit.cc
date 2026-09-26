#include "effect.h"
#include "unit_genericfx.h"
#include "utils/int_math.h"
#include <algorithm>

static Effect s_effect_instance;
static int32_t cached_values[UNIT_GENERICFX_MAX_PARAM_COUNT];

__unit_callback int8_t unit_init(const unit_runtime_desc_t *desc)
{
  if (!desc)
    return k_unit_err_undef;
  if (desc->target != unit_header.common.target)
    return k_unit_err_target;
  if (!UNIT_API_IS_COMPAT(desc->api))
    return k_unit_err_api_version;
  if (desc->samplerate != s_effect_instance.getSampleRate())
    return k_unit_err_samplerate;
  if (desc->input_channels != 2 || desc->output_channels != 2)
    return k_unit_err_geometry;
  if (!desc->hooks.sdram_alloc)
    return k_unit_err_memory;

  float *allocated = reinterpret_cast<float *>(
      desc->hooks.sdram_alloc(s_effect_instance.getBufferSize() * sizeof(float)));
  if (!allocated)
    return k_unit_err_memory;

  std::fill(allocated, allocated + s_effect_instance.getBufferSize(), 0.f);
  s_effect_instance.init(allocated);

  for (int id = 0; id < UNIT_GENERICFX_MAX_PARAM_COUNT; ++id)
    cached_values[id] = static_cast<int32_t>(unit_header.common.params[id].init);

  return k_unit_err_none;
}

__unit_callback void unit_teardown() { s_effect_instance.teardown(); }
__unit_callback void unit_reset() { s_effect_instance.reset(); }
__unit_callback void unit_resume() { s_effect_instance.resume(); }
__unit_callback void unit_suspend() { s_effect_instance.suspend(); }

__unit_callback void unit_render(const float *in, float *out, uint32_t frames)
{
  s_effect_instance.process(in, out, frames);
}

__unit_callback void unit_set_param_value(uint8_t id, int32_t value)
{
  if (id >= unit_header.common.num_params)
    return;
  value = clipminmaxi32(unit_header.common.params[id].min, value,
                        unit_header.common.params[id].max);
  cached_values[id] = value;
  s_effect_instance.setParameter(id, value);
}

__unit_callback int32_t unit_get_param_value(uint8_t id)
{
  return id < UNIT_GENERICFX_MAX_PARAM_COUNT ? cached_values[id] : 0;
}

__unit_callback const char *unit_get_param_str_value(uint8_t id, int32_t value)
{
  if (id >= unit_header.common.num_params)
    return nullptr;
  value = clipminmaxi32(unit_header.common.params[id].min, value,
                        unit_header.common.params[id].max);
  return s_effect_instance.getParameterStrValue(id, value);
}

__unit_callback void unit_touch_event(uint8_t id, uint8_t phase,
                                      uint32_t x, uint32_t y)
{
  s_effect_instance.touchEvent(id, phase, x, y);
}

__unit_callback void unit_set_tempo(uint32_t tempo)
{
  const float bpm = (tempo >> 16) +
                    (tempo & 0xFFFF) / static_cast<float>(0x10000);
  s_effect_instance.setTempo(bpm);
}

__unit_callback void unit_tempo_4ppqn_tick(uint32_t counter)
{
  s_effect_instance.tempo4ppqnTick(counter);
}
