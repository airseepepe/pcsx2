/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#include "PsxCommon.h"
#include "VU.h"
#include "iCore.h"
#include "Hw.h"
#include "iR3000A.h"

extern u32 g_psxMaxRecMem;
int g_psxWriteOk=1;
static u32 writectrl;

#ifdef PCSX2_VIRTUAL_MEM

int psxMemInit()
{
	// all mem taken care by memInit
	return 0;
}

void psxMemReset()
{
	memset(psxM, 0, 0x00200000);
}

void psxMemShutdown()
{
}

u8 psxMemRead8(u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;
	
	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				return psxHu8(mem);
			else
				return psxHwRead8(mem);
			break;

#ifdef _DEBUG
		case 0x1d00: assert(0);
#endif

		case 0x1f40:
			mem &= 0x1fffffff;
			return psxHw4Read8(mem);

		case 0x1000: return DEV9read8(mem & 0x1FFFFFFF);

		default:
			assert( g_psxWriteOk );
			return *(u8*)PSXM(mem);
	}
}

u16 psxMemRead16(u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				return psxHu16(mem);
			else
				return psxHwRead16(mem);
			break;

		case 0x1d00:
			SIF_LOG("Sif reg read %x value %x\n", mem, psxHu16(mem));
			switch(mem & 0xF0)
			{
				case 0x40: return psHu16(0x1000F240) | 0x0002;
				case 0x60: return 0;
				default: return *(u16*)(PS2MEM_HW+0xf200+(mem&0xf0));
			}
			break;

		case 0x1f90:
			return SPU2read(mem & 0x1FFFFFFF);
		case 0x1000:
			return DEV9read16(mem & 0x1FFFFFFF);

		default:
			assert( g_psxWriteOk );
			return *(u16*)PSXM(mem);
	}
}

u32 psxMemRead32(u32 mem)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				return psxHu32(mem);
			else
				return psxHwRead32(mem);
			break;

		case 0x1d00:
			SIF_LOG("Sif reg read %x value %x\n", mem, psxHu32(mem));
			switch(mem & 0xF0)
			{
				case 0x40: return psHu32(0x1000F240) | 0xF0000002;
				case 0x60: return 0;
				default: return *(u32*)(PS2MEM_HW+0xf200+(mem&0xf0));
			}
			break;

		case 0x1fff: return g_psxWriteOk;
		case 0x1000:
			return DEV9read32(mem & 0x1FFFFFFF);

		default:
			//assert(g_psxWriteOk);
			if( mem == 0xfffe0130 )
				return writectrl;
			else if( mem == 0xffffffff )
				return writectrl;
			else if( g_psxWriteOk )
				return *(u32*)PSXM(mem);
			else return 0;
	}
}

void psxMemWrite8(u32 mem, u8 value)
{
	u32 t = (mem >> 16) & 0x1fff;

	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				psxHu8(mem) = value;
			else
				psxHwWrite8(mem, value);
			break;

		case 0x1f40:
			mem&= 0x1fffffff;
			psxHw4Write8(mem, value);
			break;

		case 0x1d00:
			SysPrintf("sw8 [0x%08X]=0x%08X\n", mem, value);
			*(u8*)(PS2MEM_HW+0xf200+(mem&0xff)) = value;
			break;

		case 0x1000:
			DEV9write8(mem & 0x1fffffff, value);
			return;

		default:
			assert(g_psxWriteOk);
			*(u8  *)PSXM(mem) = value;
			psxCpu->Clear(mem&~3, 1);
			break;
	}
}

void psxMemWrite16(u32 mem, u16 value)
{
	u32 t = (mem >> 16) & 0x1fff;
	switch(t) {
		case 0x1600:
			//HACK: DEV9 VM crash fix
			break;
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				psxHu16(mem) = value;
			else
				psxHwWrite16(mem, value);
			break;

		case 0x1d00:
			switch (mem & 0xf0) {
				case 0x10:
					// write to ps2 mem
					psHu16(0x1000F210) = value;
					return;
				case 0x40:
				{
					u32 temp = value & 0xF0;
					// write to ps2 mem
					if(value & 0x20 || value & 0x80)
					{
						psHu16(0x1000F240) &= ~0xF000;
						psHu16(0x1000F240) |= 0x2000;
					}
						
					if(psHu16(0x1000F240) & temp) psHu16(0x1000F240) &= ~temp;
					else psHu16(0x1000F240) |= temp;
					return;
				}
				case 0x60:
					psHu32(0x1000F260) = 0;
					return;
				default:
					assert(0);
			}
			return;

		case 0x1f90:
			SPU2write(mem & 0x1FFFFFFF, value); return;
			
		case 0x1000:
			DEV9write16(mem & 0x1fffffff, value); return;
		default:
			assert( g_psxWriteOk );
			*(u16 *)PSXM(mem) = value;
			psxCpu->Clear(mem&~3, 1);
			break;
	}
}

