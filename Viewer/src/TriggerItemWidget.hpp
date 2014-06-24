//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//
//============================================================================

#ifndef TRIGGERITEMWIDGET_HPP_
#define TRIGGERITEMWIDGET_HPP_

#include <QWidget>

#include "InfoPanelItem.hpp"
#include "ViewNodeInfo.hpp"

class TriggerItemWidget : public QWidget, public InfoPanelItem
{
public:
	TriggerItemWidget(QWidget *parent=0);

	void reload(ViewNodeInfo_ptr);
	QWidget* realWidget();
	void clearContents();

};

#endif

