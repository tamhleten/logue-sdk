#include "unit_genericfx.h"

const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54414D48U,
    .unit_id = 0x00000006U,
    .version = 0x00010000U,
    .name = "ricochet",
    .num_params = 7,
    .params = {
      // min, max, center, init, type, frac, frac_mode, reserved, name
      {25, 200, 112, 100, k_unit_param_type_none, 2,
       k_unit_param_frac_mode_decimal, 0, {"BEATS"}},
      {500, 900, 700, 700, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"ACCEL"}},
      {-1000, 1000, 0, 0, k_unit_param_type_drywet, 1,
       k_unit_param_frac_mode_decimal, 0, {"DRY WET"}},
      {400, 900, 650, 680, k_unit_param_type_none, 3,
       k_unit_param_frac_mode_decimal, 0, {"DECAY"}},
      {4, 12, 8, 8, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"BOUNCES"}},
      {-48, -12, -30, -28, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"THRESHOLD"}},
      {20, 100, 60, 55, k_unit_param_type_none, 0,
       k_unit_param_frac_mode_decimal, 0, {"GRAIN MS"}},
      {0, 0, 0, 0, k_unit_param_type_none, 0, 0, 0, {""}}
    }
  },
  .default_mappings = {
    {k_genericfx_param_assign_x, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 25, 200, 100},
    {k_genericfx_param_assign_y, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 500, 900, 700},
    {k_genericfx_param_assign_depth, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, -1000, 1000, 0},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 400, 900, 680},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 4, 12, 8},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, -48, -12, -28},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 20, 100, 55},
    {k_genericfx_param_assign_none, k_genericfx_curve_linear,
     k_genericfx_curve_unipolar, 0, 0, 0}
  }
};
