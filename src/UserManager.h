#ifndef USER_MANAGER_H
#define USER_MANAGER_H

#include "User.h"
#include <string>
#include <vector>

class UserManager
{
public:
    UserManager();
    explicit UserManager(const std::string& sharedRoot);

    void SetSharedRoot(const std::string& sharedRoot);
    const std::string& GetSharedRoot() const;

    bool Load();
    bool SaveUser(const User& user);
    bool HasUser(const std::string& username) const;
    bool HasAdmin() const;
    bool ValidateLogin(const std::string& username, const std::string& password, User& outUser) const;

    const std::vector<User>& GetUsers() const;

private:
    std::string BuildUserDataPath() const;
    std::string EscapeCsv(const std::string& value) const;
    std::vector<std::string> SplitCsv(const std::string& line) const;
    std::string Trim(const std::string& text) const;

private:
    std::string m_sharedRoot;
    std::vector<User> m_users;
};

#endif
