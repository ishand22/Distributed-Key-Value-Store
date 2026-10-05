#include "dkv/engine.hpp"
#include "dkv/raft.hpp"
#include "dkv/thread_pool.hpp"
#include <iostream>
#include <sstream>

int main(int argc,char**argv){
    std::filesystem::path directory=argc>1?argv[1]:"data";
    try {
        dkv::Engine store(directory); dkv::RaftNode raft(0); raft.begin_election();
        std::cout<<"DKV single-node mode. Data: "<<directory.string()<<"\nCommands: put <key> <value> | get <key> | del <key> | scan | compact | status | quit\n";
        std::string line;
        while(std::cout<<"> "&&std::getline(std::cin,line)){
            std::istringstream in(line);std::string cmd,key,value;in>>cmd;
            try {
                if(cmd=="put"){if(!(in>>key))throw std::runtime_error("usage: put <key> <value>");std::getline(in,value);if(!value.empty()&&value.front()==' ')value.erase(0,1);if(value.empty())throw std::runtime_error("value cannot be empty");if(!raft.append_command("PUT "+key+" "+value))throw std::runtime_error("no leader");store.put(key,value);std::cout<<"OK\n";}
                else if(cmd=="get"){if(!(in>>key))throw std::runtime_error("usage: get <key>");auto v=store.get(key);if(v)std::cout<<*v<<'\n';else std::cout<<"NOT_FOUND\n";}
                else if(cmd=="del"){if(!(in>>key))throw std::runtime_error("usage: del <key>");std::cout<<(store.erase(key)?"OK":"NOT_FOUND")<<'\n';}
                else if(cmd=="scan"){for(const auto&[k,v]:store.scan())std::cout<<k<<" = "<<v<<'\n';}
                else if(cmd=="compact"){store.compact();std::cout<<"COMPACTED\n";}
                else if(cmd=="status"){std::cout<<"role="<<(raft.role()==dkv::Role::Leader?"leader":"follower")<<" term="<<raft.term()<<" committed="<<raft.commit_index()<<" keys="<<store.size()<<'\n';}
                else if(cmd=="quit"||cmd=="exit")break;
                else if(!cmd.empty())std::cout<<"Unknown command\n";
            }catch(const std::exception&e){std::cerr<<"ERROR: "<<e.what()<<'\n';}
        }
    }catch(const std::exception&e){std::cerr<<"Fatal: "<<e.what()<<'\n';return 1;}
}
