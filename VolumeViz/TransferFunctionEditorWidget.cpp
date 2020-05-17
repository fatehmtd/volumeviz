#include "TransferFunctionEditorWidget.h"

TransferFunctionEditorWidget::TransferFunctionEditorWidget(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	connect(ui._curveEditorWidget, &CurveEditorWidget::colorsUpdated, ui._colorWidget, &ColorWidget::setColors);
	ui._curveEditorWidget->updateGraphs();
}

TransferFunctionEditorWidget::~TransferFunctionEditorWidget()
{
}

CurveEditorWidget* TransferFunctionEditorWidget::getCurveEditorWidget() const
{
	return ui._curveEditorWidget;
}
