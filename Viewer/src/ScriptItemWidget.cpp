//============================================================================
// Copyright 2014 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#include "ScriptItemWidget.hpp"

#include "Defs.hpp"
#include "DState.hpp"
#include "Node.hpp"
#include "ServerHandler.hpp"
#include "Suite.hpp"
#include "Variable.hpp"

#include "boost/date_time/posix_time/posix_time.hpp"

//========================================================
//
// ScriptItemWidget
//
//========================================================

ScriptItemWidget::ScriptItemWidget(QWidget *parent) : QPlainTextEdit(parent)
{
}

QWidget* ScriptItemWidget::realWidget()
{
	return this;
}

void ScriptItemWidget::reload(ViewNodeInfo_ptr nodeInfo)
{
	loaded_=true;
	
	if(nodeInfo->isNode())
	{
		Node* n=nodeInfo->node();

		QString txt;
		std::string fName,msg,err;
		if(ServerHandler* s=nodeInfo->server())
		{
			if(s->readFile(n,"ECF_SCRIPT",fName,msg,err))
				setPlainText(QString::fromStdString(msg));
			else
				setPlainText(QString::fromStdString(err));	
		}		
	}
	else
	{
		clear();
	}
}

void ScriptItemWidget::clearContents()
{
	loaded_=false;
	clear();
}



static InfoPanelItemMaker<ScriptItemWidget> maker1("script");