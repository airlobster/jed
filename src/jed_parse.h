// jed_parse.h

#ifndef __JED_PARSE_H_
#define	__JED_PARSE_H_

#include <stdio.h>
#include "jed_doc.h"


typedef enum _jed_parser_event_type {
	JEVENT_ERROR,
	JEVENT_BEGIN_OBJECT,
	JEVENT_END_OBJECT,
	JEVENT_BEGIN_ARRAY,
	JEVENT_END_ARRAY,
	JEVENT_KEY,
	JEVENT_STRING,
	JEVENT_NUMERIC,
	JEVENT_TRUE,
	JEVENT_FALSE,
	JEVENT_NULL
} jed_parser_event_type;


int jed_parse(FILE* is, void(*handler)(void* ctx, jed_parser_event_type type, const char* buf), void* ctx);

#endif
