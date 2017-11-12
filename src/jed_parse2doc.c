// jed_parse2doc.h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "jed_parse2doc.h"
#include "jed_parse.h"

typedef struct _context {
	jed_doc_element* root;
	jed_doc_element* currContainer;
	char* key;
} context;

static void context_delete_key(context* ctx) {
	if( ctx->key ) {
		free(ctx->key);
 		ctx->key = 0;
	}
}

static void onJsonEvent(void* ctx, jed_parser_event_type type, const char* buf) {
	context* x = (context*)ctx;
	switch( type ) {
		case JEVENT_ERROR: {
			break;
		}
		case JEVENT_BEGIN_OBJECT: {
			x->currContainer = jed_doc_add_object(x->currContainer, x->key);
			if( ! x->root )
				x->root = x->currContainer;
			context_delete_key(x);
			break;
		}
		case JEVENT_END_OBJECT: {
			x->currContainer = x->currContainer->parent;
			break;
		}
		case JEVENT_BEGIN_ARRAY: {
			x->currContainer = jed_doc_add_array(x->currContainer, x->key);
			if( ! x->root )
				x->root = x->currContainer;
			context_delete_key(x);
			break;
		}
		case JEVENT_END_ARRAY: {
			x->currContainer = x->currContainer->parent;
			break;
		}
		case JEVENT_KEY: {
			x->key = strdup(buf);
			break;
		}
		case JEVENT_STRING: {
			jed_doc_element* e = jed_doc_add_string(x->currContainer, x->key, buf);
			if( ! x->root )
				x->root = e;
			context_delete_key(x);
			break;
		}
		case JEVENT_NUMERIC: {
			jed_doc_element* e = jed_doc_add_numeric(x->currContainer, x->key, buf);
			if( ! x->root )
				x->root = e;
			context_delete_key(x);
			break;
		}
		case JEVENT_TRUE: {
			jed_doc_element* e = jed_doc_add_boolean(x->currContainer, x->key, 1);
			if( ! x->root )
				x->root = e;
			context_delete_key(x);
			break;
		}
		case JEVENT_FALSE: {
			jed_doc_element* e = jed_doc_add_boolean(x->currContainer, x->key, 0);
			if( ! x->root )
				x->root = e;
			context_delete_key(x);
			break;
		}
		case JEVENT_NULL: {
			jed_doc_element* e = jed_doc_add_null(x->currContainer, x->key);
			if( ! x->root )
				x->root = e;
			context_delete_key(x);
			break;
		}
	}
}

jed_document* jed_load_json(FILE* is) {
	context ctx = {0};
	jed_document* doc = jed_doc_create();
	ctx.currContainer = doc->elements;
	if( jed_parse(is, onJsonEvent, &ctx) < 0 ) {
		jed_doc_destroy(doc);
		ASSERT(0, "Failed!");
	}
	doc->elements = ctx.root;
	return doc;
}

jed_document* jed_load_json_s(const char* s) {
	jed_document* doc = 0;
	FILE* f = fmemopen((void*)s, strlen(s), "r");
	doc = jed_load_json(f);
	fclose(f);
	return doc;
}

