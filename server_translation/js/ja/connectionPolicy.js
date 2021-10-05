/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
        resourcesSection: "MQTT クライアントでのコンシュームが許可されるリソースを指定します。",
        allowDurable: "クライアントに永続サブスクリプションを許可",
        allowDurableHelp: "チェック・マークを付けた場合、MQTT クライアントは CleanSession 0 に接続できます。",
        allowPersistentMessages: "永続メッセージを許可",
        allowPersistentMessagesHelp: "チェック・マークを付けた場合、MQTT クライアントは 1 または 2 の QoS でメッセージをパブリッシュできます。",
        // Strings for Expected Message Rate
        expectedMessageRateTitle: "予想メッセージ速度",
        expectedMessageRateHoverHelp: "予想メッセージ速度によって、各接続で使用できるメモリーと " +
            "帯域幅の量が決定されます。  高いメッセージ速度が予想される場合には、 " +
            "予想メッセージ速度を高または最大に設定することで、パフォーマンスを改善できます。  反対に " +
            "低いメッセージ速度が予想され、リソース使用率を低く維持する必要がある場合には、予想メッセージ速度を低に設定できます。",
        lowRate: "低",
        defaultRate: "デフォルト",
        highRate: "高",
        maxRate: "最大"
});
