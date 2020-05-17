#pragma once

#include <QWidget>
#include <QPaintEvent>
#include <QVector>
#include <QPair>
#include <QLinearGradient>

class ColorWidget : public QWidget
{
	Q_OBJECT
public:
	ColorWidget(QWidget* parent = nullptr);
	~ColorWidget();
public slots:
	void setColors(const QVector<QPair<QPointF, QColor>>& colors);
protected:
	virtual void paintEvent(QPaintEvent* event);
	QVector<QPair<QPointF, QColor>> _colors;
	QLinearGradient _linearGradient;
};

