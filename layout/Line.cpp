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
#include <IPMFont.h>
#include <IWaxCollection.h>
#include <IWaxLine.h>
#include <IWaxRun.h>
// Library headers
#include <TabStop.h>
// Module header
#include "FallbackRun.h"
#include "InlineObjectRun.h"
#include "Line.h"
#include "Run.h"
#include "Tiler.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;


void tile::clear()
{
	for (iterator i=begin(), i_e = end(); i != i_e; ++i)
		delete *i;
}

tile::~tile()
{
	clear();
}


bool tile::fill_by_span(IComposeScanner & scanner, TextIndex offset, TextIndex span)
{
	TextIndex	total_span = 0;
	PMReal		x = _region.Left();

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
			run * const r = create_run(ds, x, ti, run_span);
			if (r == nil)	return false;
			push_back(r);

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


void tile::break_into(tile & rest)
{
	iterator r = begin(), r_e = end();
	PMReal		  advance = 0;
	PMReal const desired = _region.Width();
	for (; r != r_e; ++r)
	{
		PMReal const advance_ = advance + (*r)->width();
		if (advance_ > desired)	break;
		advance = advance_;
	}
	if (r == r_e)	return;

	run * const overset = *r;
	run::const_iterator cl = overset->find_break(desired - advance);
	if (cl != overset->end())
		rest.push_back(overset->split(cl));
	
	rest.splice(rest.end(), *this, ++r, end());
}


void tile::justify()
{
	run::stretch js, s = {{0,0},{0,0},{0,0},{0,0}};

	// Collect the stretch
	for (const_iterator r = begin(), r_e = end(); r != r_e; ++r)
		(*r)->calculate_stretch(s);

	// Calculate the stretch values for all the classes.
	PMReal	  const width = content_dimensions().X();
	PMReal			stretch = _region.Width() - width;
	if (stretch == 0)	return;

	PMReal stretches[4] = {0,0,0,0};
	for (int level = glyf::fill; level != glyf::fixed && stretch != 0; ++level)
	{
		PMReal const contrib = stretch < 0 ? std::max(s[level].min, stretch) : std::min(s[level].max, stretch);
		stretches[level] = stretch/contrib;
		stretch -= contrib;
	}
	if (stretch != 0)
		stretches[glyf::space] += stretch;

	// Apply each glyphs stretch level.
	//for (iterator r = begin(), r_e = end(); r != r_e; ++r)
	//{
	//	(*r)->get_stretch_ratios(js);

	//	for (run::iterator cl = (*r)->begin(), cl_e = (*r)->end(); cl != cl_e; ++cl)
	//	{
	//		for (cluster::iterator c = (*cl)->begin(), c_e = (*cl)->end(); c != c_e; ++c)
	//		{
	//			const glyf & g = c->glyph;
	//			g.kern() += g.width()*js[g.justification()]*stretches[g.justification()];
	//		}
	//	}
	//}
}


run * tile::create_run(IDrawingStyle * ds, PMReal & x, TextIterator & ti, TextIndex span)
{
	InterfacePtr<IPMFont>			font = ds->QueryFont();
	InterfacePtr<ICompositionStyle> cs(ds, UseDefaultIID());

	if (ti.IsNull() || font == nil || cs == nil)
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
			r = new fallback_run(ds);
			if (r && r->fill(ti, span)) break;
			delete r;

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

	x += r->width();
	for (;!ti.IsNull() && (*ti).GetValue() == kTextChar_Tab; ++ti)
	{
		const TabStop tab = cs->GetTabStopAfter(x);

		cluster * cl = r->open_cluster();
		cl->add_glyf(glyf(ds->GetSpaceGlyph(), glyf::fixed, tab.GetPosition() - x, ds->GetLeading(), 0));
		cl->add_chars();
		cl->break_weight() = cluster::breakweight::whitespace;
		r->close_cluster(cl);

		x = tab.GetPosition();
	}

	return r;
}


void tile::update_line_metrics(line_metrics & lm) const
{
	for (const_iterator r = begin(), r_e = end(); r != r_e; ++r)
		lm += (*r)->get_style();
}


struct line_t : public std::vector<tile*>
{
	~line_t() throw() { clear(); }
	void clear() throw() { for (iterator i = begin(); i != end(); ++i) delete *i; std::vector<tile*>::clear(); }
};


IWaxLine * nrsc::compose_line(tiler & tile_manager, IParagraphComposer::RecomposeHelper & helper, const TextIndex ti)
{
	IComposeScanner	* scanner = helper.GetComposeScanner();
	IDrawingStyle	* ds = scanner->GetCompleteStyleAt(ti);
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
		if (!line.front()->fill_by_span(*scanner, ti, helper.GetParagraphEnd()-ti))
			return nil;

		// Flow text into any remaining tiles, (not the common case)
		// The complicated looking body below gets the most recently added tile, then 
		//  inserts the input tile at *t onto the end, passing it into break into.
		while (++t != tiles.end())
			line.back()->break_into(**line.insert(line.end(), new tile(*t)));
		
		// Collect any overset text for this line into a run off tile so the actual 
		// tiles are correctly set.
		tile runoff;
		line.back()->break_into(runoff);

		// Check tile depths
		line_span = 0;
		for (line_t::iterator i = line.begin(); i != line.end(); ++i)
		{
			(*i)->update_line_metrics(lm);
			line_span += (*i)->span();
		}
	} while (!tile_manager.line_deep_enough(lm) || line_span == 0);


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


bool nrsc::rebuild_line(const IParagraphComposer::RebuildHelper & helper)
{
	const IWaxLine	* wl = helper.GetWaxLine();
	IComposeScanner * scanner = helper.GetComposeScanner();
	TextIndex	ti = helper.GetTextIndex();
	PMReal const	y_bottom = wl->GetYPosition(),
					y_top = y_bottom - wl->GetYAdvance();

	// Rebuild the and refill tile list from the wax line.
	line_t	line;
	int tile_span = 0;
	for (int i=0, n_tiles = wl->GetNumberOfTiles(); i != n_tiles; ++i, ti += tile_span)
	{
		tile * t;
		line.push_back(t = new tile(PMRect(wl->GetXPosition(i), y_top, wl->GetXPosition(i) + wl->GetTargetWidth(i), y_bottom)));
		t->fill_by_span(*scanner, ti, wl->GetTextSpanInTile(i));

		tile_span = t->span();
		if (tile_span != wl->GetTextSpanInTile(i)) return false;
	}

	if (line.size() == 0)
		return false;

	InterfacePtr<IWaxCollection> wc(wl,UseDefaultIID());
	if (wc->GetWaxLine() == nil)
		wc->SetWaxLine(wl);

	// Create the wax runs
	for (line_t::iterator t = line.begin(), t_e = line.end(); t != t_e; ++t)
	{
		PMReal x = (*t)->position().X() - wl->GetXPosition();

		for (tile::iterator r = (*t)->begin(), r_e = (*t)->end(); r != r_e; ++r)
		{
			IWaxRun * wr = (*r)->wax_run();
			if (wr == nil)
				return false;
			wr->SetXPosition(x);
			x += (*r)->width();
			wc->AddRun(wr);
		}
	}
	wc->ConstructionComplete();

	return true;
}
