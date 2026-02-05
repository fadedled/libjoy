/********************************************************************************
 * Copyright (C) 2025 Evoca
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************/


#include <ogc/si.h>
#include <ogc/pad.h>
#include <ogc/machine/processor.h>
#include <string.h>
#include <joy.h>


u8 num_ch = JOY_CHANMAX;
u8 has_pak;
u8 pak_init;


struct {
	u32 type;
	u32 reset;
	u8 err;
} __pads[JOY_CHANMAX];

struct Pak {
	u32 type;
	u32 reset;
	u32 rumble;
	u8 out_buffer[40];
	u8 in_buffer[40];
} __paks[JOY_CHANMAX];

u32 debug_pack_type[4];

#define __JOY_RESET(joy) (*((u32*)joy) = 0)

static void __scan_cb(s32 ch, u32 type) { /*Does nothing*/ }



u32 JOY_Init(JOYStatus *joy, PADStatus *pad)
//u32  PAD64_Init(u8 *bitpattern, PAD64Status *pstat)
{
	PAD_Init();
	for (u32 i = 0; i < SI_MAX_CHAN; ++i) {
		__pads[i].err = JOY_ERR_NO_CONTROLLER;
		joy[i].err = JOY_ERR_NO_CONTROLLER;
		pad[i].err = PAD_ERR_NO_CONTROLLER;
	}
	return 0;
}

u32 JOY_Read(JOYStatus *joy, PADStatus *pad)
{
	u32 reset_mask = 0;
	u32 level;
	u32 channels = 0;

	PAD_Read(pad);

	_CPU_ISR_Disable(level);

	for (u32 ch = 0; ch < num_ch; ++ch) {
		u32 type = SI_GetType(ch);
		u32 reset_bit = SI_CHAN_BIT(ch);

		if ((type & SI_ERROR_NO_RESPONSE) ||
			(type & 0x0F) ||
			(type & 0xFF) ) {

			if (joy->err == JOY_ERR_NONE) {
				reset_mask |= reset_bit;
				__JOY_RESET(joy);
				joy->err = JOY_ERR_NO_CONTROLLER;
			}
		} else {
			//Check for N64 pad
			if ((type & SI_TYPE_MASK) == SI_TYPE_N64) {
				//Store pak info
				debug_pack_type[ch] = type;
				u32 pak_status = (type >> 8) & 0x3;
				u32 pak = ((type >> 8) & 0x1) << ch;

				has_pak = (has_pak & ~pak) | pak;	//Set pak bit
				pak_init &= ~(pak & ~channels);					//If no pak then deinit
				channels |= 1 << ch;
				if (pak) {
					__paks[ch].type = ((pak_init >> ch) & 0x1) ? __paks[ch].type : JOY_PAK_TYPE_UNKNOWN;
				} else {
					__paks[ch].type = JOY_PAK_TYPE_NONE;
				}


				//Get controller state
				__paks[ch].out_buffer[0] = 0x01;
				SI_Transfer(ch, __paks[ch].out_buffer, 1, joy, 4, __scan_cb, 0);

				joy->err = JOY_ERR_NONE;
			} else if ( (type & SI_TYPE_MASK) == SI_TYPE_GC) {
				//Check and reset if pad has no contoller
				if (pad->err == PAD_ERR_NO_CONTROLLER) {
					reset_mask |= reset_bit;
					pad->err = PAD_ERR_NOT_READY;
				}
			} else {
				if (joy->err == JOY_ERR_NONE) {
					reset_mask |= reset_bit;
					__JOY_RESET(joy);
					joy->err = JOY_ERR_NO_CONTROLLER;
				}
			}
		}
		++joy;
		++pad;
	}
	//Reset the channels
	if (reset_mask) {
		PAD_Reset(reset_mask);
	}
	_CPU_ISR_Restore(level);
	return channels;
}

u32 JOY_GetType(u32 ch)
{
	return 0;
}

u32 JOY_SetCh(u32 channel_num)
{
	return (num_ch = (num_ch <= 4) ? channel_num : 4);
}

void JOY_SetPollingCallback(PollHandler poll_cb, u32 poll_rate)
{
	SI_SetSamplingRate(poll_rate);
	SI_RegisterPollingHandler(poll_cb);
}





/********************************************************
 * Controller Pak functions
 *******************************************************/

//Tables for very fast ckechsum calc
const u8 ckeck_tbl_lo[32] = {
	0x00, 0x15, 0x1f, 0x0a, 0x0b, 0x1e, 0x14, 0x01, 0x16, 0x03, 0x09, 0x1c, 0x1d, 0x08, 0x02, 0x17,
	0x19, 0x0c, 0x06, 0x13, 0x12, 0x07, 0x0d, 0x18, 0x0f, 0x1a, 0x10, 0x05, 0x04, 0x11, 0x1b, 0x0e
};

