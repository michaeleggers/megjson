#ifndef MEGJSON_PARSER_H
#define MEGJSON_PARSER_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MEGJ_MEMORY_SIZE 1024 * 1024 * 1024

typedef struct MegjToken MegjToken;
typedef struct MegjNode MegjNode;

typedef struct MegjMemory
{
    uint32_t size;
    uint32_t used;
    uint8_t * data;
} MegjMemory;

typedef enum MegjType
{
    MEGJ_OBJECT,
    MEGJ_ARRAY,
    MEGJ_NUMBER,
    MEGJ_STRING,
    MEGJ_TRUE,
    MEGJ_FALSE,
    MEGJ_NULL,
    
    MEGJ_OBJECT_CLOSE,
    MEGJ_ARRAY_CLOSE,
    MEGJ_COLON,
    MEGJ_COMMA,

    MEGJ_EOF,
    MEGJ_UNKNOWN_VALUE
} MegjType;

struct MegjToken
{
    MegjType type;
    
    union
    {
//        char name[512]; /* TODO: Is there a max. length of how long a string can be in the json-spec? */
	char * name;
        float f_num;
    } data;
    
    int lineno;
    int size;
};

struct MegjNode
{
    MegjToken token;
    MegjNode * child;
    MegjNode * sibling;
    int count;
    int capacity;
};

typedef struct MegjDocument
{
    MegjNode * tree;
    MegjToken * values;
    int capacity;
    int size;
} MegjDocument;

MegjNode * megj_start();
MegjNode * megj_object();
MegjNode * megj_string_value();
MegjNode * megj_value();
MegjNode * megj_array();
MegjNode * megj_string();
MegjNode * megj_number();
int megj_number2(char * out_number, char * buffer);
MegjNode * megj_true();
MegjNode * megj_false();
MegjNode * megj_null();
void megj_print_ast(MegjNode * root);
void _megj_print_ast(MegjNode * node);
static void _megj_unknown_value(char * message);
void megj_cleanup();
void * megj_malloc(uint32_t size);



#ifdef MEGJ_PARSER_IMPLEMENTATION

static int _megj_g_lineno;

int megj_skipWhitespaces(char * buffer)
{
    int skipped = 0;
    while (*buffer == ' ' || *buffer == '\t') { 
        buffer++;
        skipped++; 
    }
    return skipped;
}

void _megj_print_indent(int indentationCount)
{
    for (int i=0; i<indentationCount; ++i)
        printf(" ");
}

int skipLine(char * buffer)
{
    int skipped = 0;
    while (*buffer == '\n' || *buffer == '\r') { buffer++; skipped++; }
    return skipped;
}

int megj_name(char ** out_name, char * buffer)
{
    char * buffer_start = buffer;
    int length = 0;
    while (*buffer != '"') {
        buffer++;
        length++;
    }
    *out_name = (char*)megj_malloc( (length+1) * sizeof(char) );
    memcpy( (void*)*out_name, (void*)buffer_start, length );
    (*out_name)[length] = '\0';
    return length;
}

int megj_number2(char * out_number, char * buffer)
{
    int length = 0;
    while ( (*buffer >= '0' && *buffer <= '9') || (*buffer == '.')
	    || (*buffer == '+') || (*buffer == '-') ) {
        *out_number++ = *buffer++;
        length++;
    }
    *out_number = '\0';
    return length;
}

int is_number(char * buffer)
{
    if ( (*buffer >= '0' && *buffer <= '9')
        || (*buffer == '+') || (*buffer == '-') ) return 1;
    else return 0;
}

int skip_whitespaces_and_linebreaks(char ** buffer)
{
    int skipped = 0;
    while (**buffer == '\n' || 
           **buffer == '\r' ||
           **buffer == ' ' ||
           **buffer == '\t') { 
        if (**buffer == '\n' || **buffer == '\r') {
            _megj_g_lineno++;
        }
        (*buffer)++;
        skipped++;
    }
    return skipped;
}

