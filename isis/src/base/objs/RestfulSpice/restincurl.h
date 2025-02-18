#pragma once

/*
    MIT License

    Copyright (c) 2018 Jarle Aase

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.

    On Github: https://github.com/jgaa/RESTinCurl
*/

/*! \file */ 

#include <algorithm>
#include <atomic>
#include <deque>
#include <exception>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <assert.h>
#include <curl/curl.h>
#include <curl/easy.h>
#include <fcntl.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef RESTINCURL_WITH_OPENSSL_THREADS
#   include <openssl/crypto.h>
#endif

/*! \def RESTINCURL_MAX_CONNECTIONS
 * \brief Max concurrent connections
 * 
 * This option restrains the maximum number of concurrent
 * connections that will be used at any time. It sets 
 * libcurl's `CURLMOPT_MAXCONNECTS` option, and also
 * puts any new requests in a waiting queue. As soon as 
 * some active request finish, the oldest waiting request
 * will be served (FIFO queue).
 * 
 * The default value is 32
 */
#ifndef RESTINCURL_MAX_CONNECTIONS
#   define RESTINCURL_MAX_CONNECTIONS 32L
#endif

/*! \def RESTINCURL_ENABLE_ASYNC
 * \brief Enables or disables asynchronous mode.
 * 
 * In  asynchronous mode, many requests can be 
 * served simultaneously using a worker-thread. 
 * In synchronous mode, you use the current
 * thread to serve only one request at the time. 
 * 
 * Default is 1 (asynchronous mode enabled).
 */

#ifndef RESTINCURL_ENABLE_ASYNC
#   define RESTINCURL_ENABLE_ASYNC 1
#endif

/*! \def RESTINCURL_IDLE_TIMEOUT_SEC
 * \brief How long to wait for the next request before the idle worker-thread is stopped.
 * 
 * This will delete curl's connection-cache and cause a new thread to be created
 * and new connections to be made if there are new requests at at later time.
 * 
 * Note that this option is only relevant in asynchronous mode.
 * 
 * Default is 60 seconds.
 */
#ifndef RESTINCURL_IDLE_TIMEOUT_SEC
#   define RESTINCURL_IDLE_TIMEOUT_SEC 60
#endif

/*! \def RESTINCURL_LOG_VERBOSE_ENABLE
 * \brief Enable very verbose logging
 * 
 * This option enabled very verbose logging, suitable to
 * pinpoint problems during development / porting of the library itself.
 * 
 * Default is 0 (disabled).
 */
#ifndef RESTINCURL_LOG_VERBOSE_ENABLE
#   define RESTINCURL_LOG_VERBOSE_ENABLE 0
#endif

#if defined(_LOGFAULT_H) && !defined (RESTINCURL_LOG) && !defined (RESTINCURL_LOG_TRACE)
#   define RESTINCURL_LOG(msg)    LFLOG_DEBUG << "restincurl: " << msg
#   if RESTINCURL_LOG_VERBOSE_ENABLE
#       define RESTINCURL_LOG_TRACE(msg) LFLOG_IFALL_TRACE("restincurl: " << msg)
#   endif // RESTINCURL_LOG_VERBOSE_ENABLE
#endif

#if defined(RESTINCURL_USE_SYSLOG) || defined(RESTINCURL_USE_ANDROID_NDK_LOG)
#   ifdef RESTINCURL_USE_SYSLOG
#       include <syslog.h>
#   endif
#   ifdef RESTINCURL_USE_ANDROID_NDK_LOG
#       include <android/log.h>
#   endif
#   include <sstream>
#   define RESTINCURL_LOG(msg) ::restincurl::Log(restincurl::LogLevel::DEBUG).Line() << msg
#endif

/*! \def RESTINCURL_ENABLE_DEFAULT_LOGGER
 * \brief Enables a simple built-in logger
 * 
 * RESTinCurl has a very simple built in logger.
 * 
 * It writes to either std::clog, the Unix syslog or Androids log facility.
 * 
 * - Define RESTINCURL_USE_SYSLOG to use syslog
 * - Define RESTINCURL_USE_ANDROID_NDK_LOG to use the Android NDK logger.
 * 
 * By default, it will write to std::clog.
 * 
 * Default value is 0 (disabled)
 */
#ifndef RESTINCURL_ENABLE_DEFAULT_LOGGER
#   define RESTINCURL_ENABLE_DEFAULT_LOGGER 0
#endif

/*! \def RESTINCURL_LOG
 * \brief Macro to log debug messages
 * 
 * If you want to log messages from the library to your own log facility, you
 * may define this macro to do so.
 * 
 * Note that the logging statements in the library expect the log `msg` to be
 * std::ostream compliant. The macro must be able to deal with statement such as:
 *   - RESTINCURL_LOG("test");
 *   - RESTINCURL_LOG("test " << 1 << " and " << 2);
 * 
 * If you manually define this macro, you should not define `RESTINCURL_ENABLE_DEFAULT_LOGGER`.
 */
#ifndef RESTINCURL_LOG
#   if RESTINCURL_ENABLE_DEFAULT_LOGGER
#       define RESTINCURL_LOG(msg) std::clog << msg << std::endl
#   else
#       define RESTINCURL_LOG(msg)
#   endif
#endif

/*! \def RESTINCURL_LOG_TRACE
 * \brief Macro to log debug messages
 * 
 * If you want to log trace-messages from the library to your own log facility, you
 * may define this macro to do so.
 * 
 * Note that the logging statements in the library expect the log `msg` to be
 * std::ostream compliant. The macro must be able to deal with statement such as:
 *   - RESTINCURL_LOG_TRACE("test");
 *   - RESTINCURL_LOG_TRACE("test " << 1 << " and " << 2);
 * 
 * If you manually define this macro, you should not define `RESTINCURL_ENABLE_DEFAULT_LOGGER`.
 * 
 * This macro is only called if `RESTINCURL_LOG_VERBOSE_ENABLE` is defined and not 0.
 */
#ifndef RESTINCURL_LOG_TRACE
#   if RESTINCURL_LOG_VERBOSE_ENABLE
#       define RESTINCURL_LOG_TRACE(msg) RESTINCURL_LOG(msg)
#   else
#       define RESTINCURL_LOG_TRACE(msg)
#   endif
#endif

namespace restincurl {

#if defined(RESTINCURL_USE_SYSLOG) || defined(RESTINCURL_USE_ANDROID_NDK_LOG)
    enum class LogLevel { DEBUG };

