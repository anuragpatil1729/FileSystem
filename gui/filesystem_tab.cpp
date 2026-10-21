#include "filesystem_tab.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QDialog>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QFormLayout>

FileSystemTab::FileSystemTab(std::shared_ptr<FileSystem> fs, QWidget* parent)
    : QWidget(parent)
    , fs_(std::move(fs))
    , currentPath_("/")
{
    setupUi();
    refresh();
}

void FileSystemTab::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Top Navigation & Action Bar
    auto* navBar = new QHBoxLayout();
    navBar->setSpacing(8);

    auto* pathLabel = new QLabel("<b>Location:</b>", this);
    pathEdit_ = new QLineEdit(this);
    pathEdit_->setText("/");
    pathEdit_->setPlaceholderText("Enter directory path e.g. /projects");

    upButton_ = new QPushButton("⬆ Up", this);
    upButton_->setToolTip("Navigate to parent directory");

    refreshButton_ = new QPushButton("🔄 Refresh", this);
    newFileButton_ = new QPushButton("📄 New File", this);
    newDirButton_ = new QPushButton("📁 New Folder", this);
    deleteButton_ = new QPushButton("🗑 Delete", this);
    deleteButton_->setStyleSheet("QPushButton { background-color: #f38ba8; color: #11111b; font-weight: bold; }");
    formatButton_ = new QPushButton("⚡ Format Disk...", this);

    navBar->addWidget(pathLabel);
    navBar->addWidget(pathEdit_, 1);
    navBar->addWidget(upButton_);
    navBar->addWidget(refreshButton_);
    navBar->addWidget(newFileButton_);
    navBar->addWidget(newDirButton_);
    navBar->addWidget(deleteButton_);
    navBar->addWidget(formatButton_);
    mainLayout->addLayout(navBar);

    // Main Splitter (Left: File List, Right: Editor)
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Left container: File List
    auto* leftContainer = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftContainer);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto* listTitle = new QLabel("<b>Files & Directories</b>", this);
    fileTree_ = new QTreeWidget(this);
    fileTree_->setHeaderLabels({"Name", "Type", "Size"});
    fileTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    fileTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    fileTree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    fileTree_->setAlternatingRowColors(true);
    fileTree_->setRootIsDecorated(false);

    leftLayout->addWidget(listTitle);
    leftLayout->addWidget(fileTree_);
    splitter->addWidget(leftContainer);

    // Right container: File Viewer & Editor
    auto* rightContainer = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(rightContainer);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* editorHeader = new QHBoxLayout();
    editorStatusLabel_ = new QLabel("<i>No file selected</i>", this);
    saveFileButton_ = new QPushButton("💾 Save Changes", this);
    saveFileButton_->setEnabled(false);
    saveFileButton_->setStyleSheet("QPushButton { background-color: #a6e3a1; color: #11111b; font-weight: bold; }");

    editorHeader->addWidget(editorStatusLabel_, 1);
    editorHeader->addWidget(saveFileButton_);

    fileEditor_ = new QTextEdit(this);
    fileEditor_->setPlaceholderText("File contents will be displayed here. You can edit text and click 'Save Changes' to commit to virtual disk.");
    QFont monoFont("Menlo", 11);
    monoFont.setStyleHint(QFont::Monospace);
    fileEditor_->setFont(monoFont);

    rightLayout->addLayout(editorHeader);
    rightLayout->addWidget(fileEditor_);
    splitter->addWidget(rightContainer);
    splitter->setStretchFactor(0, 4);
    splitter->setStretchFactor(1, 6);
    mainLayout->addWidget(splitter, 1);

    // Bottom Stats Box
    auto* statsBox = new QGroupBox("Virtual Disk Geometry & Health", this);
    auto* statsLayout = new QHBoxLayout(statsBox);

    geometryLabel_ = new QLabel(this);
    geometryLabel_->setTextFormat(Qt::RichText);

    auto* barsLayout = new QVBoxLayout();
    auto* blockRow = new QHBoxLayout();
    blockUsageLabel_ = new QLabel("Blocks: ", this);
    blockUsageBar_ = new QProgressBar(this);
    blockUsageBar_->setTextVisible(true);
    blockRow->addWidget(blockUsageLabel_);
    blockRow->addWidget(blockUsageBar_, 1);

    auto* inodeRow = new QHBoxLayout();
    inodeUsageLabel_ = new QLabel("Inodes: ", this);
    inodeUsageBar_ = new QProgressBar(this);
    inodeUsageBar_->setTextVisible(true);
    inodeRow->addWidget(inodeUsageLabel_);
    inodeRow->addWidget(inodeUsageBar_, 1);

    barsLayout->addLayout(blockRow);
    barsLayout->addLayout(inodeRow);

    statsLayout->addWidget(geometryLabel_, 1);
    statsLayout->addLayout(barsLayout, 1);
    mainLayout->addWidget(statsBox);

    // Connections
    connect(upButton_, &QPushButton::clicked, this, &FileSystemTab::onNavigateUp);
    connect(refreshButton_, &QPushButton::clicked, this, &FileSystemTab::onRefreshClicked);
    connect(pathEdit_, &QLineEdit::returnPressed, [this]() {
        loadDirectory(pathEdit_->text().toStdString());
    });
    connect(fileTree_, &QTreeWidget::itemDoubleClicked, this, &FileSystemTab::onItemDoubleClicked);
    connect(fileTree_, &QTreeWidget::itemSelectionChanged, this, &FileSystemTab::onItemSelectionChanged);
    connect(newFileButton_, &QPushButton::clicked, this, &FileSystemTab::onCreateFileClicked);
    connect(newDirButton_, &QPushButton::clicked, this, &FileSystemTab::onCreateDirectoryClicked);
    connect(deleteButton_, &QPushButton::clicked, this, &FileSystemTab::onDeleteClicked);
    connect(saveFileButton_, &QPushButton::clicked, this, &FileSystemTab::onSaveFileClicked);
    connect(formatButton_, &QPushButton::clicked, this, &FileSystemTab::onFormatClicked);
}

