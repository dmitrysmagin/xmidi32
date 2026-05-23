#include "file.h"
#include <cstdio>

bool LoadFile(const std::string filename, std::vector<char> &buffer) {
	FILE *f = fopen(filename.c_str(), "rb");
	if(f) {
		fseek(f, 0, SEEK_END);
		size_t sz = ftell(f);
		rewind(f);
		
		if(sz != 0) {
			buffer.resize(sz);
			
			size_t res = fread(&buffer[0], 1, sz, f);
			if(res != sz) {
				fclose(f);
				return false;
			}
			
			fclose(f);
			return true;
		}
		
		fclose(f);
		return false;
	}
	return false;
}

bool SaveFile(const std::string filename, const unsigned char *buffer, size_t buffer_len) {
	if (buffer_len == 0) {
		return false;
	}

	FILE *f = fopen(filename.c_str(), "wb");
	if(f) {
		size_t res = fwrite(buffer, sizeof(char), buffer_len, f);
		if (res != buffer_len) {
			fclose(f);
			return false;
		}
		
		fclose(f);
		return true;
	}
	return false;
}
