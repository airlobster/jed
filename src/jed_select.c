// jed_select.c

#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include <fnmatch.h>
#include "utils.h"
#include "stack.h"
#include "jed_select.h"
#include "jed_path_parse.h"
#include "jed_parse2doc.h"


typedef struct _select_context {
	unsigned long opt;
	jed_doc_elements_set** ppResults;
} select_context;

////////////////////////////////////////////////////////////////////////////////////

static int iswildcard(char const* s) {
	while( *s ) {
		if( *s == '!' || *s == '*' || *s == '?' )
			return 1;
		++s;
	}
	return 0;
}

static int match(const char* pattern, const char* str) {
	int neg = 0;
	while( isspace(*pattern) )
		++pattern;
	if( *pattern == '!' ) {
		neg = 1;
		++pattern;
		while( isspace(*pattern) )
			++pattern;
	}
	int match = fnmatch(pattern, str, 0) == 0;
	return neg ? ! match : match;
}

static int condition_keymatch(jed_doc_element* e, void* data) {
	const char* pattern = (const char*)data;
	if( ! e->key )
		return 0;
	return match(pattern, e->key);
}

static int condition_indexrange(jed_doc_element* e, void* data) {
	ASSERT(e->parent && e->parent->type==JDOC_TYPE_ARRAY, "Cannot index on none-array elements!");
	const index_range* range = (const index_range*)data;
	int i=0;
	jed_doc_element* c = e->parent->v.children;
	while( c ) {
		if( c == e )
			return i >= range->from && i <= range->to;
		++i;
		c = c->next;
	}
	return 0;
}

static int condition_valuemask(jed_doc_element* e, void* data) {
	const char* mask = (const char*)data;
	switch( e->type ) {
		case JDOC_TYPE_NUMERIC:
		case JDOC_TYPE_STRING: {
			return match(mask, e->v.str);
		}
		case JDOC_TYPE_TRUE: {
			return match(mask, "true");
		}
		case JDOC_TYPE_FALSE: {
			return match(mask, "false");
		}
		default: {
			break;
		}
	}
	return 0;
}

static void jed_path_expand_set(jed_doc_elements_set** ppset) {
	jed_doc_elements_set* out = jed_doc_create_set();
	int n;
	for(n=0; n < (*ppset)->nElements; ++n) {
		jed_doc_element* e = (*ppset)->elements[n];
		if( e->type != JDOC_TYPE_OBJECT && e->type != JDOC_TYPE_ARRAY )
			continue;
		jed_doc_element* child = e->v.children;
		while( child ) {
			jed_doc_set_append(out, child);
			child = child->next;
		}
	}
	jed_doc_destroy_set(*ppset);
	*ppset = out;
}

static void add_new_keyed_element(jed_doc_elements_set* siblings, jed_doc_elements_set* set, char const* key) {
	TRACE("Adding key \"%s\"", key);
	int n;
	for(n=0; n < siblings->nElements; ++n) {
		jed_doc_element* parent = siblings->elements[n]->parent;
		if( ! parent )
			break;
		ASSERT(parent->type == JDOC_TYPE_OBJECT, "Cannot add keys to a none-object element!");
		jed_doc_element* enew = jed_doc_add_null(parent, key);
		jed_doc_set_append(set, enew);
		break;
	}
}

static void add_index(jed_doc_elements_set* set, int index) {
	TRACE("Adding index \"%d\"", index);
}

////////////////////////////////////////////////////////////////////////////////////

static void onPathLevel(void* ctx) {
	select_context const* selctx = (select_context const*)ctx;
	jed_path_expand_set(selctx->ppResults);
}

static void onPathLevelRecursive(void* ctx) {
	select_context const* selctx = (select_context const*)ctx;
	ASSERT(0, "Recursive search is not implemented yet!");
}

static void onPathKey(char const* key, void* ctx) {
	select_context const* selctx = (select_context const*)ctx;
	jed_doc_elements_set* out = jed_doc_set_enum(*selctx->ppResults, condition_keymatch, (void*)key);
	if( ! out->nElements && (selctx->opt & SELECT_OPT_ADDMISSING) && ! iswildcard(key) )
		add_new_keyed_element(*selctx->ppResults, out, key);	
	jed_doc_destroy_set(*selctx->ppResults);
	*selctx->ppResults = out;
}

static void onPathIndexRange(index_range const* range, void* ctx) {
	select_context const* selctx = (select_context const*)ctx;
	jed_doc_elements_set* out = jed_doc_set_enum_children(*selctx->ppResults, condition_indexrange, (void*)range);
	if( ! out->nElements && (selctx->opt & SELECT_OPT_ADDMISSING) ) {
	}
	jed_doc_destroy_set(*selctx->ppResults);
	*selctx->ppResults = out;
}

static void onPathValueMask(char const* mask, void* ctx) {
	select_context const* selctx = (select_context const*)ctx;
	jed_doc_elements_set* out = jed_doc_set_enum_children(*selctx->ppResults, condition_valuemask, (void*)mask);
	jed_doc_destroy_set(*selctx->ppResults);
	*selctx->ppResults = out;
}


int jed_select(jed_document* doc, const char* path, unsigned long opt, jed_doc_elements_set** ppResults) {
	select_context ctx = { opt, ppResults };
	path_handler handler = {0};
	handler.onLevel = onPathLevel;
	handler.onLevelRecursive = onPathLevelRecursive;
	handler.onKey = onPathKey;
	handler.onIndexRange = onPathIndexRange;
	handler.onValueMask = onPathValueMask;
	return jed_path_parse(path, &handler, &ctx);
}

////////////////////////////////////////////////////////////////////////////////////


