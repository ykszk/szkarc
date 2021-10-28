#include "szkarc.h"
#include <thread>
#include <codecvt>
#include <mz.h>
#include <mz_os.h>
#include <mz_strm.h>
#include <mz_strm_os.h>

#ifdef _WIN32
#include <windows.h>

int get_physical_core_counts() {
  typedef BOOL(WINAPI* LPFN_GLPI)(
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION,
    PDWORD);
  BOOL done = FALSE;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;
  DWORD returnLength = 0;

  LPFN_GLPI glpi = (LPFN_GLPI)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetLogicalProcessorInformation");
  if (NULL == glpi) {
    throw std::runtime_error("GetLogicalProcessorInformation is not supported.");
  }
  while (!done) {
    DWORD rc = glpi(buffer, &returnLength);
    if (FALSE == rc) {
      if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        if (buffer) {
          free(buffer);
        }
        buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(
          returnLength);

        if (NULL == buffer) {
          throw std::runtime_error("Error: Allocation failure");
        }
      }
      else {
        throw std::runtime_error("Error");
      }
    }
    else {
      done = TRUE;
    }
  }

  DWORD byteOffset = 0;
  int processorCoreCount = 0;
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
  if (NULL == ptr) {
    throw std::runtime_error("Error: Allocation failure");
  }
  while (byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength) {
    if (ptr->Relationship == RelationProcessorCore) {
      processorCoreCount++;
    }
    byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    ptr++;
  }
  return processorCoreCount;
}
int32_t stream_os_open(void* stream, const char* path, int32_t mode) {
  typedef struct mz_stream_win32_s {
    mz_stream       stream;
    HANDLE          handle;
    int32_t         error;
  } mz_stream_win32;

  mz_stream_win32* win32 = (mz_stream_win32*)stream;
  uint32_t desired_access = 0;
  uint32_t creation_disposition = 0;
  uint32_t share_mode = FILE_SHARE_READ;
  uint32_t flags_attribs = FILE_ATTRIBUTE_NORMAL;
  wchar_t* path_wide = NULL;


  if (path == NULL)
    return MZ_PARAM_ERROR;

  /* Some use cases require write sharing as well */
  share_mode |= FILE_SHARE_WRITE;

  if ((mode & MZ_OPEN_MODE_READWRITE) == MZ_OPEN_MODE_READ) {
    desired_access = GENERIC_READ;
    creation_disposition = OPEN_EXISTING;
  }
  else if (mode & MZ_OPEN_MODE_APPEND) {
    desired_access = GENERIC_WRITE | GENERIC_READ;
    creation_disposition = OPEN_EXISTING;
  }
  else if (mode & MZ_OPEN_MODE_CREATE) {
    desired_access = GENERIC_WRITE | GENERIC_READ;
    creation_disposition = CREATE_ALWAYS;
  }
  else {
    return MZ_PARAM_ERROR;
  }

  path_wide = mz_os_unicode_string_create(path, MZ_ENCODING_CODEPAGE_932);
  if (path_wide == NULL)
    return MZ_PARAM_ERROR;

  win32->handle = CreateFileW(path_wide, desired_access, share_mode, NULL,
    creation_disposition, flags_attribs, NULL);

  mz_os_unicode_string_delete(&path_wide);

  if (mz_stream_os_is_open(stream) != MZ_OK) {
    win32->error = GetLastError();
    return MZ_OPEN_ERROR;
  }

  if (mode & MZ_OPEN_MODE_APPEND)
    return mz_stream_os_seek(stream, 0, MZ_SEEK_END);

  return MZ_OK;
}
std::string wstr2utf8(std::wstring const& src)
{
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
  return converter.to_bytes(src);
}
#else
int get_physical_core_counts() {
  return std::thread::hardware_concurrency();
}
int32_t stream_os_open(void* stream, const char* path, int32_t mode) {
  return mz_stream_os_open(stream, path, mode);
}
#endif

