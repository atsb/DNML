// =================================================================================================
//	CFileLFG.cp
//		DNML for Macintosh Ｌｅａｆ形式ファイル読込み処理
//
//	Copyright 2001, A.Iisaka
// =================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <path2fss.h>

#include "CFileLFG.h"

// =================================================================================================
//	CLeafPack Class
// =================================================================================================

// -------------------------------------------------------------------------------------------------
//	･ CLeafPack										Constructer				[public]
// -------------------------------------------------------------------------------------------------
CLeafPack::CLeafPack(
	char		*inPackName)
{
	FSSpec			spec;
	long			count;
	unsigned char	fbuff[256];
	
	mType  = LPTYPE_UNKNOWN;
	mSize  = 0;
	
	if (__path2fss(inPackName, &spec) != noErr) {
		return;
	}
	if (FSpOpenDF(&spec, fsRdPerm, &mRefNum) != noErr) {
		return;
	}
	GetEOF(mRefNum, &count);
	mSize = count;
	
	count = 10;
	SetFPos(mRefNum, fsFromStart, 0);
	FSRead(mRefNum, &count, fbuff);
	
	// マジックコードのチェック
	if (memcmp(fbuff, "LEAFPACK", 8)) {
		mSize = 0;
		FSClose(mRefNum);
		return;
	}

    /* check type */
	mFileNum = fbuff[8] | fbuff[9]<<8;
    if (mFileNum == 0x0248 || mFileNum == 0x03e1) {
        mType = LPTYPE_TOHEART;
    } else
    if (mFileNum == 0x01fb) {
        mType = LPTYPE_KIZUWIN;
    } else
    if (mFileNum == 0x0193) {
        mType = LPTYPE_SIZUWIN;
    } else
    if (mFileNum == 0x0072) {
        mType = LPTYPE_SAORIN;
    } else {
        mType = LPTYPE_UNKNOWN;
    }

    /* KEY の自動取得 */
    guessKey();
}

// -------------------------------------------------------------------------------------------------
//	･ CLeafPack										Deconstructer			[public]
// -------------------------------------------------------------------------------------------------
CLeafPack::~CLeafPack(void)
{
	FSClose(mRefNum);
}

// -------------------------------------------------------------------------------------------------
//	･ guessKey																[protected]
// -------------------------------------------------------------------------------------------------
void 
CLeafPack::guessKey(void)
{
	unsigned char	*p    = new unsigned char[24 * mFileNum];
	long			count = 24 * mFileNum;
	
	SetFPos(mRefNum, fsFromLEOF, -24 * mFileNum);
	FSRead(mRefNum, &count, p);
	
	/* zero */
	mKey[0] = p[11];

	/* 1st position, (maybe :-)) constant */
	mKey[1] = (p[12] - 0x0a) & 0xff;
	mKey[2] = p[13];
	mKey[3] = p[14];
	mKey[4] = p[15];

	/* 2nd position, from 1st next position */
	mKey[5] = (p[38] - p[22] + mKey[0]) & 0xff;
	mKey[6] = (p[39] - p[23] + mKey[1]) & 0xff;

	/* 3rd position, from 2nd next position */
	mKey[7] = (p[62] - p[46] + mKey[2]) & 0xff;
	mKey[8] = (p[63] - p[47] + mKey[3]) & 0xff;

	/* 1st next position, from 2nd position */
    mKey[9]  = (p[20] - p[36] + mKey[3]) & 0xff;
    mKey[10] = (p[21] - p[37] + mKey[4]) & 0xff;

	delete p;
}

// -------------------------------------------------------------------------------------------------
//	･ regularizeName														[protected]
// -------------------------------------------------------------------------------------------------
void 
CLeafPack::regularizeName(
	char		*inName)
{
	char	buf[12];
	int		i = 0;

	strcpy(buf, inName);
	while (i < 8 && buf[i] != 0x20) {
		inName[i] = buf[i];
		i++;
	}
    inName[i++] = '.';

	/* file extention */
	inName[i++] = buf[8];
	inName[i++] = buf[9];
	inName[i++] = buf[10];

	inName[i] = '\0';
}

