/*
Neural Network 3.0 (c++ 11)
This is intended to run the application class in which Neural Network 3.0 is.
This code contains code from denravonska and myself.
*/

#include "neuralnetwork3.h"

// Do not include the header file in any other Gridcoin files
// Goal: header should only be accessible by neuralnetwork3.cpp (to be renamed)
//       will extern functions that other parts of gridcoin can access

/*************************
* Network Node Namespace *
*************************/

using nd = nndata;

/****************************
* Network Node Declarations *
****************************/

// Network Node Functions
// We only extern the functions allowed to be used by the gridcoin wallet
extern bool SyncData();
bool IsNetworkNodeDownloader();
bool SyncData();
bool GatherProjectHeader(bool participant);
bool DownloadProjectFile(const std::string& sPrjFile, const std::string& sETag);
void DeleteProjectFile(const std::string& sETag);

// Network Node Database Functions
bool SearchDatabase(int nTypeQuery, const std::string& sTable, const std::string& sKey, std::string& sValuea, std::string& sValueb);
bool InsertDatabase(int nTypeQuery, const std::string& sTable, const std::string& sKey, const std::string& sValuea, const std::string& sValueb = "");

// Dummy Network Conditions
// This area to be removed once tested and works and integrated into gridcoin
std::vector<std::pair<std::string, std::string>> vWhitelist;
std::vector<std::string> vCPIDs;

void nndata::setdummy()
{

    // Dummy CPIDs -- In live network conditions the cpids and whitelist are in cache
    vCPIDs.push_back("f636448e64b48e26aaf610a48a48bb91");
    vCPIDs.push_back("0b5ef259411ec18e8dac2be0b732fd23");
    vCPIDs.push_back("a92e1cf0903633d62283ea1f101a1af3");
    vCPIDs.push_back("af73cd19f79a0ade8e6ef695faa2c776");
    vCPIDs.push_back("55cd02be28521073d367f7ca38615682");
    vWhitelist.push_back(std::make_pair("enigma@home", "http://www.enigmaathome.net/@"));
    vWhitelist.push_back(std::make_pair("odlk1", "https://boinc.multi-pool.info/latinsquares/@"));
    vWhitelist.push_back(std::make_pair("sztaki", "http://szdg.lpds.sztaki.hu/szdg/@"));
    return;
}

int64_t mocktime()
{
    return time(NULL);
}


void startnn()
{
/*    nndata nn;


    nn.setdummy();
    int64_t nStart = mocktime();

    nn.DownloadProjectFiles();

    nn.ProcessProjectData();
    int64_t nElapsed = mocktime() - nStart;

    if (fDebug5)
        printf("NN : time : Took %" PRId64 " to complete\n", nElapsed);
*/
    _log(NN_DEBUG, "testcall", "yes this works");
    return;
}

// End mock tests

/**********************
* Network Node Logger *
**********************/

void _log(logattribute eType, const std::string& sCall, const std::string& sMessage)
{
    std::string sType;
    std::string sOut;

    try
    {
        switch (eType)
        {
            case (NN_DEBUG):     sType = "DEBUG";
            case (NN_INFO):      sType = "INFO";
            case (NN_WARNING):   sType = "WARNING";
            case (NN_ERROR):     sType = "ERROR";
        }
    }

    catch (std::exception& ex)
    {
        printf("NN : logger : exception occured in _log function (%s)\n", ex.what());

        return;
    }

    sOut = std::to_string(time(NULL)) + " [" + sType + "] <" + sCall + "> : " + sMessage;

    nnlogger log;

    log.output(sOut);

    return;
}

/*************************
* Network Node Functions *
*************************/

bool IsNetworkNodeDownloader()
{
    // Use GetArg to see if neuralnode equals true or false once in gridcoin code
    // Check whether they are a participant by grc address default address and day
    // If true this node downloads the project files and can be shared to other non
    // downloaders in the network. This method with neural network file share we can
    // all participate
    return true;
}

