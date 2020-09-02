#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

#define MEGJ_MEMORY_SIZE 500 * 1024 * 1024

typedef struct JsonToken JsonToken;
typedef struct JsonNode JsonNode;

typedef struct JsonMemory
{
    uint32_t size;
    uint32_t used;
    uint8_t * data;
} JsonMemory;

typedef enum JsonType
{
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_NUMBER,
    JSON_STRING,
    JSON_TRUE,
    JSON_FALSE,
    JSON_NULL,
    
    JSON_OBJECT_CLOSE,
    JSON_ARRAY_CLOSE,
    JSON_COLON,
    JSON_COMMA,

    JSON_EOF,
    JSON_UNKNOWN_VALUE
} JsonType;

struct JsonToken
{
    JsonType type;
    
    union
    {
//        char name[512]; /* TODO: Is there a max. length of how long a string can be in the json-spec? */
	char * name;
        float f_num;
    } data;
    
    int lineno;
    int size;
};

struct JsonNode
{
    JsonToken token;
    JsonNode * child;
    JsonNode * sibling;
    int count;
    int capacity;
};

typedef struct JsonDocument
{
    JsonNode * tree;
    JsonToken * values;
    int capacity;
    int size;
} JsonDocument;

JsonNode * json_start();
JsonNode * json_object();
JsonNode * json_string_value();
JsonNode * json_value();
JsonNode * json_array();
JsonNode * json_string();
JsonNode * json_number();
int json_number2(char * out_number, char * buffer);
JsonNode * json_true();
JsonNode * json_false();
JsonNode * json_null();
void json_print_ast(JsonNode * root);
void _json_print_ast(JsonNode * node);
static void unknown_value(char * message);
void json_cleanup();
void * json_malloc(uint32_t size);



#ifdef JSON_PARSER_IMPLEMENTATION

static JsonDocument document;
static int g_lineno;

int json_skipWhitespaces(char * buffer)
{
    int skipped = 0;
    while (*buffer == ' ' || *buffer == '\t') { 
        buffer++;
        skipped++; 
    }
    return skipped;
}

void print_indent(int indentationCount)
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

int json_name(char ** out_name, char * buffer)
{
    char * buffer_start = buffer;
    int length = 0;
    while (*buffer != '"') {
        buffer++;
        length++;
    }
    *out_name = (char*)json_malloc( (length+1) * sizeof(char) );
    memcpy( (void*)*out_name, (void*)buffer_start, length );
    (*out_name)[length] = '\0';
    return length;
}

int json_number2(char * out_number, char * buffer)
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
            g_lineno++;
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
	unknown_value(tmp);
	return 0;
    }
}

static char * buf;
static int indent;
static JsonToken g_json_token;
static JsonMemory g_json_memory;

