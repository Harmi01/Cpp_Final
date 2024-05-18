#pragma once
#include <QtWidgets>
#include <QtMath>
#include <QSet>
#include <QVector3D>
#include "lighting.h"
#include "representation.h"

struct ClippedLine {
	QVector<QPoint> points;
	bool isClipped = false;
};

class ViewerWidget :public QWidget {
	Q_OBJECT
private:
	QSize areaSize = QSize(0, 0);
	QImage* img = nullptr;
	QPainter* painter = nullptr;
	uchar* data = nullptr;

	bool drawLineActivated = false;
	bool drawCircleActivated = false;
	bool drawPolygonActivated = false;
	QPoint drawLineBegin = QPoint(0, 0);
	QPoint drawCircleCenter = QPoint(0, 0);
	QPoint moveStart = QPoint(0, 0);
	QVector<QPoint> originalPointsVector;

	std::vector<std::pair<std::reference_wrapper<Shape>, int>> zBuffer;
	int currentLayer;
	QColor borderColor, fillingColor;

public:
	ViewerWidget(QSize imgSize, QWidget* parent = Q_NULLPTR);
	~ViewerWidget();
	void resizeWidget(QSize size);

	//Image functions
	bool setImage(const QImage& inputImg);
	QImage* getImage() { return img; };
	QPainter* getPainter() { return painter; }
	bool isEmpty();
	bool changeSize(int width, int height);

	void setPixel(int x, int y, uchar r, uchar g, uchar b, uchar a = 255);
	void setPixel(int x, int y, double valR, double valG, double valB, double valA = 1.);
	void setPixel(int x, int y, const QColor& color);
	bool isInside(QPoint point) { return (point.x() > 0 && point.y() > 0 && point.x() < img->width() - 1 && point.y() < img->height() - 1) ? true : false; }
	bool isInside(int x, int y) { return (x > 0 && y > 0 && x < img->width() && y < img->height()) ? true : false; }

	//Draw functions
	void drawShape(Shape& shape);
	void moveShapeUp(int zBufferPosition);
	void moveShapeDown(int zBufferPosition);
	void addToZBuffer(Shape& shape, int depth);
	void redrawAllShapes();

	//	Lines
	void drawLine(Line& line);
	void setDrawLineBegin(QPoint begin) { drawLineBegin = begin; }
	QPoint getDrawLineBegin() { return drawLineBegin; }
	void setDrawLineActivated(bool state) { drawLineActivated = state; }
	bool getDrawLineActivated() { return drawLineActivated; }
	void drawLineBresenham(QVector<QPoint>& linePoints);
	void moveLine(const QPoint& offset);
	void turnLine(int angle);
	QPoint getLineCenter(const Line& line) const;
	void scaleLine(double scaleX, double scaleY);
	
	//	Circles
	void drawCircle(Circle& circle);
	void drawSymmetricPoints(const QPoint& center, int x, int y);
	void drawSymmetricPointsFilled(const QPoint& center, int x, int y);
	void setDrawCircleActivated(bool state) { drawCircleActivated = state; }
	bool getDrawCircleActivated() { return drawCircleActivated; }
	void setDrawCircleCenter(QPoint center) { drawCircleCenter = center; }
	QPoint getDrawCircleCenter() { return drawCircleCenter; }
	void moveCircle(const QPoint& offset);
	void scaleCircle(double scaleX, double scaleY);

	// Polygons
	void drawPolygon(MyPolygon& polygon);
	void setMoveStart(QPoint start) { moveStart = start; };
	QPoint getMoveStart() { return moveStart; }
	void movePolygon(const QPoint& offset);
	void turnPolygon(int angle);
	QPoint getPolygonCenter(const MyPolygon& polygon) const;
	void scalePolygon(double scaleX, double scaleY);
	
	//  **Trimming functions**
	QVector<QPoint> trimPolygon(const MyPolygon& polygon);
	void clipLineWithPolygon(QVector<QPoint> linePoints);

