#ifndef FRAME_MAP_WIDGET_HPP
#define FRAME_MAP_WIDGET_HPP

#include <QWidget>
#include <vector>
#include <cstddef>
#include <cstdint>

struct FrameDisplayInfo {
    std::size_t frameIndex;
    bool used;
    std::uint32_t pid;
    std::size_t virtualPage;
};

class FrameMapWidget : public QWidget {
    Q_OBJECT
public:
    explicit FrameMapWidget(QWidget* parent = nullptr);

    void setFrames(const std::vector<FrameDisplayInfo>& frames);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;
    QSize minimumSizeHint() const override;

private:
    std::vector<FrameDisplayInfo> frames_;
    QColor getColorForPid(std::uint32_t pid) const;
};

#endif // FRAME_MAP_WIDGET_HPP
