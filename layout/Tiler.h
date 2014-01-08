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

#pragma once

// Language headers
// Interface headers
#include <IParagraphComposer.h>
// Library headers
#include <K2Vector.h>
// Module header

// Forward declarations
// InDesign interfaces
class IDrawingStyle;
// Graphite forward delcarations
// Project forward declarations

namespace nrsc
{

struct line_metrics
{
	line_metrics(const IDrawingStyle * ds = nil);

	PMReal	leading,
			ascent,
			cap_height,
			em_box_height,
			x_height,
			em_box_depth,
			icf_bottom_inset,
			icf_top_inset;
	PMReal & fixed_height;

	PMReal & operator [](int k);
	PMReal operator [](int k) const;
	line_metrics & operator += (const IDrawingStyle * const);
	line_metrics & operator += (const IDrawingStyle &);
};

inline
line_metrics::line_metrics(const IDrawingStyle * ds)
: leading(0.0),
  ascent(0.0),
  cap_height(0.0),
  em_box_height(0.0),
  x_height(0.0),
  fixed_height(ascent),		// Fixed height and ascent are the same since ID takes care of this
  em_box_depth(0.0),
  icf_bottom_inset(0.0),
  icf_top_inset(0.0)
{
	if (ds != nil)
		this->operator += (*ds);
}

inline
PMReal & line_metrics::operator [](int k)
{
	switch(k)
	{
	case Text::kFLOLeading:			return leading; break;
	case Text::kFLOAscent:			return ascent; break;
	case Text::kFLOCapHeight:		return cap_height; break;
	case Text::kFLOEmBoxHeight:		return em_box_height; break;
	case Text::kFLOxHeight:			return x_height; break;
	case Text::kFLOFixedHeight:		return fixed_height; break;
	case Text::kFLOEmBoxDepth: 		return em_box_depth; break;
	case Text::kFLOICFBottomInset: 	return icf_bottom_inset; break;
	case Text::kFLOICFTopInset:		return icf_top_inset; break;
	default:						return leading;
	}
}

inline
PMReal line_metrics::operator [](int k) const {
	return const_cast<line_metrics *>(this)->operator[](k);
}

inline
line_metrics & line_metrics::operator += (const IDrawingStyle &ds)
{
	return this->operator += (&ds);
}



class tiler
{
	static const PMReal GRID_ALIGNMENT_OFFSET;
public:
	tiler(IParagraphComposer::Tiler &, 
			ParcelKey, 
			PMReal y, 
			Text::FirstLineOffsetMetric);
	~tiler(void);

	bool	next_line(TextIndex curr_pos, 
					  const line_metrics & line,
					  Text::GridAlignmentMetric grid_metric,
					  const PMReal & line_indent_left, 
					  const PMReal & line_lndent_right);

	const PMRectCollection	& tiles() const;
	const ParcelKey			& parcel() const;

	bool	line_deep_enough(const line_metrics &) const;
	void	setup_wax_line(IWaxLine * wl, line_metrics & metrics) const;

private:
	bool try_get_tiles(PMReal min_width, const line_metrics & line, TextIndex curr_pos);
	bool get_grid_alignment_metric();

	IParagraphComposer::Tiler & _tiler;
	// Updateable state from IParagraphComposer::Tiler::GetTiles
	ParcelKey					_parcel_key;
	PMReal						_height;
	PMReal						_TOP_height;
	Text::FirstLineOffsetMetric _TOP_height_metric;
	Text::GridAlignmentMetric	_grid_alignment_metric;
	PMRectCollection			_tiles;
	PMReal						_y_offset;
	bool16						_at_TOP;
	bool16						_parcel_pos_dependent;
	PMReal						_left_margin;
	PMReal						_righ_margin;
};

inline
const PMRectCollection & tiler::tiles() const
{
	return _tiles;
}

inline
const ParcelKey	& tiler::parcel() const
{
	return _parcel_key;
}

inline
bool  tiler::line_deep_enough(const line_metrics &lm) const
{
	return !(lm.leading > _height || (_at_TOP && lm[_TOP_height_metric] > _TOP_height));
}

}