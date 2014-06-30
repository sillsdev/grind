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
#include <iterator>
#include <list>
// Interface headers
#include <IDrawingStyle.h>
// Library headers
// Module header
#include "Box.h"

// Forward declarations
// InDesign interfaces
class IWaxGlyphs;
class IWaxRun;
// Graphite forward delcarations
// Project forward declarations

namespace nrsc
{

/*	cluster_thread
	This class provides a cluster centric model of the paragraph or line, 
	representing runs and spans pointing into the contiguous cluster list.
*/
class cluster_thread : private std::list<cluster>
{
	typedef std::list<cluster>	base_t;

	template <typename C, typename R> class _iterator;
	class _span;

public:
	class run;
	typedef	std::list<run>	runs_t;

	// Member types
	typedef _iterator<base_t::iterator, runs_t::iterator>				iterator;
	typedef _iterator<base_t::const_iterator, runs_t::const_iterator>	const_iterator;
	typedef std::reverse_iterator<iterator>								reverse_iterator;
	typedef std::reverse_iterator<const_iterator>						const_reverse_iterator;
	using base_t::difference_type;
	using base_t::size_type;
	using base_t::const_reference;
	using base_t::reference;

	//Iterators
	iterator 		begin();
	const_iterator	begin() const;
	iterator 		end();
	const_iterator	end() const;
	reverse_iterator 		rbegin();
	const_reverse_iterator	rbegin() const;
	reverse_iterator 		rend();
	const_reverse_iterator	rend() const;
	
	// Capacity
	using base_t::empty;
	using base_t::size;

	// Element access
	using base_t::front;
	using base_t::back;

	// Modifiers
	cluster_thread	split(iterator const &);
	void			fit_trailing_whitespace(const PMReal margin);

	// Properties
	PMPoint			dimensions() const;
	iterator		trailing_whitespace();
	const_iterator	trailing_whitespace() const;
	const runs_t &	runs() const;

	// Operations
	void get_stretch_ratios(glyf::stretch & s) const;
	void			trim_trailing_whitespace(const PMReal letter_space);
	void			merge_runs();
	void			push_run(run & r);
	void			push_back(const value_type & val);

private:
	runs_t				_runs;
	runs_t::iterator	_trailing_ws_run;
};



/*	cluster_thread::_iterator
	This nothing more than an iterator or the cluster list which also keeps 
	track of the current clusters run.
*/
template<typename C, typename R>
class cluster_thread::_iterator : public std::iterator<typename std::iterator_traits<C>::iterator_category,
													   typename std::iterator_traits<C>::value_type>
{
	typedef std::iterator<typename std::iterator_traits<C>::iterator_category, typename std::iterator_traits<C>::value_type> base_t;
	C _cl;
	R _r;
	
public:
	_iterator(C cl, R r) : _cl(cl), _r(r) {}

	bool	operator==(const _iterator<C,R> & rhs) const	{ return _cl.operator==(rhs._cl); }
	bool	operator!=(const _iterator<C,R> & rhs) const	{ return _cl.operator!=(rhs._cl); }

	reference	operator*() const 		{ return _cl.operator*(); }
	pointer 	operator->() const 		{ return _cl.operator->(); }

	_iterator<C,R> & 	operator++() 		{ if (++_cl == _r->end()) ++_r; return *this; }
	_iterator<C,R> & 	operator++(int) 	{ _iterator<C,R> tmp = *this; operator++(); return tmp; }

	_iterator<C,R> & 	operator--()		{ if (_cl == _r->begin()) --_r; --_cl; return *this; }
	_iterator<C,R> & 	operator--(int)		{ _iterator<C,R> tmp = *this; operator--(); return tmp; }

	typename R::reference	run() const			{ return *_r; }

	operator C () const { return _cl; }

	friend class cluster_thread;
};



/*	cluster_thread::_span
	A span is simply a sub sequence of the cluster list which masquerades as a
	container.
*/
class cluster_thread::_span
{
	base_t::iterator _b, _e;
	base_t::size_type _s;

	virtual void split_into(base_t::iterator const &, _span &);

	friend cluster_thread;
public:
	_span(base_t::iterator f, base_t::iterator l) : _b(f), _e(l), _s(std::distance(_b,_e)) {}

	// Member types
	typedef base_t::iterator				iterator;
	typedef base_t::const_iterator			const_iterator;
	typedef base_t::reverse_iterator		reverse_iterator;
	typedef base_t::const_reverse_iterator	const_reverse_iterator;
	typedef base_t::difference_type			difference_type;
	typedef base_t::size_type				size_type;
	typedef base_t::const_reference			const_reference;
	typedef base_t::reference				reference;

	// Iterators
	iterator 		begin()					{ return _b; }
	const_iterator	begin() const			{ return _b; }
	iterator 		end()					{ return _e; }
	const_iterator	end() const				{ return _e; }
	reverse_iterator 		rbegin()		{ return reverse_iterator(_e); }
	const_reverse_iterator	rbegin() const	{ return reverse_iterator(_e); }
	reverse_iterator 		rend()			{ return reverse_iterator(_b); }
	const_reverse_iterator	rend() const	{ return reverse_iterator(_b); }

