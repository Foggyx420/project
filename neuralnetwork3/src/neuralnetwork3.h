
#ifndef NEURALNETWORK3_H
#define NEURALNETWORK3_H
// This file is to not be included anywhere else but the main neuralnetwork3.cpp file!

// c++/boost/external include files
#include <string>
#include <cmath>
#include <zlib.h>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <curl/curl.h>
#include <inttypes.h>
#include <experimental/filesystem>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

/*************************
* Network Node Namespace *
*************************/

namespace fs = std::experimental::filesystem;
namespace boostpt = boost::property_tree;
namespace boostio = boost::iostreams;

// Dummy mock stuff
void startnn();

/*********************
* Network Node ENUMS *
*********************/

enum logattribute {
    NN_DEBUG,
    NN_INFO,
    NN_WARNING,
    NN_ERROR
};

/***************
* Network Vars *
***************/

bool fDebug5 = true;
std::string nndbfile = "NetworkNode/nn.db";

/*************************
* Network Node Functions *
*************************/

void _log(logattribute eType, const std::string& sCall, const std::string& sMessage);

/********************
* Network Node Data *
********************/

class nndata
{
public:
    // Neural Node
/*    bool IsNeuralNodeDownloader();

    // Database related
    bool DownloadProjectFiles();
    bool ProcessProjectData();
    bool ImportStatsData(const std::string& project);
*/
    bool exists_prjfile(const std::string& projectfile)
    {
        try
        {
            std::string prjfile = projectfile + ".gz";
            fs::path check_file = "NetworkNode";
            check_file /= prjfile;

            return fs::exists(check_file) ? true : false;
        }

        catch (std::exception& ex)
        {
            printf("NN : exists_prjfile : Exception occured verifying existence of project file (%s)\n", ex.what());

            return false;
        }
    }

    bool destroy_db()
    {
        try
        {
            fs::path check_file = "NodeNetwork";
            check_file /= "nn.db";
            return fs::remove(check_file) ? true : false;
        }

        catch (std::exception& ex)
        {
            printf("NN : destroy DB : Exception occured removing database (%s)\n", ex.what());

            return false;
        }
    }

    bool drop_db(const std::string& type, const std::string& table)
    {
        if (type.empty() || table.empty())
        {
            printf("NN : drop DB : Arugements must all have value\n");

            return false;
        }
    }
};

/********************
* Network Node Curl *
********************/

class nncurl
{
private:

    CURL* curl;
    std::string buffer;
    std::string header;
    long http_code;
    CURLcode res;

    static size_t writeheader(void* ptr, size_t size, size_t nmemb, void* userp)
    {
        ((std::string*)userp)->append((char*)ptr, size * nmemb);
        return size * nmemb;
    }

    static size_t writetofile(void* ptr, size_t size, size_t nmemb, FILE* fp)
    {
        return fwrite(ptr, size, nmemb, fp);
    }

public:

    nncurl()
        : curl(curl_easy_init())
    {
    }

    ~nncurl()
    {
        if (curl)
            curl_easy_cleanup(curl);
    }

    bool http_download(const std::string& url, const std::string& destination)
    {
        try {
            FILE* fp;

            fp = fopen(destination.c_str(), "wb");

            if(!fp)
            {
                _log(NN_ERROR, "nncurl_http_download", "Failed to open project file to download project data into <destination=" + destination + ">");

                return false;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writetofile);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            curl_easy_setopt(curl, CURLOPT_PROXY, "");
            res = curl_easy_perform(curl);
            fclose(fp);

            if (res > 0)
            {
                if (fp)
                    fclose(fp);

                _log(NN_ERROR, "nncurl_http_download", "Failed to download project file <urlfile=" + url + "> (" + curl_easy_strerror(res) + ")");

                return false;
            }

            return true;

        }

        catch (std::exception& ex)
        {
            _log(NN_ERROR, "nncurl_http_download", "Std exception occured (" + std::string(ex.what()) + ")");

            return false;
        }
    }

    bool http_header(const std::string& url, std::string& etag)
    {
        struct curl_slist* headers = NULL;

        headers = curl_slist_append(headers, "Accept: */*");
        headers = curl_slist_append(headers, "User-Agent: Header-Reader");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeheader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_PROXY, "");
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        res = curl_easy_perform(curl);