const u8 ckeck_tbl_hi[32] = {
	0x00, 0x07, 0x0e, 0x09, 0x1c, 0x1b, 0x12, 0x15, 0x0d, 0x0a, 0x03, 0x04, 0x11, 0x16, 0x1f, 0x18,
	0x1a, 0x1d, 0x14, 0x13, 0x06, 0x01, 0x08, 0x0f, 0x17, 0x10, 0x19, 0x1e, 0x0b, 0x0c, 0x05, 0x02
};

static void __inittype_cb(s32 ch, u32 type)
{
	if (__paks[ch].in_buffer[0] == 0x00) { // transfer or controller pak
		//Test for Transfer Pak
		//__writeControllerAccessory1(ch, 0x8000, 0x84);
		//__readControllerAccessory(ch, 0x8000);
		__paks[ch].type = JOY_PAK_TYPE_MEMORY; //(__paks[ch].in_buffer[0] == 0x84 ? JOY_PAK_TYPE_TRANSFER : JOY_PAK_TYPE_MEMORY);
	} else if (__paks[ch].in_buffer[0] == 0x80) {	// Is rumble pak
		__paks[ch].type = JOY_PAK_TYPE_RUMBLE;
	}
	//pak is now initialized
	pak_init |= 1 << ch;
}


static inline u32 __genAddressChecksum(u32 addr) {
	return ckeck_tbl_lo[(addr >> 5) & 0x1F] ^ ckeck_tbl_hi[(addr >> 10) & 0x1F] ^ (addr >> 15);
}

static void __readControllerAccessory(u32 ch, u32 addr, SICallback cb)
{
	__paks[ch].out_buffer[0] = 0x02;
	__paks[ch].out_buffer[1] = (addr >> 8) & 0xFF;
	__paks[ch].out_buffer[2] = (addr  & 0xE0) | __genAddressChecksum(addr);
	SI_Transfer(ch, __paks[ch].out_buffer, 3, __paks[ch].in_buffer, 33, cb, 0);
}

static void __writeControllerAccessory1(u32 ch, u32 addr, u8 data, SICallback cb)
{
	__paks[ch].out_buffer[0] = 0x03;
	__paks[ch].out_buffer[1] = (addr >> 8) & 0xFF;
	__paks[ch].out_buffer[2] = (addr  & 0xE0) | __genAddressChecksum(addr);
	memset(__paks[ch].out_buffer+3, data, 32);
	SI_Transfer(ch, __paks[ch].out_buffer, 35, __paks[ch].in_buffer, 1, cb, 0);
}

static void __writeControllerAccessory(u32 ch, u32 addr, u8 *data, u8 num_bytes, SICallback cb)
{
	//Max 32 bytes
	num_bytes = num_bytes > 32 ? 32 : num_bytes;
	__paks[ch].out_buffer[0] = 0x03;
	__paks[ch].out_buffer[1] = (addr >> 8) & 0xFF;
	__paks[ch].out_buffer[2] = (addr  & 0xE0) | __genAddressChecksum(addr);
	memcpy(__paks[ch].out_buffer+3, data, num_bytes);
	SI_Transfer(ch, __paks[ch].out_buffer, 35, __paks[ch].in_buffer, 1, cb, 0);
}


u32 JOY_InitPak(u32 ch)
{
	//Only when there is a pak detected but not initialized
	if (((has_pak >> ch) & 1)) {
		// already intitialized
		if ((pak_init >> ch) & 1) {
			return __paks[ch].type;
		}
		__paks[ch].type = JOY_PAK_TYPE_UNKNOWN;
		//TODO: Must this be done?
		//__writeControllerAccessory1(ch, 0x8000, 0xFE);
		//Test for Rumble Pak
		__readControllerAccessory(ch, 0x8000, __inittype_cb);
	} else {
		__paks[ch].type = JOY_PAK_TYPE_NONE;
	}
	return __paks[ch].type;
}

u32 JOY_GetPakType(u32 ch)
{
	return __paks[ch].type;
}


/*
 * Controller Pak functions
*/

/*
 * Rumble Pak functions
*/

s32 JOY_RumbleCtrl(u32 ch, u32 enable)
{
	enable = enable > 0;
	//if (__paks[ch].type == JOY_PAK_TYPE_RUMBLE) {
	if (__paks[ch].rumble != enable) {
		__paks[ch].rumble = enable;
		__writeControllerAccessory1(ch, 0xC000, enable, __scan_cb);
	}
	//	return 1;
	//}
	return 0;
}

/*
 * Transfer Pak functions
*/


