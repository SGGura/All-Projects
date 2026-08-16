/**
 * @file GSP_Driver.c
 * @brief Буфер дисплея и вся логика рисования. Обновление экрана — через Refresh_LCD(Mem_LCD).
 */
#define GSP_DRIVER_BUILD
#include "GSP_Driver.h"
#include "../Config/GUI_GSP_Config.h"
#include <string.h>

/* Объявления функций дисплея (реализация в проекте, например Graphics/ERC12864.c) */
extern void Refresh_LCD(unsigned char *buffer);
extern void Init_LCD(void);

#define SIN45   46341
#define ONEP25  81920

#define MASK_LL 0xffffffffffffffffLL

/* Буфер дисплея (только здесь) */
static unsigned char Mem_LCD[SIZE_DISP];

/* Состояние рисования (используется макросами из GSP_Driver.h) */
unsigned char _color;
short _clipRgn;
short _clipLeft, _clipTop, _clipRight, _clipBottom;
short _cursorX, _cursorY;
short _lineType;
unsigned char _lineThickness;

#ifndef SOLID_LINE
#define SOLID_LINE   0
#endif
#define CLIP_DISABLE 0
#define CLIP_ENABLE  1

//*****************************************************
void PutPixel(short x, short y)
{
	int index;
	unsigned char mask;
	if (x < 0 || y < 0 || x > GetMaxX() || y > GetMaxY()) return;
	if (_clipRgn && (x < _clipLeft || x > _clipRight || y < _clipTop || y > _clipBottom)) return;
	mask = (unsigned char)(1 << (y & 7));
	index = ((y >> 3) << 7) + x;
	if (_color == BLACK) Mem_LCD[index] |= mask;
	else                  Mem_LCD[index] &= (unsigned char)~mask;
}

//*****************************************************
void PutPixelXOR(short x, short y)
{
	int index;
	unsigned char mask;
	if (x < 0 || y < 0 || x > GetMaxX() || y > GetMaxY()) return;
	if (_clipRgn && (x < _clipLeft || x > _clipRight || y < _clipTop || y > _clipBottom)) return;
	mask = (unsigned char)(1 << (y & 7));
	index = ((y >> 3) << 7) + x;
	Mem_LCD[index] ^= mask;
}

//*****************************************************
unsigned short GetPixel(short x, short y)
{
	int index;
	unsigned char mask;
	if (_clipRgn && (x < _clipLeft || x > _clipRight || y < _clipTop || y > _clipBottom)) return 0;
	mask = (unsigned char)(1 << (y & 7));
	index = ((y >> 3) << 7) + x;
	return (Mem_LCD[index] & mask) ? 1 : 0;
}

//*****************************************************
void Bar(short x1, short y1, short x2, short y2)
{
	int i, j;
	long long Coll, Mask;
	short Width, Hight, Hight_Bait;
	if (x1 < 0) x1 = 0;
	if (x2 > GetMaxX()) x2 = GetMaxX();
	if (y1 < 0) y1 = 0;
	if (y2 > GetMaxY()) y2 = GetMaxY();
	if (_clipRgn) {
		if (x1 < _clipLeft)   x1 = _clipLeft;
		if (x2 > _clipRight)  x2 = _clipRight;
		if (y1 < _clipTop)    y1 = _clipTop;
		if (y2 > _clipBottom) y2 = _clipBottom;
	}
	Width = x2 - x1 + 1;
	Hight = y2 - y1 + 1;
	Hight_Bait = Hight / 8;
	if (Hight % 8) ++Hight_Bait;
	Mask = MASK_LL >> (64 - Hight);
	Coll = MASK_LL & Mask;
	Coll <<= y1;
	for (i = 0; i < Width; ++i) {
		if ((x1 + i) < 128) {
			for (j = 0; j < 8; ++j) {
				if (_color == BLACK)
					Mem_LCD[x1 + j * 128 + i] |= *((char *)(&Coll) + j);
				else
					Mem_LCD[x1 + j * 128 + i] &= (unsigned char)~*((char *)(&Coll) + j);
			}
		}
	}
}

//*****************************************************
static void ClearLCD(void)
{
	if (_color == BLACK) memset(Mem_LCD, 0xff, SIZE_DISP);
	else                  memset(Mem_LCD, 0x00, SIZE_DISP);
}

