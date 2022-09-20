/*
 * PROJECT:   M2-Team Common Library
 * FILE:      M2WindowsHelpers.h
 * PURPOSE:   Definition for the Windows helper functions
 *
 * LICENSE:   The MIT License
 *
 * DEVELOPER: Mouri_Naruto (Mouri_Naruto AT Outlook.com)
 */

#pragma once

#ifndef _M2_WINDOWS_EXTENDED_HELPERS_
#define _M2_WINDOWS_EXTENDED_HELPERS_

#include <Mile.Windows.h>

/**
 * If the type T is a reference type, provides the member typedef type which is
 * the type referred to by T. Otherwise type is T.
 */
template<class T> struct M2RemoveReference { typedef T Type; };
template<class T> struct M2RemoveReference<T&> { typedef T Type; };
template<class T> struct M2RemoveReference<T&&> { typedef T Type; };
#ifdef __cplusplus_winrt
template<class T> struct M2RemoveReference<T^> { typedef T Type; };
#endif

namespace M2
{
    /**
     * Disable C++ Object Copying
     */
    class CDisableObjectCopying
    {
    protected:
        CDisableObjectCopying() = default;
        ~CDisableObjectCopying() = default;

    private:
        CDisableObjectCopying(
            const CDisableObjectCopying&) = delete;
        CDisableObjectCopying& operator=(
            const CDisableObjectCopying&) = delete;
    };

    /**
     * Disable C++ Object Moving
     */
    class CDisableObjectMoving
    {
    protected:
        CDisableObjectMoving() = default;
        ~CDisableObjectMoving() = default;

    private:
        CDisableObjectMoving(
            const CDisableObjectCopying&&) = delete;
        CDisableObjectMoving& operator=(
            const CDisableObjectCopying&&) = delete;
    };

    /**
     * The implementation of smart object.
     */
    template<typename TObject, typename TObjectDefiner>
    class CObject : CDisableObjectCopying, CDisableObjectMoving
    {
    protected:
        TObject m_Object;
    public:
        CObject(TObject Object = TObjectDefiner::GetInvalidValue()) :
            m_Object(Object)
        {

        }

        ~CObject()
        {
            this->Close();
        }

        TObject* operator&()
        {
            return &this->m_Object;
        }

        TObject operator=(TObject Object)
        {
            if (Object != this->m_Object)
            {
                this->Close();
                this->m_Object = Object;
            }
            return (this->m_Object);
        }

        operator TObject()
        {
            return this->m_Object;
        }

        bool IsInvalid()
        {
            return (this->m_Object == TObjectDefiner::GetInvalidValue());
        }

        TObject Detach()
        {
            TObject Object = this->m_Object;
            this->m_Object = TObjectDefiner::GetInvalidValue();
            return Object;
        }

        void Close()
        {
            if (!this->IsInvalid())
            {
                TObjectDefiner::Close(this->m_Object);
                this->m_Object = TObjectDefiner::GetInvalidValue();
            }
        }

        TObject operator->() const
        {
            return this->m_Object;
        }
    };

    /**
     * The handle definer for HANDLE object.
     */
#pragma region CHandle

    struct CHandleDefiner
    {
        static inline HANDLE GetInvalidValue()
        {
            return INVALID_HANDLE_VALUE;
        }

        static inline void Close(HANDLE Object)
        {
            ::MileCloseHandle(Object);
        }
    };

    typedef CObject<HANDLE, CHandleDefiner> CHandle;

#pragma endregion

    /**
     * The handle definer for COM object.
     */
#pragma region CComObject

    template<typename TComObject>
    struct CComObjectDefiner
    {
        static inline TComObject GetInvalidValue()
        {
            return nullptr;
        }

        static inline void Close(TComObject Object)
        {
            Object->Release();
        }
    };

    template<typename TComObject>
    class CComObject : public CObject<TComObject, CComObjectDefiner<TComObject>>
    {

    };

#pragma endregion

