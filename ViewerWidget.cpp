#include   "ViewerWidget.h"

ViewerWidget::ViewerWidget(QSize imgSize, QWidget* parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_StaticContents);
	setMouseTracking(true);
	if (imgSize != QSize(0, 0)) {
		img = new QImage(imgSize, QImage::Format_ARGB32);
		img->fill(Qt::white);
		resizeWidget(img->size());
		setPainter();
		setDataPtr();
	}
}
ViewerWidget::~ViewerWidget()
{
	delete painter;
	delete img;
}
void ViewerWidget::resizeWidget(QSize size)
{
	this->resize(size);
	this->setMinimumSize(size);
	this->setMaximumSize(size);
}

//-----------------------------------------
//		*** Image functions ***
//-----------------------------------------

bool ViewerWidget::setImage(const QImage& inputImg)
{
	if (img != nullptr) {
		delete painter;
		delete img;
	}
	img = new QImage(inputImg);
	if (!img) {
		return false;
	}
	resizeWidget(img->size());
	setPainter();
	setDataPtr();
	update();

	return true;
}
bool ViewerWidget::isEmpty()
{
	if (img == nullptr) {
		return true;
	}

	if (img->size() == QSize(0, 0)) {
		return true;
	}
	return false;
}

bool ViewerWidget::changeSize(int width, int height)
{
	QSize newSize(width, height);

	if (newSize != QSize(0, 0)) {
		if (img != nullptr) {
			delete painter;
			delete img;
		}

		img = new QImage(newSize, QImage::Format_ARGB32);
		if (!img) {
			return false;
		}
		img->fill(Qt::white);
		resizeWidget(img->size());
		setPainter();
		setDataPtr();
		update();
	}

	return true;
}

void ViewerWidget::clear()
{
	img->fill(Qt::white);
	zBuffer.clear();
	update();
}

void ViewerWidget::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	QRect area = event->rect();
	painter.drawImage(area, *img, area);
}

//-----------------------------------------
//		*** Point drawing functions ***
//-----------------------------------------

void ViewerWidget::setPixel(int x, int y, uchar r, uchar g, uchar b, uchar a)
{
	r = r > 255 ? 255 : (r < 0 ? 0 : r);
	g = g > 255 ? 255 : (g < 0 ? 0 : g);
	b = b > 255 ? 255 : (b < 0 ? 0 : b);
	a = a > 255 ? 255 : (a < 0 ? 0 : a);

	size_t startbyte = y * img->bytesPerLine() + x * 4;
	data[startbyte] = b;
	data[startbyte + 1] = g;
	data[startbyte + 2] = r;
	data[startbyte + 3] = a;
}
void ViewerWidget::setPixel(int x, int y, double valR, double valG, double valB, double valA)
{
	valR = valR > 1 ? 1 : (valR < 0 ? 0 : valR);
	valG = valG > 1 ? 1 : (valG < 0 ? 0 : valG);
	valB = valB > 1 ? 1 : (valB < 0 ? 0 : valB);
	valA = valA > 1 ? 1 : (valA < 0 ? 0 : valA);

	size_t startbyte = y * img->bytesPerLine() + x * 4;
	data[startbyte] = static_cast<uchar>(255 * valB);
	data[startbyte + 1] = static_cast<uchar>(255 * valG);
	data[startbyte + 2] = static_cast<uchar>(255 * valR);
	data[startbyte + 3] = static_cast<uchar>(255 * valA);
}
void ViewerWidget::setPixel(int x, int y, const QColor& color)
{
	if (!color.isValid() || x < 0 || y < 0 || x >= img->width() || y >= img->height()) {
		return;
	}

	size_t startbyte = y * img->bytesPerLine() + x * 4;

	data[startbyte] = color.blue();
	data[startbyte + 1] = color.green();
	data[startbyte + 2] = color.red();
	data[startbyte + 3] = color.alpha();
}

//-----------------------------------------
//		*** Drawing functions ***
//-----------------------------------------
void ViewerWidget::drawShape(const Shape& shape) {
	switch (shape.getType()) {
	case Shape::LINE: {
		const Line& line = static_cast<const Line&>(shape);
		drawLine(line);
		break;
	}
	case Shape::RECTANGLE: {
		const MyRectangle& rectangle = static_cast<const MyRectangle&>(shape);
		break;
	}
	case Shape::POLYGON: {
		const MyPolygon& polygon = static_cast<const MyPolygon&>(shape);
		drawPolygon(polygon);
		break;
	}
	case Shape::CIRCLE: {
		const Circle& circle = static_cast<const Circle&>(shape);
		drawCircle(circle);
		break;
	}
	case Shape::BEZIER_CURVE: {
		const BezierCurve& curve = static_cast<const BezierCurve&>(shape);
		drawCurve(curve);
		break;
	}
	default:
		break;
	}
}

