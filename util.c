
#include "stdafx.h"
#include "util.h"
#include "log.h"

/*
 * Allocate memory and terminate execution if it is not successful. The
 * length of the memory is size bytes. The pointer returned must be
 * passed to free().
 */
void* require_mem(size_t size)
{
	void* ret = malloc(size);

	if(ret == NULL)
		errorout(E_MALLOC, _T("Failed to allocate memory"));

	return ret;
}

/*
 * Allocate memory, clear it, and terminate execution if it is not
 * successful. The length of the memory in bytes is the product of nmemb
 * and size. The pointer returned must be passed to free().
 */
void* require_cmem(size_t nmemb, size_t size)
{
	void* ret = calloc(nmemb, size);

	if(ret == NULL)
		errorout(E_MALLOC, _T("Failed to allocate memory"));

	return ret;
}

/*
 * Generate a temporary file name with the given ID and terminate
 * execution if it is not successful. The ID that is used to generate
 * the file is stored in the value pointed to by id. The name that is
 * generated is stored in the buffer pointed to by name. The buffer
 * pointed to by name is assumed to be at least MAX_PATH + 1 in length.
 * The values of id and name must not be NULL. NOTE: the file will be
 * created on disk.
 */
void (require_tmp_file)(LPTSTR name, UINT* id)
{
	UINT l_id = id != NULL ? *id : 0;

	RT_NOT_NULL(name);

	get_tmp_file(name, &l_id);

	if(id != NULL)
		*id = l_id;

	if(l_id == 0)
		errorout(E_TMPF, _T("Failed to acquire a temporary file"));

	writelog(kDEBUG, _T("Required temp file: '%s' (%u) (%#X)\n"), name, l_id,
			l_id);
}

/*
 * Generate a temporary file name with the given ID and. The ID that is
 * used to generate the file is stored in the value pointed to by id.
 * The name that is generated is stored in the buffer pointed to by
 * name. The buffer pointed to by name is assumed to be at least
 * MAX_PATH + 1 in length. The values of id and name must not be NULL.
 * NOTE: the file will be created on disk.
 */
void (get_tmp_file)(LPTSTR name, UINT* id)
{
	UINT l_id = id != NULL ? *id : 0;

	RT_NOT_NULL(name);

	l_id = GetTempFileName(_T("."), _T("H2P"), l_id, name);

	if(id != NULL)
		*id = l_id;

	if(l_id != 0)
		writelog(kDEBUG, _T("Requested temp file: '%s' (%u) (%#X)\n"), name,
			*id, *id);
	else
		writelog(kDEBUG, _T("Failed to acquire a temporary file\n"));
}

/*
 * Open a file with the given name and mode and terminate execution on
 * failure. Neither path nor mode may be NULL. Returns the return value
 * of open_file() if successful.
 */
FILE* (require_open_file)(LPCTSTR path, LPCTSTR mode)
{
	FILE* ret = NULL;

	RT_NOT_NULL(path);
	RT_NOT_NULL(mode);

	writelog(kDEBUG, _T("Opening file '%s' with mode '%s'\n"), path, mode);
	ret = open_file(path, mode);

	if(ret == NULL)
		errorout(E_BADF, _T("Failed to open file"));

	return ret;
}

/*
 * Duplicate a string using dynamic memory and return the pointer to the
 * new string. If it is not successful, it terminates execution. The
 * value of s must not be NULL. The pointer returned by this procedure
 * must be passed to free().
 */
LPTSTR require_dup_str(LPCTSTR s)
{
	RT_NOT_NULL(s);

	return require_strf(_T("%s"), s);
}

/*
 * Allocate a string based on the given format and arguments using
 * dynamic memory and return the pointer to the new string. If it is not
 * successful, execution is terminated. The value of fmt must not be
 * NULL. The pointer returned must be passed to free().
 */
LPTSTR require_strf(LPCTSTR fmt, ...)
{
	LPTSTR ret = NULL;
	va_list args;

	RT_NOT_NULL(fmt);

	va_start(args, fmt);
	ret = require_vstrf(fmt, args);
	va_end(args);

	return ret;
}

/*
 * Allocate a string based on the given format and va_list using dynamic
 * memory and return the pointer to the new string. If it is not
 * successful, execution is terminated. The value of fmt must not be
 * NULL. The pointer returned must be passed to free().
 */
LPTSTR require_vstrf(LPCTSTR fmt, va_list args)
{
	LPTSTR ret = NULL;
	size_t size = 0;
	int orig_size = -1;
	int len = -1;
	va_list args_cp;
	
	RT_NOT_NULL(fmt);

	va_copy(args_cp, args);
	len = _vsntprintf(NULL, 0, fmt, args_cp);
	va_end(args_cp);

	if(len < 0)
		errorout(E_STR, _T("Failed to calculate length requirement"));

	orig_size = len + 1;
	size = orig_size;
	ret = (LPTSTR)require_mem(size * sizeof(*ret));

	if(ret == NULL)
		errorout(E_MALLOC, _T("Failed to allocate memory"));

	len = _vsntprintf(ret, size, fmt, args);

	if(len >= orig_size || len < 0)
		errorout(E_STR, _T("Failed to insert string"));

	return ret;
}

