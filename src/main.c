#include <stdio.h>
#include <stdint.h>

#define JSON_PARSER_IMPLEMENTATION
#include "json_parser.h"

typedef struct MyFile {
    uint32_t size;
    char * data;
} MyFile;

MyFile read_file(char * filepath) {
    MyFile file;
    FILE * f = fopen(filepath, "r");
    fseek(f, 0, SEEK_END);
    file.size = ftell(f);
    fseek(f, 0, SEEK_SET);
    file.data = (char*)malloc(file.size+1);
    int elems_read = fread((void*)(file.data), sizeof(char), file.size, f);
    file.data[elems_read] = '\0';
    fclose(f);
    return file;
}

int main(int argc, char ** argv)
{

    MyFile json_file = read_file("../testfiles/array.json");
    printf("%s\n", json_file.data);

    JsonDocument doc = json_parse(json_file.data);
    //_json_print_ast(doc.tree);
    JsonNode * stuff_array = json_get_value_by_name(doc.tree, "stuff");
    JsonNode * element = json_get_child(stuff_array);
    while (element != NULL) {
	element = json_get_next_value(element);
    }



    
    return 0;
}
