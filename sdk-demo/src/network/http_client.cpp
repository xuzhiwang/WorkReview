#include "sdk/network/http_client.h"
#include "sdk/sdk_c_api.h"

#include <curl/curl.h>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <atomic>
#include <future>
#include <sstream>

namespace sdk {

// libcurl初始化管理
class CurlGlobalInit {
public:
    CurlGlobalInit() {
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~CurlGlobalInit() {
        curl_global_cleanup();
    }
    
    static CurlGlobalInit& getInstance() {
        static CurlGlobalInit instance;
        return instance;
    }
};

// HTTP客户端实现
class HttpClient::Impl {
public:
    explicit Impl(const HttpClientConfig& config) : config_(config) {
        // 确保libcurl已初始化
        CurlGlobalInit::getInstance();
        
        stats_.total_requests = 0;
        stats_.successful_requests = 0;
        stats_.failed_requests = 0;
        stats_.total_time = std::chrono::milliseconds(0);
        stats_.average_time = std::chrono::milliseconds(0);
    }
    
    ~Impl() = default;
    
    HttpResponse get(const std::string& url) {
        HttpRequest request(url);
        request.setMethod(HttpMethod::GET);
        return executeRequest(request);
    }
    
    HttpResponse post(const std::string& url, const std::string& body) {
        HttpRequest request(url);
        request.setMethod(HttpMethod::POST);
        request.setBody(body);
        return executeRequest(request);
    }
    
    HttpResponse put(const std::string& url, const std::string& body) {
        HttpRequest request(url);
        request.setMethod(HttpMethod::PUT);
        request.setBody(body);
        return executeRequest(request);
    }
    
    HttpResponse del(const std::string& url) {
        HttpRequest request(url);
        request.setMethod(HttpMethod::DELETE);
        return executeRequest(request);
    }
    
    HttpResponse request(const HttpRequest& request) {
        return executeRequest(request);
    }
    
    std::future<HttpResponse> getAsync(const std::string& url) {
        return std::async(std::launch::async, [this, url]() {
            return get(url);
        });
    }
    
    std::future<HttpResponse> postAsync(const std::string& url, const std::string& body) {
        return std::async(std::launch::async, [this, url, body]() {
            return post(url, body);
        });
    }
    
    std::future<HttpResponse> putAsync(const std::string& url, const std::string& body) {
        return std::async(std::launch::async, [this, url, body]() {
            return put(url, body);
        });
    }
    
    std::future<HttpResponse> deleteAsync(const std::string& url) {
        return std::async(std::launch::async, [this, url]() {
            return del(url);
        });
    }
    
    std::future<HttpResponse> requestAsync(const HttpRequest& request) {
        return std::async(std::launch::async, [this, request]() {
            return executeRequest(request);
        });
    }
    
    void requestAsync(const HttpRequest& request, ResponseCallback callback) {
        std::thread([this, request, callback]() {
            auto response = executeRequest(request);
            if (callback) {
                callback(response);
            }
        }).detach();
    }
    
    const HttpClientConfig& getConfig() const {
        return config_;
    }
    
    void setConfig(const HttpClientConfig& config) {
        std::lock_guard<std::mutex> lock(config_mutex_);
        config_ = config;
    }
    
    void setGlobalHeader(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(headers_mutex_);
        global_headers_[key] = value;
    }
    
    void removeGlobalHeader(const std::string& key) {
        std::lock_guard<std::mutex> lock(headers_mutex_);
        global_headers_.erase(key);
    }
    
    void clearGlobalHeaders() {
        std::lock_guard<std::mutex> lock(headers_mutex_);
        global_headers_.clear();
    }
    
