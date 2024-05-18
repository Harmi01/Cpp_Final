#include "ImageViewer.h"

ImageViewer::ImageViewer(QWidget* parent)
	: QMainWindow(parent), ui(new Ui::ImageViewerClass), currentLayer(-1)
{
	ui->setupUi(this);
	vW = new ViewerWidget(QSize(500, 500));
	ui->scrollArea->setWidget(vW);

	ui->scrollArea->setBackgroundRole(QPalette::Dark);
	ui->scrollArea->setWidgetResizable(true);
	ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	vW->setObjectName("ViewerWidget");
	vW->installEventFilter(this);

	fillingColor = Qt::blue;
	borderColor = Qt::blue;
	QString style_sheet = QString("background-color: #%1;").arg(fillingColor.rgba(), 0, 16);
	ui->pushButtonSetColor->setStyleSheet(style_sheet);
	ui->pushButtonSetBorderColor->setStyleSheet(style_sheet);

	vW->setBorderColor(borderColor);
	vW->setFillingColor(fillingColor);

	connect(ui->listWidget, &QListWidget::currentRowChanged, this, &ImageViewer::layerSelectionChanged);
}

// Event filters
bool ImageViewer::eventFilter(QObject* obj, QEvent* event)
{
	if (obj->objectName() == "ViewerWidget") {
		return ViewerWidgetEventFilter(obj, event);
	}
	return false;
}

