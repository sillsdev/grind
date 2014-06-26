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
// Library headers
// Module header
#include "Box.h"

// Forward declarations
// InDesign interfaces
class IDrawingStyle;
// Graphite forward delcarations
// Project forward declarations

namespace nrsc
{

class cluster_thread : private std::list<cluster>
{
	typedef std::list<cluster>	base_t;

	template <typename C, typename R> class _iterator;
	
	class run
	{
		base_t::iterator _b, _e;
		base_t::size_type _s;

		// Modifiers
		run split(base_t::iterator const &);

	public:
		run(base_t::iterator f, base_t::iterator l) : _b(f), _e(l), _s(std::distance(_b,_e)) {}

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

		// Element access
		reference front()				{ return *_b; }
		const_reference front() const	{ return *_b; }
		reference back()				{ iterator t = _e; --t; return *t; }
		const_reference back() const	{ const_iterator t = _e; --t; return *t; }

		friend cluster_thread;
	};


public:
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
	cluster_thread split(iterator const &);

private:
	runs_t		_runs;
};


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

	_iterator<C,R> & 	operator++() 		{ if (++_cl == r->end()) ++_r; return *this; }
	_iterator<C,R> & 	operator++(int) 	{ _iterator<C,R> tmp = *this; operator++(); return tmp; }

	_iterator<C,R> & 	operator--()		{ if (_cl == r->begin()) --_r; --_cl; return *this; }
	_iterator<C,R> & 	operator--(int)		{ _iterator<C,R> tmp = *this; operator--(); return tmp; }

	typename R::reference	run() const			{ return *_r; }

	friend cluster_thread cluster_thread::split(cluster_thread::iterator const &);
};


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

} // End of namespace nrsc