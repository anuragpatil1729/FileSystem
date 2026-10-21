#ifndef MEMORY_TAB_HPP
#define MEMORY_TAB_HPP

#include "memory_manager.hpp"
#include "frame_map_widget.hpp"

#include <QWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>
#include <memory>

class MemoryTab : public QWidget {
    Q_OBJECT
public:
    explicit MemoryTab(std::shared_ptr<MemoryManager> memoryManager, QWidget* parent = nullptr);

    void refresh();

signals:
    void statusMessage(const QString& message);

private slots:
    void onPolicyChanged();
    void onAllocateProcess();
    void onFreeProcess();
    void onWriteMemory();
    void onReadMemory();
    void onTranslateAddress();
    void onRunDemo();
    void onResetStats();

private:
    std::shared_ptr<MemoryManager> mem_;

    // Policies
    QComboBox* allocPolicyCombo_;
    QComboBox* replPolicyCombo_;
    QPushButton* runDemoBtn_;
    QPushButton* resetStatsBtn_;

    // Process allocation
    QSpinBox* allocPidSpin_;
    QSpinBox* allocSizeSpin_;
    QPushButton* allocBtn_;
    QPushButton* freeBtn_;

    // Memory Access Simulator
    QSpinBox* accessPidSpin_;
    QSpinBox* accessAddrSpin_;
    QLineEdit* accessValEdit_;
    QPushButton* writeBtn_;
    QPushButton* readBtn_;
    QPushButton* translateBtn_;

    // Status Log / Result Box
    QTextEdit* logEdit_;

    // Visual Frame Map & Stats
    FrameMapWidget* frameMapWidget_;
    QLabel* freeMemLabel_;
    QProgressBar* freeMemBar_;
    QLabel* pageFaultsLabel_;
    QLabel* pageReplacementsLabel_;

    void setupUi();
    void updateFramesAndStats();
    void logMessage(const QString& msg);
};

#endif // MEMORY_TAB_HPP
