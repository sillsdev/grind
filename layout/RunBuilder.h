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
#include "Box.h"

// Forward declarations
class TextIterator;
// InDesign interfaces
class IDrawingStyle;
// Graphite forward delcarations



namespace nrsc
{
// Project forward declarations
class cluster_thread;



class run_builder : public cluster_thread::run
{
	void	layout_span_with_spacing(TextIterator &, const TextIterator &, PMReal, glyf::justification_t);

protected:
	IDrawingStyle  *	_drawing_style;
	cluster_thread *	_thread;
	TextIndex			_span;

	run_builder();
	
	virtual bool	layout_span(TextIterator first, size_t span) = 0;
	void			add_glue(glyf::justification_t level, PMReal width, cluster::penalty::type bw=cluster::penalty::whitespace);
	void			add_letter(int glyph_id, PMReal width, cluster::penalty::type bw=cluster::penalty::letter, bool to_cluster=false);


public:
	virtual			~run_builder();
	bool			build_run(cluster_thread & thread, IDrawingStyle * ds, TextIterator & ti, TextIndex span);
};



inline 
run_builder::run_builder()
: _drawing_style(0), _thread(0), _span(0) 
{
}

} // End of namespace nrsc