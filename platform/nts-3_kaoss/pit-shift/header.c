#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54414D48U,
    .unit_id = 0x00000002U,
    .version = 0x00010200U,
    .name = "pit-shift",
    .num_params = 3,
    .params = {
      // min, max, center, init, type, frac, frac_mode, reserved, name
      {-1000, 1000, 0, 0, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"SHIFT"}},
      {1024, 5024, 3024, 5024, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"WINDOW"}},
      {0, 1000, 0, 1000, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"WET LEVEL"}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
  },
  .default_mappings = {
    {k_genericfx_param_assign_x, k_genericfx_curve_linear,
     k_genericfx_curve_bipolar, -1000, 1000, 0},
    {k_genericfx_param_assign_y, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 1024, 5024, 5024},
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 1000, 1000},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
