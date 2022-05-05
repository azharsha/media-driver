///////////////////////////////////////////////////////////////////////////////
/*
 * Copyright (c) 2022, Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
///////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <map>
#include <algorithm>
#include <sys/stat.h>
#include <ctime>
#ifdef __linux__
#include <bits/stdc++.h>
#endif

static const int32_t  MAJ_VERSION   = 1;
static const int32_t  MIN_VERSION   = 1;
static const char *HEADER_EXT       = ".h";
static const char *SOURCE_EXT       = ".c";
static const char *PARAM_I          = "-i";
static const char *PARAM_O          = "-o";
static const char *PARAM_V          = "-v";
static const char *PARAM_INDEX      = "-index";
static const char *PARAM_TOTAL      = "-t";

#ifdef LINUX_
static const char *FILE_SEP         = "/";
#else
static const char *FILE_SEP         = "\\";
#endif

static std::string COPYRIGHT = "";
static const std::string copyright_front =
    "/*\n"
    " * Copyright (c) "
    ;
static const std::string copyright_end =
    ", Intel Corporation\n"
    " *\n"
    " * Permission is hereby granted, free of charge, to any person obtaining a\n"
    " * copy of this software and associated documentation files (the\n"
    " * 'Software'), to deal in the Software without restriction, including\n"
    " * without limitation the rights to use, copy, modify, merge, publish,\n"
    " * distribute, sublicense, and/or sell copies of the Software, and to\n"
    " * permit persons to whom the Software is furnished to do so, subject to\n"
    " * the following conditions:\n"
    " *\n"
    " * The above copyright notice and this permission notice shall be included\n"
    " * in all copies or substantial portions of the Software.\n"
    " *\n"
    " * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND, EXPRESS\n"
    " * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\n"
    " * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\n"
    " * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY\n"
    " * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,\n"
    " * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE\n"
    " * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.\n"
    "*/\n"
    "\n"
    "////////////////////////////////////////////////////////////////////////////////\n"
    "// !!! WARNING - AUTO GENERATED FILE. DO NOT EDIT DIRECTLY. !!!\n"
    "// Generated by KernelBinToSource.exe tool\n"
    "////////////////////////////////////////////////////////////////////////////////\n"
    ;


//-----------------------------------------------------------------------------
// String EndsWith
//-----------------------------------------------------------------------------
bool strEndsWith(
    const std::string &str,
    const std::string &substr)
{
    size_t i = str.rfind(substr);
    return (i != std::string::npos) && (i == (str.length() - substr.length()));
}

//-----------------------------------------------------------------------------
// Append Path
//-----------------------------------------------------------------------------
std::string appendPath(
    const std::string &sPath,
    const std::string &sSubPath)
{
    std::string strAppendPath = sPath;
    if (!strEndsWith(sPath, FILE_SEP))
    {
        strAppendPath += FILE_SEP;
    }
    strAppendPath += sSubPath;

    return strAppendPath;
}

//-----------------------------------------------------------------------------
// Get the filename from file path
//-----------------------------------------------------------------------------
std::string getFileName(const std::string &sFilePath)
{
#ifdef LINUX_
    std::string::size_type uiLoc = sFilePath.rfind("/");
#else
    std::string::size_type uiLoc = sFilePath.rfind("\\");
#endif

    if (uiLoc != std::string::npos)
    {
        return sFilePath.substr(uiLoc+1, sFilePath.length());
    }

    return sFilePath;
}