JsonToken json_get_token()
{
    skip_whitespaces_and_linebreaks(&buf);
    JsonToken token;
    if ( is_number(buf) ) {
	char asciiNumber[32];
	buf += json_number2(asciiNumber, buf);
	token.type = JSON_NUMBER;
	token.data.f_num = atof(asciiNumber);
	advance_to_next_non_an(&buf);
    }
    else {
	switch (*buf)
	{
	case '{':
	{
	    token.type = JSON_OBJECT;
	    token.size = 0;
	    buf++;
	} break;
        
	case '}':
	{
	    token.type = JSON_OBJECT_CLOSE;
	    token.size = 0;
	    buf++;
	} break;
        
	case '[':
	{
	    token.type = JSON_ARRAY;
	    token.size = 0;
	    buf++;
	} break;
        
	case ']':
	{
	    token.type = JSON_ARRAY_CLOSE;
	    token.size = 0;
	    buf++;
	} break;
        
	case '"':
	{
	    buf++; // step over opening ' " '
	    token.type = JSON_STRING;
	    token.size = 0;
	    buf += json_name( &(token.data.name), buf );
	    buf++; // step over closing ' " '
	} break;
        
	case 'f':
	{
	    if (check_value_string(&buf, "false", 5)) {		
		token.type = JSON_FALSE;
		token.size = 0;
	    }
	    else {
		token.type = JSON_UNKNOWN_VALUE;
		token.size = 0;
	    }
	    advance_to_next_non_an(&buf);
	} break;
        
	case 't':
	{
	    if (check_value_string(&buf, "true", 4)) {
		token.type = JSON_TRUE;
		token.size = 0;
	    }
	    else {
		token.type = JSON_UNKNOWN_VALUE;
		token.size = 0;
	    }
	    advance_to_next_non_an(&buf);
	} break;

	case 'n':
	{
	    if (check_value_string(&buf, "null", 4)) {
		token.type = JSON_NULL;
		token.size = 0;
	    }
	    else {
		token.type = JSON_UNKNOWN_VALUE;
		token.size = 0;
	    }
	    advance_to_next_non_an(&buf);
	} break;
        
	case ',':
	{
	    token.type = JSON_COMMA;
	    token.size = 0;
	    advance_to_next_whitespace(&buf);
	} break;
        
	case ':':
	{
	    token.type = JSON_COLON;
	    token.size = 0;
	    advance_to_next_whitespace(&buf);
	} break;

	case '\0':
	{
	    token.type = JSON_EOF;
	    token.size = 0;
	} break;
	
	default:	   
	{
	    token.type = JSON_UNKNOWN_VALUE;
	    token.size = 0;
	    check_value_string(&buf, "", 1);
	}
	}
    }
    skip_whitespaces_and_linebreaks(&buf);
    return token;
}

void print_token2(JsonToken * token)
{
    switch (token->type)
    {
    case JSON_OBJECT:
    {
	printf("OBJECT\n");
    } break;
        
    case JSON_OBJECT_CLOSE:
    {
	printf("OBJECT-CLOSE\n");
    } break;
        
    case JSON_ARRAY:
    {
	printf("ARRAY\n");
    } break;
        
    case JSON_ARRAY_CLOSE:
    {
	printf("ARRAY-CLOSE\n");
    } break;
        
    case JSON_STRING:
    {
	printf("STRING: %s\n", token->data.name);
    } break;
        
    case JSON_NUMBER:
    {
	printf("NUMBER: %f\n", token->data.f_num);
    } break;
        
    case JSON_TRUE:
    {
	printf("TRUE\n");
    } break;
        
    case JSON_FALSE:
    {
	printf("FALSE\n");
    } break;

    case JSON_NULL:
    {
	printf("NULL\n");	      
    } break;
        
    case JSON_COMMA:
    {
	printf(",\n");
    } break;
        
    case JSON_COLON:
    {
	printf(":\n");
    } break;

    case JSON_EOF:
    {
	printf("EOF\n");
    } break;

    case JSON_UNKNOWN_VALUE:
    {
	printf("UNKNOWN VALUE!\n");
    } break;
    }
}

void print_token(JsonToken * token)
{
    switch (token->type)
    {
    case JSON_OBJECT:
    {
	print_indent(indent);
	printf("OBJECT\n");
	indent += 2;
    } break;
        
    case JSON_OBJECT_CLOSE:
    {
	indent -= 2;
	print_indent(indent);
	printf("OBJECT-CLOSE\n");
    } break;
        
    case JSON_ARRAY:
    {
	print_indent(indent);
	printf("ARRAY\n");
	indent += 2;
    } break;
        
    case JSON_ARRAY_CLOSE:
    {
	indent -= 2;
	print_indent(indent);
	printf("ARRAY-CLOSE\n");
    } break;
        
    case JSON_STRING:
    {
	print_indent(indent);
	printf("STRING: %s\n", token->data.name);
    } break;
        
    case JSON_NUMBER:
    {
	print_indent(indent);
	printf("NUMBER: %f\n", token->data.f_num);
    } break;
        
    case JSON_TRUE:
    {
	print_indent(indent);
	printf("TRUE\n");
    } break;
        
    case JSON_FALSE:
    {
	print_indent(indent);
	printf("FALSE\n");
    } break;

    case JSON_NULL:
    {
	print_indent(indent);
	printf("NULL\n");
    } break;
        
    case JSON_COMMA:
    {
	print_indent(indent);
	printf(",\n");
    } break;
        
    case JSON_COLON:
    {
	print_indent(indent);
	printf(":\n");
    } break;

    case JSON_EOF:
    {
	printf("EOF\n");
    } break;

    case JSON_UNKNOWN_VALUE:
    {
	print_indent(indent);
	printf("UNKNOWN VALUE!\n");
    } break;
    }
}