// -------------------------------------------------------------------------------------------------
//	･ findFile																[protected]
// -------------------------------------------------------------------------------------------------
Boolean
CLeafPack::findFile(
	char		*inName)
{
	char			*sp, *dp, fname[32];
	unsigned char	*buff = new unsigned char[24 * mFileNum];
	unsigned char	*p    = buff;
	long			count = 24 * mFileNum;
	int				i, j;
	int				k = 0;
	int				b[4];
	
	for (sp = inName, dp = fname; *sp != '\0'; sp++, dp++) {
		if ('a' <= *sp && *sp <= 'z') {
			*dp = *sp - 'a' + 'A';
		} else {
			*dp = *sp;
		}
	}
	*dp = '\0';

	SetFPos(mRefNum, fsFromLEOF, -24 * mFileNum);
	FSRead(mRefNum, &count, p);
	
	for (i = 0; i < mFileNum; i++) {
		/* get filename */
		for (j = 0; j < 12; j++) {
			mFile.name[j] = (*p++ - mKey[k]) & 0xff;
			k = (++k) % LP_KEY_LEN;
		}
		regularizeName(mFile.name);

		/* a position in the archive file */
		for (j = 0; j < 4; j++) {
			b[j] = (*p++ - mKey[k]) & 0xff;
			k = (++k) % LP_KEY_LEN;
		}
		mFile.pos = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];

		/* file length */
		for (j = 0; j < 4; j++) {
			b[j] = (*p++ - mKey[k]) & 0xff;
			k = (++k) % LP_KEY_LEN;
		}
		mFile.len = (b[3] << 24) | (b[2] << 16) | (b[1] << 8) | b[0];

		/* the head of the next file */
		for (j = 0; j < 4; j++) {
			b[j] = (*p++ - mKey[k]) & 0xff;
			k = (++k) % LP_KEY_LEN;
		}

		if (!strcmp(fname, mFile.name)) {
			break;
		}
	}

	delete buff;
	return (i == mFileNum) ? false : true;
}

// -------------------------------------------------------------------------------------------------
//	･ extractFile															[protected]
// -------------------------------------------------------------------------------------------------
unsigned char *
CLeafPack::extractFile(
	size_t		*outSize)
{
	int				i;
	unsigned char	*ret, *buff;
	unsigned char	*p, *q;
	size_t			size;
	long			count = mFile.len;
	
    /* 領域確保 */
	buff = new unsigned char[mFile.len];
	if (!buff)	goto exit_proc;
	ret  = new unsigned char[mFile.len];
	if (!ret)	goto exit_proc;

	size = mFile.len;						/* サイズ */
	if (outSize) {
        *outSize = size;
	}

	SetFPos(mRefNum, fsFromStart, mFile.pos);
	FSRead(mRefNum, &count, buff);
	p = buff;								/* 転送元 */
	q = ret;								/* 転送先 */

	/* キーの解除 & copy */
	for (i = 0; i < size; i++) {
		int a = *p++;
		a = (a - mKey[i % LP_KEY_LEN]) & 0xff;
		*q++ = a;
	}

exit_proc:
	if (buff)	delete buff;
	return ret;
}


// =================================================================================================
//	CFileLFG Class
// =================================================================================================

// -------------------------------------------------------------------------------------------------
//	･ CFileLFG										Constructer				[public]
// -------------------------------------------------------------------------------------------------
CFileLFG::CFileLFG(
	char		*fname,
	int			mode,
	int			r,
	int			g,
	int			b )
{
	char		packName[256], fileName[32];
	CLeafPack	*leafPack = nil;
	size_t		size;
	
	mCGB      = nil;
	xsize     = 0;
	ysize     = 0;
	mFileBuff = nil;

	/* ファイル名の分離 */
	if (!separateFileName(fname, packName, fileName)) {
		goto exit_proc;
	}

	/* パックファイル・オープン */
	if ((leafPack = new CLeafPack(packName)) == nil) {
		return;
	}
	
	/* データのアンパック */
	if (!leafPack->findFile(fileName)) {
		goto exit_proc;
	}
	if ((mFileBuff = leafPack->extractFile(&size)) == nil) {
		goto exit_proc;
	}
	
	/* イメージ展開 */
	if (strstr(fileName, ".LFG") != nil) {
		loadLFG(mode, r, g, b);
	} else
	if (strstr(fileName, ".LF2") != nil) {
		loadLF2(mode, r, g, b);
	}
	
exit_proc:
	if (leafPack)	delete leafPack;
}