void psxMemWrite32(u32 mem, u32 value)
{
	u32 t = (mem >> 16) & 0x1fff;
	switch(t) {
		case 0x1f80:
			mem&= 0x1fffffff;
			if (mem < 0x1f801000)
				psxHu32(mem) = value;
			else
				psxHwWrite32(mem, value);
			break;

		case 0x1d00:
			switch (mem & 0xf0) {
				case 0x10:
					// write to ps2 mem
					psHu32(0x1000F210) = value;
					return;
				case 0x20:
					// write to ps2 mem
					psHu32(0x1000F220) &= ~value;
					return;
				case 0x30:
					// write to ps2 mem
					psHu32(0x1000F230) |= value;
					return;
				case 0x40:
				{
					u32 temp = value & 0xF0;
					// write to ps2 mem
					if(value & 0x20 || value & 0x80)
					{
						psHu32(0x1000F240) &= ~0xF000;
						psHu32(0x1000F240) |= 0x2000;
					}

					
					if(psHu32(0x1000F240) & temp) psHu32(0x1000F240) &= ~temp;
					else psHu32(0x1000F240) |= temp;
					return;
				}
				case 0x60:
					psHu32(0x1000F260) = 0;
					return;

				default:
					*(u32*)(PS2MEM_HW+0xf200+(mem&0xf0)) = value;
			}
				
			return;

		case 0x1000:
			DEV9write32(mem & 0x1fffffff, value);
			return;

		case 0x1ffe:
			if( mem == 0xfffe0130 ) {
				writectrl = value;
				switch (value) {
					case 0x800: case 0x804:
					case 0xc00: case 0xc04:
					case 0xcc0: case 0xcc4:
					case 0x0c4:
						g_psxWriteOk = 0;
						//PSXMEM_LOG("writectrl: writenot ok\n");
						break;
					case 0x1e988:
					case 0x1edd8:
						g_psxWriteOk = 1;
						//PSXMEM_LOG("writectrl: write ok\n");
						break;
					default:
						PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
						break;
				}
			}
			break;

		default:
			
			if( g_psxWriteOk ) {
				*(u32 *)PSXM(mem) = value;
				psxCpu->Clear(mem&~3, 1);
			}

			break;
	}
}

#else

// TLB functions

#ifdef TLB_DEBUG_MEM
void* PSXM(u32 mem)
{
    return (psxMemRLUT[(mem) >> 16] == 0 ? NULL : (void*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)));
}

void* _PSXM(u32 mem)
{
    return ((void*)(psxMemRLUT[(mem) >> 16] + ((mem) & 0xffff)));
}
#endif

s8 *psxM;
s8 *psxP;
s8 *psxH;
s8 *psxS;
uptr *psxMemWLUT;
uptr *psxMemRLUT;

