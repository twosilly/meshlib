/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005-2008                                           \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *   
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/


#ifndef EditPointFactoryPLUGIN_H
#define EditPointFactoryPLUGIN_H

#include <QObject>
#include <common/interfaces.h>

class LanguageFactory : public QObject, public MeshEditInterfaceFactory
{
	Q_OBJECT
	MESHLAB_PLUGIN_IID_EXPORTER(MESH_EDIT_INTERFACE_FACTORY_IID)
	Q_INTERFACES(MeshEditInterfaceFactory)

public:
        LanguageFactory();
        virtual ~LanguageFactory() { delete QAction_Language; }

	//gets a list of actions available from this plugin
    //从该插件获取可用操作的列表
	virtual QList<QAction *> actions() const;
	
	//get the edit tool for the given action
    //获取给定操作的编辑工具
	virtual MeshEditInterface* getMeshEditInterface(QAction *);
    
	//get the description for the given action
    //获取给定操作的描述
    virtual QString getEditToolDescription(QAction *);
	
private:
	QList <QAction *> actionList;
	
        QAction *QAction_Language;
        //QAction *editPointFittingPlane;
};

#endif
