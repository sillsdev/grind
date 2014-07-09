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
#include <IDrawingStyle.h>
#include <IFontInstance.h>
#include <IHierarchy.h>
#include <IJustificationStyle.h>
#include <ILayoutUtils.h>
#include <IPMFont.h>
#include <IWaxCollection.h>
#include <IWaxLine.h>
#include <IWaxRun.h>
// Library headers
#include <TabStop.h>
// Module header
#include "FallbackRun.h"
#include "GraphiteRun.h"
#include "GrFaceCache.h"
#include "InlineObjectRun.h"
#include "Line.h"
#include "Run.h"
#include "Tiler.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;


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
//			push_back(r);
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
	for (const_iterator cl = begin(), cl_e = end(); cl != cl_e; ++cl)
		s += cl->span();

	return s;
}

PMPoint tile::content_dimensions() const
{
	PMReal w = 0, h = 0;
	PMReal advance = 0;

	for (run::const_iterator cl = base_t::begin(), cl_e = trailing_whitespace(); cl != cl_e; w += cl->width(), ++cl);
	for (const_iterator r = begin(), r_e = end(); r != r_e; h = std::max(h, r->height()), ++r);

	return PMPoint(w,h);
}

namespace 
{
	inline float total_stretch(const bool stretch, const glyf::stretch & ts) {
		return ToFloat(stretch ? ts[glyf::space].max + ts[glyf::letter].max + ts[glyf::glyph].max 
							   : -(ts[glyf::space].min + ts[glyf::letter].min + ts[glyf::glyph].min));
	}

	inline float badness(const PMReal & r) {
		return pow(ToFloat(r),3);
	}

	inline float demerits(const float b, cluster::penalty::type p) {
		return pow(b,2) + (p < 0 ? -pow(p,2) : pow(p,2));
	}
}

struct break_point
{
	break_point(tile & t) 
	: run(t.begin()), 
	  cluster(run->begin()),  
	  demerits(std::numeric_limits<float>::infinity()) {}

	void improve(const tile::iterator & r, const run::iterator & cl, const float d) {
		if (d > demerits) return;
		run = r; cluster = cl; demerits = d;
	}

	tile::iterator	run;
	run::iterator	cluster;
	float			demerits;
};


void tile::fixup_break_weights()
{
	//for (run::iterator r = begin(), r_e = end(), prev_cl = (*r)->begin(); r != r_e; ++r)
	//{
	//	for (run::iterator cl = (*r)->begin(), cl_ = ++cl, cl_e = (*r)->end(); cl != cl_e; prev_cl = cl++)
	//	{
	//	}	
	//}
}


void tile::break_into(tile & rest)
{
	glyf::stretch js, s = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	get_stretch_ratios(js);

	PMReal			advance = 0;
	PMReal const	desired = _region.Width();

	break_point	best = *this,
		        fallback = *this;
	for (iterator r = begin(), r_e = end(); r != r_e; ++r)
	{
		PMReal const	desired_adj = desired + InterfacePtr<IJustificationStyle>(r->get_style(), UseDefaultIID())->GetAlteredLetterspace(false),
						fallback_stretch = desired_adj/1.2;

		PMReal const	space_width = r->get_style()->GetSpaceWidth();
		if (InterfacePtr<ICompositionStyle>(r->get_style(), UseDefaultIID())->GetNoBreak())
		{
			advance += r->width();
			r->calculate_stretch(js, s);
			continue;
		}
		
		for (run::iterator cl = r->begin(), cl_e = r->end(); cl != cl_e; ++cl)
		{
			advance += cl->width();
			cl->calculate_stretch(space_width, js, s);

			const PMReal stretch = desired_adj - advance;
			const float ts = total_stretch(stretch > 0, s),
						b = badness(stretch/ts),
						fb = badness(stretch/(stretch > 0 ? fallback_stretch : ts)),
						d = demerits(b, cl->break_penalty()),
						fd = demerits(fb, cl->break_penalty());
			
			if (b < 1)	best.improve(r,cl,d);
			if (fb < 1) fallback.improve(r,cl,fd);
		}
	}
	
	if (best.cluster == front().begin())
		best = fallback;

	// Walk forwards adding any trailing whitespace
	for (iterator const r_e = end(); best.run != r_e; best.cluster = best.run->begin())
	{
		while (best.cluster != best.run->end() && (++best.cluster)->whitespace());
		if (best.cluster != best.run->end() || ++best.run == r_e) break;
	}

	if (advance <= desired || best.run == end())	return;

	//if (best.cluster != best.run->end())
	//	rest.push_back(best.run->split(best.cluster));
	//rest.splice(rest.end(), *this, ++best.run, end());
	split(base_t::iterator(best.cluster, best.run));
}


