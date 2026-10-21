#ifndef GANTT_WIDGET_HPP
#define GANTT_WIDGET_HPP

#include "scheduler.hpp"

#include <QWidget>
#include <vector>

class GanttWidget : public QWidget {
    Q_OBJECT
public:
    explicit GanttWidget(QWidget* parent = nullptr);

    void setTimeline(const std::vector<GanttSlice>& timeline, std::size_t totalTime);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize minimumSizeHint() const override;

private:
    std::vector<GanttSlice> timeline_;
    std::size_t totalTime_;

    QColor getColorForPid(std::uint32_t pid) const;
};

#endif // GANTT_WIDGET_HPP
