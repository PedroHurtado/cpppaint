#pragma once

#include <memory>
#include <string>

#include "paint/IShape.h"
#include "paint/Point.h"

namespace paint {

class Square : public IShape {
public:
    Square(double side, int color, Point position);

    double Area() const override;
    int Color() const override;
    Point Position() const override;
    void MoveTo(Point position) override;
    std::unique_ptr<IShape> Clone() const override;
    std::string ToString() const override;

private:
    double side_;
    int color_;
    Point position_;
};

}  // namespace paint
