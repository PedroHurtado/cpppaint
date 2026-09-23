// Pruebas mínimas con <cassert>. Cada función comprueba un patrón o un
// principio. Si alguna aserción falla, el ejecutable aborta con código != 0 y
// CTest marca el test como fallido.
#include <cassert>
#include <cmath>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "paint/AddShapeCommand.h"
#include "paint/App.h"
#include "paint/Canvas.h"
#include "paint/Circle.h"
#include "paint/CommandManager.h"
#include "paint/CommandRegistry.h"
#include "paint/DuplicateShapeCommand.h"
#include "paint/ICanvas.h"
#include "paint/IReader.h"
#include "paint/IShape.h"
#include "paint/IWriter.h"
#include "paint/MoveShapeCommand.h"
#include "paint/ShapeFactory.h"
#include "paint/ShapeRegistration.h"
#include "paint/Square.h"

using namespace paint;

// ---------------------------------------------------------------------------
// Dobles de prueba, escritos a mano. Existen gracias a que App y los comandos
// dependen de interfaces (ICanvas, IReader, IWriter) y no de clases concretas.
// ---------------------------------------------------------------------------

/// Lienzo falso: mismo contrato que Canvas, pero sin Singleton y contando
/// llamadas. Antes de ICanvas esto era imposible de escribir.
class FakeCanvas : public ICanvas {
public:
    void Add(std::unique_ptr<IShape> shape) override {
        ++adds;
        shapes_.push_back(std::move(shape));
    }

    std::unique_ptr<IShape> Remove(IShape* handle) override {
        ++removes;
        for (std::size_t i = 0; i < shapes_.size(); ++i) {
            if (shapes_[i].get() != handle) continue;
            std::unique_ptr<IShape> out = std::move(shapes_[i]);
            shapes_.erase(shapes_.begin() + static_cast<std::ptrdiff_t>(i));
            return out;
        }
        return nullptr;
    }

    IShape& At(std::size_t index) override { return *shapes_.at(index); }
    std::size_t Count() const override { return shapes_.size(); }
    void Clear() override { shapes_.clear(); }
    void Print(IWriter&) const override { ++prints; }

    int adds = 0;
    int removes = 0;
    mutable int prints = 0;

private:
    std::vector<std::unique_ptr<IShape>> shapes_;
};

/// Entrada falsa: sirve líneas de un vector. Sustituye a std::cin.
class StringReader : public IReader {
public:
    explicit StringReader(std::vector<std::string> lines)
        : lines_(std::move(lines)) {}

    bool ReadLine(std::string& line) override {
        if (next_ >= lines_.size()) return false;
        line = lines_[next_++];
        return true;
    }

private:
    std::vector<std::string> lines_;
    std::size_t next_ = 0;
};

/// Salida falsa: guarda lo escrito para poder afirmar sobre ello.
class RecordingWriter : public IWriter {
public:
    void Write(const std::string& line) override { lines.push_back(line); }

    bool Contains(const std::string& needle) const {
        for (const std::string& line : lines)
            if (line.find(needle) != std::string::npos) return true;
        return false;
    }

    std::vector<std::string> lines;
};

// ---------------------------------------------------------------------------
// Patrones creacionales
// ---------------------------------------------------------------------------

// Factory: crea la figura correcta a partir de una línea de texto.
static void TestFactoryCreatesCircle() {
    ShapeFactory factory;
    RegisterBuiltinShapes(factory);

    auto shape = factory.Create("circle 5 2 10 20");
    assert(shape != nullptr);
    assert(shape->Color() == 2);
    assert(std::abs(shape->Area() - 3.14159265358979 * 25.0) < 1e-6);
    assert(shape->Position().x == 10 && shape->Position().y == 20);
}

// Factory: sabe enumerarse con su sintaxis (de ahi sale la ayuda).
static void TestFactoryListsItself() {
    ShapeFactory factory;
    RegisterBuiltinShapes(factory);

    std::vector<std::string> usages;
    factory.ForEachListed(
        [&usages](const std::string& usage, const std::string& description) {
            assert(!description.empty());
            usages.push_back(usage);
        });

    assert(usages.size() == 2);                              // map => ordenados
    assert(usages[0] == "circle <radio> <color> <x> <y>");
    assert(usages[1] == "square <lado> <color> <x> <y>");
}

// Prototype: una figura sabe clonarse manteniendo su estado.
static void TestPrototypeClone() {
    Circle original(3.0, 1, Point{4, 5});
    auto copy = original.Clone();

    assert(copy->Color() == 1);
    assert(std::abs(copy->Area() - original.Area()) < 1e-9);
    assert(copy->Position().x == 4 && copy->Position().y == 5);
}

// ---------------------------------------------------------------------------
// Command, sobre el Canvas singleton
// ---------------------------------------------------------------------------

// Command: añadir es reversible (undo/redo).
static void TestAddUndoRedo() {
    ICanvas& canvas = Canvas::Instance();
    canvas.Clear();
    CommandManager manager;

    manager.Execute(std::make_unique<AddShapeCommand>(
        canvas, std::make_unique<Square>(2.0, 0, Point{0, 0})));
    assert(canvas.Count() == 1);

    assert(manager.Undo());
    assert(canvas.Count() == 0);

    assert(manager.Redo());
    assert(canvas.Count() == 1);
}

