/*****************************************************************************
 *
 * Filename:
 * ---------
 *    charging_pmic.c
 *
 * Project:
 * --------
 *   ALPS_Software
 *
 * Description:
 * ------------
 *   This file implements the interface between BMT and ADC scheduler.
 *
 * Author:
 * -------
 *  Oscar Liu
 *
 *============================================================================
  * $Revision:   1.0  $
 * $Modtime:   11 Aug 2005 10:28:16  $
 * $Log:   //mtkvs01/vmdata/Maui_sw/archives/mcu/hal/peripheral/inc/bmt_chr_setting.h-arc  $
 *             HISTORY
 * Below this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *------------------------------------------------------------------------------
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by PVCS VM. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/
#include <mach/charging.h>
#include "bq24296.h"
#include <mach/upmu_common.h>
#include <mach/mt_gpio.h>
#include <cust_gpio_usage.h>
#include <mach/upmu_hw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>
#include <cust_charging.h>

#include <mach/upmu_sw.h>
#include <linux/xlog.h>
#include <linux/delay.h>
#include <mach/mt_sleep.h>
#include <mach/mt_boot.h>
#include <mach/system.h>

#include <linux/spinlock.h>
#include <cust_charging.h>
#include <mach/mt_gpio.h>
#include <linux/wakelock.h>
#include <mach/mt6311.h>

 // ============================================================ //
 //define
 // ============================================================ //
#define STATUS_OK	0
#define STATUS_UNSUPPORTED	-1
#define GETARRAYNUM(array) (sizeof(array)/sizeof(array[0]))


 // ============================================================ //
 //global variable
 // ============================================================ //
#define GPIO_CHR_CE_PIN  GPIO124
int gpio_off_dir  = GPIO_DIR_OUT;
int gpio_off_out  = GPIO_OUT_ONE;
int gpio_on_dir   = GPIO_DIR_OUT;
int gpio_on_out   = GPIO_OUT_ZERO;

kal_bool charging_type_det_done = KAL_TRUE;

static CHARGER_TYPE g_charger_type = CHARGER_UNKNOWN;
const kal_uint32 VBAT_CV_VTH[]=
{
	3504000,    3520000,    3536000,    3552000,
	3568000,    3584000,    3600000,    3616000,
	3632000,    3648000,    3664000,    3680000,
	3696000,    3712000,	3728000,    3744000,
	3760000,    3776000,    3792000,    3808000,
	3824000,    3840000,    3856000,    3872000,
	3888000,    3904000,    3920000,    3936000,
	3952000,    3968000,    3984000,    4000000,
	4016000,    4032000,    4048000,    4064000,
	4080000,    4096000,    4112000,    4128000,
	4144000,    4160000,    4176000,    4192000,	
	4208000,    4224000,    4240000,    4256000,
	4272000,    4288000,    4304000
};

const kal_uint32 CS_VTH[]=
{
	51200,  57600,  64000,  70400,
	76800,  83200,  89600,  96000,
	102400, 108800, 115200, 121600,
	128000, 134400, 140800, 147200,
	153600, 160000, 166400, 172800,
	179200, 185600, 192000, 198400,
	204800, 211200, 217600, 224000
}; 

 const kal_uint32 INPUT_CS_VTH[]=
 {
	 CHARGE_CURRENT_100_00_MA,  CHARGE_CURRENT_150_00_MA,	CHARGE_CURRENT_500_00_MA,  CHARGE_CURRENT_900_00_MA,
	 CHARGE_CURRENT_1000_00_MA, CHARGE_CURRENT_1500_00_MA,  CHARGE_CURRENT_2000_00_MA,  CHARGE_CURRENT_MAX
 }; 

 const kal_uint32 VCDT_HV_VTH[]=
 {
	  BATTERY_VOLT_04_200000_V, BATTERY_VOLT_04_250000_V,	  BATTERY_VOLT_04_300000_V,   BATTERY_VOLT_04_350000_V,
	  BATTERY_VOLT_04_400000_V, BATTERY_VOLT_04_450000_V,	  BATTERY_VOLT_04_500000_V,   BATTERY_VOLT_04_550000_V,
	  BATTERY_VOLT_04_600000_V, BATTERY_VOLT_06_000000_V,	  BATTERY_VOLT_06_500000_V,   BATTERY_VOLT_07_000000_V,
	  BATTERY_VOLT_07_500000_V, BATTERY_VOLT_08_500000_V,	  BATTERY_VOLT_09_500000_V,   BATTERY_VOLT_10_500000_V		  
 };

static kal_uint32 charging_error = false;
 // ============================================================ //
 // function prototype
 // ============================================================ //
 
static kal_uint32 charging_get_error_state(void);
static kal_uint32 charging_set_error_state(void *data);
 
 // ============================================================ //
 //extern variable
 // ============================================================ //
 
 // ============================================================ //
 //extern function
 // ============================================================ //
 extern kal_uint32 upmu_get_reg_value(kal_uint32 reg);
 extern bool mt_usb_is_device(void);
 extern void Charger_Detect_Init(void);
 extern void Charger_Detect_Release(void);
 extern void mt_power_off(void);
 extern bool get_usb_current_unlimited(void);
 extern int hw_charging_get_charger_type(void);
 // ============================================================ //
 kal_uint32 charging_value_to_parameter(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	if (val < array_size)
	{
		return parameter[val];
	}
	else
	{
		battery_xlog_printk(BAT_LOG_CRTI, "Can't find the parameter \r\n");	
		return parameter[0];
	}
}

 
 kal_uint32 charging_parameter_to_value(const kal_uint32 *parameter, const kal_uint32 array_size, const kal_uint32 val)
{
	kal_uint32 i;

    battery_xlog_printk(BAT_LOG_CRTI, "array_size = %d \r\n", array_size);
    
	for(i=0;i<array_size;i++)
	{
		if (val == *(parameter + i))
		{
				return i;
		}
	}

    battery_xlog_printk(BAT_LOG_CRTI, "NO register value match. val=%d\r\n", val);
	//TODO: ASSERT(0);	// not find the value
	return 0;
}


 static kal_uint32 bmt_find_closest_level(const kal_uint32 *pList,kal_uint32 number,kal_uint32 level)
 {
	 kal_uint32 i;
	 kal_uint32 max_value_in_last_element;
 
	 if(pList[0] < pList[1])
		 max_value_in_last_element = KAL_TRUE;
	 else
		 max_value_in_last_element = KAL_FALSE;
 
	 if(max_value_in_last_element == KAL_TRUE)
	 {
		 for(i = (number-1); i != 0; i--)	 //max value in the last element
		 {
			 if(pList[i] <= level)
			 {
				 return pList[i];
			 }	  
		 }

 		 battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, small value first \r\n");
		 return pList[0];
		 //return CHARGE_CURRENT_0_00_MA;
	 }
	 else
	 {
		 for(i = 0; i< number; i++)  // max value in the first element
		 {
			 if(pList[i] <= level)
			 {
				 return pList[i];
			 }	  
		 }

		 battery_xlog_printk(BAT_LOG_CRTI, "Can't find closest level, large value first \r\n"); 	 
		 return pList[number -1];
  		 //return CHARGE_CURRENT_0_00_MA;
	 }
 }

 static kal_uint32 is_chr_det(void)
 {
	 kal_uint32 val=0;
   
	 val = mt6325_upmu_get_rgs_chrdet();
 
	 battery_xlog_printk(BAT_LOG_CRTI,"[is_chr_det] %d\n", val);
	 
	 return val;
 }
 static kal_uint32 charging_hw_init(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	
//	upmu_set_rg_bc11_bb_ctrl(1);    //BC11_BB_CTRL    
//    upmu_set_rg_bc11_rst(1);        //BC11_RST
	
#if 0 //no use
    //pull PSEL low
    mt_set_gpio_mode(GPIO_CHR_PSEL_PIN,GPIO_MODE_GPIO);  
    mt_set_gpio_dir(GPIO_CHR_PSEL_PIN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CHR_PSEL_PIN,GPIO_OUT_ZERO);
#endif    
    
    //pull CE low
#if 1//no use
    mt_set_gpio_mode(GPIO_CHR_CE_PIN,GPIO_MODE_GPIO);  
    mt_set_gpio_dir(GPIO_CHR_CE_PIN,GPIO_DIR_OUT);
    mt_set_gpio_out(GPIO_CHR_CE_PIN,GPIO_OUT_ZERO);    
#endif

    //battery_xlog_printk(BAT_LOG_FULL, "gpio_number=0x%x,gpio_on_mode=%d,gpio_off_mode=%d\n", gpio_number, gpio_on_mode, gpio_off_mode);	

    bq24296_set_en_hiz(0x0);
    /* lenovo-sw zhangrc2 change vdpm from 4.68v to 4.36v 2015-01-08 begin */
    bq24296_set_vindpm(0x6); //VIN DPM check 4.36V
     /* lenovo-sw zhangrc2 change vdpm from 4.68v to 4.36v 2015-01-08 end */
    bq24296_set_reg_rst(0x0);
    bq24296_set_wdt_rst(0x1); //Kick watchdog	
    bq24296_set_sys_min(0x5); //Minimum system voltage 3.5V	
    bq24296_set_iprechg(0x3); //Precharge current 512mA
    bq24296_set_iterm(0x0); //Termination current 128mA

