// jed-parse.c

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "stack.h"
#include "utils.h"
#include "dynbuffer.h"
#include "jed_parse.h"

#define	DEF_BUF_SIZE	(1024*32)

typedef enum _jed_parse_state {
	JSTATE_ERROR=-1,
	JSTATE_UNKNOWN=0,
	JSTATE_DETECT=1,
	JSTATE_OBJECT=2,
	JSTATE_ARRAY=3,
	JSTATE_STRING=4,
	JSTATE_NUMERIC=5,
	JSTATE_RESERVED=6,
	JSTATE_KEY=7,
	JSTATE_EATPAIRSEP=8,
	JSTATE_WAIT=9,
	JSTATE_ESCAPE=10
} jed_parse_state;

typedef struct _jed_parser {
	stack* state;
	dynbuffer* buf;
} jed_parser;


static jed_parser* jed_parser_create();
static void jed_parser_destroy(jed_parser* parser);
static void jed_parser_push(jed_parser* parser, jed_parse_state state);
static jed_parse_state jed_parser_pop(jed_parser* parser);
static jed_parse_state jed_parser_peek(jed_parser* parser);
static const char* jed_parse_statename(jed_parse_state st);


const char* jed_parse_statename(jed_parse_state st) {
	const char* names[] = {
		"ERROR",
		"UNKNOWN",
		"DETECT",
		"OBJECT",
		"ARRAY",
		"STRING",
		"NUMERIC",
		"RESERVED",
		"KEY",
		"EATPAIRSEP",
		"WAIT",
		"ESCAPE"
	};
	return names[st+1];
}

jed_parser* jed_parser_create() {
	jed_parser* parser = (jed_parser*)malloc(sizeof(jed_parser));
	parser->state = stack_create();
	parser->buf = dynbuf_create();
	jed_parser_push(parser, JSTATE_UNKNOWN);
	return parser;
}

void jed_parser_destroy(jed_parser* parser) {
	dynbuf_destroy(parser->buf);
	stack_destroy(parser->state);
	free(parser);
}

void jed_parser_push(jed_parser* parser, jed_parse_state state) {
	stack_push(parser->state, (void*)state);
}

jed_parse_state jed_parser_pop(jed_parser* parser) {
	return (jed_parse_state)stack_pop(parser->state);
}

jed_parse_state jed_parser_peek(jed_parser* parser) {
	return (jed_parse_state)stack_peek(parser->state);
}

int jed_parse(FILE* is, void(*handler)(void* ctx, jed_parser_event_type type, const char* buf), void* ctx) {
	jed_parser* parser = jed_parser_create();
	jed_parse_state state = JSTATE_UNKNOWN;
	int done = 0;
	int c;
	while( ! done ) {
		int c = fgetc(is);
		done = c == EOF;
		jed_parse_state state = jed_parser_peek(parser);
		LOG("state=%s, ch=\'%c\'", jed_parse_statename(state), c);
		if( state == JSTATE_ERROR )
			break;
		switch( state ) {
			case JSTATE_UNKNOWN: {
				jed_parser_push(parser, JSTATE_DETECT);
				ungetc(c, is);
				break;
			}
			case JSTATE_DETECT: {
				if( isspace(c) )
					break;
				jed_parser_pop(parser);
				dynbuf_reset(parser->buf);
				if( done ) {
					break;
				} else if( c == '[' ) {
					handler(ctx, JEVENT_BEGIN_ARRAY, 0);
					jed_parser_push(parser, JSTATE_ARRAY);
				} else if( c == '{' ) {
					handler(ctx, JEVENT_BEGIN_OBJECT, 0);
					jed_parser_push(parser, JSTATE_OBJECT);
				} else if( c == '\"' ) {
					jed_parser_push(parser, JSTATE_STRING);
				} else if( strchr("-+0123456789eE.", c) ) {
					ungetc(c, is);
					jed_parser_push(parser, JSTATE_NUMERIC);
				} else if( isalpha(c) ) {
					ungetc(c, is);
					jed_parser_push(parser, JSTATE_RESERVED);
				} else {
					ungetc(c, is);
					handler(ctx, JEVENT_ERROR, 0);
					jed_parser_push(parser, JSTATE_ERROR);
				}
				break;
			}
			case JSTATE_OBJECT: {
				if( done ) {
					break;
				} else if( c == '}' ) {
					handler(ctx, JEVENT_END_OBJECT, 0);
					jed_parser_pop(parser);
				} else if( c == '\"' ) {
					jed_parser_push(parser, JSTATE_KEY);
					dynbuf_reset(parser->buf);
				} else if( c == ',' ) {
					// legit. just skip it
				} else if( isspace(c) ) {
					// legit. just skip it
				} else {
					ungetc(c, is);
					handler(ctx, JEVENT_ERROR, 0);
					jed_parser_push(parser, JSTATE_ERROR);
				}
				break;
			}
			case JSTATE_ARRAY: {
				if( done ) {
					break;
				} else if( c == ']' ) {
					handler(ctx, JEVENT_END_ARRAY, 0);
					jed_parser_pop(parser);
				} else if( c == ',' ) {
					// legit. skip it.
				} else if( ! isspace(c) ) {
					ungetc(c, is);
					jed_parser_push(parser, JSTATE_DETECT);
				}
				break;
			}
			case JSTATE_KEY: {
				if( c == '\"' ) {
					handler(ctx, JEVENT_KEY, dynbuf_str(parser->buf));
					jed_parser_pop(parser);
					jed_parser_push(parser, JSTATE_DETECT);
					jed_parser_push(parser, JSTATE_EATPAIRSEP);
				} else {
					dynbuf_append_char(parser->buf, c);
				}
				break;
			}
			case JSTATE_STRING: {
				if( c == '\"' ) {
					handler(ctx, JEVENT_STRING, dynbuf_str(parser->buf));
					jed_parser_pop(parser);
				} else if( c == '\\' ) {
					dynbuf_append_char(parser->buf, c);
					jed_parser_push(parser, JSTATE_ESCAPE);
				} else {
					dynbuf_append_char(parser->buf, c);
				}
				break;
			}
			case JSTATE_NUMERIC: {
				if( ! strchr("-+0123456789eE.", c) ) {
					const char* v = dynbuf_str(parser->buf);
					handler(ctx, JEVENT_NUMERIC, v);
					jed_parser_pop(parser);
				} else if( ! done ) {
					dynbuf_append_char(parser->buf, c);
				}
				break;
			}
			case JSTATE_RESERVED: {
				if( ! isalpha(c) ) {
					ungetc(c, is);
					const char* s = dynbuf_str(parser->buf);
					if( strcmp(s, "true")==0 ) {
						handler(ctx, JEVENT_TRUE, 0);
					} else if( strcmp(s, "false")==0 ) {
						handler(ctx, JEVENT_FALSE, 0);
					} else if( strcmp(s, "null")==0 ) {
						handler(ctx, JEVENT_NULL, 0);
					} else {
						handler(ctx, JEVENT_ERROR, 0);
						jed_parser_push(parser, JSTATE_ERROR);
					}
					jed_parser_pop(parser);
				} else {
					dynbuf_append_char(parser->buf, c);
				}
				break;
			}
			case JSTATE_ESCAPE: {
				dynbuf_append_char(parser->buf, c);
				jed_parser_pop(parser);
				break;
			}
			case JSTATE_EATPAIRSEP: {
				if(  c == ':' ) {
					jed_parser_pop(parser);
				}
				break;
			}
			default: {
				break;
			}
		}
	}
	jed_parser_destroy(parser);
	return state;
}
