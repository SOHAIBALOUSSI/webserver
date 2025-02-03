#include <gtest/gtest.h> // Include Google Test
#include "../../include/HttpRequest.hpp"

TEST(HttpRequestParseTest, MalformedRequestLine) {
    const char* buffer = "INVALID /path/to/resource HTTP/1.1\r\n";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;

    EXPECT_THROW({
        try {
            httpRequest.parse(buffer, bufferLen);
        } catch (const HttpRequestError& e) {
            EXPECT_STREQ(e.what(), "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 18\r\n\r\nMalformed request line");
            throw;
        }
    }, HttpRequestError);
}


TEST(HttpRequestParseTest, ValidHeaders) {
    const char* buffer = "Host: example.com\r\nContent-Length: 12\r\n\r\n";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    httpRequest.state = HttpRequest::HEADERS; // Set initial state to HEADERS

    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 41); // Length of headers plus "\r\n\r\n"
    EXPECT_EQ(httpRequest.getState(), HttpRequest::BODY);
    EXPECT_EQ(httpRequest.getHeaders().at("host"), "example.com");
    EXPECT_EQ(httpRequest.getHeaders().at("content-length"), "12");
}


TEST(HttpRequestParseTest, IncompleteHeaders) {
    const char* buffer = "Host: example.com\r\nContent-Length: ";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    httpRequest.state = HttpRequest::HEADERS; // Set initial state to HEADERS

    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 0);
    EXPECT_EQ(httpRequest.getState(), HttpRequest::HEADERS);
}

TEST(HttpRequestParseTest, MalformedHeaders) {
    const char* buffer = "InvalidHeader example.com\r\nContent-Length: 12\r\n\r\n";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    httpRequest.state = HttpRequest::HEADERS; // Set initial state to HEADERS

    EXPECT_THROW({
        try {
            httpRequest.parse(buffer, bufferLen);
        } catch (const HttpRequestError& e) {
            EXPECT_STREQ(e.what(), "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\nContent-Length: 20\r\n\r\nMalformed header field");
            throw;
        }
    }, HttpRequestError);
}

TEST(HttpRequestParseTest, ValidBody) {
    const char* buffer = "Hello World!";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    httpRequest.state = HttpRequest::BODY; // Set initial state to BODY
    httpRequest.getHeaders()["content-length"] = "12"; // Simulate parsed Content-Length header

    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 12); // Length of "Hello World!"
    EXPECT_EQ(httpRequest.getState(), HttpRequest::COMPLETE);
    EXPECT_EQ(httpRequest.getBody(), "Hello World!");
}

TEST(HttpRequestParseTest, IncompleteBody) {
    const char* buffer = "Hello Wo";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    httpRequest.state = HttpRequest::BODY; // Set initial state to BODY
    httpRequest.getHeaders()["content-length"] = "12"; // Simulate parsed Content-Length header

    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 0);
    EXPECT_EQ(httpRequest.getState(), HttpRequest::BODY);
    EXPECT_EQ(httpRequest.getBody(), "Hello Wo"); // Partial body stored
}



                // THIS TEST FAILS nEEd TO fix Body Parsing
TEST(HttpRequestParseTest, ChunkedTransferEncoding) {
    const char* buffer = "4\r\nThis \r\nA\r\nis a chunked\r\nC\r\nrequest.\r\n0\r\n\r\n";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    httpRequest.state = HttpRequest::BODY; // Set initial state to BODY
    httpRequest.getHeaders()["transfer-encoding"] = "chunked"; // Simulate parsed Transfer-Encoding header

    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 49); // Total length of chunks
    EXPECT_EQ(httpRequest.getState(), HttpRequest::COMPLETE);
    EXPECT_EQ(httpRequest.getBody(), "This is a chunked request.");
}

TEST(HttpRequestParseTest, EmptyRequest) {
    const char* buffer = "";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;

    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 0);
    EXPECT_EQ(httpRequest.getState(), HttpRequest::REQUESTLINE);
}

TEST(HttpRequestParseTest, CompleteRequest) {
    const char* buffer = "GET /path/to/resource HTTP/1.1\r\nHost: example.com\r\nContent-Length: 0\r\n\r\n";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    size_t bytesParsed = -1;
    while (bytesParsed != 0)
        bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 0);
    EXPECT_EQ(httpRequest.getState(), HttpRequest::COMPLETE);
    EXPECT_EQ(httpRequest.getMethod(), "GET");
    EXPECT_EQ(httpRequest.getURI(), "/path/to/resource");
    EXPECT_EQ(httpRequest.getVersion(), "HTTP/1.1");
    EXPECT_EQ(httpRequest.getHeaders().at("host"), "example.com");
    EXPECT_EQ(httpRequest.getBody(), ""); // No body expected
}

