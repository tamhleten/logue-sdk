/*
 * Native NTS-3 feedback/ring-mod sound generator.
 */

#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),

    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,

    .dev_id = 0x0,

    // Different from the template/pluck unit.
    .unit_id = 0x00000001U,

    .version = 0x00010000U,

    .name = "fb-ring",

    .num_params = 3,

    .params = {

      // min, max, center, init,
      // type, frac, frac_mode, reserved, name

      // X: 1 .. 1000 ms
      {
        1,
        1000,
        0,
        344,
        k_unit_param_type_msec,
        0,
        k_unit_param_frac_mode_decimal,
        0,
        {"DELAY"}
      },

      // Y: MIDI note -> osc_notehzf()
      {
        1,
        104,
        0,
        60,
        k_unit_param_type_midi_note,
        0,
        k_unit_param_frac_mode_decimal,
        0,
        {"MOD PITCH"}
      },

      // Depth: 0.000 .. 2.000
      // Internally 0 .. 2000, displayed with 3 decimal places.
      {
        0,
        2000,
        0,
        1921,
        k_unit_param_type_none,
        3,
        k_unit_param_frac_mode_decimal,
        0,
        {"GAIN"}
      },

      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
  },

  .default_mappings = {

    // X = delay
    {
      k_genericfx_param_assign_x,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      1,
      1000,
      344
    },

    // Y = modulation pitch
    {
      k_genericfx_param_assign_y,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      1,
      104,
      60
    },

    // Depth = feedback gain
    {
      k_genericfx_param_assign_depth,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      0,
      2000,
      1921
    },

    {
      k_genericfx_param_assign_none,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      0, 0, 0
    },

    {
      k_genericfx_param_assign_none,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      0, 0, 0
    },

    {
      k_genericfx_param_assign_none,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      0, 0, 0
    },

    {
      k_genericfx_param_assign_none,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      0, 0, 0
    },

    {
      k_genericfx_param_assign_none,
      k_genericfx_curve_linear,
      k_genericfx_curve_unipolar,
      0, 0, 0
    }
  }
};