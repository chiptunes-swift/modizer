#pragma once

//#include "devlegcy.h"

/* an interface for the OKIM6258 and similar chips */

/*typedef struct _okim6258_interface okim6258_interface;
struct _okim6258_interface
{
	int divider;
	int adpcm_type;
	int output_12bits;
};*/


#define FOSC_DIV_BY_1024	0
#define FOSC_DIV_BY_768		1
#define FOSC_DIV_BY_512		2

#define TYPE_3BITS      	0
#define TYPE_4BITS			1

#define	OUTPUT_10BITS		0
#define	OUTPUT_12BITS		1

//TODO:  MODIZER changes start / YOYOFR
void okim6258_set_mute_mask(UINT8 ChipID, UINT32 MuteMask);
//TODO:  MODIZER changes end / YOYOFR

void okim6258_update(UINT8 ChipID, stream_sample_t **outputs, int samples);
int device_start_okim6258(UINT8 ChipID, int clock, int divider, int adpcm_type, int output_12bits);
void device_stop_okim6258(UINT8 ChipID);
void device_reset_okim6258(UINT8 ChipID);

//void okim6258_set_divider(running_device *device, int val);
//void okim6258_set_clock(running_device *device, int val);
//int okim6258_get_vclk(running_device *device);

void okim6258_set_divider(UINT8 ChipID, int val);
void okim6258_set_clock(UINT8 ChipID, int val);
int okim6258_get_vclk(UINT8 ChipID);

//READ8_DEVICE_HANDLER( okim6258_status_r );
//WRITE8_DEVICE_HANDLER( okim6258_data_w );
//WRITE8_DEVICE_HANDLER( okim6258_ctrl_w );

/*UINT8 okim6258_status_r(UINT8 ChipID, offs_t offset);
void okim6258_data_w(UINT8 ChipID, offs_t offset, UINT8 data);
void okim6258_ctrl_w(UINT8 ChipID, offs_t offset, UINT8 data);*/
void okim6258_write(UINT8 ChipID, UINT8 Port, UINT8 Data);

void okim6258_set_options(UINT16 Options);
void okim6258_set_srchg_cb(UINT8 ChipID, SRATE_CALLBACK CallbackFunc, void* DataPtr);

//DECLARE_LEGACY_SOUND_DEVICE(OKIM6258, okim6258);
