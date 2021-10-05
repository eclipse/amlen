/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.test.cli.util;

import java.util.ArrayList;


/**
 * CLICommandExecution executes the commands specified in the specified 
 * command properties file via SSH.  The result (exit code, stdout and stderr)
 * of the commands will be written to the stdout.
 * 
 * The expected format of the input properties file is as follows:
 *   CLICommandCount=n
 *   CLICommand.1.Input.Command=
 *   CLICommand.1.Input.Option.Count=m
 *   CLICommand.1.Input.Option.1.Key=
 *   CLICommand.1.Input.Option.1.Value=
 *   ...................Option.m.Key=
 *   ...................Option.m.Value=
 *   .....
 *   CLICommand.n.Input.Command=
 *   CLICommand.n.Input.Option.Count=k
 *   CLICommand.n.Input.Option.1.Key=
 *   CLICommand.n.Input.Option.1.Value=
 *   ...................Option.k.Key=
 *   ...................Option.k.Value=
 *   
 *   Example 1:
 *     Execute 2 commands:
 *       "imaserver stop" and "imaserver start"
 *   
 *   CLICommandCount=2
 *   CLICommand.1.Input.Command=imaserver stop
 *   CLICommand.1.Input.Option.Count=0
 *   CLICommand.2.Input.Command=imaserver start
 *   CLICommand.2.Input.Option.Count=0
 *
 *   Example 2:
 *     Execute 1 single command:
 *       imaserver create MessageHub "Name=testhub" "Description=Test Hub"
 *   CLICommandCount=1
 *   CLICommand.1.Input.Command=imaserver create MessageHub
 *   CLICommand.1.Input.Option.Count=2
 *   CLICommand.1.Input.Option.1.Key=Name
 *   CLICommand.1.Input.Option.1.Value=testhub
 *   CLICommand.1.Input.Option.2.Key=Description
 *   CLICommand.1.Input.Option.2.Value=Test Hub
 *
 * The format of the output is as follows:
 *   CLICommandCount=n
 *   CLICommand.1.ExecutedCommand=
 *   CLICommand.1.Output.ExitCode=
 *   CLICommand.1.Output.Stdout.Count=m
 *   CLICommand.1.Output.Stdout.Line.1=
 *   ...........1.Output.Stdout.Line.m=
 *   CLICommand.1.Output.Stderr.Count=k
 *   CLICommand.1.Output.Stderr.Line.1=
 *   ...........1.Output.Stderr.Line.k=
 *   ......
 *   CLICommand.n.ExecutedCommand=
 *   CLICommand.n.Output.ExitCode=
 *   CLICommand.n.Output.Stdout.Count=i
 *   CLICommand.n.Output.Stdout.Line.1=
 *   ...........n.Output.Stdout.Line.i=
 *   CLICommand.n.Output.Stdout.Count=j
 *   CLICommand.n.Output.Stdout.Line.1=
 *   ...........n.Output.Stdout.Line.j=
 *   
 *   Example output of 
 *      imaserver create MessageHub "Name=testhub" "Description=Test Hub"
 *   
 *   CLICommandCount=1
 *   CLICommand.1.ExecutedCommand=imaserver create MessageHub "Name=testhub" "Description=Test Hub"
 *   CLICommand.1.Output.ExitCode=0
 *   CLICommand.1.Output.Stdout.Count=1
 *   CLICommand.1.Output.Stdout.Line.1=The requested configuration change has completed successfully.
 *   CLICommand.1.Output.Stderr.Count=0
 *
 */
public class CLICommandExecution {

	/**
	 * @param args The following arguments are expected:
	 *   HOST=<IP address or hostname>
	 *   USER=<user to login the server>
	 *   PASSWORD=<user password>
	 *   INPUT_PROPERTIES_FQFN=<fully qualified file name of the input command properties file>
	 * 
	 * Exit code 0 for success execution
	 *   Note the caller must examine output for exit code of individual command.
	 * Exit code 1 for failure
	 */
	public static void main(String[] args) {
		CLICommandExecution test = new CLICommandExecution();
		System.exit(test.runTest(new CLICommandTest(args)) ? 0 : 1);
	}
	
	public boolean runTest(CLICommandTest test) {
		boolean result = false;

		try {
			test.buildInputCommands();
			ArrayList<CLICommand> list = test.exececuteCommands();
			
			//TODO: move this block of code into CLICommandTest class
			System.out.println("CLICommandCount=" + list.size());
			int i = 1;
			for (CLICommand cmd : list) {
				System.out.println("CLICommand." + i + ".ExecutedCommand=" + cmd.getCommand());
				System.out.println("CLICommand." + i + ".Output.ExitCode=" + cmd.getRetCode());
				int sizeStdoutList = 0;
				int sizeStderrList = 0;
				if (cmd.getStdoutList() != null) {
					sizeStdoutList = cmd.getStdoutList().size();
				}
				if (cmd.getStderrList() != null) {
					sizeStderrList = cmd.getStderrList().size();
				}
				
				System.out.println("CLICommand." + i + ".Output.Stdout.Count="+ sizeStdoutList);
				int j = 1;
				if (sizeStdoutList > 0) {
					for (String line : cmd.getStdoutList()) {
						System.out.println("CLICommand." + i + ".Output.Stdout.Line." + j + "=" + line);
						j++;
					}
				}
				
				System.out.println("CLICommand." + i + ".Output.Stderr.Count="+ sizeStderrList);
				int k = 0;
				if (sizeStderrList > 0) {
					for (String line : cmd.getStderrList()) {
						System.out.println("CLICommand." + i + ".Output.Stderr.Line." + j + "=" + line);
						k++;
					}
				}
				
				i++; // command count
			}
			result = true;
		} catch (Exception e) {
			e.printStackTrace();
		}
		return result;
	}

}
