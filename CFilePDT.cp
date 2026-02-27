// =================================================================================================
//	CFilePDT.cp
//		DNML for Macintosh ＰＤＴ形式ファイル読込み処理
//
//	Copyright 2001, A.Iisaka
// =================================================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <path2fss.h>

#include "CFilePDT.h"

// -------------------------------------------------------------------------------------------------
//	･ インライン関数
// -------------------------------------------------------------------------------------------------
inline int		ReadInt(unsigned char* b, int pos)
{
	int 	ret;
	ret  =  b[pos];
	ret += (b[pos+1]<<8);
	ret += (b[pos+2]<<16);
	ret += (b[pos+3]<<24);
	return ret;
};

inline bool		CheckHeader(unsigned char* b, char* header)
{
	int		i = 0;
	while (header[i]) {
		if ((char)(b[i]) != header[i]) {
			return false;
		}
		i++;
	}
	return true;
};

// -------------------------------------------------------------------------------------------------
//	･ CFilePDT										Constructer				[public]
// -------------------------------------------------------------------------------------------------
CFilePDT::CFilePDT(
	char		*fname,
	int			mode,
	int			r,
	int			g,
	int			b )
{
	int				num, srccount, i, bit, count, maskptr, n;
	int				filesize, size, index[16];
	unsigned char	*repeat, *buf, *bufend, *src, flag;
	unsigned char	*srcbuf = ReadFile(fname, &filesize);

	if (srcbuf) {
		if (CheckHeader(srcbuf, "PDT1\0")) {
			xsize   = ReadInt(srcbuf, 12);
			ysize   = ReadInt(srcbuf, 16);
			size    = xsize * ysize;
			maskptr = ReadInt(srcbuf, 28);
		} else {
			delete[] srcbuf;
			srcbuf  = nil;
		}
	}
	if ((mCGB = new CGBuffer(xsize, ysize, 3, xsize * 3, true)) != nil) {
		unsigned char	*imgbuf = mCGB->GetBuffer();
		unsigned char	*mskbuf = mCGB->GetMaskBuffer();
		
		if (maskptr) {					// Mask処理
			memset(mskbuf, '\0', size);
			src      = srcbuf + maskptr;
			bit      = 0;
			srccount = filesize - maskptr;		
			bufend   = mskbuf + size;
			buf      = mskbuf;

			while ((buf < bufend) && (srccount > 0)) {
				if (!bit) {
					bit  = 8;
					flag = *src++;
					srccount--;
				}
				if (flag & 0x80) {
					*buf++ = *src++;
					srccount--;
				} else {
					count     = (*src++)+2;
					repeat    = buf-((*src++)+1);
					srccount -= 2;
					for (i = 0; (i < count) && (buf < bufend); i++) {
						*buf++ = *repeat++;
					}
				}
				bit--;
				flag <<= 1;
			}
		}
		size  *= 3;

		if (!strcmp((char *)srcbuf, "PDT10")) {
			// PDT10 形式
			src = srcbuf + 32;
			bit = 0;
			if (maskptr) {
				srccount = maskptr - 32;
			} else {
				srccount = filesize - 32;
			}
			bufend = imgbuf + size;
			buf    = imgbuf;
			while ((buf < bufend) && (srccount > 0)) {
				if (!bit) {
					bit  = 8;
					flag = *src++;
					srccount--;
				}
				if (flag & 0x80) {
					*buf++ = src[2];
					*buf++ = src[1];
					*buf++ = src[0];
					src      += 3;
					srccount -= 3;
				} else {
					num  = *src++;
					num += ((*src++) << 8);
					srccount -= 2;
					count = ((num & 15) + 1) * 3;
					repeat = buf - ((num >> 4) + 1) * 3;
					for (i = 0; (i < count) && (buf < bufend); i++) {
						*buf++ = *repeat++;
					}
				}
				bit--;
				flag <<= 1;
			}
		} else {
			// PDT11 形式
			for (i = 0; i < 16; i++) {
				index[i]  = (srcbuf[i*4+0x420]);
				index[i] |= (srcbuf[i*4+0x421]<<8);
				index[i] |= (srcbuf[i*4+0x422]<<16);
				index[i] |= (srcbuf[i*4+0x423]<<24);
			}
			src = srcbuf + 0x460;
			bit = 0;
			if (maskptr) {
				srccount = maskptr - 32;
			} else {
				srccount = filesize - 32;
			}
			bufend = imgbuf + size;
			buf    = imgbuf;
			while ((buf < bufend) && (srccount > 0)) {
				if (!bit) {
					bit  = 8;
					flag = *src++;
					srccount--;
				}
				if (flag & 0x80) {
					n = (*src++) * 4 + 0x20;
					*buf++ = srcbuf[n+2];
					*buf++ = srcbuf[n+1];
					*buf++ = srcbuf[n];
					srccount--;
				} else {
					num  = *src++;
					srccount--;
					count = (((num >> 4) & 15) + 2) * 3;
					if ((buf + count) >= bufend) {
						count = bufend-buf;
					}
					repeat = buf-(index[num&15])*3;
					for (i = 0; (i < count) && (buf < bufend); i++) {
						*buf++ = *repeat++;
					}
				}
				bit--;
				flag <<= 1;
			}
		}
		delete[] srcbuf;

		if (!maskptr && mode) {
			unsigned char	*sbuf = imgbuf, *srcd;
			unsigned char	*mbuf = mskbuf, *mskd;
			int				bpl   = xsize * 3, rr, gg, bb;

			for (int y = 0; y < ysize; y++) {
				srcd = sbuf;
				mskd = mbuf;
				for (int x = 0; x < xsize; x++) {
					if (x == 0 && y == 0) {
						rr = srcd[0];
						gg = srcd[1];
						bb = srcd[2];
					}
					if (mode == 1) {
						if (srcd[0] == rr && srcd[1] == gg && srcd[2] == bb) {
							*mskd = 0;
						} else {
							*mskd = 0xff;
						}
					} else
					if (mode == 2) {
						if (srcd[0] == r && srcd[1] == g && srcd[2] == b) {
							*mskd = 0;
						} else {
							*mskd = 0xff;
						}
					}
					srcd += 3;
					mskd++;
				}
				sbuf += bpl;
				mbuf += xsize;
			}
		}
	}
}

