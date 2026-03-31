#include "ScreenNavigator.h"

ScreenNavigator::ScreenNavigator()
    : m_screen(ScreenType::Login),
      m_loggedIn(false),
      m_currentProject(""),
      m_selectedTaskIndex(-1)
{
}

void ScreenNavigator::SetScreen(ScreenType screen)
{
    m_screen = screen;
}

ScreenType ScreenNavigator::GetScreen() const
{
    return m_screen;
}

void ScreenNavigator::SetLoggedInUser(const User& user)
{
    m_loggedInUser = user;
    m_loggedIn = true;
}

const User& ScreenNavigator::GetLoggedInUser() const
{
    return m_loggedInUser;
}

bool ScreenNavigator::IsLoggedIn() const
{
    return m_loggedIn;
}

void ScreenNavigator::Logout()
{
    m_loggedIn = false;
    m_loggedInUser = User();
    m_accessibleProjects.clear();
    m_currentProject.clear();
    m_selectedTaskIndex = -1;
    ClearBanner();
    m_screen = ScreenType::Login;
}

void ScreenNavigator::SetCurrentProject(const std::string& projectName)
{
    m_currentProject = projectName;
}

const std::string& ScreenNavigator::GetCurrentProject() const
{
    return m_currentProject;
}

void ScreenNavigator::SetAccessibleProjects(const std::vector<std::string>& projectNames)
{
    m_accessibleProjects = projectNames;
}

const std::vector<std::string>& ScreenNavigator::GetAccessibleProjects() const
{
    return m_accessibleProjects;
}

void ScreenNavigator::SetSelectedTaskIndex(int index)
{
    m_selectedTaskIndex = index;
}

int ScreenNavigator::GetSelectedTaskIndex() const
{
    return m_selectedTaskIndex;
}

void ScreenNavigator::SetStatusMessage(const std::string& message)
{
    m_message = message;
}

const std::string& ScreenNavigator::GetStatusMessage() const
{
    return m_message;
}

void ScreenNavigator::ClearStatusMessage()
{
    m_message.clear();
}

void ScreenNavigator::SetErrorMessage(const std::string& error)
{
    m_error = error;
}

const std::string& ScreenNavigator::GetErrorMessage() const
{
    return m_error;
}

void ScreenNavigator::ClearErrorMessage()
{
    m_error.clear();
}

void ScreenNavigator::ClearBanner()
{
    m_message.clear();
    m_error.clear();
}
