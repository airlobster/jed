// jed_doc.c

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "utils.h"
#include "jed_doc.h"


typedef struct _doc_print_context {
	int nest;
	const char* linesep;
	const char* indent;
} doc_print_context;


static jed_doc_element* jed_doc_element_create(jed_doc_element* parent, jed_doc_element_type type, const char* key);
static void jed_doc_print_element_children(FILE* os, jed_doc_element* e, doc_print_context* ctx);
static void jed_doc_print_element(FILE* os, jed_doc_element* e, doc_print_context* ctx);



jed_document* jed_doc_create() {
	jed_document* doc = (jed_document*)malloc(sizeof(jed_document));
	doc->elements = 0;
	return doc;
}

jed_document* jed_doc_clone(jed_document* doc) {
	jed_document* nd = jed_doc_create();
	doc->elements = jed_doc_clone_element(0, doc->elements);
	return nd;
}

void jed_doc_destroy(jed_document* doc) {
	if( doc->elements )
		jed_doc_element_destroy(doc->elements);
	free(doc);
}

jed_doc_element* jed_doc_element_create(jed_doc_element* parent, jed_doc_element_type type, const char* key) {
//	ASSERT(!parent || (!key || parent->type==JDOC_TYPE_OBJECT), "Cannot add element with a key to a non-object element");
	jed_doc_element* e = (jed_doc_element*)malloc(sizeof(jed_doc_element));
	e->type = type;
	e->parent = 0;
	e->next = 0;
	e->prev = 0;
	e->key = 0;
	if( key )
		e->key = strdup(key);
	if( parent )
		jed_doc_append_element(parent, e);
	return e;
}

void jed_doc_append_element(jed_doc_element* parent, jed_doc_element* e) {
	ASSERT(parent->type == JDOC_TYPE_OBJECT || parent->type == JDOC_TYPE_ARRAY,
		"Cannot add childs to a none-container element");
	ASSERT(parent->type != JDOC_TYPE_OBJECT || e->key, "Object's children must have a key");
	e->parent = parent;
	e->prev = 0;
	e->next = 0;
	if( parent->v.children ) {
		jed_doc_element* c = parent->v.children;
		while( c->next )
			c = c->next;
		c->next = e;
		e->prev = c;
	} else {
		parent->v.children = e;
	}
}

jed_doc_element* jed_doc_add_object(jed_doc_element* parent, const char* key) {
	jed_doc_element* e = jed_doc_element_create(parent, JDOC_TYPE_OBJECT, key);
	e->v.children = 0;
	return e;
}
jed_doc_element* jed_doc_add_array(jed_doc_element* parent, const char* key) {
	jed_doc_element* e = jed_doc_element_create(parent, JDOC_TYPE_ARRAY, key);
	e->v.children = 0;
	return e;
}
jed_doc_element* jed_doc_add_string(jed_doc_element* parent, const char* key, const char* str) {
	jed_doc_element* e = jed_doc_element_create(parent, JDOC_TYPE_STRING, key);
	if( str )
		e->v.str = strdup(str);
	else
		e->v.str = 0;
	return e;
}
jed_doc_element* jed_doc_add_numeric(jed_doc_element* parent, const char* key, const char* d) {
	jed_doc_element* e = jed_doc_element_create(parent, JDOC_TYPE_NUMERIC, key);
	if( d )
		e->v.str = strdup(d);
	else
		e->v.str = 0;
	return e;
}
jed_doc_element* jed_doc_add_boolean(jed_doc_element* parent, const char* key, int b) {
	jed_doc_element* e = jed_doc_element_create(parent, b ? JDOC_TYPE_TRUE : JDOC_TYPE_FALSE, key);
	e->v.b = b;
	return e;
}
jed_doc_element* jed_doc_add_null(jed_doc_element* parent, const char* key) {
	jed_doc_element* e = jed_doc_element_create(parent, JDOC_TYPE_NULL, key);
	e->v.children = 0;
	return e;
}

jed_doc_element* jed_doc_clone_element(jed_doc_element* parent, jed_doc_element* e) {
	jed_doc_element* enew = 0;
	switch( e->type ) {
		case JDOC_TYPE_OBJECT: {
			enew = jed_doc_add_object(parent, e->key);
			jed_doc_element* c = e->v.children;
			while( c ) {
				jed_doc_clone_element(enew, c);
				c = c->next;
			}
			break;
		}
		case JDOC_TYPE_ARRAY: {
			enew = jed_doc_add_array(parent, e->key);
			jed_doc_element* c = e->v.children;
			while( c ) {
				jed_doc_clone_element(enew, c);
				c = c->next;
			}
			break;
		}
		case JDOC_TYPE_STRING: {
			enew = jed_doc_add_string(parent, e->key, e->v.str);
			break;
		}
		case JDOC_TYPE_NUMERIC: {
			enew = jed_doc_add_numeric(parent, e->key, e->v.str);
			break;
		}
		case JDOC_TYPE_FALSE:
		case JDOC_TYPE_TRUE: {
			enew = jed_doc_add_boolean(parent, e->key, e->v.b);
			break;
		}
		case JDOC_TYPE_NULL: {
			enew = jed_doc_add_null(parent, e->key);
			break;
		}
	}
	return enew;
}

