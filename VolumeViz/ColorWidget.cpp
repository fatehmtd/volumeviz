#include "ColorWidget.h"

#include <QPainter>

ColorWidget::ColorWidget(QWidget* parent) : QWidget(parent)
{

}

ColorWidget::~ColorWidget()
{

}

void ColorWidget::setColors(const QVector<QPair<QPointF, QColor>>& colors)
{
	_linearGradient = QLinearGradient();
	_colors = colors;	
	for (auto& color : _colors)
	{
		_linearGradient.setColorAt(color.first.x(), color.second);
	}

	update();
}


void ColorWidget::paintEvent(QPaintEvent* event)
{
	QWidget::paintEvent(event);

	_linearGradient.setStart(0, 0);
	_linearGradient.setFinalStop(width(), 0);

	QBrush brush(_linearGradient);

	QPainter painter;
	painter.begin(this);
	painter.fillRect(0, 0, width(), height(), brush);
	painter.end();
}