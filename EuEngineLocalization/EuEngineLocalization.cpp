// EuEngineLocalization.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "EuEngineLocalization.h"
#include "stdlib.h"

// This is an example of an exported variable
EUENGINELOCALIZATION_API int nEuEngineLocalization=0;

#define FLAG_EXTRA_SPACES 0x1

// This is an example of an exported function.

EUENGINELOCALIZATION_API void __stdcall InitCharInfoArray(BYTE*** pppCharBase, DWORD index, BYTE* pCharInfo)
{
    // pppCharBase is the address of where the 0x00 character charinfo suppose to stay if it was not crasked.
    // ppCharInfoHolder is a 65535 * sizeof(byte*) array.
    BYTE** ppCharInfoHolder = *pppCharBase;
    // ppCharInfoHolder[0] (not *ppCharInfoHolder[0]) will store info about last character info.
    // ppCharInfoHolder[1] (not *ppCharInfoHolder[1]) will store the substituted char.
    // ppCharInfoHolder[2] to ppCharInfoHolder[6] (not *ppCharInfoHolder[1], etc) will be used for a temporary fake character info if necessary.
    if (!ppCharInfoHolder)
    {
        *pppCharBase = (BYTE**)malloc(65535*sizeof(BYTE*));
        ppCharInfoHolder = *pppCharBase;
        if (!ppCharInfoHolder)
            return;
        for (int i = 0; i < 65535; ++i)
            ppCharInfoHolder[i] = 0;
    }
    // memleak if there's too many characters!
    if (index > 65535)
        return;
    ppCharInfoHolder[index] = pCharInfo;
    if (index == 0x20)
    {
        BYTE** ppOriginalCharInfoHolder = (BYTE**)pppCharBase;
        ppOriginalCharInfoHolder[0x20] = pCharInfo;
    }
    else if (index == 0x40)
    {
        // fill the regular space with the info of 0x40 ('@') to detect errors
        BYTE** ppOriginalCharInfoHolder = (BYTE**)pppCharBase;
        for (unsigned i = 0x21; i < 0xff; ++i)
        {
            ppOriginalCharInfoHolder[i] = (BYTE*)malloc(16);
            for (int j = 0; j < 4; ++j)
                ((DWORD*)ppOriginalCharInfoHolder[i])[j] = ((DWORD*)pCharInfo)[j];
        }
    }
}


// Check if the character is part of a Chinese character or not. 
// If it is, substitute it to a dummy character to bypass all special symbol checks.
// return the fake character.
EUENGINELOCALIZATION_API DWORD __stdcall LoadCharInfoPhase1(unsigned char* str, int index, BYTE*** pppCharBase, int /*flag*/)
{
    // pppCharBase is the address of where the 0x00 character charinfo suppose to stay if it was not crasked.
    // ppCharInfoHolder is a 65535 * sizeof(byte*) array.
    BYTE** ppCharInfoHolder = *pppCharBase;
    // ppCharInfoHolder[0] (not *ppCharInfoHolder[0]) will store info about last character info.
    // ppCharInfoHolder[1] (not *ppCharInfoHolder[1]) will store the substituted char.
    // ppCharInfoHolder[2] to ppCharInfoHolder[6] (not *ppCharInfoHolder[1], etc) will be used for a temporary fake character info if necessary.
    DWORD& lastCharInfo = (DWORD&)(ppCharInfoHolder[0]);
    unsigned char& sub = (unsigned char&)(ppCharInfoHolder[1]);
    BYTE* fakeChar = (BYTE*)&ppCharInfoHolder[2];

    // always clear the sub character;
    sub = 0;
    if (index == 0)
        lastCharInfo = 0;
    if (lastCharInfo != 0)
    {
        // second character of a Chinese character.
        // substitute the character and let the phase 2 process the rest.
        sub = str[index];
        str[index] = 0x2e;
    }
    else
    {
        unsigned char fc = str[index];
        if (fc < 128)
        {
            // let ascii go
        }
        else
        {
            // 0xa3 + pic name(probably ascii): external picture. 
            // 0xa4: currency sign, probably followed by space
            // 0xa7 + color code (probably ascii): change color
            if (fc == 0xa7 || fc == 0xa3 || fc == 0xa4)
            {
                unsigned char sc = str[index + 1];
                if (sc > 128)
                {
                    // 0xa3/0xa4/0xa7 + larger than 128 second byte: probably a chinese character.
                    sub = str[index];
                    str[index] = 0x2e;
                }
            }
            else
            {
                // probably Chinese character
                sub = str[index];
                str[index] = 0x2e;
            }
        }
    }

    return str[index];
}

