#pragma once

#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

class CsvDataFrame
{

private:
	std::vector<std::string> mFrameContents;
	int  mNumColumns;

	// Scan through a single line and add values to the contents vector
	void parseLine(std::string inputLine);

public:	
	bool loadData(std::string inputFilePath);

	std::vector<std::string>& getData();
	
	int getNumColumns();

	size_t getNumCells();
};
