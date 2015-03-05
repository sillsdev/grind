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

// Language headers
// Interface headers
#include "VCPlugInHeaders.h"
// Library headers
// Module header
#include "Box.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;

const cluster::penalty::type	
	cluster::penalty::mandatory		= -10000.0,
	cluster::penalty::whitespace	= -1.0,
	cluster::penalty::word			= -0.5,
	cluster::penalty::intra			= 0.5,
	cluster::penalty::letter		= 0.75,
	cluster::penalty::clip			= 1,
	cluster::penalty::never			= 10000.0;

void cluster::add_glyf(const glyf &g)
{
	push_back(g);
}


PMReal cluster::width() const throw()
{
	PMReal advance = 0;

	for (cluster::const_iterator g = begin(); g != end(); ++g)
		advance += g->width();

	return advance;
}


void cluster::trim(const PMReal alt_ltr_spc)
{
	for (cluster::reverse_iterator g = rbegin(); g != rend(); ++g)
	{
		switch(g->justification())
		{
		case glyf::glyph:
			g->shift(alt_ltr_spc);
			break;
		case glyf::letter:
			g->width() -= alt_ltr_spc;
			return;
			break;
		default:
			break;
		}
	}
}


void cluster::calculate_stretch(const PMReal & space_width, const glyf::stretch & js, glyf::stretch & s) const
{
	for (cluster::const_iterator g = begin(), g_e = end(); g != g_e; ++g)
	{
		const glyf::justification_t level = g->justification();

		switch(level)
		{
		case glyf::fill:
			s[glyf::fill].min += space_width*js[glyf::fill].min;
			s[glyf::fill].max += space_width*js[glyf::fill].max;
			++s[glyf::fill].num;
			break;
		case glyf::space:
			s[glyf::space].min += space_width*js[glyf::space].min;
			s[glyf::space].max += space_width*js[glyf::space].max;
			++s[glyf::space].num;
		case glyf::letter:
			s[glyf::letter].min += space_width*js[glyf::letter].min;
			s[glyf::letter].max += space_width*js[glyf::letter].max;
			++s[glyf::letter].num;
		case glyf::glyph:
			s[glyf::glyph].min += g->width()*js[glyf::glyph].min;
			s[glyf::glyph].max += g->width()*js[glyf::glyph].max;
			++s[glyf::glyph].num;
			break;
		case glyf::fixed:
			++s[glyf::fixed].num;
			break;
		}
	}
}
