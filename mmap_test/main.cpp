// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>
#include <iostream>

#define BUF_SIZE 1024*64
#define VIEW_SIZE BUF_SIZE / 2

int main(int argc, char* argv[])
{
    std::cout << "Hello MMF!\n";
    bool restore = false;

    if (argc > 1 && std::string(argv[1]) == "-r") 
    {
        std::cout << "restore heap\n";
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
    LPVOID lpView = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, VIEW_SIZE);
    if (lpView == NULL)
    {
        std::cerr << "MapViewOfFile failed: " << GetLastError() << std::endl;
        CloseHandle(hMap);
        CloseHandle(hFile);
        return 3;
    }

    // Access the committed page as a normal pointer
    struct Foo
    {
        bool BarBool;
        char* BarString;
        int BarInt;
    };

    if (restore)
    {
        LPVOID current = lpView;

        // read the base address
        long base = *reinterpret_cast<long*>(current);

        // save base address as first item
        *(long*)current = (long)current;

        current = (LPVOID)((char*)lpView + sizeof(LPVOID));

        // load the structure
        Foo* p = (Foo*)current;
        int barStringOffset = (long)p->BarString - base;
        p->BarString =  (char*)lpView + barStringOffset;

        std::cout
            << "lpView: " << std::addressof(lpView) << "\n"
            << "barStringOffset: " << barStringOffset << "\n";

        std::cout 
            << p->BarBool << ", "
            << "0x" << std::hex << p->BarInt << ", "
            << p->BarString << std::endl;
    }
    else 
    {
        // save base address as first item
        LPVOID current = lpView;
        *(long*)current = (long)current;

        current = (LPVOID)((char*)lpView + sizeof(LPVOID));

        // save structure content
        Foo* p = (Foo*)current;
        p->BarBool = true;
        p->BarInt = 0x12345678;

        current = (LPVOID)((char*)current + sizeof(Foo));

        //void* pBarString = (char*)lpView + sizeof(Foo);
        CopyMemory(current, "Hello world!", 12);
        p->BarString = (char*)current;
    }

    UnmapViewOfFile(lpView);
    CloseHandle(hMap);
    CloseHandle(hFile);

    return 0;
}