    /**
     * The handle definer for memory block.
     */
#pragma region CMemory

    template<typename TMemory>
    struct CMemoryDefiner
    {
        static inline TMemory GetInvalidValue()
        {
            return nullptr;
        }

        static inline void Close(TMemory Object)
        {
            free(Object);
        }
    };

    template<typename TMemory>
    class CMemory : public CObject<TMemory, CMemoryDefiner<TMemory>>
    {
    public:
        CMemory(TMemory Object = CMemoryDefiner<TMemory>::GetInvalidValue()) :
            CObject<TMemory, CMemoryDefiner<TMemory>>(Object)
        {

        }

        bool Alloc(size_t Size)
        {
            this->Free();
            this->m_Object = reinterpret_cast<TMemory>(malloc(Size));
            return (nullptr != this->m_Object);
        }

        void Free()
        {
            this->Close();
        }
    };

#pragma endregion

    /**
     * The handle definer for memory block allocated by the M2AllocMemory and
     * M2ReAllocMemory function..
     */
#pragma region CM2Memory

    template<typename TMemory>
    struct CM2MemoryDefiner
    {
        static inline TMemory GetInvalidValue()
        {
            return nullptr;
        }

        static inline void Close(TMemory Object)
        {
            ::MileFreeMemory(Object);
        }
    };

    template<typename TMemoryBlock>
    class CM2Memory :
        public CObject<TMemoryBlock, CM2MemoryDefiner<TMemoryBlock>>
    {

    };

#pragma endregion

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

    /**
     * The handle definer for HKEY object.
     */
#pragma region CHKey

    struct CHKeyDefiner
    {
        static inline HKEY GetInvalidValue()
        {
            return nullptr;
        }

        static inline void Close(HKEY Object)
        {
            ::MileRegCloseKey(Object);
        }
    };

    typedef CObject<HKEY, CHKeyDefiner> CHKey;

#pragma endregion

    /**
     * The handle definer for PSID object.
     */
#pragma region CSID

    struct CSIDDefiner
    {
        static inline PSID GetInvalidValue()
        {
            return nullptr;
        }

        static inline void Close(PSID Object)
        {
            ::MileFreeSid(Object);
        }
    };

    typedef CObject<PSID, CSIDDefiner> CSID;

#pragma endregion

#endif

    /**
     * The implementation of thread.
     */
    class CThread
    {
    private:
        CHandle m_Thread;

    public:
        CThread() = default;

        template<class TFunction>
        CThread(
            _In_ TFunction&& workerFunction,
            _In_ DWORD dwCreationFlags = 0)
        {
            auto ThreadFunctionInternal = [](LPVOID lpThreadParameter) -> DWORD
            {
                auto function = reinterpret_cast<TFunction*>(
                    lpThreadParameter);
                (*function)();
                delete function;
                return 0;
            };

            ::MileCreateThread(
                nullptr,
                0,
                ThreadFunctionInternal,
                reinterpret_cast<LPVOID>(
                    new TFunction(std::move(workerFunction))),
                dwCreationFlags,
                nullptr,
                &this->m_Thread);
        }

        HANDLE Detach()
        {
            return this->m_Thread.Detach();
        }

        DWORD Resume()
        {
            DWORD PreviousSuspendCount = static_cast<DWORD>(-1);
            ::MileResumeThread(this->m_Thread, &PreviousSuspendCount);
            return PreviousSuspendCount;
        }

        DWORD Suspend()
        {
            DWORD PreviousSuspendCount = static_cast<DWORD>(-1);
            ::MileSuspendThread(this->m_Thread, &PreviousSuspendCount);
            return PreviousSuspendCount;
        }

        DWORD Wait(
            _In_ DWORD dwMilliseconds = INFINITE,
            _In_ BOOL bAlertable = FALSE)
        {
            DWORD Result = WAIT_FAILED;
            ::MileWaitForSingleObject(
                this->m_Thread, dwMilliseconds, bAlertable, &Result);
            return Result;
        }

    };

