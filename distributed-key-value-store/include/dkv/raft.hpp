#pragma once
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace dkv {
enum class Role { Follower, Candidate, Leader };
struct LogEntry { std::uint64_t term; std::string command; };

// Raft state-machine core. Network transport and peer RPC are intentionally
outside this class; callers provide vote/replication results via its methods.
class RaftNode {
public:
    explicit RaftNode(std::uint64_t id, std::size_t cluster_size = 1);
    std::uint64_t begin_election();
    bool record_vote(std::uint64_t term, std::uint64_t voter);
    bool append_command(std::string command);
    bool record_replication(std::uint64_t index, std::uint64_t peer);
    void observe_higher_term(std::uint64_t term);
    [[nodiscard]] Role role() const;
    [[nodiscard]] std::uint64_t term() const;
    [[nodiscard]] std::uint64_t commit_index() const;
    [[nodiscard]] std::vector<LogEntry> committed_entries() const;
private:
    std::uint64_t id_;
    std::size_t cluster_size_;
    mutable std::mutex mutex_;
    Role role_ = Role::Follower;
    std::uint64_t term_ = 0, commit_index_ = 0;
    std::vector<LogEntry> log_;
    std::vector<std::size_t> acknowledgements_;
    std::vector<bool> voted_;
};
}
