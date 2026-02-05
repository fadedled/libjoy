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
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

//Add the text font in tex_font_4bpp_data
#include "log_font.inc"

#define MAX_TEXT_SIZE	4096 //4K of values

//Char codes
#define CHR_NL			'\n'
#define CHR_POS			0x90
#define CHR_SCALE8		0x92
#define CHR_SCALE16		0x93


static const u32 text_colors[] = {
	0xFFFFFFFF, //WHITE
	0xFF0000FF, //RED
	0x00FF00FF,	//GREEN
	0x0000FFFF, //BLUE
	0xFFFFFFFF, //YELLOW
	0xFFFFFFFF, //CYAN
	0xFFFFFFFF  //MAGENTA
	//0xFFFFFFFF, //CYAN
};

static GXTexObj tex_font;
static Mtx ident;
static char buffer[1024];

static struct Text {
	u8  data[MAX_TEXT_SIZE];
	u32 datalen;
	u32 vtxlen;
} _text;

const u32 text_scale = 1;


void LOG_Init(void)
{
	GX_InitTexObj(&tex_font, (void *) tex_font_4bpp_data, 128, 64, GX_TF_I4, GX_MIRROR, GX_MIRROR, GX_FALSE);
	GX_InitTexObjLOD(&tex_font, GX_NEAR, GX_NEAR, 0, 0, 0, GX_DISABLE, GX_DISABLE, GX_ANISO_1);

	//Setup vertex attr
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_POS,  GX_POS_XY, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

	//set the default values
	_text.datalen = 0;
	_text.vtxlen = 0;
}


void LOG_Scale(int mode)
{
	if (mode != DLOG_SCALE_8) {
		_text.data[_text.datalen] = CHR_SCALE16;
	} else {
		_text.data[_text.datalen] = CHR_SCALE8;
	}
	_text.datalen++;
}

int LOG_printf(int x, int y, const char* fmt, ...)
{
	va_list args;
	int num_chars;
	//Create the printed text
	va_start(args, fmt);
	num_chars = vsprintf(buffer, fmt, args);
	va_end(args);

	if (_text.datalen + num_chars >= MAX_TEXT_SIZE) {
		return 0;
	}

	//Put start position
	_text.data[_text.datalen++] = CHR_POS;
	_text.data[_text.datalen++] = (x >> 3);
	_text.data[_text.datalen++] = (y >> 3);

	//Draw text
	u8 *tmp = (u8*) buffer;
	while (*tmp) {
		u32 chr = *tmp;
		switch (chr) {
			case '\n': {
				_text.data[_text.datalen++] = CHR_NL;
			} break;
			default: {
				_text.data[_text.datalen++] = chr;
				_text.vtxlen++;
			}
		}
		++tmp;
	}
	return num_chars;
}


void LOG_Draw(void)
{
	u32 x_begin = 0, x = 0, y = 0;

	//Set up GX state
	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
	//GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
	guMtxIdentity(ident);
	GX_LoadPosMtxImm(ident, GX_PNMTX0);

	GX_LoadTexObj(&tex_font, GX_TEXMAP0);

	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 8);
	GX_EnableTexOffsets(GX_TEXCOORD0, GX_FALSE, GX_TRUE);
	GX_SetPointSize((8 << text_scale) * 6, GX_TO_ONE);

	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ADDHALF, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	u32 len = _text.vtxlen;
	u8 *tmp = _text.data;
	u32 tcolor = text_colors[0];

	GX_Begin(GX_POINTS, GX_VTXFMT7, len);
	for (u32 i = 0; i < _text.datalen; ++i, ++tmp) {
		u32 chr = *tmp;

		switch(chr) {
			case CHR_NL: { //Sets the NewLine position
				y += (8 << text_scale);
				x = x_begin;
			} break;
			case CHR_POS: { //Sets the start position (+2)
				++tmp; ++i; // next value
				x = x_begin = ((u32) *tmp) << 3;
				++tmp; ++i; // next value
				y = ((u32) *tmp) << 3;
			} break;
			case CHR_SCALE8: { //Sets scale to 8x8 tiles
				//text_scale = 0;
			} break;
			case CHR_SCALE16: { //Sets scale to 16x16 tiles
				//text_scale = 1;
			} break;
			default: { // Draws character
				u32 tx = chr & 0xF;
				u32 ty = (chr >> 4) & 0x7;
				GX_Position2s16(x, y); GX_TexCoord2s16(tx, ty);
				x += (8 << text_scale);
			} break;
		}
	}
	GX_End();

	GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);

	_text.datalen = 0;
	_text.vtxlen = 0;
}