// jed_path_parse.c

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <fnmatch.h>
#include "utils.h"
#include "stack.h"
#include "jed_path_parse.h"

////////////////////////////////////////////////////////////////////////////////////

typedef enum _jed_doc_path_state {
	JPATH_STATE_BEGIN,
	JPATH_STATE_TOKEN,
	JPATH_STATE_RECURSE,
	JPATH_STATE_KEY,
	JPATH_STATE_PREDICATE,
	JPATH_STATE_INDEXFROM,
	JPATH_STATE_INDEXTO,
	JPATH_STATE_ENDINDEXRANGE,
	JPATH_STATE_SKIPSRANGESEP,
	JPATH_STATE_VALUEMASK,
	JPATH_STATE_SKIPSPACES,
} jed_doc_path_state;


int jed_path_parse(const char* path, path_handler const* handler, void* ctx) {
	const char* tokstart=0;
	const char* p = path;
	index_range range = {0};
	int done = 0;
	stack* state = stack_create();
	stack_push(state, (void*)JPATH_STATE_BEGIN);
	stack_push(state, (void*)JPATH_STATE_SKIPSPACES);
	while( ! done ) {
		done = !*p;
		if( ! stack_size(state) )
			break;
		jed_doc_path_state st = (jed_doc_path_state)stack_peek(state);
		LOG("state=%d, left=%s", st, p);
		switch( st ) {
			case JPATH_STATE_BEGIN: {
				if( *p == '/' ) {
					stack_pop(state);
					stack_push(state, (void*)JPATH_STATE_TOKEN);
				} else {
					done = 1;
				}
				--p;
				break;
			}
			case JPATH_STATE_TOKEN: {
				if( !*p )
					break;
				if( *p == '/' ) {
					stack_push(state, (void*)JPATH_STATE_RECURSE);
				} else if( *p == '[' ) {
					if( handler->onBeginPredicate )
						handler->onBeginPredicate(ctx);
					stack_push(state, (void*)JPATH_STATE_PREDICATE);
					stack_push(state, (void*)JPATH_STATE_SKIPSPACES);
				} else {
					tokstart = p;
					--p;
					stack_push(state, (void*)JPATH_STATE_KEY);
				}
				break;
			}
			case JPATH_STATE_RECURSE: {
				stack_pop(state);
				if( *p == '/' ) {
					if( handler->onLevelRecursive )
						handler->onLevelRecursive(ctx);
				} else {
					if( handler->onLevel )
						handler->onLevel(ctx);
					--p;
				}
				break;
			}
			case JPATH_STATE_KEY: {
				if( !*p || *p == '/' || *p == '[' ) {
					char* k = strndup(tokstart, p - tokstart);
					if( handler->onKey )
						handler->onKey(k, ctx);
					free(k);
					--p;
					stack_pop(state);
				}
				break;
			}
			case JPATH_STATE_PREDICATE: {
				if( *p == ']' ) {
					if( handler->onEndPredicate )
						handler->onEndPredicate(ctx);
					stack_pop(state);
				} else if( isdigit(*p) ) {
					stack_push(state, (void*)JPATH_STATE_ENDINDEXRANGE);
					stack_push(state, (void*)JPATH_STATE_SKIPSPACES);
					stack_push(state, (void*)JPATH_STATE_SKIPSRANGESEP);
					stack_push(state, (void*)JPATH_STATE_SKIPSPACES);
					stack_push(state, (void*)JPATH_STATE_INDEXFROM);
					--p;
					range.from = range.to = 0;
				} else if( *p == '~' ) {
					stack_push(state, (void*)JPATH_STATE_VALUEMASK);
					tokstart = p + 1;
				} else {
					ASSERT(0, "Invalid predicate!");
				}
				break;
			}
			case JPATH_STATE_INDEXFROM: {
				if( isdigit(*p) ) {
					range.from = range.from * 10 + *p - '0';
					range.to = range.from;
				} else {
					stack_pop(state);
					--p;
				}
				break;
			}
			case JPATH_STATE_INDEXTO: {
				if( isdigit(*p) ) {
					if( range.to < 0 )
						range.to = 0;
					range.to = range.to * 10 + *p - '0';
				} else {
					stack_pop(state);
					--p;
				}
				break;
			}
			case JPATH_STATE_SKIPSRANGESEP: {
				stack_pop(state);
				if( *p == '-' ) {
					stack_push(state, (void*)JPATH_STATE_INDEXTO);
					range.to = -1;
				} else {
					--p;
				}
				break;
			}
			case JPATH_STATE_ENDINDEXRANGE: {
				if( range.to < range.from )
					range.to = INT_MAX;
				if( handler->onIndexRange )
					handler->onIndexRange(&range, ctx);
				stack_pop(state);
				--p;
				break;
			}
			case JPATH_STATE_VALUEMASK: {
				if( *p == ']' ) {
					char* mask = strndup(tokstart, p - tokstart);
					if( handler->onValueMask )
						handler->onValueMask(mask, ctx);
					if( handler->onEndPredicate )
						handler->onEndPredicate(ctx);
					free(mask);
					stack_pop(state);
					--p;
				}
				break;
			}
			case JPATH_STATE_SKIPSPACES: {
				if( ! isspace(*p) ) {
					--p;
					stack_pop(state);
				}
				break;
			}
		}
		if( p >= path && !*p )
			break;
		++p;
	}
	stack_destroy(state);
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////


