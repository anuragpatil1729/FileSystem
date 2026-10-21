#include "frame_map_widget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <algorithm>

FrameMapWidget::FrameMapWidget(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    setMinimumHeight(150);
}

void FrameMapWidget::setFrames(const std::vector<FrameDisplayInfo>& frames) {
    frames_ = frames;
    update();
}

void FrameMapWidget::clear() {
    frames_.clear();
    update();
}

QSize FrameMapWidget::minimumSizeHint() const {
    return QSize(400, 150);
}

QColor FrameMapWidget::getColorForPid(std::uint32_t pid) const {
    static const std::vector<QColor> colors = {
        QColor(137, 180, 250), // Blue
        QColor(166, 227, 161), // Green
        QColor(249, 226, 175), // Yellow
        QColor(243, 139, 168), // Red
        QColor(203, 166, 247), // Mauve
        QColor(250, 179, 135)  // Peach
    };
    return colors[pid % colors.size()];
}

void FrameMapWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int totalFrames = static_cast<int>(frames_.size());
    if (totalFrames == 0) {
        painter.setPen(QColor(147, 153, 178));
        painter.drawText(rect(), Qt::AlignCenter, "No physical memory frame data available.");
        return;
    }

    const int cols = (totalFrames <= 16) ? 8 : 16;
    const int rows = (totalFrames + cols - 1) / cols;

    const int padding = 10;
    const int gap = 8;
    const int availWidth = width() - 2 * padding;
    const int availHeight = height() - 2 * padding;

    const int cellW = std::max(40, (availWidth - (cols - 1) * gap) / cols);
    const int cellH = std::max(50, (availHeight - (rows - 1) * gap) / rows);

    QFont titleFont = painter.font();
    titleFont.setPointSize(8);
    titleFont.setBold(true);

    QFont detailFont = painter.font();
    detailFont.setPointSize(7);
    detailFont.setBold(false);

    for (int i = 0; i < totalFrames; ++i) {
        const int r = i / cols;
        const int c = i % cols;
        const int x = padding + c * (cellW + gap);
        const int y = padding + r * (cellH + gap);
        QRect cellRect(x, y, cellW, cellH);

        const auto& f = frames_[i];
        if (f.used) {
            QColor pidCol = getColorForPid(f.pid);
            painter.setBrush(QColor(pidCol.red(), pidCol.green(), pidCol.blue(), 60));
            painter.setPen(QPen(pidCol, 1.5));
            painter.drawRoundedRect(cellRect, 6, 6);

            painter.setPen(pidCol);
            painter.setFont(titleFont);
            painter.drawText(QRect(x, y + 4, cellW, 14), Qt::AlignCenter, QString("Frame %1").arg(f.frameIndex));

            painter.setPen(QColor(205, 214, 244));
            painter.setFont(detailFont);
            painter.drawText(QRect(x, y + 20, cellW, 12), Qt::AlignCenter, QString("PID %1").arg(f.pid));
            painter.drawText(QRect(x, y + 34, cellW, 12), Qt::AlignCenter, QString("Page %1").arg(f.virtualPage));
        } else {
            painter.setBrush(QColor(30, 30, 46));
            QPen dashedPen(QColor(69, 71, 90), 1, Qt::DashLine);
            painter.setPen(dashedPen);
            painter.drawRoundedRect(cellRect, 6, 6);

            painter.setPen(QColor(108, 112, 134));
            painter.setFont(titleFont);
            painter.drawText(QRect(x, y + 6, cellW, 14), Qt::AlignCenter, QString("Frame %1").arg(f.frameIndex));
            painter.setFont(detailFont);
            painter.drawText(QRect(x, y + 26, cellW, 14), Qt::AlignCenter, "FREE");
        }
    }
}