    /**
     * Wraps a critical section.
     */
    class CCriticalSection
    {
    private:
        CRITICAL_SECTION m_CriticalSection;

    public:
        CCriticalSection()
        {
            ::MileInitializeCriticalSection(&this->m_CriticalSection);
        }

        ~CCriticalSection()
        {
            ::MileDeleteCriticalSection(&this->m_CriticalSection);
        }

        _Acquires_lock_(m_CriticalSection) void Lock()
        {
            ::MileEnterCriticalSection(&this->m_CriticalSection);
        }

        _Releases_lock_(m_CriticalSection) void Unlock()
        {
            ::MileLeaveCriticalSection(&this->m_CriticalSection);
        }

        _When_(return, _Acquires_exclusive_lock_(m_CriticalSection))
            bool TryLock()
        {
            return ::MileTryEnterCriticalSection(&this->m_CriticalSection);
        }
    };

    /**
     * Wraps a slim reader/writer (SRW) lock.
     */
    class CSRWLock
    {
    private:
        SRWLOCK m_SRWLock;

    public:
        CSRWLock()
        {
            ::MileInitializeSRWLock(&this->m_SRWLock);
        }

        _Acquires_lock_(m_SRWLock) void ExclusiveLock()
        {
            ::MileAcquireSRWLockExclusive(&this->m_SRWLock);
        }

        _Acquires_lock_(m_SRWLock) bool TryExclusiveLock()
        {
            return ::MileTryAcquireSRWLockExclusive(&this->m_SRWLock);
        }

        _Releases_lock_(m_SRWLock) void ExclusiveUnlock()
        {
            ::MileReleaseSRWLockExclusive(&this->m_SRWLock);
        }

        _Acquires_lock_(m_SRWLock) void SharedLock()
        {
            ::MileAcquireSRWLockShared(&this->m_SRWLock);
        }

        _Acquires_lock_(m_SRWLock) bool TrySharedLock()
        {
            return ::MileTryAcquireSRWLockShared(&this->m_SRWLock);
        }

        _Releases_lock_(m_SRWLock) void SharedUnlock()
        {
            ::MileReleaseSRWLockShared(&this->m_SRWLock);
        }
    };

    /**
     * Provides automatic locking and unlocking of a critical section.
     *
     * @remarks The AutoLock object must go out of scope before the CritSec.
     */
    class AutoCriticalSectionLock
    {
    private:
        CCriticalSection* m_pCriticalSection;

    public:
        _Acquires_lock_(m_pCriticalSection) AutoCriticalSectionLock(
            CCriticalSection& CriticalSection) :
            m_pCriticalSection(&CriticalSection)
        {
            m_pCriticalSection->Lock();
        }

        _Releases_lock_(m_pCriticalSection) ~AutoCriticalSectionLock()
        {
            m_pCriticalSection->Unlock();
        }
    };

    /**
     * Provides automatic trying to lock and unlocking of a critical section.
     *
     * @remarks The AutoLock object must go out of scope before the CritSec.
     */
    class AutoTryCriticalSectionLock
    {
    private:
        CCriticalSection* m_pCriticalSection;
        bool m_IsLocked = false;

    public:
        _Acquires_lock_(m_pCriticalSection) AutoTryCriticalSectionLock(
            CCriticalSection& CriticalSection) :
            m_pCriticalSection(&CriticalSection)
        {
            this->m_IsLocked = m_pCriticalSection->TryLock();
        }

        _Releases_lock_(m_pCriticalSection) ~AutoTryCriticalSectionLock()
        {
            m_pCriticalSection->Unlock();
        }

        bool IsLocked() const
        {
            return this->m_IsLocked;
        }
    };