//ViewerWidget Events
bool ImageViewer::ViewerWidgetEventFilter(QObject* obj, QEvent* event)
{
	ViewerWidget* w = static_cast<ViewerWidget*>(obj);

	if (!w) {
		return false;
	}

	if (event->type() == QEvent::MouseButtonPress) {
		ViewerWidgetMouseButtonPress(w, event);
	}
	else if (event->type() == QEvent::MouseButtonRelease) {
		ViewerWidgetMouseButtonRelease(w, event);
	}
	else if (event->type() == QEvent::MouseMove) {
		ViewerWidgetMouseMove(w, event);
	}
	else if (event->type() == QEvent::Leave) {
		ViewerWidgetLeave(w, event);
	}
	else if (event->type() == QEvent::Enter) {
		ViewerWidgetEnter(w, event);
	}
	else if (event->type() == QEvent::Wheel) {
		ViewerWidgetWheel(w, event);
	}

	return QObject::eventFilter(obj, event);
}
void ImageViewer::ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
	static MyPolygon* polygon = nullptr;
	static BezierCurve* curve = nullptr;

	//	>> Line Drawing
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawLine->isChecked() && !lineDone)
	{
		if (w->getDrawLineActivated()) {
			int layerIndex = ui->listWidget->count() + 1;
			ui->listWidget->addItem(QString("Line %1").arg(layerIndex));
			int newRowIndex = ui->listWidget->count() - 1;
			ui->listWidget->setCurrentRow(newRowIndex);

			Line* line = new Line(w->getDrawLineBegin(), e->pos(), layerIndex, ui->checkBoxFilling->isChecked());
			w->drawShape(*line);

			w->setDrawLineActivated(false);
			lineDone = true;
		}
		else {
			w->setDrawLineBegin(e->pos());
			w->setDrawLineActivated(true);
			w->setPixel(e->pos().x(), e->pos().y(), borderColor);
			w->update();
		}
	}

	//	>> Circle Drawing
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawCircle->isChecked() && !circleDone)
	{
		if (w->getDrawCircleActivated()) {
			int layerIndex = ui->listWidget->count() + 1;
			ui->listWidget->addItem(QString("Circle %1").arg(layerIndex));
			int newRowIndex = ui->listWidget->count() - 1;
			ui->listWidget->setCurrentRow(newRowIndex);

			Circle* circle = new Circle(w->getDrawCircleCenter(), e->pos(), layerIndex, ui->checkBoxFilling->isChecked());
			w->drawShape(*circle);
			w->setDrawCircleActivated(false);
			circleDone = true;
		}
		else {
			w->setDrawCircleCenter(e->pos());
			w->setDrawCircleActivated(true);
			w->setPixel(e->pos().x(), e->pos().y(), borderColor);
			w->update();
		}
	}

	//	>> Polygon Drawing
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawPolygon->isChecked()) {
		if (!polygon) {
			polygon = new MyPolygon(QVector<QPoint>(), ui->listWidget->count() + 1, ui->checkBoxFilling->isChecked());
			ui->listWidget->addItem(QString("Polygon %1").arg(ui->listWidget->count() + 1));
			int newRowIndex = ui->listWidget->count() - 1;
			ui->listWidget->setCurrentRow(newRowIndex);
		}

		w->setPixel(e->pos().x(), e->pos().y(), borderColor);
		polygon->addPoint(e->pos());
		w->update();
	}
	if (e->button() == Qt::RightButton && ui->toolButtonDrawPolygon->isChecked()) {
		if (polygon) {
			w->drawShape(*polygon);
			delete polygon;
			polygon = nullptr;
		}
		ui->toolButtonDrawPolygon->setDisabled(true);
	}
	//	>> Curve Drawing
	if (e->button() == Qt::LeftButton && ui->toolButtonDrawCurve->isChecked() && !curveDone) {
		if (!curve) {
			curve = new BezierCurve(QVector<QPoint>(), ui->listWidget->count() + 1, ui->checkBoxFilling->isChecked());
			ui->listWidget->addItem(QString("Bezier Curve %1").arg(ui->listWidget->count() + 1));
			int newRowIndex = ui->listWidget->count() - 1;
			ui->listWidget->setCurrentRow(newRowIndex);
		}

		curve->addPoint(e->pos());
		w->setPixel(e->pos().x(), e->pos().y(), borderColor);
		w->update();
	}
	if (e->button() == Qt::RightButton && ui->toolButtonDrawCurve->isChecked()) {
		if (curve) {
			w->drawShape(*curve);
			delete curve;
			curve = nullptr;
		}
		ui->toolButtonDrawCurve->setDisabled(true);
		curveDone = true;
	}
}
void ImageViewer::ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);
}
void ImageViewer::ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event)
{
	QMouseEvent* e = static_cast<QMouseEvent*>(event);

	//	>> Polygon Movement
	if (ui->toolButtonDrawPolygon->isChecked()) {
		if (e->buttons() & Qt::LeftButton && ui->pushButtonMove->isChecked()) {
			QPoint offset = e->pos() -  w->getMoveStart();
			if (!w->getMoveStart().isNull()) {
				w->movePolygon(offset);
			}
			w->setMoveStart(e->pos());
		}
		else if (ui->pushButtonMove->isChecked()) {
			w->setMoveStart(QPoint());
		}
	}
	
	//	>> Line Movement
	if (ui->toolButtonDrawLine->isChecked()) {
		if (e->buttons() & Qt::LeftButton && ui->pushButtonMove->isChecked()) {
			QPoint offset = e->pos() - w->getMoveStart();
			if (!w->getMoveStart().isNull()) {
				w->moveLine(offset);
			}
			w->setMoveStart(e->pos());
		}
		else if (ui->pushButtonMove->isChecked()) {
			w->setMoveStart(QPoint());
		}
	}
	
	//	>> Curve Movement
	if (ui->toolButtonDrawCurve->isChecked()) {
		if (e->buttons() & Qt::LeftButton && ui->pushButtonMove->isChecked()) {
			QPoint offset = e->pos() - w->getMoveStart();
			if (!w->getMoveStart().isNull()) {
				w->moveCurve(offset);
			}
			w->setMoveStart(e->pos());
		}
		else if (ui->pushButtonMove->isChecked()) {
			w->setMoveStart(QPoint());
		}
	}

	//	>> Circle Movement
	if (ui->toolButtonDrawCircle->isChecked()) {
		if (e->buttons() & Qt::LeftButton && ui->pushButtonMove->isChecked()) {
			QPoint offset = e->pos() - w->getMoveStart();
			if (!w->getMoveStart().isNull()) {
				w->moveCircle(offset);
			}
			w->setMoveStart(e->pos());
		}
		else if (ui->pushButtonMove->isChecked()) {
			w->setMoveStart(QPoint());
		}
	}
}
void ImageViewer::ViewerWidgetLeave(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetEnter(ViewerWidget* w, QEvent* event)
{
}
void ImageViewer::ViewerWidgetWheel(ViewerWidget* w, QEvent* event)
{
	QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);

	if (ui->checkBoxScale->isChecked()) {
		int deltaY = wheelEvent->angleDelta().y();
		double scale = 1.0;
		if (deltaY < 0) {
			scale = 0.75;
		}
		else if (deltaY > 0) {
			scale = 1.25;
		}
		if (ui->toolButtonDrawPolygon->isChecked()) {
			w->scalePolygon(scale, scale);
		}
		if (ui->toolButtonDrawLine->isChecked()) {
			w->scaleLine(scale, scale);
		}
		if (ui->toolButtonDrawCurve->isChecked()) {
			w->scaleCurve(scale, scale);
		}
		if (ui->toolButtonDrawCircle->isChecked()) {
			w->scaleCircle(scale, scale);
		}
	}
}

//ImageViewer Events
void ImageViewer::closeEvent(QCloseEvent* event)
{
	if (QMessageBox::Yes == QMessageBox::question(this, "Close Confirmation", "Are you sure you want to exit?", QMessageBox::Yes | QMessageBox::No))
	{
		event->accept();
	}
	else {
		event->ignore();
	}
}

