#include <stdio.h>
#include <stdint.h>

#define MEGJ_PARSER_IMPLEMENTATION
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

    MyFile megj_file = read_file("../testfiles/fatguy.json");
    printf("%s\n", megj_file.data);

    MegjDocument doc = megj_parse(megj_file.data);
    _megj_print_ast(doc.tree);
#if 1
    MegjNode * frames = megj_get_value_by_name(doc.tree, "frames");
    MegjNode * meta = megj_get_value_by_name(doc.tree, "meta");
    MegjNode * frame = megj_get_child(frames);
    while (frame != NULL) {
	MegjNode * filename_node = megj_get_value_by_name(frame, "filename");
	char * filename = megj_value_name(filename_node);	
	printf("filename: %s\n", filename);
	frame = megj_get_next_value(frame);
    }
    MegjNode * app_node = megj_get_value_by_name(meta, "app");
    MegjNode * size_node = megj_get_value_by_name(meta, "size");
    MegjNode * width_node = megj_get_value_by_name(size_node, "w");
    MegjNode * height_node = megj_get_value_by_name(size_node, "h");
    float width = megj_value_float(width_node);
    float height = megj_value_float(height_node);
    char * app_name = megj_value_name(app_node);
    printf("app name: %s, width: %f, height: %f\n", app_name, width, height);

#endif

    megj_cleanup();
    
    printf("done!\n");
    
    return 0;
}
