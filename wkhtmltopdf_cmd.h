#pragma once

#include "stdafx.h"
#include "parse.h"

enum html_to_pdf_options {
	kPDF_NONE = 0, /* no effect */
	kPDF_COVER = 1 << 0, /* indicate a wkhtmltopdf cover page */
	kPDF_NO_OUTLINE = 1 << 1, /* do not embed an outline */
	kPDF_DUMP = 1 << 2, /* dump XML outline to file */
	kPDF_FOOTER = 1 << 3, /* indicate a footer is present */
	kPDF_OFFSET = 1 << 4, /* indicate a page offset is present */
	kPDF_ORIENTATION = 1 << 5, /* indicate an orientation is present */
	kPDF_SIZE = 1 << 6, /* indicate a size is present */
	kPDF_HEADER = 1 << 7, /* indicate a header is present */
	kPDF_MARGINS = 1 << 8, /* indicate margins are present */
	kPDF_FIRST_PAGE = 1 << 9, /* indicate this is the first page */

	/* normal body page preset */
	kPDF_NORM = kPDF_NO_OUTLINE | kPDF_DUMP | kPDF_OFFSET | kPDF_ORIENTATION
			| kPDF_SIZE | kPDF_MARGINS,

	/* normal TOC page preset */
	kPDF_TOC = kPDF_NO_OUTLINE | kPDF_SIZE | kPDF_ORIENTATION | kPDF_MARGINS,

	/* normal cover page preset */
	kPDF_COVER_PAGE = kPDF_NO_OUTLINE | kPDF_ORIENTATION | kPDF_SIZE
			| kPDF_MARGINS | kPDF_FIRST_PAGE
};

/*
 * Simplified segment information to process for generating the
 * wkhtmltopdf command.
 */
struct wkhtmltopdf_cmd_info {
	struct pdf_margins margins; /* margin information */
	LPCTSTR exe; /* path to the executable */
	LPCTSTR outline_target; /* file to dump XML outline into */
	LPCTSTR footer_url; /* full footer URL */
	LPCTSTR header_url; /* full header URL */
	LPCTSTR orientation; /* orientation string */
	LPCTSTR size; /* size string */
	LPCTSTR source; /* full source URL */
	LPCTSTR target; /* path to output file */
	unsigned long pages; /* number of pages to offset */
	int options; /* combination of html_to_pdf_options enums */
};

extern LPCTSTR pdf_getter_exe; /* path to the PDF getter executable */

void do_segment_to_pdf(UINT*, UINT*, unsigned long int, const struct pdf_info*,
		const struct pdf_segment_info*, int);
void do_segment_to_pdf_async(FILE**, UINT*, UINT*, unsigned long int,
		const struct pdf_info*, const struct pdf_segment_info*, int);
void do_wkhtmltopdf_execute(FILE**, const struct wkhtmltopdf_cmd_info*);
