#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54414D48U,
    .unit_id = 0x00000005U,
    .version = 0x00010000U,
    .name = "scrap-metal",
    .num_params = 8,
    .params = {
      // min, max, center, init, type, frac, frac_mode, reserved, name
      {60, 600, 330, 170, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"RING FREQ"}},
      {0, 1000, 500, 750, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"DESTRUCTION"}},
      {-1000, 1000, 0, 1000, k_unit_param_type_drywet, 1,
       k_unit_param_frac_mode_decimal, 0, {"DRY WET"}},
      {-24, 0, -12, -12, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"PITCH"}},
      {5, 200, 102, 35, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"SENSITIVITY"}},
      {20, 200, 110, 80, k_unit_param_type_none, 1,
       k_unit_param_frac_mode_decimal, 0, {"GATE RATE"}},
      {0, 3, 0, 0, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"PATTERN"}},
      {0, 1000, 500, 550, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"PITCH MIX"}}
    }
  },
  .default_mappings = {
    {k_genericfx_param_assign_x, k_genericfx_curve_exp,
     k_genericfx_curve_unipolar, 60, 600, 170},
    {k_genericfx_param_assign_y, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 1000, 750},
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, -1000, 1000, 1000},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, -24, 0, -12},
    {k_genericfx_param_assign_none, k_genericfx_curve_exp,
     k_genericfx_curve_unipolar, 5, 200, 35},
    {k_genericfx_param_assign_none, k_genericfx_curve_exp,
     k_genericfx_curve_unipolar, 20, 200, 80},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 3, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 1000, 550}
  }
};
