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
//#include <IHierarchy.h>
#include <IJustificationStyle.h>
//#include <ILayoutUtils.h>
//#include <IPMFont.h>
#include <IWaxCollection.h>
#include <IWaxGlyphs.h>
#include <IWaxLine.h>
#include <IWaxRun.h>
// Library headers
//#include <TabStop.h>
// Module header
//#include "FallbackRun.h"
//#include "GraphiteRun.h"
//#include "GrFaceCache.h"
//#include "InlineObjectRun.h"
#include "Line.h"
#include "Run.h"
#include "Tile.h"
#include "Tiler.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;


void line::update_line_metrics(line_metrics & lm)
{
	for (const_iterator t = begin(), t_e = end(); t != t_e; ++t)
	{
		for (tile::const_iterator r = t->begin(), r_e = t->end(); r != r_e; ++r)
		{
			lm += (*r)->get_style();
			_span += (*r)->span();
		}
	}
}


void line::fill_wax_line(IWaxLine & wl) const
{
	// Set the number of tiles on the line and whether to allow shuffling
	wl.SetNumberOfTiles(size());
	if (size() > 1)		wl.SetNoShuffle(kTrue);

	// the x position, width and number
	// of characters in each tile.
	int tile_num = 0;
	for (const_iterator t = begin(), t_e = end(); t != t_e; ++t, ++tile_num)
	{
		wl.SetTextSpanInTile(t->span(), tile_num); 
		wl.SetXPosition(t->position().X(), tile_num);
		wl.SetTargetWidth(t->dimensions().X(), tile_num);
	}

}



IWaxLine * nrsc::compose_line(tiler & tile_manager, gr_face_cache & faces, IParagraphComposer::RecomposeHelper & helper, const TextIndex ti)
{
	IComposeScanner	* scanner = helper.GetComposeScanner();
	line_metrics	lm(scanner->GetCompleteStyleAt(ti));
	line			ln;
	bool const		first_line = helper.GetParagraphStart() == ti;

	//PMString fn = helper.GetDataBase()->GetSysFile()->GetString();

	do
	{
		// Get tiles for the line.
		if (!tile_manager.next_line(ti, lm, ln))
			break;

		// Create the first tile and fill with the entire paragraph (this will almost always be overset)
		line::iterator t = ln.begin();
		if (!t->fill_by_span(*scanner, faces, ti, helper.GetParagraphEnd()-ti))
			return nil;

		// Handle drop caps.
		bool drop_caps = first_line && tile_manager.drop_lines() > 1;
		if (drop_caps)
		{
			tile & drop_tile = *t;
			drop_tile.break_drop_caps(lm.drop_caps_scale_factor(tile_manager.drop_lines()), tile_manager.drop_clusters(), *++t);
		}

		// Flow text into any remaining tiles, (not the common case)
		// Push the runoff tile onto the end of the line to collect 
		//  any overset text.
		cluster::penalty::type const max_penalty = ln.size() - int(drop_caps) > 1
													? cluster::penalty::intra 
													: cluster::penalty::letter;
		ln.push_back(tile());
		for (line::iterator t_e = --ln.end(); t != t_e && !t->empty();)
		{
			tile & last = *t;
			last.apply_tab_widths();
			last.break_into(*++t);
		}
		ln.pop_back();

		// Check tile depths
		ln.update_line_metrics(lm);
	} while (tile_manager.need_retry_line(lm) || ln.span() == 0);


	IWaxLine* wl = helper.QueryNewWaxLine();
	if (wl == nil)		return nil;

	if (tile_manager.drop_lines() > 1)
	{
		PMReal const indent = first_line ? ln.front().dimensions().X() : tile_manager.drop_indent();
		int32  const n_lines = tile_manager.drop_lines();
		wl->SetDropCapIndents(1, &indent, &n_lines);
	}

	ln.fill_wax_line(*wl);
	tile_manager.setup_wax_line(wl, lm);

	helper.ApplyComposedLine(wl, ln.span());
	return wl;
}


bool nrsc::rebuild_line(gr_face_cache & faces, const IParagraphComposer::RebuildHelper & helper)
{
	TextIndex	      ti = helper.GetTextIndex();
	IWaxLine const 	* wl = helper.GetWaxLine();
	IComposeScanner * scanner = helper.GetComposeScanner();
	IDrawingStyle   * para_style   = scanner->GetParagraphStyleAt(ti);
	PMReal const	  y_bottom = wl->GetYPosition(),
					  y_top = y_bottom - wl->GetYAdvance();
	InterfacePtr<ICompositionStyle>		cs(para_style, UseDefaultIID());
	InterfacePtr<IJustificationStyle>	js(para_style, UseDefaultIID());
	bool			  has_drop_cap = ti == helper.GetParagraphStart() && wl->GetDropCapIndents() == 1;

	// Rebuild the and refill tile list from the wax line.
	line	ln;
	int tile_span = 0;
	PMReal alignment_offset = 0;
	for (int i=0, n_tiles = wl->GetNumberOfTiles(); i != n_tiles; ++i, ti += tile_span)
	{
		ln.push_back(tile(PMRect(wl->GetXPosition(i), y_top, wl->GetXPosition(i) + wl->GetTargetWidth(i), y_bottom)));
		tile & t = ln.back();
		t.fill_by_span(*scanner, faces, ti, wl->GetTextSpanInTile(i));

		tile_span = t.span();
		if (tile_span != wl->GetTextSpanInTile(i)) return false;
	}

	for (line::iterator t = has_drop_cap ? ++ln.begin() : ln.begin(), t_e = ln.end(); t != t_e; ++t)
		alignment_offset = t->align_text(helper, js, cs);

	if (ln.size() == 0)
		return false;

	InterfacePtr<IWaxCollection> wc(wl,UseDefaultIID());
	if (wc->GetWaxLine() == nil)
		wc->SetWaxLine(wl);

	line::iterator t = ln.begin();
	// Handle drop capse
	if (has_drop_cap)
	{
		t->apply_drop_caps_kern();

		int32 drop_lines = 1.0;
		line_metrics lm(scanner->GetCompleteStyleAt(ti));

		wl->GetDropCapIndents(nil, &drop_lines);
		const PMReal scale = lm.drop_caps_scale_factor(drop_lines);
		PMReal x = t->position().X() - wl->GetXPosition() + alignment_offset;		
		for (tile::iterator r = t->begin(), r_e = t->end(); r != r_e; ++r)
		{
			(*r)->scale(scale);
			IWaxRun * wr = (*r)->wax_run();
			if (wr == nil)
				return false;
			wr->SetXPosition(x);
			wr->SetYPosition(wr->GetYPosition() + (drop_lines-1)*lm.leading);
			x += (*r)->width()*scale;
			wc->AddRun(wr);
		}

		++t;
	}

	// Create the wax runs
	for (line::iterator t_e = ln.end(); t != t_e; ++t)
	{
		PMReal x = t->position().X() - wl->GetXPosition() + alignment_offset;

		for (tile::iterator r = t->begin(), r_e = t->end(); r != r_e; ++r)
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
