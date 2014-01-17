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


run::run(IDrawingStyle * ds)
: _drawing_style(ds),
  _height(ds->GetLeading()),
  _span(0)
{
}

inline
size_t run::num_glyphs() const
{
	size_t n = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
		n += (*cl_i)->size();

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
			case kTextChar_CR:
			case kTextChar_SoftCR:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::fill);
				back()->break_weight() = cluster::breakweight::mandatory;
				++ti; ++_span;
				return true; 
				break;
			case kTextChar_Tab:
				layout_span(start, ti - start);
				return true; 
				break;
			case kTextChar_Table:				
			case kTextChar_TableContinued:
			case kTextChar_ObjectReplacementCharacter:	
				layout_span(start, ti - start);
				back()->break_weight() = cluster::breakweight::whitespace;
				return true; 
				break;
			case kTextChar_FlushSpace:
				layout_span_with_spacing(start, ti, _drawing_style->GetSpaceWidth(), glyf::fill); 
				break;
			case kTextChar_EnQuadSpace:
			case kTextChar_EnSpace:
				layout_span_with_spacing(start, ti, _drawing_style->GetEnSpaceWidth(false), glyf::space);
				break;
			case kTextChar_EmSpace:
				layout_span_with_spacing(start, ti, em_space_width, glyf::space);
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
				layout_span_with_spacing(start, ti, font->GetGlyphWidth(font->GetGlyphID(kTextChar_Comma)), glyf::fixed);
				break;
			case kTextChar_ThinSpace:
				layout_span_with_spacing(start, ti, em_space_width/8, glyf::fixed);
				break;
			case kTextChar_HairSpace:
				layout_span_with_spacing(start, ti, em_space_width/24, glyf::space);
				break;
			case kTextChar_ZeroSpaceBreak:
				layout_span_with_spacing(start, ti, 0, glyf::fixed);
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
	layout_span(first, last - first);
	add_glue(level, width);
	first++ = last;
}


run::~run() throw()
{
	for (iterator i = begin(), i_e = end(); i != i_e; ++i)
		delete *i;
}



void run::add_glue(glyf::justification_t level, PMReal width, cluster::breakweight::type bw)
{
	cluster * cl = new cluster();

	cl->add_glyf(glyf(_drawing_style->GetSpaceGlyph(), level, width, _height, 0));
	cl->add_chars();
	cl->break_weight() = bw;
	push_back(cl);
}


void run::add_letter(int glyph_id, PMReal width, cluster::breakweight::type bw, bool to_cluster)
{
	cluster * cl = 0;
	if (to_cluster && !empty())
		cl = back();
	else
	{
		cl = new cluster();
		push_back(cl);
	}

	cl->add_glyf(glyf(glyph_id, glyf::letter, width, _height, 0));
	cl->add_chars();
	cl->break_weight() = bw;
}


void run::push_front(value_type b)
{
	_height = std::max(b->height() + b->depth(), _height);
	base_t::push_front(b);
}

void run::push_back(value_type b)
{
	_height = std::max(b->height() + b->depth(), _height);
	base_t::push_back(b);
}

bool run::joinable(const run & rhs) const
{
	return rhs._drawing_style->CanShareWaxRunWith(rhs._drawing_style);
}

run & run::join(run & rhs) 
{
	splice(end(), rhs);

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
		cluster & cl = **cl_i;
		for (cluster::const_iterator c = cl.begin(); c != cl.end(); ++c, ++w, ++xo, ++yo)
		{
			*w = 0;
			*xo = ToFloat(c->pos().X());
			*yo = ToFloat(c->pos().Y());
			glyphs.AddGlyph(c->id(), ToFloat(c->advance()));
		}
	}

	// Position diacritics
	InterfacePtr<IWaxGlyphsME> glyphs_me(&glyphs, IID_IWAXGLYPHSME);
	if (glyphs_me != nil)
		glyphs_me->AddGlyphMEData(num, x_offs, y_offs, widths);

	// Do the mappings
	int	i  = 0, 
		gi = 0;
	for (const_iterator cl_i = begin(); cl_i != end(); ++cl_i)
	{
		const cluster & cl = **cl_i;
		const PMReal	cluster_advance = cl.width() + cl.back().kern();
		
		glyphs.AddMappingWidth(cluster_advance);
		glyphs.AddMappingRange(i++, gi, cl.size());
		for (unsigned char n = cl.span()-1; n; --n, ++i)
		{
			glyphs.AddMappingWidth(0);
			glyphs.AddMappingRange(i, gi, cl.size());
		}
		gi += cl.size();
	}

	delete x_offs;
	delete y_offs;
	delete widths;

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

	render_run(*glyphs);

	return wr;
}


void run::get_stretch_ratios(stretch & s) const
{
	PMReal min,des,max;
	InterfacePtr<IJustificationStyle>	js(_drawing_style, UseDefaultIID());

	s[glyf::fixed].min = 0;
	s[glyf::fixed].max = 0;

	js->GetGlyphscale(&min, &des, &max);
	s[glyf::glyph].min = 1-min;
	s[glyf::glyph].max = max-1;

	js->GetLetterspace(&min, &des, &max);
	s[glyf::letter].min = 1-min;
	s[glyf::letter].max = max-1;

	js->GetWordspace(&min, &des, &max);
	s[glyf::space].min = 1-min;
	s[glyf::space].max = max-1;
	s[glyf::fill].min = 1-min;
	s[glyf::fill].max = 1000000.0;
}


void run::calculate_stretch(stretch & s) const
{
	stretch js;
	get_stretch_ratios(js);

	for (const_iterator cl = begin(), cl_e = end(); cl != cl_e; ++cl)
	{
		for (cluster::const_iterator g = (*cl)->begin(), g_e = (*cl)->end(); g != g_e; ++g)
		{
			const glyf::justification_t level = g->justification();
			const PMReal unit_width = level > glyf::glyph ? _drawing_style->GetSpaceWidth() : g->width();

			s[level].min += unit_width*js[level].min;
			s[level].max += unit_width*js[level].max;
		}
	}
}


run::const_iterator run::find_break(PMReal desired) const
{
	// Walk forwards to find the point where a cluster crosses the desired width
	const_iterator			cl = begin();
	const_iterator const	cl_e = end();
	PMReal advance = 0;
	for (; cl != cl_e; ++cl)
	{
		const PMReal advance_ = advance + (*cl)->width();
		if (advance_ > desired)	break;
		advance = advance_;
	}
	if (cl == cl_e)		return cl_e;

	const_iterator best_cl = cl;
	for (const_iterator const cl_b = begin(); cl != cl_b; --cl)
	{
		// TODO: Investigate ways to use the distance from the desired
		//       break demote potential breaks furthest from the 
		//       desired break point.
		// advance -= cl->width();
		if ((*cl)->break_weight() < (*best_cl)->break_weight())
			best_cl = cl;
	}

	return best_cl;
}


PMReal run::width() const
{
	PMReal advance = 0;

	for (const_iterator cl = begin(), cl_e = end(); cl != cl_e; advance += (*cl)->width(), ++cl);

	return advance;
}

run * run::split(run::const_iterator position) 
{
	run * new_run = clone_empty();
	new_run->_drawing_style = _drawing_style;
	new_run->_height        = _height;
	for (const_iterator i=begin(); i != position; new_run->_span += (*i)->span());
	
	new_run->splice(new_run->begin(), *this, position, end());
	
	_span -= new_run->_span;

	return new_run;
}