void * json_malloc(uint32_t size)
{
    assert( ((g_json_memory.used + size) <= g_json_memory.size) && "Out of memory!" );
    void * addr = (void*)(g_json_memory.data + g_json_memory.used);
    g_json_memory.used += size;
    return addr;
}

void json_cleanup()
{
    free((void*)g_json_memory.data);
    g_json_memory.used = 0;    
}

JsonNode * new_json_node()
{
    JsonNode * new_node = (JsonNode *)json_malloc(sizeof(JsonNode));
    new_node->child = 0;
    new_node->sibling = 0;
    new_node->token = (JsonToken){0};
    return new_node;
}

static void unknown_value(char * message)
{
    fprintf(stderr, "\n>>> Unknown JSON-value at line %d: %s\n", g_lineno, message);
}

static void syntax_error(char * message)
{
    fprintf(stderr, "\n>>> Syntax error at line %d %s", g_lineno, message);
}

static void match(JsonType expected_token_type)
{
    if (g_json_token.type == expected_token_type) {
        g_json_token = json_get_token();
    }
    else {
        syntax_error("unexpected token -> ");
        print_token(&g_json_token);
	abort();
    }
}

JsonNode * json_object()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_OBJECT);
//    g_json_token = json_get_token();
    /* After object starts, next value-type must be either JSON_STRING or JSON_NULL */
    if (g_json_token.type == JSON_STRING) {
	t->child = json_string_value();
    }
    else { // g_json_object.type == JSON_OBJECT_CLOSE !
	// t->child = json_null(); /* TODO: should this be undefined or get the json-value 'null'? */
	t->child = 0;	
    }
    JsonNode * p = t->child;
    while (g_json_token.type == JSON_COMMA) {
        match(JSON_COMMA);
        JsonNode * q = json_string_value();
        p->sibling = q;
        p = q;
    }
    match(JSON_OBJECT_CLOSE);
    return t;
}

JsonNode * json_string_value()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_STRING);
    match(JSON_COLON);
    t->child = json_value();
    return t;
}

JsonNode * json_value()
{
    JsonNode * t = 0;
    switch(g_json_token.type)
    {
        case JSON_OBJECT:
        {
            t = json_object();
        }
        break;
        
        case JSON_ARRAY:
        {
            t = json_array();
        }
        break;
        
        case JSON_STRING:
        {
            t = json_string();
        }
        break;
        
        case JSON_NUMBER:
        {
            t = json_number();
        }
        break;
        
        case JSON_TRUE:
        {
            t = json_true();
        }
        break;
        
        case JSON_FALSE:
        {
            t = json_false();
        }
        break;
        
        case JSON_NULL:
        {
	    t = json_null();
        }
        break;
        
        default:
        {
            syntax_error("unexpected token -> ");
            print_token(&g_json_token);
//            g_json_token = json_get_token();
	    abort();
        }
        break;
    }
    return t;
}

JsonNode * json_array()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_ARRAY);
//    g_json_token = json_get_token();
    t->child = json_value();
    JsonNode * p = t->child;
    while (g_json_token.type == JSON_COMMA) {
        match(JSON_COMMA);
        JsonNode * q = json_value();
        p->sibling = q;
        p = q;
    }
    match(JSON_ARRAY_CLOSE);
    return t;
}

JsonNode * json_string()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_STRING);
    return t;
}

