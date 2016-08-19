// HTMLToPDFHelper.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "cmd.h"
#include "parse.h"
#include "toc.h"
#include "wkhtmltopdf_cmd.h"
#include "pdftk_cmd.h"
#include "util.h"
#include "log.h"

static void do_get_cover_page(UINT*, const struct pdf_info*);
static void do_get_watermark(UINT*, const struct pdf_info*);

int _tmain(int argc, _TCHAR* argv[])
{
	struct pdf_info info; /* info from the main part of the instruction file */
	FILE* input = NULL; /* instruction file */
	FILE* outline_html_file = NULL; /* HTML table of contents file */
	UINT* merge_files_arr = NULL; /* IDs of temp files for each segment */
	unsigned long int curr_pt = 0; /* current segment number */
	unsigned long int total_pages = 0;
	UINT cover_page_id = 0; /* ID of cover page PDF */
	UINT outline_pdf_id = 0; /* ID of TOC PDF temp file */
	UINT watermark_id = 0; /* ID of watermark PDF temp file */
	TCHAR cover_page_path[MAX_PATH + 1] = _T(""); /* path to cover page PDF */
	TCHAR outline_pdf[MAX_PATH + 1] = _T(""); /* path to TOC PDF */

	errorfd = logfd = require_log(NULL);
	setbuf(errorfd, NULL);

	/* Require one argument for the instruction file */
	if(argc < 2)
		errorout(E_ARG, _T("Instruction file name required"));

	input = require_open_file(argv[1], _T("r"));

	/* Retrive main instruction information */
	get_pdf_info(&info, input);

	/* Allocate memory for the IDs of the segments' temp files */
	merge_files_arr = (UINT*) require_cmem(info.segments,
			sizeof(merge_files_arr[0]));

	/* Download and convert the cover page, if one is present */
	if(info.cover_page.segment != NULL && info.cover_page.size != NULL
			&& info.cover_page.orientation != NULL)
		do_get_cover_page(&cover_page_id, &info);

	if(info.toc_opts == kPDF_TOC_SHOW)
		outline_html_file = open_toc_stream(&outline_pdf_id, &info);
	else
		outline_html_file = require_open_file(_T("nul"), _T("w"));

	write_toc_start(outline_html_file, info.font_family, info.font_size);

	/*
	 * For each expected segment, read the segment's information from
	 * the instruction file, dowload and convert it to a PDF, and save
	 * the ID of its temp file.
	 */
	for(curr_pt = 0; curr_pt < info.segments; ++curr_pt) {
		struct pdf_segment_info part; /* segment information */
		unsigned long int pages = 0; /* pages in this segment */
		FILE* outline_file = NULL; /* temp file for the segment outline dump */
		unsigned long int dest_page = 0; /* TOC page number for titles */
		UINT outline_id = 0; /* ID of the outline dump temp file */
		int options = kPDF_NORM;
		TCHAR title[BUFSIZ] = _T(""); /* current title for the TOC */
		TCHAR outline[MAX_PATH + 1] = _T(""); /* name of the outline file */
		TCHAR target[MAX_PATH + 1] = _T(""); /* name of the segment PDF */

		/* Read and initialize the segment information */
		get_pdf_segment_info(&part, input);
		require_tmp_file(outline, &outline_id);
		require_tmp_file(target, &merge_files_arr[curr_pt]);
		outline_file = require_open_file(outline, _T("r"));

		/* Outline XML is in UTF-8 so set transparent conversion */
		setmode(fileno(outline_file), _O_U8TEXT);

		if(info.hf_opts == kPDF_HF_SHOW || info.hf_opts == kPDF_HF_SPECIAL) {
			if(info.header_url != NULL)
				options |= kPDF_HEADER;
			if(info.footer_url != NULL)
				options |= kPDF_FOOTER;
		}

		/* Execute conversion */
		do_segment_to_pdf(&merge_files_arr[curr_pt], &outline_id, total_pages,
				&info, &part, options);
		destroy_pdf_segment_info(&part);

		/* Determine the number of new pages added by this segment */
		get_number_of_pages(&pages, target);
	
		/* Add each title and page number in this segment to the TOC */
		do {
			get_toc_item(title, LENGTHOF(title) - 1, &dest_page, outline_file);

			if(dest_page != total_pages && dest_page != ULONG_MAX)
				write_toc_item(outline_html_file, title, dest_page);
		} while(dest_page != ULONG_MAX);

		release_file(outline_file);
		total_pages += pages;
		remove_tmp_file(outline);
	}

	write_toc_end(outline_html_file);

	/* Close the TOC stream */
	if(info.toc_opts == kPDF_TOC_SHOW) {
		int status = _pclose(outline_html_file);

		if(status != 0)
			errorout(E_PDFGETTER, _T("%s exited with status %d"),
					pdf_getter_exe, status);

		require_tmp_file(outline_pdf, &outline_pdf_id);
	} else {
		release_file(outline_html_file);
	}

	release_file(input);
	require_tmp_file(cover_page_path, &cover_page_id);

	if(info.watermark_url != NULL)
		do_get_watermark(&watermark_id, &info);

	/* Merge the PDF segments */
	do_merge_pdfs(info.target_path, cover_page_path, outline_pdf,
			info.segments, merge_files_arr, watermark_id);

	if(info.header_url != NULL)
		remove_tmp_file(info.header_url);

	if(info.first_header_url != NULL)
		remove_tmp_file(info.first_header_url);

	if(info.footer_url != NULL)
		remove_tmp_file(info.footer_url);

	if(info.first_footer_url != NULL)
		remove_tmp_file(info.first_footer_url);

	destroy_pdf_info(&info);

	/* Clean up temporary files and memory */
	while(--curr_pt < info.segments) {
		TCHAR target[MAX_PATH + 1] = _T("");

		get_tmp_file(target, &merge_files_arr[curr_pt]);

		if(merge_files_arr[curr_pt] != 0)
			remove_tmp_file(target);
	}

	if(cover_page_id != 0)
		remove_tmp_file(cover_page_path);

	if(watermark_id != 0) {
		TCHAR watermark_pdf[MAX_PATH + 1] = _T(""); /* path to watermark PDF */

		get_tmp_file(watermark_pdf, &watermark_id);
		remove_tmp_file(watermark_pdf);
	}

	remove_tmp_file(outline_pdf);
	free(merge_files_arr);
	release_file(logfd);
	return E_SUCCESS;
}

