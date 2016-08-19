
#include "stdafx.h"
#include "wkhtmltopdf_cmd.h"
#include "cmd.h"
#include "util.h"
#include "log.h"

LPCTSTR pdf_getter_exe = _T("wkhtmltopdf");

static void generate_cmd_str(LPTSTR, const struct wkhtmltopdf_cmd_info*);

/*
 * Execute a synchronous instance of wkhtmltopdf. The values pointed to
 * by target_id and outline_id are used to generate the temporary file
 * names. The value of pages is used as the page offset for the segment.
 * The structures pointed to by info and segment are used to provide
 * information about the current segment. The value of options is a
 * bitwise combination of html_to_pdf_options enumerations that
 * indicates the arguments that should be passed to the executable. The
 * values of target_id, info and segment must not be NULL. The value of
 * outline_id must not be NULL if options specifies kPDF_DUMP. The
 * values pointed to by target_id and outline_id are set appropriately
 * to the IDs that are used to generate the temporary files.
 */
void do_segment_to_pdf(UINT* target_id, UINT* outline_id,
		unsigned long int pages, const struct pdf_info* info,
		const struct pdf_segment_info* segment, int options) 
{
	FILE* pipe = NULL;
	int status = 0;
	
	do_segment_to_pdf_async(&pipe, target_id, outline_id, pages, info, segment,
			options);
	status = _pclose(pipe);

	if(status != 0)
		errorout(E_PDFGETTER, _T("%s exited with status %d"), pdf_getter_exe,
				status);
}

/*
 * Execute an asynchronous instance of wkhtmltopdf. The values pointed
 * to by target_id and outline_id are used to generate the temporary
 * file names. The value of pages is used as the page offset for the
 * segment. The structures pointed to by info and segment are used to
 * provide information about the current segment. The value of options
 * is a bitwise combination of html_to_pdf_options enumerations that
 * indicates the arguments that should be passed to the executable. The
 * values of target_id, info, pipe and segment must not be NULL. The
 * value of outline_id must not be NULL if options specifies kPDF_DUMP.
 * The values of target_id and outline_id are set appropriately to the
 * IDs that are used to generate the temporary files. The value pointed
 * to by pipe is set to the FILE pointer returned by _tpopen(). That
 * FILE pointer must be passed to _pclose().
 */
void do_segment_to_pdf_async(FILE** pipe, UINT* target_id, UINT* outline_id,
		unsigned long int pages, const struct pdf_info* info,
		const struct pdf_segment_info* section, int options)
{
	struct wkhtmltopdf_cmd_info cmd_info;
	LPTSTR session_str = NULL;
	LPTSTR source = NULL;
	LPTSTR footer_url = NULL;
	LPTSTR header_url = NULL;
	TCHAR outline_path[MAX_PATH + 1] = _T("");
	TCHAR target_path[MAX_PATH + 1] = _T("");

	RT_NOT_NULL(target_id);
	RT_NOT_NULL(info);
	RT_NOT_NULL(section);
	RT_NOT_NULL(info->base_url);

	if(options & kPDF_DUMP) {
		RT_NOT_NULL(outline_id);

		require_tmp_file(outline_path, outline_id);
	}

	cmd_info.footer_url = NULL;
	cmd_info.header_url = NULL;
	require_tmp_file(target_path, target_id);

	if(info->session != NULL)
		session_str = require_strf(_T("&SESSION_OVERRIDE=%s"), info->session);
	else
		session_str = require_strf(_T(""));

	if(options & kPDF_FIRST_PAGE && info->hf_opts == kPDF_HF_SPECIAL) {
		if(options & kPDF_FOOTER)
			cmd_info.footer_url = info->first_footer_url;

		if(options & kPDF_HEADER)
			cmd_info.header_url = info->first_header_url;
	} else {
		if(options & kPDF_FOOTER)
			cmd_info.footer_url = info->footer_url;

		if(options & kPDF_HEADER)
			cmd_info.header_url = info->header_url;
	}

	source = require_strf(_T("%s%s%s"), info->base_url != NULL ?
				info->base_url : _T(""), section->segment, session_str);
	free(session_str);
	cmd_info.source = source;
	cmd_info.exe = pdf_getter_exe;
	cmd_info.options = options;
	cmd_info.orientation = section->orientation;
	cmd_info.outline_target = outline_path;
	cmd_info.pages = pages;
	cmd_info.size = section->size;
	cmd_info.target = target_path;
	cmd_info.margins = info->margins;

	do_wkhtmltopdf_execute(pipe, &cmd_info);
	free(source);
	free(footer_url);
	free(header_url);
}

