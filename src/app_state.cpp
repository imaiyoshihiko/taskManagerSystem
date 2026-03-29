#include "app_state.h"
#include "storage.h"

#include <windows.h>
#include <iostream>
#include <algorithm>
#include <cstring>

namespace
{
    AppState g_app;

    bool HasAdminUser(const std::vector<UserAccount>& users)
    {
        for (size_t i = 0; i < users.size(); ++i)
        {
            if (users[i].username == "admin" && users[i].role == "admin") return true;
        }
        return false;
    }

    void CopyToBuffer(char* dst, size_t size, const std::string& value)
    {
        if (size == 0) return;
        std::snprintf(dst, size, "%s", value.c_str());
    }

    void EnsureConsole()
    {
        if (GetConsoleWindow() == nullptr)
        {
            AllocConsole();
            FILE* fp;
            freopen_s(&fp, "CONIN$", "r", stdin);
            freopen_s(&fp, "CONOUT$", "w", stdout);
            freopen_s(&fp, "CONOUT$", "w", stderr);
        }
    }

    bool RunInitialCliSetup(AppState& app)
    {
        EnsureConsole();
        std::cout << "=== 初期設定 ===\n";

        std::string sharedRoot;
        if (!Storage::LoadSharedFolderPath(sharedRoot))
        {
            std::cout << "共有フォルダのパスを入力してください: ";
            std::getline(std::cin, sharedRoot);
            if (sharedRoot.empty())
            {
                app.error = "共有フォルダパスが空です。";
                return false;
            }
            if (!Storage::EnsureDirectory(sharedRoot) || !Storage::SaveSharedFolderPath(sharedRoot))
            {
                app.error = "共有フォルダ設定に失敗しました。";
                return false;
            }
        }
        app.sharedRoot = sharedRoot;
        Storage::EnsureDirectory(app.sharedRoot);

        Storage::LoadUsers(app.sharedRoot, app.users);
        if (!HasAdminUser(app.users))
        {
            std::string pass1, pass2;
            std::cout << "admin ユーザの初期登録を行います。\n";
            std::cout << "パスワード: ";
            std::getline(std::cin, pass1);
            std::cout << "パスワード再入力: ";
            std::getline(std::cin, pass2);
            if (pass1.empty() || pass1 != pass2)
            {
                app.error = "admin のパスワード入力が不正です。";
                return false;
            }
            UserAccount admin;
            admin.username = "admin";
            admin.password = pass1;
            admin.role = "admin";
            if (!Storage::AppendUser(app.sharedRoot, admin))
            {
                app.error = "admin ユーザ保存に失敗しました。";
                return false;
            }
            std::cout << "admin を登録しました。\n";
        }
        Storage::LoadUsers(app.sharedRoot, app.users);
        app.cliInitialized = true;
        app.setupComplete = true;
        app.message = "初期設定が完了しました。";
        return true;
    }
}

AppState& GetAppState()
{
    return g_app;
}

bool InitializeApplication()
{
    AppState& app = GetAppState();
    if (!RunInitialCliSetup(app)) return false;
    ReloadProjectsAndUsers();
    return true;
}

void ReloadProjectsAndUsers()
{
    AppState& app = GetAppState();
    Storage::LoadUsers(app.sharedRoot, app.users);
    Storage::LoadProjects(app.sharedRoot, app.projects);

    if (app.loggedIn)
    {
        app.currentAccessibleProjects = Storage::AccessibleProjects(app.sharedRoot, app.currentUser.username, app.currentUser.role);
        if (!app.currentAccessibleProjects.empty())
        {
            if (std::find(app.currentAccessibleProjects.begin(), app.currentAccessibleProjects.end(), app.currentProject) == app.currentAccessibleProjects.end())
            {
                app.currentProject = app.currentAccessibleProjects.front();
            }
        }
        else
        {
            app.currentProject.clear();
        }
    }
}