int advance_to_next_whitespace(char ** buffer)
{
    int skipped = 0;
    while (**buffer != ' ') {
        (*buffer)++;
        skipped++;
    }
    return skipped;
}

int advance_to_next_char(char ** buffer)
{
    (*buffer)++;
    return skip_whitespaces_and_linebreaks(buffer);
}

int advance_to_next_non_an(char ** buffer)
{
    int skipped = 0;
    while (   (**buffer >= '0' && **buffer <= '9')
           || (**buffer >= 'A' && **buffer <= 'Z')
           || (**buffer >= 'a' && **buffer <= 'z') ) {
        (*buffer)++;
        skipped++;
    }
    return skipped;
}

int check_value_string(char ** buffer, char * valuestring, int valuestring_length)
{
    assert(valuestring_length <= 64); /* TODO: max length of a value-string is 5 ('false'). */
    char * buffer_before = *buffer;
    int advanced = advance_to_next_non_an(buffer);
    char tmp[64+1];
    memcpy( (void*)tmp, (void*)buffer_before, advanced * sizeof(char) );
    tmp[advanced] = '\0';
    
    if ( !strcmp(tmp, valuestring) ) {
	return 1;
    }
    else {
	_megj_unknown_value(tmp);
	return 0;
    }
}

static char * _megj_g_buf;
static int indent;
static MegjToken _megj_g_token;
static MegjMemory _megj_g_memory;

MegjToken megj_get_token()
{
    skip_whitespaces_and_linebreaks(&_megj_g_buf);
    MegjToken token;
    if ( is_number(_megj_g_buf) ) {
	char asciiNumber[32];
	_megj_g_buf += megj_number2(asciiNumber, _megj_g_buf);
	token.type = MEGJ_NUMBER;
	token.data.f_num = atof(asciiNumber);
	advance_to_next_non_an(&_megj_g_buf);
    }
    else {
	switch (*_megj_g_buf)
	{
	case '{':
	{
	    token.type = MEGJ_OBJECT;
	    token.size = 0;
	    _megj_g_buf++;
	} break;
        
	case '}':
	{
	    token.type = MEGJ_OBJECT_CLOSE;
	    token.size = 0;
	    _megj_g_buf++;
	} break;
        
	case '[':
	{
	    token.type = MEGJ_ARRAY;
	    token.size = 0;
	    _megj_g_buf++;
	} break;
        
	case ']':
	{
	    token.type = MEGJ_ARRAY_CLOSE;
	    token.size = 0;
	    _megj_g_buf++;
	} break;
        
	case '"':
	{
	    _megj_g_buf++; // step over opening ' " '
	    token.type = MEGJ_STRING;
	    token.size = 0;
	    _megj_g_buf += megj_name( &(token.data.name), _megj_g_buf );
	    _megj_g_buf++; // step over closing ' " '
	} break;
        
	case 'f':
	{
	    if (check_value_string(&_megj_g_buf, "false", 5)) {		
		token.type = MEGJ_FALSE;
		token.size = 0;
	    }
	    else {
		token.type = MEGJ_UNKNOWN_VALUE;
		token.size = 0;
	    }
	    advance_to_next_non_an(&_megj_g_buf);
	} break;
        
	case 't':
	{
	    if (check_value_string(&_megj_g_buf, "true", 4)) {
		token.type = MEGJ_TRUE;
		token.size = 0;
	    }
	    else {
		token.type = MEGJ_UNKNOWN_VALUE;
		token.size = 0;
	    }
	    advance_to_next_non_an(&_megj_g_buf);
	} break;

	case 'n':
	{
	    if (check_value_string(&_megj_g_buf, "null", 4)) {
		token.type = MEGJ_NULL;
		token.size = 0;
	    }
	    else {
		token.type = MEGJ_UNKNOWN_VALUE;
		token.size = 0;
	    }
	    advance_to_next_non_an(&_megj_g_buf);
	} break;
        
	case ',':
	{
	    token.type = MEGJ_COMMA;
	    token.size = 0;
	    _megj_g_buf++; // step over comma
	    advance_to_next_non_an(&_megj_g_buf);
	} break;
        
	case ':':
	{
	    token.type = MEGJ_COLON;
	    token.size = 0;
	    advance_to_next_whitespace(&_megj_g_buf);
	} break;

	case '\0':
	{
	    token.type = MEGJ_EOF;
	    token.size = 0;
	} break;
	
	default:	   
	{
	    token.type = MEGJ_UNKNOWN_VALUE;
	    token.size = 0;
	    check_value_string(&_megj_g_buf, "", 1);
	}
	}
    }
    skip_whitespaces_and_linebreaks(&_megj_g_buf);
    return token;
}

