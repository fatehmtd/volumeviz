#pragma once

#include <QWidget>
#include "ui_TransferFunctionEditorWidget.h"

#include "CurveEditorWidget.h"
#include "ColorWidget.h"
#include "TransparencyWidget.h"

class TransferFunctionEditorWidget : public QWidget
{
	Q_OBJECT

public:
	TransferFunctionEditorWidget(QWidget *parent = Q_NULLPTR);
	~TransferFunctionEditorWidget();

	CurveEditorWidget* getCurveEditorWidget() const;
private:
	Ui::TransferFunctionEditorWidget ui;
};
