#include "mainwindow.hpp"
#include "filesystem_tab.hpp"
#include "scheduler_tab.hpp"
#include "memory_tab.hpp"
#include "terminal_tab.hpp"

#include <QStatusBar>
#include <QMenuBar>
#include <QMessageBox>
#include <QApplication>
#include <fstream>

namespace {
bool fileExists(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    return f.is_open();
}
} // namespace

MainWindow::MainWindow(const std::string& diskPath, QWidget* parent)
    : QMainWindow(parent)
    , diskPath_(diskPath)
{
    initCoreSubsystems();
    setupUi();
    setupStyle();
    showStatusMessage("VirtualFileSystem Environment initialized successfully.");
}

MainWindow::~MainWindow() {
    if (fs_ && fs_->isMounted()) {
        fs_->unmount();
    }
}

void MainWindow::initCoreSubsystems() {
    fs_ = std::make_shared<FileSystem>(diskPath_);

    if (fileExists(diskPath_)) {
        if (!fs_->mount()) {
            // Mount failure
        }
    } else {
        fs_->format();
    }

    scheduler_ = std::make_shared<Scheduler>(2);
    memoryManager_ = std::make_shared<MemoryManager>(64 * 1024);
}

void MainWindow::setupUi() {
    setWindowTitle("VFS Studio - Virtual Operating System (C++17)");
    resize(1100, 750);
    setMinimumSize(900, 600);

    // Menu Bar
    auto* menu = menuBar();
    auto* fileMenu = menu->addMenu("&File");
    auto* refreshAct = fileMenu->addAction("🔄 &Refresh All Subsystems");
    refreshAct->setShortcut(QKeySequence::Refresh);
    connect(refreshAct, &QAction::triggered, this, &MainWindow::onSystemStateChanged);

    fileMenu->addSeparator();
    auto* exitAct = fileMenu->addAction("&Exit");
    exitAct->setShortcut(QKeySequence::Quit);
    connect(exitAct, &QAction::triggered, this, &QWidget::close);

    auto* helpMenu = menu->addMenu("&Help");
    auto* aboutAct = helpMenu->addAction("&About VirtualFileSystem");
    connect(aboutAct, &QAction::triggered, this, &MainWindow::onAbout);

    // Central Tab Widget
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setDocumentMode(true);

    fsTab_ = new FileSystemTab(fs_, this);
    schedTab_ = new SchedulerTab(scheduler_, this);
    memTab_ = new MemoryTab(memoryManager_, this);
    termTab_ = new TerminalTab(fs_, scheduler_, memoryManager_, this);

    tabWidget_->addTab(fsTab_, "🗄️ Filesystem Explorer");
    tabWidget_->addTab(schedTab_, "⚡ CPU Scheduler");
    tabWidget_->addTab(memTab_, "🧠 Virtual Memory");
    tabWidget_->addTab(termTab_, "💻 Interactive Shell");

    setCentralWidget(tabWidget_);

    // Status Bar
    auto* bar = statusBar();
    statusDiskLabel_ = new QLabel(this);
    if (fs_->isMounted()) {
        statusDiskLabel_->setText(QString("  🟢 Virtual Disk: %1 (Mounted)").arg(QString::fromStdString(diskPath_)));
    } else {
        statusDiskLabel_->setText(QString("  🔴 Virtual Disk: %1 (Unmounted)").arg(QString::fromStdString(diskPath_)));
    }

    statusMsgLabel_ = new QLabel("Ready", this);
    bar->addWidget(statusDiskLabel_);
    bar->addPermanentWidget(statusMsgLabel_, 1);

    // Inter-tab connections
    connect(termTab_, &TerminalTab::systemStateChanged, this, &MainWindow::onSystemStateChanged);
    connect(fsTab_, &FileSystemTab::fileSystemChanged, this, &MainWindow::onSystemStateChanged);

    connect(fsTab_, &FileSystemTab::statusMessage, this, &MainWindow::showStatusMessage);
    connect(schedTab_, &SchedulerTab::statusMessage, this, &MainWindow::showStatusMessage);
    connect(memTab_, &MemoryTab::statusMessage, this, &MainWindow::showStatusMessage);
    connect(termTab_, &TerminalTab::statusMessage, this, &MainWindow::showStatusMessage);
}

