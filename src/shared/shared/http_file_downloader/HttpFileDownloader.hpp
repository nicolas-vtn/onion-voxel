#pragma once

#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#include <filesystem>
#include <string>
#include <vector>

namespace onion::voxel
{
	class HttpFileDownloader
	{
	  public:
		static bool DownloadFile(const std::string& url, const std::filesystem::path& destinationPath)
		{
			HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), destinationPath.string().c_str(), 0, nullptr);
			return SUCCEEDED(hr);
		}

		static std::vector<uint8_t> DownloadFile(const std::string& url)
		{
			std::vector<uint8_t> result;

			// ---- Convert URL to wide string ----
			int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
			std::wstring wurl(wlen, 0);
			MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], wlen);

			// ---- Crack URL ----
			URL_COMPONENTS components{};
			components.dwStructSize = sizeof(components);

			wchar_t host[256];
			wchar_t path[2048];

			components.lpszHostName = host;
			components.dwHostNameLength = _countof(host);

			components.lpszUrlPath = path;
			components.dwUrlPathLength = _countof(path);

			WinHttpCrackUrl(wurl.c_str(), 0, 0, &components);

			// ---- Open session ----
			HINTERNET hSession = WinHttpOpen(L"OnionEngine/1.0",
											 WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
											 WINHTTP_NO_PROXY_NAME,
											 WINHTTP_NO_PROXY_BYPASS,
											 0);

			if (!hSession)
				return {};

			// ---- Connect ----
			HINTERNET hConnect =
				WinHttpConnect(hSession,
							   std::wstring(components.lpszHostName, components.dwHostNameLength).c_str(),
							   components.nPort,
							   0);

			if (!hConnect)
			{
				WinHttpCloseHandle(hSession);
				return {};
			}

			// ---- Open request ----
			bool isHttps = components.nScheme == INTERNET_SCHEME_HTTPS;

			HINTERNET hRequest =
				WinHttpOpenRequest(hConnect,
								   L"GET",
								   std::wstring(components.lpszUrlPath, components.dwUrlPathLength).c_str(),
								   nullptr,
								   WINHTTP_NO_REFERER,
								   WINHTTP_DEFAULT_ACCEPT_TYPES,
								   isHttps ? WINHTTP_FLAG_SECURE : 0);

			if (!hRequest)
			{
				WinHttpCloseHandle(hConnect);
				WinHttpCloseHandle(hSession);
				return {};
			}

			// ---- Send request ----
			if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
				!WinHttpReceiveResponse(hRequest, nullptr))
			{
				WinHttpCloseHandle(hRequest);
				WinHttpCloseHandle(hConnect);
				WinHttpCloseHandle(hSession);
				return {};
			}

			// ---- Read data ----
			DWORD size = 0;

			do
			{
				DWORD downloaded = 0;

				if (!WinHttpQueryDataAvailable(hRequest, &size) || size == 0)
					break;

				std::vector<uint8_t> buffer(size);

				if (!WinHttpReadData(hRequest, buffer.data(), size, &downloaded))
					break;

				result.insert(result.end(), buffer.begin(), buffer.begin() + downloaded);

			} while (size > 0);

			// ---- Cleanup ----
			WinHttpCloseHandle(hRequest);
			WinHttpCloseHandle(hConnect);
			WinHttpCloseHandle(hSession);

			return result;
		}
	};
} // namespace onion::voxel