    Stats getStats() const {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        return stats_;
    }

private:
    HttpResponse executeRequest(const HttpRequest& request) {
        auto start_time = std::chrono::steady_clock::now();
        
        HttpResponse response;
        CURL* curl = curl_easy_init();
        
        if (!curl) {
            response.error_ = "Failed to initialize CURL";
            updateStats(false, std::chrono::milliseconds(0));
            return response;
        }
        
        // RAII管理CURL句柄
        std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl_guard(curl, curl_easy_cleanup);
        
        try {
            setupCurlOptions(curl, request, response);
            
            CURLcode res = curl_easy_perform(curl);
            
            if (res != CURLE_OK) {
                response.error_ = curl_easy_strerror(res);
                response.status_code_ = 0;
            } else {
                long response_code;
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
                response.status_code_ = static_cast<int>(response_code);
            }
            
        } catch (const std::exception& e) {
            response.error_ = e.what();
            response.status_code_ = 0;
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        response.response_time_ = duration;
        
        updateStats(response.isSuccess(), duration);
        
        return response;
    }
    
    void setupCurlOptions(CURL* curl, const HttpRequest& request, HttpResponse& response) {
        // 设置URL
        curl_easy_setopt(curl, CURLOPT_URL, request.getUrl().c_str());
        
        // 设置HTTP方法
        switch (request.getMethod()) {
            case HttpMethod::GET:
                curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
                break;
            case HttpMethod::POST:
                curl_easy_setopt(curl, CURLOPT_POST, 1L);
                break;
            case HttpMethod::PUT:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
                break;
            case HttpMethod::DELETE:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
                break;
            case HttpMethod::PATCH:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
                break;
            case HttpMethod::HEAD:
                curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
                break;
            case HttpMethod::OPTIONS:
                curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
                break;
        }
        
        // 设置请求体
        if (!request.getBody().empty()) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request.getBody().c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, request.getBody().length());
        }
        
        // 设置超时
        auto timeout_ms = request.getTimeout().count();
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, config_.connection_timeout.count());
        
        // 设置用户代理
        curl_easy_setopt(curl, CURLOPT_USERAGENT, config_.user_agent.c_str());
        
        // 设置头部
        struct curl_slist* headers = nullptr;
        
        // 添加全局头部
        {
            std::lock_guard<std::mutex> lock(headers_mutex_);
            for (const auto& header : global_headers_) {
                std::string header_str = header.first + ": " + header.second;
                headers = curl_slist_append(headers, header_str.c_str());
            }
        }
        
        // 添加请求特定头部
        for (const auto& header : request.getHeaders()) {
            std::string header_str = header.first + ": " + header.second;
            headers = curl_slist_append(headers, header_str.c_str());
        }
        
        if (headers) {
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        }
        
        // 设置响应回调
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response.body_);
        
        // 设置头部回调
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, headerCallback);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response.headers_);
        
        // SSL设置
        if (config_.verify_ssl) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        } else {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
        
        // 跟随重定向
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, config_.max_redirects);
        
        // 压缩支持
        if (config_.enable_compression) {
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        }
        
        // 清理头部列表
        if (headers) {
            curl_slist_free_all(headers);
        }
    }
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
        size_t total_size = size * nmemb;
        userp->append(static_cast<char*>(contents), total_size);
        return total_size;
    }
    
    static size_t headerCallback(void* contents, size_t size, size_t nmemb, HttpHeaders* userp) {
        size_t total_size = size * nmemb;
        std::string header(static_cast<char*>(contents), total_size);
        
        // 解析头部
        size_t colon_pos = header.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = header.substr(0, colon_pos);
            std::string value = header.substr(colon_pos + 1);
            
            // 去除空白字符
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);
            
            (*userp)[key] = value;
        }
        
        return total_size;
    }
    
    void updateStats(bool success, std::chrono::milliseconds duration) {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        
        stats_.total_requests++;
        if (success) {
            stats_.successful_requests++;
        } else {
            stats_.failed_requests++;
        }
        
        stats_.total_time += duration;
        if (stats_.total_requests > 0) {
            stats_.average_time = stats_.total_time / stats_.total_requests;
        }
    }

