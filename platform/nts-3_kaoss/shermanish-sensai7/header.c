#include "unit_genericfx.h"
// Distinct unit ID allows naked Shermanish and Sensai7 Shermanish to coexist.
const __unit_header genericfx_unit_header_t unit_header = {
  .common = {
    .header_size=sizeof(genericfx_unit_header_t),
    .target=UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
    .api=UNIT_API_VERSION,
    .dev_id=0x54414D48U, .unit_id=0x00000009U,
    .version=0x00010000U, .name="Shermanish S7", .num_params=8,
    .params={
      {0,16383,8192,5941,k_unit_param_type_strings,0,k_unit_param_frac_mode_decimal,0,{"X FREQ+INTERVAL"}},
      {0,16383,8192,6566,k_unit_param_type_strings,0,k_unit_param_frac_mode_decimal,0,{"Y REGEN+DRIVE"}},
      {0,16383,8192,64,k_unit_param_type_strings,0,k_unit_param_frac_mode_decimal,0,{"D MORPH+ENV"}},
      {-120,120,0,0,k_unit_param_type_none,1,k_unit_param_frac_mode_decimal,0,{"INTERVAL SHIFT"}},
      {0,1000,1000,1000,k_unit_param_type_none,3,k_unit_param_frac_mode_decimal,0,{"DRIVE MAX"}},
      {0,1000,1000,1000,k_unit_param_type_none,3,k_unit_param_frac_mode_decimal,0,{"ENV MAX"}},
      {0,9,2,2,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"ENV TIME"}},
      {0,2,0,0,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"ROUTING"}}
    }
  },
  .default_mappings={
    {k_genericfx_param_assign_x,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,16383,5941},
    {k_genericfx_param_assign_y,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,16383,6566},
    {k_genericfx_param_assign_depth,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,16383,64},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,-120,120,0},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,1000},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,1000},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,9,2},
    {k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,2,0}
  }
};
