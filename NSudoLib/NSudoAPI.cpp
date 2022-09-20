/*
 * PROJECT:   NSudo Shared Library
 * FILE:      NSudoAPI.cpp
 * PURPOSE:   Implementation for NSudo Shared Library
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#include "NSudoAPI.h"

#include "Mile.Windows.h"

#include "M2.Base.h"

#include <cstdio>
#include <cwchar>

#include <type_traits>
#include <utility>

namespace Mile
{
    /**
     * Disable C++ Object Copying
     */
    class DisableObjectCopying
    {
    protected:
        DisableObjectCopying() = default;
        ~DisableObjectCopying() = default;

    private:
        DisableObjectCopying(
            const DisableObjectCopying&) = delete;
        DisableObjectCopying& operator=(
            const DisableObjectCopying&) = delete;
    };

    /**
     * Disable C++ Object Moving
     */
    class DisableObjectMoving
    {
    protected:
        DisableObjectMoving() = default;
        ~DisableObjectMoving() = default;

    private:
        DisableObjectMoving(
            const DisableObjectMoving&&) = delete;
        DisableObjectMoving& operator=(
            const DisableObjectMoving&&) = delete;
    };

    /**
     * Scope Exit Event Handler (ScopeGuard)
     */
    template<typename EventHandlerType>
    class ScopeExitEventHandler : DisableObjectCopying, DisableObjectMoving
    {
    private:
        bool m_Canceled;
        EventHandlerType m_EventHandler;

    public:

        ScopeExitEventHandler() = delete;

        explicit ScopeExitEventHandler(EventHandlerType&& EventHandler) :
            m_Canceled(false),
            m_EventHandler(std::forward<EventHandlerType>(EventHandler))
        {

        }

        ~ScopeExitEventHandler()
        {
            if (!this->m_Canceled)
            {
                this->m_EventHandler();
            }
        }

        void Cancel()
        {
            this->m_Canceled = true;
        }
    };
}

/**
 * @remark You can read the definition for this function in "NSudoAPI.h".
 */
