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

//Add the text font in tex_font_4bpp_data
#include "log_font.inc"


static u32 text_scale = 1;
static u32 text_color = 0xFFFFFFFF;

static void __setPrintState(void)
{
	GXTexObj tex_font;
	Mtx ident;

	//Set up GX state
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_POS,  GX_POS_XY, GX_S16, 0);
	GX_SetVtxAttrFmt(GX_VTXFMT7, GX_VA_TEX0, GX_TEX_ST, GX_S16, 0);

	GX_ClearVtxDesc();
	GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
	GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);

	guMtxIdentity(ident);
	GX_LoadPosMtxImm(ident, GX_PNMTX0);

	GX_SetNumChans(0);
	GX_SetNumTexGens(1);
	GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
	GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
	GX_SetTexCoordScaleManually(GX_TEXCOORD0, GX_TRUE, 8, 8);
	GX_EnableTexOffsets(GX_TEXCOORD0, GX_FALSE, GX_TRUE);
	GX_SetPointSize((8 << text_scale) * 6, GX_TO_ONE);


	GX_InitTexObj(&tex_font, (void *) tex_font_4bpp_data, 128, 64, GX_TF_I4, GX_MIRROR, GX_MIRROR, GX_FALSE);
	GX_InitTexObjLOD(&tex_font, GX_NEAR, GX_NEAR, 0, 0, 0, GX_DISABLE, GX_DISABLE, GX_ANISO_1);
	GX_LoadTexObj(&tex_font, GX_TEXMAP0);

	GX_SetTevKColor(GX_KCOLOR0, (GXColor){0xff, 0xff, 0xff, 0xff});
	GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_K0_A);
	GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
	GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC);
	GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
	GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
	GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ADDHALF, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);

	GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
}

static void __exitPrintState(void)
{
	GX_SetBlendMode(GX_BM_NONE, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
}

int LOG_printf(int x, int y, const char* fmt, ...)
{
	char buffer[1024];
	va_list args;
	int num_chars;
	//Create the printed text
	va_start(args, fmt);
	num_chars = vsprintf(buffer, fmt, args);
	va_end(args);

	__setPrintState();

	//Draw text
	char *tmp = buffer;
	int begin_x = x;
	GX_Begin(GX_POINTS, GX_VTXFMT7, num_chars);
	while (*tmp) {
		u32 chr = *tmp;
		u32 tx = (chr) & 0xF;
		u32 ty = (chr >> 4) & 0x7;
		GX_Position2s16(x, y); GX_TexCoord2s16(tx, ty);
		x += (8 << text_scale);
		//New line starts bellow at next char
		//(even so the blank char is shown)
		if (*tmp == '\n') {
			y += (8 << text_scale);
			x = begin_x;
		}
		++tmp;
	}
	GX_End();

	__exitPrintState();

	return num_chars;
}