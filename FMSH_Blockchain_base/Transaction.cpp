
#include "Transaction.h"

std::string Transaction::CompressTransaction() {
	return std::to_string(number) + '|' + data;
}

Transaction::Transaction(int num, std::string transaction) {
	number = num;
	data = transaction;
}

Transaction::Transaction(std::string compressedTransaction) {
	//number
	std::size_t pos = compressedTransaction.find('|');
	number = std::stoi(compressedTransaction.substr(0, pos), nullptr, 10);
	//data
	data = compressedTransaction.substr(pos + 1);
}

Transaction::Transaction() {
	number = 0;
	data = "";
}

bool operator< (const Transaction& lhs, const Transaction& rhs) {
	return (lhs.number < rhs.number);
};