#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
    bq24296_set_vreg(0x35); // lenovo-sw zhangrc2 change cv from 4.304 to 4.352  
#else
    bq24296_set_vreg(0x2C); //VREG 4.208V
#endif    
  
    bq24296_set_batlowv(0x1); //BATLOWV 3.0V
    bq24296_set_vrechg(0x0); //VRECHG 0.1V (4.108V)
    bq24296_set_en_term(0x1); //Enable termination
    bq24296_set_watchdog(0x1); //WDT 40s
    bq24296_set_en_timer(0x0); //Disable charge timer
    bq24296_set_int_mask(0x0); //Disable fault interrupt
    
    return status;
 }


 static kal_uint32 charging_dump_register(void *data)
 {
 	kal_uint32 status = STATUS_OK;

    battery_xlog_printk(BAT_LOG_CRTI, "charging_dump_register\r\n");

	bq24296_dump_register();
   	
	return status;
 }	


 static kal_uint32 charging_enable(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 enable = *(kal_uint32*)(data);

	if(KAL_TRUE == enable)
	{
		bq24296_set_en_hiz(0x0);	            	
        bq24296_set_chg_config(0x1); // charger enable
	}
	else
	{
#if defined(CONFIG_USB_MTK_HDRC_HCD)
   		if(mt_usb_is_device())
#endif 			
    	{
	        bq24296_set_chg_config(0x0);
    	}
	}
		
	return status;
 }


 static kal_uint32 charging_set_cv_voltage(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint16 register_value;
	kal_uint32 cv_value = *(kal_uint32 *)(data);	
	
	if(cv_value == BATTERY_VOLT_04_200000_V)
	{
#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
        //highest of voltage will be 4.3V, because powerpath limitation
		cv_value = 4304000;		
#else
	    //use nearest value
		cv_value = 4208000;
#endif	    
 	}
	register_value = charging_parameter_to_value(VBAT_CV_VTH, GETARRAYNUM(VBAT_CV_VTH), cv_value);

#if defined(HIGH_BATTERY_VOLTAGE_SUPPORT)
    bq24296_set_vreg(0x35); // lenovo-sw zhangrc2 change cv from 4.304 to 4.352  
#else
    bq24296_set_vreg(0x2C); //VREG 4.208V
#endif 
	//bq24296_set_vreg(register_value); 

	return status;
 } 	


 static kal_uint32 charging_get_current(void *data)
 {
    kal_uint32 status = STATUS_OK;
    //kal_uint32 array_size;
    //kal_uint8 reg_value;
    
    kal_uint8 ret_val=0;    
    kal_uint8 ret_force_20pct=0;
	    
    //Get current level
    bq24296_read_interface(bq24296_CON2, &ret_val, CON2_ICHG_MASK, CON2_ICHG_SHIFT);
    
    //Get Force 20% option
    bq24296_read_interface(bq24296_CON2, &ret_force_20pct, CON2_FORCE_20PCT_MASK, CON2_FORCE_20PCT_SHIFT);
    
    //Parsing
    ret_val = (ret_val*64) + 512;
    
    if (ret_force_20pct == 0)
    {
        //Get current level
        //array_size = GETARRAYNUM(CS_VTH);
        //*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH,array_size,reg_value);
        *(kal_uint32 *)data = ret_val;
    }   
    else
    {
        //Get current level
        //array_size = GETARRAYNUM(CS_VTH_20PCT);
        //*(kal_uint32 *)data = charging_value_to_parameter(CS_VTH,array_size,reg_value);
        //return (int)(ret_val<<1)/10;
        *(kal_uint32 *)data = (int)(ret_val<<1)/10;
    }   
	
    return status;
 }  
  


 static kal_uint32 charging_set_current(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;
	kal_uint32 current_value = *(kal_uint32 *)data;
	
	array_size = GETARRAYNUM(CS_VTH);
	set_chr_current = bmt_find_closest_level(CS_VTH, array_size, current_value);
	register_value = charging_parameter_to_value(CS_VTH, array_size ,set_chr_current);
	bq24296_set_ichg(register_value);
	
	return status;
 } 	
 

 static kal_uint32 charging_set_input_current(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 set_chr_current;
	kal_uint32 array_size;
	kal_uint32 register_value;
    
	array_size = GETARRAYNUM(INPUT_CS_VTH);
	set_chr_current = bmt_find_closest_level(INPUT_CS_VTH, array_size, *(kal_uint32 *)data);
	register_value = charging_parameter_to_value(INPUT_CS_VTH, array_size ,set_chr_current);	
        
    bq24296_set_iinlim(register_value);

	return status;
 } 	


 static kal_uint32 charging_get_charging_status(void *data)
 {
 	kal_uint32 status = STATUS_OK;
	kal_uint32 ret_val;

	ret_val = bq24296_get_chrg_stat();
	
	if(ret_val == 0x3)
		*(kal_uint32 *)data = KAL_TRUE;
	else
		*(kal_uint32 *)data = KAL_FALSE;
	
	return status;
 } 	


 static kal_uint32 charging_reset_watch_dog_timer(void *data)
 {
	 kal_uint32 status = STATUS_OK;
 
     battery_xlog_printk(BAT_LOG_CRTI, "charging_reset_watch_dog_timer\r\n");
 
	 bq24296_set_wdt_rst(0x1); //Kick watchdog
	 
	 return status;
 }
 
 
  static kal_uint32 charging_set_hv_threshold(void *data)
  {
	 kal_uint32 status = STATUS_OK;
 
	 kal_uint32 set_hv_voltage;
	 kal_uint32 array_size;
	 kal_uint16 register_value;
	 kal_uint32 voltage = *(kal_uint32*)(data);
	 
	 array_size = GETARRAYNUM(VCDT_HV_VTH);
	 set_hv_voltage = bmt_find_closest_level(VCDT_HV_VTH, array_size, voltage);
	 register_value = charging_parameter_to_value(VCDT_HV_VTH, array_size ,set_hv_voltage);
	 mt6325_upmu_set_rg_vcdt_hv_vth(register_value);
 
	 return status;
  }
 
 
  static kal_uint32 charging_get_hv_status(void *data)
  {
	   kal_uint32 status = STATUS_OK;
 
	   *(kal_bool*)(data) = mt6325_upmu_get_rgs_vcdt_hv_det();
	   
	   return status;
  }


 static kal_uint32 charging_get_battery_status(void *data)
 {
		 kal_uint32 status = STATUS_OK;
	 	 kal_uint32 val = 0;
#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
		 *(kal_bool*)(data) = 0; // battery exist
		 battery_xlog_printk(BAT_LOG_CRTI,"[charging_get_battery_status] battery exist for bring up.\n");
#else
	pmic_read_interface( MT6325_CHR_CON7, &val, MT6325_PMIC_BATON_TDET_EN_MASK, MT6325_PMIC_BATON_TDET_EN_SHIFT);
	battery_xlog_printk(BAT_LOG_FULL,"[charging_get_battery_status] BATON_TDET_EN = %d\n", val);
	if (val) {
		 mt6325_upmu_set_baton_tdet_en(1);
		 mt6325_upmu_set_rg_baton_en(1);
		 *(kal_bool*)(data) = mt6325_upmu_get_rgs_baton_undet();
	} else {
		*(kal_bool*)(data) =  KAL_FALSE;
	}
#endif
		  
		 return status;
 }


 static kal_uint32 charging_get_charger_det_status(void *data)
 {

		 kal_uint32 status = STATUS_OK;
		 kal_uint32 val = 0;
	 
#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
		 val = 1;
		 battery_xlog_printk(BAT_LOG_CRTI,"[charging_get_charger_det_status] charger exist for bring up.\n"); 
#else    
		 #if !defined(CONFIG_MTK_DUAL_INPUT_CHARGER_SUPPORT)
		 val = mt6325_upmu_get_rgs_chrdet();
		 #else
		 if(((g_diso_state >> 1) & 0x3) != 0x0 || mt6325_upmu_get_rgs_chrdet())
		     val = KAL_TRUE;
		 else
		     val = KAL_FALSE;
		 #endif
#endif
		 
		 *(kal_bool*)(data) = val;
		 if(val == 0)
			 g_charger_type = CHARGER_UNKNOWN;
			   
		 return status;
 }


