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
#include <IDrawingStyle.h>
#include <IFontInstance.h>
// Library headers
#include <unicode/uchar.h>
#include <PMRealGlyphPoint.h>
#include <textiterator.h>
#include "graphite2/Font.h"
#include "graphite2/Segment.h"
// Module header
#include "GraphiteRun.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;

namespace
{
	
	cluster::penalty::type penalty(int const bw)
	{
		if (bw < gr_breakWord)	return (bw-20)/10.0;
		if (bw < 17)			return (bw-17)/4.0;
		if (bw < gr_breakIntra)	return (bw-17)/6.0;
		return bw/40.0;
	}

	int resolve_penalty(const gr_segment * const s, TextIndex i)
	{
		const size_t n_chars = gr_seg_n_cinfo(s);
		const int	before = std::max(i < n_chars ? gr_cinfo_break_weight(gr_seg_cinfo(s, i)) : gr_breakNone, 0),
					after  = std::min(++i < n_chars ? gr_cinfo_break_weight(gr_seg_cinfo(s, i)) : gr_breakNone, 0),
					resolved = std::max(before, -after);
		return resolved == gr_breakNone ? gr_breakClip : resolved;
	}


	glyf::justification_t justification(const gr_segment * const s, const gr_slot * const g)
	{
		if (gr_slot_attached_to(g))	// Attached to a parent must be a diacritic
			return glyf::glyph;

		const gr_char_info * const ci_before = gr_seg_cinfo(s, gr_slot_before(g));
		const gr_char_info * const ci_after  = gr_seg_cinfo(s, gr_slot_after(g));

		if (gr_cinfo_before(ci_before) != gr_cinfo_after(ci_after)) // 1 -> many or many -> many either way only allow glyph stretching.
			return glyf::letter;

		if (ci_before == ci_after)  // We have a letter or a space
			return u_isspace(gr_cinfo_unicode_char(ci_before)) ? glyf::space : glyf::letter;				
		else						// Many to one, a ligature.
			return glyf::letter;
	}
}


run * graphite_run::clone_empty() const
{
	return new graphite_run();
}


bool graphite_run::layout_span(TextIterator ti, size_t span)
{
	if (span ==0)
		return true;

	// Make a graphite font object.
	gr_font *grfont = gr_make_font(ToFloat(_drawing_style->GetEmSpaceWidth(false)), _face);
	if (grfont == nil)
		return false;

	// Make a segment
	WideString	chars;
	ti.AppendToStringAndIncrement(&chars, span);

	gr_segment * const seg = gr_make_seg(grfont, _face, 0, nil, gr_utf16, chars.GrabUTF16Buffer(0), span, gr_nobidi + gr_nomirror);
	if (seg == nil)
	{
		gr_font_destroy(grfont);
		return false;
	}


	// Add the glyphs with their natural widths
	const PMReal y_pos_scale = _drawing_style->GetYScale() / _drawing_style->GetXScale();
	cluster * cl = open_cluster();
	unsigned int	cl_before = gr_cinfo_base(gr_seg_cinfo(seg, gr_slot_before(gr_seg_first_slot(seg)))), 
					cl_after  = gr_cinfo_base(gr_seg_cinfo(seg, gr_slot_after(gr_seg_first_slot(seg))));
	float predicted_orign = 0.0;
	for (const gr_slot * s = gr_seg_first_slot(seg); s != 0; s = gr_slot_next_in_segment(s))
	{
		unsigned int before = gr_cinfo_base(gr_seg_cinfo(seg, gr_slot_before(s)));
		unsigned int after = gr_cinfo_base(gr_seg_cinfo(seg, gr_slot_after(s)));

		cl_before = std::min(before, cl_before);
		if (before > cl_after && gr_slot_can_insert_before(s))
		{
			// Finish off this cluster
			cl->add_chars(cl_after - cl_before + 1);
			cl->break_penalty() = penalty(resolve_penalty(seg, cl_after));

			// Open a fresh one.
			cl = open_cluster();
			cl_before = before;
			predicted_orign = gr_slot_origin_X(s);
		}
		cl_after = std::max(after, cl_after);

		// Add the glyph
		cl->add_glyf(glyf(gr_slot_gid(s), 
						  justification(seg, s), 
						  gr_slot_advance_X(s, 0, grfont),
						  PMPoint(gr_slot_origin_X(s)-predicted_orign, gr_slot_origin_Y(s)*y_pos_scale)));

		predicted_orign = gr_slot_origin_X(s) + gr_slot_advance_X(s, 0, grfont);
	}
	// Close the last open cluster
	cl->add_chars(cl_after - cl_before + 1);
	cl->break_penalty() = penalty(resolve_penalty(seg, cl_after));

	// Tidy up
	gr_seg_destroy(seg);
	gr_font_destroy(grfont);
	return true;
}
