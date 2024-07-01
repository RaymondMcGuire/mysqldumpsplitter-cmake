// Copyright (C) 2011 by RODO - INTELLIGENT COMPUTING (www.rodo.nl)
//
// 2017-2018 - fixes by Viachaslau Tratsiak (aka restorer)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

enum class ReturnValues {
  Success = 0,
  BadArguments,
  CanNotOpen,
  PrematureEndSQL,
  CanNotOpenForWrite,
  CanNotWrite,
  MaxByteSizeExceeded,
  UnknownReason
};

bool isNewLine(char c) { return (c == '\n' || c == '\r'); }

void outputStatistics(size_t currentBytes, size_t maxBytes,
                      const std::string &prefix, const std::string &suffix = "",
                      size_t barWidth = 37) {
  std::cout << '\n' << std::setw(4) << std::right << prefix << '[';

  float ratio = static_cast<float>(currentBytes) / maxBytes;
  size_t maxBar = static_cast<size_t>(ratio * barWidth);

  std::cout << std::string(maxBar, '=') << std::string(barWidth - maxBar, ' ')
            << ']' << suffix;
  std::cout.flush();
}

std::string createOutputFilename(const fs::path &inputPath, size_t partCount) {
  std::string stem = inputPath.stem().string();
  std::string extension = inputPath.extension().string();

  std::ostringstream partStream;
  partStream << std::setw(5) << std::setfill('0') << partCount;

  return stem + "-" + partStream.str() + extension;
}

ReturnValues processFile(const fs::path &inputPath, size_t maxByteSize,
                         size_t outputBarWidth) {
  std::ifstream sqlFile(inputPath, std::ios::binary);
  if (!sqlFile) {
    std::cerr << "Can't open file (" << inputPath << ") for reading\n";
    return ReturnValues::CanNotOpen;
  }

  std::cout << "Good to go! Will split (" << inputPath
            << ") to separate files with a maximum size of " << maxByteSize
            << " bytes" << std::endl;

  std::string lastReadStatement;
  for (size_t partCount = 0;; ++partCount) {
    std::string outputFilename = createOutputFilename(inputPath, partCount);
    std::ofstream currentOutputFile(outputFilename, std::ios::binary);

    if (!currentOutputFile) {
      std::cerr << "Fatal: Failed to open (" << outputFilename
                << ") for writing" << std::endl;
      return ReturnValues::CanNotOpenForWrite;
    }

    std::cout << "Will write part [" << partCount << "] to " << outputFilename
              << std::endl;

    std::string blockOfStatements = lastReadStatement;
    lastReadStatement.clear();

    while (sqlFile) {
      std::string singleStatement;
      bool insideQuotes = false;
      bool isEscaped = false;

      for (char current; sqlFile.get(current);) {
        if (isEscaped) {
          isEscaped = false;
        } else if (current == '\\') {
          isEscaped = true;
        } else if (current == '\'') {
          insideQuotes = !insideQuotes;
        } else if (current == ';' && !insideQuotes) {
          singleStatement += current;
          break;
        }
        singleStatement += current;

        if (singleStatement.length() > 1ULL * 1024 * 1024 * 1024) {
          std::cerr << std::endl
                    << "Too long statement - probably an internal bug"
                    << std::endl;
          return ReturnValues::MaxByteSizeExceeded;
        }
      }

      std::string linePreview = singleStatement.substr(0, 40);
      linePreview.erase(
          std::remove_if(linePreview.begin(), linePreview.end(), isNewLine),
          linePreview.end());

      outputStatistics(blockOfStatements.length(), maxByteSize,
                       std::to_string(partCount) + " ",
                       " " + linePreview + "...", outputBarWidth);

      if (singleStatement.length() > maxByteSize) {
        std::cerr << std::endl
                  << "Fatal: Smallest statement is bigger ("
                  << singleStatement.length() << ") than given bytesize ("
                  << maxByteSize << ")" << std::endl;
        return ReturnValues::MaxByteSizeExceeded;
      }

      if (blockOfStatements.length() + singleStatement.length() > maxByteSize) {
        lastReadStatement = singleStatement;
        break;
      }

      blockOfStatements += singleStatement;
    }

    outputStatistics(blockOfStatements.length(), maxByteSize,
                     std::to_string(partCount), " Writing to file\n",
                     outputBarWidth);
    currentOutputFile << blockOfStatements;

    if (!currentOutputFile) {
      std::cerr << "Fatal: Failed to write to output file, aborting\n";
      return ReturnValues::CanNotWrite;
    }

    if (!sqlFile) {
      break; // We're done!
    }
  }

  return ReturnValues::Success;
}

int main(int argc, char *const argv[]) {
  if (argc < 3) {
    std::cout << "Usage: " << argv[0]
              << " <input file> <maximum output file size in bytes> [output "
                 "bar width]\n";
    return static_cast<int>(ReturnValues::BadArguments);
  }

  fs::path inputFilePath = argv[1];
  size_t maxByteSize = std::stoull(argv[2]);
  size_t outputBarWidth = (argc >= 4) ? std::stoul(argv[3]) : 60;

  return static_cast<int>(
      processFile(inputFilePath, maxByteSize, outputBarWidth));
}