//Image functions
bool ImageViewer::openImage(QString filename)
{
	QImage loadedImg(filename);
	if (!loadedImg.isNull()) {
		return vW->setImage(loadedImg);
	}
	return false;
}
bool ImageViewer::saveImage(QString filename)
{
	QFileInfo fi(filename);
	QString extension = fi.completeSuffix();

	QImage* img = vW->getImage();
	return img->save(filename, extension.toStdString().c_str());
}

//-----------------------------------------
//		*** Tools 2D slots ***
//-----------------------------------------

void ImageViewer::layerSelectionChanged(int currentRow) {
	currentLayer = currentRow;
	vW->setLayer(currentLayer);
}

void ImageViewer::on_actionSave_as_triggered()
{
	QString folder = settings.value("folder_img_save_path", "").toString();

	QString fileFilter = "Image data (*.bmp *.gif *.jpg *.jpeg *.png *.pbm *.pgm *.ppm .*xbm .* xpm);;All files (*)";
	QString fileName = QFileDialog::getSaveFileName(this, "Save image", folder, fileFilter);
	if (!fileName.isEmpty()) {
		QFileInfo fi(fileName);
		settings.setValue("folder_img_save_path", fi.absoluteDir().absolutePath());

		if (!saveImage(fileName)) {
			msgBox.setText("Unable to save image.");
			msgBox.setIcon(QMessageBox::Warning);
		}
		else {
			msgBox.setText(QString("File %1 saved.").arg(fileName));
			msgBox.setIcon(QMessageBox::Information);
		}
		msgBox.exec();
	}
}

void ImageViewer::on_actionClear_triggered()
{
	polygonDone = false;
	lineDone = false;
	curveDone = false;
	circleDone = false;
	rectangleDone = false;
	objectLoaded = false;

	ui->toolButtonDrawPolygon->setDisabled(false);
	ui->toolButtonDrawCurve->setDisabled(false);
	ui->toolButtonDrawCurve->setChecked(false);
	ui->toolButtonDrawPolygon->setChecked(false);
	ui->toolButtonDrawRectangle->setChecked(false);
	ui->toolButtonDrawCircle->setChecked(false);
	ui->toolButtonDrawLine->setChecked(false);
	ui->pushButtonMove->setChecked(false);
	ui->checkBoxScale->setChecked(false);
	ui->checkBoxFilling->setChecked(false);
	vW->clear();
}

void ImageViewer::on_actionExit_triggered()
{
	this->close();
}

void ImageViewer::on_pushButtonSetColor_clicked()
{
	QColor newColor = QColorDialog::getColor(fillingColor, this);
	if (newColor.isValid()) {
		QString style_sheet = QString("background-color: #%1;").arg(newColor.rgba(), 0, 16);
		ui->pushButtonSetColor->setStyleSheet(style_sheet);
		fillingColor = newColor;
	}
	vW->setFillingColor(fillingColor);
}

void ImageViewer::on_pushButtonSetBorderColor_clicked()
{
	QColor newColor = QColorDialog::getColor(borderColor, this);
	if (newColor.isValid()) {
		QString style_sheet = QString("background-color: #%1;").arg(newColor.rgba(), 0, 16);
		ui->pushButtonSetBorderColor->setStyleSheet(style_sheet);
		borderColor = newColor;
	}
	vW->setBorderColor(borderColor);
}

void ImageViewer::on_pushButtonTurn_clicked() {
	if (ui->toolButtonDrawPolygon->isChecked()) {
		vW->turnPolygon(ui->spinBoxTurn->value());
	}
	if (ui->toolButtonDrawLine->isChecked()) {
		vW->turnLine(ui->spinBoxTurn->value());
	}
	if (ui->toolButtonDrawCurve->isChecked()) {
		vW->turnCurve(ui->spinBoxTurn->value());
	}
}

void ImageViewer::on_pushButtonScale_clicked() {
	if (ui->toolButtonDrawPolygon->isChecked()) {
		vW->scalePolygon(ui->doubleSpinBoxScaleX->value(), ui->doubleSpinBoxScaleY->value());
	}
	if (ui->toolButtonDrawLine->isChecked()) {
		vW->scaleLine(ui->doubleSpinBoxScaleX->value(), ui->doubleSpinBoxScaleY->value());
	}
	if (ui->toolButtonDrawCurve->isChecked()) {
		vW->scaleCurve(ui->doubleSpinBoxScaleX->value(), ui->doubleSpinBoxScaleY->value());
	}
	if (ui->toolButtonDrawCircle->isChecked()) {
		vW->scaleCircle(ui->doubleSpinBoxScaleX->value(), ui->doubleSpinBoxScaleY->value());
	}
}