void ViewerWidget::addToZBuffer(const Shape& shape, int depth) {
	zBuffer.push_back(std::make_pair(shape, depth));

	std::sort(zBuffer.begin(), zBuffer.end(), [](const std::pair<Shape, int>& a, const std::pair<Shape, int>& b) {
		return a.second < b.second;
		});
}

void ViewerWidget::moveShapeUp(int zBufferPosition) {
	auto it = std::find_if(zBuffer.begin(), zBuffer.end(), [zBufferPosition](const auto& pair) {
		return pair.second == zBufferPosition;
		});

	if (it != zBuffer.end() && it != zBuffer.begin()) {
		auto prevIt = std::prev(it);
		std::iter_swap(it, prevIt);
		std::swap(it->second, prevIt->second);
		redrawAllShapes();
	}
}

void ViewerWidget::moveShapeDown(int zBufferPosition) {
	auto it = std::find_if(zBuffer.begin(), zBuffer.end(), [zBufferPosition](const auto& pair) {
		return pair.second == zBufferPosition;
		});

	if (it != zBuffer.end() && (it + 1) != zBuffer.end()) {
		auto nextIt = std::next(it);
		std::iter_swap(it, nextIt);
		std::swap(it->second, nextIt->second);
		redrawAllShapes();
	}
}

void ViewerWidget::redrawAllShapes() {
	clear();
	for (const auto& shapePair : zBuffer) {
		drawShape(shapePair.first);
	}
	update();
}

//-----------------------------------------
//		*** Line functions ***
//-----------------------------------------
void ViewerWidget::drawLine(const Line& line)
{
	painter->setPen(QPen(borderColor));

	QVector<QPoint> linePoints = line.getPoints();
	QVector<QPoint> lineToClip = line.getPoints();

	clipLineWithPolygon(lineToClip);

	// Overenie, ci bola usecka orezana a aktualizacia linePoints podla potreby
	if (lineToClip.size() == 2) { // Kontrola, ci orezanie zmenilo body
		linePoints.append(lineToClip[0]);
		linePoints.append(lineToClip[1]);
	}
	else {
		// Ak orezanie uplne odstranilo usecku alebo nezmenilo body, vykreslenie povodnej usecky
		linePoints.append(line.getPoints()[0]);
		linePoints.append(line.getPoints()[1]);
	}

	drawLineBresenham(linePoints);
	addToZBuffer(line, line.getZBufferPosition());
	redrawAllShapes();
}

void ViewerWidget::clipLineWithPolygon(QVector<QPoint> linePoints) {
	if (linePoints.size() < 2) {
		return; // Nedostatok bodov na vytvorenie èiary
	}

	QVector<QPoint> clippedPoints;
	QPoint P1 = linePoints[0], P2 = linePoints[1];
	double t_min = 0, t_max = 1; // Inicializácia t-hodnôt
	QPoint d = P2 - P1; // Smerový vektor úseèky
	//qDebug() << "Povodny useckovy segment od" << P1 << "do" << P2;

	// Definícia hrán orezovacieho obdåžnika
	QVector<QPoint> E = { QPoint(0,0), QPoint(500,0), QPoint(500,500), QPoint(0,500) };

	for (int i = 0; i < E.size(); i++) {
		QPoint E1 = E[i];
		QPoint E2 = E[(i + 1) % E.size()]; // Zopnutie pre poslednú hranu

		QPoint normal = QPoint(E2.y() - E1.y(), E1.x() - E2.x()); // Opravené znamienko

		QPoint w = P1 - E1; // Vektor z koncového bodu hrany k P1

		double dn = d.x() * normal.x() + d.y() * normal.y();
		double wn = w.x() * normal.x() + w.y() * normal.y();
		if (dn != 0) {
			double t = -wn / dn;
			//qDebug() << "Hodnota t priesecnika s hranou" << i << ":" << t;
			if (dn > 0 && t <= 1) {
				t_min = std::max(t, t_min); // Aktualizácia t_min, ak dn > 0 a t <= 1
			}
			else if (dn < 0 && t >= 0) {
				t_max = std::min(t, t_max); // Aktualizácia t_max, ak dn < 0 a t >= 0
			}
		}
	}

	//qDebug() << "t_min:" << t_min << "t_max:" << t_max;

	if (t_min < t_max) {
		QPoint clippedP1 = P1 + (P2 - P1) * t_min; // Výpoèet orezaného zaèiatoèného bodu
		QPoint clippedP2 = P1 + (P2 - P1) * t_max; // Výpoèet orezaného koncového bodu
		//qDebug() << "Orezany useckovy segment od" << clippedP1 << "do" << clippedP2;

		clippedPoints.push_back(clippedP1);
		clippedPoints.push_back(clippedP2);
	}
	else {
		//qDebug() << "Useckovy segment je uplne mimo orezovacej oblasti alebo je neplatny.";
	}

	// Aktualizácia pôvodných linePoints s orezanými bodmi
	if (!clippedPoints.isEmpty()) {
		linePoints = clippedPoints;
	}
}

