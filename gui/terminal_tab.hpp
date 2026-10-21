#ifndef TERMINAL_TAB_HPP
#define TERMINAL_TAB_HPP

#include "filesystem.hpp"
#include "scheduler.hpp"
#include "memory_manager.hpp"

#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QStringList>
#include <memory>

class TerminalTab : public QWidget {
    Q_OBJECT
public:
    explicit TerminalTab(std::shared_ptr<FileSystem> fs,
                         std::shared_ptr<Scheduler> scheduler,
                         std::shared_ptr<MemoryManager> memoryManager,
                         QWidget* parent = nullptr);

signals:
    void systemStateChanged();
    void statusMessage(const QString& message);

public slots:
    void executeCommand(const QString& cmdLine);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onCommandEntered();
    void onClearTerminal();

private:
    std::shared_ptr<FileSystem> fs_;
    std::shared_ptr<Scheduler> scheduler_;
    std::shared_ptr<MemoryManager> mem_;

    QTextEdit* consoleOutput_;
    QLineEdit* commandInput_;
    QPushButton* executeBtn_;
    QPushButton* clearBtn_;

    QStringList history_;
    int historyIndex_;

    void setupUi();
    void appendOutput(const QString& text, const QString& color = "#cdd6f4");
    void printHelp();
};

#endif // TERMINAL_TAB_HPP
