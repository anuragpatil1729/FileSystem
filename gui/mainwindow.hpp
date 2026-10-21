#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "filesystem.hpp"
#include "scheduler.hpp"
#include "memory_manager.hpp"

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <memory>

class FileSystemTab;
class SchedulerTab;
class MemoryTab;
class TerminalTab;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const std::string& diskPath = "data/virtual_disk.bin", QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSystemStateChanged();
    void showStatusMessage(const QString& msg);
    void onAbout();

private:
    std::string diskPath_;
    std::shared_ptr<FileSystem> fs_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<MemoryManager> memoryManager_;

    QTabWidget* tabWidget_;
    FileSystemTab* fsTab_;
    SchedulerTab* schedTab_;
    MemoryTab* memTab_;
    TerminalTab* termTab_;

    QLabel* statusDiskLabel_;
    QLabel* statusMsgLabel_;

    void initCoreSubsystems();
    void setupUi();
    void setupStyle();
};

#endif // MAINWINDOW_HPP
