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
class box
{
protected:
	PMReal			_width,
					_height,
					_depth;
	box(PMReal width, PMReal height, PMReal depth);

public:
	virtual ~box() throw();

	PMReal	width() const throw();
	PMReal	height() const throw();
	PMReal	depth() const throw();
};


inline
box::box(PMReal w, PMReal h, PMReal d)
: _width(w),
  _height(h),
  _depth(d)
{}

inline
PMReal box::width() const throw() {
	return _width;
}

inline
PMReal box::height() const throw() {
	return _height;
}

inline
PMReal box::depth() const throw() {
	return _depth;
}


class glyf : public box
{
public:
	enum justification_t		{fixed, glyph, letter, space, fill};

private:
	PMPoint					_pos;
	int						_id;
	PMReal					_kern;
	justification_t			_just_level;

public:
	glyf(unsigned short id, justification_t level, PMReal width, PMReal height, PMReal depth, PMPoint pos=PMPoint(0,0));
	
	unsigned short	id() const throw();
	PMReal			kern() const throw();
	PMReal		  & kern() throw();
	PMReal			advance() const throw();
	PMPoint		  & pos() throw();
	const PMPoint & pos() const throw();
	justification_t justification() const throw();
	void			set_glue() throw();
};

inline
glyf::glyf(unsigned short id, justification_t l, PMReal w, PMReal h, PMReal d, PMPoint p)
: box(w, h, d),
  _pos(p),
  _id(id),
  _kern(0.0),
  _just_level(l)
{}

inline
unsigned short glyf::id() const throw() {
	return _id;
}

inline
PMReal glyf::kern() const throw() {
	return _kern;
}

inline
PMReal & glyf::kern() throw() {
	return _kern;
}

inline
PMReal glyf::advance() const throw() {
	return _width + _kern;
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

class cluster : public box
{
private:
	typedef std::vector<glyf>	components_t;
	components_t	_components;
	unsigned char	_span;
	int				_breakweight;

public:
	struct breakweight { enum type { 
		mandatory	= 0,
		whitespace	= 10,
		word		= 15,
		intra		= 20,
		letter		= 30,
		clip		= 40,
		never		= USHRT_MAX
	}; };

	typedef components_t::iterator		iterator;
	typedef components_t::value_type	value_type;
	typedef components_t::pointer		pointer;
	typedef components_t::reference		reference;
	typedef components_t::const_iterator	const_iterator;
	typedef components_t::const_pointer		const_pointer;
	typedef components_t::const_reference	const_reference;

	cluster();

	PMReal			width() const throw();

	void			add_glyf(const glyf &);
	void			add_chars(TextIndex n=1);
	iterator		begin();
	const_iterator	begin() const;
	iterator		end();
	const_iterator	end() const;

	reference		front();
	const_reference	front() const;
	reference		back();
	const_reference back() const;

	size_t			size() const;
	size_t			span() const;
	int				break_weight() const;
	int &			break_weight();
};

inline
cluster::cluster()
: box(0,0,0),
  _span(0),
  _breakweight(cluster::breakweight::clip)
{
	_components.reserve(2);
}

inline
void cluster::add_chars(TextIndex n)
{
	_span += n;
}

inline
cluster::iterator cluster::begin()
{
	return _components.begin();
}

inline
cluster::const_iterator cluster::begin() const
{
	return _components.begin();
}

inline 
cluster::iterator cluster::end()
{
	return _components.end();
}

inline 
cluster::const_iterator cluster::end() const
{
	return _components.end();
}

inline
cluster::reference cluster::front()
{
	return _components.front();
}

inline
cluster::const_reference cluster::front() const
{
	return _components.front();
}

inline
cluster::reference cluster::back()
{
	return _components.back();
}

inline
cluster::const_reference cluster::back() const
{
	return _components.back();
}

inline 
size_t cluster::size() const
{
	return _components.size();
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