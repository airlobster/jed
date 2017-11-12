// jed_select.h

#ifndef __JED_SELECT_H_
#define	__JED_SELECT_H_

#include "jed_doc.h"


typedef enum _jed_select_options {
	SELECT_OPT_ADDMISSING=0x1
} jed_select_options;


int jed_select(jed_document* doc, const char* path, unsigned long opt, jed_doc_elements_set** ppResults);

#endif