bool SyncData()
{
    if (IsNetworkNodeDownloader())
    {
        // Download Participant true; begin checks
        // Require 90% project success rate
        int nRequired = std::floor((double)vWhitelist.size() * 0.9);
        int nSuccess = 0;

        for (auto const& vWL : vWhitelist)
        {
            if (vWL.first.empty() || vWL.second.empty())
            {
                _log(NN_WARNING, "SyncData", "Entry in Whitelist vector is empty!");

                continue;
            }

            std::size_t pos = vWL.second.find("@");
            std::string sUrl = vWL.second.substr(0, pos) + "stats/";
            std::string sPrjFile = sUrl + "user.gz";
            std::string sETag;

            if (GatherProjectHeader(sPrjFile, sETag))
            {
                std::string sDBETag;
                std::string sDummy;

                if (SearchDatabase(2, "prjfiles", vWL.first, sDBETag, sDummy))
                {
                    if (sDBETag == sETag)
                    {
                        // No change
                        _log(NN_INFO, "SyncData", "Project file on server for " + vWL.first + " is unchanged; no need to download");

                        nSuccess++;

                        continue;
                    }
                }

                else
                    _log(NN_WARNING, "SyncData", "Project file for " + vWL.first + " not found in database (new?)");

                // Here we download project files
                _log(NN_INFO, "SyncData", "Project file on server for " + vWL.first + " is new");

                if (!sDBETag.empty())
                    DeleteProjectFile(sDBETag);

                if (DownloadProjectFile(sPrjFile, sETag))
                {
                    _log(NN_INFO, "SyncData", "Project file for " + vWL.first + " downloaded and stored as " + sETag + ".gz");

                    if (InsertDatabase(2, "prjfiles", vWL.first, sETag))
                    {
                        nSuccess++;

                        continue;
                    }

                    else
                    {
                        _log(NN_ERROR, "SyncData", "Failed to insert ETag for project " + vWL.first);

                        continue;
                    }
                }

                else
                {
                    _log(NN_ERROR, "SyncData", "Failed to download project " + vWL.first + "'s file");

                    continue;
                }
            }

            else
            {
                _log(NN_ERROR, "SyncData", "Failed to receive project header for " + vWL.first);

                continue;
            }
        }

        if (nSuccess < nRequired)
        {
            _log(NN_ERROR, "SyncData", "Failed to retrieve required amount of projects <Successcount=" + std::to_string(nSuccess) + ", Requiredcount=" + std::to_string(nRequired) + ", whitelistcount=" + std::to_string(vWhitelist.size()) + ">");

            return false;
        }

        else
        {
            // Call function to add/compare in database
            return true;
        }
    }

    else
    {
        // Not Download Participant; check for new ETags and request from network nodes
    }
}

bool GatherProjectHeader(const std::string& sPrjFile, std::string& sETag)
{
    nncurl prjheader;

    if (prjheader.http_header(sPrjFile, sETag))
    {
        _log(NN_INFO, "GatherProjectHeader", "Successfully received project file header <prjfile=" + sPrjFile + ", etag=" + sETag + ">");

        return true;
    }

    else
    {
        _log(NN_WARNING, "GatherProjectHeader", "Failed to receive project file header");

        return false;
    }
}

bool DownloadProjectFile(const std::string& sPrjFile, const std::string& sETag)
{
    try
    {
        fs::path check_file = "NetworkNode";

        if (!fs::exists(check_file))
            fs::create_directory(check_file);

        check_file /= sETag + ".gz";

        // File already exists
        if (fs::exists(check_file))
            return true;

        nncurl prjdownload;

        if (prjdownload.http_download(sPrjFile, check_file.string))
        {
            _log(NN_INFO, "DownloadProjectFile", "Successfully downloaded project file <prjfile=" + sPrjFile + ", etags=" + sETag + ">");

            return true;
        }

        else
        {
            _log(NN_ERROR, "DownloadProjectFile", "Failed to download project file <prjfile=" + sPrjFile + ", etags=" + sETag + ">");

            return false;
        }
    }

    catch (std::exception& ex)
    {
        _log(NN_ERROR, "DownloadProjectFile", "Std exception occured (" + ex.what() + ")");

        return false;
    }
}

