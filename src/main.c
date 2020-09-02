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

    MyFile json_file = read_file("../testfiles/array_with_empty_object.json");
    printf("%s\n", json_file.data);

    JsonDocument doc = json_parse(json_file.data);
    _json_print_ast(doc.tree);
#if 0
    JsonNode * frames = json_get_value_by_name(doc.tree, "frames");
    JsonNode * meta = json_get_value_by_name(doc.tree, "meta");
    JsonNode * frame = json_get_child(frames);
    while (frame != NULL) {
	JsonNode * filename_node = json_get_value_by_name(frame, "filename");
	char * filename = json_value_name(filename_node);	
	printf("filename: %s\n", filename);
	frame = json_get_next_value(frame);
    }
    JsonNode * app_node = json_get_value_by_name(meta, "app");
    JsonNode * size_node = json_get_value_by_name(meta, "size");
    JsonNode * width_node = json_get_value_by_name(size_node, "w");
    JsonNode * height_node = json_get_value_by_name(size_node, "h");
    float width = json_value_float(width_node);
    float height = json_value_float(height_node);
    char * app_name = json_value_name(app_node);
    printf("app name: %s, width: %f, height: %f\n", app_name, width, height);
  
#endif


    
    return 0;
}
