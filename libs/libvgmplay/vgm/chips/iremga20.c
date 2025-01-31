/*********************************************************

Irem GA20 PCM Sound Chip

It's not currently known whether this chip is stereo.


Revisions:

04-15-2002 Acho A. Tang
- rewrote channel mixing
- added prelimenary volume and sample rate emulation

05-30-2002 Acho A. Tang
- applied hyperbolic gain control to volume and used
  a musical-note style progression in sample rate
  calculation(still very inaccurate)

02-18-2004 R. Belmont
- sample rate calculation reverse-engineered.
  Thanks to Fujix, Yasuhiro Ogawa, the Guru, and Tormod
  for real PCB samples that made this possible.

02-03-2007 R. Belmont
- Cleaned up faux x86 assembly.

*********************************************************/

//#include "emu.h"
#include <stdlib.h>
#include <string.h>	// for memset
#include <stddef.h>	// for NULL
#include "mamedef.h"
#include "iremga20.h"

#define MAX_VOL 256

struct IremGA20_channel_def
{
	UINT32 rate;
	//UINT32 size;
	UINT32 start;
	UINT32 pos;
	UINT32 frac;
	UINT32 end;
	UINT32 volume;
	UINT32 pan;
	//UINT32 effect;
	UINT8 play;
	UINT8 Muted;
};

typedef struct _ga20_state ga20_state;
struct _ga20_state
{
	UINT8 *rom;
	UINT32 rom_size;
	//sound_stream * stream;
	UINT16 regs[0x40];
	struct IremGA20_channel_def channel[4];
};


#define MAX_CHIPS	0x02
static ga20_state GA20Data[MAX_CHIPS];

/*INLINE ga20_state *get_safe_token(device_t *device)
{
	assert(device != NULL);
	assert(device->type() == IREMGA20);
	return (ga20_state *)downcast<legacy_device_base *>(device)->token();
}*/

//TODO:  MODIZER changes start / YOYOFR
#include "../../../../src/ModizerVoicesData.h"
//TODO:  MODIZER changes end / YOYOFR


//static STREAM_UPDATE( IremGA20_update )
void IremGA20_update(UINT8 ChipID, stream_sample_t **outputs, int samples)
{
	//ga20_state *chip = (ga20_state *)param;
	ga20_state *chip = &GA20Data[ChipID];
	UINT32 rate[4], pos[4], frac[4], end[4], vol[4], play[4];
	UINT8 *pSamples;
	stream_sample_t *outL, *outR;
	int i, sampleout;
    
    //TODO:  MODIZER changes start / YOYOFR
    //search first voice linked to current chip
    int m_voice_ofs=-1;
    int m_total_channels=4;
    for (int ii=0;ii<=SOUND_MAXVOICES_BUFFER_FX-m_total_channels;ii++) {
        if (((m_voice_ChipID[ii]&0x7F)==(m_voice_current_system&0x7F))&&(((m_voice_ChipID[ii]>>8)&0xFF)==m_voice_current_systemSub)) {
            m_voice_ofs=ii;
            break;
        }
    }
    int smplIncr=44100*1024/m_voice_current_samplerate+1;
    //TODO:  MODIZER changes end / YOYOFR

	/* precache some values */
	for (i=0; i < 4; i++)
	{
		rate[i] = chip->channel[i].rate;
		pos[i] = chip->channel[i].pos;
		frac[i] = chip->channel[i].frac;
		end[i] = chip->channel[i].end - 0x20;
		vol[i] = chip->channel[i].volume;
		play[i] = (! chip->channel[i].Muted) ? chip->channel[i].play : 0;
	}

	i = samples;
	pSamples = chip->rom;
	outL = outputs[0];
	outR = outputs[1];

	for (i = 0; i < samples; i++)
	{
		sampleout = 0;
        
        //TODO:  MODIZER changes start / YOYOFR
        if (m_voice_ofs>=0) {
            int ofs_start=m_voice_current_ptr[m_voice_ofs+0];
            int ofs_end=(m_voice_current_ptr[m_voice_ofs+0]+smplIncr);
            
            if ((ofs_end>>10)>(ofs_start>>10))
            for (;;) {
                for (int ii=0;ii<4;ii++) {
                    if (play[ii]) { m_voice_buff[m_voice_ofs+ii][(ofs_start>>10)&(SOUND_BUFFER_SIZE_SAMPLE-1)]=LIMIT8((((int)(pSamples[pos[ii]])-0x80)*(int)(vol[ii]))>>8);
                        //LIMIT8(((((int)(pSamples[pos[ii]]) - 0x80) * vol[ii])>>8));
                    }
                }
                ofs_start+=1024;
                if (ofs_start>=ofs_end) break;
            }
            while ((ofs_end>>10)>SOUND_BUFFER_SIZE_SAMPLE) ofs_end-=(SOUND_BUFFER_SIZE_SAMPLE<<10);
            for (int ii=0;ii<4;ii++) m_voice_current_ptr[m_voice_ofs+ii]=ofs_end;
        }
        //TODO:  MODIZER changes end / YOYOFR

		// update the 4 channels inline
		if (play[0])
		{
			sampleout += (pSamples[pos[0]] - 0x80) * vol[0];
			frac[0] += rate[0];
			pos[0] += frac[0] >> 24;
			frac[0] &= 0xffffff;
			play[0] = (pos[0] < end[0]);
		}
		if (play[1])
		{
			sampleout += (pSamples[pos[1]] - 0x80) * vol[1];
			frac[1] += rate[1];
			pos[1] += frac[1] >> 24;
			frac[1] &= 0xffffff;
			play[1] = (pos[1] < end[1]);
		}
		if (play[2])
		{
			sampleout += (pSamples[pos[2]] - 0x80) * vol[2];
			frac[2] += rate[2];
			pos[2] += frac[2] >> 24;
			frac[2] &= 0xffffff;
			play[2] = (pos[2] < end[2]);
		}
		if (play[3])
		{
			sampleout += (pSamples[pos[3]] - 0x80) * vol[3];
			frac[3] += rate[3];
			pos[3] += frac[3] >> 24;
			frac[3] &= 0xffffff;
			play[3] = (pos[3] < end[3]);
		}

		sampleout >>= 2;
		outL[i] = sampleout;
		outR[i] = sampleout;
	}

	/* update the regs now */
	for (i=0; i < 4; i++)
	{
		chip->channel[i].pos = pos[i];
		chip->channel[i].frac = frac[i];
		if (! chip->channel[i].Muted)
			chip->channel[i].play = play[i];
	}
}

