// =================================================================================================
//	CFileQTI.h
//		DNML for Macintosh ＱｕｉｃｋＴｉｍｅ互換形式ファイル読込み処理クラス定義
//
//	Copyright 2001, A.Iisaka
// =================================================================================================

#ifndef _H_CFileQTI
#define _H_CFileQTI

#include "CDnmlConst.h"
#include "CGBuffer.h"

// -------------------------------------------------------------------------------------------------
//	･ CFileQTI class
// -------------------------------------------------------------------------------------------------
class CFileQTI
{
private:
	CGBuffer		*mCGB;
	int				xsize, ysize;
public:
							CFileQTI(char* fname, int mode, int r, int g, int b);
							~CFileQTI(void);

	int						GetSizeX(void)				{ return xsize; };
	int						GetSizeY(void)				{ return ysize; };
	unsigned char			*GetBuffer(void)			{ return mCGB->GetBuffer(); };
	unsigned char			*GetMaskBuffer(void)		{ return mCGB->GetMaskBuffer(); };
	CGBuffer				*GetCGBuffer(void)			{ return mCGB; }
	void					SetCGBuffer(CGBuffer *cgb)	{ mCGB = cgb; };
};

#endif	// _H_CFileQTI