private:
    HttpClientConfig config_;
    std::mutex config_mutex_;
    
    HttpHeaders global_headers_;
    std::mutex headers_mutex_;
    
    mutable std::mutex stats_mutex_;
    Stats stats_;
};

// HttpRequest实现
HttpRequest::HttpRequest() = default;

HttpRequest::HttpRequest(const std::string& url) : url_(url) {}

HttpRequest& HttpRequest::setUrl(const std::string& url) {
    url_ = url;
    return *this;
}

HttpRequest& HttpRequest::setMethod(HttpMethod method) {
    method_ = method;
    return *this;
}

HttpRequest& HttpRequest::setHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
    return *this;
}

HttpRequest& HttpRequest::setHeaders(const HttpHeaders& headers) {
    headers_ = headers;
    return *this;
}

HttpRequest& HttpRequest::setBody(const std::string& body) {
    body_ = body;
    return *this;
}

HttpRequest& HttpRequest::setBody(const std::vector<uint8_t>& body) {
    body_.assign(body.begin(), body.end());
    return *this;
}

HttpRequest& HttpRequest::setTimeout(std::chrono::milliseconds timeout) {
    timeout_ = timeout;
    return *this;
}

HttpRequest& HttpRequest::setUserAgent(const std::string& user_agent) {
    user_agent_ = user_agent;
    return *this;
}

HttpRequest& HttpRequest::setBasicAuth(const std::string& username, const std::string& password) {
    // 实现Basic认证
    std::string auth = username + ":" + password;
    // 这里需要base64编码，简化实现
    setHeader("Authorization", "Basic " + auth);
    return *this;
}

HttpRequest& HttpRequest::setBearerToken(const std::string& token) {
    setHeader("Authorization", "Bearer " + token);
    return *this;
}

HttpRequest& HttpRequest::setProxy(const std::string& proxy_url) {
    proxy_url_ = proxy_url;
    return *this;
}

HttpRequest& HttpRequest::setVerifySSL(bool verify) {
    verify_ssl_ = verify;
    return *this;
}

// HttpResponse实现
HttpResponse::HttpResponse() = default;

std::string HttpResponse::getHeader(const std::string& key) const {
    auto it = headers_.find(key);
    return it != headers_.end() ? it->second : "";
}

// HttpClient实现
HttpClient::HttpClient(const HttpClientConfig& config) 
    : pImpl_(std::make_unique<Impl>(config)) {}

HttpClient::~HttpClient() = default;

HttpClient::HttpClient(HttpClient&&) noexcept = default;
HttpClient& HttpClient::operator=(HttpClient&&) noexcept = default;

HttpResponse HttpClient::get(const std::string& url) {
    return pImpl_->get(url);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body) {
    return pImpl_->post(url, body);
}

HttpResponse HttpClient::put(const std::string& url, const std::string& body) {
    return pImpl_->put(url, body);
}

HttpResponse HttpClient::del(const std::string& url) {
    return pImpl_->del(url);
}

HttpResponse HttpClient::request(const HttpRequest& request) {
    return pImpl_->request(request);
}

std::future<HttpResponse> HttpClient::getAsync(const std::string& url) {
    return pImpl_->getAsync(url);
}

std::future<HttpResponse> HttpClient::postAsync(const std::string& url, const std::string& body) {
    return pImpl_->postAsync(url, body);
}

std::future<HttpResponse> HttpClient::putAsync(const std::string& url, const std::string& body) {
    return pImpl_->putAsync(url, body);
}

std::future<HttpResponse> HttpClient::deleteAsync(const std::string& url) {
    return pImpl_->deleteAsync(url);
}

std::future<HttpResponse> HttpClient::requestAsync(const HttpRequest& request) {
    return pImpl_->requestAsync(request);
}

void HttpClient::requestAsync(const HttpRequest& request, ResponseCallback callback) {
    pImpl_->requestAsync(request, callback);
}