EXTERN_C HRESULT WINAPI NSudoCreateProcess(
    _In_ LPCWSTR CommandLine,
    _In_opt_ LPCWSTR CurrentDirectory)
{
    DWORD MandatoryLabelRid = SECURITY_MANDATORY_SYSTEM_RID;
    DWORD ProcessPriority = ABOVE_NORMAL_PRIORITY_CLASS;
    DWORD ShowWindowMode = SW_SHOWDEFAULT;

    HRESULT hr = S_OK;

    DWORD SessionID = static_cast<DWORD>(-1);

    HANDLE CurrentProcessToken = INVALID_HANDLE_VALUE;
    HANDLE DuplicatedCurrentProcessToken = INVALID_HANDLE_VALUE;
    
    HANDLE OriginalLsassProcessToken = INVALID_HANDLE_VALUE;
    HANDLE SystemToken = INVALID_HANDLE_VALUE;

    HANDLE hToken = INVALID_HANDLE_VALUE;
    HANDLE OriginalToken = INVALID_HANDLE_VALUE;

    auto Handler = Mile::ScopeExitEventHandler([&]()
        {
            if (CurrentProcessToken !=INVALID_HANDLE_VALUE)
            {
                ::MileCloseHandle(CurrentProcessToken);
            }

            if (DuplicatedCurrentProcessToken != INVALID_HANDLE_VALUE)
            {
                ::MileCloseHandle(DuplicatedCurrentProcessToken);
            }

            if (OriginalLsassProcessToken != INVALID_HANDLE_VALUE)
            {
                ::MileCloseHandle(OriginalLsassProcessToken);
            }

            if (SystemToken != INVALID_HANDLE_VALUE)
            {
                ::MileCloseHandle(SystemToken);
            }

            if (hToken != INVALID_HANDLE_VALUE)
            {
                ::MileCloseHandle(hToken);
            }

            if (OriginalToken != INVALID_HANDLE_VALUE)
            {
                ::MileCloseHandle(OriginalToken);
            }

            ::MileSetCurrentThreadToken(nullptr);
        });

    DWORD ReturnLength = 0;

    hr = ::MileOpenCurrentProcessToken(
        MAXIMUM_ALLOWED, &CurrentProcessToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileDuplicateToken(
        CurrentProcessToken,
        MAXIMUM_ALLOWED,
        nullptr,
        SecurityImpersonation,
        TokenImpersonation,
        &DuplicatedCurrentProcessToken);
    if (hr != S_OK)
    {
        return hr;
    }

    LUID_AND_ATTRIBUTES RawPrivilege;

    hr = ::MileGetPrivilegeValue(SE_DEBUG_NAME, &RawPrivilege.Luid);
    if (hr != S_OK)
    {
        return hr;
    }

    RawPrivilege.Attributes = SE_PRIVILEGE_ENABLED;

    hr = ::MileAdjustTokenPrivilegesSimple(
        DuplicatedCurrentProcessToken,
        &RawPrivilege,
        1);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileSetCurrentThreadToken(DuplicatedCurrentProcessToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileOpenCurrentThreadToken(
        MAXIMUM_ALLOWED, FALSE, &CurrentProcessToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileGetTokenInformation(
        CurrentProcessToken,
        TokenSessionId,
        &SessionID,
        sizeof(DWORD),
        &ReturnLength);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileOpenLsassProcessToken(
        MAXIMUM_ALLOWED,
        &OriginalLsassProcessToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileDuplicateToken(
        OriginalLsassProcessToken,
        MAXIMUM_ALLOWED,
        nullptr,
        SecurityImpersonation,
        TokenImpersonation,
        &SystemToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileAdjustTokenAllPrivileges(
        SystemToken,
        SE_PRIVILEGE_ENABLED);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileSetCurrentThreadToken(SystemToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileOpenServiceProcessToken(
        L"TrustedInstaller",
        MAXIMUM_ALLOWED,
        &OriginalToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileDuplicateToken(
        OriginalToken,
        MAXIMUM_ALLOWED,
        nullptr,
        SecurityIdentification,
        TokenPrimary,
        &hToken);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileSetTokenInformation(
        hToken,
        TokenSessionId,
        (PVOID)&SessionID,
        sizeof(DWORD));
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileAdjustTokenAllPrivileges(hToken, SE_PRIVILEGE_ENABLED);
    if (hr != S_OK)
    {
        return hr;
    }

    hr = ::MileSetTokenMandatoryLabel(hToken, MandatoryLabelRid);
    if (hr != S_OK)
    {
        return hr;
    }    

    DWORD dwCreationFlags = CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | CREATE_NEW_CONSOLE;

    STARTUPINFOW StartupInfo = { 0 };
    PROCESS_INFORMATION ProcessInfo = { 0 };

    StartupInfo.cb = sizeof(STARTUPINFOW);

    StartupInfo.lpDesktop = const_cast<LPWSTR>(L"WinSta0\\Default");

    StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = static_cast<WORD>(ShowWindowMode);

    LPVOID lpEnvironment = nullptr;

    LPWSTR ExpandedString = nullptr;

    hr = ::MileCreateEnvironmentBlock(&lpEnvironment, hToken, TRUE);
    if (hr == S_OK)
    {
        hr = ::MileExpandEnvironmentStringsWithMemory(
            CommandLine,
            &ExpandedString);
        if (hr == S_OK)
        {
            hr = ::MileCreateProcessAsUser(
                hToken,
                nullptr,
                ExpandedString,
                nullptr,
                nullptr,
                FALSE,
                dwCreationFlags,
                lpEnvironment,
                CurrentDirectory,
                &StartupInfo,
                &ProcessInfo);
            if (hr == S_OK)
            {
                ::MileSetPriorityClass(
                    ProcessInfo.hProcess, ProcessPriority);

                ::MileResumeThread(ProcessInfo.hThread, nullptr);

                ::MileWaitForSingleObject(
                    ProcessInfo.hProcess, 0, FALSE, nullptr);

                ::MileCloseHandle(ProcessInfo.hProcess);
                ::MileCloseHandle(ProcessInfo.hThread);
            }

            ::MileFreeMemory(ExpandedString);
        }

        ::MileDestroyEnvironmentBlock(lpEnvironment);
    }

    return S_OK;
}