//WRITE8_DEVICE_HANDLER( irem_ga20_w )
void irem_ga20_w(UINT8 ChipID, offs_t offset, UINT8 data)
{
	//ga20_state *chip = get_safe_token(device);
	ga20_state *chip = &GA20Data[ChipID];
	int channel;

	//logerror("GA20:  Offset %02x, data %04x\n",offset,data);

	//chip->stream->update();

	channel = offset >> 3;

	chip->regs[offset] = data;

	switch (offset & 0x7)
	{
		case 0: /* start address low */
			chip->channel[channel].start = ((chip->channel[channel].start)&0xff000) | (data<<4);
			break;

		case 1: /* start address high */
			chip->channel[channel].start = ((chip->channel[channel].start)&0x00ff0) | (data<<12);
			break;

		case 2: /* end address low */
			chip->channel[channel].end = ((chip->channel[channel].end)&0xff000) | (data<<4);
			break;

		case 3: /* end address high */
			chip->channel[channel].end = ((chip->channel[channel].end)&0x00ff0) | (data<<12);
			break;

		case 4:
			chip->channel[channel].rate = 0x1000000 / (256 - data);
			break;

		case 5: //AT: gain control
			chip->channel[channel].volume = (data * MAX_VOL) / (data + 10);
			break;

		case 6: //AT: this is always written 2(enabling both channels?)
			chip->channel[channel].play = data;
			chip->channel[channel].pos = chip->channel[channel].start;
			chip->channel[channel].frac = 0;
			break;
	}
}

//READ8_DEVICE_HANDLER( irem_ga20_r )
UINT8 irem_ga20_r(UINT8 ChipID, offs_t offset)
{
	//ga20_state *chip = get_safe_token(device);
	ga20_state *chip = &GA20Data[ChipID];
	int channel;

	//chip->stream->update();

	channel = offset >> 3;

	switch (offset & 0x7)
	{
		case 7:	// voice status.  bit 0 is 1 if active. (routine around 0xccc in rtypeleo)
			return chip->channel[channel].play ? 1 : 0;

		default:
			logerror("GA20: read unk. register %d, channel %d\n", offset & 0xf, channel);
			break;
	}

	return 0;
}

static void iremga20_reset(ga20_state *chip)
{
	int i;

	for( i = 0; i < 4; i++ ) {
		chip->channel[i].rate = 0;
		//chip->channel[i].size = 0;
		chip->channel[i].start = 0;
		chip->channel[i].pos = 0;
		chip->channel[i].frac = 0;
		chip->channel[i].end = 0;
		chip->channel[i].volume = 0;
		chip->channel[i].pan = 0;
		//chip->channel[i].effect = 0;
		chip->channel[i].play = 0;
	}
}


