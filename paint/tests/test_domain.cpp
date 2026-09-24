// Tests del módulo 1 (dominio) sin Canvas: Point, contrato IShape, Circle y
// Square. Sigue el plan de day-03/ejercicio_plan_de_pruebas_paint.md, §6.1-6.4.
// Ningún doble: una figura no tiene colaboradores.
#include <gtest/gtest.h>

#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>

#include "paint/Circle.h"
#include "paint/IShape.h"
#include "paint/Point.h"
#include "paint/Square.h"

namespace paint {
// Para que gtest escriba "(7, 8)" en vez de un volcado de bytes cuando falla
// una comparación de Point. Va aquí y no en Point.h: es cosa del test.
void PrintTo(const Point& p, std::ostream* os) {
    *os << "(" << p.x << ", " << p.y << ")";
}
}  // namespace paint

using namespace paint;

namespace {
constexpr double kPi = 3.14159265358979;
constexpr double kTolerance = 1e-9;
}  // namespace

// ===========================================================================
// §6.1 Point
// ===========================================================================

TEST(Point, PorDefectoValeCeroCero) {
    Point p;

    EXPECT_EQ(0, p.x);
    EXPECT_EQ(0, p.y);
}

// operator== es código de producción nuevo: lleva sus propios tests.
TEST(Point, DosPuntosConLasMismasCoordenadasSonIguales) {
    EXPECT_EQ((Point{3, 4}), (Point{3, 4}));
}

TEST(Point, SiDifiereLaXNoSonIguales) {
    EXPECT_NE((Point{3, 4}), (Point{9, 4}));
}

TEST(Point, SiDifiereLaYNoSonIguales) {
    EXPECT_NE((Point{3, 4}), (Point{3, 9}));
}

// ===========================================================================
// §6.2 Contrato IShape — escrito una vez, ejecutado contra cada figura.
// Una figura nueva (Triangle) solo tiene que añadir su Maker y su línea en
// ShapeTypes para heredar la suite entera.
// ===========================================================================

template <typename T>
struct Maker;

template <>
struct Maker<Circle> {
    static constexpr const char* kName = "Circle";
    static std::unique_ptr<IShape> Make(int color, Point position) {
        return std::make_unique<Circle>(2.0, color, position);
    }
};

template <>
struct Maker<Square> {
    static constexpr const char* kName = "Square";
    static std::unique_ptr<IShape> Make(int color, Point position) {
        return std::make_unique<Square>(3.0, color, position);
    }
};

template <typename T>
class ShapeContract : public ::testing::Test {
protected:
    std::unique_ptr<IShape> shape = Maker<T>::Make(5, Point{10, 20});
};

using ShapeTypes = ::testing::Types<Circle, Square>;
TYPED_TEST_SUITE(ShapeContract, ShapeTypes);

// C1
TYPED_TEST(ShapeContract, MoveToDejaLaFiguraEnLaPosicionPedida) {
    this->shape->MoveTo(Point{7, 8});

    EXPECT_EQ((Point{7, 8}), this->shape->Position());
}

// C2
TYPED_TEST(ShapeContract, MoveToNoAlteraNiColorNiArea) {
    const int color = this->shape->Color();
    const double area = this->shape->Area();

    this->shape->MoveTo(Point{7, 8});

    EXPECT_EQ(color, this->shape->Color());
    EXPECT_DOUBLE_EQ(area, this->shape->Area());
}

// C3
TYPED_TEST(ShapeContract, CloneTieneElMismoEstadoObservable) {
    std::unique_ptr<IShape> clone = this->shape->Clone();

    EXPECT_DOUBLE_EQ(this->shape->Area(), clone->Area());
    EXPECT_EQ(this->shape->Color(), clone->Color());
    EXPECT_EQ(this->shape->Position(), clone->Position());
    EXPECT_EQ(this->shape->ToString(), clone->ToString());
}

// C4
TYPED_TEST(ShapeContract, MoverElClonNoMueveAlOriginal) {
    std::unique_ptr<IShape> clone = this->shape->Clone();

    clone->MoveTo(Point{99, 99});

    EXPECT_EQ((Point{10, 20}), this->shape->Position());
    EXPECT_NE(this->shape.get(), clone.get());
}

// C5
TYPED_TEST(ShapeContract, ElClonConservaElTipoDinamico) {
    std::unique_ptr<IShape> clone = this->shape->Clone();

    EXPECT_NE(nullptr, dynamic_cast<TypeParam*>(clone.get()));
}

// C6
TYPED_TEST(ShapeContract, ToStringContieneTipoAreaColorYPosicion) {
    const std::string text = this->shape->ToString();

    EXPECT_NE(std::string::npos, text.find(Maker<TypeParam>::kName));
    EXPECT_NE(std::string::npos,
              text.find("area: " + std::to_string(this->shape->Area())));
    EXPECT_NE(std::string::npos, text.find("color: 5"));
    EXPECT_NE(std::string::npos, text.find("pos: (10, 20)"));
}

// ===========================================================================
// §6.3 Circle — lo que solo es suyo
// ===========================================================================

TEST(Circle, ElConstructorDejaColorPosicionYAreaCoherentes) {
    // Arrange
    Point p{10, 10};
    Circle circle(2.0, 3, p);

    // Act — vacío: en un test de constructor, construir ES el acto.

    // Assert
    EXPECT_EQ(3, circle.Color());
    EXPECT_EQ((Point{10, 10}), circle.Position());
    EXPECT_NEAR(kPi * 4.0, circle.Area(), kTolerance);  // en vez de radio == 2
}

TEST(Circle, AreaEsPiPorRadioAlCuadrado) {
    Circle circle(5.0, 0, Point{});

    EXPECT_NEAR(kPi * 25.0, circle.Area(), kTolerance);
}

TEST(Circle, RadioCeroDaAreaCero) {
    Circle circle(0.0, 0, Point{});

    EXPECT_DOUBLE_EQ(0.0, circle.Area());
}

// HUECO — hoy Circle(-2.0, ...) construye y devuelve área positiva. El test
// fija UNA decisión (lanzar invalid_argument) y queda desactivado hasta que se
// tome de verdad: quitadle el DISABLED_ y lo veréis en rojo.
TEST(Circle, DISABLED_RadioNegativoLanzaInvalidArgument) {
    EXPECT_THROW(Circle(-2.0, 0, Point{}), std::invalid_argument);
}

// ===========================================================================
// §6.4 Square — simétrico
// ===========================================================================

TEST(Square, ElConstructorDejaColorPosicionYAreaCoherentes) {
    Square square(3.0, 4, Point{1, 2});

    EXPECT_EQ(4, square.Color());
    EXPECT_EQ((Point{1, 2}), square.Position());
    EXPECT_DOUBLE_EQ(9.0, square.Area());
}

TEST(Square, AreaEsLadoAlCuadrado) {
    Square square(7.0, 0, Point{});

    EXPECT_DOUBLE_EQ(49.0, square.Area());  // exacta: lado entero, sin kPi
}

TEST(Square, LadoCeroDaAreaCero) {
    Square square(0.0, 0, Point{});

    EXPECT_DOUBLE_EQ(0.0, square.Area());
}

// HUECO — mismo caso, misma decisión pendiente.
TEST(Square, DISABLED_LadoNegativoLanzaInvalidArgument) {
    EXPECT_THROW(Square(-3.0, 0, Point{}), std::invalid_argument);
}