	// Capacity
	bool		empty() const		{ return _s == 0; }
	size_type	size() const		{ return _s; }
	size_type	span() const;
	size_type	num_glyphs() const;


	// Element access
	reference front()				{ return *_b; }
	const_reference front() const	{ return *_b; }
	reference back()				{ iterator t = _e; --t; return *t; }
	const_reference back() const	{ const_iterator t = _e; --t; return *t; }
};



/*	cluster_thread::run
	Ultimately represents an InDesign WaxRun object, this is a span which 
	knows it's clusters IDrawingStyle, scaling info and other geometry.
*/
class cluster_thread::run : public cluster_thread::_span
{
	InterfacePtr<IDrawingStyle>	_ds;
	PMReal						_extra_scale,
								_h;

protected:
	struct ops
	{
		virtual bool			render(cluster_thread::run const & run, IWaxGlyphs & glyphs) const;
		virtual void			destroy();
		virtual const ops *		clone() const;
	};
	const ops * _ops;


	run(IDrawingStyle * const ds, PMReal height, base_t::iterator f, base_t::iterator l, const ops &);
	run(IDrawingStyle * const ds, PMReal height, base_t::iterator f, base_t::iterator l);

	PMReal & height();

public:
	run(const run &);
	virtual ~ run();

	// Operations
	bool			joinable(run const & rhs) const;
	run	&			join(run & rhs);
	
	PMReal			width() const;
	PMReal			height() const;

	IWaxRun		  * wax_run() const;
	IDrawingStyle * get_style() const;

	void get_stretch_ratios(glyf::stretch &) const;
	void calculate_stretch(glyf::stretch const & js, glyf::stretch & s) const;
	void apply_desired_widths();
	void adjust_widths(PMReal fill_space, PMReal word_space, PMReal letter_space, PMReal glyph_scale);
private:
	static ops _default_ops;
};



/* Inline functions for cluster_thread
*/
inline
cluster_thread::iterator cluster_thread::begin()
{
	return iterator(base_t::begin(), _runs.begin());
}

inline
cluster_thread::const_iterator cluster_thread::begin() const
{
	return const_iterator(base_t::begin(), _runs.begin());
}

inline
cluster_thread::iterator cluster_thread::end()
{
	return iterator(base_t::end(), _runs.end());
}

inline
cluster_thread::const_iterator cluster_thread::end() const
{
	return const_iterator(base_t::end(), _runs.end());
}

inline
cluster_thread::reverse_iterator cluster_thread::rbegin()
{
	return reverse_iterator(end());
}

inline
cluster_thread::const_reverse_iterator cluster_thread::rbegin() const
{
	return const_reverse_iterator(end());
}

inline
cluster_thread::reverse_iterator cluster_thread::rend()
{
	return reverse_iterator(begin());
}

inline
cluster_thread::const_reverse_iterator cluster_thread::rend() const
{
	return const_reverse_iterator(begin());
}

inline
cluster_thread::iterator cluster_thread::trailing_whitespace()
{
	return iterator(_trailing_ws_run->begin(), _trailing_ws_run);
}

inline
cluster_thread::const_iterator cluster_thread::trailing_whitespace() const
{
	return const_iterator(_trailing_ws_run->begin(), _trailing_ws_run);
}

inline
void cluster_thread::push_run(run & r)
{
	r._b = _runs.back().end();
	r._e = base_t::end();
	r._s = std::distance(r._b,r._e);

	_runs.push_back(r);
}



/* Inline functions for cluster_thread::run
*/
inline
cluster_thread::run::run(IDrawingStyle * const ds, PMReal height, base_t::iterator f, base_t::iterator l, const ops & rops) 
: _span(f,l), _ds(ds), _h(height), _ops(&rops) 
{
}

inline
cluster_thread::run::run(IDrawingStyle * const ds, PMReal height, base_t::iterator f, base_t::iterator l) 
: _span(f,l), _ds(ds), _h(height), _ops(&_default_ops) 
{
}

inline
cluster_thread::run::run(const run & rhs) 
: _span(rhs), _ds(rhs._ds), _h(rhs._h), _ops(rhs._ops->clone()) 
{
}

inline
bool cluster_thread::run::joinable(const run & rhs) const
{
	return rhs._ds->CanShareWaxRunWith(rhs._ds);
}

inline
cluster_thread::run & cluster_thread::run::join(cluster_thread::run & rhs) 
{
	_e = rhs._e; _s += rhs._s;
	return *this;
}

inline
PMReal cluster_thread::run::width() const
{
	PMReal advance = 0;
	for (const_iterator cl = begin(), cl_e = end(); cl != end(); advance += cl->width(), ++cl);
	return advance;
}

inline
PMReal cluster_thread::run::height() const
{
	return _h;
}

inline
PMReal & cluster_thread::run::height()
{
	return _h;
}

inline
IDrawingStyle  * cluster_thread::run::get_style() const
{
	return _ds;
}

} // End of namespace nrsc