void DeleteProjectFile(const std::string& sETag)
{
    try
    {
        // Check if the file exists
        fs::path check_file = "NetworkNode";

        check_file /= sETag + ".gz";

        if (fs::exists(check_file))
        {
            if (fs::remove(check_file))
                return;

            else
            {
                _log(NN_WARNING, "DeleteProjectFile", "Project file was not found");

                return;
            }
        }
    }

    catch (std::exception& ex)
    {
        _log(NN_ERROR, "DeleteProjectFile", "Std exception occured (" + ex.what() + ")");

        return;
    }

}

/************************
* Network Node Database *
************************/

bool SearchDatabase(int nTypeQuery, const std::string& sTable, const std::string& sKey, std::string& sValuea, std::string& sValueb)
{
    std::string sSearchQuery;

    if (nTypeQuery == 1)
        sSearchQuery = "SELECT valuea, valueb FROM '" + sTable + "' WHERE key = '" + sKey + "';";

    if (nTypeQuery == 2)
        sSearchQuery = "SELECT valuea FROM '" + sTable + "' WHERE key = '" + sKey + "';";

    nndb* db;

    if (db->search_query(nTypeQuery, sSearchQuery, sValuea, sValueb))
        return true;

    else
    {
        _log(NN_WARNING, "SearchDatabase", "No result found <type=" + std::to_string(nTypeQuery) + ", table=" + sTable + ", key=" + sKey + ">");

        return false;
    }
}

