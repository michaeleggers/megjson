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

    MyFile json_file = read_file("../testfiles/fatguy.json");
    printf("%s\n", json_file.data);

    JsonDocument doc = json_parse(json_file.data);
    _json_print_ast(doc.tree);
    JsonNode * element = json_get_child(doc.tree);
    while (element != NULL) {
	JsonNode * latitude = json_get_value_by_name(element, "Latitude");
	JsonNode * longitude = json_get_value_by_name(element, "Longitude");
	float latitude_f = json_value_float(latitude);
	float longitude_f = json_value_float(longitude);
	printf("Latitude: %f, Longitude: %f\n", latitude_f, longitude_f);
	
	element = json_get_next_value(element);
    }



    
    return 0;
}