//*****************************************************
unsigned short Line(short x1, short y1, short x2, short y2)
{
	short deltaX, deltaY, error, stepErrorLT, stepErrorGE, stepX, stepY, steep, temp;
	short style, type, isSolidLine;
	int thickness = _lineThickness;
	MoveTo(x2, y2);
	isSolidLine = (_lineType == SOLID_LINE);

	if (x1 == x2) {
		if (y1 > y2) { temp = y1; y1 = y2; y2 = temp; }
		style = 0; type = 1;
		for (temp = y1; temp <= y2; temp++) {
			if (isSolidLine && thickness == 0) { PutPixel(x1, temp); continue; }
			if (isSolidLine && thickness == 1) {
				PutPixel(x1, temp); PutPixel(x1 + 1, temp); PutPixel(x1 + 2, temp);
				continue;
			}
			if (isSolidLine) {
				PutPixel(x1, temp);
				for (int i = 1; i <= thickness; i++) PutPixel(x1 + i, temp);
				continue;
			}
			if (type) {
				if (thickness == 0) PutPixel(x1, temp);
				else {
					PutPixel(x1, temp);
					for (int i = 1; i <= thickness; i++) PutPixel(x1 + i, temp);
				}
			}
			style++;
			if (style == _lineType) { type ^= 1; style = 0; }
		}
		return 1;
	}
	if (y1 == y2) {
		if (x1 > x2) { temp = x1; x1 = x2; x2 = temp; }
		style = 0; type = 1;
		for (temp = x1; temp <= x2; temp++) {
			if (isSolidLine && thickness == 0) { PutPixel(temp, y1); continue; }
			if (isSolidLine && thickness == 1) {
				PutPixel(temp, y1); PutPixel(temp, y1 + 1); PutPixel(temp, y1 + 2);
				continue;
			}
			if (isSolidLine) {
				PutPixel(temp, y1);
				for (int i = 1; i <= thickness; i++) PutPixel(temp, y1 + i);
				continue;
			}
			if (type) {
				if (thickness == 0) PutPixel(temp, y1);
				else {
					PutPixel(temp, y1);
					for (int i = 1; i <= thickness; i++) PutPixel(temp, y1 + i);
				}
			}
			style++;
			if (style == _lineType) { type ^= 1; style = 0; }
		}
		return 1;
	}

	stepX = (x2 >= x1) ? 1 : -1;
	deltaX = (x2 >= x1) ? (x2 - x1) : (x1 - x2);
	stepY = (y2 >= y1) ? 1 : -1;
	deltaY = (y2 >= y1) ? (y2 - y1) : (y1 - y2);
	steep = 0;
	if (deltaX < deltaY) {
		steep = 1;
		temp = deltaX; deltaX = deltaY; deltaY = temp;
		temp = x1; x1 = y1; y1 = temp;
		temp = stepX; stepX = stepY; stepY = temp;
		if (isSolidLine) PutPixel(y1, x1);
	} else {
		if (isSolidLine) PutPixel(x1, y1);
	}
	stepErrorGE = deltaX << 1;
	stepErrorLT = deltaY << 1;
	error = stepErrorLT - deltaX;

	if (isSolidLine && thickness == 0) {
		while (--deltaX >= 0) {
			if (error >= 0) { y1 += stepY; error -= stepErrorGE; }
			x1 += stepX; error += stepErrorLT;
			if (steep) PutPixel(y1, x1); else PutPixel(x1, y1);
		}
	} else if (isSolidLine && thickness == 1) {
		while (--deltaX >= 0) {
			if (error >= 0) { y1 += stepY; error -= stepErrorGE; }
			x1 += stepX; error += stepErrorLT;
			if (steep) {
				PutPixel(y1, x1); PutPixel(y1 + 1, x1); PutPixel(y1 + 2, x1);
			} else {
				PutPixel(x1, y1); PutPixel(x1, y1 + 1); PutPixel(x1, y1 + 2);
			}
		}
	} else if (isSolidLine) {
		while (--deltaX >= 0) {
			if (error >= 0) { y1 += stepY; error -= stepErrorGE; }
			x1 += stepX; error += stepErrorLT;
			if (steep) {
				PutPixel(y1, x1);
				for (int i = 1; i <= thickness; i++) PutPixel(y1 + i, x1);
			} else {
				PutPixel(x1, y1);
				for (int i = 1; i <= thickness; i++) PutPixel(x1, y1 + i);
			}
		}
	} else {
		style = 0; type = 1;
		if (type) {
			if (steep) PutPixel(y1, x1);
			else      PutPixel(x1, y1);
		}
		style++;
		if (style == _lineType) { type ^= 1; style = 0; }
		while (--deltaX >= 0) {
			if (error >= 0) { y1 += stepY; error -= stepErrorGE; }
			x1 += stepX; error += stepErrorLT;
			if (type) {
				if (steep) PutPixel(y1, x1);
				else      PutPixel(x1, y1);
			}
			style++;
			if (style == _lineType) { type ^= 1; style = 0; }
		}
	}
	return 1;
}

