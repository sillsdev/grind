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
#include <limits>
#include <list>
#include <vector>
// Interface headers
// Library headers
// Module header

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations

namespace nrsc 
{

/* box class
This represents the idea of TeX style box in it's justifaction/line-break 
algorithm, with the addition of a kern attribute to handle InDesign's per 
character kerning concept.
*/

class glyf 
{
public:
	enum justification_t		{fill, space, letter, glyph, fixed, tab};
	typedef struct { PMReal min, max; TextIndex num; }	stretch[5];

private:
	PMReal			_width;
	PMPoint			_pos;
	int				_id;
	justification_t	_just_level;

public:
	glyf(unsigned short id, justification_t level, PMReal width, PMPoint pos=PMPoint(0,0));
	
	unsigned short	id() const throw();
	PMReal			width() const throw();
	PMReal        & width() throw();
	void			shift(const PMPoint & delta) throw();
	void			shift(const PMReal & delta) throw();
	void			kern(const PMReal & delta) throw();
	PMPoint		  & pos() throw();
	const PMPoint & pos() const throw();
	justification_t justification() const throw();
	void			set_glue() throw();
};

inline
glyf::glyf(unsigned short id, justification_t l, PMReal w, PMPoint p)
: _width(w), 
  _pos(p),
  _id(id),
  _just_level(l)
{}

inline
unsigned short glyf::id() const throw() {
	return _id;
}

inline
PMReal glyf::width() const throw() {
	return _width;
}

inline
PMReal & glyf::width() throw() {
	return _width;
}

inline
void glyf::shift(const PMPoint & delta) throw()
{
	_pos += delta;
}

inline
void glyf::shift(const PMReal & delta) throw()
{
	_pos.X() += delta;
}

inline
void glyf::kern(const PMReal & delta) throw()
{
	_width += delta;
	shift(delta);
}

inline
PMPoint & glyf::pos() throw()
{
	return _pos;
}

inline
const PMPoint & glyf::pos() const throw()
{
	return _pos;
}

inline
glyf::justification_t glyf::justification() const throw()
{
	return _just_level;
}

inline
void glyf::set_glue() throw()
{
	_just_level = fixed;
}



class cluster : private std::vector<glyf>
{
private:
	typedef std::vector<glyf>	base_t;

	unsigned char	_span;
	float				_penalty;

public:
	// Member types
	typedef base_t::iterator				iterator;
	typedef base_t::reverse_iterator		reverse_iterator;
	typedef base_t::value_type				value_type;
	typedef base_t::pointer					pointer;
	typedef base_t::reference				reference;
	typedef base_t::const_iterator			const_iterator;
	typedef base_t::const_reverse_iterator	const_reverse_iterator;
	typedef base_t::const_pointer			const_pointer;
	typedef base_t::const_reference			const_reference;

	struct penalty { 
		typedef float	type;
		static const type	mandatory,
							whitespace,
							word,
							intra,
							letter,
							clip,
							never;
	};

	// Constructor
	cluster();

	//Iterators
	using base_t::begin;
	using base_t::end;
	using base_t::rbegin;
	using base_t::rend;

	// Capacity
	using base_t::size;
	size_t	span() const;

	// Element access
	using base_t::front;
	using base_t::back;

	// Modifiers
	void	add_glyf(const glyf &);
	void	add_chars(TextIndex n=1);

	// Operations
	void trim(const PMReal ws);

	// Properties
	PMReal			width() const;
	penalty::type	break_penalty() const;
	penalty::type &	break_penalty();
	bool			whitespace() const;
	void			calculate_stretch(const PMReal & space_width, const glyf::stretch & js, glyf::stretch & s) const;
};

inline
cluster::cluster()
: _span(0),
  _penalty(cluster::penalty::clip)
{
	reserve(1);
}

inline
void cluster::add_chars(TextIndex n)
{
	_span += n;
}

inline 
size_t cluster::span() const
{
	return _span;
}

inline 
cluster::penalty::type cluster::break_penalty() const
{
	return _penalty;
}

inline 
cluster::penalty::type & cluster::break_penalty()
{
	return _penalty;
}

inline
bool cluster::whitespace() const
{
	if (size() == 1)
	{
		switch(front().justification())
		{
		case glyf::fill:
		case glyf::space:
		case glyf::tab:		return true;
		case glyf::fixed:	return _penalty == penalty::whitespace;
		}
	}
	return false;
}

} // end of namespace nrsc