// -------------------------------------------------------------------------------------------------
//	･ CFileLFG										Deconstructer			[public]
// -------------------------------------------------------------------------------------------------
CFileLFG::~CFileLFG(void)
{
	if (mFileBuff)		delete[] mFileBuff;
}

// -------------------------------------------------------------------------------------------------
//	･ separateFileName														[protected]
// -------------------------------------------------------------------------------------------------
Boolean
CFileLFG::separateFileName(
	char		*fname,
	char		*packName,
	char		*fileName)
{
	char		*sp, *dp, *lp, pathName[256];
	
	for (sp = fname, dp = pathName; *sp != '\0'; sp++, dp++) {
		if ('a' <= *sp && *sp <= 'z') {
			*dp = *sp - 'a' + 'A';
		} else {
			*dp = *sp;
		}
	}
	*dp = '\0';
	
	if ((lp = strstr(pathName, ".PAK")) == nil) {
		return false;
	}
	lp += 4;
	memcpy(packName, pathName, lp - pathName);
	packName[lp-pathName] = '\0';
	strcpy(fileName, lp + 1);

	return true;
}

// -------------------------------------------------------------------------------------------------
//	･ loadLFG																[protected]
// -------------------------------------------------------------------------------------------------
void
CFileLFG::loadLFG(
	int			mode,
	int			r,
	int			g,
	int			b)
{
	int				width, height, xoffset, yoffset;
	size_t			size;
	int				transColor, paletteNum;
	unsigned char	palette[16][3];
	unsigned char	*work;	/* 作業領域 */


	/* check magic number 'LEAFCODE' */
	if (memcmp(mFileBuff, "LEAFCODE", 8)){
		return;
	}

    /* サイズ取得 */
	xoffset =  (mFileBuff[33] << 8 | mFileBuff[32]) * 8;
	yoffset =   mFileBuff[35] << 8 | mFileBuff[34];
	width   = ((mFileBuff[37] << 8 | mFileBuff[36]) + 1) * 8 - xoffset;
	height  = ((mFileBuff[39] << 8 | mFileBuff[38]) + 1)     - yoffset;
    size    = mFileBuff[44] | (mFileBuff[45] << 8)
    		| (mFileBuff[46] << 16) | (mFileBuff[47] << 24);
	if ((work = new unsigned char[size]) == nil) {
		return;
	}
	
	/* イメージ領域確保 */
	if ((mCGB = new CGBuffer(width, height, 3, width*3, true, 0)) == nil) {
		delete work;
		return;
	}

	/* read palette */
	paletteNum = 16;
	transColor = mFileBuff[41];
    {
		const unsigned char		*p = mFileBuff + 8;
		unsigned char			*q = &palette[0][0];
        int						upper, lower;

		for (int i = 0; i < 24; i++) {
			upper = *p   & 0xf0; upper |= upper >> 4;
			lower = *p++ & 0x0f; lower |= lower << 4;
			*q++ = upper;
			*q++ = lower;
		}
    }

	/* 展開 */
	{
		unsigned char	*loadBuf = mFileBuff + 48;
		unsigned char	*saveBuf = work;
		unsigned char	*ring = new unsigned char[0x1000];
	    int				i, j, c, m, flag, pos, len;
	
		/* initialize ring buffer */
		memset(ring, 0, 0x1000);

		/* extract data */
		for (i = 0, c = 0, m = 0xfee; i < size;) {
	
			/* flag bits, which indicates data or location */
			if (--c < 0) {
				flag = *loadBuf++;
				c = 7;
			}
			if (flag & 0x80) {
				/* data */
				saveBuf[i++] = ring[m++] = *loadBuf++;
				m &= 0xfff;
			} else {
				/* copy from ring buffer */
				int data = loadBuf[0] + (loadBuf[1]<<8); loadBuf += 2;
	            
				len = (data & 0x0f) + 3;
				pos = data >> 4;
	            
				for (j = 0; j < len; j++) {
					saveBuf[i++] = ring[m++] = ring[pos++];
					m   &= 0xfff;
					pos &= 0xfff;
				}
			}
			flag = flag << 1;
		}
		delete ring;
	}

	/* convert to indexed image */
	{
		int				i;
        int				direction = mFileBuff[40];
		unsigned char	*p        = work;
		unsigned char	q1, q2;

		unsigned char	*dstbuf   = mCGB->GetBuffer();
		unsigned char	*mskbuf   = mCGB->GetMaskBuffer();
		int				dstbpl    = mCGB->GetBPL();
		int				dstbpp    = mCGB->GetBPP();
		int				mskbpl    = mCGB->GetSizeX();
		unsigned char	*dstd, *mskd;
		int				tr        = -1;
		
		if (!direction) {	/* VERTICAL */
			int		x = 0, y = 0;
			for (i = 0; i < size; i++, p++) {
				q1 = (*p & 0x80)>>4 | (*p & 0x20)>>3 | (*p & 0x08)>>2 | (*p & 0x02)>>1;
				dstd = dstbuf + y*dstbpl + x*dstbpp;
				mskd = mskbuf + y*mskbpl + x;
				dstd[0] = palette[q1][0];
				dstd[1] = palette[q1][1];
				dstd[2] = palette[q1][2];
				dstd   += dstbpp;
				if (mode == MASK_NONE || mode == MASK_ALPHA) {
					*mskd++ = 0xff;
				} else {
					if (mode == MASK_AUTO && transColor > paletteNum) {
						if (tr == -1) {
							tr = q1;
							*mskd++ = 0x00;
						} else {
							*mskd++ = (q1 == tr) ? 0x00 : 0xff;
						}
					} else {
						*mskd++ = (q1 == transColor) ? 0x00 : 0xff;
					}
				}

                q2 = (*p & 0x40)>>3 | (*p & 0x10)>>2 | (*p & 0x04)>>1 | (*p & 0x01);
				dstd[0] = palette[q2][0];
				dstd[1] = palette[q2][1];
				dstd[2] = palette[q2][2];
				if (mode == MASK_NONE || mode == MASK_ALPHA) {
					*mskd++ = 0xff;
				} else {
					if (mode == MASK_AUTO && transColor > paletteNum) {
						*mskd++ = (q2 == tr) ? 0x00 : 0xff;
					} else {
						*mskd++ = (q2 == transColor) ? 0x00 : 0xff;
					}
				}

				if (++y >= height) {
					y  = 0;
					x += 2;
				}
			}
		} else {			/* HORIZONTAL */
			unsigned char	*dstd = dstbuf;
			unsigned char	*mskd = mskbuf;
			
			for (i = 0; i < size; i++, p++) {
				q1 = (*p & 0x80)>>4 | (*p & 0x20)>>3 | (*p & 0x08)>>2 | (*p & 0x02)>>1;
				dstd[0] = palette[q1][0];
				dstd[1] = palette[q1][1];
				dstd[2] = palette[q1][2];
				dstd   += dstbpp;
				if (mode == MASK_NONE || mode == MASK_ALPHA) {
					*mskd++ = 0xff;
				} else {
					if (mode == MASK_AUTO && transColor > paletteNum) {
						if (tr == -1) {
							tr = q1;
							*mskd++ = 0x00;
						} else {
							*mskd++ = (q1 == tr) ? 0x00 : 0xff;
						}
					} else {
						*mskd++ = (q1 == transColor) ? 0x00 : 0xff;
					}
				}
				q2 = (*p & 0x40)>>3 | (*p & 0x10)>>2 | (*p & 0x04)>>1 | (*p & 0x01);
				dstd[0] = palette[q2][0];
				dstd[1] = palette[q2][1];
				dstd[2] = palette[q2][2];
				dstd   += dstbpp;
				if (mode == MASK_NONE || mode == MASK_ALPHA) {
					*mskd++ = 0xff;
				} else {
					if (mode == MASK_AUTO && transColor > paletteNum) {
						*mskd++ = (q2 == tr) ? 0x00 : 0xff;
					} else {
						*mskd++ = (q2 == transColor) ? 0x00 : 0xff;
					}
				}
			}
		}
	}

    /* 作業領域解放 */
	delete work;
}