kal_bool charging_type_detection_done(void)
{
	 return charging_type_det_done;
}

//extern CHARGER_TYPE hw_charger_type_detection(void);

 static kal_uint32 charging_get_charger_type(void *data)
 {

		kal_uint32 status = STATUS_OK;
		 
#if defined(CONFIG_POWER_EXT) || defined(CONFIG_MTK_FPGA)
		*(CHARGER_TYPE*)(data) = STANDARD_HOST;
#else
		
    #if defined(MTK_WIRELESS_CHARGER_SUPPORT)
		int wireless_state = 0;
		if(wireless_charger_gpio_number!=0)
		{
			wireless_state = mt_get_gpio_in(wireless_charger_gpio_number);
			if(wireless_state == WIRELESS_CHARGER_EXIST_STATE)
			{
				*(CHARGER_TYPE*)(data) = WIRELESS_CHARGER;
				battery_xlog_printk(BAT_LOG_CRTI, "WIRELESS_CHARGER!\n");
				return status;
			}
		}
		else
		{
			battery_xlog_printk(BAT_LOG_CRTI, "wireless_charger_gpio_number=%d\n", wireless_charger_gpio_number);
		}
		
		if(g_charger_type!=CHARGER_UNKNOWN && g_charger_type!=WIRELESS_CHARGER)
		{
			*(CHARGER_TYPE*)(data) = g_charger_type;
			battery_xlog_printk(BAT_LOG_CRTI, "return %d!\n", g_charger_type);
			return status;
		}
    #endif
		
		if(is_chr_det()==0)
		{
			g_charger_type = CHARGER_UNKNOWN; 
			*(CHARGER_TYPE*)(data) = CHARGER_UNKNOWN;
			battery_xlog_printk(BAT_LOG_CRTI, "[charging_get_charger_type] return CHARGER_UNKNOWN\n");
			return status;
		}
		
		charging_type_det_done = KAL_FALSE;
	
		*(CHARGER_TYPE*)(data) = hw_charging_get_charger_type();
		//*(CHARGER_TYPE*)(data) = STANDARD_HOST;
		//*(CHARGER_TYPE*)(data) = STANDARD_CHARGER;
	
		charging_type_det_done = KAL_TRUE;
	
		g_charger_type = *(CHARGER_TYPE*)(data);
		
#endif
	
		return status;

}

