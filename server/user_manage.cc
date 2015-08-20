#include "server/user_manage.h"

#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/ostream_iterator.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <sstream>
#include "common/logging.h"
#include "common/timer.h"

namespace galaxy {
namespace ins {

std::string UserManager::CalcUuid(const std::string& name) {
    using namespace boost::archive::iterators;
    int64_t now = ins_common::timer::get_micros();
    std::string text = name + boost::lexical_cast<std::string>(now);
    std::stringstream uuid;
    typedef base64_from_binary <
        transform_width <
            const char *,
            6,
            8
        >
    > base64_text;

    std::copy(base64_text(text.c_str()),
              base64_text(text.c_str() + text.size()),
              std::ostream_iterator<char>(uuid));

    return uuid.str();
}

std::string UserManager::CalcName(const std::string& uuid) {
    using namespace boost::archive::iterators;
    int64_t now = ins_common::timer::get_micros();
    std::string text = uuid + boost::lexical_cast<std::string>(now);
    std::stringstream name;
    typedef transform_width <
        binary_from_base64 <
            const char *
        >,
        8,
        6
    > base64_code;

    std::copy(base64_code(text.c_str()),
              base64_code(text.c_str() + text.size()),
              std::ostream_iterator<char>(name));
    return name.str();
}

UserManager::UserManager(const UserInfo& root) {
    user_list_["root"] = root;
    user_list_["root"].set_username("root");
    user_list_["root"].clear_uuid();
}

Status UserManager::Login(const std::string& name,
                          const std::string& password,
                          std::string* uuid) {
    MutexLock lock(&mu_);
    std::map<std::string, UserInfo>::iterator user_it = user_list_.find(name);
    if (user_it == user_list_.end()) {
        LOG(WARNING, "Inexist user tried to login :%s", name.c_str());
        return kUnknownUser;
    }
    if (!user_it->second.has_uuid()) {
        LOG(WARNING, "Try to log in a logged account :%s", name.c_str());
        return kUserExists;
    }
    if (user_it->second.passwd() != password) {
        LOG(WARNING, "Password error for logging :%s", name.c_str());
        return kPasswordError;
    }

    user_it->second.set_uuid(CalcUuid(name));
    logged_users_[user_it->second.uuid()] = name;
    if (uuid != NULL) {
        *uuid = user_it->second.uuid();
    }
    return kOk;
}

Status UserManager::Logout(const std::string& uuid) {
    MutexLock lock(&mu_);
    std::map<std::string, std::string>::iterator online_it = logged_users_.find(uuid);
    if (online_it == logged_users_.end()) {
        LOG(WARNING, "Logout for an inexist user :%s", uuid.c_str());
        return kUnknownUser;
    }

    user_list_[online_it->second].clear_uuid();
    logged_users_.erase(online_it);
    return kOk;
}

Status UserManager::Register(const std::string& name, const std::string& password) {
    MutexLock lock(&mu_);
    std::map<std::string, UserInfo>::iterator user_it = user_list_.find(name);
    if (user_it != user_list_.end()) {
        LOG(WARNING, "Try to register an exist user :%s", name.c_str());
        return kUserExists;
    }
    user_list_[name].set_username(name);
    user_list_[name].set_passwd(password);
    return kOk;
}

Status UserManager::ForceOffline(const std::string& myid, const std::string& uuid) {
    MutexLock lock(&mu_);
    std::map<std::string, std::string>::iterator online_it = logged_users_.find(myid);
    if (online_it == logged_users_.end()) {
        return kUnknownUser;
    }
    if (online_it->second == "root" || myid == uuid) {
        return Logout(uuid);
    }
    return kPermissionDenied;
}

Status UserManager::DeleteUser(const std::string& myid, const std::string& name) {
    MutexLock lock(&mu_);
    std::map<std::string, std::string>::iterator online_it = logged_users_.find(myid);
    if (online_it == logged_users_.end()) {
        return kUnknownUser;
    }
    if (online_it->second != "root" && online_it->second != name) {
        return kPermissionDenied;
    }
    std::map<std::string, UserInfo>::iterator user_it = user_list_.find(online_it->second);
    if (user_it == user_list_.end()) {
        LOG(WARNING, "Try to delete an inexist user :%s", name.c_str());
        return kNotFound;
    }
    if (user_it->second.has_uuid()) {
        logged_users_.erase(user_it->second.uuid());
    }
    user_list_.erase(user_it);
    return kOk;
}

bool UserManager::IsLoggedIn(const std::string& uuid) {
    MutexLock lock(&mu_);
    return logged_users_.find(uuid) != logged_users_.end();
}

bool UserManager::IsValidUser(const std::string& name) {
    MutexLock lock(&mu_);
    return user_list_.find(name) != user_list_.end();
}

Status UserManager::TruncateOnlineUsers(const std::string& myid) {
    MutexLock lock(&mu_);
    std::map<std::string, std::string>::iterator online_it = logged_users_.find(myid);
    if (online_it == logged_users_.end() || online_it->second != "root") {
        return kPermissionDenied;
    }
    logged_users_.clear();
    return kOk;
}

Status UserManager::TruncateAllUsers(const std::string& myid) {
    MutexLock lock(&mu_);
    std::map<std::string, std::string>::iterator online_it = logged_users_.find(myid);
    if (online_it == logged_users_.end() || online_it->second != "root") {
        return kPermissionDenied;
    }
    logged_users_.clear();
    user_list_.clear();
    return kOk;
}

std::string UserManager::GetUsernameFromUuid(const std::string& uuid) {
    if (IsLoggedIn(uuid)) {
		return logged_users_[uuid];
	}
	return "";
}

}
}
