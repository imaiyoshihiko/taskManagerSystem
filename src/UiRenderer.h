#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "UserManager.h"
#include "ProjectManager.h"
#include "TaskManager.h"
#include "ScreenNavigator.h"

class UiRenderer
{
public:
    UiRenderer(UserManager& userManager,
               ProjectManager& projectManager,
               TaskManager& taskManager,
               ScreenNavigator& navigator);

    void Render();
    bool InitializeFromCli();

private:
    struct SimpleDate
    {
        int year;
        int month;
        int day;
    };

    struct TaskEditBuffer
    {
        char id[32];
        char name[128];
        char category[128];
        int assigneeIndex;
        char effort[64];
        char startDate[32];
        char endDate[32];
        int progress;
        int priorityIndex;
        int statusIndex;
        char comment[512];
        bool createMode;

        TaskEditBuffer();
    };

private:
    void RenderLoginScreen();
    void RenderRegisterScreen();
    void RenderAdminScreen();
    void RenderTaskListScreen();
    void RenderTaskDetailScreen();
    void RenderTaskEditScreen();
    void RenderProjectSwitchScreen();
    void RenderBanner();
    void ReloadAfterLogin();
    void ResetTaskEditBuffer(bool createMode);
    void ApplyTaskEdit(Task& task);
    bool SaveTaskList(const std::vector<Task>& tasks);
    bool CurrentUserCanEditAllFields() const;
    bool CurrentUserCanDeleteSelected() const;
    int ClampInt(int value, int minValue, int maxValue) const;
    bool ParseDate(const std::string& text, SimpleDate& out) const;
    time_t ToTimeT(const SimpleDate& value) const;
    int DaysBetween(const SimpleDate& a, const SimpleDate& b) const;
    SimpleDate AddDays(const SimpleDate& value, int days) const;

private:
    UserManager& m_userManager;
    ProjectManager& m_projectManager;
    TaskManager& m_taskManager;
    ScreenNavigator& m_navigator;

    char m_loginName[128];
    char m_loginPassword[128];
    int m_registerRoleIndex;
    int m_registerUserIndex;
    char m_registerPassword1[128];
    char m_registerPassword2[128];
    char m_newProjectName[128];
    char m_memberNameInput[128];
    int m_memberProjectIndex;
    int m_filterIndex;
    bool m_showGantt;
    TaskEditBuffer m_taskEdit;
};

#endif