void HttpClient::setConfig(const HttpClientConfig& config) {
    pImpl_->setConfig(config);
}

const HttpClientConfig& HttpClient::getConfig() const {
    return pImpl_->getConfig();
}

void HttpClient::setGlobalHeader(const std::string& key, const std::string& value) {
    pImpl_->setGlobalHeader(key, value);
}

void HttpClient::removeGlobalHeader(const std::string& key) {
    pImpl_->removeGlobalHeader(key);
}

void HttpClient::clearGlobalHeaders() {
    pImpl_->clearGlobalHeaders();
}

HttpClient::Stats HttpClient::getStats() const {
    return pImpl_->getStats();
}

} // namespace sdk

// =============================================================================
// C API实现
// =============================================================================

// 全局请求管理
static std::unordered_map<sdk_http_request_id_t, std::future<sdk::HttpResponse>> g_async_requests;
static std::mutex g_async_requests_mutex;
static std::atomic<sdk_http_request_id_t> g_next_request_id{1};

// 转换函数
static sdk::HttpMethod convertMethod(sdk_http_method_t method) {
    switch (method) {
        case SDK_HTTP_METHOD_GET:
            return sdk::HttpMethod::GET;
        case SDK_HTTP_METHOD_POST:
            return sdk::HttpMethod::POST;
        case SDK_HTTP_METHOD_PUT:
            return sdk::HttpMethod::PUT;
        case SDK_HTTP_METHOD_DELETE:
            return sdk::HttpMethod::DELETE;
        case SDK_HTTP_METHOD_PATCH:
            return sdk::HttpMethod::PATCH;
        case SDK_HTTP_METHOD_HEAD:
            return sdk::HttpMethod::HEAD;
        case SDK_HTTP_METHOD_OPTIONS:
            return sdk::HttpMethod::OPTIONS;
        default:
            return sdk::HttpMethod::GET;
    }
}

static void convertResponse(const sdk::HttpResponse& cpp_response, sdk_http_response_t* c_response) {
    if (!c_response) return;
    
    c_response->status_code = cpp_response.getStatusCode();
    c_response->response_time_ms = static_cast<uint32_t>(cpp_response.getResponseTime().count());
    
    // 复制错误信息
    std::strncpy(c_response->error_message, cpp_response.getError().c_str(), 
                sizeof(c_response->error_message) - 1);
    c_response->error_message[sizeof(c_response->error_message) - 1] = '\0';
    
    // 复制响应体
    const auto& body = cpp_response.getBody();
    c_response->body.size = static_cast<uint32_t>(body.length());
    c_response->body.capacity = c_response->body.size + 1;
    c_response->body.data = static_cast<char*>(malloc(c_response->body.capacity));
    if (c_response->body.data) {
        std::memcpy(c_response->body.data, body.c_str(), body.length());
        c_response->body.data[body.length()] = '\0';
    }
    
    // 复制头部
    const auto& headers = cpp_response.getHeaders();
    c_response->headers.count = static_cast<uint32_t>(headers.size());
    c_response->headers.capacity = c_response->headers.count;
    
    if (c_response->headers.count > 0) {
        c_response->headers.headers = static_cast<sdk_http_header_t*>(
            malloc(sizeof(sdk_http_header_t) * c_response->headers.count));
        
        if (c_response->headers.headers) {
            size_t i = 0;
            for (const auto& header : headers) {
                std::strncpy(c_response->headers.headers[i].key, header.first.c_str(), 
                            sizeof(c_response->headers.headers[i].key) - 1);
                c_response->headers.headers[i].key[sizeof(c_response->headers.headers[i].key) - 1] = '\0';
                
                std::strncpy(c_response->headers.headers[i].value, header.second.c_str(), 
                            sizeof(c_response->headers.headers[i].value) - 1);
                c_response->headers.headers[i].value[sizeof(c_response->headers.headers[i].value) - 1] = '\0';
                
                i++;
            }
        }
    }
}

