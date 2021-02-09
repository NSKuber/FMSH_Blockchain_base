#pragma once

#include <string>
#include <ctime>

//A class for generic block in blockchain

class Block {
public:
	int number;
	int complexity;
	std::string prevBlockHash;
	std::string data;
	std::string blockHash;
	long int nonce;
	time_t timestamp;

	Block* prevBlock;
	Block* nextBlock;

	Block(int, int, std::string, std::string, std::string, long int, time_t);
	Block(std::string);
	Block();

	std::string CompressBlock();

};
