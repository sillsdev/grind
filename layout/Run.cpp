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

// Language headers
// Interface headers
#include "VCPlugInHeaders.h"
#include <ICompositionStyle.h>
#include <IDrawingStyle.h>
#include <IFontInstance.h>
#include <IJustificationStyle.h>
#include <IWaxGlyphs.h>
#include <IWaxGlyphsME.h>
#include <IWaxRenderData.h>
#include <IWaxRun.h>
// Library headers
#include <PMRealGlyphPoint.h>
#include <textiterator.h>
// Module header
#include "Run.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;


const textchar kTextChar_EnQuadSpace = 0x2000;

run::run()
: _trailing_ws(end()),
  _glyph_stretch(0),
  _scale(1.0),
  _drawing_style(nil),
  _height(0),
  _span(0)
{
}

run::run(IDrawingStyle * ds)
: _trailing_ws(end()),
  _glyph_stretch(0),
  _scale(1.0),
  _drawing_style(ds),
  _height(ds->GetLeading()),
  _span(0)
{
}


run::~run() throw()
{
}


inline
size_t run::num_glyphs() const
{
	size_t n = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
		n += cl_i->size();

	return n;
}


bool run::fill(TextIterator & ti, TextIndex span)
{
	InterfacePtr<IFontInstance>		font = _drawing_style->QueryFontInstance(kFalse);
	InterfacePtr<ICompositionStyle> comp_style(_drawing_style, UseDefaultIID());
	const PMReal em_space_width = _drawing_style->GetEmSpaceWidth(false);
	if (font == nil || comp_style == nil)
		return false;

	TextIterator start = ti;

	for (; _span != span && !ti.IsNull(); ++ti, ++_span)
	{
		switch((*ti).GetValue())
		{
			case kTextChar_BreakRunInStyle:
				layout_span_with_spacing(start, ti, 0, glyf::space);
				++ti; ++_span;
				return true;
				break;
			case kTextChar_CR:
			case kTextChar_SoftCR:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::space);
				back().break_penalty() = cluster::penalty::mandatory;
				++ti; ++_span;
				return true; 
				break;
			case kTextChar_Tab:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::tab); 
				break;
			case kTextChar_Table:				
			case kTextChar_TableContinued:
			case kTextChar_ObjectReplacementCharacter:	
				layout_span(start, ti - start);
				back().break_penalty() = cluster::penalty::whitespace;
				return true; 
				break;
			case kTextChar_HardSpace:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::space);
				back().break_penalty() = cluster::penalty::never;
				break;
			case kTextChar_FlushSpace:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::fill); 
				break;
			case kTextChar_EnQuadSpace:
			case kTextChar_EnSpace:
				layout_span_with_spacing(start, ti, _drawing_style->GetEnSpaceWidth(false), glyf::fixed);
				break;
			case kTextChar_EmSpace:
				layout_span_with_spacing(start, ti, em_space_width, glyf::fixed);
				break;
			case kTextChar_ThirdSpace:
				layout_span_with_spacing(start, ti, em_space_width/3, glyf::fixed);
				break;
			case kTextChar_QuarterSpace:
				layout_span_with_spacing(start, ti, em_space_width/4, glyf::fixed);
				break;
			case kTextChar_SixthSpace:
				layout_span_with_spacing(start, ti, em_space_width/6, glyf::fixed);
				break;
			case kTextChar_FigureSpace:
				layout_span_with_spacing(start, ti, font->GetGlyphWidth(font->GetGlyphID(kTextChar_Zero)), glyf::fixed);
				break;
			case kTextChar_PunctuationSpace:
				layout_span_with_spacing(start, ti, font->GetGlyphWidth(font->GetGlyphID(kTextChar_Period)), glyf::fixed);
				break;
			case kTextChar_ThinSpace:
				layout_span_with_spacing(start, ti, em_space_width/8, glyf::fixed);
				break;
			case kTextChar_HairSpace:
				layout_span_with_spacing(start, ti, em_space_width/24, glyf::fixed);
				break;
			case kTextChar_NarrowNoBreakSpace:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::fixed);
				back().break_penalty() = cluster::penalty::never;
				break;
			case kTextChar_ZeroSpaceBreak:
				layout_span_with_spacing(start, ti, 0, glyf::fixed);
				break;
			case kTextChar_ZeroWidthNonJoiner:
			case kTextChar_ZeroWidthJoiner:
				layout_span_with_spacing(start, ti, 0, glyf::glyph);
				back().break_penalty() = cluster::penalty::never;
				break;
			case kTextChar_ZeroSpaceNoBreak:
				layout_span_with_spacing(start, ti, 0, glyf::fixed);
				back().break_penalty() = cluster::penalty::never;
				break;
			default: break;
		}
	}

	if (start != ti)
		layout_span(start, ti - start);

	return true;
}


