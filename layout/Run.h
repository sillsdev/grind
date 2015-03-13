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

//	void layout_span_with_spacing(TextIterator &, const TextIterator &, PMReal, glyf::justification_t);
	size_t num_glyphs() const;

	base_t::iterator	_trailing_ws;
	PMReal				_glyph_stretch,
						_scale;

protected:
	run();

	void	add_glue(glyf::justification_t level, PMReal width, cluster::penalty::type bw=cluster::penalty::whitespace, cluster::type_t t=cluster::space);
	void	add_letter(int glyph_id, PMReal width, cluster::penalty::type bw=cluster::penalty::letter, bool to_cluster=false);

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
	iterator		trailing_whitespace();
	const_iterator	trailing_whitespace() const;

	// Capacity
	using base_t::empty;
	using base_t::size;
	size_t span() const;

	// Element access
	using base_t::front;
	using base_t::back;

	// Modifiers
	using base_t::clear;
	pointer	open_cluster(cluster::type_t type=cluster::ink);
	run * split(const_iterator position);

	// Operations
	bool			fill(TextIterator & ti, TextIndex span);
	bool			joinable(const run & rhs) const;
	run	&			join(run & rhs);
	
	PMReal			width(const bool include_trailing_whitespace=false) const;
	PMReal			height() const;

	IWaxRun		  * wax_run() const;
	IDrawingStyle * get_style() const;

	void calculate_stretch(const glyf::stretch & js, glyf::stretch & s) const;
	void apply_desired_widths();
	void adjust_widths(PMReal fill_space, PMReal word_space, PMReal letter_space, PMReal glyph_scale);
	void trim_trailing_whitespace(const PMReal letter_space);
	void fit_trailing_whitespace(const PMReal margin);
	void scale(PMReal n);
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
run::pointer run::open_cluster(cluster::type_t type)
{
	push_back(type);
	return &back();
}

inline
run::iterator run::trailing_whitespace()
{
	return _trailing_ws;
}

inline
run::const_iterator run::trailing_whitespace() const
{
	return _trailing_ws;
}

inline
void run::scale(PMReal n)
{
	_scale = n;
}

} // end of namespace nrsc