    class Log {
    public:
        Log(const LogLevel level) : level_{level} {}
        ~Log() {
#   ifdef RESTINCURL_USE_SYSLOG
            static const std::array<int, 1> syslog_priority = { LOG_DEBUG };
            static std::once_flag syslog_opened;
            std::call_once(syslog_opened, [] {
                openlog(nullptr, 0, LOG_USER);
            });
#   endif
#   ifdef RESTINCURL_USE_ANDROID_NDK_LOG
            static const std::array<int, 1> android_priority = { ANDROID_LOG_DEBUG };
#   endif
            const auto msg = out_.str();

#   ifdef RESTINCURL_USE_SYSLOG
            syslog(syslog_priority.at(static_cast<int>(level_)), "%s", msg.c_str());
#   endif
#   ifdef RESTINCURL_USE_ANDROID_NDK_LOG
            __android_log_write(android_priority.at(static_cast<int>(level_)),
                                "restincurl", msg.c_str());
#   endif
        }

        std::ostringstream& Line() { return out_; }

private:
        const LogLevel level_;
        std::ostringstream out_;
    };
#endif



    using lock_t = std::lock_guard<std::mutex>;

    /*! The Result from a request\
     */
    struct Result {
        Result() = default;
        Result(const CURLcode& code) {
            curl_code = code;
            msg = curl_easy_strerror(code);
        }
        
        /*! Check if the reqtest appears to be successful */
        bool isOk() const noexcept {
            if (curl_code == CURLE_OK) {
                if ((http_response_code >= 200) && (http_response_code < 300)) {
                    return true;
                }
            }
            
            return false;
        }

        /*! The CURLcode returned by libcurl for this request.
         * 
         * CURLE_OK (or 0) indicates success.
         */
        CURLcode curl_code = {};
        
        /*! The HTTP result code for the request */
        long http_response_code = {};
        
        /*! If the request was unsuccessful (curl_code != 0), the error string reported by libcurl. */
        std::string msg;
        
        /*! The body of the request returned by the server.
         * 
         * Note that if you specified your own body handler or body variable, for the request, `body` will be empty.
         */
        std::string body;
    };

    enum class RequestType { GET, PUT, POST, HEAD, DELETE, PATCH, OPTIONS, POST_MIME, INVALID };
    
    /*! Completion debug_callback
     * 
     * This callback is called when a request completes, or fails.
     * 
     * \param result The result of the request.
     */
    using completion_fn_t = std::function<void (const Result& result)>;

    /*! Base class for RESTinCurl exceptions */
    class Exception : public std::runtime_error {
    public:
        Exception(const std::string& msg) : runtime_error(msg) {}
    };

    /*! Exception thrown when some system function, like `pipe()` failed. */
    class SystemException : public Exception {
    public:
        SystemException(const std::string& msg, const int e) : Exception(msg + " " + strerror(e)), err_{e} {}

        int getErrorCode() const noexcept { return err_; }

    private:
        const int err_;
    };

    /*! Exception thrown when a curl library method failed. */
    class CurlException : public Exception {
    public:
        CurlException(const std::string msg, const CURLcode err)
            : Exception(msg + '(' + std::to_string(err) + "): " + curl_easy_strerror(err))
            , err_{err}
            {}

         CurlException(const std::string msg, const CURLMcode err)
            : Exception(msg + '(' + std::to_string(err) + "): " + curl_multi_strerror(err))
            , err_{err}
            {}

        int getErrorCode() const noexcept { return err_; }

    private:
        const int err_;
    };

    class EasyHandle {
    public:
        using ptr_t = std::unique_ptr<EasyHandle>;
        using handle_t = decltype(curl_easy_init());

        EasyHandle() {
            RESTINCURL_LOG("EasyHandle created: " << handle_);
        }

        ~EasyHandle() {
            Close();
        }

        void Close() {
            if (handle_) {
                RESTINCURL_LOG("Cleaning easy-handle " << handle_);
                curl_easy_cleanup(handle_);
                handle_ = nullptr;
            }
        }

        operator handle_t () const noexcept { return handle_; }

    private:
        handle_t handle_ = curl_easy_init();
    };

    /*! Curl option wrapper class
     * 
     * This is just a thin C++ wrapper over Curl's `curl_easy_setopt()` method.
     */
    class Options {
    public:
        Options(EasyHandle& eh) : eh_{eh} {}

        /*! Set an option 
         * 
         * \param opt CURLoption enum to change
         * \param value Value to set
         */
        template <typename T>
        Options& Set(const CURLoption& opt, const T& value) {
            const auto ret = curl_easy_setopt(eh_, opt, value);
            if (ret) {
                throw CurlException(
                    std::string("Setting option ") + std::to_string(opt), ret);
            }
            return *this;
        }

        /*! Set an option 
         * 
         * \param opt CURLoption enum to change
         * \param value String value to set
         */
        Options& Set(const CURLoption& opt, const std::string& value) {
            return Set(opt, value.c_str());
        }

    private:
        EasyHandle& eh_;
    };

    /*! Abstract base class for Data Handlers
     * 
     * Data handlers are used to handle libcurl's callback requirements
     * for input and output data.
     */
    struct DataHandlerBase {
        virtual ~DataHandlerBase() = default;
    };

    /*! Template implementation for input data to curl during a request.
     * 
     * This handler deals with the data received from the HTTP server
     * during a request. This implementation will typically use
     * T=std::string and just store the received data in a string. For 
     * json/XML payloads that's probably all you need. But if you receive
     * binary data, you may want to use a container like std::vector or std::deque in stead.
     */
    template <typename T>
    struct InDataHandler : public DataHandlerBase{
        InDataHandler(T& data) : data_{data} {
            RESTINCURL_LOG_TRACE("InDataHandler address: " << this);
        }

        static size_t write_callback(char *ptr, size_t size, size_t nitems, void *userdata) {
            assert(userdata);
            auto self = reinterpret_cast<InDataHandler *>(userdata);
            const auto bytes = size * nitems;
            if (bytes > 0) {
                std::copy(ptr, ptr + bytes, std::back_inserter(self->data_));
            }
            return bytes;
        }

        T& data_;
    };

     /*! Template implementation for output data to curl during a request.
     * 
     * This handler deals with the data sent to the HTTP server
     * during a request (POST, PATCH etc). This implementation will typically use
     * T=std::string and just store the data in a string. For 
     * json/XML payloads that's probably all you need. But if you send
     * binary data, you may want to use a container like std::vector or std::deque in stead.
     */
    template <typename T>
    struct OutDataHandler : public DataHandlerBase {
        OutDataHandler() = default;
        OutDataHandler(const T& v) : data_{v} {}
        OutDataHandler(T&& v) : data_{std::move(v)} {}

