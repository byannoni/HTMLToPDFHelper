#pragma once

#include "stdafx.h"

/* Header and footer display options */
enum pdf_hf_opts {
	kPDF_HF_SHOW,
	kPDF_HF_HIDE,
	kPDF_HF_SPECIAL /* Show w/special first page */
};

/* Table of contents display options */
enum pdf_toc_opts {
	kPDF_TOC_SHOW,
	kPDF_TOC_HIDE
};

/* Segment information */
struct pdf_segment_info {
	LPTSTR orientation; /* the orientation of the pages in this part */
	LPTSTR size; /* the paper size of the pages in this part */
	LPTSTR segment; /* the query string for this part */
};

/* Margin information */
struct pdf_margins {
	LPTSTR top; /* top margin (encompasses header)*/
	LPTSTR bottom; /* bottom margin (encompasses footer)*/
	LPTSTR left; /* left margin */
	LPTSTR right; /* right margin */
	LPTSTR header; /* spacing between header and content */
	LPTSTR footer; /* spacing between footer and content */
};

/* Global PDF information */
struct pdf_info {
	struct pdf_segment_info cover_page; /* cover page segment */
	struct pdf_margins margins; /* global margins */
	LPTSTR watermark_url; /* watermark query string */
	LPTSTR first_header_url; /* first page header query string */
	LPTSTR first_footer_url; /* first page footer query string */
	LPTSTR session; /* GUID for SESSION_OVERRIDE */
	LPTSTR base_url; /* URL to Cockpit with trailing / */
	LPTSTR footer_url; /* normal footer query string */
	LPTSTR header_url; /* normal header query string */
	LPTSTR target_path; /* path to the final PDF output file */
	LPTSTR font_size; /* size of the document font for the TOC */
	LPTSTR font_family; /* font family of the document font for the TOC */
	unsigned long int segments; /* number of segments to expect */
	enum pdf_hf_opts hf_opts; /* header and footer display options */
	enum pdf_toc_opts toc_opts; /* table of contents display options */
};

void get_pdf_info(struct pdf_info*, FILE*);
void get_pdf_segment_info(struct pdf_segment_info*, FILE*);
void destroy_pdf_info(struct pdf_info*);
void destroy_pdf_margins(struct pdf_margins*);
void destroy_pdf_segment_info(struct pdf_segment_info*);