JsonNode * json_number()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_NUMBER);
    return t;
}

JsonNode * json_true()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_TRUE);
    return t;
}

JsonNode * json_false()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_FALSE);
    return t;
}

JsonNode * json_null()
{
    JsonNode * t = new_json_node();
    t->token = g_json_token;
    match(JSON_NULL);
    return t;
}

JsonNode * json_start()
{
    JsonNode * t = 0;
    switch (*buf) {
    case '{': {
	g_json_token = json_get_token();
	t = json_object();
    } break;

    case '[': {
	g_json_token = json_get_token();
	t = json_array();
    } break;
    default: {
	assert("json document must start with object or array!\n");
    }
    }

    return t;
}

JsonDocument json_parse(char * buffer)
{
    // TODO(Michael): assert buffer
    
#if 0    
    // print buffer to console for debugging
    char * bufferPos = buffer;
    while (*bufferPos != '\0')
    {
        int skipped = json_skipWhitespaces(bufferPos);
        bufferPos += skipped;
        printf("%c", *bufferPos);
        bufferPos++;
    }
    
    // print tokens to console for debugging
    buf = buffer;
    while (*buf != '\0') {
        g_json_token = json_get_token();
        print_token(&g_json_token);
    }
#endif

    /* Initialize Memory */
    {
	g_json_memory.data = (uint8_t*)malloc(MEGJ_MEMORY_SIZE);
	memset((uint8_t*)g_json_memory.data, 0, MEGJ_MEMORY_SIZE);
	g_json_memory.size = MEGJ_MEMORY_SIZE;
	g_json_memory.used = 0;
    }
    
    JsonDocument document;
    buf = buffer;
    JsonNode * t = 0;
//    g_json_token = json_get_token();
    skip_whitespaces_and_linebreaks(&buf);
    t = json_start();
    //_json_print_ast(t);
    document.tree = t;
    return document;
}

static int indentation;
void _json_print_ast(JsonNode * tree)
{
    indentation += 2;
    while (tree) {
        print_indent(indentation);
        JsonToken token = tree->token;
        print_token2(&token);
        if (tree->child) {
            _json_print_ast(tree->child);
        }
        tree = tree->sibling;
    }
    indentation -= 2;
}

JsonNode * json_get_value_by_name(JsonNode * node, char * name)
{
    JsonNode * t = 0;
    if ( !strcmp(node->child->token.data.name, name) ) {
        t = node->child->child;
    }
    else {
        JsonNode * sibling = node->child->sibling;
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

JsonNode * json_get_child(JsonNode * node)
{
    return node->child;
}

JsonNode * json_get_next_value(JsonNode * node)
{
    return node->sibling;
}

float json_value_float(JsonNode * node)
{
    if (node->token.type == JSON_NUMBER) {
        return node->token.data.f_num;
    }
    else {
        // TODO(Michael): how to handle this error?
    }
    return 0.f;
}

int json_value_bool(JsonNode * node)
{
    if (node->token.type == JSON_TRUE) {
        return 1;
    }
    else if (node->token.type == JSON_FALSE) {
        return 0;
    }
    return -1; // TODO(Michael): what to return in error-case?
}

char * json_value_name(JsonNode * node)
{
    if (node->token.type == JSON_STRING) {
        return node->token.data.name;
    }
    else {
        return 0;
    }
}

void json_print_ast(JsonNode * root)
{
    printf("\n\n>>> AST <<<\n\n");
    int indentation = 0;
    JsonNode * current_node = root;
    while (current_node) {
        JsonToken token = current_node->token;
        if ( (token.type == JSON_OBJECT) || (token.type == JSON_ARRAY) ) {
            indentation += 2;
        }
        else if ( (token.type == JSON_OBJECT_CLOSE) || (token.type == JSON_ARRAY_CLOSE) ) {
            indentation -= 2;
        }
        print_indent(indentation);
        print_token2(&token);
        current_node = current_node->child;
    }
}

#endif

#endif
