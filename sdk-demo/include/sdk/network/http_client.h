#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <future>
#include <chrono>
#include <functional>

namespace sdk {
    
    // HTTP方法
    enum class HttpMethod {
        GET,
        POST,
        PUT,
        DELETE,
        PATCH,
        HEAD,
        OPTIONS
    };
    
    // HTTP状态码
    enum class HttpStatusCode {
        OK = 200,
        CREATED = 201,
        NO_CONTENT = 204,
        BAD_REQUEST = 400,
        UNAUTHORIZED = 401,
        FORBIDDEN = 403,
        NOT_FOUND = 404,
        INTERNAL_SERVER_ERROR = 500,
        BAD_GATEWAY = 502,
        SERVICE_UNAVAILABLE = 503
    };
    
    // HTTP头部类型
    using HttpHeaders = std::unordered_map<std::string, std::string>;
    
    // HTTP请求类
    class HttpRequest {
    public:
        HttpRequest() = default;
        explicit HttpRequest(const std::string& url);
        
        // 设置URL
        HttpRequest& setUrl(const std::string& url);
        
        // 设置方法
        HttpRequest& setMethod(HttpMethod method);
        
        // 设置头部
        HttpRequest& setHeader(const std::string& key, const std::string& value);
        HttpRequest& setHeaders(const HttpHeaders& headers);
        
        // 设置请求体
        HttpRequest& setBody(const std::string& body);
        HttpRequest& setBody(const std::vector<uint8_t>& body);
        
        // 设置超时
        HttpRequest& setTimeout(std::chrono::milliseconds timeout);
        
        // 设置用户代理
        HttpRequest& setUserAgent(const std::string& user_agent);
        
        // 设置认证
        HttpRequest& setBasicAuth(const std::string& username, const std::string& password);
        HttpRequest& setBearerToken(const std::string& token);
        
        // 设置代理
        HttpRequest& setProxy(const std::string& proxy_url);
        
        // 设置SSL验证
        HttpRequest& setVerifySSL(bool verify);
        
        // 获取器
        const std::string& getUrl() const { return url_; }
        HttpMethod getMethod() const { return method_; }
        const HttpHeaders& getHeaders() const { return headers_; }
        const std::string& getBody() const { return body_; }
        std::chrono::milliseconds getTimeout() const { return timeout_; }
        
    private:
        std::string url_;
        HttpMethod method_ = HttpMethod::GET;
        HttpHeaders headers_;
        std::string body_;
        std::chrono::milliseconds timeout_{30000};  // 30秒默认超时
        std::string user_agent_;
        std::string proxy_url_;
        bool verify_ssl_ = true;
        
        friend class HttpClient;
    };
    
    // HTTP响应类
    class HttpResponse {
    public:
        HttpResponse() = default;
        
        // 获取状态码
        int getStatusCode() const { return status_code_; }
        
        // 获取头部
        const HttpHeaders& getHeaders() const { return headers_; }
        std::string getHeader(const std::string& key) const;
        
        // 获取响应体
        const std::string& getBody() const { return body_; }
        const std::vector<uint8_t>& getBinaryBody() const { return binary_body_; }
        
        // 获取响应时间
        std::chrono::milliseconds getResponseTime() const { return response_time_; }
        
        // 获取错误信息
        const std::string& getError() const { return error_; }
        
        // 检查是否成功
        bool isSuccess() const { return status_code_ >= 200 && status_code_ < 300; }
        
        // JSON解析 (如果启用JSON支持)
        #ifdef ENABLE_JSON_SUPPORT
        nlohmann::json getJson() const;
        #endif
        
    private:
        int status_code_ = 0;
        HttpHeaders headers_;
        std::string body_;
        std::vector<uint8_t> binary_body_;
        std::chrono::milliseconds response_time_{0};
        std::string error_;
        
        friend class HttpClient;
    };
    
