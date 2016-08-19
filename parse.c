
#include "stdafx.h"
#include "parse.h"
#include "util.h"
#include "log.h"

extern LPTSTR header_html = NULL;
static void trim(LPTSTR);
static void chomp(LPTSTR);
static void init_pdf_info(struct pdf_info*);
static void init_pdf_margins(struct pdf_margins*);
static void init_pdf_segment_info(struct pdf_segment_info*);

/*
 * Initialize the given pdf_margins structure. The value of margins
 * must not be NULL.
 */
void init_pdf_margins(struct pdf_margins* margins)
{
	RT_NOT_NULL(margins);

	margins->top = NULL;
	margins->bottom = NULL;
	margins->left = NULL;
	margins->right = NULL;
	margins->header = NULL;
	margins->footer = NULL;
}

/*
 * Initialize the given pdf_segment_info structure. The value of info
 * must not be NULL.
 */
void init_pdf_segment_info(struct pdf_segment_info* info)
{
	RT_NOT_NULL(info);

	info->orientation = NULL;
	info->size = NULL;
	info->segment = NULL;
}

/*
 * Initialize the given pdf_info structure. The value of info must not
 * be NULL.
 */
void init_pdf_info(struct pdf_info* info)
{
	RT_NOT_NULL(info);

	init_pdf_margins(&info->margins);
	init_pdf_segment_info(&info->cover_page);

	info->toc_opts = kPDF_TOC_SHOW;
	info->hf_opts = kPDF_HF_SHOW;
	info->watermark_url = NULL;
	info->first_header_url = NULL;
	info->first_footer_url = NULL;
	info->session = NULL;
	info->base_url = NULL;
	info->footer_url = NULL;
	info->header_url = NULL;
	info->target_path = NULL;
	info->font_family = NULL;
	info->font_size = NULL;
	info->segments = ULONG_MAX;
}

/*
 * Parses the given file for information to initialize the pdf_info
 * structure pointed to by pi. Neither pi nor fd may be NULL.
 */
