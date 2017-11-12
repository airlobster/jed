// jed.c

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include "utils.h"
#include "queue.h"
#include "jed_parse2doc.h"
#include "jed_select.h"
#include "getoptex.h"
#include "help.h"

typedef enum _operator {
	OP_REF,
	OP_ASSIGN,
	OP_APPEND,
	OP_DELETE
} operator;

static void usage(FILE* os) {
	fputs(help, os);
}

static void process_expression(jed_document* doc, const char* expr, jed_doc_elements_set** set) {
	char* lval=0;
	char* rval=0;
	operator op = OP_REF;
	jed_doc_elements_set* rset = 0;
	jed_document* rdoc = 0;

	// skip leading spaces
	while( isspace(*expr) )
		++expr;
	if( ! *expr )
		return;

	// split expression into l-value, operator and r-value
	const char* pSep = strchr(expr, '=');
	if( pSep ) {
		op = OP_ASSIGN;
		const char* lvalEnd = pSep;
		if( *(pSep-1) == '+' ) {
			op = OP_APPEND;
			--lvalEnd;
		}
		lval = strndup(expr, lvalEnd - expr);
		++pSep;
		while( isspace(*pSep) )
			++pSep;
		rval = strdup(pSep);
	} else if( *expr == '-' ) {
		op = OP_DELETE;
		++expr;
		while( isspace(*expr) )
			++expr;
		lval = strdup(expr);
	} else {
		lval = strdup(expr);
	}

	// select elements from the original doc
	jed_select(doc, lval, op == OP_ASSIGN ? SELECT_OPT_ADDMISSING : 0, set);

	// create a set of elements from the r-value
	if( rval ) {
		rset = jed_doc_create_set();
		if( *rval == '/' ) { // path?
			// select elements from the original document
			if( doc && doc->elements )
				jed_doc_set_append(rset, doc->elements);
			jed_select(doc, rval, 0, &rset);
		} else { // JSON
			// create a JSON document and select elements from it
			rdoc = jed_load_json_s(rval);
			if( rdoc && rdoc->elements )
				jed_doc_set_append(rset, rdoc->elements);
		}
	}

	switch( op ) {
		case OP_REF: {
			LOG("Return %s", lval);
			break;
		}
		case OP_ASSIGN: {
			LOG("Assign %s to %s", rval, lval);
			if( ! rset->nElements )
				break; // nothing to assign
			ASSERT(rset->nElements==1, "Cannot assign from more than one element!");
			if( (*set)->nElements == 0 ) {
				jed_doc_element* root = jed_doc_add_null(0, 0);
				doc->elements = root;
				jed_doc_set_append(*set, root);
			}
			int n;
			for(n=0; n < (*set)->nElements; ++n) {
				jed_doc_element* eold = (*set)->elements[n];
				jed_doc_element* enew = jed_doc_clone_element(0, rset->elements[0]);
				enew->parent = eold->parent;
				if( enew->key )
					free(enew->key);
				enew->key = eold->key ? strdup(eold->key) : 0;
				enew->prev = eold->prev;
				if( eold->prev )
					eold->prev->next = enew;
				enew->next = eold->next;
				if( eold->next )
					eold->next->prev = enew;
				if( doc->elements == eold )
					doc->elements = enew;
				if( eold->parent && eold->parent->v.children == eold )
					eold->parent->v.children = enew;
				jed_doc_element_destroy(eold);
			}
			break;
		}
		case OP_APPEND: {
			LOG("Append %s to %s", rval, lval);
			if( ! rset->nElements )
				break;
			int i;
			for(i=0; i < (*set)->nElements; ++i) {
				jed_doc_element* target = (*set)->elements[i];
				ASSERT(target->type == JDOC_TYPE_ARRAY, "Cannot append to none-array elements!");
				int j;
				for(j=0; j < rset->nElements; ++j) {
					jed_doc_element* source = jed_doc_clone_element(0, rset->elements[j]);
					if( source->key ) {
						free(source->key);
						source->key = 0;
					}
					source->next = 0;
					source->prev = 0;
					source->parent = target;
					if( target->v.children ) {
						// find last child
						jed_doc_element* c = target->v.children;
						while( c->next )
							c = c->next;
						c->next = source;
						source->prev = c;
					} else {
						target->v.children = source;
					}
				}
			}
			break;
		}
		case OP_DELETE: {
			LOG("Delete %s", lval);
			int n;
			for(n=0; n < (*set)->nElements; ++n) {
				jed_doc_element* e = (*set)->elements[n];
				if( e->prev )
					e->prev->next = e->next;
				if( e->next )
					e->next->prev = e->prev;
				if( e->parent && e->parent->v.children == e )
					e->parent->v.children = e->next;
				jed_doc_element_destroy(e);
			}
			break;
		}
	}

	// if original document is modified, the result set is the whole modified document.
	if( op != OP_REF ) {
		jed_doc_reset_set(*set);
		if( doc && doc->elements )
			jed_doc_set_append(*set, doc->elements);
	}

	// cleanup
	if( rset )
		jed_doc_destroy_set(rset);
	if( rdoc )
		jed_doc_destroy(rdoc);
	if( lval )
		free(lval);
	if( rval )
		free(rval);
}

GETOPT_BEGIN(__options)
GETOPT_OPT('e',"expression",required_argument)
GETOPT_OPT('v',"verbose",no_argument)
GETOPT_OPT('h',"help",no_argument)
GETOPT_END()

void process_cmd_option(int c, char* const arg, void* ctx) {
	queue* expressions = (queue*)ctx;
	switch( c ) {
		case 'e': {
			queue_pushtail(expressions, strdup(arg));
			break;
		}
		case 'v': {
			set_verbose(1);
			break;
		}
		case 'h': {
			usage(stdout);
			exit(0);
		}
		default: {
			ASSERT(0, "Unknown option!");
			break;
		}
	}
}

int main(int argc, char* argv[]) {
	int c;
	jed_document* doc = 0;
	jed_doc_elements_set* scope = 0;
	queue* expressions = 0;

	TRACE_MEM("Start");

	scope = jed_doc_create_set();
	expressions = queue_create(free, 0);

	getopt_ex(argc, argv, __options, process_cmd_option, expressions);

	TRACE_MEM("Before load JSON");

	// load JSON from STDIN
	doc = jed_load_json(stdin);

	// create initial search set
	if( doc && doc->elements )
		jed_doc_set_append(scope, doc->elements);

	TRACE_MEM("Before processing");

	// process expressions
	while( queue_length(expressions) ) {
		char* expr = (char*)queue_pophead(expressions);
		process_expression(doc, expr, &scope);
		free(expr);
	}

	TRACE_MEM("After processing");

	if( scope ) {
		jed_doc_print_set(stdout, scope);
		jed_doc_destroy_set(scope);
	}

	jed_doc_destroy(doc);
	queue_destroy(expressions);

	TRACE_MEM("End");

	return 0;
}


