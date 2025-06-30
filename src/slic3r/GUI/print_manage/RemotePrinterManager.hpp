#ifndef REMOTE_PRINTER_MANAGER_H
#define REMOTE_PRINTER_MANAGER_H

#include <functional>
#include <mutex>
#include <condition_variable>
#include "Device/LanPrinterInterface.hpp"
#include "Device/OctoPrintInterface.hpp"
#include "Device/KlipperInterface.hpp"
#include "Device/Klipper4408Interface.hpp"
#include <curl/curl.h>
#include "Device/KlipperCXInterface.hpp"
#include <condition_variable>
namespace RemotePrint {
enum class RemotePrinerType {
    REMOTE_PRINTER_TYPE_NONE = -1,
    REMOTE_PRINTER_TYPE_LAN,        // lan
    REMOTE_PRINTER_TYPE_OCTOPRINT,  // octoprint
    REMOTE_PRINTER_TYPE_KLIPPER,    // klipper
    REMOTE_PRINTER_TYPE_KLIPPER4408, // klipper4408
    REMOTE_PRINTER_TYPE_CX,
};

size_t read_callback(void* ptr, size_t size, size_t nmemb, void* stream);
int    progress_callback(void* ptr, curl_off_t totalToDownload, curl_off_t nowDownloaded, curl_off_t totalToUpload, curl_off_t nowUploaded);
std::string url_encode(const std::string &value);

class RemotePrinterManager
{
public:
    static RemotePrinterManager& getInstance()
    {
        static std::unique_ptr<RemotePrinterManager> instance;
        static std::once_flag flag;
        std::call_once(flag, []() {
            instance.reset(new RemotePrinterManager());
        });
        return *instance;
    }

    static void destroyInstance() {
        instance.reset();
    }

    ~RemotePrinterManager();

    void uploadThread();
    void pushUploadTasks(const std::string&               ipAddress,
                        const std::string&                      fileName,
                        const std::string&                      filePath,
                        std::function<void(std::string, float,double)> progressCallback,
                        std::function<void(std::string, int)>   uploadStatusCallback = nullptr, 
                        std::function<void(std::string, std::string)>   onCompleteCallback = nullptr);
    void pushUploadMultTasks(const std::string&               ipAddress,
                        const std::string&                      fileName,
                        const std::string&                      filePath,
                        std::function<void(std::string, float,double)> progressCallback,
                        std::function<void(std::string, int)>   uploadStatusCallback = nullptr, 
                        std::function<void(std::string, std::string)>   onCompleteCallback = nullptr);
    void cancelUpload(const std::string& ipAddress);
    void uploadFileByLan(const std::string& ipAddress,
                         const std::string& fileName,
                         const std::string& filePath,
                         std::function<void(float,double)> progressCallback = nullptr,
                         std::function<void(std::string, int)>   uploadStatusCallback = nullptr,
                         std::function<void(std::string, std::string)>   onCompleteCallback = nullptr);

    void setOldPrinterMap(std::string& ipAddress);
    void setKlipperPrinterMap(const std::string& ipAddress,int port);
    int getKlipperPrinterMap(const std::string& ipAddress);
    void addDownloadTask(const std::function<void()>& task);
private:
    RemotePrinterManager();

    RemotePrinterManager(const RemotePrinterManager&)            = delete;
    RemotePrinterManager& operator=(const RemotePrinterManager&) = delete;

    void pushFile(const std::string&                      ipAddress,
                  const std::string&                      fileName,
                  const std::string&                      filePath,
                  std::function<void(std::string, float,double)> progressCallback,
                  std::function<void(std::string, int)>   uploadStatusCallback = nullptr,
                  std::function<void(std::string, std::string)> onCompleteCallback = nullptr);
    void workerThread();
    LanPrinterInterface*  m_pLanPrinterInterface;
    OctoPrintInterface*   m_pOctoPrinterInterface;
    KlipperInterface*     m_pKlipperInterface;
    Klipper4408Interface* m_pKlipper4408Interface;
    KlipperCXInterface* m_pKlipperCXInterface;

    std::mutex m_mtxUpload;
    std::condition_variable m_cvUpload;
    std::deque<std::tuple<std::string, std::string, std::string, std::function<void(std::string, float,double)>, std::function<void(std::string, int)>, std::function<void(std::string, std::string)> >> m_uploadTasks;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> m_multUploadThreads;
    bool m_bExit = false;

    static std::unique_ptr<RemotePrinterManager> instance;
    static std::once_flag flag;

    RemotePrinerType determinePrinterType(const std::string& ipAddress);

    std::vector<std::string> oldPrinters;
    std::map<std::string,int> mapKlipperPort;
    std::condition_variable condition;
    std::atomic<bool> stop_flag;
    std::mutex queue_mutex;
};
} // namespace RemotePrintManage

#endif // REMOTE_PRINTER_MANAGER_H