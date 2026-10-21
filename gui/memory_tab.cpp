#include "memory_tab.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <sstream>
#include <iostream>
#include <regex>

MemoryTab::MemoryTab(std::shared_ptr<MemoryManager> memoryManager, QWidget* parent)
    : QWidget(parent)
    , mem_(std::move(memoryManager))
{
    setupUi();
    refresh();
}

void MemoryTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // 1. Policy & Demo Toolbar
    auto* policyBox = new QGroupBox("Memory Management & Page Replacement Policies", this);
    auto* policyLayout = new QHBoxLayout(policyBox);

    policyLayout->addWidget(new QLabel("<b>Allocation Policy:</b>", this));
    allocPolicyCombo_ = new QComboBox(this);
    allocPolicyCombo_->addItem("Demand Paging (On-demand frame allocation)", static_cast<int>(AllocationPolicy::DEMAND_PAGING));
    allocPolicyCombo_->addItem("Eager Allocation (Pre-allocate all frames)", static_cast<int>(AllocationPolicy::EAGER));
    policyLayout->addWidget(allocPolicyCombo_, 1);

    policyLayout->addWidget(new QLabel("<b>Replacement Policy:</b>", this));
    replPolicyCombo_ = new QComboBox(this);
    replPolicyCombo_->addItem("LRU (Least Recently Used)", static_cast<int>(ReplacementPolicy::LRU));
    replPolicyCombo_->addItem("FIFO (First In, First Out)", static_cast<int>(ReplacementPolicy::FIFO));
    policyLayout->addWidget(replPolicyCombo_, 1);

    runDemoBtn_ = new QPushButton("🧪 Run Memory Demo", this);
    runDemoBtn_->setStyleSheet("QPushButton { background-color: #89b4fa; color: #11111b; font-weight: bold; }");
    resetStatsBtn_ = new QPushButton("🔄 Reset Counters", this);

    policyLayout->addWidget(runDemoBtn_);
    policyLayout->addWidget(resetStatsBtn_);
    mainLayout->addWidget(policyBox);

    // 2. Middle Grid: Left = Allocation & Simulation, Right = Activity Log
    auto* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(12);

    auto* leftControls = new QVBoxLayout();

    // Process Virtual Memory Allocation Box
    auto* allocBox = new QGroupBox("Process Virtual Address Space", this);
    auto* allocGrid = new QGridLayout(allocBox);

    allocPidSpin_ = new QSpinBox(this);
    allocPidSpin_->setRange(1, 100);
    allocPidSpin_->setValue(1);

    allocSizeSpin_ = new QSpinBox(this);
    allocSizeSpin_->setRange(1024, 64 * 1024);
    allocSizeSpin_->setSingleStep(4096);
    allocSizeSpin_->setValue(16384);
    allocSizeSpin_->setSuffix(" bytes (4 pages)");

    allocBtn_ = new QPushButton("Allocate Memory", this);
    allocBtn_->setStyleSheet("QPushButton { background-color: #a6e3a1; color: #11111b; font-weight: bold; }");

    freeBtn_ = new QPushButton("Free Process", this);
    freeBtn_->setStyleSheet("QPushButton { background-color: #f38ba8; color: #11111b; font-weight: bold; }");

    allocGrid->addWidget(new QLabel("Target PID:", this), 0, 0);
    allocGrid->addWidget(allocPidSpin_, 0, 1);
    allocGrid->addWidget(new QLabel("Virtual Size:", this), 1, 0);
    allocGrid->addWidget(allocSizeSpin_, 1, 1);
    allocGrid->addWidget(allocBtn_, 2, 0);
    allocGrid->addWidget(freeBtn_, 2, 1);
    leftControls->addWidget(allocBox);

    // Memory Access & Translation Simulator Box
    auto* accessBox = new QGroupBox("Virtual Address R/W & Address Translation", this);
    auto* accessGrid = new QGridLayout(accessBox);

    accessPidSpin_ = new QSpinBox(this);
    accessPidSpin_->setRange(1, 100);
    accessPidSpin_->setValue(1);

    accessAddrSpin_ = new QSpinBox(this);
    accessAddrSpin_->setRange(0, 65535);
    accessAddrSpin_->setValue(4200);

    accessValEdit_ = new QLineEdit("0xAB", this);
    accessValEdit_->setPlaceholderText("Byte (e.g. 0xAB or 171)");

    writeBtn_ = new QPushButton("✍ Write Byte", this);
    readBtn_ = new QPushButton("📖 Read Byte", this);
    translateBtn_ = new QPushButton("🔍 Translate", this);

    accessGrid->addWidget(new QLabel("Process PID:", this), 0, 0);
    accessGrid->addWidget(accessPidSpin_, 0, 1);
    accessGrid->addWidget(new QLabel("Virtual Address:", this), 1, 0);
    accessGrid->addWidget(accessAddrSpin_, 1, 1);
    accessGrid->addWidget(new QLabel("Data Byte (Val):", this), 2, 0);
    accessGrid->addWidget(accessValEdit_, 2, 1);

    auto* btnRow = new QHBoxLayout();
    btnRow->addWidget(writeBtn_);
    btnRow->addWidget(readBtn_);
    btnRow->addWidget(translateBtn_);
    accessGrid->addLayout(btnRow, 3, 0, 1, 2);

    leftControls->addWidget(accessBox);
    middleLayout->addLayout(leftControls, 4);

    // Right: Real-time Memory Operation Log
    auto* logBox = new QGroupBox("Memory Operations & Paging Log", this);
    auto* logLayout = new QVBoxLayout(logBox);

    logEdit_ = new QTextEdit(this);
    logEdit_->setReadOnly(true);
    QFont monoFont("Menlo", 10);
    monoFont.setStyleHint(QFont::Monospace);
    logEdit_->setFont(monoFont);
    logLayout->addWidget(logEdit_);

    middleLayout->addWidget(logBox, 5);
    mainLayout->addLayout(middleLayout);

    // 3. Physical Frame Visualizer & Stats
    auto* frameBox = new QGroupBox("Physical Memory Frame Allocation Grid (4 KB Frames)", this);
    auto* frameLayout = new QVBoxLayout(frameBox);

    frameMapWidget_ = new FrameMapWidget(this);
    frameLayout->addWidget(frameMapWidget_);

    auto* statsRow = new QHBoxLayout();
    freeMemLabel_ = new QLabel("Physical Memory: -", this);
    freeMemBar_ = new QProgressBar(this);
    freeMemBar_->setTextVisible(true);

    pageFaultsLabel_ = new QLabel("Page Faults: 0", this);
    pageFaultsLabel_->setStyleSheet("color: #fab387; font-weight: bold; font-size: 13px;");

    pageReplacementsLabel_ = new QLabel("Evictions / Replacements: 0", this);
    pageReplacementsLabel_->setStyleSheet("color: #f38ba8; font-weight: bold; font-size: 13px;");

    statsRow->addWidget(freeMemLabel_);
    statsRow->addWidget(freeMemBar_, 1);
    statsRow->addSpacing(20);
    statsRow->addWidget(pageFaultsLabel_);
    statsRow->addSpacing(15);
    statsRow->addWidget(pageReplacementsLabel_);

    frameLayout->addLayout(statsRow);
    mainLayout->addWidget(frameBox);

    // Connections
    connect(allocPolicyCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MemoryTab::onPolicyChanged);
    connect(replPolicyCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MemoryTab::onPolicyChanged);
    connect(runDemoBtn_, &QPushButton::clicked, this, &MemoryTab::onRunDemo);
    connect(resetStatsBtn_, &QPushButton::clicked, this, &MemoryTab::onResetStats);
    connect(allocBtn_, &QPushButton::clicked, this, &MemoryTab::onAllocateProcess);
    connect(freeBtn_, &QPushButton::clicked, this, &MemoryTab::onFreeProcess);
    connect(writeBtn_, &QPushButton::clicked, this, &MemoryTab::onWriteMemory);
    connect(readBtn_, &QPushButton::clicked, this, &MemoryTab::onReadMemory);
    connect(translateBtn_, &QPushButton::clicked, this, &MemoryTab::onTranslateAddress);
}