        static size_t read_callback(char *bufptr, size_t size, size_t nitems, void *userdata) {
            assert(userdata);
            OutDataHandler *self = reinterpret_cast<OutDataHandler *>(userdata);
            const auto bytes = size * nitems;
            auto out_bytes = std::min<size_t>(bytes, (self->data_.size() - self->sendt_bytes_));
            std::copy(self->data_.cbegin() + self->sendt_bytes_,
                      self->data_.cbegin() + (self->sendt_bytes_ + out_bytes),
                      bufptr);
            self->sendt_bytes_ += out_bytes;

            RESTINCURL_LOG_TRACE("Sent " << out_bytes << " of total " << self->data_.size() << " bytes.");
            return out_bytes;
        }

        T data_;
        size_t sendt_bytes_ = 0;
    };

    class Request {
    public:
        using ptr_t = std::unique_ptr<Request>;

        Request()
        : eh_{std::make_unique<EasyHandle>()}
        {
        }

        Request(EasyHandle::ptr_t&& eh)
        : eh_{std::move(eh)}
        {
        }

        ~Request() {
            if (headers_) {
                curl_slist_free_all(headers_);
            }
        }

        void Prepare(const RequestType rq, completion_fn_t completion) {
            request_type_ = rq;
            SetRequestType();
            completion_ = std::move(completion);
        }

        void OpenSourceFile(const std::string& path) {
            assert(!fp_);
            auto fp = fopen(path.c_str(), "rb");
            if (!fp) {
                const auto e = errno;
                throw SystemException{std::string{"Unable to open file "} + path, e};
            }

            fp_= std::shared_ptr<FILE>(fp, std::fclose);
        }

        FILE *GetSourceFp() {
            assert(fp_);
            return fp_.get();
        }

        // Synchronous execution.
        void Execute() {
            const auto result = curl_easy_perform(*eh_);
            CallCompletion(result);
        }

        void Complete(CURLcode cc, const CURLMSG& /*msg*/) {
            CallCompletion(cc);
        }

        EasyHandle& GetEasyHandle() noexcept { assert(eh_); return *eh_; }
        RequestType GetRequestType() noexcept { return request_type_; }

        void SetDefaultInHandler(std::unique_ptr<DataHandlerBase> ptr) {
            default_in_handler_ = std::move(ptr);
        }

        void SetDefaultOutHandler(std::unique_ptr<DataHandlerBase> ptr) {
            default_out_handler_ = std::move(ptr);
        }

        using headers_t = curl_slist *;
        headers_t& GetHeaders() {
            return headers_;
        }

        std::string& getDefaultInBuffer() {
            return default_data_buffer_;
        }

        void InitMime() {
            if (!mime_) {
                mime_ = curl_mime_init(*eh_);
            }
        }

        void AddFileAsMimeData(const std::string& path,
                               const std::string& name,
                               const std::string& remoteName,
                               const std::string& mimeType) {

            InitMime();
            assert(mime_);
            auto * part = curl_mime_addpart(mime_);
            curl_mime_filedata(part, path.c_str());
            curl_mime_name(part, name.empty() ? "file" :name.c_str());

            if (!remoteName.empty()) {
                curl_mime_filename(part, remoteName.c_str());
            }

            if (!mimeType.empty()) {
                curl_mime_type(part, mimeType.c_str());
            }
        }

    private:
        void CallCompletion(CURLcode cc) {
            Result result(cc);

            curl_easy_getinfo (*eh_, CURLINFO_RESPONSE_CODE,
                               &result.http_response_code);
            RESTINCURL_LOG("Complete: http code: " << result.http_response_code);
            if (completion_) {
                if (!default_data_buffer_.empty()) {
                    result.body = std::move(default_data_buffer_);
                }
                completion_(result);
            }
        }

        void SetRequestType() {
            switch(request_type_) {
                case RequestType::GET:
                    curl_easy_setopt(*eh_, CURLOPT_HTTPGET, 1L);
                    break;
                case RequestType::PUT:
                    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
                    curl_easy_setopt(*eh_, CURLOPT_UPLOAD, 1L);
                    break;
                case RequestType::POST:
                    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
                    curl_easy_setopt(*eh_, CURLOPT_UPLOAD, 0L);
                    curl_easy_setopt(*eh_, CURLOPT_POST, 1L);
                    break;
                case RequestType::HEAD:
                    curl_easy_setopt(*eh_, CURLOPT_NOBODY, 1L);
                    break;
                case RequestType::OPTIONS:
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "OPTIONS");
                    break;
                case RequestType::PATCH:
                    headers_ = curl_slist_append(headers_, "Transfer-Encoding: chunked");
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "PATCH");
                    break;
                case RequestType::DELETE:
                    curl_easy_setopt(*eh_, CURLOPT_CUSTOMREQUEST, "DELETE");
                    break;
                case RequestType::POST_MIME:
                    InitMime();
                    curl_easy_setopt(*eh_, CURLOPT_MIMEPOST, mime_);
                    break;
                default:
                    throw Exception("Unsupported request type" + std::to_string(static_cast<int>(request_type_)));
            }
        }

        EasyHandle::ptr_t eh_;
        RequestType request_type_ = RequestType::INVALID;
        completion_fn_t completion_;
        std::unique_ptr<DataHandlerBase> default_out_handler_;
        std::unique_ptr<DataHandlerBase> default_in_handler_;
        headers_t headers_ = nullptr;
        std::string default_data_buffer_;
        std::shared_ptr<FILE> fp_;
        curl_mime *mime_ = {};
    };

