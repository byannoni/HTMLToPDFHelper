
#include "stdafx.h"
#include "cmd.h"
#include "log.h"
#include "util.h"

static enum cmd_err vrun(FILE**, LPCTSTR mode, LPCTSTR, va_list);

/*
 * Constructs a string based on the given format and executes it as a
 * command.* This procedure blocks until the command has finished
 * executing. If pstatus is not NULL, the return value of the command
 * is stored in the value at which pstatus points. The string is
 * constructed by _vsntprintf() and format must not be NULL.
 */
enum cmd_err run(int* pstatus, LPCTSTR format, ...)
{
	FILE* cmdfd = NULL;
	enum cmd_err ret = CMD_ERR_SUCCESS;
	va_list argv;

	va_start(argv, format);
	ret = vrun(&cmdfd, _T("r"), format, argv);
	va_end(argv);

	if(ret == CMD_ERR_SUCCESS) {
		if(cmdfd != NULL) {
			if(pstatus != NULL)
				*pstatus = _pclose(cmdfd);
			else
				_pclose(cmdfd);
		} else {
			ret = CMD_ERR_POPEN_FAILED;
		}
	}

	return ret;
}

/*
 * Constructs a string based on the given format and executes it as a
 * command. This procedure does not block. The value of pcmdfd must not
 * be NULL. A write-only FILE pointer created by _tpopen() is stored in
 * the value pointed to by pcmdfd. This FILE pointer must be passed to
 * _pclose(). The string is constructed by _vsntprintf() and format must
 * not be NULL.
 */
enum cmd_err runw(FILE** pcmdfd, LPCTSTR format, ...)
{
	enum cmd_err ret = CMD_ERR_SUCCESS;
	va_list argv;

	va_start(argv, format);
	ret = vrun(pcmdfd, _T("w"), format, argv);
	va_end(argv);

	return ret;
}

/*
 * Constructs a string based on the given format and executes it as a
 * command. This procedure does not block. The value of pcmdfd must not
 * be NULL. A FILE pointer created by _tpopen() with the given mode
 * string is stored in the value pointed to by pcmdfd. This FILE pointer
 * must be passed to _pclose(). The string is constructed by
 * _vsntprintf() and format must not be NULL.
 */
static enum cmd_err vrun(FILE** pcmdfd, LPCTSTR mode, LPCTSTR format,
		va_list argv)
{
	LPTSTR cmd_line = NULL;
	int string_len = INT_MAX;
	enum cmd_err ret = CMD_ERR_SUCCESS;

	RT_NOT_NULL(pcmdfd);
	RT_NOT_NULL(mode);
	RT_NOT_NULL(format);

	cmd_line = require_vstrf(format, argv);
	string_len = _tcslen(cmd_line);

	if(string_len < CMD_MAX_LEN) {
		writelog(kDEBUG, _T("Running command: '%s'\n"), cmd_line);
		*pcmdfd = _tpopen(cmd_line, mode);

		if(*pcmdfd == NULL)
			ret = CMD_ERR_POPEN_FAILED;
	} else {
		ret = CMD_ERR_TOO_LONG;
	}

	free(cmd_line);
	return ret;
}
