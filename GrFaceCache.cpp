/*
The MIT License (MIT)

Copyright (c) 2013 SIL International

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "VCPlugInHeaders.h"

// Language headers

// Interface headers
#include "IPMFont.h"

// Library headers
#include "adobe/unicode.hpp"
#include <graphite2/Font.h>

// Module header
#include "GrFaceCache.h"


GrFaceCache::GrFaceCache(size_t capacity, unsigned int max_dwell)
: _capacity(capacity), 
  _max_dwell(max_dwell)
{
}

GrFaceCache::~GrFaceCache(void)
{
	for (store_t::iterator i = _faces.begin(); i != _faces.end(); ++i)
		destroy_entry(*i);
}


GrFaceCache::value_t GrFaceCache::operator [] (const key_t font)
{
	if (font == nil)	return 0;

	const IPMFont::FontTechnology ft = font->GetFontTechnology();
	const K2Vector<PMString> * const paths = font->GetFullPath();
	const PMString path = paths ? (*paths)[0] : nil;
	if (ft != IPMFont::kTrueTypeFont 
		|| paths[0].empty())
		return 0;

	store_t::iterator i = std::find(_faces.begin(), _faces.end(), path);
	if (i == _faces.end())
	{
		spring_clean();
		i = _faces.insert(_faces.begin(), entry(path, face_from_platform_font(path)));
	}

//	freshen(i);
	return i == _faces.end() ? 0 : (*i).face;
}


void GrFaceCache::destroy_entry(const entry & e)
{
	gr_face_destroy(e.face);
}


void GrFaceCache::spring_clean()
{
	while (_faces.size() >= _capacity)
	{
		destroy_entry(_faces.back());
		_faces.pop_back();
	}
}

void GrFaceCache::freshen(const store_t::iterator & i)
{
	_faces.splice(_faces.begin(), _faces, i);
}

gr_face * GrFaceCache::face_from_platform_font(const PMString & path)
{
	std::string utf8_path;
	adobe::to_utf8(path.begin(), path.end(), std::back_inserter(utf8_path));

	return gr_make_file_face(utf8_path.c_str(), gr_face_default);
}

inline
GrFaceCache::entry::entry(const PMString & k, const value_t f) throw()
: key(k),
  face(f)
{}

inline
bool GrFaceCache::entry::operator ==(const GrFaceCache::entry &rhs) const throw()
{ 
	return rhs.key == key;
}