//static DEVICE_RESET( iremga20 )
void device_reset_iremga20(UINT8 ChipID)
{
	//iremga20_reset(get_safe_token(device));
	ga20_state *chip = &GA20Data[ChipID];
	
	iremga20_reset(chip);
	memset(chip->regs, 0x00, 0x40 * sizeof(UINT16));
}

//static DEVICE_START( iremga20 )
int device_start_iremga20(UINT8 ChipID, int clock)
{
	//ga20_state *chip = get_safe_token(device);
	ga20_state *chip;
	int i;

	if (ChipID >= MAX_CHIPS)
		return 0;
	
	chip = &GA20Data[ChipID];

	/* Initialize our chip structure */
	//chip->rom = *device->region();
	//chip->rom_size = device->region()->bytes();
	chip->rom = NULL;
	chip->rom_size = 0x00;

	iremga20_reset(chip);

	for ( i = 0; i < 0x40; i++ )
		chip->regs[i] = 0;

	//chip->stream = device->machine().sound().stream_alloc( *device, 0, 2, device->clock()/4, chip, IremGA20_update );

	/*device->save_item(NAME(chip->regs));
	for (i = 0; i < 4; i++)
	{
		device->save_item(NAME(chip->channel[i].rate), i);
		device->save_item(NAME(chip->channel[i].size), i);
		device->save_item(NAME(chip->channel[i].start), i);
		device->save_item(NAME(chip->channel[i].pos), i);
		device->save_item(NAME(chip->channel[i].end), i);
		device->save_item(NAME(chip->channel[i].volume), i);
		device->save_item(NAME(chip->channel[i].pan), i);
		device->save_item(NAME(chip->channel[i].effect), i);
		device->save_item(NAME(chip->channel[i].play), i);
	}*/
	for (i = 0; i < 4; i ++)
		chip->channel[i].Muted = 0x00;
	
	return clock / 4;
}

void device_stop_iremga20(UINT8 ChipID)
{
	ga20_state *chip = &GA20Data[ChipID];
	
	free(chip->rom);	chip->rom = NULL;
	
	return;
}

void iremga20_write_rom(UINT8 ChipID, offs_t ROMSize, offs_t DataStart, offs_t DataLength,
						const UINT8* ROMData)
{
	ga20_state *chip = &GA20Data[ChipID];
	
	if (chip->rom_size != ROMSize)
	{
		chip->rom = (UINT8*)realloc(chip->rom, ROMSize);
		chip->rom_size = ROMSize;
		memset(chip->rom, 0xFF, ROMSize);
	}
	if (DataStart > ROMSize)
		return;
	if (DataStart + DataLength > ROMSize)
		DataLength = ROMSize - DataStart;
	
	memcpy(chip->rom + DataStart, ROMData, DataLength);
	
	return;
}


void iremga20_set_mute_mask(UINT8 ChipID, UINT32 MuteMask)
{
	ga20_state *chip = &GA20Data[ChipID];
	UINT8 CurChn;
	
	for (CurChn = 0; CurChn < 4; CurChn ++)
		chip->channel[CurChn].Muted = (MuteMask >> CurChn) & 0x01;
	
	return;
}




/**************************************************************************
 * Generic get_info
 **************************************************************************/

/*DEVICE_GET_INFO( iremga20 )
{
	switch (state)
	{
		// --- the following bits of info are returned as 64-bit signed integers ---
		case DEVINFO_INT_TOKEN_BYTES:					info->i = sizeof(ga20_state);					break;

		// --- the following bits of info are returned as pointers to data or functions ---
		case DEVINFO_FCT_START:							info->start = DEVICE_START_NAME( iremga20 );	break;
		case DEVINFO_FCT_STOP:							// nothing //									break;
		case DEVINFO_FCT_RESET:							info->reset = DEVICE_RESET_NAME( iremga20 );	break;

		// --- the following bits of info are returned as NULL-terminated strings ---
		case DEVINFO_STR_NAME:							strcpy(info->s, "Irem GA20");					break;
		case DEVINFO_STR_FAMILY:					strcpy(info->s, "Irem custom");					break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");							break;
		case DEVINFO_STR_SOURCE_FILE:						strcpy(info->s, __FILE__);						break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright Nicola Salmoria and the MAME Team"); break;
	}
}


DEFINE_LEGACY_SOUND_DEVICE(IREMGA20, iremga20);*/
