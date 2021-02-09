#pragma once

#include <string>
#include <fstream>
#include "sha.h"
#include "hex.h"
#include "Block.h"

//Mining functions 

const char* hex_char_to_bin(char c);

std::string hex_str_to_bin_str(std::string hex);

std::string SHA256Hash(std::string input);

bool IsHashCorrect(std::string hash, int complexity);

//std::string readWholeFile();