void _megj_print_token2(MegjToken * token)
{
    switch (token->type)
    {
    case MEGJ_OBJECT:
    {
	printf("OBJECT\n");
    } break;
        
    case MEGJ_OBJECT_CLOSE:
    {
	printf("OBJECT-CLOSE\n");
    } break;
        
    case MEGJ_ARRAY:
    {
	printf("ARRAY\n");
    } break;
        
    case MEGJ_ARRAY_CLOSE:
    {
	printf("ARRAY-CLOSE\n");
    } break;
        
    case MEGJ_STRING:
    {
	printf("STRING: %s\n", token->data.name);
    } break;
        
    case MEGJ_NUMBER:
    {
	printf("NUMBER: %f\n", token->data.f_num);
    } break;
        
    case MEGJ_TRUE:
    {
	printf("TRUE\n");
    } break;
        
    case MEGJ_FALSE:
    {
	printf("FALSE\n");
    } break;

    case MEGJ_NULL:
    {
	printf("NULL\n");	      
    } break;
        
    case MEGJ_COMMA:
    {
	printf(",\n");
    } break;
        
    case MEGJ_COLON:
    {
	printf(":\n");
    } break;

    case MEGJ_EOF:
    {
	printf("EOF\n");
    } break;

    case MEGJ_UNKNOWN_VALUE:
    {
	printf("UNKNOWN VALUE!\n");
    } break;
    }
}

void print_token(MegjToken * token)
{
    switch (token->type)
    {
    case MEGJ_OBJECT:
    {
	_megj_print_indent(indent);
	printf("OBJECT\n");
	indent += 2;
    } break;
        
    case MEGJ_OBJECT_CLOSE:
    {
	indent -= 2;
	_megj_print_indent(indent);
	printf("OBJECT-CLOSE\n");
    } break;
        
    case MEGJ_ARRAY:
    {
	_megj_print_indent(indent);
	printf("ARRAY\n");
	indent += 2;
    } break;
        
    case MEGJ_ARRAY_CLOSE:
    {
	indent -= 2;
	_megj_print_indent(indent);
	printf("ARRAY-CLOSE\n");
    } break;
        
    case MEGJ_STRING:
    {
	_megj_print_indent(indent);
	printf("STRING: %s\n", token->data.name);
    } break;
        
    case MEGJ_NUMBER:
    {
	_megj_print_indent(indent);
	printf("NUMBER: %f\n", token->data.f_num);
    } break;
        
    case MEGJ_TRUE:
    {
	_megj_print_indent(indent);
	printf("TRUE\n");
    } break;
        
    case MEGJ_FALSE:
    {
	_megj_print_indent(indent);
	printf("FALSE\n");
    } break;

    case MEGJ_NULL:
    {
	_megj_print_indent(indent);
	printf("NULL\n");
    } break;
        
    case MEGJ_COMMA:
    {
	_megj_print_indent(indent);
	printf(",\n");
    } break;
        
    case MEGJ_COLON:
    {
	_megj_print_indent(indent);
	printf(":\n");
    } break;

    case MEGJ_EOF:
    {
	printf("EOF\n");
    } break;

    case MEGJ_UNKNOWN_VALUE:
    {
	_megj_print_indent(indent);
	printf("UNKNOWN VALUE!\n");
    } break;
    }
}

