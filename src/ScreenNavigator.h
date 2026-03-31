#ifndef SCREEN_NAVIGATOR_H
#define SCREEN_NAVIGATOR_H

#include "ScreenType.h"
#include "User.h"
#include <string>
#include <vector>

class ScreenNavigator
{
public:
    ScreenNavigator();

    void SetScreen(ScreenType screen);
    ScreenType GetScreen() const;

    void SetLoggedInUser(const User& user);
    const User& GetLoggedInUser() const;
    bool IsLoggedIn() const;
    void Logout();

    void SetCurrentProject(const std::string& projectName);
    const std::string& GetCurrentProject() const;

    void SetAccessibleProjects(const std::vector<std::string>& projectNames);
    const std::vector<std::string>& GetAccessibleProjects() const;

    void SetSelectedTaskIndex(int index);
    int GetSelectedTaskIndex() const;

    void SetStatusMessage(const std::string& message);
    const std::string& GetStatusMessage() const;
    void ClearStatusMessage();

    void SetErrorMessage(const std::string& error);
    const std::string& GetErrorMessage() const;
    void ClearErrorMessage();

    void ClearBanner();

private:
    ScreenType m_screen;
    bool m_loggedIn;
    User m_loggedInUser;
    std::vector<std::string> m_accessibleProjects;
    std::string m_currentProject;
    int m_selectedTaskIndex;
    std::string m_message;
    std::string m_error;
};

#endif
