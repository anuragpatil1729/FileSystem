#ifndef FILESYSTEM_TAB_HPP
#define FILESYSTEM_TAB_HPP

#include "filesystem.hpp"

#include <QWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <memory>

class FileSystemTab : public QWidget {
    Q_OBJECT
public:
    explicit FileSystemTab(std::shared_ptr<FileSystem> fs, QWidget* parent = nullptr);

    void refresh();

signals:
    void statusMessage(const QString& message);
    void fileSystemChanged();

private slots:
    void onNavigateUp();
    void onRefreshClicked();
    void onItemDoubleClicked(QTreeWidgetItem* item, int column);
    void onItemSelectionChanged();
    void onCreateFileClicked();
    void onCreateDirectoryClicked();
    void onDeleteClicked();
    void onSaveFileClicked();
    void onFormatClicked();

private:
    std::shared_ptr<FileSystem> fs_;
    std::string currentPath_;
    std::string currentEditingFilePath_;

    QLineEdit* pathEdit_;
    QPushButton* upButton_;
    QPushButton* refreshButton_;
    QPushButton* newFileButton_;
    QPushButton* newDirButton_;
    QPushButton* deleteButton_;
    QPushButton* saveFileButton_;
    QPushButton* formatButton_;

    QTreeWidget* fileTree_;
    QTextEdit* fileEditor_;
    QLabel* editorStatusLabel_;

    QLabel* geometryLabel_;
    QProgressBar* blockUsageBar_;
    QProgressBar* inodeUsageBar_;
    QLabel* blockUsageLabel_;
    QLabel* inodeUsageLabel_;

    void setupUi();
    void loadDirectory(const std::string& path);
    void updateDiskGeometryUi();
    std::string normalizePath(const std::string& path) const;
};

#endif // FILESYSTEM_TAB_HPP