void FileSystemTab::refresh() {
    loadDirectory(currentPath_);
    updateDiskGeometryUi();
}

std::string FileSystemTab::normalizePath(const std::string& path) const {
    if (path.empty() || path[0] != '/') {
        return "/" + path;
    }
    return path;
}

void FileSystemTab::loadDirectory(const std::string& path) {
    if (!fs_ || !fs_->isMounted()) {
        editorStatusLabel_->setText("<b>Filesystem not mounted.</b>");
        return;
    }

    std::string target = normalizePath(path);
    auto entries = fs_->listDirectory(target);
    if (!fs_->getLastError().empty() && entries.empty()) {
        // Check if path is actually a file or invalid
        if (target != "/") {
            emit statusMessage(QString("Failed to open %1: %2")
                                   .arg(QString::fromStdString(target))
                                   .arg(QString::fromStdString(fs_->getLastError())));
            return;
        }
    }

    currentPath_ = target;
    pathEdit_->setText(QString::fromStdString(currentPath_));
    fileTree_->clear();

    // Up item if not root
    if (currentPath_ != "/") {
        auto* upItem = new QTreeWidgetItem(fileTree_);
        upItem->setText(0, "..");
        upItem->setText(1, "[Parent Directory]");
        upItem->setText(2, "-");
        upItem->setData(0, Qt::UserRole, QString(".."));
    }

    for (const auto& name : entries) {
        if (name == "." || name == "..") continue;

        std::string fullItemPath = (currentPath_ == "/") ? ("/" + name) : (currentPath_ + "/" + name);
        auto* item = new QTreeWidgetItem(fileTree_);
        item->setText(0, QString::fromStdString(name));
        item->setData(0, Qt::UserRole, QString::fromStdString(fullItemPath));

        // Test if directory
        auto subEntries = fs_->listDirectory(fullItemPath);
        bool isDir = true;
        if (subEntries.empty() && fs_->getLastError().find("not a directory") != std::string::npos) {
            isDir = false;
        }

        if (isDir) {
            item->setText(1, "📁 Directory");
            item->setText(2, "-");
            item->setData(1, Qt::UserRole, "dir");
        } else {
            item->setText(1, "📄 Regular File");
            item->setData(1, Qt::UserRole, "file");
            std::string content = fs_->readFile(fullItemPath);
            item->setText(2, QString("%1 B").arg(content.size()));
        }
    }

    updateDiskGeometryUi();
}

