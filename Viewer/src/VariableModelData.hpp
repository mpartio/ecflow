//============================================================================
// Copyright 2015 ECMWF.
// This software is licensed under the terms of the Apache Licence version 2.0
// which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
// In applying this licence, ECMWF does not waive the privileges and immunities
// granted to it by virtue of its status as an intergovernmental organisation
// nor does it submit to any jurisdiction.
//============================================================================

#ifndef VARIABLEMODELDATA_H
#define VARIABLEMODELDATA_H

#include <vector>

#include <QColor>
#include <QObject>

#include "NodeObserver.hpp"
#include "VInfo.hpp"

class Node;
class ServerHandler;


class VariableModelData
{
public:
	virtual ~VariableModelData() {};

	virtual const std::string& dataName()=0;
	virtual std::string type()=0;
	virtual QColor colour()=0;
	const std::string& name(int index) const;
	const std::string& value(int index) const;
	bool isGenVar(int index) const;
	int varNum() const;
	bool hasName(const std::string& n) const;

	void buildAlterCommand(std::vector<std::string>& cmd,
			           const std::string& action, const std::string& type,
			           const std::string& name,const std::string& value);

    void clear();
	virtual void reload()=0;
	virtual void setValue(int index,const std::string& val)=0;
	virtual void add(const std::string& name,const std::string& val)=0;
	virtual void remove(int index,const std::string& val)=0;
	virtual bool isNode(const Node*)=0;
	virtual bool isServer(ServerHandler*)=0;
	virtual bool sizeChanged()=0;

	std::vector<std::pair<std::string,std::string> > vars_;
	std::vector<std::pair<std::string,std::string> > genVars_;
};

class VariableServerData : public VariableModelData
{
public:
	VariableServerData(ServerHandler*);
	const std::string& dataName();
	std::string type();
	QColor colour();
	void setValue(int index,const std::string& val);
	void add(const std::string& name,const std::string& val);
	void remove(int index,const std::string& val){};

	bool isNode(const Node*) {return false;}
	bool isServer(ServerHandler* s) {return server_==s;}
	bool sizeChanged() {return true;}

protected:
	void reload();
	ServerHandler* server_;
};


class VariableNodeData : public VariableModelData
{
public:
	VariableNodeData(Node*);
	const std::string& dataName();
	std::string type();
	QColor colour();
	void setValue(int index,const std::string& val);
	void add(const std::string& name,const std::string& val);
	void remove(int index,const std::string& varName);

	bool isNode(const Node* n) {return node_==n;}
	bool isServer(ServerHandler* s) {return false;}
	bool sizeChanged();

protected:
	void reload();
	Node* node_;
};


class VariableModelDataHandler : public QObject
{
Q_OBJECT

public:
	VariableModelDataHandler();
	~VariableModelDataHandler();

	void reload(VInfo_ptr info);
	void clear();
	int count() const {return static_cast<int>(data_.size());}
	int varNum(int index) const;
	VariableModelData* data(int index) const;
	void nodeChanged(const Node* node, const std::vector<ecf::Aspect::Type>&);

Q_SIGNALS:
	void reloadBegin();
	void reloadEnd();
	void dataChanged(int);

protected:
	std::vector<VariableModelData*> data_;

	ServerHandler* server_;

};


#endif
