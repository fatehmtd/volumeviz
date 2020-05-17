#include "CurveEditorWidget.h"
#include <qdebug>
#include <QColorDialog>

CurveEditorWidget::CurveEditorWidget(QWidget* parent) : QCustomPlot(parent)
{
	////////////////////////////////////////////////////////////////////////////////////////////
	connect(this, &QCustomPlot::mouseDoubleClick, this, &CurveEditorWidget::onMouseDoubleClick);
	connect(this, &QCustomPlot::mousePress, this, &CurveEditorWidget::onMousePress);
	connect(this, &QCustomPlot::mouseMove, this, &CurveEditorWidget::onMouseMove);
	connect(this, &QCustomPlot::mouseRelease, this, &CurveEditorWidget::onMouseRelease);
	////////////////////////////////////////////////////////////////////////////////////////////
	xAxis->setTickLength(1);
	yAxis->setTickLength(1);
	xAxis->setSubTickLength(1);
	yAxis->setSubTickLength(1);

	_histogramPlot = new QCPBars(xAxis, yAxis);
	_histogramPlot->setAntialiased(false);
	_histogramPlot->setPen(QPen(QBrush(Qt::green), 2));

	float gap = 0.01f;
	xAxis->setRange(0.0f - gap, 1.0f + gap);
	yAxis->setRange(0.0f - gap, 1.0f + gap);
	axisRect()->setAutoMargins(QCP::MarginSide::msNone);
	axisRect()->setMargins(QMargins(0, 0, 0, 0));
	////////////////////////////////////////////////////
	_curveGraph = addGraph();
	_curveGraph->setLineStyle(QCPGraph::lsLine);
	_curveGraph->setPen(QPen(QBrush(Qt::blue), 2));
	////////////////////////////////////////////////////
	_pointsGraph = addGraph();
	_pointsGraph->setLineStyle(QCPGraph::lsNone);
	_pointsGraph->setScatterStyle(QCPScatterStyle::ssDisc);
	_pointsGraph->setPen(QPen(QBrush(Qt::red), 10));

	setEpsilon(0.01f); // separation threshold
	setThreshold(0.0535f); // mouse selection threshold

	// default curve
	addPoint(0.0f, 0.0f, qRgb(255, 0, 0));
	addPoint(0.37f, 0.0f, qRgb(255, 255, 255));
	addPoint(0.72f, 0.5f, qRgb(170, 0, 0));
	addPoint(1.0f, 1.0f, qRgb(0, 0, 255));

	updateGraphs();
}

CurveEditorWidget::~CurveEditorWidget()
{
}

void CurveEditorWidget::addPoint(float value, float alpha, QRgb color)
{
	int index = 0;

	if (_points.isEmpty() || value < _points.first().key)
	{
		_points.prepend(QCPGraphData(value, alpha));
		_colors.prepend(color);
	}
	else if (value > _points.last().key)
	{
		_points.append(QCPGraphData(value, alpha));
		_colors.append(color);
	}
	else
	{
		for (int i = 1; i < _points.length(); i++)
		{
			const auto& previous = _points[i - 1];
			const auto& next = _points[i];

			if (value >= previous.key && value <= next.key)
			{
				_points.insert(i, QCPGraphData(value, alpha));
				_colors.insert(i, color);
				break;
			}
		}
	}
}

void CurveEditorWidget::removePoint(int index)
{
	if (index < 1 || index >= _points.size() - 1) return;
	_points.remove(index);
	_colors.remove(index);

	updateGraphs();
}

void CurveEditorWidget::onMouseDoubleClick(QMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::LeftButton:
	{
		const int index = getSelectedPoint(e->pos().x(), e->pos().y());

		if (index != -1) // double clicked on a control point
		{
			auto& color = _colors[index];
			auto tempColor = QColorDialog::getColor(color, this);
			if (tempColor.isValid())
				color = tempColor;
		}
		else // double clicked on an empty space
		{
			auto p = e->pos();
			float w = width(), h = height();
			addPoint((float)p.x() / w, 1.0f - (float)p.y() / h, qRgb(rand() % 256, rand() % 256, rand() % 256));
			//addPoint((float)p.x() / w, 1.0f - (float)p.y() / h, qRgb(255, 255, 255));
		}
		updateGraphs();
	}
	break;
	case Qt::RightButton:
	{
	}
	break;
	}
}