void ViewerWidget::drawLineBresenham(QVector<QPoint>& linePoints) {
	int p, k1, k2;
	int dx = linePoints.last().x() - linePoints.first().x();  // Rozdiel x súradníc
	int dy = linePoints.last().y() - linePoints.first().y();  // Rozdiel y súradníc

	int adx = abs(dx); // Absolútna hodnota dx
	int ady = abs(dy); // Absolútna hodnota dy

	int x = linePoints.first().x(); // Zaèiatoèná x pozícia
	int y = linePoints.first().y(); // Zaèiatoèná y pozícia

	int incrementX = (dx > 0) ? 1 : -1; // Urèenie smeru posunu po x-ovej osi
	int incrementY = (dy > 0) ? 1 : -1; // Urèenie smeru posunu po y-ovej osi

	if (adx > ady) {
		// Èiara je strmšia v x-ovej osi
		p = 2 * ady - adx;  // Inicializácia rozhodovacieho parametra
		k1 = 2 * ady;       // Konštanta pre horizontálny krok
		k2 = 2 * (ady - adx);  // Konštanta pre diagonálny krok

		while (x != linePoints.last().x()) {
			painter->drawPoint(x, y); // Kreslenie bodu na aktuálnych súradniciach
			x += incrementX; // Posun v x-ovej osi
			if (p >= 0) {
				y += incrementY; // Posun v y-ovej osi, ak je to potrebné
				p += k2; // Aktualizácia rozhodovacieho parametra
			}
			else {
				p += k1; // Aktualizácia rozhodovacieho parametra
			}
		}
	}
	else {
		// Èiara je strmšia v y-ovej osi
		p = 2 * adx - ady;  // Inicializácia rozhodovacieho parametra
		k1 = 2 * adx;       // Konštanta pre vertikálny krok
		k2 = 2 * (adx - ady);  // Konštanta pre diagonálny krok

		while (y != linePoints.last().y()) {
			painter->drawPoint(x, y); // Kreslenie bodu na aktuálnych súradniciach
			y += incrementY; // Posun v y-ovej osi
			if (p >= 0) {
				x += incrementX; // Posun v x-ovej osi, ak je to potrebné
				p += k2; // Aktualizácia rozhodovacieho parametra
			}
			else {
				p += k1; // Aktualizácia rozhodovacieho parametra
			}
		}
	}

	painter->drawPoint(linePoints.last().x(), linePoints.last().y()); // Vykreslenie posledného bodu
	update();
}

void ViewerWidget::moveLine(const QPoint& offset) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::LINE) {
			Line line = static_cast<Line&>(pair.first);
			QVector<QPoint> points = line.getPoints();

			for (QPoint& point : points) {
				point += offset;
			}

			Line movedLine(points[0], points[1], line.getZBufferPosition(), line.getIsFilled());
			pair.first = movedLine;

			redrawAllShapes();
		}
	}
}

void ViewerWidget::turnLine(int angle) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::LINE) {
			Line line = static_cast<Line&>(pair.first);
			QVector<QPoint> points = line.getPoints();
			QPoint center = getLineCenter(line);

			double radians = qDegreesToRadians(static_cast<double>(angle));
			double cosAngle = std::cos(radians);
			double sinAngle = std::sin(radians);

			QVector<QPoint> rotatedPoints;

			for (QPoint& point : points) {
				int translatedX = point.x() - center.x();
				int translatedY = point.y() - center.y();

				int rotatedX = static_cast<int>(translatedX * cosAngle - translatedY * sinAngle);
				int rotatedY = static_cast<int>(translatedX * sinAngle + translatedY * cosAngle);

				rotatedX += center.x();
				rotatedY += center.y();

				rotatedPoints.push_back(QPoint(rotatedX, rotatedY));
			}

			Line rotatedLine(rotatedPoints[0], rotatedPoints[1], line.getZBufferPosition(), line.getIsFilled());
			pair.first = rotatedLine;

			redrawAllShapes();
		}
	}
}