#if RESTINCURL_ENABLE_ASYNC

    class Signaler {
        enum FdUsage { FD_READ = 0, FD_WRITE = 1};

    public:
        using pipefd_t = std::array<int, 2>;

        Signaler() {
            auto status = pipe(pipefd_.data());
            if (status) {
                throw SystemException("pipe", status);
            }
            for(auto fd : pipefd_) {
                int flags = 0;
                if (-1 == (flags = fcntl(fd, F_GETFL, 0)))
                    flags = 0;
                fcntl(fd, F_SETFL, flags | O_NONBLOCK);
            }
        }

        ~Signaler() {
            for(auto fd : pipefd_) {
                close(fd);
            }
        }

        void Signal() {
            char byte = {};
            RESTINCURL_LOG_TRACE("Signal: Signaling!");
            if (write(pipefd_[FD_WRITE], &byte, 1) != 1) {
                throw SystemException("write pipe", errno);
            }
        }

        int GetReadFd() { return pipefd_[FD_READ]; }

        bool WasSignalled() {
            bool rval = false;
            char byte = {};
            while(read(pipefd_[FD_READ], &byte, 1) > 0) {
                RESTINCURL_LOG_TRACE("Signal: Was signalled");
                rval = true;
            }

            return rval;
        }

    private:
        pipefd_t pipefd_;
    };
    
    /*! Thread support for the TLS layer used by libcurl.
     * 
     * Some TLS libraries require that you supply callback functions
     * to deal with thread synchronization.
     * 
     * See https://curl.haxx.se/libcurl/c/threadsafe.html
     * 
     * You can deal with this yourself, or in the case
     * that RESTinCurl support your TLS library, you can use
     * this class.
     * 
     * Currently supported libraries:
     * 
     *  - Opeenssl (only required for OpenSSL <= 1.0.2) 
     *      define `RESTINCURL_WITH_OPENSSL_THREADS`
     * 
     * The class is written so that you can use it in your code, and
     * it will only do something when the appropriate define is set.
     * 
     * This class is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
     */
    class TlsLocker 
    {
    public:
        
#ifdef RESTINCURL_WITH_OPENSSL_THREADS 
    static void opensslLockCb(int mode, int type, char *, int) {
        if(mode & CRYPTO_LOCK) {
           lock();
        }
        else {
           unlock();
        }
    }
    
    static unsigned long getThreadId(void) {
        return reinterpret_cast<unsigned long>(::this_thread::get_id());
    }
    
    /*! Enable the built-in support. */
    static void tlsLockInit() {
        CRYPTO_set_id_callback((unsigned long (*)())getThreadId);
        CRYPTO_set_locking_callback((void (*)())opensslLockCb);
    }
    
    /*! Free up resources used when finished using TLS. */
    static void tlsLockKill() {
        CRYPTO_set_locking_callback(NULL);
    }
    
#else
    /*! Enable the built-in support. */
    static void tlsLOckInit() {
        ; // Do nothing
    }
    
    /*! Free up resources used when finished using TLS. */
    static void tlsLockKill() {
        ; // Do nothing
    }
#endif


    

    private:
        static void lock() {
            mutex_.lock();
        }
        
        static void unlock() {
            mutex_.unlock();
        }
        
        static std::mutex mutex_;
    };
    
    class Worker {
        class WorkerThread {
        public:
            WorkerThread(std::function<void ()> && fn)
            : thread_{std::move(fn)} {}

            ~WorkerThread() {
                if (thread_.get_id() == std::this_thread::get_id()) {
                    // Allow the thread to finish exiting the lambda
                    thread_.detach();
                }
            }

            void Join() const {
                std::call_once(joined_, [this] {
                    const_cast<WorkerThread *>(this)->thread_.join();
                });
            }

            bool Joinable() const {
                return thread_.joinable();
            }

            operator std::thread& () { return thread_; }

        private:
            std::thread thread_;
            mutable std::once_flag joined_;
        };

    public:
        Worker() = default;

        ~Worker() {
            if (thread_ && thread_->Joinable()) {
                Close();
                Join();
            }
        }

        void PrepareThread() {
            // Must only be called when the mutex_ is acquired!
            assert(!mutex_.try_lock());
            if (abort_ || done_) {
                return;
            }
            if (!thread_) {
                thread_ = std::make_shared<WorkerThread>([&] {
                    try {
                        RESTINCURL_LOG("Starting thread " << std::this_thread::get_id());
                        Init();
                        Run();
                        Clean();
                    } catch (const std::exception& ex) {
                        RESTINCURL_LOG("Worker: " << ex.what());
                    }
                    RESTINCURL_LOG("Exiting thread " << std::this_thread::get_id());

                    lock_t lock(mutex_);
                    thread_.reset();
                });
            }
            assert(!abort_);
            assert(!done_);
        }

        static std::unique_ptr<Worker> Create() {
            return std::make_unique<Worker>();
        }

        void Enqueue(Request::ptr_t req) {
            RESTINCURL_LOG_TRACE("Queuing request ");
            lock_t lock(mutex_);
            PrepareThread();
            queue_.push_back(std::move(req));
            Signal();
        }

        void Join() const {
            decltype(thread_) thd;

            {
                lock_t lock(mutex_);
                if (thread_ && thread_->Joinable()) {
                    thd = thread_;
                }
            }

            if (thd) {
                thd->Join();
            }
        }

        // Let the current transfers complete, then quit
        void CloseWhenFinished() {
            {
                lock_t lock(mutex_);
                close_pending_ = true;
            }
            Signal();
        }

        // Shut down now. Abort all transfers
        void Close() {
            {
                lock_t lock(mutex_);
                abort_ = true;
            }
            Signal();
        }

        // Check if the run loop has finished.
        bool IsDone() const {
            lock_t lock(mutex_);
            return done_;
        }

        bool HaveThread() const noexcept {
            lock_t lock(mutex_);
            if (thread_) {
                return true;
            }
            return false;
        }

        size_t GetNumActiveRequests() const {
            lock_t lock(mutex_);
            return ongoing_.size();
        }

    private:
        void Signal() {
            signal_.Signal();
        }

        void Dequeue() {
            decltype(queue_) tmp;

            {
                lock_t lock(mutex_);
                if ((queue_.size() + ongoing_.size()) <= RESTINCURL_MAX_CONNECTIONS) {
                    tmp = std::move(queue_);
                    pending_entries_in_queue_ = false;
                } else {
                    auto remains = std::min<size_t>(RESTINCURL_MAX_CONNECTIONS - ongoing_.size(), queue_.size());
                    if (remains) {
                        auto it = queue_.begin();
                        RESTINCURL_LOG_TRACE("Adding only " << remains << " of " << queue_.size()
                            << " requests from queue: << RESTINCURL_MAX_CONNECTIONS=" << RESTINCURL_MAX_CONNECTIONS);
                        while(remains--) {
                            assert(it != queue_.end());
                            tmp.push_back(std::move(*it));
                            ++it;
                        }
                        queue_.erase(queue_.begin(), it);
                    } else {
                        assert(ongoing_.size() == RESTINCURL_MAX_CONNECTIONS);
                        RESTINCURL_LOG_TRACE("Adding no entries from queue: RESTINCURL_MAX_CONNECTIONS="
                            << RESTINCURL_MAX_CONNECTIONS);
                    }
                    pending_entries_in_queue_ = true;
                }
            }

            for(auto& req: tmp) {
                assert(req);
                const auto& eh = req->GetEasyHandle();
                RESTINCURL_LOG_TRACE("Adding request: " << eh);
                ongoing_[eh] = std::move(req);
                const auto mc = curl_multi_add_handle(handle_, eh);
                if (mc != CURLM_OK) {
                    throw CurlException("curl_multi_add_handle", mc);
                }
            }
        }

        void Init() {
            if ((handle_ = curl_multi_init()) == nullptr) {
                throw std::runtime_error("curl_multi_init() failed");
            }

            curl_multi_setopt(handle_, CURLMOPT_MAXCONNECTS, RESTINCURL_MAX_CONNECTIONS);
        }

        void Clean() {
            if (handle_) {
                RESTINCURL_LOG_TRACE("Calling curl_multi_cleanup: " << handle_);
                curl_multi_cleanup(handle_);
                handle_ = nullptr;
            }
        }

        bool EvaluateState(const bool transfersRunning, const bool doDequeue) const noexcept {
            lock_t lock(mutex_);

            RESTINCURL_LOG_TRACE("Run loop: transfers_running=" << transfersRunning
                << ", do_dequeue=" << doDequeue
                << ", close_pending_=" << close_pending_);

            return !abort_ && (transfersRunning || !close_pending_);
        }

        auto GetNextTimeout() const noexcept {
            return std::chrono::steady_clock::now()
                + std::chrono::seconds(RESTINCURL_IDLE_TIMEOUT_SEC);
        }

        void Run() {
            int transfers_running = -1;
            fd_set fdread = {};
            fd_set fdwrite = {};
            fd_set fdexcep = {};
            bool do_dequeue = true;
            auto timeout = GetNextTimeout();

            while (EvaluateState(transfers_running, do_dequeue)) {

                if (do_dequeue) {
                    Dequeue();
                    do_dequeue = false;
                }

                /* timeout or readable/writable sockets */
                const bool initial_ideling = transfers_running == -1;
                curl_multi_perform(handle_, &transfers_running);
                if ((transfers_running == 0) && initial_ideling) {
                    transfers_running = -1; // Let's ignore close_pending_ until we have seen a request
                }

                // Shut down the thread if we have been idling too long
                if (transfers_running <= 0) {
                    if (timeout < std::chrono::steady_clock::now()) {
                        RESTINCURL_LOG("Idle timeout. Will shut down the worker-thread.");
                        break;
                    }
                } else {
                    timeout = GetNextTimeout();
                }

                int numLeft = {};
                while (auto m = curl_multi_info_read(handle_, &numLeft)) {
                    assert(m);
                    auto it = ongoing_.find(m->easy_handle);
                    if (it != ongoing_.end()) {
                        RESTINCURL_LOG("Finishing request with easy-handle: "
                            << (EasyHandle::handle_t)it->second->GetEasyHandle()
                            << "; with result: " << m->data.result << " expl: '" << curl_easy_strerror(m->data.result)
                            << "'; with msg: " << m->msg);

                        try {
                            it->second->Complete(m->data.result, m->msg);
                        } catch(const std::exception& ex) {
                            RESTINCURL_LOG("Complete threw: " << ex.what());
                        }
                        if (m->msg == CURLMSG_DONE) {
                            curl_multi_remove_handle(handle_, m->easy_handle);
                        }
                        it->second->GetEasyHandle().Close();
                        ongoing_.erase(it);
                    } else {
                        RESTINCURL_LOG("Failed to find easy_handle in ongoing!");
                        assert(false);
                    }
                }

                {
                    lock_t lock(mutex_);
                    // Avoid using select() as a timer when we need to exit anyway
                    if (abort_ || (!transfers_running && close_pending_)) {
                        break;
                    }
                }

                auto next_timeout = std::chrono::duration_cast<std::chrono::milliseconds>(
                    timeout - std::chrono::steady_clock::now());
                long sleep_duration = std::max<long>(1, next_timeout.count());
                /* extract sleep_duration value */


                FD_ZERO(&fdread);
                FD_ZERO(&fdwrite);
                FD_ZERO(&fdexcep);

                int maxfd = -1;
                if (transfers_running > 0) {
                    curl_multi_timeout(handle_, &sleep_duration);
                    if (sleep_duration < 0) {
                        sleep_duration = 1000;
                    }

                    /* get file descriptors from the transfers */
                    const auto mc = curl_multi_fdset(handle_, &fdread, &fdwrite, &fdexcep, &maxfd);
                    RESTINCURL_LOG_TRACE("maxfd: " << maxfd);
                    if (mc != CURLM_OK) {
                        throw CurlException("curl_multi_fdset", mc);
                    }

                    if (maxfd == -1) {
                        // Curl want's us to revisit soon
                        sleep_duration = 50;
                    }
                } // active transfers

                struct timeval tv = {};
                tv.tv_sec = sleep_duration / 1000;
                tv.tv_usec = (sleep_duration % 1000) * 1000;

                const auto signalfd = signal_.GetReadFd();

                RESTINCURL_LOG_TRACE("Calling select() with timeout of "
                    << sleep_duration
                    << " ms. Next timeout in " << next_timeout.count() << " ms. "
                    << transfers_running << " active transfers.");

                FD_SET(signalfd, &fdread);
                maxfd = std::max(signalfd,  maxfd) + 1;

                const auto rval = select(maxfd, &fdread, &fdwrite, &fdexcep, &tv);
                RESTINCURL_LOG_TRACE("select(" << maxfd << ") returned: " << rval);

                if (rval > 0) {
                    if (FD_ISSET(signalfd, &fdread)) {
                        RESTINCURL_LOG_TRACE("FD_ISSET was true: ");
                        do_dequeue = signal_.WasSignalled();
                    }

                }
                if (pending_entries_in_queue_) {
                    do_dequeue = true;
                }
            } // loop


            lock_t lock(mutex_);
            if (close_pending_ || abort_) {
                done_ = true;
            }
        }

        bool close_pending_ {false};
        bool abort_ {false};
        bool done_ {false};
        bool pending_entries_in_queue_ = false;
        decltype(curl_multi_init()) handle_ = {};
        mutable std::mutex mutex_;
        std::shared_ptr<WorkerThread> thread_;
        std::deque<Request::ptr_t> queue_;
        std::map<EasyHandle::handle_t, Request::ptr_t> ongoing_;
        Signaler signal_;
    };