void MemoryTab::refresh() {
    updateFramesAndStats();
}

void MemoryTab::logMessage(const QString& msg) {
    logEdit_->append(msg);
}

void MemoryTab::updateFramesAndStats() {
    if (!mem_) return;

    // Capture memory map string
    std::stringstream ss;
    auto* oldBuf = std::cout.rdbuf(ss.rdbuf());
    mem_->printMemoryMap();
    std::cout.rdbuf(oldBuf);

    std::string output = ss.str();
    std::istringstream stream(output);
    std::string line;

    std::vector<FrameDisplayInfo> frames;
    // Format: "Frame X: USED (PID Y, VPage Z)" or "Frame X: FREE"
    std::regex usedRegex(R"(Frame\s+(\d+):\s+USED\s+\(PID\s+(\d+),\s+VPage\s+(\d+)\))");
    std::regex freeRegex(R"(Frame\s+(\d+):\s+FREE)");

    while (std::getline(stream, line)) {
        std::smatch match;
        if (std::regex_search(line, match, usedRegex)) {
            FrameDisplayInfo info;
            info.frameIndex = std::stoul(match[1]);
            info.used = true;
            info.pid = static_cast<std::uint32_t>(std::stoul(match[2]));
            info.virtualPage = std::stoul(match[3]);
            frames.push_back(info);
        } else if (std::regex_search(line, match, freeRegex)) {
            FrameDisplayInfo info;
            info.frameIndex = std::stoul(match[1]);
            info.used = false;
            info.pid = 0;
            info.virtualPage = 0;
            frames.push_back(info);
        }
    }

    frameMapWidget_->setFrames(frames);

    const std::size_t totalMem = mem_->getPhysicalMemorySize();
    const std::size_t freeMem = mem_->getFreeMemory();
    const std::size_t usedMem = totalMem - freeMem;

    freeMemLabel_->setText(QString("Physical RAM (%1 KB):").arg(totalMem / 1024));
    freeMemBar_->setRange(0, static_cast<int>(totalMem));
    freeMemBar_->setValue(static_cast<int>(usedMem));
    freeMemBar_->setFormat(QString("%1 KB / %2 KB Used (%3 KB Free)")
                               .arg(usedMem / 1024)
                               .arg(totalMem / 1024)
                               .arg(freeMem / 1024));

    pageFaultsLabel_->setText(QString("Page Faults: %1").arg(mem_->getPageFaultCount()));
    pageReplacementsLabel_->setText(QString("Evictions / Replacements: %1").arg(mem_->getPageReplacementCount()));
}