QPoint ViewerWidget::getLineCenter(const Line& line) const {
	QVector<QPoint> points = line.getPoints();
	if (points.isEmpty()) {
		return QPoint();
	}

	double centroidX = (points.first().x() + points.last().x()) / 2;
	double centroidY = (points.first().y() + points.last().y()) / 2;

	return QPoint(static_cast<int>(centroidX), static_cast<int>(centroidY));
}

void ViewerWidget::scaleLine(double scaleX, double scaleY) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::LINE) {
			Line line = static_cast<Line&>(pair.first);
			QVector<QPoint> points = line.getPoints();
			QPoint center = getLineCenter(line);

			QVector<QPoint> scaledPoints;

			for (QPoint& point : points) {
				double newX = center.x() + (point.x() - center.x()) * scaleX;
				double newY = center.y() + (point.y() - center.y()) * scaleY;

				scaledPoints.append(QPoint(static_cast<int>(std::round(newX)), static_cast<int>(std::round(newY))));
			}

			Line scaledLine(scaledPoints[0], scaledPoints[1], line.getZBufferPosition(), line.getIsFilled());
			pair.first = scaledLine;

			redrawAllShapes();
		}
	}
}

//-----------------------------------------
//		*** Circle functions ***
//-----------------------------------------
void ViewerWidget::drawCircle(const Circle& circle) {
	QPoint center = circle.getPoints()[0];
	QPoint radiusPoint = circle.getPoints()[1];
	int r = std::sqrt(std::pow(radiusPoint.x() - center.x(), 2) + std::pow(radiusPoint.y() - center.y(), 2));
	int x = 0;
	int y = r;
	int p = 1 - r;

	if (circle.getIsFilled()) {
		painter->setPen(QPen(fillingColor));
		for (int i = -r; i <= r; i++) {
			painter->drawPoint(center.x() + i, center.y() + r);
			painter->drawPoint(center.x() + i, center.y() - r);
		}
	}
	else {
		painter->setPen(QPen(borderColor));
		drawSymmetricPoints(center, x, y);
	}

	while (x < y) {
		x++;
		if (p < 0) {
			p += 2 * x + 1;
		}
		else {
			y--;
			p += 2 * (x - y) + 1;
		}

		if (circle.getIsFilled()) {
			painter->setPen(QPen(fillingColor));
			drawSymmetricPointsFilled(center, x, y);
		}
		else {
			painter->setPen(QPen(borderColor));
			drawSymmetricPoints(center, x, y);
		}
	}

	if (circle.getIsFilled()) {
		painter->setPen(QPen(borderColor));
		x = 0;
		y = r;
		p = 1 - r;
		drawSymmetricPoints(center, x, y);

		while (x < y) {
			x++;
			if (p < 0) {
				p += 2 * x + 1;
			}
			else {
				y--;
				p += 2 * (x - y) + 1;
			}
			drawSymmetricPoints(center, x, y);
		}
	}

	addToZBuffer(circle, circle.getZBufferPosition());
	redrawAllShapes();
}

void ViewerWidget::drawSymmetricPoints(const QPoint& center, int x, int y) {
	QPoint points[8] = {
		QPoint(x, y),
		QPoint(y, x),
		QPoint(-x, y),
		QPoint(-y, x),
		QPoint(-x, -y),
		QPoint(-y, -x),
		QPoint(x, -y),
		QPoint(y, -x)
	};

	for (auto& point : points) {
		painter->drawPoint(center.x() + point.x(), center.y() + point.y());
	}
}

void ViewerWidget::drawSymmetricPointsFilled(const QPoint& center, int x, int y) {
	for (int i = -x; i <= x; i++) {
		painter->drawPoint(center.x() + i, center.y() + y);
		painter->drawPoint(center.x() + i, center.y() - y);
	}
	for (int i = -y; i <= y; i++) {
		painter->drawPoint(center.x() + i, center.y() + x);
		painter->drawPoint(center.x() + i, center.y() - x);
	}
}

void ViewerWidget::moveCircle(const QPoint& offset) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::CIRCLE) {
			Circle circle = static_cast<Circle&>(pair.first);
			QVector<QPoint> points = circle.getPoints();

			for (QPoint& point : points) {
				point += offset;
			}

			Circle movedCircle(points[0], points[1], circle.getZBufferPosition(), circle.getIsFilled());
			pair.first = movedCircle;

			redrawAllShapes();
		}
	}
}

