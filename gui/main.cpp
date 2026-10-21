#include "mainwindow.hpp"

#include <QApplication>
#include <QFileInfo>
#include <QDir>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("VFS-Studio");
    app.setApplicationDisplayName("VFS Studio");

    std::string diskPath = "data/virtual_disk.bin";
    if (argc > 1) {
        diskPath = argv[1];
    } else {
        // Automatically check if running from build directory
        if (!QFileInfo::exists(QString::fromStdString(diskPath))) {
            if (QFileInfo::exists("../data/virtual_disk.bin")) {
                diskPath = "../data/virtual_disk.bin";
            } else if (QFileInfo::exists("../../data/virtual_disk.bin")) {
                diskPath = "../../data/virtual_disk.bin";
            }
        }
    }

    MainWindow window(diskPath);
    window.show();

    return app.exec();
}
