/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

// Reviewed by Dev and ID
// Any exceptions are flagged with // Needs review until review is complete

define({
	    configurationTitle: "管理端点",
	    editLink: "编辑",
	    configurationTagline_unsecured: "管理端点未受保护。" +
	            "管理端点允许您通过使用 Web UI 或 REST API 来管理服务器。" +
	    		"要保护该端点，请配置安全概要文件和一项或多项配置策略。",
	    configurationTagline: "管理端点允许您通过使用 Web UI 或 REST API 来管理服务器。" +
	    		"验证是否有相应的安全概要文件和配置策略可用。",
	    editDialogTitle: "编辑管理端点",
	    editDialogInstruction: "Web UI 和 REST API 可连接到管理端点，以执行管理任务和监视任务。",
	    // property labels and hover helps
	    interfaceLabel:  "IP 地址：",
	    interfaceTooltip: "管理端点侦听的 IP 地址。",
	    portLabel: "端口：",
	    portTooltip: "管理端点侦听的端口。端口号必须在范围 1 到 65535（含）内。",
	    securityProfileLabel: "安全概要文件：",
	    securityProfileTooltip: "要保护连接并定义执行认证的方式，请选择安全概要文件。" +
	    		"现有安全概要文件包含在列表中。" +
	    		"要创建新的安全概要文件，请转至<em>安全设置</em>页面。",
	    configurationPoliciesLabel: "配置策略：",
	    configurationPoliciesTooltip: "要保护管理端点，请添加至少一项（不超过 100 项）配置策略。" +
	    		"配置策略授权已连接的客户机执行特定的管理任务和监视任务。" +
	    		"配置策略按显示的顺序进行评估。要更改顺序，请使用工具栏上的向上和向下箭头。",
	    // configuration policy chooser dialog
	    addConfigPolicyDialogTitle: "将配置策略添加到管理端点中",
	    addConfigPoliciesDialogInstruction: "配置策略授权已连接的客户机执行特定的管理任务和监视任务。",
	    addLabel: "添加",
	    configurationPolicyLabel: "配置策略",
	    descriptionLabel: "描述",	
	    authorityLabel: "权限",
	    none: "无",
	    all: "全部",
	    // buttons
	    saveButton: "保存",
	    cancelButton: "取消",
	    add: "添加",
        // messages
        savingProgress: "正在保存...",
        getAdminEndpointError: "检索管理端点配置时发生错误。",
        saveAdminEndpointError: "保存管理端点配置时发生错误。",
        getSecurityProfilesError: "检索安全概要文件时发生错误。",
        tooManyPolicies: "选中的策略数过多。管理端点最多能包含 100 项配置策略。",
        certificateInvalidMessage: "Web UI 无法连接到 ${IMA_PRODUCTNAME_SHORT} 服务器。" +
            "服务器可能已停止，或者保护管理端点的证书可能无效。" +
            "如果证书无效，您必须添加一个例外。" +
            "<a href='{0}' target='_blank'>单击此处</a>以浏览至将提示您添加例外（如果需要）的页面。",
        certificateRefreshInstruction: "添加例外之后，您可能需要刷新此页面。",
        certificatePreventError: "请参阅 IBM Knowledge Center 中的 ${IMA_PRODUCTNAME_SHORT} 文档，以获取有关防止发生此问题的信息。",
        // AdminUserID and AdminUserPassword
        superUserTitle: "管理超级用户",
        superUserTagline: "管理超级用户标识和密码存储在本地，这可确保即使 LDAP 配置错误或不可用，仍可以管理 ${IMA_PRODUCTNAME_SHORT} 服务器。" +
        		"管理超级用户有权执行所有操作。" +
        		"请确保管理超级用户标识和密码难以猜测。",

        adminUserIDLabel: "用户标识：",
        changeAdminUserIDLink: "更改用户标识",
        editAdminUserIdDialogTitle: "编辑管理超级用户标识",
        editAdminUserIdDialogInstruction: "将管理超级用户标识更改为难以猜测的值。",
        saveAdminUserIDError: "保存管理超级用户标识时发生错误。",
        getAdminUserIDError: "检索管理超级用户标识时发生错误。",

        changeAdminUserPasswordLink: "更改密码",
        editAdminUserPasswordDialogTitle: "编辑管理超级用户密码",
        editAdminUserPasswordDialogInstruction: "将管理超级用户密码更改为难以猜测的值。",
        adminUserPasswordLabel: "密码",
        confirmPasswordLabel: "确认密码",
        passwordInvalidMessage : "密码不匹配。",
        saveAdminUserPasswordError: "保存管理超级用户密码时发生错误。"            
});