// -------------------------------------------------------------------------------------------------
//	･ CFilePDT										Deconstructer			[public]
// -------------------------------------------------------------------------------------------------
CFilePDT::~CFilePDT(void)
{
	if (mCGB) {
		delete mCGB;
	}
};

// -------------------------------------------------------------------------------------------------
//	･ ConvertCapital														[protected]
// -------------------------------------------------------------------------------------------------
void
CFilePDT::ConvertCapital(
	unsigned char	*buf )
{
	for (int i = 0; i < strlen((char*)buf); i++) {
		if ((buf[i] >= 'a') && (buf[i] <= 'z')) {
			buf[i] -= 0x20;
		}
	}
}

// -------------------------------------------------------------------------------------------------
//	･ ReadFile																[protected]
// -------------------------------------------------------------------------------------------------
unsigned char *
CFilePDT::ReadFile(
	char		*file,
	int			*size )
{
	FSSpec			spec;
	unsigned char	tmp[256];
	unsigned char	*ret = nil, *ret2;
	short			fRefNum;
	long			count;
	Boolean			packed;
	int				filenum, i, pos;
	
	*size = 0;
	if (__path2fss(file, &spec) == noErr) {
		if (FSpOpenDF(&spec, fsRdPerm, &fRefNum) == noErr) {
			// 先頭４バイトが 'PACL' の時、PAC形式？
			SetFPos(fRefNum, fsFromStart, 0);
			memset(tmp, '\0', sizeof(tmp));
			count = 4;
			FSRead(fRefNum, &count, tmp);
			packed = (!memcmp(tmp, "PACL", 4)) ? true : false;
			SetFPos(fRefNum, fsFromStart, 0);

			if (packed) {
				// PACLファイル
				ConvertCapital((unsigned char*)file);
				count = 32;
				FSRead(fRefNum, &count, tmp);
				if (!strcmp((char*)tmp, "PACL")) {
					filenum = (tmp[16] | (tmp[17] << 8) | (tmp[18] << 16) | (tmp[19] << 24));
					for (i = 0; i < filenum; i++) {
						count = 32;
						FSRead(fRefNum, &count, tmp);
						ConvertCapital(tmp);
						if (!strcmp((char*)tmp, file)) {
							count = (tmp[20] | (tmp[21] << 8) | (tmp[22] << 16) | (tmp[23] << 24));
							pos   = (tmp[16] | (tmp[17] << 8) | (tmp[18] << 16) | (tmp[19] << 24));
							ret   = new unsigned char[count];
							if (ret) {
								SetFPos(fRefNum, fsFromStart, pos);
								FSRead(fRefNum, &count, tmp);
								*size = (int)count;
								ret2 = Unpack(ret, size);	// PACK圧縮解凍
								if (ret2) {
									delete[] ret;
									ret = ret2;
								}
							}
							break;
						}
					}
				}
			} else {
				// RAWファイル
				GetEOF(fRefNum, &count);
				ret   = new unsigned char[count];
				if (ret) {
					SetFPos(fRefNum, fsFromStart, 0);
					FSRead(fRefNum, &count, ret);
					*size = (int)count;
				}
			}
			FSClose(fRefNum);
		}
	}
	return ret;
}

// -------------------------------------------------------------------------------------------------
//	･ Unpack																[protected]
// -------------------------------------------------------------------------------------------------
unsigned char *
CFilePDT::Unpack(
	unsigned char	*src,
	int				*size )
{
	int				num, srccount;
	int				i, bit, count, rawlen;
	unsigned char* repeat;
	unsigned char* bufend;
	unsigned char* dst;
	unsigned char* buf = 0;
	unsigned char flag;

	if (!strcmp((char*)src, "PACK")) {
		*size    = (src[8]  | (src[9]  << 8) | (src[10] << 16) | (src[11] << 24));
		rawlen   = (src[12] | (src[13] << 8) | (src[14] << 16) | (src[15] << 24));
		buf      = new unsigned char[*size];
		dst      = buf;
		bit      = 0;
		srccount = rawlen-16;
		bufend   = buf+(*size);
		src     += 16;
		while ((dst < bufend) && (srccount > 0)) {
			if (!bit) {
				bit  = 8;
				flag = *src++;
				srccount--;
			}
			if (flag & 0x80) {
				*dst++ = *src++;
				srccount--;
			} else {
				num        = *src++;
				num       += ((*src++) << 8);
				srccount  -= 2;
				count      = (num &15) + 2;
				num      >>= 4;
				repeat = (dst - num) - 1;
				for (i = 0; (i < count) && (dst < bufend); i++) {
					*dst++ = *repeat++;
				}
			}
			bit--;
			flag <<= 1;
		}
	}
	return buf;
}
