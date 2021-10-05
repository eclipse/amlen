// Copyright (c) 2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package svt.home.test;

public class ScratchTests {

	public ScratchTests() {
		// TODO Auto-generated constructor stub
	}

	public static void main(String[] args) {
		svt.home.test.ScratchTests st = new svt.home.test.ScratchTests();
		st.run();

	}
	public void run() {
		System.out.println("Int Arg key port value " + "6003");
		Integer dave = Integer.parseInt("6003");
		System.out.println("parsed arg " + dave);
	}

}
 