//-----------------------------------------------------------------------------
// Writes the Header file
//-----------------------------------------------------------------------------
int32_t writeHeaderFile(
    const uint32_t      *pBuffer,
    uint32_t             uiSize,
    const std::string   &sFileName,
    const std::string   &sVarName)
{
    int32_t             iStatus = -1;
    std::ofstream       oOutStream;
    std::stringstream   oSs;
    std::string         sHeaderSentry;
    std::string         sSizeName;

    oOutStream.open(sFileName.c_str(), std::ios::out | std::ios::trunc );
    if (!oOutStream.is_open())
    {
        printf("Error: Unable to open file '%s' for writing\n", sFileName.c_str());
        goto finish;
    }

    sHeaderSentry = "__" + sVarName + "_H__";
    sSizeName = "extern const unsigned int " + sVarName + "_SIZE";

    oSs << COPYRIGHT
        << "#ifndef " << sHeaderSentry << std::endl
        << "#define " << sHeaderSentry << std::endl << std::endl
        << sSizeName.c_str() << ";" << std::endl
        << "extern const unsigned int " << sVarName.c_str() << "[]" << ";"
        << std::endl << std::endl;

    oSs << "#endif // " << sHeaderSentry << std::endl;

    oOutStream << oSs.rdbuf();

    printf("Header file '%s' generated successfully!\n", sFileName.c_str());

    iStatus = 0;

finish:
    if (oOutStream.is_open())
    {
        oOutStream.close();
    }

    return iStatus;
}

//-----------------------------------------------------------------------------
// Writes Partial Source file
//-----------------------------------------------------------------------------
int32_t writePartialSourceFile(
    const uint32_t      *pBuffer,
    uint32_t            uiSize,
    const std::string   &sFileName,
    const std::string   &sVarName,
    const uint32_t      &index,
    const uint32_t      &total)
{
    int32_t             iStatus = -1;
    std::ofstream       oOutStream;
    std::stringstream   oSs;
    std::string         sSizeName;
    std::string         sPlatformName;
    const uint32_t uiHexLen = 11;
    char sHex[uiHexLen];
    uint32_t offset_size = 0;
    uint32_t final_size = (uiSize + total) * sizeof(uint32_t);
    uint32_t stub = 0;
    
    int gPos     = 0;
    int underPos = 0;

    oOutStream.open(sFileName.c_str(), std::ios::out | std::ios::trunc );
    if (!oOutStream.is_open())
    {
        printf("Error: Unable to open file '%s' for writing\n", sFileName.c_str());
        goto finish;
    }

    sSizeName = "extern const unsigned int " + sVarName + "_SIZE";

    sPlatformName = sVarName;

    // support platform/ip names of both G series and Xe series
    gPos = sPlatformName.find_first_of('G', 3);
    underPos = sPlatformName.find_first_of('_');
    if (gPos > 3 && gPos - 1 == underPos) 
    {
        sPlatformName.erase(0, gPos + 1);
        sPlatformName = "GEN" + sPlatformName;
    }
    else
    {
        sPlatformName.erase(0, underPos + 1);
        std::map<std::string, std::string> nameMap = {
            {"XE_HPG", "DG2"}, {"XE_HPG_CMFCPATCH", "DG2_CMFCPATCH"}
        };

        if (nameMap.find(sPlatformName) == nameMap.end())
        {
            printf("Error: Unable to recognize platform name '%s', KernelBinToSource tool should add support for newer platform\n", sPlatformName.c_str());
            goto finish;
        }
        sPlatformName = nameMap[sPlatformName];
    }

    oSs << COPYRIGHT << std::endl;
    oSs << "#ifdef IGFX_" << sPlatformName.c_str() << "_SUPPORTED" << std::endl;
    oSs << sSizeName.c_str() << " = " << final_size << ";" << std::endl
        << "extern const unsigned int " << sVarName.c_str() << "[] ="
        << std::endl << "{";

    for(stub = 0; stub < total; stub++)
    {
        if (stub % 8 == 0)
        {
            oSs << std::endl << "    ";
        }

        if( stub == index + 1)
        {
            offset_size = uiSize * sizeof(uint32_t);
        }

        snprintf(sHex, uiHexLen, "0x%08x", offset_size);

        sHex[uiHexLen - 1] = '\0';

        if (stub % 8 == 7)
        {
            oSs << sHex << ",";
        }else
        {
            oSs << sHex << ", ";
        }
    }

    for (uint32_t i = 0; i < uiSize; i++)
    {
        if ((i + stub) % 8 == 0)
        {
            oSs << std::endl << "    ";
        }

        snprintf(sHex, uiHexLen, "0x%08x", pBuffer[i]);
        sHex[uiHexLen - 1] = '\0';
        oSs << sHex;

        if (i < (uiSize - 1))
        {
            if ((i + stub) % 8 == 7)
            {
                oSs << ",";
            }
            else
            {
                oSs << ", ";
            }
        }
    }

    oSs << std::endl << "};" << std::endl;

    //Dummy kernel filled with 216 bytes
    oSs << "#else" << std::endl
        << sSizeName.c_str() << " = 216;" << std::endl
        << "extern const unsigned int " << sVarName.c_str() << "[] = {" << std::endl
        << "    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000," << std::endl
        << "};" << std::endl
        << "#endif" << std::endl;

    oOutStream << oSs.rdbuf();

    printf("Source file '%s' generated successfully!\n", sFileName.c_str());

    iStatus = 0;

finish:
    if (oOutStream.is_open())
    {
        oOutStream.close();
    }

    return iStatus;
}

