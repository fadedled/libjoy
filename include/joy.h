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

#ifndef __JOY_H__
#define __JOY_H__


#include <gctypes.h>
#include <ogc/pad.h>


#define JOY_CHAN0      0
#define JOY_CHAN1      1
#define JOY_CHAN2      2
#define JOY_CHAN3      3
#define JOY_CHANMAX    4


#define JOY_ERR_NONE           ( 0)
#define JOY_ERR_NO_CONTROLLER  (-1)
#define JOY_ERR_NOT_READY      (-2)
#define JOY_ERR_TRANSFER       (-3)

/* Type of inserted device */
#define JOY_TYPE_CONTROLLER    0x1 /* Standard N64 Controller */
#define JOY_TYPE_VRU           0x2 /* N64 Voice Recognition Unit */
#define JOY_TYPE_MOUSE         0x3 /* N64 Mouse */
#define JOY_TYPE_KEYBOARD      0x4 /* N64 Keyboard */

/* Type of inserted pak in N64 controller */
#define JOY_PAK_TYPE_NONE      0x00 /* No pak detected */
#define JOY_PAK_TYPE_MEMORY    0x01 /* Controller Pak */
#define JOY_PAK_TYPE_RUMBLE    0x02 /* Rumble Pak */
#define JOY_PAK_TYPE_TRANSFER  0x03 /* Transfer Pak */
#define JOY_PAK_TYPE_UNKNOWN   0xFF /* Pak detected but not initialized */

/* N64 button mask */
#define JOY_BTN_C_RIGHT	       0x0001
#define JOY_BTN_C_LEFT	       0x0002
#define JOY_BTN_C_DOWN	       0x0004
#define JOY_BTN_C_UP		   0x0008
#define JOY_TRG_R			   0x0010
#define JOY_TRG_L			   0x0020
#define JOY_BTN_RIGHT		   0x0100
#define JOY_BTN_LEFT		   0x0200
#define JOY_BTN_DOWN		   0x0400
#define JOY_BTN_UP		       0x0800
#define JOY_BTN_START		   0x1000
#define JOY_TRG_Z			   0x2000
#define JOY_BTN_B			   0x4000
#define JOY_BTN_A			   0x8000

#ifdef __cplusplus
    extern "C" {
#endif /*__cplusplus*/

/* Controller status */
typedef struct _joystatus_t {
    u16 button;
    s8 stickX;
    s8 stickY;
    u8 err;
} JOYStatus;

/* Controller Pak entry */
typedef struct _mempakentry_t {
    u32 vendor;
    u16 game_id;
    u16 inode;
    u8 region;
    u8 blocks;
    u8 valid;
    u8 entry_id;
    u8 name[19];
} MemPakEntry;

typedef void (*PollHandler)(u32, void*);

/* 64 Controller functions */
u32 JOY_Init(JOYStatus *joy, PADStatus *pad);
u32 JOY_Read(JOYStatus *joy, PADStatus *pad);
u32 JOY_GetType(u32 ch);
u32 JOY_SetCh(u32 ch);
void JOY_SetPollingCallback(PollHandler poll_cb, u32 poll_rate);

/* Pak functions */
u32 JOY_InitPak(u32 ch);
u32 JOY_GetPakType(u32 ch);

/* Rumble Pak control */
s32 JOY_RumbleCtrl(u32 ch, u32 enable);


#if 0   //Not implemented yet
/* Controller Pak control */
s32 JOY_MemInit(u32 ch);
s32 JOY_MemValidate(u32 ch);
s32 JOY_MemFormat(u32 ch);
s32 JOY_MemFindEntry(u32 ch, u32 index, MemPakEntry *entry);
s32 JOY_MemReadEntry(u32 ch, MemPakEntry *entry, u8 *data);
s32 JOY_MemWriteEntry(u32 ch, MemPakEntry *entry, u8 *data);
s32 JOY_MemDeleteEntry(u32 ch, MemPakEntry *entry);
s32 JOY_MemNumEntries(u32 ch, MemPakEntry *entry);
#endif

#ifdef __cplusplus
    }
#endif /*__cplusplus*/

#endif /*__JOY_H__*/