        if (res > 0)
        {
            _log(NN_ERROR, "nncurl_http_header", "Failed to capture header of project file <urlfile=" + url + ">");

            return false;
        }

        std::istringstream ssinput(header);
        for (std::string line; std::getline(ssinput, line);)
        {
            if (line.find("ETag: ") != std::string::npos)
            {
                size_t pos;

                std::string modstring = line;

                pos = modstring.find("ETag: ");

                etag = modstring.substr(pos+6, modstring.size());

                etag = etag.substr(1, etag.size() - 3);
            }
        }

        if (fDebug5)
        {
            _log(NN_INFO, "nncurl_http_header", "Captured ETag for project url <urlfile=" + url + ", ETag=" + etag + ">");
        }

        return true;
    }
};

/************************
* Network Node Database *
************************/

class nndb
{
private:

    sqlite3* db = nullptr;
    int res = 0;
    sqlite3_stmt* stmt = nullptr;

public:

    nndb()
    {
        res = sqlite3_open_v2(nndbfile.c_str(), &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, nullptr);

        if (res)
            _log(NN_ERROR, "nndb", "sqlite error occured while opening database (" + std::string(sqlite3_errstr(res)) + ")");
    }

    ~nndb()
    {
        res = sqlite3_close_v2(db);

        if (res)
            _log(NN_ERROR, "nndb", "sqlite error occured while closing database (" + std::string(sqlite3_errstr(res)) + ")");
    }

    bool insert_query(bool table, const std::string& querystring)
    {
        sqlite3_prepare_v2(db, querystring.c_str(), querystring.size(), &stmt, NULL);

        res = sqlite3_step(stmt);

        if (!table && res != SQLITE_DONE)
        {
            sqlite3_finalize(stmt);

            return false;
        }

        sqlite3_finalize(stmt);

        return true;
    }

    bool search_query(int typequery, const std::string& searchquery, std::string& valuea, std::string& valueb)
    {
        /*
         * Type:
         * 1 = project          : project data (CPID -> TTL CREDIT, TDC)
         *   = control          : control calls of old project data (CPID -> TTL CREDIT, TDC)
         *   = contract         : contract releated data (CPID -> TDC, MAG)
         * 2 = project files    : project etag storage (PROJECT -> ETAG)
         *   = whitelist        : whitelist data
         *   = beacons          : beacons list
         */

        sqlite3_prepare_v2(db, searchquery.c_str(), searchquery.size(), &stmt, NULL);

        try
        {
            if (sqlite3_step(stmt) == SQLITE_ROW)
            {
                if (typequery == 1)
                {
                    valuea = (const char*)sqlite3_column_text(stmt, 0);
                    valueb = (const char*)sqlite3_column_text(stmt, 1);
                }

                if (typequery == 2)
                    valuea = (const char*)sqlite3_column_text(stmt, 0);

                sqlite3_finalize(stmt);

                return true;
            }
        }

        catch (std::bad_cast &ex)
        {
            _log(NN_DEBUG, "nndb_search_query", "Cast exception for <type=" + std::to_string(typequery) + ", query=" + searchquery + "> (" + ex.what() + ")");
        }

        sqlite3_finalize(stmt);

        return false;
    }

    bool drop_query(const std::string table)
    {
        std::string dropquery = "DROP TABLE IF EXISTS '" + table + "';";

        sqlite3_prepare_v2(db, dropquery.c_str(), dropquery.size(), &stmt, NULL);

        sqlite3_step(stmt);

        return true;
    }

};

/**********************
* Network Node Logger *
**********************/

class nnlogger
{
private:

    std::ofstream logfile;

public:

    nnlogger()
    {
        fs::path plogfile = fs::current_path();
        plogfile /= "NetworkNode";

        if (!fs::exists(plogfile))
            fs::create_directory(plogfile);

        plogfile /= "nn.log";

        logfile.open(plogfile.c_str(), std::ios_base::out | std::ios_base::app);

        if (!logfile.is_open())
            printf("NN : Logging : Failed to open logging file");
    }

    ~nnlogger()
    {
        if (logfile.is_open())
        {
            logfile.flush();
            logfile.close();
        }
    }

    void output(const std::string& tofile)
    {
        if (logfile.is_open())
            logfile << tofile << std::endl;

        return;
    }
};

#endif // NEURALNETWORK3_H