void jed_doc_element_destroy(jed_doc_element* element) {
	switch( element->type ) {
		case JDOC_TYPE_OBJECT:
		case JDOC_TYPE_ARRAY: {
			while( element->v.children ) {
				jed_doc_element* next = element->v.children->next;
				jed_doc_element_destroy(element->v.children);
				element->v.children = next;
			}
			break;
		}
		case JDOC_TYPE_NUMERIC:
		case JDOC_TYPE_STRING: {
			if( element->v.str )
				free(element->v.str);
			break;
		}
		default: {
			break;
		}
	}
	if( element->key )
		free(element->key);
	free(element);
}

void jed_doc_print(FILE* os, jed_document* doc) {
	doc_print_context ctx;
	ctx.nest = 0;
	ctx.linesep = "\n";
	ctx.indent = "\t";
	if( doc->elements )
		jed_doc_print_element(os, doc->elements, &ctx);
}

static void jed_doc_print_element_children(FILE* os, jed_doc_element* e, doc_print_context* ctx) {
	if( ctx )
		++ctx->nest;
	jed_doc_element* c = e->v.children;
	while( c ) {
		jed_doc_print_element(os, c, ctx);
		c = c->next;
		if( c )
			fprintf(os, ",");
	}
	if( ctx )
		--ctx->nest;
}

static void jed_doc_print_element(FILE* os, jed_doc_element* e, doc_print_context* ctx) {
	// print keys only for sub-elements
	if( e->key && (!ctx || ctx->nest != 0) )
		fprintf(os, "\"%s\":", e->key);
	switch( e->type ) {
		case JDOC_TYPE_OBJECT: {
			fprintf(os, "{");
			jed_doc_print_element_children(os, e, ctx);
			fprintf(os, "}");
			break;
		}
		case JDOC_TYPE_ARRAY: {
			fprintf(os, "[");
			jed_doc_print_element_children(os, e, ctx);
			fprintf(os, "]");
			break;
		}
		case JDOC_TYPE_STRING: {
			fprintf(os, "\"%s\"", e->v.str);
			break;
		}
		case JDOC_TYPE_NUMERIC: {
			fprintf(os, "%s", e->v.str);
			break;
		}
		case JDOC_TYPE_TRUE: {
			fprintf(os, "true");
			break;
		}
		case JDOC_TYPE_FALSE: {
			fprintf(os, "false");
			break;
		}
		case JDOC_TYPE_NULL: {
			fprintf(os, "null");
			break;
		}
	}
}

#define	INITIAL_SET_SIZE	(20)

jed_doc_elements_set* jed_doc_create_set() {
	jed_doc_elements_set* s = (jed_doc_elements_set*)malloc(sizeof(jed_doc_elements_set));
	s->elements = (jed_doc_element**)malloc(sizeof(jed_doc_element*)*INITIAL_SET_SIZE);
	s->maxElements = INITIAL_SET_SIZE;
	s->nElements = 0;
	return s;
}

void jed_doc_destroy_set(jed_doc_elements_set* set) {
	int n;
	free(set->elements);
	free(set);
}

void jed_doc_reset_set(jed_doc_elements_set* set) {
	set->nElements = 0;
}

void jed_doc_set_append(jed_doc_elements_set* set, jed_doc_element* e) {
	if( set->nElements == set->maxElements ) {
		set->maxElements += INITIAL_SET_SIZE;
		set->elements = (jed_doc_element**)realloc(set->elements, sizeof(jed_doc_element*)*set->maxElements);
	}
	set->elements[set->nElements++] = e;
}

jed_doc_elements_set* jed_doc_set_enum(jed_doc_elements_set* set, int(*condition)(jed_doc_element* e, void* data), void* data) {
	int i;
	jed_doc_elements_set* out = jed_doc_create_set();
	for(i=0; i < set->nElements; ++i) {
		if( !condition || condition(set->elements[i], data) )
			jed_doc_set_append(out, set->elements[i]);
	}
	return out;
}

jed_doc_elements_set* jed_doc_set_enum_children(jed_doc_elements_set* set, int(*condition)(jed_doc_element* e, void* data), void* data) {
	int i;
	jed_doc_elements_set* out = jed_doc_create_set();
	for(i=0; i < set->nElements; ++i) {
		jed_doc_element* e = set->elements[i];
		if( e->type != JDOC_TYPE_OBJECT && e->type != JDOC_TYPE_ARRAY )
			continue;
		jed_doc_element* c = e->v.children;
		while( c ) {
			if( !condition || condition(c, data) )
				jed_doc_set_append(out, c);
			c = c->next;
		}
	}
	return out;
}

void jed_doc_print_set(FILE* os, jed_doc_elements_set* set) {
	doc_print_context ctx = {0};
	int n;
	for(n=0; n < set->nElements; ++n) {
		jed_doc_print_element(os, set->elements[n], &ctx);
		fprintf(os, "\n");
	}
}

