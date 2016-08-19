#pragma once

#include "stdafx.h"

#define CMD_MAX_LEN 8192

enum cmd_err {
	CMD_ERR_SUCCESS, /* no error */
	CMD_ERR_TOO_LONG, /* exceeded maximum command length (CMD_MAX_LEN) */
	CMD_ERR_POPEN_FAILED /* _tpopen() was unsuccessful */
};

enum cmd_err run(int*, LPCTSTR, ...);
enum cmd_err runw(FILE**, LPCTSTR, ...);