bool InsertDatabase(int nTypeQuery, const std::string& sTable, const std::string& sKey, const std::string& sValuea, const std::string& sValueb)
{
    nndb* db;
    std::string sInsertQuery;

    if (nTypeQuery == 1)
    {
        sInsertQuery = "CREATE TABLE IF NOT EXISTS '" + sTable + "' (key TEXT PRIMARY NOT NULL, valuea TEXT NOT NULL, valueb TEXT NOT NULL);";

        if (!db->insert_query(sInsertQuery))
        {
            _log(NN_ERROR, "InsertDatabase", "Failed to insert into database <type=" + std::to_string(nTypeQuery) + ", table=" + sTable + ", query=" + sInsertQuery + ">");

            return false;
        }

        sInsertQuery = "INSERT OR REPLACE INTO '" + sTable + "' VALUES('" + sKey + "', '" + sValuea + "', '" + sValueb + "');";

        if (!db->insert_query(sInsertQuery))
        {
            _log(NN_ERROR, "InsertDatabase", "Failed to insert into database <type=" + std::to_string(nTypeQuery) + ", table=" + sTable + ", query=" + sInsertQuery + ">");

            return false;
        }

        return true;
    }

    else if (nTypeQuery == 2)
    {
        sInsertQuery = "CREATE TABLE IF NOT EXISTS '" + sTable + "' (key TEXT PRIMARY NOT NULL, valuea TEXT NOT NULL);";

        if (!db->insert_query(sInsertQuery))
        {
            _log(NN_ERROR, "InsertDatabase", "Failed to insert into database <type=" + std::to_string(nTypeQuery) + ", table=" + sTable + ", query=" + sInsertQuery + ">");

            return false;
        }

        sInsertQuery = "INSERT OR REPLACE INTO '" + sTable + "' VALUES('" + sKey + "', '" + sValuea + "');";

        if (!db->insert_query(sInsertQuery))
        {
            _log(NN_ERROR, "InsertDatabase", "Failed to insert into database <type=" + std::to_string(nTypeQuery) + ", table=" + sTable + ", query=" + sInsertQuery + ">");

            return false;
        }

        return true;
    }
}
/*
bool nndata::ProcessProjectData()
{
    // TODO:
    // 1) Process the project file
    // 2) Put data from project file into a vector
    // 3) Run through CPID database and remove any project data for a cpid not in the project data file (anymore)
    // 4) Process project data export to project_projectname database

    for (const auto& wl : vWhitelist)
    {
        if (fDebug5)
            printf("#%s\n", wl.first.c_str());

        if (!ImportStatsData(wl.first))
        {
            printf("NN : ProcessProjectData : Failed to process data for %s\n", wl.first.c_str());
        }
    }
    return true;
}

bool nndata::ImportStatsData(const std::string& project)
{
    // Get the project data file from the database

    std::string sPrj;
    std::string dummy;

    nndb db;

    if (!db.search_db("projectfiles", "projectfiles", project, sPrj, dummy))
    {
        printf("NN : ImportStatsData : search for project filename failed <project=%s other=%s>\n", project.c_str(), sPrj.c_str());

        return false;
    }

    // Make sure the old project table is removed from any pervious processing
    // If does not exist it will fail but that is perfectly fine
    // Other note is cpids no longer in project will also fade this way
    if (db.drop_db("control", project))
    {
        if (!db.moveprj_db(project))
        {
            printf("NN : ImportStatsData : Failed to move project table to a new table <project=%s>\n", project.c_str());
        }
    }
    try
    {
        // GZIP input stream
        std::ifstream input_file("NeuralNetwork/" + sPrj + ".gz", std::ios_base::in | std::ios_base::binary);

        if (!input_file)
        {
            // If it is not found because project server is down we dont update the project stats in database
            printf("NN : ImportStatsData : Unable to open project file for %s\n", project.c_str());

            return false;
        }

        boostio::filtering_istream in;
        in.push(boostio::gzip_decompressor());
        in.push(input_file);

        // Read XML from GZIP stream

        boostpt::ptree pt;
        read_xml(in, pt);

        // Process user nodes
        for(const boostpt::ptree::value_type& v : pt.get_child("users"))
        {
            if(v.first != "user")
                continue;

            // Gather details for NN
            const std::string& cpid = v.second.get<std::string>("cpid");
            bool bFound = false;

            for (const auto& vc : vCPIDs)
            {
                if (cpid == vc)
                {
                    bFound = true;

                    break;
                }
            }

            if (!bFound)
            {
                if (fDebug5)
                    //printf("NN : ImportStatsData : cpid %s not in beacon list; ignoring.\n", cpid.c_str());

                    continue;
            }

            // Extract credits & data.
            double dNewCredit = v.second.get<double>("total_credit");

            // Let compare to the TCD already in database if one exists

            std::string sLastCredit;

            printf("control: %s %f\n", cpid.c_str(), dNewCredit);
            db.search_db("control", project, cpid, sLastCredit, dummy);

            // No Previous credit
            if (sLastCredit.empty())
            {
                db.insert_db("project", project, cpid, std::to_string(dNewCredit));
            }

            else
            {
                double dLastCredit;

                try
                {
                    // Convert string to double
                    dLastCredit = std::stof(sLastCredit);

                }

                catch (std::exception& ex)
                {
                    printf("NN : ImportStatsData : std::exception while converting string to double <string=%s> (%s); ignoring this entry\n", sLastCredit.c_str(), ex.what());

                    continue;
                }
 // fix this: not continue here idiot! its 0 TDC ....
                if (dNewCredit == dLastCredit)
                {
                    // No Change
                    continue;
                }

                else if (dNewCredit < dLastCredit)
                {
                    // Incase bad stats and results in a negative number
                    // Need to decide whether or not to set to 0 or just keep previous data
                    // for now we keep previous TCD
                    if (fDebug5)
                        printf("NN : ImportStatsData : cpid %s has credit violation; New credit is less then Last credit in database <new=%f, old=%f>\n", cpid.c_str(), dNewCredit, dLastCredit);

                    continue;
                }
                // fix this: need calcuated TDC
                else if (dNewCredit > dLastCredit)
                {
                    // Update credit in database
                    db.insert_db("project", project, cpid, std::to_string(dNewCredit));

                    continue;
                }
            }
        }

        input_file.close();
        db.~nndb();
        return true;
    }

    catch (boost::exception &ex)
    {
        printf("NN : ImportStatsData : boost exception (%s)\n", boost::diagnostic_information(ex).c_str());

        return false;
    }

    catch (std::exception &ex)
    {
        printf("NN : ImportStatsData : std exception (%s)\n", ex.what());

        return false;
    }

    return true;
}
*/