//*****************************************************
void FastV_Line(int x, int y0, int y1, int interval) { (void)interval; Line(x, y0, x, y1); }

//*****************************************************
void FastH_Line(int y, int x0, int x1, int interval) { (void)interval; Line(x0, y, x1, y); }

//*****************************************************
unsigned short Bevel(short x1, short y1, short x2, short y2, short rad)
{
	short style, type, xLimit, xPos, yPos, error;
	union { unsigned long Val; unsigned short w[2]; } temp;

	temp.Val = (unsigned long)(SIN45 * rad);
	xLimit = temp.w[1] + 1;
	temp.Val = (unsigned long)(ONEP25 - ((long)rad << 16));
	error = (short)temp.w[1];
	yPos = rad;
	style = 0; type = 1;

	if (rad) {
		x1 += rad; x2 -= rad; y1 += rad; y2 -= rad;
		for (xPos = 0; xPos <= xLimit; xPos++) {
			if ((++style) == _lineType) { type ^= 1; style = 0; }
			if (type) {
				PutPixel(x2 + xPos, y1 - yPos);
				PutPixel(x2 + yPos, y1 - xPos);
				PutPixel(x2 + xPos, y2 + yPos);
				PutPixel(x2 + yPos, y2 + xPos);
				PutPixel(x1 - xPos, y2 + yPos);
				PutPixel(x1 - yPos, y2 + xPos);
				PutPixel(x1 - yPos, y1 - xPos);
				PutPixel(x1 - xPos, y1 - yPos);
				if (_lineThickness) {
					PutPixel(x2 + xPos, y1 - yPos - 1); PutPixel(x2 + xPos, y1 - yPos + 1);
					PutPixel(x2 + yPos + 1, y1 - xPos); PutPixel(x2 + yPos - 1, y1 - xPos);
					PutPixel(x2 + xPos, y2 + yPos - 1); PutPixel(x2 + xPos, y2 + yPos + 1);
					PutPixel(x2 + yPos + 1, y2 + xPos); PutPixel(x2 + yPos - 1, y2 + xPos);
					PutPixel(x1 - xPos, y2 + yPos - 1); PutPixel(x1 - xPos, y2 + yPos + 1);
					PutPixel(x1 - yPos + 1, y2 + xPos); PutPixel(x1 - yPos - 1, y2 + xPos);
					PutPixel(x1 - yPos + 1, y1 - xPos); PutPixel(x1 - yPos - 1, y1 - xPos);
					PutPixel(x1 - xPos, y1 - yPos + 1); PutPixel(x1 - xPos, y1 - yPos - 1);
				}
			}
			if (error > 0) { yPos--; error += 5 + ((xPos - yPos) << 1); }
			else           error += 3 + (xPos << 1);
		}
	}
	if (x2 - x1) Line(x1, y1 - rad, x2, y1 - rad);
	if (y2 - y1) Line(x1 - rad, y1, x1 - rad, y2);
	if ((x2 - x1) || (y2 - y1)) {
		Line(x2 + rad, y1, x2 + rad, y2);
		Line(x1, y2 + rad, x2, y2 + rad);
	}
	return 1;
}

//*****************************************************
unsigned short FillBevel(short x1, short y1, short x2, short y2, short rad)
{
	short yLimit, xPos, yPos, err, xCur, yCur, yNew;
	union { unsigned long Val; unsigned short w[2]; } temp;

	if (rad) {
		x1 += rad; x2 -= rad; y1 += rad; y2 -= rad;
		temp.Val = (unsigned long)(SIN45 * rad);
		yLimit = temp.w[1];
		temp.Val = (unsigned long)(ONEP25 - ((long)rad << 16));
		err = (short)temp.w[1];
		xPos = rad; yPos = 0;
		xCur = xPos; yCur = yPos; yNew = yPos;
		while (yPos <= yLimit) {
			yNew = yPos;
			if (err > 0) { xPos--; err += 5 + ((yPos - xPos) << 1); }
			else         err += 3 + (yPos << 1);
			yPos++;
			if (xCur != xPos) {
				Bar(x1 - xCur, y2 + yCur, x2 + xCur, y2 + yNew);
				Bar(x1 - yNew, y2 + xPos, x2 + yNew, y2 + xCur);
				Bar(x1 - yNew, y1 - xCur, x2 + yNew, y1 - xPos);
				Bar(x1 - xCur, y1 - yNew, x2 + xCur, y1 - yCur);
				xCur = xPos; yCur = yPos;
			}
		}
	}
	if ((x2 - x1) || (y2 - y1)) Bar(x1 - rad, y1, x2 + rad, y2);
	return 1;
}

