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
	std::string  mFilePath;

	// Scan through a single line and add values to the contents vector
	void parseLine(std::string_view inputLine);

public:	
	bool loadData(const std::string& inputFilePath);

	bool saveData(const std::string& outputFilePath);

	std::vector<std::string>& getData();
	
	int getNumColumns();

	size_t getNumCells();
	
	const std::string& getFilePath();
};

void renderSpreadSheet(DataFrame& dataFrame, int currentStartIndex, int currentEndIndex, float cellWidth);

}