    /**
     * Provides automatic exclusive locking and unlocking of a slim
     * reader/writer (SRW) lock.
     *
     * @remarks The AutoLock object must go out of scope before the CritSec.
     */
    class AutoSRWExclusiveLock
    {
    private:
        CSRWLock* m_SRWLock;

    public:
        _Acquires_lock_(m_SRWLock) AutoSRWExclusiveLock(
            CSRWLock& SRWLock) :
            m_SRWLock(&SRWLock)
        {
            m_SRWLock->ExclusiveLock();
        }

        _Releases_lock_(m_SRWLock) ~AutoSRWExclusiveLock()
        {
            m_SRWLock->ExclusiveUnlock();
        }
    };

    /**
     * Provides automatic trying to exclusive lock and unlocking of a slim
     * reader/writer (SRW) lock.
     *
     * @remarks The AutoLock object must go out of scope before the CritSec.
     */
    class AutoTrySRWExclusiveLock
    {
    private:
        CSRWLock* m_SRWLock;
        bool m_IsLocked = false;

    public:
        _Acquires_lock_(m_SRWLock) AutoTrySRWExclusiveLock(
            CSRWLock& SRWLock) :
            m_SRWLock(&SRWLock)
        {
            this->m_IsLocked = m_SRWLock->TryExclusiveLock();
        }

        _Releases_lock_(m_SRWLock) ~AutoTrySRWExclusiveLock()
        {
            m_SRWLock->ExclusiveUnlock();
        }

        bool IsLocked() const
        {
            return this->m_IsLocked;
        }
    };

    /**
     * Provides automatic shared locking and unlocking of a slim
     * reader/writer (SRW) lock.
     *
     * @remarks The AutoLock object must go out of scope before the CritSec.
     */
    class AutoSRWSharedLock
    {
    private:
        CSRWLock* m_SRWLock;

    public:
        _Acquires_lock_(m_SRWLock) AutoSRWSharedLock(
            CSRWLock& SRWLock) :
            m_SRWLock(&SRWLock)
        {
            m_SRWLock->SharedLock();
        }

        _Releases_lock_(m_SRWLock) ~AutoSRWSharedLock()
        {
            m_SRWLock->SharedUnlock();
        }
    };

    /**
     * Provides automatic trying to shared lock and unlocking of a slim
     * reader/writer (SRW) lock.
     *
     * @remarks The AutoLock object must go out of scope before the CritSec.
     */
    class AutoTrySRWSharedLock
    {
    private:
        CSRWLock* m_SRWLock;
        bool m_IsLocked = false;

    public:
        _Acquires_lock_(m_SRWLock) AutoTrySRWSharedLock(
            CSRWLock& SRWLock) :
            m_SRWLock(&SRWLock)
        {
            this->m_IsLocked = m_SRWLock->TrySharedLock();
        }

        _Releases_lock_(m_SRWLock) ~AutoTrySRWSharedLock()
        {
            m_SRWLock->SharedUnlock();
        }

        bool IsLocked() const
        {
            return this->m_IsLocked;
        }
    };

    /**
     * A template for implementing an object which the type is a singleton. I
     * do not need to free the memory of the object because the OS releases all
     * the unshared memory associated with the process after the process is
     * terminated.
     */
    template<class ClassType>
    class CSingleton : CDisableObjectCopying, CDisableObjectMoving
    {
    private:
        static CCriticalSection m_SingletonCS;
        static ClassType* volatile m_Instance = nullptr;

    protected:
        CSingleton() = default;
        ~CSingleton() = default;

    public:
        static ClassType* Get()
        {
            M2::AutoCriticalSectionLock Lock(this->m_SingletonCS);

            if (!this->m_Instance)
            {
                this->m_Instance = new ClassType();
            }

            return this->m_Instance;
        }
    };
}

#endif // !_M2_WINDOWS_EXTENDED_HELPERS_

#ifndef _M2_WINDOWS_BASE_EXTENDED_HELPERS_
#define _M2_WINDOWS_BASE_EXTENDED_HELPERS_

