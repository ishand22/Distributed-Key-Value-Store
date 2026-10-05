#include "dkv/raft.hpp"
#include <algorithm>
#include <stdexcept>
namespace dkv {
RaftNode::RaftNode(std::uint64_t id,std::size_t n):id_(id),cluster_size_(std::max<std::size_t>(1,n)),acknowledgements_(1,1),voted_(cluster_size_,false){if(id_>=cluster_size_)throw std::invalid_argument("node id must be below cluster size");}
std::uint64_t RaftNode::begin_election(){std::lock_guard lock(mutex_);++term_;role_=Role::Candidate;std::fill(voted_.begin(),voted_.end(),false);voted_[id_]=true;if(cluster_size_==1)role_=Role::Leader;return term_;}
bool RaftNode::record_vote(std::uint64_t t,std::uint64_t voter){std::lock_guard lock(mutex_);if(t>term_){term_=t;role_=Role::Follower;}if(t!=term_||role_!=Role::Candidate||voter>=cluster_size_||voted_[voter])return false;voted_[voter]=true;auto votes=static_cast<std::size_t>(std::count(voted_.begin(),voted_.end(),true));if(votes>cluster_size_/2)role_=Role::Leader;return true;}
bool RaftNode::append_command(std::string c){std::lock_guard lock(mutex_);if(role_!=Role::Leader)return false;log_.push_back({term_,std::move(c)});acknowledgements_.push_back(1);if(cluster_size_==1)commit_index_=log_.size();return true;}
bool RaftNode::record_replication(std::uint64_t index,std::uint64_t peer){std::lock_guard lock(mutex_);if(role_!=Role::Leader||index==0||index>log_.size()||peer>=cluster_size_||peer==id_)return false;auto& count=acknowledgements_[index];++count;if(count>cluster_size_/2)commit_index_=std::max(commit_index_,index);return true;}
void RaftNode::observe_higher_term(std::uint64_t t){std::lock_guard lock(mutex_);if(t>term_){term_=t;role_=Role::Follower;}}
Role RaftNode::role()const{std::lock_guard lock(mutex_);return role_;}std::uint64_t RaftNode::term()const{std::lock_guard lock(mutex_);return term_;}std::uint64_t RaftNode::commit_index()const{std::lock_guard lock(mutex_);return commit_index_;}
std::vector<LogEntry> RaftNode::committed_entries()const{std::lock_guard lock(mutex_);return {log_.begin(),log_.begin()+static_cast<std::ptrdiff_t>(commit_index_)};}
}