//-----------------------------------------------------------------------------
// Writes the Source file
//-----------------------------------------------------------------------------
int32_t writeSourceFile(
    const uint32_t      *pBuffer,
    uint32_t            uiSize,
    const std::string   &sFileName,
    const std::string   &sVarName)
{
    int32_t             iStatus = -1;
    std::ofstream       oOutStream;
    std::stringstream   oSs;
    std::string         sSizeName;
    std::string         sPlatformName;
    const uint32_t uiHexLen = 11;
    char sHex[uiHexLen];

    int gPos     = 0;
    int underPos = 0;

    oOutStream.open(sFileName.c_str(), std::ios::out | std::ios::trunc );
    if (!oOutStream.is_open())
    {
        printf("Error: Unable to open file '%s' for writing\n", sFileName.c_str());
        goto finish;
    }

    sSizeName = "extern const unsigned int " + sVarName + "_SIZE";

    sPlatformName = sVarName;
    
    // support platform/ip names of both G series and Xe series
    gPos = sPlatformName.find_first_of('G', 3);
    underPos = sPlatformName.find_first_of('_');
    if (gPos > 3 && gPos - 1 == underPos)
    {
        sPlatformName.erase(0, gPos + 1);
        sPlatformName = "GEN" + sPlatformName;
    }
    else
    {
        sPlatformName.erase(0, underPos + 1);
        std::map<std::string, std::string> nameMap = {
            {"XE_HPG", "DG2"}, {"XE_HPG_CMFCPATCH", "DG2_CMFCPATCH"}
        };

        if (nameMap.find(sPlatformName) == nameMap.end())
        {
            printf("Error: Unable to recognize platform name '%s', KernelBinToSource tool should add support for newer platform\n", sPlatformName.c_str());
            goto finish;
        }
        sPlatformName = nameMap[sPlatformName];
    }

    oSs << COPYRIGHT << std::endl;
    oSs << "#ifdef IGFX_" << sPlatformName.c_str() << "_SUPPORTED" << std::endl;

    oSs << sSizeName.c_str() << " = " << uiSize * sizeof(uint32_t) << ";" << std::endl
        << "extern const unsigned int " << sVarName.c_str() << "[] ="
        << std::endl << "{";

    for (uint32_t i = 0; i < uiSize; i++)
    {
        if (i % 8 == 0)
        {
            oSs << std::endl << "    ";
        }

        snprintf(sHex, uiHexLen, "0x%08x", pBuffer[i]);
        sHex[uiHexLen - 1] = '\0';
        oSs << sHex;

        if (i < (uiSize - 1))
        {
            oSs << ", ";
        }
    }
    oSs << std::endl << "};" << std::endl;

    //Dummy kernel filled with 216 bytes
    oSs << "#else" << std::endl
        << sSizeName.c_str() << " = 216;" << std::endl
        << "extern const unsigned int " << sVarName.c_str() << "[] = {" << std::endl
        << "    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000," << std::endl
        << "};" << std::endl
        << "#endif" << std::endl;

    oOutStream << oSs.rdbuf();

    printf("Source file '%s' generated successfully!\n", sFileName.c_str());

    iStatus = 0;

finish:
    if (oOutStream.is_open())
    {
        oOutStream.close();
    }

    return iStatus;
}

