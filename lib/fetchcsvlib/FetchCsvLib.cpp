#include "FetchCsvLib.h"
#include <stdio.h>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

namespace FetchCSV
{

bool DataFrame::loadData(const std::string& inputFilePath, const char delimiter)
{

	// Clear the frame contents (if any exist currently)
	mFrameContents.clear();


	// Load the input file
	std::ifstream inputFile { inputFilePath };

	if (!inputFile)
	{
		std::cerr << "Error opening input file!" << '\n';
		return false;
	}			

	// Assign a file path
	mFilePath = inputFilePath;

	// First line of the file
	std::string inputLine {};
	std::getline(inputFile, inputLine);

	// This call to parseLine lets us get the number of columns
	parseLine(inputLine, delimiter);
	mNumColumns = mFrameContents.size();

	//Parse subsequent lines
	while (std::getline(inputFile, inputLine))
	{
		parseLine(inputLine, delimiter);
	}
	
	std::cout << "File " << inputFilePath << " loaded successfully" << '\n';
	std::cout << mFrameContents.size() << " cells loaded" << '\n';
	
	return true;

}

void DataFrame::parseLine(std::string_view inputLine, const char delimiter)
{
	std::string valueBuff {};
	bool isInQuotes { false };	

	// Add values to the buffer, then add to dataframe and clear buffer when we reach a comma
	for (const char ch : inputLine)
	{
		valueBuff += ch;
		
		if (ch == '"')
		{
			valueBuff.pop_back();
			isInQuotes = !isInQuotes;
		}

		if ( (ch == delimiter) && !(isInQuotes))
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
size_t DataFrame::getNumColumns() const { return mNumColumns; }
size_t DataFrame::getNumCells() const { return mFrameContents.size(); }
const std::string& DataFrame::getFilePath() const { return mFilePath; }

bool DataFrame::saveData(const std::string& outputFilePath, const char delimiter)
{
	std::ofstream outputFile { outputFilePath };

	if (!outputFile)
	{
		std::cerr << "Error loading output file!" << '\n';
		return false;
	}

	int index { 1 };

	for (const std::string& value : mFrameContents)
	{
		// Check for embedded comma(s)
		if (value.find(delimiter) != std::string::npos)
		{
			outputFile << '"' << value << '"';
		}
		else
		{
			outputFile << value;
		}

		// Check if it's the end of a row
		if (index % mNumColumns == 0)
		{
			outputFile << '\n';
		}
		else
		{
			outputFile << delimiter;
		}

		++index;

	}	

	return true;
}

void renderSpreadSheet(DataFrame& dataFrame, size_t currentStartIndex, size_t currentEndIndex, float cellWidth)
{
	const float maxLabelWidth {ImGui::CalcTextSize("0000000000").x}; //Support label widths up to a value of 10 billion - 1
	const size_t numColumns { dataFrame.getNumColumns() };
	for (size_t i { currentStartIndex }; i < currentEndIndex; ++i)
	{
		bool isStartOfLine = (i % numColumns == 0);
		bool isEndOfLine = ( !( ((i + 1) % numColumns == 0 )) );

		//Row label
		if (isStartOfLine)
		{
			// Convert to a std::string then return a c-style string for ImGui's text label
			std::string rowLabelStr = std::to_string(i / numColumns);
			const char* rowLabel = rowLabelStr.c_str();
			ImGui::Text(rowLabel);
			ImGui::SameLine();
			const float paddedTextWidth = ImGui::CalcTextSize(rowLabel).x;
			const float framePaddingWidth = ImGui::GetStyle().FramePadding.x;
			// Add dynamic padding (supports very long numbers)
			ImGui::Dummy((ImVec2( (maxLabelWidth - paddedTextWidth - (framePaddingWidth * 2)), 0.0)));
			ImGui::SameLine();
		}

		//Cell
		std::string cellLabel { "##Input" +  std::to_string(i) };
		ImGui::PushItemWidth(cellWidth); //TODO: A more dynamic item width
		ImGui::InputText(cellLabel.c_str(), &dataFrame.getData()[i]);
		ImGui::PopItemWidth();
		if (isEndOfLine)
		{
			ImGui::SameLine();
		}			

	}

}

}
