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
#include <IPMFont.h>
#include <IGridRelatedStyle.h>
#include <ITextParcelList.h>
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
	// We use the an IPMFont and point size instead of an IFontInstance because
	// the IFontInstance will transform all it's metrics through it's set matrix
	// such as when horizontal or vertical scaling is used and we need unscaled values.
	const PMReal point_sz = ds->GetPointSize();
	InterfacePtr<IPMFont> font = ds->QueryFont();
	if (font == nil)	return *this;

	// Update tracked line metric values
	leading		  = std::max(leading,		ds->GetLeading());
	ascent		  = std::max(ascent,		font->GetAscent(point_sz));
	cap_height	  = std::max(cap_height,	font->GetCapHeight(point_sz));
	x_height	  = std::max(x_height,		font->GetXHeight(point_sz));
	em_box_height = std::max(em_box_height,	font->GetEmBoxHeight(point_sz, false));
	em_box_depth  = std::max(em_box_depth,  font->GetHorizEmBoxDepth());

	// TODO: Enable if necessary.
	//PMReal top, bottom;
	//font->GetICFBoxInsets(point_sz, top, bottom, false);
	//icf_top_inset	 = std::max(icf_top_inset,	top);
	//icf_bottom_inset = std::max(icf_bottom_inset, bottom);

	return *this;
}


const PMReal tiler::GRID_ALIGNMENT_OFFSET;


tiler::tiler(IParagraphComposer::RecomposeHelper & helper)
: _helper(helper),
  _height(0.0),
  _parcel_key(helper.GetStartingParcelKey()),
  _TOP_height(0.0),
  _TOP_height_metric(_parcel_key.IsValid() 
						? helper.GetTextParcelList()->GetFirstLineOffsetMetric(_parcel_key) 
					    : Text::kFLOLeading),
  _grid_alignment_metric(Text::kGANone),
  _y_offset(helper.GetStartingYPosition()),
  _at_TOP(kFalse),
  _parcel_pos_dependent(kFalse),
  _left_margin(0.0),
  _right_margin(0.0)
{
}


tiler::~tiler(void)
{
}


bool tiler::next_line(TextIndex curr_pos, 
						 const line_metrics & line,
						 const IDrawingStyle * ds)
{
	_tiles.clear();

	InterfacePtr<ICompositionStyle> cs(ds, UseDefaultIID());
	InterfacePtr<IGridRelatedStyle> grs(ds, UseDefaultIID());

	if (cs == nil || grs == nil)	return false;

	// The minimum width of a tile must be big enough to fit the indents plus one
	// glyph, which we assume to be a capital M.
	const bool first_line = curr_pos == _helper.GetParagraphStart();
	_height = line.leading;
	_y_offset_original = _y_offset;
	_grid_alignment_metric = (grs->GetAlignOnlyFirstLine() == kFalse || first_line) 
								? grs->GetGridAlignmentMetric() 
								: Text::kGANone;
	const PMReal	line_indent_left  = cs->IndentLeftBody()
									     + (first_line ? cs->IndentLeftFirst() : 0), 
					line_indent_right = cs->IndentRightBody(),
					min_width		  = line.em_box_height + line_indent_left + line_indent_right;
	do
	{
		_TOP_height = line[_TOP_height_metric];
	}
	while (!try_get_tiles(min_width, line, curr_pos));

	for (PMRectCollection::iterator r = _tiles.begin(); r != _tiles.end(); ++r)
	{
		r->Left() = std::max(r->Left(), _left_margin + line_indent_left);
		r->Right() = std::min(r->Right(), _right_margin - line_indent_right);
	}

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
	return _helper.GetTiles(min_width,
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
						   &_right_margin);	
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