/* ----- PutImage: 1BPP Flash and External ----- */
static void PutImage1BPP(short left, short top, FLASH_BYTE *bitmap, unsigned char stretch);
static void PutImage1BPPExt(short left, short top, void *bitmap, unsigned char pos);

//*****************************************************
unsigned short PutImage(short left, short top, void *bitmap, unsigned char pos)
{
	FLASH_BYTE *flashAddress;
	unsigned char colorDepth;
	unsigned short colorTemp = GetColor();

	switch (*(short *)bitmap) {
#if defined(USE_BITMAP_FLASH)
	case (short)MEM_FLASH:
		flashAddress = ((BITMAP_FLASH *)bitmap)->address;
		colorDepth = *(flashAddress + 1);
		if (colorDepth == 1) PutImage1BPP(left, top, flashAddress, pos);
		break;
#endif
#if defined(USE_BITMAP_EXTERNAL)
	case EXTERNAL:
		ExternalMemoryCallback((EXTDATA *)bitmap, 1, 1, &colorDepth);
		if (colorDepth == 1) PutImage1BPPExt(left, top, bitmap, pos);
		break;
#endif
	default:
		break;
	}
	SetColor(colorTemp);
	return 1;
}

//*****************************************************
static void PutImage1BPP(short left, short top, FLASH_BYTE *bitmap, unsigned char stretch)
{
	FLASH_BYTE *flashAddress = bitmap + 2;
	unsigned short sizeY = *((FLASH_WORD *)flashAddress); flashAddress += 2;
	unsigned short sizeX = *((FLASH_WORD *)flashAddress); flashAddress += 2;
	unsigned short pallete[2];
	pallete[0] = *((FLASH_WORD *)flashAddress); flashAddress += 2;
	pallete[1] = *((FLASH_WORD *)flashAddress); flashAddress += 2;
	unsigned char temp = 0, mask;
	unsigned short x, y, xc, yc;
	unsigned char stretchX, stretchY;
	unsigned short currentColor;
	short runStart, runLength;
	unsigned char pixelColor;

	if (pallete[0] == pallete[1]) {
		SetColor((unsigned char)pallete[0]);
		if (stretch <= 1) {
			for (y = 0; y < sizeY; y++)
				for (x = 0; x < sizeX; x++) PutPixel(left + x, top + y);
		} else {
			for (y = 0; y < sizeY; y++)
				for (stretchY = 0; stretchY < stretch; stretchY++)
					for (x = 0; x < sizeX; x++)
						for (stretchX = 0; stretchX < stretch; stretchX++)
							PutPixel(left + x * stretch + stretchX, top + y * stretch + stretchY);
		}
		return;
	}
	yc = top;
	for (y = 0; y < sizeY; y++) {
		mask = 0; xc = left; currentColor = pallete[0]; runStart = xc; runLength = 0;
		for (x = 0; x < sizeX; x++) {
			if (mask == 0) { temp = *flashAddress++; mask = 0x01; }
			pixelColor = (mask & temp) ? 1 : 0;
			unsigned short targetColor = pallete[pixelColor];
			if (targetColor == currentColor) runLength++;
			else {
				if (runLength > 0) {
					SetColor((unsigned char)currentColor);
					for (short i = 0; i < runLength; i++) PutPixel(runStart + i, yc);
				}
				currentColor = targetColor; runStart = xc; runLength = 1;
			}
			if (stretch <= 1) xc++; else xc += stretch;
			mask <<= 1;
		}
		if (runLength > 0) {
			SetColor((unsigned char)currentColor);
			for (short i = 0; i < runLength; i++) PutPixel(runStart + i, yc);
		}
		yc++;
	}
}