int psxMemInit()
{
	int i;

	psxMemRLUT = (uptr*)_aligned_malloc(0x10000 * sizeof(uptr),16);
	psxMemWLUT = (uptr*)_aligned_malloc(0x10000 * sizeof(uptr),16);
	memset(psxMemRLUT, 0, 0x10000 * sizeof(uptr));
	memset(psxMemWLUT, 0, 0x10000 * sizeof(uptr));

	psxM = (s8*)SysMmap(PS2MEM_PSX_, 0x00200000);
	psxP = (s8*)SysMmap(PS2MEM_BASE_+0x1f000000, 0x00010000);
	psxH = (s8*)SysMmap(PS2MEM_BASE_+0x1f800000, 0x00010000);
	psxS = (s8*)SysMmap(PS2MEM_BASE_+0x1d000000, 0x00010000);

    assert( (uptr)psxM <= 0xffffffff && (uptr)psxP <= 0xffffffff && (uptr)psxH <= 0xffffffff && (uptr)psxS <= 0xffffffff);

	if (psxMemRLUT == NULL || psxMemWLUT == NULL || 
		psxM == NULL || psxP == NULL || psxH == NULL) {
		SysMessage(_("Error allocating memory")); return -1;
	}

	memset(psxM, 0, 0x00200000);
	memset(psxP, 0, 0x00010000);
	memset(psxH, 0, 0x00010000);
	memset(psxS, 0, 0x00010000);


// MemR
	for (i=0; i<0x0080; i++) psxMemRLUT[i + 0x0000] = (uptr)&psxM[(i & 0x1f) << 16];
	for (i=0; i<0x0080; i++) psxMemRLUT[i + 0x8000] = (uptr)&psxM[(i & 0x1f) << 16];
	for (i=0; i<0x0080; i++) psxMemRLUT[i + 0xa000] = (uptr)&psxM[(i & 0x1f) << 16];

	for (i=0; i<0x0001; i++) psxMemRLUT[i + 0x1f00] = (uptr)&psxP[i << 16];

	for (i=0; i<0x0001; i++) psxMemRLUT[i + 0x1f80] = (uptr)&psxH[i << 16];
	for (i=0; i<0x0001; i++) psxMemRLUT[i + 0xbf80] = (uptr)&psxH[i << 16];

	for (i=0; i<0x0040; i++) psxMemRLUT[i + 0x1fc0] = (uptr)&PS2MEM_ROM[i << 16];
	for (i=0; i<0x0040; i++) psxMemRLUT[i + 0x9fc0] = (uptr)&PS2MEM_ROM[i << 16];
	for (i=0; i<0x0040; i++) psxMemRLUT[i + 0xbfc0] = (uptr)&PS2MEM_ROM[i << 16];

	for (i=0; i<0x0004; i++) psxMemRLUT[i + 0x1e00] = (uptr)&PS2MEM_ROM1[i << 16];
	for (i=0; i<0x0004; i++) psxMemRLUT[i + 0x9e00] = (uptr)&PS2MEM_ROM1[i << 16];
	for (i=0; i<0x0004; i++) psxMemRLUT[i + 0xbe00] = (uptr)&PS2MEM_ROM1[i << 16];

	for (i=0; i<0x0001; i++) psxMemRLUT[i + 0x1d00] = (uptr)&psxS[i << 16];
	for (i=0; i<0x0001; i++) psxMemRLUT[i + 0xbd00] = (uptr)&psxS[i << 16];

// MemW
	for (i=0; i<0x0080; i++) psxMemWLUT[i + 0x0000] = (uptr)&psxM[(i & 0x1f) << 16];
	for (i=0; i<0x0080; i++) psxMemWLUT[i + 0x8000] = (uptr)&psxM[(i & 0x1f) << 16];
	for (i=0; i<0x0080; i++) psxMemWLUT[i + 0xa000] = (uptr)&psxM[(i & 0x1f) << 16];

	for (i=0; i<0x0001; i++) psxMemWLUT[i + 0x1f00] = (uptr)&psxP[i << 16];

	for (i=0; i<0x0001; i++) psxMemWLUT[i + 0x1f80] = (uptr)&psxH[i << 16];
	for (i=0; i<0x0001; i++) psxMemWLUT[i + 0xbf80] = (uptr)&psxH[i << 16];

//	for (i=0; i<0x0008; i++) psxMemWLUT[i + 0xbfc0] = (uptr)&psR[i << 16];

//	for (i=0; i<0x0001; i++) psxMemWLUT[i + 0x1d00] = (uptr)&psxS[i << 16];
//	for (i=0; i<0x0001; i++) psxMemWLUT[i + 0xbd00] = (uptr)&psxS[i << 16];

	return 0;
}

void psxMemReset() {
	memset(psxM, 0, 0x00200000);
	memset(psxP, 0, 0x00010000);
	//memset(psxS, 0, 0x00010000);
}

void psxMemShutdown()
{
	SysMunmap((uptr)psxM, 0x00200000); psxM = NULL;
	SysMunmap((uptr)psxP, 0x00010000); psxP = NULL;
	SysMunmap((uptr)psxH, 0x00010000); psxH = NULL;
    SysMunmap((uptr)psxS, 0x00010000); psxS = NULL;
	_aligned_free(psxMemRLUT);
	_aligned_free(psxMemWLUT);
}

u8 psxMemRead8(u32 mem) {
	char *p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			return psxHu8(mem);
		else
			return psxHwRead8(mem);
	} else
	if (t == 0x1f40) {
		mem&= 0x1fffffff;
		return psxHw4Read8(mem);
	} else {
		p = (char *)(psxMemRLUT[mem >> 16]);
		if (p != NULL) {
			return *(u8 *)(p + (mem & 0xffff));
		} else {
			if (t == 0x1000) return DEV9read8(mem & 0x1FFFFFFF);
			PSXMEM_LOG("err lb %8.8lx\n", mem);
			return 0;
		}
	}
}

