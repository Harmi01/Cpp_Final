#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets>
#include "ui_ImageViewer.h"
#include "ViewerWidget.h"
#include "lighting.h"
#include "representation.h"

class ImageViewer : public QMainWindow
{
	Q_OBJECT

public:
	ImageViewer(QWidget* parent = Q_NULLPTR);

private:
	Ui::ImageViewerClass* ui;
	ViewerWidget* vW;

	QColor fillingColor;
	QColor borderColor;
	QSettings settings;
	QMessageBox msgBox;

	bool polygonDone = false;
	bool lineDone = false;
	bool circleDone = false;
	bool curveDone = false;
	bool rectangleDone = false;
	bool objectLoaded = false;
	int currentLayer;

	//Event filters
	bool eventFilter(QObject* obj, QEvent* event);

	//ViewerWidget Events
	bool ViewerWidgetEventFilter(QObject* obj, QEvent* event);
	void ViewerWidgetMouseButtonPress(ViewerWidget* w, QEvent* event);
	void ViewerWidgetMouseButtonRelease(ViewerWidget* w, QEvent* event);
	void ViewerWidgetMouseMove(ViewerWidget* w, QEvent* event);
	void ViewerWidgetLeave(ViewerWidget* w, QEvent* event);
	void ViewerWidgetEnter(ViewerWidget* w, QEvent* event);
	void ViewerWidgetWheel(ViewerWidget* w, QEvent* event);

	//ImageViewer Events
	void closeEvent(QCloseEvent* event);

	//Image functions
	bool openImage(QString filename);
	bool saveImage(QString filename);

private slots:
	void on_actionSave_as_triggered();
	void on_actionClear_triggered();
	void on_actionExit_triggered();
	void layerSelectionChanged(int currentRow);

//-----------------------------------------
//		*** Tools 2D slots ***
//-----------------------------------------
	void on_pushButtonSetColor_clicked();
	void on_pushButtonSetBorderColor_clicked();
	void on_pushButtonTurn_clicked();
	void on_pushButtonScale_clicked();
};
