#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace FetchCSV
{

class DataFrame
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

void renderSpreadSheet(DataFrame dataFrame, int currentStartIndex, int currentEndIndex);

}
