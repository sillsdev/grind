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
	enum justification_t		{fill, space, letter, glyph, fixed};

private:
	PMReal			_width,
					_height,
					_depth;
	PMPoint			_pos;
	int				_id;
	justification_t	_just_level;

public:
	glyf(unsigned short id, justification_t level, PMReal width, PMReal height, PMReal depth, PMPoint pos=PMPoint(0,0));
	
	unsigned short	id() const throw();
	PMReal			width() const throw();
	PMReal			height() const throw();
	PMReal			depth() const throw();
	PMReal			advance() const throw();
	void			shift(const PMPoint & delta) throw();
	void			shift(const PMReal & delta) throw();
	void			kern(const PMReal & width) throw();
	PMPoint		  & pos() throw();
	const PMPoint & pos() const throw();
	justification_t justification() const throw();
	void			set_glue() throw();
};

inline
glyf::glyf(unsigned short id, justification_t l, PMReal w, PMReal h, PMReal d, PMPoint p)
: _width(w), 
  _height(h), 
  _depth(d),
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
PMReal glyf::height() const throw() {
	return _height;
}

inline
PMReal glyf::depth() const throw() {
	return _depth;
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

	PMReal			_height,
					_depth;
	unsigned char	_span;
	int				_breakweight;

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

	struct breakweight { enum type { 
		mandatory	= 0,
		whitespace	= 10,
		word		= 15,
		intra		= 20,
		letter		= 30,
		clip		= 40,
		never		= USHRT_MAX
	}; };

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
	PMReal			height() const;
	PMReal			depth() const;
	int				break_weight() const;
	int &			break_weight();
};

inline
cluster::cluster()
: _height(0),
  _depth(0),
  _span(0),
  _breakweight(cluster::breakweight::clip)
{
	reserve(1);
}

inline
PMReal cluster::height() const {
	return _height;
}

inline
PMReal cluster::depth() const {
	return _depth;
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
int cluster::break_weight() const
{
	return _breakweight;
}

inline 
int & cluster::break_weight()
{
	return _breakweight;
}


} // end of namespace nrsc