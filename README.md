# Why Use This?
Like I was, you're probably frustrated by how much spreadsheet editors like Excel and LibreOffice Calc assume about values in CSV files, often converting them and losing specific formatting (e.g. dates and numbers left-padded with 0s). Preseving this data involves importing said file and converting all of its values to text, which can be a pain for making small changes to multiple files. This application is a quick and easy way to open, search, edit, and save CSV files while retaining all of their formatting at the single-character level.

# Libraries Used (THANK YOU!)
- Dear ImGui
- GLFW
- Native File Dialog Extended

# Compiling with CMake
1. Clone this repository
2. Install CMake (if you don't already have it)
3. Install Vulkan SDK using your preferred package manager (apt-get, winget, etc.)
4. Navigate to the main project directory
5. ~cmake -B build~
6. ~cmake --build build~
7. The binary should be deposited into the *build* directory
8. Have fun!