// -------------------------------------------------------------------------------------------------
//	･ loadLF2																[protected]
// -------------------------------------------------------------------------------------------------
void
CFileLFG::loadLF2(
	int			mode,
	int			r,
	int			g,
	int			b)
{
	int				width, height, xoffset, yoffset;
	size_t			size;
	int				transColor, paletteNum;
	unsigned char	palette[256][3];
	
    /* check magic number 'LEAF256' */
    if (memcmp(mFileBuff, "LEAF256\0", 8)) {
		return;
    }

	/* サイズ取得 */
//	xoffset =  (mFileBuff[9]  << 8) | mFileBuff[8];
//	yoffset =  (mFileBuff[11] << 8) | mFileBuff[10];
//	width   = ((mFileBuff[13] << 8) | mFileBuff[12]) + xoffset;
//	height  = ((mFileBuff[15] << 8) | mFileBuff[14]) + yoffset;
//	size    = (width - xoffset) * (height - yoffset);
	xoffset = 0;
	yoffset = 0;
	width   = ((mFileBuff[13] << 8) | mFileBuff[12]);
	height  = ((mFileBuff[15] << 8) | mFileBuff[14]);
	size    = width * height;

    /* read palette */
	transColor = mFileBuff[0x12];
	paletteNum = mFileBuff[0x16];
    {
        const unsigned char	*p = mFileBuff + 0x18;
		for (int i = 0; i < paletteNum; i++) {
			palette[i][2] = *p++;
			palette[i][1] = *p++;
			palette[i][0] = *p++;
		}
	}

	/* イメージ領域確保 */
	if ((mCGB = new CGBuffer(width, height, 3, width*3, true, 0)) == nil) {
		return;
	}
	unsigned char	*dstbuf = mCGB->GetBuffer();
	unsigned char	*mskbuf = mCGB->GetMaskBuffer();
	int				dstbpl  = mCGB->GetBPL();
	int				dstbpp  = mCGB->GetBPP();
	int				mskbpl  = mCGB->GetSizeX();
	unsigned char	*dstd, *mskd;
	
	/* イメージ展開 */
	{
		unsigned char	*ring = new unsigned char[0x1000];
		unsigned char	d;
		int		i, j, c, m, flag;
		int		upper, lower, pos, len, idx, p;
		int		rwidth  = width  - xoffset;
		int		rheight = height - yoffset;
		int		tr = -1;
		
		p = 0x18 + paletteNum * 3;

		/* initialize ring buffer */
		memset(ring, '\0', 0x1000);
  
		/* extract data */
		for (i = 0, c = 0, m = 0xfee; i < size; ) {
			/* flag bits, which indicates data or location */
			if (--c < 0) {
				flag = mFileBuff[p++];
				flag ^= 0xff;
				c = 7;
			}
    
			if (flag & 0x80) {
				/* data */
				d = mFileBuff[p++];
				d ^= 0xff;
				ring[m++] = d;
				idx = (i % rwidth) + rwidth * (rheight - i / rwidth - 1);

				dstd = dstbuf + idx * dstbpp;
				dstd[0] = palette[d][0];
				dstd[1] = palette[d][1];
				dstd[2] = palette[d][2];
				mskd = mskbuf + idx;
				if (mode == MASK_NONE || mode == MASK_ALPHA) {
					*mskd = 0xff;
				} else {
					if (mode == MASK_AUTO && transColor > paletteNum) {
						if (tr == -1) {
							tr = d;
							*mskd = 0x00;
						} else {
							if (d == tr) {
								*mskd = 0x00;
							} else {
								*mskd = 0xff;
							}
						}
					} else {
						*mskd = (d == transColor) ? 0x00 : 0xff;
					}
				}
				i++;
				m &= 0xfff;
			} else {
				/* copy from ring buffer */
				upper = mFileBuff[p++];
				upper ^= 0xff;

				lower = mFileBuff[p++];
				lower ^= 0xff;
      
				len = (upper & 0x0f) + 3;
				pos = (upper >> 4) + (lower << 4);

				for(j = 0; j < len && i < size; j++) {
					idx = (i % rwidth) + rwidth * (rheight - i / rwidth - 1);
					ring[m++] = d = ring[pos++];
					dstd = dstbuf + idx * dstbpp;
					dstd[0] = palette[d][0];
					dstd[1] = palette[d][1];
					dstd[2] = palette[d][2];
					mskd = mskbuf + idx;
					if (mode == MASK_NONE || mode == MASK_ALPHA) {
						*mskd = 0xff;
					} else {
						if (mode == MASK_AUTO && transColor > paletteNum) {
							if (tr == -1) {
								tr = d;
								*mskd = 0x00;
							} else {
								if (d == tr) {
									*mskd = 0x00;
								} else {
									*mskd = 0xff;
								}
							}
						} else {
							*mskd = (d == transColor) ? 0x00 : 0xff;
						}
					}

					i++;
					m &= 0xfff;
					pos &= 0xfff;
				}
			}
			flag = flag << 1;
		}

		delete[] ring;
	}
}
