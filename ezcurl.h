#ifndef WDONG_EZCURL
#define WDONG_EZCURL

#include <curl/curl.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <boost/assert.hpp>

namespace ezcurl {
    using std::string;
    using std::runtime_error;
    using std::istringstream;
    using std::ostringstream;
    using std::unordered_map;

    class Curl {
        static size_t read_callback (void *ptr, size_t size, size_t count, void *stream) {
            istringstream *ss = reinterpret_cast<istringstream *>(stream);
            size_t total = count * size;
            ss->read(reinterpret_cast<char *>(ptr), total);
            return ss->gcount();
        }

        static size_t write_callback (void *ptr, size_t size, size_t count, void *stream) {
            ostringstream *ss = reinterpret_cast<ostringstream *>(stream);
            size_t total = count * size;
            auto before = ss->tellp();
            ss->write(reinterpret_cast<char *>(ptr), total);
            return ss->tellp() - before;
        }

        static size_t header_callback (void *ptr, size_t size, size_t count, void *stream) {
            size_t sz = size * count;
            // if glog 
#ifdef _LOGGING_H_
            LOG(INFO) << string((char *)ptr, sz);
#endif
            return sz;
        }

        CURL *curl;
    public:

        Curl (): curl(curl_easy_init()) {
            BOOST_VERIFY(curl);
            curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
        }

        ~Curl () {
            BOOST_VERIFY(curl);
            curl_easy_cleanup(curl);
        }

        void post (string const &url, string const &input, string *blob, struct curl_slist *headers = nullptr) {
            ostringstream oss;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &oss);
            //curl_easy_setopt(curl, CURLOPT_POST, 1); // implied
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, input.c_str());
            if (headers) {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                throw runtime_error("curl:" + string(curl_easy_strerror(res)));
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
            curl_easy_setopt(curl, CURLOPT_POST, 0);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, nullptr);
            *blob = oss.str();
        }

        void get (string const &url, string *blob, struct curl_slist *headers = nullptr) {
            ostringstream oss;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            if (headers) {
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
            }
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &oss);
            CURLcode res = curl_easy_perform(curl);
            if (res != CURLE_OK) {
                throw runtime_error("curl:" + string(curl_easy_strerror(res)));
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, 0);
            *blob = oss.str();
        }

#if 0
        void post (string const &url, string const &input, Json *json, struct curl_slist *headers = nullptr) {
            string rep;
            //LOG(INFO) << "POST URL: " << url << " INPUT: " << input;
            post(url, input, &rep, headers);
            string error;
            *json = Json::parse(rep, error);
        }

        void get (string const &url, Json *json, struct curl_slist *headers = nullptr) {
            string rep;
            get(url, &rep, headers);
            string error;
            *json = Json::parse(rep, error);
        }
#endif

        class Headers {
            struct curl_slist *slist;
        public:
            Headers (): slist(nullptr) {
            }
            ~Headers () {
                if (slist) {
                    curl_slist_free_all(slist);
                }
            }
            Headers &add (string const &str) {
                slist = curl_slist_append(slist, str.c_str());
                return *this;
            }
            struct curl_slist *get () {
                return slist;
            }
        };

        class Params {
            unordered_map<string, string> params;
        public:
            void add (string const&name, string const&value) {
                params[name] = value;
            }

            void add (string const &name, int value) {
                params[name] = lexical_cast<string>(value);
            }

            string encode () const {
                ostringstream ss;
                bool first = true;
                for (auto const &p: params) {
                    if (first) {
                        first = false;
                    }
                    else {
                        ss.put('&');
                    }
                    urlencode(ss, p.first);
                    ss.put('=');
                    urlencode(ss, p.second);
                }
                return ss.str();
            }
        };

    };
}

#endif
