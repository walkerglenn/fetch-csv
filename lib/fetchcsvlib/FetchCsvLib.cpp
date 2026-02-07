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

void renderSpreadSheet(DataFrame dataFrame, int currentStartIndex, int currentEndIndex)
{
	int numColumns { dataFrame.getNumColumns() };
	for (int i { currentStartIndex }; i < currentEndIndex; ++i)
	{
		bool isStartOfLine = (i % numColumns == 0);
		bool isEndOfLine = ( !( ((i + 1) % numColumns == 0 )) );

		//Row label
		if (isStartOfLine)
		{
			const char* rowLabel = std::to_string(i / numColumns).c_str();
			ImGui::Text(rowLabel);
			ImGui::SameLine();
			const float paddedTextWidth = ImGui::CalcTextSize(rowLabel).x;
			const float framePaddingWidth = ImGui::GetStyle().FramePadding.x;
			// Add dynamic padding (supports very long numbers) TODO: Replace magic number 200.0 with something that makes sense
			ImGui::Dummy((ImVec2( (200.0 - paddedTextWidth - (framePaddingWidth * 2)), 0.0)));
			ImGui::SameLine();
		}

		//Cell
		std::string cellLabel { "##Input" +  std::to_string(i) };
		ImGui::PushItemWidth(200.0f); //TODO: A more dynamic item width
		ImGui::InputText(cellLabel.c_str(), &dataFrame.getData()[i]);
		ImGui::PopItemWidth();
		if (isEndOfLine)
		{
			ImGui::SameLine();
		}			

	}

}

}
