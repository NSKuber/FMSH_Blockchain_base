
#include <string>
#include <fstream>
#include "sha.h"
#include "hex.h"
#include "Block.h"

const char* hex_char_to_bin(char c)
{
	switch (toupper(c))
	{
	case '0': return "0000";
	case '1': return "0001";
	case '2': return "0010";
	case '3': return "0011";
	case '4': return "0100";
	case '5': return "0101";
	case '6': return "0110";
	case '7': return "0111";
	case '8': return "1000";
	case '9': return "1001";
	case 'A': return "1010";
	case 'B': return "1011";
	case 'C': return "1100";
	case 'D': return "1101";
	case 'E': return "1110";
	case 'F': return "1111";
	}
}

std::string hex_str_to_bin_str(std::string hex)
{
	std::string bin;
	for (unsigned i = 0; i != hex.length(); ++i)
		bin += hex_char_to_bin(hex[i]);
	return bin;
}

std::string SHA256Hash(std::string input) {
	CryptoPP::byte digest[CryptoPP::SHA256::DIGESTSIZE];
	CryptoPP::SHA256 hash;
	hash.CalculateDigest(digest, (const CryptoPP::byte*)input.c_str(), input.length());
	CryptoPP::HexEncoder encoder;
	std::string output;
	encoder.Attach(new CryptoPP::StringSink(output));
	encoder.Put(digest, sizeof(digest));
	encoder.MessageEnd();
	return output;
}

//Checks the complexity of the resulting hash
bool IsHashCorrect(std::string hash, int complexity) {
	std::string bin_hash = hex_str_to_bin_str(hash);
	for (int i = 0; i < complexity; ++i) {
		if (bin_hash[i] != '0') {
			return false;
		}
	}
	return true;
}

//Leftover from development
/*
std::string readWholeFile() {
	std::ifstream inputFile;
	inputFile.open("Transactions.txt");
	std::stringstream sstr;
	sstr << inputFile.rdbuf();
	inputFile.close();

	std::ofstream tempFile;
	tempFile.open("Transactions.txt");
	tempFile.close();

	return sstr.str();
}
*/