    // HTTP客户端配置
    struct HttpClientConfig {
        std::string user_agent = "CrossPlatformSDK/1.0.0";
        std::chrono::milliseconds default_timeout{30000};
        std::chrono::milliseconds connection_timeout{5000};
        int max_redirects = 5;
        bool verify_ssl = true;
        std::string ca_cert_path;
        size_t max_concurrent_requests = 10;
        bool enable_compression = true;
        bool enable_cookies = false;
        std::string proxy_url;
    };
    
    // 请求回调类型
    using ProgressCallback = std::function<void(size_t downloaded, size_t total)>;
    using ResponseCallback = std::function<void(const HttpResponse&)>;
    
    // HTTP客户端类
    class HttpClient {
    public:
        // 构造函数
        explicit HttpClient(const HttpClientConfig& config = HttpClientConfig{});
        
        // 析构函数
        ~HttpClient();
        
        // 禁用拷贝，允许移动
        HttpClient(const HttpClient&) = delete;
        HttpClient& operator=(const HttpClient&) = delete;
        HttpClient(HttpClient&&) noexcept;
        HttpClient& operator=(HttpClient&&) noexcept;
        
        // 同步请求方法
        HttpResponse get(const std::string& url);
        HttpResponse post(const std::string& url, const std::string& body);
        HttpResponse put(const std::string& url, const std::string& body);
        HttpResponse del(const std::string& url);  // delete是关键字
        HttpResponse request(const HttpRequest& request);
        
        // 异步请求方法
        std::future<HttpResponse> getAsync(const std::string& url);
        std::future<HttpResponse> postAsync(const std::string& url, const std::string& body);
        std::future<HttpResponse> putAsync(const std::string& url, const std::string& body);
        std::future<HttpResponse> deleteAsync(const std::string& url);
        std::future<HttpResponse> requestAsync(const HttpRequest& request);
        
        // 带回调的异步请求
        void requestAsync(const HttpRequest& request, ResponseCallback callback);
        
        // 文件下载
        HttpResponse downloadFile(const std::string& url, const std::string& file_path);
        std::future<HttpResponse> downloadFileAsync(const std::string& url, 
                                                   const std::string& file_path,
                                                   ProgressCallback progress = nullptr);
        
        // 文件上传
        HttpResponse uploadFile(const std::string& url, const std::string& file_path,
                               const std::string& field_name = "file");
        std::future<HttpResponse> uploadFileAsync(const std::string& url, 
                                                 const std::string& file_path,
                                                 const std::string& field_name = "file",
                                                 ProgressCallback progress = nullptr);
        
        // 配置管理
        void setConfig(const HttpClientConfig& config);
        const HttpClientConfig& getConfig() const;
        
        // 设置全局头部
        void setGlobalHeader(const std::string& key, const std::string& value);
        void removeGlobalHeader(const std::string& key);
        void clearGlobalHeaders();
        
        // 连接池管理
        void setMaxConcurrentRequests(size_t max_requests);
        size_t getActiveRequestCount() const;
        
        // 取消所有请求
        void cancelAllRequests();
        
        // 获取统计信息
        struct Stats {
            size_t total_requests = 0;
            size_t successful_requests = 0;
            size_t failed_requests = 0;
            std::chrono::milliseconds total_time{0};
            std::chrono::milliseconds average_time{0};
        };
        Stats getStats() const;
        
    private:
        class Impl;
        std::unique_ptr<Impl> pImpl_;
    };
    
    // 便利函数
    namespace http {
        // 快速GET请求
        inline HttpResponse get(const std::string& url) {
            static HttpClient client;
            return client.get(url);
        }
        
        // 快速POST请求
        inline HttpResponse post(const std::string& url, const std::string& body) {
            static HttpClient client;
            return client.post(url, body);
        }
        
        // URL编码
        std::string urlEncode(const std::string& str);
        std::string urlDecode(const std::string& str);
        
        // 构建查询字符串
        std::string buildQueryString(const std::unordered_map<std::string, std::string>& params);
        
        // 解析URL
        struct ParsedUrl {
            std::string scheme;
            std::string host;
            int port = 0;
            std::string path;
            std::string query;
            std::string fragment;
        };
        ParsedUrl parseUrl(const std::string& url);
    }
}