/*
 * Execute an asynchronous instance of wkhtmltopdf. The structure
 * pointed to by cmd_info is used to provide information about the
 * command to be executed and its arguments. The values of pipe and
 * cmd_info must not be NULL. The value pointed to by pipe is set to the
 * FILE pointer returned by _tpopen(). That FILE pointer must be passed
 * to _pclose().
 */
void do_wkhtmltopdf_execute(FILE** pipe,
		const struct wkhtmltopdf_cmd_info* cmd_info)
{
	LPTSTR cmd_base = (LPTSTR) require_cmem(sizeof(*cmd_base), CMD_MAX_LEN);
	enum cmd_err err = CMD_ERR_SUCCESS;

	RT_NOT_NULL(pipe);
	RT_NOT_NULL(cmd_info);

	generate_cmd_str(cmd_base, cmd_info);
	err = runw(pipe, _T("%s"), cmd_base);
	free(cmd_base);

	if(*pipe == NULL)
		errorout(E_BADF, _T("Failed to open pipe to %s"), pdf_getter_exe);

	if(err != CMD_ERR_SUCCESS)
		errorout(E_CMD, _T("Failed to execute %s (%d)"), pdf_getter_exe, err);
}

/*
 * Generate a wkhtmltopdf command from the given options. The structure
 * pointed to by cmd_info specifies which arguments and information to
 * include in the command. The buffer pointed to by cmd is assumed to be
 * at least CMD_MAX_LEN in length. The values of cmd and cmd_info must
 * not be NULL. If this procedure succeeds, the command string is stored
 * in the buffer pointed to by cmd. Otherwise, the contents of the
 * buffer pointed to by cmd are unspecified.
 */
static void generate_cmd_str(LPTSTR cmd,
		const struct wkhtmltopdf_cmd_info* cmd_info)
{
	int ret = -1;
	int max_len = CMD_MAX_LEN;

	RT_NOT_NULL(cmd_info);
	RT_NOT_NULL(cmd_info->source);
	RT_NOT_NULL(cmd_info->target);
	RT_NOT_NULL(cmd);

	ret = _sntprintf(cmd, max_len, _T("%s --disable-smart-shrinking"),
			pdf_getter_exe);

	if(ret >= max_len || ret < 0)
		goto fail;

	max_len -= ret;
	cmd += ret;

	if(cmd_info->options & kPDF_NO_OUTLINE) {
		ret = _sntprintf(cmd, max_len, _T(" --no-outline"));

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_DUMP) {
		RT_NOT_NULL(cmd_info->outline_target);

		ret = _sntprintf(cmd, max_len, _T(" --dump-outline %s"),
				cmd_info->outline_target);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_FOOTER) {
		RT_NOT_NULL(cmd_info->footer_url);

		ret = _sntprintf(cmd, max_len, _T(" --footer-html \"%s\""),
				cmd_info->footer_url);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_OFFSET) {
		ret = _sntprintf(cmd, max_len, _T(" --page-offset %lu"),
				cmd_info->pages);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_ORIENTATION) {
		RT_NOT_NULL(cmd_info->orientation);

		ret = _sntprintf(cmd, max_len, _T(" -O %s"), cmd_info->orientation);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_SIZE) {
		ret = _sntprintf(cmd, max_len, _T(" -s %s"), cmd_info->size);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_HEADER) {
		RT_NOT_NULL(cmd_info->header_url);

		ret = _sntprintf(cmd, max_len, _T(" --header-html \"%s\""),
				cmd_info->header_url);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_MARGINS) {
		RT_NOT_NULL(cmd_info->margins.bottom);
		RT_NOT_NULL(cmd_info->margins.left);
		RT_NOT_NULL(cmd_info->margins.right);
		RT_NOT_NULL(cmd_info->margins.top);
		RT_NOT_NULL(cmd_info->margins.footer);
		RT_NOT_NULL(cmd_info->margins.header);

		ret = _sntprintf(cmd, max_len, _T(" -B %s -L %s -R %s -T %s")
				_T(" --footer-spacing %s --header-spacing %s"),
				cmd_info->margins.bottom, cmd_info->margins.left,
				cmd_info->margins.right, cmd_info->margins.top,
				cmd_info->margins.footer, cmd_info->margins.header);

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	if(cmd_info->options & kPDF_COVER) {
		ret = _sntprintf(cmd, max_len, _T(" cover"));

		if(ret >= max_len || ret < 0)
			goto fail;

		max_len -= ret;
		cmd += ret;
	}

	ret = _sntprintf(cmd, max_len, _T(" \"%s\" %s"),
			cmd_info->source, cmd_info->target);

	if(ret < max_len && ret >= 0)
		return;

fail:
	errorout(E_CMD, _T("%s (options: %#X)"),
			_T("Failed to create format string for PDF getter"),
			cmd_info->options);
}