/**
 * Retrieves the calling thread's last-error code value. The last-error code is
 * maintained on a per-thread basis. Multiple threads do not overwrite each
 * other's last-error code.
 *
 * @param IsLastFunctionCallSucceeded Set this parameter TRUE if you can be
 *                                    sure that the last call was succeeded.
 *                                    Otherwise, set this parameter FALSE.
 * @param UseLastErrorWhenSucceeded Set this parameter TRUE if you want to use
 *                                  last-error code if the last call was
 *                                  succeeded. Otherwise, set this parameter
 *                                  FALSE.
 * @return The calling thread's last-error code.
 */
DWORD M2GetLastWin32Error(
    _In_ BOOL IsLastFunctionCallSucceeded = FALSE,
    _In_ BOOL UseLastErrorWhenSucceeded = FALSE);

/**
 * Retrieves the calling thread's last-error code value. The last-error code is
 * maintained on a per-thread basis. Multiple threads do not overwrite each
 * other's last-error code.
 *
 * @param KnownFailed Set this parameter TRUE if you can be sure that the last
 *                    call was failed, Otherwise, set this parameter FALSE.
 * @param LastErrorCode A pointer to a variable that returns the calling
 *                      thread's last-error code. This parameter can be NULL.
 * @return The calling thread's last-error code which is converted to an
 *         HRESULT value.
 */
HRESULT M2GetLastHResultError(
    _In_ BOOL IsLastFunctionCallSucceeded = FALSE,
    _In_ BOOL UseLastErrorWhenSucceeded = FALSE);

/**
 * Retrieves the address of an exported function or variable from the specified
 * dynamic-link library (DLL).
 *
 * @param lpProcAddress The address of the exported function or variable.
 * @param hModule A handle to the DLL module that contains the function or
 *                variable. The LoadLibrary, LoadLibraryEx, LoadPackagedLibrary
 *                or GetModuleHandle function returns this handle. This
 *                function does not retrieve addresses from modules that were
 *                loaded using the LOAD_LIBRARY_AS_DATAFILE flag. For more
 *                information, see LoadLibraryEx.
 * @param lpProcName The function or variable name, or the function's ordinal
 *                   value. If this parameter is an ordinal value, it must be
 *                   in the low-order word; the high-order word must be zero.
 * @return HRESULT. If the function succeeds, the return value is S_OK.
 */
template<typename ProcedureType>
inline HRESULT M2GetProcAddress(
    _Out_ ProcedureType& lpProcAddress,
    _In_ HMODULE hModule,
    _In_ LPCSTR lpProcName)
{
    return M2GetProcAddress(
        hModule, lpProcName, reinterpret_cast<FARPROC*>(&lpProcAddress));
}

#endif // !_M2_WINDOWS_BASE_EXTENDED_HELPERS_

#ifndef _M2_WINDOWS_HELPERS_
#define _M2_WINDOWS_HELPERS_

#include <map>
#include <memory>
#include <string>
#include <vector>

#pragma region String

/**
 * Converts from the UTF-8 string to the UTF-16 string.
 *
 * @param UTF8String The UTF-8 string you want to convert.
 * @return A converted UTF-16 string.
 */
std::wstring M2MakeUTF16String(const std::string& UTF8String);

/**
 * Converts from the UTF-16 string to the UTF-8 string.
 *
 * @param UTF16String The UTF-16 string you want to convert.
 * @return A converted UTF-8 string.
 */
std::string M2MakeUTF8String(const std::wstring& UTF16String);

/**
 * Write formatted data to a string.
 *
 * @param Format Format-control string.
 * @param ... Optional arguments to be formatted.
 * @return A formatted string if successful, "N/A" otherwise.
 */
std::wstring M2FormatString(
    _In_z_ _Printf_format_string_ wchar_t const* const Format,
    ...);