inline
void run::layout_span_with_spacing(TextIterator & first, const TextIterator & last, PMReal width, glyf::justification_t level)
{
	const size_t span = last - first;
	if (span)
		layout_span(first, span);
		
	add_glue(level, width);
	first = last;
	++first;
}


void run::add_glue(glyf::justification_t level, PMReal width, cluster::penalty::type bw)
{
	
	cluster * cl = open_cluster();

	cl->add_glyf(glyf(_drawing_style->GetSpaceGlyph(), level, width));
	cl->add_chars();
	cl->break_penalty() = bw;
}


void run::add_letter(int glyph_id, PMReal width, cluster::penalty::type bw, bool to_cluster)
{
	if (!to_cluster || empty())
		open_cluster();

	cluster & cl = back();
	cl.add_glyf(glyf(glyph_id, to_cluster ? glyf::glyph : glyf::letter, width));
	cl.add_chars();
	cl.break_penalty() = bw;
}


bool run::joinable(const run & rhs) const
{
	return rhs._drawing_style->CanShareWaxRunWith(rhs._drawing_style);
}


run & run::join(run & rhs) 
{
	splice(end(), rhs);
	_span += rhs._span;

	return *this;
}


bool run::render_run(IWaxGlyphs & glyphs) const
{
	const size_t num = num_glyphs();
	float * x_offs = new float[num],
		  * y_offs = new float[num],
		  * widths = new float[num];

	//Assemble glyphs
	float * xo = x_offs,
		  * yo = y_offs,
		  * w  = widths;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
	{
		const cluster & cl = *cl_i;
		for (cluster::const_iterator g = cl.begin(); g != cl.end(); ++g, ++w, ++xo, ++yo)
		{
			*w = 0;
			*xo = ToFloat(_scale*g->pos().X());
			*yo = ToFloat(_scale*g->pos().Y());
			glyphs.AddGlyph(g->id(), ToFloat(_scale*g->advance()));
		}
	}

	// Position shifted glyphs
	InterfacePtr<IWaxGlyphsME> glyphs_me(&glyphs, UseDefaultIID());
	if (glyphs_me != nil)
		glyphs_me->AddGlyphMEData(num, x_offs, y_offs, widths);

	// Do the mappings
	int	i  = 0, 
		gi = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
	{
		const cluster & cl = *cl_i;
		
		glyphs.AddMappingWidth(_scale*cl.width());
		glyphs.AddMappingRange(i++, gi, cl.size());
		for (unsigned char n = cl.span()-1; n; --n, ++i)
		{
			glyphs.AddMappingWidth(0);
			glyphs.AddMappingRange(i, gi, cl.size());
		}
		gi += cl.size();
	}

	delete [] x_offs;
	delete [] y_offs;
	delete [] widths;

	return true;
}


IWaxRun * run::wax_run() const
{
	// Check we've got composed text to put in the run and a line to put it in.
	if (_span <= 0)	return nil;

	// Create a new wax run object.
	InterfacePtr<IWaxRun> wr(static_cast<IWaxRun*>(::CreateObject(kWaxTextRunBoss, IID_IWAXRUN)));
	if (wr == nil)	return nil;

	wr->SetYPosition(_drawing_style->GetEffectiveBaseline());

	// Fill out the render data for the run from the drawing style.
	InterfacePtr<IWaxRenderData> rd(wr, UseDefaultIID());
	InterfacePtr<IWaxGlyphs>     glyphs(wr, UseDefaultIID());
	if (rd == nil || glyphs == nil)	return nil;

	_drawing_style->FillOutRenderData(rd, kFalse/*horizontal*/);
	_drawing_style->AddAdornments(wr);
	wr->AddRef();

	// Apply x skew.
	PMMatrix glyphs_mat;
	PMPoint pc;
	glyphs_mat = glyphs->GetAllGlyphsMatrix(&pc);
	glyphs_mat.SkewTo(_drawing_style->GetSkewAngle());
	glyphs_mat.Scale(_scale*(1+_glyph_stretch), _scale);
	glyphs->SetAllGlyphsMatrix(glyphs_mat, pc);

	render_run(*glyphs);

	return wr;
}