void get_pdf_info(struct pdf_info* pi, FILE* fd)
{
	TCHAR buf[BUFSIZ] = _T("");

	RT_NOT_NULL(pi);
	RT_NOT_NULL(fd);

	init_pdf_info(pi);

	while(_fgetts(buf, LENGTHOF(buf), fd) != NULL) {
		LPTSTR line = require_dup_str(buf);
		LPTSTR var = line;
		LPTSTR val = _tcschr(line, _T('='));
		TCHAR comment[LENGTHOF(_T(";"))] = _T("");

		if(_stscanf(line, _T("%1[;]"), comment) == 1) {
			free(line);

			if(_tcschr(line, _T('\n')) == NULL)
				skip_line(fd);

			continue;
		}

		if(val == NULL) {
			/*
			 * We can use _stscanf() as a whitespace-insensitive
			 * comparison here because it returns EOF if an input
			 * failure occurs before any conversion and we have no
			 * conversions here.
			 */
			if(_stscanf(line, _T(" end \n")) != EOF) {
				free(line);
				break;
			}

			free(line);
			continue;
		} else {
			*val++ = _T('\0');
			trim(val);
			trim(var);

			if(_tcscmp(_T("iSegments"), var) == 0) {
				pi->segments = require_strtoul(val, NULL, 10);
			} else if(_tcscmp(_T("sBaseURL"), var) == 0) {
				pi->base_url = require_dup_str(val);
			} else if(_tcscmp(_T("sTargetPath"), var) == 0) {
				pi->target_path = require_dup_str(val);
			} else if(_tcscmp(_T("sCoverPageURL"), var) == 0) {
				pi->cover_page.segment = require_dup_str(val);
			} else if(_tcscmp(_T("sSize"), var) == 0) {
				pi->cover_page.size = require_dup_str(val);
			} else if(_tcscmp(_T("sOrientation"), var) == 0) {
				pi->cover_page.orientation = require_dup_str(val);
			} else if(_tcscmp(_T("sSession"), var) == 0) {
				pi->session = require_dup_str(val);
			} else if(_tcscmp(_T("sTopMargin"), var) == 0) {
				pi->margins.top = require_dup_str(val);
			} else if(_tcscmp(_T("sBottomMargin"), var) == 0) {
				pi->margins.bottom = require_dup_str(val);
			} else if(_tcscmp(_T("sLeftMargin"), var) == 0) {
				pi->margins.left = require_dup_str(val);
			} else if(_tcscmp(_T("sRightMargin"), var) == 0) {
				pi->margins.right = require_dup_str(val);
			} else if(_tcscmp(_T("sHeaderMargin"), var) == 0) {
				pi->margins.header = require_dup_str(val);
			} else if(_tcscmp(_T("sFooterMargin"), var) == 0) {
				pi->margins.footer = require_dup_str(val);
			} else if(_tcscmp(_T("sTableOfContentsOptions"), var) == 0) {
				if(_tcscmp(val, _T("Show")) == 0)
					pi->toc_opts = kPDF_TOC_SHOW;
				else if(_tcscmp(val, _T("Don't show")) == 0)
					pi->toc_opts = kPDF_TOC_HIDE;
			} else if(_tcscmp(_T("sHeaderFooterOptions"), var) == 0) {
				if(_tcscmp(val, _T("Show")) == 0)
					pi->hf_opts = kPDF_HF_SHOW;
				else if(_tcscmp(val, _T("Don't show")) == 0)
					pi->hf_opts = kPDF_HF_HIDE;
				else if(_tcscmp(val, _T("Show w/first page special")) == 0)
					pi->hf_opts = kPDF_HF_SPECIAL;
			} else if(_tcscmp(var, _T("sWatermarkURL")) == 0) {
				pi->watermark_url = require_dup_str(val);
			} else if(_tcscmp(var, _T("sDocFontSize")) == 0) {
				pi->font_size = require_dup_str(val);
			} else if(_tcscmp(var, _T("sDocFontFamily")) == 0) {
				pi->font_family = require_dup_str(val);
			} else if(_tcscmp(var, _T("sHeaderHTML")) == 0) {
				FILE* header_html_file = NULL;
				LPTSTR ext = NULL;
				LPTSTR old_name = NULL;
				TCHAR fname[MAX_PATH + 1] = _T("");

				require_tmp_file(fname, NULL);
				old_name = require_dup_str(fname);
				ext = _tcsrchr(fname, _T('.'));

				if(ext != NULL)
					ext[0] = _T('\0');

				pi->header_url = require_strf(_T("%.*s.html"),
						MAX_PATH - LENGTHOF(_T(".html")) + 1, fname);
				_trename(old_name, pi->header_url);
				header_html_file = require_open_file(pi->header_url, _T("w"));

				if(_fputts(val, header_html_file) < 0)
					errorout(E_BADF, _T("Failed to generate header file"));

				while(_tcschr(buf, _T('\n')) == NULL && _fgetts(buf,
						LENGTHOF(buf), fd) != NULL)
					if(_fputts(buf, header_html_file) < 0)
						errorout(E_BADF, _T("Failed to generate header file"));

				release_file(header_html_file);
				free(old_name);
			} else if(_tcscmp(var, _T("sFooterHTML")) == 0) {
				FILE* footer_html_file = NULL;
				LPTSTR ext = NULL;
				LPTSTR old_name = NULL;
				TCHAR fname[MAX_PATH + 1] = _T("");

				require_tmp_file(fname, NULL);
				old_name = require_dup_str(fname);
				ext = _tcsrchr(fname, _T('.'));

				if(ext != NULL)
					ext[0] = _T('\0');

				pi->footer_url = require_strf(_T("%.*s.html"),
						MAX_PATH - LENGTHOF(_T(".html")) + 1, fname);
				_trename(old_name, pi->footer_url);
				footer_html_file = require_open_file(pi->footer_url, _T("w"));

				if(_fputts(val, footer_html_file) < 0)
					errorout(E_BADF, _T("Failed to generate footer file"));

				while(_tcschr(buf, _T('\n')) == NULL && _fgetts(buf,
						LENGTHOF(buf), fd) != NULL)
					if(_fputts(buf, footer_html_file) < 0)
						errorout(E_BADF, _T("Failed to generate footer file"));

				release_file(footer_html_file);
			} else if(_tcscmp(var, _T("sFirstHeaderHTML")) == 0) {
				FILE* header_html_file = NULL;
				LPTSTR old_name = NULL;
				LPTSTR ext = NULL;
				TCHAR fname[MAX_PATH + 1] = _T("");

				require_tmp_file(fname, NULL);
				old_name = require_dup_str(fname);
				ext = _tcsrchr(fname, _T('.'));

				if(ext != NULL)
					ext[0] = _T('\0');

				pi->first_header_url = require_strf(_T("%.*s.html"),
						MAX_PATH - LENGTHOF(_T(".html")) + 1, fname);
				_trename(old_name, pi->first_header_url);
				header_html_file = require_open_file(pi->first_header_url,
						_T("w"));

				if(_fputts(val, header_html_file) < 0)
					errorout(E_BADF, _T("Failed to write header file"));

				while(_tcschr(buf, _T('\n')) == NULL && _fgetts(buf,
						LENGTHOF(buf), fd) != NULL)
					if(_fputts(buf, header_html_file) < 0)
						errorout(E_BADF, _T("Failed to write header file"));

				release_file(header_html_file);
			} else if(_tcscmp(var, _T("sFirstFooterHTML")) == 0) {
				FILE* footer_html_file = NULL;
				LPTSTR old_name = NULL;
				LPTSTR ext = NULL;
				TCHAR fname[MAX_PATH + 1] = _T("");

				require_tmp_file(fname, NULL);
				old_name = require_dup_str(fname);
				ext = _tcsrchr(fname, _T('.'));

				if(ext != NULL)
					ext[0] = _T('\0');

				pi->first_footer_url = require_strf(_T("%.*s.html"),
						MAX_PATH - LENGTHOF(_T(".html")) + 1, fname);
				_trename(old_name, pi->first_footer_url);
				footer_html_file = require_open_file(pi->first_footer_url,
						_T("w"));

				if(_fputts(val, footer_html_file) < 0)
					errorout(E_BADF, _T("Failed to write footer file"));

				while(_tcschr(buf, _T('\n')) == NULL && _fgetts(buf,
						LENGTHOF(buf), fd) != NULL)
					if(_fputts(buf, footer_html_file) < 0)
						errorout(E_BADF, _T("Failed to write footer file"));

				release_file(footer_html_file);
			}
		}

		free(line);
	}

	if(pi->segments == ULONG_MAX || pi->base_url == NULL
			|| pi->target_path == NULL)
		errorout(E_BADSEGMENT, _T("Failed to read initial segment"));
}

