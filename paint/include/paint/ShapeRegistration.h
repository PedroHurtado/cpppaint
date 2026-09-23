#pragma once

namespace paint {

class ShapeFactory;

/// Registra las figuras "de serie" en la fábrica. Añadir una figura nueva es
/// una línea más aquí (o en otro fichero), sin tocar ShapeFactory (OCP).
void RegisterBuiltinShapes(ShapeFactory& factory);

}  // namespace paint
