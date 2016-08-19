
#pragma once

#include "stdafx.h"

#ifdef _DEBUG
#include "log.h"

#define require_tmp_file(name, id) \
	require_tmp_file_dbg((name), (id), _T(__FILE__), __LINE__)

#define get_tmp_file(name, id) \
	get_tmp_file_dbg((name), (id), _T(__FILE__), __LINE__)

#define remove_tmp_file(name) \
	remove_tmp_file_dbg((name), _T(__FILE__), __LINE__)

#define require_open_file(name, mode) \
	require_open_file_dbg((name), (mode), _T(__FILE__), __LINE__)

#define release_file(file) release_file_dbg((file), _T(__FILE__), __LINE__)

#define open_file(path, mode) \
	open_file_dbg((path), (mode), _T(__FILE__), __LINE__)
#endif

#ifdef KEEP_TMP_FILES
#ifdef remove_tmp_file
#undef remove_tmp_file
#endif
#define remove_tmp_file(name)
#endif

void* require_mem(size_t);
void* require_cmem(size_t, size_t);
void (require_tmp_file)(LPTSTR, UINT*);
void (get_tmp_file)(LPTSTR, UINT*);
FILE* (require_open_file)(LPCTSTR, LPCTSTR);
LPTSTR require_dup_str(LPCTSTR);
LPTSTR require_strf(LPCTSTR, ...);
LPTSTR require_vstrf(LPCTSTR, va_list);
unsigned long int require_strtoul(LPCTSTR, LPTSTR*, int);
double require_strtod(LPCTSTR, LPTSTR*);
FILE* require_log(LPCTSTR);
void skip_line(FILE*);
int (remove_tmp_file)(LPCTSTR);
int (release_file)(FILE*);
FILE* (open_file)(LPCTSTR, LPCTSTR);

void get_tmp_file_dbg(LPTSTR, UINT*, LPCTSTR, int);
FILE* require_open_file_dbg(LPCTSTR, LPCTSTR, LPCTSTR, int);
FILE* open_file_dbg(LPCTSTR, LPCTSTR, LPCTSTR, int);
int release_file_dbg(FILE*, LPCTSTR, int);
void require_tmp_file_dbg(LPTSTR, UINT*, LPCTSTR, int);
void remove_tmp_file_dbg(LPCTSTR, LPCTSTR, int);