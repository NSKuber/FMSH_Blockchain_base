// FMSH_Blockchain_base.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <process.h>
#include <thread>             // std::thread, std::this_thread::yield
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable>
#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <fstream>
#include <vector>
#include <map> 
#include "Block.h"
#include "Mining.h"
#include "Transaction.h"

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "43012"

//using namespace CryptoPP;
using namespace std;

int timePerBlock = 3;

int mainComplexity;
int compPower;
bool isServerOnline = true;
bool hasReceivedBlock = false;
bool hasReceivedBlockchain = false;
std::vector<Block> Blockchain;

std::map<int,Transaction> UnusedTransactions;

//Calculate computational power of this PC (one thread)
int GetComputationPower() {
	auto begin = std::chrono::steady_clock::now();
	for (int i = 0; i < 1000; i++) {
		SHA256Hash(to_string(rand()));
	}
	auto end = std::chrono::steady_clock::now();

	return (int)(1000 / ((double)std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count()/1000000000));
}

std::mutex m;
std::condition_variable cv;
std::condition_variable cvComplexity;

//Send the mined block to server
void SendBlockToServer(std::string compressedBlock, SOCKET serverSocket) {
	int iSendResult;
	char const *sendbuf = compressedBlock.c_str();

	//std::cout << "Sending out block data: " << sendbuf << std::endl;

	iSendResult = send(serverSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(serverSocket);
		//WSACleanup();
	}
	std::cout << "LOG: Sent out mined block data to the server: " << sendbuf << std::endl;
}

//Remove transaction used in a received block from the pool
void RemoveTransactionsUsedInBlock(std::string data) {
	int pos = data.find("!");
	while (pos > 0) {
		int num = stoi(data.substr(0,pos), nullptr, 10);
		UnusedTransactions.erase(num);
		data = data.substr(pos + 1);
		pos = data.find("!");
		data = data.substr(pos + 1);
		pos = data.find("!");
	}
}

//Parser of incoming network data
std::string HandleIncomingData(std::string data) {
	size_t end = data.find(';');
	if (end < 0) {
		printf("Received data in wrong format!\n");
		return "";
	}
	size_t start = data.find(':');
	if (start < 0) {
		printf("Received data in wrong format!\n");
		return "";
	}
	std::string type = data.substr(0, start);

	data = data.substr(start + 1, end - start - 1);
	if (type == "Complexity") {
		std::cout << "Received complexity data;\n";
		mainComplexity = stoi(data, nullptr, 10);
		return "";
	}
	else if (type == "Block") {
		std::cout << "Received block data;\n";
		Block newBlock = Block(data);
		if (newBlock.number == 0)
			return "EmptyBlock";
		RemoveTransactionsUsedInBlock(newBlock.data);
		Blockchain.push_back(newBlock);
		hasReceivedBlock = true;
		//printf("Received a block (socket thread)\n");
		cv.notify_one();
		return "";
	}
	else if (type == "Transaction") {
		std::cout << "Received transaction data: " + data + "\n";
		Transaction newTransaction = Transaction(data);
		if (newTransaction.number == 0)
			return "EmptyTransaction";
		UnusedTransactions.insert(std::pair<int, Transaction>(newTransaction.number,newTransaction));
		return "";
	}
	else {
		printf("Received data in wrong format!\n");
		return "";
	}
}

//Initial data exchange on connect
void ReceiveAndSendInitialData(SOCKET ClientSocket) {

	int iSendResult;
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;

	std::string clientType = "Miner;";
	char const *sendbuf = clientType.c_str();

	//std::cout << "Sending out client type data: " << sendbuf << std::endl;

	iSendResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iSendResult == SOCKET_ERROR) {
		//printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
		//WSACleanup();
	}
	
	std::cout << "LOG: Connected to the server\n";

	// receiving blockchain until received empty block
	std::string receivedData;
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			//printf("Bytes received: %d\n", iResult);
			receivedData = HandleIncomingData(std::string(recvbuf));
		}
		else if (iResult == 0)
			std::cout << "LOG: Connection closed with server on socket " + std::to_string(ClientSocket) << std::endl;
		else {
			//printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
		}

	} while ((iResult > 0) && (receivedData != "EmptyBlock"));

	std::cout << "LOG: Received all blocks from server\n";
	hasReceivedBlock = false;

	// receiving transactions until received empty transaction
	do {
		iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			//printf("Bytes received: %d\n", iResult);
			receivedData = HandleIncomingData(std::string(recvbuf));
		}
		else if (iResult == 0)
			std::cout << "LOG: Connection closed with server on socket " + std::to_string(ClientSocket) << std::endl;
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ClientSocket);
		}

	} while ((iResult > 0) && (receivedData != "EmptyTransaction"));

	std::cout << "LOG: Received all transactions from server\n";

	// Sending out own computational power
	std::string s = std::string("Power:") + std::to_string(compPower) + std::string(";");
	sendbuf = s.c_str();

	iSendResult = send(ClientSocket, sendbuf, (int)strlen(sendbuf), 0);
	if (iSendResult == SOCKET_ERROR) {
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(ClientSocket);
	}

	// Receiving complexity data
	iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		printf("Data received: %.*s\n", iResult, (char*)recvbuf);
		string result = string(recvbuf);
		string tempComp;
		std::size_t pos1 = result.find("Complexity:");
		if (pos1 >= 0) {
			std::size_t pos2 = result.find(";");
			tempComp = result.substr(pos1 + 11, pos2 - 1);
			mainComplexity = stoi(tempComp, nullptr, 10);
		}
	}
	else if (iResult == 0)
		std::cout << "LOG: Connection closed with server on socket " + std::to_string(ClientSocket) << std::endl;
	else
		printf("recv failed with error: %d\n", WSAGetLastError());
}