void MemoryTab::onPolicyChanged() {
    auto allocP = static_cast<AllocationPolicy>(allocPolicyCombo_->currentData().toInt());
    auto replP = static_cast<ReplacementPolicy>(replPolicyCombo_->currentData().toInt());

    mem_->setAllocationPolicy(allocP);
    mem_->setReplacementPolicy(replP);

    logMessage(QString("Policies updated: Allocation=%1, Replacement=%2")
                   .arg(allocP == AllocationPolicy::DEMAND_PAGING ? "DEMAND_PAGING" : "EAGER")
                   .arg(replP == ReplacementPolicy::LRU ? "LRU" : "FIFO"));
}

void MemoryTab::onAllocateProcess() {
    std::uint32_t pid = static_cast<std::uint32_t>(allocPidSpin_->value());
    std::size_t bytes = static_cast<std::size_t>(allocSizeSpin_->value());

    if (mem_->allocateProcess(pid, bytes)) {
        logMessage(QString("SUCCESS: Allocated %1 virtual bytes (%2 pages) for PID %3.")
                       .arg(bytes).arg(bytes / MemoryManager::PAGE_SIZE).arg(pid));
        emit statusMessage(QString("Virtual memory allocated for PID %1").arg(pid));
    } else {
        logMessage(QString("ERROR: Failed to allocate memory for PID %1 (possibly already allocated or insufficient memory).").arg(pid));
    }
    updateFramesAndStats();
}

void MemoryTab::onFreeProcess() {
    std::uint32_t pid = static_cast<std::uint32_t>(allocPidSpin_->value());
    if (mem_->freeProcess(pid)) {
        logMessage(QString("SUCCESS: Released virtual memory and physical frames for PID %1.").arg(pid));
        emit statusMessage(QString("Released memory for PID %1").arg(pid));
    } else {
        logMessage(QString("ERROR: Failed to free PID %1 (PID not found).").arg(pid));
    }
    updateFramesAndStats();
}

void MemoryTab::onWriteMemory() {
    std::uint32_t pid = static_cast<std::uint32_t>(accessPidSpin_->value());
    std::size_t addr = static_cast<std::size_t>(accessAddrSpin_->value());

    QString valStr = accessValEdit_->text().trimmed();
    int val = 0;
    if (valStr.startsWith("0x", Qt::CaseInsensitive)) {
        val = valStr.toInt(nullptr, 16);
    } else {
        val = valStr.toInt();
    }

    std::size_t prevFaults = mem_->getPageFaultCount();
    if (mem_->write(pid, addr, static_cast<std::uint8_t>(val))) {
        bool faulted = (mem_->getPageFaultCount() > prevFaults);
        logMessage(QString("WRITE SUCCESS: PID %1 wrote 0x%2 (%3) to virtual addr %4 %5")
                       .arg(pid)
                       .arg(static_cast<std::uint8_t>(val), 2, 16, QLatin1Char('0'))
                       .arg(val)
                       .arg(addr)
                       .arg(faulted ? "[PAGE FAULT HANDLED & FRAME ALLOCATED]" : ""));
        emit statusMessage(QString("Wrote byte to PID %1 at virtual address %2").arg(pid).arg(addr));
    } else {
        logMessage(QString("WRITE ERROR: Failed to write to PID %1 at virtual addr %2 (Invalid PID, out of bounds, or memory full).")
                       .arg(pid).arg(addr));
    }
    updateFramesAndStats();
}

