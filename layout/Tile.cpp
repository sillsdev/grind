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
#include <IComposeScanner.h>
#include <ICompositionStyle.h>
//#include <IDrawingStyle.h>
#include <IFontInstance.h>
#include <IHierarchy.h>
#include <IJustificationStyle.h>
#include <ILayoutUtils.h>
#include <IPMFont.h>
// Library headers
#include <TabStop.h>
// Module header
#include "Box.h"
#include "FallbackRun.h"
#include "GraphiteRun.h"
#include "GrFaceCache.h"
#include "InlineObjectRun.h"
#include "Run.h"
#include "Tile.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;


namespace 
{
	inline 
	float total_stretch(const bool stretch, const glyf::stretch & ts) {
		return ToFloat(stretch ? ts[glyf::space].max + ts[glyf::letter].max + ts[glyf::glyph].max 
							   : -(ts[glyf::space].min + ts[glyf::letter].min + ts[glyf::glyph].min));
	}

	inline 
	float badness(const PMReal & r) {
		return pow(ToFloat(r),3);
	}

	inline 
	float demerits(const float b, cluster::penalty::type p) {
		return pow(b,2) + (p < 0 ? -pow(p,2) : pow(p,2));
	}

	struct break_point
	{
		break_point(const tile & t) 
		: run(t.begin()), 
		  cluster((*run)->begin()),  
		  demerits(std::numeric_limits<float>::infinity()) {}

		void improve(const tile::const_iterator & r, const run::const_iterator & cl, const float d) {
			if (d > demerits) return;
			run = r; cluster = cl; demerits = d;
		}

		tile::const_iterator	run;
		run::const_iterator	cluster;
		float			demerits;
	};


	inline 
	bool is_glyph(const int gid, const cluster & cl)
	{
		return cl.size() == 1 && cl.front().id() == gid;
	}


	inline
	bool is_tab(const cluster & cl)
	{
		return cl.size() == 1 && cl.front().justification() == glyf::tab;
	}


	PMReal process_tab(run::iterator tab, const PMReal pos, const TabStop & ts, const PMReal width, const PMReal align_width)
	{
		PMReal tab_width = ts.GetPosition() - pos;
				
		if (tab_width > 0.0)
		{
			switch (ts.GetAlignment())
			{
			case TabStop::kTabAlignLeft:	break;
			case TabStop::kTabAlignCenter:	tab_width -= std::min(width/2, tab_width);		break;
			case TabStop::kTabAlignRight:	tab_width -= std::min(width, tab_width);		break;
			case TabStop::kTabAlignChar:	tab_width -= std::min(std::min(align_width, width), tab_width);	break;
			}
			glyf & tg = tab->front();
			tg.kern(tab_width - tg.width());
		}

		return tab_width;
	}


	PageType get_page_type(const IParagraphComposer::RebuildHelper & helper)
	{
		InterfacePtr<IHierarchy> hierarchy(helper.GetDataBase(), helper.GetParcelFrameUID(), UseDefaultIID());
		return Utils<ILayoutUtils>()->GetPageType(UIDRef(helper.GetDataBase(),
														 Utils<ILayoutUtils>()->GetOwnerPageUID(hierarchy)));
	}

}



void tile::clear()
{
	for (iterator i=begin(), i_e = end(); i != i_e; ++i)
		delete *i;
}


tile::~tile()
{
	clear();
}


bool tile::fill_by_span(IComposeScanner & scanner, gr_face_cache & faces, TextIndex offset, TextIndex span)
{
	TextIndex	total_span = 0;

	do
	{
		IDrawingStyle * ds = nil;
		TextIndex		run_span = 0;
		TextIterator	ti = scanner.QueryDataAt(offset, &ds, &run_span);
		if (ti.IsNull())		return true;	// End of line
		if (ds == nil)			return false;	// Problem
		if (run_span > span)	run_span = span;

		do
		{
			run * const r = create_run(faces, ds, ti, run_span);
			if (r == nil)	return false;
			push_back(r);
			r->apply_desired_widths();

			const size_t consumed = r->span();
			run_span -= consumed;
			span	 -= consumed;
			offset	 += consumed;
		} while (run_span > 0);
	} while (span > 0);

	return true;
}


