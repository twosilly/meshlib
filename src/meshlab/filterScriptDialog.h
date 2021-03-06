/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005                                                \/)\/    *
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
#ifndef FILTER_SCRIPT_DIALOG_H
#define FILTER_SCRIPT_DIALOG_H

#include <QDialog>
#include "../common/filterscript.h"

namespace Ui 
{
	class scriptDialog;
	class scriptEditor;
} 

class FilterScriptDialog : public QDialog
{
Q_OBJECT
public:
	FilterScriptDialog(QWidget *parent = 0);
	~FilterScriptDialog();
  void setScript(FilterScript *scr);

private slots:
	//will do all things that need to be done before the script runs then trigger the running of the script
	//specifically with the PARMESH type we need to to set the mesh pointer based on the int given
	void applyScript();

	void clearScript();	
	void saveScript();	
	void openScript();	
	
	//moves the filter selected in scriptListWidget up in the script
	void moveSelectedFilterUp();
	
	//moves the filter selected in scriptListWidget down in the script
	void moveSelectedFilterDown();
	
	//removes the selected filter from the script
	void removeSelectedFilter();
	
	//edit the parameters of the selected filter
	void editSelectedFilterParameters();
	
private:
	Ui::scriptDialog* ui;
  FilterScript *scriptPtr;

  void editOldParameters(const int row);
  void editXMLParameters(const int row);

};

#endif