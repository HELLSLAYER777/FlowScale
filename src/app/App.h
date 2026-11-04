// FlowScale — application shell (owns the main window; kept tiny on purpose).
#pragma once
#include "app/MainWindow.h"

namespace fs {

class App {
public:
    int Run(HINSTANCE hInst) {
        if (!wnd_.Create(hInst)) return 1;
        return wnd_.Run();
    }

private:
    MainWindow wnd_;
};

} // namespace fs