//-----------------------------------------------------------------------------
// Creates the Source file
//-----------------------------------------------------------------------------
int32_t createSourceFile(
    const std::string &sInputFile,
    const std::string &sOutputDir,
    const std::string &sVar,
    const uint32_t &index,
    const uint32_t &total)
{
    struct stat     StatResult;
    int32_t         iStatus = -1;
    std::ifstream   oInStream;
    std::string     sInputFileName;
    std::string     sOutputFile;
    std::string     sVarName;
    uint32_t        *pBuffer    = nullptr;
    uint32_t        uiSize      = 0;
    float fSize     = (ceil)(0.0f);
    uint32_t uiIntSize  = 0;

    // Check if input file exists
    if (0 != stat(sInputFile.c_str(), &StatResult))
    {
        printf("Error: Unable to read file '%s'\n", sInputFile.c_str());
        goto finish;
    }
    uiSize = StatResult.st_size;

    if (sOutputDir.length() > 0)
    {
        // Check if out direction exists and validate it is a directory
        if (0 != stat(sOutputDir.c_str(), &StatResult))
        {
            printf("Error: Unable to find directory '%s'\n", sOutputDir.c_str());
            goto finish;
        }
        if (0 == (StatResult.st_mode & S_IFDIR))
        {
            printf("Error: Path '%s' is a valid directory\n", sOutputDir.c_str());
            goto finish;
        }
    }

    sInputFileName = getFileName(sInputFile);
    if (sOutputDir.find("media_softlet") != std::string::npos)
    {
        // For media softlet, move vp kernel from media ip to render ip
        std::string::size_type replaceIndex = sInputFileName.find("_hpm");

        if(replaceIndex != std::string::npos)
        {
            sInputFileName.replace(replaceIndex, 4, "_hpg");
        }
    }

    {
        // Set the Output path
        std::string::size_type uiLoc = sInputFileName.find(".", 0 );
        sOutputFile = (uiLoc != std::string::npos) ? sInputFileName.substr(0, uiLoc) : sInputFileName;
        sVarName = (sVar.length() == 0) ? sOutputFile : sVar;
    }

    // Read the File
    oInStream.open(sInputFile.c_str(), std::ios::in | std::ios::binary);
    if (! oInStream.is_open())
    {
        printf("Error: Unable to open file '%s' for reading\n", sInputFile.c_str());
        goto finish;
    }

    // Read from the file
    fSize     = (ceil)(uiSize / 4.0f);
    uiIntSize  = static_cast<uint32_t>(fSize);

    pBuffer = new uint32_t[uiIntSize];
    pBuffer[uiIntSize - 1] = 0; // set the last entry as 0 for handling non uint32_t file sizes.
    oInStream.read(reinterpret_cast<char*>(pBuffer), uiSize);

    if (oInStream.bad())
    {
        printf("Error: Unable to read from file '%s'\n", sInputFile.c_str());
        goto finish;
    }
    else
    {
        sOutputFile += SOURCE_EXT;
        sOutputFile = appendPath(sOutputDir, sOutputFile);

        std::transform(sVarName.begin(), sVarName.end(), sVarName.begin(), ::toupper);
        if (index == -1)
        {
            iStatus = writeSourceFile(pBuffer, uiIntSize, sOutputFile, sVarName);
        }
        else
        {
            iStatus = writePartialSourceFile(pBuffer, uiIntSize, sOutputFile, sVarName, index, total);
        }
    }

finish:
    if (oInStream.is_open())
    {
        oInStream.close();
    }

    if (pBuffer)
    {
        delete [] pBuffer;
        pBuffer = nullptr;
    }
    return iStatus;
}