void FileSystemTab::updateDiskGeometryUi() {
    if (!fs_ || !fs_->isMounted()) {
        geometryLabel_->setText("<font color='#f38ba8'>Filesystem unmounted or uninitialized.</font>");
        blockUsageBar_->setValue(0);
        inodeUsageBar_->setValue(0);
        return;
    }

    const auto& sb = fs_->getSuperblock();
    geometryLabel_->setText(
        QString("<b>Status:</b> Mounted<br>"
                "<b>Block Size:</b> %1 bytes | <b>Data Start:</b> Block %2<br>"
                "<b>Max File Size:</b> %3 KB (%4 bytes)")
            .arg(sb.blockSize)
            .arg(sb.dataBlockStart)
            .arg(fs_->getMaxFileSize() / 1024)
            .arg(fs_->getMaxFileSize())
    );

    const int totalBlocks = static_cast<int>(sb.totalBlocks);
    const int freeBlocks = static_cast<int>(sb.freeBlocks);
    const int usedBlocks = totalBlocks - freeBlocks;
    blockUsageBar_->setRange(0, totalBlocks);
    blockUsageBar_->setValue(usedBlocks);
    blockUsageBar_->setFormat(QString("%1 / %2 Used (%3% free)").arg(usedBlocks).arg(totalBlocks)
                                  .arg(totalBlocks > 0 ? (freeBlocks * 100 / totalBlocks) : 0));

    const int totalInodes = static_cast<int>(sb.totalInodes);
    const int freeInodes = static_cast<int>(sb.freeInodes);
    const int usedInodes = totalInodes - freeInodes;
    inodeUsageBar_->setRange(0, totalInodes);
    inodeUsageBar_->setValue(usedInodes);
    inodeUsageBar_->setFormat(QString("%1 / %2 Inodes Used").arg(usedInodes).arg(totalInodes));
}

void FileSystemTab::onNavigateUp() {
    if (currentPath_ == "/") return;
    auto pos = currentPath_.find_last_of('/');
    if (pos == 0) {
        loadDirectory("/");
    } else if (pos != std::string::npos) {
        loadDirectory(currentPath_.substr(0, pos));
    }
}

void FileSystemTab::onRefreshClicked() {
    refresh();
    emit statusMessage("Filesystem refreshed.");
}

void FileSystemTab::onItemDoubleClicked(QTreeWidgetItem* item, int /*column*/) {
    if (!item) return;
    QString target = item->data(0, Qt::UserRole).toString();
    if (target == "..") {
        onNavigateUp();
        return;
    }

    QString type = item->data(1, Qt::UserRole).toString();
    if (type == "dir") {
        loadDirectory(target.toStdString());
    } else {
        // Load into editor
        currentEditingFilePath_ = target.toStdString();
        std::string content = fs_->readFile(currentEditingFilePath_);
        fileEditor_->setPlainText(QString::fromStdString(content));
        editorStatusLabel_->setText(QString("<b>Editing:</b> %1 (%2 bytes)")
                                        .arg(QString::fromStdString(currentEditingFilePath_))
                                        .arg(content.size()));
        saveFileButton_->setEnabled(true);
    }
}

void FileSystemTab::onItemSelectionChanged() {
    auto items = fileTree_->selectedItems();
    if (items.isEmpty()) return;
    auto* item = items.first();
    QString target = item->data(0, Qt::UserRole).toString();
    if (target == "..") return;

    QString type = item->data(1, Qt::UserRole).toString();
    if (type == "file") {
        currentEditingFilePath_ = target.toStdString();
        std::string content = fs_->readFile(currentEditingFilePath_);
        fileEditor_->setPlainText(QString::fromStdString(content));
        editorStatusLabel_->setText(QString("<b>Selected:</b> %1 (%2 bytes)")
                                        .arg(QString::fromStdString(currentEditingFilePath_))
                                        .arg(content.size()));
        saveFileButton_->setEnabled(true);
    }
}

void FileSystemTab::onCreateFileClicked() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "Create Regular File",
                                         "Enter file name:", QLineEdit::Normal,
                                         "new_file.txt", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    std::string fullPath = (currentPath_ == "/") ? ("/" + name.trimmed().toStdString())
                                                : (currentPath_ + "/" + name.trimmed().toStdString());

    if (fs_->createFile(fullPath)) {
        emit statusMessage(QString("File created: %1").arg(QString::fromStdString(fullPath)));
        refresh();
        emit fileSystemChanged();
    } else {
        QMessageBox::critical(this, "Create File Error",
                              QString("Failed to create file: %1").arg(QString::fromStdString(fs_->getLastError())));
    }
}