void ReloadCurrentProjectTasks()
{
    AppState& app = GetAppState();
    app.currentTasks.clear();
    app.currentTaskFileVersion = 1;
    app.selectedTaskIndex = -1;

    if (app.currentProject.empty()) return;

    TaskFileBundle bundle;
    if (Storage::LoadLatestTasks(app.sharedRoot, app.currentProject, bundle))
    {
        app.currentTasks = bundle.tasks;
        app.currentTaskFileVersion = bundle.version;
        app.message = "最新タスクを読み込みました。 version=" + std::to_string(bundle.version);
    }
    else
    {
        app.error = "タスクファイルの読み込みに失敗しました。";
    }
}

void ResetTaskEditorFromSelected(bool createMode)
{
    AppState& app = GetAppState();
    app.createMode = createMode;
    app.editMode = !createMode;

    std::vector<std::string> members;
    Storage::LoadProjectMembers(app.sharedRoot, app.currentProject, members);

    if (createMode)
    {
        int maxId = 0;
        for (size_t i = 0; i < app.currentTasks.size(); ++i)
        {
            maxId = std::max(maxId, app.currentTasks[i].id);
        }
        std::snprintf(app.editId, sizeof(app.editId), "%d", maxId + 1);
        CopyToBuffer(app.editName, sizeof(app.editName), "");
        CopyToBuffer(app.editCategory, sizeof(app.editCategory), "");
        app.editAssigneeIndex = 0;
        CopyToBuffer(app.editEffort, sizeof(app.editEffort), "");
        CopyToBuffer(app.editStartDate, sizeof(app.editStartDate), "2026-03-28");
        CopyToBuffer(app.editEndDate, sizeof(app.editEndDate), "2026-03-28");
        app.editProgress = 0;
        app.editPriorityIndex = 1;
        app.editStatusIndex = 0;
        CopyToBuffer(app.editComment, sizeof(app.editComment), "");
        return;
    }

    if (app.selectedTaskIndex < 0 || app.selectedTaskIndex >= static_cast<int>(app.currentTasks.size())) return;
    const TaskItem& t = app.currentTasks[app.selectedTaskIndex];
    std::snprintf(app.editId, sizeof(app.editId), "%d", t.id);
    CopyToBuffer(app.editName, sizeof(app.editName), t.name);
    CopyToBuffer(app.editCategory, sizeof(app.editCategory), t.category);
    CopyToBuffer(app.editEffort, sizeof(app.editEffort), t.effort);
    CopyToBuffer(app.editStartDate, sizeof(app.editStartDate), t.startDate);
    CopyToBuffer(app.editEndDate, sizeof(app.editEndDate), t.endDate);
    app.editProgress = t.progress;
    CopyToBuffer(app.editComment, sizeof(app.editComment), t.comment);

    static const char* priorities[] = {"Low", "Medium", "High"};
    app.editPriorityIndex = 0;
    for (int i = 0; i < 3; ++i)
    {
        if (t.priority == priorities[i]) app.editPriorityIndex = i;
    }

    static const char* statuses[] = {"Todo", "Doing", "Done", "Blocked"};
    app.editStatusIndex = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (t.status == statuses[i]) app.editStatusIndex = i;
    }

    app.editAssigneeIndex = 0;
    for (size_t i = 0; i < members.size(); ++i)
    {
        if (members[i] == t.assignee)
        {
            app.editAssigneeIndex = static_cast<int>(i);
            break;
        }
    }
}

bool CurrentUserCanEditAllFields()
{
    AppState& app = GetAppState();
    if (!app.loggedIn) return false;
    if (app.currentUser.role == "admin") return true;
    if (app.selectedTaskIndex < 0 || app.selectedTaskIndex >= static_cast<int>(app.currentTasks.size()))
    {
        return app.currentUser.role == "creator";
    }
    const TaskItem& t = app.currentTasks[app.selectedTaskIndex];
    return app.currentUser.role == "creator" || t.creator == app.currentUser.username;
}

bool CurrentUserCanDeleteSelected()
{
    return CurrentUserCanEditAllFields();
}