void tile::justify()
{
	// Calculate the stretch values for all the classes.
	PMReal  width   = content_dimensions().X();
	PMReal	stretch = _region.Width() - width;
	if (stretch == 0)	return;

	// Collect the stretch
	glyf::stretch total = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	collect_stretch(total);

	// Drop the last letter as we don't want add letterspace stretch to the
	// last character on a line.
	total[glyf::letter].max -= total[glyf::letter].max/total[glyf::letter].num;
	total[glyf::letter].min -= total[glyf::letter].min/total[glyf::letter].num;
	--total[glyf::letter].num;

	PMReal  stretches[5] = {0,0,0,0,0};
	if (stretch < 0)
	{
		for (int level = glyf::fill; level != glyf::fixed && stretch != 0; ++level)
			stretch -= stretches[level] = std::max(total[level].min, stretch);
	}
	else
	{
		for (int level = glyf::fill; level != glyf::fixed && stretch != 0; ++level)
			stretch -= stretches[level] = std::min(total[level].max, stretch);

		// How to deal with emergency stretch
		if (stretch > 0)
			if (total[glyf::space].num > 0)				stretches[glyf::space]  += stretch;
			else if (total[glyf::letter].num > 0)		stretches[glyf::letter] += stretch;
			else return;
	}

	stretches[glyf::fill]   /= total[glyf::fill].num;
	stretches[glyf::space]  /= total[glyf::space].num;
	stretches[glyf::letter] /= total[glyf::letter].num;
	stretches[glyf::glyph]	/= width;

	// Apply stretch levels to each content run
	for (iterator r = begin(), r_e = trailing_whitespace().run_iter(); r != r_e; ++r)
		r->adjust_widths(stretches[glyf::fill], stretches[glyf::space], stretches[glyf::letter], stretches[glyf::glyph]);

	// Apply stretch levels only to fill spaces in trailing whitespace.
	for (iterator r = trailing_whitespace().run_iter(), r_e = end(); r != r_e; ++r)
		r->adjust_widths(stretches[glyf::fill], 0, 0, 0);

	if (total[glyf::letter].num)
		trim_trailing_whitespace(stretches[glyf::letter]);
}



namespace 
{

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

inline
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

}



