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
#include <IParcel.h>
#include <IFontInstance.h>
#include <IWaxLine.h>
// Library headers
// Module header
#include "Tiler.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations

using namespace nrsc;

line_metrics & line_metrics::operator +=(const IDrawingStyle * const ds)
{
	InterfacePtr<IFontInstance> font = ds->QueryFontInstance(false);
	if (font == nil)	return *this;

	// Update tracked line metric values
	leading		  = std::max(leading,		ds->GetLeading());
	ascent		  = std::max(ascent,		font->GetAscent());
	cap_height	  = std::max(cap_height,	font->GetCapHeight());
	x_height	  = std::max(x_height,		font->GetXHeight());
	em_box_height = std::max(em_box_height,	font->GetEmBoxHeight());
	em_box_depth  = std::max(em_box_depth,  font->GetHorizEmBoxDepth());

	// TODO: Enable if necessary.
	//PMReal top, bottom;
	//font->GetICFBoxInsets(top, bottom);
	//icf_top_inset	 = std::max(icf_top_inset,	top);
	//icf_bottom_inset = std::max(icf_bottom_inset, bottom);

	return *this;
}


const PMReal tiler::GRID_ALIGNMENT_OFFSET;


tiler::tiler(IParagraphComposer::Tiler & t, 
					 ParcelKey pk, 
					 PMReal y, 
					 Text::FirstLineOffsetMetric flo)
: _tiler(t),
  _height(0.0),
  _parcel_key(pk),
  _TOP_height(0.0),
  _TOP_height_metric(flo),
  _grid_alignment_metric(Text::kGANone),
  _y_offset(y),
  _at_TOP(kFalse),
  _parcel_pos_dependent(kFalse),
  _left_margin(0.0),
  _righ_margin(0.0)
{
}


tiler::~tiler(void)
{
}


bool tiler::next_line(TextIndex curr_pos, 
						 const line_metrics & line,
						 Text::GridAlignmentMetric grid_metric,
						 const PMReal & line_indent_left, 
						 const PMReal & line_lndent_right)
{
	_tiles.clear();

	// The minimum width of a tile must be big enough to fit the indents plus one
	// glyph, which we assume to be a capital M.
	const PMReal min_width = line.em_box_height + line_indent_left + line_lndent_right;
	_grid_alignment_metric = grid_metric;
	_height = line.leading;
	do
	{
		_TOP_height = line[_TOP_height_metric];
	}
	while (!try_get_tiles(min_width, line, curr_pos));

	if (_tiles.size() == 0)
	{
		_y_offset += line.leading;
		return false;
	}

	_y_offset = _tiles[0].Bottom();

	return true;
}

bool tiler::try_get_tiles(PMReal min_width, const line_metrics & line, TextIndex curr_pos)
{
	return _tiler.GetTiles(min_width,
						   line.leading,
						   _TOP_height,
						   _grid_alignment_metric,	// Grid alignment metric
						   GRID_ALIGNMENT_OFFSET,	// No offset adjustment 
						   Text::kRomanLeadingModel,// Leading model
						   line.leading,
						   0.0,						// Leading model offset
						   line.leading - line.cap_height,
						   curr_pos,
						   kTrue,					//affectedByVerticalJust.
						   &_parcel_key,
						   &_y_offset, 
						   &_TOP_height_metric,
						   _tiles,
						   &_at_TOP,
						   &_parcel_pos_dependent,
						   &_left_margin,
						   &_righ_margin);	
}


void tiler::setup_wax_line(IWaxLine * wl, line_metrics & metrics) const
{
	// Set the y position, lineheight, tof lineheight and leading model.
	wl->SetCompositionYPosition(_y_offset);
	wl->SetLineHeight(_height);	// TODO: make this use a user provided value, that follows the leading model presumably.
	wl->SetTOFLineHeight(metrics[_TOP_height_metric], _TOP_height_metric);
	wl->SetLeadingModel(Text::kRomanLeadingModel); // TODO: make this use user provided value.

	// Set other wax properties.
	wl->SetGridAlignment(_grid_alignment_metric, GRID_ALIGNMENT_OFFSET);
	wl->SetParcelKey(_parcel_key);
	wl->SetAtTOF(_at_TOP);
	wl->SetParcelPositionDependent(_parcel_pos_dependent);
}
