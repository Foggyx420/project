
#ifndef NEURALNETWORK3_H
#define NEURALNETWORK3_H

// c++/boost/external include files
#include <string>
#include <fstream>
#include <iostream>
#include <sqlite3.h>
#include <curl/curl.h>
#include <inttypes.h>
#include <experimental/filesystem>
#include <boost/filesystem.hpp>
#include <boost/exception/exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
// TODO move to std experimental as it has been pushed in c++ 17
// Namespace for classes
namespace fs = std::experimental::filesystem;
namespace boostfs = boost::filesystem;
using namespace std;
using namespace boost;

static std::string nndbfile = "NeuralNetwork/nn.db";
static bool fDebug5 = true;

void startnn();

class nndata
{
public:
    // Dummy data
    void setdummy();

    // Neural Node
    bool IsNeuralNode();

    // Database related
    bool DownloadProjectFiles();
    bool ProcessProjectData();
    bool ImportStatsData(const std::string& project);
private:
};

class nncurl
{
private:
    CURL* curl;
    std::string buffer;
    long http_code;

    static size_t writeheader(void* ptr, size_t size, size_t nmemb, void* userp)
    {
        ((string*)userp)->append((char*)ptr, size * nmemb);
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
        CURLcode res;

        try {
            boostfs::path destpath = boostfs::current_path() / "NeuralNetwork";

            if (!boostfs::exists(destpath))
                boostfs::create_directory(destpath);

            destpath /= destination;

            // Incase project server down lets not destroy existing project data
            FILE* fp = fopen("temp", "wb");

            if(!fp)
            {
                if (fDebug5)
                    printf("NN : curl : Failed to open temporary file to download project data; Check file permissions\n");

                return false;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writetofile);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
            res = curl_easy_perform(curl);
            fclose(fp);

            if (res > 0)
            {
                if (fDebug5)
                    printf("NN : curl : http_download error: %s\n", curl_easy_strerror(res));

                return false;
            }

            boostfs::copy_file("temp", destpath, boostfs::copy_option::overwrite_if_exists);
            boostfs::remove("temp");

            return true;

        }

        catch (const boost::exception& ex)
        {
            printf("NN : Boost exception %s\n", boost::diagnostic_information(ex).c_str());

            return false;
        }
    }

    bool http_header(const std::string& url, std::string& header)
    {
        CURLcode res;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeheader);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buffer);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        res = curl_easy_perform(curl);

        if (res > 0)
        {
            if (fDebug5)
                printf("NN : curl : http_header error (%s)\n", curl_easy_strerror(res));

            return false;
        }

        header = buffer;

        return true;
    }
};

class nndb
{
public:
    nndb()
    {
        res = sqlite3_open(nndbfile.c_str(), &db);

        if (res)
        {
            printf("NN : DB : sqlite error (Failed to open database file)\n");
        }
    }

    ~nndb()
    {
        sqlite3_close(db);
    }

    bool dbexists()
    {
        try
        {
            fs::path check_file = nndbfile;

            return fs::exists(check_file) ? true : false;
        }

        catch (std::exception& ex)
        {
            printf("NN : exists DB : Exception occured while verifying database exists (%s)\n", ex.what());

            return false;
        }
    }

    bool destroy_db()
    {
        try
        {
            fs::path check_file = nndbfile;
            return fs::remove(check_file) ? true : false;
        }

        catch (std::exception& ex)
        {
            printf("NN : destroy DB : Exception occured removing database (%s)\n", ex.what());

            return false;
        }

    }

    bool insert_db(const std::string& table, const std::string& key, const std::string& value)
    {
        // No empty fields allowed
        if (table.empty() || key.empty() || value.empty())
        {
            printf("NN : insertDB : Arugements must all have value\n");

            return false;
        }

        // Create the table if it does not yet exist (meaning new database)
        std::string createquery = "CREATE TABLE IF NOT EXISTS " + table + " (key TEXT PRIMARY KEY NOT NULL, value TEXT NOT NULL);";
        sqlite3_stmt* createstmt;

        sqlite3_prepare(db, createquery.c_str(), createquery.size(), &createstmt, NULL);

        if (sqlite3_step(createstmt) != SQLITE_DONE)
        {
            if (fDebug5)
                printf("NN : insertDB : sqlite error create table <table=%s, key=%s, value=%s> (%s)\n", table.c_str(), key.c_str(), value.c_str(), sqlite3_errmsg(db));

            return false;
        }

        // Inject data into the database by table -> key (primary key of database) -> value
        std::string insertquery = "INSERT OR REPLACE INTO " + table + " VALUES('" + key + "', '" + value + "');";
        sqlite3_stmt* insertstmt;

        sqlite3_prepare(db, insertquery.c_str(), insertquery.size(), &insertstmt, NULL);

        if (sqlite3_step(insertstmt) != SQLITE_DONE)
        {
            if (fDebug5)
                printf("NN : insertDB : sqlite error injecting to table <table=%s, key=%s, value=%s> (%s)\n", table.c_str(), key.c_str(), value.c_str(), sqlite3_errmsg(db));

            return false;
        }

        return true;
    }

    bool delete_db(const std::string& table, const std::string& key)
    {
        // No empty fields allowed
        if (table.empty() || key.empty())
        {
            printf("NN : injectDB : Arugements must all have value\n");

            return false;
        }

        // Delete by key
        std::string deletequery = "DELETE FROM " + table + " WHERE key = '" + key + "';";
        sqlite3_stmt* deletestmt;

        sqlite3_prepare(db, deletequery.c_str(), deletequery.size(), &deletestmt, NULL);

        if (sqlite3_step(deletestmt) != SQLITE_DONE)
        {
            if (fDebug5)
                printf("NN : deleteDB : sqlite error deleting in table <table=%s, key=%s> (%s)\n", table.c_str(), key.c_str(), sqlite3_errmsg(db));

            return false;
        }

        return true;
    }

    bool search_db(const std::string& table, const std::string& key, std::string& value)
    {
        // No empty fields allowed
        if (table.empty() || key.empty())
        {
            printf("NN : searchDB : Arguemnts must all have value\n");

            return false;
        }

        std::string searchquery = "SELECT value FROM " + table + " WHERE key = '" + key + "';";
        sqlite3_stmt* searchstmt;

        sqlite3_prepare(db, searchquery.c_str(), searchquery.size(), &searchstmt, NULL);

        sqlite3_step(searchstmt);

        try
        {
            value = reinterpret_cast<const char*>(sqlite3_column_text(searchstmt, 0));
        }

        catch (std::bad_cast &ex)
        {
            if(fDebug5)
                printf("NN : search DB : base cast of value of reply from database (%s)\n", ex.what());
        }

        // Can't use SQLITE_DONE OR OK..
        if (value.empty())
        {
            if (fDebug5)
                printf("NN : search DB : value returned as empty in ket of table <table=%s, key=%s>\n", table.c_str(), key.c_str());

            return false;
        }
        return true;
    }

private:
    sqlite3* db;
    int res = 0;

};

#endif // NEURALNETWORK3_H
