#ifndef SCHEDULER_TAB_HPP
#define SCHEDULER_TAB_HPP

#include "scheduler.hpp"
#include "gantt_widget.hpp"

#include <QWidget>
#include <QTableWidget>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <memory>

class SchedulerTab : public QWidget {
    Q_OBJECT
public:
    explicit SchedulerTab(std::shared_ptr<Scheduler> scheduler, QWidget* parent = nullptr);

    void refresh();

signals:
    void statusMessage(const QString& message);

private slots:
    void onAddProcess();
    void onLoadDemo();
    void onRunScheduler();
    void onClearQueue();

private:
    std::shared_ptr<Scheduler> scheduler_;

    // Form inputs
    QLineEdit* procNameEdit_;
    QSpinBox* burstSpin_;
    QSpinBox* prioritySpin_;
    QSpinBox* arrivalSpin_;
    QSpinBox* quantumSpin_;

    QPushButton* addProcBtn_;
    QPushButton* runBtn_;
    QPushButton* loadDemoBtn_;
    QPushButton* clearBtn_;

    // Tables & widgets
    QTableWidget* queueTable_;
    GanttWidget* ganttWidget_;
    QLabel* execLogLabel_;
    QTableWidget* metricsTable_;

    // Summary Cards
    QLabel* avgWaitingLabel_;
    QLabel* avgTurnaroundLabel_;
    QLabel* cpuUtilLabel_;
    QLabel* totalTimeLabel_;

    void setupUi();
    void updateQueueTable();
    void updateResultsView();
};

#endif // SCHEDULER_TAB_HPP
