#pragma once

#include "stdafx.h"

enum error_code {
	E_SUCCESS = 0, /* no error */
	E_ARG = 1, /* invalid argument */
	E_MALLOC = 2, /* failure to allocate memory */
	E_NULLPTR = 3, /* null pointer error */
	E_TMPF = 4, /* failure to acquire a temp file */
	E_PDFGETTER = 5, /* pdf getter failure */
	E_CMD = 6, /* failure to execute a command */
	E_BADSEGMENT = 7, /* failure to read instruction segment */
	E_BADF = 8, /* failure to use file */
	E_STR = 9, /* failure to manipulate strings */
	E_PDFMERGER = 10, /* pdf merger failure */
	E_PDF, /* error reading PDF */

	E_LAST
};

enum log_level {
	kQUIET,
	kNORM,
	kVERBOSE,
	kDEBUG,
	kLL_LAST
};

extern FILE* errorfd;
extern FILE* logfd;
extern enum log_level log_filter;

int writelog(enum log_level, LPCTSTR, ...);
void errorout(enum error_code, LPCTSTR, ...);
