#include "paint/App.h"
#include "paint/Canvas.h"
#include "paint/ConsoleReader.h"
#include "paint/ConsoleWriter.h"

// Composition root: aquí se cablean las piezas concretas. Es el único sitio
// que conoce ConsoleReader, ConsoleWriter y la instancia singleton del Canvas;
// todo lo demás trabaja contra abstracciones.
int main() {
    paint::ConsoleReader reader;
    paint::ConsoleWriter writer;
    paint::App app(paint::Canvas::Instance(), reader, writer);
    app.Run();
    return 0;
}
