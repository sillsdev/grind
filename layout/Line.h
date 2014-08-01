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
// Module header
#include "Tile.h"

// Forward declarations
// InDesign interfaces
class IWaxLine;
// Graphite forward delcarations

namespace nrsc 
{
// Project forward declarations
class gr_face_cache;
struct line_metrics;
class tiler;

class line : private std::list<tile>
{
	typedef std::list<tile>	base_t;
	size_t	_span;
public:
	line();
	~line() throw();
	
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
	size_t	span() const throw();

	// Element access
	using base_t::front;
	using base_t::back;

	// Modifiers
	using base_t::push_back;
	using base_t::pop_back;
	using base_t::erase;
	void clear();

	// Operations
	void	update_line_metrics(line_metrics & lm);
	void	fill_wax_line(IWaxLine &) const;
};



IWaxLine *	compose_line(tiler &, gr_face_cache &, IParagraphComposer::RecomposeHelper &, const TextIndex ti);
bool		rebuild_line(gr_face_cache & faces, const IParagraphComposer::RebuildHelper &);


inline
line::line()
: _span(0)
{
}

inline
line::~line() throw() 
{ 
	clear(); 
}

inline
size_t line::span() const throw()
{
	return _span;
}

inline
void line::clear()
{
	base_t::clear();
	_span = 0;
}

} // end of namespace nrsc