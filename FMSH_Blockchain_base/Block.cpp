
#include "Block.h"

//Convert a block into a string to send over the network
std::string Block::CompressBlock() {
	return std::to_string(number) + '|' + std::to_string(complexity) + '|' + prevBlockHash + '|' + data + '|' + blockHash + \
		'|' + std::to_string(nonce) + '|' + std::to_string(timestamp);
}

//Construct a block from a string received over the network
Block::Block(std::string compressedBlock) {
	//number
	std::size_t pos = compressedBlock.find('|');
	number = std::stoi(compressedBlock.substr(0, pos), nullptr, 10);
	compressedBlock = compressedBlock.substr(pos + 1);
	//complexity
	pos = compressedBlock.find('|');
	complexity = std::stoi(compressedBlock.substr(0, pos), nullptr, 10);
	compressedBlock = compressedBlock.substr(pos + 1);
	//prevBlockHash
	pos = compressedBlock.find('|');
	prevBlockHash = compressedBlock.substr(0, pos);
	compressedBlock = compressedBlock.substr(pos + 1);
	//data
	pos = compressedBlock.find('|');
	data = compressedBlock.substr(0, pos);
	compressedBlock = compressedBlock.substr(pos + 1);
	//blockHash
	pos = compressedBlock.find('|');
	blockHash = compressedBlock.substr(0, pos);
	compressedBlock = compressedBlock.substr(pos + 1);
	//nonce
	pos = compressedBlock.find('|');
	nonce = std::stoi(compressedBlock.substr(0, pos), nullptr, 10);
	compressedBlock = compressedBlock.substr(pos + 1);
	//timestamp
	timestamp = (time_t)std::stoi(compressedBlock, nullptr, 10);
}

//Default constructor
Block::Block() {
	number = 0;
	complexity = 0;
	prevBlockHash = "";
	data = "";
	blockHash = "";
	nonce = 0;
	timestamp = time(NULL);
}

Block::Block(int num, int comp, std::string prevHash, std::string input, std::string hash, long int n, time_t time) {
	number = num;
	complexity = comp;
	prevBlockHash = prevHash;
	data = input;
	blockHash = hash;
	nonce = n;
	timestamp = time;
}