//-----------------------------------------------------------------------------
// Creates the Header file
//-----------------------------------------------------------------------------
int32_t createHeaderFile(
    const std::string &sInputFile,
    const std::string &sOutputDir,
    const std::string &sVar)
{
    struct stat     StatResult;
    int32_t         iStatus = -1;
    std::ifstream   oInStream;
    std::string     sInputFileName;
    std::string     sOutputFile;
    std::string     sVarName;
    uint32_t        *pBuffer    = nullptr;
    uint32_t        uiSize      = 0;
    float fSize     = (ceil)(0.0f);
    uint32_t uiIntSize  = 0;

    // Check if input file exists
    if (0 != stat(sInputFile.c_str(), &StatResult))
    {
        printf("Error: Unable to read file '%s'\n", sInputFile.c_str());
        goto finish;
    }
    if (StatResult.st_size > 0)
    {
        uiSize = StatResult.st_size;
    }
    else
    {
        printf("Error: The read file '%s' is null!\n", sInputFile.c_str());
        goto finish;
    }

    if (sOutputDir.length() > 0)
    {
        // Check if out direction exists and validate it is a directory
        if (0 != stat(sOutputDir.c_str(), &StatResult))
        {
            printf("Error: Unable to find directory '%s'\n", sOutputDir.c_str());
            goto finish;
        }
        if (0 == (StatResult.st_mode & S_IFDIR))
        {
            printf("Error: Path '%s' is a valid directory\n", sOutputDir.c_str());
            goto finish;
        }
    }

    sInputFileName = getFileName(sInputFile);
    if (sOutputDir.find("media_softlet") != std::string::npos)
    {
        // For media softlet, move vp kernel from media ip to render ip
        std::string::size_type replaceIndex = sInputFileName.find("_hpm");

        if(replaceIndex != std::string::npos)
        {
            sInputFileName.replace(replaceIndex, 4, "_hpg");
        }
    }

    {
        // Set the Output path
        std::string::size_type uiLoc = sInputFileName.find(".", 0 );
        sOutputFile = (uiLoc != std::string::npos) ? sInputFileName.substr(0, uiLoc) : sInputFileName;
        sVarName = (sVar.length() == 0) ? sOutputFile : sVar;
    }

    // Read the File
    oInStream.open(sInputFile.c_str(), std::ios::in | std::ios::binary);
    if (! oInStream.is_open())
    {
        printf("Error: Unable to open file '%s' for reading\n", sInputFile.c_str());
        goto finish;
    }

    // Read from the file
    fSize     = (ceil)(uiSize / 4.0f);
    uiIntSize  = static_cast<uint32_t>(fSize);

    pBuffer = new uint32_t[uiIntSize];
    pBuffer[uiIntSize - 1] = 0; // set the last entry as 0 for handling non uint32_t file sizes.
    oInStream.read(reinterpret_cast<char*>(pBuffer), uiSize);

    if (oInStream.bad())
    {
        printf("Error: Unable to read from file '%s'\n", sInputFile.c_str());
        goto finish;
    }
    else
    {  
        sOutputFile += HEADER_EXT;
        sOutputFile = appendPath(sOutputDir, sOutputFile);
    }
    std::transform(sVarName.begin(), sVarName.end(), sVarName.begin(), ::toupper);
    iStatus = writeHeaderFile(pBuffer, uiIntSize, sOutputFile, sVarName);

finish:
    if (oInStream.is_open())
    {
        oInStream.close();
    }

    if (pBuffer)
    {
        delete [] pBuffer;
        pBuffer = nullptr;
    }
    return iStatus;
}