extern "C" {

bool sdk_http_get(const char* url, const sdk_http_headers_t* headers, sdk_http_response_t* response) {
    if (!url || !response) {
        return false;
    }
    
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return false;
        }
        
        auto http_client = sdk_instance.getHttpClient();
        if (!http_client) {
            return false;
        }
        
        auto cpp_response = http_client->get(url);
        convertResponse(cpp_response, response);
        
        return cpp_response.isSuccess();
    } catch (...) {
        return false;
    }
}

bool sdk_http_post(const char* url, const sdk_http_headers_t* headers, 
                  const void* body, uint32_t body_size, sdk_http_response_t* response) {
    if (!url || !response) {
        return false;
    }
    
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return false;
        }
        
        auto http_client = sdk_instance.getHttpClient();
        if (!http_client) {
            return false;
        }
        
        std::string body_str;
        if (body && body_size > 0) {
            body_str.assign(static_cast<const char*>(body), body_size);
        }
        
        auto cpp_response = http_client->post(url, body_str);
        convertResponse(cpp_response, response);
        
        return cpp_response.isSuccess();
    } catch (...) {
        return false;
    }
}

sdk_http_request_id_t sdk_http_request_async(
    sdk_http_method_t method, const char* url,
    const sdk_http_headers_t* headers, const void* body, uint32_t body_size,
    uint32_t timeout_ms, sdk_http_response_callback_t callback, void* user_data
) {
    if (!url) {
        return 0;
    }
    
    try {
        auto& sdk_instance = sdk::SDK::getInstance();
        if (!sdk_instance.isInitialized()) {
            return 0;
        }
        
        auto http_client = sdk_instance.getHttpClient();
        if (!http_client) {
            return 0;
        }
        
        sdk::HttpRequest request(url);
        request.setMethod(convertMethod(method));
        request.setTimeout(std::chrono::milliseconds(timeout_ms));
        
        if (body && body_size > 0) {
            std::string body_str(static_cast<const char*>(body), body_size);
            request.setBody(body_str);
        }
        
        sdk_http_request_id_t request_id = g_next_request_id.fetch_add(1);
        
        auto future = http_client->requestAsync(request);
        
        {
            std::lock_guard<std::mutex> lock(g_async_requests_mutex);
            g_async_requests[request_id] = std::move(future);
        }
        
        // 如果有回调，启动异步处理
        if (callback) {
            std::thread([request_id, callback, user_data]() {
                try {
                    std::future<sdk::HttpResponse> future;
                    {
                        std::lock_guard<std::mutex> lock(g_async_requests_mutex);
                        auto it = g_async_requests.find(request_id);
                        if (it != g_async_requests.end()) {
                            future = std::move(it->second);
                            g_async_requests.erase(it);
                        }
                    }
                    
                    if (future.valid()) {
                        auto cpp_response = future.get();
                        
                        sdk_http_response_t c_response = {};
                        convertResponse(cpp_response, &c_response);
                        
                        callback(request_id, &c_response, user_data);
                        
                        // 清理响应
                        sdk_http_response_free(&c_response);
                    }
                } catch (...) {
                    // 错误处理
                }
            }).detach();
        }
        
        return request_id;
    } catch (...) {
        return 0;
    }
}

void sdk_http_response_free(sdk_http_response_t* response) {
    if (!response) return;
    
    if (response->body.data) {
        free(response->body.data);
        response->body.data = nullptr;
        response->body.size = 0;
        response->body.capacity = 0;
    }
    
    if (response->headers.headers) {
        free(response->headers.headers);
        response->headers.headers = nullptr;
        response->headers.count = 0;
        response->headers.capacity = 0;
    }
}

void sdk_http_headers_free(sdk_http_headers_t* headers) {
    if (!headers) return;
    
    if (headers->headers) {
        free(headers->headers);
        headers->headers = nullptr;
        headers->count = 0;
        headers->capacity = 0;
    }
}

} // extern "C"
