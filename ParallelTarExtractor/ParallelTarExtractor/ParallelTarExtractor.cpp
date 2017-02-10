#include <string>
#include <iostream>
#include <fstream>
#include <stdlib.h> 

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

tar_header readHeader(ifstream& inputFile, int nextHeaderBlock)
{
	tar_header header;
	inputFile.seekg( (nextHeaderBlock * 512));
	cout << "before header read" << inputFile.tellg() << endl;
	inputFile.read((char *)&header, sizeof(header));
	cout << "after header read" << inputFile.tellg() << endl;

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

	outFile.open(header.name);
	cout << "before body read" << inputFile.tellg() << endl;
	inputFile.read(body, size); //reading 10 more chars than it should, which are gone before the next read file??
	cout << "after body read" << inputFile.tellg() << endl;

	outFile << body;
	outFile.close();
}

int main(int argc, char *argv[])
{
	ifstream inputFile;
	int nextHeaderBlock = 0;
	
	if (argv[1] != nullptr)
	{
		string tarFileName = argv[1];
		inputFile.open(tarFileName);
		if (inputFile.good())
		{
			do {
				//read header
				tar_header header = readHeader(inputFile, nextHeaderBlock);
				//fork
			

				//if child
					nextHeaderBlock += ((511 + convertSizeToInt(header.size)) / 512) + 1; //an offset of 512 is the full header
					//continue;

				//if parent
				int checksum = strtol(header.checksum, NULL, 8);

				// Put spaces in the checksum area before we compute it.
				for (int i = 0; i < 8; i++) {
						header.checksum[i] = ' ';
				}
				//compute checksum
				int computedChecksum = 0;
				for (int i = 0; i < sizeof(header); i++) {
					computedChecksum += ((char*) &header)[i];
				}
				//write to file
				if (computedChecksum == checksum)
					writeBody(inputFile, header);

			} while (!inputFile.eof()); //there are more blocks;

			inputFile.close();
		}
		else
			cout << "Error extracting file. Could not open source file" << endl;

		return EXIT_SUCCESS;
	}
	cout << "Error extracting file. Must provide a file name" << endl;

	return EXIT_SUCCESS;
}