/*
 * Redis on Taf, aka 'Rot'
 */
#include "RotServer.h"
#include "RotHook.h"
#include "RotImp.h"
#include <iostream>
#include <algorithm>
#include "server.h" //redis server

#define APP_ID_MAX 128

RotServer g_app;

void  rot_oom_handler(size_t allocation_size)
{
    serverLog(LL_WARNING,"Out Of Memory allocating %zu bytes!", allocation_size);
    serverPanic("Redis aborting for OUT OF MEMORY");
}

int rot_log_sinker(int level, const char *msg)
{
    if (level == LL_WARNING)
    {
        LOG->warn() << msg  << endl;
    }
    else
    {
        LOG->debug() << msg  << endl;
    }

    return TAF_HOOK_DONE;
}

/**
 * @return 0 on success otherwise -1.
 */
int RotServer::initRedis(TC_Config &tConf)
{
    typedef std::map<int, std::string>::value_type Iter;
    auto max_it = std::max_element(appid2name_.begin(), appid2name_.end(),
            [] (const Iter &it1, const Iter &it2) -> bool { return it1.first < it2.first; });

    int dbnum = (max_it == appid2name_.end()?2: (max_it->first+1));
    long long maxmemory_bytes = TC_Common::toSize(tConf.get("/main/<maxmemory>"), 1024*1024*500);

    LOG->debug() << __FUNCTION__ << "|database number:" << dbnum << ", max-memory:"
        << maxmemory_bytes << " bytes" << endl;


    memset(&server, 0, sizeof(server)); //:D

    /* refer to server.c:main() but only keep what we need */

    setlocale(LC_COLLATE,""); //TODO need this?
    zmalloc_enable_thread_safeness();
    zmalloc_set_oom_handler(rot_oom_handler);
    srand(time(NULL)^getpid());

    struct timeval tv;
    gettimeofday(&tv,NULL);
    dictSetHashFunctionSeed(tv.tv_sec^tv.tv_usec^getpid());

    initServerConfig(1);
    server.unixsocket        = nullptr;
    server.ipfd_count        = 0;
    server.sentinel_mode     = 0;
    server.syslog_enabled    = 0;
    server.daemonize         = 1;
    server.supervised        = 0;
    server.port              = 0;    //hand over the job to TAF
    server.dbnum             = dbnum;
    server.maxmemory         = maxmemory_bytes;
    server.maxmemory_policy  = MAXMEMORY_ALLKEYS_LRU;
    server.maxmemory_samples = 5;
    /* set hook function */
    server.logSinker         = rot_log_sinker;
    /*server.todo_of.erase_fn = NULL */
    /*server.todo_of.del_fn   = NULL */

    initServer(1);
    LOG->debug() << "Redis server started, Redis version " << REDIS_VERSION << endl;
    linuxMemoryWarnings();

    return 0;
}

void RotServer::exitRedis()
{
    if (server.el) aeDeleteEventLoop(server.el);
}

void RotServer::initialize()
{
    //initialize application here

    addServant<RotImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".RotObj");

    string sConf = ServerConfig::ServerName + ".conf";
    addConfig(sConf);

    TC_Config tConf;
    tConf.parseFile(ServerConfig::BasePath + sConf);

    auto appmapcfg = tConf.getDomainMap("/main/app"); /* app-name=app-id */
    for (const auto &item : appmapcfg)
    {
        int id = std::stoi(item.second);
        if (appid2name_.count(id) > 0)
        {
            LOG->error() << "Wrong config: Same app ID found for " <<  item.first << endl;
            exit(0);
        }

        appid2name_[id] = item.first;
    }

    int iret = initRedis(tConf);
    if (iret)
    {
        LOG->error() << "failed to init redis instance" << endl;
    }

    FDLOG() << "Rot server restarted" << endl;
}

void RotServer::destroyApp()
{
    //destroy application here: 
    //

    exitRedis();
}



redisDb* RotServer::lookforDb(int appId)
{
    if (0 <= appId && appId < server.dbnum)
    {
       return &server.db[appId];
    }

    return nullptr;
}



int main(int argc, char *argv[])
{
    try
    {
        g_app.main(argc, argv);
        g_app.waitForShutdown();
    }
    catch(std::exception &e)
    {
        std::cerr << "std::exception:" << e.what() << std::endl;
    }
    catch(...)
    {
        std::cerr << "unknown exception." << std::endl;
    }

    return -1;
}
