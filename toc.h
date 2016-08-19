#pragma once 

#include "stdafx.h"

void get_number_of_pages(unsigned long int*, LPCTSTR);
void get_toc_item(LPTSTR, size_t, unsigned long int*, FILE*);
void write_toc_start(FILE*, LPCTSTR, LPCTSTR);
void write_toc_item(FILE*, LPCTSTR, unsigned long int);
void write_toc_end(FILE*);
FILE* open_toc_stream(UINT* outline_pdf_id, const struct pdf_info* info);
