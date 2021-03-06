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
#include <list>
// Interface headers
#include <IParagraphComposer.h>
// Library headers
// Module header
#include "Box.h"

// Forward declarations
// InDesign interfaces
class IComposeScanner;
class ICompositionStyle;
class IDrawingStyle;
class IJustificationStyle;
// Graphite forward delcarations

namespace nrsc 
{
// Project forward declarations
class	gr_face_cache;
struct	line_metrics;
class	run;

class tile : private std::list<run*>
{
	typedef std::list<run*>	base_t;

	PMRect	_region;

	// disable the assignment operator.
	tile &	operator = (const tile &);

	static run    * create_run(gr_face_cache & faces, IDrawingStyle * ds, TextIterator & ti, TextIndex span);

public:
	tile();
	tile(const PMRect & region);
	virtual ~tile() throw();

	// Member types
	using base_t::const_iterator;
	using base_t::iterator;
	using base_t::difference_type;
	using base_t::size_type;

	//Iterators
	using base_t::begin;
	using base_t::end;

	// Capacity
	using base_t::empty;
	using base_t::size;
	size_t	span() const;

	// Geometry
	PMPoint     position() const;
	PMPoint     content_dimensions() const;
	PMPoint	    dimensions() const;

	// Element access
	using base_t::front;
	using base_t::back;

	// Modifiers
	using base_t::push_front;
	using base_t::push_back;
	void	clear();
	bool	fill_by_span(IComposeScanner & scanner, gr_face_cache & faces, TextIndex offset, TextIndex span);

	// Operations
	void	justify(bool ragged);
	void	apply_tab_widths();
	PMReal	align_text(const IParagraphComposer::RebuildHelper & helper, IJustificationStyle * js, ICompositionStyle *);
	void	break_into(tile & rest, cluster::penalty::type const max_penalty = cluster::penalty::clip);
	void	break_drop_caps(PMReal scale, int elems, tile &);
	void	get_stretch_ratios(glyf::stretch & js) const;
};



inline
tile::tile()
{
}


inline
tile::tile(const PMRect & region)
: _region(region)
{
}


inline
PMPoint tile::position() const
{
	return _region.LeftTop();
}


inline
PMPoint	tile::dimensions() const
{
	return _region.Dimensions();
}

} // end of namespace nrsc