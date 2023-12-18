#ifndef SERVER_PROFILE_H_
#define SERVER_PROFILE_H_ 1

#include <string>
#include <set>
#include <map>
#include "../shared/channel.h"

class Profile {
    public:
        std::string username;
        std::set<std::string> followers;
        uint8_t session_count;
};

class ProfileManager {
    private:
        std::map<Address, std::string> session_addresses;
        std::map<std::string, Profile> profiles;
};

#endif