void * megj_malloc(uint32_t size)
{
    assert( ((_megj_g_memory.used + size) <= _megj_g_memory.size) && "Out of memory!" );
    void * addr = (void*)(_megj_g_memory.data + _megj_g_memory.used);
    _megj_g_memory.used += size;
    return addr;
}

void megj_cleanup()
{
    free((void*)_megj_g_memory.data);
    _megj_g_memory.used = 0;    
}

MegjNode * new_megj_node()
{
    MegjNode * new_node = (MegjNode *)megj_malloc(sizeof(MegjNode));
    new_node->child = 0;
    new_node->sibling = 0;
    new_node->token = (MegjToken){0};
    return new_node;
}

static void _megj_unknown_value(char * message)
{
    fprintf(stderr, "\n>>> Unknown JSON-value at line %d: %s\n", _megj_g_lineno, message);
}

static void _megj_syntax_error(char * message)
{
    fprintf(stderr, "\n>>> Syntax error at line %d %s", _megj_g_lineno, message);
}

static void match(MegjType expected_token_type)
{
    if (_megj_g_token.type == expected_token_type) {
        _megj_g_token = megj_get_token();
    }
    else {
        _megj_syntax_error("unexpected token -> ");
        print_token(&_megj_g_token);
	abort();
    }
}

MegjNode * megj_object()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_OBJECT);
//    _megj_g_token = megj_get_token();
    /* After object starts, next value-type must be either MEGJ_STRING or MEGJ_NULL */
    if (_megj_g_token.type == MEGJ_STRING) {
	t->child = megj_string_value();
    }
    else { // g_megj_object.type == MEGJ_OBJECT_CLOSE !
	// t->child = megj_null(); /* TODO: should this be undefined or get the json-value 'null'? */
	t->child = 0;	
    }
    MegjNode * p = t->child;
    while (_megj_g_token.type == MEGJ_COMMA) {
        match(MEGJ_COMMA);
        MegjNode * q = megj_string_value();
        p->sibling = q;
        p = q;
    }
    match(MEGJ_OBJECT_CLOSE);
    return t;
}

MegjNode * megj_string_value()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_STRING);
    match(MEGJ_COLON);
    t->child = megj_value();
    return t;
}

MegjNode * megj_value()
{
    MegjNode * t = 0;
    switch(_megj_g_token.type)
    {
        case MEGJ_OBJECT:
        {
            t = megj_object();
        }
        break;
        
        case MEGJ_ARRAY:
        {
            t = megj_array();
        }
        break;
        
        case MEGJ_STRING:
        {
            t = megj_string();
        }
        break;
        
        case MEGJ_NUMBER:
        {
            t = megj_number();
        }
        break;
        
        case MEGJ_TRUE:
        {
            t = megj_true();
        }
        break;
        
        case MEGJ_FALSE:
        {
            t = megj_false();
        }
        break;
        
        case MEGJ_NULL:
        {
	    t = megj_null();
        }
        break;
        
        default:
        {
            _megj_syntax_error("unexpected token -> ");
            print_token(&_megj_g_token);
//            _megj_g_token = megj_get_token();
	    abort();
        }
        break;
    }
    return t;
}

MegjNode * megj_array()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_ARRAY);
//    _megj_g_token = megj_get_token();
    t->child = megj_value();
    MegjNode * p = t->child;
    while (_megj_g_token.type == MEGJ_COMMA) {
        match(MEGJ_COMMA);
        MegjNode * q = megj_value();
        p->sibling = q;
        p = q;
    }
    match(MEGJ_ARRAY_CLOSE);
    return t;
}

MegjNode * megj_string()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_STRING);
    return t;
}

MegjNode * megj_number()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_NUMBER);
    return t;
}

MegjNode * megj_true()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_TRUE);
    return t;
}

MegjNode * megj_false()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_FALSE);
    return t;
}

MegjNode * megj_null()
{
    MegjNode * t = new_megj_node();
    t->token = _megj_g_token;
    match(MEGJ_NULL);
    return t;
}