void MainWindow::setupStyle() {
    // Modern Dark Theme (Catppuccin Mocha inspired)
    QString qss = R"(
        QMainWindow {
            background-color: #1e1e2e;
            color: #cdd6f4;
        }
        QWidget {
            color: #cdd6f4;
            font-family: "Helvetica Neue", Arial, sans-serif;
            font-size: 13px;
        }
        QTabWidget::pane {
            border: 1px solid #313244;
            background-color: #181825;
            border-radius: 6px;
        }
        QTabBar::tab {
            background-color: #1e1e2e;
            color: #a6adc8;
            padding: 10px 18px;
            margin-right: 2px;
            border-top-left-radius: 6px;
            border-top-right-radius: 6px;
            font-weight: bold;
        }
        QTabBar::tab:selected {
            background-color: #181825;
            color: #89b4fa;
            border-top: 2px solid #89b4fa;
        }
        QTabBar::tab:hover:!selected {
            background-color: #252538;
            color: #cdd6f4;
        }
        QGroupBox {
            border: 1px solid #313244;
            border-radius: 6px;
            margin-top: 14px;
            padding-top: 14px;
            font-weight: bold;
            color: #89b4fa;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 6px;
            left: 10px;
        }
        QPushButton {
            background-color: #313244;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 6px 14px;
            font-weight: 500;
        }
        QPushButton:hover {
            background-color: #45475a;
        }
        QPushButton:pressed {
            background-color: #585b70;
        }
        QPushButton:disabled {
            background-color: #181825;
            color: #6c7086;
            border-color: #313244;
        }
        QLineEdit, QSpinBox, QComboBox {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 1px solid #45475a;
            border-radius: 6px;
            padding: 5px 8px;
            selection-background-color: #89b4fa;
            selection-color: #11111b;
        }
        QLineEdit:focus, QSpinBox:focus, QComboBox:focus {
            border: 1px solid #89b4fa;
        }
        QTreeWidget, QTableWidget, QTextEdit {
            background-color: #181825;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 6px;
            gridline-color: #313244;
            selection-background-color: #45475a;
            selection-color: #cdd6f4;
        }
        QHeaderView::section {
            background-color: #1e1e2e;
            color: #a6adc8;
            padding: 6px;
            border: 1px solid #313244;
            font-weight: bold;
        }
        QProgressBar {
            background-color: #1e1e2e;
            border: 1px solid #313244;
            border-radius: 4px;
            text-align: center;
            color: #cdd6f4;
            font-weight: bold;
            height: 18px;
        }
        QProgressBar::chunk {
            background-color: #89b4fa;
            border-radius: 3px;
        }
        QStatusBar {
            background-color: #11111b;
            color: #a6adc8;
            border-top: 1px solid #313244;
        }
        QMenuBar {
            background-color: #181825;
            color: #cdd6f4;
            border-bottom: 1px solid #313244;
        }
        QMenuBar::item:selected {
            background-color: #313244;
            border-radius: 4px;
        }
        QMenu {
            background-color: #1e1e2e;
            color: #cdd6f4;
            border: 1px solid #313244;
            border-radius: 6px;
            padding: 4px;
        }
        QMenu::item:selected {
            background-color: #45475a;
            border-radius: 4px;
        }
    )";
    qApp->setStyleSheet(qss);
}

void MainWindow::onSystemStateChanged() {
    fsTab_->refresh();
    schedTab_->refresh();
    memTab_->refresh();

    if (fs_->isMounted()) {
        statusDiskLabel_->setText(QString("  🟢 Virtual Disk: %1 (Mounted)").arg(QString::fromStdString(diskPath_)));
    } else {
        statusDiskLabel_->setText(QString("  🔴 Virtual Disk: %1 (Unmounted)").arg(QString::fromStdString(diskPath_)));
    }
}

void MainWindow::showStatusMessage(const QString& msg) {
    statusMsgLabel_->setText(msg);
}

void MainWindow::onAbout() {
    QMessageBox::about(this, "About VirtualFileSystem",
                       "<h3>VirtualFileSystem Studio</h3>"
                       "<p>A modern user-space operating system simulator in <b>C++17 & Qt 6</b>.</p>"
                       "<ul>"
                       "<li><b>Block-based Filesystem:</b> Inodes, bitmaps, hierarchical paths, multi-block files</li>"
                       "<li><b>CPU Virtualization:</b> Round-Robin scheduler with interactive Gantt timeline</li>"
                       "<li><b>Virtual Memory:</b> Demand paging, FIFO/LRU page replacement, frame map</li>"
                       "<li><b>Interactive Shell:</b> Command line integration with full feature parity</li>"
                       "</ul>");
}