void FileSystemTab::onCreateDirectoryClicked() {
    bool ok = false;
    QString name = QInputDialog::getText(this, "Create Directory",
                                         "Enter directory name:", QLineEdit::Normal,
                                         "new_folder", &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    std::string fullPath = (currentPath_ == "/") ? ("/" + name.trimmed().toStdString())
                                                : (currentPath_ + "/" + name.trimmed().toStdString());

    if (fs_->createDirectory(fullPath)) {
        emit statusMessage(QString("Directory created: %1").arg(QString::fromStdString(fullPath)));
        refresh();
        emit fileSystemChanged();
    } else {
        QMessageBox::critical(this, "Create Directory Error",
                              QString("Failed to create directory: %1").arg(QString::fromStdString(fs_->getLastError())));
    }
}

void FileSystemTab::onDeleteClicked() {
    auto items = fileTree_->selectedItems();
    if (items.isEmpty()) {
        QMessageBox::information(this, "Delete", "Please select an item to delete.");
        return;
    }

    auto* item = items.first();
    QString target = item->data(0, Qt::UserRole).toString();
    if (target == "..") return;

    QString type = item->data(1, Qt::UserRole).toString();
    auto reply = QMessageBox::question(this, "Confirm Deletion",
                                       QString("Are you sure you want to delete '%1'?").arg(target),
                                       QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    bool success = false;
    if (type == "dir") {
        success = fs_->deleteDirectory(target.toStdString());
    } else {
        success = fs_->deleteFile(target.toStdString());
    }

    if (success) {
        emit statusMessage(QString("Deleted %1 successfully.").arg(target));
        if (currentEditingFilePath_ == target.toStdString()) {
            fileEditor_->clear();
            editorStatusLabel_->setText("<i>No file selected</i>");
            saveFileButton_->setEnabled(false);
            currentEditingFilePath_.clear();
        }
        refresh();
        emit fileSystemChanged();
    } else {
        QMessageBox::critical(this, "Delete Error",
                              QString("Failed to delete %1:\n%2").arg(target).arg(QString::fromStdString(fs_->getLastError())));
    }
}

void FileSystemTab::onSaveFileClicked() {
    if (currentEditingFilePath_.empty()) return;

    std::string text = fileEditor_->toPlainText().toStdString();
    if (fs_->writeFile(currentEditingFilePath_, text)) {
        editorStatusLabel_->setText(QString("<b>Saved:</b> %1 (%2 bytes)")
                                        .arg(QString::fromStdString(currentEditingFilePath_))
                                        .arg(text.size()));
        emit statusMessage(QString("Wrote %1 bytes to %2").arg(text.size()).arg(QString::fromStdString(currentEditingFilePath_)));
        refresh();
        emit fileSystemChanged();
    } else {
        QMessageBox::critical(this, "Save File Error",
                              QString("Failed to write to file:\n%1").arg(QString::fromStdString(fs_->getLastError())));
    }
}

void FileSystemTab::onFormatClicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Format Virtual Disk");
    auto* layout = new QFormLayout(&dialog);

    auto* blocksSpin = new QSpinBox(&dialog);
    blocksSpin->setRange(64, 65536);
    blocksSpin->setValue(DiskManager::DEFAULT_BLOCK_COUNT);

    auto* inodesSpin = new QSpinBox(&dialog);
    inodesSpin->setRange(16, 4096);
    inodesSpin->setValue(FileSystem::DEFAULT_INODE_COUNT);

    layout->addRow("Total Blocks:", blocksSpin);
    layout->addRow("Total Inodes:", inodesSpin);

    auto* warn = new QLabel("<font color='#f38ba8'><b>WARNING:</b> Formatting will erase all files and directories on the virtual disk.</font>", &dialog);
    layout->addRow(warn);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        if (fs_->format(blocksSpin->value(), inodesSpin->value())) {
            currentPath_ = "/";
            currentEditingFilePath_.clear();
            fileEditor_->clear();
            saveFileButton_->setEnabled(false);
            editorStatusLabel_->setText("<i>Disk reformatted successfully.</i>");
            refresh();
            emit statusMessage("Virtual disk formatted successfully.");
            emit fileSystemChanged();
        } else {
            QMessageBox::critical(this, "Format Error",
                                  QString("Format failed: %1").arg(QString::fromStdString(fs_->getLastError())));
        }
    }
}