MegjNode * megj_start()
{
    MegjNode * t = 0;
    switch (*_megj_g_buf) {
    case '{': {
	_megj_g_token = megj_get_token();
	t = megj_object();
    } break;

    case '[': {
	_megj_g_token = megj_get_token();
	t = megj_array();
    } break;
    default: {
	assert("json document must start with object or array!\n");
    }
    }

    return t;
}

MegjDocument megj_parse(char * buffer)
{
    // TODO(Michael): assert buffer
    
#if 0    
    // print buffer to console for debugging
    char * bufferPos = buffer;
    while (*bufferPos != '\0')
    {
        int skipped = megj_skipWhitespaces(bufferPos);
        bufferPos += skipped;
        printf("%c", *bufferPos);
        bufferPos++;
    }
    
    // print tokens to console for debugging
    _megj_g_buf = buffer;
    while (*_megj_g_buf != '\0') {
        _megj_g_token = megj_get_token();
        print_token(&_megj_g_token);
    }
#endif

    /* Initialize Memory */
    {
	_megj_g_memory.data = (uint8_t*)malloc(MEGJ_MEMORY_SIZE);
	memset((uint8_t*)_megj_g_memory.data, 0, MEGJ_MEMORY_SIZE);
	_megj_g_memory.size = MEGJ_MEMORY_SIZE;
	_megj_g_memory.used = 0;
    }
    
    MegjDocument document;
    _megj_g_buf = buffer;
    MegjNode * t = 0;
//    _megj_g_token = megj_get_token();
    skip_whitespaces_and_linebreaks(&_megj_g_buf);
    t = megj_start();
    //_megj_print_ast(t);
    document.tree = t;
    return document;
}

static int _megj_g_indentation;
void _megj_print_ast(MegjNode * tree)
{
    _megj_g_indentation += 2;
    while (tree) {
        _megj_print_indent(_megj_g_indentation);
        MegjToken token = tree->token;
        _megj_print_token2(&token);
        if (tree->child) {
            _megj_print_ast(tree->child);
        }
        tree = tree->sibling;
    }
    _megj_g_indentation -= 2;
}

MegjNode * megj_get_value_by_name(MegjNode * node, char * name)
{
    MegjNode * t = 0;
    if ( !strcmp(node->child->token.data.name, name) ) {
        t = node->child->child;
    }
    else {
        MegjNode * sibling = node->child->sibling;
        while (sibling) {
            if ( !strcmp(sibling->token.data.name, name) ) {
                t = sibling->child;
                break;
            }
            sibling = sibling->sibling;
        }
    }
    return t;
}

MegjNode * megj_get_child(MegjNode * node)
{
    return node->child;
}

MegjNode * megj_get_next_value(MegjNode * node)
{
    return node->sibling;
}

float megj_value_float(MegjNode * node)
{
    if (node->token.type == MEGJ_NUMBER) {
        return node->token.data.f_num;
    }
    else {
        // TODO(Michael): how to handle this error?
    }
    return 0.f;
}

int megj_value_bool(MegjNode * node)
{
    if (node->token.type == MEGJ_TRUE) {
        return 1;
    }
    else if (node->token.type == MEGJ_FALSE) {
        return 0;
    }
    return -1; // TODO(Michael): what to return in error-case?
}

char * megj_value_name(MegjNode * node)
{
    if (node->token.type == MEGJ_STRING) {
        return node->token.data.name;
    }
    else {
        return 0;
    }
}

void megj_print_ast(MegjNode * root)
{
    printf("\n\n>>> AST <<<\n\n");
    int _megj_g_indentation = 0;
    MegjNode * current_node = root;
    while (current_node) {
        MegjToken token = current_node->token;
        if ( (token.type == MEGJ_OBJECT) || (token.type == MEGJ_ARRAY) ) {
            _megj_g_indentation += 2;
        }
        else if ( (token.type == MEGJ_OBJECT_CLOSE) || (token.type == MEGJ_ARRAY_CLOSE) ) {
            _megj_g_indentation -= 2;
        }
        _megj_print_indent(_megj_g_indentation);
        _megj_print_token2(&token);
        current_node = current_node->child;
    }
}

#endif

#endif
