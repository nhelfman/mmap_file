// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>
#include <iostream>

#define BUF_SIZE 1024*64
#define VIEW_SIZE BUF_SIZE / 2

// pointer to view on mapped view
LPVOID lpView = __nullptr;

// type for defining based pointers 
typedef char __based(lpView) *pBasedPtr; 

struct Foo
{
    bool BarBool;
    pBasedPtr BarString;
    int BarInt;
};

void printFoo(Foo* p)
{
    std::cout
        << "Foo = \n"
        << "{\n"
        << "\tBarBool = " << p->BarBool << ";\n"
        << "\tBarString = " << (char*)p->BarString << ";\n"
        << "\tBarInt = 0x" << std::hex << p->BarInt << ";\n"
        << "}\n";
}

int main(int argc, char* argv[])
{
    std::cout << "Starting\n";
    bool restore = false;

    if (argc > 1 && std::string(argv[1]) == "-r") 
    {
        restore = true;
    }

    const char* filename = "mapped.bin";
    wchar_t wfilename[MAX_PATH];

    // Convert the filename to a Unicode string
    MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, MAX_PATH);

    // Create a file handle
    HANDLE hFile = CreateFileW(wfilename,
        GENERIC_READ | GENERIC_WRITE, 
        0,  // share mode - no share
        NULL, 
        restore ? OPEN_EXISTING : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        std::cerr << "CreateFile failed: " << GetLastError() << std::endl;
        return 1;
    }

    // Create a file mapping object
    HANDLE hMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, BUF_SIZE, NULL);
    if (hMap == NULL)
    {
        std::cerr << "CreateFileMapping failed: " << GetLastError() << std::endl;
        CloseHandle(hFile);
        return 2;
    }

    // Map a view of the file into the address space
    lpView = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, VIEW_SIZE);
    if (lpView == NULL)
    {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(hMap);
        CloseHandle(hFile);
        return 3;
    }

    if (restore)
    {
        std::cout << "Restore heap from file\n";

        LPVOID current = lpView;

        // load the structure from memory
        Foo* p = (Foo*)current;
        std::cout << "lpView: " << std::addressof(lpView) << "\n";

        printFoo(p);
    }
    else 
    {
        std::cout << "Mapping structure to mmap file\n";
        std::cout << "lpView: " << std::addressof(lpView) << "\n\n";

        // save base address as first item
        LPVOID current = lpView;
       
        // save structure content
        Foo* p = (Foo*)current;
        p->BarBool = true;
        p->BarInt = 0x12345678;

        int strOffset = sizeof(Foo);
        current = (LPVOID)((char*)current + strOffset);

        CopyMemory(current, "Hello world!", 12);
        p->BarString = (pBasedPtr)strOffset;

        printFoo(p);
    }

    UnmapViewOfFile(lpView);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return 0;
}