EUENGINELOCALIZATION_API void* __stdcall LoadCharInfoPhase2(unsigned char* str, int index, BYTE*** pppCharBase, int flag)
{
    // pppCharBase is the address of where the 0x00 character charinfo suppose to stay if it was not crasked.
    // ppCharInfoHolder is a 65535 * sizeof(byte*) array.
    BYTE** ppCharInfoHolder = *pppCharBase;
    void* returnValue;
    // ppCharInfoHolder[0] (not *ppCharInfoHolder[0]) will store info about last character info.
    // ppCharInfoHolder[1] (not *ppCharInfoHolder[1]) will store the substituted char.
    // ppCharInfoHolder[2] to ppCharInfoHolder[6] (not *ppCharInfoHolder[1], etc) will be used for a temporary fake character info if necessary.
    DWORD& lastCharInfo = (DWORD&)(ppCharInfoHolder[0]);
    unsigned char& sub = (unsigned char&)(ppCharInfoHolder[1]);
    BYTE* fakeChar = (BYTE*)&ppCharInfoHolder[2];

    // recover substituted character
    if (sub != 0)
    {
        str[index] = sub;
        sub = 0;
    }
    if (index == 0)
        lastCharInfo = 0;
    if (lastCharInfo != 0)
    {
        // second character of a Chinese character.
        // prepare fackChar according to flags.
        if ((flag & FLAG_EXTRA_SPACES) == FLAG_EXTRA_SPACES)
            {
                if (str[index] != 0x20)
                {
                    returnValue = ppCharInfoHolder[0x2e];
                    lastCharInfo = 0;
                }
                else 
                    returnValue = ppCharInfoHolder[0x20];
            }
        else
        {
            returnValue = fakeChar;
            lastCharInfo = 0;
        }
        
    }
    else
    {
        unsigned char fc = str[index];
        if (fc == 0)
        {
            returnValue = nullptr;
        }
        else if (fc < 128)
        {
            // ascii
            returnValue = ppCharInfoHolder[(unsigned)fc];
        }
        else
        {
            unsigned char sc = str[index+1];
            if (sc == 0)
            {
                returnValue = ppCharInfoHolder[(unsigned)fc];
            }
            else
            {
                if (sc == 0x20 && ((flag & FLAG_EXTRA_SPACES) == FLAG_EXTRA_SPACES))
                {
                    for (int i = index+2; str[i] != 0; ++i)
                    {
                        if (str[i] != 0x20)
                        {
                            sc = str[i];
                            /*str[index + 1] = str[i];
                            str[i] = 0x2e;*/
                            break;
                        }
                    }
                    // sc = str[index + 1];
                }
                returnValue = ppCharInfoHolder[((unsigned)sc) << 8 | fc];
                if (returnValue)
                {
                    lastCharInfo = fc;
                }
                else
                {
                    returnValue = ppCharInfoHolder[(unsigned)fc];
                }             
            }
        }
    }

    return returnValue;
}


EUENGINELOCALIZATION_API void __stdcall Utf8ToGB(unsigned char* str)
{
    unsigned char* cur = str;
    while (*cur != 0)
        ++cur;
    int size = cur - str;
    wchar_t buf[200];

    if (MultiByteToWideChar(CP_UTF8, 0, (LPCCH)str, -1, buf, 200) > 0)
    {
        WideCharToMultiByte(936, 0, buf, -1, (LPSTR)str, size, nullptr, nullptr);
    }
}