/*
 * Parses the given file for information to initialize the
 * pdf_segment_info structure pointed to by pi. Neither ppi nor fd may
 * be NULL.
 */
void get_pdf_segment_info(struct pdf_segment_info* ppi, FILE* fd)
{
	TCHAR line[BUFSIZ] = _T("");

	RT_NOT_NULL(ppi);
	RT_NOT_NULL(fd);

	init_pdf_segment_info(ppi);

	while(_fgetts(line, LENGTHOF(line), fd) != NULL) {
		LPTSTR var = line;
		LPTSTR val = _tcschr(line, _T('='));
		TCHAR comment[LENGTHOF(_T(";"))] = _T("");

		if(_stscanf(line, _T("%1[;]"), comment) == 1) {
			if(_tcschr(line, _T('\n')) == NULL)
				skip_line(fd);

			continue;
		}

		if(val == NULL) {
			/*
			 * We can use _stscanf() as a whitespace-insensitive
			 * comparison here because it returns EOF if an input
			 * failure occurs before any conversion and we have no
			 * conversion directives here.
			 */
			if(_stscanf(line, _T(" end \n")) != EOF)
				break;

			continue;
		} else {
			*val++ = _T('\0');
			trim(val);
			trim(var);

			if(_tcscmp(_T("sOrientation"), var) == 0) {
				ppi->orientation = require_dup_str(val);
			} else if(_tcscmp(_T("sSize"), var) == 0) {
				ppi->size = require_dup_str(val);
			} else if(_tcscmp(_T("sSegmentURL"), var) == 0) {
				ppi->segment = require_dup_str(val);
			}
		}
	}

	if(ppi->segment == NULL || ppi->orientation == NULL || ppi->size == NULL)
		errorout(E_BADSEGMENT, _T("Failed to load segment"));
}

/*
 * Removes whitespace from the beginning and end of the given string as
 * determined by _istspace(). The pointer str must not be NULL.
 */
static void trim(LPTSTR str)
{
	LPTSTR beginning = str;
	LPTSTR end = NULL;

	RT_NOT_NULL(str);
		
	while(_istspace(*beginning))
		++beginning;

	if(*beginning == _T('\0')) {
		str[0] = _T('\0');
		return;
	}

	end = str + _tcslen(str) - 1;
	
	while(end > str && _istspace(*end))
		--end;

	*++end = _T('\0');
	memmove(str, beginning, (end - beginning + 1) * sizeof(str[0]));
}

/*
 * Removes trailing newlines from the end of the given string. The
 * pointer str must not be NULL.
 */
static void chomp(LPTSTR str)
{
	LPTSTR end = NULL;
	
	RT_NOT_NULL(str);

	end = str + _tcslen(str);

	while(end > str && *end == _T('\n'))
		--end;

	if(end > str)
		end[-1] = _T('\0');
}

/*
 * This procedure destroys the object pointed to by pi. It can be
 * reinitialized with get_pdf_info(). An attempt to use it after
 * calling this procedure results in undefined behavior. The value of pi
 * must not be NULL.
 */
void destroy_pdf_info(struct pdf_info* pi)
{
	RT_NOT_NULL(pi);

	free(pi->base_url);
	free(pi->target_path);
	free(pi->first_footer_url);
	free(pi->first_header_url);
	free(pi->footer_url);
	free(pi->header_url);
	free(pi->session);
	free(pi->watermark_url);
	free(pi->font_family);
	free(pi->font_size);

	destroy_pdf_margins(&pi->margins);
	destroy_pdf_segment_info(&pi->cover_page);
}

/*
 * This procedure destroys the object pointed to by margins. It can be
 * reinitialized with init_pdf_margins. An attempt to use it after
 * calling this procedure results in undefined behavior. The value of
 * margins must not be NULL.
 */
void destroy_pdf_margins(struct pdf_margins* margins)
{
	RT_NOT_NULL(margins);

	free(margins->bottom);
	free(margins->top);
	free(margins->left);
	free(margins->right);
	free(margins->footer);
	free(margins->header);
}

/*
 * This procedure destroys the object pointed to by ppi. It can be
 * reinitialized with get_pdf_segment_info(). An attempt to use it
 * after calling this procedure results in undefined behavior. The value
 * of ppi must not be NULL.
 */
void destroy_pdf_segment_info(struct pdf_segment_info* ppi)
{
	RT_NOT_NULL(ppi);

	free(ppi->orientation);
	free(ppi->segment);
	free(ppi->size);
}
