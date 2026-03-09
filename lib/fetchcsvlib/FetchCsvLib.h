#pragma once

#include "imgui.h"
#include "imgui_stdlib.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <optional>

namespace FetchCSV
{

class DataFrame
{

private:
	std::vector<std::string> mFrameContents;
	size_t  mNumColumns;
	std::string  mFilePath;

	// Scan through a single line and add values to the contents vector
	void parseLine(std::string_view inputLine, const char delimiter);

public:	
	bool loadData(const std::string& inputFilePath, const char delimiter = ',');

	bool saveData(const std::string& outputFilePath, const char delimiter = ',');

	std::vector<std::string>& getData();
	
	size_t getNumColumns() const;

	size_t getNumCells() const;
	
	const std::string& getFilePath() const;

	std::optional< size_t> getIndexOfValue(std::string_view searchValue);

};

void renderSpreadSheet(DataFrame& dataFrame, size_t currentStartIndex, size_t currentEndIndex, float cellWidth, std::pair<bool, size_t>& searchState);

}