void ViewerWidget::scaleCircle(double scaleX, double scaleY) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::CIRCLE) {
			Circle circle = static_cast<Circle&>(pair.first);
			QVector<QPoint> points = circle.getPoints();
			QPoint center = points[0];
			QPoint radiusPoint = points[1];

			int newX = center.x() + static_cast<int>((radiusPoint.x() - center.x()) * scaleX);
			int newY = center.y() + static_cast<int>((radiusPoint.y() - center.y()) * scaleY);
			points[1] = QPoint(newX, newY);

			Circle scaledCircle(points[0], points[1], circle.getZBufferPosition(), circle.getIsFilled());
			pair.first = scaledCircle;

			redrawAllShapes();
		}
	}
}


//-----------------------------------------
//		*** Polygon Functions ***
//-----------------------------------------
void ViewerWidget::drawPolygon(const MyPolygon& polygon) {
	const QVector<QPoint>& pointsVector = polygon.getPoints();

	if (pointsVector.size() < 2) {
		QMessageBox::warning(this, "Nizky pocet bodov", "Nebol dosiahnuty minimalny pocet bodov pre vykreslenie polygonu.");
		return;
	}

	painter->setPen(QPen(borderColor));
	QVector<QPoint> polygonPoints = pointsVector;

	// Kontrola, či sú všetky body mimo definovaného plátna/kresliacej oblasti
	bool allPointsOutside = std::all_of(pointsVector.begin(), pointsVector.end(), [this](const QPoint& point) {
		return !isInside(point);
		});

	if (allPointsOutside) {
		qDebug() << "Polygon je mimo hranicu.";
		return;
	}

	// Kontrola každého bodu, či sa nachádza v kresliacej oblasti, a prípadné orezanie polygonu
	for (QPoint point : pointsVector) {
		if (!isInside(point)) {
			polygonPoints = trimPolygon(polygon); // Orezanie polygónu, ak nejaké body prekračujú hranice
			break;
		}
	}

	if (polygon.getIsFilled()) {
		fillPolygon(polygon);
	}
	painter->setPen(QPen(borderColor));
	if (!polygonPoints.isEmpty()) {
		for (int i = 0; i < polygonPoints.size() - 1; i++) {
			drawLine(Line(polygonPoints.at(i), polygonPoints.at(i + 1), polygon.getZBufferPosition(), polygon.getIsFilled()));
		}
		drawLine(Line(polygonPoints.last(), polygonPoints.first(), polygon.getZBufferPosition(), polygon.getIsFilled()));
	}

	addToZBuffer(polygon, polygon.getZBufferPosition());
	redrawAllShapes();
}

QPoint ViewerWidget::getPolygonCenter(const MyPolygon& polygon) const {
	const QVector<QPoint>& points = polygon.getPoints();
	if (points.isEmpty()) {
		return QPoint();
	}

	double centroidX = 0;
	double centroidY = 0;
	for (const QPoint& point : points) {
		centroidX += point.x();
		centroidY += point.y();
	}

	centroidX /= points.size();
	centroidY /= points.size();

	return QPoint(static_cast<int>(centroidX), static_cast<int>(centroidY));
}

void ViewerWidget::scalePolygon(double scaleX, double scaleY) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::POLYGON) {
			MyPolygon polygon = static_cast<MyPolygon&>(pair.first);
			const QVector<QPoint>& points = polygon.getPoints();
			QPoint center = getPolygonCenter(polygon);

			QVector<QPoint> scaledPoints;
			for (const QPoint& point : points) {
				double newX = center.x() + (point.x() - center.x()) * scaleX;
				double newY = center.y() + (point.y() - center.y()) * scaleY;

				scaledPoints.append(QPoint(static_cast<int>(std::round(newX)), static_cast<int>(std::round(newY))));
			}

			MyPolygon scaledPolygon(scaledPoints, polygon.getZBufferPosition(), polygon.getIsFilled());
			pair.first = scaledPolygon;

			redrawAllShapes();
		}
	}
}

void ViewerWidget::movePolygon(const QPoint& offset) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::POLYGON) {
			MyPolygon polygon = static_cast<MyPolygon&>(pair.first);
			const QVector<QPoint>& points = polygon.getPoints();

			QVector<QPoint> movedPoints;
			for (const QPoint& point : points) {
				movedPoints.append(point + offset);
			}

			MyPolygon movedPolygon(movedPoints, polygon.getZBufferPosition(), polygon.getIsFilled());
			pair.first = movedPolygon;

			redrawAllShapes();
		}
	}
}

