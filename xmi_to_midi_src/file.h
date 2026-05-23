#ifndef __XMI_FILE_H
#define __XMI_FILE_H

#include <vector>
#include <string>

bool LoadFile(const std::string filename, std::vector<char> &buffer);
bool SaveFile(const std::string filename, const unsigned char *buffer, size_t buffer_len);

#endif
