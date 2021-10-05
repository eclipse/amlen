/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

define(['dojo/_base/declare',
        'dojo/_base/lang',
        'dojo/_base/sniff',
		'dojo/aspect',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/query',
		'dijit/form/Button',
		'dijit/form/DropDownButton',
		'dijit/Menu',
		'dijit/MenuItem',
		'dijit/CheckedMenuItem',
		'dijit/PopupMenuItem',
		'dijit/ToolbarSeparator',
		'dijit/MenuSeparator',
		'gridx/Grid',
		'gridx/core/model/cache/Sync',
		'gridx/modules/select/Row',		
		'gridx/modules/IndirectSelect',
		'gridx/modules/RowHeader',
		'gridx/modules/extendedSelect/Row',		
		'gridx/modules/CellWidget',
		'gridx/modules/Focus',
		'gridx/modules/Pagination',
		'ism/widgets/ColumnResizer',
		'gridx/modules/ToolBar',
		'gridx/modules/Filter',
		'gridx/modules/SummaryBar',
		'gridx/modules/move/Row',
		'gridx/modules/GroupHeader',
		'gridx/modules/HiddenColumns',
		'gridx/modules/VScroller',
		'idx/gridx/modules/pagination/PaginationBar',
		'idx/gridx/modules/Sort',
		'ism/widgets/QuickFilter',
		'idx/gridx/modules/pagination/PaginationBar',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, sniff, aspect, domClass, domConstruct, query, Button, DropDownButton, Menu, MenuItem, 
				CheckedMenuItem, PopupMenuItem, ToolbarSeparator, MenuSeparator, Grid, Sync, SelectRow, IndirectSelect, 
				RowHeader, ExtendedSelectRow, CellWidget, Focus, Pagination, ColumnResizer, ToolBar, Filter, SummaryBar, 
				MoveRow, GroupHeader, HiddenColumns, VScroller, PaginationBar, Sort, QuickFilter, OneUIPaginationBar, nls) {

	return {
		// summary:
		// 		Utilities for creating Grids

		ADD_BUTTON: "add",
		DELETE_BUTTON: "delete",
		EDIT_BUTTON: "edit",
		UP_BUTTON: "up",
		DOWN_BUTTON: "down",
		
		constructor: function(args) {
			// summary:
			// 		Constructor.
			
			dojo.safeMixin(this, args);
		},

		/**
		 * Create and instantiate a grid
		 * 
		 * @param gridId id to use for the grid
		 * @param parentId id for the node to place the grid inside
		 * @param store the data store for the grid
		 * @param structure column structure for the grid
		 * @param options map of options to override defaults:
		 *           filter: true to add quick filter bar
		 *           paging: true to add pagination bar
		 *           heightTrigger: number of rows for initial size
		 *           suppressSelection: true to suppress selection
		 *           suppressSort: true to suppress sort
		 *           basic: override other options to only include CellWidget, ToolBar
		 *           moveRow: true includes move row module
		 *           headerGroups: headerGroups to use, if any,
		 *           actionsMenuTitle: menu title for additional actions, if not 'Other Actions',
		 *           extendedSelection: true to use IndirectSelection with extended selection (trigger on cell),
		 *           					suppressSelection overrides it,
		 *           multipleSelect: true to allow multiple rows to be selected
		 *           vscroller: true to add a vscroller (ignored if paging is true)
		 *           height: Optional height to set when autoHeight is off or using a vscroller.
		 * @returns {___module_gridx_Grid}
		 */
		createGrid: function(gridId, parentId, store, structure, options) {
			console.debug("GridFactory.createGrid "+gridId);
			
			var mods = [CellWidget, Focus, ColumnResizer];
						
			if (!options.suppressSort) {
				mods.push(Sort);
			}
			
			var multiple = options.multiple ? true : false;
			if (!options.suppressSelection) {
				mods.push(ToolBar);
				mods.push({ moduleClass: SelectRow, triggerOnCell: true, multiple: multiple });
				if (options.extendedSelection) {
					mods.push(RowHeader);
					mods.push(IndirectSelect);
					mods.push(ExtendedSelectRow);
				}
			}
			
			if (options.filter) {
				mods.push(Filter, { moduleClass: QuickFilter, containerId: gridId+"_FilterInputContainer" });
			}
			
			var vscroller = options.vscroller;
			
			if (options.paging) {
				mods.push({ moduleClass: Pagination, initialPageSize: 10 });
				if (options.shortPaging) {
					mods.push({ moduleClass: OneUIPaginationBar, sizes: [10, 25, 50] });
				} else {
					mods.push({ moduleClass: OneUIPaginationBar, sizes: [10, 25, 50, 100] });
				}
				vscroller = false;
			} else {
				mods.push(SummaryBar);
			}
			
			if (vscroller) {
				mods.push(VScroller);
			}

			if (options.basic) {
				mods = [CellWidget, ToolBar];
			}
			
			if (options.moveRow) {
				mods.push(MoveRow);
			}
						
			if (options.headerGroups) {
				mods.push(GroupHeader);
			}
			
			var autoHeightTriggerValue = 1;
			if (options.heightTrigger !== undefined) {
				autoHeightTriggerValue = options.heightTrigger;
			}
			
			var autoHeight = autoHeightTriggerValue < 1;
			if (sniff('ie')) {
				// autoHeight false with focus is broken in IE
				autoHeight = true;
			} 
			
			// set column widths per user preferences
			if (structure && !options.basic && !options.suppressSelection) {
				var defaults = ism.user.columnWidthDefaults[gridId] ? undefined : {};  // to save the original values
				for (var col=0, len=structure.length; col < len; col++) {
					var colId = structure[col].id;
					if (defaults) {
						var def = structure[col].width;
						if (def !== undefined) {
							var index = def.indexOf("px");
							def = index > 0 ? def.substring(0, index) : def;
						}
						defaults[colId] = {};
						defaults[colId].width = def;
					}
					structure[col].width = ism.user.getColumnWidth(gridId, colId, structure[col].width);
				}
				if (defaults) {
					ism.user.columnWidthDefaults[gridId] = defaults;
				}				
				
				if (structure.length > 4) {
					mods.push(HiddenColumns);
				}
			}
		
			domConstruct.place("<div id='"+gridId+"'></div>", parentId);
			var gridOptions = {
					id: gridId,
					cacheClass: Sync,
					store: store,
					autoHeight: autoHeight,
					autoWidth: true,
					structure: structure,
					modules: mods
				};
			if (options.headerGroups) {
				gridOptions.headerGroups = options.headerGroups;
				gridOptions.ismHeaderGroupMap = options.headerGroupMap;
				gridOptions.ismOriginalHeaderGroups = options.headerGroups;
			}
			gridOptions.ignoreColResize = false;
			var grid = new Grid(gridOptions, gridId);
			domClass.add(grid.domNode,"compact");
			if (grid.hiddenColumns) {
				var hiddenColumns = ism.user.getHiddenColumns(grid.id);
				for (var j = 0, numHCols=hiddenColumns.length; j < numHCols; j++) {	
					grid.hiddenColumns.add(hiddenColumns[j]);
				}
			}
			grid.startup();
			this._adjustHeaderGroupForHiddenColumns(grid, options.headerGroups, options.headerGroupMap);

			// make the grid have a fixed height for a small number of rows but expand to fit when there are more rows
			var height = options.height ? options.height : (options.headerGroups ? "231px" :"181px");
			if (vscroller) {
				grid.domNode.style.height = height;
			} else if (autoHeight === false) {
				grid.domNode.style.height = height;
				aspect.before(grid.model,"onSizeChange",function(size) {
					console.debug("grid size change: "+size);
					if (size < autoHeightTriggerValue) {
						grid.set("autoHeight",false);
						grid.domNode.style.height = height;
					} else {
						grid.set("autoHeight",true);
					}
				},true);
			}  else if (sniff('ie')) {
				grid.domNode.style.height = height;				
			}
			
			// after updating to Gridx 1.3, it doesn't seem to be 
			//     adjusting the vertical size automatically...
			aspect.after(grid.model,"onSizeChange",function(size) {
				console.debug("after grid size change: "+size);
				setTimeout(function(){
					grid.vLayout.reLayout();
				}, 10);
			},true);
			if (grid.pagination) {
				aspect.after(grid.pagination, "onChangePageSize", function() {
					setTimeout(function(){
						grid.vLayout.reLayout();
					}, 10);
				});
				aspect.after(grid.pagination, "onSwitchPage", function() {
					setTimeout(function(){
						grid.vLayout.reLayout();
					}, 10);
				});
			}
		
			// build view menu
			if (!options.basic && !options.suppressSelection) {
				// choose columns and reset columns
				if (grid.hiddenColumns) {
					this._addShowColumnsMenuItems(grid, structure);
				}
				
				// listen for resize events
				aspect.after(grid.columnResizer, "ismOnResize", function(gridId, colId, newWidth, oldWidth) {
					if (this.grid && this.grid.ignoreColResize !== true) {
						ism.user.setColumnWidth(gridId, colId, newWidth+"px");
					}
				}, true);
				// add reset column width action
				if (grid.toolBar && ism.user.columnWidthDefaults[gridId] && structure.length > 1) {
					var resetButton = new MenuItem({
						id: gridId+"_resetColWidthsItem",
						label: nls.action.ResetColWidth, 
						onClick: function() {
							// reset the grid column widths to the default ones
							var defaults = ism.user.columnWidthDefaults[grid.id];
							for (var colId in defaults) {
								grid.columnResizer.setWidth(colId, defaults[colId].width);
							}
						}
					});					
					this._addViewMenuItem(grid, resetButton);
				}
			}
			
			// Listen for window resize events to work around gridx defect 12379
			dojo.connect(window, 'onresize', function(){grid.resize(); console.log('in window resize');});
			
			// if sort is suppressed, set title attributes
			if (options.suppressSort) {
				query('.gridxCell', grid.header.domNode).forEach(function(node){
					var column = grid.column(node.getAttribute('colid'), 1);
					node.setAttribute('title', column.def().tooltip || column.name());
				});
			}

			return grid;
		},
		
		_addShowColumnsMenuItems: function(grid, structure) {
			var cols = structure !== undefined ? structure : [];
			// add choose columns menu item
			var columnsSubMenu = new Menu();
			var currentGroup = undefined;
			var checkedMenuItems = [];
			// the first column must be shown, so skip it
			for (var k = 1, numCols=cols.length; k < numCols; k++) {	
				var column = cols[k];						
				var menuItem = new CheckedMenuItem({
					id: grid.id+"_chooseColumnsItem_"+column.id,
					label: column.name, 
					colId: column.id,
					checked: ism.user.getColumnShown(grid.id, column.id, true),
					gridFactory: this,
					onChange: function(checked) {
						// save the choice
						ism.user.setColumnShown(grid.id, this.colId, checked);
						// hide or show the column
						if (checked) {
							grid.hiddenColumns.remove(this.colId);
						} else {
							grid.hiddenColumns.add(this.colId);
						}
						this.gridFactory._adjustHeaderGroupForHiddenColumns(grid, grid.ismOriginalHeaderGroups, grid.ismHeaderGroupMap);
					}
				});
				// add separator?
				if (grid.ismHeaderGroupMap) {
					var newGroup = grid.ismHeaderGroupMap[column.id];
					if (currentGroup !== undefined && currentGroup !== newGroup) {
						// add a separator
						columnsSubMenu.addChild(new MenuSeparator());
					}
					currentGroup = newGroup;
				} 
				checkedMenuItems.push(menuItem);
				columnsSubMenu.addChild(menuItem);
			}
			grid.ismCheckedMenuItems = checkedMenuItems;
			var chooseColumns = new PopupMenuItem({label: nls.action.ChooseColumns, popup: columnsSubMenu });
			this._addViewMenuItem(grid, chooseColumns);
			
			// add resetColumns menu item
			var resetColumns = new MenuItem({
				id: grid.id+"_resetColumnssItem",
				label: nls.action.ResetColumns, 
				gridFactory: this,
				onClick: function() {
					// reset the grid columns shown
					ism.user.showAllColumns(grid.id);
					grid.hiddenColumns.clear();
					this.gridFactory._adjustHeaderGroupForHiddenColumns(grid, grid.ismOriginalHeaderGroups, grid.ismHeaderGroupMap);
					// now reconcile the state of all the checkboxes
					for (var menuItemIndex in grid.ismCheckedMenuItems) {
						var checkedMenuItem = grid.ismCheckedMenuItems[menuItemIndex];
						checkedMenuItem.set("checked",true);
					}
				}
			});					
			this._addViewMenuItem(grid, resetColumns);			
		},
				
		_adjustHeaderGroupForHiddenColumns: function(grid, groups, map) {
			if (!grid || !groups || !map || !grid.hiddenColumns) {
				return;
			}

			var getIndexForGroup = function(headerGroups, groupId) {
				if (!headerGroups || !groupId) {
					return -1;
				}
				for (var index = 0, len=headerGroups.length; index < len; index++) {
					if (headerGroups[index].id === groupId) {
						return index;
					}
				}
				return -1;
			};
					
			var hiddenColumns = ism.user.getHiddenColumns(grid.id);
			var modifiedHeaderGroups = lang.clone(groups);
			
			// first need to handle any hidden columns not in a group
			var indexesToRemove = [];
			var groupId;
			var numHCols = hiddenColumns.length;
			for (var ci = 0; ci < numHCols; ci++) {	
				groupId = map[hiddenColumns[ci]];
				if (typeof groupId === "number") {
					groupIndex = groupId;
					if (groupIndex < modifiedHeaderGroups.length && typeof modifiedHeaderGroups[groupIndex] === "number") {
						modifiedHeaderGroups[groupIndex] = modifiedHeaderGroups[groupIndex] - 1; 
						if (modifiedHeaderGroups[groupIndex] < 1 && indexesToRemove.indexOf(groupIndex) > -1) {
							indexesToRemove.push(groupIndex);
						}					
					}
				}
			}
			indexesToRemove.sort(function(a, b){return b-a;}); // sort descending
			var numIndexes=indexesToRemove.length;
			for (ci = 0; ci < numIndexes; ci++) {
				modifiedHeaderGroups.splice(indexesToRemove[ci], 1);
			}
			
			// now handle the ones that are in a group
			for (ci = 0; ci < numHCols; ci++) {	
				// need to adjust the headerGroup structure
				groupId = map[hiddenColumns[ci]];
				var groupIndex = getIndexForGroup(modifiedHeaderGroups, groupId);
				if (groupIndex >= 0) {
					modifiedHeaderGroups[groupIndex].children--;
					if (modifiedHeaderGroups[groupIndex].children < 1) {
						modifiedHeaderGroups.splice(groupIndex, 1);
					}
				}
			}
			grid.ignoreColResize = true; // don't persist any column resizes that happen as a result of this
			grid.header.groups = modifiedHeaderGroups;
			grid.header.refresh();
			// On Chrome and Safari, columns are not resizing correctly
			// This workaround helps, but does not entirely solve the issue.
			// There is still a problem when the group header is wider than the
			// remaining columns. The gridx column width calculation is off.
			var defaultColWidths = ism.user.columnWidthDefaults[grid.id];
			defaultColWidths = defaultColWidths ? defaultColWidths : {};
			for (var colId in defaultColWidths) {
				var width = ism.user.getColumnWidth(grid.id, colId, defaultColWidths[colId].width);
				var index = width.indexOf("px");
				width = index > 0 ? width.substring(0, index) : width;
				grid.columnResizer.setWidth(colId, width);
			}
			grid.ignoreColResize = false;				
		},
		
		addToolbarButton: function(/* Grid */ grid, /* string */ item, /* boolean */ enabled) {
			console.debug("populateToolbar item="+item);
			var button = null;
			var addSeparator = false;
			var index = this._getDropDownMenuIndex(grid);
			if (index < 0) {
				index = undefined;
			} else {
				addSeparator = true;
				index = index === 0 ? 0 : index - 1;
			}

			if (item == this.ADD_BUTTON) {
				console.debug("add button");
				button = new Button({
					id: grid.id + "_add",
					label: nls.action.Add,
					showLabel: false,
					iconClass: 'ismIconAdd'
				});
			} else if (item == this.DELETE_BUTTON) {
				console.debug("delete button");
				button = new Button({
					id: grid.id + "_delete",
					label: nls.action.Delete,
					showLabel: false,
					iconClass: 'ismIconRemove'
				});
			} else if (item == this.EDIT_BUTTON) {
				console.debug("edit button");
				button = new Button({
					id: grid.id + "_edit",
					label: nls.action.Edit,
					showLabel: false,
					iconClass: 'ismIconEdit'
				});
			} else if (item == this.UP_BUTTON) {
				console.debug("up button");
				button = new Button({
					id: grid.id + "_up",
					label: nls.action.MoveUp,
					showLabel: false,
					iconClass: 'ismIconMoveUp'
				});
			} else if (item == this.DOWN_BUTTON) {
				console.debug("down button");
				button = new Button({
					id: grid.id + "_down",
					label: nls.action.MoveDown,
					showLabel: false,
					iconClass: 'ismIconMoveDown'
				});
			}
			
			if (button) {
				grid.toolBar.widget.addChild(button, index);
				button.set('disabled',!enabled);
				if (addSeparator) {
					this._addDropDownMenuSeparator(grid, null, button);
				}
				
				aspect.after(button, "onClick", function() {
					dojo.publish("lastAccess", {requestName: item+".button"});
				});
			}
			return button;
		},
		
		addToolbarMenuItem: function(/* Grid */ grid, /* MenuItem */ item, /* String */ label) {
			// create the drop down menu if this is the first menu item
			var addSeparator = false;
			if (!label) {
				label = nls.action.OtherActions;
				addSeparator = true;
			}
			
			var menuId = grid.id + "_" + "actionsMenu";
			var buttonId = menuId + "Button";
			
			var menu = dijit.byId(menuId);
			var index = undefined;
			if (!menu) {
				// is there already a view menu?
				var viewMenuButton = this._getViewDropDownButton(grid);
				var separator = undefined;
				if (addSeparator) {
					separator = this._addDropDownMenuSeparator(grid, viewMenuButton);
					index = grid.toolBar.widget.getIndexOfChild(separator) + 1;
				} else if (viewMenuButton) {
					index = grid.toolBar.widget.getIndexOfChild(viewMenuButton) - 1;
					index = index < 0 ? 0 : index; 
				}
				menu = new Menu({id: menuId});
				aspect.after(menu, "onOpen", function() {
					dojo.publish("lastAccess", {requestName: menuId+".menu"});
				});
				grid.toolBar.widget.addChild(new DropDownButton({
					id: buttonId,
					'class': 'idxButtonCompact',
					label: label,
					dropDown: menu
				}), index);				
			} 
			menu.addChild(item);
			aspect.after(item, "onClick", function() {
				dojo.publish("lastAccess", {requestName: buttonId+".button"});
			});
		},
		
		_getViewDropDownButton: function(/* Grid */ grid) {
			return dijit.byId(grid.id + "_" + "viewMenuButton");
		}, 
		
		_addViewMenuItem: function(/* Grid */ grid, /* MenuItem */ item) {
			var menuId = grid.id + "_" + "viewMenu";
			var buttonId = menuId + "Button";
			
			var menu = dijit.byId(menuId);
			if (!menu) {
				menu = new Menu({id: menuId});
				grid.toolBar.widget.addChild(new DropDownButton({
					id: buttonId,
					'class': 'idxButtonCompact',
					label: nls.action.View,
					dropDown: menu
				}));				
			} 
			menu.addChild(item);			
		},
		
		_addDropDownMenuSeparator: function(grid, before, after) {
			var separator = dijit.byId(grid.id + "_dropDownMenuSeparator");
			if (separator) {
				return separator;
			}
			var index = undefined;
			if (after) {
				index = grid.toolBar.widget.getIndexOfChild(after) + 1;
			} else if (before) {
				index = grid.toolBar.widget.getIndexOfChild(before);
				index = index < 2 ? 0 : index - 1;
			}
			separator = new ToolbarSeparator({id: grid.id + "_dropDownMenuSeparator"});
			grid.toolBar.widget.addChild(separator, index);
			return separator;
		},
		
		_getDropDownMenuIndex: function(grid) {
			var index = undefined;
			var separator = dijit.byId(grid.id + "_dropDownMenuSeparator");
			if (separator) {
				index = grid.toolBar.widget.getIndexOfChild(separator);
			} else {
				var menu = dijit.byId(grid.id + "_actionsMenuButton");
				if (!menu) {
					menu = dijit.byId(grid.id + "_viewMenuButton");
					if (!menu) {
						return -1;
					}
				}
				index = grid.toolBar.widget.getIndexOfChild(menu);
			}
			return index;
		}		
	};

});

