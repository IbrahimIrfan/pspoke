#include <nitro.h>
#include <string.h>
#include "port/sim_config_prj.h"
/* Native PSP runs the ordinary 30-update game cadence. No config file or UI. */
static SIM_Config_prj_type config;
void SIM_Config_prj_LoadDefaults(SIM_Config_prj_type *p){memset(p,0,sizeof(*p));}
void SIM_Config_prj_init(void){SIM_Config_prj_LoadDefaults(&config);}
SIM_Config_prj_type *SIM_Config_prj_GetConfig(void){return &config;}
