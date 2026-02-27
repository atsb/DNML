// =================================================================================================
//	CFileQTI.cp
//		DNML for Macintosh ＰＤＴ形式ファイル読込み処理
//
//	Copyright 2001, A.Iisaka
// =================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <path2fss.h>

#include "CFileQTI.h"

// -------------------------------------------------------------------------------------------------
//	･ CFileQTI										Constructer				[public]
// -------------------------------------------------------------------------------------------------
CFileQTI::CFileQTI(
	char		*fname,
	int			mode,
	int			r,
	int			g,
	int			b )
{
	FSSpec					spec;
	Rect					rect;
	GraphicsImportComponent	cl = nil;
	GWorldPtr				gw;
	GDHandle				saveGD;
	GWorldPtr				saveGW;
	PixMapHandle			pixMap;
	Str255					pname;
	unsigned char			rr1, gg1, bb1;
	unsigned char			*srcbuf, *dstbuf, *mask2, *dstd, *srcd;
	int						srcbpl, dstbpl, x, y;
	
	strcpy((char *)pname, fname);
	C2PStr((char *)pname);
	FSMakeFSSpec(0, 0, pname, &spec);
	if (GetGraphicsImporterForFile(&spec, &cl) == noErr) {
		GraphicsImportGetNaturalBounds(cl, &rect);
		OffsetRect(&rect, -rect.left, -rect.top);
		GraphicsImportSetBoundsRect(cl, &rect);
		NewGWorld(&gw, 32, &rect, 0, 0, 0);
		GraphicsImportSetGWorld(cl, (CGrafPtr)gw, nil);
		GraphicsImportDraw(cl);

		if ((mCGB = new CGBuffer(rect.right, rect.bottom, 3, (rect.right)*3, true)) != nil) {
			dstbpl = mCGB->GetBPL();
			dstbuf = mCGB->GetBuffer();
			mask2  = mCGB->GetMaskBuffer();

			GetGWorld(&saveGW, &saveGD);
			SetGWorld(gw, nil);
			pixMap = GetGWorldPixMap(gw);
			LockPixels(pixMap);
			srcbuf = (unsigned char*)GetPixBaseAddr(pixMap);
			srcbpl = ((*pixMap)->rowBytes) & 0x3fff;

			if (mode == MASK_COLOR) {
				rr1 = r;
				gg1 = g;
				bb1 = b;
			}
			srcbuf++;
			for (y = 0; y <= rect.bottom - 1; y++) {
				dstd = dstbuf;
				srcd = srcbuf;
				for (x = 0; x <= rect.right - 1; x++) {
					if (mode == MASK_NONE || mode == MASK_ALPHA) {
						*dstd++ = srcd[0];
						*dstd++ = srcd[1];
						*dstd++ = srcd[2];
						*mask2  = 0xff;
					} else {
						if (y == 0 && x == 0 && MASK_AUTO) {
							rr1 = srcd[0];
							gg1 = srcd[1];
							bb1 = srcd[2];
						}
						if ((rr1 == srcd[0]) && (gg1 == srcd[1]) && (bb1 == srcd[2])) {
							*dstd++ = 0;
							*dstd++ = 0;
							*dstd++ = 0;
							*mask2  = 0;
						} else {
							*dstd++ = srcd[0];
							*dstd++ = srcd[1];
							*dstd++ = srcd[2];
							*mask2  = 0xff;
						}
					}
					mask2++;
					srcd += 4;
				}
				dstbuf += dstbpl;
				srcbuf += srcbpl;
			}

			UnlockPixels(pixMap);
			SetGWorld(saveGW, saveGD);

			DisposeGWorld(gw);
			CloseComponent(cl);
		}
	} else {
		mCGB = new CGBuffer(640, 480, 3, 640*3, true, 0);
	}
}

// -------------------------------------------------------------------------------------------------
//	･ CFileQTI										Deconstructer			[public]
// -------------------------------------------------------------------------------------------------
CFileQTI::~CFileQTI(void)
{
	if (mCGB) {
		delete mCGB;
	}
}
