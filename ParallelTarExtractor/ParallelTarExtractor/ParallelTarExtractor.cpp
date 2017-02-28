#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>

using namespace std;

struct tar_header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char linkflag[1];
	char linkname[100];
	char pad[255];
};

bool nextHeader(ifstream& inputFile, int nextHeaderBlock)
{
	//check if end of file
	if (inputFile.eof())
		return false;
	//Go to the next header
	inputFile.seekg((nextHeaderBlock * 512));

	//Check that the next header has content. If not go to the next block
	if (inputFile.peek() == '\0' && inputFile.good())
		return false;
	
	if (inputFile.good() && !inputFile.eof())
		return true;
}

bool hasNext(ifstream& inputFile, int nextHeaderBlock)
{
	//check if end of file
	int currentPosition = inputFile.tellg();
	bool next = nextHeader(inputFile, nextHeaderBlock);
	inputFile.seekg(currentPosition);

	return next;
}

tar_header readHeader(ifstream& inputFile, int nextHeaderBlock)
{
	tar_header header;
	//inputFile.seekg( (nextHeaderBlock * 512));
	//while (inputFile.peek() == '/0')
	//	inputFile.seekg(512, inputFile.cur);

	inputFile.read((char *)&header, sizeof(header));
	return header;
}

int convertSizeToInt(char size[12])
{
	int sizeInt = strtol(size, NULL, 8);

	return sizeInt;
}

void writeBody(ifstream& inputFile, tar_header& header) {
	ofstream outFile;

	//calculate size of body
	int size = convertSizeToInt(header.size);
	char* body = new char [size];
	string name = header.name;

	if (name[name.size() - 1] == '/')
	{
		string strInst = "mkdir -p " + name; //will not work if you dont remove the '/'
		char * instruction;
		strcpy(instruction, strInst.c_str());
		system(instruction); //use c call instead
	}
	else
	{
		outFile.open(name);
		inputFile.read(body, size);
		body[size] = '\0';

		outFile << body;
		outFile.close();
	}
}

int main(int argc, char *argv[])
{
	ifstream inputFile;
	int nextHeaderBlock = 0;
	
	if (argv[1] != nullptr)
	{
		string tarFileName = argv[1];
		inputFile.open(tarFileName);

		inputFile.seekg(0, inputFile.end);
		int lengthOfFile = inputFile.tellg();
		inputFile.seekg(0, inputFile.beg);

		if (inputFile.good())
		{
			do
			{
				//read header
				tar_header header = readHeader(inputFile, nextHeaderBlock);
				nextHeaderBlock += ((511 + convertSizeToInt(header.size)) / 512) + 1; //an offset of 512 is the full header

				//fork
				pid_t pid = fork();

				if (pid == 0)
				{
					//if child
					cout << "reading with child process" << endl;
					continue;
				}

				//if parent
				int checksum = strtol(header.checksum, NULL, 8);

				// Put spaces in the checksum area before we compute it.
				for (int i = 0; i < 8; i++) {
					header.checksum[i] = ' ';
				}

				//compute checksum
				int computedChecksum = 0;
				for (int i = 0; i < sizeof(header); i++) {
					computedChecksum += ((char*)&header)[i];
				}
				//write to file
				if (computedChecksum == checksum)
					writeBody(inputFile, header);
				return EXIT_SUCCESS;

			} while (nextHeader(inputFile, nextHeaderBlock)); //there are more headers

			inputFile.close();
			return EXIT_SUCCESS;
		}
		else
			cout << "Error extracting file. Could not open source file" << endl;

		return EXIT_SUCCESS;
	}
	else
		cout << "Error extracting file. Must provide a file name" << endl;

	return EXIT_SUCCESS;
}