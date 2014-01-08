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
// Library headers
// Module header
#include "Run.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations

namespace nrsc 
{
// Project forward declarations

class inline_object : public run
{
	UIDRef	_inline_UID_ref;

	// Prevent automatic copy-ctor and assignment generation
	inline_object(const inline_object &);
	inline_object & operator = (const inline_object &);

	virtual bool  layout_span(TextIterator first, size_t span);
	virtual bool  render_run(IWaxRun & run) const;
	virtual run * clone_empty() const;

public:
	inline_object(IDrawingStyle * ds);
};


inline
inline_object::inline_object(IDrawingStyle * ds)
: run(ds),
  _inline_UID_ref(nil, kInvalidUID)
{
}

} // end of namespace nrsc