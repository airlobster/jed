// jed_path_parse.h

#ifndef __JED_PATH_PARSE_H_
#define	__JED_PATH_PARSE_H_

#include "jed_doc.h"


typedef struct _index_range {
	int from;
	int to;
} index_range;


typedef struct _path_handler {
	void(*onLevel)(void* ctx);
	void(*onKey)(char const* mask, void* ctx);
	void(*onBeginPredicate)(void* ctx);
	void(*onEndPredicate)(void* ctx);
	void(*onIndexRange)(index_range const* range, void* ctx);
	void(*onValueMask)(char const* mask, void* ctx);
} path_handler;


int jed_path_parse(const char* path, path_handler const* handler, void* ctx);

#endif

