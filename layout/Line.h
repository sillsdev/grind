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
#include <PMRect.h>
#include <PMReal.h>
// Module header
#include "ClusterThread.h"

// Forward declarations
class PMReal;
// InDesign interfaces
class IComposeScanner;
class ICompositionStyle;
class IDrawingStyle;
class IJustificationStyle;
class IWaxLine;
// Graphite forward delcarations

namespace nrsc 
{
// Project forward declarations
class	gr_face_cache;
struct	line_metrics;
class	run;
class   tiler;

class tile : private cluster_thread
{
	typedef cluster_thread	base_t;

	PMRect	_region;

	// disable the assignment operator.
	tile(const tile & rhs);
	tile & operator = (const tile &);


	run *		create_run(gr_face_cache & faces, IDrawingStyle * ds, TextIterator & ti, TextIndex span);
	void		fixup_break_weights();

public:
	tile(const PMRect & region=PMRect());
	virtual ~tile() throw();

	// Member types
	typedef base_t::runs_t::const_iterator			const_iterator;
	typedef base_t::runs_t::iterator				iterator;
	typedef base_t::runs_t::const_reverse_iterator	const_reverse_iterator;
	typedef base_t::runs_t::reverse_iterator		reverse_iterator;
	typedef base_t::runs_t::difference_type			difference_type;
	typedef base_t::runs_t::size_type				size_type;
	typedef base_t::runs_t::reference				reference;
	typedef base_t::runs_t::const_reference			const_reference;

	//Iterators
	iterator		begin()					{ return runs().begin(); }
	const_iterator	begin() const			{ return runs().begin(); }
	iterator		end()					{ return runs().end(); }
	const_iterator	end() const				{ return runs().end(); }
	reverse_iterator 		rbegin()		{ return runs().rbegin(); }
	const_reverse_iterator 	rbegin() const	{ return runs().rbegin(); }
	reverse_iterator 		rend()			{ return runs().rend(); }
	const_reverse_iterator 	rend() const	{ return runs().rend(); }

	// Capacity
	bool		empty() const	{ return runs().empty(); }
	size_type	size() const	{ return runs().size(); }
	size_t	span() const;

	// Geometry
	PMPoint     position() const;
	PMPoint     content_dimensions() const;
	PMPoint	    dimensions() const;

	// Element access
	reference		front()			{ return runs().front(); }
	const_reference	front() const	{ return runs().front(); }
	reference		back()			{ return runs().back(); }
	const_reference	back() const	{ return runs().back(); }


	// Modifiers
//	using base_t::push_front;
//	using base_t::runs_t::push_back;
	using base_t::clear;

	bool	fill_by_span(IComposeScanner & scanner, gr_face_cache & faces, TextIndex offset, TextIndex span);

	// Operations
	void	justify();
	void	apply_tab_widths(ICompositionStyle *);
	PMReal	align_text(const IParagraphComposer::RebuildHelper & helper, IJustificationStyle * js, ICompositionStyle *);
	void	break_into(tile &);
	void	update_line_metrics(line_metrics &) const;
};


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


IWaxLine * compose_line(tiler &, gr_face_cache &, IParagraphComposer::RecomposeHelper &, const TextIndex ti);

bool rebuild_line(gr_face_cache & faces, const IParagraphComposer::RebuildHelper &);

} // end of namespace nrsc