void MemoryTab::onReadMemory() {
    std::uint32_t pid = static_cast<std::uint32_t>(accessPidSpin_->value());
    std::size_t addr = static_cast<std::size_t>(accessAddrSpin_->value());

    std::uint8_t val = 0;
    if (mem_->read(pid, addr, val)) {
        logMessage(QString("READ SUCCESS: PID %1 at virtual addr %2 = 0x%3 (%4)")
                       .arg(pid)
                       .arg(addr)
                       .arg(val, 2, 16, QLatin1Char('0'))
                       .arg(static_cast<int>(val)));
        emit statusMessage(QString("Read byte from PID %1 at virtual address %2: 0x%3").arg(pid).arg(addr).arg(val, 2, 16, QLatin1Char('0')));
    } else {
        logMessage(QString("READ ERROR: Failed to read from PID %1 at addr %2 (Page not present or unmapped).").arg(pid).arg(addr));
    }
    updateFramesAndStats();
}

void MemoryTab::onTranslateAddress() {
    std::uint32_t pid = static_cast<std::uint32_t>(accessPidSpin_->value());
    std::size_t addr = static_cast<std::size_t>(accessAddrSpin_->value());

    std::size_t phys = mem_->translateAddress(pid, addr);
    if (phys != static_cast<std::size_t>(-1)) {
        logMessage(QString("TRANSLATION: PID %1 Virtual Address %2 ➔ Physical Address %3 (Physical Frame %4, Offset %5)")
                       .arg(pid).arg(addr).arg(phys)
                       .arg(phys / MemoryManager::PAGE_SIZE)
                       .arg(phys % MemoryManager::PAGE_SIZE));
    } else {
        logMessage(QString("TRANSLATION: PID %1 Virtual Address %2 is NOT mapped to physical memory (Page Fault condition).")
                       .arg(pid).arg(addr));
    }
}

void MemoryTab::onRunDemo() {
    logMessage("----------------------------------------");
    logMessage("Starting Virtual Memory & Demand Paging Demonstration...");

    mem_->setAllocationPolicy(AllocationPolicy::DEMAND_PAGING);
    mem_->setReplacementPolicy(ReplacementPolicy::LRU);
    allocPolicyCombo_->setCurrentIndex(0);
    replPolicyCombo_->setCurrentIndex(0);

    constexpr std::uint32_t DEMO_PID = 1;
    constexpr std::size_t DEMO_SIZE = 12 * 1024; // 3 virtual pages
    constexpr std::size_t DEMO_ADDR = 4200;      // In page 1

    mem_->freeProcess(DEMO_PID); // Ensure clean state for PID 1

    if (!mem_->allocateProcess(DEMO_PID, DEMO_SIZE)) {
        logMessage("Demo failed to allocate virtual memory.");
        return;
    }
    logMessage(QString("Allocated %1 virtual bytes (3 pages) for PID %2.").arg(DEMO_SIZE).arg(DEMO_PID));

    // Access address 4200
    if (mem_->write(DEMO_PID, DEMO_ADDR, 0xAB)) {
        logMessage(QString("Wrote byte 0xAB to virtual address %1 (Triggered page fault, allocated physical frame).").arg(DEMO_ADDR));
    }

    std::uint8_t readVal = 0;
    if (mem_->read(DEMO_PID, DEMO_ADDR, readVal)) {
        logMessage(QString("Verified: Read byte 0x%1 from virtual address %2.")
                       .arg(readVal, 2, 16, QLatin1Char('0')).arg(DEMO_ADDR));
    }

    std::size_t phys = mem_->translateAddress(DEMO_PID, DEMO_ADDR);
    logMessage(QString("Translation: Virtual Address %1 ➔ Physical Address %2.").arg(DEMO_ADDR).arg(phys));
    logMessage(QString("Page Faults: %1 | Evictions: %2")
                   .arg(mem_->getPageFaultCount())
                   .arg(mem_->getPageReplacementCount()));
    logMessage("Demo completed successfully!");
    logMessage("----------------------------------------");

    updateFramesAndStats();
}

void MemoryTab::onResetStats() {
    mem_->resetStatistics();
    logMessage("Memory fault and replacement statistics reset.");
    updateFramesAndStats();
}
