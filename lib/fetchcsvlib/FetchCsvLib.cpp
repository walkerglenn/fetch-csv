#include "FetchCsvLib.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace FetchCSV
{

bool DataFrame::loadData(std::string inputFilePath)
{

	// Clear the frame contents (if any exist currently)
	mFrameContents.clear();


	// Load the input file
	std::ifstream inputFile { inputFilePath };

	if (!inputFile)
	{
		std::cerr << "Error opening file!" << '\n';
		return false;
	}			

	// First line of the file
	std::string inputLine {};
	std::getline(inputFile, inputLine);

	// This call to parseLine lets us get the number of columns
	parseLine(inputLine);
	mNumColumns = static_cast<int>(mFrameContents.size());
	std::cout << "Size: " << mNumColumns << '\n';

	//Parse subsequent lines
	while (std::getline(inputFile, inputLine))
	{
		parseLine(inputLine);
	}
	
	std::cout << "File " << inputFilePath << " loaded successfully!" << '\n';
	std::cout << mFrameContents.size() << " cells loaded" << '\n';
	
	return true;

}

void DataFrame::parseLine(std::string inputLine)
{
	std::string valueBuff {};
	
	// Add values to the buffer, then add to dataframe and clear buffer when we reach a comma
	for (const char ch : inputLine)
	{
		valueBuff += ch;

		if (ch == ',')
		{
			// Remove the comma from the end of the value
			valueBuff.pop_back();

			mFrameContents.push_back(valueBuff);
			valueBuff.clear();
		}	
	}
	
	mFrameContents.push_back(valueBuff);
}

std::vector<std::string>& DataFrame::getData() { return mFrameContents; }
int DataFrame::getNumColumns() { return mNumColumns; }
size_t DataFrame::getNumCells() { return mFrameContents.size(); }

}
