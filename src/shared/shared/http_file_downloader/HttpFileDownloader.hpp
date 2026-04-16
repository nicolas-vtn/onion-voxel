#pragma once

#include <filesystem>
#include <string>
#include <vector>

#ifdef _WIN32

    #define WIN32_LEAN_AND_MEAN
    #define NOMINMAX
    #include <Windows.h>
    #include <urlmon.h>
    #pragma comment(lib, "urlmon.lib")

    #include <winhttp.h>
    #pragma comment(lib, "winhttp.lib")

#else

    #include <curl/curl.h>

#endif

namespace onion::voxel
{
    class HttpFileDownloader
    {
    public:
        // --------------------------------------------------------
        // Download to file
        // --------------------------------------------------------
        static bool DownloadFile(const std::string& url, const std::filesystem::path& destinationPath)
        {
#ifdef _WIN32
            HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), destinationPath.string().c_str(), 0, nullptr);
            return SUCCEEDED(hr);

#else
            CURL* curl = curl_easy_init();
            if (!curl)
                return false;

            FILE* file = fopen(destinationPath.string().c_str(), "wb");
            if (!file)
            {
                curl_easy_cleanup(curl);
                return false;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

            CURLcode res = curl_easy_perform(curl);

            fclose(file);
            curl_easy_cleanup(curl);

            return res == CURLE_OK;
#endif
        }

        // --------------------------------------------------------
        // Download to memory
        // --------------------------------------------------------
        static std::vector<uint8_t> DownloadFile(const std::string& url)
        {
#ifdef _WIN32
            return DownloadFile_WinHTTP(url);
#else
            return DownloadFile_Curl(url);
#endif
        }

    private:

#ifdef _WIN32

        static std::vector<uint8_t> DownloadFile_WinHTTP(const std::string& url)
        {
            std::vector<uint8_t> result;

            int wlen = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
            std::wstring wurl(wlen, 0);
            MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, &wurl[0], wlen);

            URL_COMPONENTS components{};
            components.dwStructSize = sizeof(components);

            wchar_t host[256];
            wchar_t path[2048];

            components.lpszHostName = host;
            components.dwHostNameLength = _countof(host);

            components.lpszUrlPath = path;
            components.dwUrlPathLength = _countof(path);

            WinHttpCrackUrl(wurl.c_str(), 0, 0, &components);

            HINTERNET hSession = WinHttpOpen(L"OnionEngine/1.0",
                                             WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                             WINHTTP_NO_PROXY_NAME,
                                             WINHTTP_NO_PROXY_BYPASS,
                                             0);

            if (!hSession)
                return {};

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

            if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(hRequest, nullptr))
            {
                WinHttpCloseHandle(hRequest);
                WinHttpCloseHandle(hConnect);
                WinHttpCloseHandle(hSession);
                return {};
            }

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

            WinHttpCloseHandle(hRequest);
            WinHttpCloseHandle(hConnect);
            WinHttpCloseHandle(hSession);

            return result;
        }

#else

        // libcurl write callback
        static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
        {
            size_t totalSize = size * nmemb;
            auto* vec = static_cast<std::vector<uint8_t>*>(userp);

            uint8_t* data = static_cast<uint8_t*>(contents);
            vec->insert(vec->end(), data, data + totalSize);

            return totalSize;
        }

        static std::vector<uint8_t> DownloadFile_Curl(const std::string& url)
        {
            std::vector<uint8_t> result;

            CURL* curl = curl_easy_init();
            if (!curl)
                return {};

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

            CURLcode res = curl_easy_perform(curl);
            curl_easy_cleanup(curl);

            if (res != CURLE_OK)
                return {};

            return result;
        }

#endif
    };
}