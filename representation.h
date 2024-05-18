#pragma once

#include <QPoint>
#include <QVector>
#include <memory>
#include <variant>

class Shape {
public:
    enum ShapeType { LINE, RECTANGLE, POLYGON, CIRCLE, BEZIER_CURVE };

    Shape(ShapeType type, int zBufferPosition, bool isFilled)
        : type(type), zBufferPosition(zBufferPosition), isFilled(isFilled) {}
    virtual ~Shape() {}

    ShapeType getType() const { return type; }
    int getZBufferPosition() const { return zBufferPosition; }
    bool getIsFilled() const { return isFilled; }

    virtual QVector<QPoint> getPoints() const = 0;
    virtual void addPoint(QPoint point) {}

protected:
    ShapeType type;
    int zBufferPosition;
    bool isFilled;
};

class Line : public Shape {
public:
    Line(const QPoint& p1, const QPoint& p2, int zBufferPosition, bool isFilled)
        : Shape(Shape::LINE, zBufferPosition, isFilled), p1(p1), p2(p2) {}

    QVector<QPoint> getPoints() const override {
        return { p1, p2 };
    }

private:
    QPoint p1, p2;
};

class MyRectangle : public Shape {
public:
    MyRectangle(const QPoint& p1, const QPoint& p2, int zBufferPosition, bool isFilled)
        : Shape(Shape::RECTANGLE, zBufferPosition, isFilled), p1(p1), p2(p2) {}

    QVector<QPoint> getPoints() const override {
        return { p1, p2, QPoint(p1.x(), p2.y()), QPoint(p2.x(), p1.y()) };
    }

private:
    QPoint p1, p2;
};

class MyPolygon : public Shape {
public:
    MyPolygon(const QVector<QPoint>& points, int zBufferPosition, bool isFilled)
        : Shape(Shape::POLYGON, zBufferPosition, isFilled), points(points) {}

    QVector<QPoint> getPoints() const override {
        return points;
    }

    void addPoint(QPoint point) override {
        points.append(point);
    }

private:
    QVector<QPoint> points;
};

class Circle : public Shape {
public:
    Circle(const QPoint& center, const QPoint& edge, int zBufferPosition, bool isFilled)
        : Shape(Shape::CIRCLE, zBufferPosition, isFilled), center(center), edge(edge) {}

    QVector<QPoint> getPoints() const override {
        return { center, edge };
    }

private:
    QPoint center, edge;
};

class BezierCurve : public Shape {
public:
    BezierCurve(const QVector<QPoint>& controlPoints, int zBufferPosition, bool isFilled)
        : Shape(Shape::BEZIER_CURVE, zBufferPosition, isFilled), controlPoints(controlPoints) {}

    QVector<QPoint> getPoints() const override {
        return controlPoints;
    }

    void addPoint(QPoint point) override {
        controlPoints.append(point);
    }

private:
    QVector<QPoint> controlPoints;
};