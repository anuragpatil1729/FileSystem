#include "terminal_tab.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QKeyEvent>
#include <QScrollBar>
#include <sstream>
#include <iostream>
#include <iomanip>

TerminalTab::TerminalTab(std::shared_ptr<FileSystem> fs,
                         std::shared_ptr<Scheduler> scheduler,
                         std::shared_ptr<MemoryManager> memoryManager,
                         QWidget* parent)
    : QWidget(parent)
    , fs_(std::move(fs))
    , scheduler_(std::move(scheduler))
    , mem_(std::move(memoryManager))
    , historyIndex_(-1)
{
    setupUi();

    appendOutput("Virtual Operating System Environment ready!", "#a6e3a1");
    appendOutput("Type 'help' for available CLI commands, or use quick action chips below.\n", "#89b4fa");
}

void TerminalTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // Quick Command Buttons Bar
    auto* quickBar = new QHBoxLayout();
    quickBar->setSpacing(6);
    quickBar->addWidget(new QLabel("<b>Quick Run:</b>", this));

    auto addChip = [this, quickBar](const QString& cmd) {
        auto* btn = new QPushButton(cmd, this);
        btn->setStyleSheet("QPushButton { background-color: #313244; color: #cdd6f4; border: 1px solid #45475a; border-radius: 4px; padding: 4px 8px; font-size: 11px; } "
                           "QPushButton:hover { background-color: #45475a; }");
        connect(btn, &QPushButton::clicked, [this, cmd]() {
            executeCommand(cmd);
        });
        quickBar->addWidget(btn);
    };

    addChip("help");
    addChip("info");
    addChip("ls /");
    addChip("cpu");
    addChip("mem");
    addChip("mem-map");

    quickBar->addStretch(1);
    clearBtn_ = new QPushButton("Clear Console", this);
    quickBar->addWidget(clearBtn_);
    mainLayout->addLayout(quickBar);

    // Console Output Window
    consoleOutput_ = new QTextEdit(this);
    consoleOutput_->setReadOnly(true);
    QFont terminalFont("Menlo", 11);
    terminalFont.setStyleHint(QFont::Monospace);
    consoleOutput_->setFont(terminalFont);
    consoleOutput_->setStyleSheet("QTextEdit { background-color: #11111b; color: #cdd6f4; border: 1px solid #313244; border-radius: 6px; padding: 10px; }");

    mainLayout->addWidget(consoleOutput_, 1);

    // Command Prompt Input
    auto* inputRow = new QHBoxLayout();
    auto* promptLabel = new QLabel("<b>vfs&gt;</b>", this);
    promptLabel->setStyleSheet("color: #a6e3a1; font-family: Menlo, monospace; font-size: 13px; font-weight: bold;");

    commandInput_ = new QLineEdit(this);
    commandInput_->setFont(terminalFont);
    commandInput_->setPlaceholderText("Enter shell command (e.g. ls /, info, mkdir /demo, sched-run 2)...");
    commandInput_->setStyleSheet("QLineEdit { background-color: #1e1e2e; color: #cdd6f4; border: 1px solid #45475a; border-radius: 6px; padding: 6px 10px; }");
    commandInput_->installEventFilter(this);

    executeBtn_ = new QPushButton("Execute", this);
    executeBtn_->setStyleSheet("QPushButton { background-color: #89b4fa; color: #11111b; font-weight: bold; padding: 6px 16px; }");

    inputRow->addWidget(promptLabel);
    inputRow->addWidget(commandInput_, 1);
    inputRow->addWidget(executeBtn_);
    mainLayout->addLayout(inputRow);

    // Connections
    connect(commandInput_, &QLineEdit::returnPressed, this, &TerminalTab::onCommandEntered);
    connect(executeBtn_, &QPushButton::clicked, this, &TerminalTab::onCommandEntered);
    connect(clearBtn_, &QPushButton::clicked, this, &TerminalTab::onClearTerminal);
}