//*****************************************************
static void PutImage1BPPExt(short left, short top, void *bitmap, unsigned char pos)
{
	unsigned long memOffset;
	BITMAP_HEADER bmp;
	unsigned short pallete[2];
	unsigned char lineBuffer[((128 + 1) / 8) + 1];
	unsigned char *pData;
	short byteWidth;
	unsigned char temp, mask;
	unsigned short sizeX, sizeY, x, y, xc, yc;
	unsigned short currentColor;
	short runStart, runLength;
	unsigned char pixelColor;

	ExternalMemoryCallback((EXTDATA *)bitmap, 0, sizeof(BITMAP_HEADER), &bmp);
	ExternalMemoryCallback((EXTDATA *)bitmap, sizeof(BITMAP_HEADER), 2 * sizeof(unsigned short), pallete);
	memOffset = sizeof(BITMAP_HEADER) + 2 * sizeof(unsigned short);
	byteWidth = bmp.width >> 3;
	if (bmp.width & 0x0007) byteWidth++;
	sizeX = (unsigned short)bmp.width;
	sizeY = (unsigned short)bmp.height;
	if (pos == 1) left = (GetMaxX() - (short)sizeX) / 2;
	else if (pos == 2) top = (GetMaxY() - (short)sizeY) / 2;
	else if (pos == 3) { left = (GetMaxX() - (short)sizeX) / 2; top = (GetMaxY() - (short)sizeY) / 2; }

	if (pallete[0] == pallete[1]) {
		SetColor((unsigned char)pallete[0]);
		for (y = 0; y < sizeY; y++)
			for (x = 0; x < sizeX; x++) PutPixel(left + x, top + y);
		return;
	}
	yc = top;
	for (y = 0; y < sizeY; y++) {
		ExternalMemoryCallback((EXTDATA *)bitmap, (unsigned int)memOffset, byteWidth, lineBuffer);
		memOffset += byteWidth;
		pData = lineBuffer; mask = 0; xc = left; runLength = 0;
		for (x = 0; x < sizeX; x++) {
			if (mask == 0) { temp = *pData++; mask = 0x01; }
			pixelColor = (mask & temp) ? 1 : 0;
			unsigned short targetColor = pallete[pixelColor];
			if (runLength == 0) { currentColor = targetColor; runStart = xc; runLength = 1; }
			else if (targetColor == currentColor) runLength++;
			else {
				SetColor((unsigned char)currentColor);
				for (short i = 0; i < runLength; i++) PutPixel(runStart + i, yc);
				currentColor = targetColor; runStart = xc; runLength = 1;
			}
			xc++; mask <<= 1;
		}
		if (runLength > 0) {
			SetColor((unsigned char)currentColor);
			for (short i = 0; i < runLength; i++) PutPixel(runStart + i, yc);
		}
		yc++;
	}
}

//*****************************************************
short GetImageWidth(void *bitmap)
{
	short width = 0;
	switch (*(short *)bitmap) {
#if defined(USE_BITMAP_FLASH)
	case (short)MEM_FLASH:
		return (short)(*((FLASH_WORD *)((BITMAP_FLASH *)bitmap)->address + 2));
#endif
#if defined(USE_BITMAP_EXTERNAL)
	case EXTERNAL:
		ExternalMemoryCallback((EXTDATA *)bitmap, 4, 2, &width);
		return width;
#endif
	default:
		return 0;
	}
}

//*****************************************************
short GetImageHeight(void *bitmap)
{
	short height = 0;
	switch (*(short *)bitmap) {
#if defined(USE_BITMAP_FLASH)
	case (short)MEM_FLASH:
		return (short)(*((FLASH_WORD *)((BITMAP_FLASH *)bitmap)->address + 1));
#endif
#if defined(USE_BITMAP_EXTERNAL)
	case EXTERNAL:
		ExternalMemoryCallback((EXTDATA *)bitmap, 2, 2, &height);
		return height;
#endif
	default:
		return 0;
	}
}

//*****************************************************
void GSP_Display_Refresh(void)
{
	Refresh_LCD(Mem_LCD);
}

//*****************************************************
void GSP_Display_Clear(void)
{
	ClearLCD();
}

//*****************************************************
void GSP_Display_Init(void)
{
	SetLineType(SOLID_LINE);
	SetLineThickness(0);
	MoveTo(0, 0);
	SetColor(WHITE);
	Init_LCD();
	ClearLCD();
	Refresh_LCD(Mem_LCD);
	SetClip(CLIP_DISABLE);
}

//*****************************************************
int GSP_Init(void)
{
	if (!GSP_HW.alloc || !GSP_HW.free) return -1;
	if (GSP_HW.display.Init) GSP_HW.display.Init();
	return 0;
}

//*****************************************************
int GSP_KeyPoll(void)
{
	if (GSP_HW.keyboard.key_poll) return GSP_HW.keyboard.key_poll();
	return 0;
}

//*****************************************************
int GSP_ReadKey(void)
{
	if (GSP_HW.keyboard.read_key) return GSP_HW.keyboard.read_key();
	return -1;  /* нет нажатой клавиши */
}