	//	**Polygon filling handling**

	//	<Subclass for edges>
	class Edge {
	private:
		QPoint startPoint_;  // Zaèiatoèný bod hrany
		QPoint endPoint_;    // Koncový bod hrany
		double slope_;       // Sklon hrany
		double x_;           // Aktuálna x pozícia pre vyplòovanie pomocou ScanLine algoritmu
		double w_;           // Inverzný sklon pre aktualizáciu x

	public:
		// Konštruktor prijíma zaèiatoèný a koncový bod hrany a inicializuje èlenské premenné
		Edge() : startPoint_(QPoint(0, 0)), endPoint_(QPoint(0, 0)), slope_(0.0), x_(0.0), w_(0.0) {}
		Edge(QPoint start, QPoint end) : startPoint_(start), endPoint_(end), x_(0.0), w_(0.0) {
			calculateAttributes();
		}

		// Výpoèet atribútov hrany (sklon, inverzný sklon)
		void calculateAttributes() {
			double dx = static_cast<double>(endPoint_.x() - startPoint_.x());
			double dy = static_cast<double>(endPoint_.y() - startPoint_.y());

			if (dx == 0) {
				slope_ = std::numeric_limits<double>::max(); // Nastavenie smernice/sklonu na maximálnu hodnotu pre double, reprezentuje vertikálny sklon
				w_ = 0; // Pre vertikálne hrany je inverzný sklon nulový
			}
			else {
				slope_ = dy / dx; // Výpoèet sklonu ako pomer zmeny y k zmene x
				w_ = 1.0 / slope_; // Výpoèet inverzného sklonu
			}

			x_ = static_cast<double>(startPoint_.x());

			// y-ová súradnica zaèiatoèného bodu je vždy menšia ako y-ová súradnica koncového bodu
			if (startPoint_.y() > endPoint_.y()) {
				swapStartEndPoints();
				calculateAttributes(); // Rekurzívny prepoèet atribútov, ak došlo k výmene bodov
			}
		}

		// Metóda pre výmenu zaèiatoèného a koncového bodu
		void swapStartEndPoints() {
			std::swap(startPoint_, endPoint_);
		}

		// Úprava koncového bodu hrany o -1 na y-ovej súradnici, použitie po naèítaní hrán
		void adjustEndPoint() {
			endPoint_.setY(endPoint_.y() - 1);
		}

		// Gettery pre prístup k èlenským premenným
		QPoint startPoint() const { return startPoint_; }
		QPoint endPoint() const { return endPoint_; }
		double slope() const { return slope_; }
		double x() const { return x_; }
		double w() const { return w_; }

		// Setter pre nastavenie aktuálnej x-ovej pozície
		void setX(double x) { x_ = x; }
	};

	static bool compareByY(const Edge& edge1, const Edge& edge2){ return edge1.startPoint().y() < edge2.startPoint().y(); }
	static bool compareByX(const Edge& edge1, const Edge& edge2){ return edge1.x() < edge2.x(); }
	
	void fillPolygon(const MyPolygon& polygon);
	QVector<Edge> loadEdges(const QVector<QPoint>& points);

	//	** Curve function declarations **
	void drawCurve(BezierCurve& curve);
	void moveCurve(const QPoint& offset);
	void scaleCurve(double scaleX, double scaleY);
	void turnCurve(int angle);
	QPoint calculateCurveCenter(const BezierCurve& curve) const;

	//Get/Set functions
	uchar* getData() { return data; }
	void setDataPtr() { data = img->bits(); }
	void setPainter() { painter = new QPainter(img); }
	void setBorderColor(QColor border) { borderColor = border; }
	void setFillingColor(QColor filling) { fillingColor = filling; }
	void setLayer(int layer) { currentLayer = layer; }

	int getImgWidth() { return img->width(); };
	int getImgHeight() { return img->height(); };

	void clear();

public slots:
	void paintEvent(QPaintEvent* event) Q_DECL_OVERRIDE;
};
