
#pragma once

#include "stdafx.h"

extern LPCTSTR pdf_merger_exe; /* path to PDF merge utility */

void do_merge_pdfs(LPCTSTR, LPCTSTR, LPCTSTR, size_t, UINT*,UINT);