u16 psxMemRead16(u32 mem) {
	char *p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			return psxHu16(mem);
		else
			return psxHwRead16(mem);
	} else {
		p = (char *)(psxMemRLUT[mem >> 16]);
		if (p != NULL) {
			if (t == 0x1d00) {
				u16 ret;
				switch(mem & 0xF0)
				{
				case 0x00:
					ret= psHu16(0x1000F200);
					break;
				case 0x10:
					ret= psHu16(0x1000F210);
					break;
				case 0x40:
					ret= psHu16(0x1000F240) | 0x0002;
					break;
				case 0x60:
					ret = 0;
					break;
				default:
					ret = psxHu16(mem);
					break;
				}
				SIF_LOG("Sif reg read %x value %x\n", mem, ret);
				return ret;
			}
			return *(u16 *)(p + (mem & 0xffff));
		} else {
			if (t == 0x1F90)
				return SPU2read(mem & 0x1FFFFFFF);
			if (t == 0x1000) return DEV9read16(mem & 0x1FFFFFFF);
			PSXMEM_LOG("err lh %8.8lx\n", mem);
			return 0;
		}
	}
}

u32 psxMemRead32(u32 mem) {
	char *p;
	u32 t;
	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			return psxHu32(mem);
		else
			return psxHwRead32(mem);
	} else {
		//see also Hw.c
		p = (char *)(psxMemRLUT[mem >> 16]);
		if (p != NULL) {
			if (t == 0x1d00) {
				u32 ret;
				switch(mem & 0xF0)
				{
				case 0x00:
					ret= psHu32(0x1000F200);
					break;
				case 0x10:
					ret= psHu32(0x1000F210);
					break;
				case 0x20:
					ret= psHu32(0x1000F220);
					break;
				case 0x30:	// EE Side
					ret= psHu32(0x1000F230);
					break;
				case 0x40:
					ret= psHu32(0x1000F240) | 0xF0000002;
					break;
				case 0x60:
					ret = 0;
					break;
				default:
					ret = psxHu32(mem);
					break;
				}
				SIF_LOG("Sif reg read %x value %x\n", mem, ret);
				return ret;
			}
			return *(u32 *)(p + (mem & 0xffff));
		} else {
			if (t == 0x1000) return DEV9read32(mem & 0x1FFFFFFF);
			
			if (mem != 0xfffe0130) {
#ifdef PSXMEM_LOG
				if (g_psxWriteOk) PSXMEM_LOG("err lw %8.8lx\n", mem);
#endif
			} else {
				return writectrl;
			}
			return 0;
		}
	}
}

void psxMemWrite8(u32 mem, u8 value) {
	char *p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			psxHu8(mem) = value;
		else
			psxHwWrite8(mem, value);
	} else
	if (t == 0x1f40) {
		mem&= 0x1fffffff;
		psxHw4Write8(mem, value);
	} else {
		p = (char *)(psxMemWLUT[mem >> 16]);
		if (p != NULL) {
			*(u8  *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		} else {
			if ((t & 0x1FFF)==0x1D00) SysPrintf("sw8 [0x%08X]=0x%08X\n", mem, value);
			if (t == 0x1d00) {
				psxSu8(mem) = value; return;
			}
			if (t == 0x1000) {
				DEV9write8(mem & 0x1fffffff, value); return;
			}
			PSXMEM_LOG("err sb %8.8lx = %x\n", mem, value);
		}
	}
}

