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
#include <IDrawingStyle.h>
#include <IFontInstance.h>
// Library headers
#include <unicode/uchar.h>
#include <PMRealGlyphPoint.h>
#include <textiterator.h>
// Module header
#include "FallbackRun.h"

// Forward declarations
// InDesign interfaces
// Graphite forward delcarations
// Project forward declarations
using namespace nrsc;

run * fallback_run::clone_empty() const
{
	return new fallback_run();
}


bool fallback_run::layout_span(TextIterator ti, size_t span)
{
	if (span ==0)
		return true;

	InterfacePtr<IFontInstance>	font = _drawing_style->QueryFontInstance(kFalse);
	if (font == nil)
		return false;

	const PMReal min_width = _drawing_style->GetEmSpaceWidth(false)/48.0;
	WideString	chars;
	ti.AppendToStringAndIncrement(&chars, span);

	// Make a segment
	PMRealGlyphPoint * gps = new PMRealGlyphPoint[span];
	font->FillOutGlyphIDs(gps, span, chars.GrabUTF16Buffer(0), chars.NumUTF16TextChars());
	font->GetKerns(gps, span);


	// Add the glyphs with their natural widths
	PMReal prev_shift = 0;
	PMRealGlyphPoint * gp = gps;
	for (WideString::const_iterator c = chars.begin(); c != chars.end(); ++c, ++gp)
	{
		const int gid = gp->GetGlyphID();
		
		if (u_isspace(*c))
			add_glue(glyf::space, _drawing_style->GetSpaceWidth(), u_isWhitespace(*c) ? cluster::breakweight::whitespace : cluster::breakweight::never);
		else
		{
			PMReal glyph_width = font->GetGlyphWidth(gid);
			add_letter(gid, glyph_width, cluster::breakweight::letter, glyph_width < min_width);
		}

		// Set the kerning.
		glyf & last_glyf = back().back();
		last_glyf.shift(PMPoint(gp->GetXPosition() - prev_shift, 0));
		prev_shift = gp->GetXPosition();
	}
	
	delete [] gps;

	return true;
}
