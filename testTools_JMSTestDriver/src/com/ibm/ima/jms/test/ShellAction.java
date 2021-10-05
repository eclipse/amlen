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

import java.io.IOException;
import java.util.Scanner;

import javax.jms.JMSException;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

/**
 *
 * expectedRC "x" = ignore return code
 * expectedRC "-" = non-zero return code
 * expectedRC "0" = command must pass
 * expectedRC "#" = command must fail with specific error code #
 */
public class ShellAction extends ApiCallAction {

    private String  _command;
    private Integer _expected_rc;
    private Boolean _print_result;
      
    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public ShellAction(Element config, DataRepository dataRepository,
            TrWriter trWriter) {
        super(config, dataRepository, trWriter);

        config.setAttribute("action_api", "ShellAction");

        _command = _actionParams.getProperty("command");
        _expected_rc = Integer.parseInt(_actionParams.getProperty("expected_rc", "0"));
        _print_result = Boolean.parseBoolean(_actionParams.getProperty("print_result", "false"));
        
    }
    
    /* (non-Javadoc)
     * @see com.ibm.mds.rm.test.Action#run()
     */
    @Override    
    protected boolean invokeApi() throws JMSException {
        boolean rc;
        
        try {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2783", "Action {0}: Invoking shell command: `{1}`", _id, _command);
            runCommand(_command, _print_result);
       
            rc = true; 
        }catch (JMSException jmse) {
        	throw jmse;
        }catch (Throwable e) {       
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2100", "Action {0} failed. The reason is: {1}", _id, e.getMessage());
            _trWriter.writeException(e);
            rc = false;
        }
        return rc;
    }

    private void runCommand(String command, Boolean printResult) throws JMSException {
        try {
        	
            /* Commands with environment variables should look like this:
             * " echo My favorite colors are ``COLOR1`` and ``COLOR2`` "
             * (The environment variable is surrounded with 2 backticks (`) on either side.
             * */
            
            try {
                command = EnvVars.replace(command);
            } catch (Exception e) {
                throw new JMSException(e.getMessage(), e.getMessage());
            }
      
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2783", "ShellAction new command: {0}", command);	

            int returned_rc = -1;
            
            Process pr = null;
            String output = "";
            
            try {
            	pr = Runtime.getRuntime().exec(command);
            } catch(IOException ioexe) {
            	returned_rc = 127;
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "exec failed: {0}", ioexe.getMessage());
            }
            
            if(pr != null) {
            	pr.waitFor();
            	returned_rc = pr.exitValue();
            	StringBuilder sb = new StringBuilder();
            	Scanner stdout_buf = new Scanner(pr.getInputStream());
            	Scanner stderr_buf = new Scanner(pr.getErrorStream());
            	while(stdout_buf.hasNext())
            		sb.append(stdout_buf.nextLine()).append("\n");
            	sb.append("\n");
            	while(stderr_buf.hasNext())
            		sb.append(stderr_buf.nextLine()).append("\n");
            	output = sb.toString(); 
            	stdout_buf.close();
                stderr_buf.close();
            }	
            
            if (_print_result)
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2783", "Action {0}: result from shell: \n{1}", _id, output);
                        
            if (_expected_rc.equals("-")) {
            	if (_expected_rc.intValue() == 0) {
            		_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "RC: {1}, non-zero return code expected.", returned_rc);
            		throw new JMSException("UnexpectedRC", "UnexpectedRC");
            	} else {
            		return;
            	}
            } else if (_expected_rc.equals("x")) {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "RC: {1}, ignoring return code.", returned_rc);            	
            	return;
            } else if(_expected_rc.intValue() != returned_rc) {
            	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Expected RC: {0}, Actual RC: {1}", _expected_rc, returned_rc);
            	throw new JMSException("UnexpectedRC", "UnexpectedRC");
            }
         
            
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

}