/**
 * Parses a command line string and returns an array of the command line
 * arguments, along with a count of such arguments, in a way that is similar to
 * the standard C run-time.
 *
 * @param CommandLine A string that contains the full command line. If this
 *                    parameter is an empty string the function returns an
 *                    array with only one empty string.
 * @return An array of the command line arguments, along with a count of such
 *         arguments.
 */
std::vector<std::wstring> M2SpiltCommandLine(
    const std::wstring& CommandLine);

/**
 * Parses a command line string and get more friendly result.
 *
 * @param CommandLine A string that contains the full command line. If this
 *                    parameter is an empty string the function returns an
 *                    array with only one empty string.
 * @param ApplicationName The application name.
 * @param UnresolvedCommandLine The unresolved command line.
 */
void M2SpiltCommandLineEx(
    const std::wstring& CommandLine,
    std::wstring& ApplicationName,
    std::wstring& UnresolvedCommandLine);

/**
 * Searches a path for a file name.
 *
 * @param Path A pointer to a null-terminated string of maximum length MAX_PATH
 *             that contains the path to search.
 * @return A pointer to the address of the string if successful, or a pointer
 *         to the beginning of the path otherwise.
 */
template<typename CharType>
CharType M2PathFindFileName(CharType Path)
{
    CharType FileName = Path;

    for (size_t i = 0; i < MAX_PATH; ++i)
    {
        if (!(Path && *Path))
            break;

        if (L'\\' == *Path || L'/' == *Path)
            FileName = Path + 1;

        ++Path;
    }

    return FileName;
}

#pragma endregion

#pragma region Module

/**
 * Retrieves the path of the executable file of the current process.
 *
 * @return If the function succeeds, the return value is the path of the
 *         executable file of the current process. If the function fails, the
 *         return value is an empty string.
 */
std::wstring M2GetCurrentProcessModulePath();

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

/**
 * Loads the specified module with the optimization of the mitigation of DLL
 * preloading attacks into the address space of the calling process safely. The
 * specified module may cause other modules to be loaded.
 *
 * @param ModuleHandle If the function succeeds, this parameter's value is a
 *                     handle to the loaded module. You should read the
 *                     documentation about LoadLibraryEx API for further
 *                     information.
 * @param LibraryFileName A string that specifies the file name of the module
 *                        to load. You should read the documentation about
 *                        LoadLibraryEx API for further information.
 * @param Flags The action to be taken when loading the module. You should read
 *              the documentation about LoadLibraryEx API for further
 *              information.
 * @return HRESULT. If the function succeeds, the return value is S_OK.
 */
HRESULT M2LoadLibraryEx(
    _Out_ HMODULE* ModuleHandle,
    _In_ LPCWSTR LibraryFileName,
    _In_ DWORD Flags);

#endif

#pragma endregion

#pragma region Environment

/**
 * Retrieves the path of the system directory.
 *
 * @param SystemFolderPath The string of the path of the system directory.
 * @return HRESULT. If the function succeeds, the return value is S_OK.
 */
HRESULT M2GetSystemDirectory(
    std::wstring& SystemFolderPath);

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

/**
 * Retrieves the path of the shared Windows directory on a multi-user system.
 *
 * @param WindowsFolderPath The string of the path of the shared Windows
 *                          directory on a multi-user system.
 * @return HRESULT. If the function succeeds, the return value is S_OK.
 */
HRESULT M2GetWindowsDirectory(
    std::wstring& WindowsFolderPath);

/**
 * Enables the Per-Monitor DPI Aware for the specified dialog using the
 * internal API from Windows.
 *
 * @return INT. If failed. returns -1.
 * @remarks You need to use this function in Windows 10 Threshold 1 or Windows
 *          10 Threshold 2.
 */
INT M2EnablePerMonitorDialogScaling();

#endif

#pragma endregion

#endif // _M2_WINDOWS_HELPERS_
