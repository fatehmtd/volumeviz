#pragma once
#include <QWidget>
#include <QLinearGradient>

class TransparencyWidget : public QWidget
{
	Q_OBJECT
public:
	TransparencyWidget(QWidget* parent = nullptr);
protected:
	void paintEvent(QPaintEvent* event) override;
	void resizeEvent(QResizeEvent* event) override;
protected:
	QLinearGradient _linearGradient;
};

