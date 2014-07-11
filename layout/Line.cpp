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
//#include <IFontInstance.h>
//#include <IHierarchy.h>
#include <IJustificationStyle.h>
//#include <ILayoutUtils.h>
//#include <IPMFont.h>
#include <IWaxCollection.h>
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


void line::clear()
{
	for (iterator t = begin(), t_e = end(); t != t_e; ++t) delete *t;
	base_t::clear();
}


void line::update_line_metrics(line_metrics & lm)
{
	for (iterator t = begin(), t_e = end(); t != t_e; ++t)
	{
		for (tile::const_iterator r = (*t)->begin(), r_e = (*t)->end(); r != r_e; ++r)
			lm += (*r)->get_style();
	}
}


IWaxLine * nrsc::compose_line(tiler & tile_manager, gr_face_cache & faces, IParagraphComposer::RecomposeHelper & helper, const TextIndex ti)
{
	IComposeScanner	* scanner = helper.GetComposeScanner();
	line_metrics	lm(scanner->GetCompleteStyleAt(ti));
	line			ln;

	do
	{
		if (tile_manager.drop_lines() > 1)
		{
			line_metrics dclm = lm;
			dclm *= tile_manager.drop_lines();

			// Get tiles for the drop caps line at double height.
			if (!tile_manager.next_line(ti, dclm))
				break;
		}

		// Get tiles for the line at normal height.
		if (!tile_manager.next_line(ti, lm))
			break;

		// Create the first tile and fill with the entire paragraph (this will almost always be overset)
		PMRectCollection const & tiles = tile_manager.tiles();
		PMRectCollection::const_iterator t = tiles.begin();
		ln.push_back(new tile(*t));
		if (!ln.front()->fill_by_span(*scanner, faces, ti, helper.GetParagraphEnd()-ti))
			return nil;

		if (tile_manager.drop_lines() > 1)
		{
			// Break the tile at the requested number of clusters.
			tile * last = ln.back();
			last->apply_tab_widths();
			last->split_into(tile_manager.drop_clusters(), **ln.insert(ln.end(), new tile(*++t)));
		}

		// Flow text into any remaining tiles, (not the common case)
		// The complicated looking body below gets the most recently added tile, then 
		//  inserts the input tile onto the end, passing it into break_into.
		while (++t != tiles.end())
		{
			tile * last = ln.back();
			last->apply_tab_widths();
			last->break_into(**ln.insert(ln.end(), new tile(*t)));
		}
		
		// Collect any overset text for this line into a run off tile so the actual 
		// tiles are correctly set.
		ln.back()->apply_tab_widths();
		tile runoff;
		ln.back()->break_into(runoff);

		// Check tile depths
		ln.update_line_metrics(lm);
	} while (tile_manager.need_retry_line(lm) || ln.span() == 0);


	// TODO: Investigate use of QueryNewWaxline() instead.
	IWaxLine* wl = helper.QueryNewWaxLine(); //::CreateObject2<IWaxLine>(kWaxLineBoss);
	if (wl == nil)		return nil;

	// Set the number of tiles on the line and whether to allow shuffling
	wl->SetNumberOfTiles(ln.size());
	if (ln.size() > 1)	wl->SetNoShuffle(kTrue);

	// the x position, width and number
	// of characters in each tile.
	for (int i = 0; i != ln.size(); ++i)
	{
		wl->SetTextSpanInTile(ln[i]->span(), i); 
		wl->SetXPosition(ln[i]->position().X(), i);
		wl->SetTargetWidth(ln[i]->dimensions().X(), i);
	}

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
	PMReal const	y_bottom = wl->GetYPosition(),
					y_top = y_bottom - wl->GetYAdvance();
	InterfacePtr<ICompositionStyle>		cs(para_style, UseDefaultIID());
	InterfacePtr<IJustificationStyle>	js(para_style, UseDefaultIID());


	// Rebuild the and refill tile list from the wax line.
	line	ln;
	int tile_span = 0;
	PMReal alignment_offset = 0;
	for (int i=0, n_tiles = wl->GetNumberOfTiles(); i != n_tiles; ++i, ti += tile_span)
	{
		tile * t;
		ln.push_back(t = new tile(PMRect(wl->GetXPosition(i), y_top, wl->GetXPosition(i) + wl->GetTargetWidth(i), y_bottom)));
		t->fill_by_span(*scanner, faces, ti, wl->GetTextSpanInTile(i));

		tile_span = t->span();
		if (tile_span != wl->GetTextSpanInTile(i)) return false;

		alignment_offset = t->align_text(helper, js, cs);
	}

	if (ln.size() == 0)
		return false;

	InterfacePtr<IWaxCollection> wc(wl,UseDefaultIID());
	if (wc->GetWaxLine() == nil)
		wc->SetWaxLine(wl);

	// Create the wax runs
	for (line::iterator t = ln.begin(), t_e = ln.end(); t != t_e; ++t)
	{
		PMReal x = (*t)->position().X() - wl->GetXPosition() + alignment_offset;

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
