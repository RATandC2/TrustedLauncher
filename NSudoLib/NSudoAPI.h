/*
 * PROJECT:   NSudo Shared Library
 * FILE:      NSudoAPI.h
 * PURPOSE:   Definition for NSudo Shared Library
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#ifndef NSUDO_API
#define NSUDO_API

#ifndef __cplusplus
#error "[NSudoAPI] You should use a C++ compiler."
#endif

#include <Windows.h>

EXTERN_C HRESULT WINAPI NSudoCreateProcess(
    _In_ LPCWSTR CommandLine,
    _In_opt_ LPCWSTR CurrentDirectory);

#endif
