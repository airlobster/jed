// jed_parse2doc.h

#ifndef __PARSE2DOC_H_
#define	__PARSE2DOC_H_

#include <stdio.h>
#include "jed_doc.h"


jed_document* jed_load_json(FILE* is);
jed_document* jed_load_json_s(const char* s);

#endif