size_t tile::span() const 
{
	size_t s = 0;
	for (const_iterator r = begin(), r_e = end(); r != r_e; ++r)
		s += (*r)->span();

	return s;
}


PMPoint tile::content_dimensions() const
{
	PMReal w = 0, h = 0;
	for (const_iterator r = begin(), r_e = end(); r != r_e; ++r)
	{
		w += (*r)->width();
		h = std::max(h,(*r)->height());
	}

	return PMPoint(w,h);
}


void tile::get_stretch_ratios(glyf::stretch & s) const
{
	InterfacePtr<IJustificationStyle>	js(front()->get_style(), UseDefaultIID());
	InterfacePtr<ICompositionStyle>		cs(front()->get_style(), UseDefaultIID());

	if (js == nil  || cs == nil)
		return;

	PMReal min,des,max;
	switch(cs->GetParagraphAlignment())
	{
	case ICompositionStyle::kTextAlignJustifyFull:
	case ICompositionStyle::kTextAlignJustifyLeft:
	case ICompositionStyle::kTextAlignJustifyCenter:
	case ICompositionStyle::kTextAlignJustifyRight:
		js->GetWordspace(&min, &des, &max);
		s[glyf::space].min = des - min;
		s[glyf::space].max = max - des;
		s[glyf::space].num = 0;

		js->GetLetterspace(&min, &des, &max);
		s[glyf::letter].min = des - min;
		s[glyf::letter].max = max - des;
		s[glyf::letter].num = 0;

		js->GetGlyphscale(&min, &des, &max);
		s[glyf::glyph].min = des - min;
		s[glyf::glyph].max = max - des;
		s[glyf::glyph].num = 0;

		s[glyf::fill].min = s[glyf::space].min;
		s[glyf::fill].max = 1000000.0;
		s[glyf::fill].num = 0;

		s[glyf::fixed].min = 0;
		s[glyf::fixed].max = 0;
		s[glyf::fixed].num = 0;
		break;
	default:
		memset(&s,0,sizeof(s));
		break;
	}
}


void tile::break_into(tile & rest)
{
	if (empty()) return;

	glyf::stretch js, s = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	get_stretch_ratios(js);

	PMReal			advance = 0,
					whitespace_advance = 0;
	PMReal const	desired = _region.Width();

	break_point	best = *this,
		        fallback = *this;
	for (iterator r = begin(), r_e = end(); r != r_e; ++r)
	{
		PMReal const	altered_letterspace = InterfacePtr<IJustificationStyle>((*r)->get_style(), UseDefaultIID())->GetAlteredLetterspace(false),
						desired_adj = desired + altered_letterspace,
						fallback_stretch = desired_adj/1.2;

		if (InterfacePtr<ICompositionStyle>((*r)->get_style(), UseDefaultIID())->GetNoBreak())
		{
			advance += (*r)->width();
			(*r)->calculate_stretch(js, s);
			continue;
		}
		
		PMReal const	space_width = (*r)->get_style()->GetSpaceWidth();
		for (run::iterator cl = (*r)->begin(), cl_e = (*r)->end(); cl != cl_e; ++cl)
		{
			bool const is_whitespace = cl->whitespace();
			if (is_whitespace)
				whitespace_advance += cl->width();
			else
			{
				advance += whitespace_advance + cl->width();
				whitespace_advance = 0;
			}
			cl->calculate_stretch(space_width, js, s);

			const PMReal stretch = desired_adj - advance;
			const float ts = total_stretch(stretch > 0, s),
						b = badness(stretch/ts),
						fb = badness(stretch/(stretch > 0 ? fallback_stretch : ts)),
						d = demerits(b, cl->break_penalty()),
						fd = demerits(fb, cl->break_penalty());
			
			if (stretch < 0 && best.cluster == front()->begin())
				best = fallback;

			if (b < 1)	best.improve(r,cl,d);
			if (fb < 1 && stretch > 0) fallback.improve(r,cl,fd);
		}
	}
	
	if (best.cluster == front()->begin())
		best = fallback;

	// Walk forwards adding any trailing whitespace
	for (iterator const r_e = end(); best.run != r_e; best.cluster = (*best.run)->begin())
	{
		while (best.cluster != (*best.run)->end() && (++best.cluster)->whitespace());
		if (best.cluster != (*best.run)->end() || ++best.run == r_e) break;
	}

	if (advance <= desired || best.run == end())	return;

	if (best.cluster != (*best.run)->end())
		rest.push_back((*best.run)->split(best.cluster));
	rest.splice(rest.end(), *this, ++best.run, end());
}


