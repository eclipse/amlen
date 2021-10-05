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
    	serverMenuLabel: "服务器：",
    	serverMenuHint: "选择要管理的服务器",
    	recentServersLabel: "最近服务器：",
    	addRemoveLinkLabel: "添加或移除服务器",
    	addRemoveDialogTitle: "添加或移除服务器",
    	addRemoveDialogInstruction: "添加或移除可使用此 Web UI 管理的 ${IMA_PRODUCTNAME_SHORT} 服务器。",
    	serverName: "服务器名称",
    	serverNameTooltip: "此服务器的显示名称。",
        adminAddress: "管理地址：",
        adminAddressTooltip: "服务器的管理端点正在侦听的 IP 地址或主机名。",
        adminPort: "管理端口：",
        adminPortTooltip: "服务器的管理端点正在侦听的端口。端口号必须在范围 1 到 65535（含）内。",
        sendWebUICredentials: "发送 Web UI 凭证：",
        sendWebUICredentialsTooltip: "Web UI 是否应将您用于登录的用户标识和密码发送至管理端点。" +
        		"如果管理端点需要有效用户标识和密码，Web UI 会将您用于登录的用户标识和密码发送至管理端点。" +
        		"否则，将提示您输入访问管理端点所需的用户标识和密码。",
        //cluster: "Cluster",
        description: "描述：",
        portInvalidMessage: "端口号必须在范围 1 到 65535（含）内。",
        duplicateServerTitle: "重复的服务器",
        duplicateServerMessage: "由于列表中已存在具有指定地址和端口的服务器，因此无法添加此服务器。",
        closeButton: "关闭",
        saveButton: "保存",
        cancelButton: "取消",
        testButton: "测试连接",
        YesButton: "是",
        deletingProgress: "正在删除...",
        removeConfirmationTitle: "确定要从受管服务器列表中移除此服务器吗？",
        addServerDialogTitle: "添加服务器",
        editDialogTitle: "编辑服务器",
        removeServerDialogTitle: "移除服务器",
        addServerDialogInstruction: "添加可使用此 Web UI 管理的 ${IMA_PRODUCTNAME_SHORT} 服务器。" /*+
        		"When you add a server that is in a cluster, other servers in the same cluster will appear in your list of servers."*/,
        testingProgress: "正在测试...",
        testingFailed: "测试连接失败。",
        testingSuccess: "测试连接成功。"        
});