void tile::apply_tab_widths(ICompositionStyle * cs)
{
	PMReal	pos = position().X(),
			max_pos = pos + dimensions().X();
	PMReal	width = 0,
			align_width = std::numeric_limits<float>::max();

	run::iterator	tab;
	TabStop			ts;
	ts.SetPosition(pos);

	for (iterator r = begin(), r_e = end(); r != r_e && pos < max_pos; ++r)
	{
		// Get this run's font
		InterfacePtr<IFontInstance>	font = r->get_style()->QueryFontInstance(kFalse);
		Text::GlyphID target = font->GetGlyphID(ts.GetAlignToChar().GetValue());
			
		// Find a tab
		run::iterator		cl = r->begin();
		run::iterator const cl_e = &trailing_whitespace().run() == &(*r) ? trailing_whitespace() : r->end();

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


PageType get_page_type(const IParagraphComposer::RebuildHelper & helper)
{
	InterfacePtr<IHierarchy> hierarchy(helper.GetDataBase(), helper.GetParcelFrameUID(), UseDefaultIID());
	return Utils<ILayoutUtils>()->GetPageType(UIDRef(helper.GetDataBase(),
													 Utils<ILayoutUtils>()->GetOwnerPageUID(hierarchy)));
}


PMReal tile::align_text(const IParagraphComposer::RebuildHelper & helper, IJustificationStyle * js, ICompositionStyle * cs)
{
	if (empty()) return 0;

	const bool	last_line = back().back().break_penalty() == cluster::penalty::mandatory;

	trim_trailing_whitespace(js->GetAlteredLetterspace(false));

	apply_tab_widths(cs);

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
	fit_trailing_whitespace(line_white_space);

	return alignment_offset;
}


tile::run * tile::create_run(gr_face_cache &faces, IDrawingStyle * ds, TextIterator & ti, TextIndex span)
{
	InterfacePtr<IPMFont>			font = ds->QueryFont();

	if (ti.IsNull() || font == nil)
		return nil;

	switch((*ti).GetValue())
	{
	case kTextChar_ObjectReplacementCharacter:
	{
		inline_object r(ds);

		if (!r.fill(*this, ti, span))	return nil;
		break;
	}
	default:
		switch(font->GetFontType())
		{
		case IPMFont::kOpenTypeTTFontType:
		case IPMFont::kTrueTypeFontType:
		{
			gr_face * const face = faces[font];
			if (face)
			{
				graphite_run r(face, ds);
				if (r.fill(*this, ti, span))	
					break;
			}
		}
		default:
			fallback_run r(ds);
			if (!r.fill(*this, ti, span))	return nil;
		}
		break;
	}

	if (back().empty())
	{
		runs().pop_back();
		return nil;
	}

	return static_cast<run *>(&runs().back());
}


void tile::update_line_metrics(line_metrics & lm) const
{
	for (const_iterator r = begin(), r_e = end(); r != r_e; ++r)
		lm += r->get_style();
}


struct line_t : public std::vector<tile*>
{
	~line_t() throw() { clear(); }
	void clear() throw() { for (iterator i = begin(); i != end(); ++i) delete *i; std::vector<tile*>::clear(); }
};


IWaxLine * nrsc::compose_line(tiler & tile_manager, gr_face_cache & faces, IParagraphComposer::RecomposeHelper & helper, const TextIndex ti)
{
	IComposeScanner	* scanner = helper.GetComposeScanner();
	IDrawingStyle	* ds = scanner->GetCompleteStyleAt(ti);
	InterfacePtr<ICompositionStyle>		cs(ds, UseDefaultIID());
	const bool	first_line = ti == helper.GetParagraphStart();
	line_t			line;
	line_metrics	lm(ds);
	size_t			line_span;

	do
	{
		line.clear();

		// Get tiles for this line
		if (!tile_manager.next_line(ti, lm, ds))
			break;
		const PMRectCollection & tiles = tile_manager.tiles();

		// Create the first tile and fill with the entire paragraph (this will almost always be overset)
		PMRectCollection::const_iterator t = tiles.begin();
		line.push_back(new tile(*t));
		if (!line.front()->fill_by_span(*scanner, faces, ti, helper.GetParagraphEnd()-ti))
			return nil;

		// Flow text into any remaining tiles, (not the common case)
		// The complicated looking body below gets the most recently added tile, then 
		//  inserts the input tile onto the end, passing it into break_into.
		while (++t != tiles.end())
		{
			tile * & last = line.back();
			last->apply_tab_widths(cs);
			last->break_into(**line.insert(line.end(), new tile(*t)));
		}
		
		// Collect any overset text for this line into a run off tile so the actual 
		// tiles are correctly set.
		line.back()->apply_tab_widths(cs);
		tile runoff;
		line.back()->break_into(runoff);

		// Check tile depths
		line_span = 0;
		for (line_t::iterator i = line.begin(); i != line.end(); ++i)
		{
			(*i)->update_line_metrics(lm);
			line_span += (*i)->span();
		}
	} while (tile_manager.need_retry_line(lm) || line_span == 0);


	// TODO: Investigate use of QueryNewWaxline() instead.
	IWaxLine* wl = helper.QueryNewWaxLine(); //::CreateObject2<IWaxLine>(kWaxLineBoss);
	if (wl == nil)		return nil;

	// Set the number of tiles on the line and whether to allow shuffling
	wl->SetNumberOfTiles(line.size());
	if (line.size() > 1)	wl->SetNoShuffle(kTrue);

	// the x position, width and number
	// of characters in each tile.
	for (int i = 0; i != line.size(); ++i)
	{
		wl->SetTextSpanInTile(line[i]->span(), i); 
		wl->SetXPosition(line[i]->position().X(), i);
		wl->SetTargetWidth(line[i]->dimensions().X(), i);
	}

	tile_manager.setup_wax_line(wl, lm);

	helper.ApplyComposedLine(wl, line_span);
	return wl;
}

bool nrsc::rebuild_line(gr_face_cache & faces, const IParagraphComposer::RebuildHelper & helper)
{
	TextIndex	      ti = helper.GetTextIndex();
	IWaxLine const 	* wl = helper.GetWaxLine();
	IComposeScanner * scanner = helper.GetComposeScanner();
	IDrawingStyle   * para_style   = scanner->GetParagraphStyleAt(ti);
	PMReal const	y_bottom = wl->GetYPosition(),
					y_top = y_bottom - wl->GetYAdvance();
	InterfacePtr<ICompositionStyle>		cs(para_style, UseDefaultIID());
	InterfacePtr<IJustificationStyle>	js(para_style, UseDefaultIID());


	// Rebuild the and refill tile list from the wax line.
	line_t	line;
	int tile_span = 0;
	PMReal alignment_offset = 0;
	for (int i=0, n_tiles = wl->GetNumberOfTiles(); i != n_tiles; ++i, ti += tile_span)
	{
		tile * t;
		line.push_back(t = new tile(PMRect(wl->GetXPosition(i), y_top, wl->GetXPosition(i) + wl->GetTargetWidth(i), y_bottom)));
		t->fill_by_span(*scanner, faces, ti, wl->GetTextSpanInTile(i));

		tile_span = t->span();
		if (tile_span != wl->GetTextSpanInTile(i)) return false;

		alignment_offset = t->align_text(helper, js, cs);
	}

	if (line.size() == 0)
		return false;

	InterfacePtr<IWaxCollection> wc(wl,UseDefaultIID());
	if (wc->GetWaxLine() == nil)
		wc->SetWaxLine(wl);

	// Create the wax runs
	for (line_t::iterator t = line.begin(), t_e = line.end(); t != t_e; ++t)
	{
		PMReal x = (*t)->position().X() - wl->GetXPosition() + alignment_offset;

		for (tile::iterator r = (*t)->begin(), r_e = (*t)->end(); r != r_e; ++r)
		{
			IWaxRun * wr = r->wax_run();
			if (wr == nil)
				return false;
			wr->SetXPosition(x);
			x += r->width();
			wc->AddRun(wr);
		}
	}
	wc->ConstructionComplete();

	return true;
}