bool TerminalTab::eventFilter(QObject* obj, QEvent* event) {
    if (obj == commandInput_ && event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Up) {
            if (!history_.isEmpty()) {
                if (historyIndex_ == -1) historyIndex_ = history_.size() - 1;
                else if (historyIndex_ > 0) historyIndex_--;
                commandInput_->setText(history_[historyIndex_]);
            }
            return true;
        } else if (keyEvent->key() == Qt::Key_Down) {
            if (historyIndex_ != -1) {
                if (historyIndex_ < history_.size() - 1) {
                    historyIndex_++;
                    commandInput_->setText(history_[historyIndex_]);
                } else {
                    historyIndex_ = -1;
                    commandInput_->clear();
                }
            }
            return true;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void TerminalTab::appendOutput(const QString& text, const QString& color) {
    QString html = QString("<span style='color: %1;'>%2</span>")
                       .arg(color)
                       .arg(text.toHtmlEscaped().replace("\n", "<br/>"));
    consoleOutput_->append(html);
    consoleOutput_->verticalScrollBar()->setValue(consoleOutput_->verticalScrollBar()->maximum());
}

void TerminalTab::onClearTerminal() {
    consoleOutput_->clear();
}

void TerminalTab::onCommandEntered() {
    QString line = commandInput_->text().trimmed();
    if (line.isEmpty()) return;

    commandInput_->clear();
    history_.append(line);
    historyIndex_ = -1;

    executeCommand(line);
}

void TerminalTab::printHelp() {
    appendOutput(
        "===== VirtualFileSystem Commands =====\n"
        "Filesystem:\n"
        "  help                        Show available commands\n"
        "  ls [path]                   List directory contents\n"
        "  create <path>               Create an empty file\n"
        "  mkdir <path>                Create a directory\n"
        "  write <path> <text>         Write text to a file\n"
        "  read <path>                 Read content from a file\n"
        "  rm <path>                   Delete a regular file\n"
        "  rmdir <path>                Delete an empty directory\n"
        "  info                        Show filesystem geometry and status\n"
        "  format [blocks] [inodes]    Format the virtual disk\n"
        "\nCPU Scheduler:\n"
        "  cpu                         Run CPU scheduler demo with predefined workload\n"
        "  sched-add <name> <burst> [priority] [arrival]   Add process to scheduler\n"
        "  sched-run [quantum]         Execute scheduler with configured/added processes\n"
        "  sched-clear                 Clear all scheduled processes\n"
        "\nVirtual Memory Manager:\n"
        "  mem                         Run virtual memory manager demo\n"
        "  mem-alloc <pid> <bytes>     Allocate virtual memory for a process\n"
        "  mem-free <pid>              Release virtual memory of a process\n"
        "  mem-write <pid> <addr> <val> Write byte to process virtual address\n"
        "  mem-read <pid> <addr>       Read byte from process virtual address\n"
        "  mem-map                     Display physical memory frame allocation map\n"
        "  mem-policy <eager|demand> [fifo|lru] Configure memory policies\n"
        "======================================\n",
        "#f9e2af"
    );
}

void TerminalTab::executeCommand(const QString& cmdLine) {
    appendOutput(QString("vfs> %1").arg(cmdLine), "#89dceb");

    std::istringstream input(cmdLine.toStdString());
    std::string command;
    input >> command;

    if (command == "help") {
        printHelp();
    }
    else if (command == "format") {
        std::size_t blocks = DiskManager::DEFAULT_BLOCK_COUNT;
        std::uint32_t inodes = FileSystem::DEFAULT_INODE_COUNT;
        input >> blocks >> inodes;
        if (fs_->format(blocks, inodes)) {
            appendOutput(QString("Disk formatted successfully (%1 blocks, %2 inodes).").arg(blocks).arg(inodes), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Format failed: %1").arg(QString::fromStdString(fs_->getLastError())), "#f38ba8");
        }
    }
    else if (command == "ls") {
        std::string path = "/";
        input >> path;
        auto entries = fs_->listDirectory(path);
        if (!fs_->isMounted()) {
            appendOutput("Filesystem is not mounted.", "#f38ba8");
        } else if (entries.empty()) {
            appendOutput(QString("Directory is empty or not found: %1").arg(QString::fromStdString(path)), "#fab387");
        } else {
            QString out;
            for (const auto& e : entries) out += QString::fromStdString(e) + "\n";
            appendOutput(out.trimmed(), "#cdd6f4");
        }
    }
    else if (command == "create") {
        std::string path;
        input >> path;
        if (path.empty()) {
            appendOutput("Usage: create <path>", "#f9e2af");
        } else if (fs_->createFile(path)) {
            appendOutput(QString("File created: %1").arg(QString::fromStdString(path)), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to create file: %1").arg(QString::fromStdString(fs_->getLastError())), "#f38ba8");
        }
    }
    else if (command == "mkdir") {
        std::string path;
        input >> path;
        if (path.empty()) {
            appendOutput("Usage: mkdir <path>", "#f9e2af");
        } else if (fs_->createDirectory(path)) {
            appendOutput(QString("Directory created: %1").arg(QString::fromStdString(path)), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to create directory: %1").arg(QString::fromStdString(fs_->getLastError())), "#f38ba8");
        }
    }
    else if (command == "write") {
        std::string path;
        input >> path;
        std::string data;
        std::getline(input >> std::ws, data);
        if (path.empty() || data.empty()) {
            appendOutput("Usage: write <path> <text>", "#f9e2af");
        } else if (fs_->writeFile(path, data)) {
            appendOutput(QString("File written successfully (%1 bytes).").arg(data.size()), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to write file: %1").arg(QString::fromStdString(fs_->getLastError())), "#f38ba8");
        }
    }
    else if (command == "read") {
        std::string path;
        input >> path;
        if (path.empty()) {
            appendOutput("Usage: read <path>", "#f9e2af");
        } else if (!fs_->exists(path)) {
            appendOutput(QString("File not found: %1").arg(QString::fromStdString(path)), "#f38ba8");
        } else {
            std::string content = fs_->readFile(path);
            appendOutput(QString::fromStdString(content), "#cdd6f4");
        }
    }
    else if (command == "rm") {
        std::string path;
        input >> path;
        if (path.empty()) {
            appendOutput("Usage: rm <path>", "#f9e2af");
        } else if (fs_->deleteFile(path)) {
            appendOutput(QString("File deleted: %1").arg(QString::fromStdString(path)), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to delete file: %1").arg(QString::fromStdString(fs_->getLastError())), "#f38ba8");
        }
    }
    else if (command == "rmdir") {
        std::string path;
        input >> path;
        if (path.empty()) {
            appendOutput("Usage: rmdir <path>", "#f9e2af");
        } else if (fs_->deleteDirectory(path)) {
            appendOutput(QString("Directory deleted: %1").arg(QString::fromStdString(path)), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to delete directory: %1").arg(QString::fromStdString(fs_->getLastError())), "#f38ba8");
        }
    }
    else if (command == "info") {
        std::stringstream ss;
        auto* old = std::cout.rdbuf(ss.rdbuf());
        fs_->printInfo();
        std::cout.rdbuf(old);
        appendOutput(QString::fromStdString(ss.str()), "#fab387");
    }
    else if (command == "cpu") {
        scheduler_->clear();
        scheduler_->setTimeQuantum(2);
        scheduler_->createProcess("Process-A", 5, 1, 0);
        scheduler_->createProcess("Process-B", 3, 1, 0);
        scheduler_->run();

        QString out = "========== CPU SCHEDULER ==========\n";
        out += "Algorithm: Round Robin (Simulated CPU Time)\nTime quantum: 2 simulated time units\n\n";
        out += "Gantt Timeline:\n";
        for (const auto& slice : scheduler_->getTimeline()) {
            out += QString("| P%1 [%2-%3] ").arg(slice.pid).arg(slice.startTime).arg(slice.endTime);
        }
        out += "|\n\nExecution order: ";
        for (const auto pid : scheduler_->getExecutionLog()) {
            out += QString("P%1 ").arg(pid);
        }
        out += QString("\n\nAverage waiting time: %1\nAverage turnaround time: %2\nCPU Utilization: %3%\n")
                   .arg(scheduler_->getAverageWaitingTime(), 0, 'f', 2)
                   .arg(scheduler_->getAverageTurnaroundTime(), 0, 'f', 2)
                   .arg(scheduler_->getCpuUtilization(), 0, 'f', 2);
        out += "===================================\n";
        appendOutput(out, "#a6e3a1");
        emit systemStateChanged();
    }
    else if (command == "sched-add") {
        std::string name;
        std::size_t burst = 0;
        int priority = 0;
        std::size_t arrival = 0;
        input >> name >> burst;
        if (input >> priority) input >> arrival;
        if (name.empty() || burst == 0) {
            appendOutput("Usage: sched-add <name> <burst> [priority] [arrival]", "#f9e2af");
        } else {
            auto pid = scheduler_->createProcess(name, burst, priority, arrival);
            appendOutput(QString("Process created: PID %1 (%2, burst=%3, priority=%4, arrival=%5)")
                             .arg(pid).arg(QString::fromStdString(name)).arg(burst).arg(priority).arg(arrival), "#a6e3a1");
            emit systemStateChanged();
        }
    }
    else if (command == "sched-run") {
        std::size_t q = 0;
        if (input >> q && q > 0) {
            scheduler_->setTimeQuantum(q);
        }
        if (!scheduler_->hasProcesses()) {
            appendOutput("No processes in scheduler. Use 'sched-add' or 'cpu'.", "#fab387");
        } else {
            scheduler_->run();
            appendOutput("Scheduler execution completed.", "#a6e3a1");
            emit systemStateChanged();
        }
    }
    else if (command == "sched-clear") {
        scheduler_->clear();
        appendOutput("Scheduler queue cleared.", "#a6e3a1");
        emit systemStateChanged();
    }
    else if (command == "mem") {
        std::stringstream ss;
        auto* old = std::cout.rdbuf(ss.rdbuf());

        mem_->setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
        mem_->setReplacementPolicy(ReplacementPolicy::LRU);
        mem_->allocateProcess(1, 12 * 1024);
        mem_->write(1, 4200, 0xAB);
        std::uint8_t val = 0;
        mem_->read(1, 4200, val);
        std::cout.rdbuf(old);

        QString out = "========== MEMORY MANAGER ==========\n";
        out += "Policy: Demand Paging with LRU replacement\n";
        out += "Allocated 12288 virtual bytes for process 1.\n";
        out += "Wrote byte 0xAB to virtual address 4200 (triggered page fault)\n";
        out += QString("Read byte 0x%1 from virtual address 4200\n").arg(val, 2, 16, QLatin1Char('0'));
        out += QString("Virtual address 4200 -> Physical address %1\n").arg(mem_->translateAddress(1, 4200));
        out += QString("Page faults: %1, Evictions: %2\n").arg(mem_->getPageFaultCount()).arg(mem_->getPageReplacementCount());
        out += "====================================\n";
        appendOutput(out, "#a6e3a1");
        emit systemStateChanged();
    }
    else if (command == "mem-alloc") {
        std::uint32_t pid = 0;
        std::size_t bytes = 0;
        if (!(input >> pid >> bytes) || bytes == 0) {
            appendOutput("Usage: mem-alloc <pid> <bytes>", "#f9e2af");
        } else if (mem_->allocateProcess(pid, bytes)) {
            appendOutput(QString("Allocated %1 virtual bytes for PID %2.").arg(bytes).arg(pid), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to allocate memory for PID %1.").arg(pid), "#f38ba8");
        }
    }
    else if (command == "mem-free") {
        std::uint32_t pid = 0;
        if (!(input >> pid)) {
            appendOutput("Usage: mem-free <pid>", "#f9e2af");
        } else if (mem_->freeProcess(pid)) {
            appendOutput(QString("Released memory for PID %1.").arg(pid), "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput(QString("Failed to release memory for PID %1 (not found).").arg(pid), "#f38ba8");
        }
    }
    else if (command == "mem-write") {
        std::uint32_t pid = 0;
        std::size_t addr = 0;
        int val = 0;
        if (!(input >> pid >> addr >> val)) {
            appendOutput("Usage: mem-write <pid> <virtual_addr> <value>", "#f9e2af");
        } else if (mem_->write(pid, addr, static_cast<std::uint8_t>(val))) {
            appendOutput("Write successful.", "#a6e3a1");
            emit systemStateChanged();
        } else {
            appendOutput("Write failed (invalid PID, out of bounds, or memory full).", "#f38ba8");
        }
    }
    else if (command == "mem-read") {
        std::uint32_t pid = 0;
        std::size_t addr = 0;
        if (!(input >> pid >> addr)) {
            appendOutput("Usage: mem-read <pid> <virtual_addr>", "#f9e2af");
        } else {
            std::uint8_t val = 0;
            if (mem_->read(pid, addr, val)) {
                appendOutput(QString("Byte at virtual address %1: %2 (0x%3)")
                                 .arg(addr).arg(static_cast<int>(val)).arg(val, 2, 16, QLatin1Char('0')), "#a6e3a1");
            } else {
                appendOutput("Read failed (invalid PID or out of bounds).", "#f38ba8");
            }
        }
    }
    else if (command == "mem-map") {
        std::stringstream ss;
        auto* old = std::cout.rdbuf(ss.rdbuf());
        mem_->printMemoryMap();
        std::cout.rdbuf(old);
        appendOutput(QString::fromStdString(ss.str()), "#89b4fa");
        appendOutput(QString("Free memory: %1 / %2 bytes | Page faults: %3, Evictions: %4")
                         .arg(mem_->getFreeMemory()).arg(mem_->getPhysicalMemorySize())
                         .arg(mem_->getPageFaultCount()).arg(mem_->getPageReplacementCount()), "#fab387");
    }
    else if (command == "mem-policy") {
        std::string allocP, replP;
        input >> allocP >> replP;
        if (allocP == "demand") {
            mem_->setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
            appendOutput("Allocation policy set to DEMAND_PAGING.", "#a6e3a1");
        } else if (allocP == "eager") {
            mem_->setAllocationPolicy(AllocationPolicy::EAGER);
            appendOutput("Allocation policy set to EAGER.", "#a6e3a1");
        }
        if (replP == "lru") {
            mem_->setReplacementPolicy(ReplacementPolicy::LRU);
            appendOutput("Replacement policy set to LRU.", "#a6e3a1");
        } else if (replP == "fifo") {
            mem_->setReplacementPolicy(ReplacementPolicy::FIFO);
            appendOutput("Replacement policy set to FIFO.", "#a6e3a1");
        }
        emit systemStateChanged();
    }
    else {
        appendOutput(QString("Unknown command '%1'. Type 'help' for commands.").arg(QString::fromStdString(command)), "#f38ba8");
    }
}
