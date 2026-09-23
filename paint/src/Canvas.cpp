#include "paint/Canvas.h"

#include <algorithm>
#include <stdexcept>
#include <string>

namespace paint {

ICanvas& Canvas::Instance() {
    static Canvas instance;  // estática local: creada una vez, thread-safe (C++11)
    return instance;
}

void Canvas::Add(std::unique_ptr<IShape> shape) {
    shapes_.push_back(std::move(shape));
    // TODO (día 5, Observer): notificar a las vistas suscritas del cambio.
}

std::unique_ptr<IShape> Canvas::Remove(IShape* handle) {
    auto it = std::find_if(
        shapes_.begin(), shapes_.end(),
        [handle](const std::unique_ptr<IShape>& s) { return s.get() == handle; });

    if (it == shapes_.end())
        throw std::runtime_error("figura no encontrada en el lienzo");

    std::unique_ptr<IShape> removed = std::move(*it);
    shapes_.erase(it);
    // TODO (día 5, Observer): notificar.
    return removed;
}

IShape& Canvas::At(std::size_t index) {
    if (index >= shapes_.size())
        throw std::out_of_range("indice de figura fuera de rango: " +
                                std::to_string(index));
    return *shapes_[index];
}

std::size_t Canvas::Count() const { return shapes_.size(); }

void Canvas::Clear() { shapes_.clear(); }

void Canvas::Print(IWriter& writer) const {
    if (shapes_.empty()) {
        writer.Write("(lienzo vacio)");
        return;
    }
    for (std::size_t i = 0; i < shapes_.size(); ++i)
        writer.Write("[" + std::to_string(i) + "] " + shapes_[i]->ToString());
}

}  // namespace paint