void ViewerWidget::turnPolygon(int angle) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::POLYGON) {
			MyPolygon polygon = static_cast<MyPolygon&>(pair.first);
			const QVector<QPoint>& points = polygon.getPoints();
			QPoint center = getPolygonCenter(polygon);

			double radians = qDegreesToRadians(static_cast<double>(angle));
			double cosAngle = std::cos(radians);
			double sinAngle = std::sin(radians);

			QVector<QPoint> rotatedPoints;

			for (const QPoint& point : points) {
				// Translate the point to the origin
				int translatedX = point.x() - center.x();
				int translatedY = point.y() - center.y();

				// Rotate the point
				int rotatedX = static_cast<int>(translatedX * cosAngle - translatedY * sinAngle);
				int rotatedY = static_cast<int>(translatedX * sinAngle + translatedY * cosAngle);

				// Translate the point back
				rotatedX += center.x();
				rotatedY += center.y();

				rotatedPoints.append(QPoint(rotatedX, rotatedY));
			}

			MyPolygon rotatedPolygon(rotatedPoints, polygon.getZBufferPosition(), polygon.getIsFilled());
			pair.first = rotatedPolygon;

			redrawAllShapes();
		}
	}
}

QVector<QPoint> ViewerWidget::trimPolygon(const MyPolygon& polygon) {
	QVector<QPoint> pointsVector = polygon.getPoints();

	if (pointsVector.isEmpty()) {
		qDebug() << "pointsVector je prazdny";
		return QVector<QPoint>();
	}

	QVector<QPoint> W, polygonPoints = pointsVector; // Inicializácia pomocného vektora a kópie pôvodného vektora bodov
	QPoint S; // Pomocný bod pre prácu s bodmi polygonu

	//qDebug() << "Pociatocny pointsVector:" << pointsVector;

	int xMin[] = { 0,0,-499,-499 }; // Hranice orezania pre x súradnice

	// Prechádzame štyri hranice orezania
	for (int i = 0; i < 4; i++) {
		if (pointsVector.size() == 0) {
			//qDebug() << "pointsVector ostal prazdny, vraciam polygon:" << polygon;
			return polygonPoints;
		}

		S = polygonPoints[polygonPoints.size() - 1]; // Nastavenie S na posledný bod v polygone

		// Iterácia cez všetky body polygonu
		for (int j = 0; j < polygonPoints.size(); j++) {
			// Logika orezania založená na pozícii bodu vzh¾adom na orezavaciu hranicu
			if (polygonPoints[j].x() >= xMin[i]) {
				if (S.x() >= xMin[i]) {
					W.push_back(polygonPoints[j]);
				}
				else {
					// Vytvorenie nového bodu na hranici orezania a jeho pridanie do výstupného vektora
					QPoint P(xMin[i], S.y() + (xMin[i] - S.x()) * ((polygonPoints[j].y() - S.y()) / static_cast<double>((polygonPoints[j].x() - S.x()))));
					W.push_back(P);
					W.push_back(polygonPoints[j]);
				}
			}
			else {
				if (S.x() >= xMin[i]) {
					// Vytvorenie bodu na hranici a pridanie do W, ak predchádzajúci bod bol vnútri orezanej oblasti
					QPoint P(xMin[i], S.y() + (xMin[i] - S.x()) * ((polygonPoints[j].y() - S.y()) / static_cast<double>((polygonPoints[j].x() - S.x()))));
					W.push_back(P);
				}
			}
			S = polygonPoints[j]; // Aktualizácia S na aktuálny bod pre ïalšiu iteráciu
		}
		//qDebug() << "Po orezavani s xMin[" << i << "] =" << xMin[i] << "W:" << W;
		polygonPoints = W; // Nastavenie orezaného polygonu ako aktuálneho polygonu pre ïalšiu iteráciu
		W.clear(); // Vymazanie pomocného vektora pre ïalšie použitie

		// Rotácia bodov polygonu pre ïalšiu hranicu orezania
		for (int j = 0; j < polygonPoints.size(); j++) {
			QPoint swappingPoint = polygonPoints[j];
			polygonPoints[j].setX(swappingPoint.y());
			polygonPoints[j].setY(-swappingPoint.x());
		}
		//qDebug() << "Po vymene, polygon:" << polygon;
	}

	//qDebug() << "Vysledny orezany polygon:" << polygon;
	return polygonPoints;
}

