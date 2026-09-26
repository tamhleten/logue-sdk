#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54414D48U,
    .unit_id = 0x00000004U,
    .version = 0x00010000U,
    .name = "dub-echo",
    .num_params = 5,
    .params = {
      // min, max, center, init, type, frac, frac_mode, reserved, name
      {40, 2000, 1020, 750, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"LENGTH"}},
      {0, 300, 150, 12, k_unit_param_type_none, 1,
       k_unit_param_frac_mode_decimal, 0, {"SEPARATION"}},
      {-1000, 1000, 0, 0, k_unit_param_type_drywet, 1,
       k_unit_param_frac_mode_decimal, 0, {"DRY WET"}},
      {0, 980, 0, 700, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"FEEDBACK"}},
      {10, 3000, 1505, 1200, k_unit_param_type_none, 2,
       k_unit_param_frac_mode_decimal, 0, {"MOD RATE"}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
  },
  .default_mappings = {
    {k_genericfx_param_assign_x, k_genericfx_curve_exp,
     k_genericfx_curve_unipolar, 40, 2000, 750},
    {k_genericfx_param_assign_y, k_genericfx_curve_exp,
     k_genericfx_curve_unipolar, 0, 300, 12},
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, -1000, 1000, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 980, 700},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 10, 3000, 1200},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
