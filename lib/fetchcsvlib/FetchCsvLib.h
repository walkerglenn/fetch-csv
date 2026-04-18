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

struct AppState
{
        // This tells our spreadsheet rendering system if we need to focus on a particular cell that has a searched value in it
        std::pair<bool, size_t> searchState {false, 0};

	// Bool value that controls whether or not to search for query as full cell value, or merely a substring of the cell value
	bool isExactChecked { true };

        // Pagination variables
        size_t pageStartIndex {};
        size_t pageEndIndex {};
        size_t numRowsToDisplay { 1'000 };

        // Cell
        float cellWidth { 200.0f };

        // Spreadsheet
	bool shouldLoadCsv { true };
	char delimiter { ',' };

        // Rendering bools
        bool shouldRenderHeaders { true };
        bool showValueSearchWindow { false };
	bool showCustomDelimiterWindow { false };
	bool showSetNumRowsWindow { false };
};


class DataFrame
{

private:
	std::vector<std::string> mFrameContents {};
	size_t  mNumColumns {};
	std::string  mFilePath {};

	// Scan through a single line and add values to the contents vector
	void parseLine(std::string_view inputLine, const char delimiter);

public:	
	bool loadData(const std::string& inputFilePath, const char delimiter = ',');

	bool saveData(const std::string& outputFilePath, const char delimiter = ',');

	std::vector<std::string>& getData();
	
	size_t getNumColumns() const;

	size_t getNumCells() const;
	
	const std::string& getFilePath() const;

	std::optional< size_t> getIndexOfValue(std::string_view searchValue, bool isExact, size_t startingIndex = 0);

};

void renderSpreadSheet(DataFrame& dataFrame, size_t currentStartIndex, size_t currentEndIndex,  AppState& appState);

}