/*
 * Convert the initial part of the given string to an unsigned long int
 * value according to the given base and return the value. If it is not
 * successful, execution is terminated. The value of base must be 0 or
 * between 2 and 36 inclusive. The value of nptr must not be NULL. If
 * endptr is not NULL, it is set to the address of the first invalid
 * character in the given string.
 */
unsigned long int require_strtoul(LPCTSTR nptr, LPTSTR* endptr, int base)
{
	int ret = ULONG_MAX;

	RT_NOT_NULL(nptr);

	errno = 0;
	ret = _tcstoul(nptr, endptr, base);

	if(ret == ULONG_MAX && errno == ERANGE || errno == EINVAL)
		errorout(E_STR, _T("Failed to convert string to number"));

	return ret;
}

/*
 * Convert the initial part of the given string to an double value and
 * return the value. If it is not successful, execution is terminated.
 * The value of nptr must not be NULL. If endptr is not NULL, it is set
 * to the address of the first invalid character in the given string.
 */
double require_strtod(LPCTSTR nptr, LPTSTR* endptr)
{
	LPTSTR end = NULL;
	double ret = 0.0;

	RT_NOT_NULL(nptr);

	errno = 0;
	ret = _tcstod(nptr, &end);

	if((ret == HUGE_VAL || ret == -HUGE_VAL || ret == 0.0) && errno == ERANGE
			|| end == nptr)
		errorout(E_STR, _T("Failed to convert string to number"));

	if(endptr != NULL)
		*endptr = end;

	return ret;
}

/*
 * Produce a file name in the format prefix_YYYYMMDDTHHMMSS.PID.N.txt
 * and return a FILE pointer to it. The return value of this procedure
 * must be passed to release_file.
 */
FILE* require_log(LPCTSTR prefix)
{
	FILE* ret = NULL;
	LPTSTR file_name = NULL;
	size_t len = 0;
	time_t t = time(NULL);
	static unsigned long count = 0;
	TCHAR name_fmt_buf[31] = _T("");

	/* Generates string (example): "%s_20160711T083828.%ld.%lu.txt" */
	if(_tcsftime(name_fmt_buf, LENGTHOF(name_fmt_buf),
			_T("%%s_%Y%m%dT%H%M%S.%%ld.%%lu.txt"), localtime(&t)) == 0)
		errorout(E_STR, _T("Failed to generate time format string"));

	/* Generates string (example): "log_20160711T083828.4236.1.txt" */
	file_name = require_strf(name_fmt_buf, prefix != NULL ? prefix : _T("H2P"),
			GetCurrentProcessId(), ++count);
	ret = require_open_file(file_name, _T("w"));
	free(file_name);
	return ret;
}

/*
 * Reads the remaining data on a line in the given file. The value of
 * file must not be NULL.
 */
void skip_line(FILE* fd)
{
	TCHAR line[BUFSIZ] = _T("");

	RT_NOT_NULL(fd);

	while(_fgetts(line, LENGTHOF(line), fd) != NULL
			&& _tcschr(line, _T('\n')) == NULL)
		;
}

/*
 * Removes the file with the given name. Returns 0 if successful,
 * Otherwise, it returns -1. The value of name must not be NULL.
 */
int (remove_tmp_file)(LPCTSTR name)
{
	int ret = 0;

	RT_NOT_NULL(name);

	writelog(kDEBUG, _T("Removing temp file '%s'\n"), name);
	errno = 0;
	ret = _tremove(name);

	if(ret != 0)
		writelog(kNORM,
				_T("Failed to remove temp file '%s' (status %d): %s\n"), name,
				ret, _tcserror(errno));

	return ret;
}

/*
 * Opens the file at the given path with the given mode. The value
 * returned by this procedure must be closed as a file. The values of
 * Neither path nor mode may be NULL. This procedure returns NULL in
 * case of error.
 */
FILE* (open_file)(LPCTSTR path, LPCTSTR mode)
{
	FILE* ret = NULL;

	RT_NOT_NULL(path);
	RT_NOT_NULL(mode);

	ret = _tfopen(path, mode);
	writelog(kDEBUG, _T("Opened file at 0X%p\n"), ret);

	return ret;
}

/*
 * Opens a file with the given name and mode and returns the pointer to
 * the FILE. This procedure is only for debugging. The values of name,
 * mode, and src must not be NULL. The buffer pointed to by src should
 * be the name of the source file in which the call to this procedure
 * occurred. The value of line should be the number of the line in the
 * file src where the call to this procedure occurred. The value
 * returned by this procedure must be closed as a file. This procedure
 * returns NULL in case of error. See require_open_file().
 */