void psxMemWrite16(u32 mem, u16 value) {
	char *p;
	u32 t;

	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			psxHu16(mem) = value;
		else
			psxHwWrite16(mem, value);
	} else {
		p = (char *)(psxMemWLUT[mem >> 16]);
		if (p != NULL) {
			if ((t & 0x1FFF)==0x1D00) SysPrintf("sw16 [0x%08X]=0x%08X\n", mem, value);
			*(u16 *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		} else {
			if (t == 0x1d00) {
					switch (mem & 0xf0) {
						case 0x10:
							// write to ps2 mem
							psHu16(0x1000F210) = value;
							return;
						case 0x40:
						{
							u32 temp = value & 0xF0;
							// write to ps2 mem
							if(value & 0x20 || value & 0x80)
							{
								psHu16(0x1000F240) &= ~0xF000;
								psHu16(0x1000F240) |= 0x2000;
							}

							
							if(psHu16(0x1000F240) & temp) psHu16(0x1000F240) &= ~temp;
							else psHu16(0x1000F240) |= temp;
							return;
						}
						case 0x60:
							psHu32(0x1000F260) = 0;
							return;

					}
				psxSu16(mem) = value; return;
			}
			if (t == 0x1F90) {
				SPU2write(mem & 0x1FFFFFFF, value); return;
			}
			if (t == 0x1000) {
				DEV9write16(mem & 0x1fffffff, value); return;
			}
			PSXMEM_LOG("err sh %8.8lx = %x\n", mem, value);
		}
	}
}

void psxMemWrite32(u32 mem, u32 value) {
	char *p;
	u32 t;
	
	t = (mem >> 16) & 0x1fff;
	if (t == 0x1f80) {
		mem&= 0x1fffffff;
		if (mem < 0x1f801000)
			psxHu32(mem) = value;
		else
			psxHwWrite32(mem, value);
	} else {
		//see also Hw.c
		p = (char *)(psxMemWLUT[mem >> 16]);
		if (p != NULL) {
			*(u32 *)(p + (mem & 0xffff)) = value;
			psxCpu->Clear(mem&~3, 1);
		} else {
			if (mem != 0xfffe0130) {
				if (t == 0x1d00) {
				MEM_LOG("iop Sif reg write %x value %x\n", mem, value);
					switch (mem & 0xf0) {
						case 0x10:
							// write to ps2 mem
							psHu32(0x1000F210) = value;
							return;
						case 0x20:
							// write to ps2 mem
							psHu32(0x1000F220) &= ~value;
							return;
						case 0x30:
							// write to ps2 mem
							psHu32(0x1000F230) |= value;
							return;
						case 0x40:
						{
							u32 temp = value & 0xF0;
							// write to ps2 mem
							if(value & 0x20 || value & 0x80)
							{
								psHu32(0x1000F240) &= ~0xF000;
								psHu32(0x1000F240) |= 0x2000;
							}

							
							if(psHu32(0x1000F240) & temp) psHu32(0x1000F240) &= ~temp;
							else psHu32(0x1000F240) |= temp;
							return;
						}
						case 0x60:
							psHu32(0x1000F260) = 0;
							return;

					}
					psxSu32(mem) = value; 

					// write to ps2 mem
					if( (mem & 0xf0) != 0x60 )
						*(u32*)(PS2MEM_HW+0xf200+(mem&0xf0)) = value;
					return;   
				}
				if (t == 0x1000) {
					DEV9write32(mem & 0x1fffffff, value); return;
				}

				//if (!g_psxWriteOk) psxCpu->Clear(mem&~3, 1);
#ifdef PSXMEM_LOG
				if (g_psxWriteOk) { PSXMEM_LOG("err sw %8.8lx = %x\n", mem, value); }
#endif
			} else {
				int i;

				writectrl = value;
				switch (value) {
					case 0x800: case 0x804:
					case 0xc00: case 0xc04:
					case 0xcc0: case 0xcc4:
					case 0x0c4:
						if (g_psxWriteOk == 0) break;
						g_psxWriteOk = 0;
						memset(psxMemWLUT + 0x0000, 0, 0x80 * sizeof(uptr));
						memset(psxMemWLUT + 0x8000, 0, 0x80 * sizeof(uptr));
						memset(psxMemWLUT + 0xa000, 0, 0x80 * sizeof(uptr));
						//PSXMEM_LOG("writectrl: writenot ok\n");
						break;
					case 0x1e988:
					case 0x1edd8:
						if (g_psxWriteOk == 1) break;
						g_psxWriteOk = 1;
						for (i=0; i<0x0080; i++) psxMemWLUT[i + 0x0000] = (uptr)&psxM[(i & 0x1f) << 16];
						for (i=0; i<0x0080; i++) psxMemWLUT[i + 0x8000] = (uptr)&psxM[(i & 0x1f) << 16];
						for (i=0; i<0x0080; i++) psxMemWLUT[i + 0xa000] = (uptr)&psxM[(i & 0x1f) << 16];
						//PSXMEM_LOG("writectrl: write ok\n");
						break;
					default:
						PSXMEM_LOG("unk %8.8lx = %x\n", mem, value);
						break;
				}
			}
		}
	}
}

#endif
