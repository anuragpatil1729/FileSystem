#include "scheduler_tab.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QMessageBox>

SchedulerTab::SchedulerTab(std::shared_ptr<Scheduler> scheduler, QWidget* parent)
    : QWidget(parent)
    , scheduler_(std::move(scheduler))
{
    setupUi();
    updateQueueTable();
}

void SchedulerTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Top Controls Bar (Quantum + Run + Demo + Clear)
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(10);

    auto* qLabel = new QLabel("<b>Time Quantum:</b>", this);
    quantumSpin_ = new QSpinBox(this);
    quantumSpin_->setRange(1, 50);
    quantumSpin_->setValue(static_cast<int>(scheduler_->getTimeQuantum()));

    runBtn_ = new QPushButton("▶ Run Scheduler", this);
    runBtn_->setStyleSheet("QPushButton { background-color: #a6e3a1; color: #11111b; font-weight: bold; padding: 6px 14px; }");

    loadDemoBtn_ = new QPushButton("🧪 Load Demo Workload", this);
    clearBtn_ = new QPushButton("🧹 Clear All", this);
    clearBtn_->setStyleSheet("QPushButton { background-color: #f38ba8; color: #11111b; font-weight: bold; }");

    topBar->addWidget(qLabel);
    topBar->addWidget(quantumSpin_);
    topBar->addSpacing(15);
    topBar->addWidget(runBtn_);
    topBar->addWidget(loadDemoBtn_);
    topBar->addWidget(clearBtn_);
    topBar->addStretch(1);

    mainLayout->addLayout(topBar);

    // Middle Split: Left = Add Process & Queue, Right = Metrics Cards
    auto* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(12);

    // Add Process Box
    auto* addBox = new QGroupBox("Add Process to Ready Queue", this);
    auto* addForm = new QGridLayout(addBox);

    procNameEdit_ = new QLineEdit("Process-1", this);
    burstSpin_ = new QSpinBox(this);
    burstSpin_->setRange(1, 100);
    burstSpin_->setValue(4);

    prioritySpin_ = new QSpinBox(this);
    prioritySpin_->setRange(0, 10);
    prioritySpin_->setValue(1);

    arrivalSpin_ = new QSpinBox(this);
    arrivalSpin_->setRange(0, 100);
    arrivalSpin_->setValue(0);

    addProcBtn_ = new QPushButton("➕ Add Process", this);
    addProcBtn_->setStyleSheet("QPushButton { background-color: #89b4fa; color: #11111b; font-weight: bold; }");

    addForm->addWidget(new QLabel("Process Name:", this), 0, 0);
    addForm->addWidget(procNameEdit_, 0, 1);
    addForm->addWidget(new QLabel("CPU Burst Time:", this), 1, 0);
    addForm->addWidget(burstSpin_, 1, 1);
    addForm->addWidget(new QLabel("Priority (0=High):", this), 2, 0);
    addForm->addWidget(prioritySpin_, 2, 1);
    addForm->addWidget(new QLabel("Arrival Time:", this), 3, 0);
    addForm->addWidget(arrivalSpin_, 3, 1);
    addForm->addWidget(addProcBtn_, 4, 0, 1, 2);

    middleLayout->addWidget(addBox, 3);

    // Summary Statistics Cards
    auto* statsBox = new QGroupBox("Execution Metrics & KPI Summary", this);
    auto* statsGrid = new QGridLayout(statsBox);

    auto createCard = [](const QString& title, QLabel*& valLabel) -> QWidget* {
        auto* w = new QWidget();
        auto* l = new QVBoxLayout(w);
        l->setContentsMargins(10, 8, 10, 8);
        auto* t = new QLabel(title, w);
        t->setStyleSheet("color: #a6adc8; font-size: 11px; font-weight: bold;");
        valLabel = new QLabel("-", w);
        valLabel->setStyleSheet("color: #cdd6f4; font-size: 18px; font-weight: bold;");
        l->addWidget(t);
        l->addWidget(valLabel);
        w->setStyleSheet("background-color: #1e1e2e; border: 1px solid #313244; border-radius: 6px;");
        return w;
    };

    statsGrid->addWidget(createCard("AVG WAITING TIME", avgWaitingLabel_), 0, 0);
    statsGrid->addWidget(createCard("AVG TURNAROUND TIME", avgTurnaroundLabel_), 0, 1);
    statsGrid->addWidget(createCard("CPU UTILIZATION", cpuUtilLabel_), 1, 0);
    statsGrid->addWidget(createCard("TOTAL CLOCK TICKS", totalTimeLabel_), 1, 1);

    middleLayout->addWidget(statsBox, 4);
    mainLayout->addLayout(middleLayout);

    // Gantt Chart Section
    auto* ganttBox = new QGroupBox("Round-Robin Gantt Timeline (Simulated Clocks)", this);
    auto* ganttLayout = new QVBoxLayout(ganttBox);
    ganttWidget_ = new GanttWidget(this);
    execLogLabel_ = new QLabel("Execution Sequence: -", this);
    execLogLabel_->setStyleSheet("color: #fab387; font-weight: bold;");

    ganttLayout->addWidget(ganttWidget_);
    ganttLayout->addWidget(execLogLabel_);
    mainLayout->addWidget(ganttBox);

    // Detailed Per-Process Performance Table
    auto* tableBox = new QGroupBox("Process Performance Breakdown", this);
    auto* tableLayout = new QVBoxLayout(tableBox);

    metricsTable_ = new QTableWidget(this);
    metricsTable_->setColumnCount(8);
    metricsTable_->setHorizontalHeaderLabels({"PID", "Name", "Burst", "Arrival", "Completion", "Waiting", "Turnaround", "Response"});
    metricsTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    metricsTable_->setAlternatingRowColors(true);
    metricsTable_->verticalHeader()->setVisible(false);

    tableLayout->addWidget(metricsTable_);
    mainLayout->addWidget(tableBox, 1);

    // Connections
    connect(addProcBtn_, &QPushButton::clicked, this, &SchedulerTab::onAddProcess);
    connect(loadDemoBtn_, &QPushButton::clicked, this, &SchedulerTab::onLoadDemo);
    connect(runBtn_, &QPushButton::clicked, this, &SchedulerTab::onRunScheduler);
    connect(clearBtn_, &QPushButton::clicked, this, &SchedulerTab::onClearQueue);
}

