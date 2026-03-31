#ifndef USER_H
#define USER_H

#include <string>

class User
{
public:
    User();
    User(const std::string& username, const std::string& password, const std::string& role);

    const std::string& GetUsername() const;
    const std::string& GetPassword() const;
    const std::string& GetRole() const;

    void SetUsername(const std::string& username);
    void SetPassword(const std::string& password);
    void SetRole(const std::string& role);

    bool IsAdmin() const;
    bool IsCreator() const;
    bool IsAssignee() const;

private:
    std::string m_username;
    std::string m_password;
    std::string m_role;
};

#endif