/*
 * Retrieve and convert the cover page, placing the output into the
 * temporary file with the given ID. The structure pointed to by info
 * contains the necessary information about the cover page. Neither the
 * value of info nor the value of cover_page_id may be NULL. The value
 * pointed to by cover_page_id is set to the ID used to generate the
 * name of the temporary file.
 */
void do_get_cover_page(UINT* cover_page_id, const struct pdf_info* info)
{
	int options = kPDF_COVER_PAGE;

	RT_NOT_NULL(cover_page_id);
	RT_NOT_NULL(info);

	if(info->hf_opts == kPDF_HF_SHOW) {
		if(info->header_url != NULL)
			options |= kPDF_HEADER;
		if(info->footer_url != NULL)
			options |= kPDF_FOOTER;
	} else if(info->hf_opts == kPDF_HF_SPECIAL) {
		if(info->first_header_url != NULL)
			options |= kPDF_HEADER;
		if(info->first_footer_url != NULL)
			options |= kPDF_FOOTER;
	}

	do_segment_to_pdf(cover_page_id, NULL, 0, info, &info->cover_page,
			options);
}

/*
 * Retrieve and convert the watermark, placing the output into the
 * temporary file with the given ID. The structure pointed to by info
 * contains the necessary information about the watermark. Neither the
 * value of info nor the value of watermark_id may be NULL. The value
 * pointed to by watermark_id is set to the ID used to generate the name
 * of the temporary file.
 */
void do_get_watermark(UINT* watermark_id, const struct pdf_info* info)
{
	struct wkhtmltopdf_cmd_info cmd_info;
	FILE* pipe = NULL;
	LPTSTR source = NULL;
	LPTSTR session_str = NULL;
	int status = 0;
	TCHAR watermark_pdf[MAX_PATH + 1] = _T("");

	RT_NOT_NULL(watermark_id);
	RT_NOT_NULL(info);

	if(info->session != NULL)
		session_str = require_strf(_T("&SESSION_OVERRIDE=%s"),
			info->session);
	else
		session_str = require_strf(_T(""));

	source = require_strf(_T("%s%s%s"), info->base_url != NULL ?
			info->base_url : _T(""), info->watermark_url, session_str);
	require_tmp_file(watermark_pdf, watermark_id);
	cmd_info.exe = pdf_getter_exe;
	cmd_info.source = source;
	cmd_info.target = watermark_pdf;
	cmd_info.margins.bottom = _T("0");
	cmd_info.margins.top = _T("0");
	cmd_info.margins.right = _T("0");
	cmd_info.margins.left = _T("0");
	cmd_info.margins.header = _T("0");
	cmd_info.margins.footer = _T("0");
	cmd_info.size = info->cover_page.size;
	cmd_info.orientation = info->cover_page.orientation;
	cmd_info.options = kPDF_COVER | kPDF_MARGINS | kPDF_SIZE;
	do_wkhtmltopdf_execute(&pipe, &cmd_info);
	status = _pclose(pipe);
	free(source);
	free(session_str);

	if(status != 0)
		errorout(E_PDFGETTER, _T("%s exited with status %d"), pdf_getter_exe,
				status);
}