static kal_uint32 charging_get_is_pcm_timer_trigger(void *data)
{
    kal_uint32 status = STATUS_OK;

    if(slp_get_wake_reason() == WR_PCM_TIMER)
        *(kal_bool*)(data) = KAL_TRUE;
    else
        *(kal_bool*)(data) = KAL_FALSE;

    battery_xlog_printk(BAT_LOG_CRTI, "slp_get_wake_reason=%d\n", slp_get_wake_reason());
       
    return status;
}

static kal_uint32 charging_set_platform_reset(void *data)
{
    kal_uint32 status = STATUS_OK;

    battery_xlog_printk(BAT_LOG_CRTI, "charging_set_platform_reset\n");
 
    arch_reset(0,NULL);
        
    return status;
}

static kal_uint32 charging_get_platfrom_boot_mode(void *data)
{
    kal_uint32 status = STATUS_OK;
  
    *(kal_uint32*)(data) = get_boot_mode();

    battery_xlog_printk(BAT_LOG_CRTI, "get_boot_mode=%d\n", get_boot_mode());
         
    return status;
}

static kal_uint32 charging_set_power_off(void *data)
{
    kal_uint32 status = STATUS_OK;
  
 //   battery_xlog_printk(BAT_LOG_CRTI, "charging_set_power_off=%d\n");
    mt_power_off();
         
    return status;
}

