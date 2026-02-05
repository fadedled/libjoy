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

#ifndef __DLOG_H__
#define __DLOG_H__


#define DLOG_SCALE_8 	0
#define DLOG_SCALE_16 	1

void LOG_Init(void);
void LOG_Scale(int scale);
int  LOG_printf(int x, int y, const char* fmt, ...);
void LOG_Draw(void);

#endif /*__DLOG_H__*/
