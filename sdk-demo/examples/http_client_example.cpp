#include "sdk/sdk_core.h"
#include "sdk/network/http_client.h"
#include <iostream>
#include <chrono>
#include <thread>

using namespace sdk;

int main() {
    std::cout << "=== HTTP Client Example ===" << std::endl;
    
    try {
        // 初始化SDK
        SDKConfig config;
        config.user_agent = "HTTP-Client-Example/1.0";
        config.connection_timeout_ms = 5000;
        config.request_timeout_ms = 30000;
        config.log_level = "info";
        config.enable_console_log = true;
        
        auto result = SDK::getInstance().initialize(config);
        if (result != InitResult::SUCCESS) {
            std::cerr << "Failed to initialize SDK" << std::endl;
            return 1;
        }
        
        auto http_client = SDK::getInstance().getHttpClient();
        if (!http_client) {
            std::cerr << "Failed to get HTTP client" << std::endl;
            return 1;
        }
        
        // 示例1: 简单GET请求
        std::cout << "\n--- Example 1: Simple GET Request ---" << std::endl;
        
        auto response = http_client->get("https://httpbin.org/get");
        
        std::cout << "GET Request:" << std::endl;
        std::cout << "  Status Code: " << response.getStatusCode() << std::endl;
        std::cout << "  Response Time: " << response.getResponseTime().count() << " ms" << std::endl;
        std::cout << "  Body Size: " << response.getBody().length() << " bytes" << std::endl;
        
        if (response.isSuccess()) {
            std::cout << "  Body Preview: " << response.getBody().substr(0, 200) << "..." << std::endl;
        } else {
            std::cout << "  Error: " << response.getError() << std::endl;
        }
        
        // 示例2: POST请求
        std::cout << "\n--- Example 2: POST Request ---" << std::endl;
        
        std::string json_data = R"({
            "name": "HTTP Client Example",
            "version": "1.0",
            "platform": "Cross-Platform",
            "timestamp": ")" + std::to_string(std::time(nullptr)) + R"("
        })";
        
        HttpRequest post_request("https://httpbin.org/post");
        post_request.setMethod(HttpMethod::POST)
                   .setHeader("Content-Type", "application/json")
                   .setHeader("Accept", "application/json")
                   .setBody(json_data);
        
        auto post_response = http_client->request(post_request);
        
        std::cout << "POST Request:" << std::endl;
        std::cout << "  Status Code: " << post_response.getStatusCode() << std::endl;
        std::cout << "  Response Time: " << post_response.getResponseTime().count() << " ms" << std::endl;
        
        if (post_response.isSuccess()) {
            std::cout << "  Response received successfully" << std::endl;
        } else {
            std::cout << "  Error: " << post_response.getError() << std::endl;
        }
        
        // 示例3: 异步请求
        std::cout << "\n--- Example 3: Async Requests ---" << std::endl;
        
        // 启动多个异步请求
        std::vector<std::future<HttpResponse>> async_futures;
        
        for (int i = 0; i < 3; ++i) {
            std::string url = "https://httpbin.org/delay/" + std::to_string(i + 1);
            auto future = http_client->getAsync(url);
            async_futures.push_back(std::move(future));
            std::cout << "Started async request " << (i + 1) << " to " << url << std::endl;
        }
        
        // 等待所有异步请求完成
        for (size_t i = 0; i < async_futures.size(); ++i) {
            auto async_response = async_futures[i].get();
            std::cout << "Async request " << (i + 1) << " completed:" << std::endl;
            std::cout << "  Status: " << async_response.getStatusCode() << std::endl;
            std::cout << "  Time: " << async_response.getResponseTime().count() << " ms" << std::endl;
        }
        
        // 示例4: 带认证的请求
        std::cout << "\n--- Example 4: Authenticated Request ---" << std::endl;
        
        HttpRequest auth_request("https://httpbin.org/basic-auth/user/pass");
        auth_request.setBasicAuth("user", "pass");
        
        auto auth_response = http_client->request(auth_request);
        
        std::cout << "Authenticated Request:" << std::endl;
        std::cout << "  Status Code: " << auth_response.getStatusCode() << std::endl;
        
        if (auth_response.isSuccess()) {
            std::cout << "  Authentication successful" << std::endl;
        } else {
            std::cout << "  Authentication failed: " << auth_response.getError() << std::endl;
        }
        
        // 示例5: 自定义头部
        std::cout << "\n--- Example 5: Custom Headers ---" << std::endl;
        
        HttpRequest custom_request("https://httpbin.org/headers");
        custom_request.setHeader("X-Custom-Header", "CrossPlatform-SDK")
                     .setHeader("X-Request-ID", "12345")
                     .setHeader("X-Client-Version", "1.0.0");
        
        auto custom_response = http_client->request(custom_request);
        
        std::cout << "Custom Headers Request:" << std::endl;
        std::cout << "  Status Code: " << custom_response.getStatusCode() << std::endl;
        
        if (custom_response.isSuccess()) {
            std::cout << "  Custom headers sent successfully" << std::endl;
        }
        
        // 示例6: 错误处理
        std::cout << "\n--- Example 6: Error Handling ---" << std::endl;
        
        // 请求不存在的URL
        auto error_response = http_client->get("https://httpbin.org/status/404");
        
        std::cout << "Error Handling:" << std::endl;
        std::cout << "  Status Code: " << error_response.getStatusCode() << std::endl;
        std::cout << "  Is Success: " << (error_response.isSuccess() ? "Yes" : "No") << std::endl;
        
        // 请求无效的URL
        auto invalid_response = http_client->get("https://invalid-domain-that-does-not-exist.com");
        
        std::cout << "Invalid URL:" << std::endl;
        std::cout << "  Status Code: " << invalid_response.getStatusCode() << std::endl;
        std::cout << "  Error: " << invalid_response.getError() << std::endl;
        
        // 示例7: 超时测试
        std::cout << "\n--- Example 7: Timeout Test ---" << std::endl;
        
        HttpRequest timeout_request("https://httpbin.org/delay/10");
        timeout_request.setTimeout(std::chrono::milliseconds(2000)); // 2秒超时
        
        auto start_time = std::chrono::steady_clock::now();
        auto timeout_response = http_client->request(timeout_request);
        auto end_time = std::chrono::steady_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "Timeout Test:" << std::endl;
        std::cout << "  Actual Duration: " << duration.count() << " ms" << std::endl;
        std::cout << "  Status Code: " << timeout_response.getStatusCode() << std::endl;
        std::cout << "  Error: " << timeout_response.getError() << std::endl;
        
        // 示例8: HTTP客户端统计
        std::cout << "\n--- Example 8: Client Statistics ---" << std::endl;
        
        auto stats = http_client->getStats();
        std::cout << "HTTP Client Statistics:" << std::endl;
        std::cout << "  Total requests: " << stats.total_requests << std::endl;
        std::cout << "  Successful requests: " << stats.successful_requests << std::endl;
        std::cout << "  Failed requests: " << stats.failed_requests << std::endl;
        std::cout << "  Average response time: " << stats.average_time.count() << " ms" << std::endl;
        
        // 关闭SDK
        SDK::getInstance().shutdown();
        
        std::cout << "\n=== HTTP Client Example Completed ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