// Test Case: Valid Request Line
TEST(HttpRequestParseTest, ValidRequestLine) {
    const char* buffer = "GET /path/to/resource HTTP/1.1\r\n";
    size_t bufferLen = strlen(buffer);

    HttpRequest httpRequest;
    size_t bytesParsed = httpRequest.parse(buffer, bufferLen);

    EXPECT_EQ(bytesParsed, 32); 
    EXPECT_EQ(httpRequest.getState(), HttpRequest::HEADERS);
    EXPECT_EQ(httpRequest.getMethod(), "GET");
    EXPECT_EQ(httpRequest.getURI(), "/path/to/resource");
    EXPECT_EQ(httpRequest.getVersion(), "HTTP/1.1");
}


// Test fails in readline for some reason
TEST(HttpRequestParseTest, IncrementalParsing) {
    HttpRequest httpRequest;

    std::string request;
    // First call: Partial request line
    const char* buffer1 = "GET /path/to/resource HT";   
    size_t bufferLen1 = strlen(buffer1);
    request.append(buffer1, bufferLen1);
    size_t bytesReceived = httpRequest.parse(request.c_str(), request.size());
    EXPECT_EQ(bytesReceived, 0);
    EXPECT_EQ(httpRequest.getState(), HttpRequest::REQUESTLINE);

    // Second call: Complete request line
    const char* buffer2 = "TP/1.1\r\n";
    size_t bufferLen2 = strlen(buffer2);
    request.append(buffer2, bufferLen2);
    std::cout << "request: " << request << "\n";
    size_t bytesParsed2 = httpRequest.parse(request.c_str(), request.size());
    EXPECT_EQ(bytesParsed2, 32); //Length of the whole request line
    EXPECT_EQ(httpRequest.getState(), HttpRequest::HEADERS);
    EXPECT_EQ(httpRequest.getMethod(), "GET");
    EXPECT_EQ(httpRequest.getURI(), "/path/to/resource");
    EXPECT_EQ(httpRequest.getVersion(), "HTTP/1.1");

    // Third call: Headers
    const char* buffer3 = "Host: example.com\r\nContent-Length: 12\r\n\r\n";
    size_t bufferLen3 = strlen(buffer3);
    size_t bytesParsed3 = httpRequest.parse(buffer3, bufferLen3);
    EXPECT_EQ(bytesParsed3, 39); // Length of headers plus "\r\n\r\n"
    EXPECT_EQ(httpRequest.getState(), HttpRequest::BODY);
    EXPECT_EQ(httpRequest.getHeaders().at("host"), "example.com");
    EXPECT_EQ(httpRequest.getHeaders().at("content-length"), "12");
    // Fourth call: Body
    const char* buffer4 = "Hello World!";
    size_t bufferLen4 = strlen(buffer4);
    size_t bytesParsed4 = httpRequest.parse(buffer4, bufferLen4);
    EXPECT_EQ(bytesParsed4, 12); // Length of body
    EXPECT_EQ(httpRequest.getState(), HttpRequest::COMPLETE);
    EXPECT_EQ(httpRequest.getBody(), "Hello World!");
}

TEST(HttpRequestParseTest, LargeRequest) {
    HttpRequest httpRequest;

    // First call: Headers
    const char* buffer1 = "GET /path/to/resource HTTP/1.1\r\nHost: example.com\r\nContent-Length: 20\r\n\r\n";
    size_t bufferLen1 = strlen(buffer1);
    std::string request;
    request.append(buffer1, bufferLen1);
    size_t bytesReceived = -1;
    size_t bytesParsed1 = 0;
    while (bytesReceived != 0)
    {
        bytesReceived = httpRequest.parse(request.c_str(), request.size());
        bytesParsed1 += bytesReceived;
    }
    EXPECT_EQ(bytesParsed1, 51); // Length of headers plus "\r\n\r\n"   
    EXPECT_EQ(httpRequest.getState(), HttpRequest::BODY);
    EXPECT_EQ(httpRequest.getHeaders().at("content-length"), "20");

    // Second call: Partial body
    const char* buffer2 = "This is a very lon";
    size_t bufferLen2 = strlen(buffer2);
    request.append(buffer2, bufferLen2);
    size_t bytesParsed2 = httpRequest.parse(request.c_str(), request.size());
    EXPECT_EQ(bytesParsed2, 0); // Not enough data
    EXPECT_EQ(httpRequest.getState(), HttpRequest::BODY);
    EXPECT_EQ(httpRequest.getBody(), "This is a very lon"); // Partial body stored

    // Third call: Remaining body
    const char* buffer3 = "g request body";
    size_t bufferLen3 = strlen(buffer3);
    request. append(buffer3, bufferLen3);
    size_t bytesParsed3 = httpRequest.parse(request.c_str(), request.size());
    EXPECT_EQ(bytesParsed3, 13); // Remaining length of "g request body"
    EXPECT_EQ(httpRequest.getState(), HttpRequest::COMPLETE);
    EXPECT_EQ(httpRequest.getBody(), "This is a very long request body");
}


//g++ -std=c++17 -pthread HttpRequestTest.cpp ../Common.cpp ../config/Config.cpp ../Route.cpp HttpRequest.cpp -lgtest -lgtest_main -o HttpRequestTest
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv); // Initialize Google Test
    return RUN_ALL_TESTS(); // Run all tests
}