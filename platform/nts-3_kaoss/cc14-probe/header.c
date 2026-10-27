#include "unit_genericfx.h"
const __unit_header genericfx_unit_header_t unit_header = {
.common = {
.header_size=sizeof(genericfx_unit_header_t),
.target=UNIT_TARGET_PLATFORM | k_unit_module_genericfx,
.api=UNIT_API_VERSION, .dev_id=0x54414D48U, .unit_id=0x00000008U,
.version=0x00010000U, .name="cc14probe", .num_params=6,
.params={
{0,16383,8192,8192,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"X RAW"}},
{0,16383,8192,8192,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"Y RAW"}},
{0,16383,8192,8192,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"D RAW"}},
{0,2,0,0,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"PAIR"}},
{0,1,0,0,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"FINE/COARSE"}},
{0,1000,250,250,k_unit_param_type_none,0,k_unit_param_frac_mode_decimal,0,{"LEVEL"}}
}},
.default_mappings={
{k_genericfx_param_assign_x,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,16383,8192},
{k_genericfx_param_assign_y,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,16383,8192},
{k_genericfx_param_assign_depth,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,16383,8192},
{k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,2,0},
{k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1,0},
{k_genericfx_param_assign_none,k_genericfx_curve_linear,k_genericfx_curve_unipolar,0,1000,250}
}};
