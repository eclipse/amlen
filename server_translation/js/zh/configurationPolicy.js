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
	    title:  "配置策略",
	    tagline: "配置策略用于控制允许用户执行的管理任务。" +
	    		"必须将配置策略添加到管理端点中以使其生效。",
	    // Add / Edit dialog
	    addDialogTitle: "添加配置策略",
	    editDialogTitle: "编辑配置策略",
	    dialogInstruction: "配置策略用于控制允许用户执行的管理任务。",
		nameLabel:  "名称：",
		nameColumnLabel:  "姓名",
		descriptionLabel: "描述：",
		descriptionColumnLabel: "描述",
		authorityLabel: "权限：",
		authorityColumnLabel: "权限",
		authorityTooltip: "<dl><dt>配置：</dt><dd>授予修改服务器配置的权限。</dd>" +
		                      "<dt>查看：</dt><dd>授予查看服务器配置和状态的权限。</dd>" +
                              "<dt>监视：</dt><dd>授予查看监视数据的权限。</dd>" +
                              "<dt>管理：</dt><dd>授予发出服务请求（例如，重新启动服务器）的权限。</dd>" +
                          "</dl>",
		configure: "配置",
		view: "查看",
		monitor: "监视",
		manage: "管理",
		listSeparator: ",",

	    dialogFilterInstruction: "要将此策略中定义的管理或监视操作限制为特定的用户或用户组，请指定以下一个或多个过滤条件。" +
            "例如，选择“组标识”以将此策略限制为特定组的成员。" +
            "该策略只允许在所有指定的过滤条件均为 true 时访问。",
        dialogFilterTooltip: "必须至少指定一个过滤条件字段。" +
            "您可以在大多数过滤条件中使用单个星号 (*) 作为最后一个字符，以匹配 0 个或更多个字符。" +
            "客户机 IP 地址可包含星号 (*) 或者 IPv4 或 IPv6 地址或范围的逗号分隔列表，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
            "IPv6 地址必须括在方括号中，例如：[2001:db8::]。",

	    clientIPLabel: "客户机 IP 地址：",
	    clientIPColumnLabel: "客户机 IP 地址",
	    userIdLabel: "用户标识：",
	    userIdColumnLabel: "用户标识",
	    groupIdLabel: "组标识：",
	    groupIdColumnLabel: "组标识",
	    commonNameLabel: "证书公共名称：",
	    commonNameColumnLabel: "证书公共名称",
	    invalidWildcard: "您可以使用单个星号 (*) 作为最后一个字符，以匹配 0 个或更多个字符。",
	    invalidClientIP: "客户机 IP 地址必须包含星号 (*) 或者 IPv4 或 IPv6 地址或范围的逗号分隔列表，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
            "IPv6 地址必须括在方括号中，例如：[2001:db8::]。",
        invalidFilter: "必须至少指定一个过滤条件字段。",
	    // buttons
	    saveButton: "保存",
	    cancelButton: "取消",
	    closeButton: "关闭",
        // messages
        savingProgress: "正在保存...",
        updatingMessage: "正在更新...",
        deletingProgress: "正在删除...",
        noItemsGridMessage: "没有要显示的项目",
        getConfigPolicyError: "检索配置策略时发生错误。",
        saveConfigPolicyError: "保存配置策略时发生错误。",
        deleteConfigurationPolicyError: "删除配置策略时发生错误。",
        // remove dialog
        removeDialogTitle: "删除配置策略",
        removeDialogContent: "确定要删除此配置策略吗？"
});
