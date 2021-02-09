#pragma once
#include <string>

//Basic transaction class, containing string data
class Transaction {
public:
	int number;
	std::string data;

	std::string CompressTransaction();
	Transaction(int, std::string);
	Transaction(std::string);
	Transaction();

};

inline bool operator< (const Transaction& lhs, const Transaction& rhs);