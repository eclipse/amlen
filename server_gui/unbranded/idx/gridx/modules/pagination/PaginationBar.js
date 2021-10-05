/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
define([
	"dojo/_base/declare",
	"dojo/query",
	"dijit/registry",
	"gridx/modules/pagination/PaginationBar",
	"gridx/support/LinkPager",
	"gridx/support/LinkSizer",
	"gridx/support/GotoPageButton",
	"./GotoPagePane",
	"idx/widget/Dialog",
	"dijit/form/Button",
	"idx/form/NumberTextBox"
], function(declare, query, registry, PaginationBar,
	LinkPager, LinkSizer, GotoPageButton, GotoPagePane,
	Dialog, Button, NumberTextBox){

	return declare(PaginationBar, {
		_init: function(pos){
			var t = this,
				gotoBtnProt = GotoPageButton.prototype;
			t._add(LinkPager, 1, pos, 'stepper', {
				className: 'gridxPagerStepperTD',
				visibleSteppers: t.arg('visibleSteppers')
			});
			t._add(LinkSizer, 2, pos, 'sizeSwitch', {
				className: 'gridxPagerSizeSwitchTD',
				sizes: t.arg('sizes'),
				sizeSeparator: t.arg('sizeSeparator')
			});
			t._add(GotoPageButton, 3, pos, 'gotoButton', {
				className: 'gridxPagerGoto',
				dialogClass: Dialog,
				buttonClass: Button,
				numberTextBoxClass: NumberTextBox,
				gotoPagePane: GotoPagePane,
				dialogProps: {
					'class': 'gridxGotoPageDialog',
					executeOnEnter: false,
					buttons: [
						new Button({
							label: t.grid.nls.gotoDialogOKBtn,
							onClick: function(){
								query('.gridxPagerGotoBtn', t.grid.domNode).some(function(node){
									var gotoBtn = registry.byNode(node);
									var dlg = gotoBtn._gotoDialog;
									if(dlg.open){
										dlg.content._onOK();
										return true;
									}
								});
							}
						})
					]
				}
			});
		}
	});
});