static kal_uint32 charging_get_power_source(void *data)
{
	kal_uint32 status = STATUS_UNSUPPORTED;

	return status;
}

static kal_uint32 charging_get_csdac_full_flag(void *data)
{
	return STATUS_UNSUPPORTED;	
}

static kal_uint32 charging_set_ta_current_pattern(void *data)
{
	return STATUS_UNSUPPORTED;	
}

static kal_uint32 charging_get_error_state(void)
{
	return charging_error;
}

static kal_uint32 charging_set_error_state(void *data)
{
	kal_uint32 status = STATUS_OK;
	charging_error = *(kal_uint32*)(data);
	
	return status;
}

 static kal_uint32 (* const charging_func[CHARGING_CMD_NUMBER])(void *data)=
 {
 	 charging_hw_init
	,charging_dump_register  	
	,charging_enable
	,charging_set_cv_voltage
	,charging_get_current
	,charging_set_current
	,charging_set_input_current
	,charging_get_charging_status
	,charging_reset_watch_dog_timer
	,charging_set_hv_threshold
	,charging_get_hv_status
	,charging_get_battery_status
	,charging_get_charger_det_status
	,charging_get_charger_type
	,charging_get_is_pcm_timer_trigger
	,charging_set_platform_reset
	,charging_get_platfrom_boot_mode
	,charging_set_power_off
	,charging_get_power_source
	,charging_get_csdac_full_flag
	,charging_set_ta_current_pattern
	,charging_set_error_state
 };

 
 /*
 * FUNCTION
 *		Internal_chr_control_handler
 *
 * DESCRIPTION															 
 *		 This function is called to set the charger hw
 *
 * CALLS  
 *
 * PARAMETERS
 *		None
 *	 
 * RETURNS
 *		
 *
 * GLOBALS AFFECTED
 *	   None
 */
 kal_int32 chr_control_interface(CHARGING_CTRL_CMD cmd, void *data)
 {
	 kal_int32 status;
	 if(cmd < CHARGING_CMD_NUMBER)
		 status = charging_func[cmd](data);
	 else
		 return STATUS_UNSUPPORTED;
 
	 return status;
 }


