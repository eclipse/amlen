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
package com.ibm.ima.sample.test.aj;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Iterator;

public class NetworkProxy implements Runnable
{

	private ServerSocket localSocket;
	private final int localPort;
	private final int remotePort;
	private final InetAddress remoteAddress;
	private final ArrayList<Connection> connections;
	private boolean running;
	boolean firstRun = true;

	/**
	 * @param localPort
	 *            int - The port used by the MQXR channel
	 * @param remoteAddress
	 *            InetAddress - The address of the proxy
	 * @param remotePort
	 *            int - The port the proxy should use
	 * @param commandMessage
	 *            int - The value of the first byte of the command message you
	 *            wish to block
	 */
	public NetworkProxy(int localPort, InetAddress remoteAddress, int remotePort)
	{
		this.localPort = localPort;
		this.remoteAddress = remoteAddress;
		this.remotePort = remotePort;
		connections = new ArrayList<Connection>();
	}

	public static void main(String[] args)
	{
		try
		{
			NetworkProxy network_proxy = new NetworkProxy(1987, InetAddress.getByName("9.20.123.102"), 16102);
			network_proxy.start();
			System.out.println("Network proxy Started");
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public void start() throws IOException
	{
		localSocket = new ServerSocket(localPort);
		localSocket.setReuseAddress(true);
		running = true;
		System.out.println("Local socket created and is now running.");
		new Thread(this).start();
	}

	@SuppressWarnings("unchecked")
	public void closeConnections()
	{
		synchronized (connections)
		{
			// clone to prevent shutdown calling back in and modifying the list
			ArrayList<Connection> connCopy = (ArrayList<Connection>) connections
					.clone();
			Iterator<Connection> it = connCopy.iterator();
			while (it.hasNext())
			{
				Connection c = (Connection) it.next();
				c.shutdown();
			}
		}
	}

	public void shutdown()
	{
		if (running == false)
		{
			return;
		}
		running = false;
		try
		{
			localSocket.close();
		}
		catch (IOException ioe)
		{
		}
		closeConnections();
	}

	public int getConnectionCount()
	{
		return connections.size();
	}

	public Iterator<Connection> connections()
	{
		return connections.iterator();
	}

	public void run()
	{
		System.out.println("In Run!");
		while (running)
		{
			Socket inbound, outbound;
			try
			{
				inbound = localSocket.accept();
				System.out.println("Accepted Listener!");
			}
			catch (Exception e)
			{
				shutdown();
				continue;
			}
			try
			{
				outbound = new Socket(remoteAddress, remotePort);
			}
			catch (Exception e)
			{
				try
				{
					inbound.close();
				}
				catch (Exception e2)
				{
				}
				continue;
			}
			try
			{
				Connection conn = null;
				synchronized (connections)
				{
					if (running)
					{
						conn = new Connection(this, inbound, outbound);
						connections.add(conn);
						conn.start();
					}
				}
			}
			catch (Exception e)
			{
				try
				{
					inbound.close();
				}
				catch (Exception e2)
				{
				}
				try
				{
					outbound.close();
				}
				catch (Exception e2)
				{
				}
				shutdown();
				continue;
			}
		}
	}

	public void shutdownConnection(Connection connection)
	{
		synchronized (connections)
		{

			try
			{
				connection.shutdown();
			}
			catch (Exception e)
			{

			}
			connections.remove(connection);
		}
	}

	class Connection
	{

		private boolean running;
		private final NetworkProxy parent;
		private final Socket localSocket;
		private final Socket remoteSocket;
		private final InputStream fromLocal, fromRemote;
		private final OutputStream toLocal, toRemote;
		private final Pipe localToRemotePipe;
		private final Pipe remoteToLocalPipe;
		String name;

		public Socket getLocalSocket()
		{
			return localSocket;
		}

		public Connection(NetworkProxy parent, Socket localSocket,
				Socket remoteSocket) throws IOException
		{
			this.parent = parent;
			name = localSocket.getInetAddress() + ":" + localSocket.getPort();
			this.localSocket = localSocket;
			this.remoteSocket = remoteSocket;
			fromLocal = localSocket.getInputStream();
			toLocal = localSocket.getOutputStream();
			fromRemote = remoteSocket.getInputStream();
			toRemote = remoteSocket.getOutputStream();
			System.out.println("Creating connections!");
			localToRemotePipe = new Pipe(this, fromLocal, toRemote, "C -> S");
			remoteToLocalPipe = new Pipe(this, fromRemote, toLocal, "C <- S");
		}

		public void start()
		{
			running = true;
			System.out.println("Starting the pipes!");
			localToRemotePipe.start();
			remoteToLocalPipe.start();
		}

		public synchronized void shutdown()
		{
			if (running)
			{
				running = false;
				localToRemotePipe.shutdown();
				remoteToLocalPipe.shutdown();
				try
				{
					localSocket.close();
				}
				catch (Exception e)
				{
				}
				try
				{
					remoteSocket.close();
				}
				catch (Exception e)
				{
				}

				parent.shutdownConnection(this);
			}
		}
	}

	class Pipe extends Thread
	{

		private final InputStream in;
		private final OutputStream out;
		private boolean running = true;
		private final Connection parent;

		public Pipe(Connection parent, InputStream in, OutputStream out,
				String direction)
		{
			this.parent = parent;
			this.in = in;
			this.out = out;
			buffer = new ByteArrayOutputStream();
		}

		public void shutdown()
		{
			if (running)
			{
				running = false;
				try
				{
					in.close();
				}
				catch (Exception e)
				{
				}
				try
				{
					out.close();
				}
				catch (Exception e)
				{
				}
				parent.shutdown();
			}
		}

		@Override
		public void run()
		{
			while (running)
			{
				try
				{
					int b = in.read();
					if (b != -1)
					{
						byteRead(b);
						out.write(b);
					}
					else
					{
						shutdown();
					}
				}
				catch (Exception e)
				{
					shutdown();
				}
			}
		}

		private static final byte READY = 0;
		private static final byte IN_REMAINING_LENGTH = 1;
		private static final byte IN_PAYLOAD = 2;

		private ByteArrayOutputStream buffer;
		private byte state = 0;
		private byte remainingLength;
		private int multiplier;

		public void byteRead(int v)
		{
			buffer.write(v);
			if (state == READY)
			{
				multiplier = 1;
				remainingLength = 0;
				state = IN_REMAINING_LENGTH;
			}
			else
				if (state == IN_REMAINING_LENGTH)
				{
					remainingLength += ((v & 0x7F) * multiplier);
					multiplier *= 128;
					if ((v & 0x80) == 0)
					{
						state = IN_PAYLOAD;
					}
				}
				else
					remainingLength--;
			if (state == IN_PAYLOAD && remainingLength == 0)
			{
				try
				{
					buffer.close();
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
				buffer = new ByteArrayOutputStream();
				state = READY;
			}
		}
	}
}
