// jed_doc.h

#ifndef __JED_DOC_H_
#define	__JED_DOC_H_

#include <stdio.h>

typedef enum _jed_doc_element_type {
//	JDOC_TYPE_DOC,
	JDOC_TYPE_OBJECT,
	JDOC_TYPE_ARRAY,
	JDOC_TYPE_STRING,
	JDOC_TYPE_NUMERIC,
	JDOC_TYPE_TRUE,
	JDOC_TYPE_FALSE,
	JDOC_TYPE_NULL
} jed_doc_element_type;

typedef struct _jed_doc_element {
	struct _jed_doc_element* parent;
	struct _jed_doc_element* next;
	struct _jed_doc_element* prev;
	jed_doc_element_type type;
	char* key;
	union _v {
		struct _jed_doc_element* children;
		char* str;
		int b;
	} v;
} jed_doc_element;

typedef struct __jed_document {
	jed_doc_element* elements;
} jed_document;


jed_document* jed_doc_create();
jed_document* jed_doc_clone(jed_document* doc);
void jed_doc_destroy(jed_document* doc);
void jed_doc_print(FILE* os, jed_document* doc);

void jed_doc_append_element(jed_doc_element* parent, jed_doc_element* e);

jed_doc_element* jed_doc_add_object(jed_doc_element* parent, const char* key);
jed_doc_element* jed_doc_add_array(jed_doc_element* parent, const char* key);
jed_doc_element* jed_doc_add_string(jed_doc_element* parent, const char* key, const char* str);
jed_doc_element* jed_doc_add_numeric(jed_doc_element* parent, const char* key, const char* d);
jed_doc_element* jed_doc_add_boolean(jed_doc_element* parent, const char* key, int b);
jed_doc_element* jed_doc_add_null(jed_doc_element* parent, const char* key);

jed_doc_element* jed_doc_clone_element(jed_doc_element* parent, jed_doc_element* e);

void jed_doc_element_destroy(jed_doc_element* element);

// set

typedef struct _jed_doc_elements_set {
	jed_doc_element** elements;
	int maxElements;
	int nElements;
} jed_doc_elements_set;


jed_doc_elements_set* jed_doc_create_set();
void jed_doc_destroy_set(jed_doc_elements_set* set);
void jed_doc_reset_set(jed_doc_elements_set* set);
void jed_doc_set_append(jed_doc_elements_set* set, jed_doc_element* e);
jed_doc_elements_set* jed_doc_set_enum(jed_doc_elements_set* set, int(*condition)(jed_doc_element* e, void* data), void* data);
jed_doc_elements_set* jed_doc_set_enum_children(jed_doc_elements_set* set, int(*condition)(jed_doc_element* e, void* data), void* data);
void jed_doc_print_set(FILE* os, jed_doc_elements_set* set);

#endif