//-----------------------------------------------------------------------------
// Prints Usage
//-----------------------------------------------------------------------------
void printUsage(const std::string &sProgram)
{
    std::cout << "Usage: "
              << sProgram.c_str()
              << " (" << PARAM_I << " InPath)"
              << " [" << PARAM_O << " OutPath]"
              << " [" << PARAM_V << " VarName]"
              << " [" << PARAM_INDEX << " Kernel Index]"
              << " [" << PARAM_TOTAL << " Kernel Total]"
              << std::endl
              << "    " << PARAM_I << " Path to Kernel binary input file (required)"             << std::endl
              << "    " << PARAM_O << " Path to Kernel binary output directory (optional)"       << std::endl
              << "    " << PARAM_V << " Variable Name on the generated source file (optional)"   << std::endl
              << "    " << PARAM_INDEX << " Variable kernel Index  (optional)"                   << std::endl
              << "    " << PARAM_TOTAL << " Variable kernel total count (optional)"              << std::endl;

}

//-----------------------------------------------------------------------------
// Parses input
//-----------------------------------------------------------------------------
int32_t parseInput(
    int32_t             argc,
    char                *argv[],
    const std::string   &sProgram,
    std::string         &sInput,
    std::string         &sOutput,
    std::string         &sVarName,
    uint32_t            &index,
    uint32_t            &total)
{
    int32_t iStatus = -1;

    // Must have even number of arguments (excluding argv[0])
    if (argc % 2 == 0)
    {
        goto finish;
    }

    for (int i = 1; i < argc; i += 2)
    {
        if (0 == strcmp(PARAM_I, argv[i]))
        {
            sInput = argv[i+1];
        }
        else if (0 == strcmp(PARAM_O, argv[i]))
        {
            sOutput = argv[i+1];
        }
        else if (0 == strcmp(PARAM_V, argv[i]))
        {
            sVarName = argv[i+1];
        }
        else if (0 == strcmp(PARAM_INDEX, argv[i]))
        {
            index = atoi(argv[i+1]);
        }
        else if (0 == strcmp(PARAM_TOTAL, argv[i]))
        {
            total = atoi(argv[i+1]) + 1;
        }
        else
        {
            std::cout << "Error: Invalid option " << argv[i] << std::endl;
            goto finish;
        }
    }

    if (sInput.length() == 0)
    {
        goto finish;
    }

    iStatus = 0;

finish:
    if (iStatus != 0)
    {
        printUsage(sProgram);
    }

    return iStatus;
}

//-----------------------------------------------------------------------------
// Prints time
//-----------------------------------------------------------------------------
void printTime(int64_t lFreq, int64_t lTime)
{
    double dsec = ((double)lTime) / ((double)lFreq);
    double dms  = dsec * 1000;
    std::cout << "Time Took " << std::setw(2) << dsec << "s "
              << "(" << dms << "ms)"
              << std::endl;
}

//-----------------------------------------------------------------------------
// Main Function
//-----------------------------------------------------------------------------
int32_t main(int argc, char *argv[])
{
    std::string sProgram    = getFileName(argv[0]);
    //get current time
    time_t t = time(0);
    tm* now = localtime(&t);
    uint64_t year = 1900 + now->tm_year;

    std::cout << std::endl
              << sProgram.c_str() << " v" << MAJ_VERSION << "." << MIN_VERSION << std::endl
              << "(c) Intel Corporation " << year << ". All rights reserved." << std::endl << std::endl;

    COPYRIGHT = copyright_front + std::to_string(year) + copyright_end;

    int32_t iStatus = -1;
    std::string sInputPath;
    std::string sOutputDir;
    std::string sVarName;
    uint32_t index=-1;
    uint32_t total=-1;

    iStatus = parseInput(argc, argv, sProgram, sInputPath, sOutputDir, sVarName, index, total);
    if (iStatus != 0)
    {
        goto finish;
    }

    iStatus = createHeaderFile(sInputPath, sOutputDir, sVarName);

    iStatus = createSourceFile(sInputPath, sOutputDir, sVarName, index, total);

finish:
    return iStatus;
}