//Session with the server
DWORD WINAPI SessionWithServer(LPVOID data) {

	SOCKET ConnectSocket = (SOCKET)data;
	// Process the client.

	int iSendResult;
	int iResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	std::string receivedData;

	// Receiving all kinds of data
	do {
		iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			std::cout << "LOG: Received data from the server: ";
			receivedData = HandleIncomingData(std::string(recvbuf));
		}
		else if (iResult == 0)
			std::cout << "LOG: Connection closed with server on socket " + std::to_string(ConnectSocket) << std::endl;
		else {
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(ConnectSocket);
		}

	} while (iResult > 0);

	// shutdown the connection since we're done
	iResult = shutdown(ConnectSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		//WSACleanup();
	}

	// cleanup
	closesocket(ConnectSocket);
	//WSACleanup();

	std::cout << "LOG: Server disconnected, shutting down mining\n";
	isServerOnline = false;
	return 0;

}

//Trying to connect to the server
SOCKET ConnectToServer(char* address) {
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL,
		*ptr = NULL,
		hints;
	int iResult;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return INVALID_SOCKET;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(address, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return INVALID_SOCKET;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return INVALID_SOCKET;
	}

	return ConnectSocket;

}

//The actual mining function
Block ProofOfWork(Block prevBlock, int complexity, std::string data) {
	Block output = prevBlock;
	output.number = output.number + 1;
	output.prevBlockHash = output.blockHash;
	output.data = data;
	output.complexity = complexity;

	long int nonce;
	std::string hash;
	time_t timestamp;
	do {
		nonce = rand() * RAND_MAX + rand();
		timestamp = time(NULL);
		hash = SHA256Hash(std::to_string(output.number) + output.data + output.prevBlockHash + std::to_string(timestamp) + std::to_string(nonce));
	} while ((IsHashCorrect(hash, output.complexity) != true) && (!hasReceivedBlock) && (isServerOnline));

	//printf("Mining stopped\n");
	if ((hasReceivedBlock) || (!isServerOnline)) {
		//printf("Received a block (mining thread)\n");
		std::cout << "LOG: Received a block from the server, restarting my mining\n";
		return Block();
	}
	else {
		std::cout << "LOG: Mined a block\n";
		output.nonce = nonce;
		output.blockHash = hash;
		output.timestamp = timestamp;
		return output;
	}

	//return output;
}

//Take several unused transactions (up to maxNum) and convert them into string for use in the block
std::string CompressUnusedTransactions(int maxNum) {
	int num = 0;
	std::string output = "";
	for (std::map<int, Transaction>::iterator it = UnusedTransactions.begin(); it != UnusedTransactions.end(); ++it) {
		Transaction t = it->second;
		output += to_string(t.number) + "!" + t.data + "!";
		num++;
		if (num == maxNum) {
			break;
		}
	}
	return output;
}

int main(int argc, char **argv)
{

	// Validate the parameters
	if (argc != 2) {
		printf("usage: %s server-address\n", argv[0]);
		return 1;
	}

	// Calculating computational power
	compPower = GetComputationPower();
	cout << "LOG: Computational power is " << compPower << " hashes per second\n";

	cout << "LOG: Connecting to server..." << endl;

	SOCKET ConnectSocket = ConnectToServer(argv[1]);

	if (ConnectSocket == INVALID_SOCKET) {
		cout << "LOG: Connection failed, restart the program while making sure the server is running\n";
		return 1;
	}

	//cout << "Connection succeeded, sending own proccessing power and receiving blockchain\n";

	DWORD dwThreadId;

	// Receiving blockchain and sending computational power
	ReceiveAndSendInitialData(ConnectSocket);

	CreateThread(NULL, 0, SessionWithServer, (LPVOID)ConnectSocket, 0, &dwThreadId);

	//cout << "Sent own proccessing power, waiting for complexity data\n";

	std::unique_lock<std::mutex> lk(m);
	//cvComplexity.wait(lk, [] {return (mainComplexity > 0); });

	//cout << "Complexity will be " << mainComplexity << " zeroes\n";
	cout << "LOG: Received all required data, proceeding to mining\n";

	srand(time(nullptr));

	cout << std::string(50, '-') << endl;
	
	ofstream outputFile;
	outputFile.open("Blockchain.txt");

	Block newBlock = Block();

	while (isServerOnline) {

		// Mining until found block or received block
		std::string input = CompressUnusedTransactions(3);
		Block PoW;
		if (Blockchain.size() == 0) 
			PoW = ProofOfWork(Block(), mainComplexity, input);
		else
			PoW = ProofOfWork(Blockchain[Blockchain.size() - 1], mainComplexity, input);

		// If mined block then send it to server
		if (PoW.number != 0) {
			SendBlockToServer("Block:" + PoW.CompressBlock() + ";", ConnectSocket);
			// Wait until block received
			cv.wait(lk, [] {return (hasReceivedBlock); });
		}

		hasReceivedBlock = false;
		Block latestBlock = Blockchain[Blockchain.size() - 1];

		std::string output (50,'-');
		output += "\nBlock #" + to_string(latestBlock.number) + ", complexity is " + to_string(latestBlock.complexity) + '\n';
		output += "Input string was: " + latestBlock.data + '\n';
		output += "Previous block hash was: " + latestBlock.prevBlockHash + '\n';
		output += "Found nonce " + to_string(latestBlock.nonce) + " with hash: " + latestBlock.blockHash + '\n';
		output += "Timestamp: " + to_string(latestBlock.timestamp) + "\n" + std::string (50, '-') + "\n";

		cout << output;
		outputFile << output;
		outputFile.flush();
	}

	std::string s;
	std::cin >> s;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
