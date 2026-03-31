#include "User.h"

User::User()
    : m_username(), m_password(), m_role()
{
}

User::User(const std::string& username, const std::string& password, const std::string& role)
    : m_username(username), m_password(password), m_role(role)
{
}

const std::string& User::GetUsername() const { return m_username; }
const std::string& User::GetPassword() const { return m_password; }
const std::string& User::GetRole() const { return m_role; }

void User::SetUsername(const std::string& username) { m_username = username; }
void User::SetPassword(const std::string& password) { m_password = password; }
void User::SetRole(const std::string& role) { m_role = role; }

bool User::IsAdmin() const { return m_role == "admin"; }
bool User::IsCreator() const { return m_role == "creator" || m_role == "admin"; }
bool User::IsAssignee() const { return m_role == "assignee" || m_role == "creator" || m_role == "admin"; }