FILE* require_open_file_dbg(LPCTSTR name, LPCTSTR mode, LPCTSTR src, int line)
{
	FILE* ret = NULL;

	RT_NOT_NULL(name);
	RT_NOT_NULL(mode);
	RT_NOT_NULL(src);

	writelog(kDEBUG, _T("Open file required at %s:%d\n"), src, line);
	ret = (require_open_file)(name, mode);
	return ret;
}

/*
 * Closes the given file. The value of file must not be NULL. This
 * procedure returns 0 on success. It returns EOF in case of error.
 * Regardless of success, accessing the stream represented by file after
 * calling this procedure invokes undefined behavior.
 */
int (release_file)(FILE* file)
{
	RT_NOT_NULL(file);

	writelog(kDEBUG, _T("Closing file at 0X%p\n"), file);
	return fclose(file);
}

/*
 * Opens the given file and returns the pointer to the FILE. This
 * procedure is only for debugging. The values of path, mode, and src
 * must not be NULL. The buffer pointed to by src should be the name of
 * the source file in which the call to this procedure occurred. The
 * value of line should be the number of the line in the file src where
 * the call to this procedure occurred. The value returned by this
 * procedure must be closed as a file. This procedure returns NULL in
 * case of error. See open_file().
 */
FILE* open_file_dbg(LPCTSTR path, LPCTSTR mode, LPCTSTR src, int line)
{
	FILE* ret = NULL;

	RT_NOT_NULL(path);
	RT_NOT_NULL(mode);
	RT_NOT_NULL(src);

	writelog(kDEBUG, _T("Open file requested at %s:%d\n"), src, line);
	ret = (open_file)(path, mode);
	return ret;
}

/*
 * Closes the given file. This procedure is only for debugging. The
 * values of file and src must not be NULL. The buffer pointed to by src
 * should be the name of the source file in which the call to this
 * procedure occurred. The value of line should be the number of the
 * line in the file src where the call to this procedure occurred. This
 * procedure returns 0 on success. It returns EOF in case of error.
 * Regardless of success, accessing the stream represented by file after
 * calling this procedure invokes undefined behavior. See
 * release_file().
 */
int release_file_dbg(FILE* file, LPCTSTR src, int line)
{
	RT_NOT_NULL(file);
	RT_NOT_NULL(src);

	writelog(kDEBUG, _T("Close file reqested at %s:%d\n"), src, line);
	return (release_file)(file);
}

/*
 * Generate a temporary file name with the given ID and terminate
 * execution if it is not successful. This procedure is only for
 * debugging. The ID that is used to generate the file is stored in the
 * value pointed to by id. The name that is generated is stored in the
 * buffer pointed to by name. The buffer pointed to by name is assumed
 * to be at least MAX_PATH + 1 in length. The values of src, id and name
 * must not be NULL. The buffer pointed to by src should be the name of
 * the source file in which the call to this procedure occurred. The
 * value of line should be the number of the line in the file src where
 * the call to this procedure occurred. NOTE: the file will be created
 * on disk. See require_tmp_file().
 */
void require_tmp_file_dbg(LPTSTR name, UINT* id, LPCTSTR src, int line)
{
	RT_NOT_NULL(name);
	RT_NOT_NULL(src);

	writelog(kDEBUG, _T("Temp file required at %s:%d\n"), src, line);
	(require_tmp_file)(name, id);
}

/*
 * Generate a temporary file name with the given ID. This procedure is
 * only for debugging. The ID that is used to generate the file is
 * stored in the value pointed to by id. The name that is generated is
 * stored in the buffer pointed to by name. The buffer pointed to by
 * name is assumed to be at least MAX_PATH + 1 in length. The values of
 * src, id and name must not be NULL. The buffer pointed to by src
 * should be the name of the source file in which the call to this
 * procedure occurred. The value of line should be the number of the
 * line in the file src where the call to this procedure occurred. NOTE:
 * the file will be created on disk. See get_tmp_file().
 */
void get_tmp_file_dbg(LPTSTR name, UINT* id, LPCTSTR src, int line)
{
	RT_NOT_NULL(name);
	RT_NOT_NULL(src);

	writelog(kDEBUG, _T("Temp file requested at %s:%d\n"), src, line);
	(get_tmp_file)(name, id);
}

/*
 * Removes the file with the given name. This procedure is only for
 * debugging. Returns 0 if successful, Otherwise, it returns -1. The
 * values of name and src must not be NULL. The buffer pointed to by src
 * should be the name of the source file in which the call to this
 * procedure occurred. The value of line should be the number of the
 * line in the file src where the call to this procedure occurred. See
 * remove_tmp_file().
 */
void remove_tmp_file_dbg(LPCTSTR name, LPCTSTR src, int line)
{
	RT_NOT_NULL(name);
	RT_NOT_NULL(src);

	writelog(kDEBUG, _T("Temp file removed at %s:%d\n"), src, line);
	(remove_tmp_file)(name);
}
