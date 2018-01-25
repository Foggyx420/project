/*
Neural Network 3.0 (c++ 11)
This is intended to run the application class in which Neural Network 3.0 is.
This code contains code from denravonska and myself.
*/

#include "neuralnetwork3.h"


// Team Requirement?
bool bTeamReq = true;
// Dummy Network Conditions
vector<pair<string, string>> vWhitelist;
vector<string> vCPIDs;

void nndata::setdummy()
{

    // Dummy CPIDs
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

void startnn()
{
    bool test;
    nndb db;
    nndata nn;

    test = db.dbexists();

    if (test)
        printf("1 true\n");
    else
        printf("1 false\n");

    db.insert_db("whitelist", "projecta", "http://projecta.com");

    db.insert_db("whitelist", "projecta", "http://projecta.com");

    db.insert_db("whitelist", "projecta", "http://projecta.com");
    db.insert_db("whitelist", "projectb", "http://projectb.com");
    db.insert_db("whitelist", "projectc", "http://projectc.com");
    db.insert_db("whitelist", "projectc", "http://projectc.com");
    db.delete_db("whitelist", "projectb");

    std::string stest;
    db.search_db("whitelist", "projecta", stest);
    printf("test search %s\n", stest.c_str());

    // phase 2
    nn.setdummy();
    nn.ProcessProjectData();

    return;
}
/*///////////////////////
// Neural Node Section //
///////////////////////*/

// Check if user is allowing himself to be a neural node
bool nndata::IsNeuralNode()
{
    // Use GetArg to see if neuralnode equals true or false once in gridcoin code
    return true;
}

/*///////////////////////
// Database Section    //
///////////////////////*/

bool nndata::DownloadProjectFiles()
{
    for (auto const& vWL : vWhitelist)
    {
        if (vWL.first.empty() || vWL.second.empty())
            continue;

        std::size_t pos = vWL.second.find("@");
        std::string sUrl = vWL.second.substr(0, pos) + "stats/";
        std::string sTeam = vWL.first + ".team.gz";
        std::string sUser = vWL.first + ".user.gz";

        if (fDebug5)
            printf("NN : Downloading whitelisted project %s data.\n", vWL.first.c_str());

        // Download team file and user file
        nncurl project;

        if (!project.http_download(sUrl + "team.gz", sTeam))
            if (fDebug5)
                printf("NN : Failure downloading Project %s team file\n", vWL.first.c_str());

        if (!project.http_download(sUrl + "user.gz", sUser))
            if (fDebug5)
                printf("NN : Failure downloading Project %s user file\n", vWL.first.c_str());
    }

    return true;
}

bool nndata::ProcessProjectData()
{
    // TODO:
    // 1) Process the project file
    // 2) Put data from project file into a vector
    // 3) Run through CPID database and remove any project data for a cpid not in the project data file (anymore)
    // 4) Process project data export to project_projectname database

    for (const auto& wl : vWhitelist)
    {
        printf("#%s\n", wl.first.c_str());

        ImportStatsData(wl.first);
    }
}

bool nndata::ImportStatsData(const std::string& project)
{
    try
    {
        // GZIP input stream
        std::ifstream input_file("NeuralNetwork/" + project + ".user.gz", std::ios_base::in | std::ios_base::binary);
        boost::iostreams::filtering_istream in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(input_file);

        // Read XML from GZIP stream
        using boost::property_tree::ptree;

        ptree pt;
        read_xml(in, pt);

        // Process user nodes
        for(const ptree::value_type& v : pt.get_child("users"))
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
            double total_credit = v.second.get<double>("total_credit");
            std::cout << "Credits for" << cpid << " " << std::fixed << total_credit << std::endl;
        }

        input_file.close();
    }

    catch (boost::exception &ex)
    {
        printf("NN : ImportStatsData : boost exception (%s)\n", boost::diagnostic_information(ex).c_str());

        return false;
    }

    return true;
}