void SchedulerTab::refresh() {
    updateQueueTable();
}

void SchedulerTab::updateQueueTable() {
    // If not executed, show empty
    if (!scheduler_->hasProcesses()) {
        execLogLabel_->setText("Execution Sequence: (ready queue is empty)");
    }
}

void SchedulerTab::onAddProcess() {
    QString name = procNameEdit_->text().trimmed();
    if (name.isEmpty()) name = "Process";
    std::size_t burst = static_cast<std::size_t>(burstSpin_->value());
    int priority = prioritySpin_->value();
    std::size_t arrival = static_cast<std::size_t>(arrivalSpin_->value());

    scheduler_->createProcess(name.toStdString(), burst, priority, arrival);
    emit statusMessage(QString("Added %1 (Burst=%2, Priority=%3, Arrival=%4) to scheduler.")
                           .arg(name).arg(burst).arg(priority).arg(arrival));

    // Next suggested name
    static int pCounter = 2;
    procNameEdit_->setText(QString("Process-%1").arg(pCounter++));
    updateResultsView();
}

void SchedulerTab::onLoadDemo() {
    scheduler_->clear();
    scheduler_->setTimeQuantum(2);
    quantumSpin_->setValue(2);

    scheduler_->createProcess("Process-A", 5, 1, 0);
    scheduler_->createProcess("Process-B", 3, 1, 0);

    emit statusMessage("Loaded predefined demo workload: Process-A (burst 5), Process-B (burst 3). Running...");
    onRunScheduler();
}

void SchedulerTab::onRunScheduler() {
    if (!scheduler_->hasProcesses()) {
        QMessageBox::information(this, "Scheduler", "No processes in queue. Please add processes or click 'Load Demo Workload'.");
        return;
    }

    scheduler_->setTimeQuantum(static_cast<std::size_t>(quantumSpin_->value()));
    scheduler_->run();

    updateResultsView();
    emit statusMessage(QString("Scheduler simulation completed with Quantum = %1.").arg(quantumSpin_->value()));
}

void SchedulerTab::onClearQueue() {
    scheduler_->clear();
    ganttWidget_->clear();
    metricsTable_->setRowCount(0);
    avgWaitingLabel_->setText("-");
    avgTurnaroundLabel_->setText("-");
    cpuUtilLabel_->setText("-");
    totalTimeLabel_->setText("-");
    execLogLabel_->setText("Execution Sequence: -");
    emit statusMessage("Scheduler queue and results cleared.");
}

void SchedulerTab::updateResultsView() {
    const auto& timeline = scheduler_->getTimeline();
    ganttWidget_->setTimeline(timeline, scheduler_->getTotalExecutionTime());

    // Execution sequence string
    const auto& execLog = scheduler_->getExecutionLog();
    if (!execLog.empty()) {
        QString seq = "Execution Sequence: ";
        for (std::size_t i = 0; i < execLog.size(); ++i) {
            seq += QString("P%1").arg(execLog[i]);
            if (i + 1 < execLog.size()) seq += " ➔ ";
        }
        execLogLabel_->setText(seq);
    }

    // Process performance table
    const auto records = scheduler_->getSchedulingRecords();
    metricsTable_->setRowCount(static_cast<int>(records.size()));

    for (int r = 0; r < static_cast<int>(records.size()); ++r) {
        const auto& rec = records[static_cast<std::size_t>(r)];
        metricsTable_->setItem(r, 0, new QTableWidgetItem(QString("P%1").arg(rec.pid)));
        metricsTable_->setItem(r, 1, new QTableWidgetItem(QString::fromStdString(rec.name)));
        metricsTable_->setItem(r, 2, new QTableWidgetItem(QString::number(rec.burstTime)));
        metricsTable_->setItem(r, 3, new QTableWidgetItem(QString::number(rec.arrivalTime)));
        metricsTable_->setItem(r, 4, new QTableWidgetItem(QString::number(rec.completionTime)));
        metricsTable_->setItem(r, 5, new QTableWidgetItem(QString::number(rec.waitingTime)));
        metricsTable_->setItem(r, 6, new QTableWidgetItem(QString::number(rec.turnaroundTime)));
        metricsTable_->setItem(r, 7, new QTableWidgetItem(QString::number(rec.responseTime)));

        for (int c = 0; c < 8; ++c) {
            metricsTable_->item(r, c)->setTextAlignment(Qt::AlignCenter);
        }
    }

    // KPI Cards
    avgWaitingLabel_->setText(QString("%1 units").arg(scheduler_->getAverageWaitingTime(), 0, 'f', 2));
    avgTurnaroundLabel_->setText(QString("%1 units").arg(scheduler_->getAverageTurnaroundTime(), 0, 'f', 2));
    cpuUtilLabel_->setText(QString("%1 %").arg(scheduler_->getCpuUtilization(), 0, 'f', 1));
    totalTimeLabel_->setText(QString("%1 units").arg(scheduler_->getTotalExecutionTime()));
}
