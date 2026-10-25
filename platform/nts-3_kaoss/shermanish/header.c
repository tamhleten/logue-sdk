#include "unit_genericfx.h"
const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size = sizeof(genericfx_unit_header_t),
    .target = UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api = UNIT_API_VERSION,
    .dev_id = 0x54414D48U, .unit_id = 0x00000007U,
    .version = 0x00010000U, .name = "shermanish", .num_params = 8,
    .params = {
      {0, 1000, 500, 360, k_unit_param_type_none, 0, k_unit_param_frac_mode_decimal, 0, {"FREQ"}},
      {-240, 240, 0, 0, k_unit_param_type_none, 1, k_unit_param_frac_mode_decimal, 0, {"INTERVAL"}},
      {0, 1000, 500, 400, k_unit_param_type_none, 3, k_unit_param_frac_mode_decimal, 0, {"REGEN"}},
      {0, 1000, 500, 74, k_unit_param_type_none, 0, k_unit_param_frac_mode_decimal, 0, {"DRIVE"}},
      {0, 2, 1, 0, k_unit_param_type_none, 0, k_unit_param_frac_mode_decimal, 0, {"ROUTING"}},
      {0, 1000, 500, 0, k_unit_param_type_none, 3, k_unit_param_frac_mode_decimal, 0, {"ENV AMT"}},
      {0, 1000, 500, 0, k_unit_param_type_none, 3, k_unit_param_frac_mode_decimal, 0, {"WOBBLE"}},
      {-1000, 1000, 0, 1000, k_unit_param_type_drywet, 1, k_unit_param_frac_mode_decimal, 0, {"DRY WET"}}
    }
  },
  .default_mappings = {
    {k_genericfx_param_assign_x,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,360},
    {k_genericfx_param_assign_y,k_genericfx_curve_linear,k_genericfx_curve_unipolar,-240,240,0},
    {k_genericfx_param_assign_depth,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,400},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,74},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,2,0},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,0},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,0},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,-1000,1000,1000}
  }
};
