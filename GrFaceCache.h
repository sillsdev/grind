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
#include <list>
// Interface headers
// Library headers
#include <PMString.h>
// Module header

// Forward declarations
struct gr_face;
class IPMFont;

/** A cache of Graphite2 gr_face objects keyed to file path. Lifetime is 
	recorded counting accesses and an entry's dwell time is reset if it is 
	accessed and an entry is evicted if it is the oldest and the cache is at 
	capacity or if it's dwell time is greater than the maximum allowed.
*/
class GrFaceCache
{
public:
	typedef IPMFont *		key_t;
	typedef gr_face *		value_t;

	/** Create a gr_face object cache.
		@param capacity IN The maximum number of gr_face objects that should be 
			cached at any time. A value of 0 signifies unlimited capacity
		@param max_dwell IN The maimum dwell time. A value of 0 signifies 
			unlimited time.
		
	*/
    GrFaceCache(size_t capacity, unsigned int max_dwell=0);
	~GrFaceCache(void);

	bool empty() const 
	{
		return _faces.empty();
	}

	size_t size() const
	{
		return _faces.size();
	}

	size_t capacity() const
	{
		return _capacity;
	}

	value_t	operator [] (const key_t k);


private:
	struct entry
	{
		PMString key;
		value_t face;

		entry(const PMString &, const value_t=0) throw();

		bool operator == (const entry & rhs) const throw();
	};

	typedef std::list<entry>	store_t;

	void					destroy_entry(const entry & e);
	void					spring_clean();
	void					freshen(const store_t::iterator & i);

	gr_face *				face_from_platform_font(const PMString &);
	store_t					_faces;

	const size_t			_capacity;
	const unsigned int		_max_dwell;
};
