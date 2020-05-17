#pragma once

#include <qcustomplot/qcustomplot.h>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QPointF>

class CurveEditorWidget : public QCustomPlot
{
	Q_OBJECT
protected:
	QCPGraph* _curveGraph = nullptr, * _pointsGraph = nullptr;
	QCPBars* _histogramPlot = nullptr;
	QVector<QCPGraphData> _points;
	QVector<QColor> _colors;
	int _selectedPointIndex = -1;
	float _epsilon = 0.01f;
	float _thresh = 0.01f;
public:
	CurveEditorWidget(QWidget* parent);
	~CurveEditorWidget();

	void addPoint(float value, float alpha, QRgb color);
	void removePoint(int index);
	void updateGraphs();
	void setEpsilon(float eps);
	void setThreshold(float t);

	void setHistogram(unsigned int numBins, unsigned int* bins);

	QVector<QPair<QPointF, QColor>> getTransferFunction() const;
protected slots:
	void onMouseDoubleClick(QMouseEvent* e);
	void onMousePress(QMouseEvent* e);
	void onMouseMove(QMouseEvent* e);
	void onMouseRelease(QMouseEvent* e);
signals:
	void colorsUpdated(const QVector<QPair<QPointF, QColor>>&);
protected:
	int getSelectedPoint(int x, int y);
};
