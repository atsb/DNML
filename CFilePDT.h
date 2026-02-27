// =================================================================================================
//	CFilePDT.h
//		DNML for Macintosh ＰＤＴ形式ファイル読込み処理クラス定義
//
//	Copyright 2001, A.Iisaka
// =================================================================================================

#ifndef _H_CFilePDT
#define _H_CFilePDT

#include "CGBuffer.h"

// -------------------------------------------------------------------------------------------------
//	･ CFilePDT class
// -------------------------------------------------------------------------------------------------
class CFilePDT
{
private:
	CGBuffer		*mCGB;
	int				xsize, ysize;
public:
							CFilePDT(char* fname, int mode, int r, int g, int b);
							~CFilePDT(void);

	int						GetSizeX(void)				{ return xsize; };
	int						GetSizeY(void)				{ return ysize; };
	unsigned char			*GetBuffer(void)			{ return mCGB->GetBuffer(); };
	unsigned char			*GetMaskBuffer(void)		{ return mCGB->GetMaskBuffer(); };
	CGBuffer				*GetCGBuffer(void)			{ return mCGB; }
	void					SetCGBuffer(CGBuffer *cgb)	{ mCGB = cgb; };
	
protected:
	void					ConvertCapital( unsigned char *buf );
	unsigned char			*ReadFile( char *file, int *size );
	unsigned char			*Unpack( unsigned char *src, int *size );
};

#endif	// _H_CFilePDT