void tile::set_drop_caps(size_t l, size_t n, tile & rest)
{
	for (iterator r = begin(), r_e = end(); r != r_e; ++r)
	{
		for (run::iterator cl = (*r)->begin(), cl_e = (*r)->end(); cl != cl_e; ++cl, --n)
		{
			if (n == 0)
			{
				rest.push_back((*r)->split(cl));
				break;
			}
		}
		(*r)->scale(l);

		if (n == 0)
		{
			rest.splice(rest.end(), *this, ++r, end());
			rest._region.Left() = _region.Right() = _region.Left() + content_dimensions().X()*l;
			break;
		}
	}
}


void tile::justify()
{
	glyf::stretch js, s = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	get_stretch_ratios(js);

	// Calculate the stretch values for all the classes.
	PMReal  width   = content_dimensions().X();
	PMReal	stretch = _region.Width() - width;
	PMReal  stretches[5] = {0,0,0,0,0};

	// Collect the stretch
	for (const_iterator r = begin(), r_e = end(); r != r_e; ++r)
		(*r)->calculate_stretch(js, s);

	// Drop the last letter as we don't want add letterspace stretch to the
	// last character on a line.
	s[glyf::letter].max -= s[glyf::letter].max/s[glyf::letter].num;
	s[glyf::letter].min -= s[glyf::letter].min/s[glyf::letter].num;
	--s[glyf::letter].num;

	if (stretch == 0)	return;
	if (stretch < 0)
	{
		for (int level = glyf::fill; level != glyf::fixed && stretch != 0; ++level)
			stretch -= stretches[level] = std::max(-s[level].min, stretch);
	}
	else
	{
		for (int level = glyf::fill; level != glyf::fixed && stretch != 0; ++level)
			stretch -= stretches[level] = std::min(s[level].max, stretch);

		// How to deal with emergency stretch
		if (stretch > 0)
			if (s[glyf::space].num > 0)				stretches[glyf::space]  += stretch;
			else if (s[glyf::letter].num > 0)		stretches[glyf::letter] += stretch;
			else return;
	}

	stretches[glyf::fill]   /= s[glyf::fill].num;
	stretches[glyf::space]  /= s[glyf::space].num;
	stretches[glyf::letter] /= s[glyf::letter].num;
	stretches[glyf::glyph]	/= width;

	// Apply stretch levels to each run.
	for (iterator r = begin(), r_e = end(); r != r_e; ++r)
		(*r)->adjust_widths(stretches[glyf::fill], stretches[glyf::space], stretches[glyf::letter], stretches[glyf::glyph]);

	if (s[glyf::letter].num)
		back()->trim_trailing_whitespace(stretches[glyf::letter]);
}


void tile::apply_tab_widths()
{
	if (empty()) return;

	PMReal	pos = position().X(),
			max_pos = pos + dimensions().X();
	PMReal	width = 0,
			align_width = std::numeric_limits<float>::max();

	run::iterator	tab;
	TabStop			ts;
	ts.SetPosition(pos);

	InterfacePtr<ICompositionStyle> cs(front()->get_style(), UseDefaultIID());
	for (iterator r = begin(), r_e = end(); r != r_e && pos < max_pos; ++r)
	{
		// Get this run's font
		InterfacePtr<IFontInstance>	font = (*r)->get_style()->QueryFontInstance(kFalse);
		Text::GlyphID target = font->GetGlyphID(ts.GetAlignToChar().GetValue());
			
		// Find a tab
		run::iterator		cl = (*r)->begin();
		run::iterator const cl_e = (*r)->trailing_whitespace();

		do
		{
			for (; cl != cl_e && !is_tab(*cl); width += cl->width(), ++cl)
			{
				if (width < align_width && is_glyph(target, *cl))
					align_width = width;
			}
			if (cl == cl_e) break;

			width += process_tab(tab, pos, ts, width, align_width);

			ts = cs->GetTabStopAfter(pos + width);
			tab = cl++;
			pos += width;
			width = 0;
			align_width = std::numeric_limits<float>::max();
		} while (cl != cl_e);
	}

	// End of line is an implicit tab stop
	process_tab(tab, pos, ts, width, align_width);
}


