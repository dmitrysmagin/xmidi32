#include <stdio.h>
#include "file.h"
#include "xmi.h"

int main(int argc, char **argv) {
	for(int i = 1; i < argc; i++) {
		std::vector<char> file_buffer;
		std::string in_file = argv[i] + std::string(".xmi");
		std::string out_file = argv[i] + std::string(".midi");
		if (!LoadFile(in_file, file_buffer)) {
			printf("Could not load %s\n", in_file.c_str());
			continue;
		}
		
		printf("Loaded %s\n", in_file.c_str());
		printf("Converting %s to %s\n", in_file.c_str(), out_file.c_str());

		size_t ptr_len = 0;
		unsigned char *ptr = TranscodeXmiToMid((const unsigned char*)&file_buffer[0], file_buffer.size(), &ptr_len);

		if(!ptr) {
			printf("Could not convert %s\n", out_file.c_str());
			continue;
		}

		if (!SaveFile(out_file, ptr, ptr_len)) {
			printf("Could not save %s\n", out_file.c_str());
			continue;
		}
		
		printf("Converted and saved %s to %s\n", in_file.c_str(), out_file.c_str());
	}
	
	return 0;
}
