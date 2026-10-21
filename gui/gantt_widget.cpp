#include "gantt_widget.hpp"

#include <QPainter>
#include <QPaintEvent>
#include <algorithm>

GanttWidget::GanttWidget(QWidget* parent)
    : QWidget(parent)
    , totalTime_(0)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(110);
}

void GanttWidget::setTimeline(const std::vector<GanttSlice>& timeline, std::size_t totalTime) {
    timeline_ = timeline;
    totalTime_ = totalTime;
    if (totalTime_ == 0 && !timeline_.empty()) {
        totalTime_ = timeline_.back().endTime;
    }
    update();
}

void GanttWidget::clear() {
    timeline_.clear();
    totalTime_ = 0;
    update();
}

QSize GanttWidget::minimumSizeHint() const {
    return QSize(400, 110);
}

QColor GanttWidget::getColorForPid(std::uint32_t pid) const {
    static const std::vector<QColor> colors = {
        QColor(137, 180, 250), // Catppuccin Blue
        QColor(166, 227, 161), // Green
        QColor(249, 226, 175), // Yellow
        QColor(243, 139, 168), // Red
        QColor(203, 166, 247), // Mauve
        QColor(250, 179, 135), // Peach
        QColor(148, 226, 213), // Teal
        QColor(116, 199, 236)  // Sapphire
    };
    return colors[pid % colors.size()];
}

void GanttWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int marginX = 20;
    const int chartTop = 15;
    const int chartHeight = 50;
    const int chartWidth = std::max(10, w - 2 * marginX);

    // Draw background container
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(30, 30, 46));
    painter.drawRoundedRect(0, 0, w, h, 8, 8);

    if (timeline_.empty() || totalTime_ == 0) {
        painter.setPen(QColor(147, 153, 178));
        QFont font = painter.font();
        font.setPointSize(12);
        painter.setFont(font);
        painter.drawText(rect(), Qt::AlignCenter, "No simulation data. Click 'Run Scheduler' or 'Load Demo' above.");
        return;
    }

    // Draw timeline slices
    QFont labelFont = painter.font();
    labelFont.setPointSize(10);
    labelFont.setBold(true);

    QFont timeFont = painter.font();
    timeFont.setPointSize(9);
    timeFont.setBold(false);

    for (const auto& slice : timeline_) {
        const double xStart = marginX + (static_cast<double>(slice.startTime) / totalTime_) * chartWidth;
        const double xEnd = marginX + (static_cast<double>(slice.endTime) / totalTime_) * chartWidth;
        const double sliceW = std::max(2.0, xEnd - xStart);

        QRectF rectSlice(xStart, chartTop, sliceW, chartHeight);
        QColor sliceColor = getColorForPid(slice.pid);

        // Fill slice
        painter.setBrush(sliceColor);
        painter.setPen(QPen(QColor(17, 17, 27), 1.5));
        painter.drawRoundedRect(rectSlice, 4, 4);

        // Process name in center of slice if wide enough
        if (sliceW > 24) {
            painter.setPen(QColor(17, 17, 27));
            painter.setFont(labelFont);
            QString label = QString("P%1").arg(slice.pid);
            if (sliceW > 55 && !slice.name.empty()) {
                label = QString::fromStdString(slice.name);
            }
            painter.drawText(rectSlice, Qt::AlignCenter, label);
        }
    }

    // Draw time tick marks
    painter.setFont(timeFont);
    painter.setPen(QColor(186, 194, 222));

    // Always draw start mark
    painter.drawText(QRectF(marginX - 15, chartTop + chartHeight + 6, 30, 20),
                     Qt::AlignHCenter | Qt::AlignTop, "0");

    // Draw slice boundary ticks
    for (const auto& slice : timeline_) {
        const double xEnd = marginX + (static_cast<double>(slice.endTime) / totalTime_) * chartWidth;
        painter.drawLine(QPointF(xEnd, chartTop + chartHeight), QPointF(xEnd, chartTop + chartHeight + 4));
        painter.drawText(QRectF(xEnd - 15, chartTop + chartHeight + 6, 30, 20),
                         Qt::AlignHCenter | Qt::AlignTop, QString::number(slice.endTime));
    }
}
