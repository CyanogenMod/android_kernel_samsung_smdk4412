/****************************************************************************
 *
 *	Copyright(c) 2012 Yamaha Corporation. All rights reserved.
 *
 *	Module		: mcparser.h
 *
 *	Description	: MC Driver parser header
 *
 *	Version		: 1.0.0	2012.12.13
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.	In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *	claim that you wrote the original software. If you use this software
 *	in a product, an acknowledgment in the product documentation would be
 *	appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *	misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ****************************************************************************/

#ifndef _MCPARSER_H_
#define _MCPARSER_H_

#include "mcresctrl.h"

struct MCDRV_AEC_D7_INFO {
	const UINT8	*pbChunkTop;
	UINT32	dChunkSize;
};


SINT32	McParser_GetD7Chunk(const UINT8 *pbPrm,
				UINT32 dSize,
				struct MCDRV_AEC_D7_INFO *psD7Info);
SINT32	McParser_AnalyzeD7Chunk(
				struct MCDRV_AEC_D7_INFO *psD7Info,
				struct MCDRV_AEC_INFO *psAECInfo);



#endif /* _MCPARSER_H_ */