PMReal tile::align_text(const IParagraphComposer::RebuildHelper & helper, IJustificationStyle * js, ICompositionStyle * cs)
{
	if (empty()) return 0;

	run & last_run = *back();
	const bool	last_line = last_run.back().break_penalty() == cluster::penalty::mandatory;

	last_run.trim_trailing_whitespace(js->GetAlteredLetterspace(false));

	apply_tab_widths();

	PMReal	alignment_offset = 0,
			line_white_space = dimensions().X() - content_dimensions().X();
	switch (cs->GetParagraphAlignment())
	{
	case ICompositionStyle::kTextAlignJustifyFull:
		justify();
		line_white_space = 0;
		break;
	case ICompositionStyle::kTextAlignJustifyLeft:
		if (!last_line)
		{
			justify();
			line_white_space = 0;
		}
		break;
	case ICompositionStyle::kTextAlignJustifyCenter:
		if (!last_line)
		{
			justify();
			line_white_space = 0;
		}
		else
		{
			line_white_space /= 2;
			alignment_offset = line_white_space;
		}
		break;
	case ICompositionStyle::kTextAlignJustifyRight:
		if (!last_line)
		{
			justify();
			line_white_space = 0;
		}
		else
		{
			alignment_offset = line_white_space;
			line_white_space = 0;
		}
		break;
	case ICompositionStyle::kTextAlignLeft:
		break;
	case ICompositionStyle::kTextAlignCenter:
		line_white_space /= 2;
		alignment_offset = line_white_space;
		break;
	case ICompositionStyle::kTextAlignRight:
		alignment_offset = line_white_space;
		line_white_space = 0;
		break;
	case ICompositionStyle::kTextAlignToBinding:
		switch(get_page_type(helper))
		{
		case kLeftPage:		alignment_offset = line_white_space; line_white_space = 0; break;
		case kUnisexPage:	break;
		case kRightPage:	break;
		}
		break;
	case ICompositionStyle::kTextAlignAwayBinding:
		switch(get_page_type(helper))
		{
		case kLeftPage:		break;
		case kUnisexPage:	break;
		case kRightPage:	alignment_offset = line_white_space; line_white_space = 0; break;
		}
		break;
	default:	
		break;
	}
	last_run.fit_trailing_whitespace(line_white_space);

	return alignment_offset;
}


run * tile::create_run(gr_face_cache &faces, IDrawingStyle * ds, TextIterator & ti, TextIndex span)
{
	InterfacePtr<IPMFont>			font = ds->QueryFont();

	if (ti.IsNull() || font == nil)
		return nil;

	run * r = nil;

	switch((*ti).GetValue())
	{
	case kTextChar_ObjectReplacementCharacter:
		r = new inline_object(ds);
		if (!r)	return nil;

		r->fill(ti, span);
		break;

	default:
		switch(font->GetFontType())
		{
		case IPMFont::kOpenTypeTTFontType:
		case IPMFont::kTrueTypeFontType:
		{
			gr_face * const face = faces[font];
			if (face)
			{
				r = new graphite_run(face, ds);
				if (r && r->fill(ti, span)) break;
				delete r;
			}
		}
		default:
			r = new fallback_run(ds);
			if (r)	r->fill(ti, span);
			break;
		}
		break;
	}

	if (!r || r->span() == 0)
	{
		delete r;
		return nil;
	}

	return r;
}
