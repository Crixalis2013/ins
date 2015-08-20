#ifndef GALAXY_INS_META_H_
#define GALAXY_INS_META_H_
#include <stdio.h>
#include <string>
#include <map>
#include <stdint.h>
#include "proto/ins_node.pb.h"

namespace galaxy {
namespace ins {

class UserManager;

class Meta {
public:
    Meta(const std::string& data_dir);
    ~Meta();
    int64_t ReadCurrentTerm();
    void ReadVotedFor(std::map<int64_t, std::string>& voted_for);
    void ReadUserList(UserManager* manager);
    void WriteCurrentTerm(int64_t term); 
    void WriteVotedFor(int64_t term, const std::string& server_id);
    void WriteUserInfo(const UserInfo& user);
    void WriteUserList(const UserManager& manager);
private:
    std::string data_dir_;
    FILE* term_file_;
    FILE* vote_file_;
    FILE* user_file_;
};

} //namespace ins
} //namespace galaxy
#endif
