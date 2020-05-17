#include "TransparencyWidget.h"
#include <QLinearGradient>
#include <QPainter>

TransparencyWidget::TransparencyWidget(QWidget* parent) : QWidget(parent)
{
	_linearGradient.setColorAt(0, qRgb(255, 255, 255));
	_linearGradient.setColorAt(1, qRgb(0, 0, 0));
}

void TransparencyWidget::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);

	QBrush brush(_linearGradient);

	QPainter painter;
	painter.begin(this);
	painter.fillRect(0, 0, width(), height(), brush);
	painter.end();
}

void TransparencyWidget::resizeEvent(QResizeEvent* event)
{
	_linearGradient.setStart(0, 0);
	_linearGradient.setFinalStop(0, height());
}
