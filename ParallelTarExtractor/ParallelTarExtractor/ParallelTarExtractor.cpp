#include <string.h>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstdlib>
#include <vector>

using namespace std;

//structure of a tar header
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

//Checks if there is another header to read, if there is it will move the position of
//reading to the start of the header and it will return true, otherwise will return false
bool nextHeader(ifstream& inputFile, int nextHeaderBlock, int lengthOfFile, int position)
{
	//check if the file is actually done or a eof character was read but there is more content
	position += (nextHeaderBlock * 512);
	if (position < lengthOfFile)
		inputFile.clear();
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

	return false;
}

//reads a tar header
tar_header readHeader(ifstream& inputFile, int nextHeaderBlock, int position)
{
	tar_header header;
	inputFile.read((char *)&header, sizeof(header));
	position += 512;
	return header;
}

//converts a 12 bit octal number to an int
int convertSizeToInt(char size[12])
{
	int sizeInt = strtol(size, NULL, 8);

	return sizeInt;
}

//if header indicates a directory it creates a directory with the name indicated
//if header indicates a file it creates a file with the name indicated and it
//writes the contents of the body of the file
void writeBody(ifstream& inputFile, string mainDir, tar_header& header) {
	ofstream outFile;

	//calculate size of body
	int size = convertSizeToInt(header.size);
	string name = mainDir + "/" + header.name;

	if (name[name.size() - 1] == '/')
	{
		//header indicated directory
		name = name.substr(0, name.size() - 1);
		int status = mkdir(name.c_str(), 0700);
		if (status != 0)
		{
			string error = "Couldn't make Directory" + name;
			perror(error.c_str());
		}
		else
			cout << "Created directory: " << name.c_str() << endl;
		
		
	}
	else
	{
		//header indicated file
		char* body = new char[size];
		vector<char> bodyV;
		bodyV.resize(size);
		inputFile.read(&bodyV[0], size);
		bodyV[size -1] = '\0';

		outFile.open(name);
		for (size_t i = 0; i < bodyV.size(); i++)
			outFile << bodyV[i];
		outFile.close();

		cout << "Created File: " << name.c_str() << endl;
	}
}

int main(int argc, char *argv[])
{
	ifstream inputFile;
	int nextHeaderBlock = 0;
	
	if (argv[1] != nullptr)
	{
		string tarFileName = argv[1];

		//creates a base directory where the files will be extracted to
		string mainDir = tarFileName + "Dir";
		int status = mkdir(mainDir.c_str(), 0700);
		if (status != 0)
		{
			string error = "Couldn't make Directory" + mainDir;
			perror(error.c_str());
		}

		inputFile.open(tarFileName);

		inputFile.seekg(0, inputFile.end);
		int lengthOfFile = inputFile.tellg();
		inputFile.seekg(0, inputFile.beg);
		int position = 0;

		if (inputFile.good())
		{
			do //do loop while there are more headers to read
			{
				//read header
				tar_header header = readHeader(inputFile, nextHeaderBlock, position);
				nextHeaderBlock += ((511 + convertSizeToInt(header.size)) / 512) + 1; //an offset of 512 is the full header

				//fork
				pid_t pid = fork();

				if (pid == 0)
				{
					//if child wait to avoid creation of files before directories
					//and then continue to read the next header if available
					sleep(2);
					continue;
				}

				//if parent continue with the computations and write the contents of the body
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
					writeBody(inputFile, mainDir, header);
				return EXIT_SUCCESS;

			} while (nextHeader(inputFile, nextHeaderBlock, lengthOfFile, position)); //there are more headers

			inputFile.close();
			cout << "Done with extraction of " << tarFileName.c_str() << endl;
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