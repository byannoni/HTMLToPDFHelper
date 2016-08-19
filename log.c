
#include "stdafx.h"
#include "log.h"

extern FILE* errorfd = NULL; /* File to log errors */
extern FILE* logfd = NULL; /* File to log information */
extern enum log_level log_filter = /* Maximum verbosity to log */
#ifdef _DEBUG
		kDEBUG
#else
		kNORM
#endif
		;

static LPCTSTR tags[kLL_LAST] = {
	_T("[ERROR] "),
	_T("[MSG  ] "),
	_T("[INFO ] "),
	_T("[DEBUG] ")
};

/*
 * Writes a message to logfd if the given level is greater than
 * log_filter. Returns a non-negative number on success, otherwise EOF.
 */
int writelog(enum log_level level, LPCTSTR format, ...)
{
	int ret = 0;

	RT_NOT_NULL(format);

	if(level <= log_filter) {
		va_list args;

		if(level < kLL_LAST)
			_fputts(tags[level], logfd);
		
		va_start(args, format);

		if(logfd)
			ret = _vftprintf(logfd, format, args);
		else
			ret = EOF;

		va_end(args);
	}

	return ret;
}

/*
 * Writes a message to errorfd if log_filter is greater than kQUIET and
 * exit()s with the given value. This procedure never returns.
 */
void errorout(enum error_code ret, LPCTSTR format, ...)
{
	RT_NOT_NULL(format);

	if(kQUIET < log_filter && errorfd != NULL) {
		va_list args;

		_fputts(tags[kQUIET], errorfd);
		va_start(args, format);

		if(errorfd)
			_vftprintf(errorfd, format, args);

		va_end(args);
		_fputtc(_T('\n'), errorfd);
	}

	exit(ret);
}
