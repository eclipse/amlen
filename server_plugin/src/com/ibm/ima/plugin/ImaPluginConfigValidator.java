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
package com.ibm.ima.plugin;

/**
 * A plug-in custom properties validator.
 * If plug-in implements this interface the validate method will be called
 * every time plug-in is installed or plug-in configuration files
 * (pluginproperties.json and/or plugin.json) are updated.
 * 
 */
public interface ImaPluginConfigValidator {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	/** Method to validate custom plug-in configuration 
     * @param plugin  The plug-in object which contains methods which the plug-in can call to get the configuration. 
	 * @throws ImaPluginException thrown if configuration is not valid
	 */
	public void validate(ImaPlugin plugin) throws ImaPluginException; 
}
