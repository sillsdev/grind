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
#include <functional>
#include <list>
#include <vector>
// Interface headers
// Library headers
// Module header
#include "Box.h"

// Forward declarations
class TextIterator;
// InDesign interfaces
class IDrawingStyle;
class IWaxGlyphs;
class IWaxRun;
// Graphite forward delcarations

namespace nrsc 
{


class run : protected std::list<cluster>
{
	typedef std::list<cluster>	base_t;

	// Hide copy constructor and assignment operator.
	run(const run&);
	run & operator = (const run &);

	void run::layout_span_with_spacing(TextIterator &, const TextIterator &, PMReal, glyf::justification_t);
	size_t num_glyphs() const;

public:
	typedef struct { PMReal min, max; }	stretch[5];

protected:
	void	add_glue(glyf::justification_t level, PMReal width, cluster::breakweight::type bw=cluster::breakweight::whitespace);
	void	add_letter(int glyph_id, PMReal width, cluster::breakweight::type bw=cluster::breakweight::letter, bool to_cluster=false);

	run(IDrawingStyle *);

	virtual bool	layout_span(TextIterator first, size_t span) = 0;
	virtual bool	render_run(IWaxGlyphs & run) const;
	virtual run	  * clone_empty() const = 0;


	InterfacePtr<IDrawingStyle>	_drawing_style;
	PMReal			_height;
	TextIndex		_span;

public:
	virtual ~run() throw();

	// Member types
	using base_t::const_iterator;
	using base_t::iterator;
	using base_t::difference_type;
	using base_t::size_type;
	using base_t::const_reference;
	using base_t::reference;

	//Iterators
	using base_t::begin;
	using base_t::end;

	// Capacity
	using base_t::empty;
	using base_t::size;
	size_t span() const;

	// Element access
	using base_t::front;
	using base_t::back;

	// Modifiers
	using base_t::clear;
	reference	open_cluster();
	void		close_cluster(const_reference );
	run * split(const_iterator position);

	// Operations
	bool			fill(TextIterator & ti, TextIndex span);
	bool			joinable(const run & rhs) const;
	run	&			join(run & rhs);
	const_iterator	find_break(PMReal desired) const;
	
	PMReal			width() const;
	PMReal			height() const;

	IWaxRun		  * wax_run() const;
	IDrawingStyle * get_style() const;

	void get_stretch_ratios(stretch &) const;
	void calculate_stretch(stretch & s) const;
};


inline
size_t run::span() const
{
	return _span;

}

inline
PMReal run::height() const
{
	return _height;
}

inline
IDrawingStyle  * run::get_style() const
{
	return _drawing_style;
}

inline
run::reference run::open_cluster()
{
	resize(size()+1);
	return back();
}

inline
void run::close_cluster(run::const_reference cl)
{
	_height = std::max(cl.height() + cl.depth(), _height);
}


} // end of namespace nrsc