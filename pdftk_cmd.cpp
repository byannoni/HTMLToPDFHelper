
#include "stdafx.h"
#include "cmd.h"
#include "util.h"
#include "log.h"

LPCTSTR pdf_merger_exe = _T("pdftk");

static void generate_merge_files_str(LPTSTR, UINT*, size_t);

/*
 * Merge the given cover page, table of contents and segment PDFs. The
 * final PDF is written to the file named by the value of target. The
 * length of the array of IDs of segment temporary files, arr, must be
 * of size n and contain the names of the files in the order in which
 * they should be merged. The values of target, cover, and toc must not
 * be NULL. The value of arr must not be NULL unless n is equal to zero.
 */
void do_merge_pdfs(LPCTSTR target, LPCTSTR cover, LPCTSTR toc, size_t n,
		UINT* arr, UINT watermark_id)
{
	LPTSTR output_path = NULL;
	LPTSTR files = (LPTSTR)require_cmem(sizeof(*files), CMD_MAX_LEN);
	UINT tmp_output_id = 0;
	enum cmd_err err = CMD_ERR_SUCCESS;
	int status = 0;
	TCHAR tmp_output_path[MAX_PATH + 1] = _T("");

	RT_NOT_NULL(cover);
	RT_NOT_NULL(target);
	RT_NOT_NULL(toc);

	generate_merge_files_str(files, arr, n);

	if(watermark_id != 0) {
		require_tmp_file(tmp_output_path, &tmp_output_id);
		output_path = tmp_output_path;
	} else {
		output_path = require_dup_str(target);
	}

	err = run(&status, _T("%s %s %s %s cat output %s"), pdf_merger_exe,
			cover, toc, files, output_path);
	free(files);

	if(status != 0)
		errorout(E_PDFMERGER, _T("%s exited with status %d"), pdf_merger_exe,
				status);

	if(err != CMD_ERR_SUCCESS)
		errorout(E_CMD, _T("Failed to execute %s (%d)"), pdf_merger_exe, err);

	if(watermark_id != 0) {
		TCHAR watermark_pdf[MAX_PATH + 1] = _T("");

		require_tmp_file(watermark_pdf, &watermark_id);
		run(&status, _T("%s %s background %s output %s"), pdf_merger_exe,
				output_path, watermark_pdf, target);

		if(status != 0)
			errorout(E_PDFMERGER, _T("%s exited with status %d"),
					pdf_merger_exe, status);

		if(err != CMD_ERR_SUCCESS)
			errorout(E_CMD, _T("Failed to execute %s (%d)"), pdf_merger_exe,
					err);

		remove_tmp_file(tmp_output_path);
	} else {
		free(output_path);
	}

}

/*
 * Expand the given array of n IDs of temporary file names, arr, to the
 * given string. The elements are separated by spaces. The value of str
 * must not be NULL. The value of arr must not be NULL unless n is equal
 * to zero.
 */
static void generate_merge_files_str(LPTSTR str, UINT* arr, size_t n)
{
	size_t elem = 0;
	int max_len = CMD_MAX_LEN;

	RT_NOT_NULL(str);

	if(n > 0)
		RT_NOT_NULL(arr);

	for(elem = 0; elem < n; ++elem) {
		int len = -1;
		TCHAR fname[MAX_PATH + 1] = _T("");

		require_tmp_file(fname, &arr[elem]);
		len = _sntprintf(str, max_len, _T("%s%s"),
				fname, elem < n - 1 ? _T(" ") : _T(""));

		if(len < 0 || len >= max_len)
			errorout(E_STR, _T("Failed to concatenate files list"));

		max_len -= len;
		str += len;
	}
}
