#include "dkv/engine.hpp"
#include <fstream>
#include <stdexcept>

namespace dkv {
Engine::Engine(std::filesystem::path directory) : directory_(std::move(directory)), wal_(directory_/"store.wal"), snapshot_(directory_/"store.sst") {
    std::filesystem::create_directories(directory_); recover();
}
std::string Engine::encode(const std::string& s) {
    static constexpr char hex[]="0123456789ABCDEF"; std::string out; out.reserve(s.size()*2);
    for (unsigned char c : s) { out += hex[c>>4]; out += hex[c&15]; } return out;
}
std::optional<std::string> Engine::decode(const std::string& s) {
    if (s.size()%2) return std::nullopt;
    auto digit=[](char c)->int { if(c>='0'&&c<='9')return c-'0'; if(c>='A'&&c<='F')return c-'A'+10; if(c>='a'&&c<='f')return c-'a'+10; return -1; };
    std::string out; out.reserve(s.size()/2); for(std::size_t i=0;i<s.size();i+=2){int a=digit(s[i]),b=digit(s[i+1]);if(a<0||b<0)return std::nullopt;out.push_back(static_cast<char>((a<<4)|b));} return out;
}
void Engine::recover() {
    auto load=[this](const std::filesystem::path& file, bool wal) {
        std::ifstream in(file); std::string line;
        while(std::getline(in,line)) { auto a=line.find('\t'), b=a==std::string::npos?a:line.find('\t',a+1); if(a==std::string::npos||b==std::string::npos) continue;
            auto k=decode(line.substr(a+1,b-a-1)); auto v=decode(line.substr(b+1)); if(!k||!v) continue;
            if(line[0]=='P') data_[*k]={*v,false}; else if(line[0]=='D') data_[*k]={"",true};
        }
        (void)wal;
    };
    load(snapshot_,false); load(wal_,true);
}
void Engine::append(char op,const std::string& key,const std::string& value) {
    std::ofstream out(wal_,std::ios::app); if(!out) throw std::runtime_error("cannot open WAL");
    out<<op<<'\t'<<encode(key)<<'\t'<<encode(value)<<'\n'; out.flush(); if(!out) throw std::runtime_error("WAL write failed");
}
void Engine::put(const std::string& k,const std::string& v){std::lock_guard lock(mutex_);append('P',k,v);data_[k]={v,false};}
bool Engine::erase(const std::string& k){std::lock_guard lock(mutex_);auto it=data_.find(k);if(it==data_.end()||it->second.deleted)return false;append('D',k,"");it->second={"",true};return true;}
std::optional<std::string> Engine::get(const std::string& k)const{std::lock_guard lock(mutex_);auto it=data_.find(k);if(it==data_.end()||it->second.deleted)return std::nullopt;return it->second.value;}
std::vector<std::pair<std::string,std::string>> Engine::scan()const{std::lock_guard lock(mutex_);std::vector<std::pair<std::string,std::string>> out;for(const auto&[k,v]:data_)if(!v.deleted)out.emplace_back(k,v.value);return out;}
std::size_t Engine::size()const{std::lock_guard lock(mutex_);std::size_t n=0;for(const auto&[k,v]:data_){(void)k;if(!v.deleted)++n;}return n;}
void Engine::compact(){std::lock_guard lock(mutex_);auto tmp=snapshot_;tmp += ".tmp";{std::ofstream out(tmp,std::ios::trunc);if(!out)throw std::runtime_error("cannot write snapshot");for(const auto&[k,v]:data_)out<<(v.deleted?'D':'P')<<'\t'<<encode(k)<<'\t'<<encode(v.value)<<'\n';out.flush();if(!out)throw std::runtime_error("snapshot write failed");}std::error_code ec;std::filesystem::remove(snapshot_,ec);std::filesystem::rename(tmp,snapshot_);std::ofstream clear(wal_,std::ios::trunc);if(!clear)throw std::runtime_error("cannot reset WAL");}
}
