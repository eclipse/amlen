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
/**
 *
 */

package com.ibm.ima.jms.test;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

import java.io.PrintWriter;
import java.util.Scanner;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;

/**
 */

public class HttpAction extends Action{
    private String _protocol;
    private String _url; // contain port
    private String _url_parameter;
    private final String charset = "UTF-8";
    private String _query;
    private String _output;

    public HttpAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        config.setAttribute("action_api", "HttpAction");
        _protocol = _actionParams.getProperty("protocol", "GET");
        _url = _actionParams.getProperty("url");
        _url_parameter = _actionParams.getProperty("parameter", "");
        _output = _actionParams.getProperty("output");

        if (!_protocol.equalsIgnoreCase("GET")) {
            throw new UnsupportedOperationException("protocol " + _protocol + " is not (yet?) supported");
        }
    }

    @Override
    protected boolean run() {
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "in invokeApi of HttpAction");

        try {
            _query = String.format("?param=%s", URLEncoder.encode(_url_parameter, charset));
            _url = EnvVars.replace(_url);

            String u = _url;
            if(!_url.startsWith("http://") && !_url.startsWith("https://"))
                u = "http://" + _url;
            if(_url_parameter != "")
                u += _query;

            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "new url: " + u);
            HttpURLConnection con = (HttpURLConnection) new URL(u).openConnection();
            con.setRequestProperty("Accept-Charset", charset);
            Scanner response = new Scanner(con.getInputStream());
            int status = con.getResponseCode();
            if (status >= 400) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "HttpRequest received bad status code: " + status);
                response.close();
                throw new Exception("Bad Http Status code: " + status);

            }
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "The http status code is: " + status);
            StringBuilder sb = new StringBuilder();
            while(response.hasNextLine()) {
                sb.append(response.nextLine()+"\n");
            }

            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "response content: " + sb.toString());

            if(_output != null && !_output.equals("")) {
                PrintWriter log = new PrintWriter(_output, charset);
                log.write(sb.toString());
                log.close();
            }
            
            response.close();
        } catch (Exception exc) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Exception thrown in HttpAction: " + exc.getMessage());
            exc.printStackTrace();
            return false;
        }
        return true;
    }

}
