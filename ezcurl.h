#ifndef WDONG_EZCURL
#define WDONG_EZCURL

#include <curl/curl.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <boost/assert.hpp>

namespace ezcurl {
    using std::string;
    using std::runtime_error;
    using std::ostream;
    using std::istringstream;
    using std::ostringstream;
    using std::unordered_map;
    using boost::lexical_cast;

    // call this in multi-thread app before anything else 
    // for single threaded app, no need to call this
    void GlobalInit () {
        curl_global_init(CURL_GLOBAL_ALL);
    }


    void urlencode (ostream &ss, string const &s) {
        static const char lookup[]= "0123456789abcdef";
        for (char c: s) {
            if ( (48 <= c && c <= 57) ||//0-9
                 (65 <= c && c <= 90) ||//abc...xyz
                 (97 <= c && c <= 122) || //ABC...XYZ
                 (c=='-' || c=='_' || c=='.' || c=='~')
            ) {
                ss.put(c);
            }
            else
            {
                ss.put('%');
                ss.put(lookup[(c&0xF0)>>4]);
                ss.put(lookup[(c&0x0F)]);
            }
        }
    }   

    struct Form {
        struct curl_httppost *formpost;
        struct curl_httppost *lastptr;

        Form (): formpost(nullptr), lastptr(nullptr) {
        }

        ~Form () {
            curl_formfree(formpost);
        }

        // these values must not be release during the life spam of form
#ifndef EZCURL_COMPAT
        void add (string const &name, string const *value) {
            CURLFORMcode res = curl_formadd(&formpost, &lastptr,
                         CURLFORM_COPYNAME, name.c_str(),
                         CURLFORM_PTRCONTENTS, &(*value)[0],
                         CURLFORM_CONTENTLEN, curl_off_t(value->size()),
                         CURLFORM_END);
            if (res != 0) {
                throw runtime_error("curl form: " + boost::lexical_cast<string>(res));
            }
        }
#endif

        void add_file (string const &name, string const &file) {
            CURLFORMcode res = curl_formadd(&formpost, &lastptr,
                         CURLFORM_COPYNAME, name.c_str(),
                         CURLFORM_FILE, file.c_str(),
                         CURLFORM_END);
            if (res != CURLE_OK) {
                throw runtime_error("curl form: " + boost::lexical_cast<string>(res));
            }
        }

        friend class CURL;

    };


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

        void post (string const &url, Form const &form, string *blob, struct curl_slist *headers = nullptr) {
            ostringstream oss;
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &oss);
            //curl_easy_setopt(curl, CURLOPT_POST, 1); // implied
            curl_easy_setopt(curl, CURLOPT_HTTPPOST, form.formpost);
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
