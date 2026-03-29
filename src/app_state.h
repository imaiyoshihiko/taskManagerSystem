#ifndef TASK_MANAGER_APP_STATE_H
#define TASK_MANAGER_APP_STATE_H

#include "models.h"
#include <string>
#include <vector>

enum class ScreenType
{
    Login,
    Register,
    Admin,
    TaskList,
    TaskDetail,
    TaskEdit,
    ProjectSwitch
};

struct AppState
{
    bool cliInitialized = false;
    bool setupComplete = false;

    std::string sharedRoot;
    std::vector<UserAccount> users;
    std::vector<ProjectInfo> projects;

    bool loggedIn = false;
    UserAccount currentUser;
    std::vector<std::string> currentAccessibleProjects;
    std::string currentProject;

    std::vector<TaskItem> currentTasks;
    int currentTaskFileVersion = 1;
    int selectedTaskIndex = -1;
    std::string filterAssignee;

    ScreenType screen = ScreenType::Login;
    std::string message;
    std::string error;

    bool showGantt = false;

    char loginName[128] = "";
    char loginPassword[128] = "";

    int registerRoleIndex = 1;
    int registerUserIndex = 0;
    char registerPassword1[128] = "";
    char registerPassword2[128] = "";

    char newProjectName[128] = "";
    char memberNameInput[128] = "";
    int memberProjectIndex = 0;

    char editId[32] = "0";
    char editName[128] = "";
    char editCategory[128] = "";
    int editAssigneeIndex = 0;
    char editEffort[64] = "";
    char editStartDate[32] = "2026-03-28";
    char editEndDate[32] = "2026-03-28";
    int editProgress = 0;
    int editPriorityIndex = 1;
    int editStatusIndex = 0;
    char editComment[512] = "";
    bool editMode = false;
    bool createMode = false;
};

AppState& GetAppState();
bool InitializeApplication();
void ReloadProjectsAndUsers();
void ReloadCurrentProjectTasks();
void ResetTaskEditorFromSelected(bool createMode);
bool CurrentUserCanEditAllFields();
bool CurrentUserCanDeleteSelected();

#endif