QVector<ViewerWidget::Edge> ViewerWidget::loadEdges(const QVector<QPoint>& points) {
	QVector<Edge> edges;

	for (int i = 0; i < points.size(); i++) {
		// Urèenie zaèiatoèného a koncového bodu hrany
		QPoint startPoint = points[i];
		QPoint endPoint = points[(i + 1) % points.size()]; // Po poslednom bode, vrátenie sa na prvý

		// Priame vytvorenie hrany bez manuálneho výpoètu sklonu
		Edge edge(startPoint, endPoint);

		// Upravenie koncového bodu hrany pod¾a pôvodnej logiky, ak je to potrebné
		edge.adjustEndPoint();

		edges.push_back(edge);
	}

	// Prepoèet sklonu a zmena bodov prebieha v konštruktore triedy

	std::sort(edges.begin(), edges.end(), compareByY); // Usporiadanie hrán pod¾a ich y-ovej súradnice
	return edges;
}

void ViewerWidget::fillPolygon(const MyPolygon& polygon) {
	const QVector<QPoint>& points = polygon.getPoints();

	if (points.isEmpty()) {
		//qDebug() << "Neobsahuje body pre vyplnanie.";
		return;
	}
	// Naèítanie hrán z bodov
	QVector<Edge> edges = loadEdges(points);
	if (edges.isEmpty()) {
		//qDebug() << "Vektor hran je prazdny.";
		return; // Predèasný výstup, ak neboli generované žiadne hrany
	}

	// Inicializácia yMin a yMax na základe prvej hrany
	int yMin = edges.front().startPoint().y();
	int yMax = edges.front().endPoint().y();

	// Nájdenie celkových yMin a yMax hodnôt
	for (const Edge& edge : edges) {
		int y1 = edge.startPoint().y();
		int y2 = edge.endPoint().y();
		yMin = qMin(yMin, qMin(y1, y2));
		yMax = qMax(yMax, qMax(y1, y2));
	}

	//qDebug() << "Prepocitane yMin:" << yMin << "yMax:" << yMax;

	// Kontrola platnosti hodnôt yMin a yMax
	if (yMin >= yMax) {
		//qDebug() << "Neplatne yMin a yMax hodnoty. Mozne nespravne nastavenie hrany.";
		return;
	}

	// Tabu¾ka hrán, inicializovaná tak, aby pokrývala od yMin po yMax
	QVector<QVector<Edge>> TH(yMax - yMin + 1);

	//qDebug() << "yMin:" << yMin << "yMax:" << yMax;

	// Populácia tabu¾ky hrán
	for (const auto& edge : edges) {
		int index = edge.startPoint().y() - yMin; // Index založený na offsete yMin
		if (index < 0 || index >= TH.size()) {
			//qDebug() << "Invalid index:" << index << "for edge start point y:" << edge.startPoint().y();
			continue;
		}
		TH[index].append(edge);
	}

	QVector<Edge> activeEdgeList; // Zoznam aktívnych hrán (AEL)

	// Zaèiatok prechodu scan line od yMin po yMax
	for (int y = yMin; y <= yMax; y++) {
		// Pridanie hrán do AEL
		for (const auto& edge : TH[y - yMin]) {
			activeEdgeList.append(edge);
		}

		// Zoradenie AEL pod¾a aktuálnej hodnoty X
		std::sort(activeEdgeList.begin(), activeEdgeList.end(), [](const Edge& a, const Edge& b) {
			return a.x() < b.x();
			});

		// Kreslenie èiar medzi pármi hodnôt X
		for (int i = 0; i < activeEdgeList.size(); i += 2) {
			if (i + 1 < activeEdgeList.size()) {
				int startX = qRound(activeEdgeList[i].x());
				int endX = qRound(activeEdgeList[i + 1].x());
				for (int x = startX; x <= endX; x++) {
					setPixel(x, y, fillingColor); // Vyplnenie medzi hranami
				}
			}
		}

		// Aktualizácia a odstránenie hrán z AEL
		QMutableVectorIterator<Edge> it(activeEdgeList);
		while (it.hasNext()) {
			Edge& edge = it.next();
			if (edge.endPoint().y() == y) {
				it.remove(); // Odstránenie hrany, ak konèí na aktuálnej scan line
			}
			else {
				edge.setX(edge.x() + edge.w()); // Aktualizácia X pre ïalšiu scan line
			}
		}
	}
}

//-----------------------------------------
//		*** Curve functions ***
//-----------------------------------------