void CurveEditorWidget::onMousePress(QMouseEvent* e)
{
	_selectedPointIndex = getSelectedPoint(e->pos().x(), e->pos().y());
	switch (e->button())
	{
	case Qt::LeftButton:
	{
	}
	break;
	case Qt::RightButton:
	{
		if (_selectedPointIndex > -1)
		{
			removePoint(_selectedPointIndex);
		}
	}
	break;
	}
}

float clamp(float v, float mn, float mx)
{
	if (v < mn) return mn;
	if (v > mx) return mx;
	return v;
}

void CurveEditorWidget::onMouseMove(QMouseEvent* e)
{
	const float w = width(), h = height();
	const float x = (float)e->pos().x() / w, y = 1.0f - (float)e->pos().y() / h;

	auto buttons = e->buttons();
	bool leftButton = buttons & Qt::LeftButton;
	bool rightButton = buttons & Qt::RightButton;
	bool middleButton = buttons & Qt::MiddleButton;

	if (leftButton)
	{
		if (_selectedPointIndex == -1) return;

		auto& point = _points[_selectedPointIndex];

		point.value = clamp(y, 0.0f, 1.0f);

		if (_selectedPointIndex > 0 && _selectedPointIndex < _points.size() - 1)
		{
			const auto prevPoint = _points[_selectedPointIndex - 1];
			const auto nextPoint = _points[_selectedPointIndex + 1];

			point.key = clamp(x, prevPoint.key + _epsilon, nextPoint.key - _epsilon);
		}

		updateGraphs();
	}

	if (rightButton)
	{
	}

	if (middleButton)
	{
		if (_selectedPointIndex == -1) return;

		auto& point = _points[_selectedPointIndex];

		if (y > point.value)
		{
			point.value = 1.0f;
		}
		else if (y < point.value)
		{
			point.value = 0.0f;
		}

		updateGraphs();
		_selectedPointIndex = -1;
	}
}

void CurveEditorWidget::onMouseRelease(QMouseEvent* e)
{
	_selectedPointIndex = -1;
	//setCursor(QCursor(Qt::CursorShape::ArrowCursor));
}

void CurveEditorWidget::updateGraphs()
{
	_pointsGraph->data()->set(_points);
	_curveGraph->data()->set(_points);
	replot();
	emit colorsUpdated(getTransferFunction());
}

void CurveEditorWidget::setEpsilon(float eps)
{
	_epsilon = eps;
}

void CurveEditorWidget::setThreshold(float t)
{
	_thresh = t;
}

void CurveEditorWidget::setHistogram(unsigned int numBins, unsigned int* bins)
{
	QVector<double> values(numBins), keys(numBins);
	double norm = 0.0f;
	float maxbin = bins[0];
	for (int i = 0; i < numBins; i++)
	{
		keys[i] = (float)i / (float)numBins;
		if (maxbin < bins[i])
			maxbin = bins[i];
		norm += powf(bins[i], 2.0f);
	}
	norm = 1.0f/sqrtf(norm);
	for (int i = 0; i < numBins; i++)
	{
		values[i] = (float)bins[i] * norm;
	}

	_histogramPlot->setWidth(1.0f / (float)numBins);
	_histogramPlot->setData(keys, values);
	replot();
}

#include <QDebug>

QVector<QPair<QPointF, QColor>> CurveEditorWidget::getTransferFunction() const
{
	QVector<QPair<QPointF, QColor>> points(_points.size());
	for (int i = 0; i < _points.size(); i++)
	{
		points[i] = qMakePair(QPointF(_points[i].key, _points[i].value), _colors[i]);
	}
	return std::move(points);
}

int CurveEditorWidget::getSelectedPoint(int x, int y)
{
	const float w = width();
	const float h = height();
	const float xx = x / w;
	const float yy = 1.0f - y / h;

	float distance = 999999999999.0f;
	int index = -1;

	for (int i = 0; i < _points.size(); i++)
	{
		const auto& p = _points[i];
		const float dx = xx - p.key;
		const float dy = yy - p.value;

		const float d = sqrtf(dx * dx + dy * dy);

		if (distance > d)
		{
			distance = d;
			index = i;
		}
	}
	return distance <= _thresh ? index : -1;
}