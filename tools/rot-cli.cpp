
#include <iostream>
#include <iterator>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <Rot.h>

using namespace std;
using namespace taf;
using namespace Comm;

#define eq(s1, s2) boost::iequals(s1, s2)

static string OBJ_NAME_ROUTE = "Comm.RotServer.RotObj@tcp -h 10.21.32.223 -t 60000 -p 10100";
int main()
{
    Communicator comm;
    RotPrx prx;

    try
    {
        int appId = 0;

        comm.stringToProxy(OBJ_NAME_ROUTE, prx);

        while (1)
        {
            string  inp;

            cout << "$>";
            std::getline(cin, inp);

            vector<string> v;
            boost::split(v, inp, boost::is_any_of("\t "));

            /* delete empty ones */
            v.erase(std::remove_if(v.begin(), v.end(), [](const string &s) { return s.empty() || std::all_of(s.begin(), s.end(), [](char c) { return isspace(c); });}), v.end());

            if (v.empty())
            {
                continue;
            }

            auto &cmd = v[0];

            if (eq(cmd,"quit") || eq(cmd, "q"))//q
            {
                break;
            }
            else if (eq(cmd, "conn")) //connect to another machine
            {
                string taf_obj_name = inp;
                taf_obj_name.erase(0,  taf_obj_name.find_first_of("\t ")+1);
                comm.stringToProxy(taf_obj_name, prx);
                try
                {
                    prx->taf_ping();
                }
                catch(std::exception &e)
                {
                    cout << e.what() << endl;
                }
            }
            else if (eq(cmd, "switch")) //switch app-id
            {
                string appName;
                appId = std::stoi(v[1]);
                auto iret = prx->getAppName(appId, appName);
                cout << "switched to app id:" << appId << " app name:" << appName << endl;
                if (iret != 0)
                {
                    cout << "no mached app, back to default app id 0" << endl;
                    appId = 0;
                }
            }
            else if (eq(cmd, "set")) // set key value
            {
                string key=v[1], value=v[2];
                StringRobjOption opt;
                auto iret = prx->set(appId, key, value, opt);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "mset")) // mset [nx] k v k v k v
            {
                int nx = 0;
                if (eq(v[1], "nx")) nx = 1;
                size_t begin = nx?2:1;
                int pairs = (v.size()-begin)/2;

                int pos=begin;
                map<string, string> mkv;
                for (int i=0; i<pairs; ++i, pos+=2)
                {
                    mkv[v[pos]] = v[pos+1];
                }

                auto iret = prx->mset(appId, mkv, nx);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd,  "get")) //get key
            {
                string key=v[1], value;
                auto iret = prx->get(appId, key, value);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (value.empty()) cout << "'Empty'" << endl;
                else  cout << value << endl;
            }
            else if (eq(cmd, "mget")) //mget key key key
            {
                vector<string> vk;
                std::copy(v.begin()+1, v.end(), back_inserter(vk));

                map<string,  string> mkv;
                auto iret = prx->mget(appId, vk, mkv);
                if (iret) cout << "'Failed' " << iret << endl;
                for (auto &kv:mkv)
                {
                    cout << kv.first << ":" << kv.second<<endl;
                }
            }
            else if (eq(cmd, "incrby")) //incrby key number
            {
                string key=v[1];
                long incr=std::stol(v[2]), result;
                auto iret = prx->incrby(appId, key, incr, result);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << result << endl;
            }
            else if (eq(cmd, "incrbyfloat")) //incrbyfloat key  incr
            {
                string key = v[1];
                double inc = std::stod(v[2]);
                double result=0;

                auto iret = prx->incrbyfloat(appId, key, inc, result);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << result << endl;
            }
            else if (eq(cmd, "append")) //append key value
            {
                string key=v[1], value=v[2];
                auto iret = prx->append(appId, key, value);
                if (iret) cout<< "'Failed' " << iret << endl;
                else
                {
                    string  result;
                    prx->get(appId, key, result);
                    cout << result << endl;
                }
            }
            else if (eq(cmd, "push") || eq(cmd, "lpush") || eq(cmd, "rpush") ||
                     eq(cmd, "pushx") || eq(cmd, "lpushx") || eq(cmd, "rpushx")) //push key v1 v2 v3  lpush/rpush
            {
                string key=v[1];
                vector<string> items;

                std::copy(v.begin()+2, v.end() , back_inserter(items));
                ListRobjOption opt;
                if (tolower(cmd[cmd.size()-1]) == 'x')
                    opt.set_if_exist = 1;

                auto dir = ELIST_TAIL;
                if (tolower(cmd[0]) == 'l') dir = ELIST_HEAD;

                long length;
                auto iret = prx->push(appId, key, items, dir, opt, length);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << "length:" << length << endl;
            }
            else if (eq(cmd, "pop") || eq(cmd, "lpop") || eq(cmd, "rpop")) //pop key, lpop/rpop
            {
                string key=v[1];
                auto dir = cmd[0] == 'r' ? Comm::ELIST_TAIL:Comm::ELIST_HEAD;
                string item;

                auto iret = prx->pop(appId, key, dir, item);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (item.empty()) cout << "'Empty'";
                else                   cout << item << endl;
            }
            else if(eq(cmd, "lindex")) //lindex key index
            {
                string key = v[1];
                int index = std::stoi(v[2]);
                string value;
                auto iret = prx->lindex(appId, key, index, value);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << value << endl;
            }
            else if(eq(cmd, "lset"))  //lset key index value
            {
                string key = v[1];
                int index = std::stoi(v[2]);
                string value = v[3];
                auto iret = prx->lset(appId, key, index, value);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "llen")) //llen key
            {
                string key = v[1];
                long length;
                auto iret = prx->llen(appId, key, length);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << length << endl;
            }
            else if (eq(cmd, "lrem")) //lrem key count value
            {
                string key = v[1];
                long count = std::stol(v[2]);
                string value = v[3];
                long removed=0;
                auto iret = prx->lrem(appId, key, count, value, removed);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << removed << " removed" << endl;
            }
            else if (eq(cmd, "lrange")) //lrange key 0 -1
            {
                string key=v[1];
                long start = 0;
                long end = -1;

                if (v.size() >= 4)
                {
                    start=std::stoi(v[2]);
                    end=std::stoi(v[3]);
                }

                vector<string> items;
                prx->lrange(appId, key, start, end, items);
                cout << "[";
                std::copy(items.begin(), items.end(), ostream_iterator<string>(cout, ", "));
                cout << "]" << endl;
            }
            else if (eq(cmd, "hmset")) //hmset key f v  f v f v
            {
                string key = v[1];
                int pairs = (v.size()-2)/2;

                int pos = 2;
                map<string, string> mFV;
                for (int i =0; i < pairs ; ++i, pos+=2)
                {
                    mFV[v[pos]] = v[pos+1];
                }

                auto iret = prx->hmset(appId, key, mFV);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "hmget")) //hmget key f f f
            {
                string key = v[1];
                vector<string> vF;
                std::copy(v.begin()+2, v.end(), back_inserter(vF));

                map<string, string> mFV;
                auto iret = prx->hmget(appId, key, vF, mFV);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (mFV.empty()) cout << "'Empty'" << endl;
                else
                {
                    for (auto &fv: mFV)
                    {
                        cout << fv.first<< ":" << fv.second << endl;
                    }
                }
            }
            else if (eq(cmd, "hgetall"))  //hgetall key
            {
                string key = v[1];
                map<string, string> mFV;
                auto iret = prx->hgetall(appId, key, mFV);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (mFV.empty()) cout << "'Empty'" << endl;
                else
                {
                    for (auto &fv: mFV)
                    {
                        cout << fv.first<< ":" << fv.second << endl;
                    }
                }
            }
            else if (eq(cmd, "hexists"))  //hexists key field
            {
                string key = v[1];
                string field = v[2];

                int existed = 0;
                auto iret = prx->hexists(appId, key, field, existed);
                if (iret) cout << "'Failed' " << iret << endl;
                else
                {
                    cout << (existed?"exists":"not exist") << endl;
                }
            }
            else if (eq(cmd, "hdel")) //hdel key f f f
            {
                string key = v[1];
                vector<string> vF;

                std::copy(v.begin()+1, v.end(), back_inserter(vF));
                auto iret = prx->hdel(appId, key, vF);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "sadd")) //sadd key m m m
            {
                string key = v[1];
                vector<string> vM;
                std::copy(v.begin()+2, v.end(), back_inserter(vM));

                auto iret = prx->sadd(appId, key, vM);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "srem")) //srem key m m m
            {
                string key = v[1];
                vector<string> vM;
                std::copy(v.begin()+2, v.end(), back_inserter(vM));

                auto iret = prx->srem(appId, key, vM);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "spop")) //spop key [count]
            {
                string key = v[1];
                long count = 1;
                if (v.size()>2)
                    count = std::stol(v[2]);

                vector<string> vM;
                auto iret = prx->spop(appId, key, count,vM);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (vM.empty()) cout << "'Empty'" << endl;
                else
                {
                    std::copy(vM.begin(), vM.end(), ostream_iterator<string>(cout, "  "));
                    cout << endl;
                }
            }
            else if (eq(cmd, "sismember")) //sismember key m
            {
                string key = v[1];
                string mem = v[2];
                int is_mem=0;
                auto iret = prx->sismember(appId, key, mem, is_mem);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (is_mem) cout << "yes" <<  endl;
                else cout << "no" << endl;
            }
            else if (eq(cmd, "smembers")) //smembers key
            {
                string key = v[1];
                vector<string> vM;
                auto iret = prx->smembers(appId, key, vM);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (vM.empty()) cout << "'Empty'" << endl;
                else
                {
                    std::copy(vM.begin(), vM.end(), ostream_iterator<string>(cout, "  "));
                    cout << endl;
                }
            }
            else if (eq(cmd, "sinter") || eq(cmd, "sinterstore")) //sinter/sinterstore [storekey] k k k
            {
                int store = 0;
                string storekey;
                string cmd_l = cmd;
                boost::algorithm::to_lower(cmd_l);
                if (cmd_l.find("store") != string::npos)
                {
                    storekey = v[1];
                    store = 1;
                }

                vector<string> vK;
                vector<string> vResults;
                if (store) std::copy(v.begin()+2, v.end(), back_inserter(vK));
                else       std::copy(v.begin()+1, v.end(), back_inserter(vK));

                auto iret = prx->sinter(appId, vK, storekey, vResults);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (store)
                {
                    cout << " Store successsfully!" <<endl;
                }
                else
                {
                    if (vResults.empty()) cout << "'Empty'" << endl;
                    else
                    {
                        std::copy(vResults.begin(), vResults.end(), ostream_iterator<string>(cout, " "));
                        cout << endl;
                    }
                }
            }
            else if (eq(cmd, "sdiff") || eq(cmd, "sdiffstore"))// sdiff/sdiffstore [storekey] k k k
            {
                int store = 0;
                string storekey;
                string cmd_l = cmd;
                boost::algorithm::to_lower(cmd_l);
                if (cmd_l.find("store") != string::npos)
                {
                    storekey = v[1];
                    store = 1;
                }

                vector<string> vK;
                vector<string> vResults;
                if (store) std::copy(v.begin()+2, v.end(), back_inserter(vK));
                else       std::copy(v.begin()+1, v.end(), back_inserter(vK));

                auto iret = prx->sdiff(appId, vK, storekey, vResults);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (store)
                {
                    cout << " Store successsfully!" <<endl;
                }
                else
                {
                    if (vResults.empty()) cout << "'Empty'" << endl;
                    else
                    {
                        std::copy(vResults.begin(), vResults.end(), ostream_iterator<string>(cout, " "));
                        cout << endl;
                    }
                }
            }
            else if (eq(cmd, "sunion") || eq(cmd, "sunionstore"))  //sunion/sunionstore [storekey] k k k
            {
                int store = 0;
                string storekey;
                string cmd_l = cmd;
                boost::algorithm::to_lower(cmd_l);
                if (cmd_l.find("store") != string::npos)
                {
                    storekey = v[1];
                    store = 1;
                }

                vector<string> vK;
                vector<string> vResults;
                if (store) std::copy(v.begin()+2, v.end(), back_inserter(vK));
                else       std::copy(v.begin()+1, v.end(), back_inserter(vK));

                auto iret = prx->sunion(appId, vK, storekey, vResults);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (store)
                {
                    cout << " Store successsfully!" <<endl;
                }
                else
                {
                    if (vResults.empty()) cout << "'Empty'" << endl;
                    else
                    {
                        std::copy(vResults.begin(), vResults.end(), ostream_iterator<string>(cout, " "));
                        cout << endl;
                    }
                }
            }
            else if (eq(cmd, "zadd")) //zadd key s m s m s m
            {
                string key = v[1];
                int pairs = (v.size()-2)/2;

                int pos = 2;
                vector<ZsetScoreMember> vSM;
                ZsetScoreMember sm;
                for (int i=0; i<pairs; ++i, pos+=2)
                {
                    sm.score = std::stod(v[pos]);
                    sm.member = v[pos+1];
                    vSM.push_back(sm);
                }

                ZsetRobjOption opt; //TODO
                    //taf::Char set_if_not_exist;
                    //taf::Char set_if_exist;
                auto iret = prx->zadd(appId, key, vSM, opt);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "zrem")) //zrem key m m m
            {
                string key = v[1];
                vector<string> vM;
                std::copy(v.begin()+1, v.end(), back_inserter(vM));
                auto iret = prx->zrem(appId, key, vM);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "zrank") || eq(cmd, "zrevrank"))//zrank/zrevrank key m
            {
                string key = v[1];
                string member = v[2];
                long rank=0;
                int reverse = 0;

                if (tolower(cmd[1]) == 'r' &&
                    tolower(cmd[2]) == 'e' &&
                    tolower(cmd[3]) == 'v')
                    reverse = 1;

                auto iret = prx->zrank(appId, key, member, reverse, rank);
                if (iret) cout << "'Failed' " << iret;
                else if (reverse) cout << "rev-rank:  " << rank <<endl;
                else          cout << "rank: " << rank << endl;
            }
            else if (eq(cmd, "zincrby")) //zincrby key incr member
            {
                string key = v[1];
                double increment = std::stod(v[2]);
                string member = v[3];
                double new_score=0;
                auto iret = prx->zincrby(appId, key, increment, member, new_score);
                if (iret) cout << "'Failed' " << iret << endl;
                else cout << "new score:" << new_score << endl;
            }
            else if (eq(cmd, "zrange") || eq(cmd, "zrevrange")) //zrange key start end
            {
                string key = v[1];
                long start = 0;
                long end = -1;

                if (v.size() >= 4)
                {
                    start = std::stol(v[2]);
                    end   = std::stol(v[3]);
                }

                Comm::ZsetRangeOption opt;
                opt.with_scores = 1;
                if (tolower(cmd[1]) == 'r' &&
                    tolower(cmd[2]) == 'e' &&
                    tolower(cmd[3]) == 'v')
                    opt.reverse = 1;

                vector<Comm::ZsetScoreMember> vSM;
                auto iret = prx->zrange(appId, key, start, end, opt, vSM);
                if (iret) cout << "'Failed' "  << endl;
                else if (vSM.empty()) cout << "'Emtpy'" << endl;
                else
                {
                    for (auto &sm :vSM)
                    {
                        cout << sm.member << " : " << sm.score << endl;
                    }
                }
            }
            else if (eq(cmd, "ttl")) //ttl key
            {
                string key=v[1];

                taf::Int64 sec;
                prx->ttl(appId, key, sec);
                cout << sec << " seconds" << endl;
            }
            else if (eq(cmd, "del")) //del k1 k2 k3...
            {
                vector<string> items;
                std::copy(v.begin()+1, v.end(), back_inserter(items));
                long deleted=0;
                auto iret = prx->del(appId, items, deleted);
                if (iret) cout << "'Failed' " << iret << endl;
                else      cout << deleted << " deleted" << endl;
            }
            else if (eq(cmd, "expire")) //expire key 5
            {
                string key = v[1];
                taf::Int64  seconds=std::stol(v[2]);
                auto iret = prx->expire(appId, key, seconds);
                cout << ((iret==0)?"'Okay'":"'Failed'") << endl;
            }
            else if (eq(cmd, "keys")) //keys
            {
                map<string, string> keytypes;
                auto iret = prx->keys(appId, keytypes);
                if (iret) cout << "'Failed' " << iret << endl;
                else if (keytypes.empty()) cout << "'Empty'" << endl;
                else
                {
                    for (auto &kt : keytypes)
                    {
                        cout << "'"<< kt.first << "' : '" <<  kt.second << "'" << endl;
                    }
                }
            }
            else
            {
                cout << "no matched command '" << cmd << "'" <<endl;
            }

        }
    }
    catch(std::exception e)
    {
        cout << "exception got! " << e.what() << endl;
    }

    cout << "Bye bye..."<<endl;

    return 0;
}

