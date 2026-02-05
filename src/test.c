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


#include <gccore.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <joy.h>
#include "log/log.h"

#define DEFAULT_FIFO_SIZE       (256*1024)

extern u32 debug_pack_type[4];

extern const u8 tex_pad64_4bpp_data[] ATTRIBUTE_ALIGN(32);
u8 *frameBuffer[2] ATTRIBUTE_ALIGN(32);
GXTexObj tex_n64pad;

extern const u8 tex_padgc_4bpp_data[] ATTRIBUTE_ALIGN(32);
GXTexObj tex_gcpad;

static void reset_cb(u32 val, void *data)
{
	PAD_Reset(PAD_CHAN0_BIT | PAD_CHAN1_BIT | PAD_CHAN2_BIT | PAD_CHAN3_BIT);
	exit(0);
}

//#define USE_SAMPLE_CB

JOYStatus pads64[JOY_CHANMAX] = {0};
PADStatus pads[PAD_CHANMAX] = {0};

#ifdef USE_SAMPLE_CB
static void gcsample_cb()
{
	//PAD_Read((PADStatus*)pads);
	JOY_Read((JOYStatus*) pads64, (PADStatus*) pads);
}
#endif



static void __drawPad64(JOYStatus *status, u32 x, u32 y)
{
	const u32 PAD_WIDTH = 112;

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	GX_SetNumChans(0);
	Mtx mat;
	guMtxIdentity(mat);
	guMtxScale(mat, 2.0, 2.0, 1.0);
	guMtxTransApply(mat, mat, x, y, 0.0);
	GX_LoadPosMtxImm(mat, GX_PNMTX0);
	GX_LoadTexObj(&tex_n64pad, GX_TEXMAP0);

	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 8);

	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xff, 0xff, 0xff, 0xff});

	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, GX_CC_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	u32 btn = status->button;
	u8 cr = (btn >> 0) & 1;
	u8 cl = (btn >> 1) & 1;
	u8 cd = (btn >> 2) & 1;
	u8 cu = (btn >> 3) & 1;
	u8 tr = (btn >> 4) & 1;
	u8 tl = (btn >> 5) & 1;
	u8 dr = (btn >> 8) & 1;
	u8 dl = (btn >> 9) & 1;
	u8 dd = (btn >> 10) & 1;
	u8 du = (btn >> 11) & 1;
	u8 start = (btn >> 12) & 1;
	u8 tz = (btn >> 13) & 1;
	u8 b  = (btn >> 14) & 1;
	u8 a  = (btn >> 15) & 1;

	//Draw the Triggers
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
		//Left Trigger
		GX_Position2s16(9+0,  tl+7+0 ); GX_TexCoord2s16(2, 12);
		GX_Position2s16(9+0,  tl+7+16); GX_TexCoord2s16(2, 14);
		GX_Position2s16(9+24, tl+7+0 ); GX_TexCoord2s16(5, 12);
		GX_Position2s16(9+24, tl+7+16); GX_TexCoord2s16(5, 14);

		//Right Trigger
		GX_Position2s16(79+0,  tr+7+0 ); GX_TexCoord2s16(5, 12);
		GX_Position2s16(79+0,  tr+7+16); GX_TexCoord2s16(5, 14);
		GX_Position2s16(79+24, tr+7+0 ); GX_TexCoord2s16(2, 12);
		GX_Position2s16(79+24, tr+7+16); GX_TexCoord2s16(2, 14);
	GX_End();

	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	//Draw the Controller
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(0, 0); GX_TexCoord2s16(0, 0);
		GX_Position2s16(0, PAD_WIDTH - 32); GX_TexCoord2s16(0, 10);
		GX_Position2s16(PAD_WIDTH, 0); GX_TexCoord2s16(14, 0);
		GX_Position2s16(PAD_WIDTH, PAD_WIDTH - 32); GX_TexCoord2s16(14, 10);
	GX_End();

	if (status->err != JOY_ERR_NONE) {
		return;
	}

	//Draw the Stick
	s8 sx = 48 + (((s32)status->stickX) >> 4);
	s8 sy = 53 - (((s32)status->stickY) >> 4);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(sx+0,  sy+0); GX_TexCoord2s16(0, 12);
		GX_Position2s16(sx+0,  sy+16); GX_TexCoord2s16(0, 14);
		GX_Position2s16(sx+16, sy+0); GX_TexCoord2s16(2, 12);
		GX_Position2s16(sx+16, sy+16); GX_TexCoord2s16(2, 14);
	GX_End();

	//Start Button
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x88, 0x33, 0x22, 0xff});	//Red
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, start, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(52+0, 34+0); GX_TexCoord2s16(3, 13);
		GX_Position2s16(52+0, 34+8); GX_TexCoord2s16(3, 14);
		GX_Position2s16(52+8, 34+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(52+8, 34+8); GX_TexCoord2s16(4, 14);
	GX_End();

	//A Button
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x22, 0x33, 0x88, 0xff});	//Blue
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, a, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(82+0, 41+0); GX_TexCoord2s16(3, 13);
		GX_Position2s16(82+0, 41+8); GX_TexCoord2s16(3, 14);
		GX_Position2s16(82+8, 41+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(82+8, 41+8); GX_TexCoord2s16(4, 14);
	GX_End();

	//B Button
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x22, 0x88, 0x33, 0xff});	//Green
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, b, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(74+0, 33+0); GX_TexCoord2s16(3, 13);
		GX_Position2s16(74+0, 33+8); GX_TexCoord2s16(3, 14);
		GX_Position2s16(74+8, 33+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(74+8, 33+8); GX_TexCoord2s16(4, 14);
	GX_End();

	//C Buttons
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x88, 0x88, 0x33, 0xff});	//Yellow
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, cu, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		//C-UP button
		GX_Position2s16(92+0, 21+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(92+0, 21+8); GX_TexCoord2s16(4, 14);
		GX_Position2s16(92+8, 21+0); GX_TexCoord2s16(5, 13);
		GX_Position2s16(92+8, 21+8); GX_TexCoord2s16(5, 14);
	GX_End();
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, cl, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		//C-LEFT button
		GX_Position2s16(85+0, 28+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(85+0, 28+8); GX_TexCoord2s16(4, 14);
		GX_Position2s16(85+8, 28+0); GX_TexCoord2s16(5, 13);
		GX_Position2s16(85+8, 28+8); GX_TexCoord2s16(5, 14);
	GX_End();
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, cr, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		//C-RIGHT button
		GX_Position2s16(99+0, 28+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(99+0, 28+8); GX_TexCoord2s16(4, 14);
		GX_Position2s16(99+8, 28+0); GX_TexCoord2s16(5, 13);
		GX_Position2s16(99+8, 28+8); GX_TexCoord2s16(5, 14);
	GX_End();
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, cd, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		//C-DOWN button
		GX_Position2s16(92+0, 34+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(92+0, 34+8); GX_TexCoord2s16(4, 14);
		GX_Position2s16(92+8, 34+0); GX_TexCoord2s16(5, 13);
		GX_Position2s16(92+8, 34+8); GX_TexCoord2s16(5, 14);
	GX_End();

	//We only make buttons brighter
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xff, 0xff, 0xff, 0xff});
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	if (du) { //UP
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(18+0, 28+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(18+0, 28+4); GX_TexCoord2s16(4, 14);
			GX_Position2s16(18+4, 28+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(18+4, 28+4); GX_TexCoord2s16(5, 14);
		GX_End();
	}
	if (dl) { //LEFT
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(13+0, 33+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(13+0, 33+4); GX_TexCoord2s16(4, 14);
			GX_Position2s16(13+4, 33+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(13+4, 33+4); GX_TexCoord2s16(5, 14);
		GX_End();
	}
	if (dr) { //RIGHT
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(23+0, 33+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(23+0, 33+4); GX_TexCoord2s16(4, 14);
			GX_Position2s16(23+4, 33+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(23+4, 33+4); GX_TexCoord2s16(5, 14);
		GX_End();
	}
	if (dd) { //DOWN
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(18+0, 38+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(18+0, 38+4); GX_TexCoord2s16(4, 14);
			GX_Position2s16(18+4, 38+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(18+4, 38+4); GX_TexCoord2s16(5, 14);
		GX_End();
	}

	if (tz) { //Z Trigger
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(79+0, 66+0 ); GX_TexCoord2s16(4, 13);
			GX_Position2s16(79+0, 66+12); GX_TexCoord2s16(4, 14);
			GX_Position2s16(79+8, 66+0 ); GX_TexCoord2s16(5, 13);
			GX_Position2s16(79+8, 66+12); GX_TexCoord2s16(5, 14);
		GX_End();
	}

}


static void __drawPadGC(PADStatus *status, u32 x, u32 y)
{
	const u32 PAD_WIDTH = 112;

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	Mtx mat;
	GX_SetNumChans(0);
	guMtxIdentity(mat);
	guMtxScale(mat, 2.0, 2.0, 1.0);
	guMtxTransApply(mat, mat, x, y, 0.0);
	GX_LoadPosMtxImm(mat, GX_PNMTX0);
	GX_LoadTexObj(&tex_gcpad, GX_TEXMAP0);

	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 8);

	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xff, 0xff, 0xff, 0xff});

	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_KONST, GX_CC_ZERO);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	u32 btn = status->button;
	u8 dl = (btn >> 0) & 1;
	u8 dr = (btn >> 1) & 1;
	u8 dd = (btn >> 2) & 1;
	u8 du = (btn >> 3) & 1;
	u8 tz = (btn >> 4) & 1;
	u8 tr = (btn >> 5) & 1;
	u8 tl = (btn >> 6) & 1;
	u8 a  = (btn >> 8) & 1;
	u8 b  = (btn >> 9) & 1;
	u8 bx  = (btn >> 10) & 1;
	u8 by  = (btn >> 11) & 1;
	u8 start = (btn >> 12) & 1;

	//Draw the Triggers
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 8);
		//Left Trigger
		GX_Position2s16(9+0,  tl+7+0 ); GX_TexCoord2s16(2, 12);
		GX_Position2s16(9+0,  tl+7+16); GX_TexCoord2s16(2, 14);
		GX_Position2s16(9+24, tl+7+0 ); GX_TexCoord2s16(5, 12);
		GX_Position2s16(9+24, tl+7+16); GX_TexCoord2s16(5, 14);

		//Right Trigger
		GX_Position2s16(79+0,  tr+7+0 ); GX_TexCoord2s16(5, 12);
		GX_Position2s16(79+0,  tr+7+16); GX_TexCoord2s16(5, 14);
		GX_Position2s16(79+24, tr+7+0 ); GX_TexCoord2s16(2, 12);
		GX_Position2s16(79+24, tr+7+16); GX_TexCoord2s16(2, 14);
	GX_End();

	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	//Draw the Controller
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(0, 0); GX_TexCoord2s16(0, 0);
		GX_Position2s16(0, PAD_WIDTH - 32); GX_TexCoord2s16(0, 10);
		GX_Position2s16(PAD_WIDTH, 0); GX_TexCoord2s16(14, 0);
		GX_Position2s16(PAD_WIDTH, PAD_WIDTH - 32); GX_TexCoord2s16(14, 10);
	GX_End();

	if (status->err != PAD_ERR_NONE) {
		return;
	}

	//Draw the Sticks
	s8 sx = 15 + (((s32)status->stickX) >> 4);
	s8 sy = 20 - (((s32)status->stickY) >> 4);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(sx+0,  sy+0);  GX_TexCoord2s16(0, 12);
		GX_Position2s16(sx+0,  sy+16); GX_TexCoord2s16(0, 14);
		GX_Position2s16(sx+16, sy+0);  GX_TexCoord2s16(2, 12);
		GX_Position2s16(sx+16, sy+16); GX_TexCoord2s16(2, 14);
	GX_End();

	s8 csx = 68 + (((s32)status->substickX) >> 4);
	s8 csy = 47 - (((s32)status->substickY) >> 4);
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xEE, 0xEE, 0x44, 0xff});	//Yellow
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(csx+0,  csy+0);  GX_TexCoord2s16(2, 10);
		GX_Position2s16(csx+0,  csy+16); GX_TexCoord2s16(2, 12);
		GX_Position2s16(csx+16, csy+0);  GX_TexCoord2s16(4, 10);
		GX_Position2s16(csx+16, csy+16); GX_TexCoord2s16(4, 12);
	GX_End();

	//Start Button
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x77, 0x77, 0x77, 0xff});	//Gray
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, start, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(52+0, 25+0); GX_TexCoord2s16(3, 13);
		GX_Position2s16(52+0, 25+8); GX_TexCoord2s16(3, 14);
		GX_Position2s16(52+8, 25+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(52+8, 25+8); GX_TexCoord2s16(4, 14);
	GX_End();

	//X Button
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, by, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(79+0, 12+0);  GX_TexCoord2s16(5, 13);
		GX_Position2s16(79+0, 12+8);  GX_TexCoord2s16(5, 14);
		GX_Position2s16(79+16, 12+0); GX_TexCoord2s16(7, 13);
		GX_Position2s16(79+16, 12+8); GX_TexCoord2s16(7, 14);
	GX_End();

	//Y Button
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, bx, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(97+0, 20+0);  GX_TexCoord2s16(2, 12);
		GX_Position2s16(97+0, 20+16); GX_TexCoord2s16(2, 14);
		GX_Position2s16(97+8, 20+0);  GX_TexCoord2s16(3, 12);
		GX_Position2s16(97+8, 20+16); GX_TexCoord2s16(3, 14);
	GX_End();

	//A Button
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x22, 0x88, 0x33, 0xff});	//Green
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, a, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(80+0, 20+0);  GX_TexCoord2s16(0, 10);
		GX_Position2s16(80+0, 20+16);  GX_TexCoord2s16(0, 12);
		GX_Position2s16(80+16, 20+0);  GX_TexCoord2s16(2, 10);
		GX_Position2s16(80+16, 20+16); GX_TexCoord2s16(2, 12);
	GX_End();

	//B Button
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0x88, 0x22, 0x33, 0xff});	//Red
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, b, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
		GX_Position2s16(73+0, 30+0); GX_TexCoord2s16(3, 13);
		GX_Position2s16(73+0, 30+8); GX_TexCoord2s16(3, 14);
		GX_Position2s16(73+8, 30+0); GX_TexCoord2s16(4, 13);
		GX_Position2s16(73+8, 30+8); GX_TexCoord2s16(4, 14);
	GX_End();

	//We only make buttons brighter
	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xff, 0xff, 0xff, 0xff});
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	if (du) { //UP
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(37+0, 46+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(37+0, 46+3); GX_TexCoord2s16(4, 14);
			GX_Position2s16(37+3, 46+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(37+3, 46+3); GX_TexCoord2s16(5, 14);
		GX_End();
	}
	if (dl) { //LEFT
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(32+0, 51+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(32+0, 51+3); GX_TexCoord2s16(4, 14);
			GX_Position2s16(32+3, 51+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(32+3, 51+3); GX_TexCoord2s16(5, 14);
		GX_End();
	}
	if (dr) { //RIGHT
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(42+0, 51+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(42+0, 51+3); GX_TexCoord2s16(4, 14);
			GX_Position2s16(42+3, 51+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(42+3, 51+3); GX_TexCoord2s16(5, 14);
		GX_End();
	}
	if (dd) { //DOWN
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(37+0, 56+0); GX_TexCoord2s16(4, 13);
			GX_Position2s16(37+0, 56+3); GX_TexCoord2s16(4, 14);
			GX_Position2s16(37+3, 56+0); GX_TexCoord2s16(5, 13);
			GX_Position2s16(37+3, 56+3); GX_TexCoord2s16(5, 14);
		GX_End();
	}

	if (tz) { //Z Trigger
		GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT0, 4);
			GX_Position2s16(79+0, 66+0 ); GX_TexCoord2s16(4, 13);
			GX_Position2s16(79+0, 66+12); GX_TexCoord2s16(4, 14);
			GX_Position2s16(79+8, 66+0 ); GX_TexCoord2s16(5, 13);
			GX_Position2s16(79+8, 66+12); GX_TexCoord2s16(5, 14);
		GX_End();
	}

}



int main()
{
	u32	fb;
	u32 xfb_height;
	Mtx44 perspective;
	Mtx modelview;
	GXRModeObj *rmode;
	void *gp_fifo = NULL;

	GXColor background = {0x66, 0x66, 0x66, 0x00};

	VIDEO_Init();

	rmode = VIDEO_GetPreferredMode(NULL);

	fb = 0;
	// allocate 2 framebuffers for double buffering
	frameBuffer[0] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	frameBuffer[1] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_SetBlack(false);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	fb ^= 1;

	// setup the fifo and then init the flipper
	gp_fifo = memalign(32, DEFAULT_FIFO_SIZE);
	memset(gp_fifo, 0, DEFAULT_FIFO_SIZE);

	GX_Init(gp_fifo, DEFAULT_FIFO_SIZE);

	// other gx setup
	GX_SetViewport(0,0,rmode->fbWidth, rmode->efbHeight,0,1);
	xfb_height = GX_SetDispCopyYScale(1.0);
	GX_SetScissor(0,0,rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopySrc(0,0,rmode->fbWidth, rmode->efbHeight);
	GX_SetDispCopyDst(rmode->fbWidth, xfb_height);
	GX_SetCopyFilter(rmode->aa, rmode->sample_pattern, GX_TRUE, rmode->vfilter);
	GX_SetFieldMode(rmode->field_rendering, ((rmode->viHeight==2*rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE));

	GX_SetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);

	GX_SetCullMode(GX_CULL_NONE);
	GX_CopyDisp(frameBuffer[fb], GX_TRUE);
	GX_SetDispCopyGamma(GX_GM_1_0);
	GX_SetDither(GX_DISABLE);
	VIDEO_SetNextFramebuffer(frameBuffer[fb]);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	GX_CopyDisp(frameBuffer[1], GX_TRUE);
	VIDEO_SetNextFramebuffer(frameBuffer[1]);
	VIDEO_Flush();
	VIDEO_WaitVSync();

	// setup the vertex descriptor
	// tells the flipper to expect direct data
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XY, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);

	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetTevOp(GX_TEVSTAGE0, GX_REPLACE);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);

	//Init texture for n64 pad
	GX_InitTexObj(&tex_n64pad, (void *) tex_pad64_4bpp_data, 112, 112, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjLOD(&tex_n64pad, GX_NEAR, GX_NEAR, 0, 0, 0, GX_DISABLE, GX_DISABLE, GX_ANISO_1);

	//Init texture for gc pad
	GX_InitTexObj(&tex_gcpad, (void *) tex_padgc_4bpp_data, 112, 112, GX_TF_I4, GX_CLAMP, GX_CLAMP, GX_FALSE);
	GX_InitTexObjLOD(&tex_gcpad, GX_NEAR, GX_NEAR, 0, 0, 0, GX_DISABLE, GX_DISABLE, GX_ANISO_1);


	guOrtho(perspective, 0, 480, 0, 640, 0, 64.0f);
	GX_LoadProjectionMtx(perspective, GX_ORTHOGRAPHIC);

	GXColor konst = {0xff, 0x00, 0xff, 0xE0};
	GX_SetTevKColor(GX_KCOLOR0, konst);
	GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);
	GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);

	GX_SetViewport(0, 0, rmode->fbWidth, rmode->efbHeight, 0, 1);
	GX_InvVtxCache();
	GX_InvalidateTexAll();

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	guMtxIdentity(modelview);
	GX_LoadPosMtxImm(modelview, GX_PNMTX0);
	GX_SetCopyClear(background, 0x000000);
	GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_AND, GX_ALWAYS, 0);
	GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);

	//Initialize GC pads
	JOY_Init(pads64, pads);
	LOG_Init();
#ifdef USE_SAMPLE_CB
	JOY_SetPollingCallback(gcsample_cb, 0);
#endif

	SYS_SetResetCallback(reset_cb);

	while(1) {
#ifndef USE_SAMPLE_CB
		JOY_Read(pads64, pads);
#endif

		if ((pads[0].button   & (PAD_BUTTON_START | PAD_TRIGGER_Z)) == (PAD_BUTTON_START | PAD_TRIGGER_Z)) exit(0);
		if ((pads64[0].button & (JOY_BTN_START | JOY_TRG_Z)) == (JOY_BTN_START | JOY_TRG_Z)) exit(0);


		u32 pad_pos[4][2] = {
			{32, 16}, {362, 16}, {32, 248}, {363, 248}
		};

		for (u32 i = 0; i < JOY_CHANMAX; ++i) {
			u32 x = pad_pos[i][0];
			u32 y = pad_pos[i][1];
			if (pads64[i].err == JOY_ERR_NONE) {
				__drawPad64(&pads64[i], x, y);
				u32 ret_val = debug_pack_type[i];
				u32 pak_type = JOY_InitPak(i);
				JOY_RumbleCtrl(i, pads64[i].button & JOY_TRG_R);

				LOG_printf(x, y + 140,
					"PAD: N64 (%d)\nbtn:   %04X\nstickX: %3d\nstickY: %3d\n",
					pads64[i].err, pads64[i].button, (s32) pads64[i].stickX, (s32) pads64[i].stickY);
				//Show pak type
				char *pak_name = "NONE";
				switch(pak_type) {
					case JOY_PAK_TYPE_RUMBLE:	pak_name = "RUMBLE"; break;
					case JOY_PAK_TYPE_MEMORY:	pak_name = "MEMORY"; break;
					case JOY_PAK_TYPE_TRANSFER:	pak_name = "TRANSFER"; break;
					case JOY_PAK_TYPE_UNKNOWN:	pak_name = "UNKNOWN"; break;
				}
				LOG_printf(x, y + 140 + 64, "PAK: %08x %s", ret_val, pak_name);
			} else if (pads[i].err == PAD_ERR_NONE) {
				//__drawPad64((PAD64Status*) (&pads[i]), x, y);
				__drawPadGC(&pads[i], x, y);
				LOG_printf(x, y + 140,
					"PAD: GC  (%d)\nbtn:   %04X\nstickX: %3d\nstickY: %3d\n",
					pads[i].err, pads[i].button, (s32) pads[i].stickX, (s32) pads[i].stickY);
			} else {
				LOG_printf(x, y + 140,
					"PAD: NONE(%d)\nbtn:   %04X\nstickX: %3d\nstickY: %3d\n",
					pads[i].err, 0, 0, 0);
			}
		}
		LOG_Draw();
		GX_DrawDone();
		GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
		GX_SetAlphaUpdate(GX_TRUE);
		GX_SetColorUpdate(GX_TRUE);

		GX_CopyDisp(frameBuffer[fb], GX_TRUE);
		VIDEO_SetNextFramebuffer(frameBuffer[fb]);
		VIDEO_Flush();
		VIDEO_WaitVSync();

	}
	return 0;
}