void run::calculate_stretch(const glyf::stretch & js, glyf::stretch & s) const
{
	const PMReal	space_width = _drawing_style->GetSpaceWidth();

	for (const_iterator cl = begin(), cl_e = _trailing_ws; cl != cl_e; ++cl)
		cl->calculate_stretch(space_width, js, s);

	for (const_iterator cl = _trailing_ws, cl_e = end(); cl != cl_e; ++cl)
	{
		for (cluster::const_iterator g = cl->begin(), g_e = cl->end(); g != g_e; ++g)
		{
			if (g->justification() != glyf::fill) continue;

			s[glyf::fill].min += space_width*js[glyf::fill].min;
			s[glyf::fill].max += space_width*js[glyf::fill].max;
			++s[glyf::fill].num;
		}
	}
}


void run::apply_desired_widths()
{
	InterfacePtr<IJustificationStyle>	js(_drawing_style, UseDefaultIID());
	const PMReal	space_width = _drawing_style->GetSpaceWidth();

	adjust_widths(0, js->GetAlteredWordspace()-space_width, js->GetAlteredLetterspace(false), 0);
}


void run::adjust_widths(PMReal fill_space, PMReal word_space, PMReal letter_space, PMReal glyph_scale)
{
	for (iterator cl = begin(), cl_e = _trailing_ws; cl != cl_e; ++cl)
	{
		for (cluster::iterator g = cl->begin(), g_e = cl->end(); g != g_e; ++g)
		{
			g->kern(glyph_scale*g->width());
			g->shift(glyph_scale*g->pos().X());

			switch (g->justification())
			{
			case glyf::fill:
				g->kern(fill_space);
				break;
			case glyf::space:
				g->kern(letter_space + word_space);
				break;
			case glyf::letter:
				g->kern(letter_space);
				break;
			case glyf::glyph:
				g->shift(-letter_space);
				break;
			case glyf::fixed:
				if (g->width() > 0) g->kern(letter_space);
				break;
			default: 
				break;
			}
		}
	}

	for (iterator cl = _trailing_ws, cl_e = end(); cl != cl_e; ++cl)
	{
		for (cluster::iterator g = cl->begin(), g_e = cl->end(); g != g_e; ++g)
		{
			if (g->justification() != glyf::fill) continue;

			g->kern(fill_space);
		}
	}

	_glyph_stretch = glyph_scale;
}


PMReal run::width() const
{
	PMReal advance = 0;

	for (const_iterator cl = begin(), cl_e = end(); cl != _trailing_ws; advance += cl->width(), ++cl);

	return advance;
}


run * run::split(run::const_iterator position) 
{
	// Set up a new run of the same type.
	run * new_run	 = clone_empty();
	new_run->_drawing_style = _drawing_style;
	new_run->_height        = _height;

	// transfer the clusters.
	new_run->splice(new_run->begin(), *this, position, end());

	// Calculate the new_run's span and update this runs span.
	for (const_iterator i=new_run->begin(), e = new_run->end(); i != e; ++i)
		new_run->_span += i->span();
	_span -= new_run->_span;

	_trailing_ws = end();
	return new_run;
}


void run::trim_trailing_whitespace(const PMReal letter_space)
{
	if (_trailing_ws == end())
	{
		// Find the last non-whitespace cluster
		reverse_iterator	cl = rbegin();
		for (const reverse_iterator cl_e = rend(); cl != cl_e; ++cl)
			if (!cl->whitespace()) break;

		if (cl == rend())
		{
			_trailing_ws = begin();
			return;
		}
		_trailing_ws = cl.base();
	}

	// Re-assign any letterspace contributed whitespace in the final
	// glyph to the adjacent trailing whitespace.
	if (_trailing_ws != end())
	{
		iterator non_ws = _trailing_ws;
		--non_ws;
		non_ws->trim(letter_space);
		_trailing_ws->front().kern(letter_space);
	}
}


void run::fit_trailing_whitespace(const PMReal margin)
{
	const size_t ws_count = std::distance(_trailing_ws, end());

	// Shrink the trailing whitespace to fit into the margin space
	const PMReal trailing_ws = margin/ws_count;
	for (iterator cl = _trailing_ws, cl_e = end(); cl != cl_e; ++cl)
		cl->front().kern(std::min(trailing_ws - cl->front().width(), PMReal(0)));
}