#endif // RESTINCURL_ENABLE_ASYNC

  
    /*! Convenient interface to build requests.
     * 
     * Even if this is a light-weight wrapper around libcurl, we have a 
     * simple and modern way to define our requests that contains
     * convenience-methods for the most common use-cases. 
     */
    class RequestBuilder {
        // noop handler for incoming data
        static size_t write_callback(char *ptr, size_t size, size_t nitems, void *userdata) {
            const auto bytes = size * nitems;
            return bytes;
        }

        static int debug_callback(CURL *handle, curl_infotype type,
             char *data, size_t size,
             void *userp) {

            std::string msg;
            switch(type) {
            case CURLINFO_TEXT:
                msg = "==> Info: ";
                break;
            case CURLINFO_HEADER_OUT:
                msg =  "=> Send header: ";
                break;
            case CURLINFO_DATA_OUT:
                msg = "=> Send data: ";
                break;
            case CURLINFO_SSL_DATA_OUT:
                msg = "=> Send SSL data: ";
                break;
            case CURLINFO_HEADER_IN:
                msg = "<= Recv header: ";
                break;
            case CURLINFO_DATA_IN:
                msg = "<= Recv data: ";
                break;
            case CURLINFO_SSL_DATA_IN:
                msg = "<= Recv SSL data: ";
                break;
            case CURLINFO_END: // Don't seems to be used
                msg = "<= End: ";
                break;
            }

            std::copy(data, data + size, std::back_inserter(msg));
            RESTINCURL_LOG(handle << " " << msg);
            return 0;
        }

    public:
        using ptr_t = std::unique_ptr<RequestBuilder>;
        RequestBuilder(
#if RESTINCURL_ENABLE_ASYNC
            Worker& worker
#endif
        )
        : request_{std::make_unique<Request>()}
        , options_{std::make_unique<class Options>(request_->GetEasyHandle())}
#if RESTINCURL_ENABLE_ASYNC
        , worker_(worker)
#endif
        {}

        ~RequestBuilder() {
        }

    protected:
        RequestBuilder& Prepare(RequestType rt, const std::string& url) {
            assert(request_type_ == RequestType::INVALID);
            assert(!is_built_);
            request_type_  = rt;
            url_ = url;
            return *this;
        }

    public:
        bool CanSendFile() const noexcept {
            return request_type_ == RequestType::POST
                    || request_type_ == RequestType::PUT;
        }


        /*! 
         * \name Type of request
         * 
         * Use one of these functions to declare what HTTP request you want to use.
         * 
         * \param url Url to call. Must be a complete URL, starting with 
         *      "http://" or "https://" 
         * 
         * Note that you must use only only one of these methods in one request.
         */
        //@{
         
        /*! Use a HTTP GET request */
        RequestBuilder& Get(const std::string& url) {
            return Prepare(RequestType::GET, url);
        }

        /*! Use a HTTP HEAD request */
        RequestBuilder& Head(const std::string& url) {
            return Prepare(RequestType::HEAD, url);
        }

        /*! Use a HTTP POST request */
        RequestBuilder& Post(const std::string& url) {
            return Prepare(RequestType::POST, url);
        }

        /*! Use a HTTP POST request */
        RequestBuilder& PostMime(const std::string& url) {
            return Prepare(RequestType::POST_MIME, url);
        }

        /*! Use a HTTP PUT request */
        RequestBuilder& Put(const std::string& url) {
            return Prepare(RequestType::PUT, url);
        }

        /*! Use a HTTP PATCH request */
        RequestBuilder& Patch(const std::string& url) {
            return Prepare(RequestType::PATCH, url);
        }

        /*! Use a HTTP DELETE request */
        RequestBuilder& Delete(const std::string& url) {
            return Prepare(RequestType::DELETE, url);
        }

        /*! Use a HTTP OPTIONS request */
        RequestBuilder& Options(const std::string& url) {
            return Prepare(RequestType::OPTIONS, url);
        }
        //@}

        /*! Specify a HTTP header for the request.
         * 
         * \param value The value of the header-line, properly formatted according to the relevant HTTP specifications.
         */
        RequestBuilder& Header(const char *value) {
            assert(value);
            assert(!is_built_);
            request_->GetHeaders() = curl_slist_append(request_->GetHeaders(), value);
            return *this;
        }

         /*! Specify a HTTP header for the request.
         * 
         * \param name Name of the header
         * \param value The value of the header
         * 
         * This is a convenience method that will build the appropriate header for you.
         */
        RequestBuilder& Header(const std::string& name,
                               const std::string& value) {
            const auto v = name + ": " + value;
            return Header(v.c_str());
        }

        /*! Sets the content-type to "Application/json; charset=utf-8" */
        RequestBuilder& WithJson() {
            return Header("Content-type: Application/json; charset=utf-8");
        }
        
        /*! Sets the content-type to "Application/json; charset=utf-8"
         * 
         * \param body Json payload to send with the request. 
        */
        RequestBuilder& WithJson(std::string body) {
            WithJson();
            return SendData(std::move(body));
        }

        /*! Sets the accept header to "Application/json" */
        RequestBuilder& AcceptJson() {
            return Header("Accept: Application/json");
        }

        /*! Sets a Curl options.
         * 
         * \param opt CURLoption enum specifying the option
         * \param value Value to set. 
         * 
         * It is critical that the type of the value is of the same type that
         * libcurl is expecting for the option. RESTinCurl makes no attempt
         * to validate or cast the values.
         * 
         * Please refer to the libcurl documentation for curl_easy_setopt() 
         */
        template <typename T>
        RequestBuilder& Option(const CURLoption& opt, const T& value) {
            assert(!is_built_);
            options_->Set(opt, value);
            return *this;
        }

        /*! Enables or disables trace logging for requests. 
         * 
         * The trace logging will show detailed information about what
         * libcurl does and data sent and received during a request. 
         * 
         * Basically it sets `CURLOPT_DEBUGFUNCTION` and `CURLOPT_VERBOSE`.
         */
        RequestBuilder& Trace(bool enable = true) {
            if (enable) {
                Option(CURLOPT_DEBUGFUNCTION, debug_callback);
                Option(CURLOPT_VERBOSE, 1L);
            } else {
                Option(CURLOPT_VERBOSE, 0L);
            }
            
            return *this;
        }

        /*! Set request timeout 
         * 
         * \param timeout Timeout in milliseconds. Set to -1 to use the default.
         */
        RequestBuilder& RequestTimeout(const long timeout) {
            request_timeout_ = timeout;
            return *this;
        }

        /*! Set the connect timeout for a request
         * 
         * \param timeout Timeout in milliseconds. Set to -1 to use the default.
         */
        RequestBuilder& ConnectTimeout(const long timeout) {
            connect_timeout_ = timeout;
            return *this;
        }

        /*! Send a file
         *
         *  \param path Full path to the file to send.
         *
         *  \throws SystemException if the file cannot be opened.
         *  \throws Exception if the method is called for a non-send operation
         */
        RequestBuilder& SendFile(const std::string& path) {
            assert(!is_built_);
            assert(CanSendFile());
            if (!CanSendFile()) {
                throw Exception{"Invalid curl operation for a file upload"};
            }

            assert(request_);
            request_->OpenSourceFile(path);
            struct stat st = {};
            if(fstat(fileno(request_->GetSourceFp()), &st) != 0) {
                const auto e = errno;
                throw SystemException{std::string{"Unable to stat file "} + path, e};
            }

            // set where to read from (on Windows you need to use READFUNCTION too)
            options_->Set(CURLOPT_READDATA, request_->GetSourceFp());
            options_->Set(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(st.st_size));
            have_data_out_ = true;
            return *this;
        }

        /*! Send a file as a multipart/form mime segment
         *
         *  \param path Full path to the file tro send
         *  \param name Otional name to use for the file in the mime segment
         *  \param remoteName Optional name to label the file as
         *         for the remote end
         *  \param mimeType Optional mime-type for the file
         *
         *  \throws Exception if the method is called for a
         *          non-mime-post operation
         */
        RequestBuilder& SendFileAsMimeData(const std::string& path,
                               const std::string& name = {},
                               const std::string& remoteName = {},
                               const std::string& mimeType = {}) {
            assert(request_);
            assert(request_type_ == RequestType::POST_MIME);
            if (request_type_ != RequestType::POST_MIME) {
                throw Exception{"Must use PostMime operation to add mime attachments"};
            }
            request_->AddFileAsMimeData(path, name, remoteName, mimeType);
            return *this;
        }

        /*! Send a file
         *
         *  \param path Full path to the file to send.
         *
         *  \throws SystemException if the file cannot be opened.
         *  \throws Exception if the method is called for a non-send operation
         */
        RequestBuilder& SendFileAsForm(const std::string& path) {
            assert(!is_built_);
            assert(CanSendFile());
            if (!CanSendFile()) {
                throw Exception{"Invalid curl operation for a file upload"};
            }

            assert(request_);
            request_->OpenSourceFile(path);
            struct stat st = {};
            if(fstat(fileno(request_->GetSourceFp()), &st) != 0) {
                const auto e = errno;
                throw SystemException{std::string{"Unable to stat file "} + path, e};
            }

            // set where to read from (on Windows you need to use READFUNCTION too)
            options_->Set(CURLOPT_READDATA, request_->GetSourceFp());
            options_->Set(CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(st.st_size));
            have_data_out_ = true;
            return *this;
        }

        /*! Specify Data Handler for outbound data
         * 
         * You can use this method when you need to use a Data Handler, rather than a simple string,
         * to provide the data for a POST, PUT etc. request.
         * 
         * \param dh Data Handler instance.
         * 
         * Note that the Data Handler is passed by reference. It is your responsibility that the 
         * instance is present at least until the request has finished (your code owns the Data Handler instance).
         */
        template <typename T>
        RequestBuilder& SendData(OutDataHandler<T>& dh) {
            assert(!is_built_);
            options_->Set(CURLOPT_READFUNCTION, dh.read_callback);
            options_->Set(CURLOPT_READDATA, &dh);
            have_data_out_ = true;
            return *this;
        }

        /*! Convenience method to specify a object that contains the data to send during a request.
         * 
         * \param data Data to send. Typically this will be a std::string, std::vector<char> or a similar
         *      object. 
         * 
         * RESTinCurl takes ownership of this data (by moving it).
         */
        template <typename T>
        RequestBuilder& SendData(T data) {
            assert(!is_built_);
            auto handler = std::make_unique<OutDataHandler<T>>(std::move(data));
            auto& handler_ref = *handler;
            request_->SetDefaultOutHandler(std::move(handler));
            return SendData(handler_ref);
        }

        /*! Specify Data Handler for inbound data
         * 
         * You can use this method when you need to use a Data Handler, rather than a simple string,
         * to receive data during the request.
         * 
         * \param dh Data Handler instance.
         * 
         * Note that the Data Handler is passed by reference. It is your responsibility that the 
         * instance is present at least until the request has finished (your code owns the Data Handler instance).
         */
        template <typename T>
        RequestBuilder& StoreData(InDataHandler<T>& dh) {
            assert(!is_built_);
            options_->Set(CURLOPT_WRITEFUNCTION, dh.write_callback);
            options_->Set(CURLOPT_WRITEDATA, &dh);
            have_data_in_ = true;
            return *this;
        }

        /*! Convenience method to specify a object that receives incoming data during a request.
         * 
         * \param data Buffer to hold incoming data. Typically this will be a std::string, 
         *      std::vector<char> or a similar object. 
         * 
         * Note that data is passed by reference. It is your responsibility that the 
         * instance is present at least until the request has finished (your code owns the object).
         */
        template <typename T>
        RequestBuilder& StoreData(T& data) {
            assert(!is_built_);
            auto handler = std::make_unique<InDataHandler<T>>(data);
            auto& handler_ref = *handler;
            request_->SetDefaultInHandler(std::move(handler));
            return StoreData(handler_ref);
        }

        /*! Do not process incoming data
         *
         * The response body will be read from the network, but
         * not buffered and not available for later
         * inspection.
         */
        RequestBuilder& IgnoreIncomingData() {
            options_->Set(CURLOPT_WRITEFUNCTION, write_callback);
            have_data_in_ = true;
            return *this;
        }

        /*! Specify a callback that will be called when the request is complete (or failed).
         * 
         * \param fn Callback to be called
         * 
         * For asynchronous requests, the callback will be called from the worker-thread shared
         * by all requests and timers for the client instance. It is imperative that you return
         * immediately, and don't keep the thread busy more than strictly required. If you need
         * do do some computing or IO in response to the information you receive, you should
         * do that in another thread.
         */
        RequestBuilder& WithCompletion(completion_fn_t fn) {
            assert(!is_built_);
            completion_ = std::move(fn);
            return *this;
        }

        /*! HTTP Basic Authentication
         *
         * Authenticate the request with HTTP Basic Authentication.
         *
         * \param name Name to authenticate with
         * \param passwd Password to authenticate with
         *
         * Note that if name or password is empty, authentication is
         * ignored. This makes it simple to add optional authentication
         * to your project, by simply assigning values to the strings
         * you pass here, or not.
         */
        RequestBuilder& BasicAuthentication(const std::string& name,
                                            const std::string& passwd) {
            assert(!is_built_);

            if (!name.empty() && !passwd.empty()) {
                options_->Set(CURLOPT_USERNAME, name.c_str());
                options_->Set(CURLOPT_PASSWORD, passwd.c_str());
            }

            return *this;
        }

        /*! Set a Curl compatible read handler. 
         * 
         * \param handler Curl C API read handler
         * 
         * You probably don't need to call this directly.
         */
        RequestBuilder& SetReadHandler(size_t (*handler)(char *, size_t , size_t , void *), void *userdata) {
            options_->Set(CURLOPT_READFUNCTION, handler);
            options_->Set(CURLOPT_READDATA, userdata);
            have_data_out_ = true;
            return *this;
        }

        /*! Set a Curl compatible write handler. 
         * 
         * \param handler Curl C API write handler
         * 
         * You probably don't need to call this directly.
         */
        RequestBuilder& SetWriteHandler(size_t (*handler)(char *, size_t , size_t , void *), void *userdata) {
            options_->Set(CURLOPT_WRITEFUNCTION,handler);
            options_->Set(CURLOPT_WRITEDATA, userdata);
            have_data_in_ = true;
            return *this;
        }

        void Build() {
            if (!is_built_) {
                // Set up Data Handlers
                if (!have_data_in_) {
                    // Use a default std::string. We expect json anyway...
                    this->StoreData(request_->getDefaultInBuffer());
                }

                if (have_data_out_) {
                    options_->Set(CURLOPT_UPLOAD, 1L);
                }

                if (request_timeout_ >= 0) {
                    options_->Set(CURLOPT_TIMEOUT_MS, request_timeout_);
                }

                if (connect_timeout_ >= 0) {
                    options_->Set(CURLOPT_CONNECTTIMEOUT_MS, connect_timeout_);
                }

                // Set headers
                if (request_->GetHeaders()) {
                    options_->Set(CURLOPT_HTTPHEADER, request_->GetHeaders());
                }

                // TODO: Prepare the final url (we want nice, correctly encoded request arguments)
                options_->Set(CURLOPT_URL, url_);
                RESTINCURL_LOG("Preparing connect to: " << url_);

                // Prepare request
                request_->Prepare(request_type_, std::move(completion_));
                is_built_ = true;
            }
        }

        /*! Execute the request synchronously
         * 
         * This will execute the request and call the callback (if you declared one) in the
         * current thread before the method returns. 
         * 
         * \throws restincurl::Exception derived exceptions on error
         * 
         * This method is available even when `RESTINCURL_ENABLE_ASYNC` is enabled ( != 0).
         */
        void ExecuteSynchronous() {
            Build();
            request_->Execute();
        }

#if RESTINCURL_ENABLE_ASYNC

         /*! Execute the request asynchronously
         * 
         * This will queue the request for processing. If the number of active requests are
         * less than `RESTINCURL_MAX_CONNECTIONS`, the request will start executing almost immediately. 
         * 
         * The method returns immediately.
         * 
         * \throws restincurl::Exception derived exceptions on error
         * 
         * This method is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
         */
        void Execute() {
            Build();
            worker_.Enqueue(std::move(request_));
        }
#endif

    private:
        std::unique_ptr<Request> request_;
        std::unique_ptr<class Options> options_;
        std::string url_;
        RequestType request_type_ = RequestType::INVALID;
        bool have_data_in_ = false;
        bool have_data_out_ = false;
        bool is_built_ = false;
        completion_fn_t completion_;
        long request_timeout_ = 10000L; // 10 seconds
        long connect_timeout_ = 3000L; // 1 second
#if RESTINCURL_ENABLE_ASYNC
        Worker& worker_;
#endif
    };

    /*! The high level abstraction of the Curl library.
     * 
     * An instance of a Client will, if asynchronous mode is enabled, create a
     * worker-thread when the first request is made. This single worker-thread
     * is then shared between all requests made for this client. When no requests
     * are active, the thread will wait for a while (see RESTINCURL_IDLE_TIMEOUT_SEC), 
     * keeping libcurl's connection-cache warm, to serve new requests as efficiently as 
     * possible. When the idle time period expires, the thread will terminate and 
     * close the connection-cache associated with the client. 
     * If a new request is made later on, a new worker-thread will be created.
     */
    class Client {

    public:
        /*! Constructor
         * 
         * \param init Set to true if you need to initialize libcurl.
         * 
         * Libcurl require us to call an initialization method once, before using the library.
         * If you use libcurl exclusively from RESTinCurl, `init` needs to be `true` (default). If
         * you use RESTinCurl in a project that already initialize libcurl, you should pass `false`
         * to the constructor.
         * 
         * RESTinCurl will only call libcurl's initialization once, no matter how many times you 
         * call the constructor. It's therefore safe to always use the default value when you want RESTinCurl
         * to deal with libcurl's initialization.
         */
        Client(const bool init = true) {
            if (init) {
                static std::once_flag flag;
                std::call_once(flag, [] {
                    RESTINCURL_LOG("One time initialization of curl.");
                    curl_global_init(CURL_GLOBAL_DEFAULT);
                });
            }
        }

        /*! Destructor
         * 
         * The destructor will try to clean up resources (when in asynchronous mode)
         * ant may wait for a little while for IO operations to stop and the worker-thread to 
         * finish. 
         * 
         * This is to prevent your application for crashing if you exit the main thread while 
         * the worker-thread is still working and accessing memory own by the Client instance.
         */
        virtual ~Client() {
#if RESTINCURL_ENABLE_ASYNC
            if (worker_) {
                try {
                    worker_->Close();
                } catch (const std::exception& ex) {
                    RESTINCURL_LOG("~Client(): " << ex.what());
                }
            }
#endif
        }

        /*! Build a request
         * 
         * Requests are "built" using a series of statements
         * to fully express what you want to do and how you want RESTinCurl to do it.
         * 
         * Example
         * \code
                restincurl::Client client;
                client.Build()->Get("https://api.example.com")
                    .AcceptJson()
                    .Header("X-Client", "restincurl")
                    .WithCompletion([&](const Result& result) {
                        // Do something
                    })
                    .Execute();
           \endcode
         */
        std::unique_ptr<RequestBuilder> Build() {
            return std::make_unique<RequestBuilder>(
#if RESTINCURL_ENABLE_ASYNC
                *worker_
#endif
            );
        }

#if RESTINCURL_ENABLE_ASYNC
        /*! Shut down the event-loop and clean up internal resources when all active and queued requests are done.
         * 
         * This method is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
         */
        void CloseWhenFinished() {
            worker_->CloseWhenFinished();
        }

        /*! Close the client.
         * 
         * This method aborts any and all requests. 
         * 
         * The worked-thread will exit shortly after this method is
         * called, if it was running.
         * 
         * This method is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
         */
        void Close() {
            worker_->Close();
        }

        /*! Wait for the worker-thread to finish.
         * 
         * You should call Close() first.
         * 
         * This method is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
         */
        void WaitForFinish() {
            worker_->Join();
        }

        /*! Check if the client instance has a worker-thread.
         * 
         * This method is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
         */
        bool HaveWorker() const {
            return worker_->HaveThread();
        }

        /*! Get the number of active / ongoing requests.
         * 
         * This method is only available when `RESTINCURL_ENABLE_ASYNC` is nonzero.
         */
        size_t GetNumActiveRequests() {
            return worker_->GetNumActiveRequests();
        }
#endif

    private:
#if RESTINCURL_ENABLE_ASYNC
        std::unique_ptr<Worker> worker_ = std::make_unique<Worker>();
#endif
    };


} // namespace