// Command: mover es reversible y restaura la posición previa.
static void TestMoveUndo() {
    ICanvas& canvas = Canvas::Instance();
    canvas.Clear();
    canvas.Add(std::make_unique<Circle>(1.0, 0, Point{0, 0}));
    CommandManager manager;

    manager.Execute(std::make_unique<MoveShapeCommand>(canvas, 0, Point{7, 8}));
    assert(canvas.At(0).Position().x == 7 && canvas.At(0).Position().y == 8);

    assert(manager.Undo());
    assert(canvas.At(0).Position().x == 0 && canvas.At(0).Position().y == 0);
}

// Prototype + Command: duplicar clona y es reversible.
static void TestDuplicate() {
    ICanvas& canvas = Canvas::Instance();
    canvas.Clear();
    canvas.Add(std::make_unique<Circle>(4.0, 5, Point{1, 1}));
    CommandManager manager;

    manager.Execute(std::make_unique<DuplicateShapeCommand>(canvas, 0));
    assert(canvas.Count() == 2);
    assert(canvas.At(1).Color() == 5);

    assert(manager.Undo());
    assert(canvas.Count() == 1);
}

// ---------------------------------------------------------------------------
// DIP: lo que el refactor hace posible
// ---------------------------------------------------------------------------

// Los comandos ya no necesitan el Singleton: valen contra cualquier ICanvas.
static void TestCommandsRunAgainstAFakeCanvas() {
    FakeCanvas canvas;
    CommandManager manager;

    manager.Execute(std::make_unique<AddShapeCommand>(
        canvas, std::make_unique<Square>(2.0, 3, Point{1, 2})));
    assert(canvas.adds == 1 && canvas.Count() == 1);

    manager.Execute(std::make_unique<DuplicateShapeCommand>(canvas, 0));
    assert(canvas.adds == 2 && canvas.Count() == 2);

    assert(manager.Undo());
    assert(canvas.removes == 1 && canvas.Count() == 1);
}

// La App lee de un IReader, no de std::cin: el REPL entero es testeable.
static void TestAppRunsAgainstFakeReaderAndWriter() {
    FakeCanvas canvas;
    StringReader reader(
        {"circle 5 2 0 0", "square 3 1 10 10", "duplicate 0", "undo", "exit"});
    RecordingWriter writer;

    App app(canvas, reader, writer);
    app.Run();

    assert(canvas.adds == 3);     // 2 figuras + 1 duplicado
    assert(canvas.removes == 1);  // el undo del duplicado
    assert(canvas.Count() == 2);
    assert(writer.Contains("deshecho"));
}

// "exit" corta el bucle: lo que venga después no se ejecuta.
static void TestExitStopsTheLoop() {
    FakeCanvas canvas;
    StringReader reader({"exit", "circle 5 2 0 0"});
    RecordingWriter writer;

    App app(canvas, reader, writer);
    app.Run();

    assert(canvas.adds == 0);
}

// Un verbo desconocido no rompe nada, y un índice inválido tampoco.
static void TestErrorsAreReportedNotThrown() {
    FakeCanvas canvas;
    StringReader reader({"jorobar", "move 99 1 1"});
    RecordingWriter writer;

    App app(canvas, reader, writer);
    app.Run();

    assert(writer.Contains("comando desconocido: jorobar"));
    assert(writer.Contains("error: "));
}

// ---------------------------------------------------------------------------
// OCP: añadir un verbo sin tocar App
// ---------------------------------------------------------------------------

// El test que justifica el registro: un verbo nuevo, cero cambios en App.
static void TestNewVerbWithoutTouchingApp() {
    FakeCanvas canvas;
    StringReader reader({"circle 5 2 0 0", "vaciar", "print"});
    RecordingWriter writer;

    App app(canvas, reader, writer);
    app.Registry().Register("vaciar", "", "borra el lienzo",
                            [](AppContext& context, std::istream&) {
                                context.canvas.Clear();
                                context.writer.Write("lienzo vaciado");
                                return Status::Continue;
                            });
    app.Run();

    assert(canvas.adds == 1);
    assert(canvas.Count() == 0);
    assert(writer.Contains("lienzo vaciado"));
}

// La ayuda se genera del registro y de la fábrica: no hay texto a mano.
static void TestHelpListsWhatIsRegistered() {
    FakeCanvas canvas;
    StringReader reader({});
    RecordingWriter writer;

    App app(canvas, reader, writer);
    app.Registry().Register(
        "vaciar", "", "borra el lienzo",
        [](AppContext&, std::istream&) { return Status::Continue; });
    app.Run();  // Run() imprime la ayuda al arrancar

    assert(writer.Contains("vaciar"));         // el verbo nuevo se lista solo
    assert(writer.Contains("borra el lienzo"));
    assert(writer.Contains("duplicate"));      // los de serie siguen ahi
    assert(writer.Contains("circle <radio>"));  // las figuras, con su sintaxis
    assert(writer.Contains("square <lado>"));
    assert(!writer.Contains("quit"));          // alias oculto: no ensucia
}

int main() {
    TestFactoryCreatesCircle();
    TestFactoryListsItself();
    TestPrototypeClone();
    TestAddUndoRedo();
    TestMoveUndo();
    TestDuplicate();
    TestCommandsRunAgainstAFakeCanvas();
    TestAppRunsAgainstFakeReaderAndWriter();
    TestExitStopsTheLoop();
    TestErrorsAreReportedNotThrown();
    TestNewVerbWithoutTouchingApp();
    TestHelpListsWhatIsRegistered();
    return 0;
}
