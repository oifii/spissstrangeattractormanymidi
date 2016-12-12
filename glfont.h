/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef _glfonth_
#define _glfonth_

//*********************************************************
//GLFONT.H -- Header for GLFONT.CPP
//Copyright (c) 1998 Brad Fish
//Copyright (c) 2002 Henri Kyrki
//See glFont.txt for terms of use
//10.5 2002
//*********************************************************

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#include <string>
#include <GL/gl.h>

namespace GLFontError {
	struct InvalidFile{};
	struct InvalidFont{};
}

class GLFontBase {
public:
	GLFontBase();
	void Begin();
	virtual ~GLFontBase();
protected:

	void CreateImpl(const string &FileName, GLuint Tex, bool PixelPerfect = FALSE);

	typedef struct
	{
	union {
		float dx;
		int width;
	};
	union {
		float dy;
		int height;
	};
	float tx1, ty1;
	float tx2, ty2;
	} GLFONTCHAR;

	typedef struct
	{
	int Tex;
	int TexWidth, TexHeight;
	int IntStart, IntEnd;
	GLFONTCHAR *Char;
	} GLFONT;

	GLFONT Font;
	bool ok;
private:
	void FreeResources();
};

class GLFont : public GLFontBase {
public:
	GLFont();
	void Create(const string &FileName, GLuint Tex);
	void TextOut (string String, float x, float y, float z);
};

class PixelPerfectGLFont : public GLFontBase {
public:
	PixelPerfectGLFont();
	void Create(const string &FileName, GLuint Tex);
	void TextOut (string String, int x, int y, int z);
};

#endif
//End of file