void ViewerWidget::drawCurve(const BezierCurve& curve) {
	// << Beziérova krivka >>
	const QVector<QPoint>& curvePoints = curve.getPoints();
	if (curvePoints.size() < 2) {
		QMessageBox::warning(this, "Nedostatocny pocet bodov", "Nemozno nakreslit krivku s menej ako dvomi riadiacimi bodmi.", QMessageBox::Ok);
		return;
	}

	painter->setPen(QPen(borderColor));
	float deltaT = 0.01f;
	QPoint Q0 = curvePoints[0];

	for (float t = deltaT; t <= 1; t += deltaT) {
		QVector<QPoint> tempPoints = curvePoints;

		for (int i = 1; i < tempPoints.size(); i++) {
			for (int j = 0; j < tempPoints.size() - i; j++) {
				tempPoints[j] = tempPoints[j] * (1 - t) + tempPoints[j + 1] * t;
			}
		}

		drawLine(Line(Q0, tempPoints[0], curve.getZBufferPosition(), curve.getIsFilled()));
		Q0 = tempPoints[0];
	}
	if (deltaT * floor(1 / deltaT) < 1) {
		drawLine(Line(Q0, curvePoints.last(), curve.getZBufferPosition(), curve.getIsFilled()));
	}

	addToZBuffer(curve, curve.getZBufferPosition());
	redrawAllShapes();
}

void ViewerWidget::moveCurve(const QPoint& offset) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::BEZIER_CURVE) {
			BezierCurve curve = static_cast<BezierCurve&>(pair.first);
			const QVector<QPoint>& points = curve.getPoints();

			QVector<QPoint> movedPoints;
			for (const QPoint& point : points) {
				movedPoints.append(point + offset);
			}

			BezierCurve movedCurve(movedPoints, curve.getZBufferPosition(), curve.getIsFilled());
			pair.first = movedCurve;

			redrawAllShapes();
		}
	}
}

void ViewerWidget::scaleCurve(double scaleX, double scaleY) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::BEZIER_CURVE) {
			BezierCurve curve = static_cast<BezierCurve&>(pair.first);
			const QVector<QPoint>& points = curve.getPoints();
			QPoint center = calculateCurveCenter(curve);

			QVector<QPoint> scaledPoints;
			for (const QPoint& point : points) {
				double newX = center.x() + (point.x() - center.x()) * scaleX;
				double newY = center.y() + (point.y() - center.y()) * scaleY;

				scaledPoints.append(QPoint(static_cast<int>(std::round(newX)), static_cast<int>(std::round(newY))));
			}

			BezierCurve scaledCurve(scaledPoints, curve.getZBufferPosition(), curve.getIsFilled());
			pair.first = scaledCurve;

			redrawAllShapes();
		}
	}
}

void ViewerWidget::turnCurve(int angle) {
	if (currentLayer >= 0 && currentLayer < zBuffer.size()) {
		auto& pair = zBuffer[currentLayer];
		if (pair.first.getType() == Shape::BEZIER_CURVE) {
			BezierCurve curve = static_cast<BezierCurve&>(pair.first);
			const QVector<QPoint>& points = curve.getPoints();
			QPoint center = calculateCurveCenter(curve);

			double radians = qDegreesToRadians(static_cast<double>(angle));
			double cosAngle = std::cos(radians);
			double sinAngle = std::sin(radians);

			QVector<QPoint> rotatedPoints;

			for (const QPoint& point : points) {
				// Translate the point to the origin
				int translatedX = point.x() - center.x();
				int translatedY = point.y() - center.y();

				// Rotate the point
				int rotatedX = static_cast<int>(translatedX * cosAngle - translatedY * sinAngle);
				int rotatedY = static_cast<int>(translatedX * sinAngle + translatedY * cosAngle);

				// Translate the point back
				rotatedX += center.x();
				rotatedY += center.y();

				rotatedPoints.append(QPoint(rotatedX, rotatedY));
			}

			BezierCurve rotatedCurve(rotatedPoints, curve.getZBufferPosition(), curve.getIsFilled());
			pair.first = rotatedCurve;

			redrawAllShapes();
		}
	}
}

QPoint ViewerWidget::calculateCurveCenter(const BezierCurve& curve) const {
	const QVector<QPoint>& points = curve.getPoints();
	if (points.isEmpty()) {
		return QPoint();
	}

	double centroidX = 0;
	double centroidY = 0;
	for (const QPoint& point : points) {
		centroidX += point.x();
		centroidY += point.y();
	}

	centroidX /= points.size();
	centroidY /= points.size();

	return QPoint(static_cast<int>(centroidX), static_cast<int>(centroidY));
}
