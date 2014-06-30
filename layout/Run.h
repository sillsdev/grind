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
#include "ClusterThread.h"

// Forward declarations
class TextIterator;
// InDesign interfaces
class IDrawingStyle;
class IWaxGlyphs;
class IWaxRun;
// Graphite forward delcarations

namespace nrsc 
{


class run : public cluster_thread::run
{
	typedef std::list<cluster>	base_t;

	// Hide copy constructor and assignment operator.
	//run(const run&);
	//run & operator = (const run &);

	void layout_span_with_spacing(cluster_thread & thread, TextIterator &, const TextIterator &, PMReal, glyf::justification_t);
	//size_t num_glyphs() const;

	//base_t::iterator	_trailing_ws;
	//PMReal				_extra_scale;

protected:
	void	add_glue(cluster_thread & thread, glyf::justification_t level, PMReal width, cluster::penalty::type bw=cluster::penalty::whitespace);
	void	add_letter(cluster_thread & thread, int glyph_id, PMReal width, cluster::penalty::type bw=cluster::penalty::letter, bool to_cluster=false);

	run(IDrawingStyle * ds);
	run(IDrawingStyle * ds, const cluster_thread::run::ops &);

	virtual bool	layout_span(cluster_thread & thread, TextIterator first, size_t span) = 0;


	//InterfacePtr<IDrawingStyle>	_drawing_style;
	//PMReal			_height;
	TextIndex		_span;

public:
	virtual ~run() throw();

	//// Member types
	//using base_t::const_iterator;
	//using base_t::iterator;
	//using base_t::difference_type;
	//using base_t::size_type;
	//using base_t::const_reference;
	//using base_t::reference;

	////Iterators
	//using base_t::begin;
	//using base_t::end;
	//iterator		trailing_whitespace();
	//const_iterator	trailing_whitespace() const;

	//// Capacity
	//using base_t::empty;
	//using base_t::size;
	//size_t span() const;

	//// Element access
	//using base_t::front;
	//using base_t::back;

	//// Modifiers
	//using base_t::clear;
	//pointer	open_cluster();
	//run * split(const_iterator position);

	//// Operations
	bool			fill(cluster_thread & thread, TextIterator & ti, TextIndex span);
	//bool			joinable(const run & rhs) const;
	//run	&			join(run & rhs);
	//
	//PMReal			width() const;
	//PMReal			height() const;

	//IWaxRun		  * wax_run() const;
	//IDrawingStyle * get_style() const;

	//void get_stretch_ratios(glyf::stretch &) const;
	//void calculate_stretch(const glyf::stretch & js, glyf::stretch & s) const;
	//void apply_desired_widths();
	//void adjust_widths(PMReal fill_space, PMReal word_space, PMReal letter_space, PMReal glyph_scale);
	//void trim_trailing_whitespace(const PMReal letter_space);
	//void fit_trailing_whitespace(const PMReal margin);

};

inline
run::run(IDrawingStyle * ds)
: cluster_thread::run(ds, ds->GetLeading(),base_t::iterator(), base_t::iterator()),
  _span(0)
{
}

inline
run::run(IDrawingStyle * ds, const cluster_thread::run::ops & rops)
: cluster_thread::run(ds, ds->GetLeading(),base_t::iterator(), base_t::iterator(), rops),
  _span(0)
{
}

//inline
//size_t run::span() const
//{
//	return _span;
//
//}
//
//inline
//PMReal run::height() const
//{
//	return _height;
//}
//
//inline
//IDrawingStyle  * run::get_style() const
//{
//	return _drawing_style;
//}
//
//inline
//run::pointer run::open_cluster()
//{
//	resize(size()+1);
//	return &back();
//}
//
//inline
//run::iterator run::trailing_whitespace()
//{
//	return _trailing_ws;
//}
//
//inline
//run::const_iterator run::trailing_whitespace() const
//{
//	return _trailing_ws;
//}

} // end of namespace nrsc